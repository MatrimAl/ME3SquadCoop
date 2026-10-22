import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))
from ammo_reconcile_verification import ammo_reconcile_evidence


def fixture():
    def row(event, side, used, at, reason):
        return dict(ammo_reconcile=event, side=side, request=4294967297, generation=1,
                    channel_index=26, used=used, spare=210, monotonic_ms=at, reason=reason)
    host = [row("received-request", "host", 0, 10000, "native-owner-pc-rpc"),
            row("replied", "host", 2, 10300, "settled-host-snapshot")]
    guest = [row("requested", "guest", 3, 1000, "settled-guest-weapon"),
             row("applied", "guest", 2, 1350, "native-owner-pc-response")]
    return host, guest


class ReconciliationEvidenceTests(unittest.TestCase):
    def test_distinct_local_clocks_do_not_require_synchronization(self):
        result = ammo_reconcile_evidence(*fixture())
        self.assertTrue(result["reconciliation_roundtrip_verified"])
        self.assertTrue(result["difference_corrected_observed"])
        self.assertEqual(result["roundtrips"][0]["guest_roundtrip_ms"], 350)
        self.assertFalse(result["firing_verified"])
        self.assertFalse(result["full_combat_verified"])

    def test_zero_authoritative_count_cannot_prove_firing(self):
        host, guest = fixture(); host[-1]["used"] = guest[-1]["used"] = 0
        result = ammo_reconcile_evidence(host, guest)
        self.assertTrue(result["difference_corrected_observed"])
        self.assertFalse(result["firing_verified"])

    def test_each_native_stage_is_required(self):
        for side in (0, 1):
            for index in (0, 1):
                traces = fixture(); traces[side].pop(index)
                self.assertFalse(ammo_reconcile_evidence(*traces)["reconciliation_roundtrip_verified"])

    def test_changed_identity_snapshot_and_metadata_fail(self):
        for key, value in [("request", 4294967298), ("generation", 2), ("channel_index", 27),
            ("used", 8), ("spare", 209), ("used", True), ("reason", "unverified"),
            ("side", "host"), ("monotonic_ms", -1), ("monotonic_ms", 7000)]:
            traces = fixture(); traces[1][-1][key] = value
            with self.subTest(key=key, value=value):
                self.assertFalse(ammo_reconcile_evidence(*traces)["reconciliation_roundtrip_verified"])

    def test_duplicate_stage_is_ambiguous(self):
        for side in (0, 1):
            traces = fixture(); traces[side].append(copy.deepcopy(traces[side][-1]))
            self.assertFalse(ammo_reconcile_evidence(*traces)["reconciliation_roundtrip_verified"])

    def test_already_matching_storage_is_not_a_mod_correction(self):
        traces = fixture(); traces[1][-1]["ammo_reconcile"] = "already-matched"
        result = ammo_reconcile_evidence(*traces)
        self.assertTrue(result["reconciliation_roundtrip_verified"])
        self.assertFalse(result["difference_corrected_observed"])

    def test_native_exception_blocks_positive_roundtrip(self):
        traces = fixture(); traces[0].append({"exception": "fixture"})
        self.assertFalse(ammo_reconcile_evidence(*traces)["reconciliation_roundtrip_verified"])


if __name__ == "__main__":
    unittest.main()
