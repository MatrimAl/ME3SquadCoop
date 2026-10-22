# Runtime integration status

There is no verified shareable gameplay runtime yet. Native filename and archive
validation do not establish playable co-op.

## Separate native entries

`build.py` now compiles `ME3SquadCoopProbe.asi` from `squad_probe.c` and
`ME3SquadCoop.asi` from `squad_coop.c`. The packaging fallback also uses the latter
source. The ordinary entry is no longer an inert DLC scaffold or a renamed
protected probe. Its supported executable requires both the complete on-disk
SHA-256 and the existing in-memory timestamp/landmark checks before hooks.
The successful build fingerprint covers both ASIs and the protected launcher.

The ordinary entry requires an enabled DLC file and explicit configuration:
`ME3_SQUAD_COOP_ENABLED=1`, role `host` or `guest`, an explicit bind IPv4 and
port, and a peer IPv4 for a guest. Configuration rejects loopback, missing
endpoints and QA-role defaults. The endpoint parser opens no socket. DLC file
presence is an installation gate, not proof that the engine mounted a valid DLC.
Missing/disabled configuration or DLC at startup installs no bridge.

The exact-build bridge forwards native Tick once and performs adapters on the
game thread. Startup fingerprinting and bounded bridge discovery run on a worker,
outside DllMain. The bridge currently **attaches only to an already established
native host/client world**. It does not yet consume the configured endpoint to
start listening or joining. No implicit story travel or map selection occurs.
Thus valid configuration plus a bridge is still insufficient for playable co-op.

## Shared ownership and presentation

`squad_session_core.h` contains native peer membership, captured object identity,
Ashley acquisition through base `PlayerController.Possess`, host-story invariants,
scene release/reacquisition and native connection-cleanup handoff. Entry policy
callbacks keep the protected probe's original warmup, synthetic scene gates and
QA equipment observations outside the ordinary entry. The ordinary entry also
uses the existing guest camera, local player-context, cinematic-registration and
normal equipment-handoff implementations. These are shared code paths, not new
claims that actual dialogue or combat now works.

The ordinary policy has no movement/firing bots, timed synthetic scene gate,
forced disconnect, `test-action.txt`, sandbox marker, private Documents routing,
probe role/environment fallback, ammo correction or native firing observer.
It binds the driver through the actual current world and checks native net mode.
A world change or ownership fault stops the lease conservatively; continuous
mission travel is not yet verified or promised.

`WorldInfo.bPlayersOnly`, unreadable pause state, native cinematic mode and active
conversation state prevent host control. The PlayersOnly pause also disables
guest adapters, letting their existing cleanup run. The code waits for normal
engine restoration and never clears this flag or dismisses message boxes.
Cleanup now also requires the captured pawn identity before calling UnPossess;
an unchanged PC pointer alone cannot justify touching a reused pawn.

The exported enable request defers cleanup to Tick. Removing the DLC while a
bridge exists requests cleanup on the next gate poll (at most one second).
The small forwarding bridge remains resident until the game closes: runtime
unloading is unsupported. `ME3SquadCoopActive` reports the enabled-DLC file gate;
bridge and runtime-error exports are separate, and none is a campaign verdict.

## Offline evidence and remaining verification

The native ordinary-entry harness exercises actual shared cleanup using synthetic
objects and a fixture-owned ProcessEvent: UnPossess precedes AI Possess, story
pointers remain unchanged, reused pawn and changed world trigger no call, and
handoff remains pending until driver membership removal is observed. It also
checks normal Tick forwarding, configuration policy, SHA-256 including wrong or
missing files, camera/cinematic predicates, DLC-file gating and absence of probe
sentinels/exports from the ordinary binary. This starts no game. Deliberately
compiled cleanup faults must fail the same fixture; these are not engine tests.

Run after a successful normal native build:

```powershell
python tests/native_probe_harness.py
python tests/native_runtime_harness.py <zig.exe>
python tests/native_runtime_mutants.py <zig.exe>
python tests/native_session_gate_mutants.py <zig.exe>
```

Actual-entry game validation still requires the independent copy, original-file
locks, private profiles and bounded cleanup. The protected probe and ordinary
entry both own the same Tick slot, so loading them both in their normal modes is
invalid. A future explicit probe **guards-only** mode must retain its existing
isolation checks, then skip action-plan/client-role orchestration and Tick
installation. Only the ordinary entry would own Tick. Current stage zero alone
is insufficient because the client test role forces the probe's join stage.
Any research network bootstrap must be explicitly separate and preserved in
its evidence; it is not an ordinary-runtime host/join implementation.

Still required: ordinary listen/join startup using explicit endpoints, protected
actual-entry control/camera/disconnect runs, reliable equipment and authoritative
enemy damage, actual full conversations, mission transitions, two physical PCs
over Radmin, authored installable DLC, and Mod Manager install/disable/rollback.
Do not label the offline integration as a release.
