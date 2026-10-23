"""Reject old command-buffer/pawn contracts and overly broad host admission."""
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
    environment = dict(os.environ, ZIG_GLOBAL_CACHE_DIR=str(ROOT / "local/zig-cache"),
                       ZIG_LOCAL_CACHE_DIR=str(ROOT / "local/zig-local-cache"),
                       TMP=str(ROOT / "local/compiler-tmp"), TEMP=str(ROOT / "local/compiler-tmp"))
    faults = {
        "command-buffer-0x20-early": (
            "runtime_transport.h", "ME3RuntimeTransportSelfTest",
            "uintptr_t command_address=base+0x1196e0+7+(intptr_t)displacement;",
            "uintptr_t command_address=base+0x17fdaa0;"),
        "invented-player-class": (
            "runtime_host_bootstrap.h", "ME3RuntimeHostStartupSelfTest",
            'is_kind_of(base,c.pawn,"SFXPawn_Player")', 'is_kind_of(base,c.pawn,"BioPlayerPawn")'),
        "admit-any-bio-pawn": (
            "runtime_host_bootstrap.h", "ME3RuntimeHostStartupSelfTest",
            'is_kind_of(base,c.pawn,"SFXPawn_Player")', 'is_kind_of(base,c.pawn,"BioPawn")'),
    }
    for name, (header_name, export, old, new) in faults.items():
        assert verdict(baseline, export) == 0, "run native_runtime_harness.py first"
        with tempfile.TemporaryDirectory(prefix="runtime-contract-mutant-", dir=ROOT / "local") as temporary:
            folder = Path(temporary)
            shutil.copytree(ROOT / "src", folder / "src")
            header = folder / "src" / header_name
            content = header.read_text(encoding="utf-8")
            assert content.count(old) == 1, name
            header.write_text(content.replace(old, new), encoding="utf-8")
            dll = folder / "runtime-contract-mutant.dll"
            result = subprocess.run([str(compiler), "cc", "-target", "x86_64-windows-gnu", "-std=c17",
                "-O2", "-shared", "-DME3_RUNTIME_FIXTURE", str(folder / "src/squad_coop.c"),
                "-lbcrypt", "-o", str(dll)], env=environment, capture_output=True, text=True, timeout=120)
            assert result.returncode == 0, result.stderr
            assert verdict(dll, export) == 10, name
            print(f"PASS rejected {name}", flush=True)


if __name__ == "__main__":
    main()
