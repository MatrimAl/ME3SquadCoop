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

These observations come from bounded tests with **two real game processes on one Windows PC**. They do not establish full campaign support.

| Area | Observed result | Limit |
| --- | --- | --- |
| Native connection | Host and guest connected through LE3's UE3 networking | Cross-PC play untested |
| Squad control | Guest Ashley possession and short replicated movement | Sustained mission progression unverified |
| Player context | Guest-local squad context initialization and cleanup | Equipment readiness still inconsistent |
| Cameras | Shoulder-camera adaptation and actual intro Matinee camera motion/cleanup | Full conversations unverified |
| Session lifecycle | Synthetic scene release/reacquire and ordinary disconnect restored native AI | Synthetic scene gates are not real dialogue tests |
| Weapon input | Earlier runs consumed authoritative ammunition after guest firing input | Stop-firing reliability and ammo agreement failed in some runs |
| Isolation | Independent game copy, original-file read locks and private profile routing used | These protections must be established again for every test |

Equipment results remain mixed. One audit produced no eligible fire pulses; later runs observed firing and stopping with differing counts, then two pulses with matching consumed/reserve counts. This does not prove the weapon problem solved. Recent completed audit runs recorded no native exceptions or original-profile changes.

## Current work

Protected runs have both failed before squadmate acquisition and exercised synthetic handoffs. Measured changes reduced repeated function discovery and cold channel-lookup stalls. The latest pair retained control/handoff/disconnect behavior and observed firing and stopping, but ammunition counts still differed after the second pulse. Read-only telemetry records actual weapon-channel identities and sampled update state. Startup repeatability and equipment synchronization remain immediate blockers. See [progress notes](docs/PROGRESS.md).

1. Make equipment initialization repeatable, including inventory links and action-state readiness.
2. Verify both firing and stopping, with matching host/guest ammunition observations.
3. Test a real full conversation while the guest owns a squadmate, then verify camera/control restoration.
4. Verify enemy damage, powers, sustained progression and mission transitions.
5. Test two physical PCs over Radmin.
6. Validate installation, disable/uninstall and rollback before releasing a gameplay package.
7. Expand beyond two participants: per-guest squad assignment, independent ownership leases, disconnect handling and capacity/replication tests.

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
| `src/function_lookup_cache.h`, `src/lookup_profile.h` | Live-validated discovery cache and opt-in CPU-path wall-clock measurements |
| `src/combat_*` | Equipment observations and opt-in firing test input |
| `src/*verification.py` | Evidence evaluators; labels alone do not count as proof |
| `src/safe_workspace.py`, `src/share_guard.py` | Independent-copy verification and original-file locks |
| `src/sandbox_launch.c` | Bounded native launcher |
| `boot_probe.py`, `pair_probe.py` | Private-workspace research orchestration |
| `tools/` | Configuration and packaging tooling |
| `tests/` | Synthetic Python tests and separate Windows native harness |

`src/squad_coop.c` is an experimental DLC-gate scaffold. The current build script builds both ASI variants from `squad_probe.c`; the release-named output is not evidence of release readiness.

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

