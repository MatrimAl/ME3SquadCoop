# Power input research

Guest power use is **unverified**. No power remapping, forced unlock,
cooldown change or replacement transport has been implemented.

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
