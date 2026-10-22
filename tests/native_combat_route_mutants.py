"""Compile unsafe route steering policies; the native fixture must reject them."""
import argparse
from pathlib import Path
from native_channel_mutants import check_mutations

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("zig", type=Path)
    args = parser.parse_args()
    check_mutations(args.zig.resolve(strict=True), {
        "route-stale-identity": [("p->identity==1&&", "")],
        "route-excess-input": [("||gain>0.45f", "")],
        "route-off-corridor": [("<=180&&fabsf", "<=2000&&fabsf")],
        "route-unbounded-start": [("<=1500&&", "<=15000&&")],
        "route-fname-number-shift": [("(unsigned long)(number-1)", "(unsigned long)number")],
    }, "combat_route_qa.h", "SquadCombatRouteSelfTest")
