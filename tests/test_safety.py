import contextlib
import io
import json
import os
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))
import safe_workspace as s


class SafetyTests(unittest.TestCase):
    def setUp(self):
        temp_root = Path(__file__).resolve().parents[1] / "local" / "test-tmp"
        temp_root.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=temp_root)
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.original = self.root / "original"
        (self.original / "Binaries" / "Win64").mkdir(parents=True)
        self.exe = self.original / "Binaries" / "Win64" / "MassEffect3.exe"
        self.exe.write_bytes(b"SYNTHETIC TEST DATA")
        self.copy = self.root / "test-copy"

    def prepare(self):
        with contextlib.redirect_stdout(io.StringIO()):
            return s.prepare(self.original, self.copy)

    def verify(self, original=False):
        with contextlib.redirect_stdout(io.StringIO()):
            return s.verify(self.copy, original)

    def test_independent_copy_original_survives_changes(self):
        self.prepare()
        target = self.copy / self.exe.relative_to(self.original)
        self.assertNotEqual(self.exe.stat().st_ino, target.stat().st_ino)
        target.write_bytes(b"test modification")
        self.assertEqual(self.exe.read_bytes(), b"SYNTHETIC TEST DATA")
        self.assertEqual(self.verify(), ["Binaries/Win64/MassEffect3.exe"])
        self.assertEqual(self.verify(original=True), [])

    def test_no_overwrite_of_existing_destination(self):
        self.copy.mkdir()
        sentinel = self.copy / "user-file"
        sentinel.write_text("keep")
        with self.assertRaises(FileExistsError):
            self.prepare()
        self.assertEqual(sentinel.read_text(), "keep")

    def test_reject_source_destination_overlap(self):
        for destination in (self.original, self.original / "nested", self.root):
            with self.subTest(destination=destination), self.assertRaises(ValueError):
                s.prepare(self.original, destination)

    def test_reject_manifest_path_escape(self):
        for rel in ("../outside", "/absolute", "C:/Windows/x", "a\\b", "a/../b", "a//b"):
            with self.subTest(rel=rel), self.assertRaises(ValueError):
                s.safe_path(self.original, rel)

    def test_incomplete_sandbox_is_not_verified(self):
        self.prepare()
        marker = json.loads((self.copy / s.MARKER).read_text())
        marker["status"] = "incomplete"
        (self.copy / s.MARKER).write_text(json.dumps(marker))
        with self.assertRaises(ValueError):
            self.verify()

    def test_detects_extra_and_missing_files(self):
        self.prepare()
        (self.copy / "injected-file").write_bytes(b"extra")
        (self.copy / self.exe.relative_to(self.original)).unlink()
        self.assertEqual(set(self.verify()), {"Binaries/Win64/MassEffect3.exe", "injected-file"})

    def test_detects_original_modification(self):
        self.prepare()
        self.exe.write_bytes(b"external change")
        self.assertEqual(self.verify(original=True), ["Binaries/Win64/MassEffect3.exe"])

    def test_rejects_symlink_when_creation_is_available(self):
        try:
            os.symlink(self.exe, self.original / "linked")
        except OSError:
            self.skipTest("Windows symlink creation is unavailable")
        with self.assertRaises(ValueError):
            self.prepare()


if __name__ == "__main__":
    unittest.main(verbosity=2)
