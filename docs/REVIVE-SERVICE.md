# Local world service for revive setup

The authored SFXSquadCoop_ReviveService Actor holds Setup as a reflected
reference. SFXSquadCoop_ReviveFactory.CreateFor supplies a script factory for
the [native preparation candidate](REVIVE-NATIVE-BRIDGE.md). Its explicit,
default-off guest path can call this factory and poll/retire the service;
actual runtime dispatch has not been observed. Normal
Actor.Spawn is intended to give this local state a world-owned lifetime;
actual spawning, GC/rooting, destruction and travel remain unverified.

CreateFor accepts an admitted party pawn, local controller, optional loaded
animation set, positive scope token and explicit presentation mode. It checks
the current shared world, spawns with that controller as Owner and calls
Initialize. The returned actor may own failed-preparation cleanup; a pointer
alone is not preparation success. The native caller must retain/status/retire
the same lifetime rather than continually spawn replacements on status0.

The actor is hidden, noncolliding, RemoteRole=None, with no movement replication,
autonomous Tick or lifespan timeout. It retains one pawn/controller/token and
Setup. It does not load assets, send input/network messages or authorize a
co-op session. Runtime must explicitly poll and retire before control handoff.

Setup.Prepare now accepts optional bDeferAdmission. The service always uses
it, leaving the authored gate closed. Prepared checks local resources without
the gate; Ready also requires admission open. Activate is separate and only
opens a valid ordinary idle binding. The runtime must first establish peer
agreement; the scope token is a local discriminator, not authentication.

Status codes are0 unavailable/busy/uninitialized/invalid-token or lost readiness,
1 prepared/closed,2 locally active,3 pending retirement,4 retired. These are
sampled local states, not authorization for a later operation. Retire latches
retirement, delegates ordered cleanup and retains Setup until complete.
DestroyRetired requests normal destruction only after matching-token retirement
cleared Setup and the world is current. Pending cleanup is never forcibly
discarded by a timer or Destroy override. External destruction and old-world
disposal still require native lifecycle evidence.

RetireForHandoff now distinguishes a control-release boundary from full
retirement. It first calls Retire for the same token. If that remains pending,
it can return true only after the action binding has been released and the
animation lease has read back zero native references after its one nonforced
unregister request (or observed removal). Status remains 3 until final removal
is observed; the caller must retain/poll the same service, forbid replacement
setup and wait for Status 4 before DestroyRetired. This method does not perform
the control handoff or establish when a scene will begin.

After an unregister request, animation retirement only reads the captured
pawn/world/mesh/set lifetime. A controller/role or menu-state change no longer
prevents observing actual removal. Registration and the unregister request
itself still require the captured ordinary idle control context. A changed
mesh/world or shared reference count does not grant handoff permission. If a
scene already prevents that initial request, retirement can still remain
pending: runtime must arrange the pre-handoff call, not bypass these guards.

The factory is separate because the supplied compiler could not resolve a new
class's self-type/class literal while compiling its definition. Service compiles
first and the factory uses that registered class normally, returning Actor.
No existing game class/package was changed.

Seven classes compiled without errors/warnings and with unchanged tracked
inputs, in order: context, action, binding, animation, setup, service, factory.
Package SHA-256:
`3013011e50500da6cb5af656ab11ab2894c0adf4dba8dccf22c990ea199e4d0b`.
This is the script-only compilation. The separate
[owner-derived asset package](REVIVE-ASSET-PACKAGE.md) has a different digest
and an explicit factory-default animation anchor; it remains private.
Earlier failed compiler directories remain unusable. The native candidate
implements loaded class/default/function resolution and reflected argument
packing, but actual ABI acceptance/factory invocation remain unobserved.
Per-peer admission and production retirement ordering are unfinished. No game,
fixture, install, mount, service invocation or behavioral tests ran for this
revision. Uninitialized spawned actors now retain a retired token through
RetireUninitialized, permitting normal destruction without creating Setup.
