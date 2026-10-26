# Coordinated revive preparation and retirement

The [local world service](REVIVE-SERVICE.md) now retains this object and uses
deferred preparation plus separate activation for the future native bridge.
It is compiled but no runtime spawns or invokes it yet.

Retirement now clears the completed Binding reference before waiting on
animation removal. HandoffReady is separate from retirement completion: it
requires retirement latched, no active preparation/mutation, the binding gone,
and either no animation lease or that lease's confirmed unregister/readback
boundary. A caller must retain the coordinator until full Retire succeeds and
must not prepare a replacement while removal remains pending. See the
[service handoff contract](REVIVE-SERVICE.md).

A separate [script DLC candidate](SCRIPT-DLC-CANDIDATE.md) now prepares config/
Mount/TOC around the compiled classes; it remains uninstalled and does not
establish shared loading or invoke this coordinator.

`scripts/SFXSquadCoop_ReviveSetup.uc` joins the existing authored binding and
optional animation lease into one local lifecycle. It is compiled, not loaded
or called by runtime code. The owner runtime must retain the object across
updates, establish admitted per-peer pawn/session identity, supply an already
loaded animation set when needed, and retire before relinquishing control.

Prepare takes a party pawn, local admission controller, optional loaded set
and explicit simulated-presentation mode. Role/controller preflight precedes
animation changes. If the mesh already resolves CB_MP_Revive, it skips the
animation lease. Otherwise it registers the supplied set through that lease.
It then binds the authored SFXSquadCoop_ReviveAction through the native
instancing path. Its new instance starts with bAdmissionClosed=true, so native
CanDo calls during partial preparation refuse the authored action. Only after
the final context/animation/binding checks does OpenAdmission enable it, unless
optional bDeferAdmission is requested. That mode leaves admission closed until
explicit Activate. Prepared checks resources without gate state; Ready also
checks the gate. The service uses deferred mode for future peer coordination.
This coordinator does not select the existing basegame revive
class, whose lifecycle differs from the authored target-state bookkeeping.

The preparing/busy guards span native calls so a reentrant Prepare/Retire
cannot replace the in-progress component records. Failed preparation records
LastFailure, disables readiness and attempts ordered retirement. A false
return may still have HasPendingWork=true; the caller must retain the object
and poll Retire rather than discard it or prepare a different pawn.

Ready rechecks ordinary-world/action binding and exact sequence lookup. If
the authored admission gate is closed, readiness is false even if the class
and instance remain installed. If
this setup owns an animation registration, IsRegistered additionally requires
the same current context/mesh, a unique temporary record with positive count,
one mesh-set occurrence and the exact expected sequence. Another consumer
may raise the count; this does not prove exclusive ownership and retirement
still refuses to guess its reference away. Ready is local preparation, not
network agreement, native CanDo acceptance, animation playback or revival.

Retire clears readiness first, releases the action binding first and only
then releases the optional animation lease. A busy action, pending target
cleanup, changed ownership or native delayed removal retains the component
records and bRetiring. Completed cleanup drops the records and permits a new
preparation. It does not force Stop, restore a foreign slot, change controller
caches, force-remove an animation set, launch a game or set story authority.
Binding.Release now closes the exact authored instance's admission gate before
checking whether a running action prevents slot removal. It leaves that action
running its normal completion/interrupt/cleanup path; later retirement polls
do not reopen admission. Changed action/controller identity refuses the write.
The runtime must call retirement before handoff and coordinate the peers.
The local gate cannot prevent native requests already in flight or authorize
story/session membership, and it does not apply to the existing basegame class.

## Integration sequence still required

1. Load/resolve the shared authored package and approved asset dependency on
   both peers, without treating disk presence as engine loading.
2. Establish actual session membership, current pawn/control/world and the
   appropriate owned/simulated representation. Construct/retain one setup per
   admitted pawn lifetime; do not continuously recreate it on failure.
3. Call Prepare once for that lifetime, inspect its return and pending state.
   Publish usability only after the required peers have current readiness.
4. Keep observing readiness and session changes while preserving native
   action admission. A transient nonordinary scene is not a new preparation.
5. Close new interaction admission and call/poll Retire before control/world
   handoff. Preserve unresolved records. Do not claim cleanup merely because
   a script call returned or an actor became temporarily unavailable.

These are requirements for the pending runtime integration, not executed
behavior. Scene/handoff ordering, native animation removal, class mounting,
peer negotiation and actual control after recovery remain unverified.

All five authored classes compiled together without errors/warnings, with
unchanged tracked inputs. The private package digest is
`add7fe987215ccac548c087a2805841e115f54fae1b930b41815e5405471ea4a`
for the coordinated authored-admission revision.
No game, fixture, package reload, deployment or behavioral tests ran. See
[binding](REVIVE-BINDING.md), [animation](REVIVE-ANIMATION.md) and
[action](REVIVE-ACTION.md) for the individual evidence boundaries.
