"""Compile unsafe engine control gates; the real native fixture must reject them."""
import argparse
from pathlib import Path
from native_channel_mutants import check_mutations

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("zig", type=Path)
    args = parser.parse_args()
    check_mutations(args.zig.resolve(strict=True), {
        "unreadable-cinematic-state-accepted": [('if (!read_at(c.pc + 0x77c, &flags, 4)) return "controller-flags-unreadable";', 'read_at(c.pc + 0x77c, &flags, 4);')],
        "invalid-conversation-count-accepted": [('if (active < 0) return "conversation-count-invalid";', '')],
        "players-only-ignored": [('if (world_flags & 0x400) return "players-only";', '')],
        "unreadable-players-only-accepted": [('if (!read_at(c.world + 0x7c0, &world_flags, 4)) return "world-flags-unreadable";', 'read_at(c.world + 0x7c0, &world_flags, 4);')],
        "pending-players-only-ignored": [('if (world_flags & 0x800) return "players-only-pending";', '')],
        "native-pause-owner-ignored": [('if (pauser) return "engine-paused";', '')],
        "unreadable-pause-owner-accepted": [('if (!read_at(c.world + 0x670, &pauser, 8)) return "pauser-unreadable";', 'pauser = 0; read_at(c.world + 0x670, &pauser, 8);')],
    }, "squad_session_core.h", "SquadSessionGateSelfTest")
