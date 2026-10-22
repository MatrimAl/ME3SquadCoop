"""Evidence gates for native connection and temporary guest control probes."""
import math


def connection_evidence(rows):
    """Distinguish an observed interval from a connection still active at EOF.

    Intentional disconnects must end the current interval without erasing an
    earlier one. Neither interval proves lifecycle cleanup or cross-PC play.
    Samples separated by more than 2.5 seconds cannot establish continuity.
    """
    elapsed = None
    tail = []
    longest = []
    errors = []
    clock_valid = True
    previous_frame = None

    def duration(samples):
        return samples[-1] - samples[0] if len(samples) > 1 else 0

    def qualifies(samples):
        return clock_valid and len(samples) >= 3 and duration(samples) >= 3000

    for row in rows:
        if "game_thread" in row and "elapsed_ms" in row:
            clock = row["elapsed_ms"]
            if (type(clock) is not int or clock < 0 or
                    (previous_frame is not None and clock <= previous_frame)):
                clock_valid = False
                elapsed = None
            else:
                elapsed = previous_frame = clock
        if row.get("network") == "progress" and row.get("progress_kind") == 4:
            errors.append(row.get("pending_error", ""))
        if row.get("network") == "state":
            if type(row.get("net_mode")) is int and row["net_mode"] == 3 and elapsed is not None:
                if tail and not 0 < elapsed - tail[-1] <= 2500:
                    tail = []
                tail.append(elapsed)
                if (duration(tail), len(tail)) > (duration(longest), len(longest)):
                    longest = tail.copy()
            else:
                tail = []
    return {"consecutive_client_samples": len(tail), "continuous_client_mode_ms": duration(tail),
            "longest_client_interval_samples": len(longest),
            "longest_client_interval_ms": duration(longest),
            "sampling_clock_valid": clock_valid,
            "sustained_client_mode_observed": qualifies(longest),
            "client_mode_at_last_sample": bool(tail),
            "failure_messages": errors,
            "persistent_client_mode_verified": qualifies(tail) and not errors}


def control_evidence(host, guest):
    samples = [r for r in host if r.get("pair_control") == "host-sample"]
    restored = [r for r in host if r.get("pair_control") == "restored"]
    observed = [r for r in guest if r.get("pair_control") == "guest-observation" and r.get("ashley_pawn") is True and r.get("net_mode") == 3]
    valid = [r for r in observed if r.get("reciprocal_controller") is True and r.get("camera_targets_ashley") is True]
    positions = [r for r in valid if isinstance(r.get("position"), list) and len(r["position"]) == 3 and
                 all(isinstance(v, (int, float)) and math.isfinite(v) for v in r["position"])]
    neutral = [r for r in positions if 0 <= r.get("input_elapsed_ms", -1) < 2000]
    pulse_done = [r for r in positions if 4000 <= r.get("input_elapsed_ms", -1) <= 7000]
    neutral_distance = math.dist(neutral[0]["position"], neutral[-1]["position"]) if len(neutral) >= 2 else None
    pulse_distance = math.dist(neutral[-1]["position"], pulse_done[0]["position"]) if neutral and pulse_done else None
    story = bool(samples) and all(r.get("host_story_preserved") is True and r.get("remote_owns_ashley") is True for r in samples)
    restore = bool(restored) and all(r.get("host_and_ai_restored") is True and r.get("story_changed_during_test") is False for r in restored)
    server_neutral = [r for r in samples if 0 < r.get("held_ms", -1) <= 2000]
    server_pulse = [r for r in samples if 4000 <= r.get("held_ms", -1) <= 6000]
    # A possession/physics snap before input must not count as network input.
    # The change in radial distance is a conservative lower bound on motion.
    server_delta = (abs(server_pulse[0].get("ashley_distance", 0) - server_neutral[-1].get("ashley_distance", 0))
                    if server_neutral and server_pulse else None)
    host_motion = server_delta is not None and server_delta > 10
    host_stationary = bool(samples) and all(isinstance(r.get("shepard_distance"), (int, float)) and 0 <= r["shepard_distance"] < 5 for r in samples)
    movement = neutral_distance is not None and neutral_distance < 5 and pulse_distance is not None and pulse_distance > 10 and host_motion and host_stationary
    return {"host_possession_dispatched": any(r.get("pair_control") == "remote-possession-dispatched" for r in host),
            "host_sample_count": len(samples), "host_story_preserved": story,
            "guest_ashley_sample_count": len(observed), "guest_camera_and_controller_samples": len(valid),
            "guest_neutral_distance": neutral_distance, "guest_input_distance": pulse_distance,
            "server_input_motion_lower_bound": server_delta,
            "host_observed_ashley_movement": host_motion, "shepard_stationary": host_stationary,
            "host_and_ai_restored": restore,
            "guest_control_roundtrip_verified": bool(story and restore and len(valid) >= 3 and len(valid) == len(observed) and movement),
            "campaign_coop_verified": False}
