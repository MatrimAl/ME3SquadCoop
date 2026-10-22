"""Reject unsafe normal-action forwarding and cleanup; no game launches."""
import argparse
from pathlib import Path
from native_channel_mutants import check_mutations

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("zig", type=Path)
    args = parser.parse_args()
    check_mutations(args.zig.resolve(strict=True), {
        "action-without-opt-in": [("||opted!=1", "")],
        "action-release-with-wrong-owner": [("if(safe!=1){p->pending=0;return -1;}", "")],
        "action-release-before-tap-bound": [("now-p->pressed_at<100", "now-p->pressed_at<10")],
        "action-reused-input-slot": [("!guest_camera_stamp_core_equal(saved+i,current+i)||", "")],
        "action-wrong-alias": [('wchar_t alias[]=L"PC_Action";', 'wchar_t alias[]=L"PressAction";')],
        "action-without-actual-touch": [("return touching==1&&", "return ")],
        "action-too-close-to-top": [("top_distance>=45&&top_distance<=230", "top_distance>=0&&top_distance<=230")],
        "action-unbounded-approach": [("elapsed<10000", "elapsed<100000")],
    }, "combat_action_qa.h", "SquadCombatActionSelfTest")
