"""Build sandbox research binaries with a workspace-local Zig C compiler."""
import argparse
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / "src"))
from build_state import record_current


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("zig", type=Path)
    args = p.parse_args()
    compiler = args.zig.resolve(strict=True)
    output = ROOT / "local" / "build"
    output.mkdir(parents=True, exist_ok=True)
    (output / "current-build.json").unlink(missing_ok=True)
    temp = ROOT / "local" / "compiler-tmp"
    temp.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, ZIG_GLOBAL_CACHE_DIR=str(ROOT / "local" / "zig-cache"),
               ZIG_LOCAL_CACHE_DIR=str(ROOT / "local" / "zig-local-cache"),
               TMP=str(temp), TEMP=str(temp))
    for source, filename, extra in [
        ("squad_probe.c", "ME3SquadCoopProbe.asi", ["-shared"]),
        ("squad_probe.c", "ME3SquadCoop.asi", ["-shared", "-DME3_SQUAD_COOP_RELEASE"]),
        ("sandbox_launch.c", "sandbox_launch.exe", ["-municode", "-ladvapi32"]),
    ]:
        command = [str(compiler), "cc", "-target", "x86_64-windows-gnu", "-std=c17", "-O2", "-Wall",
                   str(ROOT / "src" / source), "-o", str(output / filename), *extra]
        result = subprocess.run(command, cwd=ROOT, env=env, capture_output=True,
                                text=True, encoding="utf-8", errors="replace")
        (output / (filename + ".build.txt")).write_text(result.stdout + result.stderr, encoding="utf-8")
        if result.returncode:
            print("\n".join((result.stdout + result.stderr).splitlines()[:12]), flush=True)
            raise SystemExit(result.returncode)
        print("BUILT " + filename, flush=True)
    record_current(ROOT)


if __name__ == "__main__":
    main()
