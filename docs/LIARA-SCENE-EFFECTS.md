# Local Liara cinematic effects

The guest's scene output guards also withhold three presentation keys in the
Mars Liara introduction. This adapter handles those natural EventTrack
callbacks without executing their sequence output links: `Liara_Biotic_Cast`,
`Singularity`, and `stop_singularity`.

Only the exact main intro action and its two authored event tracks qualify.
Each callback must belong to the current authenticated host scene and a live
native Matinee actor channel. The callback queues evidence; it invokes no
native effect or Kismet node. General output-link calls have no event track
and cannot enter this path. Eight queue slots, a 500 ms lifetime and one
native effect call per Tick bound the work. Repeated callbacks from the two
tracks cannot double the same phase in the same scene/actor/channel lifetime.

Tick validates the authored link to the exact effect node, target variable,
spawn parameters, current effect manager and one of two allowed assets. The
charge uses the unique existing Liara actor with the authored tag. The
singularity uses the exact package Note actor. The allowed module closure is
limited to emitter, lens flare, sound, location, lifetime, parameters and
effect material modules. Unrecognized payloads are refused.

The adapter uses the existing native client effect manager's PlayOnTarget,
StartOnTarget and Stop methods through ProcessEvent. It validates reflected
function flags, parameter sizes, return layout, fields and the 124-byte target
structure before readiness. Target initialization retains the authored native
default direction vectors. Method identities and native entries must remain
unchanged. Scene assets are validated at use, so unloaded assets cannot make
readiness circular. The current capability is `ME3CI1 R 3`.

Only a GUID returned by this guest's own StartOnTarget call is retained. Stop
uses that GUID with the same live world, manager and effect. Scene completion,
replacement or authenticated connection scope loss retires the effect even
when a detached Matinee/channel UObject still exists. Expired object identities
are dropped without dereferencing or calling them. The authored node's
LastStarted field is never written.

The selected native manager bodies manage local effects. Package inspection
found no replicated effect payload fields in their component hierarchy, but
the effective native RemoteRole default was not established; this candidate
does not claim a proven ROLE_None default. No gameplay state, scene clock,
animation pose, animation tree, sequence impulse or guest story progression
is authored by this adapter.

Local fixtures cover target ABI/defaults, method forwarding and LastError,
queue capacity, duplicate tracks, phase order and changed native-instance
identity. They do not prove rendered effects or full cinematic parity. A real
paired Liara scene must compare effect timing, location, duplicates, stop and
disconnect cleanup alongside body/face animation, voices and subtitles.
