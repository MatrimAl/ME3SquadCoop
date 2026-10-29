# Guest viewing during host scenes

The scene spectator candidate lets the guest follow the host camera during a
full conversation or cinematic, including intervals when native story control
has released the human companion. Shepard retains story and dialogue authority.
Camera-free ambient lines keep the existing playable companion behavior.

The host samples its actual local camera position, rotation and FOV while its
native conversation/cinematic mode is active. A verified non-ambient full
conversation for the current Shepard can also admit the interval before the
mode changes. Movement warnings, distance and a blocked trigger alone never
start viewing. There are no Mars coordinates or Liara-specific actor rules.

Bounded scalar snapshots use the existing native owner-PC RPCs. The receiver
requires the actual ReceivedBunch frame, current local PC, native owner channel,
connection, driver and world. It does not require companion possession. This
separate scene admission does not relax existing equipment or revive messages.
Only the host-to-guest direction is consumed. Nonces and increasing sequences
reject old publications; malformed, non-finite and out-of-range views refuse.

The camera filter runs after the real stock Camera.UpdateCamera script in the
existing shared executor hook. It temporarily replaces only CameraCache.POV;
the native timestamp and all actor transforms, view targets, control, plot and
game modes remain unchanged. Stock camera updates continue. A separate exact
GetFOVAngle result filter also handles a native locked-FOV camera without
changing its lock or value fields.

An inactive host snapshot, a view older than 500 ms, disable, disconnect or a
changed camera/world lifetime returns to the native view. Conditional cleanup
restores only the cache value still owned by this adapter and preserves an
external replacement. Native companion reacquisition remains responsible for
returning Ashley control; this feature never possesses or teleports a pawn.

The feature is enabled by default. Set
`ME3_SQUAD_COOP_SCENE_SPECTATOR=0` to disable it. Active updates are capped at
20 Hz; an inactive snapshot is sent on initial admission and on scene exit.
Native logs distinguish host scene transitions, received viewing admission,
actual cache publication and restoration. The passive recorder retains these
rows without driving the game.

This transmits a camera view, not rendered video or a separate audio stream.
Level streaming, native scene actors, dialogue audio and subtitles still use
the game. The reported later Mars/Liara interval was not captured by the last
owned recording. Actual Liara visibility/audio, smooth playback and companion
control restoration require an owner gameplay check. This addition does not
establish the cause of the reported physical progression barrier or repair the
separately retained guest crash.
