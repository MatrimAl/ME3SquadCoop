# Native firing observations

`ME3_SQUAD_COOP_NATIVE_FIRE_TRACE=1` enables a bounded research observer alongside
`ME3_SQUAD_COOP_AMMO_WIRE_TRACE=1` in the existing protected progress test.
The default is off. This changes no gameplay values and is not a firing fix.

The supported build's native `SFXWeapon_NativeBase.FireAmmunition` script bridge
tail-calls virtual slot `0x7d8` with the weapon as its sole argument. The observed
selected weapon uses vtable RVA `0x14fa430`, with the slot pointing at RVA
`0xc62120`. This native fallback ultimately dispatches a resolved function through
`ProcessEvent`; the exact resolved target still needs runtime observation.
Installation requires the supported image, the exact complete
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
runtime evidence is required separately.

In a protected paired run on 2026-10-23, six original native calls per side each
advanced consumed count by one. Two 250 ms guest input pulses settled at
host/guest 3/3 and 6/6, reserves 210/210, and stopped consuming after release.
Snapshot correction was disabled. Each incoming host start-fire request used
mode 2 and advanced its script cursor while entering `WeaponFiring`.

This is one bounded success. The preceding pair received firing RPCs without
authoritative consumption; no intentional firing behavior fix separates the
runs. Repeatability with observation disabled and the cause of the difference
remain open. No authoritative enemy health/shield change was measured, so this
run does not establish enemy damage or full combat. Private raw captures and
the audited build/source fingerprints are retained outside the public repository.

The optional script observer now also covers `Pawn.StartFire`, both base and
SFX inventory-manager `StartFire` implementations, `BioPawn.Tick` and
`BioPawn.HandleWeaponFiring` (20 exact methods total). Pawn, inventory and weapon
receivers are distinguished; inventory observations require the existing pawn
and weapon references to agree and the manager's instigator to be that pawn.
The two per-frame entries are recorded only while the controller's fire input
is pressed, within the existing record budget. These hooks do not invoke an
extra firing or gate function. Internal VM calls may bypass `UFunction.Func`,
so missing observer entries alone cannot establish that a method did not run.

Owned script analysis identifies the natural path as `BioPawn.Tick` checking
`IsLocallyControlled`, then `HandleWeaponFiring` checking pending fire and
`CanFireWeapon` before calling `Pawn.StartFire`. These selected script bodies
contain no player-pawn class cast. The native local-control predicate remains
a separate verification requirement. The expanded observer passes its DLL
self-test and seven compiled negative controls; this is observer validation,
not a new in-game combat result.

The first bounded pair with this expansion rejected observer installation on
both sides. Live reflection confirmed that `SFXInventoryManager.StartFire`
has a local object variable `W` after its byte parameter; the original
weapon-only validator required the parameter to be the last child. The
validator now pins that specific local's owner, name, offset and flags and
continues to reject unexpected children. Installation failures also identify
the rejected method. The corrected build passes the native harness. A later
bounded pair verified installation of all20 methods on both sides. That pair
also dispatched no fire pulse: its inventory reference became null roughly
six seconds after the first healthy ownership baseline, before the eight-second
fire-test baseline could complete. No weapon-call silence from this run proves
an engine refusal to fire. The earlier pair dispatched no fire pulse
because the inventory-chain eligibility gate refused it; the session lifecycle
passed, with no native exceptions or original-profile changes. Neither combat
nor full dialogue was verified.

Read-only native-slot capture and exact-image analysis resolved the Ashley
local-control predicate: in a network session its controlled player controller
requires `Player` to be an `Engine.LocalPlayer`. The inspected predicate has no
pawn-class or Role restriction. Static logic and slot identity do not prove
the predicate's result at a particular firing tick.

The isolated fire bot now waits two seconds of continuously valid ownership
and equipment before its ordinary 250ms input pulse. The earlier eight-second
QA delay exceeded the measured six-second inventory window and prevented any
input observation. Native permission checks, per-lease attempt limits,
identity/chain validation and release behavior are unchanged. This timing
change does not repair inventory replication or alter normal player input.

A subsequent protected pair exercised that earlier pulse. The guest's normal
250ms input reached the host's native `ServerStartFire` with mode2. Three
original `FireAmmunition` calls per side advanced consumed count0 to3; the
existing verifier recognized authoritative and guest firing on the same
weapon/control window. Two retained post-release samples per side stayed at3,
but the stricter stop gate remained unverified. Enemy damage was not measured.
The later inventory class-mismatch receive still cleared the guest's manager,
so this is evidence of firing while the chain is valid, not sustained combat
or an inventory repair. Full dialogue and common travel also remain unverified.
