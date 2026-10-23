"""Observe one explicitly supplied protected test PID; never launch or modify it."""
from __future__ import annotations

import argparse
import json
import math
from pathlib import Path
import sys
import time

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))
from authoritative_enemy_sampler import WindowsProcessMemory
from gameplay_entry_observer import observe_entry, evaluate_entry_interval


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pid", type=int, required=True)
    parser.add_argument("--sandbox", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True, help="New private JSONL file outside the game sandbox")
    parser.add_argument("--seconds", type=float, default=30)
    parser.add_argument("--interval", type=float, default=0.5)
    args = parser.parse_args(argv)
    if (args.pid <= 0 or not math.isfinite(args.seconds) or not 0.1 <= args.seconds <= 120 or
            not math.isfinite(args.interval) or not 0.1 <= args.interval <= 5):
        parser.error("positive PID, duration 0.1..120s and interval 0.1..5s required")
    if args.output.suffix != ".jsonl" or args.output.resolve().is_relative_to(args.sandbox.resolve()):
        parser.error("output must be a new private .jsonl file outside the game sandbox")
    rows = []
    try:
        with WindowsProcessMemory(args.pid, sandbox_dir=args.sandbox) as reader, args.output.open("x", encoding="utf-8") as stream:
            started = time.monotonic()
            deadline = started + args.seconds
            while time.monotonic() < deadline:
                row = observe_entry(reader, reader.base_address, deadline=min(deadline, time.monotonic() + 1))
                row.update(pid=args.pid, elapsed_seconds=time.monotonic() - started)
                rows.append(row)
                stream.write(json.dumps(row) + "\n")
                stream.flush()
                print(json.dumps({key: row[key] for key in
                                  ("elapsed_seconds", "status", "current_map_verified", "blockers")}), flush=True)
                remaining = deadline - time.monotonic()
                if remaining > 0:
                    time.sleep(min(args.interval, remaining))
        summary = {"exact_mars_world_observed": any(row["current_map_verified"] for row in rows),
                   "ordinary_standalone_interval_observed": evaluate_entry_interval(rows, maximum_gap_seconds=args.interval + 1.1),
                   "playable_coop_verified": False, "samples": len(rows)}
        print(json.dumps(summary), flush=True)
        return 0 if summary["ordinary_standalone_interval_observed"] else 1
    except (OSError, RuntimeError, ValueError) as exc:
        print(f"Observation failed: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
