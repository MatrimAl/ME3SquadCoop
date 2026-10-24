"""Reject lost-resume and repeated-generation bugs without launching a game."""
import argparse
from pathlib import Path
from native_channel_mutants import check_mutations

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("zig", type=Path)
    args = parser.parse_args()
    check_mutations(args.zig.resolve(strict=True), {
        "ignore-scene-and-disable-scope": [
            ("int scoped_owned=owned&&ordinary_mode&&enabled;", "int scoped_owned=owned;")],
        "advance-generation-every-held-frame": [
            ("int edge=scoped_owned&&(!guest_player_context_owned_last||pc!=guest_player_context_last_pc||",
             "int edge=scoped_owned&&(1||pc!=guest_player_context_last_pc||")],
    }, "guest_player_context_adapter.h", "SquadGuestContextScopeSelfTest")
