"""Stage private feature-review artifacts; never launch, install or record."""
from pathlib import Path
from datetime import datetime, timezone
import argparse
import hashlib
import json
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'src'))
from build_state import verify_current

FEATURES = [
    ('01-movement-cover', 'native-candidate',
     ['Walk, turn, sprint and take/leave cover on the guest.', 'Repeat after a real scene return.'],
     ['Sustained synchronized host/guest position, ground contact and usable camera.',
      'No unexplained teleport/fall or stuck input.'], []),
    ('02-weapons', 'native-candidate',
     ['Aim, fire, release, reload, then change weapon.', 'Repeat while an enemy is present.'],
     ['Firing stops after release on authority; magazine/reserve state converges.',
      'Attribute actual enemy damage separately; ammo alone is insufficient.'], []),
    ('03-powers', 'native-candidate',
     ['Use normal keyboard quickslots and full wheel.', 'Select and cast distinct available powers.'],
     ['Guest pawn is the caster; native targeting, rank and cooldown remain intact.',
      'Show authoritative effects and usable input after closing the wheel.'], []),
    ('04-shift-menu', 'native-candidate',
     ['Open and close the Shift command menu during ongoing combat.'],
     ['The other player/world continue moving; normal play resumes on close.',
      'Intentional story/cinematic restrictions remain separate.'], []),
    ('05-revive', 'integration-pending',
     ['Down one player, approach/use, interrupt an attempt, then complete a later attempt.'],
     ['Animation and duration on both peers; one authoritative revival.',
      'Cancelled use/death-timer state restores; recovered player can move, aim and act.'],
     ['Engine startup loading and actual service invocation on both peers.', 'Scoped class/animation admission on both peers.',
      'Retirement before scene/control handoff; native deferred cleanup.']),
    ('06-scenes-travel', 'native-candidate',
     ['Complete real dialogue/cinematic, then move and fight.', 'Later change mission or reload.'],
     ['Host retains story/save authority; guest control/camera/collision return.',
      'Mission transition restores session/character without per-mission manual setup.'],
     ['Protected launch/profile/file guards must be re-established for each actual run.']),
]


def sha(path):
    h = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


def write(path, data):
    with path.open('x', encoding='utf-8') as stream:
        json.dump(data, stream, indent=2)
        stream.write('\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--script-receipt', type=Path, required=True)
    parser.add_argument('--ffmpeg', type=Path, required=True)
    args = parser.parse_args()
    private = (ROOT / 'local').resolve(strict=True)
    output = args.output.absolute()
    if output.exists() or not output.resolve().is_relative_to(private):
        parser.error('Output must be a fresh directory below repository local/.')
    for ancestor in output.parents:
        if ancestor.exists() and ancestor.is_symlink():
            parser.error('Symbolic output ancestors are forbidden.')
        if ancestor.exists() and getattr(ancestor.stat(), 'st_file_attributes', 0) & 0x400:
            parser.error('Reparse output ancestors are forbidden.')
    verify_current(ROOT)
    receipt_path = args.script_receipt.resolve(strict=True)
    if not receipt_path.is_relative_to(private):
        parser.error('Use a private repository compile receipt.')
    receipt = json.loads(receipt_path.read_text(encoding='utf-8-sig'))
    owner_assets = receipt.get('Status') == 'compiled-with-owner-assets-not-installed'
    if receipt.get('Status') not in ('compiled-not-installed', 'compiled-with-owner-assets-not-installed') or receipt.get('InputsUnchanged') is not True:
        raise RuntimeError('Unusable script receipt')
    if owner_assets and (receipt.get('ContainsOwnerGameAssets') is not True or
            receipt.get('DistributionAllowed') is not False or
            receipt.get('ScriptExecutableExportsPreserved') is not True or
            receipt.get('FactoryAnchorInjected') is not True or
            receipt.get('CompressedSequencePreserved') is not True or receipt.get('RelinkErrors') or
            receipt.get('AssetContract', {}).get('AnimSet') != 'ME3SquadCoopActions.ReviveAnimSet'):
        raise RuntimeError('Unusable private owner-asset derivation')
    for row in receipt['Classes']:
        if row.get('HasErrors') or row.get('Errors') or row.get('Warnings'):
            raise RuntimeError('Script diagnostics must be reviewed before staging')
    for name, expected in receipt['InputsAfter'].items():
        if sha(Path(name)) != expected.lower():
            raise RuntimeError('Script compilation input has changed')
    package = Path(receipt['Package']).resolve(strict=True)
    if not package.is_relative_to(private) or sha(package) != receipt['PackageSHA256'].lower():
        raise RuntimeError('Script package identity mismatch')
    ffmpeg = args.ffmpeg.resolve(strict=True)
    revision = subprocess.run(['git', 'rev-parse', 'HEAD'], cwd=ROOT, check=True,
                              capture_output=True, text=True).stdout.strip()
    output.mkdir(parents=True)
    artifacts = output / 'artifacts'
    artifacts.mkdir()
    staged = []
    for source in [*(ROOT / 'local/build' / name for name in
                     ('ME3SquadCoop.asi', 'ME3SquadCoopProbe.asi', 'sandbox_launch.exe', 'MinHook-LICENSE.txt')),
                   package]:
        before = sha(source)
        destination = artifacts / source.name
        shutil.copyfile(source, destination)
        if sha(destination) != before or sha(source) != before:
            raise RuntimeError('Staging copy changed identity; output is incomplete')
        staged.append({'source': str(source), 'staged': str(destination), 'sha256': before})
    shutil.copyfile(receipt_path, artifacts / 'compile-result.json')
    for key, readiness, actions, criteria, blockers in FEATURES:
        directory = output / key
        directory.mkdir()
        for child in ('host', 'guest', 'telemetry', 'review'):
            (directory / child).mkdir()
        write(directory / 'case.json', {
            'feature': key, 'readiness': readiness, 'actions': actions,
            'acceptance_criteria': criteria, 'activation_blockers': blockers,
            'run_status': 'not-started', 'gameplay_verified': False,
            'host_recording': None, 'guest_recording': None, 'telemetry': [],
            'recording_policy': {'scope': 'explicit game window HWND per peer',
                'container': 'mkv', 'audio': 'not-configured', 'inputs': 'human',
                'review': 'live snapshots and later timestamped video review; not continuous model vision'},
            'clock': {'video_start_utc': None, 'video_start_monotonic_ns': None,
                      'native_log_alignment': 'must be measured; not assumed'},
            'markers': 'markers.jsonl', 'verdict': 'unreviewed'})
    write(output / 'manifest.json', {
        'status': 'staged-not-installed', 'prepared_utc': datetime.now(timezone.utc).isoformat(),
        'revision': revision, 'artifacts': staged,
        'ffmpeg': {'path': str(ffmpeg), 'sha256': sha(ffmpeg),
                   'capture_started': False, 'window_binding': 'required-before-recording'},
        'game_executed': False, 'tests_run': False, 'installed': False,
        'contains_owner_game_assets': owner_assets, 'distribution_allowed': False,
        'note': 'These are review candidates. Staging is not activation, recording or gameplay evidence.'})
    print(json.dumps({'status': 'staged-not-installed', 'directory': str(output),
                      'feature_cases': len(FEATURES), 'recording_started': False}))


if __name__ == '__main__':
    main()
