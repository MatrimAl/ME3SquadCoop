# Gradual manual feature review with per-peer recordings

`tools/prepare_feature_review.py` stages current native binaries and one
explicit private compiled-script package outside any game installation. It
checks the native build fingerprint, script receipt/source/dependency hashes
and copied artifact digests. It does not deploy, activate, launch or record.
The staged manifest preserves the source revision and artifact identity;
future source changes require a fresh staging directory.

Six ordered cases separate movement/cover, weapons, powers, Shift command menu,
revive and real scene/travel return. Each has actions, acceptance criteria,
activation blockers and separate host/guest/telemetry/review directories. Every
case starts not-started/unreviewed. Native candidates still require protected
launch review; revive remains integration-pending until shared loading,
runtime admission and component invocation/retirement are connected.

Prepare with an explicit fresh absolute directory below repository local/:

```powershell
python tools/prepare_feature_review.py --output $freshPrivateReviewRoot `
  --script-receipt $privateCompileReceipt --ffmpeg $existingFFmpeg
```

## Window recording candidate

`tools/record_feature_window.ps1` is a separate explicitly invoked recorder for
an already running protected game PID and its exact HWND. It verifies the PID's
executable path against the selected sandbox root and supported executable
digest, then verifies the window belongs to that PID. The sandbox marker is
only an additional check; the protected launcher must separately establish
independent copy, original-file read locks, private profiles and cleanup guards.
The recorder does not establish those protections or start a game.

```powershell
& tools/record_feature_window.ps1 -GameProcessId $explicitGamePID `
  -WindowHandle $explicitGameHWND -SandboxRoot $verifiedSandboxRoot `
  -FFmpegPath $existingFFmpeg -OutputDirectory $freshPrivateTakeDirectory
```

The recorder sends no game input and captures only the selected window through
gfxcapture. Each peer needs its own recorder/take directory. Audio is not
configured. It writes MKV segments targeting30 seconds with keyframes, allowing
closed segments to be decoded for review while later footage records. Segment
timing depends on actual frames; no continuous redraw or frame delivery is
guaranteed. Keep capture rectangles, captions and privacy scope tied to each
game window rather than the desktop.

Create a STOP file in a take directory to request graceful encoder stop. It
also requests stop when that game exits or HWND ownership no longer matches.
Only its own encoder is killed if graceful stop fails; no game process is
terminated. No capture duration deadline stops the human's gameplay. The
encoder stop wait is a cleanup bound, not a gameplay duration limit.

Metadata records requested UTC/monotonic start, clock frequency, explicit
PID/HWND/encoder identity, arguments, exit and segment files. Process startup
is not the exact first frame timestamp. Align video and native log clocks
explicitly before attributing an event; segment offsets alone are not proof.
Use timestamped markers.jsonl entries for feature/action/repro notes and retain
the corresponding native logs. Never treat file presence, length or exit0 as
proof of valid frames or gameplay success. Decode/inspect selected frames and
probe duration/packet timestamps before reviewing a take.

## Observation and readiness

During an actual authorized run, Codex can inspect snapshots or closed video
segments at intervals and compare them to telemetry. This is intermittent
observation, not uninterrupted model vision. Written observations should cite
the peer, take/segment timestamp and relevant telemetry identity, preserving
uncertain timing and missed intervals. Host/guest position agreement, firing
release, actual damage, power caster and scene control require the appropriate
native evidence in addition to visible footage.

Current preparation produced six private case directories and verified copies
without installing them. An existing workspace FFmpeg's local help advertises
gfxcapture with HWND selection; it was not installed or downloaded. Recorder
PowerShell and preparation Python syntax were parsed. No capture, live-window
inspection, game, fixture or behavioral tests ran. Encoder compatibility,
closed-segment frame inspection, clock alignment and actual performance still
need the later authorized manual run. Native sources did not change.
