"""Exercise production scene-reference forwarding and scalar output guards.

Native capture/identity/installation are explicit stand-ins. The fixture does
not open a game, resolve package indexes or validate the live native ABI.
"""
import argparse, hashlib, os
from pathlib import Path
import subprocess, tempfile
from native_scene_weapon_return import function
from native_power_order_intent import structure

ROOT=Path(__file__).resolve().parents[1]
def extract(source,name,result):
    declaration='static '+result+' '+name+'('
    adapted=source.replace(declaration,'static int '+name+'(',1)
    return function(adapted,name).replace('static int '+name+'(',declaration,1)

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('zig',type=Path)
    args=parser.parse_args()
    scene=(ROOT/'src/runtime_scene_reference.h').read_text(encoding='utf-8')
    shared=(ROOT/'src/runtime_initial_reference.h').read_text(encoding='utf-8')
    types='\n'.join(structure(scene,n)for n in ['RuntimeSceneReferenceScope','RuntimeSceneReference'])+'\n'
    helpers='\n\n'.join(extract(scene,n,r)for n,r in [
        ('runtime_scene_reference_scalar_policy','int'),('runtime_scene_reference_bits','void'),
        ('runtime_scene_reference_index','void'),('runtime_scene_reference_forward','int')])+'\n'
    helpers+='\n'.join(extract(shared,n,'void')for n in ['runtime_initial_bits_forward','runtime_initial_index_forward'])+'\n'
    local=ROOT/'local';local.mkdir(exist_ok=True);cache=local/'scene-reference-cache';cache.mkdir(exist_ok=True)
    env=dict(os.environ,ZIG_GLOBAL_CACHE_DIR=str(cache/'global'),ZIG_LOCAL_CACHE_DIR=str(cache/'local'),TMP=str(cache),TEMP=str(cache))
    with tempfile.TemporaryDirectory(prefix='scene-reference-',dir=local)as temp:
        folder=Path(temp)
        (folder/'scene_reference_types.h').write_text(types,encoding='utf-8')
        (folder/'scene_reference_helpers.h').write_text(helpers,encoding='utf-8')
        output=folder/'fixture.exe'
        built=subprocess.run([str(args.zig.resolve(strict=True)),'cc','-target','x86_64-windows-gnu',
            '-std=c17','-O2','-Wall','-I',str(folder),str(ROOT/'tests/native_scene_reference_fixture.c'),'-o',str(output)],
            cwd=ROOT,env=env,capture_output=True,text=True,encoding='utf-8',errors='replace',timeout=300)
        if built.returncode:
            print('\n'.join((built.stdout+built.stderr).splitlines()[:25]));raise SystemExit(built.returncode)
        subprocess.run([str(output)],cwd=ROOT,check=True,timeout=30)
        print('Production scene/shared helper SHA256 '+hashlib.sha256((types+helpers).encode()).hexdigest())
        # A foreign property must be refused before installing scalar hooks.
        prepare_types=structure(scene,'RuntimeSceneReference')
        prepare_helpers=extract(scene,'runtime_scene_reference_slot_ready','int')+'\n'+extract(scene,'runtime_scene_reference_prepare','void')
        (folder/'scene_prepare_types.h').write_text(prepare_types,encoding='utf-8')
        for mutant in (False,True):
            current=prepare_helpers
            if mutant:
                current=current.replace('if(!runtime_scene_reference_slot_ready(base))return;','',1)
                assert current!=prepare_helpers
            (folder/'scene_prepare_helpers.h').write_text(current,encoding='utf-8')
            output=folder/('prepare-mutant.exe'if mutant else'prepare.exe')
            built=subprocess.run([str(args.zig.resolve(strict=True)),'cc','-target','x86_64-windows-gnu',
                '-std=c17','-O2','-Wall','-I',str(folder),str(ROOT/'tests/native_scene_reference_prepare_fixture.c'),'-o',str(output)],
                cwd=ROOT,env=env,capture_output=True,text=True,encoding='utf-8',errors='replace',timeout=300)
            if built.returncode:print(built.stderr);raise SystemExit(built.returncode)
            result=subprocess.run([str(output)],cwd=ROOT,capture_output=True,text=True,timeout=30)
            if mutant:
                assert result.returncode!=0,'Guest property precheck-order regression escaped fixture'
                print('PASS late guest property-precheck negative control rejected')
            else:
                print(result.stdout,end='');assert result.returncode==0,result.stderr
if __name__=='__main__':main()
