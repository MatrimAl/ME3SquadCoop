import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))
from session_verification import session_evidence


class SessionVerification(unittest.TestCase):
    def fixture(self, source="synthetic"):
        host, guest = [], []
        for epoch, start in ((1, 1000), (2, 22000)):
            host.append({"session": "acquired", "epoch": epoch, "monotonic_ms": start,
                         "remote_owns_ashley": True, "host_story_preserved": True})
            for elapsed in range(0, 16001, 1000):
                host.append({"session": "sample", "epoch": epoch, "held_ms": elapsed,
                             "monotonic_ms": start + elapsed, "world_time": elapsed / 1000,
                             "remote_owns_ashley": True, "host_story_preserved": True, "mode": 0})
                guest.append({"session": "guest-observation", "epoch": epoch + 10,
                              "local_epoch": True, "monotonic_ms": start + elapsed,
                              "held_ms": elapsed, "net_mode": 3, "ashley_pawn": True,
                              "reciprocal_controller": True, "camera_targets_ashley": True,
                              "position": [0, 0, 0]})
            if epoch == 1:
                host.append({"session": "gate-observation", "blocked": True,
                             "source": source, "mode": 7, "monotonic_ms": 18000})
                host.append(self.release(epoch, "scene", 18000))
                host.append({"session": "blocked", "reason": "scene", "mode": 7})
                host.append({"session": "gate-observation", "blocked": False,
                             "source": source, "mode": 0, "monotonic_ms": 21000})
            else:
                host.append({"session": "disconnect-observation", "epoch": 2,
                             "connected": False, "source": "engine", "monotonic_ms": 39000})
                host.append(self.release(epoch, "disconnect", 39000))
        return host, guest

    @staticmethod
    def release(epoch, reason, clock):
        return {"session": "released", "epoch": epoch, "reason": reason,
                "monotonic_ms": clock, "ai_restored": True,
                "host_story_preserved": True, "remote_detached": True}

    def test_complete_synthetic_lifecycle_does_not_claim_real_scene_or_campaign(self):
        result = session_evidence(*self.fixture())
        for fact in ("persistent_control_verified", "scene_release_verified",
                     "reacquire_after_scene_verified", "disconnect_restore_verified"):
            self.assertTrue(result[fact], result["protocol_errors"])
        self.assertFalse(result["engine_scene_test_verified"])
        self.assertFalse(result["campaign_coop_verified"])

    def test_observed_engine_scene_is_separate_fact(self):
        self.assertTrue(session_evidence(*self.fixture("engine"))["engine_scene_test_verified"])

    def test_stale_epoch_reacquisition_fails(self):
        host, guest = self.fixture()
        next(row for row in host if row.get("session") == "acquired" and row["epoch"] == 2)["epoch"] = 1
        self.assertFalse(session_evidence(host, guest)["protocol_valid"])

    def test_release_without_acquisition_fails(self):
        self.assertFalse(session_evidence([self.release(1, "test-stop", 10)], [])["protocol_valid"])

    def test_duplicate_release_and_sample_after_release_fail(self):
        for kind in ("released", "sample"):
            host, guest = self.fixture()
            row = copy.deepcopy(host[-1] if kind == "released" else host[-3])
            row["monotonic_ms"] = 40000
            host.append(row)
            self.assertFalse(session_evidence(host, guest)["protocol_valid"])

    def test_missing_guest_camera_invalidates_control(self):
        host, guest = self.fixture()
        guest[2].pop("camera_targets_ashley")
        self.assertFalse(session_evidence(host, guest)["persistent_control_verified"])

    def test_story_changed_then_restored_remains_failure(self):
        host, guest = self.fixture()
        host[5]["host_story_preserved"] = False
        self.assertFalse(session_evidence(host, guest)["persistent_control_verified"])

    def test_transient_duration_and_insufficient_samples_fail(self):
        host, guest = self.fixture()
        for row in host:
            if row.get("session") == "sample":
                row["held_ms"] /= 10
        self.assertFalse(session_evidence(host, guest)["persistent_control_verified"])
        host, guest = self.fixture()
        guest = [row for row in guest if row["held_ms"] in (0, 16000)]
        self.assertFalse(session_evidence(host, guest)["persistent_control_verified"])

    def test_no_scene_or_disconnect_event_cannot_be_inferred_from_release_reason(self):
        host, guest = self.fixture()
        host = [row for row in host if row.get("session") not in ("gate-observation", "disconnect-observation", "blocked")]
        result = session_evidence(host, guest)
        self.assertFalse(result["scene_release_verified"])
        self.assertFalse(result["disconnect_restore_verified"])

    def test_acquisition_during_scene_hold_fails(self):
        host, guest = self.fixture()
        host = [row for row in host if not (row.get("session") == "gate-observation" and row["blocked"] is False)]
        self.assertFalse(session_evidence(host, guest)["protocol_valid"])

    def test_disconnect_requires_ai_restoration_without_live_remote(self):
        host, guest = self.fixture()
        host[-1]["ai_restored"] = False
        self.assertFalse(session_evidence(host, guest)["disconnect_restore_verified"])
        host, guest = self.fixture()
        host[-1]["remote_detached"] = False
        self.assertFalse(session_evidence(host, guest)["disconnect_restore_verified"])

    def test_strict_boolean_and_finite_monotonic_clocks(self):
        for key, value in (("held_ms", float("nan")), ("held_ms", -1),
                           ("held_ms", True), ("world_time", float("inf")),
                           ("world_time", -1), ("remote_owns_ashley", 1),
                           ("monotonic_ms", 1)):
            host, guest = self.fixture()
            host[5][key] = value
            self.assertFalse(session_evidence(host, guest)["protocol_valid"], key)

    def test_guest_local_counter_uses_os_clock_and_release_grace_is_not_counted(self):
        host, guest = self.fixture()
        tail = copy.deepcopy(guest[16])
        tail.update(monotonic_ms=18500, held_ms=17500)
        guest.insert(17, tail)
        result = session_evidence(host, guest)
        self.assertTrue(result["persistent_control_verified"], result["protocol_errors"])
        self.assertEqual(result["guest_propagation_grace_samples"], 1)
        self.assertEqual(result["guest_sample_counts"]["1"], 17)
        tail["monotonic_ms"] = 20000
        self.assertFalse(session_evidence(host, guest)["protocol_valid"])

    def test_empty_trace_does_not_verify_any_behavior(self):
        result = session_evidence([], [])
        self.assertFalse(result["persistent_control_verified"])
        self.assertFalse(result["scene_release_verified"])
        self.assertFalse(result["disconnect_restore_verified"])

    def test_duplicate_guest_samples_do_not_substitute_for_ten_observations(self):
        host, guest = self.fixture()
        guest = [copy.deepcopy(row) for row in (guest[0], guest[17]) for _ in range(20)]
        self.assertFalse(session_evidence(host, guest)["persistent_control_verified"])

    def test_elapsed_claim_requires_fifteen_seconds_on_available_os_clock(self):
        host, guest = self.fixture()
        for row in host + guest:
            if "monotonic_ms" in row:
                row["monotonic_ms"] /= 10
        self.assertFalse(session_evidence(host, guest)["persistent_control_verified"])

    def idle_fixture(self):
        host, guest = self.fixture()
        guest += [self.idle(20000, 3),
                  {"session": "disconnect-request", "monotonic_ms": 38500,
                   "source": "ordinary-engine-console"}, self.idle(40000, 0)]
        guest.sort(key=lambda row: row["monotonic_ms"])
        return host, guest

    @staticmethod
    def idle(clock, mode):
        return {"session": "guest-idle", "monotonic_ms": clock,
                "net_mode": mode, "has_pawn": False,
                "acknowledged_ashley": False, "camera_targets_ashley": True}

    def test_scene_detach_needs_guest_evidence_and_does_not_claim_story_camera(self):
        result = session_evidence(*self.idle_fixture())
        self.assertTrue(result["guest_scene_detach_verified"], result["protocol_errors"])
        self.assertEqual(result["guest_scene_detached_epochs"], [1])
        self.assertFalse(result["engine_scene_test_verified"])
        self.assertFalse(result["campaign_coop_verified"])

    def test_missing_idle_and_stale_idle_cannot_prove_scene_detachment(self):
        host, guest = self.fixture()
        self.assertFalse(session_evidence(host, guest)["guest_scene_detach_verified"])
        for clock in (500, 5000, 18000, 19000, 22000, 23000):
            mutated = guest + [self.idle(clock, 3)]
            mutated.sort(key=lambda row: row["monotonic_ms"])
            self.assertFalse(session_evidence(host, mutated)["guest_scene_detach_verified"], clock)

    def test_scene_idle_requires_no_pawn_and_client_mode_with_strict_booleans(self):
        for key, value in (("has_pawn", True), ("has_pawn", 0), ("net_mode", 0),
                           ("net_mode", True), ("acknowledged_ashley", 0),
                           ("camera_targets_ashley", 1)):
            host, guest = self.idle_fixture()
            idle = next(row for row in guest if row["session"] == "guest-idle" and row["net_mode"] == 3)
            idle[key] = value
            self.assertFalse(session_evidence(host, guest)["guest_scene_detach_verified"], key)

    def test_owned_guest_after_release_grace_invalidates_scene_detach(self):
        host, guest = self.idle_fixture()
        late = copy.deepcopy(guest[16])
        late.update(monotonic_ms=20500, held_ms=19500)
        guest.append(late)
        guest.sort(key=lambda row: row["monotonic_ms"])
        self.assertFalse(session_evidence(host, guest)["guest_scene_detach_verified"])

    def test_engine_disconnect_requires_both_peers_and_ordinary_request(self):
        result = session_evidence(*self.idle_fixture())
        self.assertTrue(result["engine_disconnect_verified"], result["protocol_errors"])
        for removed in ("disconnect-request", "guest-idle"):
            host, guest = self.idle_fixture()
            guest = [row for row in guest if row["session"] != removed]
            self.assertFalse(session_evidence(host, guest)["engine_disconnect_verified"])
        host, guest = self.idle_fixture()
        next(row for row in host if row["session"] == "disconnect-observation")["source"] = "synthetic"
        self.assertFalse(session_evidence(host, guest)["engine_disconnect_verified"])

    def test_idle_before_request_or_wrong_request_source_is_not_disconnect_proof(self):
        host, guest = self.idle_fixture()
        request = next(row for row in guest if row["session"] == "disconnect-request")
        request["source"] = "synthetic"
        self.assertFalse(session_evidence(host, guest)["engine_disconnect_verified"])
        host, guest = self.idle_fixture()
        idle = next(row for row in guest if row["session"] == "guest-idle" and row["net_mode"] == 0)
        idle["monotonic_ms"] = 38001
        guest.sort(key=lambda row: row["monotonic_ms"])
        self.assertFalse(session_evidence(host, guest)["engine_disconnect_verified"])


if __name__ == "__main__":
    unittest.main()
