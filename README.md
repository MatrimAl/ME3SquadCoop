> AI-assisted development is used extensively. Contributions and claims still require human review and actual verification.

# ME3 Squad Co-op

Experimental campaign co-op research for **Mass Effect 3 in Mass Effect Legendary Edition (LE3)**.

Play toward a shared campaign where the host controls Shepard and guests control existing squadmates. The first milestone targets **one host + one guest**, initially Ashley. Expanding to more guests is a planned follow-up, not a currently supported feature.

**Status: research prototype. A complete playable campaign and a validated player installation are not available yet.** This repository publishes source and test tooling so the work can continue openly, with clear limits on what has actually been demonstrated.

## The goal

- Keep the host authoritative over story choices, world state and saves.
- Give guests control of existing squad members without duplicating Shepard or disrupting the story roster.
- Support shared combat, dialogue/cinematic handoffs and mission progression.
- Connect by direct IP over LAN or Radmin VPN.
- Deliver a shareable, reversible mod with ME3Tweaks Mod Manager support.
- Later, extend session ownership and squad assignment to more than one guest.

## What has been demonstrated

These observations come from bounded tests with real game processes on one Windows PC. They do not establish full campaign support.

| Area | Observed result | Limit |
| --- | --- | --- |
| Native connection | Host and guest connected through LE3's UE3 networking | Cross-PC play untested |
| Ordinary host startup | Separate runtime opened a native listener in the current Mars world; Windows confirmed the requested local Radmin interface and port | Ordinary guest connection and control still need validation |
| Squad control | Guest Ashley possession and short replicated movement | Sustained mission progression unverified |
| Player context | Guest-local squad context initialization and cleanup | Equipment readiness still inconsistent |
| Cameras | Shoulder-camera adaptation and actual intro Matinee camera motion/cleanup | Full conversations unverified |
| Session lifecycle | Synthetic scene release/reacquire and ordinary disconnect restored native AI | Synthetic scene gates are not real dialogue tests |
| Weapon input | Earlier runs consumed authoritative ammunition after guest firing input | Stop-firing reliability and ammo agreement failed in some runs |
| Isolation | Independent game copy, original-file read locks and private profile routing used | These protections must be established again for every test |

Equipment results remain mixed: bounded runs have produced native firing and matching ammunition counts, while unchanged-build repeats have failed. Reliable firing, stopping and enemy damage remain unverified. Recent experiments also include native crashes; successful isolation checks must not be mistaken for gameplay success. See [progress notes](docs/PROGRESS.md).

## Current work

Gameplay is the priority. The current blocker is usable squadmate control after a scene: actual landing release/reacquisition has been observed, but Ashley can retain incomplete collision and fall or move out of sync. The latest walking evaluator rejected shared progression despite passing ownership handoffs. The collision candidate now cancels stale released state and reports separate refusal checks; its native fixtures pass, but a positive in-game repair is still unverified. See [collision restoration](docs/COLLISION-RESTORATION.md). Recent local-pair tests also exposed a custom focus-loss pause before possession; a guarded candidate builds and passes native fixtures, with live verification pending. See [background focus](docs/BACKGROUND-FOCUS.md).

Combat is also incomplete. Native guest firing requests have reached the authoritative host, but the host can defer firing while the port-arms animation flag remains set. A previous positive ammunition run does not establish repeatability, powers or enemy damage. Full conversations and sustained mission progression still require real gameplay evidence.

The separate ordinary runtime opened a native host listener without reloading the current Mars mission, but ordinary guest connection/control has not been validated. The activation DLC also failed engine acceptance. Protected gameplay research therefore remains separate from an end-user installation. See [runtime integration](docs/RUNTIME-INTEGRATION.md) and [activation packaging](docs/DLC-PACKAGING.md). Radmin validation and package delivery follow the gameplay work.

1. Verify scene return with healthy collision and sustained, synchronized walking.
2. Make equipment/animation readiness repeatable; verify firing and stopping on both sides.
3. Test a real full conversation while the guest owns a squadmate, then verify camera/control restoration.
4. Verify enemy damage, powers, sustained progression and mission transitions.
5. Test two physical PCs over Radmin.
6. Validate installation, disable/uninstall and rollback before releasing a gameplay package.
7. Expand beyond two participants with per-guest squad assignment and capacity tests.

There is no promised completion date. See [validation notes](docs/VALIDATION.md) for the evidence boundaries.

## Supported research identity

The native code contains offsets and function signatures for one observed build:

- Game: LE3, not the original 2012 ME3 executable
- Platform: Windows x64; Unreal Engine 3
- Executable version: `2.0.0.48602`
- Executable SHA-256: `b453871eb6eacff0c646490ffbc588f743399a829b1f9e31cd7775dc95da8001`

Other builds require fresh identification and validation. Do not reuse offsets based only on the game name.

## Source layout

| Path | Purpose |
| --- | --- |
| `src/squad_probe.c` | Native research ASI and bounded experiment hooks |
| `src/session_control.h` | Guest possession leases, handoff and disconnect restoration |
| `src/guest_*` | Player-context, camera, cinematic and equipment adapters |
| `src/ammo_change_trace.h` | Opt-in read-only ammunition changes, channel identities and sampled update state |
| `src/ammo_wire_trace.h`, `src/ammo_wire_verification.py` | Opt-in native field encoding/decoding observations and conservative evidence summaries |
| `src/ammo_reconcile.h`, `src/ammo_reconcile_protocol.h`, `src/ammo_reconcile_verification.py` | Default-off owner-controller snapshot requests, guarded guest counter correction and separate roundtrip evidence |
| `src/fire_rpc_trace.h` | Default-off native script-entry observations for class/state weapon methods; internal VM calls may bypass these entries |
| `src/function_lookup_cache.h`, `src/lookup_profile.h` | Live-validated discovery cache and opt-in CPU-path wall-clock measurements |
| `src/combat_*` | Equipment observations and opt-in firing test input |
| `src/*verification.py` | Evidence evaluators; labels alone do not count as proof |
| `src/safe_workspace.py`, `src/share_guard.py` | Independent-copy verification and original-file locks |
| `src/sandbox_launch.c` | Bounded native launcher |
| `boot_probe.py`, `pair_probe.py` | Private-workspace research orchestration |
| `tools/` | Configuration and packaging tooling |
| `tests/` | Synthetic Python tests and separate Windows native harness |

`src/squad_coop.c` is a separate experimental ordinary runtime entry, sharing ownership/cleanup and camera/context adapters with the protected probe. It attaches to an existing native host/client world by default and has explicit one-shot guest join and in-place host Listen requests. Native interface selection requires a matching process `-MULTIHOME` argument, and actual driver endpoints are checked. Protected actual-entry host startup and local binding have been observed; ordinary guest startup and control remain unverified. `build.py` compiles each ASI from its own entry. See [runtime integration](docs/RUNTIME-INTEGRATION.md); an output filename does not establish release readiness.

## Build from source

Requirements: Windows x64, Python 3.10+ (the tested Python runtime), and [Zig 0.15.2](https://ziglang.org/download/). Fetch dependencies from their official sources and verify their published checksums. Compiler/runtime downloads are not included.

```powershell
python build.py C:\Tools\zig-0.15.2\zig.exe
```

Build outputs and their current-source fingerprint are written to ignored `local/build/`:

- `ME3SquadCoopProbe.asi`
- `ME3SquadCoop.asi`
- `sandbox_launch.exe`

Building does not launch or install anything. Do not install these research outputs into your original game folder or share them as a finished mod.

Run the synthetic Python suite without launching the game:

```powershell
python -m unittest discover -s tests -p "test_*.py" -v
```

The archive integration test additionally requires a built ASI and 7-Zip; it skips when the ASI is absent. The Windows DLL harness is a separate script, run only after a successful build:

```powershell
python tests/native_probe_harness.py
```

The harness exercises DLL guards and temporary fixtures. Neither command establishes in-game campaign support.

## Running research experiments

The launch scripts currently expect a sibling `coop-anything/local/me3-le/` private workspace with independently verified copy manifests, private profile configuration and isolated runtime files. A fresh clone does not include or manufacture those prerequisites. Do not launch the game until every protection is set up and verified. See [development and safety](docs/DEVELOPMENT.md).

No game files, saves, personal profiles, runtime logs or third-party mod assets are distributed here. No account, matchmaking or public service bypass is part of the project. Package/GUID validation remains intact.

## Contributions welcome

Start with [CONTRIBUTING.md](CONTRIBUTING.md), report a bug or propose research using the [issue forms](https://github.com/MatrimAl/ME3SquadCoop/issues/new/choose), and open a draft PR against `dev/campaign-coop`. Code, tests, research, documentation and translations are welcome.

## Development workflow

`main` records reviewed source snapshots. Active work starts on `dev/campaign-coop`; subsequent changes should use focused branches and be reviewed before merging. Keep implementation, tests and evidence limits together. Never promote a synthetic success into a claim about a real mission.

For bug reports, include the exact build, reproduction steps and sanitized observations. Do not upload saves, copyrighted assets, access tokens or unredacted runtime logs.

## References and attribution

- [Co-op Anything](https://github.com/cranebeaker/coop-anything): research workflow and shared knowledge.
- [LEMP ASI sources](https://github.com/PayDay42/lemp-asi): reference research for LE3 networking; not a runtime dependency. Preserve upstream notices for any future incorporated code.
- [Original ME3 campaign co-op proof of concept](https://www.nexusmods.com/masseffect3/mods/723): historical reference for the original game, not proof of LE3 compatibility.
- [ME3Tweaks](https://me3tweaks.com/): intended packaging ecosystem; install/disable/rollback integration still needs validation.

This is an unofficial community experiment. Mass Effect and its game assets belong to their respective owners. Project-owned code is available under the [MIT License](LICENSE). Third-party material retains its own terms; game assets are not included.

