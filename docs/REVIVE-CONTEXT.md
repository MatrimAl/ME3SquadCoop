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

ReleaseReservation clears only within the same current action/context and
authority. If the flag was already cleared while that context remains valid,
it drops its own record without another target write. If context/ownership
changed, it returns false and retains the record; Reset/Capture then refuse
to discard it in order to start another attempt. This is pending cleanup, not
proof of a restored target. Cancel latches cancellation and attempts release.
The future owning action must call release before surrendering its action
identity and handle a false result explicitly.

The flag has no owner/generation field. This component cannot detect an
external clear-and-reassert cycle entirely between observations. It is not a
global lock against arbitrary mods or native writes. Current-action/target
identity limits clearing; live stop/callback ordering still needs evidence.

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
pending. Native usage-module/death-timer baselines and cleanup, damage/native
interruptions, animation availability and surrounding native end/stop ordering
remain incomplete. This component's reservation bookkeeping alone is not
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

The authored context compiled with zero compiler errors/warnings and saved to
a fresh private package using the new build command. The receipt status was
`compiled-not-installed`; tracked tool/Oodle/Core/Engine/SFXGame/source hashes
were unchanged. The saved package digest was
`5922d1574e873d5b0eac76455b550bed410330af01860d94e74c644b14e59c48`
for the reservation/current-world revision.
Failed earlier compiler outputs remain separate private directories and are
not candidates. No native-source change required rebuilding the ASIs.

No game, behavioral test, fixture, package reload or runtime activation ran.
Compiler success does not verify these predicates on live objects, event
reentrancy, action replication or revival. Full campaign revival remains
incomplete.
