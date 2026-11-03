# Use a power during a native reload

The guest command catalog previously required no current action. This rejected
every power key while reloading, before the stock power input could cancel
reload. The retained `SFXGameModeDefault.UsePower` script cancels the user's
weapon reload before its normal order, blocking-action and application checks.

The command catalog now also accepts an actual current `ReloadBase` instance
from the owned pawn's matching native class/instance tables. The same route
also admits the four verified [player evades](GUEST-EVADE-POWER.md); other busy
actions retain the existing restrictions. Dead, downed, frozen, root-motion override,
ignored input, full-scene and lost-ownership states remain excluded.

A queued power during that reload is sent once to the existing default mode's
`UsePower` event with the actual owned pawn and selected power name. This keeps
native reload cancellation, target selection, cooldown checks and server power
start together. Idle powers keep their existing order/application route. The
adapter does not end an action, write ammunition, start an animation, change
camera distance or send a replacement RPC itself.

Dispatch requires the current mode manager, its default mode, input permissions,
power catalog, mapped name, controller/pawn/channel scope and reload instance to
remain valid. The exact two-parameter method ABI is checked. A stale request or
failed validation is discarded. Status 5 means the native input event was
dispatched; it does not claim a successful cast or host-side effect.

Offline fixtures exercise the production event dispatcher, exact arguments,
expired/future submissions, refused scope, null inputs and LastError. Admission
checks cover action bounds, concurrent powers, ignored input, frozen/root-motion
states and death/downed/busy flags. These checks do not exercise an actual
reload or prove camera, ammunition or gameplay parity.

At the next owner test, start reload with a partly used magazine and press an
available power key while it is running. Compare the interruption, one actual
cast, native cooldown, camera distance and host-side effect. Repeat after moving
the power to another numbered slot. Also finish one ordinary reload without a
power input and check that its usual ammunition transfer is preserved.
