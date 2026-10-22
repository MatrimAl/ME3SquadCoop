"""Bounded, read-only Windows process-memory CLI sampler for authoritative enemy health and shields.

Usage:
    python tools/sample_authoritative_combat.py --pid <HOST_PID> [--seconds 10.0] [--interval 0.25]

Requirements:
    - Never launches, injects into, or modifies a game process.
    - Requires explicitly supplied host PID.
    - Verifies isolated executable path and SHA-256.
    - Outputs private JSONL records with monotonic timestamps.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys
import time

# Ensure src/ is on Python search path
SRC_DIR = Path(__file__).resolve().parents[1] / "src"
if str(SRC_DIR) not in sys.path:
    sys.path.insert(0, str(SRC_DIR))

from authoritative_enemy_sampler import (
    AuthoritativeEnemySampler,
    WindowsProcessMemory,
    compute_file_sha256,
    evaluate_damage_decrease,
    verify_isolated_executable,
    MAX_BOUNDED_DURATION_S,
    MIN_BOUNDED_DURATION_S,
    MAX_BOUNDED_INTERVAL_S,
    MIN_BOUNDED_INTERVAL_S,
    SUPPORTED_EXECUTABLE_SHA256,
)


def get_default_sandbox() -> Path:
    """Resolve default isolated sandbox path."""
    repo_root = Path(__file__).resolve().parents[1]
    # Common layout: work/ME3SquadCoop-*/... with sibling work/coop-anything/local/me3-le/test-install/ME3
    candidate1 = repo_root.parent / "coop-anything" / "local" / "me3-le" / "test-install" / "ME3"
    if candidate1.exists():
        return candidate1
    # Fallback to local test-install if colocated
    candidate2 = repo_root / "local" / "test-install" / "ME3"
    return candidate2


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--pid", type=int, required=True, help="Explicitly supplied host game PID (required)")
    parser.add_argument(
        "--seconds",
        type=float,
        default=10.0,
        help=f"Sampling duration in seconds ({MIN_BOUNDED_DURATION_S} to {MAX_BOUNDED_DURATION_S}, default: 10.0)",
    )
    parser.add_argument(
        "--interval",
        type=float,
        default=0.25,
        help=f"Sampling interval in seconds ({MIN_BOUNDED_INTERVAL_S} to {MAX_BOUNDED_INTERVAL_S}, default: 0.25)",
    )
    parser.add_argument(
        "--sandbox",
        type=Path,
        default=None,
        help="Path to isolated game test-install sandbox directory",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=None,
        help="Path for output private JSONL file (defaults to local/me3-le/...)",
    )
    parser.add_argument(
        "--target",
        type=str,
        default=None,
        help="Optional target actor hex pointer to focus evaluation on",
    )
    args = parser.parse_args()

    # Enforce bounded parameters
    if not (MIN_BOUNDED_DURATION_S <= args.seconds <= MAX_BOUNDED_DURATION_S):
        print(
            f"Error: --seconds must be between {MIN_BOUNDED_DURATION_S} and {MAX_BOUNDED_DURATION_S}",
            file=sys.stderr,
        )
        return 1
    if not (MIN_BOUNDED_INTERVAL_S <= args.interval <= MAX_BOUNDED_INTERVAL_S):
        print(
            f"Error: --interval must be between {MIN_BOUNDED_INTERVAL_S} and {MAX_BOUNDED_INTERVAL_S}",
            file=sys.stderr,
        )
        return 1

    sandbox_dir = args.sandbox if args.sandbox else get_default_sandbox()

    # Determine private output path
    output_path = args.output
    if output_path is None:
        if sandbox_dir.name == "ME3" and sandbox_dir.parent.name == "test-install":
            private_dir = sandbox_dir.parent.parent
        else:
            private_dir = sandbox_dir.parent
        if not private_dir.exists():
            private_dir = Path(__file__).resolve().parents[1] / "local"
        private_dir.mkdir(parents=True, exist_ok=True)
        tick = int(time.monotonic() * 1000) & 0xFFFFFFFF
        output_path = private_dir / f"combat-authoritative-samples-{args.pid}-{tick}.jsonl"

    print(f"Authoritative Enemy Combat Sampler")
    print(f"Target PID:           {args.pid}")
    print(f"Duration:             {args.seconds:.2f}s")
    print(f"Interval:             {args.interval:.2f}s")
    print(f"Sandbox Directory:    {sandbox_dir}")
    print(f"Output JSONL:         {output_path}")
    print(f"Permissions:          STRICT READ-ONLY (0x410 PROCESS_VM_READ | PROCESS_QUERY_INFORMATION)")
    print(f"Executable SHA-256:   {SUPPORTED_EXECUTABLE_SHA256}")

    try:
        with WindowsProcessMemory(args.pid, sandbox_dir=sandbox_dir) as memory:
            print(f"Connected to PID {args.pid}. Module base: 0x{memory.base_address:x}")
            print(f"Executable path verified: {memory.module_path}")

            sampler = AuthoritativeEnemySampler(memory, memory.base_address)

            samples_collected = 0
            with output_path.open("w", encoding="utf-8") as stream:
                def on_sample(sample: dict):
                    nonlocal samples_collected
                    samples_collected += 1
                    status = sample["status"]
                    hostiles = sample["hostiles_count"]
                    failures = len(sample["read_failures"])
                    print(
                        f"[{sample['sample_index']:03d}] t={sample['monotonic_time_s']:.2f}s "
                        f"status={status} hostiles={hostiles} failures={failures}"
                    )

                samples = sampler.run_bounded_sampling(
                    duration_seconds=args.seconds,
                    interval_seconds=args.interval,
                    output_stream=stream,
                    callback=on_sample,
                )

            # Analyze observations
            analysis = evaluate_damage_decrease(samples, target_pointer=args.target)
            print("\nSampling Summary:")
            print(f"Total samples collected: {len(samples)}")
            print(f"Output written to:       {output_path}")
            print(f"Decrease observed:       {analysis['decrease_observed']}")
            if analysis["decrease_observed"]:
                print(f"Target pointer:          {analysis['target_pointer']}")
                print(f"Health decrease:         {analysis['health_decrease']:.2f}")
                print(f"Shield decrease:         {analysis['shield_decrease']:.2f}")
                print(f"Guest attribution:       {analysis['guest_damage_attribution_proven']} (UNPROVEN - separate attribution required)")
                print(f"Attribution note:        {analysis['attribution_note']}")

            return 0
    except (PermissionError, RuntimeError, OSError, ValueError) as exc:
        print(f"Error during sampling: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
