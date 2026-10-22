"""Conservative evidence gates for an epoch-based companion control lease.

Rows are telemetry, never commands. Synthetic scene gates prove the lease state
machine only; they cannot prove an actual conversation or cinematic was tested.
"""
import math


def _number(value):
    return type(value) in (int, float) and math.isfinite(value) and value >= 0


def _epoch(value):
    return type(value) is int and value > 0


def session_evidence(host_rows, guest_rows):
    errors, leases, scene_releases, disconnect_releases = [], {}, [], []
    active, latest_epoch, gate, gate_sequence = None, 0, None, 0
    disconnected, host_clock = {}, None
    story_ok = True

    def error(message):
        errors.append(message)

    for index, row in enumerate(host_rows):
        kind = row.get("session")
        if kind not in {"acquired", "sample", "released", "blocked", "gate-observation", "disconnect-observation"}:
            continue
        clock = row.get("monotonic_ms")
        if clock is not None:
            if not _number(clock) or (host_clock is not None and clock < host_clock):
                error("host monotonic_ms invalid or backwards")
            else:
                host_clock = clock
        if kind == "gate-observation":
            gate_sequence += 1
            if type(row.get("blocked")) is not bool or row.get("source") not in ("engine", "synthetic") or type(row.get("mode")) is not int:
                error("invalid gate observation")
                continue
            gate = dict(row, sequence=gate_sequence)
            continue
        if kind == "blocked":
            if not gate or gate["blocked"] is not True or row.get("reason") != gate.get("reason", "scene"):
                error("blocked claim lacks a matching observed gate")
            continue
        epoch = row.get("epoch")
        if not _epoch(epoch):
            error("invalid epoch")
            continue
        if kind == "disconnect-observation":
            if row.get("connected") is not False or row.get("source") not in ("engine", "synthetic") or active != epoch:
                error("invalid disconnect observation")
            else:
                disconnected[epoch] = row
            continue
        if row.get("host_story_preserved") is not True:
            story_ok = False
            error("host story invariant changed or missing")
        if kind == "acquired":
            if active is not None or epoch <= latest_epoch or epoch in leases:
                error("duplicate, stale, or overlapping acquisition")
                continue
            if gate and gate["blocked"]:
                error("acquisition while scene gate is blocked")
            if row.get("remote_owns_ashley") is not True:
                error("acquisition lacks reciprocal remote ownership")
            leases[epoch] = {"samples": [], "start": clock, "end": None,
                             "acquire_index": index, "gate_sequence": gate_sequence}
            latest_epoch, active = epoch, epoch
            continue
        if active != epoch or epoch not in leases:
            error("sample/release without an active matching acquisition")
            continue
        lease = leases[epoch]
        if kind == "sample":
            held = row.get("held_ms")
            world = row.get("world_time")
            samples = lease["samples"]
            if (not _number(held) or (samples and held < samples[-1]["held_ms"]) or
                    (world is not None and (not _number(world) or
                     any(s.get("world_time") is not None and world < s["world_time"] for s in samples)))):
                error("invalid or backwards lease time")
                continue
            if row.get("remote_owns_ashley") is not True or type(row.get("mode")) is not int:
                error("sample lacks remote ownership or mode")
            samples.append(row)
            continue
        if row.get("reason") not in {"scene", "disconnect", "disabled", "ownership-lost", "test-stop"}:
            error("unknown release reason")
        restored = all(row.get(key) is True for key in ("ai_restored", "host_story_preserved", "remote_detached"))
        if not restored:
            error("release did not restore AI/story or detach remote owner")
        lease["end"] = clock
        lease["release_index"] = index
        if row.get("reason") == "scene":
            if gate and gate["blocked"] is True and gate.get("reason", "scene") == "scene" and restored:
                scene_releases.append((epoch, gate_sequence, gate["source"]))
            else:
                error("scene release lacks an observed blocked scene gate")
        if row.get("reason") == "disconnect":
            if epoch in disconnected and restored:
                disconnect_releases.append(epoch)
            else:
                error("disconnect release lacks an observed disconnect")
        active = None

    observations = {epoch: [] for epoch in leases}
    guest_clock, guest_times, grace_samples = None, {}, 0
    idle_rows, disconnect_requests, guest_owned_clocks = [], [], []
    for guest_index, row in enumerate(guest_rows):
        kind = row.get("session")
        if kind not in {"guest-observation", "guest-idle", "disconnect-request"}:
            continue
        clock = row.get("monotonic_ms")
        if clock is not None:
            if not _number(clock) or (guest_clock is not None and clock < guest_clock):
                error("guest monotonic_ms invalid or backwards")
                continue
            guest_clock = clock
        if kind == "guest-idle":
            if (_number(clock) and type(row.get("net_mode")) is int and
                    all(type(row.get(key)) is bool for key in
                        ("has_pawn", "acknowledged_ashley", "camera_targets_ashley"))):
                idle_rows.append((guest_index, row))
            continue
        if kind == "disconnect-request":
            if _number(clock) and row.get("source") == "ordinary-engine-console":
                disconnect_requests.append((guest_index, row))
            continue
        if _number(clock) and row.get("ashley_pawn") is True:
            guest_owned_clocks.append(clock)
        # A guest's local counter is not the host's ownership epoch. Both game
        # processes use the same OS clock, so an observed lease interval can map it.
        epoch = row.get("epoch")
        if row.get("local_epoch") is True or epoch is None:
            matches = [key for key, lease in leases.items() if clock is not None and
                       lease["start"] is not None and clock >= lease["start"] and
                       (lease["end"] is None or clock < lease["end"])]
            if len(matches) == 1:
                epoch = matches[0]
            elif not matches and clock is not None and len([
                    key for key, lease in leases.items() if lease["end"] is not None and
                    0 <= clock - lease["end"] <= 1000]) == 1:
                # An ownership RPC can lag the server release. Ignore this
                # bounded propagation tail; it is never stable-control evidence.
                grace_samples += 1
                continue
            elif epoch is None and len(leases) == 1 and clock is None:
                epoch = next(iter(leases))
            else:
                error("guest observation cannot be attributed to a host epoch")
                continue
        if not _epoch(epoch) or epoch not in leases:
            error("guest observation has an unknown or invalid epoch")
            continue
        lease = leases[epoch]
        if clock is not None and lease["start"] is not None and (clock < lease["start"] or
                (lease["end"] is not None and clock >= lease["end"])):
            error("guest observation is outside its lease interval")
        elapsed = row.get("held_ms", row.get("input_elapsed_ms"))
        if not _number(elapsed) or (epoch in guest_times and elapsed < guest_times[epoch]):
            error("invalid or backwards guest elapsed time")
            continue
        guest_times[epoch] = elapsed
        position = row.get("position")
        if (type(row.get("net_mode")) is not int or row["net_mode"] != 3 or
                any(row.get(key) is not True for key in ("ashley_pawn", "reciprocal_controller", "camera_targets_ashley")) or
                not isinstance(position, list) or len(position) != 3 or
                not all(type(v) in (int, float) and math.isfinite(v) for v in position)):
            error("guest sample lacks valid pawn/controller/camera/position evidence")
            continue
        observations[epoch].append(row)

    stable = []
    durations = {}
    for epoch, lease in leases.items():
        times = [sample["held_ms"] for sample in lease["samples"]]
        durations[epoch] = times[-1] - times[0] if times else 0
        clocks = [sample.get("monotonic_ms") for sample in lease["samples"]]
        wall_stable = (not clocks or not all(clock is not None for clock in clocks) or
                       clocks[-1] - clocks[0] >= 15000)
        guest_times_seen = {row.get("held_ms", row.get("input_elapsed_ms")) for row in observations[epoch]}
        if len(set(times)) >= 10 and durations[epoch] >= 15000 and wall_stable and len(guest_times_seen) >= 10:
            stable.append(epoch)
    valid = not errors and story_ok
    reacquired = any(new > old and lease["gate_sequence"] > sequence and lease["samples"]
                     for old, sequence, _ in scene_releases for new, lease in leases.items())
    detached_scenes = []
    for epoch, _, _ in scene_releases:
        released_at = leases[epoch]["end"]
        if released_at is None:
            continue
        next_starts = [lease["start"] for newer, lease in leases.items()
                       if newer > epoch and lease["start"] is not None]
        next_acquired_at = min(next_starts) if next_starts else math.inf
        lower = released_at + 1000
        # Pawn detachment proves control was relinquished. A camera that still
        # follows Ashley does not prove the game's conversation camera is correct.
        detached = any(lower < row["monotonic_ms"] < next_acquired_at and
                       row["net_mode"] == 3 and row["has_pawn"] is False
                       for _, row in idle_rows)
        still_owned = any(lower < clock < next_acquired_at for clock in guest_owned_clocks)
        if detached and not still_owned:
            detached_scenes.append(epoch)
    engine_disconnect = False
    for epoch in disconnect_releases:
        observed = disconnected[epoch]
        observed_at = observed.get("monotonic_ms")
        started_at = leases[epoch]["start"]
        if observed.get("source") != "engine" or not _number(observed_at) or started_at is None:
            continue
        engine_disconnect |= any(
            started_at <= request["monotonic_ms"] <= observed_at and
            any(idle_index > request_index and idle["monotonic_ms"] >= request["monotonic_ms"] and
                idle["net_mode"] == 0 for idle_index, idle in idle_rows)
            for request_index, request in disconnect_requests)
    return {"protocol_valid": valid, "protocol_errors": errors,
            "host_story_preserved": story_ok,
            "epochs": list(leases), "stable_epochs": stable,
            "host_sample_counts": {str(e): len(l["samples"]) for e, l in leases.items()},
            "guest_sample_counts": {str(e): len(rows) for e, rows in observations.items()},
            "guest_propagation_grace_samples": grace_samples,
            "continuous_control_ms": {str(e): duration for e, duration in durations.items()},
            "persistent_control_verified": bool(valid and stable),
            "scene_release_verified": bool(valid and scene_releases),
            "guest_scene_detach_verified": bool(valid and detached_scenes),
            "guest_scene_detached_epochs": detached_scenes,
            "reacquire_after_scene_verified": bool(valid and reacquired),
            "disconnect_restore_verified": bool(valid and disconnect_releases),
            "engine_disconnect_verified": bool(valid and engine_disconnect),
            "engine_scene_test_verified": bool(valid and any(source == "engine" for _, _, source in scene_releases)),
            "campaign_coop_verified": False}
