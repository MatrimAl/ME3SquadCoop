> AI-assisted development is used extensively. Contributions and claims still require human review and actual verification.

# ME3 Squad Co-op

Experimental campaign co-op research for **Mass Effect 3 in Mass Effect Legendary Edition (LE3)**.

Play toward a shared campaign where the host controls Shepard and guests control existing squadmates. The first milestone targets **one host + one guest**, initially Ashley. Expanding to more guests is a planned follow-up, not a currently supported feature.

**Status: research prototype. A complete playable campaign and a validated player installation are not available yet.** This repository publishes source and test tooling so the work can continue openly, with clear limits on what has actually been demonstrated.

A narrow Mars airlock guard keeps the authored transition's temporary player
lists ordered with the host Shepard first if a native two-player list puts the
guest first. It preserves the graph and actor transforms. This possible source
of the reported later pushback still needs an owner check; see
[airlock primary player](docs/MARS-AIRLOCK-PLAYER.md).

The next offline candidate also supplies the owned guest's native incoming
weapon-hit direction queue from an actual actor-channel impact notification.
It preserves native damage/reactions and filters initial, duplicate and stale
ownership state. Rendered direction feedback still needs an owner gameplay
check; see [hit direction](docs/GUEST-HIT-DIRECTION.md).

The owned guest also has a narrow dropped-ammo admission adapter for the
companion's disabled pickup flag. It keeps the general flag unchanged and
uses the stock capacity, quantity and host-authoritative pickup lifecycle.
Actual reserve gain and shared pickup behavior remain owner checks; see
[dropped ammo](docs/GUEST-AMMO-PICKUP.md).

An offline supply timing candidate also requests the three-second shared
respawn found in selected LEMP maps for qualifying native campaign containers.
The stock grant and visibility lifecycle remains in place. Native fixtures
passed; live replenishment and absolute multiplayer ammo amounts remain
unverified. See [shared ammo supply](docs/MP-AMMO-SUPPLY.md).

The current ordinary build adds Shepard-only admission for classified story
entries, guest entry warnings and a Mars shuttle close rule that waits for all
human players to leave the cabin. Native fixtures cover the adapter contracts;
live discovery, movement boundaries and animation remain owner checks. See
[story trigger authority](docs/STORY-TRIGGER-AUTHORITY.md) for scope and test steps.

It also registers an unassigned guest in the existing native host team and
retains human companion control during proven camera-free ambient lines.
In the protected manual `dbd7265` session, native team registration and ambient
control retention were observed. The owner confirmed damage in both directions
and that the small Ashley interaction no longer forcibly moves her. See
[team registration](docs/GUEST-TEAM-REGISTRATION.md) and
[ambient control](docs/AMBIENT-COMPANION-CONTROL.md).

In the protected manual `af2880c` session, the owner confirmed that reload/
power actions keep the guest shoulder distance and the early movement fence
opens after Shepard passes. Both peers admitted the owned weapon's human
damage multiplier. A possible one-round clip rebound was reported during
rapid fire/reload; the cause and visible HUD result remain under review.
In the later `b515e98` session, the owner reported good directional evades
and normal low-shield/break feedback. The evade camera still differed, X
weapon swapping failed, and powers stopped after weapon/slot changes.
The next candidate retains the shoulder scale through verified evades,
bridges owned guest swap input to stock NextWeapon, and resolves native named
slot moves through the guest power map. A refused drop no longer disables
the whole valid wheel. A measured reflected-layout cache and timing counters
address repeated metadata work; a frame-rate improvement is not yet proven.
These additions need gameplay checks; complete movement, animation and
weapon parity remain unproven. See [control changes](docs/GUEST-CONTROL-REFRESH.md),
[weapon and presentation scope](docs/GUEST-WEAPON-PRESENTATION.md) and
[movement scope](docs/GUEST-MOVEMENT-CONTEXT.md).

The owner subsequently confirmed X and power use after weapon switching in
`4ff0a2d`, with smoother play. Slot moves still failed, a rare power-camera
close-up remained and the ammunition rebound continued. The next correction
handles owned slot requests before their native destination clear and treats
the actual DoingDrag bit as the key-input blocker. An existing shoulder scale
survives a verified unpaused Command/PowerWheel menu. A bounded native firing
observer will correlate the remaining ammo issue; it does not repair counters.

In `5374e7c`, the owner confirmed the Shift-to-power camera correction but
could not move a power. The follow-up admits the separate native drag-end
assignment path and completes the stock local FullAuto stop on a real release.
The same candidate prepares normal guest melee combos and an Ashley-owned
damage passive. The follow-up adds guarded Soldier heavy/storm actions, normal
melee shoulder retention and retryable native cleanup. Guest heavy actions
cannot issue their specific campaign cosmetic write through the guarded path;
Shepard's native setter path is preserved. These new controls still need
gameplay validation. Cover grab and left/right normal/heavy edge actions are
also prepared from the loaded player's native classes, with independent asset
checks and temporary animation cleanup. Complete multiplayer mechanics and
campaign parity remain unfinished. See
[guest melee scope](docs/GUEST-MELEE.md) and [ammo evidence](docs/AMMO-REPLICATION.md).

A separate traversal lease also prepares the loaded player's mantle, four
cover-slip and ladder actions. Real inputs and the native geometry, animation,
physics and network routes perform these moves. Existing evades remain under
their previous lease. The new traversal and camera outcomes still need owner
gameplay checks; see [movement scope](docs/GUEST-MOVEMENT-CONTEXT.md).

The local FullAuto release adapter also cancels the exact named stock deferred
start during port-arms preparation, before completing the ordinary pawn stop.
It supports an as-yet-empty native inventory pending entry and rechecks the
owned scope after the native timer call. This follows the same multiplayer-like
press/hold/release target; it is not a gameplay-confirmed ammo fix.

The offline movement extension also separates running and standing gap jumps
and supplies the four native 90-degree cover corner actions missing from
Ashley's table. It uses the loaded player's classes and the engine's real
input, geometry and replication paths, with independent clip and ownership
checks. These additions are prepared for a later owner gameplay test.

The next owned reaction lease also prepares the player's light-hit and
shield-breach animation family. The six standard entries are changed together,
including the two absent player alternates, while the shield reaction remains
independent. Native damage and reaction selection are preserved. No game has
run these additions; see [guest hit reactions](docs/GUEST-HIT-REACTIONS.md).

The same reaction lease now also prepares the player's incoming melee/stagger
family, including its absent alternate, and keeps the native HitReaction
camera. It preserves each action's normal interruption and root-motion rules.
These offline additions still need owner gameplay checks.

A separate owned selector guard now applies the player's existing in-cover
knockback refusal to human Ashley. It executes the stock false-return branch
without changing damage or candidate arrays; ordinary stagger fallback remains.
The rule is prepared offline and needs a later owner gameplay check.

The next native reaction pair also supplies the player's missing on-fire
actions. The exact numbered animation name is preserved and must already be
on the current mesh. Native fire damage, cooldown and timeline rules remain
unchanged. Actual fire-hit presentation and recovery still need owner testing.

The current build binds the guest's existing native Shepard HUD player panel
to owned Ashley, with the previous health/shield display retained as fallback.
Native keyboard power slots/icons remain enabled. Shift Command mode now retains
co-op ownership/context. Native appearance and power effects await owner testing;
see [guest player controls](docs/GUEST-PLAYER-HUD.md).
The same scoped controller preparation now also covers the host's authoritative
Ashley power instances; live power effects and AI recovery remain owner checks.
Both peers also receive a teammate health/shield readout with native downed/dead
status: Ashley on the host and Shepard on the guest. Replica readings can remain
unknown; visual appearance and update behavior await owner testing.
It also includes a scoped fix for a queued fire start surviving a real guest
fire release during weapon preparation. Live cancellation and a fresh press
afterward remain owner checks; see [firing scope](docs/NATIVE-FIRE-OBSERVATION.md).
The revive candidate now prepares both players on both peers and opens the
normal interaction path after a native connection-scoped agreement. It needs
the separately prepared private owner-derived addon on both peers; the public
archive does not contain game animation assets. In-game revival remains an
owner check. See [revive integration](docs/REVIVE-NATIVE-BRIDGE.md).

The ordinary runtime now connects the local host/guest pair and retains guest
Ashley control. Remote Shepard weapon/mask presentation is corrected, authored
shuttle actors survive guest loading, and the measured scene-return collision
variants are admitted. The normal guest now also includes the previously
protected [owned movement-payload correction](docs/OWNED-OPTIMIZED-RECEIVE.md).
This latest integration passed native fixtures; gameplay confirmation is
pending owner testing. A completed shuttle-door gesture still differs on the
guest. See [current validation](docs/VALIDATION.md) for the remaining scope.

## The goal

- Keep the host authoritative over story choices, world state and saves.
- Give guests control of existing squad members without duplicating Shepard or disrupting the story roster.
- Support shared combat, dialogue/cinematic handoffs and mission progression.
- Connect by direct IP over LAN or Radmin VPN.
- Deliver a shareable, reversible mod with ME3Tweaks Mod Manager support.
- Later, extend session ownership and squad assignment to more than one guest.

Guest control should feel like official ME3 multiplayer through normal movement,
camera, weapon and power inputs. The campaign remains host-authoritative. The
external panel and optional power shortcuts are supporting tools; completion
requires normal gameplay controls. See the [player experience criteria](docs/CAMPAIGN-PLAYER-EXPERIENCE.md).

## What has been demonstrated

These observations come from bounded tests with real game processes on one Windows PC. They do not establish full campaign support.

| Area | Observed result | Limit |
| --- | --- | --- |
| Native connection | Host and guest connected through LE3's UE3 networking | Cross-PC play untested |
| Ordinary startup | Separate runtime opened a native listener and connected the local guest in the current Mars world | Full campaign and cross-PC behavior remain unverified |
| Squad control | Guest Ashley possession; one protected run admitted a21-second shared walking interval and retained133 seconds of control in one epoch | Route-wide synchronization and mission progression unverified |
| Player context | Guest-local squad context initialization and cleanup | Equipment readiness still inconsistent |
| Cameras | Shoulder-camera adaptation and actual intro Matinee camera motion/cleanup | Full conversations unverified |
| Session lifecycle | Synthetic scene release/reacquire and ordinary disconnect restored native AI | Synthetic scene gates are not real dialogue tests |
| Weapon input | Earlier runs consumed authoritative ammunition after guest firing input | Stop-firing reliability and ammo agreement failed in some runs |
| Isolation | Independent game copy, original-file read locks and private profile routing used | These protections must be established again for every test |

Earlier equipment runs produced mixed firing, stopping and ammunition results. In the later protected manual `dbd7265` session, the owner confirmed enemy damage in both directions. That bounded result does not establish weapon parity or sustained combat across the campaign. Recent experiments also include native crashes; successful isolation checks must not be mistaken for gameplay success. See [progress notes](docs/PROGRESS.md).

## Current work

Gameplay is the priority. The current blocker is usable squadmate control after a scene: actual landing release/reacquisition has been observed, but Ashley can retain incomplete collision and fall or move out of sync. The latest walking evaluator rejected shared progression despite passing ownership handoffs. One protected run exercised the exact-shape collision repair and retained ground contact, but guest movement still failed. A later run captured a large position discontinuity inside the unchanged native receive call, followed by falling; the exact replicated field or codec remains unproven. See [collision restoration](docs/COLLISION-RESTORATION.md). Recent local-pair tests also exposed a custom focus-loss pause before possession; a guarded candidate now has protected live getter and mode-0 landing-scene observations; movement and full dialogue remain unverified. See [background focus](docs/BACKGROUND-FOCUS.md).

Combat is also incomplete. Earlier runs exposed authoritative fire deferral while the port-arms animation flag remained set. The owner later confirmed bidirectional enemy damage in `dbd7265`; powers, full weapon parity and sustained mission progression still require gameplay evidence. The new weapon, camera, shield-feedback and passage changes have passed native checks but have not been tested in a game.

The separate ordinary runtime now connects the same-PC guest without reloading
the host's current Mars mission. The current authored activation resolves its
config and TLK in the protected copy; earlier activation failures remain
documented. A validated player installation and cross-PC Radmin behavior are
still pending. See [runtime integration](docs/RUNTIME-INTEGRATION.md) and
[activation packaging](docs/DLC-PACKAGING.md).

1. Verify scene return with healthy collision and sustained, synchronized walking.
2. Make equipment/animation readiness repeatable; verify firing and stopping on both sides.
3. Test a real full conversation while the guest owns a squadmate, then verify camera/control restoration.
4. Extend the confirmed bidirectional damage result to sustained combat; verify powers, progression and mission transitions.
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

