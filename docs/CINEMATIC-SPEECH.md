# Guest cinematic speech

Native Liara Matinee playback supplies body animation and the authored tracks.
Localized VOElements keys additionally expect a local conversation controller.
The guest has no full story conversation controller: its dialogue panel only
mirrors choices and sends advice. Enabling Matinee replication alone therefore
does not supply this localized speech path.

The speech adapter observes the original native VOElements key callback on
both peers. The original callback runs once with unchanged arguments and
LastError. It captures only the exact selected action's reciprocal group,
track instance, native Matinee actor channel and authenticated owner connection.
On the host the instance must name the actual controller and current line.

Normal host Tick reads the current entry/reply phase and its reflected node's
`srText`, actual speaker and host Shepard sex. It does not use the VO track's
label: four intro labels differ from the current conversation entries. A
`ME3VO1` message carries scene nonce, monotonic line generation, native Matinee
and speaker channels, conversation resource, line reference and presentation
flags. Repeated messages keep the same generation. Stops or a newer line end
the earlier presentation. The existing native owner-PC ReceivedBunch frame
authenticates reception; the receiver queues scalars without calling the engine.
An independent authenticated `H` message supplies host sex before playback.

The guest requires its own natural key from the same Matinee channel and the
current host scene. It resolves the existing local conversation whose authored
MatineeSequence is that exact sequence. The bounded object scan is cached for
the three selected conversations and yields after two milliseconds of scanning.
The chosen line must be unique and in the matching entry/reply array. Actor
channels are revalidated at use. A missing, stale, ambiguous or substituted
identity does not start audio. Intro lines already owned by a native host FOVO
node retain the corresponding existing guest FOVO path, preventing duplicate
playback.

Only the authenticated host-Shepard replica receives the host's nonreplicated
`Pawn.bIsFemale` cosmetic bit. A compare-and-exchange preserves every other bit.
Ashley and the host's own actor are untouched. A narrow natural FOVO activation
context returns that same host replica from the native player getter only at
its exact sex-selection call site. This prevents Ashley's local sex from
selecting the wrong Shepard voice. No squad-player field is borrowed.
The main intro's existing native FOVO nodes receive the same narrow player
selection only after exact live Matinee/group/track-instance node membership
is established. Their natural keys and activation are not replayed.

For localized speech the adapter constructs a fresh native FaceOnlyVO object
using the stock factory call ABI and attaches it to the exact owner sequence.
Its two default inputs and all four default outputs remain present because
native activation indexes them directly. Every output edge, direct output
target, event link and variable actor link must be empty. No authored story
node is reused. The normal native sequence scheduler performs activation,
preloading, voice, face and subtitle playback. The adapter does not call full
conversation setup, line scripts, plot events or Matinee activation.

The isolated node allows presentation during conversation and keeps ambient
interruption and world priority flags disabled. A stale owned activation is
refused before native playback and retires through its empty failure outputs.
Cleanup uses the native EndFOVO and pointer-array removal routines for this
exact node, outside Sequence.UpdateOp. It waits if another FOVO owns the same
actor, and never stops or unlinks an unrelated node. Expired world identities
are dropped without calls into the old world. Scope loss, line replacement,
host stop and a 500 ms message expiry end the owned lease.

The current transport capability is `ME3CI1 R 4`; it additionally requires
prepared speech-key, isolated activation and player-getter hooks. Native ABI,
field layouts, supported-image bytes and hook identities are checked. Local
fixtures cover message bounds, namespace, integer overflow, generations,
forwarding arguments/LastError and pending cancellation. They do not prove
rendered speech or exact cinematic parity. Full paired intro/dialogue, repeat
branches, host skip, disconnect, female Shepard, subtitle settings and firing
after the scene remain gameplay checks.
Before host sex is known the guest sends `ME3CI1 H 4` through the already
guarded capability namespace. This announces speech infrastructure only and
cannot admit a Matinee. Only such a verified peer receives the sex message;
full R4 admission follows the authenticated identity. Older peers therefore
receive no unknown speech messages, and no circular readiness condition is
introduced.
