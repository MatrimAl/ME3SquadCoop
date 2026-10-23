"""Reject unsafe collision-shape and compare-exchange variants offline."""
import argparse
from pathlib import Path
from native_channel_mutants import check_mutations

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("zig", type=Path)
    args = parser.parse_args()
    check_mutations(args.zig.resolve(strict=True), {
        "collision-wrong-component-response": [("s->component==0xaa", "s->component==0xba")],
        "collision-clobbers-unrelated-flags": [("expected|0x40000000", "UINT32_C(0x60000000)")],
        "collision-stale-world-compare": [("(LONG)expected)==expected", "(LONG)(expected^0x100))==expected")],
    }, "guest_collision_restore.h", "SquadGuestCollisionSelfTest")
