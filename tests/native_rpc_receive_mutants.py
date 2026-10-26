"""Compile broken native receive contexts. No game starts."""
import argparse
from pathlib import Path
from native_channel_mutants import check_mutations

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("zig", type=Path)
    args = parser.parse_args()
    check_mutations(args.zig.resolve(strict=True), {
        "foreign-thread-inherits-receive": [
            ("if(!owner||GetCurrentThreadId()!=owner)return NULL;", "if(!owner)return NULL;")],
        "null-channel-inherits-outer": [
            ("if(!owner||GetCurrentThreadId()!=owner)return;", "if(!owner||!channel||GetCurrentThreadId()!=owner)return;")],
        "nested-receive-loses-parent": [
            ("native_rpc_receive_frame=f->previous;", "native_rpc_receive_frame=NULL;")],
    }, "native_rpc_receive_context.h", "SquadRpcReceiveContextSelfTest")
