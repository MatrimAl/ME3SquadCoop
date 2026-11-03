"""Exercise the current Bink preparation prefix on fixture state only.

The production prefix runs unchanged through its atomic enabled snapshot and
diagnostic flush. DLL discovery and import-slot installation are outside this
fixture; existing runtime fixtures cover decoder forwarding and input guards.
"""
import argparse
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("zig", type=Path)
    args = parser.parse_args()
    source = (ROOT / "src/runtime_bink_movie.h").read_text(encoding="utf-8")
    start = source.index("typedef int (*RuntimeBinkMulti)")
    end = source.index("static int runtime_bink_movie_multi(", start)
    types = source[start:end]
    start = source.index("static void runtime_bink_movie_prepare(")
    end = source.index("    if(!enabled||s->installed||s->refused", start)
    prefix = source[start:end] + "}\n"
    current = "(g_runtime_config.role==ME3_ROLE_HOST||g_runtime_config.role==ME3_ROLE_GUEST)"
    if prefix.count(current) != 1:
        raise RuntimeError("Bink preparation role gate changed; review extraction")
    config = (ROOT / "src/runtime_config.h").read_text(encoding="utf-8")
    enums = []
    for name in ("Me3RuntimeRole", "Me3RuntimeConfigStatus"):
        start = config.index("typedef enum " + name)
        end = config.index(";", start) + 1
        enums.append(config[start:end])
    local = ROOT / "local"
    local.mkdir(exist_ok=True)
    temp = local / "compiler-tmp"
    temp.mkdir(exist_ok=True)
    env = dict(os.environ, ZIG_GLOBAL_CACHE_DIR=str(local / "zig-cache"),
               ZIG_LOCAL_CACHE_DIR=str(local / "zig-local-cache"), TMP=str(temp), TEMP=str(temp))
    with tempfile.TemporaryDirectory(prefix="bink-scope-", dir=local) as tmp:
        directory = Path(tmp)
        (directory / "bink_scope_types.h").write_text("\n".join(enums) + "\n" + types, encoding="utf-8")
        for mutant in (False, True):
            body = prefix.replace(current, "g_runtime_config.role==ME3_ROLE_GUEST", 1) if mutant else prefix
            (directory / "bink_scope_prefix.h").write_text(body, encoding="utf-8")
            output = directory / ("mutant.exe" if mutant else "fixture.exe")
            build = subprocess.run([str(args.zig.resolve(strict=True)), "cc", "-target", "x86_64-windows-gnu",
                                    "-std=c17", "-O2", "-Wall", "-I", str(directory),
                                    str(ROOT / "tests/native_bink_scope_fixture.c"), "-o", str(output)],
                                   cwd=ROOT, env=env, capture_output=True, text=True, encoding="utf-8",
                                   errors="replace", timeout=300)
            if build.returncode:
                print("\n".join((build.stdout + build.stderr).splitlines()[:20]), flush=True)
                raise SystemExit(build.returncode)
            result = subprocess.run([str(output)], cwd=ROOT, capture_output=True, text=True, timeout=30)
            if mutant:
                if result.returncode == 0:
                    raise RuntimeError("Old guest-only admission mutant escaped the regression")
                print("PASS old guest-only admission mutant rejected", flush=True)
            else:
                print(result.stdout, end="", flush=True)
                if result.returncode:
                    print(result.stderr, end="", flush=True)
                    raise SystemExit(result.returncode)
        print("Production Bink preparation prefix SHA256 " + hashlib.sha256(prefix.encode()).hexdigest(), flush=True)


if __name__ == "__main__":
    main()
