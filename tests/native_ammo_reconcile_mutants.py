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
        "local-activity-keeps-pending-reply": [("pending->active=0;", "(void)pending;")],
        "old-control-generation-accepted": [("generation==p->generation&&", "")],
        "wrong-response-request-accepted": [("m->request==p->request&&", "")],
        "response-during-firing-accepted": [("idle==1&&", "")],
    }, "ammo_reconcile_protocol.h", "SquadAmmoReconcileSelfTest")

    check_mutations(args.zig.resolve(strict=True), {
        "host-replies-before-quiet-window": [("if(now-s->quiet_at<250)return 0;", "")],
        "host-replies-while-firing": [("if(idle!=1||", "if(")],
        "host-keeps-expired-request": [("now-s->received_at>5000", "0")],
        "duplicate-extends-host-deadline": [("return m->channel==s->channel?2:0;", "{s->received_at=now;return m->channel==s->channel?2:0;}")],
    }, "ammo_reconcile_host.h", "SquadAmmoReconcileSelfTest")

    check_mutations(args.zig.resolve(strict=True), {
        "guest-requests-before-settle": [("now-s->quiet_at<250", "now-s->quiet_at<249")],
        "guest-exceeds-retry-budget": [("s->attempts<3", "s->attempts<4")],
        "guest-retries-expired-request": [("now-s->pending.sent_at>5000", "now-s->pending.sent_at>5001")],
        "guest-reuses-request-sequence": [("|++*sequence", "|*sequence")],
        "guest-exceeds-request-budget": [("*sequence>=64", "*sequence>64")],
    }, "ammo_reconcile_guest.h", "SquadAmmoReconcileSelfTest")

    check_mutations(args.zig.resolve(strict=True), {
        "same-scope-erases-new-request": [("if(!generation||s->generation!=generation)", "if(1)")],
        "scope-change-keeps-old-state": [("*s=(AmmoReconcileState){0};", "")],
        "missing-scope-admitted": [("return generation!=0;", "return 1;")],
    }, "ammo_reconcile_state.h", "SquadAmmoReconcileSelfTest")
