# Shared ammo supply

Selected retained LEMP Cerberus and Noveria map containers explicitly use a
three-second `RespawnTime`. The two inspected Mars containers use the same
respawning heatsink archetype and inherit the class default of twenty seconds.
This is evidence from the inspected LEMP reconstruction and campaign packages,
not proof of every original multiplayer map or campaign supply point.

The stock container grants ammo to the touching pawn, then enters `Sleeping`.
That state hides the ammo mesh, ignores touches, sleeps for `RespawnTime` and
returns to `Pickup`. The authority replicates the shared mesh visibility.
These inspected containers have one shared availability cycle; they do not
create an independent pickup for each player.

`runtime_supply_sleep.h` changes only the decoded native `Sleep` argument from
twenty to three seconds for a narrowly admitted existing host container. It
requires the supported executable, exact native caller, container class and
respawning archetype, authoritative role, current world, `Sleeping` state and
state-frame cursor, matching twenty-second instance/default values, and one
connected native co-op peer. Native code performs the resulting latent sleep.

The existing grant, capacity checks, sounds, shared visibility and state
transitions keep their stock execution. The adapter does not write ammunition,
`RespawnTime`, global multiplayer state, script/state-frame storage or assets.
Standalone play, ordinary dropped ammo, nonrespawning containers and other
sleep/timer values retain their original path. No LEMP assets are required by
the runtime. `ME3_SQUAD_COOP_MP_SUPPLY_RESPAWN=0` disables this timing adapter.

For this timing adapter, all four native targets built. The ordinary runtime
fixtures passed scalar
mapping, exactly-once forwarding and LastError preservation, plus owned/foreign
entry, tail and trampoline checks. The protected probe's existing memory,
opt-in and low-integrity write-denial checks also passed. A focused source
review found no native ABI or script-cursor mismatch.

The installed LEMP difficulty configuration also establishes a quantity
difference in the common `GiveAmmo` path:

| Raw difficulty ordinal | Campaign AmmoPct | LEMP AmmoPct |
| --- | --- | --- |
| 0 | 0.75 | 0.5 |
| 1 | 0.6 | 0.5 |
| 2 | 0.4 | 0.5 |
| 3 | 0.25 | 0.25 |
| 4 | 0.25 | No explicit MP tier established |

LEMP copies its difficulty setting directly into the handler's difficulty
ordinal. The lookup selects the matching numbered configuration array, and
each listed range has equal endpoints, so difficulty score does not alter
these values. This does not establish a mapping between campaign and MP
difficulty labels.

A separate quantity candidate applies only to an admitted authoritative award
to human Shepard or the owned human Ashley in a connected co-op session. For
ordinals zero through two, it supplies a frame-local award percentage of 0.5
only when the actual base handler still has the corresponding stock campaign
value of 0.75, 0.6 or 0.4. Ordinal three already matches and follows stock code;
ordinal four, unknown ordinals and custom values also keep their original path.
The candidate reuses the existing guarded script executor hook.

The original inventory loop still excludes heavy weapons and awards
`int((MagazineSize + MaxSpareAmmo) * AmmoAwardPct)` through stock `AddAmmo`.
Its reserve cap and inactive-weapon magazine replenishment remain native.
There is no global `AmmoPct`, ammo-counter, asset or multiplayer-sound flag
write. All four targets built. Quantity fixtures passed the ordinal/value policy, local-frame selection and preservation, invalid layout/cursor refusal, conditional rollback, and selected/unselected executor forwarding with LastError. These fixtures do not establish live admission or actual awarded rounds.

No game ran these additions. Timing fixtures do not exercise full live
admission, actual sleep expiry, rendered respawn or visibility replication.
The percentage evidence does not establish per-weapon capacity parity or
actual awarded rounds. Real reserve gain, shared contention and repeated
replenishment on both peers remain owner gameplay checks. Complete multiplayer
ammo and campaign parity remain unproven.
