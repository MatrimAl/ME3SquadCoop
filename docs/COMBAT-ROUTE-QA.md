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
original start envelope and four-second stuck bound remain. The current route
limit is120 seconds: measured normal walking plus the landing scene exhausted
the earlier75-second limit before the final approach. Native process deadlines
are unchanged; this is a developer bot timing change, not a gameplay bypass.
With the existing action opt-in only, the synthetic guest disconnect moves
from60 to120 seconds after first ownership so it does not terminate the final
approach. The ordinary40/60-second scenarios and native process deadlines
remain unchanged. This longer action scenario still needs live verification.
The action trial now omits the synthetic three-second ownership release;
real engine scene gates continue to apply. A protected run showed the detached
guest beginning to fall during that artificial release, before reacquisition.
Removing it from a gameplay route prevents that test from interrupting normal
traversal; it does not repair or verify release/reacquisition collision behavior.
This correction has offline fixture coverage; successful shuttle exit and the
remaining route still require a new protected run. Captured component bounds
are geometric leads, not proof of which component produced the collision.

The subsequent protected run cleared both shuttle anchors and reached the first
road node. An actual landing conversation then held the engine control gate for
13.39 seconds in ordinary mode 0, with an active conversation controller and
playing line Matinee. The route's two-second airborne/context allowance expired
before this normal scene ended. Positive, readable engine scene evidence now
permits up to 20 seconds of neutral waiting. Unknown context keeps two seconds;
the captured identities, original story pointers, all normal input guards and
120-second total route budget remain mandatory. Nothing closes UI, advances a
conversation or writes pause flags. Resume still requires every original guard.
This waiting change remains unverified in the game.

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

## Contact-boundary braking candidate

A protected run reached the real top contact but repeatedly alternated between
neutral and forward input as the measured distance fluctuated around the
80-unit stop boundary. It timed out without dispatching the ordinary action.
The QA driver now keeps braking latched through a two-unit margin after
entering the original boundary. Actual contact and facing remain mandatory;
losing them or leaving the margin clears braking. A context pause also clears
the braking latch and stationary timer.

The action-dispatch conditions remain distance45..80, actual contact, captured
facing, speed below15 and at least250ms stationary. The approach still has
its ten-second limit. The native self-test replays measured contact distances
and rejects lost contact, wrong facing and out-of-range inputs. This removes
a test-driver input oscillation in the fixture; actual traversal is still
unverified. No game was launched to validate this change.
