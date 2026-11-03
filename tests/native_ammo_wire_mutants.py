"""Reject broken observer forwarding, nested receive isolation and record budgets.

Usage: python tests/native_ammo_wire_mutants.py <zig.exe>. No game launches.
"""
import argparse
from pathlib import Path
from native_channel_mutants import check_mutations

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("zig", type=Path)
    args = parser.parse_args()
    check_mutations(args.zig.resolve(strict=True), {
        "missing-original-forward": [("int result=ammo_wire_original(property,archive,map,data);", "int result=0;")],
        "nested-receive-inherits-parent": [("ammo_wire_receive=f;AmmoWireScope", "AmmoWireScope")],
        "exceeded-wire-record-budget": [("if(ammo_wire_rows<256)", "if(ammo_wire_rows<257)")],
    }, "ammo_wire_trace.h", "SquadAmmoWireSelfTest")
