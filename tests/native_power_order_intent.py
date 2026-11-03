"""Exercise exact host power-order intent and retirement code on fixture memory.

Scope, UObject identity and engine metadata discovery stand-ins are explicit in
the C fixture. The production intent layout reader, bit CAS, owned-context
admission and full retirement loop execute unchanged. This is not an incoming
RPC admission or gameplay test.
"""
import argparse
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile

from native_scene_weapon_return import function

ROOT = Path(__file__).resolve().parents[1]


def structure(source, name):
    start = source.index("typedef struct " + name + " {")
    end = source.index("} " + name + ";", start) + len("} " + name + ";")
    return source[start:end]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("zig", type=Path)
    args = parser.parse_args()
    context = (ROOT / "src/runtime_power_action_context.h").read_text(encoding="utf-8")
    power = (ROOT / "src/runtime_guest_power.h").read_text(encoding="utf-8")
    hud = (ROOT / "src/runtime_native_hud.h").read_text(encoding="utf-8")
    types = "\n".join(structure(source, name) for source, name in (
        (power, "RuntimePowerCatalog"), (context, "RuntimePowerActionContextRow"),
        (context, "RuntimePowerActionContext"))) + "\n"
    types += "static RuntimePowerActionContext runtime_power_action_context;\nstatic unsigned runtime_power_action_context_events;\n"
    helpers = [function(power, "runtime_power_same_handle")]
    # The same lexical extraction accepts a uintptr_t or void return type.
    def extract(source, name, result):
        declaration = "static " + result + " " + name + "("
        if source.count(declaration) != 1:
            raise ValueError("expected one production helper: " + name)
        adapted = source.replace(declaration, "static int " + name + "(", 1)
        return function(adapted, name).replace("static int " + name + "(", declaration, 1)
    helpers += [extract(hud, "runtime_native_hud_field", "uintptr_t"),
                extract(power, "runtime_power_declaring_base", "uintptr_t")]
    for name in ("runtime_power_action_context_pointer", "runtime_power_action_context_idle",
                 "runtime_power_action_context_action_idle", "runtime_power_action_context_ai",
                 "runtime_power_action_context_catalog_same"):
        helpers.append(function(context, name))
    helpers.append(extract(context, "runtime_power_action_context_log", "void"))
    for name in ("runtime_power_action_context_order_layout", "runtime_power_action_context_order_bit",
                 "runtime_power_action_context_player_order", "runtime_power_action_context_cleanup_idle",
                 "runtime_power_action_context_restore_ai_allowed", "runtime_power_action_context_retire"):
        helpers.append(function(context, name))
    generated = "\n\n".join(helpers) + "\n"
    local = ROOT / "local"
    local.mkdir(exist_ok=True)
    cache = local / "power-order-intent-cache"
    cache.mkdir(exist_ok=True)
    env = dict(os.environ, ZIG_GLOBAL_CACHE_DIR=str(cache / "global"),
               ZIG_LOCAL_CACHE_DIR=str(cache / "local"), TMP=str(cache), TEMP=str(cache))
    with tempfile.TemporaryDirectory(prefix="power-order-intent-", dir=local) as tmp:
        directory = Path(tmp)
        (directory / "power_order_intent_types.h").write_text(types, encoding="utf-8")
        (directory / "power_order_intent_helpers.h").write_text(generated, encoding="utf-8")
        output = directory / "fixture.exe"
        build = subprocess.run([str(args.zig.resolve(strict=True)), "cc", "-target", "x86_64-windows-gnu",
                                "-std=c17", "-O2", "-Wall", "-I", str(directory),
                                str(ROOT / "tests/native_power_order_intent_fixture.c"),
                                "-o", str(output)], cwd=ROOT, env=env, capture_output=True,
                               text=True, encoding="utf-8", errors="replace", timeout=300)
        if build.returncode:
            print("\n".join((build.stdout + build.stderr).splitlines()[:25]), flush=True)
            raise SystemExit(build.returncode)
        subprocess.run([str(output)], cwd=ROOT, check=True, timeout=30)
        print("Production intent/retirement helper SHA256 " + hashlib.sha256((types + generated).encode()).hexdigest(), flush=True)


if __name__ == "__main__":
    main()
