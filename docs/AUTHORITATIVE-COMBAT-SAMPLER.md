# Authoritative Combat Sampler for ME3 Legendary Edition Co-op Tests

## 1. Overview & Purpose

This tool implements a bounded, read-only Windows process-memory sampler to extract authoritative enemy health and shields during Mass Effect 3 Legendary Edition campaign co-op experiments.

Key engineering constraints:
- **Strictly Read-Only**: Opens process memory with `PROCESS_VM_READ | PROCESS_QUERY_INFORMATION` (`0x0410`). It never requests write access, executes remote threads, allocates memory, injects code, or modifies the game process.
- **Explicit Host PID**: Never launches or discovers arbitrary game processes. The host PID must be explicitly passed.
- **Isolated Sandbox Verification**: Enforces that the target executable is located within the verified isolated sandbox directory and matches SHA-256 `b453871eb6eacff0c646490ffbc588f743399a829b1f9e31cd7775dc95da8001`.
- **Authoritative Values over Legacy Placeholders**: Legacy `Pawn.Health` (+0x514) is not authoritative health. The sampler queries actual damage modules (`CurrentHealth` at +0x98) and inventory shields (`CurrentShields` at +0x3C4) with reciprocal ownership validation.
- **Attribution Skepticism**: An observed decrease in enemy health or shields proves only that health/shields decreased. It does not attribute damage to the guest without independent hit/weapon attribution proof.

---

## 2. Memory Layouts & Verified Offsets

The sampler offsets were independently verified against both static executable disassembly and live reflected `UProperty` metadata from research captures:

| Structure | Member | Offset | Verification Evidence |
|---|---|---|---|
| `GNames` | Name table pool pointer | `+0x17B33D0` | Name table chunk resolution `(index >> 29) * 8` |
| `GObjects` | Object array header (`Data`, `Count`, `Max`) | `+0x1887E40` | Live object index verification (`InternalIndex` at `+0x08`) |
| `GWorld` | Current world pointer | `+0x18B6638` | `UWorld` pointer validation |
| `UWorld` | `PersistentLevel` | `+0x88` | Validated `ULevel` ancestry |
| `ULevel` | `Actors` array (`TArray<AActor*>`) | `+0x60` | Bounded actor array enumeration |
| `WorldInfo` | `StreamingLevels` array | `+0x3E4` | Loaded level streaming enumeration |
| `LevelStreaming` | `LoadedLevel` | `+0xA0` | Pointer to loaded streaming `ULevel` |
| `LevelStreaming` | `bIsVisible` flags | `+0xCC` | Bit 0: streaming level visibility |
| `Actor` | `Modules` array (`TArray<USFXModule*>`) | `+0x70` | Native `Actor.GetModule` traversal (`RVA 0x3F36C0`) |
| `Actor` | `Location` (`FVector`) | `+0x108` | [X, Y, Z] 32-bit floats |
| `Actor` | `WorldInfo` | `+0x198` | Validates current-world membership |
| `Actor` | `ActorFlags` (`uint32`) | `+0x25C` | Bit 1 = `bHidden`, Bit 3 = `bDeleteMe` |
| `SFXModule` | `ModuleOwner` | `+0x60` | **Reciprocal ownership**: must point back to target pawn |
| `SFXModule_DamageNativeBase` | `MaxHealth` (`float`) | `+0x90` | Reflected property offset 112 (`+0x70` + `0x20`) |
| `SFXModule_DamageNativeBase` | `CurrentHealth` (`float`) | `+0x98` | Native `GetCurrentHealth` (`RVA 0xBE7B30`: `movss xmm0, [rcx + 0x98]; ret`) |
| `SFXModule_DamageNativeBase` | `ReplicatedHealth` (`uint8`) | `+0xAC` | Quantized network health presentation |
| `SFXModule_Damage` | `HealthType` (`uint8`) | `+0x214` | Enum: Default=0, Shields=1, Barrier=2, Armour=3 |
| `Pawn` | `Controller` | `+0x374` | Controller reference |
| `Pawn` | `InvManager` | `+0x414` | `SFXInventoryManager` reference |
| `Pawn` | `Health` (legacy, int32) | `+0x514` | Non-authoritative engine placeholder |
| `BioPawn` | `Squad` | `+0xBA8` | Squad membership reference |
| `BioPawn` | `bio_dead` flag | `+0xE20` | Bit 9 (`0x200`) indicates dead BioPawn |
| `SFXInventoryManager` | `InventoryChain` | `+0x280` | Head of inventory linked list |
| `Inventory` | `Owner` | `+0x180` | **Reciprocal ownership**: must point back to target pawn |
| `Inventory` | `Instigator` | `+0x190` | **Reciprocal ownership**: must point back to target pawn |
| `Inventory` | `Inventory` (next pointer) | `+0x2A8` | Next item in inventory chain |
| `Inventory` | `InvManager` | `+0x2B0` | **Reciprocal ownership**: must match pawn's `InvManager` |
| `SFXShield_Base` | `MaxShields` (`float`) | `+0x304` | Reflected property offset 740 (`+0x2E4` + `0x20`) |
| `SFXShield_Base` | `CurrentShields` (`float`) | `+0x3C4` | Reflected property offset 964 (`+0x3C4`) |
| `SFXShield_Base` | `Resistance` (`uint8`) | `+0x3E4` | Resistance class byte |
| `Controller` | `PlayerReplicationInfo` | `+0x3C4` | PRI reference (`RVA 0x497860`) |
| `PlayerReplicationInfo` | `Team` (`TeamInfo*`) | `+0x2D8` | Team reference (`RVA 0x497860`) |
| `TeamInfo` | `TeamIndex` (`int32`) | `+0x288` | Team index (0 = Player/Alliance, >0 = Hostile) |

---

## 3. Hostile Pawn Identification & Gatehouse Dummy Rejection

A common error in naive pawn discovery is classifying any non-Shepard pawn as an enemy. In Mass Effect 3 campaign levels (specifically Mars `BioD_ProMar_120Gatehouse`), a dummy scripted `BioPawn_0` exists:
- Path: `BioD_ProMar_120Gatehouse.TheWorld.PersistentLevel.BioPawn_0`
- Class: exact base `BioPawn`
- Squad: `None` (0x0)
- Team: `None`
- Controller: base engine `AIController_1` (not a combat AI)
- Shields: 0 (empty inventory)
- Health: 100.0 dummy value

The sampler strictly rejects this actor and only classifies pawns as genuine hostiles when:
1. The pawn is not the player pawn (`SFXPawn_PlayerSoldier`).
2. The pawn does not belong to the player squad (`BioPlayerSquad`).
3. The pawn's controller is not `SFXPlayerController` or `SFXAI_Henchman`.
4. The pawn's team index is not 0 (player team).
5. The pawn exhibits positive hostile metadata:
   - Hostile team index (`team_index > 0`), or
   - Assigned to a combat squad (`BioSquadCombat`), or
   - Controlled by a combat AI controller (`SFXAI_Trooper`, `BioAiController` with hostile squad), or
   - Belongs to a recognized hostile archetype class (`SFXPawn_Trooper`, `SFXPawn_Centurion`, `SFXPawn_Cannibal`, etc.).

---

## 4. Reciprocal Ownership Verification

To guard against stale pointers, misattributed components, or hijacked modules:
- **Damage Module**: `Module.ModuleOwner (+0x60)` must strictly equal the target pawn's address. Any mismatched module is rejected with an explicit failure record.
- **Inventory Shields**: The shield actor traversed in the inventory chain must satisfy:
  - `Shield.Owner (+0x180) == TargetPawn`
  - `Shield.Instigator (+0x190) == TargetPawn`
  - `Shield.InvManager (+0x2B0) == TargetPawn.InvManager`
  If any link does not match, the shield is rejected.

---

## 5. Usage Instructions

### CLI Invocation

```powershell
python tools/sample_authoritative_combat.py --pid <HOST_PID> [--seconds 10.0] [--interval 0.25]
```

### Options
- `--pid <PID>`: Explicitly supplied PID of the running host game process (required).
- `--seconds <FLOAT>`: Bounded sampling duration in seconds (allowed: `0.1` to `120.0`, default: `10.0`).
- `--interval <FLOAT>`: Bounded sampling interval in seconds (allowed: `0.05` to `5.0`, default: `0.25`).
- `--sandbox <PATH>`: Optional path to the isolated sandbox test installation.
- `--output <PATH>`: Optional path to output private JSONL file. Defaults to `work/coop-anything/local/me3-le/combat-authoritative-samples-<pid>-<tick>.jsonl`.
- `--target <HEX>`: Optional target actor pointer filter (e.g. `0x123f45a90`) to evaluate deltas on a specific hostile.

### Sample Output Record (JSONL)

```json
{
  "sample_index": 0,
  "monotonic_time_s": 1423.85,
  "monotonic_ns": 1423851029300,
  "read_only": true,
  "status": "ok",
  "world_pointer": "0x78c7b810",
  "actors_scanned": 42,
  "hostiles_count": 1,
  "hostiles": [
    {
      "pointer": "0x12a84b000",
      "path": "BioD_ProMar_100Surface.TheWorld.PersistentLevel.SFXPawn_Trooper_0",
      "class": "SFXPawn_Trooper",
      "level_visible": true,
      "hidden": false,
      "bio_dead": false,
      "location": [-21450.0, 48200.0, 310.0],
      "team_index": 1,
      "squad": "BioD_ProMar_100Surface.TheWorld.PersistentLevel.BioSquadCombat_0",
      "controller": "BioD_ProMar_100Surface.TheWorld.PersistentLevel.SFXAI_Trooper_0",
      "legacy_pawn_health": 100,
      "damage_modules": [
        {
          "pointer": "0x12f440000",
          "class": "SFXModule_Damage",
          "path": "BioD_ProMar_100Surface.TheWorld.PersistentLevel.SFXPawn_Trooper_0.DmgMod0",
          "module_owner": "0x12a84b000",
          "owner_reciprocal": true,
          "current_health": 500.0,
          "max_health": 500.0,
          "replicated_health": 15,
          "health_type": 0
        }
      ],
      "shields": [],
      "hostile_indicators": {
        "hostile_team": true,
        "combat_squad": true,
        "hostile_class": true,
        "hostile_controller": true
      }
    }
  ],
  "read_failures": []
}
```

---

## 6. Attribution Skepticism & Unresolved Limitations

### Attribution Skepticism
A decrease in authoritative health or shields proves **only that authoritative health or shields decreased**. It does **not** prove guest damage attribution. 
Attributing damage to the guest requires separate, verified proof:
1. Guest weapon fired and processed an instant hit (`ProcessInstantHit` with guest weapon pointer).
2. The hit impact target matched the hostile actor pointer.
3. Host Shepard, AI squadmates (Ashley/Kaidan), environmental hazards, and scripted level triggers did not cause the damage.

Without this attribution chain, any observed health decrease is strictly categorized as:
`"guest_damage_attribution_proven": false`.

### Unresolved Limitations
1. **Live Hostile Encounters**: During early Mars landing, no live hostile Trooper instances are spawned until the player navigates through the gatehouse into combat encounter trigger volumes.
2. **Hit Trace Instrumentation**: Full guest damage attribution requires pairing this health sampler with native `SFXPreAsyncWorkTicker.ProcessInstantHit` trace monitoring.
3. **Replication Presentation vs Host Authoritative State**: Client replicated health (`ReplicatedHealth` at +0xAC) is an 8-bit quantized byte representation. Synchronization verification between host authoritative float and client presented byte is a separate networking check.
