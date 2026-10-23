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
outside DllMain. By default it attaches to an already established native
host/client world. An explicit guest `ME3_SQUAD_COOP_START=join` (or exported
`ME3SquadCoopRequestJoin`) now requests one ordinary engine `open peer:port`
command, using the configured peer and port. It waits for validated ordinary
standalone control, no driver and no pending travel, validates the reflected
console ABI/allocator, then marks the request consumed before invoking travel.
It selects no mission and never clears pause/story state. Dispatch is not proof
of a connected guest. This path still requires protected actual-entry testing.
A process accepts at most one join attempt; retry currently requires restart.

An explicit host `ME3_SQUAD_COOP_START=listen` (or exported
`ME3SquadCoopRequestListen`) now requests one **in-place native Listen** on the
current standalone campaign world. It waits for ordinary on-foot control, no
pause/conversation, no driver and no pending travel. It validates the current
world's root package against engine LastURL, pins the native selected driver,
and passes an engine-allocated deep FURL copy to the original Listen. The game
consumes that copy. There is no implicit mission reload, fabricated `listen`
console command, package GUID rewrite or validation bypass.

Both explicit starts require the native process argument
`-MULTIHOME=<configured bind IPv4>` before dispatch. The exact socket provider
reads the engine's own command-line buffer; setting an environment variable or
FURL.Host alone cannot select the local interface. The supported argument form
is one plain, unquoted argument, with canonical IPv4 matching the configuration.
Missing, conflicting, duplicate or ambiguous forms are refused. Existing-session
attachment also checks the native driver endpoint: exact configured host bind
and port, or the configured guest bind address. The native client leaves zero in
its cached local sockaddr when the OS selects an ephemeral port, so a nonzero
cached guest port is not an admission requirement. The runtime still requires
native client net mode and a reciprocal server connection. A guest's configured
port is the remote server port; this cache is not an observation of its actual
kernel socket port. This correction has offline native fixture coverage; genuine
ordinary-entry guest control remains to be tested.

The command-line buffer is now derived from the validated getter's RIP-relative
LEA. A protected ordinary-host smoke exposed that the previous constant pointed
0x20 bytes before the actual buffer and refused a valid MULTIHOME argument.
The smoke reached the bridge and refusal, not a verified listener. Host readiness
also now requires the captured `SFXPawn_Player` hierarchy; the earlier
`BioPlayerPawn` class does not occur in the observed Shepard hierarchy. Native
fixtures keep the old buffer address empty or conflicting and represent the
actual Soldier/Player/PlayerParty hierarchy, while rejecting Ashley and a generic
BioPawn. Compiled negative controls reject both old bugs and overly broad pawn
admission. These corrections still require a new protected in-game startup run.

Host startup leases reflected `GameInfo` capacity for two participants and checks
the actual bound endpoint. Native bind can advance an occupied port; a different
bound port fails startup. Before companion admission, the code invokes the same
original master package-map Init used by Listen with zero clients and verifies
the registered core package identities and unchanged native GUIDs. Disable or
failure restores capacity only for captured object identities and values still
owned by the lease; disable cleanup waits until no companion lease/clients remain.
A changed/disposed world never authorizes writes into a reused GameInfo.

A native failure or post-start verification failure is terminal for this mod
instance: companion admission stays closed and restart is required. A listener
already created by the engine remains engine-owned until the game exits; the mod
does not invent a teardown method. In-session listen retry is unsupported.
**The new host/guest startup paths still require protected actual-entry game
verification.** Their offline fixtures do not establish playable co-op or Radmin
reachability.

`ME3SquadCoopNetworkStartStatus` reports idle (0), waiting (1), command dispatched
(2), already networked (3), ABI refused (4), malformed request (5), reserved (6),
native bind argument refused (7), native host setup verified (8), capacity refused
(9), native Listen failed (10), or post-start verification failed (11).
None means a guest connection is verified. Unknown/truncated
startup requests also remain inert, with runtime error 4.

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

Host startup fixtures run the actual checked startup pipeline on allocated
synthetic objects using fixture-owned deep-copy/Listen/Init callbacks. They
exercise successful admission, native failure, occupied-port fallback, GUID
mutation refusal, one-time FURL consumption, capacity restoration, changed values
and reused object refusal. Compiled faults must be rejected by the same fixture.
These callbacks do not run the real engine, create sockets or verify a native
Listen call in a game process.

Run after a successful normal native build:

```powershell
python tests/native_probe_harness.py
python tests/native_runtime_harness.py <zig.exe>
python tests/native_runtime_mutants.py <zig.exe>
python tests/native_session_gate_mutants.py <zig.exe>
python tests/native_host_start_mutants.py <zig.exe>
```

Actual-entry game validation still requires the independent copy, original-file
locks, private profiles and bounded cleanup. The protected probe and ordinary
entry both own the same Tick slot, so loading them both in their normal modes is
invalid. The explicit protected `ME3_SQUAD_COOP_GUARDS_ONLY_QA=1` mode now
retains marker, exact-image, private-Documents and file-import guards before
skipping the action plan, client gameplay fallback and all probe Tick hooks.
The isolation role still selects private profiles and the scoped test mutex;
it does not select a gameplay role in this mode. Missing or `0` retains normal
probe behavior; malformed/truncated values fail closed. Only the ordinary entry
may own Tick. Its separate full fingerprint/configuration/DLC gates still apply.
The native fixture uses actual guard functions on an allocated synthetic PE,
verifies the substituted import slots, Documents routing and outside-write
refusal, then checks that even an otherwise valid Tick slot remains untouched.
It tests client fallback only in normal mode and rejects a missing private marker.
The two real ASIs have subsequently loaded together in a protected host-only
game run: the guards-only probe installed the private file/profile guards while
the ordinary entry owned Tick and opened native Listen in the current Mars
world. An independent current-world reader measured listen-server mode, and
Windows confirmed the actual process-owned UDP socket at the configured local
Radmin interface and port 17779. The run used a fixed private native map argument
for initial QA entry, with no research networking bootstrap or mission reload.
Temporary authored activation files were removed after bounded shutdown; the
original profiles were unchanged and no game remained. A later PlayersOnly
observation means uninterrupted control has not been established. No guest was
launched, and this establishes no cross-PC or campaign result.
Any research network bootstrap must be explicitly separate and preserved in
its evidence; it is not an ordinary-runtime host/join implementation.

Still required: protected ordinary guest-join and
actual-entry control/camera/disconnect runs, reliable equipment and authoritative
enemy damage, actual full conversations, mission transitions, two physical PCs
over Radmin, authored installable DLC, and Mod Manager install/disable/rollback.
Do not label the offline integration as a release.
