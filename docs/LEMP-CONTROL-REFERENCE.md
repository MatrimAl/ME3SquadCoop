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

The ASI repository's MIT license covers that source; it must not be assumed to
license the DLC's game-derived content. This repository contains only authored
findings and implementation, without extracted scripts or game assets. No
LEMP game, campaign game or gameplay test was launched for this comparison.
