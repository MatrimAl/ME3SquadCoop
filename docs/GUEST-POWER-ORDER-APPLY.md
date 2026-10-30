# Complete the native own-pawn power order

The guest command bridge called `BioPlayerController.SquadOrderUsePower` and
reported its return. The retained native body shows that the own-pawn branch
writes `m_currentOrder` and returns success; it does not activate the power.
The stock `SFXGameModeDefault.UsePower` script follows that call with
`ApplyTacticalOrders`. That script consumes the pending order through
`BioPlayerInput.ActivatePower`, preserving its normal cooldown, action and
native client/server power-start checks.

After an accepted guest order, the bridge now reacquires the current controller,
pawn, input, catalog, world, registration and channel scope. It validates the
existing zero-parameter `ApplyTacticalOrders` method and reflected `PlayerOrder`
layout. Only the same pending power name, with no weapon-switch order and an
unchanged order snapshot, may enter that stock continuation once. A consumed
or replaced order, changed ownership/world, active drag, failed read or invalid
method leaves the continuation untouched. The adapter never clears the order
itself, starts a second RPC, changes a cooldown or bypasses native power checks.

`native-tactical-order-applied` means the normal continuation was dispatched.
It does not prove that its native action checks accepted the cast or that a
host-side effect occurred. The existing power-start observer retains those
separate boundaries for the next owner test.

The production dispatch fixtures cover a valid pending order, repeat delivery
after native consumption, changed name instance, target, camera snapshot,
weapon order, refused scope and unreadable memory. They check the exact event
arguments and LastError forwarding. These are offline checks; actual keyboard
casting, cooldowns and host visibility remain gameplay checks.
