"""Native script-slot identity negative controls; no game starts."""
import argparse
from pathlib import Path
from native_channel_mutants import check_mutations

if __name__ == '__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('zig',type=Path)
    args=parser.parse_args()
    check_mutations(args.zig.resolve(strict=True),{
        'wrong-slot-target-accepted':[('function==expected&&','')],
        'slot-outside-count-accepted':[('slot<(unsigned)first.count&&','')],
        'malformed-array-capacity-accepted':[('first.max>=first.count&&','')],
    },'native_script_slot.h','SquadNativeScriptSlotSelfTest')
