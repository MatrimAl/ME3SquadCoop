# Authored campaign revive action

`scripts/SFXSquadCoop_ReviveAction.uc` is an authored action5 candidate extending
the existing `SFXCustomAction_SingleAnim`. It and the separate
[revive context](REVIVE-CONTEXT.md) compile together against LE3 dependencies.
The class is not installed, mounted, constructed by the runtime or assigned to
a pawn. It does not establish a working campaign revive feature.

## Action flow

Admission retains the inherited InternalCanDoCustomAction checks. Presentation
requires distinct party pawns in the current native world and no cached AI
controller; a nonnull cached player controller must match reciprocal possession.
Authority additionally captures the context's controller, target, life, range,
ordinary-world and conversation checks. Simulated presentation does not acquire
the authority-side target reservation or dispatch resurrection.

Start requires this exact action instance in current slot5. Authority captures
the target, acquires its scoped bBeingRevived reservation, captures/disables
existing target usage and captures/pauses its existing native death timer
before starting the inherited animation
action. An absent timer is not created; an already paused timer is not owned.
The candidate names CB_MP_Revive but supplies
no animation asset. Two selected LEMP human-player carriers now have a
serialized mesh/set/sequence chain for that name; Ashley's effective animation
lookup remains unverified. See [asset and basegame class evidence](REVIVE-ANIMATION.md).
It requires one animation name and a successful native
GetAnimLengthAndPos query with a positive finite length after inherited start.
That query is a refusal condition, not proof that the required animation is
available or visibly playing on either peer in a campaign mission.

The native reviver fTimeToRevive drives the context deadline. TickCustomAction
polls progress, preserving the native CheckMoving predicate and the context's
firing/life/range/control/conversation cancellation. No polling timer is added.
Only authority dispatches the existing PlayerController.Revive. Dispatch is
consumed once before the native call and remains distinct from observing
cleared dead/downed/state predicates. No direct health write or multiplayer
GRI/profile switch is introduced.

RecoveryWait defaults to a two-second observation interval after dispatch; it
does not replace fTimeToRevive or imply revival success. Nonpositive/nonfinite
waits, invalid deadlines and nonfinite/backward recovery clocks refuse or
interrupt. The interval is initialized before dispatch so a synchronous native
callback cannot observe an uninitialized deadline. Animation-end notification
does not independently declare completion. Actual tick, notification and
completion behavior still require engine evidence.

An attempt serial distinguishes a later accepted start from an older returning
stack around inherited start/tick, animation lookup, movement cancellation and
native resurrection. Stopping blocks a reentrant start/stop and invokes the
inherited Stop at most once for a recorded base start. These guards express
source-level intent; compilation does not demonstrate native callback ordering.

Finish requests target-state release before native end/interrupt. Stop attempts
cancellation/release before inherited cleanup. Timer-pause cleanup precedes
usage/lens-state cleanup, then reservation release. A refused release sets
bTargetCleanupPending and blocks a fresh capture instead of discarding
ownership metadata. It does not clear a target after losing action/control
identity. Pending cleanup remains unresolved and may retain a target flag;
this is an activation blocker, not completed cleanup.

Replicate writes the synchronized target to ReplicatedCustomActionInfo.Target;
ClientDoCustomAction restores SyncPawn before inherited presentation. This is
an authored candidate for the native target path, not proof of working
replication or client animation. Session admission and guest assignment remain
external prerequisites for a future scoped installer.

## Remaining integration

| Requirement | Current boundary |
| --- | --- |
| Shared class availability and slot5 assignment | Neither peer loads or assigns this class yet; no per-mission edits are made |
| Animation availability | Located in two human-player carrier mesh/set chains; no scoped Ashley loading/assignment or playback evidence |
| Native target usage | Captured module/callback/field/lens baseline and scoped disable/restore; actual engine visibility, overlap and client presentation remain unverified |
| Downed death timer | Captured native binding/fields and conditional pause/resume; native timer/recovery behavior and identical replacement remain unverified |
| Cleanup after lost context | Pending metadata is retained; no foreign-context flag clearing or completed recovery |
| HUD and interruption coverage | No countdown HUD integration; damage and all native interruption paths remain unverified |
| Native end/stop ordering | Script stop path inspected; surrounding native state changes remain unresolved |
| Client and authority behavior | No engine load, tick, animation, replication or actual revival evidence |

## Static and compile evidence

The selected native-package script review decoded eight stop/animation
functions with matching serialized/VM lengths and22 aligned explicit branch
destinations. BioPawn.CustomActionEnded invokes the selected action's Stop when
started, while EndThisCustomAction delegates to native pawn.EndCustomAction.
SingleAnim start/stop own their animation/timeline/root-motion work and
optional OnTimeUp timer. This bounded review does not establish the surrounding
native end-state transition order or cleanup after travel/control changes.

Both authored classes compiled with zero errors/warnings and unchanged tracked
input hashes. The saved private package SHA-256 is
`d5098e166210004a03187a17b04df28829d86bfe70f953f34d2925a8644a2e7d`.
LEC accepts the external root-motion enums as
ERootMotionMode.RMM_Ignore and ERootMotionRotationMode.RMRM_Ignore; the body
stance animation names are an array. This is compilation/save evidence only.
No package reload, install, game, fixture or behavioral tests ran. Native
sources did not change and do not require an ASI rebuild for this candidate.
