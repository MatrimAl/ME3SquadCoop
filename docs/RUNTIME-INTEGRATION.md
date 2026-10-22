# Runtime integration status

There is no verified shareable gameplay runtime yet. Native filename and archive
validation do not establish that co-op is initialized.

## Current artifact paths

`build.py` compiles both `ME3SquadCoopProbe.asi` and `ME3SquadCoop.asi` from
`squad_probe.c`. The latter uses `ME3_SQUAD_COOP_RELEASE`, which excludes some
fixture exports. It retains the protected research entry: sandbox markers,
private Documents and file guards, QA role/environment, `test-action.txt`, and
automated test orchestration. It is not an ordinary player runtime.

`tools/build_shareable.py` can instead compile `squad_coop.c` when the named ASI
does not already exist. That source is an inert DLC-file gate and does not
initialize networking, squad possession, camera adapters or session lifecycle.
Finding `Mount.dlc` at the enabled installation path does not prove the engine
mounted the DLC. The two source paths must not be treated as interchangeable.
Existing package and release-evidence validation remains required.

## Shared foundation

`me3_runtime_identity.h` preserves the research entry's safe memory reads and
exact in-process timestamp/landmark checks. These checks supplement the complete
executable SHA-256 required by protected launchers; they are not a full hash.
Both native entry sources use these primitives.

`me3_game_support.h` shares the existing UE3 names, FString/parameter layouts,
validated function discovery cache, ordinary invocation helpers and native
package-map readiness code. These function bodies were moved unchanged from
the protected probe. `squad_pawn_readiness.h` shares its read-only ownership,
action and physics readiness helpers. Sharing those implementations does not
start the co-op lifecycle or permit an unprotected game experiment.

`runtime_config.h` defines an explicit future runtime role and endpoint. It
requires `ME3_SQUAD_COOP_ENABLED=1`, `ME3_SQUAD_COOP_ROLE=host` or `guest`, an
explicit bind IPv4 and port, and a peer IPv4 for a guest. It rejects loopback,
implicit QA defaults, an inherited test role, truncated values and a host peer.
Parsing opens no sockets or game sessions. This configuration is not yet a
player launcher or proof of initialization.

## Remaining integration

The production entry needs a complete executable fingerprint, enabled-DLC and
configuration gates, then a game-thread bridge to a shared co-op core. Real
host ownership and guest camera/control adapters must be separated from the
probe's movement/firing bots, synthetic scene gates, forced Mars travel and
timed disconnect. Keep the probe's original-file locks, private profiles and
bounded test process lifecycle intact.

The shared core must retain host story authority, restore AI and guest camera
state on actual disconnect, stop safely on map/world changes, and clean up on
disable. Production startup and host/join behavior require protected game
verification. Offline fixtures cover parsing and extraction invariants only;
real combat, cross-PC Radmin and reversible Mod Manager use remain independent
evidence requirements. Do not label an unverified integration as a release.

Run `python tests/native_runtime_harness.py <zig.exe>` after the normal native
build to compile the inert entry fixture, exercise both shared headers and
verify that QA-only environment and missing/disabled DLC remain inert. No game
is launched by this harness.
