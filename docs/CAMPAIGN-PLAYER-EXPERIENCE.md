# Campaign co-op player experience

The target is campaign play with guest character control that feels like the
official ME3 multiplayer: direct movement, aiming, weapon use and powers through
ordinary gameplay controls. The host remains Shepard and owns story choices,
world progression and saves. Guests control the existing campaign squadmates.
This is the intended experience, not a claim that it currently works.

## Completion criteria

| Player action | Required behavior | Current evidence or gap |
| --- | --- | --- |
| Move, turn and use cover | Responsive local control and consistent authoritative position through sustained combat and scene return | Short replicated movement observed; sustained walking and position continuity remain unverified |
| Aim and fire | Normal shoulder view, weapon handling, reload and reliable press/release behavior; host-confirmed enemy effects | Native firing and a prior attributed hit observed; repeatability, stopping and equipment readiness incomplete |
| Use powers | Normal keyboard and gamepad actions, native targeting and cooldowns; authoritative effects | Native routing researched; explicit scoped request candidate compiled; actual casts and ordinary bindings unverified |
| Get downed and recover | Native incapacitation and revival, followed by usable control without manual reconnect | Admission guards implemented; ownership and revival continuity unverified |
| Watch dialogue/cinematics | Native scene presentation, host story authority and automatic return to stable guest control | Limited scene handoffs observed; full conversations, collision and sustained return incomplete |
| Change missions or reload | Reestablish the guest's campaign character without repeating per-mission configuration | Optional recovery scheduling compiled; actual campaign transitions unverified |

The session panel and opt-in Ctrl+Alt power shortcuts are supporting tools. They
do not satisfy normal gameplay input or the multiplayer-style control target.
Players should not need an external panel for individual combat actions.
LEMP's installed DLC and source provide the concrete
[multiplayer control reference](LEMP-CONTROL-REFERENCE.md).

## Native integration priorities

1. Resolve sustained movement/replication and equipment/fire-release continuity.
2. Connect normal keyboard and gamepad power actions to the possessed campaign
   pawn, preserving native cooldown, targeting, rank and action checks.
3. Complete downed/revival and real dialogue return without losing that control.
4. Carry the same behavior through campaign transitions and two-PC sessions.

The retained input research provides a specific power-route comparison:
`SFXPlayerControllerMP.CastPower` in the separately retained LEMP reconstruction
uses the possessed pawn and the existing `SquadOrderUsePower` route. That is
corroborating research, not proof of the original multiplayer runtime's complete
behavior. The owned campaign keyboard route reaches the same native order
through the PC power wheel, whose quickslots/icons require initialization.
The retained gamepad `TryUsePower` script instead casts to `SFXPawn_Player`.
An Ashley possession alone does not satisfy all those paths. See
[power input research](POWER-INPUT-RESEARCH.md).

Do not infer compatibility from MP class names or replace the campaign pawn or
controller merely to satisfy a cast. Native script/action caches, replication,
campaign roster and story authority must remain coherent. The reference is the
player's control experience; its implementation needs verified native contracts.

Gameplay validation remains deferred at the owner's request. Source inspection
and compilation do not meet these completion criteria.
