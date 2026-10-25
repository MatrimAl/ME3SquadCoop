# Inventory reference receive investigation

Guest inventory continuity is unresolved. A retained real pair observed
`Pawn.InvManager` become null during the original `ActorChannel.ReceivedBunch`
call while pawn and weapon identity stayed unchanged. That call includes both
property decoding and engine callbacks; the observation alone cannot identify
which stage cleared the reference.

## Exact-image static leads

The following RVAs apply only to the supported LE3 executable. They are research
locations, not portable hook signatures or proof of runtime delivery.

- The Core ObjectProperty construction path references the ObjectProperty name
  and installs vtable RVA0x1077d88. Its virtual+0x2f8 points to RVA0x146f60.
  This entry is a short tail-call stub: it forwards the archive, data pointer
  and property+0xd0 class pointer to the supplied map's virtual+0x2c8. It does
  not independently resolve the reference.
- The master `PackageMap` table at RVA0x1086b10 has diagnostic stub methods at
  these relevant slots. It must not be assumed to implement the connection's
  object decoding.
- Native constructors associated with `PackageMapLevel` and
  `PackageMapSeekFree` install tables RVA0x11a0d20 and0x11a1030 respectively.
  Their virtual+0x2c8 entries are RVA0x602bf0 and0x602fb0; both use
  RVA0x5f92d0 at virtual+0x2b0.
- In the SeekFree decode path, zero index or archive error explicitly produces
  null. Resolving a positive actor-channel index requires a non-null channel,
  channel type2 and its closing bit clear before copying Actor+a0. Rejected
  positive references leave the output unchanged on the inspected branch;
  its prior scratch value therefore matters.
- The normal dynamic-actor branch of the serialization eligibility method
  converts an actor-channel lookup result into a boolean. No closing-bit test
  is visible in that outer branch. This is a lead, not proof of an encoder/
  decoder mismatch: lookup internals and the actual live connection map must
  still be checked.

## Next discriminating measurement

For the exact reflected `Engine.Pawn.InvManager` ObjectProperty and captured
Ashley receive scope, correlate scratch value before/after decoding, native
result, archive error and final stored pointer. Record the live map class/table
and referenced channel state. Preserve original arguments, return values,
LastError and packet position. A successful decode result alone does not prove
that a valid object was resolved or copied into the pawn.

Do not restore the pointer manually or alter package-map acceptance based on
these leads. The existing normal single-object update experiment already failed
to restore continuity and was removed.

REA accepted the extracted raw x64 artifact but its Windows Ghidra provider
rejected that target format; no pseudocode was obtained. The findings above
come from exact-file PE reads, constructor references and Capstone disassembly.
Raw game bytes, tool evidence, source hashes and logs remain private. Unwind
entries and fallback disassembly windows can include or split adjacent code;
they must not be treated as complete function boundaries without inspection.

## Scoped decoder observer

The protected probe's existing opt-in receive trace now also observes the exact
ObjectProperty serializer entry. It admits only the reflected
`Engine.Pawn.InvManager` at offset0x414, an already validated Ashley receive
frame and the exact native property-decoding return site. Nested receives push
separate frames so unrelated decoding cannot inherit the outer Ashley scope.
It logs scratch values, native result, archive error and map table separately
from receive completion. No archive payload bytes are read, consumed or changed by the
observer (the archive error field is observed), and the original serializer is forwarded once with unchanged arguments.
Return values and LastError are preserved. Output is bounded to64 records.

The actual C wrapper passes26 authored-memory cases with unconditional checks
and a deliberately failing negative canary. These tests cover decode without
final application, inventory-only changes, invalid property/caller, nested
scope and the record limit. They do not prove the runtime property layout
matches or that any game packet resolved an inventory reference.

A bounded paired run loaded the observer and independently confirmed the live
InvManager property path, vtable, size, flags and offset against these guards.
It produced no scoped field-decode record and no inventory-pointer change in
the retained receive-change rows. This does not distinguish an absent field
from a missed scope and does not validate the decoder path on a real packet.
The same run consumed no rounds in sampled Ashley weapons; valid sampled
infinite-ammo flags were false. Role-permission checks rejected the fire bot.
Thus neither the observed animation nor installation proves a successful shot.

The observer now reports at most eight coverage summaries, spaced at least five
seconds apart, independently of its field-event budget. Cumulative scoped
serializer calls, exact receive-callsite matches and reflected inventory-field
matches distinguish an inactive callback from a rejected callsite/property.
Wrong-callsite and wrong-property authored cases assert different counters;
the actual wrapper passes27 cases plus the failing canary. Coverage counts
alone still do not prove that a valid object was decoded or applied.

A later bounded real pair produced the discriminating field record. In one
receive scope, the scratch inventory pointer changed from the current manager
to null inside the original ObjectProperty serializer, with native result1 and
archive error0 before and after. The correlated receive completion stored null
in the same pawn's InvManager. The actual map table was PackageMapLevel
(RVA0x11a0d20), not SeekFree. This demonstrates decoder output preceding the
final loss and excludes a later-only callback as the explanation for that
particular sample. It still does not distinguish an encoded null from failed
reference resolution inside the map. The serializer result1 is not evidence
that a usable object was resolved.

Exact-image analysis of the concrete Level decoder explains this limitation:
it clears the output before reading the discriminator. Dynamic channel index
zero, a missing/closing/wrong-type channel, a null actor and a requested-class
rejection can all leave null while returning1. Its static-reference branch
also has package resolution and streamed-Level checks. Do not substitute the
SeekFree decoder's behavior for these branches.

The existing field record now includes the archive vtable and its byte/index
serializer slot RVAs, read without calling either slot. These identities are
needed before choosing a narrowly scoped observation of the original decoder's
index output. No archive payload is re-read or advanced by this addition.
The actual receive-wrapper fixture checks the captured slot values alongside
its forwarding and LastError invariants; the archive targets in a live run
were subsequently collected from a real receive: archive vtable RVA0x119dda8,
slot0x10 target0xc2b00 and slot0x18 target0xc3060. The first slot consumes a
bit count (the existing record labels it `archive_bytes_rva`); the second reads
a bounded integer. Both have void ABI. Their code targets are shared with
another archive vtable, so a future observer must select the concrete table
and exact property/thread/archive scope. These identities do not yet reveal
the discriminator or index from the failing receive.

An opt-in reference observer now wraps only the concrete archive table's
two original slots. It records their original-produced discriminator/index
destinations after forwarding exactly once, within an exact InvManager
property, receive frame, thread, archive and map scope. It neither reads the
payload nor invokes an additional serializer. Dynamic positive-index records
include the existing channel-table entry, type, closing flags and actor/class
identities when readable. Index0 is distinct from a missing positive-index
entry. An unreadable archive-error field is explicitly marked invalid.

The observer independently caps output at128 rows. Nested rejected properties
mask the outer scope; unrelated calls retain original arguments and LastError.
The authored fixture covers these constraints, tag/index variants, wrong
return sites/counts/maxima, and transactional two-slot installation/rollback.
Both its deliberately failing CHECK canary and the existing27 receive-wrapper
cases passed. Live index observations remain a separate requirement.

The first protected pair with the observer produced a discriminating receive:
dynamic tag1, index17, readable channel type2 with closing bit clear, and
archive error0. The channel actor was a live `SFXTeamInfo`, while the property
requested `Engine.InventoryManager`; the original serializer then produced
null and the same receive applied null to the pawn. A creation-bound read-only
capture confirmed both class identities. This sample therefore supports a
class mismatch, rather than encoded index0 or a missing channel. It does not
yet establish why the reference selected that index, or whether the sender
or the receiver's map/connection association caused the mismatch. Preserve
the class check; do not repair this by accepting the wrong object.

A follow-up pair compared the map's connection with the actual receiving
channel's connection at the field read: both were identical. The sender's
connection also used the same `PackageMapLevel` native table, excluding a
Level-versus-SeekFree codec mismatch for those captured maps. This time the
field decoded as static tag0/index13579432 and became null, with archive error0;
it did not reproduce the earlier dynamic index17. The sender still had a valid
inventory actor channel in nearby samples. Its natural scene release preceded
this receive by roughly1.3 seconds. These observations do not establish a
specific malformed payload or its origin; the next discriminant is the
original sender's encoding of this exact property around release. Do not infer
that every inventory loss follows the previously observed class mismatch.

The same protected ObjectProperty observer now admits a host-only metadata
entry at the exact InvManager send callsite. It records the original value,
concrete writer table/slots and bounded current-channel lookup before original
serialization. It does not hook writer slots, read payloads or claim which
index was written. Only current-world Ashley fields qualify; unrelated calls
still forward normally. The32-row cap and unreadable/ambiguous channel cases
are checked by an authored fixture with an active failing canary; the existing
receive-wrapper integration tests also pass. A live sender record is still
required before choosing a writer-output observation.

After the live writer identities matched, the protected observer was extended
to capture the original writer's tag/index scalars before and after its calls.
Four exact send branches have distinct return-site and maximum-value guards.
The observation stays within one exact Ashley InvManager serialization and
preserves original forwarding, return behavior and LastError. It does not read
or reconstruct packet payloads. Its local property id groups writer records;
it is not a transport sequence number and cannot pair host/client events alone.
The channel index found by metadata scanning is only a hint: zero also covers
unreadable or ambiguous metadata and is not an encoded-null verdict.

The actual-header fixture, including an active failing canary, passed branch,
normalization, nested-scope, forwarding, budget and transactional-installation
checks. Existing sender metadata and27 receive-wrapper cases also passed.
No live writer-output observation has yet been collected with this extension.

## Retained native codec and channel-lookup decompilation

The saved exact-image Ghidra project subsequently completed five selected
entries in about13 seconds, all with nonempty pseudocode and terminal exit0:

| Native entry RVA | Reviewed responsibility |
| --- | --- |
| 0x602bf0 | PackageMapLevel object-reference send/receive codec |
| 0x5f92d0 | Reference serialization eligibility |
| 0x1c1df0 | Actor-to-channel hash lookup |
| 0x3f6620 | Pawn replication property selection, including InvManager |
| 0x48c350 | ActorChannel property sender, including serializer return handling |

This uses the supported full PE and retained project; the earlier raw-artifact
provider rejection remains historical and does not describe this later export.
Pseudocode corroborates the selected instruction findings above, without
turning inferred variable types into an ABI contract. Input, manifest and
output identities and per-function body ranges remain in private evidence.

The channel hash lookup is now explicitly bounded by its selected leaf body
at0x1c1df0..0x1c1eea, ending at the actual return. It derives a bucket from
the low pointer bits but compares the complete pointer key while traversing
records. It returns the stored value without checking channel state. Thus a
low-bit hash collision alone does not explain selecting another actor's
channel; key equality is not truncated to those hash bits.

For an ordinary dynamic actor, the inspected CanSerialize branch only asks
whether this lookup returns nonnull. The codec's dynamic sender separately
reads the returned channel's index and open bit. The receiver independently
rejects a closing channel and applies the requested-class filter. Neither
OpenChannels enumeration, index-table presence nor CanSerialize success alone
therefore establishes that the exact native reference will be accepted by
the remote channel lifetime. These are separate observations, not synonyms
for equipment readiness.

The missing-hash/index0 and lifetime-state explanations remain hypotheses;
this batch does not reproduce either in a live send or explain the retained
wrong-class index17/static-index samples. The next deferred sender observation
must distinguish its original-written scalar, current connection/channel
identity and state, and final serializer result. A local property id still
does not correlate separate host/client transport events by itself.

No extra native lookup, readiness restriction, inventory-pointer restoration,
class/closing bypass, forced replication or handoff replay was implemented.
No game, archive operation or behavioral test ran. The supported source
executable's digest was rechecked unchanged after reviewing the output.

## Channel lifecycle expansion

An owner-requested deeper batch expanded the five roots by one level of
analyzed direct calls, concrete PackageMapLevel methods, selected ActorChannel
virtuals and the actual send/receive archive methods. It processed71 entries
from the saved project in about18 seconds:70 decompiled, with exact entry
0xda350 explicitly missing. A follow-up selected six additional close/cleanup
dependencies, all decompiled in about10 seconds. No analysis deadline or
per-function timeout was imposed. These counts describe export coverage;
semantic review of the whole batch is not complete.

Selected reviewed paths now distinguish three lifecycle operations:

| Entry RVA | Bounded observation |
| --- | --- |
| 0x48f800 | Associates Actor+0xa0/class+0xa8 and passes the actor/channel pair to helper0xb99b0 on connection+0x5fb0 |
| 0x48fcf0 | For a nonnull Actor, calls hash removal0xc2210 before setting channel+0x68 closing bit2 |
| 0x47f200 | Removes the Actor key, conditionally clears matching reference shadows in other channels, delegates nested reference cleanup, then clears its own Actor+0xa0 |
| 0xc2210 | Compares full pointer keys, unlinks the matching bucket record and clears its allocation bit |

The0xc2210 unwind fragment ending at0xc223d is only the entry fragment.
Its Ghidra analyzed body includes further branches; the short fragment alone
cannot describe hash removal. Likewise, a selected virtual at0x480480 formats
actor/role diagnostic text and must not be labeled a lifecycle mutation merely
because it neighbors close/cleanup slots.

The inspected cleanup's reference-shadow pass applies only on its specific
actor/channel flags and other-channel eligibility branches. It clears matching
actor values from retained shadows and marks those channels for later work;
it is not evidence that the current pawn's live InvManager was cleared there.
That distinction preserves the earlier direct serializer-output observation.

Consequently, a current actor pointer, index-table entry, hash association and
shadow value may have different lifetimes during normal teardown. Observing
one outside the exact serialization event cannot establish all the others.
The inspected removal path deletes by actor key; this does not itself prove
an overlapping-channel race or an incorrect removal in a real session.
The association helper's internal replacement behavior and actual close/send/
receive ordering remain unresolved. No engine hash bug is claimed.

This narrows the next deferred measurement to the original reference scalar
and serializer result together with its exact connection/channel lifetime,
without assuming that nearby channel enumeration is the encoder's lookup.
No package-map acceptance change, hash edit, pointer restoration or native
handoff replay was added. Source executable identity remained unchanged;
no game or behavioral test ran. The private review index separates selected
reviewed observations from the remaining exported bodies.

## Association replacement and source-index observation

The association helper0xb99b0 was exported separately from the same retained
project (one requested/one nonempty decompilation). Its existing-entry branch
uses lookup0xba520 and writes the supplied key/value pair into the selected
record; the new-entry branch allocates/reuses a record and links or rehashes
it. The key lookup helper's internals were not covered by this one-entry
follow-up. Together with removal by actor key, replacement is a concrete
ordering lead, not proof that old/new channels overlapped during a failure.
No engine map repair is justified by these static bodies alone.

The protected sender observer now adds a bounded post-call index-table
snapshot for a dynamic-actor index record when the original writer's source
scalar before serialization is in1..1022. It records whether that connection
slot is readable, its channel/actor, separately valid channel details, type,
flags, and reciprocal connection/index. The slot pointer is read again to
detect replacement during the snapshot. An absent channel remains distinct
from an unreadable slot or unreadable details. Detail values are usable only
under their explicit validity flags.

This snapshot is tied to the actual source scalar at that original serializer
call, rather than the earlier scan's metadata hint. Both pre/post scalars stay
in the record because a writer may normalize its source. The snapshot occurs
after forwarding; it cannot prove the table's exact contents at the earlier
hash lookup, the emitted packet bits or delivery to a particular receive.
Closing/open flags are observations, not an acceptance override. Static/tag
and index0 cases do not scan this table. The existing128-record limit, exact
scope/caller guards, one original call and LastError preservation stay intact.

Only the protected observer changed. No additional hook, archive operation,
normal-runtime lookup, inventory write or gameplay repair was added. Three
native targets compiled and fingerprints matched; no game or tests ran.
The new fields and association-order hypothesis still need deferred live
evidence before the inventory issue can be closed.
