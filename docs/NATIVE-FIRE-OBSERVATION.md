# Native firing observations

`ME3_SQUAD_COOP_NATIVE_FIRE_TRACE=1` enables a bounded research observer alongside
`ME3_SQUAD_COOP_AMMO_WIRE_TRACE=1` in the existing protected progress test.
The default is off. This changes no gameplay values and is not a firing fix.

The supported build's native `SFXWeapon_NativeBase.FireAmmunition` script bridge
tail-calls virtual slot `0x7d8` with the weapon as its sole argument. The observed
selected weapon uses vtable RVA `0x14fa430`, with the slot pointing at RVA
`0xc62120`. This native fallback ultimately dispatches the base script through
`ProcessEvent`. Installation requires the supported image, the exact complete
bridge/body instructions, the expected live vtable/target and reflected method
identity and ABI. The observer forwards the original argument once and preserves
the original call's incoming and outgoing `LastError` values.

The observer records at most 128 entry/leave rows. Capture requires the game
thread and the existing validated pawn, weapon, ownership, world, connection and
channel scope. Before/after snapshots include consumed/reserve counts, weapon
state, actor roles, pawn `ShotCount`, `AmmoPerShot` and
`NoAmmoUseChance.Value`. A changed control generation invalidates the snapshot.
Other weapons and threads are forwarded without observation.

The native fallback's entry is stronger evidence of that dispatch than observing
only `UFunction.Func`: VM-internal script calls may bypass the latter. This hook
still cannot see callers that bypass its particular virtual slot. Missing rows
alone do not prove that all firing paths were absent. An observed entry does not
prove ammo consumption, a valid hit, damage or full combat.

In this build `Pawn.ShotCount` is a local non-replicated field. Both base
`Pawn.WeaponFired` and the `BioPawn`/`SFXWeapon.WeaponFired` path increment it
before playing firing effects. `ServerProcessInstantHit` can reach this effects
path through `SetFlashLocation` after accepting the reported hit. It contains
no ammo-consumption call. A host `ShotCount` increase therefore does not establish
that the host ran `FireAmmunition` or consumed ammunition. Target health/shield
changes and attribution still require separate authoritative observations.

`ConsumeAmmo`'s early-return condition uses the bias-adjusted value:
`(NoAmmoUseChance.Value - 1.0) > 0.0` and
`FRand() < (NoAmmoUseChance.Value - 1.0)`. A value of `1.0` means zero chance
of skipping consumption through that branch. This interpretation was checked
against the live native operator indices rather than inferred from the name.

The native fixture covers exact forwarding, `LastError`, identity/generation/
thread guards, changed bridge/body instructions, changed vtable/target and the
paired record budget. Compiled negative controls deliberately remove forwarding
or identity protections. These checks do not establish in-game firing behavior;
the observer needs a new protected paired run for that.
