# Guest weapon return after a host scene

The latest owner test reproduced missing guest shooting after a Mars dialogue.
Its expanded observer captured a genuinely null Pawn.Weapon, while the native
replicated index still resolved the same reciprocal inventory weapon in the
Inactive state. The previous conditional return expired without dispatch.
Full scene animation and rendered camera parity also failed that owner test;
neither is established by a cached camera match or by this weapon correction.

This candidate covers one precise native continuation: after an authenticated
host scene ends, a previously selected existing guest weapon is still the
native replicated inventory-index target but Pawn.Weapon is actually null.
It invokes the stock SFXInventoryManager.ReplicatedWeaponUpdated method once.
The stock method selects through its own pending weapon, ChangedWeapon,
activation and weapon-switch animation path. It does not synthesize firing.

Admission requires the same live controller, Ashley, inventory, weapon, world,
connection and native actor-channel identities captured before the scene.
The entire bounded inventory chain must have reciprocal owner, instigator and
manager links, and the received zero-based ReplicatedWeaponIndex must resolve
that same prior weapon. No index is invented or written. A foreign selection,
missing chain, pending selection, active input/action/power, native scene gate,
replaced identity or stale scene message refuses the continuation.

Only a later authenticated inactive host message opens a ten-second return
window. A message freshness timeout does not count as scene completion. The
window remains available when the prior selection initially returns normally,
because the reported loss was delayed. A different native selection ends it.
The single invocation is consumed before entering normal engine code. No new
inventory, attachment, ownership, animation tree, transform, input, ammunition
or timer is authored. The stock method's missing-inventory timer branch is
excluded by the complete chain checks immediately before dispatch.

Return admission requires the existing weapon's exact Inactive state. It does
not reuse the equipped-idle fire-mode and animation-bit checks: native
Inactive.BeginState does not clear those fields, and ChangedWeapon/Activate is
the stock path that resumes selection. Pawn actions, powers, controller fire
and pending inventory fire requests still refuse it. The stricter equipped
predicate remains unchanged for ordinary baseline and ammo handling. Bounded
refusal events distinguish context, channel, chain and native-state failures.

The full 513-byte runtime VM and all 24 object and six name relocations of
ReplicatedWeaponUpdated are checked. The exact 142-byte GetInventoryByIndex VM
and its 13 object operands independently establish the chain/index ordering.
Native method ABI, actual class dispatch slots and Actor.ProcessEvent are
validated. First valid ordinary baseline preparation records
`baseline-stock-method-ready`; it does not invoke the return method. The full
body/identity checks run again before an actual return invocation.

Compile/fixture checks establish the guard logic only. Real dialogue return,
firing and animation correctness require a later owner gameplay check. A
startup baseline observation does not prove that the recovery branch ran.
The focused native fixture executes the current return and equipped-idle
helpers on temporary memory, including stale inactive fields and active input
refusals. Native selection dispatch and real shooting remain gameplay checks.
