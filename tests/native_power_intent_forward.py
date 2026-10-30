"""Run production intent ingress/observer forwarding on temporary memory only.

Native frame/scope capture is an explicit stand-in. This fixture tests that
functional intent survives exhausted diagnostics and original forwarding,
parameters, result and LastError remain intact. It cannot establish live RPC
authentication or actual power permission/activation.
"""
import argparse
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile
from native_scene_weapon_return import function
from native_power_order_intent import structure

ROOT=Path(__file__).resolve().parents[1]

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('zig',type=Path)
    args=parser.parse_args()
    source=(ROOT/'src/runtime_power_start_observer.h').read_text(encoding='utf-8')
    types='\n'.join(structure(source,n) for n in ('RuntimePowerStartCapture','RuntimePowerStartObserver'))+'\n'
    helpers=[]
    for name in ('runtime_power_start_observer_observe','runtime_power_start_observer_forward'):
        declaration='static void '+name+'('
        adapted=source.replace(declaration,'static int '+name+'(',1)
        helpers.append(function(adapted,name).replace('static int '+name+'(',declaration,1))
    generated='\n\n'.join(helpers)+'\n'
    local=ROOT/'local';local.mkdir(exist_ok=True)
    cache=local/'power-intent-forward-cache';cache.mkdir(exist_ok=True)
    env=dict(os.environ,ZIG_GLOBAL_CACHE_DIR=str(cache/'global'),ZIG_LOCAL_CACHE_DIR=str(cache/'local'),TMP=str(cache),TEMP=str(cache))
    with tempfile.TemporaryDirectory(prefix='power-intent-forward-',dir=local)as temp:
        directory=Path(temp)
        (directory/'power_intent_forward_types.h').write_text(types,encoding='utf-8')
        (directory/'power_intent_forward_helpers.h').write_text(generated,encoding='utf-8')
        output=directory/'fixture.exe'
        build=subprocess.run([str(args.zig.resolve(strict=True)),'cc','-target','x86_64-windows-gnu',
            '-std=c17','-O2','-Wall','-I',str(directory),str(ROOT/'tests/native_power_intent_forward_fixture.c'),'-o',str(output)],
            cwd=ROOT,env=env,capture_output=True,text=True,encoding='utf-8',errors='replace',timeout=300)
        if build.returncode:
            print('\n'.join((build.stdout+build.stderr).splitlines()[:25]),flush=True)
            raise SystemExit(build.returncode)
        subprocess.run([str(output)],cwd=ROOT,check=True,timeout=30)
        print('Production forwarding helper SHA256 '+hashlib.sha256((types+generated).encode()).hexdigest(),flush=True)

if __name__=='__main__':main()
