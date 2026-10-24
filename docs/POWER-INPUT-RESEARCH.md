# Power input research

Power use by the guest is **unverified**. No power remapping, forced unlock,
cooldown change or custom power transport has been implemented.

The retained guest input capture maps keyboard keys One through Eight to
PC_HotKey1 through PC_HotKey8, then to `UseAbility 0` through `UseAbility 7`.
The captured gamepad aliases use `TryUsePower` instead. Testing only that
gamepad command cannot establish that keyboard power input works.

The external [LExSDKv2 LE3 reference](https://github.com/llamathings/LExSDKv2/tree/000d7c3799f8f9de5db276b49008b539f5f97830/Src/LESDK/LE3)
provides these leads, not exact-build runtime proof:

| Entry | Declaration evidence | Remaining uncertainty |
| --- | --- | --- |
| `SFXGameModeDefault.UseAbility` | Integer slot; GUI/power-wheel locals | Actual slot and controlled-pawn resolution |
| `SFXGameModeDefault.TryUsePower` | Integer slot; player-specific pawn local | Whether the branch excludes a controlled squad member |
| `SFXGameModeDefault.UsePower` | Power name and an `SFXPawn` user | Target, action and permission checks |
| `BioPlayerInput.ActivatePower` | Power name, target/camera parameters; `BioPawn` local | Exact body, return meaning and accepted native action |
| `BioPlayerController.ServerStartPowerCustomAction` | Existing server RPC declaration | Call chain and server acceptance for guest Ashley |

The generated function files are ProcessEvent wrappers, not decompiled gameplay
bodies. A typed local or an RPC name does not establish executable control flow.
Do not call these through assumed layouts or create replacement transport from
this reference alone.

Next capture the exact supported build's methods and power-wheel hotkey bodies,
then test an available Ashley power through the intended input route. Evidence
must include delivered input, native action acceptance, cooldown behavior and
an authoritative effect on the intended target. Preserve native power rank,
target, cooldown and ownership restrictions.
