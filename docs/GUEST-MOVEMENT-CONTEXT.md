# Guest movement and animations

The target is Shepard-like movement and animation through the normal player
inputs. It prepares four directional evades plus eight player traversal slots
for cover slips, mantle and ladders. Full movement, animation blending and
mission parity remain unverified; gameplay confirmation is narrower than the
prepared source features.

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

## Player traversal preparation

A separate lease prepares the actual loaded player's mantle-over31,
mantle-up33, four cover slips36..39 and ladder up/down46/47. The working evade
lease is unchanged. Every row checks the exact authored AI and player class,
current pawn/controller/world/connection, CDOs and any existing idle instance.
An optional missing row does not block other prepared rows. Native geometry
queries, input, root motion, physics setters and replication execute the move;
the adapter never dispatches a move or writes a position/physics value.

Mantle and cover-slip BodyStance clips must already exist on Ashley's current
mesh. All required clips for a row are checked in one bounded mesh traversal.
Ladder Start/Stop instead retain their existing selected ladder-node AnimInfo
registration/unregistration path; the adapter does not preempt that lazy loader
or manufacture a missing asset. Normal native ladder OnUse/authority and
ServerUseLadder routes remain intact, including their existing events.

Stock VerifyCAHasBeenInstanced creates the player instance on real input.
Native array growth and individual replacements retain their own identities.
Retirement restores only still-owned rows, waits for running actions, and
keeps the core's story/AI handoff independent. A disposed old instance becomes
None so native AI can instantiate it under its current controller.

Camera retention reads the actual current player action and canonical ladder
camera request/cache. It keeps only an existing shoulder scalar under the
normal camera/scene policy; it does not set an angle, FOV, target or mode.
Native ladder cameras still calculate their view from the actual movement path.

Set ME3_SQUAD_COOP_PLAYER_TRAVERSAL=0 to disable these traversal rows. Source
fixtures and class/script evidence are not a movement gameplay pass. First-use
ladder animation loading, geometry/path replication, cover slips from both
stances, all mantle variants, camera recovery and scene/disconnect AI return
remain future owner checks. No game was launched for this preparation.
