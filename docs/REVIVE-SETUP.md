# Coordinated revive preparation and retirement

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
instancing path. This coordinator does not select the existing basegame revive
class, whose lifecycle differs from the authored target-state bookkeeping.

The preparing/busy guards span native calls so a reentrant Prepare/Retire
cannot replace the in-progress component records. Failed preparation records
LastFailure, disables readiness and attempts ordered retirement. A false
return may still have HasPendingWork=true; the caller must retain the object
and poll Retire rather than discard it or prepare a different pawn.

Ready rechecks ordinary-world/action binding and exact sequence lookup. If
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
The runtime must stop admitting new native interactions before retirement and
coordinate the peers; this source object alone cannot enforce those policies.

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
`a64c88265771fd13b0c5a952e86ac950b6ba9d56212870cfa77fa50f23a06cb6`.
No game, fixture, package reload, deployment or behavioral tests ran. See
[binding](REVIVE-BINDING.md), [animation](REVIVE-ANIMATION.md) and
[action](REVIVE-ACTION.md) for the individual evidence boundaries.
