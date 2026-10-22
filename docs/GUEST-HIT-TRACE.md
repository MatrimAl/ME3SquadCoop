# Drained guest instant-hit observations

This protected developer observer is default off. It is a way to gather
stronger combat evidence; it does not make combat work or certify a playable
mod. Enable it only in the existing protected host test with
`ME3_SQUAD_COOP_GUEST_HIT_TRACE=1` and the existing ammunition-wire trace enabled.
It is restricted to one exact supported executable, the game thread, host
authority, and the selected weapon owned by the currently remote-controlled
Ashley and its validated connection/channel generation.

The native enqueue routine only stores a shot for later processing. This
observer instead brackets the actual drained weapon `ProcessInstantHit_Internal`
dispatch through the exact supported Avenger `ProcessEvent` vtable slot. It
requires the pinned drain callsite, reflected function, argument layout, actual
cached event name, and current source ownership. Installation may wait until
the engine's cached event name is available.

The instrumentation replaces that one shared weapon vtable slot with its
forwarder. Every call, including other weapons/functions, other threads, and
NULL parameters/results, reaches the original Actor `ProcessEvent` once with
the same arguments and entry `LastError`. The original's resulting `LastError`
is preserved. Native engine gates remain in the original path. The equipment
bootstrap recognizes only this exact registered forwarder on the exact vtable
with the original native body and drain pins intact; arbitrary registered
callbacks are rejected. Disabling the marker stops capture. The protected
process must exit to remove its installed instrumentation.

For each qualifying dispatch, the observer samples the target before and after
the original call. Target identity includes object/class indices and names,
current host world, authoritative role, actual loaded and visible level, and
unique membership in that level's actor table. Health comes from the target's
unique reciprocal damage module and its actual floating-point `CurrentHealth`.
Shield/barrier state comes from the reciprocal current inventory chain and
shield actor's actual `CurrentShields`. Legacy pawn health and quantized
replication fields are not used. Missing, unreadable, non-finite, ambiguous,
replaced, or destroyed sources remain invalid; an invalid read never means zero
health or no shields.

At most 64 calls reserve 128 paired records. Nested dispatches maintain separate
scopes and are counted. Records distinguish stable health/shield pairs and
observed decreases from a damage verdict: `damage_verified` and
`hostility_verified` remain false. A decrease within this dispatch is a scoped
observation. It does not exclude synchronous scripted/delegate effects, prove a
hostile target, establish client replication, or demonstrate sustained combat.
Before/after samples also cannot exclude intermediate inventory changes that
return the same captured identities and container heads before the call exits.

Offline fixtures exercise forwarding and NULLs, `LastError`, nested invalid
calls, exact caller/function filtering, ownership generation changes, target
and module reuse, shield/inventory replacement, failed memory reads, paired
record limits, and restricted equipment-bootstrap admission. Compiled mutants
exercise selected unsafe changes. Successful installation and real target
damage still require a protected in-game observation.
