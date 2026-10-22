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

`src/ammo_change_trace.h` adds opt-in, read-only post-Tick observations while a guest reciprocally owns Ashley. Each weapon/context gets a baseline; subsequent records describe observed consumed/reserve-count changes. Several shots inside one Tick may appear as one count jump. This is sampled telemetry, not a complete event history.

Records include the pawn and weapon's validated ActorChannel indices, local object indices, process-local epoch scope, role, raw actor flags and sampled update scheduling. The channel lookup retains reciprocal actor/connection/table validation and uniqueness checks. Existing pawn-only consumers keep their type restriction; the new weapon entry point accepts only `SFXWeapon_NativeBase`. Unknown or ambiguous channel identities are `-1`. Indices identify actors only within the same connection lifetime, not across sessions or reconnects. Host ownership epochs and guest context tokens have separate scopes.

The recorder does not call game methods or write counters, flags or scheduling fields. It emits at most 128 observation rows plus one limit notice per process, including across context/weapon changes. It runs only in opted-in protected progress tests. Periodic samples of `bNetOwner`, `bNetDirty`, skip/dirty-only flags and update times are explicitly marked as taken outside replication. They do not prove a connection gate's value during replication, a packet's field contents, delivery or prediction correction.

The firing evaluator reports consumed and spare counts independently. `post_release_ammo_agreement` requires both to match after consumption has stabilized. Equal consumed counts with different reserves fail this check, even when stopping was observed successfully. Counter agreement alone does not prove that the host and guest weapon actors are counterparts, that a correction packet was delivered, or that enemy damage occurred.

A later protected paired run observed two pulses with matching final consumed counts (1/1, then 6/6) and reserve counts (210/210). An earlier run disagreed (4/6, then 8/9). The later run used unchanged native code; this is mixed runtime evidence, not a demonstrated gameplay fix. See [PROGRESS.md](PROGRESS.md) for the run and safety results.

The subsequent instrumented run matched the actual pawn/weapon channels on both sides and again observed disagreement (4/7). It also recorded sample gaps over two seconds and input pulses exceeding the requested 250 ms. Neither clean timing nor absence of diagnostic overhead is established. Profile the diagnostic and lookup paths before attributing this behavior to transport or changing replication state.

## Native field observations

After reducing measured discovery stalls, an explicitly opted-in observer now brackets the engine's native integer-property network serializer. It checks exact executable instructions, property reflection metadata and full identity stamps, current world, reciprocal possession/weapon ownership and the actual connection/channel. The observed send call uses the selected weapon's property storage. The receive call uses scratch storage, so a separate `ReceivedBunch` frame binds that decode to the selected weapon and reads final storage after the unchanged original receive returns. Nested receives cannot inherit the outer actor's identity.

`ME3_SQUAD_COOP_AMMO_WIRE_TRACE=1` enables it only inside protected progress tests. It installs a process-lifetime vtable observer by compare/exchange, as do the existing bounded research hooks. Arguments, native return value and Windows LastError are forwarded unchanged. The observer does not write ammunition, property payloads, archives, ownership, dirty flags or update times. It records at most 256 scope/field/completion rows per process, with one limit notice. It is disabled by default and is not a player installation feature.

One protected paired run recorded six host encodes and six guest decodes, comprising five `AmmoUsedCount` values and one `CurrentSpareAmmo` value. The guest had five completed receives; all six decoded values matched final weapon storage. One receive changed the guest count downward, from 4 to 3. The same run showed this ordering:

1. The guest decoded consumed count 2, with final storage also 2.
2. A later guest-local increment reached 3 at the recorded first release. The host remained at 2 in the post-release samples.
3. During the next pulse, a native receive decoded 3 and changed guest storage from 4 to 3. Final counts later matched at 5/5.

This is direct evidence that native field decoding and a correction occurred. The ordering supports investigating prediction/end-of-fire reconciliation; it does not establish the cause of every earlier disagreement or a completed fix. Ordinary count samples alone still cannot establish this path. A value that already matched before a receive is reported as a storage match, not a newly applied change. Encoding alone is not delivery, decoded scratch alone is not final storage, and corresponding encode/decode values do not prove one-to-one packet matching. Missing observations cannot prove that the engine never serialized a field outside the selected scope.

Summarize one private process log without attaching to a game:

```powershell
python tools/inspect_ammo_wire.py C:\Research\private-process.jsonl
```

The evaluator requires installed/scoped records, successful native/archive results and matching generation, channel, receive, field and clock evidence. Missing completion, duplicate decodes, changed identities and final-storage mismatches cannot pass the corresponding observation. It always leaves ammunition synchronization and packet matching unverified. The native fixture checks transparent forwarding, nested receive isolation and record limits; deliberately broken compiled variants are rejected with `python tests/native_ammo_wire_mutants.py C:\Tools\zig-0.15.2\zig.exe`.
