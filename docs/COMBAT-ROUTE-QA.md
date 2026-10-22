# Protected normal-input traversal candidate

This developer-only candidate extends the protected Mars approach test. It is
default off and does not establish traversal, combat, or an installable player
mod. Actual route and drop results still need a protected game run.

`ME3_SQUAD_COOP_COMBAT_ROUTE_QA=1` retains the approach-only behavior: normal
movement axes follow measured navigation points and stop before the drop.
Only the additional exact opt-in `ME3_SQUAD_COOP_COMBAT_ACTION_QA=1` enables a
bounded final approach and an ordinary action-button tap. Existing sandbox,
host authority, original-file protection, current-world identity, story roster,
ordinary mode, camera, grounded state, and joined-peer guards still apply.

A protected run started at the actual shuttle landing position and moved
approximately 126 units toward the first road node before stopping against the
shuttle at unchanged height. Initial displacement agreed with the requested
world direction; this does not support changing the controller-yaw input basis.
The candidate now first follows two actual shuttle `BioPathPoint` anchors through
the doorway and onto the lower exterior ground, then turns toward the road.
An earlier read-only endpoint capture directly linked these anchors through
the authored `ForcedReachSpec_10` and linked the exterior anchor to road
`PathNode_103` through an ordinary reach specification. This proves those
captured endpoints, not current collision clearance or successful traversal.
Each anchor requires its unique numbered name, shuttle package path, class,
current world, and measured coordinates. Doorway arrival uses a 20-unit radius;
exterior arrival also requires the measured lower ground height. Starts already
beyond either anchor skip it so the route does not retrace the shuttle. The
original start envelope, four-second stuck bound, and 75-second limit remain.
This correction has offline fixture coverage; successful shuttle exit and the
remaining route still require a new protected run. Captured component bounds
are geometric leads, not proof of which component produced the collision.

The action candidate verifies the actual top node, destination, touching
cylinder, and unique enabled, unpruned reach specification. It uses ordinary
movement and turn input, remains at least 45 units from the top's center, and
requires the real top node in the pawn's `Touching` array. Geometric proximity
alone is insufficient. It waits for the captured facing and a stationary
interval before forwarding `PC_Action` through the exact supported
`BioPlayerInput.DebugExecInputCommands` ABI and current binding parser. This
preserves the normal exclusive jump/use decisions and tap/hold/release chain.

A matching release is attempted after 100 ms, or immediately during cleanup.
The captured input, controller, pawn, world, manager, mode, method, and binding
must remain valid; a replacement input never receives a delayed release. Failed
release forwarding has a three-attempt limit and is reported as unverified.
The final approach has a 10-second limit. After the tap, a five-second window
records ordinary position, velocity, action, physics and grounded state. A
forwarded command does not itself prove a drop or mission progression.

No actor transform, traversal helper, physics, custom action, game mode,
permission, or plot value is assigned by this candidate. Offline native
fixtures cover opt-in, forwarding, the tap interval including timer wrap,
matching cleanup, failed forwarding, identity replacement, touching, facing,
clearance and time bounds. Compiled mutants check that those fixtures reject
selected unsafe changes. These fixtures cannot prove in-game traversal.
