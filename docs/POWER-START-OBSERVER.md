# Native power-start boundary observation

A paired owner test recorded a guest power action without a corresponding
host action. The local tactical-order return does not prove that the native
server request arrived or that the host accepted the power. Another startup
confirmed the guest controller's autonomous role and both pawns' normal native
update frequency, so changing those values would not address a demonstrated
cause.

The bounded observer watches the existing
`BioPlayerController.StartPowerCustomAction` and
`BioPlayerController.ServerStartPowerCustomAction` script executions. It uses
the already installed script-executor dispatcher. Exact function identities,
flags, parameter sizes, script lengths and native executor are checked before
the corresponding frame's parameters are read. Every original execution runs
once with its incoming arguments and resulting LastError preserved, including
disabled, refused, exhausted and nested cases. The observer does not send or
retry an RPC, synthesize input, change aim or alter a return value.

Only the guest's existing local controller and the host's currently leased
remote controller are recorded. Up to 64 selected calls per process emit
before/after records with the controller/pawn identities and roles, requested
power ID, native action/pending state, actual power-class/instance table entry,
and whether that instance names the current pawn/controller. The local start's
native return is reported separately from the host server execution. A current
ReceivedBunch channel is recorded, with a separate native actor-channel check;
its mere presence does not establish that this controller was the RPC source.

Fixtures cover the production forwarding path, identity refusal without using
the call budget, disabled capture, quota/depth limits and LastError preservation.
They do not establish an actual server power cast or fix its visual effect.
At the next owner-controlled encounter, the retained records distinguish a
missing client start, missing host receipt and host-side action refusal. No
unattended gameplay is required or performed to prepare this observation.
