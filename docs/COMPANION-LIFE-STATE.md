# Companion downed/dead admission boundary

The retained LE3 SDK declares BioPawn.bIsDowned at +0xe20/mask0x100 and
BioPawn.bIsDead at +0xe20/mask0x200. These replicated state bits are distinct
from UObject/Actor destruction, custom actions and physics mode.

The shared companion readiness predicate now refuses new control acquisition
while either bit is set. The ordinary guest power catalog also refuses either
state, invalidating the published scope and pending request at the next Tick.
The same catalog predicate runs during final pre-dispatch validation, including
after the native combat getter. Existing input baselining prevents a held power
chord from becoming a new order when a subsequent available scope is published.

The shared host equipment handoff also refuses downed/dead state at entry and
again immediately before ClientGivenTo dispatch. The final check occurs before
consuming its once-per-ownership-epoch attempt. A refused handoff therefore
remains eligible for normal reconsideration after native recovery; a previously
dispatched handoff is never replayed solely because the pawn recovered. This
does not cancel an RPC which was already dispatched or replace native inventory
replication. Both ordinary and protected entries share this predicate.

Guest player-context creation/adoption now refuses downed/dead state when
acquiring a new registration. Existing registrations retain their existing
monitoring and cleanup policy; the life bits alone do not trigger their
destruction. The same acquisition predicate is checked before native spawn
and SetPlayerPawn dispatch.

The optional guest collision repair also refuses both life bits, including
its final pre-dispatch ordinary-state check and post-call world-collision
write guard. While tracking an existing identity, a downed/dead or unreadable
life sample clears pending/released collision evidence and skips repair.
The captured healthy baseline remains, but a fresh qualifying release must
be observed after recovery. Native death collision therefore cannot itself
arm a later scene-return repair. No death flags, revival method, camera policy
or native ownership are changed by this boundary.

The local panel distinguishes a currently owned downed companion (control
state5), dead companion (6), and unavailable life-state read (7) from ordinary
control (1). Death takes precedence if both native bits are set. Scene, disabled,
travel and fault states retain their existing priority. The sampled pawn is
the host's current remote-owned companion or the guest's local-owned companion,
with identity/world/reciprocal ownership checked around the read. Released or
stale leases are not used to infer a dead companion. Ownership remains a
separate field: native possession can survive while the pawn is downed.
The panel power button requires ordinary control, while native power validation
remains authoritative. This adds enum values within the unchanged v5 layout;
older panels display Unknown for those values. No panel/death test was run.

No health, downed/dead flag, native revival function or ownership is written by
these checks. Native recovery can make admission eligible again, but every
ordinary AI, action, physics, proximity, story and ownership gate still applies.
This does not prove that the engine restores the required AI/controller context.

The active host lease currently treats unexpected native ownership changes as
ownership loss and can latch a session fault. Its release path attempts native
AI possession and requires reciprocal restoration. Correct death/revival
handling therefore needs the actual Died/Resurrect method bodies and retained
ownership observations before changing that policy. A dead bit alone does not
authorize forced AI possession or clearing a fault.

The native package-body extraction was not executed: automatic approval review
could not complete due to a usage limit. No extracted implementation is claimed.
The narrower admission/power changes use the inspected SDK declaration and
existing runtime read paths. No game, death/revival fixture or power order was
executed; full combat and revival continuity remain unverified.

## Subsequent owned script extraction and camera boundary

The earlier extraction limit was subsequently resolved by a bounded read-only
extraction from the owned SFXGame and Engine packages. All28 selected functions
decoded with matching serialized/expanded lengths; all88 explicit branch and
iterator destinations aligned with expression boundaries. The initially
unsupported dynamic-array iterator was added from the already retained decoder
format before claiming complete decoding. Package/compression-library hashes
were unchanged. These checks establish selected decode coverage, not native
callee semantics or actual revival behavior.

The selected path distinguishes incapacitation from permanent death:

| Script path | Observed responsibility |
| --- | --- |
| BioPawn.Died | Selects Downed versus base Pawn.Died according to native/game death policy; the nonpermanent branch enters Downed and returns false |
| BioPawn.Downed.BeginState/EndState | Sets/clears bIsDowned and changes mesh collision-channel/ragdoll-related handling |
| SFXPawn_PlayerParty.Downed.BeginState | Enables use under its native death/damage gates and invokes SetCollision(false,false) |
| SFXPawn_Henchman.Downed.BeginState/EndState | Changes marker modules around inherited downed handling |
| SFXPawn_Henchman.Resurrect | Requires IsDead, conditionally changes physics, then delegates to party/BioPawn resurrection |
| BioPawn.Resurrect | Restores health/native damage state, clears bIsDead and chooses ragdoll/state transitions |
| BioPlayerController.Revive | Invokes party resurrection, with separate multiplayer health/vocalization policy |
| Engine.Controller.PawnDied | Calls the pawn's UnPossessed, clears Controller.Pawn and chooses player-dead/nonplayer cleanup behavior |

The inspected downed BeginState/EndState bodies contain no explicit possession
handoff. This does not prove that inherited/native callbacks preserve ownership
in every real downed transition. Conversely, permanent-death controller cleanup
cannot be handled by merely ignoring a missing reciprocal pawn pointer or
clearing the session fault. Native Game.Killed, ragdoll/action transitions,
Ashley-specific overrides and runtime observations remain separate work.

The guest shoulder-camera policy now refuses bIsDowned/bIsDead or unreadable
life state even when possession, mode0 and weapon-action retention otherwise
look valid. A held scale lease follows its existing compare-before-restore
cleanup, preserving a native replacement rather than reasserting normal
shoulder presentation during incapacitation. The same predicate runs for new
acquisition and final pre-write validation. After native recovery, ordinary
camera acquisition still needs every existing ownership/input/mode/channel
and camera condition; recovery does not force an immediate camera write.

No health/life/physics flag, collision, revival method, possession or session
fault behavior was changed by this camera boundary. Three native targets
compiled and fingerprints matched; no game or behavioral tests ran. Actual
downed camera appearance, restoration and usable post-revive control remain
unverified.

The subsequent [ordinary interaction trace](GUEST-INTERACTION-RESEARCH.md)
connects normal selection/Used through CanUse and the native authority/client
split to party StartRevive. It does not require a Shepard-class user in the
selected scripts. Actual guest selection, action5 and authoritative revival
completion remain outside that offline evidence.
