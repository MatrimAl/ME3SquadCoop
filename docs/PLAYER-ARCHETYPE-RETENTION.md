# Ordinary guest player-template retention

The ordinary guest now retains already loaded player templates from
`BioChar_Player.Archetypes.*` in a separately owned stock async-loader group
before native join. This addresses the missing lifetime guarantee of a package
load dispatch: earlier runs lost the standalone archetype identity during
travel and intermittently failed the initial remote player reference.

The runtime enumerates live `SFXPawn_Player` descendants in that package rather
than assuming the guest-local class/sex is the host's. The native network still
resolves and chooses the host template. This does not transfer custom morphs or
load a host template from some other package. It is not full appearance support.

`AsyncLoadAssets` receives copied path/class requests for the loaded objects.
Before join, every stored group reference must match its captured live identity,
class and path. An empty group, a changed request, wrong class, stale object or
unreadable array is not ready. A new request discards the current readiness
sample so the controller/world is refreshed before travel. The diagnostic
`ME3_SQUAD_COOP_GUEST_ARCHETYPE_RETAIN=0` disables retention; ordinary preparation
otherwise enables it. There is no new socket, root-flag write or package-map edit.

The group name is a process-local existing package FName with an unused numeric
suffix, checked against both package and callback arrays before acquisition.
Only that owned group is cleared through `ClearAsyncGroup`. Explicit cancel,
disable, startup ABI/bind refusal, loader replacement, or an observed connected
client returning to an idle standalone world trigger cleanup. An observation
timeout does not imply connection failure. Cleanup can target the captured old
loader if it remains live, without touching a replacement loader. Changed or
unreadable request ownership defers cleanup rather than clearing an unrelated
group. A join that remains unconfirmed retains its references until cancellation
or another authoritative lifecycle event.

## Evidence and limits

Build: LE3 Windows x64 2.0.0.48602; executable SHA-256
`b453871eb6eacff0c646490ffbc588f743399a829b1f9e31cd7775dc95da8001`.
SFXGame.pcc SHA-256
`1d8f7acb6f28e78030982c9f2d935cddec16877272aba314ce1c21dff5ed2d18`.
Private SDK `SFXGame_f_structs.hpp` records AsyncLoadAssets at lines 2343-2352
(flags 0x424501, 40-byte parameters) and ClearAsyncGroup at 2322-2328
(flags 0x20501, eight-byte parameters). The runtime pins their native wrappers
at RVAs 0xb734b0 and 0xb73800 respectively and validates reflected function ABI.
The retained native load body RVA 0xb03c10 stores requests by package/group,
copies through RVA 0xb01550, and resolves a reference by its requested class/path.
The clear body RVA 0xb05240 removes matching group records and callbacks.
The SDK declares the loader request and callback arrays at offsets 0x68/0x78.
Raw native/script exports remain outside the repository.

All four native targets built. The ordinary no-game harness passed, including
loaded versus unresolved owned requests, wrong path/class/group, and cleanup
ownership independent of the engine's current loader. No game was launched or
installation changed. These checks do not establish real garbage-collection
retention, travel survival, remote actor creation, rendered visibility or combat.
Those outcomes remain to be checked together in a later gameplay run.
