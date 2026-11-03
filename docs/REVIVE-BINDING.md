# Scoped revive action binding candidate

`scripts/SFXSquadCoop_ReviveBinding.uc` is an authored, compiled binding
component. No runtime constructs or calls it yet. It is not a package loader,
session admission policy or proof of a working action5 on either peer.
The [setup coordinator](REVIVE-SETUP.md) now prepares animation before binding
and retires binding before animation; the coordinator is also uncalled.

The caller must establish current co-op membership, world/session lifetime and
retirement before control handoff. It supplies an already available class and
a local admission controller. Bind accepts the exact authored action class or
the existing SFXGameContent.SFXCustomAction_Revive path. The existing-class
route uses that class's own lifecycle; the authored context's usage/timer
cleanup does not automatically wrap the existing class.

## Binding and presentation contexts

Owned binding requires a role2/3 party pawn with reciprocal current
SFXPlayerController ownership. Explicit simulated presentation binding requires
role1 and no pawn controller. Both require the local admission controller's
reciprocal pawn/current world, ordinary mode, unpaused/noncinematic state and
no active/activating conversation. They do not change roles or possession.
Runtime admission and the actual replicated actor on each peer still need
integration; the presentation argument is not an authorization token.

Authored instances now start with admission closed. Binding alone does not
enable native action attempts: the setup coordinator calls OpenAdmission only
after all preparation checks. It requires the same valid binding/ordinary idle
context and no pending action work. AdmissionOpen is part of setup readiness.
The existing basegame class has no such authored gate and is not selected by
the coordinator. This local mechanism is not a peer handshake or session ACL.

Binding refuses busy/downed/dead/ragdoll pawns, arrays shorter than slot5,
populated action instances and any different existing class at slot5. It never
resizes the arrays or overwrites another action. It captures the class baseline
(None or the selected class), table lengths, actor outer, controllers and role.

The actual current mesh must resolve CB_MP_Revive through FindAnimSequence,
with the expected SequenceName and positive finite SequenceLength. This is a
loaded lookup prerequisite, not playback or skeleton compatibility evidence.
The helper neither loads a player archetype nor adds an animation set.

It assigns only the selected class entry and calls the game's existing
VerifyCAHasBeenInstanced(5,0). It does not hand-construct the action or write
m_oPawn/m_oPC/m_oAI. Readback requires the exact class/slot, captured native
outer, current pawn/controller links and no AI cache. Instancing blocks a
reentrant bind/reset/release. A false Bind result can retain a pending record
when rollback cannot prove the original context; callers must retain and
retire that record instead of treating false as 'nothing changed'.

## Retirement

Release closes the exact authored instance's admission gate before checking
idle/action cleanup. A running action can therefore keep its slot for cleanup
while new InternalCanDoCustomAction attempts refuse. Closing does not interrupt
that action or suppress its Stop. A changed binding refuses the gate write.
Release requires the same current actor/control/world/role, table lengths,
class entry and created instance, with no current action/power action. It
refuses a started action, any pawn timer targeting that action (including
paused timers), or pending authored target cleanup. It never forces Stop,
clears a timer, destroys a UObject or copies controller caches. Only its unused
created instance slot is removed and its captured class value restored.

A deleted pawn/world drops the component's record without writing its tables;
native destruction owns that actor. A different still-live world/control/slot
retains pending state. Array allocation identity and identical replacement
between observations are not exposed by this script, so these checks are not
an exclusive lock. Native end/stop ordering and pre-handoff retirement remain
unverified and are activation requirements.

## Evidence and integration boundary

A focused SFXGame script read decoded VerifyCAHasBeenInstanced completely:
727 serialized bytes/1055 VM bytes, with16 aligned explicit branch
destinations. Its normal-action creation block uses the class table, creates
with the pawn's Outer (not the pawn itself), assigns m_oPawn, then selects
m_oAI or m_oPC from the current controller. A populated slot skips creation;
the helper is not an established cache refresh for existing instances.
Package/Oodle input hashes remained unchanged.

Context, action and binding compiled together without errors/warnings, with
unchanged tracked tool/game/source hashes. The saved private package digest is
`01783b8482808e26e88bce56e579832b021204abc1ed6a71b7e2605bee925f1c`.
No package reload, runtime installation, native binding call, game or tests ran.
The native ASI sources did not change. Shared package loading, per-peer session
admission, scoped animation provisioning and invoking/retiring this component
from the runtime remain unfinished. See [animation sources](REVIVE-ANIMATION.md)
and [action limits](REVIVE-ACTION.md).
