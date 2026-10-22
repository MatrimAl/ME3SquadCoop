"""Reject research launches using stale or partially built native binaries."""
import hashlib
import json
from pathlib import Path


def fingerprint(root):
    root = Path(root)
    paths = [root / "build.py", *sorted((root / "src").glob("*.c")),
             *sorted((root / "src").glob("*.h"))]
    paths += [root / "local/build/ME3SquadCoopProbe.asi", root / "local/build/ME3SquadCoop.asi",
              root / "local/build/sandbox_launch.exe"]
    return {p.relative_to(root).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}


def verify_current(root):
    root = Path(root)
    saved = json.loads((root / "local/build/current-build.json").read_text(encoding="utf-8"))
    if saved.get("files") != fingerprint(root):
        raise RuntimeError("native binaries are stale; complete a successful build before launching")


def record_current(root):
    root = Path(root)
    (root / "local/build/current-build.json").write_text(
        json.dumps({"files": fingerprint(root)}, indent=2) + "\n", encoding="utf-8")
