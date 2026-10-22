"""Bounded, share-lock-guarded test of the independent copy."""
import argparse
import hashlib
import json
import runpy
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / "src"))
from share_guard import ReadLocks, selftest as lock_selftest
from build_state import verify_current
PRIVATE = ROOT.parent / "coop-anything" / "local" / "me3-le"
SANDBOX = PRIVATE / "test-install" / "ME3"
BUILD = ROOT / "local" / "build"
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--action", choices=["observe", "travel-mars", "possess-ashley", "move-ashley", "host-mars", "host-pair"], default="observe")
parser.add_argument("--seconds", type=int, default=35)
args = parser.parse_args()
maximum = 480 if args.action == "host-pair" else 120
if not 20 <= args.seconds <= maximum:
    parser.error(f"seconds must be between 20 and {maximum} for this action")


def hash_file(path):
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(4 * 1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def profile_snapshot():
    profile = Path(json.loads((PRIVATE / "profile-location.json").read_text())["path"])
    return sorted([{"path": p.relative_to(profile).as_posix(), "size": p.stat().st_size, "sha256": hash_file(p)}
                   for p in profile.rglob("*") if p.is_file()], key=lambda row: row["path"])


verify_current(ROOT)
marker = json.loads((SANDBOX / ".me3-coop-sandbox.json").read_text())
assert marker["status"] == "verified-copy" and marker["destination"] == str(SANDBOX)
exe = SANDBOX / "Binaries" / "Win64" / "MassEffect3.exe"
assert hash_file(exe) == "b453871eb6eacff0c646490ffbc588f743399a829b1f9e31cd7775dc95da8001"
assert not (SANDBOX / "Binaries" / "Win64" / "ASI" / "lemp.asi").exists()
assert not (SANDBOX / "BioGame" / "DLC" / "DLC_MOD_LEMultiplayer").exists()

before = profile_snapshot()
# A fresh positive protection test is required immediately before each boot.
guard = runpy.run_path(str(ROOT / "tests" / "test_probe.py"))
runtime = SANDBOX / "ME3SquadCoopRuntime"
if not runtime.exists():
    guard["owned_directory"](runtime, low=True)
captures = runtime / "captures"
if not captures.resolve().is_relative_to(runtime.resolve()):
    raise RuntimeError("capture directory escaped the runtime")
captures.mkdir(exist_ok=True)
documents = runtime / "Documents"
if not documents.exists():
    guard["owned_directory"](documents, low=True)
for role in ("host", "client"):
    role_docs = runtime / ("Documents-" + role)
    if not role_docs.exists():
        guard["owned_directory"](role_docs, low=True)
for parent in (documents, runtime / "Documents-host", runtime / "Documents-client"):
    screenshots = parent / "BioWare" / "Mass Effect Legendary Edition" / "ScreenShots"
    if not screenshots.resolve().is_relative_to(runtime.resolve()):
        raise RuntimeError("private screenshot path escaped the runtime")
    screenshots.mkdir(parents=True, exist_ok=True)
# Only the private test copy's loader log directory becomes low-writable.
# Keep its baseline logs intact under a different name for inspection.
logs = SANDBOX / "Binaries" / "Win64" / "Logs"
baseline_logs = logs.with_name("Logs.baseline")
if not baseline_logs.exists():
    assert str(logs.resolve()).startswith(str(SANDBOX.resolve()) + "\\")
    assert str(baseline_logs.resolve()).startswith(str(SANDBOX.resolve()) + "\\")
    logs.rename(baseline_logs)
    guard["owned_directory"](logs, low=True)
probe = SANDBOX / "Binaries" / "Win64" / "ASI" / "ME3SquadCoopProbe.asi"
if probe.exists():
    if hash_file(probe) != hash_file(BUILD / "ME3SquadCoopProbe.asi"):
        old = probe.with_name("ME3SquadCoopProbe." + hash_file(probe)[:12] + ".stage0-backup")
        assert not old.exists()
        probe.rename(old)
        with probe.open("xb") as f:
            f.write((BUILD / "ME3SquadCoopProbe.asi").read_bytes())
else:
    with probe.open("xb") as f:
        f.write((BUILD / "ME3SquadCoopProbe.asi").read_bytes())
enable = SANDBOX / "ME3SquadCoop.probe.enabled"
disable = SANDBOX / "ME3SquadCoop.probe.disabled"
if disable.exists():
    assert not enable.exists(), "contradictory probe gate markers"
    disable.rename(enable)
if not enable.exists():
    with enable.open("x") as f:
        f.write("Sandbox research probe. Not a co-op mod.\n")
(runtime / "test-action.txt").write_text(args.action if args.action != "observe" else "", encoding="ascii")

boot_started = time.time()
lock_proof = lock_selftest(ROOT / "local")
profile = Path(json.loads((PRIVATE / "profile-location.json").read_text())["path"])
original = Path(marker["source"])
protected_files = [original / e["path"] for e in marker["files"]] + [profile / e["path"] for e in before]
try:
    with ReadLocks(protected_files) as frozen:
        print(f"Protected {len(frozen.handles)} original files with OS sharing locks", flush=True)
        result = subprocess.run([str(BUILD / "sandbox_launch.exe"), "--boot-probe-sharelock", str(SANDBOX), str(lock_proof), str(args.seconds)],
                                capture_output=True, text=True, timeout=args.seconds + 15)
finally:
    # Job closure kills any remaining child before the protection is released.
    if enable.exists():
        assert not disable.exists()
        enable.rename(disable)
    (runtime / "test-action.txt").write_text("", encoding="ascii")
print(result.stdout.strip())
if result.stderr.strip():
    print(result.stderr.strip())
after = profile_snapshot()
files = [p for p in runtime.glob("squad_probe_*.jsonl") if p.stat().st_mtime >= boot_started]
samples = []
for path in files:
    samples += [json.loads(line) for line in path.read_text().splitlines() if line.strip()]
report = {
    "stage": {"observe": 0, "travel-mars": 1, "possess-ashley": 2, "move-ashley": 3, "host-mars": 4, "host-pair": 6}[args.action], "action": args.action, "duration_limit": args.seconds, "lemp_active_in_test": False,
    "launcher_exit": result.returncode,
    "protection_test_passed": True,
    "protection_method": "OS read-sharing locks + private Documents + game file-write import guard",
    "protected_original_files": len(protected_files),
    "original_profile_unchanged": before == after,
    "original_profile_files_before": len(before), "original_profile_files_after": len(after),
    "observer_loaded": any(s.get("observer_loaded") is True for s in samples),
    "game_thread_bridge": any(s.get("bridge") == "installed" for s in samples),
    "travel_dispatched": any(s.get("phase") == "dispatch" for s in samples),
    "local_pawn_observed": any(s.get("local_pawn") for s in samples),
    "squad_observed": any(s.get("squad_present") is True for s in samples),
    "ashley_possessed": any(s.get("ashley_controller_match") is True for s in samples),
    "shepard_restored": any(s.get("shepard_controller_match") is True for s in samples),
    "ashley_ai_restored": any(s.get("ashley_ai_match") is True for s in samples),
    "camera_host_positive": any(s.get("camera_baseline") == "shepard-positive" for s in samples),
    "squad_identity_restored": any(s.get("squad_identity_restored") is True for s in samples),
    "movement_result": next((s for s in samples if s.get("movement") == "result"), None),
    "listen_server_observed": any(s.get("net_mode") == 2 for s in samples),
    "last_network_state": next((s for s in reversed(samples) if s.get("network") == "state"), None),
    "engine_samples": sum("objects" in s for s in samples),
    "last_sample": samples[-1] if samples else None,
    "campaign_coop_verified": False,
    "boot_passed": result.returncode == 124 and any("objects" in s for s in samples),
}
passed = report["boot_passed"] and report["original_profile_unchanged"] and report["observer_loaded"]
if args.action != "observe":
    passed = passed and report["game_thread_bridge"] and report["travel_dispatched"] and report["local_pawn_observed"] and report["squad_observed"]
if args.action in ("possess-ashley", "move-ashley"):
    passed = passed and report["ashley_possessed"] and report["shepard_restored"] and report["ashley_ai_restored"]
if args.action == "move-ashley":
    move = report["movement_result"] or {}
    passed = passed and report["camera_host_positive"] and report["squad_identity_restored"] and move.get("neutral_distance", 999) < 10 and move.get("input_distance", 0) > 30 and move.get("shepard_distance", 999) < 10 and move.get("camera_targets_guest") is True and move.get("camera_distance", 0) > 30 and move.get("camera_to_guest", 9999) < 1000
if args.action in ("host-mars", "host-pair"):
    passed = passed and report["listen_server_observed"]
if args.action == "host-pair":
    passed = passed and any(s.get("net_mode") == 2 and s.get("max_players") == 2 for s in samples)
report["test_passed"] = bool(passed)
(PRIVATE / "boot-probe-result.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
if args.action != "observe":
    (PRIVATE / (args.action + "-result.json")).write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
print(json.dumps(report, indent=2))
if before != after:
    raise RuntimeError("original profile changed; stop before any further game work")
if not report["observer_loaded"]:
    raise SystemExit("No engine observer signal. Boot is not a passing game test.")
if not report["boot_passed"]:
    raise SystemExit("Game ended before the observation deadline. Boot is not a passing game test.")
if not report["test_passed"]:
    raise SystemExit("Requested game action did not meet its assertions; inspect the private result and log.")
