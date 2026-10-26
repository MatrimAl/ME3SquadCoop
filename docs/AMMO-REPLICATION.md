# Ammunition replication investigation

The observed native-base weapon class declares both `AmmoUsedCount` and `CurrentSpareAmmo` as replicated properties. A private read-only reflection capture for the supported executable showed that both use the same condition:

```text
bNetOwner && bNetDirty && Role == ROLE_Authority
```

This condition includes the owning connection. It does not establish that the condition evaluated true during a particular connection update, that the actor channel transmitted either field, or that later client prediction reconciled. `bNetOwner` and `bNetDirty` are transient engine state; an unrelated periodic snapshot cannot prove how they evaluated inside replication. Dirty-state scheduling, channel updates and prediction reconciliation remain investigation targets, not established causes.

The observed `Actor.SetNetUpdateTime` implementation only changes the actor's scheduled update time. It does not itself mark the actor dirty. Calling it alone is therefore not evidence that ammunition would be corrected. The inspection and observers below do not write replication flags or counters. The separate opt-in reconciliation adapter described at the end intentionally updates guest counters from an authenticated host response.

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

## Opt-in host snapshot reconciliation

`ME3_SQUAD_COOP_AMMO_RECONCILE=1` adds an experimental control protocol inside protected progress tests. Both processes must opt in and enable `ME3_SQUAD_COOP_AMMO_WIRE_TRACE=1` for validated weapon scopes and native receive frames. It is disabled by default, limited to the supported executable and not an end-user installation feature.

The adapter reuses `Engine.PlayerController.ServerSay` for requests and `ClientAdminMessage` for reliable responses. Requests are unreliable, with at most three attempts per pending request; this is research transport rather than a dedicated gameplay protocol. Exact reflected methods, parameter layouts and native dispatch must match. Incoming control payloads require a live native `ReceivedBunch` frame for the owning PlayerController, reciprocal connection/channel membership, the correct host/client direction and current world. A payload prefix alone never authorizes a correction. Recognized control payloads in this validated context are consumed before the ordinary chat/UI script; ordinary messages and outbound RPCs forward the original entry and arguments.

Payloads contain a bounded request ID and selected weapon channel; responses also carry consumed/reserve counts. The parser rejects malformed fields, overflow and trailing content. No new socket, firewall rule or public service is involved. The 64-bit request ID includes the guest's process-local weapon-scope generation. Host and guest generations are separate and need not have the same numeric value.

After a local count change, the guest waits for `bFire == 0`, native weapon state `Active` and 250 ms of unchanged counts. The host waits for its own idle, settled weapon and returns its actual counters; it never accepts client-provided ammunition values or writes host counters. The guest applies only the exact pending request/channel/generation within five seconds, with current reciprocal possession/weapon ownership and no intervening local count change. Already matching host values are accepted without a write. Otherwise the guest writes the two adjacent counters and verifies readback. Scene release, scope changes and disconnect invalidate pending state. Limits are 64 new requests and 128 adapter records per process. This does not repair the underlying firing request path.

The first protected paired run completed two four-stage exchanges: guest request, host receipt, host snapshot response and guest application. Both corrected consumed count 1 to 0; reserve count stayed 210. Guest-local roundtrip times were 281 and 375 ms. The host consumed no ammunition, so **neither zero/zero agreement nor successful reconciliation proves firing**. Synthetic scene release/reacquisition and native disconnect restoration passed, with zero native exceptions and unchanged original profiles. Real conversation, enemy damage, repeated firing and cross-PC behavior remain unverified.

`src/ammo_reconcile_verification.py` correlates all four stages, exact request/channel identity, each side's local generation, bounded local elapsed time and the host's response values. It never compares clocks across machines. Missing or duplicate stages, mismatched values and native exceptions block positive roundtrip evidence. Its firing, full-combat and two-computer verdicts always remain false; use separate runtime evidence for those claims. Native protocol/forwarding fixtures and compiled mutants cover stale generations, wrong request IDs and replies during firing.

## Native firing RPC entry observations

`ME3_SQUAD_COOP_FIRE_RPC_TRACE=1` separately enables read-only observations of 20 weapon, pawn and inventory script entries in protected progress tests, with `ME3_SQUAD_COOP_AMMO_WIRE_TRACE=1` supplying current scope and native receive context. Exact reflected method/parameter identity, script length and native dispatch are required before installation. Ordinary calls forward unchanged, and records are limited to 256 per process. Paired entry/exit records reserve space before forwarding: per-frame Tick/HandleWeaponFiring observations have 64 rows, while fire command methods have a separate 192-row allowance. A sustained trigger hold therefore cannot spend the command allowance on Tick alone. Exhausted budgets still forward every original call. An incoming RPC label requires the selected weapon channel and exact `FFrame` target function/object inside a live native receive; a method name alone is insufficient.

A protected pair with reconciliation disabled observed two host start-fire and two stop-fire entries for the selected guest weapon. Original forwarding returned, but state stayed `Active` across each entry and host consumed count stayed 0 while guest counts reached 3 and 6. This proves arrival at the host entry, not a complete script/native firing path. Internal VM calls may bypass `UFunction.Func`; missing nested or client entries cannot establish their absence. A pawn shot/effects counter can change independently of an observed ammunition increment and is not proof of enemy damage. The next discriminators are actual incoming script progress and the native ammunition-firing path, without weakening the separate combat requirements.


## Real aimed-fire tail ordering

The latest protected real-fire run narrowed its final one-round disagreement
without a speculative replication change. On the guest, a completed native
receive decoded AmmoUsedCount26 and left actual weapon storage26. Fifteen
milliseconds later, the selected local native FireAmmunition call entered
with26 and returned with27. The host ended at26. Nine later samples per side
remained stable, with reserves210. Thus this episode needs no missing final
decode to explain the mismatch: a local consumption followed the last observed
authoritative value. This is an ordering observation, not a general proof of
all prediction/timer behavior or every prior inventory failure.

The existing settled host snapshot adapter is an appropriate separate opt-in
candidate for this exact case: it waits for idle and unchanged local counts,
requests host values and accepts only a fresh matching owned-weapon reply.
Its earlier successful zero-ammo exchanges did not exercise actual firing.
A subsequent real-fire route run is described below. Repeatability and
ordinary-entry integration remain required before adopting it as ordinary
gameplay behavior. Host counters, native hit payloads and local
fire prediction remain unchanged by this investigation.


## Settled snapshots during real firing

The subsequent protected route enabled the existing adapter on both peers.
The strict four-stage evaluator matched three completed exchanges. After an
actual short firing pulse, the guest requested used5/spare210, the host
returned used4/spare210, and the guest applied4/210; guest-local roundtrip was
297ms. Two later exchanges already matched, at9/210 and28/210, taking282ms and
312ms respectively. A separate aimed-fire episode bracketed three valid host
Trooper health decreases. Nine later samples per side over about eight seconds
remained28/210. The aimed episode needed no adapter write, so it does not prove
that this adapter cured the preceding run's exact final-shot divergence.

This extends the earlier zero-host-consumption protocol observation to actual
firing and a real one-round correction. It is one protected run, not an ordinary
runtime implementation or repeatability claim. The aggregate four-pulse fire
verifier remained false; movement and full campaign gates also remained false.
Original profiles stayed unchanged, native exceptions were zero and all game
processes exited. The default-off adapter remains confined to the protected
probe pending ordinary-entry integration and verification.


## Pending snapshots and intervening local activity

Preparation for ordinary-entry reuse exposed a protocol-lifetime gap in the
protected adapter: a sampled count change reset its settling timer but left an
outstanding request active. Final counter equality also cannot establish that
no local activity occurred; an authoritative decode can return a predicted
counter to an earlier value. The guest now cancels the pending request whenever
its sampled used/spare values change, then begins a fresh settling interval.
A rejected old response cannot reuse that cancelled request.

When the separately opted-in native firing observer is installed, its exact
validated selected-weapon entry also invalidates the pending reply before the
unchanged original FireAmmunition call. This cancellation is independent of the
observer's record budget. It changes mod request state only; it does not consume
a native firing call, change a hit or write host ammunition. Native forwarding,
arguments and LastError remain unchanged. Both existing opt-ins are still
required for this entry-based protection; other firing paths remain outside
that slot's coverage. This is not a claim that every ammo-changing action has
an event-level observer.

All three targets built. The native reconciliation and firing-forwarding
self-tests passed. Four deliberately broken protocol variants were rejected,
including retaining a reply after local activity and accepting it when the
counts return to the earlier value. These are authored-memory/protocol checks,
not a newly observed game fault or a repeated gameplay test. No game ran for
this change. The existing adapter remains in the protected probe; moving it to
the ordinary entry still requires independent current scope, authenticated
incoming RPC receive context and lifecycle invalidation without research
markers, logging budgets or test-action policy.
