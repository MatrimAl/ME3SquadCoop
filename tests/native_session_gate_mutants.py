"""Compile unsafe engine control gates; the real native fixture must reject them."""
import argparse
from pathlib import Path
from native_channel_mutants import check_mutations

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("zig", type=Path)
    args = parser.parse_args()
    check_mutations(args.zig.resolve(strict=True), {
        "unreadable-cinematic-state-accepted": [("if (!read_at(c.pc + 0x77c, &flags, 4)) return 1;", "read_at(c.pc + 0x77c, &flags, 4);")],
        "invalid-conversation-count-accepted": [(" || active < 0", "")],
        "players-only-ignored": [("||(world_flags&0x400)", "")],
        "unreadable-players-only-accepted": [("!read_at(c.world+0x7c0,&world_flags,4)", "0")],
    }, "squad_session_core.h", "SquadSessionGateSelfTest")
