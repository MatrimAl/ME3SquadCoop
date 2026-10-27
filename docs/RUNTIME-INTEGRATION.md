# Runtime integration status

The first retained-template pair from `90786ee` opened the host listener but
never joined the guest: its retention call returned without a verified owned
group. A same-process read found no matching group record. The candidate now
invokes the pinned stock native load body with the explicit request array;
four targets built and the ordinary no-game harness passed. This revised call
has not run in the game and is not a visibility fix claim. The one game run
cleaned up with original profiles and TOC restored and no exceptions. See
[player-template retention](PLAYER-ARCHETYPE-RETENTION.md).

Ordinary guest startup now retains the loaded stock player archetypes in an
owned native loader group through join, and releases its group on explicit
lifecycle boundaries. Join admission checks the actual retained identities,
not merely successful package dispatch. Four native targets built and the
ordinary no-game harness passed. Travel survival and visibility are unverified;
no game ran. See [player-template retention](PLAYER-ARCHETYPE-RETENTION.md).

The remote appearance adapter now retains an admitted loading request across
transient completion-ABI/readiness failures and across an `ApplyAppearance`
return that still has no live mesh asset. It captures the original group and
the IDs normalized by the stock request before completion discovery; retries
require those exact IDs, pawn/world identities and group again. Later changed
appearance metadata cannot reuse that request. A live mesh asset, rather than
call dispatch alone, ends its pending state. The initial default-male admission
restriction remains; this does not create an absent replicated player actor or
establish rendered visibility. All four native outputs rebuilt and ordinary
runtime/probe harnesses passed without a game launch. The first sandbox build
failed with Zig `Unexpected`; normal-user compilation succeeded. The probe's
low-integrity child first exited 2 in the sandbox, then passed in the normal
user environment. These checks establish code/fixture behavior, not gameplay.

For LE3 2.0.0.48602, the SDK identifies the unequal final sampled appearance
integer as the transient `OverrideHelmetID` (0x1514). Selected package script
shows `FindOverrideHelmet` derives a breather-capable choice; its code alone
does not explain the observed host value or prove a rendered mismatch. The
normal appearance-completion script performs a loader cleanup after applying
the appearance; the current guarded direct `ApplyAppearance` path does not
execute that whole chain. Runtime consequences and safe cleanup conditions
remain unverified. Selected source-bound exports and receipts remain in the
private LE3 reverse-engineering workspace.

An ordinary same-PC pair from `c16b2d2` connected with an owned Shepard
channel and matching host/guest archetype and class paths. In six read-only
samples, the guest replica's skeletal-mesh reference was null in the first
three and live in the last three; the guarded `UpdateAppearance` and
`ApplyAppearance` calls returned a live asset between them. The host's last
appearance ID was 2 while the guest's was -1, so full appearance equivalence
is not established. No delayed-group retry was observed, and these data do
not prove rendered visibility or campaign gameplay. The first visibility
observer encountered a partially written owner-identity file; a restarted
observer captured the six samples. Cleanup restored the sandbox TOC, left no
game processes, and reported no native exceptions or cleanup errors.

The guarded guest appearance path now retains its original asset-group name
and rechecks it if loading was incomplete; it does not repeat
`UpdateAppearance` for the same player and world. Before `ApplyAppearance`,
it requires the same group, a nonempty set of live asset references, and the
existing player, world, mesh and channel admission checks. Synthetic fixtures
exercise group identity and reject empty or invalid references. The delayed
loading path has not yet been validated in a live game; this does not address
the separate case where the guest never receives the Shepard actor.

In one ordinary pair from `f9252b6`, both peers connected and twelve paired
snapshots showed matching, stable host/guest Shepard archetype paths and class
paths. The guest's skeletal-mesh asset was null in the first six snapshots;
the normal guarded appearance calls then reported a live asset, and six later
snapshots observed a live `SkeletalMesh` on the same guest player. This is
bounded same-PC default-male evidence, not attributed rendered visibility,
customization or a repeatability guarantee: other recorded runs still failed
to create the guest Shepard actor. Cleanup restored the sandbox TOC and left
no games, original-profile changes, native exceptions or cleanup errors.

Additional exact-build native exports clarify the asset-loader candidate. The
request copier makes a separate path string and copies the remaining fields,
including its object reference. The group-ready method returns true when a
matching group exists and every reference in its matching records is non-null;
a matching record with zero assets can also satisfy it. Therefore a ready bit
alone must not admit gameplay: the candidate must prove the particular asset's
live identity, expected class/path and retained ownership across travel. These
are static observations; no loader-based retention has been implemented or
tested. Both exports completed using the retained native project, whose files
remained byte-identical.

Follow-up read-only evidence from `f80fc88` shows the existing guest engine and
SFXAsyncAssetLoader with matching object identities in standalone and connected
samples; the four observed loader method entry RVAs and code bytes also matched.
In this same pair Shepard's initial archetype reference returned null and six
later guest channel snapshots lacked the actor. Another pair tracked the
standalone player archetype and package until the native join request, then
found their original object identities no longer live before a new archetype
resolved. This indicates pre-travel object identity cannot be assumed to survive;
it does not identify the unload mechanism or prove the failure branch.

The shipped async asset loader is a possible retention path: its reflected
request contains an asset path, class, alternate cooked package name and object
reference, and selected native functions copy requests and expose or clear group
contents. A stock script uses that group API and later clears its group. Whether
an admitted group retains the correct dynamic player archetype through travel,
how it is cleaned up, and whether its class/path agree with the host remain
unverified. No new loader call or retention behavior is implemented here.

A same-PC run from `284b314` left all four package/appearance preparation
options unset. The host's stock preload dispatched, changed the observed morph
GUID to the disk/cache GUID and opened the listener. The guest's stock
`BioChar_Player` preload also dispatched and the peers connected, but its
initial Shepard reference577545 returned null. All six later guest channel and
cache snapshots lacked the actor/archetype. This contradicts reliable recovery
from package dispatch alone; do not claim the missing-Shepard issue is fixed.
The guest focus observer recorded no focus-owned pause in 71 startup samples,
so this run did not test recovery from the preceding startup stall. Terminal
audit preserved original profiles, restored the sandbox TOC byte-identical and
left no games, native exceptions, forced cleanup or cleanup errors.

Host morph-package preparation now defaults on for an ordinary configured host,
using the same stock `SFXGame.LoadPackage` call and pinned native entry. A
pre-existing resident package is no longer required before dispatch; missing or
unreadable before/after GUIDs are marked unreadable rather than treated as zero
identity evidence. The engine still owns loading and package validation.
Host and guest attempts now bind to the admitted WorldInfo identity and refuse
native re-entry; a newly admitted world can request its own preparation. The
explicit diagnostic options can disable either preparation with0. Three native
targets built and ordinary/protected no-game harnesses passed, including option,
world identity, failure and re-entry cases. One live default host preparation
is recorded above; absent-package behavior and actual mission transitions are
not yet verified.

The existing focus getter policy is now admitted during a pending, unconsumed
ordinary guest join in its standalone world. It remains subject to the exact
Tick caller, native ABI, current local world/player, mode0 and pause-ownership
checks. Cancelled/consumed requests, host roles and disabled control do not open
this startup scope. Native return adaptation still lets the stock Tick manage
its own focus pause; the mod does not clear PlayersOnly or pause-owner flags.
Three native targets built and ordinary/protected no-game harnesses passed.
No game ran for this extension; it is not a verified fix for the startup stalls.

Ordinary guest package/appearance preparation is now enabled by default inside
the existing configured startup/session gates. The legacy diagnostic options
can explicitly disable each path with0; protected appearance entry still
requires1. Three native targets built, and ordinary/protected no-game harnesses
passed actual unset/0/1/invalid environment-option cases. Two runs without the
three guest preparation options did not dispatch join or package preparation:
the guest remained in a standalone PlayersOnly state and sampled video was
black. The native focus-owner flag was not captured, so focus loss is a lead,
not the established pause cause. Both runs cleaned up without native exceptions
or profile/TOC changes. A later live default guest preparation dispatched but
did not reliably resolve the archetype, as described above; earlier positive
runs set the options explicitly. The earlier host morph-package
path also required opt-in; its new default policy is described above.

The latest appearance bookkeeping binds its attempt to the admitted player and
WorldInfo identities rather than the whole process. Matching identities retain
their one-attempt limit through scenes or transient channel loss. A different
player/world must pass the existing missing-mesh, role, metadata, channel and
native ABI gates again; native-call re-entry is independently blocked. Full
object/class indices, names and paths distinguish reused pointers. Three native
targets built; ordinary and protected no-game harnesses passed, including
same-context refusal and changed player/world generation/path cases. No new
game ran for this change. Actual reconnect/travel lifetimes remain unverified;
the live appearance evidence below predates this bookkeeping change.

Ordinary guest startup now has a bounded positive path for host Shepard's
initial actor and default male appearance. Two candidate pairs loaded
`BioChar_Player` through the shipped `SFXGame.LoadPackage` before native join
and then resolved the same initial archetype reference that had intermittently
returned null. The second also reused the historical appearance initializer:
stock `UpdateAppearance`, then `ApplyAppearance` after the existing asset group
reported loaded. The native call retained the pawn/world and reported a live
asset. Independent paired channel snapshots saw the same guest Shepard first
with a null mesh asset, then five times with one live `SkeletalMesh` asset.
A decoded video frame contains a second nearby helmeted figure, but its visual
identity has not been independently attributed. This is model initialization
evidence, not full rendered appearance, customization or campaign validation.

The ordinary guest implementation is automatic and remains scoped:

- Guest archetype preparation calls the stock package API once per admitted world
  after ordinary guest readiness and before travel. It reuses the host loader's
  reflected parameter contract, additionally pins native entry RVA0xb6cfa0,
  and refreshes world/player readiness after the call. Setting
  `ME3_SQUAD_COOP_GUEST_ARCHETYPE_PRELOAD=0` disables it for diagnosis.
- The old appearance path runs in ordinary guest Tick unless either existing
  `ME3_SQUAD_COOP_REMOTE_APPEARANCE_QA` or
  `ME3_SQUAD_COOP_REMOTE_APPEARANCE_APPLY_QA` option disables its respective
  stage with0. It runs last with fresh connection/local context.
  Default male IDs, absent mesh, simulated role, a unique current channel and
  loaded assets remain required. Protected entry retains its marker/action gates.
- Neither path writes package maps, cached objects, roles or appearance IDs.
  No synthetic input or packet replay is introduced. Normal guest movement and
  camera remain owner-reported working; this work does not change their code.

Three native targets built. Ordinary and protected no-game harnesses passed,
including the shared appearance admission restrictions. Both package-preload
pairs and the preceding comparison pair ended at their owned deadlines with
zero recorded native exceptions, original profiles unchanged, sandbox base TOC
restored byte-identical, temporary authored files removed and no games or cleanup
errors remaining. The appearance path now attempts once in the current admitted
player/world context; general customization, actual new player lifetimes,
mission travel and repeatable combat remain
unfinished. These observations do not establish a playable campaign.

The missing-player investigation distinguished two stages. In unchanged-code
pairs from `8f2d10e`, static reference tag0/index577545 sometimes resolved
`BioChar_Player.Archetypes.Soldier.MaleShepard_Soldier` and sometimes returned
null, after which native receive changed channel flags1 to9 with Actor/Class
still absent. Each channel was independently mapped to host Shepard; numbers
vary by run, and channel9 in a positive run was a Matinee actor. A mesh component
alone was never treated as a loaded mesh asset.

A pair from `1847930` selected static map entry194/relative index1144 on both
peers, with matching GUID and object-count1352. Six post-receive snapshots found
the host's archetype cache entry populated and the guest's null. After stock
guest loading, the pair from `3352439` retained the archetype and formed player,
but all six mesh-asset samples were null. The pair from `188fa55` then exercised
the existing appearance path with the null-to-live asset result above.
Post-receive cache snapshots do not identify the exact initial resolver failure
branch. The first private sampler's entry+8 label was corrected: constructor
RVA0xc5470 shows entry+0 is an FName and entry+8 is the input object pointer;
the raw legacy evidence is preserved.

Focused exports reused the retained exact-build Ghidra database without changing
it or rerunning whole-binary analysis. Archive forwarder RVA0x464910 confirms
archive+0xb0 connection -> connection+0xf0 map. The stock package API's native
wrapper/body are RVAs0xb6cfa0/0xb965b0; its finalizer RVA0x12cfa0 performs loader
cleanup, not a demonstrated package root pin. Package availability after join
is supported by live cache observations, not inferred from dispatch alone.

The default-off `ME3_SQUAD_COOP_INITIAL_OBJECT_TRACE=1` observer forwards the
original first-object decoder and native scalar calls exactly once, recording
their produced tag/index/output only in the matching initial-bunch, thread,
archive and caller scope. It preserves LastError and never re-reads packet bits.
Its forwarding/filter fixtures passed; earlier `cb5960b` evidence captured null
before scalar integration. Historical `73adb0a`/`464dde3` and `f9b2aff` supplied
the appearance initializer, while `d14faf7` recorded the earlier actor-formation
failure. The current work reuses those paths rather than replacing working input.

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
