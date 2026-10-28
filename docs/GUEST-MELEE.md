# Guest melee preparation

The guest currently lacks Ashley's player melee slots and the passive that
provides real melee damage. The candidate prepares the six normal rifle and
pistol combo actions using the actual loaded player's class references. It
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

Stock melee input, action permissions, animations, targeting, impact damage on
the authority and action replication retain their existing paths. Neither input nor
damage is replayed. Normal hits, all combo stages, pistol transitions, camera
and disconnect/scene recovery still require an owner gameplay test.

Heavy, storm and cover melee are not prepared by this first slice. The stock
heavy action also writes a shared campaign cosmetic plot flag. That dependency
must be handled before adding it without allowing the guest to advance host
story state. Complete multiplayer mechanics and movement parity remain open.
