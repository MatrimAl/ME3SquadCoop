# Shared ammo supply timing

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

All four native targets built. The ordinary runtime fixtures passed scalar
mapping, exactly-once forwarding and LastError preservation, plus owned/foreign
entry, tail and trampoline checks. The protected probe's existing memory,
opt-in and low-integrity write-denial checks also passed. A focused source
review found no native ABI or script-cursor mismatch.

No game ran this addition. The fixtures do not exercise full live admission,
actual sleep expiry, rendered respawn or visibility replication. Ammo quantity
still follows native `GiveAmmo`, difficulty `AmmoPct` and weapon `AddAmmo`;
absolute multiplayer award amounts were not established by this inspection.
Real reserve gain, shared contention and repeated replenishment on both peers
remain owner gameplay checks. This timing candidate does not establish complete
multiplayer ammo or campaign parity.
