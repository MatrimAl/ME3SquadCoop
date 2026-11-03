# Function and channel discovery cost

The research probe previously scanned `GObjects` whenever `function_named` resolved an engine function. Repeated diagnostics therefore performed tens of thousands of object/name reads for the same function. In protected Mars measurements this was a substantial cost inside the diagnostic callback.

The replacement keeps a bounded, game-thread-only cache of 64 discovered functions. Every hit validates current object-table membership and identity stamps for the function, its owner and its `Function` class. Stamps include object index, class/outer pointers and the complete FName including its instance number. Display names are checked too. A changed or missing identity evicts the entry and falls back to ordinary discovery. Failed discoveries are not cached. Existing caller-side type, ABI and native/bytecode checks remain in place.

Only discovery targets are cached; function results and game-state values are always read through the normal calls. No game objects, counters, flags, update times or method bodies are changed by the cache. The Windows native fixture uses synthetic object/name tables and rejects altered function/owner/class identities and removed object-table entries. Separate compiled mutants truncating the FName identity or removing membership validation fail this fixture.

## Measured comparison

Two protected single-process runs used the same 100-second `travel-mars` action on the same machine, with progress diagnostics enabled. Values below are the last emitted cumulative snapshots; emission is every five seconds, so these are not complete end-of-run totals.

| Metric | Repeated discovery | Validated cache |
| --- | ---: | ---: |
| `BioPawn.GetHealthPct` resolutions | 54 | 63 |
| Total resolution time | 20.537 s | 0.306 s |
| Warm-cache calls | 0 | 62 |
| Warm-cache mean / maximum | — | 41.7 / 230 microseconds |
| `BioConversationManager.GetFullConversation` resolution time | 9.086 s / 28 calls | 0.298 s / 30 calls |
| Diagnostic callback accumulated time | 38.836 s | 1.263 s |
| Tick callback count in the snapshot | 3,368 | 10,525 |

The cached run still had an initial `GetHealthPct` discovery costing about 304 ms. The improvement removes repeated discovery, not all startup work. Callback counts and scene timing differ; these results establish resolver/diagnostic cost reduction, not a controlled FPS, GPU, transport or campaign benchmark. Inclusive profiling scopes overlap and must not be added together.

## Profiling scope

Set `ME3_SQUAD_COOP_LOOKUP_PROFILE=1` only in the protected research launch environment. `src/lookup_profile.h` uses performance-counter wall-clock timings and emits bounded cumulative snapshots. It separates pre-Tick, the original engine Tick, post-Tick and diagnostic work, and records function resolution and channel lookup costs. Cache hits have separate total/maximum timings. Profiling is disabled by default.

Cache-hit and examined-object metrics describe function discovery. Those fields are not instrumented inside the separate channel/block measurements; their zero values do not prove that an internal channel cache was unused or that no channel objects were examined. The engine Tick measurement includes engine work and waits. GPU timing is not measured; this instrumentation is not a replacement for a full engine profiler. Record fresh build fingerprints and keep raw logs private. See [PROGRESS.md](PROGRESS.md) for the paired-run result and remaining limitations.

## Current-world channel lookup

The next measured bottleneck was cold actor-channel discovery: it scanned the full object table, with an additional driver search for each matching guest channel. The resolver now reads the current `GWorld` and its `NetDriver`, after checking exact-build native instruction signatures. It validates the live World/Level/WorldInfo chain, requires the queried actor's WorldInfo to match, checks the expected network mode and, on host, requires the already established listen driver. A second scope snapshot must match after enumeration. An invalid or changing chain returns unknown; there is no global-object fallback in this resolver.

Discovery and repeated lookups both enumerate every current peer's bounded `OpenChannels`. Type, native vtable, actor pointer, connection/driver reciprocity, channel index and reciprocal channel-table membership are still validated. Duplicate channels, matches on two peers and malformed collections remain unknown. Drivers from pending or obsolete worlds cannot supply a result. The small published identity cache remains a hint for other guarded consumers; it does not bypass enumeration.

The Windows native fixture covers valid pawn/weapon identities, public type boundaries, broken world/driver/table links, a changed native pin, malformed arrays, removed object membership and duplicates within one peer or across two host peers. Mutation twins deliberately remove world binding, reciprocal table validation or ambiguity rejection; each must fail the fixture:

```powershell
python tests/native_channel_mutants.py C:\Tools\zig-0.15.2\zig.exe
```

This command compiles temporary authored-source variants under ignored `local/`. It does not launch the game or modify game files. It requires a successful current native build first. These synthetic controls establish guard behavior, not network delivery or campaign support.

A protected paired Mars rerun used the same normal host/guest duration limits as the preceding paired measurement. Last emitted cumulative channel snapshots:

| Channel resolver | Previous host | Current host | Previous guest | Current guest |
| --- | ---: | ---: | ---: | ---: |
| Calls | 439 | 564 | 2,654 | 2,071 |
| Total time | 3.696 s | 0.224 s | 2.762 s | 1.244 s |
| Maximum call | 317.520 ms | 23.977 ms | 668.479 ms | 27.274 ms |

Actual pawn/weapon channels matched on both sides. Two ownership epochs, synthetic handoff/reacquisition and native disconnect/AI restoration passed. Firing and stopping were observed, but consumed counts disagreed after the second pulse (7/8). This is a resolver-cost improvement with retained bounded lifecycle behavior, not a weapon fix. Call counts, scene timing and other machine work differ; maxima can include scheduling delays. Nonzero peaks remain, and these observations do not prove stall-free gameplay, FPS improvement, GPU performance or cross-PC transport behavior.
