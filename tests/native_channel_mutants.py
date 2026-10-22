"""Compile deliberate channel-identity faults; the native fixture must reject them.

Windows only. Usage: python tests/native_channel_mutants.py <zig.exe>
No game launches. Generated sources/binaries stay in ignored local/.
"""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src"))
from build_state import verify_current


def verdict(dll, export="SquadNetworkActorSelfTest"):
    result = subprocess.run([sys.executable, "-c",
        "import ctypes,sys; p=ctypes.CDLL(sys.argv[1]); "
        "test=getattr(p,sys.argv[2]); test.restype=ctypes.c_int; "
        "sys.exit(0 if test()==1 else 10)", str(dll), export],
        capture_output=True, text=True, timeout=30)
    return result.returncode


def check_mutations(compiler, mutations, header_name="network_actor_identity.h", export="SquadNetworkActorSelfTest"):
    verify_current(ROOT)
    assert verdict(ROOT / "local/build/ME3SquadCoopProbe.asi", export) == 0, "baseline fixture failed"
    env = dict(os.environ, ZIG_GLOBAL_CACHE_DIR=str(ROOT / "local/zig-cache"),
               ZIG_LOCAL_CACHE_DIR=str(ROOT / "local/zig-local-cache"),
               TMP=str(ROOT / "local/compiler-tmp"), TEMP=str(ROOT / "local/compiler-tmp"))
    for name, edits in mutations.items():
        with tempfile.TemporaryDirectory(prefix="channel-mutant-", dir=ROOT / "local") as tmp:
            folder = Path(tmp)
            shutil.copytree(ROOT / "src", folder / "src")
            header = folder / "src" / header_name
            content = header.read_text(encoding="utf-8")
            for old, new in edits:
                assert content.count(old) == 1, f"mutation target changed: {name}"
                content = content.replace(old, new)
            header.write_text(content, encoding="utf-8")
            dll = folder / "mutant.asi"
            build = subprocess.run([str(compiler), "cc", "-target", "x86_64-windows-gnu", "-std=c17", "-O2",
                "-shared", str(folder / "src/squad_probe.c"), "-o", str(dll)],
                cwd=ROOT, env=env, capture_output=True, text=True, timeout=120)
            assert build.returncode == 0, build.stderr
            assert verdict(dll, export) == 10, f"fixture failed to reject {name}"
            print(f"PASS rejected {name}", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("zig", type=Path)
    args = parser.parse_args()
    check_mutations(args.zig.resolve(strict=True), {
        "missing-world-binding": [("read_ptr(actor+0x198)!=s->info||", "")],
        "missing-reciprocal-table": [("read_ptr(connection + 0xf84 + (SIZE_T)index * 8) != channel", "0")],
        "accept-ambiguous-channel": [("if (++matches > 1) return -1;", "++matches;"),
                                     ("return matches == 1 ? result->index : -1;", "return matches >= 1 ? result->index : -1;")],
    })


if __name__ == "__main__":
    main()
