"""Exercise actual production dialogue admission/retirement with local engine boundaries."""
import argparse
import os
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[1]

def body(source,name):
    start=source.index('static ',source.index(name)-20)
    # Function signatures in this header are each on a single line.
    start=source.rfind('\n',0,source.index(name))+1
    opening=source.index('{',start);depth=0
    for at in range(opening,len(source)):
        depth+=(source[at]=='{')-(source[at]=='}')
        if not depth:return source[start:at+1]
    raise ValueError('Unterminated production function '+name)

def structure(source,name):
    start=source.index('typedef struct '+name+' ')
    end=source.index('} '+name+';',start)+len('} '+name+';')
    return source[start:end]

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('zig',type=Path)
    args=parser.parse_args();zig=args.zig.resolve(strict=True)
    source=(ROOT/'src/runtime_dialogue_choices.h').read_text(encoding='utf-8')
    template=(ROOT/'tests/native_dialogue_lease_fixture.c').read_text(encoding='utf-8')
    declarations='\n'.join(structure(source,name) for name in ('RuntimeDialogueRecord','RuntimeDialoguePanel'))
    production='\n'.join(body(source,name) for name in ('runtime_dialogue_lease_retire(', 'runtime_dialogue_lease_begin('))
    assert 'p->ready=0;p->retiring=1;' in production
    assert 'if(p->active&&(!p->ready||p->retiring))' in production
    variants={'production':production,
        'partial-ledger-adopted':production.replace('if(p->active&&(!p->ready||p->retiring))','if(0)',1),
        'cleanup-keeps-readiness':production.replace('p->ready=0;p->retiring=1;','',1),
        'options-write-not-retained':production.replace('if(applied)p->options=1;',''),
        'obligation-cleared-without-hide':production.replace(
            '!runtime_dialogue_body(q->base,q->script[6].function.pointer,6))return;',
            '!runtime_dialogue_body(q->base,q->script[6].function.pointer,6)){p->options=0;memset(p,0,sizeof *p);return;}',1),
        'successful-hide-repeated':production.replace('p->options=0;','',1)}
    output=ROOT/'local/dialogue-lease-fixture';output.mkdir(parents=True,exist_ok=True)
    env=dict(os.environ,ZIG_GLOBAL_CACHE_DIR=str(ROOT/'local/zig-cache'),ZIG_LOCAL_CACHE_DIR=str(ROOT/'local/zig-local-cache'))
    for name,functions in variants.items():
        current=template.replace('/* PRODUCTION_STRUCTS */',declarations).replace('/* PRODUCTION_FUNCTIONS */',functions)
        path=output/(name+'.c');path.write_text(current,encoding='utf-8');exe=output/(name+'.exe')
        subprocess.run([str(zig),'cc','-target','x86_64-windows-gnu','-std=c17','-O1',str(path),'-o',str(exe)],check=True,env=env)
        result=subprocess.run([str(exe)],capture_output=True,text=True,timeout=10)
        if name=='production':assert result.returncode==0,result.stderr
        else:assert result.returncode!=0,'Negative control escaped: '+name
    print('PASS actual production dialogue lease admission/options cleanup and five negative controls; no game launched')
if __name__=='__main__':main()
