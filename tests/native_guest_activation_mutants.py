"""Protected activation observer forwarding/scope controls; no game starts."""
import argparse
from pathlib import Path
from native_channel_mutants import check_mutations

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('zig',type=Path)
    args=parser.parse_args()
    check_mutations(args.zig.resolve(strict=True),{
        'observer-forwards-twice':[('guest_activation_original(self,frame,result);','guest_activation_original(self,frame,result);guest_activation_original(self,frame,result);')],
        'observer-admits-other-thread':[('thread==owner&&','')],
        'observer-admits-old-generation':[('generation==current&&','')],
    },'guest_activation_trace.h','SquadGuestActivationSelfTest')
