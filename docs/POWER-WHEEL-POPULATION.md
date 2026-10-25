# Native power-wheel population

The retained REA/Ghidra result for the supported executable completed
successfully. Target RVA `0xc539a0` accepts a wheel handler, pawn and group
index. This is static pseudocode evidence, not proof of a safe callable ABI
or a working guest wheel. The original extracted output stays private.

The result confirms these implementation prerequisites:

- Groups0/1/2 select existing integer-index arrays at handler offsets
  `0x3c4/0x3d4/0x3e4` and associated UI strings at
  `0x57c/0x58c/0x59c`. Other group values are unsupported.
- Powers come from the supplied pawn's manager at `pawn+0xc18`, through
  helper RVA `0xbce450`, followed by sorting. The method fills existing
  group icon capacity; it does not construct or resize that capacity.
- The method dispatches a handler event before filling icons. Its event
  identity and side effects still need resolution.
- A nonnull pawn path dereferences the handler's controller at `+0x6f8`,
  then controller input at `+0x6dc` and another controller object at
  `+0xc50`, whose field `+0x70` is passed to icon availability logic.
  These dependencies are not locally null-checked in the observed path.
- Each populated icon receives native SetPower, availability/state updates,
  styling and a handler virtual callback at `+0x520`. Unused group icons
  are cleared and refreshed. Blocker text/visibility also changes.

Consequently, calling the population function with Ashley alone is
insufficient. Integration needs validated controller dependencies, existing
group capacity, icon ownership, callback identities and retirement behavior.
Painting only the assigned icons would leave native state, blocker and
callback behavior unresolved. Existing caster selection protection still
requires the entire selected group to belong to the guest's current catalog.

The successful CLI operation reported823870ms (about13m44s), including import
and analysis. Its profile explicitly reports `ephemeral-source-immutable`.
Retained output can be reused, but this result does not establish a persistent
Ghidra project or guarantee that another CLI invocation avoids reimport.
Before a larger unattended batch, establish project reuse and collect related
function evidence together rather than repeatedly importing the executable.

The private dossier now checks the result's success flag, target digest,
provider, operation, exact procedure and nonempty pseudocode before labeling
it verified target pseudocode. It also records the evidence/output identities,
duration and import mode. These checks establish artifact identity, not
semantic completeness or gameplay correctness. No game or tests ran.

## Population dependencies resolved further

Six selected owned script exports decoded completely: immediate-order
permission180/260 serialized/VM bytes, queued-order permission146/218,
IsPawnBlocked88/124, GetCurrentHenchmanOrder122/178, base InitDisplay550/738
and PC InitDisplay196/296. The private feature dossier retains these sources.

The handler event at the beginning of population is GetCurrentHenchmanOrder.
The executable name pointer at RVA `0x17a8ae8` identifies name entry
`0x142fbd4`. Its script returns None for a missing pawn or a pawn whose
Controller is not SFXAI_Henchman. The henchman path reads GetNextOrder(1)'s
power name. It is not a new order submission. Immediate/queued permission
checks dispatch through either the henchman controller or BioPlayerController,
and refuse when neither exists. They do not require a Shepard pawn class.

The retained SDK identifies controller `+0xc50` as
BioPlayerController.m_oPlayerSelection, a BioPlayerSelection, whose `+0x70`
is m_oCurrentSelectionTarget. These declaration leads need reflected runtime
layout/owner validation before a new native population call. The helper at
`0xa97a50` is a class-hierarchy cast of the input object, not an input
configuration/reset call; its null path returns zero.

The PC wheel's `+0x520` callback resolves to `0xc4bdc0`. Its initial fragment
is only ten instructions; the next fragment contains mapping display work.
It compares each icon's pawn with cached player/henchman fields and reads
player input's named mappings. Replacing the population pawn alone can leave
mapping display inconsistent with those cached fields. Full callback extent,
menu refresh ownership and conditional retirement still require integration.

## Empty group capacity must remain selectable around

Native population clears unused capacity icons with SetPower(None). The
observed null-power branch writes the actual None name and current/desired
state5 (`0x505` at icon offset `0x1bc`). Native SelectCurrentPower only enters
the other-icon cancellation pass for current state3. Requiring every capacity
slot to hold an eligible power therefore prevented a valid smaller Ashley
list from passing the mod's caster check.

The caster candidate now accepts an unselected empty icon only with null
power/pawn, the actual None name, reflected byte-state fields and both states
exactly5. It captures/rechecks those icon identities and values with the rest
of the group. The selected slot still must contain an owned eligible power;
populated foreign/stale/duplicate entries still refuse the entire group.
This does not populate a group or relax native cast permissions. Three native
targets compiled and fingerprints matched. No game or tests ran.

## Preserve native close-time delivery

The retained complete SFXGameModePowerWheel.Deactivated script
(387 serialized/563 VM bytes) calls the base deactivation, obtains the GUI
interaction, requests PauseGame(false), then checks the mode's Outer
controller's Pawn cast to BioPawn and its bCombatPawn flag. Only that branch
invokes the same Outer controller's ApplyTacticalOrders. The GUI sound and
hint restoration follow. A later positive DisableMoveTime/cover-state branch
temporarily ignores movement and schedules EnableMovement.

The mode's Outer/controller identity is therefore part of delivery, not merely
a UI owner label. The existing mode-retention guard already checks that Outer
against the current guest PC. A population adapter should preserve the
registered context until native deactivation has consumed pending orders.
It must not independently replay ApplyTacticalOrders or emit a replacement
cast on menu close: an additional delivery path could duplicate a native
order. The conditional cover movement delay is also native behavior, not
evidence by itself of a new co-op pause fault. No close event was executed;
actual order delivery and movement recovery remain unverified.
