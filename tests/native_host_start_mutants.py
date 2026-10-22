"""Compile host-start faults; run actual checked pipeline on fixture objects."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

from native_channel_mutants import ROOT, verdict


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("zig", type=Path)
    args = parser.parse_args()
    compiler = args.zig.resolve(strict=True)
    baseline = ROOT / "local/runtime-entry-fixture/runtime-entry-fixture.dll"
    export = "ME3RuntimeHostStartupSelfTest"
    assert verdict(baseline, export) == 0, "run native_runtime_harness.py first"
    environment = dict(os.environ, ZIG_GLOBAL_CACHE_DIR=str(ROOT / "local/zig-cache"),
                       ZIG_LOCAL_CACHE_DIR=str(ROOT / "local/zig-local-cache"),
                       TMP=str(ROOT / "local/compiler-tmp"), TEMP=str(ROOT / "local/compiler-tmp"))
    faults = {
        "accept-port-fallback": (
            "runtime_transport_endpoint(base,driver,&g_runtime_config)&&", "1&&"),
        "skip-master-map": (
            "runtime_host_master_prepare(base,chain->world,driver,calls->initialize)&&", "1&&"),
        "overwrite-native-package-guid": (
            "uint32_t current[4];int matches=0;",
            "uint32_t current[4];int matches=0; memcpy((void*)(packages[j]+0x6c),guids[j],16);"),
        "double-destroy-owned-url": (
            "int listened=calls->listen((void*)chain->world,&copied,&error);",
            "int listened=calls->listen((void*)chain->world,&copied,&error); calls->destroy(&copied);"),
        "overwrite-new-capacity": (
            "InterlockedCompareExchange((LONG volatile*)(game+0x42c),lease->players,2)==2",
            "(InterlockedExchange((LONG volatile*)(game+0x42c),lease->players),1)"),
        "restore-reused-game": (
            "static int runtime_host_capacity_restore(uintptr_t base,RuntimeHostCapacityLease *lease) {\n    if(!runtime_host_capacity_identity(base,lease))",
            "static int runtime_host_capacity_restore(uintptr_t base,RuntimeHostCapacityLease *lease) {\n    if(0)"),
    }
    for name, (old, new) in faults.items():
        with tempfile.TemporaryDirectory(prefix="host-start-mutant-", dir=ROOT / "local") as temporary:
            folder = Path(temporary)
            shutil.copytree(ROOT / "src", folder / "src")
            header = folder / "src/runtime_host_bootstrap.h"
            content = header.read_text(encoding="utf-8")
            assert content.count(old) == 1, name
            header.write_text(content.replace(old, new), encoding="utf-8")
            dll = folder / "host-start-mutant.dll"
            result = subprocess.run([str(compiler), "cc", "-target", "x86_64-windows-gnu", "-std=c17",
                "-O2", "-shared", "-DME3_RUNTIME_FIXTURE", str(folder / "src/squad_coop.c"),
                "-lbcrypt", "-o", str(dll)], env=environment, capture_output=True, text=True, timeout=120)
            assert result.returncode == 0, result.stderr
            assert verdict(dll, export) == 10, name
            print(f"PASS rejected {name}", flush=True)


if __name__ == "__main__":
    main()
