# Progress

## 2026-10-26: add local actor ownership and deferred revive activation

The service/factory candidate provides a normal Actor.Spawn entry and retains
Setup as a reflected reference for one pawn/token lifetime, rather than
depending solely on a future native raw UObject pointer. It has no autonomous
Tick/network/input behavior. The service prepares with admission closed and
exposes separate activation/status/retirement for a future peer-aware bridge.

Seven classes compiled without errors/warnings and unchanged tracked inputs.
This is not live spawning, GC/rooting or session evidence. Loaded class/function/
ABI resolution, native invocation, peer agreement and pre-handoff retirement
remain unfinished. No game or tests ran. See [service contract](REVIVE-SERVICE.md).

## 2026-10-26: resolve native temporary animation removal path

Registration and thunk instruction evidence selected AddAnimSet0xa87140,
RmvAnimSet0xa93070 and RegisterCustomAnimset0xa92020. A verified direct removal
call and chained Windows unwind records selected parent0xa948e0, avoiding
fragment/nearby-function substitution. All four exact entries decompiled with
nonempty bodies and source-matching terminal receipts.

Add/remove mutate the mesh list and set bAnimSetUpdated; custom registration
uses separate map storage. The parent's temporary-record loop retains nonzero
references, decrements TimeLeft at zero and removes/calls RmvAnimSet on expiry.
PlayersOnly skips that selected block. This supports the existing nonforced
unregister/wait policy and rules out treating elapsed wall time as cleanup
permission. Nested helpers, full mesh refresh/tick scheduling and live removal
remain unverified. The first JDK-settings permission failure yielded no output;
corrected runs completed. No game, tests or runtime/script changes occurred.
See [animation cleanup evidence](REVIVE-ANIMATION.md).

## 2026-10-26: close new revive admission during pending retirement

Runtime-lifecycle review exposed a missing link: clearing setup readiness did
not itself prevent the installed authored action accepting another attempt
while its binding awaited retirement. The authored action now starts with a
closed admission gate. Setup opens it only after preparation checks; binding
release closes it before checking idle/pending cleanup, leaving a running
action to finish through its existing path. Readiness includes that gate.

Gate writes require the exact current owned binding; it is not a story/session
ACL, peer handshake or gate for the separate existing basegame class. Runtime
pre-handoff invocation and cross-peer ordering remain unfinished. Five classes
compiled without errors/warnings and unchanged tracked inputs. No game or
tests ran. See [coordinated admission](REVIVE-SETUP.md).

## 2026-10-26: give nonauthority revive presentation a local end deadline

The prior native-body assumption for BioPawn.EndCustomAction was corrected
against SDK flags and a selected package read. Its script calls DoCustomAction(0),
which reaches the started action's Stop before replacing the current IDs.
Four completion payloads matched lengths with six aligned branch targets;
GetCurrentCustomAction is a native parameter stub, not body evidence.

The authored action previously polled duration only on authority while its
animation-end callback was empty. The reference revive also runs an end timer
on presentation peers. The candidate now supplies a local native-config/world-
clock presentation deadline, ending through the local script without native
Revive, success or a server-interrupt request. Authority remains the only
revival dispatcher. Five classes compiled without errors/warnings and with
unchanged tracked inputs. Peer timing, actual cleanup and late/interrupted
delivery remain unverified; no game or tests ran. See [action lifetime](REVIVE-ACTION.md).

## 2026-10-26: make revive owner replication policy explicit

Selected native-package scripts show base action replication sets owner
notification from !bClientPredictCustomAction and requests a native net update.
The authored action now explicitly disables prediction and forced local
simulation instead of adding a persistent pawn owner-notification flag write.
The native receive path verifies/instances action IDs and dispatches the
existing target/start/interrupt flow; no custom RPC/admission bypass was added.

Eight replication exports and four base lifecycle exports decoded completely
with42/77 aligned explicit branch targets. Normal EndCustomAction propagation
and client lifetime remain unresolved; a missing exact registration-name hit
was not substituted with a nearby native body. Five authored classes compiled
without errors/warnings and with unchanged tracked inputs. No game or tests
ran; actual delivery, completion and cleanup remain activation requirements.
See [replication boundary](REVIVE-ACTION.md).

## 2026-10-26: prepare authored script startup DLC candidate

A new offline tool checks the successful compile receipt/current inputs and
copies the authored five-class package into a separate private DLC candidate.
It compiles minimal startup/seekfree config, writes the reviewed Mount format
and generates the root TOC through the existing LEC API. Readback preserved
startup operation types/values and matched the PCC/Mount/config TOC sizes.
All tracked compile inputs and tool assembly retained their hashes.

Installed LEMP/base config reads supplied bounded schema evidence; no game
config/assets were copied. Engine startup naming/localization/acceptance,
shared dependencies and runtime setup invocation remain unfinished. The
candidate changes no release installer contract and is not a demonstrated fix
for the earlier corrupt-DLC dialog. No game, install/deploy or tests ran. See
[script DLC candidate](SCRIPT-DLC-CANDIDATE.md).

## 2026-10-26: coordinate revive preparation and ordered retirement

The authored setup object joins optional animation registration and the
authored action5 binding. Existing revive lookup avoids another animation
record. Failed preparation retains pending component state; retirement drops
the binding before requesting animation removal, preventing removal while
the owned action is still active/pending. Readiness checks current binding,
sequence resolution and a positive native registration record where owned.

The coordinator keeps busy/preparing guards across native calls and exposes
local failure/pending status for the eventual runtime owner. It is not a
session admission or asset-loading replacement. All five classes compiled
with zero errors/warnings and unchanged tracked inputs. Shared loading,
per-peer invocation/admission and pre-handoff retirement remain unfinished.
No game, deployment or tests ran. See [setup contract](REVIVE-SETUP.md).

## 2026-10-26: stage gradual manual feature and recording reviews

The owner requested gradual feature preparation with reviewable gameplay
recordings. A new offline preparation tool staged six private cases and
hash-checked current native/script artifacts without installation or launch.
Movement/cover, weapons, powers, Shift menu, revive and scene/travel return
have separate criteria and per-peer video/log/review destinations. Revive is
explicitly integration-pending rather than ready to exercise.

An existing workspace FFmpeg advertises HWND-scoped gfxcapture. A recorder
candidate verifies explicit sandbox executable/PID/HWND identity and writes
per-peer MKV segments, requested start clocks and exit metadata. It sends no
input, starts no game and awaits a STOP request/game exit/window loss. Closed
segments support later intermittent frame review; continuous model vision,
first-frame/log alignment and valid capture are not claimed. Source syntax
was parsed; no recorder, game or behavioral tests ran. See
[feature preparation and capture limits](FEATURE-REVIEW.md).

## 2026-10-26: compile scoped temporary revive animation registration

The new authored animation lease accepts an already loaded set, refuses an
existing revive lookup/temporary record/mesh occurrence and uses the game's
RegisterTemporaryAnim rather than rewriting Mesh.AnimSets. Readback requires
one reference/occurrence and the exact expected sequence. Retirement follows
action-binding release, requests one nonforced unregister and remains pending
until native removal is observed; changed/shared counts are not guessed away.

Eight selected script payloads matched lengths with16 aligned branch targets.
Native AddAnimSet/RmvAnimSet/registration stubs do not establish actual native
refresh or deferred removal. All four authored classes compiled without errors/
warnings and with unchanged tracked inputs. The lease remains uncalled; asset
loading, peer admission, skeleton/playback, concurrency and native cleanup are
unfinished. No game or tests ran. See [animation lifecycle](REVIVE-ANIMATION.md).

## 2026-10-26: review owner-run inventory worker output

The owner-run worker exported the missing semantic dependency0xba520 from
the retained Ghidra project. Its receipt, nonempty body and source identity
were checked independently. Five retained inventory batches cover79 unique
requested entries/78 nonempty bodies;0xda350 remains missing. The new index
lookup compares full keys after hashing and traversing the collision chain.

The worker's broader ownership/relevance, teardown causality, complete-graph
and hypothetical crash claims were not adopted as proven facts. A private
supervisor audit preserves output hashes and narrows the accepted findings;
the canonical research now distinguishes those claims from the new lookup
evidence. No runtime code, game or tests changed. Inventory root cause and
reliable equipment readiness remain unresolved. See
[inventory review limits](INVENTORY-REPLICATION-RESEARCH.md).

## 2026-10-26: compile scoped native revive instancing component

The authored binding component accepts an already loaded authored or existing
revive class, requires CB_MP_Revive lookup on the current mesh and only fills an
empty action instance slot5 with a compatible class baseline. It invokes native
VerifyCAHasBeenInstanced for object creation/controller initialization rather
than hand-writing cached context. Owned and explicit simulated presentation
contexts are distinct; neither substitutes for runtime co-op admission.

Retirement conditionally restores only its captured class/unused instance,
refusing active/timer-owning/pending-cleanup actions and changed identity.
The instancing script decoded with matching727/1055 lengths and16 aligned
branch targets. All three authored classes compiled together without errors/
warnings and with unchanged tracked inputs. The component remains uncalled;
runtime package/class loading, per-peer admission, animation provisioning and
pre-handoff retirement remain unfinished. No game or tests ran. See
[binding contract](REVIVE-BINDING.md).

## 2026-10-26: locate revive animation carriers and basegame class

Selected offline reads found CB_MP_Revive in the female and male human soldier
player packages, each linked through its gameplay mesh's HMM_DG_Deaths dynamic
set. The serialized sequence is5.3 seconds/159 frames; this does not replace
native fTimeToRevive. Ten distinct carriers supplied1450 inspected sequence
exports; the bounded negative scans are not exhaustive game coverage.

BioP_Char also supplies a local SFXCustomAction_Revive class/default object,
narrowing an existing-class integration alternative before a new shared class
package is assumed necessary. Neither serialized presence nor the two MP
mesh chains prove loaded class resolution, Ashley animation lookup or action5
availability. No assets, archetypes, timeline or override data were copied.
The authored action now explicitly requests replication, permits movement
input and sets0.4 blends; both classes compiled with zero errors/warnings and
unchanged tracked inputs. No game or tests ran. See
[animation and class evidence](REVIVE-ANIMATION.md).

## 2026-10-26: preserve native use callbacks during revive

The authored context now captures the existing target use module's callback,
range, tip, selection/deactivation flags and lens identity/hidden state. It
temporarily disables targetability via SetTargetable(false, false), preserving
the callback and selection save variable. An already disabled or absent module
is not enabled or created. The candidate does not borrow the game's shared
DisableSelection/RestoreSelection old-state fields.

Interrupted downed attempts conditionally restore targetability and the exact
lens hidden baseline; observed native recovery retains its disabled-use state.
Changed metadata/control/module or partial restoration stays pending and blocks
a fresh capture. Timer cleanup precedes usage cleanup and reservation release.
Three selection scripts decoded completely with three aligned branch targets;
both authored classes compiled with zero errors/warnings and unchanged tracked
inputs. No game or tests ran. Visibility, overlapping state changes, client
prompts, native stop/recovery cleanup and class activation remain unverified.
See [usage contract](REVIVE-CONTEXT.md).

## 2026-10-26: capture and scope native revive death-timer pauses

The authored context now captures the target's explicit PermaDeath timer
binding and pause/loop/rate/count/dilation baseline before inherited action
start. It pauses only an existing unpaused timer via native PauseTimer and
checks the expected readback. An absent timer is not added; an existing pause
is preserved without ownership. Duration requires the captured baseline.

Cleanup releases its timer pause before bBeingRevived, resuming only within
the same authority/action/world and an unchanged downed target/timer. Native
timer removal or an already restored unchanged pause requires no timer write.
Changed state and lost identity remain pending; identical replacement between
reads cannot be detected without a native generation. Target cleanup failure
blocks a fresh capture. Usage-module leases and live native recovery/cleanup
remain unfinished. Both classes compiled with zero errors/warnings and
unchanged tracked inputs; no game, fixtures or tests ran. See
[death-timer accounting](REVIVE-CONTEXT.md).

## 2026-10-26: compile authored revive action driver

The new SingleAnim action5 candidate binds the authored context to start,
per-update native-world duration/cancellation, authority-only native Revive,
separate recovery observation and native end/interrupt requests. It includes
a replicated-target path and refuses an unsuccessful animation query rather
than substituting direct health writes. Attempt serials and a stop guard avoid
continuing an older native callback stack into a later accepted attempt.
Failed scoped reservation releases remain visible and block a fresh capture.

Both authored classes compiled together with zero errors/warnings and unchanged
tracked inputs. The class remains uninstalled and unassigned. Native target
usage/death-timer leases, animation availability, pending cleanup, native
end/stop ordering and replicated gameplay remain unfinished. No game, fixture,
package reload or tests ran; no native-source rebuild was needed. See the
[action flow and integration boundary](REVIVE-ACTION.md).

## 2026-10-26: scope authority-side revive target reservations

The authored context now acquires bBeingRevived only for its own current
action/target on authority, refusing existing reservations. Native-world
duration and dispatch require that captured reservation. Conditional release
retains pending ownership metadata when action/context no longer matches;
Reset/Capture cannot silently discard it to accept another target. Current
WorldInfo identity also gates the captured context through the native getter.

The owning action still needs pre-stop release ordering and explicit pending
cleanup handling, native use/death-timer baselines, animation and replication.
A boolean flag cannot detect an unobserved external clear/reassert cycle; no
global lock or completed cleanup is claimed. The revised class compiled with
zero errors/warnings and unchanged tracked inputs. No game, fixture or tests
ran. See [reservation contract](REVIVE-CONTEXT.md).

## 2026-10-26: add native-world revive duration and cancellation latch

The authored context now starts progress once from the reviver's current
fTimeToRevive and native world clock. Dispatch requires its completed deadline,
current action/target context and authority. Invalid/backward clocks or a
failed progress check latch cancellation, preventing silent restart when the
scope becomes eligible again. No pawn callback timer or forced configured
duration was added. Ordinary-world checks now also refuse active/activating
conversations.

The owning action still needs to poll progress each update and manage animation,
movement/damage cancellation, target reservation and death-timer baseline/
cleanup. It is not integrated or assigned to either peer. The revised class
compiled without errors/warnings into a new private package; tracked inputs
remained unchanged. No game, fixture or behavioral tests ran. See
[revive progress contract](REVIVE-CONTEXT.md).

## 2026-10-26: compile the shared authored revive-context component

The first authored UnrealScript component captures and rechecks action,
party-pawn/controller/world identities, ordinary-mode/life/range eligibility
and current action5/synchronized target. Authority-only native Revive dispatch
is one-shot per captured context and distinct from subsequent observed native
recovery. It does not write health, action arrays or stale controller caches.

The package compiler now passes an explicit intended class name. The context
compiled without errors/warnings and one private package was saved with
unchanged tracked inputs. This is a context dependency, not the full action:
animation, duration/cancellation, reservation/timer ownership, replicated target
and per-peer slot integration remain unfinished. No game, fixture or tests
ran. See [authored revive context](REVIVE-CONTEXT.md).

## 2026-10-26: prepare repeatable private script-package builds

The existing local Legendary Explorer assembly exposes FileLib, CompileClass
and package-save APIs. A new PowerShell7 command prepares ordered authored
class compilation into a fresh repository-local package, with explicit game/
tool roots, diagnostics and input/output digest receipts. Existing/outside
paths and junction/symlink output ancestors are refused; no installer or game
launcher invokes it. Failed compile sets do not reach the final package save.

Only API reflection and PowerShell syntax parsing were performed. The new tool
has not compiled a class or saved/loaded a package; no game or tests ran. It
prepares a reusable route for the missing shared revive action rather than
claiming that the action, animation or per-peer assignment exists. See
[script build preparation](SCRIPT-PACKAGE-BUILD.md).

## 2026-10-26: identify missing serialized revive action prerequisite

Fresh property reads show null custom-action class5 in the inspected Ashley
and base pawn defaults. Existing CanDoCustomAction requires a nonnull class;
the normal interaction/StartRevive route alone therefore cannot establish
revive availability. Actual mission/archetype/runtime entries remain unmeasured.

Nine retained LEMP revive functions decoded completely and expose timed start,
authoritative dispatch, synchronized target, interruption and cleanup behavior.
Its callback success field can be set outside the guarded Revive dispatch;
that label must not be used as proof of actual resurrection. The reference
also has MP-specific score behavior requiring separate compatibility review.

No action array/class or health was written, no assets were integrated, and no
game or tests ran. Research and player-experience requirements now explicitly
retain this action-availability gap. See
[revive availability findings](GUEST-INTERACTION-RESEARCH.md).

## 2026-10-26: locate native interaction selection context

Native registration/thunk inspection identifies exact selection entries
0xa9bf90/0xaa55f0; both decompiled from the retained project. Selected paths
use the selection object's owning controller, its current pawn, input and
camera rather than a separate cached Shepard/AI binding. Native candidate
registry, gating, scoring and presentation remain authoritative.

This removes an unsupported need to manually rebind a target cache, while
leaving actual guest selection/update readiness unresolved. No native update
call, selection construction or target write was added. Only research/private
indexing changed; no game or tests ran. See
[selection context findings](GUEST-INTERACTION-RESEARCH.md).

## 2026-10-26: trace ordinary guest use into authoritative revival

Fourteen selected interaction scripts decoded completely with matching lengths
and55 aligned explicit destinations. GameModeDefault uses the controller's
current selection, TryUse preserves CanUse and the normal authority/client
split, and party EnableUsage binds the native Used delegate. The target's
Used requires authority/downed/nondead state and a living party-pawn user,
then starts the user's normal revive action5.

This selected script route does not require the user to be SFXPawn_Player;
there is no evidenced need for an alternate mod revive command or multiplayer
flag rewrite. Actual input/selection, action context, completion and HUD
availability remain unverified. Only research documentation/private readers
changed; no game or tests ran. See
[normal interaction research](GUEST-INTERACTION-RESEARCH.md).

## 2026-10-26: yield guest shoulder presentation during incapacitation

All28 selected owned life-state functions decoded completely, with matching
serialized/expanded lengths and88 aligned explicit destinations. Downed state
has native collision/marker/ragdoll handling; permanent controller death can
unpossess and clear the pawn. These are different lifecycle paths, not proof
that campaign guest revival already works.

The shoulder-camera candidate now refuses downed/dead or unreadable life state
independently of action/input/mode flags, including retained weapon actions.
It yields through existing conditional baseline restoration; normal recovery
must satisfy fresh camera acquisition. No native revival, health, possession
or fault policy changed. Three native targets compiled and fingerprints
matched; no game or tests ran. See
[life-state script findings](COMPANION-LIFE-STATE.md).

## 2026-10-26: tie inventory sender evidence to its source index

A further native association-helper export shows an existing-entry update
branch as well as new record allocation. This supports investigating channel
replacement/removal ordering; it does not prove an overlapping-channel fault.
The protected sender observer now records the exact original source index's
post-call table slot, channel/actor, flags and reciprocal identity with separate
read-validity fields and slot readback. This distinguishes it from the earlier
full-table hint without calling a serializer or native lookup again.

Normal runtime behavior is unchanged. The observer cannot establish earlier
hash state, packet bits or host/client event correlation. Three native targets
compiled and fingerprints matched; no game or tests ran. Inventory root cause
and repair remain unverified. See
[source-index observation](INVENTORY-REPLICATION-RESEARCH.md).

## 2026-10-26: expand inventory analysis through channel teardown

The requested background inventory export processed71 entries with70 nonempty
decompilations and one missing exact entry. Six follow-up lifecycle dependencies
also decompiled. Selected close/cleanup bodies remove the actor's hash key;
one cleanup path additionally invalidates matching references in other-channel
shadows before clearing its own Actor. Those shadow writes must not be confused
with clearing the pawn's live inventory property. The association path and
full-key removal were retained separately, with split unwind coverage explicit.

Exporting76 bodies is not complete semantic review or a root-cause diagnosis.
Hash replacement behavior and actual serialization/teardown ordering still
need evidence; no overlapping-channel race or engine bug is asserted. No
runtime repair was added. Research documentation/private indexing changed,
source game identity stayed unchanged, and no game or tests ran. See
[inventory lifecycle findings](INVENTORY-REPLICATION-RESEARCH.md).

## 2026-10-26: resolve native inventory lookup semantics offline

Five retained-project entries decompiled in about13 seconds, covering the
concrete reference codec, eligibility, actor-channel hash lookup, Pawn property
selection and ActorChannel sender. The hash leaf compares the complete actor
pointer and does not inspect channel state. Low-bit hash collisions alone
cannot explain a wrong actor match. Dynamic CanSerialize checks lookup
presence; sender open-state and receiver closing/class checks are separate.

These findings narrow the inventory investigation but do not establish why
the retained real samples selected the wrong class or static reference index.
The already prepared original sender-output measurement remains deferred.
No speculative cache/pointer repair or additional native call was added. Only
research documentation/private evidence indexing changed; no game or tests
ran. Input executable identity remained unchanged. See
[inventory codec coverage](INVENTORY-REPLICATION-RESEARCH.md).

## 2026-10-26: preserve name-only native quickslot replacements

Source review found that untouched icon baselines and retirement compared
only power/pawn pointers. Native setup or cleanup could therefore change the
icon's full name while leaving those addresses unchanged, and later mod work
could overwrite that replacement. This is a source-level gap, not an observed
runtime failure.

The lease now captures baseline full names and uses one exact expected-value
predicate for painting, native setup and non-cleared assignment destinations.
Retirement also checks the installed full name. Baseline capture requires the
power's current PowerName to match; a later changed baseline name cannot be
restored as the original assignment. Empty baselines use actual native None.
Native setters still own UI state and text; no direct name/profile writes were
added. Three native targets compiled and fingerprints matched; no game or
tests ran. Rendering and cleanup behavior remain unverified. See
[native refresh accounting](NATIVE-QUICKSLOT-REFRESH.md).

## 2026-10-26: classify native action-context candidates

The retained project completed a14-entry action-array candidate batch in
about11 seconds:12 decompiled and two exact function entries missing. Reviewed
bodies read active-action metadata, dispatch delegates or perform broad
cleanup; none establishes a narrow controller-cache refresh. Offset matches
on unrelated/unidentified receivers remain explicitly unassigned. Indirect
callees and alternate addressing are not covered by the scan.

The private dossier now includes this completed batch and reviewed evidence
identities. No speculative action reset, array recreation or cache-field write
was added. This narrows unsafe implementation leads; it does not diagnose a
live Ashley cache fault or add a gameplay feature. No game or tests ran, and
runtime source/binaries were unchanged. See
[action context research](ACTION-CONTROLLER-CONTEXT.md).

## 2026-10-26: match native first-name lookup to the selected power object

The exact native manager lookup was traced through registration, class
constructor and vtable. Its bounded leaf compares the complete PowerName and
returns the first full-array match, without applying the eligible-catalog
filters. Ghidra did not recognize that leaf as an exact function; its selected
instructions, ending at the actual return, provide the retained evidence.

A shared read-only check now requires native name lookup to select the same
power object as the mod catalog before quickslot setup, full-wheel selection
or queued native order dispatch. An earlier ineligible duplicate can no longer
silently redirect this route. The check validates the actual dispatch entry,
layout, array and prefix readback and refuses unmodeled implementations.
Three native targets compiled and fingerprints matched; no game or tests ran.
See the [lookup contract](POWER-LOOKUP-CONTRACT.md). Actual casting remains
unverified.

## 2026-10-26: coordinate guest slots with native PC-wheel refresh

The optional icon candidate now redirects the exact native quickslot pawn
resolver and per-icon setup callers to the registered guest and mod slot map.
The original native helper/setup still owns icon, permission/state, cooldown
and display processing. Unassigned entries clear through the verified setter.
The PC Tick's saved-definition mismatch rebuild is suppressed only in the
verified guest context; periodic native setup and display continue. Saved
controller mappings and actor tags are untouched.

Pre-call expected-value checks and post-call widget/catalog/map readback
coordinate known native refresh with the paint lease. Dragging preserves the
transitional icon values. Byte pins, resident MinHook forwarding, checked
virtual CAS and post-install active checks scope the integration. Three native
targets compiled and fingerprints matched; no game or tests ran. This is a
source-level refresh candidate, not a verified stable HUD or cast. See
[native quickslot refresh](NATIVE-QUICKSLOT-REFRESH.md).

## 2026-10-26: prepare a pinned native-detour dependency

MinHook v1.3.4's x64-required sources are now vendored from official commit
c3fcafdc10146beb5919319d0683e44e3c30d537, with upstream notices and archive
identity retained. Only the ordinary runtime links the sources; the probe and
launcher do not. Build fingerprints include the vendored tree and the license
notice copied beside the ASIs. Three native targets compiled and fingerprints
matched; no game or tests ran.

This prepares interception of the observed direct quickslot pawn-resolution
call, which a setup-vtable hook alone cannot cover. No MinHook initialization,
detour creation or enabling call has been added yet. The eventual adapter
must preserve stock forwarding outside the owned guest wheel and retain
native icon/cooldown/permission handling without writing saved mappings.
Quickslot refresh coordination remains incomplete.

## 2026-10-26: align native wheel squad with registered guest context

The optional wheel context now has an independent transient-reference lease.
After validating the admitted guest/controller/world/channel/squad epoch and
reflected native wheel layout, it conditionally replaces the UI squad
reference with the registered player squad and clears stale henchman UI
caches. The engine still owns player-cache selection and native group
population; no direct population/update/visibility call was added. Pawn.Squad,
story membership, WorldInfo registration and profile mappings stay untouched.

The full-wheel caster now validates against this lease independently of the
quickslot-paint lease. Conditional retirement preserves native replacements
and leaves native player-cache transitions to the game. Acquisition stays
mode0-only; existing menu-context retention still gates mode3. Native quickslot
refresh/display coordination remains unresolved. Three native targets compiled
and fingerprints matched; no game or tests ran. Natural full-list population,
casting and restoration are unverified. See
[registered squad candidate](POWER-WHEEL-POPULATION.md).

## 2026-10-26: locate native UI context and refresh conflicts

A26-entry wheel virtual batch yielded25 decompiled entries in about12 seconds
from the saved project; its direct base-update dependency also decompiled.
Six selected visibility/lifecycle scripts decoded completely. The native base
update lazily fills the wheel's squad reference from the possessed BioPawn's
Pawn.Squad, then reuses it. This can diverge from the synthetic input adapter's
separately registered local player squad. The mismatch is now grounded in both
paths; it has not been reproduced in a new live run.

The native PC update also refreshes quickslots from saved controller mappings,
including a direct setup call when displayed names differ. This can overwrite
the optional icon lease, whose next value check retires it. Full integration
must coordinate native refresh and transient UI references; one-time painting
or forcing visibility is insufficient. See
[native update findings](POWER-WHEEL-POPULATION.md).

Flash's read-only review of the public lemp.cpp/lemp.h/repl_probe.cpp completed.
Its reported diagnostic offsets and pointer reads were checked in source.
Those selected files offer observations, not an implemented movement/inventory
repair to copy. This does not claim absence across all LEMP artifacts. The
first bridge report failed during local character encoding; rerunning with
UTF-8 output produced the result successfully. No private mod/game data was
sent to Flash. No runtime source changed; no game or tests ran.

## 2026-10-26: retained Ghidra reuse verified

The ten-entry wheel batch completed with exit0 and saved the project. Nine
functions decompiled; the known short player-cache leaf was not recognized
at an exact analyzed entry and remains explicitly missing from this export.
Reprocessing the same manifest with readOnly/noanalysis returned identical
function evidence in about12 seconds, compared with about14 minutes for the
first import/export. Three additional helpers and the visibility method then
decompiled from that same project. No replacement import was needed.

The squad branch's possessed-pawn cast is BioPawn, verified through its class
factory/name, so it does not introduce a Shepard-only class requirement.
Full cache selection has separate active-mode/session branches. The visibility
method also dispatches callbacks and clears/updates full-list icons on hide;
it must not be treated as a neutral population shortcut. See the
[population results](POWER-WHEEL-POPULATION.md). No runtime source changed,
and no game or tests ran. Full guest-wheel initialization remains incomplete.

## 2026-10-26: retain guest menu context without optional icon readiness

The guest frame's native PowerWheel context-retention predicate is now
independent of the optional icon lease and catalog. Previously an early Shift
opening or disabling icons could retire healthy existing squad registration.
The new predicate still requires current reciprocal Ashley ownership, client
world, existing registered epoch, channel/world/squad identities, matching
squad caches, exact mode-manager/mode ownership and unpaused scene state.
Downed/dead/invalid-busy and noncombat pawn states refuse retention.

The separate UI predicate still requires the complete valid icon lease and
catalog. No registration/icon acquisition or mod casting is enabled inside
the menu. Three native targets compiled and fingerprints matched; no game or
tests ran. Actual menu continuity and full power-list initialization remain
unverified. The retained Ghidra batch continues in its original session.

## 2026-10-26: gate saved-project reuse on a terminal export

The retained-analysis runner now requires a matching successful prior export
receipt before allowing saved-project noanalysis mode. It checks exit0,
completed report, retained-project status, entry count and source identity.
New receipts also identify their project root. Python syntax parsed and diff
whitespace checks passed; no game or tests ran. The original batch remains in
its same live command session, so no replacement import was launched.

Review of the complete native menu-deactivation script confirms delivery is
through its Outer controller's ApplyTacticalOrders under the BioPawn combat
flag. Population integration must retain that controller context through
deactivation and avoid a second close-time dispatch. See
[close-time delivery](POWER-WHEEL-POPULATION.md). This is static evidence,
not a new successful cast or completed full-list initialization.

## 2026-10-26: start a retained feature-level Ghidra batch

A new offline exporter compiles against the installed Ghidra12.1.4/JDK21
APIs and processes a private exact-entry manifest. Ten related wheel methods
are queued together, covering power population, cached-pawn refresh, icon
mapping, selection, quickslots and GUI lookup. Unlike the earlier ephemeral
REA CLI import, this command keeps its project. Later reads select
process/noanalysis/readOnly. Per-function outputs preserve earlier results
if a later entry fails; incomplete/missing entries remain explicit.

The exporter compiled and Python syntax parsed. The owned command started,
its log confirmed PE import and its same session remained live at observation.
No decompilation output, saved-project reuse or gameplay success is claimed
yet. No runtime source changed; no game or tests ran. See the
[exporter instructions](../tools/ghidra/README.md).

## 2026-10-26: permit verified empty capacity in an owned power group

Native population clears unused wheel capacity; the caster guard previously
rejected any group containing those empty icons. It now accepts only
unselected, fully cleared native icons: null power/pawn, actual None name and
current/desired state5, with reflected layout and final identity/value
readback. The selected power and every populated icon still require current
owned catalog membership. Duplicate/stale/foreign populated entries refuse.
Three native targets compiled and fingerprints matched; no game or tests ran.

Six population-dependency script exports decoded completely. The initial
event is GetCurrentHenchmanOrder, and controller+0xc50 is declared as the
selection component. The native icon callback also depends on cached pawn
fields for mapping display. These findings narrow the remaining full-list
bootstrap and retirement work; they do not establish completed initialization.
See [population dependencies](POWER-WHEEL-POPULATION.md).

## 2026-10-26: discover the existing guest wheel before a hotkey

The optional icon lease now reads the native GUI manager's two bounded movie
lists to locate a unique existing PC wheel belonging to the current registered
guest/controller/world. This removes the hotkey-only discovery prerequisite
from candidate code. It calls no native getter and loads no movie. Instruction
pins, reflected owner/widget layouts and registry/ownership readback gate the
observation; ambiguous or incomplete state refuses it. Registry observations
must be current on the acquisition Tick. See
[wheel discovery](POWER-WHEEL-DISCOVERY.md).

Three native targets compiled and fingerprints matched; no game or tests ran.
Discovery is mode0-only, so an immediate first-menu opening before lease
readiness remains a limitation. Full Ashley power-group initialization and
actual gameplay remain incomplete and unverified.

## 2026-10-26: native power population decompilation completed

The background REA/Ghidra operation returned successful pseudocode for the
supported image's pawn/group population method. It confirms that the supplied
pawn supplies the powers, but existing group capacity, controller dependencies,
native icon state and callbacks must also be ready. This resolves a major
initialization question without claiming a callable guest integration.
See [population findings](POWER-WHEEL-POPULATION.md). The private dossier now
validates the successful output identity rather than pointing only to the
earlier timeout. The operation took about13m44s and used an ephemeral import;
persistent project reuse remains unestablished. No runtime source changed,
and no game or tests ran.

## 2026-10-26: retain active guest ownership through the host Shift menu

The ordinary host now retains an already-active guest lease in an exact
unpaused native power-wheel mode3. This complements the command-pause policy;
previously the generic mode0-only scene gate would still release Ashley even
if simulation kept running. Retention verifies current host/story/world,
listen-server scope, disabled command-pause policy and the actual mode-manager
instance. Travel, disconnect, fault, conversation and reciprocal ownership
checks remain in force. No new guest admission or equipment startup is enabled
inside the menu. The protected probe's default policy remains unchanged.

Three native targets compiled and fingerprints matched; no game or tests ran.
Actual host-menu control continuity and gameplay remain unverified.

The separate CLI decompilation completed with provider_timeout at its default
330-second startup deadline. Its result is retained as failure evidence, not
pseudocode. Both prior Java jobs ended before the target was closed. One new
background CLI job was started with a two-hour startup allowance, without
changing installed configuration. Offline feature implementation continued
while that analysis ran.

## 2026-10-26: aggregate power-wheel evidence while Ghidra analyzes

A private offline feature-dossier builder now joins8 named native roles,
34 script-export observations and117 retained call instructions, keeping
unresolved indirect calls and incomplete native boundaries explicit. This
reduces repeated per-address discovery without claiming semantic completeness.
The input executable hash was checked before and after aggregation.

REA reports a configured Ghidra12.1.4/JDK21 backend, and the supported image
opened successfully. The initial MCP batch observation timed out after120s;
the underlying analysis was not thereby terminated. A CLI fallback was also
started after restricted process visibility was misread. Subsequent process
inspection confirmed both Java analyses progressing. No further import was
started; neither had returned decompilation evidence at this observation.
See the [offline analysis workflow](OFFLINE-ANALYSIS-WORKFLOW.md) for preventing
duplicate imports and separating advertised capability from actual results.

No runtime source/binary changed, and no game, network gameplay or tests ran.

## 2026-10-26: owned full-wheel caster/group candidate

The optional icon lease now installs a checked forwarding hook at the native
PC wheel caster virtual. For the verified SelectCurrentPower caller, it reads
the entire selected index group and accepts only current unique Ashley power
icons with matching pawn/player-controller caches and no AI cache. It returns
the owned pawn with the existing native group, allowing the stock selection,
cancellation and tactical-order path to proceed with consistent context.
Other wheels/callers and inactive leases retain stock behavior; malformed,
mixed or stale leased groups refuse. Empty henchman groups remain valid.

This does not populate Ashley's full power list or rewrite cached story pawns.
That initialization remains a prerequisite: a Shepard list is refused. Three
native targets compiled and current fingerprints matched. No game, wheel
selection, cancellation, network session or tests ran. Native order delivery,
effects and full-menu rendering/control remain unverified.

## 2026-10-26: full-wheel caster/group selection path resolved

Offline selection inspection established that full-wheel SelectCurrentPower
calls native SquadOrderUsePower directly and bypasses the quickslot bridge.
Its selected caster comes from virtual+0x508, which maps selected icon indices
to cached Shepard/henchman groups rather than reading the icon's pPawn. Other
pending icons in that group also feed CancelLastOrderedPower before selection.
The exact event name, helper and five contiguous selection unwind fragments
were identified. Icon Activate itself is only a dirty/desired-state leaf.

This narrows the remaining integration: Ashley's full power list, group indices
and cached caster must agree together. Quickslot icons and mode3 context
retention alone cannot establish full-wheel casting. Five selected exports
decoded completely, eight explicit targets aligned and input hashes stayed
unchanged. No runtime source, binary, game or tests changed for this research.

## 2026-10-26: co-op command-pause policy and owned power-menu context

The owner requested that Shift power menus keep co-op simulation running.
Owned BioWorldInfo.PauseGame shows the native command-pause policy gate:
with a valid SFXGRI and bPauseForCommand false, it does not write PlayersOnly.
The runtime now clears only that verified policy bit in an admitted host or
guest network world, beginning from an ordinary unpaused frame. It retains
the baseline for conditional same-world restoration on disable/scope loss.
Existing pause state is never forcibly cleared, and an already-open paused
menu is left to its stock close path. Missing/unverified GRI refuses the policy.

The optional icon candidate also gained an exact mode3 UI retention predicate.
It preserves an existing registered matching guest context while the power
wheel, mode manager, network/catalog and squad caches remain valid and no
pause/conversation/cinematic environmental block is present. Other scenes
still clean up; native power dispatch remains mode0-only. UI consumers can
retain/reassign icons under that limited menu predicate.

Three native targets compiled and current fingerprints matched. No game,
Shift press, network session or tests ran. Actual pause prevention, menu
rendering/control and close-time native tactical orders remain unverified.

## 2026-10-26: split UI power membership and identify full wheel-mode gap

UI icon/drop consumers now read owned eligible power membership separately
from command dispatch readiness. The command path retains idle action/power
and IgnoreLookInput checks before and after membership sampling. An existing
ordinary UI lease can observe membership changes during transient combat
actions; this reader cannot publish command scope or grant a cast.

Offline inspection also established a limitation of the earlier drop candidate:
the actual power wheel is mode3 (owned ExitPowerWheel confirms DisableMode3),
whereas current context registration yields outside mode0. Five fully decoded
wheel lifecycle scripts include pause/unpause and close-time tactical orders.
Full-wheel support therefore needs coordinated mode/pause/context/close handling;
the existing ordinary-mode drop hook does not establish it. No scene policy
was loosened. Three native targets compiled and the build fingerprint matched.
No game, panel, network or tests ran.

## 2026-10-26: native guest quickslot drop candidate

The optional icon lease now includes a checked forwarding hook at the native
PC wheel assignment virtual. Its accepted NewSet exec call chain is grounded
in the exact image. The hook accounts for the wrapper's prior destination
clear, validates current guest source icons and resolves full power names.
Accepted drops update the same process-local map used by keyboard commands;
moving an existing name swaps the prior destination assignment back.

The guest path substitutes for saved Shepard definition writes and native
feedback callbacks. It invalidates queued/list selections and waits for idle
release cleanup before bounded native icon repaint. Rejected drops retire the
lease under existing identity/restoration rules. Other wheels and inactive
leases retain stock handling. No across-restart mapping persistence, removal
gesture or MoveByName integration is claimed.

Three native targets compiled and the source/build fingerprint matched. No
game, panel, network or tests ran. Actual drag delivery, rendering, swapped
slot refresh and cleanup remain unverified; native icons still need a valid
existing wheel and owned source powers. Gamepad work remains deferred.

## 2026-10-26: coordinate guest icon updates with native mouse gestures

Offline decoding established that normal quickslot mouse-up reaches the
existing doHotKey bridge before clearing its pending source index. Eleven
exports decoded completely, all38 explicit branch targets aligned and package
hashes stayed unchanged. This establishes a route, not delivered input.

The runtime now suspends icon writes while a native gesture is pending or
dragging, cancels pre-gesture queue scope and rechecks idle state before power
dispatch. Mouse-up can enqueue before source cleanup, preserving the stock
ordering; actual dragging refuses submissions. After an observed gesture,
the icon lease retires rather than repainting over native changes. Retirement
also avoids native setters while a gesture is active or unreadable.

Native guest drag/remapping remains unimplemented. The proposed earlier
assignment hook was not applied because automatic review hit a usage limit;
the worktree was clean at this turn's start. Three native targets compiled
for this separate coordination fix. No game, panel, network or tests ran.

## 2026-10-25: optional native quickslot icon lease candidate

The guest planner now offers --native-power-icons together with native power
hotkeys. The keyboard callback records its existing owned wheel; a pre-Tick
adapter captures all eight native icon members and previous power identities.
It assigns retained guest slot powers through the verified icon SetPower ABI,
using the icon's own ProcessEvent dispatcher. It never copies native strings,
opens a movie, changes the handler story-pawn cache or saves hotkey definitions.

Normal assignment is bounded to one slot per Tick, with fresh context/UI and
cache readback after the native call. Transient power-action unavailability
does not discard an otherwise owned icon lease. Retirement conditionally
restores still-owned widgets, clears expired prior powers and leaves foreign
changes/stale worlds alone. A retired observation requires another native
hotkey command before acquisition can recur.

The candidate is off by default and needs an already-existing wheel/controller
plus a real hotkey callback. Rendering/cooldown, mouse activation, dragging and
scene/disable restoration remain unverified. Three native targets compiled;
two changed Python files syntax parsed. No game, panel, network or tests ran.

## 2026-10-25: native quickslot icon setter and cleanup constraints

Offline package and image reads identified the native SetPower implementation
and the constructor-grounded quickslot vtable. SetPower can write power/pawn
before refusing an invalid movie; pointer readback alone cannot establish a
complete icon update. The icon's ProcessEvent dispatcher differs from the
controller's, and its clear virtual is a return-only leaf. Script Cleanup only
clears two references. Restoration therefore needs the prior power identity
and current movie/icon ownership, rather than a guessed ClearIcon call.

Ten selected script exports decoded completely; native bridge bodies were
analyzed separately. Nine explicit SetQuickSlotState branch targets aligned,
and source package/library hashes were unchanged. No runtime source or binary
changed, no icon was assigned, and no game or tests ran. Native UI integration
remains the next keyboard-power task; gamepad work stays deferred.

## 2026-10-25: native keyboard power command bridge candidate

An optional guest bridge now receives the engine's existing UseAbility0..7
commands at the native PC wheel virtual. The exact exec thunk, vtable slot,
implementation entry and retained SDK corroborate the handler/int ABI. It
queues the corresponding retained guest power name through the ordinary power
path, avoiding the stock implementation's cached story-pawn/icon selection.
It forwards other controllers and disabled/out-of-thread calls to stock code.
Installation requires the original slot/signature and uses pointer CAS.

The guest-only --native-power-hotkeys planner switch enables the candidate;
Ctrl+Alt shortcuts remain independent. Panel v6 identifies named guest slots.
Existing saved Shepard definitions and wheel icons are unchanged, and an
existing controller-specific movie/handler is still needed to reach the hook.
The bridge therefore does not complete native UI alignment or prove delivery.
Gamepad work is deferred. Three native targets compiled and three Python files
syntax parsed. No game, panel, network session or tests ran.

## 2026-10-25: resolve campaign mapped-power requests by current name

The ordinary runtime gained a mapped-action queue API for the six action
numbers actually handled by campaign TryUsePower. Tick reads the corresponding
verified BioPlayerInput name and resolves it uniquely against the owned
squadmate's current eligible powers before using the existing native order.
Direct catalog requests remain available. Both paths share the same bounded
queue, expiration, scope and dispatch gates. The input object is now part of
catalog identity; mapping resolution is checked again after the combat getter.

This implements the action-to-power resolution layer, not physical gamepad
capture or normal-input integration. Missing Ashley mappings fail rather than
selecting a similarly positioned power or changing profile settings. The three
native targets compiled successfully. No game, live input, test harness or
network session was started. Full campaign co-op remains incomplete.

## 2026-10-25: stock SP test scenario reaches normal gameplay

The separate stock SP test map was exercised through its actual squad-selection
GUI: Ashley and James were selected, then the normal equipment and character
screens were confirmed. A real conversation (mode7 with an active conversation)
and subsequent cinematic completed through normal input. The read-only observer
then recorded seven ready samples with Ashley in the roster and grounded player
state, first at approximately169.91 seconds. This was a single-process test;
it does not verify guest dialogue synchronization or paired combat in this map.

The normal Save screen was available and Create New Save was selected, but no
private save file was found after shutdown. Saving and repeatable reload are
therefore unverified. No enemy factory was exercised. The bounded run ended
with no remaining game processes, unchanged original profiles and no observer
errors. Reusing the compiler's content-addressed cache reduced the observed
launcher preparation delay; the stage executable is still freshly compiled.

## 2026-10-25: native guest hits reduce a real Trooper's health

A protected pair exercised the opt-in stock guest team initialization candidate.
The guest's previously null team was registered as host team0 through
SFXGame.ChangeTeam, restored through the same stock path on scene release and
registered again after reacquisition. A near-hit snapshot found both controllers
on team0, damage enabled and modes7/8/10 inactive.

The existing exact native guest-weapon dispatch observer recorded29 valid
enter/leave pairs with health decrease. Twenty pairs match a real
SFXPawn_Trooper_0 by pointer and internal object index in overlapping host
identity records: health512.5 to194.05014. Each original dispatch was forwarded
with unchanged payload and valid ownership scope. Nine further positive pairs
belong to another target whose class was not established. This is bounded live
evidence of guest weapon damage to an actual Trooper, not just ammunition or
animation. It does not establish enemy death, powers or complete combat.

Ashley died later in the run; the host also died. The scripted route stopped
at its normal traversal guard, and subsequent guest input provenance is not
established, so this is not an automated bot combat validation or sustained
mission-progression pass. The aggregate pair verdict remains a failure.
Original profiles and source/build were unchanged, no native exception or
remaining game process was recorded. One auxiliary memory helper exited1;
the run must not be described as every check passing.

The separate fast-test candidate cold-loaded a stock SP map with Shepard in
about36.45 seconds, but stayed in GUI mode with no Ashley and no ready samples.
Normal squad selection and a repeatable native save/start still need validation;
no working resettable combat arena or real conversation has been demonstrated.

## 2026-10-25: missing guest leader and unregistered guest team

The latest protected pair acquired Ashley with grounded movement state, but
the guest remained near the start. Its bounded channel scans found no
replicated player pawn, and every sampled follow input lacked a leader. The
measured-route bot requires that leader, so no guest enemy-hit trial occurred.
Two initial channel object reads returned null; their precise host identities
and the failing resolution branch remain unknown. This repeats the missing
Shepard symptom without establishing a new collision failure.

A read-only snapshot while both controllers existed found the guest's PRI
present with a null Team. The exact native controller getter maps that state
to team255. Host team0, damage enablement and inactive scene modes were also
observed. Stock hostility logic rejects an unknown team, making missing guest
team registration a concrete candidate for the earlier no-damage hits. This
snapshot was not taken during a shot and does not prove the rejected branch.

The initial-object observer now reuses the existing original scalar hooks to
record the reference tag and index. It does not advance or replay the archive.
Unknown scalar observations remain explicitly unknown. All three outputs built
and native forwarding, scope, scalar selector and low-integrity checks passed;
the new scalar observation has not yet been exercised in a real game.

## 2026-10-25: close an enemy-hit observation gap

The latest protected pair contains an accepted enemy health decrease of52.11,
but it cannot be attributed to the guest. Fourteen earlier hits on that same
Trooper bracketed unchanged health. The first decrease interval overlaps a
scene release to AI; the128-row hit observer had already exhausted its budget
before that interval. Missing later hit records therefore do not establish
that no guest hit occurred. Later raw lower-health samples fail the independent
sampler's snapshot validity checks and are excluded from the accepted delta.

The default-off observer now retains at most512 rows and explicitly reports
exhaustion, preserving original dispatch, arguments and error state. Native
regressions cover observation beyond the former cap and forwarding after the
new cap. All three native outputs built and the native harness passed. This
instrumentation change has not yet been tested in a real pair and does not fix
or prove combat. A private read-only capture can now trigger on a fresh admitted
hit instead of relying on an automated aim press that may never occur.

One late routing snapshot found damage enabled, target damage permission set,
plot protection unset and no active modes7/8 on the single controller still
present. It was taken after the relevant shots and cannot exclude a remote
controller's damage gate during the burst. Full combat and real conversations
remain unverified.

## 2026-10-25: distinguish selected mode from active damage-blocking modes

Exact-build native analysis resolved SFXGameModeManager.IsActive through its
native registration to exec RVA B399C0 and body BB2D20. The body bounds the
mode index by GameModes count and returns the selected mode object's active
bit at D0/mask2. It does not read manager.CurrentMode. A local CurrentMode0
sample therefore cannot rule out a still-active lower-priority conversation
or cinematic mode, including one on the remote controller inspected by
SFXGRI.ModifyDamage.

The private read-only capture specification now covers every live controller's
mode7/8/10 object identities and reflected active bits, as well as GRI damage
enablement and target protection flags. It validates array bounds and owner
identities rather than copying the native body's unchecked pointer access.
Only offline analysis and syntax checking ran for this update; no actual mode
bit or damage rejection has been observed yet. No mode flag is changed and
no narrative gate is bypassed.

## 2026-10-25: live weapon damage inputs are populated at enemy hits

The protected observer validated its reflected layouts in a real pair. During
the aimed burst,26 input records reported Damage.Value38.6, DamageHench0.3,
bSuppressDamage=false and a four-entry damage-type array selecting the live
assault-rifle class for mode2. Three admitted hit records joined two actual
Trooper targets to the independent host sampler. Their health still did not
decrease. Missing weapon values, a null damage-type entry and the weapon's
suppression bit were therefore not observed causes in this run.

Further stock-script review fully decoded the damage module's3650-byte body
using the existing LegendaryExplorer DynArrayFindStruct token format. Damage
also passes SFXGRI.ModifyDamage and BioPawn.ModifyDamage; either can reject it.
Relevant conditions include general damage enablement, active conversation/
cinematic modes, plot protection, actor damage permission and hostility
relative to the attacking controller. These runtime gates remain unmeasured;
none is assigned or bypassed. Difficulty scaling is additive in the multiplier:
OutHenchDamageScale zero alone does not imply zero damage.

The completed pair preserved original profiles, recorded zero native
exceptions and left no game process. The read-only monitor reported access
denied and the memory helper exited1. A subsequent read-only routing capture
was not executed because automatic approval review hit its usage limit.
No gameplay fix or full combat/dialogue success is established by this result.

## 2026-10-25: observe actual weapon damage inputs at the existing hit drain

Added bounded read-only damage-input rows to the existing protected
selected-weapon hit observer. Current reflected property names, kinds,
offsets, sizes, flags and suppression bit mask must match before reading
Damage.Value, DamageHench, bSuppressDamage and the firing-mode damage-type
array. The actual selected class is logged, with an invalid array/index kept
unknown. No getter is replayed and no damage event, field or payload is changed.

All three native outputs built; the native forwarding/isolation harness and
private launcher negative controls passed. Actual runtime damage-input values
have not yet been captured with this addition. Valid stock Avenger mode2 and
the existing Trooper damage module are reference evidence, not grounds for
forcing a fire mode, damage class or positive damage value.

## 2026-10-25: authoritative Trooper hits without a health decrease

The host walked to the side parking position and remained grounded. Guest
aim calibrated, locked and completed the normal two-second fire/stop cycle.
The host's hit-drain records now include two actual SFXPawn_Trooper actors,
joined by host pointer and UObject identity to the independent authoritative
sampler: live AI_Trooper controllers, team1, combat squad and reciprocal
SFXModule_Damage objects. Their512.5 health did not decrease in the bracketed
calls or the39-snapshot health capture. This establishes a real hit/damage
discrepancy; it does not establish successful hostile damage or a full fight.

Selected stock LE3 script exports were fully decoded offline using the
existing pinned LegendaryExplorer-based reader. ProcessInstantHit_Internal
guards its TakeDamage call with bSuppressDamage and a nonnull HitActor;
NumHits is not a damage gate in that body. GetFireModeBaseDamage reads the
scaled Damage.Value and multiplies by DamageHench for a henchman instigator;
GetDamageType uses the requested firing-mode entry for mode2. These identify
the next damage inputs to observe, not permission to force damage or change
weapon flags. The actual runtime input values remain unknown.

The completed protected pair preserved original profiles, recorded zero
native exceptions and left no game processes. The walking evaluator and
memory helper still failed their separate checks. Full conversations and
campaign progression remain unverified.

## 2026-10-25: aimed native fire and stop observed, host obstructed the shot

The measured-input-floor repeat completed yaw/pitch calibration, reached the
32-unit aim tolerance and dispatched normal FireWeapon after stable lock.
Two seconds later it dispatched StopFiringWeapon. The authoritative host
recorded17 original FireAmmunition calls during this burst. Host and guest
subsequently settled at consumed28/reserve210, with stable post-release
samples. These are bounded aimed-fire/stop observations, not complete combat.

All17 corresponding host hit-drain records identified the real host Shepard,
whose health700/shields500 stayed unchanged. The normal engine screenshot
shows visible Shepard directly in front of Ashley; enemy health did not
decrease. This shot therefore cannot validate hostile damage. It also provides
rendered evidence that remote Shepard appeared in this default-appearance run,
without proving full custom face/helmet synchronization.

For the explicit encounter-aim test only, the host's final parking target is
now shifted110 units along X and-110 along Y, using ordinary walking within
the unchanged final-leg corridor, normal ground/input checks and45-second
lower-route bound. No collision or actor position is assigned. Native route
and channel checks pass after rebuilding all three outputs; side parking and
enemy damage still need a live repeat.

Original profiles were unchanged, native exceptions were zero and no game
remained. The memory helper exited1 and a read-only monitor reported access
denied; the observer audit is not fully passing. Full mode-7 dialogue remains
unverified.

## 2026-10-25: retain context through measured normal-look calibration

The protected cache-recovery repeat completed the encounter approach, then
calibrated yaw and pitch through normal input at312ms and625ms. The prior
`positive-channel-not-ready` context cleanup did not occur during the25-second
aim interval. This supports the cache-eviction correction in one real pair;
it does not establish general combat or cross-PC reliability.

Aim reduced the initial yaw error19611 to402 and pitch error-1292 to-91,
then hit its25-second bound without firing. The shrinking proportional inputs
stalled outside the32-unit lock tolerance. Outside that tolerance, the bot now
uses at least the0.1 magnitude that produced measured calibration response;
inside it, the input is neutral. The0.15 maximum, angle tolerance, stable-lock
duration, total deadline and normal FireWeapon/StopFiringWeapon path remain.
Native boundary/sign checks and the rebuilt harness passed. This input-floor
change still needs live validation; no hostile damage is claimed.

The completed pair left original profiles unchanged, recorded zero native
exceptions and no remaining game processes. The walking evaluator still
rejected the normal-drop scenario and the memory helper exited1. Full dialogue
and campaign progression remain unverified.

## 2026-10-25: keep native player context across discovery-cache eviction

The protected guest completed the measured encounter approach and began
aiming. It stopped219ms later because the player-context adapter reported
`positive-channel-not-ready`, while subsequent telemetry still showed the
same reciprocal, autonomous Ashley channel. Target discovery resolves more
pawns than the shared four-entry hint cache can retain; the context adapter
had treated a missing hint as lost ownership and cleaned up its local squad.

On a hint miss, the adapter now performs one existing bounded channel lookup,
including its current-world and peer-uniqueness checks, then validates the
positive result normally. It does not retain a stale channel or alter native
roles. A native regression clears all hints while keeping the real fixture
channel reciprocal; recovery succeeds and duplicate-channel recovery is
rejected. The rebuilt native harness and low-integrity write-denial fixture
passed. The change still requires a live aiming repeat; hostile damage and
full conversations remain unverified.

The completed run preserved original profiles, recorded no native exceptions
and left no game process. Paired early door capture also reproduced NULL guest
Shuttle/ShuttleDoor group actors with normal host bindings and differing pose
hashes. The visible door's own actor/body collision was disabled on both;
other collision states differ and the physical blocking contact is unproven.
No collision disabling or door-state workaround was introduced.

## 2026-10-25: stop the encounter bot outside the parked host capsule

The stable-landing repeat reached all lower anchors except the final arrival
check. Guest and host stood about60 units apart, with the guest129 units from
the last anchor and the host already parked nearer it. The guest continued
ordinary movement input without advancing. This is consistent with character
collision, not proof of a new terrain fault or expiry of the follow window.
The final guest arrival radius is now150 units; intermediate nodes retain70
and all height/identity/corridor guards remain. No actor collision is disabled.

The completed run recorded192 Ashley samples with none below kill-Z, no aim
attempt and no enemy-health decrease. Original profiles stayed unchanged,
native exceptions were zero and no game remained. The memory helper exited1.
Door capture completed but began too late to observe the host shuttle or
either opening action; it cannot establish paired door collision correctness.
Native checks passed after rebuilding; final arrival still needs a live test.

## 2026-10-25: wait for a stable landing before the aim experiment

The protected guest traversed the upper anchors and reached the actual lower
platform. Its first aim attempt began while falling and ended187ms later when
the ordinary fire context changed, before the300ms calibration threshold.
No firing or hostile damage was verified. Of192 acquired-Ashley samples,
191 were walking and one falling; none was below kill-Z. The walking evaluator
still flagged the downward velocity and rejected shared travel. That gate has
not been weakened or reclassified as a pass.

The QA driver now admits vertical gravity motion only along this measured
drop's existing horizontal corridor and bounded height span; later road legs
retain the original sloped-floor check. The native regression covers the
observed guest landing and rejects the cut corner and positions below the
drop's allowed lower height. Aim startup additionally requires actual completion
of the measured encounter approach and500ms of grounded, low-vertical-speed
ordinary control, so it cannot pause the bot on landing for a distant shot.
Both native checks pass after
rebuilding all three outputs; these changes await a live repeat.

Raw look-input gain is unchanged. Shipped PlayerInput code applies time,
look-scale, FOV and other processing before the controller's integer rotation
cast. The cast alone therefore does not establish that the current gain is
too small. Original profiles remained unchanged in the completed run, with
zero recorded native exceptions and no remaining game processes; an access
denied observation and memory-helper exit1 prevent an all-pass observer claim.

## 2026-10-25: preserve the measured guest route through the drop

A protected repeat exercised the scene epoch correction: the guest restarted
at road point2 after the real scene and advanced through the upper road to
point11. It then reverted to direct leader chasing, cut across the drop
approach and stopped when the host exceeded the follow-distance bound. The
guest remained above kill-Z, but never entered the aim test; enemy health did
not decrease. This does not establish shared progression or working combat.

The protected guest bot now retains the existing measured last upper anchor
and five lower anchors instead of reverting at that corner. It reuses node
identity/location validation and normal movement, with the same input gain,
follow deadline and corridor checks. The native geometry regression admits
retained real host walk-off/landing positions while rejecting the guest's
witnessed cut-corner position. Both exported route/initial-reader checks pass
after all three native outputs were rebuilt. This route extension still
requires live validation; it does not resolve or validate guest door states.

## 2026-10-25: observe the initial actor reference without replaying its decoder

Added a default-off protected guest diagnostic for the exact executable's
FInBunch UObject reader. Its two-argument ABI and archive return were checked
against the original native body. The observer forwards once, preserves the
output, return and LastError, and records the first decoded object's identity
only inside the existing initial-receive scope. It neither replays decoding
nor repairs channel flags, actor references or packet fields.

A separate review found an off-thread access to the game-thread observation
frame; frame reads and updates now require the owning thread. Native forwarding
checks and the low-integrity protection fixture passed after rebuilding all
three outputs. No decoded-object live result or gameplay fix is claimed yet.

The completed protected pair subsequently recorded65 initial UObject results,
all with the original archive return preserved. The remote Shepard reference
resolved to the real male Soldier archetype and bound normally in this run;
the intermittent failing case did not occur, so it remains unexplained.
All39 enemy-health snapshots showed no decrease. Shared travel failed because
the guest QA route kept its pre-scene corridor after the stock scene moved
Ashley. It now restarts its waypoint/origin tracking on the existing guest
ownership epoch, including same-pawn reacquisition. This changes bot state,
not engine transforms. Native route/forwarding checks passed after rebuilding;
the epoch correction still needs a live repeat. The completed pair preserved
original profiles and recorded no native exceptions or remaining game
processes, but its memory helper exited1, preventing an all-pass observer claim.

## 2026-10-25: keep the encounter test guest on the measured road

The last protected pair completed the host's lower route to PathNode99, but
Ashley stopped on the upper road. None of the 194 guest Ashley samples was
below world kill-Z; no enemy-aim event ran because the guest never reached
the lower platform. Five authoritative Trooper candidates were sampled with
no health decrease. This does not validate aiming or hostile damage.

The protected lower-route test bot now follows the existing, validated upper
road anchors before resuming ordinary leader following at the drop. It keeps
its own waypoint progress and controller/pawn/world identities, preserves the
existing input gain and total follow window, and stops input outside the
measured corridor. The independent two-second forward pulse is disabled in
this encounter test so it cannot push the guest away from the route after a
scene. Ordinary player input and the shipped runtime are unchanged.

All three native outputs built; the exported route/aim checks and private
launcher ownership/negative controls passed. The route change still needs
live validation. Full mode-7 conversations and hostile damage remain unproven.

The subsequent protected pair could not exercise the new guest route: the
guest's remote Shepard channel failed its initial native actor decode. The
channel's flags changed from1 to9 while both Actor and Class stayed null;
later receives retained that state. No guest-road or aim event occurred.
The host still completed its lower route, but this is not shared progression.
All89 acquired-Ashley samples stayed above kill-Z. Original profiles were
unchanged, no native exceptions were recorded and no game process remained;
the read-only monitor reported access denied, so the observer audit is not
fully passing. Diagnose the initial object decode before another encounter
attempt rather than forcing channel flags or synthesizing a Shepard actor.

## 2026-10-24: stop skipping the witnessed post-scene QA firing case

- Retained real-game logs show the fire bot refusing an acknowledged guest
  pawn with local Role2, RemoteRole2 and a simulated Role1/RemoteRole3 weapon.
  The matching paired sample has the native no-fire bit clear. The rejection
  follows the existing ownership, channel, context and equipment guards.
- The QA bot had incorrectly required the pawn's RemoteRole to remain3.
  It now accepts the witnessed counterpart values2/3 while requiring local
  autonomous Role2. Simulated/authority/unowned local pawns, unverified
  counterpart states and the native no-fire permission remain rejected.
  Weapon role requirements and preceding scope guards are preserved.
- The old predicate failed the new offline regression; the corrected predicate
  and existing native DLL harness passed. Three compiled negative controls
  rejected the old RemoteRole filter, simulated local pawn and ignored native
  no-fire permission. This changes QA input eligibility,
  not native roles, weapon state or ordinary player firing behavior.
- The prior skipped scene-return firing case still needs a live run. No
  successful damage, repeatable combat or origin-position repair is claimed.

## 2026-10-24: renew the guest combat context after a same-pawn suspension

- Fixed a local context generation bug: scene/disabled cleanup could remove the
  guest's registered combat context while the same acknowledged pawn remained
  possessed. Ownership-only generation tracking then considered the cleaned
  record current and returned without registering a replacement after gameplay
  resumed. The gate generation now observes enabled ordinary control as well as
  pawn/controller/world identity, allowing the existing cleanup/rebuild path to
  run once on return. It does not alter native ownership or pawn coordinates.
- A temporal native regression failed with the previous ownership-only policy.
  The corrected policy passes repeated held frames, scene and disabled intervals
  with unchanged pawn ownership, return from both intervals, actual ownership
  loss and a pawn change. Both compiled negative controls were rejected: the
  old ownership-only policy and a policy advancing on every held frame. The
  existing offline DLL harness also passed.
- Pending cleanup still finishes before a replacement may be created. Failed
  creation remains suppressed within a control scope; held and blocked frames
  do not repeatedly advance generations or spawn actors.
- This fixes a code path that could leave native combat input context absent.
  No game was launched, so successful scene recovery, shooting, powers, target
  damage and the separate origin-position discontinuity are not claimed fixed.

## 2026-10-24: keep control yielded when dialogue state cannot be read

- Fixed a shared host/guest scene gate that treated failed cinematic-flag and
  conversation reads as zero. An unreadable controller flag, conversation
  pointer, active count or activation flag now keeps control yielded; a negative
  conversation count is also rejected. Readable ordinary gameplay and actual
  scene handling retain their existing behavior. No story/role/position writes
  or speculative coordinate correction were added.
- New native self-test controls failed against the previous gate and passed
  after the fix. All four compiled scene-gate mutations were rejected. The
  existing offline DLL harness passed, including its private low-integrity
  child. This is source-level error-path validation; it does not
  prove this read failure occurred in the retained falling run.
- No game was launched or attached. Reliable movement, hostile damage and full
  dialogue still require later real-game verification with the owner's consent.

## 2026-10-24: expose movement failure beside lifecycle results

- Offline evaluation of the retained failed pair using the existing verifiers
  reports lifecycle acquisition/release/restoration true but common travel
  false and unsafe motion true. Reasons include a below-KillZ sample, sustained
  vertical drop and excessive downward velocity. The maximum observed replica
  position error is about54,025 units. No new game or process access was used.
- The concise pair summary now exposes the existing common-travel verdict and
  unsafe-motion observation beside the lifecycle result. It does not change
  lifecycle semantics, relax movement gates or claim campaign validation.
- No additional movement verifier was added: the existing verifier already
  rejects this real regression. The receive diagnostic remains untested live.

## 2026-10-24: bounded receive observation prepared; no new game test

- Added default-off POSITION_RECEIVE_TRACE diagnostics to the existing protected
  ActorChannel receive wrapper. It observes only the current-world guest Ashley,
  logs at most64 large position or ownership/base changes, and compares stable
  object identity before/after the unchanged native receive call. It does not
  consume packet fields, change position or modify the ordinary player ASI.
- The protected probe can explicitly enable this existing receive hook during
  collision-baseline QA without enabling the separate package-map trace.
- An actual-wrapper fixture on authored memory passed default-off forwarding
  and12 enabled cases, covering position/role changes, identity replacement,
  nonfinite values, record budget, missing marker, wrong role/thread/world and
  unrelated actors. Each case checks one original call, preserved packet/channel
  bytes and incoming/returned LastError. This is offline observer validation,
  not evidence that the game's receive function caused the discontinuity.
- Live testing is deferred at the owner's request. No gameplay correction or
  successful movement/combat claim is made by this instrumentation.

## 2026-10-24: synthetic handoff exposes a position discontinuity

- A new protected pair completed three ownership epochs and AI/disconnect
  restoration, but failed gameplay continuity. The guest Ashley moved about
  54,107 units toward the world origin within 94 ms of the synthetic scene
  release. Its sampled controller detached and network roles changed to zero.
- About 594 ms after the next host acquisition, the host Ashley also jumped
  about 54,025 units toward the origin, lost its previous world base and fell.
  The guest discontinuity preceded the host discontinuity. This establishes
  ordering, not which engine call or packet caused either change.
- The same tracked ActorChannel identity/index survived the initial guest
  jump. Its retained SavedMove still held the original world coordinates;
  after reacquisition a newly generated move held the bad origin coordinates.
  Channel closure and stale saved moves are not established as the initiating
  cause. Receiver handling of unowned pawn location/base state needs inspection.
- No collision repair ran. The discontinuities occurred before the attempted
  ordinary W input, so that key is not their initiating cause. Normal key
  delivery remains unproven; no successful walking or combat claim follows.
- The paired read-only recorder now captures validated PlayerInput identity,
  pressed keys and axes, explicitly labelled asynchronous. Its bounded capture
  starts at actual acquisition so loading cannot consume the useful window.
- The lifecycle verdict is insufficient for gameplay continuity. Originals and
  profiles remained protected, native exceptions were zero and no game remained.
  The memory helper exited1 without a final summary; this is not an unqualified
  passing observer audit. Investigate release/channel/movement state across the
  handoff before broadening collision repair or adding position corrections.

## 2026-10-24: correct the QA input sampling phase

- The input-state record is sampled inside input_bot before session_guest_frame
  writes its movement pulse and before the original engine Tick. Its former
  post_tick_axes name was misleading; zero values do not prove that the pulse
  was overwritten. The record now explicitly names its pre-input/pre-Tick phase.
- The actual W mapping is a command alias chain to Axis aBaseY Speed=1.0.
  Retained pulse-window pawn samples show PlayerWalking, grounded physics and
  no sampled freeze, attachment or move-input suppression. None establishes
  that a normal key reached the input consumer. Distinguish ordinary key
  delivery from the raw-axis QA pulse before changing gameplay state.

## 2026-10-24: mode-0 landing dialogue returns; movement still unresolved

- The corrected protected pair completed three ownership epochs and native
  disconnect/AI return. Its landing conversation was active in 128 valid paired
  samples spanning 13.375 seconds, with both PlayersOnly and the custom focus
  owner flag false. Guest control returned after this scene. This is a mode-0
  scripted landing exchange, not a full mode-7 conversation test.
- No collision repair ran: the observed returned physical shape retained both
  actor/world and blocking collision, while its type fields were custom0.
  Do not expand the repair predicate solely to force an attempt.
- The guest positions were stationary within all three ownership epochs despite
  the QA movement pulse. Input delivery/processing must be distinguished from
  an actual movement restriction before changing collision or movement flags.
- Native exceptions were zero, original protection covered 8,210 files and
  unchanged profiles, no forced/unknown cleanup occurred and no game remained.
  The lifecycle runner returned0, but a process-exit monitor permission error
  made the overall wrapper fail. The observer audit is not an unqualified pass.

## 2026-10-24: focus restoration observed; mode-0 dialogue guard corrected

- The protected host logged an actual native HasFocus result0 adapted to1 at
  the second verified Tick site, cursor280 to281. Paired records then observed
  its focus-owned pause clear and Ashley's first ownership epoch begin. This
  proves the scoped getter path executed; it does not prove campaign support.
- A later mode-0 landing conversation again had PlayersOnly and the custom
  focus-owner flag enabled. The candidate's conversation exclusion prevented
  that focus pause from being restored, stalling the scene. No collision repair
  or usable scene return was demonstrated in this run.
- A new actual-wrapper regression reproduced this refusal. The focus candidate
  now permits its verified mode-0 Tick caller during dialogue. Possession/input
  handoff guards are unchanged, and a dialogue pause without focus ownership
  still refuses adaptation. The fixture passes 26 cases after failing the new
  dialogue case before the correction. Positive live dialogue return is pending.
- The game run ended with no native exceptions, unchanged original files and
  profiles, no forced cleanup and no remaining game. A process-exit read-only
  monitor permission error prevents calling its overall observer audit clean.

## 2026-10-24: retained blocking bit measured on control return

- The focus candidate installed in both protected game processes. Its first
  actual Tick cursor advanced from71 to72 as predicted. The pair acquired Ashley
  in three ownership epochs and completed scene handoff/disconnect checks, but
  no adapted focus result was logged; focus correction is still unverified.
- Guest control returned with world collision enabled, actor collision absent,
  blocking retained, cylinder response0xaa and type0/replicated0. The paired
  recorder captured 87 valid owned/acknowledged samples on the same positive
  channel. The prior predicate rejected this partial shape, so no repair ran.
- The candidate now admits that one additional measured shape. Its new native
  regression failed before the change and passed afterward, including both
  values of the unrelated adjacent flag. Existing identity, release snapshots,
  channel, ownership and native-call guards are unchanged. All three outputs
  rebuilt, the native protection harness passed and four unsafe variants were
  rejected. A positive live repair and usable walking remain unverified.
- The completed pair recorded zero native exceptions, unchanged originals and
  profiles, no forced/unknown cleanup and no remaining game. UI, paired and
  baseline recorders completed; the memory helper lacked a final summary.

## 2026-10-24: custom focus pause confirmed before possession

- A protected pair measured 39 consecutive host samples over 10.032 seconds
  with PlayersOnly and BioWorldInfo's saved focus-pause flag both enabled,
  no host foreground focus, a stable world and reciprocal guest membership.
  The guest had focus and both pause flags disabled.
- The actual Tick script checks its own HasFocus result and uses PauseGame to
  enter and restore this pause. The disabled stock Engine/HUD focus option
  does not control this SFX path. No browser-wheel or pause flags were changed.
- HasFocus reflection and native bytes matched in both processes. The narrow
  caller-scoped candidate now builds in both entry paths, and its actual-wrapper
  fixture passes 24 positive/negative cases. The native protection harness also
  passes. It has no positive live result yet.
  See [background focus](BACKGROUND-FOCUS.md).
- This pair still never possessed Ashley. Original protection covered 8,210
  files; profiles/source/build were unchanged, native exceptions were zero,
  no forced cleanup was needed and no game remained. Both read-only UI and
  paired recorders completed. The memory helper lacked a final summary.

## 2026-10-24: pre-possession pause identified independently of collision

- The existing bounded read-only paired capture now starts from verified probe
  headers instead of waiting for Ashley possession. No game behavior changed.
  In 738 valid host and 739 valid guest snapshots, the host consistently had
  `WorldInfo.bPlayersOnly` enabled while the guest did not. Both had an unset
  PlayerController cinematic bit and zero active/activating conversations.
- A separate read-only UI capture found a current-world HintBox message handler,
  but its active, modal and force-PlayersOnly flags were all false. That object
  is not evidence of an active owning modal and does not justify dismissing it.
  The pause setter and its correct normal restore path remain under investigation.
- The native connection persisted, but Ashley was never acquired, so the latest
  collision candidate was not exercised. This run provides no collision,
  movement or combat pass. No world flags, UI state or collision were forced.
- The paired recorder completed. Original-file protection covered 8,210 files;
  profiles and the frozen source/build were unchanged, native exceptions were
  zero, no forced or unknown cleanup was needed, and no game remained. The
  memory helper ended without its own final summary; this is not complete
  memory-observer coverage.

## 2026-10-24: measured partial collision return; native mask corrected

- The paired recorder now includes collision type and cylinder response from
  buffers it already reads. It captured 323 valid owned samples after control
  return with world collision enabled, actor/blocking collision absent, cylinder
  response0xaa and type0/replicated0, on the same positive native channel.
  The previous fully-disabled predicate rejected this stable partial state.
- The candidate admits only that additional exact shape. Native type6 restores
  actor/blocking collision while preserving an already enabled world flag;
  the world compare-exchange runs only if that flag is missing.
- Reflection identifies Actor.bBlockActors as mask0x1 at the captured field.
  The old mask also included neighboring0x2, which the native type6 path does
  not restore. That unrelated flag is now excluded from the physical shape and
  preserved. The native fixture's dispatch stand-in was corrected to match the
  actual native writes instead of manufacturing that neighboring flag.
- The fixture passed both neighboring-flag values with world collision already
  present, plus the prior release/identity/ABI refusal cases. Positive live
  collision repair and shared walking still need verification.
  All three native outputs built, the probe harness passed, and four compiled
  negative controls were rejected, including reintroducing the neighboring mask.
- The measurement run recorded zero native exceptions and kept original files
  and profiles protected. Shutdown observation had a permission error; retained
  paired capture completed, but the overall audit is not an unqualified pass.
  A separate read-only mesh capture observed animation ticking enabled throughout;
  it does not justify changing render flags or claim to solve firing.

## 2026-10-23: walking probe exposes stale collision-release state

- A measured protected pair reached actual landing control return and the
  separate synthetic handoff, with guest input after reacquisition. Native
  exceptions were zero, but the repair was refused and usable synchronized
  walking remains unverified. Lifecycle success is not physical gameplay proof.
- The retained-response candidate was observed live with Actor0/block2/cylinder
  0xaa/type0/replicated0 during release. Later paired snapshots changed again
  before reacquisition. Static review found that a subsequently rejected shape
  cleared `pending` but left `released` set. An actual-adapter regression failed
  on that stale flag before the correction. Both flags now clear together;
  refusal reasons identify the existing failing check without extra engine calls.
  A shape mismatch records the already-read before/expected values once.
  All three native outputs rebuilt; the probe harness and actual-adapter
  lifecycle fixture passed, including cancellation and a later valid repair.
- The memory observer measured only 277,741,568 bytes of available system commit
  during startup, with both games allocating roughly 4 GB each. The same run
  reached gameplay without a native exception. No matching System event2004
  was found; memory exhaustion is a lead, not an established crash cause.
- Original-file/profile protection and native cleanup passed; no game remained.
  A process-exit permission error affected the read-only monitor and the memory
  helper exited1 without a final summary. Retained samples remain partial evidence, not a
  claim of complete observer coverage. No further live rerun was made here.

## 2026-10-23: landing return retains a second collision shape and RemoteRole0

- A protected pair completed the actual landing release/reacquisition and the
  separate synthetic handoff. No guest sample fell below KillZ while stationary;
  no collision repair ran. This is not a walking or collision-repair success.
- Read-only paired capture measured the same guest pawn/controller/acknowledged
  pawn and positive channel after control return with LocalRole2/RemoteRole0.
  Its stable released collision was Actor0x20000000, block0, cylinder0x12,
  type8/replicated8: actor collision remained enabled while world/blocking
  collision did not. The previous predicates rejected both this shape and role0.
- The candidate now admits only that exact additional released shape. RemoteRole0
  requires a previously captured healthy baseline; local autonomous role and all
  ownership, world, camera, channel, stable-release and native ABI gates remain.
  The actual adapter fixture passed recovery and neighboring refusal cases;
  role bytes remain unchanged. Live repair is still unverified.
- The existing protected QA mode now permits its existing two-second guest input
  pulse after reacquisition, with following/firing disabled and the host route
  still parked. No new QA mode or timer was added. Physical movement must be
  measured separately from successful possession.
- The stationary run recorded one access violation after ordinary disconnect,
  outside both authored probe and game image. A later Windows error record,
  matched by PID, creation time and executable path, attributes the fatal fault
  to NVIDIA `nvwgf2umx.dll` at RVA0x59a996. A retained live process handle did not
  prove continued game execution. Root cause remains unknown. The final audit verified 8,210 original-file
  locks, unchanged original profiles/source/build, disabled probe/restored action
  and complete cleanup with no forced, unknown or remaining game processes.

## 2026-10-23: retained cylinder response measured; narrow correction prepared

- The released-shape record measured cylinder response0xaa while the engine
  changed Actor collision/type. The former predicate required the cylinder's
  actor-collision bit to clear too. It now also accepts the already validated
  default response0xaa, while still requiring Actor mask0, a stable released
  state and all existing ownership/context/channel/native-ABI guards. Partial
  states retaining world collision remain rejected. Live repair is unverified.
- This run captured partial Actor states with response0xaa; the earlier run
  captured complete Actor collision loss without a simultaneous cylinder read.
  Their combination remains a candidate, not a jointly observed live repair.
  All three native outputs rebuilt; the native probe harness and collision
  lifecycle fixture passed, including retained-response recovery and refusal
  cases. These offline checks do not establish working in-game collision.
- Exact native `SetCollisionType` disassembly writes both type bytes and calls
  `UpdateCollision` unconditionally. No force-toggle or extra native method is
  necessary merely because the current physical type already equals6.
- The measurement run failed on its second acquisition with a guest access
  violation in the game executable at RVA0x72b6c. A Windows error record matches
  PID, creation time, executable path and offset. Cause remains unknown; no
  collision repair was called before the fault. Exact authored ASI/PDB retained.
  Final audit verified8,210 locks, unchanged profiles/source/build, disabled
  probe, restored action and complete cleanup without forced termination.

## 2026-10-23: actual landing transition reproduced falling; repair still blocked

- The route reached its actual landing scene, released Ashley to AI, reacquired
  her, and parked at waypoint3. The existing later synthetic handoff produced a
  third acquisition. Native ownership/story restoration and ordinary disconnect
  checks passed, but gameplay failed: guest epoch2 descended below KillZ.
- Paired snapshots show guest actor collision mask0/block2 and roles2/2 after
  the real scene. The adapter had accepted the initial healthy baseline but
  made no repair attempt. The RemoteRole predicate correction alone therefore
  did not resolve falling. Component response/type values during release are
  still needed to identify the remaining physical-shape rejection.
- A small, rate-limited record now logs the released shape already read by the
  adapter, including whether its existing disabled-shape predicate accepts it.
  It performs no new engine calls, physical writes or predicate relaxation.
- Recorded native exceptions and observer errors were zero. All8,210 original
  locks, unchanged profiles/source/build, disabled probe, restored action and
  complete cleanup without forced game termination were verified. The runner's
  passing lifecycle result is not a passing movement/collision result.

## 2026-10-23: stationary handoff passed without reproducing world-collision loss

- The stationary pair accepted a baseline, completed a30-second first lease,
  synthetic release and second acquisition, then restored AI/story state on
  ordinary disconnect. Guest ownership/camera returned for roughly25 seconds.
  The guest retained world collision (`0x40000000`) and local autonomous Role2;
  RemoteRole remained3. No repair attempt occurred. This does not exercise the
  historical fully disabled collision shape or the new RemoteRole2 case.
- Recorded native exceptions were zero. Final audit verified all8,210 original
  locks, unchanged profiles/source/build, disabled probe, restored action and
  complete cleanup with no forced game termination. The monitor again recorded
  an access-denied error, so the broad runner's green lifecycle result is not
  evidence of a clean overall QA pass or working collision recovery.
- Stationary QA now parks at the second acquisition. This lets ordinary route
  input reach the actual landing scene before parking, while retaining the
  existing later synthetic handoff. The earlier failing capture combined these
  two transitions before world collision disappeared. Guest bot input remains
  disabled; no extra timer, mode, role write or partial-shape repair was added.

## 2026-10-23: healthy baseline accepted in-game; stationary recovery prepared

- With the existing extended QA deadlines, Ashley was acquired and the native
  collision adapter accepted her healthy baseline on channel27. A creation-pinned
  read-only snapshot independently captured the variant/sentinel shape. The
  frozen snapshot helper still evaluates the old predicate, so its subset
  rejection is expected and is not the current adapter's verdict.
- Host and guest each produced 60 ownership samples, covering59 seconds.
  Ordinary disconnect restored AI, preserved the host story roster and removed
  native driver membership. No native exception was recorded. The monitor
  encountered an access-denied error; this is not a clean overall QA pass.
  Final audit verified8,210 locks, unchanged profiles/source/build, disabled
  probe, restored action, no forced cleanup and no remaining game process.
- The collision predicate now also accepts the observed guest RemoteRole2
  while requiring local autonomous Role2 and the existing owned/acknowledged
  client context, channel, scene and identity checks. Neither role is written.
  Local simulated/authority roles and other remote values remain rejected.
  Real-adapter offline cases passed; live recovery is still unverified.
- Stationary QA keeps the route parked and guest bot input disabled, while
  restoring the existing timed scene/reacquisition policy. It can now exercise
  recovery without adding another mode or running the later route/cinematic.

## 2026-10-23: focused rerun ended without guest acquisition

- The reduced-observation run completed with zero recorded native exceptions.
  Host listen mode and sustained guest client mode were observed, but Ashley
  was never acquired. Consequently neither the corrected collision baseline
  nor a live collision repair was exercised. Client mode alone is insufficient
  evidence of playable co-op.
- Full timeline review corrects the initial tail-only interpretation: one client
  was present while the host control gate stayed blocked. Discovery later ran
  after the guest's180-second lifetime had ended. This does not establish a
  missing-controller fault or a fix for the preceding access violation.
- All 8,210 original-file/profile locks remained held through child cleanup;
  profiles and frozen source/build were unchanged. The probe was disabled,
  the action restored, and no forced cleanup or remaining game PID was recorded.
- Current priority is reliable squadmate acquisition, collision after control
  return, and authoritative combat. Cross-PC Radmin and packaging are deferred.
  Prefer a small measured gameplay correction over additional infrastructure.

## 2026-10-23: baseline rerun failed during join; focused diagnostics reduced

- The baseline correction rerun failed before guest acquisition or baseline
  evaluation. Windows recorded a guest access violation in `VCRUNTIME140.dll`;
  the probe recorded 174 guest and three host first-chance access violations.
  No live baseline or repair claim follows from this run. The cause remains
  unknown; the collision adapter had not reached its eligible path.
- The focused probe now leaves package-lookup and initial actor receive
  observation hooks uninstalled. Neither is needed for the first-lease shape
  measurement; native package validation and channel handling run directly.
  This removes diagnostic interference without claiming it caused the fault.
- Original-file locks, unchanged profiles, disabled probe and complete process
  cleanup were verified. No forced launcher/child cleanup was recorded.

## 2026-10-23: first-lease collision refusal reproduced and corrected

- The focused run stopped the route at waypoint2 and held Ashley for 59.36
  seconds on the host before a normal guest disconnect. AI and the host story
  roster were restored. No native exception occurred. This stationary scope
  does not validate combat, scene reacquisition or the earlier Bink failure.
- A creation-pinned, read-only snapshot captured the exact rejected baseline.
  Actual and class-default physical masks were identical: actor `0x60000000`,
  block `3`, component `0xaa`, collision type `6`. Both stored replicated type
  `10` (`COLLIDE_MAX`), the initial sentinel rather than a repair argument.
  The component used the Ashley combat variant's archetype, not the class
  default component directly. The former checks incorrectly rejected both.
- Baseline admission now accepts the observed replicated sentinel and pins the
  actual component/pawn archetypes, requiring matching classes and the component
  archetype's ownership by that pawn archetype. Physical equality with the class
  default remains required. Repair still dispatches the captured physical type6;
  scene, ownership, channel and role checks remain unchanged.
- All three native outputs built. Probe/runtime harnesses, real-adapter offline
  lifecycle cases for variant/sentinel and their refusal cases passed; three
  compiled physical-mask/CAS negative controls were rejected. A new live run
  must establish that the corrected baseline arms; no live repair is claimed.

## 2026-10-23: crash module identified; focused first-lease observation prepared

- A Windows Application Error record matches the failed host's PID, creation
  time, executable path and exception RVA exactly. The faulting module was
  `bink2w64_original.DLL`, at RVA `0xc8fe`. This establishes the module, not the
  cause or absence of an upstream caller error. Exact disassembly identifies
  an asynchronous Bink worker-context table dereference on that path. The
  proxy forwards the required imports by name; proxy and original DLL hashes
  match the protected copy's baseline. No DLL replacement is justified.
- A new default-off, protected-probe-only `COLLISION_BASELINE_QA` observation
  stops the host route at its first actual acquisition, suppresses guest bot
  movement/fire and the synthetic scene, and permits only that first lease.
  Native scene/disconnect cleanup remains active. It preserves all acquisition
  and collision guards; the purpose is to measure the rejected class/template
  shape before movement or reacquisition confounds it. This focused test is
  not a combat or complete lifecycle validation.
  All three outputs built and the native probe policy/isolation fixtures
  passed. The private capture wrapper's trigger/provenance tests also passed;
  no live focused run has exercised the new policy yet.

## 2026-10-23: three acquisitions; collision refusal and host exception measured

- The next completed protected pair acquired Ashley three times. Cached and
  current-world drivers matched with one client; remote discovery and native
  membership passed. Earlier blocked samples were ordinary scene gates, not
  evidence of a stale driver. Ownership predicates were not changed between
  the preceding failed acquisition and this run.
- The first lease released to AI after 6.469 seconds during an engine scene;
  the second released after 9.516 seconds through the explicitly synthetic
  gate. Both preserved the host story roster. The third lease had at least
  6.547 seconds of host observations before a native access violation. Clean
  final release/disconnect and combat did not pass.
- Collision baseline admission was refused twice inside its class/template/
  shape checks during the first lease. Later guest samples changed from
  RemoteRole3/Role2 to 2/2, which separately fails the candidate's ordinary
  state requirement. Falling and crossing Kill Z preceded the host exception.
  Neither relaxing role guards nor claiming collision repair is justified.
- The wrapper's incremental observer and final audit completed with correct
  role headers. Original-file locks covered 8,210 files; profiles were unchanged,
  no forced launcher/child cleanup occurred, the probe ended disabled and no
  game remained. One host access violation makes the gameplay run a failure.
- The old exception record lacks a timestamp and module identity; an RVA alone
  cannot attribute it to this ASI. The probe now records monotonic time,
  allocation-query validity and cached game/probe module-base comparisons,
  without loader/symbol lookup or exception recovery in the handler. The next
  real run must establish attribution; this observation change fixes no crash.
  All three outputs rebuilt and the probe harness passed the attribution,
  incomplete-parameter and isolation fixtures. No game has run with the new
  exception metadata yet.

## 2026-10-23: research connection reproduced; squad admission still blocked

- The next protected research pair omitted the ordinary activation DLC and
  runtime ASI. Native listener/client modes and a sustained connection were
  observed, but Ashley was never acquired. The route remained at its ordinary
  host-context gate. Collision baseline/repair, firing and control return were
  therefore not exercised.
- The cached host driver's readable client array was empty in sampled camera
  inventory records. This does not establish a stale driver: the global object
  connection count is not proof of current-world membership. Admission guards
  remain in place while their individual predicates are investigated.
- The wrapper's live observer and final audit exceeded their 8 MiB whole-log
  limit. The inner result recorded unchanged original profiles, zero native
  exceptions, no forced cleanup and a disabled probe. A separate read-only
  recovery supplement hashed and streamed the completed logs and confirmed no
  remaining game processes. It does not replace the missing outer audit or
  convert the failed live observers into a pass.
- Protected-probe admission diagnostics now compare cached and current-world
  driver scope, retain counters from the existing remote-controller scan, and
  report silent pre-acquisition rejections. They do not change possession
  predicates. Sampling is limited to 120 records at two-second intervals.
  All three native outputs built; probe and ordinary runtime harnesses passed,
  including diagnostic budget/rejection fixtures and file-write isolation.
  These offline checks do not establish live acquisition or a gameplay fix.

## 2026-10-23: late ordinary join measured; activation DLC rejection reported

- The protected rerun exercised the new admission policy: five stable samples
  over 1,000 ms, with dispatch at native world time 68.026 seconds. The immediate
  console response was empty. The reflected viewport observer recorded
  `Connecting...` and closed its 60-second window normally. No sustained native
  connection, Ashley acquisition, collision repair or combat was established.
- During this run the owner supplied an in-game corrupt-DLC dialog naming
  `DLC_MOD_ME3SquadCoop`. The screenshot's process was not independently attributed.
  The activation container must not be treated as accepted by the engine. Static
  review confirms that our Mount versions, priority and serialized bytes match
  the supported native reader and official writer. Its absent DLC-root TOC is a
  concrete omission; the exact failure has not been attributed to that omission.
  A conditional native coalesced-file error also reaches the corruption dialog.
- Cleanup retained 8,210 original-file locks, unchanged profiles and zero native
  exceptions. Both native launchers ended at their limits, no game remained,
  and temporary authored ASI/Mount files were removed without forced cleanup.
- Combat investigation returns to the protected research entry, which does not
  install this activation DLC. Ordinary startup and packaging research remain
  separate from Ashley collision, firing/stop and authoritative damage evidence.

## 2026-10-23: stable guest admission and native response diagnostics

- Ordinary join now waits for a stable reciprocal player/story context rather
  than consuming its one request on a transient initial-world control frame.
  The policy observes unchanged identities and advancing native world time;
  it does not force loading, cinematic or pause flags. This changes admission
  behavior without establishing the cause of the preceding failed join.
- The immediate native console response is captured before its existing free.
  A bounded read-only observer also records changes in reflected viewport
  progress strings, explicitly distinguishing stored text from a native event.
  PendingLevel's undocumented private fields are not used.
- All three native outputs built. The ordinary native harness passed readiness,
  string safety and observer lifecycle fixtures; six compiled contract faults
  were rejected. Python discovery ran 257 tests, 2 skipped. These offline checks
  do not prove an ordinary guest connection; a protected game rerun is next.

## 2026-10-23: ordinary guest join attempted without a sustained connection

- A protected pair used the ordinary runtime for both roles, with the probe limited
  to isolation guards. Host startup reached verified status 8 again. The guest's
  one-shot join briefly entered native pending travel, but every valid guest world
  sample (213) remained standalone. No sustained native connection or Ashley
  acquisition was observed. Collision restoration, combat and cross-PC play were
  not verified by this attempt.
- The join was dispatched in the guest's early initial world, whose observed world
  time was about 0.43 seconds. Joining before stable player/story readiness is a
  hypothesis, not an established cause. The exact native rejection/error was not
  retained; a next diagnostic must capture it and distinguish initial travel from
  a genuinely stable ordinary player context without forcing readiness flags.
- The completed run retained 8,210 original-file locks, unchanged profiles and zero
  native exceptions. Both owned processes ended within their bounded launchers;
  temporary authored ASI/Mount files were removed, the probe was disabled, and no
  game process remained. No native source change or new test pass is claimed here.

## 2026-10-23: ordinary host listener exercised in the current mission

- A protected actual-entry run loaded the separate ordinary ASI with the probe
  in guards-only mode. The ordinary entry opened native Listen in the current
  Mars world without reloading it. Its startup result was 8, the independent
  current-world reader observed native net mode 2, and Windows confirmed the
  process owned the configured local Radmin interface/UDP port 17779. No guest
  was launched, and no research networking bootstrap was used.
- The preceding failed actual-entry run exposed a command-buffer address 0x20
  before the native getter target. The reader now derives the target from the
  pinned instruction. Live class inspection also showed that Shepard derives
  from SFXPawn_Player, not the previously assumed BioPlayerPawn. Native fixtures
  now contain the observed hierarchy and reject the old address/class errors.
- Parallel review corrected native guest admission for the client's cached zero
  local port and retained collision observations across ordinary-runtime paused
  scene frames. Meaningful native fixtures passed; guest connection and positive
  collision repair are separate pending game checks.
- All three native outputs rebuilt. Python discovery ran 257 tests, 2 skipped;
  ordinary native and collision lifecycle fixtures passed, and three compiled
  startup contract mutants were rejected. The actual game run retained 8,210
  original locks, unchanged profiles and bounded cleanup. Temporary authored
  ASI/Mount additions were removed and no game process remained. A later
  PlayersOnly observation rules out a claim of uninterrupted ordinary control.

Next: connect a real protected ordinary guest to this listener, then measure
Ashley ownership, equipment and usable scene/control return. Real combat, full
conversations, mission travel, two physical PCs and player installation remain open.

## 2026-10-23: shuttle exit exercised and collision loss isolated

- A new completed protected pair cleared both authored shuttle exit anchors and reached the first road node using ordinary movement. The landing obstruction correction is exercised in this run; the remaining route and jump-down traversal are still unverified.
- An actual landing conversation then held the ordinary engine gate for about 13.39 seconds. Its conversation controller and playing line Matinee were observed in mode 0. This is a real engine interruption and release/reacquisition, but it does not meet the separate full mode-7 dialogue requirement. The route stopped after its two-second context allowance; the new neutral scene-wait policy is documented separately and has not yet been rerun.
- A read-only paired transition recorder sampled both processes for 120 seconds. Guest Ashley's world/actor collision cleared during release and remained disabled through subsequent ownership epochs. Both processes later observed her falling. Inventory remained present in this run, so missing inventory is not required for the fall. Native collision restoration after control return is the next fix; captured flag changes alone do not prove its complete cause.
- Firing/stop and shared travel failed their separate evaluators; no hostile damage or full conversation was verified. Pointer ownership, AI restoration and disconnect checks passed, which expressly do not establish usable control while a pawn is falling.
- The final report protected 8,210 original game/profile files, retained unchanged profiles, recorded zero native exceptions and required no forced orphan cleanup. The probe ended disabled, no game process remained, and current native fingerprints matched. Distinct source/build/result/audit files and raw transition records remain private.
- Packaging now authors strict LE3 mount metadata and a DLC-only Mod Manager archive, replacing the fabricated startup PCC and wrong game identifier. Archive/readback and synthetic rollback checks are separate from actual Mod Manager import and engine mounting, which remain unverified. See [activation packaging](DLC-PACKAGING.md).
- The ordinary ASI now implements an explicit one-shot current-world host Listen as well as guest join. It validates native interface selection, the actual bound endpoint, current-map URL ownership, transient two-player capacity and client-free genuine package-map initialization. It never implicitly reloads a mission. The integrated build produced all three outputs; the ordinary native harness and 246 Python tests passed with two skipped. These startup paths still require protected actual-entry game tests; they were not used in the preceding route experiment.

## 2026-10-23: actual drained-hit dispatch and landing obstruction

- The numbered-name correction allowed the protected input route to start. The host moved about 126 units, then stopped against landing-shuttle geometry before the first road node. The four-second stuck guard released the injected input. No jump-down action, hostile encounter or mission progress was established.
- The default-off hit observer installed in the actual supported weapon dispatch and captured two native drained `ProcessInstantHit_Internal` calls for the leased guest weapon. Both original calls returned with valid source scope. Both had a null hit actor and zero hits, so there is no target health/shield or damage evidence. See [hit observation scope](GUEST-HIT-TRACE.md).
- The first ownership epoch received start/stop firing and settled at host/guest consumed counts 2/2, reserves 210/210. After synthetic reacquisition, Ashley's position jumped and fell on both processes; the guest's equipment chain was ineligible. The host returned to the landing only after disconnect/AI restoration. This is an additional unresolved control-return failure, not solely a guest presentation offset; the first epoch does not establish reliable combat or reacquisition.
- The experiment ended before its final paired report was written. Its launcher handle and processes were absent when checked; the enabled research marker was recovered to disabled. Both saved logs contain zero recorded native exceptions and the 16 original profile files match the preserved baseline. This recovery is not a completed lifecycle or clean-shutdown pass, and the preceding run's final report was not reused.
- The separate ordinary entry now has an explicit one-shot guest join request. A protected guards-only probe mode retains profile/file protection without owning Tick, preparing actual ordinary-entry testing. These are source/offline checks; ordinary host startup and actual-entry game validation remain unfinished. The integrated Python suite passed 238 tests with two skipped.

Next: use the measured shuttle exit anchors for ordinary movement, diagnose the second-epoch guest fall, and complete native ordinary host startup. Real enemy damage, full conversations, sustained mission travel, two-PC Radmin and player installation/rollback remain open.

## 2026-10-23: numbered route discovery and separate runtime entry

- A completed protected pair started the listen host after 43.6 seconds and connected a real guest. The route never started and no squadmate lease was acquired, so the run establishes no traversal, firing, dialogue or combat pass. Original-file locks covered 8,210 files; profiles were unchanged, native exceptions were zero, no forced cleanup was needed, the probe was disabled, and no game processes remained.
- A read-only capture found all ten intended route nodes uniquely in the current world, with the expected classes and coordinates. The discovery code compared bare FName strings such as `PathNode` with numbered names such as `PathNode_103`, which could never match. Discovery now reconstructs the suffix from the separate native FName Number field. The native fixture covers the captured numbers, and a compiled off-by-one mutation is rejected. The corrected route still needs a protected game rerun.
- The separate ordinary ASI now builds from its own entry and shares the actual companion ownership, camera and AI-restoration core with the probe. It checks the full executable SHA-256 and has no QA movement, synthetic scene timer, forced map selection or timed disconnect. It currently attaches only to an already established native session: normal listen/join bootstrap and protected validation of this entry remain unfinished. See [runtime integration](RUNTIME-INTEGRATION.md).
- After integrating the reviewed sampler and runtime changes, all three native outputs built, both native harnesses passed, and Python discovery passed 238 tests with two skipped. These offline checks do not establish a playable mod. Actual enemy damage, full conversations, sustained mission travel, two-PC Radmin and player installation/rollback remain open.

## 2026-10-23: read-only combat sampler reviewed and corrected

- An external contribution added the read-only health/shield reader, CLI and 26 fixtures. Review reproduced false decreases when reads failed or different worlds/pawns reused an address; the original evaluator was not reliable damage evidence.
- The corrected evaluator accepts only complete host/standalone Role3 snapshots in stable world, pawn, module and shield identity epochs. Missing data is unknown; identity changes and invalid snapshots break comparison. Array contents and reciprocal component links are rechecked, and read/byte/time budgets bound snapshots.
- Candidate classification now uses exact archetype/controller ancestry. It remains explicitly a candidate classification, not a verified relationship to Shepard. CDO pointer/name and actor lifecycle checks are present; exact UObject flag meanings and unobserved complete identity reuse remain limitations.
- The CLI handles unavailable worlds, verifies exact sandbox executable path and supported SHA, rejects reparse/hardlink ambiguity and refuses output overwrites or output inside the game sandbox.
- Focused offline tests cover the original false positives, valid decreases, regeneration, component replacement, invalid gaps, authority and inventory/array races. No live game was launched for this review; live hostile damage and guest attribution remain unverified. See [sampler scope and validation](AUTHORITATIVE-COMBAT-SAMPLER.md).
- A later 20-second read-only attachment to the separately protected host verified the sandbox executable and SHA and produced 22 host-NetMode-2 records. Twenty-one snapshots were valid and one rejected actual actor-array/streaming changes. It enumerated 1,488 to 1,618 actors with no hostile candidates; this verifies attachment and schema, not hostile health or damage.

## 2026-10-23: route attempt exposes frozen animation ticking

- A separate protected Mars pair enabled the actual `WorldInfo.bPlayersOnly` flag about 36.7 seconds into the world. Ashley's mesh and cover-node tick tags then remained unchanged through about 187.8 seconds. The exact native mesh tick guard skips animation nodes while this world flag is set. Animation ticking outside rendering was already enabled; pause-animation, frozen, saved-pose and no-skeleton flags did not explain this capture. No animation or world flags were changed.
- Native research identified a normal message-box lifecycle that saves and enables `bPlayersOnly`, then restores its prior value when that same message box closes. The active owning message box has not yet been captured. This supports inspecting normal UI ownership before attempting any remedy; it does not establish that a particular tutorial or player action caused the pause.
- Both route attempts rejected the joined landing position before walking or guest acquisition. They therefore provide no new firing, ownership-lifecycle or combat pass. The default-off driver now bounds its first leg by the same 1,500-unit range as its ordinary input calculation, relative to the validated first nav point, and retains the actual grounded start as its first corridor origin. Later corridors and identity, story, camera, ground and duration guards remain in place. Native outputs built, the Windows protection harness passed, and four compiled unsafe-policy variants were rejected. Actual route traversal remains unverified.
- The latest completed run protected 8,210 original game/profile files, retained unchanged profiles, recorded zero native exceptions and required no forced orphan cleanup. The probe was disabled afterward, the native source fingerprint matched, and no game processes remained. Raw captures and distinct run/build records remain private.

Next: identify and normally close the owning modal UI, then exercise the actual input route to a hostile encounter. Consistent firing, enemy damage, real dialogue, two-PC Radmin and end-user installation/rollback remain unverified.

## 2026-10-23: observer-off repeat identifies port-arms deferral

- Repeated the protected pair with the unchanged native build and native firing observer disabled. RPC observations remained enabled; snapshot correction remained disabled. The one eligible 250 ms input pulse settled at host/guest consumed counts 0/3, reserves 210/210. After synthetic reacquisition, the guest inventory chain was not eligible for a second pulse. This fails the firing requirement despite passing the separate lifecycle checks.
- The authoritative start-fire request used mode 2 and advanced its script cursor from 0 to 77. The host stayed in `SFXWeapon.Active`, storing pending fire mode 2. `BioPawn.bInPortArms` cleared, but `bPlayingPortArmsAnim` remained set. Exact `SFXWeapon.Active.IsInPortArms` bytecode checks the latter flag, so `BeginFire` defers rather than entering `WeaponFiring`. This differs from the preceding successful run, where the playing flag was clear.
- Read-only native research maps normal flag clearing to `BioAnimNodeBlendAdditiveCover` blend-out and relevance loss. Whether the failing host's mesh/node stops ticking still needs direct measurement. A normal AI capture already had animation ticking enabled outside rendering, so setting that policy without inspecting the failing lease would not establish a fix. No animation, firing-permission or ownership flag was forced.
- Two ownership epochs, synthetic release/reacquisition and native disconnect/AI restoration passed. Protection covered 8,210 original files; profiles were unchanged, native exceptions were zero, no forced orphan cleanup occurred, the probe was disabled, and no game processes remained. The tested native source fingerprint matched at completion. These protections do not turn the failed firing result into a combat pass.

Next: capture the actual leased host mesh/node lifecycle, apply a supported normal-animation remedy, and follow the measured Mars route to a genuine encounter using ordinary player input.

## 2026-10-23: native firing and ammunition consumption observed

- Added a default-off, bounded observer of the supported weapon's native `FireAmmunition` virtual dispatch. Exact native instructions, reflected method ABI, vtable/slot, game thread and current ownership/channel scope are required. Original calls are forwarded once with `LastError` preserved. Incoming RPC snapshots now also record the actual requested fire mode, script cursor, complete state path, burst counters and raw port-arms flags. See [native firing observations](NATIVE-FIRE-OBSERVATION.md).
- All three native outputs built and the Windows harness passed. Python discovery passed 197 tests, 2 skipped. Four compiled RPC-observer faults and five native-firing-observer faults were rejected, covering forwarding, identity, incoming-channel classification, out-of-script cursor rejection and `LastError`; these fixtures do not establish gameplay behavior.
- In the protected pair, with snapshot correction disabled, two 250 ms guest input pulses produced six observed native firing calls on each side. Each matching enter/leave pair advanced consumed count by one. Host incoming start-fire calls requested mode 2, advanced their script cursor from 0 to 77, and changed `SFXWeapon.Active` to `SFXWeapon.WeaponFiring`. The first native firing call occurred inside each start-fire receive; subsequent calls occurred through the ordinary firing loop.
- The two pulses settled at host/guest consumed counts 3/3 and 6/6, reserves 210/210. Counts remained stable in the post-release observation windows. No enemy health/shield decrease or damage attribution was measured. This is one successful bounded firing observation, not reliable combat or a completed weapon fix: the preceding pair received firing RPCs without authoritative consumption, and no intentional firing behavior change was introduced between the runs. Observation/timing effects and initialization differences must be investigated.
- Two ownership epochs, synthetic scene release/reacquisition, preserved host story authority and native disconnect/AI restoration passed. Original locks covered 8,210 game/profile files; profiles were unchanged, native exceptions were zero, no forced orphan cleanup occurred, the probe was disabled and no game processes remained. The tested native source fingerprint matched at completion. Detailed build, runtime and call-pair audits remain private.

Next: repeat the firing result with observation disabled, explain the difference from failed runs, and measure authoritative damage against an actual hostile target. Real conversations, sustained progression, two-PC Radmin and player installation/rollback remain open.

## 2026-10-23: guest firing RPC arrival observed on host

- Added default-off `ME3_SQUAD_COOP_FIRE_RPC_TRACE=1` observations around 15 exact-build weapon class/state native script entries, including the separate inactive-state server methods. Installation validates reflected identity, parameter layout, script length and native dispatch; calls retain their original arguments and Windows LastError. Observations require the current reciprocally owned squad weapon and are bounded to 256 records per process. This does not alter firing or consume an RPC.
- Built all three native outputs and passed the Windows harness. Three separately compiled negative controls rejected missing original forwarding, changed self arguments and classification of a different channel as an incoming RPC. Python discovery passed 197 tests, 2 skipped.
- In a protected Mars pair, the snapshot correction adapter was disabled. Two 250 ms guest input pulses produced two `Engine.Weapon.ServerStartFire` and two `SFXGame.SFXWeapon.ServerStopFire` native entries on the host. Each matched the active native receive frame, target function/object and validated weapon channel 21; all four original entry calls returned with valid scope. The weapon state was `Active` before and after each entry. Host consumed count stayed 0; guest counts stabilized at 3 and 6, reserves 210/210.
- This establishes arrival at the selected host RPC entry, not execution of every statement or a full authoritative shot. Internal VM calls can bypass `UFunction.Func`, so missing nested/client entries cannot prove those methods were never called. Separate read-only research also maps the pawn shot counter to a weapon/effects path; a changed shot counter alone does not establish ammunition consumption or enemy damage.
- Two ownership epochs, synthetic scene release/reacquisition, host story preservation and native disconnect/AI restoration passed. Original locks covered 8,210 game/profile files; profiles were unchanged, native exceptions were zero, no forced orphan cleanup occurred, the probe was disabled and no game processes remained. The tested native source fingerprint matched at completion. Raw captures, call records and distinct build/run audits remain private.

Next: distinguish incoming script-body execution from dispatch-only observation, and observe the actual native ammunition-firing path. Real conversations, sustained progression, two-PC Radmin and safe player installation remain open.

## 2026-10-22: guarded host snapshot correction exercised

- Added a default-off research adapter that requests settled authoritative ammunition through existing owner-PlayerController native RPCs. Exact method/parameter/dispatch validation and the active reciprocal native receive channel authorize control messages. Ordinary script calls retain original forwarding. No new socket or package-validation change is involved.
- Requests/replies bind a bounded ID, guest scope generation and selected weapon channel. Only a fresh matching response can update an idle guest weapon whose ownership and local baseline still match. The host reads its actual counters and never writes them. Scope changes invalidate pending state; retries, payloads and observation counts are bounded. See [AMMO-REPLICATION.md](AMMO-REPLICATION.md) for the opt-ins, transport and limits.
- Built all three native outputs and passed the Windows harness. Three separately compiled mutants accepting an old generation, wrong request ID or non-idle response were rejected. Python discovery passed 197 tests, with 2 skipped. New offline regressions require all four exchange stages and exact host snapshot values without assuming synchronized clocks. Successful correction cannot grant a firing/combat verdict.
- A protected paired Mars run completed two requests and responses on weapon channel 29. Guest consumed count was corrected from 1 to authoritative 0 twice, with reserve count 210 unchanged. Guest-local roundtrip times were 281 and 375 ms. **The host consumed no ammunition during either 250 ms pulse.** This verifies bounded counter correction, while authoritative guest firing remains a failure in the same run; it is not reliable combat.
- Two ownership epochs, synthetic scene release/reacquisition, preserved host story authority and native disconnect/AI restoration passed. Original-file locks covered 8,194 game files plus 16 profile files. Profiles were unchanged, native exceptions were zero, no forced orphan cleanup occurred, the probe was disabled and no game process remained. The tested build still matched current native sources. Raw captures, distinct build/run fingerprints and the four-stage audit remain private.

Next: locate where authoritative guest firing is lost. Real conversations, stranded-companion catch-up, sustained progression, two-PC Radmin and player installation/rollback remain open. Counter correction alone does not satisfy these requirements.

## 2026-10-22: native ammunition decoding and correction observed

- Added an explicit research opt-in for the actual integer-property network serializer, with exact-build send/receive instruction pins, reflected ammunition metadata, current world, object stamps, reciprocal ownership, connection and weapon channel guards. Receive scratch values are tied to a nested-safe original `ReceivedBunch` frame and compared with final weapon storage. The observer forwards original arguments, result and LastError and does not change ammunition or payloads. It is disabled by default and bounded to 256 records per process.
- Added a conservative offline evidence evaluator and private-log CLI. Encoding, decoding, storage matches and actual changes remain separate. Unsupported/error/ambiguous records cannot create positive application evidence; packet matching and ammunition synchronization always remain unverified.
- Built all three native outputs. The Windows harness passed forwarding, nested-scope and budget fixtures plus existing safety checks. Three compiled mutants removing original forwarding, inheriting the outer receive or exceeding the budget failed. A separate evaluator mutation accepting final-storage mismatches was caught. Python discovery: 190 tests, OK, 2 skipped.
- A protected paired Mars run matched the selected weapon channel 26 on both sides. Six host encodes and six guest decodes were observed: five consumed-count fields and one reserve-count field. Five completed guest receives matched all six decoded values to final storage; one changed consumed count from 4 to 3. No record limit was reached and the offline audit rejected no recorded field/completion events.
- The first 250 ms pulse ended at host/guest 2/3: the guest decoded 2, then a later local increment reached 3 at the recorded release. The next pulse received count 3 while the guest was at 4, producing an observed native downward correction; its final counts matched at 5/5. Both pulses stopped under the existing evaluator; reserves stayed 210/210. The correction path works in this bounded observation, while end-of-fire agreement still fails in the same run. This is diagnosis, not a completed weapon fix.
- Two ownership epochs, synthetic release/reacquisition, preserved host story authority and native disconnect/AI restoration passed. Original-file locks covered 8,210 game/profile files; original profiles were unchanged, native exceptions were zero, no forced orphan cleanup occurred, the probe was disabled and no game processes remained. A preceding protected single-process reflection capture also retained protection. Raw field/reflection logs, distinct build/run fingerprints and offline audits remain private.

Next: determine the appropriate authoritative end-of-fire reconciliation path and test it against the observed late local increment. Real dialogue, stranded-companion catch-up, sustained progression, cross-PC Radmin and installation/rollback remain open.

## 2026-10-22: bounded current-world channel discovery

- Replaced the channel resolver's full object snapshot and nested guest driver scan with the current world's native NetDriver and bounded peer/channel lists. Exact-build instruction pins, live World/Level/WorldInfo identities, actor/world binding and matching before/after scope snapshots authorize this path. No native method is called and no engine state is written by the resolver.
- Kept all-peer ambiguity rejection, connection/driver reciprocity, exact channel vtable checks, channel-index bounds and reciprocal table membership. Missing or malformed scope returns unknown. Pawn-only and weapon-only public entry points remain distinct; published cache hints cannot bypass enumeration.
- Built all three native outputs. The native harness passed valid/invalid scope, public-type, duplicate peer/channel, native-pin and membership fixtures plus existing safety checks. Three separately compiled mutation twins removing world binding, reciprocal table validation or ambiguity rejection were caught. Python discovery: 178 tests, OK, 2 skipped.
- In a protected normal-duration paired Mars run, the last cumulative channel snapshots fell from host 3.696 seconds over 439 calls to 0.224 seconds over 564 calls, and guest 2.762 seconds over 2,654 calls to 1.244 seconds over 2,071 calls. Maximum calls fell from 318/668 ms to 24/27 ms. Counts, scene timing and machine scheduling differ, so this is measured resolver cost rather than a controlled FPS/GPU benchmark; remaining peaks are not zero. See [LOOKUP-PERFORMANCE.md](LOOKUP-PERFORMANCE.md).
- Both sides matched pawn channel 27 and weapon channel 24 within this connection. Two ownership epochs, synthetic scene release/reacquisition, preserved host story authority and native disconnect/AI restoration passed. Recorded firing intervals were 250 and 312 ms. Both pulses consumed ammunition and stopped; counts matched at 3/3, then differed at 7/8, with reserves 210/210. The ammunition problem remains open.
- Original-file locks covered 8,210 game/profile files. The report recorded unchanged original profiles, zero native exceptions, no forced orphan cleanup, a disabled probe and no remaining game processes. Distinct run/build/trace/profile evidence remains private. No real conversation, mission completion, two-PC Radmin or player-installation claim was added.

Next: inspect authoritative ammunition field transmission and client prediction/correction for the matched weapon channel. Startup repeatability, stranded-companion catch-up, real dialogue, sustained progression, cross-PC Radmin and installation/rollback remain open.

## 2026-10-22: measured function-discovery stall reduction

- Added explicitly opted-in, bounded wall-clock profiling for function discovery, channel resolution and Tick/diagnostic scopes. GPU timing is not measured. A protected 100-second Mars baseline found repeated function discovery dominating diagnostic cost: 54 `GetHealthPct` resolutions accumulated 20.537 seconds, with an 829 ms maximum. The diagnostic scope accumulated 38.836 seconds in its last emitted snapshot.
- Added a 64-entry game-thread discovery cache. Every hit revalidates current object-table membership and the function, owner and Function-class identities, including complete FNames. Changed identities fall back to discovery; function results are never cached. Caller-side validation and native game behavior remain intact.
- Repeated the same protected 100-second action: 63 `GetHealthPct` resolutions accumulated 0.306 seconds, including one initial discovery; 62 warm hits averaged 41.7 microseconds and peaked at 230 microseconds. The diagnostic scope accumulated 1.263 seconds. Callback counts and scene timing differed, so this is resolver/diagnostic evidence, not a controlled FPS benchmark. See [LOOKUP-PERFORMANCE.md](LOOKUP-PERFORMANCE.md).
- Built all three native outputs. The Windows harness passed the new synthetic function/owner/class/table-membership guards and existing safety fixtures. Separate compiled mutants truncating FName identity or removing table membership were rejected. Python discovery: 178 tests, OK, 2 skipped.
- A subsequent protected paired run passed two ownership epochs, synthetic scene release/reacquisition and native disconnect/AI restoration. Both recorded press/release intervals were 250 ms; both pulses consumed ammunition on host and guest and stopped under the existing evaluator. Counts matched after the first pulse (3/3), then differed after the second (5/6), with reserves 210/210. Reliable ammunition reconciliation remains unverified.
- The change trace matched pawn channel 27 and weapon channel 31 on both sides within this connection. Recorded change-sample intervals were 15ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¾Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¾ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¾ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¾Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â¦ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â¦ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¾Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¾ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â¦ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¾Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â¦ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¦ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¾Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â¦ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â¦ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¾Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¾ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â¦ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¾Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â¦ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¦ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â¦ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€šÃ‚Â ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¾Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Â¦Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¦ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬ÃƒÂ¢Ã¢â‚¬Å¾Ã‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â¦ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¡ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Â ÃƒÂ¢Ã¢â€šÂ¬Ã¢â€žÂ¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â€šÂ¬Ã…Â¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¦ÃƒÆ’Ã†â€™Ãƒâ€ Ã¢â‚¬â„¢ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã†â€™Ãƒâ€šÃ‚Â¢ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã‚Â¡ÃƒÆ’Ã¢â‚¬Å¡Ãƒâ€šÃ‚Â¬ÃƒÆ’Ã†â€™ÃƒÂ¢Ã¢â€šÂ¬Ã‚Â¦ÃƒÆ’Ã‚Â¢ÃƒÂ¢Ã¢â‚¬Å¡Ã‚Â¬Ãƒâ€¦Ã¢â‚¬Å“31 ms in this run, compared with multi-second gaps in the previous instrumented run. These observations do not establish a complete shot history or absence of all stalls. Separate channel-resolution measurements still had cold peaks around 318 ms on host and 668 ms on guest; this remaining cost has not been fixed.
- The two single-process measurements and paired run retained original-file protection. Completed reports recorded unchanged original profiles and zero native exceptions. The paired run protected 8,210 original game/profile files, required no forced orphan cleanup, disabled the probe and left no game processes running. Raw profiles and distinct build/run fingerprints remain private.

Next: inspect remaining cold channel-resolution cost and actual ammunition field updates without removing identity guards. Startup repeatability, catch-up, real dialogue, sustained progression, cross-PC Radmin and installation/rollback remain open.

## 2026-10-22: read-only ammunition change tracing

- Added opt-in post-Tick ammunition observations for reciprocally controlled Ashley on host and guest. Baselines restart on object/context changes; subsequent records capture observed consumed/reserve changes, actual pawn/weapon channel indices, local identities, roles, actor flags and scheduling fields. Counters, replication flags and scheduling are never written by this recorder.
- Extended the validated channel resolver with a weapon-only entry point while retaining the existing pawn-only public contract. Ambiguous/missing channels remain unknown. The recorder is limited to 128 observations plus one limit notice per process across all contexts.
- Built all three native outputs and passed the Windows harness, including the new trace filter test and original-file write-denial fixtures. Python discovery: 178 tests, OK, 2 skipped. Separate compiled mutations exceeding the sample budget or merging new contexts were caught by the native self-test.
- The first bounded pair did not acquire Ashley after a slow intro and generated no trace observations. This was not a successful runtime trace test. One extended bounded retry acquired twice and passed synthetic release/reacquisition plus native disconnect/AI restoration.
- The extended run recorded three host and four guest trace observations, with two context baselines on each side and no budget exhaustion. Both sides resolved Ashley to channel 15 and her weapon to channel 9 within that connection. The first pulse changed host consumed count to 4 and guest count to 7; reserves stayed 210. Thus the differing counts were observed for the same validated weapon channel, rather than inferred from matching class names.
- The first pulse stopped under the existing evaluator but failed ammunition agreement. The second input attempt produced no observed consumption; the overall firing/stop gate remained false. Logged press/release timestamps were 2,172 ms and 515 ms apart despite the requested 250 ms duration. These timestamps do not independently establish exactly when native input changed. Trace sample gaps reached 2,344 ms on host and 2,485 ms on guest. This is sampled evidence with significant gaps, not a complete shot history or a clean timing benchmark. Instrumentation and other diagnostic overhead have not been excluded as contributors.
- Host and guest sampled update frequency 100; this is configured frequency, not measured delivery cadence. Host update timestamps advanced, but post-Tick flags were sampled outside replication and cannot prove whether a particular connection's owner/dirty condition passed. Packet field transmission, delivery and prediction reconciliation remain unknown.
- Both completed runs protected 8,210 original game/profile files, recorded unchanged original profiles and zero native exceptions, required no forced orphan cleanup, and disabled the probe after shutdown. No game processes remained after the extended run. Raw evidence and build fingerprints remain private.

Next: measure diagnostic/channel-lookup costs and trace actual per-connection field updates before attempting an ammunition correction. Startup repeatability, catch-up, real dialogue, sustained progression, cross-PC Radmin and installation/rollback remain open.

## 2026-10-22: ammunition condition inspection and reserve-agreement fix

- Fixed a false-positive ammunition comparison: equal consumed-round counts no longer pass `post_release_ammo_agreement` when host and guest reserve counts differ. Reports now retain both sides' reserve samples. Stopping and ammunition agreement remain separate findings.
- Added an offline, read-only reflection-capture recognizer for the observed owner/dirty/authority replication condition. Both ammunition fields matched it in a fresh private capture. The output omits addresses and script bytes and cannot claim runtime synchronization or a dirty-gate failure. See [AMMO-REPLICATION.md](AMMO-REPLICATION.md).
- The captured condition includes the owning connection; it does not exclude the guest categorically. Static inspection also showed that `Actor.SetNetUpdateTime` only changes scheduling, rather than marking dirty state. Actual per-connection gate evaluation, field transmission and prediction reconciliation remain unverified.
- Ran another protected two-process Mars session with unchanged native code. Two ownership epochs, synthetic handoff/reacquisition and native disconnect/AI restoration passed. The guest had 131 consecutive client-mode samples spanning 131.062 seconds before intentional exit.
- Both firing pulses consumed host and guest ammunition and stopped within the evaluator's release grace period. This time the final consumed counts matched (1/1, then 6/6), with matching reserves (210/210). The previous run disagreed. This is a positive bounded observation, not a repeatable weapon fix or full-combat pass. The updated reserve comparison was applied offline after the run.
- Original locks covered 8,210 game/profile files; profiles remained unchanged, native exceptions were zero, no forced orphan cleanup occurred and the probe was disabled after shutdown.
- Python discovery: 178 tests, OK, 2 skipped. Changed expressions/operands/metadata, missing or duplicate fields and malformed private content are rejected by synthetic tests. A mutation removing reserve comparison was caught by the new regression. No native rebuild was needed because native sources were unchanged.

Next: trace live authoritative ammunition changes through the owning actor channel and distinguish missed correction from later client prediction. Catch-up, real dialogue, sustained progression, cross-PC Radmin and installation/rollback remain open.

## 2026-10-22: connection interval reporting and protected rerun

### Completed reporting change

- Connection evidence now retains the longest observed client-mode interval separately from the trailing interval. An ordinary disconnect ends the active-at-end verdict without erasing an earlier observation. Failure messages remain visible; historical observation is not a disconnect-cleanup, cross-PC or campaign verdict.
- The paired summary adds `sustained_connection_observed`. The existing `connection_states_verified` and `persistent_client_mode_verified` checks retain their active-at-end meaning. Reconnects and sample gaps over 2.5 seconds cannot be combined into one interval; invalid or non-increasing frame clocks cannot verify continuity.
- Python discovery: 169 tests, OK, 2 skipped. Seven added regressions cover active/disconnected states, reconnect boundaries, sampling gaps, invalid clocks, errors and missing timing. An in-memory mutation that erased completed interval history was rejected by the disconnect regression.
- Offline replay of three private guest traces distinguished one still-active connection from two intentionally disconnected sessions. The latter retained 119.641 and 124.234 seconds of observed continuous client mode while correctly failing the active-at-end check. Raw traces remain private.

### Actual game rerun

- Ran the native world-floor guard build in a bounded, protected two-process Mars test. Two ownership epochs, synthetic scene release/reacquisition, preserved host story control and native disconnect/AI restoration passed the lifecycle evaluator. No real full-conversation samples were observed.
- The guest had 125 consecutive client-mode observations spanning 124.234 seconds before intentional disconnect. The new evaluator was applied offline after the run; the running orchestrator had loaded the preceding reporting code.
- Ashley was already nearby when control was acquired. No out-of-range scenario or native catch-up call occurred. This rerun therefore **does not verify the world-floor catch-up fix**; a targeted stranded-companion scenario remains required.
- Two guest firing pulses consumed ammunition on both host and guest, and consumption stabilized after release within the evaluator's one-second grace period. Consumed-round counts still disagreed: host/guest 4/6 after the first pulse and 8/9 after the second. This is bounded firing/stop evidence, not an equipment synchronization or full-combat pass; enemy damage remains unverified.
- Original-file locks covered 8,194 game files plus 16 profile files. Original profiles were unchanged, native exceptions were zero, no forced orphan cleanup occurred, and the probe was disabled after shutdown. No game processes remained.

Next: exercise catch-up with a genuinely stranded squadmate and diagnose authoritative/client ammunition disagreement. Real dialogue, sustained progression, cross-PC Radmin and installation/rollback remain open.

## 2026-10-22: world-floor catch-up guard and firing audit

- A later bounded pair exercised two ownership epochs, synthetic scene release/reacquisition and native disconnect/AI restoration. This does not establish real dialogue or repeatable campaign progression.
- Its 250 ms guest input consumed one predicted round locally, while the authoritative host count stayed zero. Firing/stop-firing verification now requires observed consumption on both sides; stable zero counts cannot pass. Client-only prediction remains an unresolved combat failure.
- A read-only live capture observed a player floor normal of `[0, 0, 1]` with a null supporting actor (`Base`). Catch-up no longer requires a supporting actor: it validates finite health, velocity and floor data, while preserving walking, world, squad and ownership checks and the game's normal navigation/catch-up routine. No position is directly written.
- The guard change has native fixture coverage for ordinary world-floor readiness and invalid, steep, falling, non-finite and dead-player cases. An in-game rerun of this specific guard change is still required; it is not a confirmed fix for every stranded-companion case.
- The bounded pair recorded zero native exceptions, unchanged original profiles, no forced orphan cleanup and a disabled probe after shutdown. Its generic persistent-connection summary was false after the intentional disconnect despite the separate lifecycle evidence passing; this reporting discrepancy needs investigation.

Next: repeat the catch-up scenario, investigate the absent authoritative shot and inventory restoration after reacquisition, then exercise actual dialogue, cross-PC Radmin and installation/rollback gates. No gameplay release is newly claimed.

## 2026-10-22: public-source runtime checks and acquisition diagnosis

The public checkout now uses the renamed Windows native harness correctly; the old launcher path prevented protected boots from this checkout.

### Changes

- Replaced the ineffective ProcessEvent query of intrinsic GetStateName with an exact-build, read-only state-frame reader. Native code bytes, live state objects and stable frame/node identities are checked; unsupported or unreadable states remain unknown. No state is changed by this reader.
- Added bounded acquisition-wait reasons and firing-method resolution diagnostics without relaxing possession, story, distance, equipment or bytecode guards.
- Included firing/stop-firing and real-dialogue evidence in paired-run reports. Dialogue verification now also requires observed ownership before the scene; a release/reacquire label alone cannot pass it.

### Actual runtime results

- A protected 35-second boot from the public source checkout passed. The original profile remained unchanged and the probe was disabled after the run.
- Two subsequent isolated paired runs connected through native networking, but neither acquired the guest squadmate. They did not exercise firing or real dialogue and are not successful session lifecycle tests.
- In the diagnostic run, acquisition reported `companion-out-of-range` 35 times. The host and squadmate became separated during the Mars intro transition. The native state reader observed `PlayerWalking` and AI `Idle`, rather than the previous misleading `None` result.
- The diagnostic run recorded zero native exceptions, unchanged original profiles, no forced orphan cleanup and a disabled probe after shutdown.

Earlier bounded possession/lifecycle observations remain documented in VALIDATION.md. The latest failure shows those results are not yet repeatable for this startup route. No campaign, combat, cross-PC Radmin or release-package milestone is newly claimed.

### Next investigation

Inspect the normal squad-follow/catch-up path and its actual ground/readiness conditions at the intro transition, then repeat ownership and equipment tests. Do not remove the distance guard or directly relocate actors to turn this failure into a pass. Reliable acquisition is required before the weapon and real-dialogue gates can be exercised.


### Relative-location field comparison (2026-10-25)

A protected pair recorded a large guest `RelativeLocation` change inside the
original native receive call. The field observer and receive entry/exit snapshot
agree, while world `Location` was unchanged in that call. This does not prove
that the field caused a later position jump. Nearby host field samples were
normal, but timestamps alone cannot identify the exact sender packet. Shared
progression, full dialogue and combat still failed validation. Original files
and profiles were preserved; read-only helper failures prevent an all-pass audit.

The opt-in observer now also records host Ashley `RelativeLocation` serialization
through the exact native sender call site and the existing writer scalar hook.
It requires a live current-world Ashley, the pinned writer/package-map tables
and a non-loading archive. It forwards the original once and never changes
payload, actor position or archive cursor. Authored-memory tests check sender
identity rejection, scalar observation, forwarding and error preservation;
these tests are not live packet matching or a gameplay fix.


### Ordinary-action test deadlock

The preceding route settled at80.75054 units from the real jump-down top with
actual Touching, zero velocity and neutral input. Braking retained neutral input
through82 units, but readiness rejected anything above80, so the bot waited
until its10-second bound. The retained native selector checks Touching, node
and reachspec eligibility and direction scores; no80-unit comparison was found.
That80-unit threshold was an authored QA heuristic. Readiness now shares the
existing82-unit braking margin, retaining actual contact, facing, speed and
settling checks. The bot still sends ordinary `PC_Action`; native selection and
traversal are untouched. Regression checks cover the recorded80.75054 case and
reject missing contact, excessive speed and insufficient settling. Live
traversal remains unverified.


A subsequent protected pair recorded matching sender/receiver `RelativeLocation`
scalars and the expected rounded value, then later a guest `Velocity` decode
followed by archive errors. The healthy field does not reproduce the earlier
large relative-position change. That run still failed shared progression and
the guest fell below world kill Z. Native exceptions were zero and original
profiles remained unchanged; the memory helper exited1. Host `Velocity` is now
included in the same bounded observer to compare the failing field directly.
Equal field values alone do not identify a packet by its header.


### Optimized movement ownership observation

The next route passed the corrected contact readiness but rejected before
ordinary action dispatch. Cleanup and setup previously shared one rejection
label; the probe now reports the first failing setup gate without relaxing it.

Retained native analysis identifies a specific framing hazard: the optimized
movement body returns without reading payload when `Actor.bNetOwner` is set.
The receiver invokes it only after reading a nonzero optimized-payload marker,
then continues to generic field headers. Sender ownership is computed per
connection and later cleared, so asynchronous host/guest flag differences do
not prove a mismatch. A protected receiver-only observer now records exact
optimized-call entry ownership, archive cursor/end/error and pawn/shadow
positions. Its authored-memory fixture checks unchanged five-argument forwarding,
identity bounds, installation pins and bounded recording. It does not change
ownership or consume the payload; the actual runtime failure mechanism remains
unverified until those call records are examined.


### Live call-site correction and combat scope

Frida observation on the protected guest proved the optimized Ashley callback
also enters from native return site `488AEB`, the earlier compiler copy in
ReceivedBunch. The observer had admitted only `489ADF`; its lack of rows was
therefore not evidence that optimized movement never ran. Both exact sites
now share the same identity/archive guards, with authored-memory coverage.
Direct Frida samples of other Ashley replicas showed ordinary cursor advances,
not the failing ownership condition. No owner-bit intervention has been applied.

A subsequent normal-action QA tap was actually pressed and released but produced
no traversal. Dispatch alone remains insufficient. Host mesh telemetry also
showed animation ticks progressing with the background-animation bit already
enabled, so enabling that bit is not justified by this run. Guest reacquisition
was followed by a missing pawn inventory manager and no second-lease fire input.
The exact native reference decode resolved a different class and returned null;
its initiating packet/ownership cause is still under investigation.


### First scoped optimized-payload correction trial

A protected baseline recorded124 optimized callbacks with `bNetOwner` set and
no archive cursor advance, although native dispatch had already consumed the
positive optimized-payload marker. Inventory was subsequently cleared by a
reference decode. A default-off QA candidate now clears only that transient
owner bit around the unchanged native callback and restores it to the same
live actor. It does not overwrite positions, decoded values or archive cursors.

The first real-client candidate run applied once, restored the bit successfully
and advanced the archive without error. No inventory-change event was observed.
The existing shared-travel evaluator passed a40-second interval: host Ashley
and guest displaced about1871.7 units with direction cosine0.9966. All105
post-acquisition guest progress samples were grounded and none below kill Z.
This does not prove continuous ground contact between samples or every scene.

The second firing pulse in each ownership epoch passed authoritative/guest
ammunition agreement and stable stop checks (used6/reserve210 in epoch1;
used17/reserve210 in epoch2). The first pulse in each epoch lacked sufficient
same-weapon baseline evidence, so the aggregate fire-stop gate remains false.
Eight authoritative hit-drain calls were observed, all with no valid target;
enemy damage remains unverified. Full dialogue and mission completion remain
unverified. Original profiles/source/build were unchanged, native exceptions
were zero and no game process remained. The memory helper exited1, so the
observer audit is not an unqualified all-pass. Repeat validation is required.

### Ordinary drop and remote player appearance

A later protected pair reached the lower Mars platform with ordinary host
movement input. Input stopped on loss of ground after about2.30 seconds;
the native pawn then entered falling physics and landed near the measured
bottom node, before guest disconnection. No position writes or forced custom
traversal action were used. This proves one host drop, not guest traversal,
enemy damage or mission completion. The guest bot's older36-second follow
window had already ended; the protected combat test now allows bounded follow
for145 seconds and requests disconnect after150 seconds. Native process
deadlines remain unchanged. That follow-window change needs a live rerun.

An owner-assisted run independently reported normal Ashley movement, firing,
release-to-stop, ammunition consumption and reload, but an invisible host
Shepard even at close range. Read-only Frida measurements confirmed a live
replicated player with a null skeletal-mesh asset on the guest, while the
host had a live composite mesh. Selected reflected appearance IDs matched,
but helmet overrides differed. The normal appearance initialization path is
being investigated; no appearance repair or full appearance synchronization
is validated. Real enemy actors were present in the manual run, but the
recorded impacts did not establish enemy health or shield loss.

The drop run recorded no native exceptions, unchanged original profiles and
no remaining game processes. Shared travel and lifecycle gates passed their
limited scopes; connection-state aggregation failed, and a read-only monitor
reported access denied. This is not an overall passing campaign test. Real
full dialogue and attributed damage to a hostile remain unverified.

### First remote appearance asset application

In one protected pair, the guest's replicated Shepard had a null skeletal-mesh
asset. The normal `UpdateAppearance` wrapper alone did not populate it in an
earlier run. Exact package flags and native Actor callspace code identify a
completion bridge without the simulated-function flag; the experiment checks
the actual player callspace target before continuing.

A separate default-off QA option checks the engine asset loader's existing
group through `IsAsyncGroupLoaded`, then calls the normal simulated
`ApplyAppearance` once. That run returned with the same pawn/world and a live
asset. Thirty subsequent channel samples retained the asset, following nine
samples without it. No role, function-flag, coordinate or asset-pointer write
was used. This covers only the guarded default male appearance case; matching
host face/helmet customization and rendered visibility remain unverified.

A preceding run could not exercise the candidate because its incoming player
actor never formed: the initial native receive marked its channel unusable.
That intermittent startup failure remains unresolved. Passive Frida tracing
in the positive run timed out without retained results and is not credited
as successful initial-object evidence. A repeat without that tracing is needed.
Original profiles were unchanged, native exceptions were zero and no game
process remained; the broader campaign, observer and lifecycle checks did not
all pass. Enemy damage and full conversations are still unverified.

### Appearance repeat and first lower encounter measurements

A subsequent pair without Frida repeated the loaded-group appearance result:
the same guarded normal API calls produced a live remote player mesh asset.
This repeats native asset initialization only; the host was outside the
inspected guest screenshot, so rendered visibility and customization still
need verification.

The new ordinary-input lower route reached measured nodes102,101,100 and21.
Its35-second limit stopped it about400 units before node99. A separate
read-only sampler verified the owned authoritative process and collected26
snapshots of five visible, loaded Trooper candidates with TeamIndex1 and
reciprocal damage-module identities. A sampled current/max health pair was
512.5/512.5. No health/shield decrease or guest damage attribution was observed.
These measurements establish real encounter targets for the next firing test.

The protected lower test now allows45 seconds for those five nodes. Its
bounded follow/disconnect windows are205/210 seconds, with native host/guest
process limits420/330 seconds; other test modes retain their previous limits.
The adjusted source compiled and offline controls passed, including evaluating
the adapted launch assignments against those native limits. The longer test
window has not yet been exercised. Original profiles were unchanged, native
exceptions were zero and no game remained after the preceding run; the
read-only monitor again reported access denied, so it was not an all-pass audit.
## Protected manual arena pair preparation

A default-off probe-only `ME3_SQUAD_COOP_ARENA_PAIR_QA` option selects the
private SPCer fixture instead of the Mars route. It requires private file
protection, host/client research roles, progress/session support, manual guest
input, host map refresh, and no Mars prewarm/reload. It disables synthetic
scene release, host route input and companion catch-up. Acquisition still uses
the shared native scene, squad identity, ownership and equipment guards.
The ordinary player ASI does not use this option. Policy fixtures passed;
this preparation alone does not establish a connected arena pair or combat.

## Unattended private arena preparation

A second default-off protected-probe flag, `ME3_SQUAD_COOP_ARENA_AUTO_QA`,
drives the fixed SPCer setup through normal TeamSelect, confirmation-box,
loadout, character-record and scene-skip callbacks. It resolves Ashley and
James from the live reflected roster and validates callback ABI and UObject
ProcessEvent before dispatch. No desktop input is sent. Both roles complete
the same map setup; the host refreshes its native package map only after
five seconds of ordinary control, and the guest joins after its complete
standalone roster is present. Native package/GUID checks remain unchanged.

After acquisition, this opt-in allows one bounded normal movement pulse,
the private arena console event after its stock spawn list is populated,
and the existing bounded guest look/fire test against a real Trooper replica.
Identity, scene, equipment, floor and native ownership checks still apply.
Input cleanup failures stop the test action. Native fixtures pass; no
unattended connection, movement, hit or kill is established by preparation.

## Offline review of unattended arena startup

The fixed native setup reached normal mode0 after the five normal menu
callbacks. Full session readiness was later than the sequential launcher's
host deadline, so the guest never started. The retained native map refresh
at294.766 seconds also leaves less than the required240-second guest lifetime
inside the480-second host limit. A longer readiness deadline alone is not a
complete fix. Live tests are deferred; see [campaign integration priorities](CAMPAIGN-NEXT.md)
for the evidence limits and proposed concurrent prewarm boundary.

## Parallel arena prewarm preparation

Added a default-off per-run coordinator event to the protected arena guest
join path and once-per-second full prewarm telemetry. The private proposed
orchestration now prepares both retained children before waiting for the host,
with bounded common observation time. Native scene, squad, ownership and
package validation remain authoritative. Native outputs were rebuilt and
launcher source assembly was syntax-compiled; fixtures and live validation
are explicitly deferred. See [campaign next steps](CAMPAIGN-NEXT.md).

## Explain shared scene admission without weakening it

The shared scene predicate now returns its first blocking reason; its existing
boolean wrapper still rejects exactly when that reason is present. Host gate
records and protected arena prewarm telemetry include `engine_block_reason`.
Mode0 can now be distinguished from cinematic, PlayersOnly and conversation
conditions without inferring that all control is available. No scene state is
written. Native outputs were rebuilt; gate fixtures and live verification are
deferred. See [scene admission diagnostics](SCENE-GATES.md).

## Equipment handoff review without a game run

The shared handoff remains limited to one normal ClientGivenTo attempt per
ownership epoch and remote controller. Scene release prevents the held-policy
callback; reacquisition uses the next epoch. This local code does not maintain
an independent queue of deferred handoff requests. Delivery and ordering of
native RPCs remain engine behavior and are not proved by this source review.

A bounded offline replay of19 retained native logs found9 handoff attempts and
no repeated epoch/remote key within a process log. It found no empty rejection
reason. This is a limited historical observation, not validation of all
reacquisition, weapon switching, inventory continuity or native message ordering.

Source review did expose overlapping formatting: the throttled resolver can
return its own cached error buffer, which the caller formatted back into the
same buffer. That self-copy is now avoided. Rejection throttling also keeps an
owned copy of the previous reason instead of a pointer into a mutable resolver
buffer, so a changed reason is not mistaken for the previous value. An unused
resolver wrapper was removed and the stale draft-only header comment corrected.
No equipment dispatch, retry eligibility, role, inventory, ammo or ownership
policy was changed. All native outputs were rebuilt; fixtures and live tests
remain deferred. This does not establish a fix for lost guest weapons.

## Bind reacquisition waiting to current object identities

The clear-control500ms admission window no longer carries only a timestamp.
It is bound to the current host, Shepard, world, squad, mode manager and driver;
identity or scope changes restart the wait. This closes a source-level stale
warmup path without clearing session faults or relaxing story/scene checks.
Native outputs were rebuilt. Regression fixture and mutation source is
prepared but execution is deferred; no new campaign-travel result is claimed.
See [admission stability](SCENE-GATES.md).

## Honor native pause during shared control admission

The shared control gate now checks the existing startup path's native Pauser
and pending PlayersOnly fields as well as active PlayersOnly. A mode0 sample
cannot admit acquisition while a pause owner exists or the pause-owner field
is unreadable. No pause or story state is cleared. Normal native outputs were
rebuilt; prepared pause/guard regression cases and live checks remain deferred.
This is a source-level guard correction, not a demonstrated campaign-pause fix.

## Parallel prewarm focus dependency

The prepared parallel trial now has a protected SPCer-only focus policy for
the two preconnection prewarms. The prior focus policy required a peer that
the coordinator deliberately withheld until both prewarms were ready. This
source-level dependency is addressed without changing ordinary player startup
or clearing native pause flags. Native outputs were rebuilt; the new policy's
fixture and live checks remain deferred. See [background focus](BACKGROUND-FOCUS.md).

## Parallel arena opening and in-place listener candidate

The first protected parallel run completed both native menu/roster prewarms
and released the join event after about115 seconds, leaving over335 seconds
on each native lifetime. The guest then rejected BioP_Global: the host had
loaded the base carrier (GUID dfa65d51-482b1901-8337c0b1-72c320fd,332 net
objects), while the guest had loaded DLC_EXP_Pack003's carrier (GUID
d002cc41-4a8ecfb3-4c769494-176b4613,392 net objects). Native validation was
unchanged. No guest Ashley acquisition or paired arena combat was established.
Both exact owned children were stopped early after this rejection; original
profiles were unchanged, no native exception was recorded, no game remained
and the private arena package was restored. The memory observer still exited1.

The next protected parallel candidate initializes the host as standalone too,
then reuses the ordinary runtime's checked current-world Listen implementation
after the complete roster and five seconds of clear control. It preserves
normal package resolution, code/ABI pins, endpoint checks, capacity contracts,
scene gates and the coordinator event. No package GUID or validation is edited.
Only protected parallel arena startup selects this path; ordinary player entry
is unchanged. Native outputs rebuilt and the existing native runtime harness
passed. This wiring and the new host standalone focus case still require a
live result; successful prior Mars Listen evidence does not prove arena joining.

The first standalone-host attempt selected the same DLC Global carrier on
both peers, but the unchanged ordinary transport contract refused the probe's
loopback bind. No guest join was released. A subsequent protected run used the
existing local Radmin interface for both processes on this same PC. Native
Listen returned verified status8 without map travel. The coordinator released
join after about124 seconds; native package negotiation completed with
validation unchanged and the guest entered net mode3.

Guest Ashley ownership and its camera target were observed for207 seconds;
the host retained Shepard/story authority for209 seconds. The normal arena
event spawned three native enemies. Across49 authoritative health samples,
one Trooper fell from504.1667 to75.7068 health. Attribution to guest firing was
not established: no guest firing success is claimed. The host's bounded walk
completed; the guest's pulse aborted. The guest's existing BioPlayerSquad
remained present with a null cached player pawn and a false native combat
getter; the context adapter currently preserves that existing squad. Supporting
this native context is the next feature blocker, separate from package loading.

Both exact owned children were stopped after observation. Host disconnect
cleanup restored Ashley's AI and preserved story state. Original profiles and
source/build were unchanged, no native exception or remaining game process
was recorded, and the private package was restored. The memory observer
exited1; the old aggregate evaluator also failed its different lifecycle
expectations. Concrete connection/ownership evidence does not establish full
arena combat, usable sustained walking, cross-PC Radmin or campaign support.

## Borrow the guest's existing native player context

The common guest context adapter now includes a lease for an existing native
BioPlayerSquad with an empty cached player. It invokes the same pinned normal
SetPlayerPawn script used by the created-squad path and requires a positive
native combat getter before admitting the registration. Collision handling
uses the registration/token from either path. The native actor, membership
and world slot remain engine-owned. This is current-world code shared by the
ordinary and protected entries, rather than another arena-specific workaround.

Cleanup uses the normal script for the real unaligned current-player field;
aligned cache restoration preserves native replacements. Local fixture work
exposed the alignment issue before any game launch. Native outputs rebuilt;
the new authored-memory lease fixture, ordinary runtime harness and collision
lifecycle fixture passed. No new live run was performed. Existing native-squad
compatibility, usable movement and guest firing are still unverified. See
[existing squad context](EXISTING-SQUAD-CONTEXT.md).

## Preserve the guest camera during native weapon draw/holster

Read-only review of the connected arena recording found actual grounded
guest movement before the short automation pulse stopped. CurrentCustomAction
changed from0 to12, then returned to0 about one second later. The camera arm
adapter restored its baseline during that interval and reapplied afterward.
This does not establish a persistent movement failure or its cause. Selected
shipped SFXGame defaults map action slot12 to DrawWeapon and11 to HolsterWeapon
for the stock Henchman/Player tables; the live instance table was not retained.

The shared camera adapter now has a separate retention policy for an already
applied lease during a verified native draw/holster action. It resolves the
current pawn's actual class/instance action tables and requires the exact
allowed class, matching pawn/controller links and no requested or active
action-owned camera. It keeps all existing scene, ownership, mode, permission,
camera target, frozen/root-motion and channel checks. Power actions and other
custom actions remain ineligible. Initial camera acquisition and the collision
adapter's camera predicate keep the original strict action policy.

This changes camera presentation only, with no action cancellation or movement
permission changes. Normal native outputs rebuilt. No new fixture or live game
execution was performed; actual retention and sustained movement remain
unverified. The native action object may carry different links or camera fields
than its stock defaults, in which case the candidate preserves the strict policy.

## Use already ready native squad contexts

The existing-squad path now also accepts both native player caches already
identifying the locally owned Ashley, after the same current-world/channel,
story/permission and actual combat-getter checks. It records a read-only
context lease, making the common collision registration/token interface
available without requiring a created squad or a SetPlayerPawn dispatch.

This case owns no cache writes. Scene, disable, ownership and world changes
end only the observation and never clear the game's preexisting player caches.
The empty-cache initialization case keeps its separate tracked cleanup policy.
Different native player contexts remain preserved. Native outputs rebuilt;
new authored-memory fixture cases are prepared, with execution deferred.
No game or new fixture was run. Campaign travel and usable combat remain
unverified; this source change removes an unnecessary created-context dependency.

## Explicit ordinary guest rejoin request

The ordinary entry now exports an explicit rejoin request after an earlier
join attempt. Callers enqueue intent only; Tick verifies standalone ownership,
native travel absence, story readiness and previous adapter cleanup before
rearming the original join implementation. The normal stable-player window is
reset and must pass again. An existing network world is refused and preserved.
The request expires after30 seconds without eligible standalone cleanup.

No automatic reconnect, forced disconnect, endpoint change, fault clearing,
mission selection or engine package-validation change is introduced. There is
no player menu wiring yet. Request-policy fixture cases were prepared; execution
is deferred. Native outputs rebuilt. No game or new fixture was run. Recovery
after a real campaign disconnect remains unverified.

## Keep retry cleanup valid until dispatch

Source review found that rejoin cleanup was checked when rearming, while the
ordinary stable-player admission window could continue afterward without
checking it again. A Tick-owned retry flag now retains that requirement until
the final console admission. It is retired on dispatch or an already networked
world; no adapter, session fault or native network state is reset.

The ordinary join path also clears its readiness samples on observed early
scene/travel, disabled or invalid-context returns. Previously these early
returns could skip the readiness observer's reset path. The existing250ms
sampling cadence is unchanged, so this is not a continuous scene observation.
Native outputs rebuilt; no game or fixture was executed. The delegated Flash
review timed out and supplied no findings; these changes come from local source
inspection, not independent review or runtime evidence.

## Prepare ordinary host/guest session settings

A pure session-plan model and command-line preparation tool now accept the
ordinary host/guest roles and explicit local/peer endpoints. They reuse the
existing endpoint validation, produce matching native bind/port arguments and
runtime environment settings, and support the normal listen/join or existing
session attachment modes. The child-environment builder removes inherited
probe switches and stale opposite-role/startup settings without mutating the
parent environment. JSON output is private and created without overwriting.

This is configuration preparation, not a new game launcher or working player
installation. No socket, game, process attachment, registry or game-file write
is performed by the planner. Sources were syntax-parsed only; behavior tests
and integrated launch verification remain deferred. See [session setup](SESSION-SETUP.md).

## Establish the native guest power-order ABI

Read-only extraction of the owned SFXGame declaration confirmed the native
SquadOrderUsePower flags, native index and nine parameter fields. The authored
call contract now pins its four-byte-packed60-byte layout and validates the
reflected method/entry and each field. This resolves an important layout
ambiguity before any dispatch: its desired-target pointer is at unaligned0x24.
Normal native outputs rebuilt; input package/library hashes stayed unchanged.

No game, fixture or power order was run. The new header is a compiled contract,
not an implemented power binding or successful cast. Dynamic power selection,
action-instance controller ownership and native target/cooldown acceptance
remain the next prerequisites. See [power input research](POWER-INPUT-RESEARCH.md).

## Queue ordinary guest power requests against current ownership

The ordinary runtime now provides an explicit scoped power-index request API.
Tick resolves the possessed pawn's current native manager/list, binds input to
object identities and power names, and dispatches the verified normal native
own-pawn tactical order after final ownership/scene/combat and layout checks.
Changing the catalog or control scope cancels pending input. A different caster
or caller-supplied target cannot be submitted through this API.

Unlearned rank, duplicate native power name and stale cached player/AI context
are rejected. Existing native gameplay restrictions and target selection remain;
no action cache, cooldown, rank or RPC is patched. A boolean native acceptance is
reported separately from a cast/effect verdict. Keyboard/menu bindings and actual
gameplay remain unfinished. Normal native outputs rebuilt; no game, fixture or
power order was executed. See [power input research](POWER-INPUT-RESEARCH.md).

## Separate manual session control surface

The ordinary runtime now has an opt-in pointer-free local IPC panel endpoint,
and a separate Windows/Tk client presents Listen, Join, Retry, enable/disable
and the current scoped power list. The client never launches or injects into a
game. It retains a query-only process identity, rejects stale snapshots and
submits only bounded whitelisted requests. Native admission and game calls
remain on Tick. Power and connection request acceptance stay distinct from
actual gameplay success.

The session planner can include the panel opt-in flag. Normal native outputs
compiled and the Python sources syntax-parsed; no GUI, IPC fixture, game or
power request was executed. Actual panel rendering, handshake, native command
acceptance and gameplay remain unverified. See [session panel](SESSION-PANEL.md).

## Expire delayed panel delivery

The local panel mailbox now includes a producer GetTickCount timestamp. Native
Tick refuses delivery older than2 seconds before issuing any runtime command;
the UI reports expiry and requires a fresh explicit request. Future-looking
timestamps are also refused by the unsigned bounded age comparison. Already
accepted ordinary join/listen waiting is a different stage and is unchanged.

The protocol is now v2/5212 bytes on both sides; old versions are rejected.
Normal native outputs rebuilt and Python syntax parsed. No game, GUI, IPC test
or timestamp fixture was executed. Real delivery/expiry behavior remains pending.

## Retire positively disposed old-world companion leases

The common core can now retire an old lease without faulting solely because
its native world and companion were positively removed from the object registry.
It requires a verified different current host world/Shepard/squad and either a
retired guest controller or a native peer now belonging to the new world without
referencing the old companion. The world chain and registry evidence are checked
twice. A rename, unreadable pointer or expired timer is not disposal evidence.

This changes bookkeeping only and never clears an existing fault or pending
disconnect cleanup. It performs no old-world possession and claims no AI return.
Ordinary peer/AI/story admission still governs reacquisition. Other incomplete
or retained-world transitions keep conservative behavior. Native outputs rebuilt;
classifier fixture cases were prepared but not executed. No game or mission
transition was run. See [mission transition](MISSION-TRANSITION.md).

## Suspend old-world calls during incomplete native travel

Host acquisition now captures the native GWorld root. If that identity changes
or becomes temporarily unavailable, the core holds the old lease without making
old-world possession calls while waiting for positive disposal/new-world
verification. A120-second Tick-observed monotonic deadline latches a persistent
fault instead of guessing recovery. Returning to the exact original root can
permit normal cleanup, but never clears a fault which already occurred.

The panel now separates transition waiting and fault from actual companion
control; it no longer treats a tracked host lease alone as reciprocal ownership.
Its v3 frame/layout is updated on both sides. Native outputs rebuilt and panel
Python syntax parsed; no game, panel, transition or fixture was executed.
Actual loading-time suitability and native travel remain unverified.

## Request a listener explicitly in a new mission

The ordinary runtime/panel now has a separate new-mission Listen request. It
requires a previous verified runtime-owned listener and positive registry
retirement of both that old world and driver, then waits for clear standalone
control with completed lease/capacity cleanup. It binds the new request to the
current root identity and rechecks old disposal/cleanup before the ordinary
native Listen call. An existing network world or changed target is preserved.
It does not retry failed native initialization in the same world or reset faults.

Capacity retirement now checks a verified changed world before the normal
enabled-listener early return. Unknown loading context retains the lease rather
than being treated as disposal. The panel exposes the manual new-mission action
and its waiting/refusal/expiry statuses. Normal native outputs rebuilt and Python
syntax parsed; no game, panel, native startup or fixture was executed. Actual
mission/network continuity and guest reconnection remain unverified.

## Cancel connection intents before native dispatch

The ordinary entry and panel now expose cancellation of pending mod connection
intents. Tick processes it before Join/Listen/retry startup. It clears only the
waiting request flags/readiness and preserves consumed-attempt history, existing
native travel/listeners/peers and session faults. A cancellation arriving after
native dispatch does not masquerade as a disconnect or driver teardown.

Native outputs rebuilt and panel Python syntax parsed. No game, panel, fixture
or cancellation behavior was executed. Concurrent request behavior and actual
native continuity remain deferred.

## Place guest power dispatch after context adapters

The ordinary guest frame now processes the power queue after collision repair,
and rereads its session/scene context immediately beforehand. Collision repair
can invoke native script; a power order can start an action or alter ownership
or world state. Keeping the order last avoids handing its pre-call context to
another adapter. Existing power ownership, catalog and native ABI checks remain.

All three native outputs rebuilt. No game, panel, power order or fixture was
executed. This is an ordering correction, not evidence of a working native cast
or completed campaign co-op.

## Expire delayed guest power requests

Power requests now retain a64-bit monotonic enqueue timestamp and are rejected
after2000ms both at queue consumption and immediately before native dispatch.
This prevents a delayed Tick from executing an old request simply because its
catalog scope still matches. Queue publication/consumption use a nonblocking
SRW lock for timestamp consistency; neither side waits or holds the lock across
engine reads/calls. Atomic pending/status observations and scope cancellation
remain independent of the timestamp. Expired requests report rejected status
with `power-request-expired` and are not automatically retried.

All three native outputs rebuilt. No game, panel, request fixture, concurrency
test or native cast was executed. Expiry and concurrent cancellation behavior
still require deferred execution checks.

## Add optional foreground guest power slot input

The ordinary guest runtime now has a default-off Ctrl+Alt+1..9 slot-action
adapter. It samples real held-key state on pre-Tick without hooks, registered
hotkeys or synthetic input. A request requires the game's own foreground PID,
consecutive ready samples in one catalog scope and a new single-chord edge.
Scene/focus/control/catalog changes and Tick gaps above500ms baseline input
again. Held keys do not autorepeat or queue across those boundaries.

Slot actions use the existing scoped power queue, including busy refusal,
expiry and every native control/ABI/rank check. Slots follow the first nine
current native catalog entries; passive or unavailable powers can be refused.
There is no persistent power-name assignment or rebind UI yet. The planner
emits the opt-in flag only for explicit guest `--power-hotkeys` plans and
preserves it through the existing sanitized child-environment path.

Flash's read-only integration review identified inherited-flag stripping and
early-return baseline risks; explicit planner support and unavailable pre-Tick
sampling address those source-level concerns. All three native outputs rebuilt,
and planner Python sources syntax parsed. No game, panel, input/fixture test,
focus switch or native power cast was executed. Keyboard compatibility and
authoritative effects remain unverified.

## Retain power shortcut names across catalog changes

The optional input adapter now fills empty slots from unique complete native
FNames, then retains occupied assignments for the runtime process lifetime.
Catalog reordering resolves each assigned name to its current index. Missing
or duplicate names remain unavailable; a missing power never causes its slot
to select another entry. Later catalogs may fill previously empty slots.
Only name values are retained, without pawn/power/world pointers. Assignments
are not persisted across process restarts and cannot yet be edited manually.

The panel v4 frame exposes the opt-in flag and nine current slot indices,
using -1 for unavailable slots. List rows display their actual Ctrl+Alt shortcut
independently of row number. Both native/Python sizes and version checks were
updated; Flash's read-only protocol inventory was checked against the diff.

All three native outputs rebuilt and panel Python syntax parsed. No game,
panel, IPC/fixture test, reordered catalog, duplicate/removed power or actual
mission transition was executed. The full campaign and native effects remain
unverified; v3 and v4 panel/runtime components must not be mixed.

## Schedule an optional host listener after verified mission disposal

Host plans can now opt into `--auto-relisten`, emitting the default-off
`ME3_SQUAD_COOP_AUTO_RELISTEN=1` policy. It schedules the existing checked
relisten request only after a previous verified runtime listener's world and
driver registry slots positively show disposal, completed session/capacity
cleanup and a clear standalone new mission with no pending native travel.
The observed target root remains pinned until the ordinary relisten path arms.
Changed/unknown targets refuse the intent; all final startup gates remain.

Each successful listener permits one subsequent automatic intent, preventing
failed attempts from cycling between temporary roots. A newly successful
listener supplies the next origin. No failed native initialization retry,
guest reconnect, forced travel, driver teardown or fault clearing is added.
Cancel pending connection suspends this automation for the entire remaining
process lifetime, even if no request was pending; explicit requests still work.
The panel helper and setup/transition documentation describe that behavior.

Flash's read-only safeguard review was checked against the implementation.
All three native outputs rebuilt and planner/panel sources syntax parsed.
No game, panel, listener, cancellation, fixture or mission transition was run.
Actual multi-mission connection continuity and guest rejoining remain unverified.

## Add optional one-shot guest recovery after native disposal

Guest plans can opt into `--auto-rejoin`, emitting the default-off
`ME3_SQUAD_COOP_AUTO_REJOIN=1` policy. Before scheduling anything it must observe
a current client driver/server connection/channel with reciprocal registered
Ashley ownership and record the validated root/driver origin. A previous join
dispatch or net mode alone is insufficient. After both origin objects have
twice-read positive registry disposal evidence, a different clean standalone
player/story context can receive one request through the existing retry path.

Automatic intents pin that standalone target through retry readiness and final
join dispatch. Context changes or lost disposal evidence refuse them. Terminal
refusals, cancellation, existing-network preservation and native dispatch clear
the pin. A newly observed owned client context is required for another automatic
intent; initial failures and failed recovery requests do not loop. Host readiness
is not probed, and no disconnect, map load, driver teardown or fault reset is
added. Cancel suspends both connection schedulers until process exit while
manual requests remain available.

Flash's terminal-path inventory was checked against the diff. An initial build
exposed an ordinary-only helper dependency in the common bootstrap; the client
scope observation now uses shared pinned helpers and explicit reciprocal native
driver/connection/channel checks. All three native outputs rebuilt; planner and
panel Python syntax parsed. No game, panel, request fixture, disconnect or
reconnect was executed. Actual mission/host timing and full campaign continuity
remain unverified.

## Add optional host readiness synchronization for guest recovery

Both session roles can opt into `--ready-channel`, emitting the default-off
`ME3_SQUAD_COOP_READY_CHANNEL=1` flag. The ordinary entry owns an independent
nonblocking UDP channel on the exact configured interface: host game-port+1,
guest ephemeral. Port65535 is refused for this option. No native network state,
story data, engine command or object pointer is sent through the channel.

The host advertises readiness only for its current verified runtime listener
and ordinary Shepard control without transition/fault. A fixed24-byte versioned
query/reply echoes a random16-byte nonce. Guest replies require the configured
host endpoint, current request nonce/local context and request age<=1000ms.
Queries are limited to one per500ms; receives to four datagrams per Tick.
Context replacement/cancel/disable invalidates cached readiness. Missing peers,
UDP resets and oversized traffic cannot turn into readiness or a retry loop.

With the flag enabled, automatic guest recovery waits for a correlated ready
reply before consuming its one-shot intent and again before native dispatch.
The existing stable-player and native Join/package/ownership checks remain
authoritative. Manual Join/Retry is independent. Fatal local channel failures
leave automatic recovery waiting and preserve manual requests. The protected
probe does not open a side-channel socket. See [readiness channel](READY-CHANNEL.md).

The delegated implementation timed out without writing files; the transport
and integration were completed and reviewed locally. All three native outputs
rebuilt, and planner/build Python sources syntax parsed. No game, socket, packet,
panel, fixture or reconnect test was executed. Reachability, packet loss, timing,
cleanup and actual cross-PC campaign continuity remain unverified.

## Show recovery/readiness state and preserve the full reply window

The local panel v5 frame adds bounded recovery flags and readiness states. It
now distinguishes Manual, Automatic and process-lifetime Paused recovery, plus
host not ready/ready, guest idle/waiting/replied-ready, channel unavailable and
disabled states. Both native/Python frame size/version checks and status
validation were updated. These sampled observations do not imply a native
connection, successful cast or campaign progression.

Read-only source review confirmed that the transport rotated an unanswered
nonce at500ms despite advertising a1000ms acceptance window. It now retains
that pending nonce through the entire window, with the send rate still bounded
and context invalidation unchanged. Host readiness also requires the current
root's driver to equal the captured verified listener driver, rather than
accepting a different current driver merely because it passes endpoint checks.

All three native outputs rebuilt and panel Python syntax parsed. No game,
panel, socket, delayed packet, IPC or recovery fixture was executed. Actual
high-latency replies, panel rendering and full campaign continuity remain
unverified.
