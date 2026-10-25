# Scene admission diagnostics

The shared host/guest control gate now exposes its first blocking reason as
`engine_block_reason`. It uses the same read-only predicate as admission:
`session_engine_blocked(c)` is exactly whether a reason exists. No story,
conversation, pause, PlayersOnly or cinematic field is cleared by this change.

The original gate order is preserved. A simultaneous unreadable field and
scene flag therefore reports the first rejected condition, not a complete
inventory of everything that might be blocking the session.

| Value | Meaning |
| --- | --- |
| `clear` | None of this engine gate's conditions rejected the sampled context. Other identity, roster, equipment and transport guards still apply. |
| `missing-context` | Controller, world or mode manager is absent. |
| `nonordinary-mode` | Current mode is not0. |
| `controller-flags-unreadable` | Controller flags could not be read. |
| `controller-cinematic-mode` | The controller's native cinematic flag is set. |
| `world-flags-unreadable` | World flags could not be read. |
| `players-only` | The world's native PlayersOnly flag is set. |
| `conversation-slot-unreadable` | The world conversation-manager slot could not be read. |
| `conversation-count-unreadable` | The conversation active count could not be read. |
| `conversation-count-invalid` | The count is negative. |
| `conversation-flags-unreadable` | Activating flags could not be read. |
| `conversation-active` | The conversation active count is positive. |
| `conversation-activating` | The conversation activating bit is set. |

Shared host `session: gate-observation` records retain their existing fields
and add this value. A reason change emits a new record even if mode and the
blocked boolean stay unchanged. The protected arena `prewarm-state` telemetry
also includes the reason before host package-map readiness, so an early wait
can be distinguished from a listener/package problem.

This does not classify radio versus full conversations or make either safe to
ignore. A mode0 active radio conversation was observed in the retained arena
trial, but the precise cause of that trial's complete wait remains unknown.
New telemetry can only answer it in a future live run. Campaign dialogue must
remain under the host's story authority; the arena's automatic skip callbacks
are not an ordinary-runtime policy.

All native outputs were rebuilt after this change. Existing gate fixtures and
live tests are deferred at the owner's request; successful compilation is the
only new executable validation. The diagnostic fields alone prove no dialogue
handoff, AI restoration, reacquisition or mission-travel support.

## Admission stability belongs to the current host context

The shared core's500ms clear-control wait is now tied to captured identities
for the host controller, Shepard, WorldInfo, squad, mode manager and native
network driver. Previously `session_free_at` held only a timestamp, and the
early return for unavailable host scope left it intact. A changed context
could therefore inherit elapsed time from an earlier context. This is a
source-level finding; no retained run proves it caused a gameplay failure.

The new window validates live object-table membership, index and name for all
six captured objects. A missing, unreadable, replaced or renamed object resets
the wait; losing host readiness, enablement or the clear scene gate also
resets it. Context observations run before and after Tick, while acquisition
still occurs only after Tick. Existing scene, peer, ownership, story, equipment
and package checks remain required. No fault latch is cleared and no retired
world is written to. This does not implement or validate continuous travel.

Regression source was prepared for each changed context address, renamed
object identity, disappearing driver, explicit reset,500ms boundary and clock
wrap. Existing scene-gate mutation definitions were updated for the reason
predicate, and the unreadable PlayersOnly case now has a readable later
conversation slot so another read failure cannot hide that guard. These
fixtures/mutants have not been executed. All three normal native outputs were
rebuilt; ordinary fixture-flavor compilation and runtime checks remain deferred.
