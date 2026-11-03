"""Compile/run the background focus wrapper on temporary objects; no game launches."""
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
    local.mkdir(exist_ok=True)
    temp = local / "compiler-tmp"
    temp.mkdir(exist_ok=True)
    env = dict(os.environ, ZIG_GLOBAL_CACHE_DIR=str(local / "zig-cache"),
               ZIG_LOCAL_CACHE_DIR=str(local / "zig-local-cache"),
               TMP=str(temp), TEMP=str(temp))
    with tempfile.TemporaryDirectory(prefix="background-focus-", dir=local) as tmp:
        output = Path(tmp) / "fixture.exe"
        build = subprocess.run([str(args.zig.resolve(strict=True)), "cc", "-target", "x86_64-windows-gnu",
                                "-std=c17", "-O2", "-Wall", "-I", str(ROOT / "src"),
                                str(ROOT / "tests/native_background_focus_fixture.c"),
                                "-o", str(output)], cwd=ROOT, env=env, capture_output=True,
                               text=True, encoding="utf-8", errors="replace", timeout=300)
        if build.returncode:
            print("\n".join((build.stdout + build.stderr).splitlines()[:20]), flush=True)
            raise SystemExit(build.returncode)
        subprocess.run([str(output)], cwd=ROOT, check=True, timeout=30)


if __name__ == "__main__":
    main()
