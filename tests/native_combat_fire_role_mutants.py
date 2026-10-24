"""Check the scoped QA role correction without starting a game."""
import argparse
from pathlib import Path
from native_channel_mutants import check_mutations

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("zig", type=Path)
    args = parser.parse_args()
    check_mutations(args.zig.resolve(strict=True), {
        "restore-initial-remote-role-only": [
            ("(pawn[0]==2||pawn[0]==3)", "(pawn[0]==3)")],
        "allow-simulated-local-pawn": [
            ("&&pawn[1]==2", "&&(pawn[1]==1||pawn[1]==2)")],
        "ignore-native-no-fire-permission": [
            ("&&!(nofire&0x10000)", "")],
    }, "combat_fire_bot.h", "SquadCombatFireRolesSelfTest")
