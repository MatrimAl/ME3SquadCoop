# Existing native guest squad context

Status: implemented candidate; live verification pending.

The connected arena run retained a native BioPlayerSquad on the guest with
CachedPlayerPawn null and a false native IsCombatEnabled result. The previous
adapter preserved every existing squad and only initialized a newly spawned
local squad when the world slot was empty. That left this legitimate existing
context without the controlled guest pawn in its local player caches.

The shared adapter can now borrow the existing squad through the verified
BioPlayerSquad.SetPlayerPawn script. Admission requires the acknowledged local
Ashley ownership, a positive current actor channel, ordinary input/scene
permissions, current-world and level identities, and agreement between the
world's squad slot and Ashley's squad. The cached pointer must be empty and
the current-player pointer either empty or already Ashley. Existing different
player contexts remain preserved. Applying the script must produce matching
caches and a positive actual native combat getter before registration succeeds.

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

Unknown captured context retains pending cleanup without writing. Positively
replaced identities/worlds retire the old lease without old-world writes. A
failed native cleanup dispatch is bounded to eight calls at least one second
apart. Pending cleanup prevents another application in the same control scope.

The native outputs compile. The authored-memory existing-context fixture checks
application, scene release, partial/no-op dispatch, failed getter rollback,
native cache replacements, unknown/replaced worlds and cleanup bounds. Engine
discovery and SetPlayerPawn are explicit stand-ins in that fixture. The ordinary
runtime harness and collision lifecycle fixture also passed. These checks do
not prove this new path works with the live game's existing native squad.
Guest firing, sustained walking and campaign travel remain unverified.
