# Existing native guest squad context

The protected research fire bot now resolves either a validated existing
squad lease or the original created-squad lease. The existing branch rechecks
the same local pawn, world, squad caches, channel and native combat getter;
the existing weapon and input guards remain in force. This is test-only code,
not an ordinary-entry fire automation. It compiled and the probe harness passed,
but neither branch was exercised in a game after this change; the harness
does not validate the new engine-owned squad branch.

Following that startup race, `4e4fcce` requires a sampled 1.5-second empty
squad window before creating a guest-local squad, resetting on ownership,
eligibility, squad presence or a sampling gap over one second. In one same-PC
run the replicated squad arrived and the existing-squad path registered it;
the prior replacement race did not recur in that run. This is sampled evidence,
not a guarantee that a later native squad cannot replace a created one. The
guest-action observer could not verify foreground window activation and exited
without sending input. The runner cleaned up successfully, but the observer's
exit was nonzero. Movement and firing therefore remain unverified.

A second ordinary pair from `2a80baf` again registered the existing squad and
observed the native connection interval. A foreground-thread attachment did
not make the guest window verifiably foreground, so the action observer sent
no input and exited nonzero. Its record says `not-ready`, but the process
trace identifies foreground activation as the actual failure. The game runner
completed, original profile and sandbox TOC were restored, and no game or
native exception remained. This is not an input or combat pass.

An ordinary pair from `f5f8f74` connected but reached a different startup
ordering: the world squad slot was empty, so the created-squad path briefly
registered a local squad and obtained a positive combat getter. Within one
sample the engine replaced the world slot. The old squad cleanup reported
`destroy_dispatched:false` and exhausted its bounded retries, leaving cleanup
pending for that process. The movement/fire observer waited specifically
for existing-squad registration and sent no input; its wait began during
preparation, so its timeout is not a movement or firing verdict. The sandbox
TOC and original profiles were restored, and no game remained. The exact
replacement actor and rejected destroy prerequisite remain to be established.

An opt-in ordinary guest candidate now admits the pinned SetPlayerPawn script
through the native role check only for the exact current squad and method on
the dispatching thread. The callspace detour forwards the original decision
for other calls; its temporary scope allows at most one matching skip result.
It is enabled only with `ME3_SQUAD_COOP_LOCAL_SQUAD_SET=1` and is not a
default-on or gameplay release. Its local fixture covers filtering, original
forwarding, one-shot behavior and other-thread isolation.
The guest session planner exposes this opt-in as `--local-squad-set`; host
plans reject it. Generating a plan does not launch the game or validate play.

In one ordinary same-PC run from `35ecadb`, the native skip was adapted and
SetPlayerPawn produced a registered guest squad context with a positive combat
getter. A later read-only snapshot found both player caches equal to the
owned Ashley and her squad pointer reciprocated. The connection stayed up for
the observed interval; original profiles and the sandbox TOC were restored,
all observers exited successfully and no native exception or game remained.
The guest process ended at its test deadline: in-game lease cleanup on an
ordinary disconnect was not observed. No movement, firing, powers, mission
travel or two-PC gameplay was verified by this run.

A further same-PC run from `ed0a665` enabled the guest's local control panel.
After acquisition, a disable request was acknowledged while still connected;
the local SetPlayerPawn cleanup dispatched and both squad player caches were
observed null, while Ashley retained her original squad link. The guest's
normal disconnect and mission travel remain unverified. The first observer
failed because Python requested unexported kernel32 interlocked symbols;
a separate local atomic helper enabled the successful retry. Thus the
coordinator's first-observer exit was nonzero even though the runner and
retry completed and sandbox/profile cleanup was clean.

Status: implemented candidate; live initialization did not verify in one
ordinary pair. In that run, method dispatch returned, admission and identity
checks passed, but `CachedPlayerPawn` remained different from the owned
Ashley; the combat getter was not reached. A later read-only observation in
the same session found the guest's current `BioPlayerSquad` at role1 with the
native Actor callspace target at RVA `0x70a250`; both player caches were empty.
Source-bound native analysis shows the script dispatcher consults that
callspace, whose role1 path can reject a function lacking flags `0x500`.
The pinned `SetPlayerPawn` flags are `0x20802`. This is a specific candidate
explanation of the no-op, not a captured callspace return or a proven fix.
Ordinary use of the current initializer must not be described as verified.

An opt-in, game-thread preflight in a later ordinary same-PC pair returned 1
for that role and function immediately before unchanged dispatch; the cached
player again remained absent. Native script dispatch treats a nonzero result
as the skip branch. The preflight is a separate native call, not a captured
return from the dispatcher, and is disabled by default. No gameplay or fix is
established by this observation.

The adapter now logs each reached post-dispatch verification independently;
`-1` indicates an earlier failure prevented a check. The authored-memory
fixture checks dispatch failure, missing cached-player write and false getter
separately. It uses a stand-in for the native dispatcher and cannot reproduce
the game's callspace decision.

The connected arena run retained a native BioPlayerSquad on the guest with
CachedPlayerPawn null and a false native IsCombatEnabled result. The previous
adapter preserved every existing squad and only initialized a newly spawned
local squad when the world slot was empty. That left this legitimate existing
context without the controlled guest pawn in its local player caches.

The shared adapter can use an existing native squad. Admission requires the acknowledged local
Ashley ownership, a positive current actor channel, ordinary input/scene
permissions, current-world and level identities, and agreement between the
world's squad slot and Ashley's squad. If both player caches already identify
the controlled Ashley, registration observes that ready context without
SetPlayerPawn or any cache writes. Otherwise the cached pointer must be empty
and the current-player pointer either empty or already Ashley; that branch
uses the verified BioPlayerSquad.SetPlayerPawn script. Existing different
player contexts remain preserved. Both paths require matching caches and a
positive actual native combat getter before registration succeeds.

The lease captures controller, pawn, player, world, level, squad, connection,
driver and channel identities. One application attempt is allowed per observed
control scope. Collision baseline/reacquisition uses the same registration
interface for borrowed and newly created contexts. This path is shared by the
ordinary runtime and protected probe and has no SPCer map-name dependency.

Scene, ownership and context changes release the lease. Clearing the unaligned
m_playerPawn field at2cc uses the same verified normal script on the game
thread, with both cache values checked immediately before dispatch. The aligned
CachedPlayerPawn field can independently be restored by compare-exchange when
the current-player value already equals its baseline or has a native replacement.
If the engine adopts the context by replacing its cached player, the adapter
preserves that context and retires its lease. It owns neither the native actor's
lifetime nor the world registration or squad membership.

A context which was already natively ready is released by ending the local
observation only. Its caches are not restored or cleared, even on disable,
ownership change, unknown context or mission travel: the mod wrote neither.
The common collision registration/token interface accepts both cases, avoiding
an unnecessary dependency on having spawned or initialized the native squad.

Unknown captured context retains pending cleanup without writing. Positively
replaced identities/worlds retire the old lease without old-world writes. A
failed native cleanup dispatch is bounded to eight calls at least one second
apart. Pending cleanup prevents another application in the same control scope.

The native outputs compile. Earlier execution of the authored-memory existing-context fixture covered
application, scene release, partial/no-op dispatch, failed getter rollback,
native cache replacements, unknown/replaced worlds and cleanup bounds. Engine
discovery and SetPlayerPawn are explicit stand-ins in that fixture. New cases
for already ready native contexts are prepared but have not been executed.
The earlier ordinary runtime harness and collision lifecycle fixture passed;
neither was rerun for this latest extension. These checks do not prove this
new path works with the live game's existing native squad.
Guest firing, sustained walking and campaign travel remain unverified.
