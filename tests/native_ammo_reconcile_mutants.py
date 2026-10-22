"""Negative controls for reconciliation identity, epoch and active firing.

Usage: python tests/native_ammo_reconcile_mutants.py <zig.exe>. No game starts.
"""
import argparse
from pathlib import Path
from native_channel_mutants import check_mutations

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("zig", type=Path)
    args = parser.parse_args()
    check_mutations(args.zig.resolve(strict=True), {
        "old-control-generation-accepted": [("generation==p->generation&&", "")],
        "wrong-response-request-accepted": [("m->request==p->request&&", "")],
        "response-during-firing-accepted": [("idle==1&&", "")],
    }, "ammo_reconcile_protocol.h", "SquadAmmoReconcileSelfTest")
