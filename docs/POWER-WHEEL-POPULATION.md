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

## Keep registration independent of optional icons

The guest frame now uses a dedicated read-only mode3 context-retention
predicate before the normal scene adapter. Previously it reused the icon
lease's stricter eligibility, so disabling optional icons or opening Shift
before icon capture could retire an otherwise healthy registered context.

Retention requires an already registered and reciprocally owned current local
Ashley/controller in a client world, a positive current registration epoch,
live channel/world/squad identities, matching native squad pawn caches, the
actual PowerWheel mode object owned by that controller and unpaused scene
state. Missing/downed/dead/busy-invalid or noncombat pawn state refuses it.
Native and synthetic squad-registration paths retain their own identity and
cleanup checks. Mode/header, environment, epoch and ownership are rechecked.

This predicate only skips releasing the current registration during that
verified menu. It does not create/register a context there. UI modification
still requires its active valid optional icon lease, hooks and current catalog;
native discovery remains mode0-only and the mod order queue remains mode0-only.
No pause/input/scene flag is cleared by retention. Three native targets
compiled and build fingerprints matched. No game or tests ran; menu continuity,
full-list initialization and close-time order delivery remain unverified.

## Retained-project results

The direct Ghidra batch completed with exit0 and saved its project. Nine of ten
requested exact entries decompiled; cached-player leaf `0xc51aa0` was not a
separate analyzed function. Its earlier instruction-grounded leaf evidence
still applies. A readOnly/noanalysis export of the same manifest took about12
seconds and returned identical per-function evidence. Initial import/export
took about14 minutes. Three new helper entries and the visibility method then
decompiled through that same saved project, without another import.

The full squad-refresh method `0xc521b0..0xc523c4` clears the three cached
pawns before selecting a controller-derived pawn and up to two other members.
It contains alternate native/session branches whose conditions must remain
explicit; its output is not equivalent to the short non-henchman cache leaf.
Its controller helper `0xa9dfa0` first casts the possessed pawn through
`0x551e70`, with separate mode1/mode2 handling. That cast's class factory
`0xa8e960` identifies BioPawn through the UTF-16 class name at `0x133e1aa`.
This path is not a Shepard-only SFXPawn_Player cast. Merely replacing that cast
would therefore address the wrong prerequisite for Ashley.

The mode query at `0xbb2d20` reads the selected mode object's active bit from
GameModes, rather than simply comparing CurrentMode. Preserve that distinction
when considering alternate control modes. Native list collection dispatches
GetPowerWheelPowers through ProcessEvent; it is not a raw copy of Powers.
Its native filtering and script overrides cannot be equated to the mod's base
membership catalog without checking the actual instances.

SetWheelVisible resolves through registration thunk `0xb63720` to
`0xc53730..0xc5390e`. It dispatches visibility callbacks and its hiding path
walks/updates existing full-list icons. Showing a wheel is therefore not a
harmless refresh call to substitute for population. Some decompiler calls lack
fully recovered argument information, and indirect-tail-call warnings remain
in SetupPlayerPowers. These outputs are retained research, not ABI permission.
No new runtime call or game/test execution followed from this batch.

## Native update path and the remaining context mismatch

A bounded selection of26 PC-wheel vtable entries in the previously observed
wheel code region produced25 decompiled methods from the saved project in
about12 seconds. The same short cache leaf remained a missing analyzed entry.
This code-region selection is navigation coverage, not the entire inherited
vtable or26 resolved semantic roles. An additional direct base-update method
then decompiled through the same project.

PC-wheel virtual `+0x3f8` resolves to `0xc58040..0xc58409` and calls base
update `0xc58410..0xc58b36` (inclusive analyzed body extent).
The base method lazily resolves the handler controller at `+0x6f8`. If the
handler squad at `+0x700` is null, it obtains the controller's BioPawn and
copies that pawn's `Squad+0xba8`. It does not obtain this reference from the
mod's registered WorldInfo player-squad slot. Once nonnull, that handler squad
is reused. On a client world it selects the non-henchman refresh, which reads
the handler squad's CachedPlayerPawn. A successful input-context registration
alone therefore does not prove the UI holds the same squad/pawn context.

This matters particularly for the synthetic context adapter, which deliberately
keeps Pawn.Squad unchanged while registering a separate local player context.
The native UI can continue reading the story/previous squad even when the
local combat getter is positive. This is a static mismatch mechanism, not a
newly reproduced live failure or a claim that every native squad is wrong.

The base update uses the native squad-changed flag to refresh group contents.
On entry to power mode, virtual `+0x4a0` at `0xc55760` also populates the
existing player and two henchman groups. A later adapter should align the
wheel's transient squad/pawn references with the admitted registration and
let these native population paths run. Changing Pawn.Squad or host story
membership, forcing visibility or repeatedly invoking population is not
necessary evidence-based integration. Any UI-reference lease must capture and
conditionally restore its baseline, preserve engine replacements and retire
on controller/world/registration changes.

The PC update adds a separate coordination problem: its periodic refresh calls
the native quickslot setup, and its visible-slot comparison can call setup
directly when icon names differ from saved controller definitions. An optional
icon lease can consequently be overwritten by known native refresh work and
retire on its next value check. This makes stable quickslot rendering
unproven. A one-time paint, more frequent painting, or vtable-only interception
of setup does not cover the observed direct call. Preserve native display,
cooldown and permission work when integrating this path.

Six visibility/lifecycle exports also decoded completely, including base
WheelVisibilityChanged1221/1809 serialized/VM bytes and PC override480/652.
They update selection, visibility, text, portraits and quickslot hover/display
state. Neither selected visibility script calls SetupPlayerPowers or writes
the controller/squad/pawn cache references. OnPanelRemoved calls cleanup;
cleanup clears references and icons. Calling a visibility callback alone
therefore does not repair the context mismatch. No UI cache reference or
native update method was changed or invoked for these findings; no game or
tests ran. Full guest-wheel initialization remains incomplete.

## Registered squad reference candidate

The optional native-icons configuration now owns a separate wheel-context
lease. Acquisition requires ordinary mode0, the existing native-hotkey bridge,
a fresh unique discovered PC wheel, reflected UI reference layouts, a current
eligible power catalog and the already registered guest squad. Both native
existing-squad and synthetic registrations must pass their own world/channel/
epoch/cache checks. No context is created or acquired inside a menu.

In the native client/non-henchman branch, the candidate conditionally swaps
only the handler's transient m_pPlayerSquad reference to that registered squad
and clears stale cached henchman UI references. It captures live same-world
baselines and uses aligned writable-pointer CAS. It does not change Pawn.Squad,
WorldInfo registration, membership, story, input/profile mappings or native
permission flags. It calls no population/visibility/update function. The normal
native Tick can then select CachedPlayerPawn and populate groups at its usual
squad-change/menu-entry points.

Native m_pShepardPawn remains engine-owned. Retirement restores only references
actually written by this lease, and only while the handler still holds the
leased squad and those exact installed values. Expired baseline actors become
null; engine replacements are preserved. The native player-cache transition
is left to normal Tick so its change detector can refresh the native groups.
No cleanup native setter is invoked on an old-world widget. A conditional
restore attempt is not proof that runtime restoration succeeded.

The full-wheel caster now uses this context lease instead of depending on
the separate quickslot-paint lease. It still validates every selected-group
icon/name/power, current registered Ashley/controller/action caches, empty
capacity state, reflected group indices and the exact native selection caller.
The final context and current mode are checked again. Native targeting,
cancellation, permissions and close-time tactical-order delivery remain in
the original game path. A quickslot paint being retired no longer alone ends
the full-wheel context lease. Quickslot rewriting by native PC Tick remains a
separate unresolved display-coordination issue.

Three native targets compiled and current fingerprints matched. No game or
tests ran. Initial lease timing, natural population/rendering, all native
callbacks, selected casting and retirement still require gameplay evidence.
