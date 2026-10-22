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
        "route-shuttle-skip-doorway": [("else *point=0;", "else *point=COMBAT_ROUTE_ROAD_FIRST;")],
        "route-shuttle-wide-arrival": [("point==0?20.0f", "point==0?70.0f")],
        "route-shuttle-ignore-lower-floor": [("point==1?20.0f:180.0f", "point==1?180.0f:180.0f")],
        "route-shuttle-skip-floor-on-start": [("fabsf(position[2]-combat_route_points[1].xyz[2])<=20", "fabsf(position[2]-combat_route_points[1].xyz[2])<=180")],
        "route-fname-number-shift": [("(unsigned long)(number-1)", "(unsigned long)number")],
        "route-scene-wait-unbounded": [("observed_scene==1?20000:2000", "observed_scene==1?75000:2000")],
        "route-unknown-waits-like-scene": [("observed_scene==1?20000:2000", "observed_scene?20000:2000")],
        "route-pause-deadline-inclusive": [("&&elapsed<budget", "&&elapsed<=budget")],
    }, "combat_route_qa.h", "SquadCombatRouteSelfTest")
