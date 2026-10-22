# Ammunition replication investigation

The observed native-base weapon class declares both `AmmoUsedCount` and `CurrentSpareAmmo` as replicated properties. A private read-only reflection capture for the supported executable showed that both use the same condition:

```text
bNetOwner && bNetDirty && Role == ROLE_Authority
```

This condition includes the owning connection. It does not establish that the condition evaluated true during a particular connection update, that the actor channel transmitted either field, or that later client prediction reconciled. `bNetOwner` and `bNetDirty` are transient engine state; an unrelated periodic snapshot cannot prove how they evaluated inside replication. Dirty-state scheduling, channel updates and prediction reconciliation remain investigation targets, not established causes.

The observed `Actor.SetNetUpdateTime` implementation only changes the actor's scheduled update time. It does not itself mark the actor dirty. Calling it alone is therefore not evidence that ammunition would be corrected. No replication flags or ammunition counters were changed for this investigation.

## Offline inspection

```powershell
python tools/inspect_ammo_replication.py C:\Research\private-capture.json
```

The tool reads an existing private reflection capture. It does not attach to a process, collect a capture, launch a game or modify files. Its sanitized JSON summary omits pointers and script bytes. Unsupported metadata returns a nonzero status and no positive condition finding. Recognition is limited to the observed expression; this is not a general UnrealScript decoder or executable identity verifier.

Input uses the private capture schema: `read_only: true` and `ammo_replication_properties`, with one entry each for `SFXGame.SFXWeapon_NativeBase.AmmoUsedCount` and `.CurrentSpareAmmo`. Each entry records the reflected `property`, `owner`, `rep_index`, `rep_offset`, `class_script_size`, `class_script_hex` and resolved `condition_operands`. Operand keys are absolute byte offsets into the class script. See the synthetic fixture in `tests/test_ammo_replication_inspection.py` for the structure. Establish the exact game identity and capture provenance separately. Do not upload the input capture or extracted scripts.

The recognizer checks property classes, layouts and replication indices, the class identity and expression offset, every non-pointer expression byte, and each encoded pointer against its resolved reflected operand. Changed short-circuit lengths, gates, masks, conversion tokens or authority comparisons fail closed.

## Runtime evidence boundary

The firing evaluator reports consumed and spare counts independently. `post_release_ammo_agreement` requires both to match after consumption has stabilized. Equal consumed counts with different reserves fail this check, even when stopping was observed successfully. Counter agreement alone does not prove that the host and guest weapon actors are counterparts, that a correction packet was delivered, or that enemy damage occurred.

A later protected paired run observed two pulses with matching final consumed counts (1/1, then 6/6) and reserve counts (210/210). An earlier run disagreed (4/6, then 8/9). The later run used unchanged native code; this is mixed runtime evidence, not a demonstrated gameplay fix. See [PROGRESS.md](PROGRESS.md) for the run and safety results.
