# Disposed-world ownership retirement

Status: compiled candidate; no new fixture or game execution. A real mission
boundary, subsequent native listener/client continuity and playable reacquisition
remain unverified.

The common session core previously treated a changed host world as lost
ownership and latched a permanent session fault. That conservative path is
still required when the old ownership graph might remain alive. A separate
retirement path now covers positive disposal of that graph.

Acquisition now also captures the actual native GWorld identity after verifying
its WorldInfo agrees with the current host context. A changed, temporarily null
or unreadable GWorld suspends old-world possession calls while the new native
graph is incomplete. The lease remains tracked; a missing pointer is not treated
as disposal. A returned original root can resume normal cleanup/admission checks.

The transition wait has a120-second monotonic deadline, checked on Tick before
accepting disposal evidence. Expiry latches a persistent control fault and stops
automatic continuation. If the exact original root returns after expiry, the
existing release path can clean up there, but the fault is never cleared.
This deadline and behavior have not been exercised against real loading times.

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
After a separately verified old listener world and driver are both retired,
the new explicit runtime/panel re-listen action can request the ordinary Listen
path in a different standalone mission. This is a separate manual startup
candidate, not automatic travel or guest reconnection; see
[runtime integration](RUNTIME-INTEGRATION.md).

An additional default-off host policy, `ME3_SQUAD_COOP_AUTO_RELISTEN=1`, can
schedule that same request without pressing the panel button. The planner
exposes it as host-only `--auto-relisten`. It requires a previously successful
runtime listener, positive registry disposal of its origin root and driver,
an ordinary standalone new mission with no driver/pending travel, completed
session/context/camera/cinematic/capacity cleanup and no persistent fault.
It observes at500ms cadence and captures a stable target root before queuing.
The pending automatic intent remains bound to that root until the normal
relisten path arms; changed or unknown target identity refuses it. The normal
readiness window, ABI, endpoint, cleanup and final target checks still apply.

Each successful runtime listener permits at most one later automatic intent.
Expiry, refusal or disable does not retry that world or hop through other
candidate roots. A newly verified listener supplies a new origin for the next
mission; manual recovery remains available after a refused attempt. An existing
network world is preserved, and failed initial native Listen is not retried.
Cancel pending connection permanently suspends this scheduler for that process,
including when there was nothing pending, without disabling manual requests.
No story travel, guest reconnect, driver teardown or fault clearing is added.
This policy is compiled only; actual native travel/listener continuity remains
unverified.
Retiring disposed bookkeeping is allowed while the mod is disabled; it does not
reenable the mod or permit a new acquisition. Native connection cleanup remains
forwarded even while ownership retirement is waiting.

This covers only a positively disposed old graph. Transitions which retain the
old registered world/pawn, incomplete new-world startup or unresolved controller
ownership retain the previous conservative behavior. Native package negotiation,
network travel, streaming and scene/collision/equipment continuity require
separate live evidence. Do not describe this as completed mission travel support.

Source-level fixture cases are prepared for same-pointer renaming, unreadable
registry data, out-of-range indices and removed/replaced slots. They have not
been executed and do not exercise the whole live travel path.
