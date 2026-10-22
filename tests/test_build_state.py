import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))
from build_state import record_current, verify_current


class BuildState(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        for name in ("build.py", "src/probe.c", "src/layout.h", "local/build/ME3SquadCoopProbe.asi", "local/build/sandbox_launch.exe"):
            path = self.root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(b"fixture")
        record_current(self.root)

    def test_source_header_change_rejects_old_binary(self):
        verify_current(self.root)
        (self.root / "src/layout.h").write_bytes(b"changed")
        with self.assertRaises(RuntimeError):
            verify_current(self.root)

    def test_partial_binary_replacement_rejected(self):
        (self.root / "local/build/ME3SquadCoopProbe.asi").write_bytes(b"new but unrecorded")
        with self.assertRaises(RuntimeError):
            verify_current(self.root)

    def test_failed_build_without_success_stamp_rejected(self):
        (self.root / "local/build/current-build.json").unlink()
        with self.assertRaises(FileNotFoundError):
            verify_current(self.root)


if __name__ == "__main__":
    unittest.main()
