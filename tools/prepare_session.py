"""Prepare host/guest connection settings without launching or changing a game."""
import argparse
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))
from runtime_session_plan import RuntimeSessionPlan


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--role", choices=("host", "guest"), required=True)
    parser.add_argument("--bind", required=True, help="local LAN/Radmin IPv4")
    parser.add_argument("--peer", help="host IPv4; required for a guest")
    parser.add_argument("--port", default="17779")
    parser.add_argument("--start", choices=("listen", "join", "attach"))
    parser.add_argument("--panel", action="store_true", help="opt into the separate local session control panel")
    parser.add_argument("--power-hotkeys", action="store_true", help="guest only: opt into Ctrl+Alt+1..9 power slots")
    parser.add_argument("--native-power-hotkeys", action="store_true", help="guest only: route native UseAbility 0..7 to guest named power slots")
    parser.add_argument("--native-power-icons", action="store_true", help="guest only: opt into native quickslot icons; requires --native-power-hotkeys")
    parser.add_argument("--auto-relisten", action="store_true", help="host only: request Listen after verified old-world disposal")
    parser.add_argument("--auto-rejoin", action="store_true", help="guest only: retry once after an owned client world's verified disposal")
    parser.add_argument("--ready-channel", action="store_true", help="opt into host readiness synchronization on UDP game port+1")
    parser.add_argument("--input-bootstrap", action="store_true", help="guest only: initialize empty native input through the local profile")
    parser.add_argument("--output", type=Path,
                        help="create a private JSON plan; existing files are never overwritten")
    args = parser.parse_args()
    try:
        plan = RuntimeSessionPlan(args.role, args.bind, args.port, args.peer, args.start,
                                  panel=args.panel, power_hotkeys=args.power_hotkeys,
                                  auto_relisten=args.auto_relisten, auto_rejoin=args.auto_rejoin,
                                  ready_channel=args.ready_channel, input_bootstrap=args.input_bootstrap,
                                  native_power_hotkeys=args.native_power_hotkeys,
                                  native_power_icons=args.native_power_icons)
        text = json.dumps(plan.to_dict(), indent=2) + "\n"
        if args.output is None:
            sys.stdout.write(text)
        else:
            with args.output.open("x", encoding="utf-8", newline="\n") as stream:
                stream.write(text)
            print("Session plan created; no game launched or installation verified.")
    except (ValueError, OSError) as error:
        # Do not repeat endpoint addresses or private output paths in errors.
        print("Cannot prepare session: " + type(error).__name__, file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
