"""Reject deliberate native observer faults; this does not start a game.

Usage: python tests/native_fire_trace_mutants.py <zig.exe>
"""
import argparse
from pathlib import Path
from native_channel_mutants import check_mutations

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("zig", type=Path)
    args = parser.parse_args()
    check_mutations(args.zig.resolve(strict=True), {
        "different-weapon-accepted": [("self==weapon&&", "")],
        "different-vtable-accepted": [("read_ptr(weapon)==base+0x14fa430&&", "")],
        "native-target-changed": [("read_ptr(base+0x14fa430+0x7d8)==(installed?(uintptr_t)native_fire_exec:base+0xc62120)&&", "")],
        "original-called-twice": [("SetLastError(before);native_fire_original(self);", "SetLastError(before);native_fire_original(self);native_fire_original(self);")],
        "last-error-not-restored": [("SetLastError(before);native_fire_original(self);", "SetLastError(19);native_fire_original(self);")],
    }, "native_fire_trace.h", "SquadNativeFireTraceSelfTest")
