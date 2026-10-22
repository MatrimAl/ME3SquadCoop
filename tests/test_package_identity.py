"""Synthetic identity evidence tests. No game, native DLL, or network access."""
import copy
import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

TOOL = Path(__file__).resolve().parents[1] / "tools" / "compare_package_identity.py"
SPEC = importlib.util.spec_from_file_location("compare_package_identity", TOOL)
identity = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(identity)


def snapshot():
    rows = []
    for package in identity.PACKAGES:
        rows.append({"identity": "package", "package": package, "guid": "00000001-00000002-00000003-00000004",
                     "generation_counts": [3], "linker": {"present": False}})
        rows.append({"identity": "net-table", "package": package, "offset": 0xa4, "count": 3,
                     "nonnull": 3, "rooted": 3, "zero_based_matches": 3, "one_based_matches": 0})
        for slot in range(3):
            rows.append({"identity": "net-object", "package": package, "offset": 0xa4, "slot": slot,
                         "path": f"{package}.Object{slot}", "class": "Class", "net_index": slot, "nonnull": True})
        rows.append({"identity": "package-summary", "package": package, "candidate_tables": 1})
    return {"complete": True, "phase": "synthetic", "errors": [], "rows": rows}


def mutate(snap, kind, package="SFXGameContent"):
    return next(row for row in snap["rows"] if row["identity"] == kind and row["package"] == package)


class IdentityComparison(unittest.TestCase):
    def setUp(self):
        self.host = snapshot()
        self.client = snapshot()

    def result(self):
        return identity.compare(self.host, self.client)

    def test_equal_maps_are_hashed_without_process_addresses(self):
        mutate(self.host, "package")["private_raw_60_110"] = "deadbeef"
        result = self.result()
        content = result["packages"]["SFXGameContent"]
        self.assertEqual(content["status"], "equivalent-observed-identity")
        self.assertTrue(result["net_index_oracle_calibrated"])
        self.assertFalse(result["repair_authorized"])
        self.assertFalse(result["campaign_coop_verified"])
        self.assertNotIn("deadbeef", json.dumps(result))
        self.assertEqual(content["table_evidence"]["host"][0]["semantic_sha256"],
                         content["table_evidence"]["client"][0]["semantic_sha256"])

    def test_one_changed_slot_path_is_not_equivalent(self):
        mutate(self.client, "net-object")["path"] = "SFXGameContent.DifferentObject"
        result = self.result()["packages"]["SFXGameContent"]
        self.assertEqual(result["status"], "different-semantic-map")
        self.assertEqual(len(result["map_differences"]), 1)
        self.assertEqual(result["map_differences"][0]["slot"], 0)

    def test_guid_only_mismatch_requires_equal_maps_and_generations(self):
        mutate(self.client, "package")["guid"] = "00000005-00000006-00000007-00000008"
        result = self.result()["packages"]["SFXGameContent"]
        self.assertEqual(result["status"], "guid-only-mismatch")
        self.assertTrue(result["fully_comparable"])
        self.assertTrue(result["semantic_map_equal"])

    def test_missing_row_cannot_be_claimed_equivalent(self):
        self.client["rows"].remove(mutate(self.client, "net-object"))
        result = self.result()["packages"]["SFXGameContent"]
        self.assertFalse(result["fully_comparable"])
        self.assertIsNone(result["semantic_map_equal"])
        self.assertEqual(result["status"], "incomplete-or-invalid-evidence")

    def test_duplicate_slot_even_when_identical_is_corruption(self):
        self.client["rows"].append(copy.deepcopy(mutate(self.client, "net-object")))
        result = self.result()["packages"]["SFXGameContent"]
        self.assertFalse(result["fully_comparable"])
        self.assertIn("duplicate slot 0", result["table_evidence"]["client"][0]["errors"])

    def test_missing_package_and_missing_table_are_distinct(self):
        self.client["rows"] = [row for row in self.client["rows"] if row["package"] != "SFXGameContent"]
        self.assertEqual(self.result()["packages"]["SFXGameContent"]["status"], "missing-package")
        self.client = snapshot()
        self.client["rows"] = [row for row in self.client["rows"]
                               if not (row["package"] == "SFXGameContent" and row["identity"] in ("net-table", "net-object"))]
        self.assertEqual(self.result()["packages"]["SFXGameContent"]["status"], "missing-table")

    def test_mixed_net_indices_cannot_calibrate_oracle(self):
        mutate(self.client, "net-object", "Core")["net_index"] = 88
        result = self.result()
        self.assertFalse(result["net_index_oracle_calibrated"])
        self.assertEqual(result["packages"]["SFXGameContent"]["status"], "oracle-uncalibrated")

    def test_one_based_convention_verified_from_both_controls(self):
        for snap in (self.host, self.client):
            for row in snap["rows"]:
                if row["identity"] == "net-object":
                    row["net_index"] += 1
                elif row["identity"] == "net-table":
                    row.update(zero_based_matches=0, one_based_matches=3)
        result = self.result()
        self.assertTrue(result["net_index_oracle_calibrated"])
        self.assertEqual(result["net_index_calibration"]["host"]["convention"], "one-based")

    def test_different_generation_arrays_are_not_guid_only(self):
        mutate(self.client, "package")["generation_counts"] = [2, 3]
        self.assertEqual(self.result()["packages"]["SFXGameContent"]["status"], "different-generations")

    def test_empty_generation_array_is_uncomparable(self):
        mutate(self.client, "package")["generation_counts"] = []
        result = self.result()
        self.assertFalse(result["packages"]["SFXGameContent"]["fully_comparable"])
        self.assertFalse(result["all_tables_and_generations_comparable"])

    def test_less_than_90_percent_rooted_is_rejected(self):
        row = mutate(self.client, "net-object")
        row.update(path="", **{"class": "", "net_index": -1})
        mutate(self.client, "net-table").update(rooted=2, zero_based_matches=2)
        self.assertFalse(self.result()["packages"]["SFXGameContent"]["fully_comparable"])

    def test_90_percent_rooted_does_not_make_unreadable_nonnull_row_comparable(self):
        header = {"offset": 0xa4, "count": 10, "nonnull": 10, "rooted": 9,
                  "zero_based_matches": 9, "one_based_matches": 0}
        rows = [{"slot": slot, "path": f"Core.Object{slot}", "class": "Class", "net_index": slot,
                 "nonnull": True} for slot in range(10)]
        rows[-1].update(path="", **{"class": "", "net_index": -1})
        result = identity.inspect_table(header, rows, "Core")
        self.assertFalse(result["valid"])
        self.assertIn("unresolved non-null semantic row 9", result["errors"])

    def test_incomplete_snapshot_is_ignored_and_no_complete_is_reported(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "capture.jsonl"
            records = [{"identity": "snapshot-begin", "phase": "host-listening"},
                       {"identity": "package", "phase": "host-listening", "package": "Core", "guid": "first"},
                       {"identity": "snapshot-end", "phase": "host-listening"},
                       {"identity": "snapshot-begin", "phase": "host-listening"},
                       {"identity": "package", "phase": "host-listening", "package": "Core", "guid": "incomplete"}]
            path.write_text("\n".join(json.dumps(row) for row in records), encoding="utf-8")
            selected = identity.select_snapshot(path, "host-listening")
            self.assertTrue(selected["complete"])
            self.assertEqual(selected["rows"][0]["guid"], "first")
            self.assertFalse(identity.select_snapshot(path, "client-lookup-failure")["complete"])

    def test_filename_candidates_calibrate_with_readable_controls_not_comments(self):
        with tempfile.TemporaryDirectory() as temporary:
            snap = snapshot()
            for name in ("Core", "SFXGame"):
                package = Path(temporary) / (name + ".pcc")
                package.write_bytes(b"synthetic nonempty package control")
                mutate(snap, "package", name)["linker"] = {
                    "present": True, "class": "LinkerLoad", "root": name,
                    "candidate_194": "misleading comment", "candidate_194_exists": True,
                    "candidate_198": str(package), "candidate_198_exists": True}
            inspected = {name: identity.inspect_package(snap, name) for name in identity.PACKAGES}
            calibrated = identity.calibrate_linker(inspected)
            self.assertTrue(calibrated["calibrated"])
            self.assertEqual(calibrated["candidate"], "198")
            mutate(snap, "package", "Core")["linker"]["root"] = "WrongRoot"
            inspected["Core"] = identity.inspect_package(snap, "Core")
            self.assertFalse(identity.calibrate_linker(inspected)["calibrated"])

    def test_relative_filename_requires_the_captured_game_working_directory(self):
        with tempfile.TemporaryDirectory() as temporary:
            snap = snapshot()
            for name in ("Core", "SFXGame"):
                (Path(temporary) / (name + ".pcc")).write_bytes(b"synthetic")
                mutate(snap, "package", name)["linker"] = {
                    "present": True, "class": "LinkerLoad", "root": name,
                    "candidate_194": name + ".pcc", "candidate_194_exists": True}
            inspected = {name: identity.inspect_package(snap, name) for name in identity.PACKAGES}
            self.assertTrue(identity.calibrate_linker(inspected, temporary)["calibrated"])


if __name__ == "__main__":
    unittest.main()
