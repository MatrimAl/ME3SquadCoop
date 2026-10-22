import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))
from ammo_wire_verification import ammo_wire_evidence


def fixture():
    return [
        {"ammo_wire": "installed", "probe_changed_payload": False},
        {"ammo_wire": "scope", "generation": 1, "channel_index": 9, "monotonic_ms": 100},
        {"ammo_wire": "field", "direction": "decode", "field": "AmmoUsedCount", "generation": 1,
         "receive_id": 1, "channel_index": 9, "monotonic_ms": 110, "value_before": 4, "value_after": 3,
         "native_result": 1, "accepted": True, "archive_error_before": 0, "archive_error_after": 0,
         "original_forwarded": True, "probe_changed_payload": False},
        {"ammo_wire": "receive-complete", "generation": 1, "receive_id": 1, "channel_index": 9,
         "monotonic_ms": 111, "field_mask": 1, "used_before": 4, "used_after": 3, "decoded_used": 3,
         "original_receive_forwarded": True, "probe_changed_payload": False},
    ]


class AmmoWireTests(unittest.TestCase):
    def test_decoded_change_and_final_storage_agree(self):
        result = ammo_wire_evidence(fixture())
        self.assertEqual(result["accepted_decodes"], 1)
        self.assertEqual(result["fields_matching_final_storage"], 1)
        self.assertEqual(result["used_count_decreases_during_receive"], 1)
        self.assertTrue(result["received_field_change_observed"])
        self.assertFalse(result["ammo_synchronization_verified"])
        self.assertFalse(result["packet_matching_verified"])

    def test_equal_prior_value_is_not_a_change(self):
        rows = fixture(); rows[-1]["used_before"] = 3
        result = ammo_wire_evidence(rows)
        self.assertTrue(result["received_value_matches_storage_observed"])
        self.assertFalse(result["received_field_change_observed"])

    def test_encoding_and_decoding_are_separate(self):
        rows = fixture()[:-1]; rows[-1].update(direction="encode", receive_id=0)
        result = ammo_wire_evidence(rows)
        self.assertEqual(result["accepted_encodes"], 1)
        self.assertEqual(result["accepted_decodes"], 0)
        self.assertFalse(result["received_value_matches_storage_observed"])

    def test_decode_without_completion_is_not_application(self):
        result = ammo_wire_evidence(fixture()[:-1])
        self.assertEqual(result["uncompleted_decode_fields"], 1)
        self.assertFalse(result["received_value_matches_storage_observed"])

    def test_invalid_native_observations_cannot_pass(self):
        for key, value in [("native_result", 0), ("native_result", True), ("archive_error_before", 1),
            ("archive_error_after", 1), ("accepted", False), ("original_forwarded", False),
            ("probe_changed_payload", True), ("value_after", True), ("field", {}), ("channel_index", 10)]:
            with self.subTest(key=key, value=value):
                rows = fixture(); rows[-2][key] = value
                self.assertFalse(ammo_wire_evidence(rows)["received_value_matches_storage_observed"])

    def test_missing_installed_or_scope_cannot_pass(self):
        for index in (0, 1):
            rows = fixture(); rows.pop(index)
            self.assertFalse(ammo_wire_evidence(rows)["received_value_matches_storage_observed"])

    def test_changed_generation_channel_or_receive_cannot_join(self):
        for key, value in [("generation", 2), ("channel_index", 10), ("receive_id", 2), ("field_mask", 2)]:
            rows = fixture(); rows[-1][key] = value
            self.assertFalse(ammo_wire_evidence(rows)["received_value_matches_storage_observed"])

    def test_mismatch_is_retained_without_application_verdict(self):
        rows = fixture(); rows[-1]["used_after"] = 5
        result = ammo_wire_evidence(rows)
        self.assertFalse(result["received_value_matches_storage_observed"])
        self.assertEqual(result["observations"][0]["after"], 5)

    def test_completion_cannot_invent_decoded_value(self):
        rows = fixture(); rows[-1]["decoded_used"] = 8; rows[-1]["used_after"] = 8
        self.assertFalse(ammo_wire_evidence(rows)["received_value_matches_storage_observed"])

    def test_duplicate_decodes_are_ambiguous(self):
        rows = fixture(); rows.insert(3, copy.deepcopy(rows[2]))
        self.assertFalse(ammo_wire_evidence(rows)["received_value_matches_storage_observed"])

    def test_reversed_clock_or_duplicate_scope_cannot_pass(self):
        rows = fixture(); rows[-1]["monotonic_ms"] = 109
        self.assertFalse(ammo_wire_evidence(rows)["received_value_matches_storage_observed"])
        rows = fixture(); rows.insert(2, copy.deepcopy(rows[1]))
        self.assertFalse(ammo_wire_evidence(rows)["received_value_matches_storage_observed"])

    def test_limit_is_reported_without_sync_claim(self):
        result = ammo_wire_evidence(fixture() + [{"ammo_wire": "limit"}])
        self.assertTrue(result["record_limit_observed"])
        self.assertFalse(result["ammo_synchronization_verified"])


if __name__ == "__main__":
    unittest.main()
