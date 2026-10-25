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

The ASI repository's MIT license covers that source; it must not be assumed to
license the DLC's game-derived content. This repository contains only authored
findings and implementation, without extracted scripts or game assets. No
LEMP game, campaign game or gameplay test was launched for this comparison.
