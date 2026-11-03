"""Run the actual optimized movement observer on authored memory; never starts a game."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("zig", type=Path)
    args = parser.parse_args()
    local = ROOT / "local"
    temp = local / "compiler-tmp"
    temp.mkdir(parents=True, exist_ok=True)
    env = dict(os.environ, ZIG_GLOBAL_CACHE_DIR=str(local / "zig-cache"),
               ZIG_LOCAL_CACHE_DIR=str(local / "zig-local-cache"),
               TMP=str(temp), TEMP=str(temp))
    with tempfile.TemporaryDirectory(prefix="optimized-consume-", dir=local) as tmp:
        output = Path(tmp) / "fixture.exe"
        build = subprocess.run([str(args.zig.resolve(strict=True)), "cc", "-target", "x86_64-windows-gnu",
                                "-std=c17", "-O2", "-Wall", "-I", str(ROOT / "src"),
                                str(ROOT / "tests/native_optimized_consume_fixture.c"),
                                "-o", str(output)], cwd=ROOT, env=env, capture_output=True,
                               text=True, encoding="utf-8", errors="replace", timeout=300)
        if build.returncode:
            print("\n".join((build.stdout + build.stderr).splitlines()[:20]), flush=True)
            raise SystemExit(build.returncode)
        canary = subprocess.run([str(output), "canary"], cwd=ROOT, capture_output=True,
                                text=True, timeout=30)
        if canary.returncode != 2 or "CHECK failed" not in canary.stderr:
            raise RuntimeError("native runtime checks are inactive or unexpected canary failure")
        print("PASS negative canary: runtime CHECK fails even in optimized build", flush=True)
        subprocess.run([str(output), "off"], cwd=ROOT, check=True, timeout=30)
        subprocess.run([str(output)], cwd=ROOT, check=True, timeout=30)


if __name__ == "__main__":
    main()
