# Guest player hit reactions

The ordinary runtime prepares the loaded player's native light-hit and
shield-breach animation reactions while a human owns Ashley on either peer.
This follows the same current world, pawn/controller, connection/channel and
native player-bit admission as the other owned movement adapters. Set
`ME3_SQUAD_COOP_PLAYER_REACTIONS=0` to disable this addition.

The actual player table selects PlayerStandardImpact at81 and its forward,
left and right variants83..85. Ashley's authored counterparts are NPC reactions
with different clips, root motion and movement/camera interruption policy.
The player variants leave movement available. Their sparse BodyStance values
remain intact; the adapter checks the three nonempty required clip names on
the current mesh and borrows the existing classes rather than building an
animation tree or changing their settings.

Slots82 and86 are absent for the player. Native GetStandardReactions otherwise
can choose the alternate NPC82, and the frontal leg-hit branch independently
can choose86. The six-entry standard family is therefore prepared together:
all exact authored classes, player classes or absent entries, CDOs, idle
instances and clips must pass before any entry changes. On a failed compare/
exchange the owned changes retire; a foreign class or instance is preserved.
This does not suppress damage or alter reaction probability/cooldown policy.

The independent slot100 supplies PlayerShieldStandardImpact. The existing
authority RequestReaction(5) after shield damage can select it once present.
Its own replicated random selector chooses the native left/right upper-body
animation, without blocking movement, shooting or aiming. The adapter does
not invoke damage, RequestReaction, Start/Stop or another shield effect.
The already confirmed low-shield and breach cosmetics remain independent.

Native authority, angle/hit-part selection, cover eligibility, lazy instancing,
animation notification/timer lifecycle and replication remain unchanged.
Retirement waits for owned running instances and restores only still-owned
entries. A controller cache rebound on the same action object is a foreign
owner even if its UObject stamp did not change. Story/AI handoff is not delayed.
Camera retention only preserves an existing shoulder scale for the exact
current player reaction, reciprocal local controller and absent action camera.
It does not set FOV, camera mode or an angle.

Native fixtures cover the six-row preparation boundary, partial apply failure,
NPC-to-None and None-to-player restoration, foreign instance refusal and
camera identity policy. Source/package evidence is not gameplay validation.
No game was launched for this change. Native RequestWeaponReaction internals,
actual first-hit/first-breach animation, peer result and scene/disconnect AI
recovery remain owner checks. Heavy knockback, fire and death reactions are
outside this change; complete multiplayer/campaign parity remains unfinished.
