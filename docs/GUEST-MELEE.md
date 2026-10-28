# Guest melee preparation

The guest currently lacks Ashley's player melee slots and the passive that
provides real melee damage. The candidate prepares the six normal rifle and
pistol combo actions, plus Soldier heavy and storm melee, using the actual
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

The normal six melee actions can retain an existing guest shoulder scale when
the actual player-owned action requests no camera and has no AI cache. This
does not acquire a camera inside an action or bypass a real scene/camera mode.

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
verified. Cover melee, complete multiplayer mechanics and movement parity
remain open. No game test was run for this candidate.
