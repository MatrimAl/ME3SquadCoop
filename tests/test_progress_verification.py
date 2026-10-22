import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))
from progress_verification import progress_evidence


class ProgressVerification(unittest.TestCase):
    def fixture(self):
        host, guest = [], []
        for tick in range(13):
            for role in ("host-local", "host-ashley", "guest-local"):
                row = {"progress": "pawn", "role": role, "monotonic_ms": tick * 1000,
                       "world_id": "BioA_Mars_100MarsLanding.PersistentLevel", "health": 100,
                       "pawn_id": "PersistentLevel.Shepard" if role == "host-local" else "net-channel:42",
                       "position": [tick * 30, 0 if role == "host-local" else 100, 225],
                       "velocity": [30, 0, 0], "fully_initialized": True, "alive": True,
                       "squad_member": True, "hidden": False,
                       "frozen": False, "root_motion_override": False, "sleeping": False,
                       "busy_conversation": False, "hard_attached": False,
                       "scripted_move_target": False, "scripted_route": False,
                       "below_world_kill_z": False, "world_kill_z": -10000,
                       "physics": 1, "action": 0,
                       "power_action": 0, "owner_state": "PlayerWalking"}
                (guest if role == "guest-local" else host).append(row)
        return host, guest

    def test_grounded_common_travel_proves_only_travel(self):
        result = progress_evidence(*self.fixture())
        self.assertTrue(result["guest_can_progress_with_host"], result)
        for key in ("mission_completion_verified", "combat_verified", "dialogue_verified", "campaign_coop_verified"):
            self.assertFalse(result[key])

    def test_intro_ship_companion_and_freefall_cannot_pass(self):
        host, guest = self.fixture()
        for row in host:
            row["position"] = [-21924, 49481, 225] if row["role"] == "host-local" else [4954, -4872, 1862]
        for index, row in enumerate(guest):
            row["position"] = [4954, -4872, 1862 - index * 1000]
            row["velocity"] = [0, 0, -1000]
            row["physics"] = 2
        result = progress_evidence(host, guest)
        self.assertFalse(result["guest_can_progress_with_host"])
        self.assertTrue(result["unsafe_guest_motion_observed"])
        self.assertGreater(result["maximum_observed_host_guest_separation"], 50000)

    def test_missing_identity_health_or_readiness_is_unknown_not_success(self):
        for key in ("world_id", "pawn_id", "health", "alive", "hidden", "below_world_kill_z", "world_kill_z"):
            host, guest = self.fixture()
            for row in host + guest:
                row.pop(key)
            result = progress_evidence(host, guest)
            self.assertFalse(result["guest_can_progress_with_host"], key)
            self.assertIn(key, result["missing_telemetry"])

    def test_not_in_actual_squad_and_hidden_scripted_custom_frozen_fail(self):
        for key, value in (("hidden", True), ("frozen", True),
                           ("fully_initialized", False), ("root_motion_override", True),
                           ("scripted_route", True), ("scripted_move_target", True),
                           ("busy_conversation", True), ("owner_state", "ScriptedMove"),
                           ("owner_state", "Custom"), ("physics", 10), ("action", 20)):
            host, guest = self.fixture()
            for row in guest:
                row[key] = value
            self.assertFalse(progress_evidence(host, guest)["guest_can_progress_with_host"], key)

    def test_strict_booleans_dead_nan_health_and_wrong_world_fail(self):
        for key, value in (("alive", 1), ("health", 0), ("health", float("nan")),
                           ("below_world_kill_z", 0), ("hidden", 0), ("world_id", "OtherMap"),
                           ("world_id", "0x123456"), ("pawn_id", "DifferentAshley")):
            host, guest = self.fixture()
            for row in guest:
                row[key] = value
            self.assertFalse(progress_evidence(host, guest)["guest_can_progress_with_host"], key)

    def test_vertical_motion_and_stationary_host_are_not_common_progress(self):
        for variant in ("vertical", "host-stationary", "opposite"):
            host, guest = self.fixture()
            for row in host + guest:
                tick = row["monotonic_ms"] / 1000
                if variant == "vertical":
                    row["position"] = [0, 100, 225 + tick * 30]
                elif variant == "host-stationary" and row["role"] == "host-local":
                    row["position"][0] = 0
                elif variant == "opposite" and row["role"] != "host-local":
                    row["position"][0] *= -1
            self.assertFalse(progress_evidence(host, guest)["guest_can_progress_with_host"], variant)

    def test_unbounded_separation_and_wrong_server_replica_fail(self):
        for variant in ("distant", "replica"):
            host, guest = self.fixture()
            for row in host:
                if row["role"] == "host-ashley":
                    row["position"][1] += 3000 if variant == "distant" else 400
            if variant == "distant":
                for row in guest:
                    row["position"][1] += 3000
            self.assertFalse(progress_evidence(host, guest)["guest_can_progress_with_host"], variant)

    def test_short_trace_time_misalignment_and_reused_guest_samples_fail(self):
        host, guest = self.fixture()
        self.assertFalse(progress_evidence(host[:18], guest[:9])["guest_can_progress_with_host"])
        for row in guest:
            row["monotonic_ms"] += 20000
        self.assertFalse(progress_evidence(host, guest)["guest_can_progress_with_host"])
        host, guest = self.fixture()
        self.assertFalse(progress_evidence(host, guest[:1])["guest_can_progress_with_host"])

    def test_trace_with_successful_window_then_fall_is_failure(self):
        host, guest = self.fixture()
        tail = copy.deepcopy(guest[-1])
        tail.update(monotonic_ms=13000, below_world_kill_z=True)
        guest.append(tail)
        self.assertFalse(progress_evidence(host, guest)["guest_can_progress_with_host"])

    def test_same_area_and_clock_monotonicity_are_checked(self):
        host, guest = self.fixture()
        for row in host + guest:
            row["area_id"] = "Landing" if row["role"] != "guest-local" else "ShuttleInterior"
        self.assertFalse(progress_evidence(host, guest)["guest_can_progress_with_host"])
        host, guest = self.fixture()
        guest.append(copy.deepcopy(guest[-1]))
        self.assertFalse(progress_evidence(host, guest)["guest_can_progress_with_host"])

    def test_empty_trace_fails_and_invalid_thresholds_rejected(self):
        self.assertFalse(progress_evidence([], [])["guest_can_progress_with_host"])
        with self.assertRaises(ValueError):
            progress_evidence([], [], max_separation=float("inf"))

    def test_kill_z_comparison_cannot_be_faked_by_false_flag(self):
        host, guest = self.fixture()
        for row in guest:
            row["world_kill_z"] = 300
        result = progress_evidence(host, guest)
        self.assertFalse(result["guest_can_progress_with_host"])
        self.assertTrue(result["unsafe_guest_motion_observed"])

    def test_health_pct_supported_without_inventing_ground_or_event_history(self):
        host, guest = self.fixture()
        for row in host + guest:
            row["health_pct"] = row.pop("health") / 100
        result = progress_evidence(host, guest)
        self.assertTrue(result["guest_can_progress_with_host"])
        self.assertFalse(result["ground_contact_verified"])
        self.assertFalse(result["fell_out_of_world_event_history_verified"])

    def test_guest_missing_local_squad_can_use_exact_authoritative_channel_roster(self):
        for local_membership in (False, None):
            host, guest = self.fixture()
            for row in guest:
                if local_membership is None:
                    row.pop("squad_member")
                else:
                    row["squad_member"] = local_membership
                row["actor_path"] = "BioP_ProMar.SFXPawn_Ashley_0"
            for row in host:
                if row["role"] == "host-ashley":
                    row["actor_path"] = "BioH_Ashley_01.SFXPawn_Ashley_3"
            result = progress_evidence(host, guest)
            self.assertTrue(result["guest_can_progress_with_host"], result)
            self.assertEqual(result["guest_squad_membership_source"], "host-authoritative-channel-match")

    def test_guest_class_or_path_cannot_replace_channel_identity(self):
        for bad_id in ("SFXPawn_Ashley", "PersistentLevel.Ashley", "net-channel:0",
                       "net-channel:-1", "net-channel:042", "net-channel:", "net-channel:True"):
            host, guest = self.fixture()
            for row in host + guest:
                if row["role"] != "host-local":
                    row["pawn_id"] = bad_id
            self.assertFalse(progress_evidence(host, guest)["guest_can_progress_with_host"], bad_id)

    def test_mismatched_channel_world_or_absent_authoritative_roster_fails(self):
        for variant in ("channel", "world", "not-roster", "unknown-roster"):
            host, guest = self.fixture()
            for row in guest:
                row["squad_member"] = False
                if variant == "channel":
                    row["pawn_id"] = "net-channel:43"
                if variant == "world":
                    row["world_id"] = "OtherWorld"
            for row in host:
                if row["role"] == "host-ashley":
                    if variant == "not-roster":
                        row["squad_member"] = False
                    if variant == "unknown-roster":
                        row.pop("squad_member")
            result = progress_evidence(host, guest)
            self.assertFalse(result["guest_can_progress_with_host"], variant)
            self.assertEqual(result["guest_squad_membership_source"], "unverified")

    def test_intro_pawn_cinematic_origin_change_is_not_a_network_pawn_fall(self):
        host, guest = self.fixture()
        for row in host + guest:
            row["monotonic_ms"] += 3000
        intro = copy.deepcopy(guest[0])
        intro.update(monotonic_ms=0, pawn_id="", position=[4954, -4872, 1862],
                     velocity=[0, 0, -1000], below_world_kill_z=True)
        cinematic = copy.deepcopy(intro)
        cinematic.update(monotonic_ms=1000, position=[0, 0, 0], velocity=[0, 0, 0])
        guest = [intro, cinematic] + guest
        result = progress_evidence(host, guest)
        self.assertTrue(result["guest_can_progress_with_host"], result)
        self.assertFalse(result["unsafe_guest_motion_observed"])
        self.assertEqual(result["excluded_guest_staging_motion_samples"], 2)

    def test_same_channel_fall_after_success_still_invalidates_all_travel(self):
        for kind in ("vertical-drop", "velocity", "kill-z"):
            host, guest = self.fixture()
            tail = copy.deepcopy(guest[-1])
            tail["monotonic_ms"] = 13000
            if kind == "vertical-drop":
                tail["position"][2] -= 600
            if kind == "velocity":
                tail["velocity"][2] = -1000
            if kind == "kill-z":
                tail["below_world_kill_z"] = True
            guest.append(tail)
            result = progress_evidence(host, guest)
            self.assertFalse(result["guest_can_progress_with_host"], kind)
            self.assertTrue(result["unsafe_guest_motion_observed"], kind)

    def test_motion_deltas_reset_for_different_pawn_world_or_large_gap(self):
        for variant in ("pawn", "world", "gap", "nonidentity"):
            host, guest = self.fixture()
            tail = copy.deepcopy(guest[-1])
            tail.update(monotonic_ms=13000, position=[360, 100, -375], velocity=[0, 0, 0])
            if variant == "pawn":
                tail["pawn_id"] = "net-channel:43"
            elif variant == "world":
                tail["world_id"] = "NewWorld"
            elif variant == "gap":
                tail["monotonic_ms"] = 15001
            else:
                unowned = copy.deepcopy(tail)
                unowned.update(monotonic_ms=12500, pawn_id="")
                guest.append(unowned)
            guest.append(tail)
            result = progress_evidence(host, guest)
            self.assertTrue(result["guest_can_progress_with_host"], (variant, result))
            self.assertFalse(result["unsafe_guest_motion_observed"], variant)
            self.assertGreater(result["guest_motion_delta_identity_or_gap_resets"], 0)

    def test_two_second_guest_burst_then_stationary_is_not_sustained_travel(self):
        host, guest = self.fixture()
        for row in host + guest:
            if row["role"] != "host-local":
                tick = row["monotonic_ms"] / 1000
                row["position"][0] = min(tick, 2) * 150
                row["velocity"][0] = 150 if tick < 2 else 0
        result = progress_evidence(host, guest)
        self.assertFalse(result["guest_can_progress_with_host"])
        candidate = result["candidate_intervals"][0]
        self.assertGreaterEqual(candidate["guest_horizontal_displacement"], 200)
        self.assertEqual(candidate["motion_by_role"]["guest-local"]["moving_duration_ms"], 2000)
        self.assertLess(candidate["motion_by_role"]["guest-local"]["duration_coverage"], 0.5)
        self.assertTrue(candidate["rejection_reasons"])

    def test_each_role_must_meet_duration_and_five_moving_interval_requirements(self):
        for stalled_role in ("host-local", "host-ashley", "guest-local"):
            host, guest = self.fixture()
            for row in host + guest:
                if row["role"] == stalled_role:
                    row["position"][0] = min(row["monotonic_ms"] / 1000, 2) * 150
            self.assertFalse(progress_evidence(host, guest)["guest_can_progress_with_host"], stalled_role)
        host, guest = self.fixture()
        host = host[:20]
        guest = guest[:10]
        for row in host + guest:
            tick = row["monotonic_ms"] / 1000
            row["monotonic_ms"] = [0, 2000, 4000, 6000, 8000, 9000, 10000, 11000, 12000, 13000][int(tick)]
            row["position"][0] = min(tick, 4) * 60
        result = progress_evidence(host, guest)
        self.assertFalse(result["guest_can_progress_with_host"])
        self.assertEqual(result["candidate_intervals"][0]["motion_by_role"]["guest-local"]["moving_intervals"], 4)
        self.assertGreater(result["candidate_intervals"][0]["motion_by_role"]["guest-local"]["duration_coverage"], 0.5)
        host, guest = self.fixture()
        for row in host + guest:
            tick = row["monotonic_ms"] / 1000
            row["monotonic_ms"] *= 2
            row["position"][0] = min(tick, 5) * 60
        result = progress_evidence(host, guest)
        self.assertFalse(result["guest_can_progress_with_host"])
        evidence = result["candidate_intervals"][0]["motion_by_role"]["guest-local"]
        self.assertEqual(evidence["moving_intervals"], 5)
        self.assertLess(evidence["duration_coverage"], 0.5)

    def test_normal_continuous_motion_exposes_full_per_role_coverage(self):
        result = progress_evidence(*self.fixture())
        self.assertTrue(result["guest_can_progress_with_host"])
        for role, evidence in result["qualifying_intervals"][0]["motion_by_role"].items():
            self.assertEqual(evidence["moving_duration_ms"], 12000, role)
            self.assertEqual(evidence["moving_intervals"], 12, role)
            self.assertEqual(evidence["duration_coverage"], 1, role)

    def test_motion_threshold_uses_observed_positions_and_half_duration_is_inclusive(self):
        host, guest = self.fixture()
        for row in host + guest:
            row["position"][0] = row["monotonic_ms"] / 1000 * 19
            row["velocity"][0] = 999
        result = progress_evidence(host, guest)
        self.assertFalse(result["guest_can_progress_with_host"])
        self.assertGreater(result["candidate_intervals"][0]["guest_horizontal_displacement"], 200)
        self.assertEqual(result["candidate_intervals"][0]["motion_by_role"]["guest-local"]["moving_intervals"], 0)
        host, guest = self.fixture()
        for row in host + guest:
            row["position"][0] = min(row["monotonic_ms"] / 1000, 6) * 40
        result = progress_evidence(host, guest)
        self.assertTrue(result["guest_can_progress_with_host"])
        self.assertEqual(result["qualifying_intervals"][0]["motion_by_role"]["guest-local"]["duration_coverage"], 0.5)


if __name__ == "__main__":
    unittest.main()
