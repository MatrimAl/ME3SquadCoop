"""Compiled negative controls for transparent weapon script observations.

Usage: python tests/native_fire_rpc_mutants.py <zig.exe>. No game starts.
"""
import argparse
from pathlib import Path
from native_channel_mutants import check_mutations

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("zig", type=Path)
    args = parser.parse_args()
    check_mutations(args.zig.resolve(strict=True), {
        "original-not-forwarded": [("delayed_fire_exec_stock(k,self,frame,result,fire_rpc_original);", "(void)self;")],
        "forwarded-self-changed": [("delayed_fire_exec_stock(k,self,frame,result,fire_rpc_original);", "delayed_fire_exec_stock(k,NULL,frame,result,fire_rpc_original);")],
        "wrong-incoming-channel-accepted": [("channel==expected&&", "")],
        "outside-script-cursor-accepted": [("code>script+(uintptr_t)length", "0")],
        "wrong-inventory-link-accepted": [("inventory==weapon_inventory&&", "")],
        "wrong-inventory-instigator-accepted": [("instigator==pawn&&", "")],
        "pawn-receiver-confused-with-weapon": [("if(receiver==1)return self==pawn;", "if(receiver==1)return self==weapon;")],
    }, "fire_rpc_trace.h", "SquadFireRpcTraceSelfTest")
