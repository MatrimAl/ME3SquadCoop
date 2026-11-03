# Authoritative host revive preparation candidate

The default-off ordinary preparation path now has two independent host slots:
the actual host story pawn and the actual guest-controlled companion from the
captured session lease. Guest slot0 continues to represent the local owned
Ashley. This is preparation with admission closed; no activation, network
readiness agreement or successful revival is claimed.

Host candidates are not discovered by class/name scanning. Admission requires
an active nonreleasing/unfaulted session, the matching session epoch, preserved
story pawn/squad identities, live captured remote/controller/connection/pawn
handles and reciprocal possession. Each candidate must resolve a unique native
ActorChannel on that exact guest connection. The captured scope now includes
full controller/pawn/world/channel/connection/driver stamps and the channel
index. The scope is checked again immediately before factory dispatch.

Each slot retains its own service, token, factory attempt and pending cleanup.
A change to the channel lifetime invalidates that scope even if pawn pointers
and the session epoch still look unchanged. Replacement requires actual service
disposal and a newly admitted scope. Native dispatch has a shared reentrancy
guard across both slots, while per-slot state is passed explicitly rather than
selected through a mutable global current-slot pointer. Local context and
transport/world scope refresh after native calls before the next slot.

## Control-release boundary

The session core now has an optional before-control-release callback. The
ordinary host installs it only with the explicit preparation opt-in; the
protected probe leaves it NULL. It runs under session_releasing before native
UnPossess/AI restoration, latching retirement and forcing a cleanup poll on
both host slots. Afterward the core reads the local context again and applies
its native-world travel guard before touching captured actors.

This callback does not wait indefinitely, force-remove animations or override
story transitions. A scene may already prevent an initial unregister request;
such cleanup remains retained. Full production action interruption/retirement
ordering is not solved by this preparation-only hook. Once unregister has
been issued, the script lease can continue its read-only removal observation
after ownership changes. Destruction still requires a fresh retired status.

## Remaining peer integration

The existing runtime readiness UDP channel reports advisory host availability
for automatic reconnect. Its reply does not bind revive package identity,
native actor-channel epochs or four required preparation lifetimes. It was not
repurposed to authorize revive. The missing guest simulated-host presentation
service, matching per-peer package/ownership agreement, admission opening and
actual mutual revive/downed behavior remain unfinished.

All three native targets built from this source; the authored seven-class
package did not change in this step. No game, fixture, behavior test, deployment
or host service invocation was performed. The existing private owner-asset
package remains a candidate; actual loading and skeletal/audio compatibility
are still unverified.
