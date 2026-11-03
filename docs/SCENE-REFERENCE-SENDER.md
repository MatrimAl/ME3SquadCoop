# Host Matinee reference metadata

The latest paired scene recorded localized Matinee references arriving as
channel-tag index zero. Static references in the same run resolved normally.
An already encoded zero cannot be explained by the guest's static-index
visibility filter. The receive record does not distinguish a null sender input
from a host object that the native package map cannot represent.

Selected authored localized actions have positive NetIndex values. Their
explicit Package exports have empty generation net-object-count arrays.
Retained native registration code uses those generation counts to populate a
forced package's network-object table. This is a candidate mismatch, not proof
of the runtime state: loading and registration may change the package metadata,
and a path match alone does not identify the runtime containing linker.

The default-off sender observer records the input at the original native
ObjectProperty serialization boundary. It uses the same single pinned property
slot as the guest observer, with a separate host callback. The guest's archive
scalar hooks are not installed for host metadata recording.

Admission requires the exact MatineeActor.InterpAction property and sender call
site, current game thread, authenticated peer connection, native actor channel,
current world and selected scene-admission lease. The observer records a
readable input separately from a null input, its signed NetIndex, and a bounded
chain of object identities leading to the outermost Package and its full FName.
It also reads the package flags and the raw headers of its generation-count
and NetObjects arrays. A modest valid generation header permits one read of
its final count value. It rechecks identity and metadata after the unchanged
original call. It does not traverse or populate NetObjects.

Native ObjectToIndex rejects an unassigned NetIndex, an absent outermost-package
entry or an index outside that package's object count. This observer supplies
NetIndex and package identity; it does not invoke ObjectToIndex or walk the
package-name hash. Raw package-map metadata is diagnostic only. An unreadable
field is unknown, and a successful native return alone does not establish a
usable reference.

Set `ME3_SQUAD_COOP_SCENE_REFERENCE_SEND_TRACE=1` explicitly on the host to enable
the bounded recording. The existing guest receive trace uses
`ME3_SQUAD_COOP_SCENE_REFERENCE_TRACE=1`. Neither is enabled by default. The
sender trace never assigns a reference, changes NetIndex or package tables,
replays a decoder, retries a send or activates a guest sequence. Unrelated,
nested and exhausted-quota calls retain the original arguments, result and
LastError.

This is a diagnostic boundary for the next real dialogue, not a cinematic fix.
It does not establish Liara visibility, character animation, audio or rendered
camera equality. Startup scenes do not reproduce the localized failure.
