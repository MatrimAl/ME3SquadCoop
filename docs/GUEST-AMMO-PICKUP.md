# Guest dropped-ammo admission

The native dropped-ammo path already handles Ashley's inventory and weapons.
Its `ValidTouch` predicate also requires `Pawn.bCanPickupInventory`, which the
player default explicitly enables. Ashley's serialized inheritance chain has
no corresponding override. The adapter only acts when her actual live flag
and class default are false and the canonical player default is true. An
already enabled live flag follows the original predicate unchanged.

The change is limited to `SFXDroppedAmmo.Pickup.ValidTouch` for the currently
owned human Ashley, on both peers. It checks the live native Pickup state,
enabled pickup, reciprocal controller possession, current world, actor channel,
connection and ownership token. Real scene/downed/control-loss contexts are
excluded. It validates the supported-image script executor, exact function ABI,
all179 VM bytes with reflected operands, and the stock return expression at148.

Defined script calls bypass the function's native-slot pointer. A single guarded
executor detour therefore filters the real frame node before applying this
adapter. Other scripts immediately forward to the unchanged trampoline, without
metadata discovery or a process-memory query on that path. The prior cover
selector's full executor check accepts only this owned patch, its unchanged
remaining bytes and the retained trampoline; a foreign replacement is refused.

For this case only, the script frame cursor selects its own existing
`return CanPickUpAmmo(Other)` expression. The executor runs once with unchanged
arguments/result storage and preserved incoming/resulting LastError. No
candidate result is forced. The stock inventory iteration still requires a
nonheavy weapon with finite ammunition below its spare-ammo maximum. Native
`Touch` still performs `GiveTo` only on authority, and native difficulty rules,
weapon `AddAmmo`, pickup sound and pickup consumption remain in place.

The mod does not set `bCanPickupInventory`, synthesize a touch, call `GiveTo` or
`AddAmmo`, fill a weapon, or write ammunition. General inventory/story pickups,
Shepard and AI retain their existing rules. Both peers use the scoped predicate
so an eligible guest does not take the client's false "ammo full" branch solely
because of the companion flag. This does not convert drops into per-player
multiplayer loot; the native shared pickup lifecycle remains authoritative.

`ME3_SQUAD_COOP_GUEST_AMMO_PICKUP=0` disables this adapter independently of
movement, weapon and HUD options. Native fixtures cover positive/negative
admission policy, cursor-only selection, refusing a wrong opcode or cursor, and
original forwarding/LastError. They do not prove a real touch, awarded reserve
ammunition, pickup consumption, native client feedback or replication.

Later owner checks:

- With a finite normal weapon below its reserve maximum, walk over a dropped
  ammo pickup as the guest. Check reserve gain, sound, pickup removal and the
  host view. Repeat while another weapon is equipped; native eligible inventory
  weapons should receive the authored amount.
- Repeat at full reserves, then while holding a heavy weapon with another
  normal weapon in the inventory. Capacity/exclusion behavior must follow the
  stock predicate and must not create extra ammunition or repeated pickups.
- Check an inactive/pooled pickup and a real scene or guest-control handoff.
  General inventory pickup permissions must remain unchanged. A pickup log
  means the stock capacity branch was selected, not that ammo was awarded.
- Correlate native `runtime_guest_ammo_pickup` events, sampled pickup permission
  and existing magazine/reserve observations. Successful native refill is
  separate from the still unresolved clip-count rebound during firing.

No game has run this addition; complete multiplayer/campaign parity remains
unproven.
