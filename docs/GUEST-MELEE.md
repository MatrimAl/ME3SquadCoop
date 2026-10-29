# Guest melee preparation

The later protected `0b8c6fe` owner test confirmed that guest melee executes,
and reported an incorrect camera position during the attacks. This confirms
the owner's observed attacks, not every combo stage or damage case.

In the later protected `3df9b60` readiness run, all six normal melee slots and
ten cover slots were published on both peers. A bounded read-only inspection
also found all three rifle clips on both owned Ashley meshes. Existing-global
source admission was observed. No input was automated, so actual attacks,
combo transitions, damage and subsequent handoff are still owner checks.

The protected `790d685` run published all six normal slots on the host but
only pistol slots78..80 on the guest; rifle slots75..77 and optional cover63
remained absent. Both own Soldier melee passives were present. This proves
preparation, not actual melee execution or damage.

The next candidate also searches the existing native global animation set
referenced by a mesh's BioDynamicAnimSet when its local sequence list lacks
a required clip. The reflected global pointer, same BioAnimSetData and exact
original-set FName must match; the actual requested sequence must be present.
It never searches unrelated assets, creates an animation set or treats a
global source as already registered on Ashley. The existing native temporary
animation ledger must still register it and confirm the clip on the mesh
before publishing a slot. Known loaded sequences and all cleanup/ownership
checks remain mandatory. Missing global references still defer the family.

Static owned-package inspection found the actual CB_RifleAtk1/2/3 sequence
exports and their animation data in the existing Soldier/Ashley packages;
no new asset or redistribution is needed. Whether the guest's current dynamic
set exposes the matching global reference still needs the next owned runtime
observation. The recorder retains `existing-global-clip-source` separately
from family publication and actual gameplay.

The native companion defaults lack Ashley's player melee slots and the passive that
provides real melee damage. The candidate prepares the six normal rifle and
pistol combo actions, Soldier heavy/storm and cover melee, using the actual
loaded player's class references. It
adds an Ashley-owned Soldier melee passive through stock PowerManager.AddPower,
with its own native initialization and default damage data. It never uses
Shepard's power instance or copies damage, rank or cooldown values.

The rifle clips were present on Ashley's current mesh. The observed pistol
clips required an existing loaded player animation set, registered through
stock RegisterTemporaryAnim. Retirement releases that reference through stock
UnregisterTemporaryAnim, restores only the class/instance cells still owned by
the lease, and removes only its own unchanged passive through RemovePower.
Foreign replacements retain their dependencies. Real actions finish before
their cells are restored; story/AI handoff is never delayed.

Removal and temporary-animation cleanup keep a pending lease when native
cleanup refuses or has not completed. Each native return rechecks the captured
world, pawn, manager and permitted owner before continuing. A synchronous
release during native dispatch cannot erase that in-flight lease. Later
retries do not remove a successfully removed passive twice.

The normal and Soldier heavy/storm actions can retain an existing guest shoulder
scale through their exact stock Melee camera. The observed canonical defaults
request `SFXGame.SFXCameraMode_Melee`; the earlier null-only camera condition
incorrectly released that scale during melee. The retained SDK declares Melee
as Combat-derived, so the existing gameplay-camera gate already accepts it.

The correction accepts only that exact requested class. Any existing action
camera must have the same class, target the current owned pawn, enable
collision and remain out of first-person mode. Class/instance identities,
camera links and target/flags are rechecked before retaining the scale.
Canonical action, current action, reciprocal player ownership and empty AI
cache checks still apply. An actual action with neither requested nor active
camera retains its previous path. Native code continues to choose the camera,
angle and FOV; scene and cinematic gates remain in force. Added native fixtures
cover the canonical camera and refuse foreign requested classes, active
classes, targets and flags. This correction has not been built or game-tested
in this auxiliary checkout yet.

Stock melee input, action permissions, animations, targeting, impact damage on
the authority and action replication retain their existing paths. Neither input nor
damage is replayed. Normal hits, all combo stages, pistol transitions, camera
and disconnect/scene recovery still require an owner gameplay test.

## Heavy and storm melee

Slots58 and73 borrow the loaded SoldierPunch action only after its animation
set, native methods and plot guard are ready. The existing native heavy action
registers its own heavy animation and applies damage through Ashley's passive.
Current class/passive preparation is pinned to the loaded Soldier data.

Stock heavy StartCustomAction also enables shared campaign cosmetic plot20881.
The guard forwards its exact SetBool exec so the engine decodes all arguments
and advances its own frame. Only that authorized Ashley heavy frame's decoded
20881/true native-body call is consumed. The actual canonical table, actor,
class, native call site and frame cursor must match. Other calls, including
Shepard's, forward once. Nested calls shadow their source; arguments and
Windows LastError remain intact. No plot is changed or changed back by the mod.

The stock getter's already-existing class/CDO path identifies the canonical
table without constructing or loading anything. Weak actor authorization
survives normal retirement so a queued heavy action cannot write the flag
after a control handoff. Heavy slots remain unavailable when the guard or
required loaded assets cannot be proved; normal melee stays independent.

Native fixtures and exact owned-executable pins pass. Real heavy/storm
animation, damage, host plot behavior and scene/AI recovery are not yet gameplay
verified. Complete multiplayer mechanics and movement parity remain open.
No game test was run for this candidate.

## Cover melee

The candidate prepares the actual loaded player's cover grab and left/right
edge actions in slots62 through71. Slot63 includes its alternate pistol stance.
The classes whose names contain Hvy still inherit PlayerMeleeBase and use the
same normal melee passive/impact path; they are not SoldierPunch heavy actions.

The grab in62 uses the existing synced ClassMelee path. Its stock damage amount
and target eligibility remain intact; the mod does not replace its configured
damage with the normal passive value or replay a hit. The attacker/victim
temporary animations, melee-over-cover sequence event and targeting/permission
checks execute through the original action.

Each optional slot is admitted independently after exact source class, base,
power layout and animation checks. Missing clips use an already-loaded player
AnimSet through the stock reference-counted temporary animation API. Slot63
must have both its rifle and pistol clips; a source-set gap defers that slot
before native mutation and preserves the working core. Extra references are
released after the borrowed actions have finished and their cells are restored.

Verified cover actions can keep the existing guest shoulder scale through their
own canonical Combat-derived cover camera. The native action selects that
camera; the adapter does not replace its angle, FOV or target. Unknown camera
classes, other owners, AI caches and scene modes retain the prior refusal path.

Slot72 is observed but not prepared: its authored AnimInfo is absent, and no
dependency on it was found in the selected input/action scripts. This candidate
does not invent a missing animation or force a transition into that action.
Cover animations, real hits, grab permissions, camera and scene/AI cleanup
remain owner gameplay checks; no new game was launched.
# Independent normal melee preparation

The `e915a55` owner test reported no melee. Both peers logged repeated
`waiting-loaded-melee-clips` with no published slots. That aggregate refusal
does not identify which clip or method prerequisite failed. Previously a missing
pistol animation set could stop every normal, heavy and cover melee slot.

Normal rifle and pistol combos now prepare as independent complete three-action
families. Each missing clip may use its own actual loaded player animation set;
all three no longer have to reside in one set. References are registered and
retired through the native per-row ledger, with readback and ownership checks.
An unavailable family is retried while idle and does not stop ready heavy,
cover or other normal actions. No animation is invented, no tree is replaced,
and no action or damage is forced. Native input chooses the weapon's family.
Full melee execution and missing-family recovery still require a game test.


## PowerManager native dispatch correction

The `aa7b1d9` run prepared a normal family but published no melee slots. Its
captured melee lease remained unassigned. The supported executable provides
a concrete blocker: SFXPowerManager is a UObject, whose ProcessEvent slot
points to RVA0xf4f80. The adapter incorrectly required Actor ProcessEvent
0x40d450, so this manager could not pass admission.

The correction validates the exact manager vtable, native GetPower slot and
UObject ProcessEvent entry, then uses that same validated native address for
AddPower and RemovePower. Actor/pawn animation dispatch remains unchanged.
Existing owner, action, class, method, asset and cleanup checks remain required;
there is no forced power, damage, animation or input. Native pin fixtures cover
the supported manager and reject the old Actor address, changed native entry,
wrong lookup slot and foreign vtable. The native harnesses passed. Actual
passive creation, ready attack families and melee damage still need an owner
check; this correction does not prove complete melee playback.
