"""Owned synthetic fixtures only; never reads/modifies an installed game."""
import copy
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import sys
import struct
import tempfile
import unittest
from unittest import mock

PROJECT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PROJECT / "src"))
import package_manifest as package


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


class PackageTests(unittest.TestCase):
    def setUp(self):
        scratch = PROJECT / "local" / "package-tests"
        scratch.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=scratch)
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.bundle = self.root / "bundle"
        self.payload = self.bundle / "payload"
        self.payload.mkdir(parents=True)
        self.game = self.root / "synthetic-test-copy"
        self.game.mkdir()
        self.original = self.root / "synthetic-original"
        self.original.mkdir()
        exe = package.safe_path(self.game, package.EXE)
        exe.parent.mkdir(parents=True)
        exe.write_bytes(b"SYNTHETIC EXECUTABLE, NEVER RUN")
        marker = {"status": "verified-copy", "destination": str(self.game.resolve()),
                  "source": str(self.original.resolve()), "fixture_test": True}
        (self.game / package.SANDBOX_MARKER).write_text(json.dumps(marker), encoding="utf-8")
        self.sentinel = self.game / "Binaries/Win64/ASI/lemp.asi"
        self.sentinel.parent.mkdir()
        self.sentinel.write_bytes(b"UNRELATED THIRD PARTY FILE")
        self.save = self.game / "Documents/Save/keep.pcsav"
        self.save.parent.mkdir(parents=True)
        self.save.write_bytes(b"SYNTHETIC SAVE MUST SURVIVE")
        self.native = "Binaries/Win64/ASI/ME3SquadCoop.asi"
        self.dlc = package.DLC_PREFIX + "ME3SquadCoop_Startup.pcc"
        self.manifest = {"schema": 1, "package_id": package.PACKAGE_ID, "version": "0.0.1-fixture",
                         "channel": "fixture", "release_ready": False, "qa": {}, "qa_evidence": {},
                         "supported_game_sha256": package.digest(exe), "files": []}
        for target, raw, kind in ((self.native, b"SYNTHETIC AUTHORED NATIVE FIXTURE", "native"),
                                  (self.dlc, b"SYNTHETIC AUTHORED DLC FIXTURE", "dlc")):
            p = package.safe_path(self.payload, target)
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_bytes(raw)
            self.manifest["files"].append({"path": target, "sha256": sha(raw), "size": len(raw),
                                           "kind": kind, "origin": "authored"})
        self.before = self.inventory()
        # Live processes are unrelated to a synthetic fixture; exercise the guard separately below.
        self.closed = mock.patch.object(package, "ensure_game_closed")
        self.closed.start()
        self.addCleanup(self.closed.stop)

    def inventory(self):
        return {p.relative_to(self.game).as_posix(): package.digest(p) for p in self.game.rglob("*") if p.is_file()}

    def install(self):
        return package.install(self.bundle, self.manifest, self.game, allow_fixture=True)

    def remove(self):
        return package.remove(self.game, expected_manifest_sha256=package.manifest_hash(self.manifest), allow_fixture=True)

    def test_install_remove_preserves_every_preexisting_file(self):
        result = self.install()
        self.assertEqual(result["files"], 2)
        self.assertEqual(self.sentinel.read_bytes(), b"UNRELATED THIRD PARTY FILE")
        self.assertEqual(self.remove()["files"], 2)
        self.assertEqual(self.inventory(), self.before)

    def test_running_game_blocks_install_and_remove_without_mutation(self):
        package.ensure_game_closed.side_effect = RuntimeError("game running")
        with self.assertRaises(RuntimeError):
            self.install()
        self.assertEqual(self.inventory(), self.before)
        package.ensure_game_closed.side_effect = None
        self.install()
        before = self.inventory()
        package.ensure_game_closed.side_effect = RuntimeError("game running")
        with self.assertRaises(RuntimeError):
            self.remove()
        self.assertEqual(self.inventory(), before)

    def test_existing_target_conflict_rejects_entire_install_before_mutation(self):
        target = package.safe_path(self.game, self.dlc)
        target.parent.mkdir(parents=True)
        target.write_bytes(b"USER CONTENT")
        expected = self.inventory()
        with self.assertRaises(FileExistsError):
            self.install()
        self.assertEqual(self.inventory(), expected)
        self.assertFalse(package.safe_path(self.game, self.native).exists())

    def test_changed_component_blocks_entire_remove_and_preserves_every_file(self):
        self.install()
        package.safe_path(self.game, self.dlc).write_bytes(b"USER EDIT")
        expected = self.inventory()
        with self.assertRaisesRegex(ValueError, "changed component preserved"):
            self.remove()
        self.assertEqual(self.inventory(), expected)

    def test_unrelated_new_files_survive_remove(self):
        self.install()
        unrelated = package.safe_path(self.game, package.DLC_PREFIX + "another-mod.txt")
        unrelated.write_bytes(b"KEEP")
        self.remove()
        self.assertEqual(unrelated.read_bytes(), b"KEEP")

    def test_missing_owned_file_is_harmless_on_remove(self):
        self.install()
        package.safe_path(self.game, self.native).unlink()
        self.assertEqual(self.remove()["files"], 1)
        self.assertEqual(self.inventory(), self.before)

    def test_payload_tamper_rejected_before_install(self):
        package.safe_path(self.payload, self.native).write_bytes(b"TAMPER")
        with self.assertRaises(ValueError):
            self.install()
        self.assertEqual(self.inventory(), self.before)

    def test_unmanifested_game_assets_cannot_hide_in_payload(self):
        (self.payload / "SFXGame.pcc").write_bytes(b"GAME DATA")
        with self.assertRaisesRegex(ValueError, "file set"):
            self.install()
        self.assertEqual(self.inventory(), self.before)

    def test_fixed_allowlist_rejects_game_saves_activation_lemp_and_loader(self):
        for target in (package.EXE, "Binaries/Win64/Activation64.dll", "Binaries/Win64/ASI/lemp.asi",
                       "Binaries/Win64/bink2w64.dll", "Documents/Save/user.pcsav",
                       "BioGame/CookedPCConsole/SFXGame.pcc", package.DLC_PREFIX + "BioP_Global.pcc"):
            with self.subTest(target=target), self.assertRaises(ValueError):
                package.allowed_target(target, "native" if target.endswith(".asi") else "dlc")

    def test_windows_path_escape_and_aliases_are_rejected(self):
        for target in ("../outside", "/absolute", "C:/outside", "a\\b", "a//b", "a/../b",
                       "a/b:stream", "a/NUL.txt", "a/file.", "a/COM1"):
            with self.subTest(target=target), self.assertRaises(ValueError):
                package.safe_path(self.game, target)

    def test_case_colliding_duplicate_manifest_entries_rejected(self):
        self.manifest["files"].append(copy.deepcopy(self.manifest["files"][0]))
        with self.assertRaises(ValueError):
            self.install()
        self.assertEqual(self.inventory(), self.before)

    def test_fixture_cannot_be_installed_by_public_default_api(self):
        with self.assertRaises(ValueError):
            package.install(self.bundle, self.manifest, self.game)
        self.assertEqual(self.inventory(), self.before)

    def test_fixture_requires_explicit_synthetic_target_marker(self):
        p = self.game / package.SANDBOX_MARKER
        marker = json.loads(p.read_text())
        marker.pop("fixture_test")
        p.write_text(json.dumps(marker))
        before = self.inventory()
        with self.assertRaises(ValueError):
            self.install()
        self.assertEqual(self.inventory(), before)

    def test_original_or_wrong_executable_rejected(self):
        for field, value in (("status", "incomplete"), ("destination", str(self.original)),
                             ("source", str(self.game.resolve()))):
            p = self.game / package.SANDBOX_MARKER
            old = p.read_text()
            marker = json.loads(old)
            marker[field] = value
            p.write_text(json.dumps(marker))
            with self.subTest(field=field), self.assertRaises(ValueError):
                self.install()
            p.write_text(old)
        package.safe_path(self.game, package.EXE).write_bytes(b"WRONG BUILD")
        with self.assertRaises(ValueError):
            self.install()

    def test_receipt_tamper_or_wrong_trusted_manifest_cannot_remove_files(self):
        self.install()
        before = self.inventory()
        with self.assertRaises(ValueError):
            package.remove(self.game, expected_manifest_sha256="0" * 64, allow_fixture=True)
        self.assertEqual(self.inventory(), before)
        receipt = self.game / package.RECEIPT
        record = json.loads(receipt.read_text())
        record["manifest"]["files"][0]["path"] = "Binaries/Win64/ASI/lemp.asi"
        receipt.write_text(json.dumps(record))
        expected = self.inventory()
        with self.assertRaises(ValueError):
            self.remove()
        self.assertEqual(self.inventory(), expected)

    def test_research_binary_rejected_even_when_renamed(self):
        raw = b"MZ fake payload with SquadProbeSelfTest export"
        package.safe_path(self.payload, self.native).write_bytes(raw)
        self.manifest["files"][0].update(sha256=sha(raw), size=len(raw))
        with self.assertRaisesRegex(ValueError, "research probe"):
            self.install()
        self.assertEqual(self.inventory(), self.before)

    def test_incomplete_qa_and_empty_placeholder_cannot_be_release(self):
        manifest = copy.deepcopy(self.manifest)
        manifest.update(channel="release", release_ready=True, supported_game_sha256=package.GAME_SHA256)
        with self.assertRaisesRegex(ValueError, "QA"):
            package.validate_manifest(manifest)
        manifest["qa"] = dict.fromkeys(package.REQUIRED_QA, True)
        manifest["qa_evidence"] = dict.fromkeys(package.REQUIRED_QA, "1" * 64)
        with self.assertRaisesRegex(ValueError, "mount"):
            package.validate_manifest(manifest)
        manifest["files"] = []
        with self.assertRaises(ValueError):
            package.validate_manifest(manifest)

    def test_private_metadata_is_rejected(self):
        self.manifest["private_install_path"] = "private-machine-folder"
        with self.assertRaisesRegex(ValueError, "private paths"):
            self.install()

    def candidate_fixture(self):
        """Structural PE/DLC bytes for packaging tests only; never runnable game payload."""
        raw = bytearray(1024)
        raw[:2] = b"MZ"
        struct.pack_into("<I", raw, 0x3C, 0x80)
        raw[0x80:0x84] = b"PE\0\0"
        struct.pack_into("<H", raw, 0x84, 0x8664)
        struct.pack_into("<H", raw, 0x96, 0x2000)
        struct.pack_into("<H", raw, 0x98, 0x20B)
        package.safe_path(self.payload, self.native).write_bytes(raw)
        self.manifest["files"][0].update(sha256=sha(raw), size=len(raw))
        mount = package.DLC_PREFIX + "Mount.dlc"
        mount_bytes = b"SYNTHETIC MOUNT FIXTURE; NOT A WORKING GAME FILE"
        package.safe_path(self.payload, mount).write_bytes(mount_bytes)
        self.manifest["files"].append({"path": mount, "sha256": sha(mount_bytes), "size": len(mount_bytes),
                                       "kind": "dlc", "origin": "authored"})
        self.manifest.update(channel="candidate", qa=dict.fromkeys(package.REQUIRED_QA, False))
        # Candidate remains exact-build gated; only this fixture's constant is substituted in the test.
        patched = mock.patch.object(package, "GAME_SHA256", self.manifest["supported_game_sha256"])
        patched.start()
        self.addCleanup(patched.stop)
        return self.manifest

    def test_candidate_can_verify_without_claiming_installable_release(self):
        self.candidate_fixture()
        package.verify_payload(self.bundle, self.manifest, installable=False)
        with self.assertRaisesRegex(ValueError, "explicit QA-only"):
            package.install(self.bundle, self.manifest, self.game)
        self.assertEqual(self.inventory(), self.before)

    def test_candidate_explicit_test_install_and_remove_preserve_all_other_files(self):
        self.candidate_fixture()
        result = package.install(self.bundle, self.manifest, self.game, allow_candidate=True)
        self.assertEqual(result["channel"], "candidate")
        self.assertFalse(result["release_ready"])
        receipt = json.loads((self.game / package.RECEIPT).read_text())
        self.assertTrue(receipt["qa_candidate"])
        package.remove(self.game, expected_manifest_sha256=package.manifest_hash(self.manifest))
        self.assertEqual(self.inventory(), self.before)

    def test_candidate_flag_does_not_relax_original_target_or_research_binary_rejection(self):
        self.candidate_fixture()
        with self.assertRaises(FileNotFoundError):
            package.install(self.bundle, self.manifest, self.original, allow_candidate=True)
        raw = b"RESEARCH SquadProbeSelfTest"
        package.safe_path(self.payload, self.native).write_bytes(raw)
        self.manifest["files"][0].update(sha256=sha(raw), size=len(raw))
        with self.assertRaisesRegex(ValueError, "research probe"):
            package.install(self.bundle, self.manifest, self.game, allow_candidate=True)
        self.assertEqual(self.inventory(), self.before)

    def test_candidate_cannot_pretend_release_ready_or_skip_real_components(self):
        self.candidate_fixture()
        self.manifest["release_ready"] = True
        with self.assertRaises(ValueError):
            package.validate_manifest(self.manifest, installable=False)
        self.manifest["release_ready"] = False
        self.manifest["files"] = self.manifest["files"][:1]
        with self.assertRaisesRegex(ValueError, "real authored DLC"):
            package.validate_manifest(self.manifest, installable=False)

    def test_release_promotion_stays_blocked_without_qa_and_artifact_fingerprints(self):
        self.candidate_fixture()
        self.manifest.update(channel="release", release_ready=True)
        with self.assertRaisesRegex(ValueError, "QA is incomplete"):
            package.validate_manifest(self.manifest, installable=False)
        self.manifest["qa"] = dict.fromkeys(package.REQUIRED_QA, True)
        with self.assertRaisesRegex(ValueError, "actual evidence artifact"):
            package.validate_manifest(self.manifest, installable=False)

    def test_builder_can_generate_honest_qa_candidate_before_import_tests(self):
        self.candidate_fixture()
        spec = importlib.util.spec_from_file_location("build_shareable", PROJECT / "tools/build_shareable.py")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        description = {"version": "0.0.1-test.candidate", "release_ready": False, "qa": {},
                       "components": [{"source": item["path"], "target": item["path"], "kind": item["kind"],
                                       "origin": "authored"} for item in self.manifest["files"]]}
        spec_path = self.root / "spec.json"
        spec_path.write_text(json.dumps(description))
        baseline = self.root / "synthetic-baseline.json"
        baseline.write_text(json.dumps({"status": "verified-copy", "files": [{"sha256": package.GAME_SHA256}]}))
        destination = self.root / "candidate"
        result = module.build(self.payload, spec_path, destination, baseline)
        self.assertEqual(result["channel"], "candidate")
        self.assertFalse(result["release_ready"])
        self.assertEqual(set(result["incomplete_qa"]), set(package.REQUIRED_QA))
        manifest = json.loads((destination / "manifest.json").read_text())
        self.assertTrue(all(value is False for value in manifest["qa"].values()))
        self.assertIn("unfinished", (destination / "PACKAGE-STATUS.md").read_text())
        with self.assertRaises(ValueError):
            package.install(destination, manifest, self.game)
        # Original/third-party hash exclusion applies equally to candidates.
        inventory = json.loads(baseline.read_text())
        inventory["files"].append({"sha256": self.manifest["files"][1]["sha256"]})
        baseline.write_text(json.dumps(inventory))
        with self.assertRaisesRegex(ValueError, "cannot be redistributed"):
            module.build(self.payload, spec_path, self.root / "blocked-candidate", baseline)
        self.assertFalse((self.root / "blocked-candidate").exists())

    def test_symbolic_target_ancestry_rejected_if_supported(self):
        target = self.game / "BioGame"
        outside = self.root / "outside"
        outside.mkdir()
        try:
            target.symlink_to(outside, target_is_directory=True)
        except OSError as error:
            self.skipTest(f"symlink privilege unavailable: {error}")
        with self.assertRaises(ValueError):
            self.install()
        self.assertEqual(list(outside.iterdir()), [])

    def test_scaffold_has_no_native_game_assets_or_install_manifest(self):
        spec = importlib.util.spec_from_file_location("build_shareable", PROJECT / "tools/build_shareable.py")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        destination = self.root / "scaffold"
        result = module.scaffold(destination)
        self.assertFalse(result["installable"])
        self.assertFalse(result["playable_mod"])
        self.assertEqual({p.name for p in destination.iterdir()},
                         {"SCAFFOLD-NOT-A-MOD.md", "package_manifest.py", "manage.py"})
        with self.assertRaises(FileExistsError):
            module.scaffold(destination)

    def test_deployment_archive_and_reversibility(self):
        if not (PROJECT / "local/build/ME3SquadCoop.asi").is_file():
            self.skipTest("build native ASI before running archive integration test")
        spec = importlib.util.spec_from_file_location("build_shareable", PROJECT / "tools/build_shareable.py")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        archive_path = self.root / "test-deployment.7z"
        result = module.build_deployment_archive(archive_path)
        self.assertTrue(result["archive_verified"])
        self.assertTrue(archive_path.is_file())
        self.assertGreater(archive_path.stat().st_size, 0)
        self.assertEqual(result["sha256"], package.digest(archive_path))
        for comp in ("Binaries/Win64/ASI/ME3SquadCoop.asi",
                     "BioGame/DLC/DLC_MOD_ME3SquadCoop/CookedPCConsole/Mount.dlc",
                     "BioGame/DLC/DLC_MOD_ME3SquadCoop/CookedPCConsole/ME3SquadCoop_Startup.pcc"):
            self.assertIn(comp, result["components"])


if __name__ == "__main__":
    unittest.main()

