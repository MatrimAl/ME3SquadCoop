# Power input research

Guest power use is **unverified**. No power remapping, forced unlock,
cooldown change or replacement transport has been implemented.
An explicit ordinary-runtime request API is now implemented as a compiled
candidate below; no player binding or live cast has been verified.

## Evidence from the supported package

A private offline read of the owned SFXGame package extracted11 selected
function exports. Their serialized script and expanded-memory lengths matched,
and decoded branch targets landed on decoded instruction boundaries. Input
package and compression-library hashes were unchanged. This is static evidence;
it does not establish that a power was accepted in a running campaign.

- Retained guest bindings map keyboard One through Eight to PC_HotKey1 through
  PC_HotKey8, then `UseAbility 0` through `UseAbility 7`.
- The actual `SFXGameModeDefault.UseAbility` script checks the combat getter,
  resolves the power-wheel GUI for its own controller and calls `doHotKey`.
  The PC-specific `doHotKey` implementation is native; its script is a bridge.
- `ApplyTacticalOrders` uses the controlled `BioPawn` and calls the local
  `BioPlayerInput.ActivatePower` for a pending power order. The latter rejects
  missing manager/power, a disabled power and active cooldown, then requires
  the selected power in that pawn's power-action list before starting it.
- `BioPlayerController.StartPowerCustomAction` uses its own controlled
  `BioPawn`, applies prediction/action checks and, for the autonomous client
  role, sends the existing server RPC after a successful can-start result.
- `ServerStartPowerCustomAction` resolves the server controller's own pawn as
  `BioPawn` and calls its normal power-action method. It does not accept a
  replacement caster pawn argument.
- The captured gamepad aliases instead use `TryUsePower`. That script casts
  the controlled pawn to `SFXPawn_Player` before its FindPower/UsePower path.
  It cannot substitute for testing the keyboard route on a squad member.

## Exact native bridge findings

Read-only analysis of the supported executable resolved the two native bridges.
This combines registration data, startup pointer initialization, unwind bounds,
class vtables and disassembly; no native power function was invoked.

- `SFXSFHandler_PCPowerWheel.doHotKey`: registration thunk RVA0xb69570,
  implementation RVA0xc5ccc0. It requires the handler's controller, valid
  quickslot definitions and initialized cached power icons. Possessing Ashley
  alone does not establish those prerequisites.
- The inherited `SFXSFHandler_PowerWheel.UsePower` script forwards to
  `m_pPlayerController.GetGameModeDefault().UsePower`. Exact owned bytecode
  establishes this receiver; matching event names alone would not.
- `BioPlayerController.SquadOrderUsePower`: thunk RVA0xb64a20,
  implementation RVA0xaa38b0. The initial supplied-pawn cast accepts `BioPawn`.
  When that pawn equals the controller's controlled pawn, it stages the normal
  pending power order; the other-pawn branch issues a henchman order.

The static route is therefore `UseAbility -> doHotKey -> inherited UsePower ->
GameModeDefault.UsePower -> SquadOrderUsePower -> ApplyTacticalOrders`.
These are function-entry RVAs for the supported image only, not portable hook
signatures or permission to call a function through an assumed ABI. Native
unwind fragments must not be mistaken for complete function boundaries.

## Next verification

An offline comparison with the already-present LEMP reconstruction found a
concrete reuse candidate: `SFXPlayerControllerMP.CastPower` checks combat input
and the possessed pawn's power-list index, then calls the existing
`SquadOrderUsePower` with that power's name and the possessed pawn. Its wrapper
does not require an MP player-pawn cast. The selected script decoded completely
(156 serialized bytes, 244 expanded VM bytes). This corroborates reuse of the
existing squad-power route for Ashley; it does not establish native acceptance,
cooldown, targeting or damage in a live guest session. LEMP is a research source
for this comparison, not a runtime requirement.

Measure the actual guest controller's quickslot definitions and the wheel's
cached icons after acquisition. Then test an available Ashley power through
its mapped keyboard input, observing delivered input, action acceptance,
cooldown and an authoritative effect on the intended target. Preserve native
restrictions. Static acceptance of the pawn base class does not prove a cast
or a target effect occurred in a running game.

The external [LExSDKv2 LE3 reference](https://github.com/llamathings/LExSDKv2/tree/000d7c3799f8f9de5db276b49008b539f5f97830/Src/LESDK/LE3)
provided initial declaration leads. The primary
[LegendaryExplorer reader](https://github.com/ME3Tweaks/LegendaryExplorer/tree/b5e746233491ec5a7cbfc077685996af37a3d125/LegendaryExplorer/LegendaryExplorerCore)
provided package/bytecode format references. Extracted game data and research
readers stay private; no game assets or decompiled bodies are distributed here.

## Action-instance ownership follow-up

Additional owned-script analysis found that the power-action eligibility path
uses the controlled BioPawn's own action classes and instances. When a missing
action is instantiated, the script records its pawn and either AI or player
controller context. Existing populated instances skip that creation block.
Action start/stop subsequently use cached controller context for input-ignore
and reticle handling. The power-specific check also rejects ragdoll/recovery
and weapon equip/put-down states. These native gameplay restrictions remain
part of the intended power path.

This raises a concrete ownership question for previously AI-controlled squad
members: do existing action instances still reference their former controller
after possession? That has not been measured. Inherited/native refresh paths
have not been ruled out, so no rebind, array recreation or forced cast is
implemented from this finding. Six selected functions decoded completely with
matching serialized/VM lengths and valid branch targets; this remains offline
evidence and does not establish successful guest powers.

## Tactical-order call contract

A new read-only owned-package extraction verified the exact
BioPlayerController.SquadOrderUsePower declaration: native index0,
function flags0x24401 and nine linked parameter fields including its boolean
return. Input package and compression-library hashes were unchanged. The
declaration uses a NameProperty power name and the supplied pawn, followed by
optional queue, indicator, selection target, input method, desired target and
target location fields. Its return is an actual boolean field, rather than
proof of a completed cast or authoritative effect.

The retained SDK's parameter offsets require four-byte packing: the desired
target pointer is unaligned at0x24, the vector starts at0x2c and the return is
at0x38, for0x3c total bytes. `guest_power_order_contract.h` records this authored
ABI and has compile-time size/offset assertions. Its runtime predicate requires
the exact reflected method/native entry and all nine field names, types,
owners, dimensions, sizes, flags, offsets and boolean masks. The complete
declaring method identity is rechecked before returning success.

The header is included by the ordinary entry for compilation. No power order
is dispatched and there is no new power input binding. Runtime verification of
this contract, controlled-pawn power selection, current action-instance
ownership, native cooldown/target acceptance and actual effects remain pending.
The next implementation must resolve power choices from the possessed pawn's
current manager; it must not accept an arbitrary caller-supplied caster or
bypass the normal tactical-order/ActivatePower/server path.

## Explicit guest power request candidate

The ordinary entry exports `ME3SquadCoopPowerScope`, `ME3SquadCoopPowerCount`,
`ME3SquadCoopRequestPower(scope,index)` and `ME3SquadCoopPowerStatus`. Export
callers touch atomic state only. A scope identifies the sampled possessed
pawn/controller/world/manager, connection/driver/channel, player-registration
token and its current power-array entries/names;
scope changes cancel pending input. Scope0 means unavailable. The generation
never wraps into a previously usable value. The list is bounded to64 entries.
No keyboard/gamepad/menu binding is provided yet.
The separate local session-panel candidate now exposes explicit power requests;
it remains unverified and adds no native keyboard/gamepad remapping.

Tick consumes one request only after ordinary guest ownership and an initialized
player context are verified. It rejects invalid indices, native duplicate power
names, stale object identities, missing/unlearned ranks and action instances
still bound to another controller or AI. The reflected manager/power-property
layouts and the complete native order ABI must match. A fresh catalog and
control/scene checks run before dispatch, including after the native combat
getter. The native receiver is the local controller and the supplied caster is
always its possessed Ashley. Optional target fields remain unset for ordinary
native target selection. Accepted dispatches are limited to one per250ms.

The command invokes only the game's normal SquadOrderUsePower entry. It changes
no ranks, controller caches, cooldowns, permissions, targets or RPC routing.
Native return1 is reported as order acceptance; return0 is native refusal.
Neither is a completed cast, server acceptance, cooldown or target-effect verdict.
Status0/1/2/3/4 means idle/queued/rejected/native-accepted/native-refused.

The runtime catalog requires all sampled power entries to be valid instantiated
objects; a sparse or not-yet-instantiated native list leaves the API unavailable.
It does not initialize or rebind those objects. All-empty or stale controller
state therefore remains an observable rejection rather than forced power use.
Normal outputs compile, but no game, request fixture, native order or performance
measurement was run. Complete input mapping, catalog presentation, action-instance
readiness and authoritative effects remain unverified.
