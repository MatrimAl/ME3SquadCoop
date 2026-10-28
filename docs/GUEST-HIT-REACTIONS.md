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

## Incoming melee and stagger follow-up

The next extension prepares the same player's stagger family87..91 and
independent direct-melee reaction96. The native angle selector can choose
both87 and88 for one direction, so the whole five-entry family is prepared
together. Ashley's NPC StaggerImpactII in88 becomes None while human-owned,
matching the actual player table.87/96 use PlayerMeleed,89 uses
KnockbackImpactForward, and90/91 use MeleedLeft/Right. The already matching
knockback92..95 and melee97..99 entries remain under native ownership.

All new classes inherit DamageReaction and retain its normal Start/Stop,
stance notification, timeline and root-motion cleanup. Required clips must
already be on the actual mesh. PlayerMeleed permits movement; the other
authored reactions retain their own movement interruption and root-motion
policies. The adapter does not remove stun, damage, cooldown or native input
restrictions. The standard and stagger families each require their own complete
preparation mask; a missing stagger clip does not disable the light-hit family.

These reactions request the exact native HitReaction camera, which inherits
Combat without its own overrides. Existing shoulder-scale retention checks the
actual action/controller and the requested/cached camera class, target pawn
and native flags. The native camera's transition, offset, FOV and shake remain
unchanged. Native IgnoreMoveInput during the action is permitted only with
that verified action's movement-disable flag; the existing scene, mode and
ownership checks still apply.

Native fixtures extend missing-mask, foreign-row partial apply/restore/retry
and NULL alternate cases to this five-row family, with camera identity and
negative request/target policy cases. No game ran these additions. Actual
incoming melee/stagger, movement/aim recovery, peer animation/root motion and
scene/disconnect AI recovery remain owner checks. Fire, explosion and death
reaction parity remains separate work.

Slot96 matches the actual player table, but the selected GetPossibleReactions
script calls its GetMeleeReactions branch only for nonplayers. A direct native
incoming-hit route for96 remains unproven. Likewise, borrowing classes does not
add SFXPawn_Player's separate in-cover GetKnockbackReactions refusal to Ashley;
the separate guard below handles that rule. This does not establish complete
incoming-reaction parity.

## Player cover rule

Stock SFXPawn_Player.GetKnockbackReactions returns false while IsInCover is
true, leaving its output candidate array unchanged. Ashley inherits the generic
BioPawn selector, so matching action classes alone cannot supply that rule.
The reaction adapter now adds a separate scoped selector guard on either peer.
It requires the actual registered human Ashley, native player bit, current
world/connection generation, exact function/parameter metadata and VM body,
and the selector frame's current Node, Object and starting Code identities.

The exact native IsInCover predicate is read from reflected cover fields under
the supported getter and leaf machine-code pins. There is no extra native
getter call. If it is true, only this call's Code cursor moves to the same
selector's existing explicit ReturnFalse instruction. The original executor
runs once with unchanged call arguments and result buffer. The supported
executor consumes that cursor; it does not reset to the function start. The
original VM supplies the return value. No candidate array, parameter, result,
damage, cover state or animation is written by the guard.

This matches the player's cover branch and allows the existing heavy-reaction
fallback to choose stagger normally. It does not cancel damage, remove all
stun or change the selector outside eligible human cover. Host Shepard, AI,
another pawn, a scene/ownership loss, an unsupported body or a moved frame
cursor continue through the original executor unchanged. The rule does not
require an idle animation or successfully borrowed reaction clips.

ME3_SQUAD_COOP_PLAYER_REACTIONS=0 disables this rule along with reaction
preparation. Native fixtures cover cursor-only selection, negative cursor/
opcode cases, unchanged forwarding/LastError and the exact cover predicate.
Static exact-image cursor proof and fixtures are not gameplay validation.
Next test: take a knockback-capable hit inside and outside cover, compare
damage and fallback stagger, verify natural cover exit, then scene/disconnect
and AI recovery on both peers. No game was launched for this addition.

## Native fire reaction

The next addition prepares the two absent player fire-reaction entries103/104
as one independent pair. Both use the current world's loaded
SFXGameContent.SFXCustomAction_PlayerOnFire class. Native GetPossibleReactions
can select either during Reaction_Fire; no mod-generated hit, fire effect,
damage tick or reaction request is added. Missing fire assets do not disable
the earlier standard/stagger families. Existing native entries win.

The retained authoritative FireDamageOverTime path also reaches ImpactWithPower
and PickFireReaction directly. It requires the native resistance/large-reaction
conditions and PowerReactionChance before selecting103/104. The adapter does
not bypass those decisions. This is a separate route from RequestReaction;
a direct fire-effect RequestReaction(4) caller was not established.

PlayerOnFire inherits DamageReaction without its own functions. Its normal
root motion, four-second cooldown, priority, vocalization and timeline effects
remain authored. It has no requested camera mode; the stock timeline supplies
its shake/rumble/blur effects. The existing retention policy only keeps an
already-held shoulder scalar for the actual owned reaction with no action
camera. Normal action movement/input restrictions remain unchanged.

The required clip is DG_Impact_OnFire_2. Its native FName is the base
DG_Impact_OnFire with Number3. The shared clip reader now accepts nonnegative
native name numbers and compares the complete index/number pair with the actual
mesh's SequenceName. It does not strip the suffix, alias another clip, load a
new asset or replace an animation tree. All previous unnumbered checks retain
their exact identity. Sparse BodyStance entries are not changed.

Each optional row checks its source/current table bounds before preparation,
observation and mutation. A shorter table cannot expose an out-of-range fire
slot or prevent the older rows from working. Native fixtures cover the pair's
complete mask, empty-slot borrowing, foreign second-row failure/rollback/retry,
short-table refusal and numbered-name acceptance/invalid-number refusal.

No game ran this addition. Actual first fire hit, timeline/animation, cooldown,
native input/root-motion recovery, peer presentation and scene/disconnect AI
return remain owner checks. Complete fire/damage and multiplayer/campaign
parity is not established by borrowing these classes.
