# Drained guest instant-hit observations

The focused damage follow-up now records supplied Impact start/location/ray
direction/penetration and distinguishes a live authoritative world actor from
the stricter BioPawn health target. A non-pawn impact no longer has to be
interpreted from a failed health sample alone: its input actor class is logged.
Geometry field offsets are read from the actual reflected native ImpactInfo
layout, with vector/float types, bounds, dimensions and nonoverlap checked;
native struct declaration order is not used as a memory-layout assumption.
Invalid geometry remains null. These are input snapshots before the original
consumer and its delegates, not proof of a fresh host physics trace or its
eventual damage target. The normal aim QA row also logs its viewpoint,
rotation and aim point with the existing 65-unit height offset unchanged.

A targeted offline decompilation of the selected ProcessInstantHit_Internal
body did not reference its optional NumHits parameter. Its TakeDamage branch
checks damage suppression and a nonnull Impact.HitActor after impact delegates.
Consequently num_hits=0 is not a miss verdict. A focused Flash review's proposed
five-frame settling requirement was also unsupported: retained ammo samples
are roughly one second apart, and the evaluator requires three spanning at
least two seconds. Missing samples do not prove continued firing or latency.
No aim/stance/payload changes were made from those hypotheses. Three native
targets built; the new fields have not been observed in game or fixtures.

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

At most 256 calls reserve 512 paired records. Nested dispatches maintain separate
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
exercise selected unsafe changes. A later protected game experiment installed
the observer and captured two actual drained calls, both with null hit actors
and zero hits. Their original forwarding completed with valid source scope.
That experiment was interrupted before its final paired report. It establishes
dispatch coverage in the retained logs, not target damage or a completed session.
A later completed retained pair provides stronger, separate evidence. Offline
review found 33 paired drained calls, including 28 during the two-second aimed
fire episode. Five calls bracketed valid host target health decreases across
two targets; every pair retained source scope, target identity and reciprocal
damage-module identity and returned from the original dispatch. One target
also appeared in retained narrative bindings as a Trooper. Those bindings are
contextual identity evidence, not a hostility predicate. The five decreases
were about 28.95 and four times 11.58 health. All five occurred with NumHits=0,
confirming that this value cannot be used as a miss verdict here.

This supersedes any implication that the completed run contained no target
health observations. It supports scoped host health decreases during selected
Ashley weapon processing. Synchronous delegate effects, hostility, guest-side
health replication and repeatability remain unresolved. Do not change aim,
damage values or hit payloads on the assumption that this run applied no
health loss. The earlier interrupted two-call run remains a separate result.
