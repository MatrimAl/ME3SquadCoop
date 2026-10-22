"""Bounded two-process connection observation; never a campaign co-op verdict."""
import ctypes
import argparse
from ctypes import wintypes
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time
import uuid

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / "src"))
from share_guard import ReadLocks, selftest as lock_selftest
from build_state import verify_current
from pair_verification import connection_evidence, control_evidence
from session_verification import session_evidence
from direct_ip_config import DirectIPConfig, validate_port
from progress_verification import progress_evidence
from guest_context_verification import guest_context_evidence
from cinematic_verification import cinematic_motion_evidence
from fire_stop_verification import fire_stop_evidence
from real_dialogue_verification import real_dialogue_evidence

PRIVATE = ROOT.parent / "coop-anything" / "local" / "me3-le"
SANDBOX = PRIVATE / "test-install" / "ME3"
BUILD = ROOT / "local" / "build"
RUNTIME = SANDBOX / "ME3SquadCoopRuntime"
HOST_SECONDS = 120
CLIENT_SECONDS = 45
READY_SECONDS = 100


def hash_file(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(4 * 1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def profile_snapshot(profile):
    return sorted(({
        "path": path.relative_to(profile).as_posix(),
        "size": path.stat().st_size,
        "sha256": hash_file(path),
    } for path in profile.rglob("*") if path.is_file()), key=lambda row: row["path"])


def running_game_pids():
    """Toolhelp avoids shell interpolation, accounts, and an external dependency."""
    class Entry(ctypes.Structure):
        _fields_ = [("dwSize", wintypes.DWORD), ("cntUsage", wintypes.DWORD),
                    ("th32ProcessID", wintypes.DWORD), ("th32DefaultHeapID", ctypes.c_size_t),
                    ("th32ModuleID", wintypes.DWORD), ("cntThreads", wintypes.DWORD),
                    ("th32ParentProcessID", wintypes.DWORD), ("pcPriClassBase", wintypes.LONG),
                    ("dwFlags", wintypes.DWORD), ("szExeFile", wintypes.WCHAR * 260)]
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel.CreateToolhelp32Snapshot.argtypes = [wintypes.DWORD, wintypes.DWORD]
    kernel.CreateToolhelp32Snapshot.restype = wintypes.HANDLE
    kernel.Process32FirstW.argtypes = [wintypes.HANDLE, ctypes.POINTER(Entry)]
    kernel.Process32NextW.argtypes = [wintypes.HANDLE, ctypes.POINTER(Entry)]
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    snapshot = kernel.CreateToolhelp32Snapshot(2, 0)
    if snapshot == ctypes.c_void_p(-1).value:
        raise ctypes.WinError(ctypes.get_last_error())
    found = []
    try:
        entry = Entry()
        entry.dwSize = ctypes.sizeof(entry)
        if not kernel.Process32FirstW(snapshot, ctypes.byref(entry)):
            raise ctypes.WinError(ctypes.get_last_error())
        while True:
            if entry.szExeFile.casefold() == "masseffect3.exe":
                found.append(entry.th32ProcessID)
            if not kernel.Process32NextW(snapshot, ctypes.byref(entry)):
                if ctypes.get_last_error() != 18:  # ERROR_NO_MORE_FILES
                    raise ctypes.WinError(ctypes.get_last_error())
                break
    finally:
        kernel.CloseHandle(snapshot)
    return found


def log_inventory():
    return {path.name: (path.stat().st_mtime_ns, path.stat().st_size)
            for path in RUNTIME.glob("squad_probe_*.jsonl")}


def fresh_role_logs(role, baseline, started_ns):
    """Require a new/changed file, new timestamp, and matching native PID header."""
    found = []
    for path in RUNTIME.glob("squad_probe_*.jsonl"):
        try:
            info = path.stat()
            signature = (info.st_mtime_ns, info.st_size)
            if info.st_mtime_ns < started_ns or baseline.get(path.name) == signature:
                continue
            raw = path.read_bytes()
        except (FileNotFoundError, PermissionError):
            continue
        # A concurrent writer may leave a partial final line: wait for its newline.
        rows = []
        for line in raw.splitlines(keepends=True):
            if not line.endswith(b"\n"):
                continue
            try:
                row = json.loads(line)
            except (ValueError, UnicodeDecodeError):
                continue
            if isinstance(row, dict):
                rows.append(row)
        header = next((row for row in rows if row.get("observer_loaded") is True
                       and row.get("role") == role), None)
        if header is None or type(header.get("pid")) is not int:
            continue
        pid = header["pid"]
        if pid <= 0 or path.name != f"squad_probe_{pid}.jsonl":
            continue
        found.append({"pid": pid, "log": path.name, "rows": rows})
    return found


def summarize(logs):
    rows = [row for log in logs for row in log["rows"]]
    network = [row for row in rows if "net_mode" in row]
    counts = {}
    for name in ("controllers", "pri", "net_drivers", "net_connections"):
        values = [row[name] for row in rows if type(row.get(name)) is int]
        counts[name] = {"maximum": max(values), "last": values[-1]} if values else None
    errors = []
    for row in rows:
        value = row.get("pending_error")
        if value and value not in errors:
            errors.append(value)
    return {"native_pids": [log["pid"] for log in logs],
            "connection_evidence": connection_evidence(rows),
            "logs": [log["log"] for log in logs],
            "network_states": network, "object_counts": counts,
            "pending_errors": errors,
            "net_modes_observed": sorted({row["net_mode"] for row in network
                                           if type(row.get("net_mode")) is int}),
            "last_network_state": network[-1] if network else None}


def await_exit(process, deadline, stop_first=False):
    """Closing a native launcher closes its kill-on-close child job."""
    if process is None:
        return None
    if stop_first and process.poll() is None:
        process.terminate()
    try:
        return process.wait(timeout=max(0.1, deadline - time.monotonic()))
    except subprocess.TimeoutExpired:
        # Host normally completes its own finally block and gate cleanup first.
        # This fallback is exceptional, bounded, and reported as a failed test.
        process.kill()
        return process.wait(timeout=10)


def stop_remaining_sandbox_games():
    """Exceptional fallback after launcher cleanup, restricted to this exact copy."""
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
    kernel.OpenProcess.restype = wintypes.HANDLE
    kernel.QueryFullProcessImageNameW.argtypes = [wintypes.HANDLE, wintypes.DWORD,
                                                 wintypes.LPWSTR, ctypes.POINTER(wintypes.DWORD)]
    kernel.TerminateProcess.argtypes = [wintypes.HANDLE, wintypes.UINT]
    kernel.WaitForSingleObject.argtypes = [wintypes.HANDLE, wintypes.DWORD]
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    expected = os.path.normcase(str((SANDBOX / "Binaries" / "Win64" / "MassEffect3.exe").resolve()))
    stopped = []
    for pid in running_game_pids():
        handle = kernel.OpenProcess(0x1000 | 0x100000 | 1, False, pid)
        if not handle:
            if ctypes.get_last_error() == 87:  # exited between snapshot and OpenProcess
                continue
            raise ctypes.WinError(ctypes.get_last_error())
        try:
            length = wintypes.DWORD(32768)
            name = ctypes.create_unicode_buffer(length.value)
            if not kernel.QueryFullProcessImageNameW(handle, 0, name, ctypes.byref(length)):
                raise ctypes.WinError(ctypes.get_last_error())
            if os.path.normcase(str(Path(name.value).resolve())) != expected:
                continue
            if kernel.WaitForSingleObject(handle, 0) == 0:
                continue
            if not kernel.TerminateProcess(handle, 125):
                raise ctypes.WinError(ctypes.get_last_error())
            if kernel.WaitForSingleObject(handle, 10000) != 0:
                raise RuntimeError("sandbox game did not exit during exceptional cleanup")
            stopped.append(pid)
        finally:
            kernel.CloseHandle(handle)
    return stopped


def run(prewarm_mars=False, refresh_host_map=False, reload_host_mars=False, pair_control=False, session_test=False, bind_ipv4="127.0.0.1", port=17779):
    verify_current(ROOT)
    loopback_qa = bind_ipv4.startswith("127.")
    host_config = DirectIPConfig("host", bind_ipv4, port, loopback_qa=loopback_qa)
    guest_config = DirectIPConfig("client", bind_ipv4, port, peer_ipv4=bind_ipv4, loopback_qa=loopback_qa)
    progress_test = os.environ.get("ME3_SQUAD_COOP_PROGRESS_TEST") == "1"
    extended_qa = progress_test and os.environ.get("ME3_SQUAD_COOP_EXTENDED_QA") == "1"
    host_seconds = 480 if extended_qa else 360 if progress_test else 240 if session_test else 210 if pair_control else 180
    client_seconds = 360 if extended_qa else 240 if progress_test else 120 if session_test else 90 if pair_control else 60 if prewarm_mars else CLIENT_SECONDS
    report = {"stage": "two-real-client-connection-observation",
              "campaign_coop_verified": False, "connection_states_verified": False,
              "join_patches_enabled": False,
              "host_limit_seconds": host_seconds, "client_limit_seconds": client_seconds,
              "client_prewarm_mars": prewarm_mars,
              "refresh_host_master_map": refresh_host_map,
              "host_reload_mars": reload_host_mars,
              "pair_control_requested": pair_control,
              "session_lifecycle_requested": session_test,
              "transport_configuration": host_config.to_dict(),
              "two_computer_radmin_verified": False,
              "protection_method": "OS read-sharing locks held across both launchers",
              "host": {}, "client": {}}
    profile = None
    before = None
    host = guest = None
    host_started = None
    baseline = {}
    host_started_ns = guest_started_ns = 0
    failed = None
    outputs = []
    try:
        marker = json.loads((SANDBOX / ".me3-coop-sandbox.json").read_text())
        if marker.get("status") != "verified-copy" or Path(marker["destination"]).resolve() != SANDBOX.resolve():
            raise RuntimeError("test copy is not a verified independent sandbox")
        if running_game_pids():
            raise RuntimeError("an existing MassEffect3 process must exit before this probe")
        exe = SANDBOX / "Binaries" / "Win64" / "MassEffect3.exe"
        if hash_file(exe) != "b453871eb6eacff0c646490ffbc588f743399a829b1f9e31cd7775dc95da8001":
            raise RuntimeError("unsupported test executable")
        if (exe.parent / "ASI" / "lemp.asi").exists() or (SANDBOX / "BioGame" / "DLC" / "DLC_MOD_LEMultiplayer").exists():
            raise RuntimeError("LEMP must be disabled in the test copy")
        profile = Path(json.loads((PRIVATE / "profile-location.json").read_text())["path"])
        before = profile_snapshot(profile)
        original = Path(marker["source"])
        protected = []
        for row in marker["files"]:
            relative = Path(row["path"])
            if relative.is_absolute() or ".." in relative.parts:
                raise RuntimeError("unsafe original manifest entry")
            protected.append(original / relative)
        protected.extend(profile / row["path"] for row in before)
        baseline = log_inventory()
        proof = lock_selftest(ROOT / "local")
        report["protection_test_passed"] = True
        # Keep the outer locks until BOTH launchers have ended, even if boot_probe
        # releases its own locks or its marker after an early failure.
        with ReadLocks(protected) as frozen:
            report["protected_original_files"] = len(frozen.handles)
            stamp = uuid.uuid4().hex[:10]
            host_output = PRIVATE / f"pair-host-{stamp}.txt"
            guest_output = PRIVATE / f"pair-client-{stamp}.txt"
            report["launcher_output_logs"] = [host_output.name, guest_output.name]
            host_stream = host_output.open("x", encoding="utf-8")
            outputs.append(host_stream)
            guest_stream = guest_output.open("x", encoding="utf-8")
            outputs.append(guest_stream)
            try:
                env = dict(os.environ, ME3_SQUAD_COOP_TEST_ROLE="host")
                env.update(ME3_SQUAD_COOP_BIND_IPV4=host_config.bind_ipv4, ME3_SQUAD_COOP_PORT=str(host_config.port), ME3_SQUAD_COOP_LOOPBACK_QA="1" if loopback_qa else "0")
                env.pop("ME3_SQUAD_COOP_PEER_IPV4", None)
                env["ME3_SQUAD_COOP_REFRESH_HOST_MAP"] = "1" if refresh_host_map else "0"
                env["ME3_SQUAD_COOP_RELOAD_HOST_MARS"] = "1" if reload_host_mars else "0"
                env["ME3_SQUAD_COOP_PAIR_CONTROL"] = "1" if pair_control else "0"
                env["ME3_SQUAD_COOP_SESSION_TEST"] = "1" if session_test else "0"
                host_started_ns = time.time_ns()
                host_started = time.monotonic()
                host = subprocess.Popen([sys.executable, str(ROOT / "boot_probe.py"),
                                         "--action", "host-pair", "--seconds", str(host_seconds)],
                                        cwd=ROOT, env=env, stdout=host_stream, stderr=subprocess.STDOUT)
                ready = False
                while time.monotonic() - host_started < READY_SECONDS:
                    logs = fresh_role_logs("host", baseline, host_started_ns)
                    refresh_ready = not refresh_host_map or any(row.get("network") == "master-map-refreshed" and row.get("verified") is True
                                                               for log in logs for row in log["rows"])
                    if refresh_ready and any(row.get("net_mode") == 2 and row.get("max_players") == 2
                                             for log in logs for row in log["rows"]):
                        ready = True
                        report["host_ready_after_seconds"] = round(time.monotonic() - host_started, 3)
                        break
                    if host.poll() is not None:
                        break
                    time.sleep(0.25)
                if not ready:
                    raise RuntimeError(f"host did not report a two-player listen server within {READY_SECONDS} seconds")
                for name in ("Documents-host", "Documents-client"):
                    directory = RUNTIME / name
                    if not directory.is_dir() or directory.is_symlink() or directory.resolve().parent != RUNTIME.resolve():
                        raise RuntimeError("role-specific private Documents directory is missing or redirected")
                # Budget from host script start (before native launch) and keep
                # ten seconds for guest job closure before the host's limit.
                if time.monotonic() - host_started > host_seconds - client_seconds - 10:
                    raise RuntimeError("not enough host lifetime remains for bounded guest")
                guest_started_ns = time.time_ns()
                guest_started = time.monotonic()
                env = dict(os.environ, ME3_SQUAD_COOP_TEST_ROLE="client")
                env.update(ME3_SQUAD_COOP_BIND_IPV4=guest_config.bind_ipv4, ME3_SQUAD_COOP_PEER_IPV4=guest_config.peer_ipv4, ME3_SQUAD_COOP_PORT=str(guest_config.port), ME3_SQUAD_COOP_LOOPBACK_QA="1" if loopback_qa else "0")
                env["ME3_SQUAD_COOP_PREWARM_MARS"] = "1" if prewarm_mars else "0"
                env["ME3_SQUAD_COOP_PAIR_CONTROL"] = "1" if pair_control else "0"
                env["ME3_SQUAD_COOP_SESSION_TEST"] = "1" if session_test else "0"
                guest = subprocess.Popen([str(BUILD / "sandbox_launch.exe"), "--boot-probe-sharelock",
                                          str(SANDBOX), str(proof), str(client_seconds)],
                                         cwd=ROOT, env=env, stdout=guest_stream, stderr=subprocess.STDOUT)
                report["client_launcher_exit"] = await_exit(guest, guest_started + client_seconds + 15)
                report["host_launcher_exit"] = await_exit(host, host_started + host_seconds + 45)
            finally:
                # Guest job closes first. Host boot script gets its full cleanup
                # window so its finally block disables the marker normally.
                if guest is not None and guest.poll() is None:
                    report["client_cleanup_terminated_launcher"] = True
                    await_exit(guest, time.monotonic() + 10, stop_first=True)
                if host is not None and host.poll() is None:
                    report["host_launcher_exit"] = await_exit(host, host_started + host_seconds + 45)
                if guest is not None:
                    report["client_launcher_exit"] = guest.returncode
                if host is not None:
                    report["host_launcher_exit"] = host.returncode
                # Never release original-file locks while an orphaned child of
                # this exact sandbox remains alive. Original installs are excluded.
                report["forced_sandbox_game_cleanup_pids"] = stop_remaining_sandbox_games()
                report["host"] = summarize(fresh_role_logs("host", baseline, host_started_ns))
                report["client"] = summarize(fresh_role_logs("client", baseline, guest_started_ns)) if guest_started_ns else summarize([])
                host_raw = [row for log in fresh_role_logs("host", baseline, host_started_ns) for row in log["rows"]]
                guest_raw = [row for log in fresh_role_logs("client", baseline, guest_started_ns) for row in log["rows"]] if guest_started_ns else []
                report["control_evidence"] = control_evidence(host_raw, guest_raw)
                report["session_evidence"] = session_evidence(host_raw, guest_raw)
                report["progress_evidence"] = progress_evidence(host_raw, guest_raw)
                report["guest_context_evidence"] = guest_context_evidence(host_raw, guest_raw)
                report["cinematic_motion_evidence"] = cinematic_motion_evidence(host_raw, guest_raw)
                report["fire_stop_evidence"] = fire_stop_evidence(host_raw, guest_raw)
                report["real_dialogue_evidence"] = real_dialogue_evidence(host_raw, guest_raw)
                report["native_exception_count"] = sum("exception" in row for row in host_raw + guest_raw)
                # The parent retains locks while the before/after snapshot is read.
                report["original_profile_unchanged"] = before == profile_snapshot(profile)
    except Exception as error:
        failed = error
        report["failure_type"] = type(error).__name__
        report["failure"] = str(error)
    finally:
        for stream in outputs:
            stream.close()
        if profile is not None and before is not None:
            after = profile_snapshot(profile)
            report["original_profile_unchanged"] = before == after
            report["original_profile_files_before"] = len(before)
            report["original_profile_files_after"] = len(after)
        host_rows = report["host"].get("network_states", [])
        report["host_listen_verified"] = any(row.get("net_mode") == 2 and row.get("max_players") == 2 for row in host_rows)
        report["guest_client_mode_verified"] = 3 in report["client"].get("net_modes_observed", [])
        host_pids = set(report["host"].get("native_pids", []))
        guest_pids = set(report["client"].get("native_pids", []))
        report["distinct_native_processes"] = bool(host_pids and guest_pids and host_pids.isdisjoint(guest_pids))
        report["probe_disabled_after_run"] = not (SANDBOX / "ME3SquadCoop.probe.enabled").exists()
        report["connection_states_verified"] = bool(
            failed is None and report["host_listen_verified"] and report["guest_client_mode_verified"]
            and report["client"].get("connection_evidence", {}).get("persistent_client_mode_verified")
            and report["distinct_native_processes"] and report.get("client_launcher_exit") == 124
            and report.get("host_launcher_exit") == 0 and report.get("original_profile_unchanged")
            and not report.get("forced_sandbox_game_cleanup_pids")
            and report["probe_disabled_after_run"])
        evidence = report.get("session_evidence", {})
        report["session_lifecycle_verified"] = bool(session_test and failed is None
            and all(evidence.get(key) for key in ("protocol_valid", "persistent_control_verified", "scene_release_verified", "guest_scene_detach_verified", "reacquire_after_scene_verified", "disconnect_restore_verified", "engine_disconnect_verified"))
            and report["distinct_native_processes"] and report.get("client_launcher_exit") == 124
            and report.get("host_launcher_exit") == 0 and report.get("original_profile_unchanged")
            and report.get("native_exception_count") == 0 and not report.get("forced_sandbox_game_cleanup_pids")
            and report["probe_disabled_after_run"])
        (PRIVATE / "pair-probe-result.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        (PRIVATE / ("pair-probe-result-" + uuid.uuid4().hex[:12] + ".json")).write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(json.dumps({key: report[key] for key in (
            "host_listen_verified", "guest_client_mode_verified", "distinct_native_processes",
            "connection_states_verified", "session_lifecycle_verified", "campaign_coop_verified", "probe_disabled_after_run")}, indent=2))
        if failed is not None:
            print("Pair probe stopped: " + type(failed).__name__ + "; inspect private result.", file=sys.stderr)
    return 0 if (report["session_lifecycle_verified"] if session_test else report["connection_states_verified"]) else 1


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prewarm-mars", action="store_true")
    parser.add_argument("--refresh-host-map", action="store_true")
    parser.add_argument("--reload-host-mars", action="store_true")
    parser.add_argument("--pair-control", action="store_true")
    parser.add_argument("--session-test", action="store_true")
    parser.add_argument("--bind-ipv4", default="127.0.0.1", help="Assigned local IPv4; paired QA runs both processes on this machine")
    parser.add_argument("--port", type=validate_port, default=17779)
    args = parser.parse_args()
    if args.refresh_host_map and not args.prewarm_mars:
        parser.error("--refresh-host-map requires --prewarm-mars")
    if args.reload_host_mars and not args.refresh_host_map:
        parser.error("--reload-host-mars requires --refresh-host-map")
    if args.pair_control and not args.reload_host_mars:
        parser.error("--pair-control requires --reload-host-mars")
    if args.session_test and not args.pair_control:
        parser.error("--session-test requires --pair-control")
    raise SystemExit(run(args.prewarm_mars, args.refresh_host_map, args.reload_host_mars, args.pair_control, args.session_test, args.bind_ipv4, args.port))
