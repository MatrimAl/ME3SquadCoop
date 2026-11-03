"""Exercise production host-send metadata and forwarding on fixture memory.

Native object/pin/channel/scene acquisition are explicit fixture boundaries.
No game is opened and no native serializer or package-map method is called.
"""
import argparse,hashlib,json,os
from pathlib import Path
import subprocess,tempfile
from native_scene_reference import extract
from native_power_order_intent import structure

ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('zig',type=Path);args=p.parse_args()
    source=(ROOT/'src/runtime_scene_reference_send.h').read_text(encoding='utf-8')
    types='\n'.join(structure(source,n)for n in ('RuntimeSceneReferenceSendMetadata','RuntimeSceneReferenceSendFrame','RuntimeSceneReferenceSend'))
    functions='\n'.join(extract(source.replace('__attribute__((noinline)) ',''),n,r)for n,r in (
        ('runtime_scene_reference_send_opted','int'),('runtime_scene_reference_send_metadata','void'),
        ('runtime_scene_reference_send_metadata_equal','int'),('runtime_scene_reference_send_lease','int'),
        ('runtime_scene_reference_send_capture','int'),('runtime_scene_reference_send_same','int'),
        ('runtime_scene_reference_send_emit','void'),('runtime_scene_reference_send_gate','int'),
        ('runtime_scene_reference_send_observed','int'),('runtime_scene_reference_send_forward','int'),
        ('runtime_scene_reference_send_prepare','void')))
    quote=extract((ROOT/'src/package_identity.h').read_text(encoding='utf-8'),'identity_quote','void')
    functions=quote+'\n'+functions
    local=ROOT/'local';local.mkdir(exist_ok=True);cache=local/'scene-send-cache';cache.mkdir(exist_ok=True)
    env=dict(os.environ,ZIG_GLOBAL_CACHE_DIR=str(cache/'global'),ZIG_LOCAL_CACHE_DIR=str(cache/'local'),TMP=str(cache),TEMP=str(cache))
    with tempfile.TemporaryDirectory(prefix='scene-send-',dir=local)as tmp:
        folder=Path(tmp);(folder/'send_types.h').write_text(types,encoding='utf-8');(folder/'send_helpers.h').write_text(functions,encoding='utf-8')
        output=folder/'fixture.exe'
        built=subprocess.run([str(args.zig.resolve(strict=True)),'cc','-target','x86_64-windows-gnu','-std=c17','-O1','-Wall',
            '-I',str(folder),str(ROOT/'tests/native_scene_reference_send_fixture.c'),'-o',str(output)],cwd=ROOT,env=env,capture_output=True,text=True,timeout=180)
        if built.returncode:print(built.stderr);raise SystemExit(built.returncode)
        for setting in (None,'0','bad','1'):
            current=dict(env);current.pop('ME3_SQUAD_COOP_SCENE_REFERENCE_SEND_TRACE',None)
            if setting is not None:current['ME3_SQUAD_COOP_SCENE_REFERENCE_SEND_TRACE']=setting
            run=subprocess.run([str(output),'enabled'if setting=='1'else'disabled'],cwd=ROOT,env=current,capture_output=True,text=True,timeout=30)
            for line in run.stdout.splitlines():
                if line.startswith('MAX_ROW '):
                    row=json.loads(line[8:]);assert len(row['action_path'])==599 and len(row['package_path'])==599
                    assert row['package_lookup_performed']is False
                    print('PASS maximal escaped action/package paths form complete JSON')
                else:print(line)
            if run.returncode:print(run.stderr);raise SystemExit(run.returncode)
        print('Production host-send helpers SHA256 '+hashlib.sha256((types+functions).encode()).hexdigest())
if __name__=='__main__':main()
