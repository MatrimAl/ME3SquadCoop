# Local session control panel candidate

Status: native code compiled and Python syntax parsed. No panel, IPC integration,
game or power order has been run for this change. The panel does not establish
a verified player installation or playable campaign.

The separate Windows/Tk panel gives an already running ordinary runtime a
manual control surface: Host/Listen, Join, Retry join, Listen in new mission, enable/disable and a
scoped power list. It displays sampled native net mode, companion ownership,
startup status and power-order status. It cannot launch, install, inject,
terminate or change the endpoints of a game. Native requests remain subject
to the existing role, story, ownership, package and transport checks.

Cancel pending connection clears only mod requests still waiting for readiness,
including rejoin/new-mission-listen intents. Native dispatch which already
happened and active peers remain untouched. Cancellation is processed on Tick,
and suspends opted-in automatic new-mission listener/rejoin schedulers for the
remainder of the process lifetime, even when no request was pending. Manual
connection requests remain available; enable does not clear this suspension.
Its panel acknowledgment means intent was accepted, not a disconnected
connection. A cancelled waiting request reports startup status18; if nothing
was pending, the existing status remains. Power requests are a separate queue.

The ordinary runtime must have explicit `ME3_SQUAD_COOP_PANEL=1` at startup.
The session planner's `--panel` option includes that flag in its generated
environment; it does not launch anything. The panel is off by default. When
protected validation resumes, the selected already owned game PID can be
supplied with `python tools/session_panel.py --pid <PID>`. A missing mapping
is an error, not a reason to launch or attach to some other game automatically.

The panel requires a local Python installation with Tkinter and Windows ctypes.
No new dependencies are installed by the tool. Closing the panel closes its
mapping/query handles and leaves the game running. Disable requests normal
adapter cleanup; it does not unload the native DLL. Enable does not reinstall
a missing DLC, bypass config or restart failed native initialization.

## Protocol and identity

Each opted-in game creates `Local\ME3SquadCoopPanel_<PID>` once. An existing
name is refused rather than reused. The bounded5264-byte, pointer-free v6
mapping contains a5244-byte status frame and a20-byte command mailbox. The
status identifies PID and exact process creation FILETIME. The client retains
a query/synchronize-only handle, checks MassEffect3.exe's filename and creation
identity, and refuses a terminated process or snapshots older than3 seconds.
It does not use ReadProcessMemory, WriteProcessMemory or remote threads.

The mapping uses Windows' default process-token object security. It is local
IPC for the current Windows session, not a cross-account security boundary or
remote administration interface. The client validates magic, version, size,
role and count, and reads the status through an odd/even sequence protocol.
The native consumer treats all mailbox data as untrusted, with a fixed command
whitelist. It accepts no arbitrary engine commands, paths, pointers, caster,
target or endpoint changes. Game calls execute on Tick, never the panel thread.

The client reserves the mailbox atomically, writes its bounded payload and
publishes the command last. The monotonically advanced request ID comes from
the mailbox, rather than a possibly older status-frame acknowledgment. The
native side copies the payload before releasing the mailbox and reports request
acceptance separately from later startup/power results. Duplicate latest IDs
are ignored. A failed client while reserving the mailbox can leave that mailbox
unavailable; no automatic mailbox stealing or process restart is implemented.

The mailbox includes the producer's Windows monotonic tick timestamp. Tick
rejects delivery more than2 seconds later, including apparent future timestamps
under unsigned wraparound arithmetic. Acknowledgment3 explicitly means expired
before delivery. The panel tells the user to submit a fresh request; it does not
resend automatically. A command accepted on time can still have its documented
native startup waiting period, such as waiting for a scene to finish. This
delivery deadline does not change the native startup or gameplay policies.
Version1/2/3/4/5 clients/runtimes are incompatible and must not be mixed.

Power commands carry the currently displayed catalog scope/index. Catalog
changes clear the panel selection, and native validation rejects stale input.
Names come from the native power-name table, rather than localized display
strings. The power list is not a promise that every listed power is ready:
rank, action ownership, cooldown, target and native gameplay checks still apply.
The optional guest runtime flag `ME3_SQUAD_COOP_POWER_HOTKEYS=1` maps
Ctrl+Alt+1..9 to retained native power names while the game is foreground and
control is ready. Each available power row shows its actual shortcut; the row
number may differ after a catalog reorder. The v6 frame includes shortcut-mode
bits and nine signed current catalog indices (-1 means unavailable), which the
client bounds-checks before rendering. It uses the same scoped request path and can operate with
the panel closed; it is not enabled by `--panel`. See the limitations and
deferred validation in [power input](POWER-INPUT-RESEARCH.md).

The independent `ME3_SQUAD_COOP_NATIVE_POWER_HOTKEYS=1` candidate routes
existing UseAbility0..7 commands to the first eight retained guest name slots.
Available rows show Power slot1..8 when the checked native bridge is installed;
these labels refer to command slots, not assumed physical keys or saved Shepard
assignments. The hotkeys field has bit0 for Ctrl+Alt and bit1 for the native
bridge; unknown bits are refused. Both can be enabled together. Native wheel
icons are not updated by this candidate, and command delivery/effects remain
unverified. The mapping remains5244+20 bytes; the version changes because the
shortcut field now has different semantics.

## Evidence boundaries

Connection request acceptance, native client net mode, companion ownership,
native power-order acceptance and an actual target effect are different states.
The panel displays those available observations without turning them into a
campaign-success verdict. It does not yet show a cooldown wheel, localized
power descriptions, a proven connection-error transcript or an in-game overlay.

The separate control state distinguishes waiting for a companion, current
control, scene suspension, mission-transition waiting and a persistent control
fault requiring restart. Host ownership is checked against reciprocal current
controller/pawn identities; an active bookkeeping lease alone is not reported
as companion ownership. These are sampled states, not progression verdicts.

The panel also displays role-specific mission recovery as Manual, Automatic or
Paused until game restart. The v6 frame carries recovery flag bits0/1 for opted-in
host/guest policies and bits2/3 for their process-lifetime cancellation latches.
Its readiness state0..7 distinguishes off, host not ready, host ready, guest idle,
waiting for host reply, host replied ready, local channel unavailable and disabled.
Unknown flags/states are refused. A readiness reply is an advisory observation;
the separate native connection and actual companion ownership still matter.
No new control command or remote action is exposed by these status fields.

The panel includes a contextual next-step message derived only from the current
validated snapshot. Persistent control faults, disabled control, mission
cleanup, companion life state and native scenes take precedence over connection
advice. Native networking without companion ownership is explicitly described
as waiting for control, not as a successful playable session. Initial connection
and mission recovery use different button guidance; automatic recovery is not
advertised as automatic initial connection. Failed or stale snapshots replace
the advice and disable controls. These messages never submit, retry or cancel
commands. Their rendering and gameplay correspondence remain untested.

No new behavior tests were executed. Deferred validation must cover protocol
layout parity, stale/changed process identity, command acknowledgment races,
partial mailbox writers, expired/future timestamps and wraparound, role/scene refusal, catalog replacement, enable/disable
cleanup, and actual native startup/power results. Shared-memory publication and
Tk rendering remain unverified despite successful compilation/syntax parsing.
