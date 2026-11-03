"""Exercise production registered-panel discovery on bounded fixture memory.

Object type/native method pins are explicit controlled boundaries. The actual
array predicates, panel ownership and two-list discovery functions are used;
no game, process, UI method or movie factory is opened.
"""
import argparse
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile

from native_dialogue_lease import body, structure

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('zig', type=Path)
    args = parser.parse_args()
    source = (ROOT / 'src/runtime_dialogue_choices.h').read_text(encoding='utf-8')
    declarations = structure(source, 'RuntimeDialogueRecord')
    predicates = '\n'.join([
        body((ROOT / 'src/package_identity.h').read_text(encoding='utf-8'), 'identity_array('),
        body((ROOT / 'src/squad_context_support.h').read_text(encoding='utf-8'), 'scene_readiness_array('),
    ])
    functions = '\n'.join(body(source, name) for name in [
        'runtime_dialogue_panel_owned(', 'runtime_dialogue_find_panel('])
    previous = functions.replace(
        'scene_readiness_array(registry+offsets[l],&lists[l],128)',
        'identity_array(registry+offsets[l],&lists[l],128)', 1)
    assert previous != functions, 'Regression mutation must change the production helper'
    template = (ROOT / 'tests/native_dialogue_discovery_fixture.c').read_text(encoding='utf-8')
    local = ROOT / 'local'
    local.mkdir(exist_ok=True)
    env = dict(os.environ, ZIG_GLOBAL_CACHE_DIR=str(local / 'zig-cache'),
               ZIG_LOCAL_CACHE_DIR=str(local / 'zig-local-cache'))
    with tempfile.TemporaryDirectory(prefix='dialogue-discovery-', dir=local) as temp:
        folder = Path(temp)
        for name, current in [('production', functions), ('empty-list-rejected', previous)]:
            combined = declarations + '\n' + predicates + '\n' + current
            path = folder / (name + '.c')
            path.write_text(template.replace('/* PRODUCTION_HELPERS */', combined), encoding='utf-8')
            executable = folder / (name + '.exe')
            result = subprocess.run([str(args.zig.resolve(strict=True)), 'cc', '-target',
                'x86_64-windows-gnu', '-std=c17', '-O1', '-Wall', str(path), '-o', str(executable)],
                cwd=ROOT, env=env, capture_output=True, text=True, timeout=120)
            assert result.returncode == 0, result.stderr
            result = subprocess.run([str(executable)], capture_output=True, text=True, timeout=10)
            if name == 'production':
                assert result.returncode == 0, result.stderr
                print(result.stdout.strip())
            else:
                assert result.returncode != 0, 'Prior nonempty-only regression escaped the fixture'
        print('PASS nonempty-only registry regression rejected; helper SHA256 ' +
              hashlib.sha256((declarations + predicates + functions).encode()).hexdigest())


if __name__ == '__main__':
    main()
