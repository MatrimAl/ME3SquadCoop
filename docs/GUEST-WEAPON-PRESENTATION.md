# Guest weapon and shield presentation

The ordinary runtime adapts two specific NPC weapon defaults while a human
owns the existing companion and weapon. Set
`ME3_SQUAD_COOP_PLAYER_WEAPON=0` to disable this adapter.

The retained `GetFireModeBaseDamage` script applies `DamageHench` whenever the
instigator is a henchman class, even when a human controls it. The adapter
borrows a value of1 for that multiplier. This removes that particular NPC
damage reduction; it does not change the weapon's base damage, upgrades,
ammunition or character powers.

`InitializeWeapon` assigns FullAuto2 for a non-human instigator; its human
branch leaves the authored selector unchanged. FullAuto2 is also a legitimate
human default and is preserved. The adapter can borrow only the same weapon
class's authored SemiAuto1 or Burst3 default, after checking its stock firing
state and fire-type entries. None0, Reload4 and unknown selectors are refused.
`SendToFiringState` indexes both arrays with this selector and assigns it to
`CurrentFireMode`; zero there means None/idle. The legacy Engine `FireInterval`
array may be empty: `SFXWeapon.GetFireInterval` instead uses
`60 / GetRateOfFire()`, so the adapter does not require that unused array.
It waits for stock Active state, no firing/pending input,
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

`ME3_SQUAD_COOP_GUEST_SHIELD_FEEDBACK=0` disables this addition. The live
owned Ashley shield is `SFXShield_Energy_Player`, derived from
`SFXShield_Player`. Its scalar shield RepNotify does not execute the player's
damage-time low-shield extra. The adapter now admits player shields and uses
their own reflected critical particle and threshold. Only a genuine
`ReplicatedShield` decrease crossing that threshold with shields remaining
queues the low effect. `MaxShields` changes are not hits.

Player shield breach already reaches native `Player.BreachShields`, including
its effects and audio. Zero/breach cancels pending low feedback and adds no
second player break effect. Nonplayer shields retain the canonical player
critical/break cosmetics as a fallback.

The real `ReplicatedEvent` is forwarded once unchanged. Deferred Tick consumes
the cosmetic under the same pawn/shield/world/channel and asset lifetimes,
within500ms; duplicates, regeneration and replacements cannot replay it.
Only stock cosmetic functions are called. No damage/breach replay, shield
value writes, player-only `PlayHit` call or animation-tree changes are added.

This correction has not run in a game. Ordinary hit-location particles,
direction indicators and every hit sound remain separate scope; low/break
cosmetics do not establish full Shepard/MP feedback parity.

## Validation

Four native targets and runtime/probe fixtures cover camera action/mode
policy, conditional field writes and adjacent-byte preservation, external
changes, pass-cache identities, shield threshold/duplicate/regeneration
decisions and exactly-once RepNotify forwarding/LastError. The owner confirmed
reload/power camera distance and the early Shepard-passage fence in the
protected `af2880c` session. The corrected fire-selector and player-shield
admission have not run in a game. Full Shepard/MP weapon parity remains
unproven.

At the next protected test:

1. Let Shepard pass the early shuttle walkway first; the guest movement fence
   should stay open for that live volume while guest story activation remains
   host-only.
2. Reload with R and use an Ashley power. The camera should keep its normal
   shoulder distance; native aim/cover transitions and a real scene must work.
3. Check guest fire/release/reload, weapon switching and damage. Then enter a
   real conversation or disconnect and check normal AI weapon behavior.
4. With Ashley's player shield, take damage across the positive low threshold,
   then to zero. Check one low effect and the existing native break effects,
   followed by regeneration; capacity changes must not produce hit effects.
   This player-shield correction still needs its first gameplay check.
