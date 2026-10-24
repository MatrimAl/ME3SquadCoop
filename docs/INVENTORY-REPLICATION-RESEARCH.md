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
