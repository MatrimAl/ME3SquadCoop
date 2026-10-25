# LEMP as the multiplayer control reference

LEMP's installed DLC and retained source are the concrete comparison target for
the [campaign player experience](CAMPAIGN-PLAYER-EXPERIENCE.md). Its own ASI
README describes a Legendary Edition horde multiplayer mod and states that it
is not affiliated with or endorsed by EA/BioWare. It is a community restoration
reference, rather than a first-party campaign co-op implementation.

The inspected ASI source is pinned to
`962ec1a467814391c8ef3957719b6c11fd0bf056` in
[PayDay42/lemp-asi](https://github.com/PayDay42/lemp-asi/tree/962ec1a467814391c8ef3957719b6c11fd0bf056).
Its README separates native online/lobby/save/voice functionality from the DLC
scripts. ASI source alone is therefore insufficient to reconstruct character
input. Its `repl_probe.cpp` provides useful investigation leads, including the
distinction between a mode reporting collected bindings and the actual local
PlayerInput holding bindings. Probe comments are leads, not runtime proof.

## Directly inspected DLC evidence

The retained private decode of the installed startup package's
`SFXGameMPContent.SFXPlayerControllerMP.CastPower` is complete: 156 serialized
bytes, 244 VM bytes, no decoder error. Its two resolved calls are
`BioPlayerInput.IsCombatEnabled` and `BioPlayerController.SquadOrderUsePower`.
Its pawn casts use `BioPawn`, not an MP player class. This supports a narrow
shared native power-order path for existing campaign companions. It does not
prove the current guest's action caches, normal input delivery or a live cast.

A subsequent bounded read of the installed LEMP startup package decoded
`MPEnterGameplay` completely (2811 serialized / 2879 VM bytes). Its actual
calls include GUI/default game-mode changes, `EnableInput`, MP options-row
setup and `UpdateInputConfiguration`. Embedded comments explain why a bare
binding reset lost profile rebinds. Those comments are historical developer
observations; the decoded calls establish the static sequence, not current
gameplay success.

The owned campaign `BioPlayerController.UpdateInputConfiguration` also decoded
completely (238 serialized / 374 VM bytes). Its body:

1. Casts the controller's PlayerReplicationInfo to `SFXPRI` and returns if absent.
2. Calls the current GameModeManager's `ResetControllerBindings`.
3. Applies stick and trigger configuration from that PRI.
4. Handles the console menu-button option on its platform branch.
5. Calls `LoadPCInputConfiguration` on the non-console branch.
6. Calls `UpdateAllBindMappingCollections`.

No player-pawn/MP-pawn cast appears in this body. The PRI prerequisite is
therefore a concrete requirement for reusing this path on an Ashley controller.
The method's serialized flags are0x20002. `LoadPCInputConfiguration` is a native
bridge (flags0x20400); its two-byte script does not reveal the native profile
loader's implementation. ResetControllerBindings similarly delegates through
its native path. No initializer was invoked from this offline analysis.

The MP options save function decoded completely (1659 / 2395 bytes), but its
observed custom commands concern chat, push-to-talk and ProlongLife; it is not
evidence that ordinary power mappings are installed there. AddMPBindingRows
and OnControllerChanged remain partially decoded and cannot support complete
control-flow claims. The inspected LEMP coalesced file yielded no matching
CastPower/TryUsePower/UseAbility/PC_HotKey value rows; this negative result does
not cover package defaults, runtime changes or inherited configuration.

For fully decoded functions, serialized/VM lengths matched and explicit branch
targets aligned with decoded expression starts. Source package and compression
library hashes were unchanged before/after extraction. Raw bodies, name maps,
decoder output and partial-decode limitations remain private.

Other inspected MP startup functions create fresh kit/player pawns and MP
replication state. `RestartPlayer` can unpossess and destroy an old pawn;
`SetPawnKit` requires MP pawn/PRI classes. Those lifecycle operations cannot
be copied wholesale onto a story squadmate. The campaign implementation must
preserve its existing character, roster and host story authority.

## Implementation consequence

Retain the shared native combat/power route while investigating how normal
keyboard and gamepad actions reach it. Establish local input bindings, game-mode
state, initialized wheel/quickslots where applicable, and the possessed pawn's
native action context. A controller-class swap, successful explicit request or
matching method name does not establish that complete input chain.

Prefer the native profile-aware UpdateInputConfiguration path over constructing
bindings or resetting them directly, once the current local controller, PRI,
input object, manager and scene scope have been verified. It must not become a
per-Tick reset or clear cinematic input locks. Inspect the native loader and
actual missing-binding condition before implementing that recovery policy;
its safe invocation and profile preservation remain unverified in the guest.

## Native profile-loader follow-up

Read-only exact-executable analysis resolved an additional prerequisite that
the script wrapper does not check. The UTF-16 native registration name
`intABioPlayerControllerexecLoadPCInputConfiguration` is at RVA0x13a2980;
its member storage at0x17a5c00 is zero on disk. Initializer0x5c540 stages
RVA0xb67b80 through stack/XMM copies and stores it at0x5c653. That thunk
finishes script-parameter handling and jumps through receiver vtable+0xaf8.

The BioPlayerController class-construction reference at0xa9e42d leads to
the constructor trampoline0xa9fb90 and instance constructor0xa97b30. The
trampoline's meaningful leaf is its test/branch/return sequence; a generic
512-byte fallback window also includes unrelated adjacent code and must not
be treated as its function body. The instance constructor's final primary
vtable write at0xa97bf7 selects0x1340768. Its ProcessEvent slot+0x230 equals
the already verified0x40d450, and slot+0xaf8 selects0xaa00b0. These are static
native-base findings; an actual guest receiver's vtable still requires checking.

The0xaa00b0..0xaa0252 body reads controller+0xb68, then immediately reads
that object's profile-array count/data at+0x68/+0x60 without a null check.
The retained SDK identifies controller+0xb68 as ProfileSettings. It searches
0x1c-byte profile rows for identifier0x37, converts the selected value and,
when nonempty, forwards it to a helper with controller.GameModeManager2.
The semantics and accepted contents of that conversion/apply helper remain
unresolved. No profile row was invented, supplied or rewritten.

The loader's tail updates an existing PlayerInput's MouseSensitivity at+0x18c
and BioPlayerInput.bUseMouseDampening at+0x3c4/mask1 from native global options.
It therefore has effects beyond adding power keys. A valid PRI alone cannot
authorize invoking UpdateInputConfiguration: the current profile object and
array, input class/identity, manager and native method target must also be
verified. Do not transplant LEMP's broad EnableInput/mode/state changes into a
campaign cinematic or use a repeated reset to mask an unidentified input fault.

This follow-up only read the supported image with SHA-256
`b453871eb6eacff0c646490ffbc588f743399a829b1f9e31cd7775dc95da8001`.
It did not call a native function, modify a game/profile or launch a process.

The downstream manager helper at0xbb4ff0 was subsequently inspected as a
bounded native disassembly. It consumes an archive-like argument and native
mapping structures; full parser semantics remain unresolved. A default-off
[guest input initialization candidate](NATIVE-INPUT-BOOTSTRAP.md) now guards
the existing native path using current identities, reflected layouts, readable
profile rows and a stable genuinely empty input list. This is source/build
progress; no native invocation or repaired input has been observed.

## Fire release and weapon preparation comparison

A separate bounded extraction decoded these installed LEMP functions fully:

| Function | Serialized / VM bytes | Observed role |
| --- | --- | --- |
| MPStopShoot | 11 / 15 | Assigns Controller.bFire=0 and returns |
| MPDrawWeapons | 1175 / 1303 | Iterates SFXPawn_Player actors, checks weapon/preparation state and requests native action12 when CanDoCustomAction permits it |
| CastPower | 156 / 244 | Reconfirms the previously inspected own-pawn power-order route |

MPDrawWeapons tracks considered pawns in its MPDrawnPawns array and skips
already-expanded weapons. Its iterator explicitly selects SFXPawn_Player;
ordinary campaign Ashley does not enter that class-filtered path. Replaying
this MP helper is therefore not an Ashley equipment initializer. Its normal
CanDoCustomAction/DoCustomAction use is a lead for the correct action path,
not permission to force an animation, clear an action cache or create weapons.

MPStopShoot alone does not call weapon StopFire, cancel CheckBeginFire or
clear PendingFireMode. This does not identify every caller or all LEMP firing
behavior. It does establish that this small helper is not a replacement for
the scoped delayed-start release candidate described in
[native fire observations](NATIVE-FIRE-OBSERVATION.md). General host-side bFire=0
sampling cannot establish that a guest released its fire button.

The private decoder was extended from the retained LegendaryExplorer iterator
and dynamic-array format readers. All three serialized/VM lengths matched;
all ten explicit branch/iterator targets aligned with decoded expressions.
The startup package and compression-library hashes remained unchanged.
No helper, input pulse, cancellation experiment or game was executed. The
protected cancellation candidate has not been promoted into the ordinary
runtime without its deferred real-client evidence.

The ASI repository's MIT license covers that source; it must not be assumed to
license the DLC's game-derived content. This repository contains only authored
findings and implementation, without extracted scripts or game assets. No
LEMP game, campaign game or gameplay test was launched for this comparison.
