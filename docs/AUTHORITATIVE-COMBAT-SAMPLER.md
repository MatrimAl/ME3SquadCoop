# Read-only combat health sampler

This offline-tested tool samples health and shields from an **already running**, explicitly supplied LE3 process. It does not launch, inject, execute engine methods or write process memory. A process handle requests only `PROCESS_VM_READ | PROCESS_QUERY_INFORMATION` (`0x410`).

**Status: diagnostic tooling, not verified live combat.** The original external contribution added the reader, CLI and 26 fixtures. Independent review reproduced false health-decrease claims from missing module reads and a different pawn/world reusing an address. The follow-up changes reject those cases and add regression coverage. No live hostile damage or guest damage attribution has been established by these changes.

## Supported build and isolation

Only Windows x64 LE3 executable SHA-256 `b453871eb6eacff0c646490ffbc588f743399a829b1f9e31cd7775dc95da8001` is supported. The CLI locates the named executable module in the supplied PID and verifies its disk SHA and exact sandbox `Binaries/Win64/MassEffect3.exe` path. Missing sandbox configuration, path escapes, symlink/reparse ancestors and multiple executable hardlinks are rejected. This validates the sampler target; it does not independently prove that the whole game copy or original profiles are isolated. Establish the existing protected test environment before launching any game.

The `--sandbox` option designates the caller's already verified private copy. It is not an automatic isolation setup or permission to point the sampler at the original install. Source-level `expected_sha` overrides exist for hermetic filesystem fixtures; the CLI uses the supported build constant.

## Data sources

These exact-build offsets agree with the existing private native/reflection captures; fixtures alone are not offset verification.

| Structure | Field | Offset |
| --- | --- | --- |
| Executable | GNames / GObjects / GWorld | `0x17B33D0` / `0x1887E40` / `0x18B6638` |
| UObject | InternalIndex / FName / Class / Outer | `0x08` / `0x48` / `0x50` / `0x40` |
| UClass | SuperField / ClassDefaultObject | `0x60` / `0x13C` |
| UWorld | Levels / PersistentLevel | `0x78` / `0x88` |
| ULevel | Actors | `0x60` |
| WorldInfo | StreamingLevels / NetMode | `0x3E4` / `0x7C8` |
| LevelStreaming | LoadedLevel / visibility bit | `0xA0` / `0xCC` mask `1` |
| Actor | Modules / WorldInfo / Role | `0x70` / `0x198` / `0x26A` |
| Actor | bDeleteMe / bPendingDelete | `0x25C` mask `8` / `0x260` mask `0x800000` |
| SFXModule | ModuleOwner | `0x60` |
| SFXModule_DamageNativeBase | CurrentHealth / MaxHealth.Value | `0x98` / `0x90` |
| Pawn | InvManager / legacy Health | `0x414` / `0x514` |
| SFXInventoryManager | InventoryChain | `0x280` |
| Inventory | next / InvManager / Owner / Instigator | `0x2A8` / `0x2B0` / `0x180` / `0x190` |
| SFXShield_Base | CurrentShields / MaxShields.Value | `0x3C4` / `0x304` |
| Controller / PRI / TeamInfo | PRI / Team / TeamIndex | `0x3C4` / `0x2D8` / `0x288` |

Health comes from actual `Actor.Modules` damage-module instances, never legacy `Pawn.Health` or class defaults. Damage `ModuleOwner` must reciprocally reference the pawn. Shields come from its actual inventory chain, requiring reciprocal `Owner`, `Instigator` and `InvManager` links. Missing or non-finite health is unknown, never zero.

## Candidate scope and authority

The sampler enumerates current persistent and loaded streaming level actor arrays, validates GObjects membership and Actor.WorldInfo, rejects deleted/pending-delete actors, and excludes hidden actors and invisible streaming levels. CDO checks use the actual UClass.ClassDefaultObject pointer and `Default__` names. **The exact-build `RF_ClassDefaultObject` and UObject pending-kill bit meanings are not independently pinned here; comprehensive object-lifetime validation is not claimed.**

Player/squad/henchman exclusions and exact archetype/controller ancestry identify **hostile candidates**. Generic BioAiController alone and names merely containing `Engineer`, `Geth`, etc. do not qualify. Known Gatehouse base-BioPawn dummy data is rejected. A class, team number or combat-squad name is not proof of the current relationship to Shepard. Records therefore retain `hostility_verified: false`; verify actual enemy behavior/relationship separately before claiming enemy damage.

A valid authoritative measurement additionally requires host/standalone `NetMode` 2 or 0 and target `Role` 3. Client floats are never accepted as authoritative damage evidence. Class/name ancestry cache is rebuilt each snapshot.

## Stable measurements and epochs

Every snapshot binds world and WorldInfo identities, actor index/FName/class/outer, module identity/owner and shield identity/owner/manager. Module membership, inventory-chain links, current world/authority, GObjects header and world/level array **contents** are checked again after sampling. Detected changes or partial reads invalidate that snapshot for delta evaluation. Reads are not atomic; mutation and complete identity reuse between observations remain possible and are explicit limitations.

`evaluate_damage_decrease` compares complete, authoritative records only within uninterrupted identity epochs. Missing targets, invalid samples, non-increasing timestamps and changed world/pawn/module/shield identities break the epoch. An unreadable module or missing shield never becomes a zero-health/zero-shield endpoint. Replaced components start a new epoch. Per-target epochs retain valid decreases, including transient shield loss followed by regeneration, without comparing different enemies.

The output field `decrease_observed` means only that a candidate's verified health/shield values decreased within one such epoch. It does not identify the shooter. `guest_damage_attribution_proven` is always false. Neither an incoming `ServerStartFire` nor the queued `SFXPreAsyncWorkTicker.ProcessInstantHit` entry proves actual damage; attribution requires the actual damage-processing path, matching weapon/controller/target, and exclusion of ambiguous sources. This tool implements no hit hook.

## Usage and budgets

```powershell
python tools/sample_authoritative_combat.py --pid <HOST_PID> --seconds 10 --interval 0.25
```

Optional `--sandbox <PATH>` selects the already verified isolated copy; the local default follows the repository's private test layout. `--output <PATH>` must be a new private `.jsonl` outside the game sandbox. Existing files are never overwritten. Raw logs remain private and must not be committed. `--target <HEX>` selects a candidate pointer for the summary; complete identity epochs still govern comparison.

Duration is 0.1–120 seconds; interval is 0.05–5 seconds. Each snapshot stops at 120,000 reader calls, 32 MiB requested data, three seconds or the run's remaining deadline, whichever occurs first. A budget failure is explicit and invalidates the sample. Individual OS reads, output writes and caller-supplied callbacks cannot be preempted by this Python deadline. Arrays, ancestry, outer chains and inventory links have independent count limits.

JSONL records include monotonic timestamps, `status`, `authoritative`, `valid_for_decrease`, identities, per-pawn `measurement_complete` and read failures. World unavailable, changing arrays and budget exhaustion produce diagnostic records instead of fabricated values. A successful CLI exit means collection finished; inspect record validity, it is not a combat pass.

## Validation

`python -m unittest discover -s tests -p test_authoritative_enemy_sampler.py -v` exercises hermetic object/level/module/chain layouts, valid health deltas, transient regeneration, partial reads, world/actor/component reuse, replacement, array mutation, authority rejection, inventory cycles, CLI world-unavailable handling, filesystem checks and bounded reads. Filesystem hardlink rejection uses mocked metadata because the desktop sandbox denies creating hardlinks.

These tests do not establish a live hostile encounter, exact object lifetime across unobserved gaps, real hit attribution, full multiplayer combat, Radmin connectivity or player installation.
