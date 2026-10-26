# Authored campaign revive context

`scripts/SFXSquadCoop_ReviveContext.uc` is an owner-authored shared component
for the [authored campaign revive action candidate](REVIVE-ACTION.md). It compiles against existing LE3 game
types, without copying the retained LEMP class or assets. It is not installed,
constructed by the runtime, assigned to action5 or called by a gameplay hook.

The component captures a candidate action, distinct party-pawn reviver/target,
their current controllers and shared world. It refuses an already reserved
target or an action whose cached pawn/player-controller/AI links do not match
the current control context. SameActors rechecks reciprocal possession, world
and deletion state, including the native current WorldInfo getter matching
the captured world. It does not repair stale action fields or ownership.

Eligibility requires an ordinary unpaused world/current game mode, a living
reviver outside ragdoll/recovery and a downed/nondead target within the target's
native revive range. It also requires BioWorldInfo and rejects active or
activating conversations through the native conversation manager. Continuing additionally requires current action5, its
exact instance in the pawn's action array, synchronized target and no firing.
Authority uses the supported native role value3 through explicit enum-to-int
conversion. No role value is written.

AcquireReservation requires authority on both pawns, current action5/exact
instance/synchronized target and an unreserved eligible target. It records
the attempt before setting the native bBeingRevived flag, with no native call
between the final false check and assignment. Duration and dispatch require
this captured reservation; another action's existing true flag is insufficient.
Client presentation must follow native action replication rather than acquire
this authority-side reservation or start its own authoritative duration.

ReleaseTargetState handles this action's timer pause before its reservation.
ReleaseReservation refuses while a timer pause remains owned and clears only
within the same current action/context and
authority. If the flag was already cleared while that context remains valid,
it drops its own record without another target write. If context/ownership
changed, it returns false and retains the record; Reset/Capture then refuse
to discard it in order to start another attempt. This is pending cleanup, not
proof of a restored target. Cancel latches cancellation and attempts release.
The owning action calls release before requesting native end/interrupt and
before inherited Stop, retaining a false result as pending target cleanup.

The flag has no owner/generation field. This component cannot detect an
external clear-and-reassert cycle entirely between observations. It is not a
global lock against arbitrary mods or native writes. Current-action/target
identity limits clearing; live stop/callback ordering still needs evidence.

## Native death-timer pause

AcquireDeathTimerPause follows authority-side reservation acquisition, before
inherited animation start. It searches the target's existing Actor.Timers for
the explicit binding FuncName=PermaDeath/TimerObj=Target. No matching timer is
a captured absent baseline; it does not create a timer. Duplicate matches
refuse. An existing timer records bPaused, bLoop, Rate, Count and
TimerTimeDilation. Rate must be positive/finite, Count nonnegative/finite and
dilation finite. An existing paused timer is preserved without being owned.

For an existing unpaused timer, ownership is recorded before native
PauseTimer(true, PermaDeath, Target), followed by contextual and value readback.
Duration cannot begin without a captured timer baseline. Continuing requires
the same binding and recorded values, including this action's expected pause;
appearance of a timer after an absent baseline cancels the attempt.

ReleaseDeathTimerPause only resumes a pause recorded by this context. It
requires the same action/control/world and authority. If the binding is gone,
it drops its record without recreating a timer. If its unchanged timer was
already resumed externally, it drops the record without another write. To
resume a still-paused timer, all captured fields must match and the target
must remain downed/nondead; a surviving timer on a recovered pawn is not
resumed as a death callback. Native unpause must read back as expected.

A duplicate/replaced/changed timer or lost context retains pending ownership;
reservation release and Reset cannot silently discard it. A timer has no
exposed generation, so identical removal/recreation entirely between reads
cannot be distinguished. This is conditional state accounting, not a global
timer lock or verified native recovery cleanup. The original timer's rate,
count, loop and dilation are never written, and no timer is added/removed.

BeginDuration starts once while the current action and its own target reservation
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

The authored action candidate now supplies animation-start/query handling,
per-update progress polling, native movement checks, a replicated-target path
and pre-end/pre-stop release attempts. Refused releases remain explicitly
pending. A scoped native death-timer pause is now included, but native timer
semantics, recovery removal and pending cleanup remain unverified. Native
usage-module baselines/cleanup, damage/native interruptions, animation
availability and surrounding native end/stop ordering remain incomplete.
This component's target-state bookkeeping alone is not
sufficient for activation or proof that stop/recovery cleanup completes.
Host story/session admission remains an external prerequisite; these checks
do not authorize interaction during conversations or acquire guest control.

The source is not a replacement for a missing action class or for its native
CanDoCustomAction checks. A caller must invoke Capture only when starting a
fresh action and must not Reset/rearm an in-progress attempt to replay a
partial dispatch. The context's direct target writes are limited to its scoped
bBeingRevived reservation. It performs no direct health, timer, collision,
profile, GRI, possession or action-array writes.

## Compile evidence

The authored context and action compiled together with zero compiler errors/
warnings and saved to a fresh private package using the build command. The receipt status was
`compiled-not-installed`; tracked tool/Oodle/Core/Engine/SFXGame/source hashes
were unchanged. The saved package digest was
`2f438318f453097ca4360db62485a575bfa128269ae21f7466a4baadcdbd7cf2`
for the native timer-pause revision.
Failed earlier compiler outputs remain separate private directories and are
not candidates. No native-source change required rebuilding the ASIs.

No game, behavioral test, fixture, package reload or runtime activation ran.
Compiler success does not verify these predicates on live objects, event
reentrancy, action replication or revival. Full campaign revival remains
incomplete.
