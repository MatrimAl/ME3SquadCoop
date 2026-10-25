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
