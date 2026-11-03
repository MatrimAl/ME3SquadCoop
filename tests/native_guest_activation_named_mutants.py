"""Early named-call observer negative controls; no game starts."""
import argparse
from pathlib import Path
from native_channel_mutants import check_mutations

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('zig',type=Path);a=p.parse_args()
    check_mutations(a.zig.resolve(strict=True),{
        'early-observer-forwards-twice':[('activation_named_original(self,frame,result);','activation_named_original(self,frame,result);activation_named_original(self,frame,result);')],
        'wrong-named-slot-admitted':[('slot==223&&','')],
        'numbered-function-name-admitted':[('!number&&','')],
    },'guest_activation_named_trace.h','SquadGuestActivationNamedSelfTest')
