# Runtime integration status

The next default-off candidate calls the shipped `SFXGame.LoadPackage` once
for `BioChar_Player`, after the guest's ordinary readiness window and before
native join travel. It reuses the host loader's exact reflected parameter
contract, additionally pins the measured native entry RVA0xb6cfa0, and refreshes
readiness after the call. No map, cached-object, role or actor field is written.
Three native targets built and the ordinary no-game harness passed. Live
results for this candidate are pending; it is not a startup or visibility fix
until the retained channel/cache observations establish its effect.

A further unchanged-code pair from `1847930` reproduced the failure and
captured the existing static map on both peers. Both maps selected entry194,
relative index1144, with matching GUID and object-count1352. All six paired
post-receive snapshots found the host's cached archetype at that index and a
null guest cache entry. The guest Shepard channel remained without an Actor.
This narrows the missing-reference case, but post-receive observations alone
do not prove which internal resolver branch ran during initial decode.
The first private sampler called entry+8 a linker; exact-image constructor
RVA0xc5470 shows entry+0 is the input object's FName and entry+8 is that object
pointer. The raw evidence is preserved and subsequent sampler labels corrected.

Two completed ordinary pairs from `8f2d10e` now distinguish the intermittent
initial-object failure. Both forwarded a static reference (tag0) with index
577545 for the channel independently mapped to host Shepard. In the first
pair, channel17 decoded `BioChar_Player.Archetypes.Soldier.MaleShepard_Soldier`,
native receive formed the player, and six later snapshots retained its guest
replica. In the second, channel9 decoded null, native flags changed1 to9,
Actor/Class remained null, and six later snapshots retained that failure.
Channel numbers must be mapped for each run; channel9 in the first pair was a
Matinee actor. Neither observation proves a gameplay repair or rendered host
appearance. A mesh component alone does not prove its mesh asset is populated.

The failing nonzero static index excludes an encoded dynamic-channel null in
this sample. Existing exact-image Level codec and resolver exports identify
the next distinction: archive error, package-entry/linker/cache resolution,
streamed-level filtering or requested-class rejection. No branch has yet been
proven responsible. A focused export of archive forwarder RVA0x464910 confirms
its archive+0xb0 connection -> connection+0xf0 map route. The retained Ghidra
database stayed byte-identical; no new whole-binary analysis was performed.
Both pairs closed at their owned deadlines with zero recorded native
exceptions, original profiles unchanged, sandbox base TOC restored byte-identical,
temporary authored files removed and no games or cleanup errors remaining.

The same initial-object opt-in now additionally observes the existing native
one-bit reference tag and bounded index outputs. Exact scalar ABIs, two-slot
installation pins and rollback policy are reused from the protected inventory
reference observer; the ordinary adapter scopes them only to the already
active initial-object decode and exact native caller addresses. It never reads
or replays packet bits. Original arguments and outputs are forwarded once,
with LastError preserved after observation; unrelated calls only forward.
Three native targets built and the ordinary harness passed, including actual
tag/index forwarding, caller filtering and output/error-state checks. The two
live results above establish bounded tag/index capture. This does not enable appearance initialization
or repair a null actor.

The ordinary initial-object observer run from `cb5960b` captured a readable
null output from the first native reference decoder for channel9, followed by
native receive changing flags1 to9 while Actor/Class remained null. Six
separate paired channel snapshots identify that channel as current host
Shepard's counterpart. This reproduces the earlier `d14faf7` decode-failure
stage; it precedes the formed-player/null-mesh condition handled by the
historical appearance experiment. Reference tag/index remain unavailable in
this integration, so the underlying unresolved reference is still unknown.
The old appearance path cannot act on a null Actor and has not been blindly
enabled or replaced. Normal guest movement/camera remain owner-reported working.
Native connection remained until the guest deadline, and terminal cleanup
preserved original profiles, restored the sandbox base TOC byte-identical,
removed temporary authored files and left no games/native exceptions/errors.

The owner reported normal guest WASD movement and mouse camera working in the
successful ordinary pair, but could not see host Shepard. Treat that as owner
input evidence, not independently measured movement/combat. No input bootstrap
change is justified by that report.

History review identified `73adb0a`/`464dde3` and evidence `f9b2aff`: their
protected appearance experiment handles an existing remote player with a null
skeletal-mesh asset. It is not wired into the ordinary entry, and its recorded
positive scope was asset application, not full customization/render proof.
The latest unchanged ordinary pair instead yielded six read-only snapshots
where host Shepard's channel mapped to a valid guest channel but no guest Actor
pointer. Both peers remained connected until bounded shutdown. This earlier
stage cannot be fixed by calling appearance code on a nonexistent actor.

The existing initial-object decoder observer is now reused by the ordinary
receive forwarder under default-off `ME3_SQUAD_COOP_INITIAL_OBJECT_TRACE=1`.
The guest installs before connection-scope readiness, records the native first
reference output and post-receive actor/class/flags, then forwards unchanged.
It reuses existing exact pins, bounded rows and synchronous output copying;
ordinary mode installs no scalar hooks, so tag/index inputs remain unknown.
No decode replay, payload/actor/channel mutation or synthetic input is added.
Three targets rebuilt and ordinary/protected no-game harnesses passed, with the
shared observer selftest also invoked by the ordinary fixture. Live decode
output for this ordinary integration remains pending.

The opt-in stock morph-package load succeeded in an ordinary-entry pair from
`0a03937`: the resident GUID changed to the disk/cache GUID through the shipped
LoadPackage API (native entry RVAb6cfa0), with no mod GUID writes. The guest
established native connection scope and join status3. Host assigned Ashley,
released her for a scene with AI/story authority preserved, reacquired her,
then restored AI/story state on the bounded guest's disconnect. Recorded
ownership intervals were31.781s and47.859s. This is one successful same-PC
ordinary-entry run, not sustained movement/combat or full campaign support.

The native connection scope remained until the guest deadline; both launchers
returned their expected timeout code124. Thus the disconnect in this run
coincides with bounded test shutdown, rather than the prior version-mismatch
rejection. Terminal audit restored the sandbox base TOC byte-identical,
preserved the original profile and left no game processes, native exceptions
or cleanup errors. The package preload is still opt-in and targeted; broader
mission coverage and normal player controls require separate evidence.

A focused default-off host candidate (`ME3_SQUAD_COOP_HOST_PACKAGE_PRELOAD=1`)
now calls the shipped public static SFXGame.LoadPackage once for the observed
morph package before Listen. It requires an idle standalone host, current game
actor, exact reflected function flags/index/parameter layout and the sole
PackageName string property. The engine-owned native entry must be within the
supported executable. The before/after resident GUIDs and native entry RVA are
logged; dispatch alone is not loading success. No GUID, file, package-map or
validation flag is written by the candidate. Player/world context is refreshed
on the following Tick before ordinary startup continues. The function pointer
is captured before invocation for logging, rather than read from an old
function object after loading. This is a targeted experiment, not a general
dependency-preload policy. Three native targets built and the ordinary runtime
harness passed with the option disabled; actual invocation/ABI admission and
its effect on the mismatch remain to be tested in-game.

Read-only follow-up ruled out a changed private morph file: its SHA-256 matches
both the original source and saved independent-copy baseline. The base
GuidCache export also contains the disk/guest GUID for this exact package,
rather than the different resident host GUID. The discrepancy is therefore in
the observed runtime package state; its creation/loading cause is unresolved.
Two selected native map/cache functions exported successfully from the retained
unchanged database. The selected SFXGame.LoadPackage function is a public static
final native declaration taking a string; its script declaration is not the
native implementation or evidence that calling it fully loads an existing
placeholder. No new game run or metadata correction was performed for this
follow-up. Keep package validation enabled while establishing the normal load
path before considering a targeted change.

The first completed event-observed ordinary pair from `cc1e555` captured a
native kind4 Connection Lost event naming a version mismatch for
`BIOG_HMF_HED_PROMorph_R`. The package GUID in the event matches the sole found
disk file; the differing advertised info GUID matches the stable host resident
package's+0x6c field. Thus this run's immediate rejection is a concrete package
identity disagreement. Why the resident identity differs remains unproven;
an incompletely initialized package is a hypothesis, not permission to rewrite
GUIDs or disable validation. No native connection interval was observed.

An earlier attempt had a reused Windows PID colliding with an existing
CREATE_NEW log, so host readiness could not be observed and no guest launched.
Its coordinator exited and restored the sandbox TOC; it is not a connection
test. The private retry preserved42 old PID logs with hashes before launching
any game, avoiding overwrites and stale evidence. Both completed-pair encoders
exited0. Final audit confirms exact base-TOC restoration, unchanged original
profile, no remaining games, native exceptions or cleanup errors.

An optional guest-only `ME3_SQUAD_COOP_JOIN_EVENT_TRACE=1` observer now records
the supported Engine.UpdateProgress virtual's title/message arguments at event
time. It is distinct from the existing post-Tick viewport-slot polling, which
may miss transient messages. Installation requires the current engine identity,
exact vtable/slot and16 supported entry bytes. A resident compare/exchange lease
forwards the native callback once with unchanged self/kind/string pointers and
preserves LastError across observation. No text pointer is retained or freed.
Recording requires the admitted engine and game thread within the existing
60-second join observation window, with at most16 process-wide event rows.
Kinds and strings are evidence, not automatic rejection/success classification.
Default configuration installs no progress observer. Three native targets built
and the ordinary runtime harness passed, including actual callback forwarding,
argument/output error-state behavior, expiry/wrap and record-budget checks.
This revision has not yet been exercised in-game.

The combined invariant-DLC/additive-base-TOC experiment now resolves and opens
the authored config on both roles. A sampled guest frame has no corrupt-DLC
modal, but ordinary join still returns to EntryMenu without an observed native
connection interval. Native connection failure diagnostics are the next work;
do not report the ordinary guest as connected. The sandbox base TOC was
restored byte-identical after process cleanup. See [package evidence](DLC-PACKAGING.md).

An earlier ordinary English-DLC candidate pair opened a native host listener,
but the guest never dispatched its join request: its story/player readiness
predicate remained blocked. A corrected full-size guest recording was black;
the earlier recording captured the tiny initial window. These observations
do not establish English DLC acceptance or a native connection. Cleanup left
no game processes, preserved the original profile, and removed the temporary
authored ASI and DLC without cleanup errors.

Join-wait logging now reports the specific failed story, pawn, squad, identity
or world-clock predicate and its monotonic time. Admission conditions and the
stable-player observation window are unchanged. Three native targets built
and the ordinary runtime fixture passed, including exact rejection-reason
checks. Its subsequent live ordinary-entry run dispatched guest join after five
samples/1000ms, then showed a named corrupt-DLC dialog and returned to EntryMenu.
No native connection interval was observed. The initial player-pawn wait and
later stable-player window were logged; earlier cinematic gates still return
before that diagnostic site, so the trace is not a complete history of every
readiness condition. See [the package result](DLC-PACKAGING.md).

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
The original join API remains one-shot. The explicit exported
`ME3SquadCoopRequestRejoin` queues a retry after an earlier attempt. Its return
value means only that intent was queued. Tick refuses an existing network world,
waits at most30 seconds for standalone player readiness, no driver/pending
travel and completed camera/context/cinematic cleanup, then rearms the same
ordinary join path with a fresh stable-player window. It never disconnects an
existing peer, clears faults or automatically retries. Startup statuses12/13/14
report retry waiting/refusal/expiry; ordinary join statuses resume once rearmed.
No player-facing reconnect button is wired yet. This candidate is compiled;
request fixture execution and actual disconnection/rejoin remain deferred.

Ordinary guests also have a default-off profile-aware native input initializer,
enabled by `ME3_SQUAD_COOP_INPUT_BOOTSTRAP=1` or the planner's guest-only
`--input-bootstrap` option. It only considers a stable empty local input list
with the current owned Ashley, valid profile/PRI and ordinary scene context.
It consumes one native UpdateInputConfiguration attempt per controller/input/
manager identity and never clears cinematic locks or resets a populated list.
This compiled candidate has not been executed; see
[native input initialization](NATIVE-INPUT-BOOTSTRAP.md).

Retry cleanup readiness remains required throughout the ordinary admission
window and immediately before console dispatch, rather than only when the
retry is initially queued. An observed blocked scene/travel or invalid native
context clears the accumulated player-readiness samples. This is sampled
startup admission at the existing250ms cadence, not continuous scene proof.

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
does not invent a teardown method. Same-world failed-listener retry is unsupported.

A separate explicit `ME3SquadCoopRequestRelisten` candidate covers a different
new mission after a previously verified runtime-owned listener. Tick requires
positive registry retirement of both that old root world and old driver,
standalone current-world story readiness, no pending native travel, no active
companion/fault/cleanup and no remaining capacity lease. It waits at most30
seconds for those conditions. The pending request then binds to the exact new
root identity and repeats the ordinary checked Listen pipeline. Changed context
or disposal evidence is refused before the native call. No driver is destroyed,
fault cleared or mission chosen by the mod. Status15/16/17 reports new-mission
listener waiting/refusal/expiry. The separate panel exposes the explicit action.
This path is compiled only; native mission travel/re-listening remains unverified.

`ME3SquadCoopCancelPendingConnection` now queues cancellation of mod-owned
Join/Listen/rejoin/new-mission-listen intents which have not reached native
dispatch. Tick processes cancellation before startup, resets their readiness
window/timers, and retains consumed-attempt history for rearmed retries. It
does not cancel native pending travel, disconnect an active peer, clear a
session fault or destroy a driver. Status18 means a pending mod request was
cancelled. With nothing pending, the existing native startup status is preserved.
The local panel provides the same explicit action. This is compiled only;
request races and actual cancellation remain untested.
**Protected actual-entry host startup has been observed; ordinary guest connection
still requires verification.** Offline fixtures do not establish playable co-op
or Radmin reachability.

`ME3SquadCoopNetworkStartStatus` reports idle (0), waiting (1), command dispatched
(2), already networked (3), ABI refused (4), malformed request (5), reserved (6),
native bind argument refused (7), native host setup verified (8), capacity refused
(9), native Listen failed (10), or post-start verification failed (11).
None means a guest connection is verified. Unknown/truncated
startup requests also remain inert, with runtime error 4.

Guest join now requires an observed ordinary player/story context before consuming
the one-shot request: reciprocal current-world Shepard ownership, the actual mode
manager, the squad's story-player link, and unchanged native object identities.
The admission policy requires five samples spanning at least one second, no
sample gap over 375 ms, and at least half a second of advancing native world time.
A scene, missing observation, reused object or regressing clock restarts the
window. These thresholds are a conservative mod policy, not an inferred native
minimum or proof of the previous rejection's cause. Context is reread before
dispatch; no readiness or pause flag is changed.

The immediate ConsoleCommand return is copied before its existing engine free.
A separate read-only observer samples the current viewport's two stored progress
strings after native Tick, at most every 250 ms for 60 seconds. Exact reflected
property types, offsets, dimensions and sizes, live object identities and stable
FString headers are required. Missing contracts produce an unavailable result.
Text is bounded and JSON-escaped into private runtime logs; it is not published.
Stored progress text is not proof that a native error event occurred. No private
PendingLevel fields, additional engine hooks or guessed error offsets are used.

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

`WorldInfo.bPlayersOnly`, its pending transition, an unreadable/non-null native
pause owner, native cinematic mode and active conversation state prevent host
control. The PlayersOnly pause also disables
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

A subsequent protected ordinary pair repeated successful host startup but failed
to establish a sustained guest connection. The one-shot guest join briefly set
native pending travel; all 213 valid guest world samples remained standalone.
The guest never acquired Ashley. The join was issued during a very early initial
world, making player/story readiness a hypothesis to test. The exact native
rejection was not captured, so neither readiness nor transport is established as
the cause. This pair provides no collision-restoration or combat proof. The run
ended with unchanged original profiles, zero native exceptions, no remaining game
processes and the temporary authored activation files removed.

Any research network bootstrap must be explicitly separate and preserved in
its evidence; it is not an ordinary-runtime host/join implementation.

Still required: protected ordinary guest-join and
actual-entry control/camera/disconnect runs, reliable equipment and authoritative
enemy damage, actual full conversations, mission transitions, two physical PCs
over Radmin, authored installable DLC, and Mod Manager install/disable/rollback.
Do not label the offline integration as a release.


## Ordinary readiness follow-up and rejected temporary DLC

A later protected ordinary pair exercised the stable-player join guard: five
samples over1000ms preceded its one-shot join at world time68.473seconds.
Host native Listen again verified, and guest ConsoleCommand dispatched.
No simultaneous native connection scope was observed. The guest transitioned
to EntryMenu; the bounded viewport observation retained only Connecting.
Decoded guest video shows an explicit corrupt-downloadable-content warning
naming the temporary authored DLC_MOD_ME3SquadCoop directory. Thus this is not
an isolated readiness or transport experiment, and the visual warning must not
be hidden behind the empty console result. The exact internal rejection reason
and its causal relationship to travel remain unproven.

The private runner installed a Mount-only activation directory. That directory
is a presence gate for the ASI, not demonstrated engine-accepted content. Resolve
its packaging against the existing full authored DLC tooling before repeating
ordinary network tests. Do not bypass engine package checks or treat presence
as successful mounting. Cleanup removed the temporary ASI and DLC, restored the
probe gate, retained original locks through shutdown, left original profiles
unchanged, recorded no native exception and left no game process.


## Dispatched join outcome

The ordinary entry now distinguishes sending a join request from observing its
native client context. After its own dispatched request, it observes the same
validated native scope used by runtime session admission. A valid scope changes
the startup status to Already networked; companion ownership remains a separate
panel field. If no valid scope is observed for60seconds, startup status19 reads
Join not confirmed and directs the player to inspect the game for a DLC or
connection message. This is an observation deadline, not a native rejection,
connection cancellation or automatic retry. A later valid scope can still
replace the unconfirmed status. It never reads UI text as an authority signal.

Cancellation, disable, host operation and explicit recovery statuses are
preserved. Publishing the observed result uses compare/exchange so it cannot
overwrite a concurrently published command status. Retry still uses the existing
native-world/loading/cleanup and stable-player gates; this change does not
acknowledge game dialogs, modify DLC, reset session faults or select a mission.
The panel gives current native network/control state precedence over a previous
unconfirmed join warning.

All three native targets built. The actual ordinary DLL harness passed,
including the outcome policy's60-second boundary, DWORD wraparound, late scope,
cancel/retry/host/disable cases. The harness was brought up to date with the
ordinary runtime's existing readiness-channel and MinHook link dependencies.
Three panel guidance tests passed. No game ran for this change; actual native
connection and full campaign behavior remain unverified.
