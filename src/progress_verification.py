"""Evidence of nearby, grounded travel; never proof of mission completion.

All distances are Unreal units. Identity strings must be stable semantic IDs
shared by both processes, not process-local pointer addresses.
"""
import math


def _finite(value):
    return type(value) in (int, float) and math.isfinite(value)


def _vector(value):
    return isinstance(value, list) and len(value) == 3 and all(_finite(x) for x in value)


def _identity(value):
    return isinstance(value, str) and bool(value.strip()) and not value.lower().startswith("0x")


def _channel_identity(value):
    if not isinstance(value, str) or not value.startswith("net-channel:"):
        return False
    number = value[len("net-channel:"):]
    return (0 < len(number) <= 10 and number.isascii() and number.isdecimal() and
            0 < int(number) <= 0x7fffffff and str(int(number)) == number)


def progress_evidence(host_rows, guest_rows, *, min_duration_ms=10000, min_samples=10,
                      max_alignment_ms=1000, max_separation=2000,
                      max_replica_error=300, min_horizontal_displacement=200,
                      max_vertical_change=200):
    """Require a sustained aligned host/host-Ashley/guest walking interval.

    Missing readiness or identity telemetry fails closed. Ordinary intro/staging
    rows can precede a valid interval. Walking physics and KillZ comparisons are
    sampled state, not proof of floor contact or a complete engine event history.
    """
    if (type(min_samples) is not int or min_samples < 2 or
            any(not _finite(value) or value <= 0 for value in
                (min_duration_ms, max_alignment_ms, max_separation, max_replica_error,
                 min_horizontal_displacement, max_vertical_change))):
        raise ValueError("positive finite thresholds and min_samples >= 2 required")
    roles = {"host-local": [], "host-ashley": [], "guest-local": []}
    problems, missing = set(), set()
    for rows, allowed in ((host_rows, {"host-local", "host-ashley"}), (guest_rows, {"guest-local"})):
        for row in rows:
            if row.get("progress") == "pawn" and row.get("role") in allowed:
                role = row["role"]
                clock = row.get("monotonic_ms")
                if not _finite(clock) or clock < 0:
                    problems.add("invalid sampling clock")
                    continue
                if roles[role] and clock <= roles[role][-1]["monotonic_ms"]:
                    problems.add("duplicate or backwards sampling clock")
                    continue
                roles[role].append(row)

    required_true = ("fully_initialized", "alive")
    required_false = ("hidden", "frozen", "root_motion_override", "sleeping",
                      "busy_conversation", "hard_attached", "scripted_move_target",
                      "scripted_route", "below_world_kill_z")

    def ready(row):
        health_key = "health_pct" if "health_pct" in row else "health"
        needed = (*required_true, *required_false, health_key, "world_kill_z", "world_id", "pawn_id",
                  "position", "velocity", "physics", "action", "power_action", "owner_state")
        missing.update(key for key in needed if key not in row)
        if any(row.get(key) is not True for key in required_true) or any(row.get(key) is not False for key in required_false):
            return False
        if (not _finite(row.get(health_key)) or row[health_key] <= 0 or
                not _finite(row.get("world_kill_z")) or
                not _identity(row.get("world_id")) or not _identity(row.get("pawn_id")) or
                not _vector(row.get("position")) or not _vector(row.get("velocity")) or
                type(row.get("physics")) is not int or row["physics"] != 1 or
                type(row.get("action")) is not int or row["action"] != 0 or
                type(row.get("power_action")) is not int or row["power_action"] != 0):
            return False
        if row["position"][2] < row["world_kill_z"]:
            return False
        state = row.get("owner_state")
        if not isinstance(state, str) or any(token in state.lower() for token in ("script", "cinematic", "frozen", "custom")):
            return False
        if row["role"] == "host-ashley":
            if "squad_member" not in row:
                missing.add("squad_member")
            if row.get("squad_member") is not True:
                return False
        # The guest's WorldInfo.PlayerSquad need not be replicated. Its squad
        # membership is derived only from an exact authoritative roster actor's
        # reciprocal ActorChannel identity, never from class or actor-path text.
        if row["role"] != "host-local" and not _channel_identity(row.get("pawn_id")):
            return False
        return abs(row["velocity"][2]) <= 200

    # Diagnose missing fields even when a timestamp has no matching counterpart.
    readiness = {id(row): ready(row) for rows in roles.values() for row in rows}
    unsafe = []
    excluded_staging_samples, motion_delta_resets = 0, 0
    previous = None
    for row in roles["guest-local"]:
        # Prewarm Shepard/cinematic origins are not the owned replicated pawn.
        # Never turn a pawn/world swap into a physical fall measurement.
        if not _channel_identity(row.get("pawn_id")) or not _identity(row.get("world_id")):
            excluded_staging_samples += 1
            if previous is not None:
                motion_delta_resets += 1
            previous = None
            continue
        if row.get("below_world_kill_z") is True or (_vector(row.get("position")) and
                _finite(row.get("world_kill_z")) and row["position"][2] < row["world_kill_z"]):
            unsafe.append("guest sampled below world KillZ")
        if _vector(row.get("velocity")) and row["velocity"][2] < -500:
            unsafe.append("guest downward velocity exceeds walking bound")
        if previous:
            elapsed = row["monotonic_ms"] - previous["monotonic_ms"]
            same_actor_world = (row["pawn_id"] == previous["pawn_id"] and
                                row["world_id"] == previous["world_id"])
            if not same_actor_world or elapsed > 2000:
                motion_delta_resets += 1
            elif (0 < elapsed <= 2000 and _vector(row.get("position")) and
                  _vector(previous.get("position")) and row["position"][2] - previous["position"][2] < -500):
                unsafe.append("guest sustained vertical drop")
        previous = row

    aligned, current, runs = [], [], []
    used_ashley, used_guest = set(), set()
    max_seen_separation, max_seen_replica = None, None
    for host in roles["host-local"]:
        matches = []
        for role, used in (("host-ashley", used_ashley), ("guest-local", used_guest)):
            candidates = [row for row in roles[role] if id(row) not in used and
                          abs(row["monotonic_ms"] - host["monotonic_ms"]) < max_alignment_ms]
            match = min(candidates, key=lambda row: abs(row["monotonic_ms"] - host["monotonic_ms"])) if candidates else None
            matches.append(match)
            if match:
                used.add(id(match))
        ashley, guest = matches
        good = ashley is not None and guest is not None and all(readiness[id(row)] for row in (host, ashley, guest))
        if ashley is not None and guest is not None:
            aligned.append((host, ashley, guest))
            clocks = [row["monotonic_ms"] for row in (host, ashley, guest)]
            good = good and max(clocks) - min(clocks) < max_alignment_ms
            if all(_vector(row.get("position")) for row in (host, ashley, guest)):
                separation = math.dist(host["position"], guest["position"])
                replica = math.dist(ashley["position"], guest["position"])
                max_seen_separation = max(separation, max_seen_separation or 0)
                max_seen_replica = max(replica, max_seen_replica or 0)
                good = good and separation <= max_separation and replica <= max_replica_error and math.dist(host["position"], ashley["position"]) <= max_separation
            else:
                good = False
            good = good and host.get("world_id") == ashley.get("world_id") == guest.get("world_id") and ashley.get("pawn_id") == guest.get("pawn_id")
            area_ids = [row.get("area_id") for row in (host, ashley, guest)]
            if any(area is not None for area in area_ids):
                good = good and all(_identity(area) for area in area_ids) and len(set(area_ids)) == 1
        if current and (host["monotonic_ms"] - current[-1][0]["monotonic_ms"] > 2000 or
                        host.get("world_id") != current[-1][0].get("world_id")):
            runs.append(current)
            current = []
        if good:
            current.append((host, ashley, guest))
        elif current:
            runs.append(current)
            current = []
    if current:
        runs.append(current)

    qualifying, candidates, best_duration = [], [], 0
    for run in runs:
        duration = run[-1][0]["monotonic_ms"] - run[0][0]["monotonic_ms"]
        best_duration = max(best_duration, duration)
        if len(run) < min_samples or duration < min_duration_ms:
            continue
        first, last = run[0], run[-1]
        displacements = [[last[index]["position"][axis] - first[index]["position"][axis]
                          for axis in range(3)] for index in (0, 1, 2)]
        horizontal = [math.hypot(delta[0], delta[1]) for delta in displacements]
        rejections, motion = [], {}
        for index, role in enumerate(("host-local", "host-ashley", "guest-local")):
            moving_ms, moving_intervals, valid_intervals = 0, 0, 0
            for before, after in zip(run, run[1:]):
                left, right = before[index], after[index]
                elapsed = right["monotonic_ms"] - left["monotonic_ms"]
                if (not 0 < elapsed <= 2000 or left["pawn_id"] != right["pawn_id"] or
                        left["world_id"] != right["world_id"]):
                    continue
                valid_intervals += 1
                distance = math.hypot(right["position"][0] - left["position"][0],
                                      right["position"][1] - left["position"][1])
                if distance * 1000 / elapsed >= 20:
                    moving_ms += elapsed
                    moving_intervals += 1
            motion[role] = {"moving_duration_ms": moving_ms, "moving_intervals": moving_intervals,
                            "eligible_intervals": valid_intervals,
                            "duration_coverage": moving_ms / duration,
                            "sample_interval_coverage": moving_intervals / valid_intervals if valid_intervals else 0}
            if moving_ms < duration * 0.5 or moving_intervals < 5:
                rejections.append(role + " lacks sustained observed horizontal motion")
            if horizontal[index] < min_horizontal_displacement:
                rejections.append(role + " insufficient horizontal displacement")
            if max(row[index]["position"][2] for row in run) - min(row[index]["position"][2] for row in run) > max_vertical_change:
                rejections.append(role + " exceeds vertical range")
        host_delta, _, guest_delta = displacements
        cosine = ((host_delta[0] * guest_delta[0] + host_delta[1] * guest_delta[1]) / (horizontal[0] * horizontal[2])
                  if horizontal[0] > 0 and horizontal[2] > 0 else None)
        if cosine is None or cosine < 0.5:
            rejections.append("host and guest lack common horizontal direction")
        evidence = {"sample_count": len(run), "duration_ms": duration,
                    "host_horizontal_displacement": horizontal[0],
                    "host_ashley_horizontal_displacement": horizontal[1],
                    "guest_horizontal_displacement": horizontal[2],
                    "movement_direction_cosine": cosine, "motion_by_role": motion,
                    "rejection_reasons": rejections}
        candidates.append(evidence)
        if not rejections:
            qualifying.append(evidence)
    if not aligned:
        problems.add("no aligned host/host-Ashley/guest samples")
    if not qualifying:
        problems.add("no sustained nearby walking common travel interval")
    problems.update(unsafe)
    # Intro rows can lack fields, but any accepted travel interval contains all
    # mandatory evidence. Missing telemetry alone never produces a positive run.
    return {"guest_can_progress_with_host": bool(qualifying and not unsafe and not any(
                "clock" in problem for problem in problems)),
            "aligned_sample_count": len(aligned), "longest_ready_interval_ms": best_duration,
            "qualifying_intervals": qualifying, "missing_telemetry": sorted(missing),
            "candidate_intervals": candidates,
            "guest_squad_membership_source": "host-authoritative-channel-match" if qualifying else "unverified",
            "negative_reasons": sorted(problems),
            "unsafe_guest_motion_observed": bool(unsafe),
            "excluded_guest_staging_motion_samples": excluded_staging_samples,
            "guest_motion_delta_identity_or_gap_resets": motion_delta_resets,
            "maximum_observed_host_guest_separation": max_seen_separation,
            "maximum_observed_host_guest_replica_error": max_seen_replica,
            "ground_contact_verified": False, "fell_out_of_world_event_history_verified": False,
            "mission_completion_verified": False, "combat_verified": False,
            "dialogue_verified": False, "campaign_coop_verified": False}
