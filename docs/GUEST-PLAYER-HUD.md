# Guest health, shields and normal power controls

The ordinary guest now enables a compact Ashley health/shield display and the
existing native keyboard power slots/icons by default. The display uses the
game's Canvas after the local BioHUD PostRender. It shows two bars and numeric
current/maximum values near the bottom center. It does not load a new movie,
change the stock target bars or change health/shield values.

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
- `ME3_SQUAD_COOP_POWER_ACTION_CONTEXT=0`
- `ME3_SQUAD_COOP_NATIVE_POWER_HOTKEYS=0`
- `ME3_SQUAD_COOP_NATIVE_POWER_ICONS=0`

All four native targets rebuilt, native fixtures and five focused session-plan
tests passed. This build has not been run in a game. The owner will check that
the bars appear, track damage/recharge, disappear during scenes, and that normal
power keys display/select Ashley powers and produce native cooldown/effects.
These implementations are progress toward LEMP/Shepard-style guest controls;
they do not establish complete multiplayer combat or campaign support.
