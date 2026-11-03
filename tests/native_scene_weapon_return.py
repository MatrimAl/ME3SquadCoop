"""Run exact production scene-return guard helpers on fixture memory only.

Only the three self-contained production functions are extracted to avoid
substituting the unrelated session, channel, script-dispatch and engine APIs
required by the full runtime header. This does not exercise scene admission,
native ReplicatedWeaponUpdated execution or real firing/animation recovery.
"""
import argparse
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def function(source, name):
    """Extract a complete current function without rewriting its body."""
    marker = "static int " + name + "("
    if source.count(marker) != 1:
        raise ValueError("expected one production helper: " + name)
    start = source.index(marker)
    opened = source.index("{", start)
    depth = 1
    at = opened + 1
    quoted = None
    comment = None
    while at < len(source):
        char = source[at]
        next_char = source[at + 1:at + 2]
        if comment == "//":
            if char == "\n":
                comment = None
        elif comment == "/*":
            if char == "*" and next_char == "/":
                comment = None
                at += 1
        elif quoted:
            if char == "\\":
                at += 1
            elif char == quoted:
                quoted = None
        elif char == "/" and next_char in ("/", "*"):
            comment = "/" + next_char
            at += 1
        elif char in ('"', "'"):
            quoted = char
        elif char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return source[start:at + 1]
        at += 1
    raise ValueError("unterminated production helper: " + name)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("zig", type=Path)
    args = parser.parse_args()
    helpers = []
    for header, names in (
        ("runtime_guest_movement.h", ("runtime_movement_idle",)),
        ("runtime_guest_scene_weapon.h", ("runtime_scene_weapon_item_idle", "runtime_scene_weapon_return_ready")),
    ):
        source = (ROOT / "src" / header).read_text(encoding="utf-8")
        helpers.extend(function(source, name) for name in names)
    generated = "\n\n".join(helpers) + "\n"
    local = ROOT / "local"
    local.mkdir(exist_ok=True)
    temp = local / "compiler-tmp"
    temp.mkdir(exist_ok=True)
    env = dict(os.environ, ZIG_GLOBAL_CACHE_DIR=str(local / "zig-cache"),
               ZIG_LOCAL_CACHE_DIR=str(local / "zig-local-cache"), TMP=str(temp), TEMP=str(temp))
    with tempfile.TemporaryDirectory(prefix="scene-weapon-return-", dir=local) as tmp:
        directory = Path(tmp)
        (directory / "scene_weapon_return_helpers.h").write_text(generated, encoding="utf-8")
        output = directory / "fixture.exe"
        build = subprocess.run([str(args.zig.resolve(strict=True)), "cc", "-target", "x86_64-windows-gnu",
                                "-std=c17", "-O2", "-Wall", "-I", str(directory),
                                str(ROOT / "tests/native_scene_weapon_return_fixture.c"),
                                "-o", str(output)], cwd=ROOT, env=env, capture_output=True,
                               text=True, encoding="utf-8", errors="replace", timeout=300)
        if build.returncode:
            print("\n".join((build.stdout + build.stderr).splitlines()[:25]), flush=True)
            raise SystemExit(build.returncode)
        subprocess.run([str(output)], cwd=ROOT, check=True, timeout=30)
        print("Production helper extraction SHA256 " + hashlib.sha256(generated.encode()).hexdigest(), flush=True)


if __name__ == "__main__":
    main()
