# Guest movement and animations

The target is Shepard-like movement and animation through the normal player
inputs. This candidate starts with the four directional evade actions. It
does not establish parity for walking, sprinting, cover, vaulting, ladders,
melee, transition blending or every mission.

Ashley's authored slots54..57 select HenchRoll actions. The stock player evade
actions in the same character package use the same `CB_EvadeLeft`,
`CB_EvadeRight`, `CB_EvadeForward` and `CB_EvadeBack` body-stance animations.
Their player direction, motion, prediction and camera policies differ from
the NPC actions. Merely rebinding a player controller on the NPC action cannot
provide the player action's behavior.

The runtime borrows only those four loaded player classes during verified
human ownership. It clears an idle action slot so the next normal input uses
stock `VerifyCAHasBeenInstanced` to create and initialize its player action.
An absent lazy instance cell is left for that normal path to allocate. Native
array growth follows the same captured pawn; each slot retains independent
ownership so a replacement in one slot cannot orphan the remaining leases.
No input is generated, action started, animation tree replaced or pawn
lifecycle replayed. Class/package, existing animation, array, world,
connection/channel and possession identities must match.

Conditional retirement restores the authored NPC slots. A running action
defers slot changes; story control release remains independent. Disposed old
instances are not restored, and stock code can recreate a missing instance.
Native or external replacements keep ownership of their values.

Set `ME3_SQUAD_COOP_PLAYER_DODGE=0` to disable this adapter. Native fixtures
passed the slot mutation/rollback, missing-cell, mutable-array, owner/lifetime
and busy-action policies. In `b515e98`, the owner reported good evades and
normal animation/movement return, with a remaining camera-angle difference.
The next candidate retains the existing shoulder scale through verified
PlayerEvade/Roll camera actions; it does not change native FOV or camera modes.
The reusable cached Roll mode after an ended action no longer counts as busy
unless that mode is actually driving the current camera/interpolation.

The camera correction needs a protected gameplay check: evade in each direction
from idle and aim; check direction, distance, animation and return to movement.
Then enter a real scene and check that human control and normal AI behavior
return. The owner should also exercise sprint, cover and vaulting to identify
the next actual difference. A passing slot fixture is not animation proof.
