"""Compile cleanup-policy faults; exercise actual shared release on fake objects."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

from native_channel_mutants import ROOT, verdict


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("zig", type=Path)
    args = parser.parse_args()
    compiler = args.zig.resolve(strict=True)
    baseline = ROOT / "local/runtime-entry-fixture/runtime-entry-fixture.dll"
    export = "ME3RuntimeCoreCleanupSelfTest"
    assert verdict(baseline, export) == 0, "run native_runtime_harness.py first"
    environment = dict(os.environ, ZIG_GLOBAL_CACHE_DIR=str(ROOT / "local/zig-cache"),
                       ZIG_LOCAL_CACHE_DIR=str(ROOT / "local/zig-local-cache"),
                       TMP=str(ROOT / "local/compiler-tmp"), TEMP=str(ROOT / "local/compiler-tmp"))
    faults = {
        "unpossess-reused-pawn": ("if (pc_live && pawn_live &&", "if (pc_live &&"),
        "omit-ai-restore": ("if (pawn_live && detached && !owner && ai_live &&",
                            "if (0 && pawn_live && detached && !owner && ai_live &&"),
        "claim-disconnect-before-removal": (
            "if (session_cleanup_pending && (!session_disconnect_observed || session_connection_member(base, session_connection) != 0))",
            "if (0)"),
    }
    for name, (old, new) in faults.items():
        with tempfile.TemporaryDirectory(prefix="runtime-mutant-", dir=ROOT / "local") as temporary:
            folder = Path(temporary)
            shutil.copytree(ROOT / "src", folder / "src")
            header = folder / "src/squad_session_core.h"
            content = header.read_text(encoding="utf-8")
            assert content.count(old) == 1, name
            header.write_text(content.replace(old, new), encoding="utf-8")
            dll = folder / "runtime-mutant.dll"
            result = subprocess.run([str(compiler), "cc", "-target", "x86_64-windows-gnu", "-std=c17",
                "-O2", "-shared", "-DME3_RUNTIME_FIXTURE", str(folder / "src/squad_coop.c"),
                "-lbcrypt", "-o", str(dll)], env=environment, capture_output=True, text=True, timeout=120)
            assert result.returncode == 0, result.stderr
            assert verdict(dll, export) == 10, name
            print(f"PASS rejected {name}", flush=True)


if __name__ == "__main__":
    main()
