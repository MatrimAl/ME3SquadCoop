# Guest collision restoration candidate

This default-off candidate addresses an actual release/reacquire failure. A
paired read-only recorder observed guest Ashley losing world and actor collision
when control was released. Those settings stayed disabled when control returned;
the guest fell, and authoritative movement later fell too. An inventory loss was
not required. The implementation still needs a protected positive game test.

Set `ME3_SQUAD_COOP_COLLISION_RESTORE=1` only for controlled research. The same
implementation is included in the protected probe and ordinary entry. It first
requires healthy, acknowledged guest ownership of Ashley on the actual native
channel. It captures complete pawn, controller, world, channel/connection/driver,
class-default and collision-cylinder/template identities. The observed healthy
physical shape must equal the actual supported class/template defaults.

The Actor blocking mask covers only reflected `bBlockActors` (0x1); its
neighboring flag is neither part of the collision contract nor restored. A
measured return can retain world collision while actor/blocking collision is
absent, with the healthy cylinder response and type0. That exact partial shape
is also eligible for the existing release/reacquire policy. Native type6 keeps
an existing world-collision flag; the compare-exchange enables it only if absent.

While that same pawn is unowned, two matching disabled snapshots record the
released state. Repair requires a later registered ownership token, reciprocal
local possession, the same native channel lifetime and objects, ordinary game
control, a living initialized pawn, no pause/conversation, and an unchanged
released snapshot. Missing or changed context never authorizes a write. A
generation receives at most one attempt, with no continuous reassertion.

The ordinary runtime keeps the session-lifetime gate separate from permission
to write on a particular frame. A scene or `bPlayersOnly` interval suspends
repair while preserving the healthy baseline and released-state observations.
An actual runtime disable or loss of the native session scope retires them.

The repair dispatches the actual reflected native `Actor.SetCollisionType` with
the captured healthy type, `COLLIDE_BlockAllButWeapons` (6). It verifies the
supported native entry, callsite and one-byte parameter ABI. This engine method
updates actor/component collision and attachment state even when the type was
already 6. All component-array members and their identities are checked around
the call. The method does not restore `bCollideWorld`: a writable, exact-value
compare-exchange restores only that captured bit, preserving unrelated bits.
Post-call verification requires the captured healthy physical shape and the
native method's corresponding replicated-type value. A logged healthy shape
does not establish floor contact, synchronized movement, firing or combat.

Disabling or retiring the candidate stops observation/repair. It does not
deliberately put a still-controlled pawn back into the invalid collision state.
Normal engine scene/release behavior remains responsible for later changes;
the adapter never forces collision while unowned or in a scene. Game exit
removes all runtime effects. No game, configuration or save file is written.

The native fixture covers captured healthy/disabled shapes, altered component
responses, stale compare-exchange expectations and preservation of unrelated
bits. These are offline checks. Real release/reacquire, movement, disconnect,
actual conversations and ordinary-entry integration still require game evidence.

The separate lifecycle fixture exercises the actual adapter against temporary
objects, with explicit stand-ins for engine discovery and native dispatch. It
covers retained observation through a scene/PlayersOnly interval, two released
samples, a later ownership token, one repair attempt, true disable, changed
identities/channel/template/released state, and ABI refusal. Run it without
launching a game:

```powershell
python tests/native_collision_lifecycle.py C:\Tools\zig-0.15.2\zig.exe
```
