"""Owned synthetic fixtures only; never reads/modifies an installed game."""
import copy
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import sys
import struct
import subprocess
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
        spec = importlib.util.spec_from_file_location("build_shareable", PROJECT / "tools/build_shareable.py")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        if not module.find_zig_compiler() or not module.find_seven_zip():
            self.skipTest("Zig compiler and 7-Zip required for archive fixture test")
        archive_path = self.root / "test-deployment.7z"
        result = module.build_deployment_archive(archive_path)
        self.assertTrue(result["archive_verified"])
        self.assertTrue(archive_path.is_file())
        self.assertGreater(archive_path.stat().st_size, 0)
        self.assertEqual(result["sha256"], package.digest(archive_path))
        self.assertFalse(result["installable"])
        self.assertFalse(result["campaign_runtime_included"])
        self.assertEqual(result["channel"], "fixture")
        for comp in ("Binaries/Win64/ASI/ME3SquadCoop.asi",
                     "BioGame/DLC/DLC_MOD_ME3SquadCoop/CookedPCConsole/Mount.dlc",
                     "BioGame/DLC/DLC_MOD_ME3SquadCoop/CookedPCConsole/ME3SquadCoop_Startup.pcc"):
            self.assertIn(comp, result["components"])
        extracted = self.root / "extracted"
        subprocess.run([module.find_seven_zip(), "x", str(archive_path), "-o" + str(extracted), "-y"],
                       check=True, capture_output=True)
        self.assertFalse((extracted / "moddesc.ini").exists())
        self.assertTrue((extracted / "research/moddesc.example.ini").is_file())
        archived_manifest = json.loads((extracted / "manifest.json").read_text(encoding="utf-8"))
        before = self.inventory()
        with self.assertRaisesRegex(ValueError, "scaffold/research"):
            package.install(extracted, archived_manifest, self.game, allow_candidate=True)
        self.assertEqual(self.inventory(), before)
        info = json.loads((extracted / "native/BUILD-INFO.json").read_text(encoding="utf-8"))
        self.assertEqual(info["artifact_sha256"], package.digest(extracted / "native/ME3SquadCoop.asi"))

    def test_packaging_compiler_discovery_finds_ci_verified_toolchain(self):
        spec = importlib.util.spec_from_file_location("build_shareable_ci", PROJECT / "tools/build_shareable.py")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        runner = self.root / "runner-temp"
        compiler = runner / "zig-x86_64-windows-0.15.2/zig.exe"
        compiler.parent.mkdir(parents=True)
        compiler.write_bytes(b"SYNTHETIC COMPILER FIXTURE; NEVER EXECUTED")
        with mock.patch.dict(os.environ, {"RUNNER_TEMP": str(runner), "ME3_SQUAD_COOP_ZIG": ""}):
            self.assertEqual(module.find_zig_compiler(), str(compiler.resolve()))

    def test_stale_native_packaging_fingerprint_rejected(self):
        spec = importlib.util.spec_from_file_location("build_shareable_fingerprint", PROJECT / "tools/build_shareable.py")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        artifact = self.root / "ME3SquadCoop.asi"
        artifact.write_bytes(b"SYNTHETIC NATIVE BYTES")
        info = {"schema": 1, "sources": module._packaging_sources(), "compiler_sha256": "1" * 64,
                "artifact_sha256": package.digest(artifact), "artifact_size": artifact.stat().st_size}
        artifact.with_suffix(".asi.build.json").write_text(json.dumps(info), encoding="utf-8")
        module._verify_packaging_build(artifact)
        artifact.write_bytes(b"OTHER NATIVE BYTES")
        with self.assertRaisesRegex(ValueError, "stale or unverified"):
            module._verify_packaging_build(artifact)

    def test_reinstall_rejected_if_installed_and_reinstall_flow(self):
        self.install()
        # Attempting install when receipt exists must fail closed
        with self.assertRaises(FileExistsError):
            self.install()
        # Reinstall helper cleanly reinstalls when components match expected state
        res = package.reinstall(self.bundle, self.manifest, self.game, allow_fixture=True)
        self.assertEqual(res["status"], "installed")
        self.assertEqual(res["files"], 2)
        # But if a file was modified, reinstall refuses to touch anything
        package.safe_path(self.game, self.dlc).write_bytes(b"MODIFIED")
        with self.assertRaisesRegex(ValueError, "modified components preserved"):
            package.reinstall(self.bundle, self.manifest, self.game, allow_fixture=True)

    def test_preexisting_collision_aborts_without_creating_receipt_or_mutating_files(self):
        target = package.safe_path(self.game, self.native)
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(b"ORIGINAL GAME FILE DO NOT OVERWRITE")
        before = self.inventory()
        with self.assertRaises(FileExistsError):
            self.install()
        # No receipt created, target untouched, no other files created
        self.assertEqual(self.inventory(), before)
        self.assertFalse((self.game / package.RECEIPT).exists())
        self.assertEqual(target.read_bytes(), b"ORIGINAL GAME FILE DO NOT OVERWRITE")

    def test_interrupted_incomplete_install_rollback_and_recovery(self):
        # Prepare an interrupted installation state where status is 'incomplete'
        # and only 1 file was copied before failure
        receipt_path = package.safe_path(self.game, package.RECEIPT)
        first_target = package.safe_path(self.game, self.manifest["files"][0]["path"])
        first_target.parent.mkdir(parents=True, exist_ok=True)
        first_target.write_bytes(package.safe_path(self.payload, self.manifest["files"][0]["path"]).read_bytes())
        created_dir = first_target.parent.relative_to(self.game).as_posix()
        receipt = {
            "schema": 1,
            "package_id": package.PACKAGE_ID,
            "manifest_sha256": package.manifest_hash(self.manifest),
            "manifest": self.manifest,
            "status": "incomplete",
            "created_directories": [created_dir],
            "installed_files": [{"path": self.manifest["files"][0]["path"],
                                 "sha256": self.manifest["files"][0]["sha256"],
                                 "size": self.manifest["files"][0]["size"]}],
            "qa_candidate": False,
        }
        package._write_json_new(receipt_path, receipt)
        # Normal install refuses to proceed over incomplete receipt
        with self.assertRaises(FileExistsError):
            self.install()
        # Inspection identifies incomplete status
        status = package.inspect_installation(self.game)
        self.assertEqual(status["status"], "incomplete")
        self.assertFalse(status["is_clean"])
        # Remove rolls back the incomplete installation cleanly
        res = package.remove(self.game, expected_manifest_sha256=package.manifest_hash(self.manifest),
                             allow_fixture=True)
        self.assertEqual(res["status"], "removed")
        self.assertFalse(receipt_path.exists())
        self.assertFalse(first_target.exists())
        # After rollback, install succeeds cleanly
        clean_res = self.install()
        self.assertEqual(clean_res["status"], "installed")

    def test_remove_cleans_empty_created_directories_and_preserves_non_empty(self):
        self.install()
        # Both created directories exist
        dlc_file = package.safe_path(self.game, self.dlc)
        dlc_parent = dlc_file.parent
        self.assertTrue(dlc_parent.is_dir())
        # Place an unrelated mod file in dlc_parent
        unrelated = dlc_parent / "unrelated_squadmate.ini"
        unrelated.write_bytes(b"[Config]\nEnabled=1\n")
        self.remove()
        # Mod files removed, but unrelated file and its containing directory are preserved
        self.assertFalse(dlc_file.exists())
        self.assertTrue(dlc_parent.is_dir())
        self.assertTrue(unrelated.is_file())
        self.assertEqual(unrelated.read_bytes(), b"[Config]\nEnabled=1\n")

    def test_inspect_installation_accurately_detects_all_states(self):
        # 1. Not installed
        st = package.inspect_installation(self.game)
        self.assertEqual(st["status"], "not_installed")
        self.assertFalse(st["receipt_present"])
        # 2. Installed clean
        self.install()
        st = package.inspect_installation(self.game)
        self.assertEqual(st["status"], "installed")
        self.assertTrue(st["is_clean"])
        self.assertEqual(len(st["files_verified"]), 2)
        # 3. Modified component
        package.safe_path(self.game, self.dlc).write_bytes(b"ALTERED")
        st = package.inspect_installation(self.game)
        self.assertEqual(st["status"], "diverged")
        self.assertFalse(st["is_clean"])
        self.assertIn(self.dlc, st["files_modified"])
        # 4. Missing component
        package.safe_path(self.game, self.dlc).unlink()
        st = package.inspect_installation(self.game)
        self.assertIn(self.dlc, st["files_missing"])

    def test_validate_native_rejects_diagnostic_probe_symbols(self):
        for marker in (b"SquadProbe", b"ME3SquadCoopProbe", b"SquadDirectIPSelfTest",
                       b"SquadGuestCameraSelfTest", b"SquadGuestCinematicSelfTest",
                       b"SquadSessionHandleSelfTest", b".me3-coop-sandbox.json"):
            fake_dll = b"MZ" + b"\0" * 60 + b"PE\0\0" + b"\x64\x86" + b"\0" * 16 + b"\0\x20" + b"\x0b\x02" + marker
            fake_path = self.root / "fake_probe.asi"
            fake_path.write_bytes(fake_dll)
            with self.subTest(marker=marker), self.assertRaisesRegex(ValueError, "research probe"):
                package.validate_native(fake_path)

    def test_invalid_replacement_preserves_existing_install_and_receipt(self):
        self.install()
        before = self.inventory()
        package.safe_path(self.payload, self.native).write_bytes(b"INVALID REPLACEMENT")
        with self.assertRaisesRegex(ValueError, "hash/size mismatch"):
            package.reinstall(self.bundle, self.manifest, self.game, allow_fixture=True)
        self.assertEqual(self.inventory(), before)

    def test_unowned_receipt_directory_rejected_before_any_remove(self):
        unrelated = self.game / "Documents/ExistingEmptyDirectory"
        unrelated.mkdir()
        self.install()
        receipt_path = self.game / package.RECEIPT
        receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
        receipt["created_directories"].append("Documents/ExistingEmptyDirectory")
        receipt_path.write_text(json.dumps(receipt), encoding="utf-8")
        before = self.inventory()
        with self.assertRaises(ValueError):
            self.remove()
        self.assertEqual(self.inventory(), before)
        self.assertTrue(unrelated.is_dir())

    def test_actual_copy_failure_persists_created_directory_journal(self):
        second = package.safe_path(self.game, self.dlc)
        original_open = Path.open

        def failing_open(path, mode="r", *args, **kwargs):
            if path == second and mode == "xb":
                raise OSError("injected second component failure")
            return original_open(path, mode, *args, **kwargs)

        with mock.patch.object(Path, "open", failing_open):
            with self.assertRaisesRegex(OSError, "second component failure"):
                self.install()
        receipt = json.loads((self.game / package.RECEIPT).read_text(encoding="utf-8"))
        self.assertEqual(receipt["status"], "incomplete")
        self.assertIn(second.parent.relative_to(self.game).as_posix(), receipt["created_directories"])
        self.remove()
        self.assertFalse(second.parent.exists())
        self.assertEqual(self.inventory(), self.before)

    def test_status_rejects_receipt_without_valid_identity_or_manifest(self):
        receipt = self.game / package.RECEIPT
        for value in ({"status": "installed"}, [], {"status": "installed", "manifest": {"files": []}}):
            with self.subTest(value=value):
                receipt.write_text(json.dumps(value), encoding="utf-8")
                status = package.inspect_installation(self.game)
                self.assertEqual(status["status"], "corrupt_receipt")
                self.assertFalse(status.get("is_clean", False))

    def replacement(self):
        original_hash = package.manifest_hash(self.manifest)
        replacement = copy.deepcopy(self.manifest)
        for item in replacement["files"]:
            target = package.safe_path(self.payload, item["path"])
            raw = target.read_bytes() + b" UPDATED"
            target.write_bytes(raw)
            item.update(size=len(raw), sha256=sha(raw))
        replacement["version"] = "0.0.2-fixture"
        return replacement, original_hash

    def test_reinstall_requires_trusted_old_hash_before_upgrading(self):
        self.install()
        before = self.inventory()
        replacement, old_hash = self.replacement()
        with self.assertRaisesRegex(ValueError, "trusted package manifest"):
            package.reinstall(self.bundle, replacement, self.game, allow_fixture=True)
        self.assertEqual(self.inventory(), before)
        result = package.reinstall(self.bundle, replacement, self.game, allow_fixture=True,
                                   expected_installed_manifest_sha256=old_hash)
        self.assertEqual(result["status"], "installed")
        self.assertEqual(package.inspect_installation(self.game)["version"], "0.0.2-fixture")
        self.assertFalse((self.game / package.TRANSACTION).exists())

    def test_reinstall_io_failure_rolls_back_bytes_and_receipt(self):
        self.install()
        before = self.inventory()
        replacement, old_hash = self.replacement()
        original_replace = package.os.replace
        second = package.safe_path(self.game, self.dlc)

        def failing_replace(source, target):
            if Path(target) == second and "new" in Path(source).parts:
                raise OSError("injected second replacement failure")
            return original_replace(source, target)

        with mock.patch.object(package.os, "replace", failing_replace):
            with self.assertRaisesRegex(OSError, "second replacement failure"):
                package.reinstall(self.bundle, replacement, self.game, allow_fixture=True,
                                   expected_installed_manifest_sha256=old_hash)
        self.assertEqual(self.inventory(), before)
        self.assertFalse((self.game / package.TRANSACTION).exists())

    def test_interrupted_reinstall_recovers_from_durable_verified_backups(self):
        class SimulatedTermination(BaseException):
            pass

        self.install()
        before = self.inventory()
        replacement, old_hash = self.replacement()
        original_replace = package.os.replace
        second = package.safe_path(self.game, self.dlc)

        def terminate(source, target):
            if Path(target) == second and "new" in Path(source).parts:
                raise SimulatedTermination()
            return original_replace(source, target)

        with mock.patch.object(package.os, "replace", terminate):
            with self.assertRaises(SimulatedTermination):
                package.reinstall(self.bundle, replacement, self.game, allow_fixture=True,
                                   expected_installed_manifest_sha256=old_hash)
        self.assertEqual(package.inspect_installation(self.game)["status"], "recovery_pending")
        result = package.recover_reinstall(self.game, expected_manifest_sha256=old_hash, allow_fixture=True)
        self.assertEqual(result["status"], "previous_install_restored")
        self.assertEqual(self.inventory(), before)

    def test_reinstall_staging_failure_never_mutates_installed_files(self):
        self.install()
        before = self.inventory()
        replacement, old_hash = self.replacement()
        original_copy = package._copy_verified

        def fail_new_stage(source, destination, item):
            if "new" in destination.relative_to(self.game).parts:
                raise OSError("injected staging failure")
            return original_copy(source, destination, item)

        with mock.patch.object(package, "_copy_verified", fail_new_stage):
            with self.assertRaisesRegex(OSError, "staging failure"):
                package.reinstall(self.bundle, replacement, self.game, allow_fixture=True,
                                   expected_installed_manifest_sha256=old_hash)
        self.assertEqual(self.inventory(), before)

    def test_unjournaled_partial_component_is_preserved_without_deleting_verified_files(self):
        self.install()
        receipt_path = self.game / package.RECEIPT
        receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
        receipt["status"] = "incomplete"
        receipt["installed_files"] = receipt["installed_files"][:1]
        receipt_path.write_text(json.dumps(receipt), encoding="utf-8")
        package.safe_path(self.game, self.dlc).write_bytes(b"PARTIAL UNVERIFIED BYTES")
        before = self.inventory()
        with self.assertRaisesRegex(ValueError, "unverified incomplete component"):
            self.remove()
        self.assertEqual(self.inventory(), before)

    def interrupted_replacement(self):
        class SimulatedTermination(BaseException):
            pass

        replacement, old_hash = self.replacement()
        original_replace = package.os.replace
        second = package.safe_path(self.game, self.dlc)

        def terminate(source, target):
            if Path(target) == second and "new" in Path(source).parts:
                raise SimulatedTermination()
            return original_replace(source, target)

        with mock.patch.object(package.os, "replace", terminate):
            with self.assertRaises(SimulatedTermination):
                package.reinstall(self.bundle, replacement, self.game, allow_fixture=True,
                                   expected_installed_manifest_sha256=old_hash)
        return replacement, old_hash

    def test_recovery_rejects_changed_backup_before_modifying_any_file(self):
        self.install()
        _, old_hash = self.interrupted_replacement()
        backup = self.game / package.TRANSACTION / "backup" / self.native
        backup.write_bytes(b"EXTERNALLY CHANGED BACKUP")
        before = self.inventory()
        with self.assertRaisesRegex(ValueError, "backup verification failed"):
            package.recover_reinstall(self.game, expected_manifest_sha256=old_hash, allow_fixture=True)
        self.assertEqual(self.inventory(), before)

    def test_recovery_preserves_changed_component_and_backups(self):
        self.install()
        _, old_hash = self.interrupted_replacement()
        package.safe_path(self.game, self.native).write_bytes(b"USER EDIT AFTER INTERRUPTION")
        before = self.inventory()
        with self.assertRaisesRegex(ValueError, "modified components preserved"):
            package.recover_reinstall(self.game, expected_manifest_sha256=old_hash, allow_fixture=True)
        self.assertEqual(self.inventory(), before)

    def test_recovery_continues_after_rollback_io_failure(self):
        self.install()
        before = self.inventory()
        replacement, old_hash = self.replacement()
        original_replace = package.os.replace

        def fail_replacement_and_rollback(source, target):
            source = Path(source)
            if (Path(target) == self.game / self.dlc and "new" in source.parts
                    or "restore" in source.parts):
                raise OSError("injected rollback storage failure")
            return original_replace(source, target)

        with mock.patch.object(package.os, "replace", fail_replacement_and_rollback):
            with self.assertRaisesRegex(RuntimeError, "backups are retained"):
                package.reinstall(self.bundle, replacement, self.game, allow_fixture=True,
                                   expected_installed_manifest_sha256=old_hash)
        package.recover_reinstall(self.game, expected_manifest_sha256=old_hash, allow_fixture=True)
        self.assertEqual(self.inventory(), before)

    def test_committed_replacement_recovery_finishes_cleanup_without_reverting(self):
        class SimulatedTermination(BaseException):
            pass

        self.install()
        replacement, old_hash = self.replacement()
        with mock.patch.object(package, "_transaction_cleanup", side_effect=SimulatedTermination):
            with self.assertRaises(SimulatedTermination):
                package.reinstall(self.bundle, replacement, self.game, allow_fixture=True,
                                   expected_installed_manifest_sha256=old_hash)
        result = package.recover_reinstall(self.game, expected_manifest_sha256=old_hash, allow_fixture=True)
        self.assertEqual(result["status"], "replacement_committed")
        self.assertEqual(package.inspect_installation(self.game)["version"], "0.0.2-fixture")

    def test_receipt_commit_before_journal_commit_can_restore_prior_install(self):
        class SimulatedTermination(BaseException):
            pass

        self.install()
        before = self.inventory()
        replacement, old_hash = self.replacement()
        original_replace = package.os.replace

        def terminate_before_commit_marker(source, target):
            if Path(target).name == "transaction.json":
                value = json.loads(Path(source).read_text(encoding="utf-8"))
                if value["status"] == "committed":
                    raise SimulatedTermination()
            return original_replace(source, target)

        with mock.patch.object(package.os, "replace", terminate_before_commit_marker):
            with self.assertRaises(SimulatedTermination):
                package.reinstall(self.bundle, replacement, self.game, allow_fixture=True,
                                   expected_installed_manifest_sha256=old_hash)
        package.recover_reinstall(self.game, expected_manifest_sha256=old_hash, allow_fixture=True)
        self.assertEqual(self.inventory(), before)

    def test_missing_candidate_permission_never_removes_existing_installation(self):
        self.candidate_fixture()
        package.install(self.bundle, self.manifest, self.game, allow_candidate=True)
        before = self.inventory()
        with self.assertRaisesRegex(ValueError, "explicit QA-only"):
            package.reinstall(self.bundle, self.manifest, self.game)
        self.assertEqual(self.inventory(), before)

    def test_replacement_conflict_with_unowned_new_component_preserves_everything(self):
        self.install()
        replacement, old_hash = self.replacement()
        extra = package.DLC_PREFIX + "ME3SquadCoop_Extra.pcc"
        raw = b"SYNTHETIC EXTRA AUTHORED COMPONENT"
        package.safe_path(self.payload, extra).write_bytes(raw)
        replacement["files"].append({"path": extra, "kind": "dlc", "origin": "authored",
                                      "size": len(raw), "sha256": sha(raw)})
        package.safe_path(self.game, extra).write_bytes(b"UNOWNED CONFLICT")
        before = self.inventory()
        with self.assertRaisesRegex(FileExistsError, "unowned component"):
            package.reinstall(self.bundle, replacement, self.game, allow_fixture=True,
                               expected_installed_manifest_sha256=old_hash)
        self.assertEqual(self.inventory(), before)

    def test_recovery_unknown_transaction_file_is_preserved_before_any_mutation(self):
        self.install()
        _, old_hash = self.interrupted_replacement()
        (self.game / package.TRANSACTION / "foreign-note.txt").write_bytes(b"PRESERVE ME")
        before = self.inventory()
        with self.assertRaisesRegex(ValueError, "unrecognized transaction content"):
            package.recover_reinstall(self.game, expected_manifest_sha256=old_hash, allow_fixture=True)
        self.assertEqual(self.inventory(), before)

    def test_mutation_apis_require_an_explicit_trusted_hash(self):
        self.install()
        before = self.inventory()
        with self.assertRaisesRegex(ValueError, "trusted package manifest fingerprint"):
            package.remove(self.game, expected_manifest_sha256=None, allow_fixture=True)
        with self.assertRaisesRegex(ValueError, "trusted old package manifest fingerprint"):
            package.recover_reinstall(self.game, expected_manifest_sha256=None, allow_fixture=True)
        self.assertEqual(self.inventory(), before)

    def test_cleanup_failure_retains_journal_and_committed_install_can_recover(self):
        self.install()
        replacement, old_hash = self.replacement()
        original_unlink = Path.unlink
        seen = 0

        def fail_second_backup_unlink(path, *args, **kwargs):
            nonlocal seen
            if package.TRANSACTION in path.parts and "backup" in path.parts:
                seen += 1
                if seen == 2:
                    raise OSError("injected cleanup failure")
            return original_unlink(path, *args, **kwargs)

        with mock.patch.object(Path, "unlink", fail_second_backup_unlink):
            with self.assertRaisesRegex(OSError, "cleanup failure"):
                package.reinstall(self.bundle, replacement, self.game, allow_fixture=True,
                                   expected_installed_manifest_sha256=old_hash)
        self.assertTrue((self.game / package.TRANSACTION / "transaction.json").is_file())
        result = package.recover_reinstall(self.game, expected_manifest_sha256=old_hash, allow_fixture=True)
        self.assertEqual(result["status"], "replacement_committed")

    def test_rollback_cleanup_failure_can_recover_already_restored_install(self):
        self.install()
        before = self.inventory()
        replacement, old_hash = self.replacement()
        original_unlink, original_replace = Path.unlink, package.os.replace
        seen = 0

        def fail_second_replacement(source, target):
            if Path(target) == self.game / self.dlc and "new" in Path(source).parts:
                raise OSError("injected replacement failure")
            return original_replace(source, target)

        def fail_second_backup_unlink(path, *args, **kwargs):
            nonlocal seen
            if package.TRANSACTION in path.parts and "backup" in path.parts:
                seen += 1
                if seen == 2:
                    raise OSError("injected rollback cleanup failure")
            return original_unlink(path, *args, **kwargs)

        with mock.patch.object(Path, "unlink", fail_second_backup_unlink), \
                mock.patch.object(package.os, "replace", fail_second_replacement):
            with self.assertRaisesRegex(RuntimeError, "backups are retained"):
                package.reinstall(self.bundle, replacement, self.game, allow_fixture=True,
                                   expected_installed_manifest_sha256=old_hash)
        package.recover_reinstall(self.game, expected_manifest_sha256=old_hash, allow_fixture=True)
        self.assertEqual(self.inventory(), before)


if __name__ == "__main__":
    unittest.main()

