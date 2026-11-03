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
requires a guest client world, its current reciprocal autonomous owner or a
previously captured owner with the same live actor/channel/connection identity,
native reader/package-map identities, matching
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

## Preserve native framing through story ownership release

A later owner session crashed in the native parallel garbage collector after
real conversation release. Its retained Windows exception context identifies
the same Ashley pointer sampled by the passive recorder and an invalid object
reference at the SDK FootstepForceFeedback offset. The previous candidate had
no admitted optimized-payload consumption event in that session. This establishes
the crash site and damaged reference, not the write that caused the damage.

The current receive gate depended on reciprocal possession even during scene
release, although the native channel could already have consumed a positive
optimized-payload marker for the previously owned Ashley. The correction now
captures the actual local PC, Ashley, world, channel/index, connection and driver
while ownership is reciprocal. During release it permits that same live scope
with local role1/2 only. Different/replaced channels, worlds, controllers or
object lifetimes do not inherit admission. Disable/scope loss retires capture.
All exact callers, archive/map/shadow checks and native owner-bit restoration
remain; the captured scope grants no gameplay input or ownership writes.

All native targets and the runtime fixture pass. No new game has run with this
change; preventing the prior corruption/crash remains a gameplay check. Invalid
object fields are not nulled and garbage collection is not bypassed.
