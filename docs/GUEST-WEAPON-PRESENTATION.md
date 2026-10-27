# Guest weapon and shield presentation

The ordinary runtime adapts two specific NPC weapon defaults while a human
owns the existing companion and weapon. Set
`ME3_SQUAD_COOP_PLAYER_WEAPON=0` to disable this adapter.

The retained `GetFireModeBaseDamage` script applies `DamageHench` whenever the
instigator is a henchman class, even when a human controls it. The adapter
borrows a value of1 for that multiplier. This removes that particular NPC
damage reduction; it does not change the weapon's base damage, upgrades,
ammunition or character powers.

`InitializeWeapon` also assigns default fire mode2 for a non-human instigator,
without undoing that choice later on possession. The adapter borrows the same
weapon class's authored human default, after proving its fire-state/type/
interval entries. It waits for stock Active state, no firing/pending input,
no custom action and no weapon switch. Initialization is not replayed. Pending
and current fire modes, ammunition and live fire requests are not written.

Both fields are conditional leases on the actual owned weapon and native
connection/channel. Existing native changes win. Captured values are restored
before the host hands Ashley to AI, and on disable/ownership loss when the
same objects still admit restoration. A replacement weapon or owner cannot
inherit the old lease. Fire-mode writes preserve adjacent bytes.

The camera adapter separately retains its existing shoulder scale through
validated native reload and power actions, and combat/aim/cover camera-mode
transitions. The recorded reload action9 previously caused the scale to be
restored from1 to0. Retention uses the actual action table, class and reciprocal
human possession; historical action PC/AI caches are not executed or changed.
It does not change native FOV, aim selection, camera modes or animations.
Initial acquisition remains idle-only and full scenes still yield control.

## Player shield cosmetics

`ME3_SQUAD_COOP_GUEST_SHIELD_FEEDBACK=0` disables this addition. A companion's
base shield does not include the local-player extras of `SFXShield_Player`.
The adapter observes the owned shield's real network `ReplicatedEvent`,
forwards it once, then queues a cosmetic after the actual shield decrease.
The next normal Tick consumes it under the same pawn/shield/world/channel
identities, at most500ms later.

The player's canonical low-shield threshold supplies its critical particle.
A transition to zero also supplies the stock `CE_ShieldBreak` effect. The
adapter calls only the existing base `ActivatePSC` and client-effect `Play`.
It never calls player-only `PlayHit` on a companion, replays damage/breach,
changes shield values, adds time dilation or forces an animation tree.
Duplicate zero notifications, regeneration and changed identities cannot
replay the queued cosmetic. Already-player shield instances are excluded.

This covers those low-shield and break effects, not every possible hit sound,
directional impact or damage animation. Missing contracts/assets are logged
and leave stock behavior. Actual dispatch and rendered appearance still need
the owner's gameplay check.

## Validation

Four native targets and runtime/probe fixtures cover camera action/mode
policy, conditional field writes and adjacent-byte preservation, external
changes, pass-cache identities, shield threshold/duplicate/regeneration
decisions and exactly-once RepNotify forwarding/LastError. No game has run
these new integrations yet. Full Shepard/MP weapon parity is not established
by these individual fixes.

At the next protected test:

1. Let Shepard pass the early shuttle walkway first; the guest movement fence
   should stay open for that live volume while guest story activation remains
   host-only.
2. Reload with R and use an Ashley power. The camera should keep its normal
   shoulder distance; native aim/cover transitions and a real scene must work.
3. Check guest fire/release/reload, weapon switching and damage. Then enter a
   real conversation or disconnect and check normal AI weapon behavior.
4. Take shield damage through the low threshold and to zero; check the native
   critical/break visuals once, followed by normal regeneration. Cosmetic
   observations do not substitute for actual damage verification.
