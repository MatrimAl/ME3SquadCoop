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

## First ordinary-run result and revised dispatch

The protected ordinary pair from `90786ee` opened the host listener but never
dispatched guest join. Its retention call logged one template and false
reference verification. A read-only snapshot of the same guest's loader found
no record with the owned group suffix. This does not distinguish array-argument
marshalling from later native group disposal, and is not a successful retention
or visibility result. Both launchers ended at their limits; the audit recorded
8210 original-file locks, unchanged original profiles, restored sandbox TOC,
no exceptions or cleanup errors and no remaining game processes.

The candidate now calls the stock native load method at RVA 0xb03c10 using the
same four arguments shown by the retained wrapper at RVA 0xb734b0: loader,
group by value, request-array pointer and delegate pointer. It validates the
wrapper contract and the method's exact-build 24-byte entry before calling.
This avoids depending on external ProcessEvent const/out-array marshalling;
no fabricated script frame or changed function flags are used. The cause of
the earlier absent group and the revised runtime result remain unverified.
No second game run was performed.

The paired window recorders were attached during startup and encoded 160x28
frames in the host recording. A decoded frame did not establish rendered
visibility. Future recording must wait for the game window's final client size
before creating the capture, and verify encoded dimensions.

The revised native call rebuilt all four targets and passed the ordinary
no-game harness. The window recorder now rejects client areas below 640x360;
its private coordinator waits for that size before capture. PowerShell parsing
passed. Neither revised capture nor native dispatch was rerun in a game.

## Explicit cooked-package identity

Each retained request now carries the verified root package's actual FName as
`AltCookedPackageName`, separate from the private group's numeric suffix. The
previous request left those eight bytes zeroed. The retained native load body
at RVA 0xb03c10 uses the alternate cooked name unless it identifies the engine's
None name; the code did not establish that an all-zero value met that condition.
The stock `SFXWeaponUIDataManager.LoadData` script (SFXGame export 31764) likewise
assigns the cooked package name explicitly. This supports supplying the known
package, but does not prove the zero value caused the absent group in the run.

Readback now requires the same cooked package both in the group record and in
every request, in addition to the existing asset/path/class identity checks.
Four native outputs rebuilt and the ordinary no-game harness passed, including
wrong group-package and wrong request-package rejection. No game was launched
for this correction; native travel survival and visibility remain unverified.
