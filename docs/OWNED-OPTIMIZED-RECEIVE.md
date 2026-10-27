# Owned guest movement payload

The ordinary guest now includes the scoped optimized-payload correction that
previously ran only in the protected experiment. The retained baseline showed
124 callbacks entering with `bNetOwner` set and consuming no payload after the
channel had already read a positive optimized-data marker. The next property
decoder then read the wrong bits; inventory loss and coordinate corruption
followed. The protected correction consumed the native payload, restored the
owner bit and retained a bounded common walking interval. It did not prove
all conversations, combat or mission transitions.

`runtime_owned_optimized_receive.h` uses the current native ReceivedBunch frame,
the two exact native callers and the existing Ashley callback ABI. Admission
requires a guest client world, its reciprocal autonomous local pawn/controller,
the current channel/connection, native reader/package-map identities, matching
shadow allocation and a non-error archive with sufficient remaining bits.
Other actors, callers and receive contexts use the unchanged original callback.

For the admitted callback only, a compare-and-swap clears the transient owner
bit. The original function receives all five original arguments exactly once.
Afterward, the adapter restores only that bit on the same live pawn, preserving
other changes made by native code. It never edits a position, velocity, role,
inventory reference, archive cursor or packet. A failed identity/restore check
disables subsequent intervention for that process. LastError is preserved
across both the forwarding and restoration paths.

The correction defaults on for ordinary guests. Set
`ME3_SQUAD_COOP_OWNED_OPTIMIZED_RECEIVE=0` before launch to compare behavior.
It is independent of the scene collision repair and stunt actor retention.

The normal build and native fixture pass; this ordinary integration has not
been exercised in a game. The owner will check sustained Ashley movement,
landing conversation control return, retained equipment and reconnect behavior.
Remote weapon/helmet changes and host story authority remain in place.

The guest shuttle body and doors are retained through client-level loading.
One completed door gesture still has a meaningful numerical pose difference.
No terminal Matinee replay, direct bone copy or animation-tree reset was added.
