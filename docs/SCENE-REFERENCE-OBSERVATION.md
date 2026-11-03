# Selected native scene-reference observation

A later paired trace identified encoded-zero localized references. The
separate default-off [host sender observer](SCENE-REFERENCE-SENDER.md) records
the original input and package metadata to narrow that sender-side failure.

Retained paired gameplay samples contain guest native Matinee actors with a
null InterpAction while the host admitted localized dialogue scenes. A null
result alone cannot distinguish an encoded null reference, package-map lookup
failure or later streamed-level/class rejection. It does not establish that
the animation tree is missing.

The observer is explicitly enabled by
`ME3_SQUAD_COOP_SCENE_REFERENCE_TRACE=1`; it is off by default. It observes at
most 64 exact Engine.MatineeActor.InterpAction property receives on the guest.
No extra decoder or package-map resolver is called. No actor reference,
sequence output, animation clock, playback state or packet payload is changed.

Admission binds the current native ReceivedBunch and its exact archive,
Matinee actor/channel/connection, current guest world and PackageMapLevel.
The reflected property must have offset 0x270, one eight-byte object element,
Const/Net flags 0x22, the exact declaring class and requested SeqAct_Interp
class. The native ObjectProperty serializer and original archive scalar
methods are pinned to the supported executable.

The existing scalar hook owns both original methods. An optional output
observer shares those original calls; a second archive hook is not installed.
Only the measured original codec return sites supply the tag and index.
Nested unrelated properties push an inactive scope, preventing attribution
to an outer Matinee receive. Original arguments, return values and LastError
are retained. The receiver log separates the serializer's scratch output from
the actor's current property cell: the latter may precede the eventual native
commit and must not be described as its final value.

Tag 1 with index 0 establishes the encoded channel-null branch. Tag 0 with a
nonzero index and null scratch output still does not identify whether static
map resolution, streamed-level visibility or class validation rejected it.
Those are further native boundaries, not permission to assign the pointer.

The fixture extracts the actual scalar policy, output observers and
forwarding helpers. Engine identity/capture/install are explicit stand-ins.
It covers both scalar branches, wrong sites and archives, nested masking and
one original call with preserved output/error. Its 20 native fixture checks
passed. This does not establish live hook installation, property admission or
native reference resolution during an actual scene. Those remain separate
runtime observations; complete cinematic parity is still unverified.
