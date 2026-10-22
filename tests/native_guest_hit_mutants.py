"""Reject unsafe hit-observer forwarding, scope and callback admission; no games."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
from native_channel_mutants import ROOT, check_mutations, verdict


def check_ordinary_guard(compiler):
    """The ordinary entry links without any observer symbol or callback."""
    with tempfile.TemporaryDirectory(prefix="ordinary-hit-guard-", dir=ROOT / "local") as tmp:
        folder = Path(tmp)
        shutil.copy2(ROOT / "src/weapon_process_event_guard.h", folder)
        source = folder / "guard.c"
        source.write_text('''#include <stdint.h>
#include "weapon_process_event_guard.h"
#ifdef ME3_SQUAD_COOP_PROTECTED_HIT_TRACE
#error The ordinary guard must not enable the protected observer
#endif
__declspec(dllexport) int OrdinaryGuardSelfTest(void) {
    return guest_equipment_weapon_pe_valid(100,200,100+0x40d450) &&
        !guest_equipment_weapon_pe_valid(100,200,999) &&
        !guest_equipment_weapon_pe_valid(100,200,0);
}
''', encoding="utf-8")
        env = dict(os.environ, ZIG_GLOBAL_CACHE_DIR=str(ROOT / "local/zig-cache"),
                   ZIG_LOCAL_CACHE_DIR=str(ROOT / "local/zig-local-cache"),
                   TMP=str(ROOT / "local/compiler-tmp"), TEMP=str(ROOT / "local/compiler-tmp"))
        dll = folder / "ordinary-guard.asi"
        build = subprocess.run([str(compiler), "cc", "-target", "x86_64-windows-gnu", "-std=c17", "-O2",
            "-shared", str(source), "-o", str(dll)], env=env, capture_output=True, text=True, timeout=120)
        assert build.returncode == 0, build.stderr
        assert verdict(dll, "OrdinaryGuardSelfTest") == 0, "ordinary entry admitted an observer callback"
        print("PASS ordinary Actor PE guard links without observer and rejects other callbacks", flush=True)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("zig", type=Path)
    compiler = parser.parse_args().zig.resolve(strict=True)
    check_ordinary_guard(compiler)
    check_mutations(compiler, {
        "hit-forwarded-twice": [("original(args->self,args->function,args->params,args->result);", "original(args->self,args->function,args->params,args->result);original(args->self,args->function,args->params,args->result);")],
        "hit-lost-entry-last-error": [("SetLastError(before_error);original", "original")],
        "hit-wrong-native-caller": [("caller&&caller==expected_caller&&", "caller&&")],
        "hit-wrong-reflected-function": [("function&&function==expected_function&&", "function&&")],
        "hit-reused-target-index": [("a->index==b->index&&", "")],
        "hit-stale-ownership-scope": [("return f->scope_after&&", "return ")],
        "hit-lost-nested-scope": [("observer->current=frame.previous;", "observer->current=&frame;")],
        "hit-unbounded-record-pairs": [("if(observer->rows>126)", "if(observer->rows>128)")],
    }, "guest_hit_trace_core.h", "SquadGuestHitTraceSelfTest")
    check_mutations(compiler, {
        "hit-arbitrary-registered-callback": [("guest_hit_registered==(uintptr_t)guest_hit_exec&&", "")],
        "hit-wrong-original": [("guest_hit_original==(ProcessEvent)(base+0x40d450)&&", "")],
        "hit-changed-native-dispatch": [("&&guest_hit_native_pins(base)&&", "&&")],
    }, "guest_hit_trace.h", "SquadGuestHitTraceSelfTest")
