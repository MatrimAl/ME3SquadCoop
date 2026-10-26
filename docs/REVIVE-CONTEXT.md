# Authored campaign revive context

`scripts/SFXSquadCoop_ReviveContext.uc` is an owner-authored shared component
for the pending campaign revive action. It compiles against existing LE3 game
types, without copying the retained LEMP class or assets. It is not installed,
constructed by the runtime, assigned to action5 or called by a gameplay hook.

The component captures a candidate action, distinct party-pawn reviver/target,
their current controllers and shared world. It refuses an already reserved
target or an action whose cached pawn/player-controller/AI links do not match
the current control context. SameActors rechecks reciprocal possession, world
and deletion state; it does not repair stale action fields or ownership.

Eligibility requires an ordinary unpaused world/current game mode, a living
reviver outside ragdoll/recovery and a downed/nondead target within the target's
native revive range. It also requires BioWorldInfo and rejects active or
activating conversations through the native conversation manager. Continuing additionally requires current action5, its
exact instance in the pawn's action array, synchronized target and no firing.
Authority uses the supported native role value3 through explicit enum-to-int
conversion. No role value is written.

BeginDuration starts once while the current action and target reservation
qualify. It captures the reviver's current native fTimeToRevive and the world's
TimeSeconds deadline. Nonpositive/nonfinite durations, nonfinite clocks and
nonadvancing/nonfinite deadlines refuse; no arbitrary duration cap or hardcoded
campaign revive time replaces native configuration. No Actor timer is created.

DurationComplete is the owning action's required progress poll. It cancels
permanently for that captured attempt when control/target/scene/firing checks
fail or the native world clock becomes invalid/backward. A later clear frame
does not restart the attempt. Cancel explicitly latches that state. The
caller must poll each action update: polling only once at the deadline cannot
observe an intervening transient cancellation condition. Movement/damage
cancellation remains a responsibility of the owning action.

DispatchRevive requires the completed native-world duration, current context,
authority on both pawns, the target's reservation flag and an unused local
dispatch record. It consumes that record
before invoking existing PlayerController.Revive. A true return means dispatch
only; the native method has no returned success result. RecoveryObserved is
separate and requires the same actors/world/control context, authority and a
target whose native dead/downed/state predicates have cleared. It does not
prove sustained movement, correct camera, collision or equipment after revival.

The future owning action must still provide animation, per-update progress polling, movement/
damage cancellation, reservation ownership, timer baseline/cleanup and native
replicated-target handling. In particular this component does not prove that
a reservation flag was set by its own action; that ownership belongs to the
pending action lease. It must not be activated merely because the flag is set.
Host story/session admission remains an external prerequisite; these checks
do not authorize interaction during conversations or acquire guest control.

The source is not a replacement for a missing action class or for its native
CanDoCustomAction checks. A caller must invoke Capture only when starting a
fresh action and must not Reset/rearm an in-progress attempt to replay a
partial dispatch. The context performs no health, timer, collision, profile,
GRI, ownership or action-array writes.

## Compile evidence

The authored context compiled with zero compiler errors/warnings and saved to
a fresh private package using the new build command. The receipt status was
`compiled-not-installed`; tracked tool/Oodle/Core/Engine/SFXGame/source hashes
were unchanged. The saved package digest was
`8c081ca40ab6682cda68f740aa82e746b5e05885420cef148c7a8e8fe42c671f`
for the duration/conversation revision.
Failed earlier compiler outputs remain separate private directories and are
not candidates. No native-source change required rebuilding the ASIs.

No game, behavioral test, fixture, package reload or runtime activation ran.
Compiler success does not verify these predicates on live objects, event
reentrancy, action replication or revival. Full campaign revival remains
incomplete.
