# Disposed-world ownership retirement

Status: compiled candidate; no new fixture or game execution. A real mission
boundary, subsequent native listener/client continuity and playable reacquisition
remain unverified.

The common session core previously treated a changed host world as lost
ownership and latched a permanent session fault. That conservative path is
still required when the old ownership graph might remain alive. A separate
retirement path now covers positive disposal of that graph.

Retirement requires a different validated current WorldInfo reached through
actual GWorld, its level and actor table. The current local controller must
reciprocally own Shepard and the new native squad must identify Shepard as its
player. Current identities and world-chain snapshots are rechecked.

The old WorldInfo and Ashley object must both have readable registry slots
which no longer hold their captured object pointers. An unreadable slot, an
out-of-range index, a renamed object at the same registered pointer or merely
failing an is-live predicate is insufficient. The current registry header and
disposal observations must agree on a second read without native engine calls.

The old guest controller must either have the same positive registry disposal
evidence or remain a valid native peer in the new world without referencing
the old Ashley. The latter case permits a native controller to persist through
travel. It does not authorize UnPossess on that new controller context.

Only mod lease bookkeeping is retired. No old-world possession, AI restoration,
story write, actor spawn/destruction or native driver teardown is performed.
The retirement event explicitly avoids claiming an AI restoration or completed
campaign transition. The protected probe retains its existing scoped team-lease
cleanup, which independently refuses writes through a changed world.

A preexisting session fault, pending disconnect cleanup or pending release
does not qualify. No fault is cleared. New ownership requires the full ordinary
admission path, actual peer membership, native squad AI and a fresh clear-control
window. Retirement does not start a new listener, reconnect a guest or carry
story/save state between missions.

This covers only a positively disposed old graph. Transitions which retain the
old registered world/pawn, incomplete new-world startup or unresolved controller
ownership retain the previous conservative behavior. Native package negotiation,
network travel, streaming and scene/collision/equipment continuity require
separate live evidence. Do not describe this as completed mission travel support.

Source-level fixture cases are prepared for same-pointer renaming, unreadable
registry data, out-of-range indices and removed/replaced slots. They have not
been executed and do not exercise the whole live travel path.
