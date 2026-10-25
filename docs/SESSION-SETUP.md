# Ordinary session settings

The session planner prepares connection settings only. It does not install,
launch or attach to a game, open sockets, change system environment variables
or establish co-op gameplay. The ordinary runtime is still an experimental
candidate; see [runtime integration](RUNTIME-INTEGRATION.md).

Use a local IPv4 belonging to the machine which will run that role. For Radmin,
use that machine's actual Radmin interface address. A guest's peer address is
the host's address, not the guest's local address. Both roles use the same port.
The examples below use illustrative addresses which must be replaced.

```powershell
python tools/prepare_session.py --role host --bind 26.1.2.3 --port 17779 --output host-session.json
python tools/prepare_session.py --role guest --bind 26.2.3.4 --peer 26.1.2.3 --port 17779 --output guest-session.json
```

The default startup action is `listen` for a host and `join` for a guest.
`--panel` adds the opt-in flag for the separate [local control panel](SESSION-PANEL.md).
Guest-only `--power-hotkeys` adds `ME3_SQUAD_COOP_POWER_HOTKEYS=1` for the
experimental Ctrl+Alt+1..9 power slot adapter. It is off by default and does not
require the panel. Slots capture native power names and retain them across
catalog changes in the same process; see [power input](POWER-INPUT-RESEARCH.md). Host plans refuse this
option. The adapter itself does not alter native key bindings.
Host-only `--auto-relisten` adds `ME3_SQUAD_COOP_AUTO_RELISTEN=1`. After an
earlier verified runtime listener, it can schedule the checked Listen path in
a ready standalone mission whose old world and driver were positively disposed.
It is off by default, does not initiate mission travel or reconnect the guest,
and permits only one automatic intent after each successful listener. Cancel pending
connection suspends automatic scheduling for the rest of that game process;
manual requests remain available. See [mission transitions](MISSION-TRANSITION.md).
Guest-only `--auto-rejoin` adds `ME3_SQUAD_COOP_AUTO_REJOIN=1`. It can request
one ordinary retry after an observed owned/registered Ashley client world's
root and driver are positively disposed and a clean standalone player context
is ready. It does not retry failed first connections, force a disconnect or
select a mission. Cancel suspends it until process exit, while manual Retry
join remains available. Host readiness and successful reconnection are not
guaranteed; see [mission transitions](MISSION-TRANSITION.md).
Both roles can opt into `--ready-channel`, using UDP game-port+1 for advisory
host readiness. With this flag, automatic guest recovery waits for a fresh
reply before consuming its one-shot intent and before native dispatch. It
requires the host to enable the same channel; manual Join/Retry is independent.
Port65535 is refused for this option. See [readiness channel](READY-CHANNEL.md).
`--start attach` prepares configuration for attaching the mod to an existing
native network session; it does not request Listen or a join. Host/join and
guest/listen combinations are rejected. A guest requires an explicit peer;
a host must not supply one. Loopback, wildcard, invalid or noncanonical IP
literals and invalid ports are rejected through the existing endpoint validator.
The planner does not verify interface ownership, subnet routing or reachability.

Output contains private endpoint addresses and must stay local. A named output
file must not exist already; the tool will not overwrite it. Omitting `--output`
prints the plan to stdout. The JSON is not a game configuration file: the current
ASI reads environment variables, not this JSON, so copying it into a game folder
does not activate the mod.

`RuntimeSessionPlan` in `src/runtime_session_plan.py` provides the shared pure
configuration model for a future launcher or UI. `arguments` returns distinct
native `-MULTIHOME` and `-PORT` argv elements. `environment` supplies only the
ordinary runtime role/startup settings. `child_environment(inherited)` produces
a new environment mapping by removing all inherited `ME3_SQUAD_COOP_` keys
case-insensitively, then adding the plan's settings. This prevents stale peer,
startup and protected-probe switches from leaking into a new ordinary session.
It does not modify the supplied mapping or process environment.

A future launcher must pass that returned environment and argv list to its
verified launch path. An environment overlay alone is insufficient: absent
keys such as an attach plan's START must actually be removed. Never interpolate
the values into a shell command. Installation/fingerprint/DLC checks, independent
test-copy protection, original-file locks and bounded process cleanup remain
separate requirements. No executable path or launch operation is provided here.

The two Python sources have been syntax-parsed only. No new behavior tests or
game runs have been performed for this planner. An integrated player launcher
and in-game session controls remain unfinished. A separate local control panel
candidate is now compiled/syntax-parsed, with integrated validation deferred.
