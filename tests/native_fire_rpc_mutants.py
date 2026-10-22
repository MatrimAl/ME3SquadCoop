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
        "original-not-forwarded": [("fire_rpc_original(self,frame,result);", "(void)self;")],
        "forwarded-self-changed": [("fire_rpc_original(self,frame,result);", "fire_rpc_original(NULL,frame,result);")],
        "wrong-incoming-channel-accepted": [("channel==expected&&", "")],
    }, "fire_rpc_trace.h", "SquadFireRpcTraceSelfTest")
