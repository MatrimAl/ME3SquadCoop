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
