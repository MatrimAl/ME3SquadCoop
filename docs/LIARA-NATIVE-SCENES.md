# Native Liara scene transport

The owner observed correct guest camera angles but absent cinematic character
animation. Offline inspection of the selected Liara packages found the main
intro scene, its 17 localized line actions and 97 later dialogue actions marked
`bClientSideOnly`. Exact native activation bodies show this bit suppresses the
MatineeActor transport even on a listen host. Camera sharing alone cannot
supply the missing animation timeline.

This candidate borrows only that flag bit for the four selected intro/dialogue
packages. Before the original native Activated/start call, it checks the current
host world, exact native action and reflected property layout, initial group
state, and a fresh owner-connection capability from the guest. Level-owned
actions must belong to a currently loaded level. Localized line actions instead
must be the actual current line of the host conversation manager. Previously
finished transport actors can be reused when their same-action/world identity
and native stopped state are valid. Native activation, actor creation, playback,
replication and teardown run normally. No scene activation is replayed, no actor
is spawned manually, and no animation clock, pose, tree or profile is authored.

The flag lease preserves all other flags and remains through the active
lifecycle. Retirement requires the exact live action/world, stopped playback,
empty native group and latent-actor lists, and a null or verified stopped native
transport actor. Unknown lifecycle state retains the ledger rather than
guessing completion. Capability expiry prevents new admissions; it does not
restart or prematurely stop an existing scene. A mixed peer without the guard
cannot advertise version 1 and therefore cannot enable this transport path.

These assets contain actual door and dialogue progression links. The guest
therefore installs two resident native guards before advertising readiness:
the action's event-track output callback and the shared ActivateOutputLink
helper for these exact selected scene actions. EventTrack.UpdateTrack itself
remains unchanged, including its native last-position bookkeeping. The output
helper returns false without setting a guest impulse. Host and unrelated
actions retain their original calls, arguments, return values and LastError.
The guest guard survives presentation opt-out, camera loss and temporary
unpossession in the same native guest network world. It does not grant a guest
authority to start or progress the scene. Presentation effects whose Kismet
events are withheld may still require separate host-originated presentation.

The capability uses `ME3CI1 R 1` through the existing native owner-controller
RPC and actual ReceivedBunch channel. Its lifetime is local and bounded to two
seconds, and the exact controller, world, driver, connection and channel are
rechecked before admission. Native hooks check supported image, selected entry
bytes and their owned hook/trampoline bytes. An unknown layout refuses new
transport admission.

This is a concrete transport correction, not proof of complete cinematic
parity. Actor selector/group resolution, gesture/FaceFX playback, voices,
subtitles, effects, exact line timing and postscene weapon recovery still need
a real paired scene observation. No local conversation or story graph is
created on the guest. Other campaign scenes outside these selected packages
remain outside the new transport scope.

Local fixtures cover namespace/version rejection, capability age boundaries,
flag-bit preservation, selected package boundaries and native forwarding or
guest output refusal. Gameplay must compare the full intro and subsequent
dialogue on both peers, including branching choices and repeated line actions;
correct camera movement or a startup capability alone does not establish this.
