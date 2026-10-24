# Background co-op focus

## Observed blocker

The supported LE3 build has a custom focus check in `BioWorldInfo.Tick`.
When the world is not already in PlayersOnly, browser-wheel permission is
available, the window lacks focus and the local game mode can pause, it marks
`m_bPausedByFocusLoss` and calls the normal `PauseGame(true)` path. On focus
return, that same Tick clears its saved flag and calls `PauseGame(false)`.
This custom path does not consult the stock Engine focus-pause option.

A protected two-process test captured 39 consecutive host observations over
10.032 seconds with PlayersOnly and the custom saved-focus flag both true,
the host window outside foreground and a valid reciprocal native guest
connection. The guest had focus and neither pause flag. This establishes a
focus-related blocker before guest possession, not a collision or combat pass.

The reflected HasFocus return ABI and its native wrapper/getter matched the
supported executable in both processes: one four-byte bool result at offset0,
mask1. Tick has two calls, with expected script cursors71/280 before the native
wrapper and72/281 afterward. Both cursor positions were derived from the actual script and native
instructions. The first site,71 to72, was subsequently observed in real calls;
the second site remains a derived constraint. That protected pair acquired
Ashley but logged no adapted focus result, so it does not prove that background
focus correction worked.

## Candidate policy

The candidate adapts only HasFocus results requested by the two verified Tick call sites in a
current, connected co-op world during ordinary gameplay. It forwards the actual
native getter once. Other callers, disconnected worlds, menus and cinematic modes keep normal
behavior. Mode-0 dialogue does not suppress this focus policy: its world Tick
must advance. The separate session gate still releases guest control to AI
during conversations, and intentional pauses without focus ownership remain
untouched. A PlayersOnly pause without the custom
focus-owner flag must remain untouched. The normal Tick code should restore
its own focus pause; the mod must not write PlayersOnly, the saved owner flag
or browser-wheel permission.

The native candidate builds and its actual-wrapper fixture passes 26 cases,
including original forwarding, error preservation, peer loss, wrong callers,
cinematics, intentional pauses and context changes during the getter. These are
temporary test objects. A subsequent protected pair observed a real false
getter result adapted at the second Tick site and normal restoration of its
focus-owned pause. After the dialogue guard correction, 128 valid samples
spanning 13.375 seconds showed an active mode-0 landing conversation without
either pause flag; control returned afterward. These are bounded protected
observations, not ordinary-runtime validation or full conversation support.
Guest movement remained stationary during the QA pulse. Reliable movement,
combat and full dialogue support remain unverified; more guests are future work.
