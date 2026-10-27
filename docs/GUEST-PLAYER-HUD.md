# Guest health, shields and normal power controls

The ordinary guest now enables a compact Ashley health/shield display and the
existing native keyboard power slots/icons by default. The display uses the
game's Canvas after the local BioHUD PostRender. It shows two bars and numeric
current/maximum values near the bottom center. It does not load a new movie,
change the stock target bars or change health/shield values.

Both peers now also have a teammate readout near the upper left: the host sees
Ashley, and the guest sees Shepard. It shows health/shield bars and numbers plus
`DOWNED - REVIVE` or `DEAD` from the native life flags. This reuses the same HUD
hook and validated value sampler; it sends no new network data. Remote values
are replica observations, so `--` remains possible and does not mean full/empty.
The readout does not require the private revive animation addon; the actual
revive action still does. `ME3_SQUAD_COOP_TEAM_HUD=0` disables this extra readout
independently of the guest's own HUD. Read-only remote scope capture remains
available when remote presentation and revive are disabled.

Teammate samples pin both local and remote pawns, controller, world, native
channel/index, connection, driver and session/registration token. Every draw
rechecks the current scope, and samples expire after one second. Disconnects,
replacement lifetimes, scenes and released ownership suppress the readout.
The host's existing Shepard HUD remains the source of its own player display.

Health comes from the possessed pawn's unique reciprocal damage module; shield
values come from its unique owned inventory shield. These are client-side
values, not proof of authoritative damage replication. Missing, ambiguous,
non-finite or invalid readings are shown as `--`. Each sample retains its pawn,
controller, world and registration identity, refreshes every 200 ms and expires
after one second. A new owner cannot inherit the old owner's sample. Scenes,
menus, disabled runtime and released ownership suppress drawing. Canvas pen
state is restored after the label, and all existing HUD events forward once.

Normal `UseAbility` keyboard commands use Ashley's named power slots. Native
quickslot icons, wheel context and selection adapters now default on with that
path. The planner supports `--no-native-power-hotkeys` and
`--no-native-power-icons` for comparison. The separate mapped gamepad command
candidate remains explicitly enabled with `--native-mapped-powers`; its actual
physical alias delivery is not established.

The power context adapter supplies the controller caches of already existing,
idle Ashley-owned power instances. It accepts an empty player-controller cache
and either no AI or a validated detached same-world AI. It never replaces a
different player controller, creates actions or changes the pawn binding.
Each conditional pointer change is recorded. Cleanup restores only the same
expected fields on idle live objects, preserves external/native replacements
and defers while an action or scene is active. No rank, cooldown, action array,
target or native power restriction is changed. The ordinary order queue still
performs its existing readiness and native acceptance checks.

Before launch, individual diagnostics can disable these additions:

- `ME3_SQUAD_COOP_GUEST_VITALS_HUD=0`
- `ME3_SQUAD_COOP_TEAM_HUD=0`
- `ME3_SQUAD_COOP_POWER_ACTION_CONTEXT=0`
- `ME3_SQUAD_COOP_NATIVE_POWER_HOTKEYS=0`
- `ME3_SQUAD_COOP_NATIVE_POWER_ICONS=0`

All four native targets rebuilt, native fixtures and five focused session-plan
tests passed. This build has not been run in a game. The owner will check that
the bars appear, track damage/recharge, disappear during scenes, and that normal
power keys display/select Ashley powers and produce native cooldown/effects.
These implementations are progress toward LEMP/Shepard-style guest controls;
they do not establish complete multiplayer combat or campaign support.

## Authoritative power controller context

The host now also prepares existing Ashley power instances for its actual remote
player controller. This was previously a guest-only adapter. The host path uses
the captured story/possession lease, session epoch and exact native actor channel,
connection and driver; it does not need a local PlayerInput on that remote PC.
It reuses read-only admission checks without depending on the revive setting or
private animation addon. Shepard's own power instances are outside this scope.

Only eligible, already owned, idle power objects with an empty PC cache are
considered. A nonempty AI cache must be the exact captured original AI, still
live and detached. The existing reflected field checks and conditional pointer
writes apply on both peers. No rank, cooldown, target, manager array or action
lifecycle is changed. Scene/disable/disconnect requests retirement before the
core's handoff; busy cleanup stays pending and never delays story/AI restoration.
An idle stamped action can clear the exact old PC cache after that PC is disposed,
without dereferencing it. A different owner or native cache replacement is kept.
The same `ME3_SQUAD_COOP_POWER_ACTION_CONTEXT=0` disables preparation for comparison.

Native fixtures cover host epoch/channel/manager/power identity refusal, the
missing-local-input contract, detached-original-AI policy, disposed-PC idle
cleanup and foreign-owner restoration refusal. This does not establish a live
power effect. The owner should cast an Ashley keyboard power at a valid enemy,
check host-side impact/cooldown, then verify normal AI powers after disconnect
and powers after reconnect. Gamepad work is deferred at the owner's request;
its existing opt-in candidate is unchanged.
