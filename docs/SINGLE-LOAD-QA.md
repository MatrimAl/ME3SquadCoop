# Current-map entry and a single host map load

The existing paired probe's Mars prewarm/reload path first enters standalone
Mars, then executes `open BioP_ProMar?listen`. This repeats map startup and its
intro. Changing a console command or seeing many `World` objects does not prove
that the current campaign map loaded or that ordinary control is available.

The protected probe already offers a one-load research alternative: invoke
`pair_probe.run` with `reload_host_mars=False` (the default). Its host dispatches
`open BioP_ProMar?listen` directly. `refresh_host_map=True` remains independent
of that choice. The guest prewarm option still causes its own map entry before
joining. This direct-listen path is a research bootstrap, not verification of
the separate ordinary runtime's host startup.

The ordinary runtime's explicit `START=listen` is the intended way to keep the
existing standalone campaign world: it calls native in-place Listen only after
its story/control guards pass. It does not reload Mars or skip a conversation.
Its actual-entry protected game verification is still unfinished. See
[runtime integration](RUNTIME-INTEGRATION.md) for the independent DLC, native
interface, selected-driver and master package-map requirements.

## Observe the next protected run

`tools/observe_gameplay_entry.py` starts no process and opens the supplied PID
with the existing read-only executable-path/SHA verifier. Supply the exact test
PID emitted by the protected launcher, while its independent-copy, private
profile, original-file-lock and bounded-cleanup protections remain active:

```powershell
python tools/observe_gameplay_entry.py --pid <test-pid> --sandbox <independent-game-dir> --output <private-dir>/entry.jsonl --seconds 30 --interval 0.5
```

Start the observer as soon as the native process appears; cold startup may spend
much of a short test loading. A new output is created exclusively; an existing
file is never overwritten. Output contains raw object identities, so retain it
only in ignored private storage.

Each snapshot follows current GWorld, its persistent level's first BioWorldInfo,
the engine LocalPlayer, reciprocal controller/pawn, mode manager and scene state.
It checks live object identities and rereads structural fields; unknown or
changing state cannot qualify as clear control. The exact map name must be
`BioP_ProMar` at the root of the current world's full object path, compared
case-insensitively like FName. The supported live map uses `Biop_ProMar`.
Shepard is checked through its observed `SFXPawn_Player` ancestry; a generic
`BioPawn` or an assumed `BioPlayerPawn` class is insufficient. A loaded map
with no controller is reported separately from ordinary control.

Interpret the fields narrowly:

- `current_map_verified`: exact current Mars chain observed, not a UObject count.
- `control_gates_clear`: sampled mode 0, no cinematic bit, no PlayersOnly/paused
  bit, no Pauser and no active/activating conversation, with reciprocal live pawn.
- `ready_for_in_place_listen`: those sampled gates, standalone net mode, no world
  driver and no pending engine travel. This is an observation, not authorization
  to call an unverified native function.
- `ordinary_standalone_interval_observed`: at least two seconds of continuous
  qualifying identities and samples, with advancing native world time. Frozen
  time, missing reads, reused identities or long sample gaps cannot pass it.
- `playable_coop_verified`: always false. No connection, firing, damage,
  dialogue-control return or mission progression is tested by this observer.

The CLI exits 0 only for the narrow ordinary standalone interval, 1 when it was
not observed, and 2 on attachment/output failure. A world already hosting or
joined cannot pass the standalone-listen interval. Per-snapshot flags still
describe why it is blocked.

If the current map never appears, inspect the test game's visible startup UI.
If the map appears with a scene, mode or pause blocker, identify the genuine
engine/UI owner and allow its normal lifecycle to finish. The observer never
clears these fields, dismisses UI, invokes travel or Listen, edits a save, skips
plot events, or changes account/activation behavior.
