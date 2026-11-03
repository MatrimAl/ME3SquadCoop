import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))
from pair_verification import connection_evidence, control_evidence


class PairVerification(unittest.TestCase):
    def fixture(self):
        host = [{"pair_control": "host-sample", "host_story_preserved": True, "held_ms": held,
                 "remote_owns_ashley": True, "ashley_distance": distance, "shepard_distance": 0}
                for held, distance in ((1000, 0), (2000, 0), (4000, 20), (5000, 20))]
        host.append({"pair_control": "restored", "host_and_ai_restored": True, "story_changed_during_test": False})
        guest = [{"pair_control": "guest-observation", "ashley_pawn": True, "net_mode": 3,
                  "reciprocal_controller": True, "camera_targets_ashley": True,
                  "input_elapsed_ms": elapsed, "position": [distance, 0, 0]}
                 for elapsed, distance in ((0, 0), (1000, 0), (4000, 20), (5000, 20))]
        return host, guest

    def test_complete_roundtrip_still_does_not_claim_campaign(self):
        result = control_evidence(*self.fixture())
        self.assertTrue(result["guest_control_roundtrip_verified"])
        self.assertFalse(result["campaign_coop_verified"])

    def test_host_story_changed_then_restored_is_still_failure(self):
        host, guest = self.fixture()
        host[1]["host_story_preserved"] = False
        self.assertFalse(control_evidence(host, guest)["guest_control_roundtrip_verified"])

    def test_one_wrong_camera_sample_is_failure(self):
        host, guest = self.fixture()
        guest[2]["camera_targets_ashley"] = False
        self.assertFalse(control_evidence(host, guest)["guest_control_roundtrip_verified"])

    def test_client_only_motion_does_not_prove_server_motion(self):
        host, guest = self.fixture()
        for row in host:
            row["ashley_distance"] = 0
        self.assertFalse(control_evidence(host, guest)["guest_control_roundtrip_verified"])

    def test_initial_server_possession_snap_does_not_prove_network_input(self):
        host, guest = self.fixture()
        for row in host:
            row["ashley_distance"] = 77
        self.assertFalse(control_evidence(host, guest)["guest_control_roundtrip_verified"])

    def test_neutral_drift_cannot_be_claimed_input_motion(self):
        host, guest = self.fixture()
        guest[1]["position"] = [20, 0, 0]
        self.assertFalse(control_evidence(host, guest)["guest_control_roundtrip_verified"])

    def test_transient_connection_cannot_pass(self):
        rows = []
        for elapsed in (1000, 3000, 5000):
            rows += [{"game_thread": 1, "elapsed_ms": elapsed}, {"network": "state", "net_mode": 3}]
        self.assertTrue(connection_evidence(rows)["persistent_client_mode_verified"])
        rows += [{"network": "state", "net_mode": 0}]
        self.assertFalse(connection_evidence(rows)["persistent_client_mode_verified"])

    def connection_rows(self, clocks):
        return [row for elapsed in clocks for row in (
            {"game_thread": 1, "elapsed_ms": elapsed},
            {"network": "state", "net_mode": 3})]

    def test_disconnect_retains_observation_but_not_active_verdict(self):
        rows = self.connection_rows((1000, 3000, 5000))
        rows.append({"network": "state", "net_mode": 0})
        result = connection_evidence(rows)
        self.assertTrue(result["sustained_client_mode_observed"])
        self.assertEqual(result["longest_client_interval_ms"], 4000)
        self.assertEqual(result["longest_client_interval_samples"], 3)
        self.assertFalse(result["client_mode_at_last_sample"])
        self.assertFalse(result["persistent_client_mode_verified"])
        self.assertEqual(result["continuous_client_mode_ms"], 0)

    def test_reconnect_does_not_join_separate_intervals(self):
        rows = self.connection_rows((1000, 2000))
        rows.append({"network": "state", "net_mode": 0})
        rows += self.connection_rows((4000, 5000))
        result = connection_evidence(rows)
        self.assertFalse(result["sustained_client_mode_observed"])
        self.assertTrue(result["client_mode_at_last_sample"])
        self.assertEqual(result["longest_client_interval_ms"], 1000)

    def test_missing_samples_cannot_prove_continuity(self):
        result = connection_evidence(self.connection_rows((1000, 3000, 9000)))
        self.assertFalse(result["sustained_client_mode_observed"])
        self.assertFalse(result["persistent_client_mode_verified"])
        self.assertEqual(result["longest_client_interval_ms"], 2000)

    def test_duplicate_backwards_and_invalid_clocks_fail_closed(self):
        for clocks in ((1000, 3000, 5000, 5000), (1000, 3000, 5000, 4000),
                       (1000, 3000, 5000, -1), (True, 2000, 4000),
                       (1000, 3000, 5000, None)):
            with self.subTest(clocks=clocks):
                result = connection_evidence(self.connection_rows(clocks))
                self.assertFalse(result["sampling_clock_valid"])
                self.assertFalse(result["sustained_client_mode_observed"])
                self.assertFalse(result["persistent_client_mode_verified"])

    def test_error_remains_visible_after_observed_interval(self):
        rows = self.connection_rows((1000, 3000, 5000))
        rows.append({"network": "progress", "progress_kind": 4, "pending_error": "Lost"})
        result = connection_evidence(rows)
        self.assertTrue(result["sustained_client_mode_observed"])
        self.assertEqual(result["failure_messages"], ["Lost"])
        self.assertFalse(result["persistent_client_mode_verified"])

    def test_active_connection_reports_both_scopes(self):
        result = connection_evidence(self.connection_rows((1000, 3000, 5000)))
        self.assertTrue(result["sustained_client_mode_observed"])
        self.assertTrue(result["persistent_client_mode_verified"])
        self.assertTrue(result["client_mode_at_last_sample"])

    def test_no_or_untimed_samples_do_not_establish_connection(self):
        for rows in ([], [{"network": "state", "net_mode": 3}] * 4):
            with self.subTest(rows=rows):
                result = connection_evidence(rows)
                self.assertFalse(result["sustained_client_mode_observed"])
                self.assertFalse(result["persistent_client_mode_verified"])

    def test_connection_error_and_missing_control_data_fail(self):
        rows = [{"game_thread": 1, "elapsed_ms": 1000}, {"network": "state", "net_mode": 3},
                {"network": "progress", "progress_kind": 4, "pending_error": "Connection lost"}]
        self.assertFalse(connection_evidence(rows)["persistent_client_mode_verified"])
        self.assertFalse(control_evidence([], [])["guest_control_roundtrip_verified"])

    def test_initial_receive_trace_is_not_an_engine_frame_clock(self):
        rows=[]
        for elapsed in (1000,3000,5000):
            rows.extend([{"game_thread":1,"elapsed_ms":elapsed},
                         {"initial_actor_trace":"received-bunch","game_thread":1,"event":elapsed},
                         {"network":"state","net_mode":3}])
        result=connection_evidence(rows)
        self.assertTrue(result["persistent_client_mode_verified"])
        self.assertEqual(result["continuous_client_mode_ms"],4000)


if __name__ == "__main__":
    unittest.main()
