"""Control an explicitly selected running ordinary runtime; never launch a game."""
import argparse
import ctypes as C
from ctypes import wintypes as W
from pathlib import PureWindowsPath
import tkinter as tk
from tkinter import ttk


class Frame(C.LittleEndianStructure):
    _pack_ = 4
    _fields_ = [(name, C.c_uint32) for name in ("magic", "version", "bytes", "pid")] + [
        ("created", C.c_uint64), ("sequence", C.c_int32)] + [
        (name, C.c_uint32) for name in (
            "role", "enabled", "startup", "netmode", "companion_owned", "power_status",
            "power_scope", "power_count", "ack_id", "ack_result", "control_state", "tick_ms",
            "recovery_flags", "readiness_state")] + [
        ("hotkeys_enabled", C.c_uint32), ("power_slot_indices", C.c_int32 * 9),
        ("power_names", (C.c_char * 80) * 64)]


class Memory(C.LittleEndianStructure):
    _pack_ = 4
    _fields_ = [("frame", Frame), ("command", C.c_int32), ("command_id", C.c_uint32),
                ("scope", C.c_uint32), ("index", C.c_uint32), ("sent_at", C.c_uint32)]


class Session:
    def __init__(self, pid):
        if not 0 < pid <= 0xffffffff:
            raise ValueError("Invalid PID")
        self.pid, self.process, self.mapping, self.view = pid, None, None, None
        self.kernel = C.WinDLL("kernel32", use_last_error=True)
        specifications = {
            "OpenProcess": ([W.DWORD, W.BOOL, W.DWORD], W.HANDLE),
            "OpenFileMappingW": ([W.DWORD, W.BOOL, W.LPCWSTR], W.HANDLE),
            "MapViewOfFile": ([W.HANDLE, W.DWORD, W.DWORD, W.DWORD, C.c_size_t], C.c_void_p),
            "UnmapViewOfFile": ([C.c_void_p], W.BOOL),
            "CloseHandle": ([W.HANDLE], W.BOOL),
            "WaitForSingleObject": ([W.HANDLE, W.DWORD], W.DWORD),
            "GetProcessTimes": ([W.HANDLE] + [C.POINTER(W.FILETIME)] * 4, W.BOOL),
            "QueryFullProcessImageNameW": ([W.HANDLE, W.DWORD, W.LPWSTR, C.POINTER(W.DWORD)], W.BOOL),
            "InterlockedCompareExchange": ([C.POINTER(C.c_int32), C.c_int32, C.c_int32], C.c_int32),
            "InterlockedExchange": ([C.POINTER(C.c_int32), C.c_int32], C.c_int32),
            "GetTickCount": ([], W.DWORD),
        }
        for name, (args, result) in specifications.items():
            function = getattr(self.kernel, name)
            function.argtypes, function.restype = args, result
        if C.sizeof(Frame) != 5244 or C.sizeof(Memory) != 5264:
            raise RuntimeError("Unsupported panel layout")
        try:
            # Query + wait only: no process memory access, injection or termination.
            self.process = self.kernel.OpenProcess(0x101000, False, pid)
            if not self.process:
                raise C.WinError(C.get_last_error())
            image, length = C.create_unicode_buffer(32768), W.DWORD(32768)
            if not self.kernel.QueryFullProcessImageNameW(self.process, 0, image, C.byref(length)):
                raise C.WinError(C.get_last_error())
            if PureWindowsPath(image.value).name.lower() != "masseffect3.exe":
                raise RuntimeError("Selected process is not MassEffect3.exe")
            times = [W.FILETIME() for _ in range(4)]
            if not self.kernel.GetProcessTimes(self.process, *(C.byref(value) for value in times)):
                raise C.WinError(C.get_last_error())
            self.created = times[0].dwLowDateTime | times[0].dwHighDateTime << 32
            self.mapping = self.kernel.OpenFileMappingW(6, False, f"Local\\ME3SquadCoopPanel_{pid}")
            if not self.mapping:
                raise RuntimeError("No panel mapping; ordinary runtime needs ME3_SQUAD_COOP_PANEL=1")
            self.view = self.kernel.MapViewOfFile(self.mapping, 6, 0, 0, C.sizeof(Memory))
            if not self.view:
                raise C.WinError(C.get_last_error())
        except BaseException:
            self.close()
            raise

    def atomic(self, offset):
        return C.cast(self.view + offset, C.POINTER(C.c_int32))

    def snapshot(self):
        if self.kernel.WaitForSingleObject(self.process, 0) != 258:
            raise RuntimeError("Selected game process ended")
        sequence = self.atomic(Frame.sequence.offset)
        for _ in range(4):
            before = self.kernel.InterlockedCompareExchange(sequence, 0, 0)
            if before & 1:
                continue
            frame = Frame.from_buffer_copy(C.string_at(self.view, C.sizeof(Frame)))
            after = self.kernel.InterlockedCompareExchange(sequence, 0, 0)
            if before != after or after & 1:
                continue
            if (frame.magic, frame.version, frame.bytes, frame.pid, frame.created) != (
                    0x4d334350, 6, 5264, self.pid, self.created):
                raise RuntimeError("Panel identity/version unavailable or different")
            if frame.role not in (1, 2) or frame.power_count > 64:
                raise RuntimeError("Invalid panel frame")
            if frame.recovery_flags & ~15 or frame.readiness_state > 7:
                raise RuntimeError("Invalid recovery status")
            if frame.hotkeys_enabled & ~3 or any(
                    value < -1 or value >= frame.power_count for value in frame.power_slot_indices):
                raise RuntimeError("Invalid power shortcut indices")
            if (self.kernel.GetTickCount() - frame.tick_ms) & 0xffffffff > 3000:
                raise RuntimeError("Game status is stale; controls unavailable")
            return frame
        raise RuntimeError("Game status is updating")

    def command(self, code, scope=0, index=0):
        frame = self.snapshot()
        if code not in range(1, 9):
            raise ValueError("Unknown command")
        if code == 4 and (not scope or scope != frame.power_scope or not 0 <= index < frame.power_count):
            raise ValueError("Power selection expired")
        mailbox = self.atomic(Memory.command.offset)
        if self.kernel.InterlockedCompareExchange(mailbox, -1, 0) != 0:
            raise RuntimeError("A command is pending")
        try:
            # The native consumer ignores -1 until the complete payload is committed.
            # Ack snapshots publish at250ms cadence; the reserved mailbox's
            # last ID is current even before the previous ack is published.
            previous_id = C.c_uint32.from_address(self.view + Memory.command_id.offset).value
            message_id = (previous_id + 1) & 0xffffffff or 1
            for offset, value in ((Memory.command_id.offset, message_id),
                                  (Memory.scope.offset, scope), (Memory.index.offset, index)):
                C.c_uint32.from_address(self.view + offset).value = value
            if self.kernel.WaitForSingleObject(self.process, 0) != 258:
                raise RuntimeError("Selected game process ended")
            C.c_uint32.from_address(self.view + Memory.sent_at.offset).value = self.kernel.GetTickCount()
            self.kernel.InterlockedExchange(mailbox, code)
            return message_id
        except BaseException:
            self.kernel.InterlockedExchange(mailbox, 0)
            raise

    def close(self):
        if self.view:
            self.kernel.UnmapViewOfFile(self.view)
            self.view = None
        for name in ("mapping", "process"):
            handle = getattr(self, name)
            if handle:
                self.kernel.CloseHandle(handle)
                setattr(self, name, None)


STARTUP = {0: "Idle", 1: "Waiting for player control", 2: "Join dispatched", 3: "Already networked",
           4: "Native contract refused", 5: "Bad request", 6: "Host unavailable", 7: "Bind refused",
           8: "Listening", 9: "Capacity refused", 10: "Listen failed", 11: "Listen verification failed",
           12: "Waiting to retry", 13: "Retry refused", 14: "Retry expired",
           15: "Waiting for new-mission listener", 16: "New-mission listener refused", 17: "New-mission listener expired",
           18: "Pending connection request cancelled"}
POWER = {0: "Idle", 1: "Queued", 2: "Rejected", 3: "Native order accepted", 4: "Native order refused"}
CONTROL = {0: "Waiting for companion", 1: "Companion controlled", 2: "Scene / control suspended",
           3: "Waiting for mission transition", 4: "Control fault — restart required",
           5: "Companion downed", 6: "Companion dead", 7: "Companion life state unavailable"}
READINESS = {0: "Off", 1: "Host not ready", 2: "Host ready", 3: "Idle",
             4: "Waiting for host reply", 5: "Host replied ready",
             6: "Unavailable — manual connection available", 7: "Disabled"}


def recovery_hint(frame):
    """Explain sampled status without dispatching or promising recovery."""
    if frame.control_state == 4:
        return "A persistent control fault occurred. A new game session is required; repeated Join requests cannot clear this fault."
    if not frame.enabled:
        return "Mod control is disabled. Enable mod requests activation; configuration and native checks still apply."
    if frame.control_state == 3:
        return "Mission transition is still settling. Wait for native world cleanup before requesting another connection."
    if frame.control_state in (5, 6):
        return "Companion is downed or dead. Continue native revival or mission recovery in the game; reconnecting is not a revive command."
    if frame.control_state == 7:
        return "Companion life state cannot be read reliably. Power requests are unavailable until a valid sample returns."
    if frame.control_state == 2:
        return "Native scene or control suspension is active. Wait for control to return."
    if frame.control_state == 1:
        return "Companion control is available. Native combat, cooldown and target rules still apply."
    if frame.netmode in (2, 3):
        return "Native networking is active; waiting for companion control. Another Join or Listen request is not needed."
    if frame.startup in (4, 5, 6, 7, 9, 10, 11):
        return "Startup was refused or failed. Check runtime configuration and diagnostics before starting a new session."
    if frame.startup in (1, 2, 12, 15):
        if frame.role == 2 and frame.readiness_state == 4:
            return "Waiting for the host readiness reply. The host must finish loading and open its listener."
        return "Connection request is in progress. Wait for its result, or use Cancel pending connection."
    if frame.startup in (13, 14, 16, 17):
        return "Recovery did not complete. Once native loading and cleanup finish, request Retry join (guest) or Listen in new mission (host)."
    enabled_bit, suspended_bit = (1, 4) if frame.role == 1 else (2, 8)
    if frame.recovery_flags & enabled_bit and not frame.recovery_flags & suspended_bit:
        return "Automatic recovery is enabled for a previously established session. Initial connection still uses Host / Listen or Join."
    if frame.role == 1:
        return "Use Host / Listen for the initial connection, or Listen in new mission after a mission change."
    return "Wait for the host to listen, then use Join initially or Retry join after a previous connection attempt."


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pid", required=True, type=int, help="PID of the already running ordinary game")
    args = parser.parse_args()
    session = Session(args.pid)
    root = tk.Tk()
    root.title(f"ME3 Squad Co-op — {args.pid}")
    root.geometry("590x580")
    status, result = tk.StringVar(value="Waiting for game status"), tk.StringVar(value="")
    guidance = tk.StringVar(value="")
    ttk.Label(root, textvariable=status, wraplength=550).pack(padx=15, pady=12, anchor="w")
    ttk.Label(root, textvariable=guidance, wraplength=550).pack(padx=15, pady=5, anchor="w")
    buttons = ttk.Frame(root)
    buttons.pack(fill="x", padx=15)
    controls = []
    last_scope, shown, pending = 0, None, None

    def send(code):
        nonlocal pending
        try:
            selected = powers.curselection()
            if code == 4 and not selected:
                raise ValueError("Select a power first")
            pending = session.command(code, last_scope if code == 4 else 0, selected[0] if code == 4 else 0)
            result.set("Request queued; waiting for game Tick")
        except (OSError, ValueError, RuntimeError) as error:
            result.set(str(error))

    for column in range(3):
        buttons.columnconfigure(column, weight=1)
    for position, (text, code) in enumerate((("Host / Listen", 1), ("Join", 2), ("Retry join", 3),
                                           ("Listen in new mission", 7), ("Disable mod", 5), ("Enable mod", 6),
                                           ("Cancel pending connection", 8))):
        button = ttk.Button(buttons, text=text, command=lambda value=code: send(value))
        button.grid(row=position // 3, column=position % 3, padx=2, pady=2, sticky="ew")
        controls.append((button, code))
    ttk.Label(root, text="Cancel stops waiting requests and automatic listener/rejoin scheduling. An active connection stays open.",
              wraplength=550).pack(padx=15, pady=5, anchor="w")
    powers = tk.Listbox(root, exportselection=False)
    powers.pack(fill="both", expand=True, padx=15, pady=12)
    power_button = ttk.Button(root, text="Request selected power", command=lambda: send(4))
    power_button.pack(pady=3)
    controls.append((power_button, 4))
    ttk.Label(root, textvariable=result, wraplength=550).pack(padx=15, pady=6, anchor="w")
    ttk.Label(root, text="Experimental. Native order acceptance does not confirm a cast or hit.").pack(pady=6)

    def refresh():
        nonlocal last_scope, shown, pending
        try:
            frame = session.snapshot()
            guidance.set(recovery_hint(frame))
            role = "Host" if frame.role == 1 else "Guest"
            net = {0: "Standalone", 2: "Listen server", 3: "Client"}.get(frame.netmode, "Unavailable")
            enabled_bit, suspended_bit = (1, 4) if frame.role == 1 else (2, 8)
            recovery = "Manual"
            if frame.recovery_flags & enabled_bit:
                recovery = "Paused until game restart" if frame.recovery_flags & suspended_bit else "Automatic"
            status.set(f"{role} | {net} | {'Enabled' if frame.enabled else 'Disabled'}\n"
                       f"Connection: {STARTUP.get(frame.startup, 'Unknown')} | Companion ownership: {bool(frame.companion_owned)}\n"
                       f"Control: {CONTROL.get(frame.control_state, 'Unknown')}\n"
                       f"Power: {POWER.get(frame.power_status, 'Unknown')}\n"
                       f"Mission recovery: {recovery} | Readiness: {READINESS.get(frame.readiness_state, 'Unknown')}")
            names = [bytes(frame.power_names[i]).split(b'\0', 1)[0].decode('utf-8', errors='replace')
                     for i in range(frame.power_count)]
            slots = tuple(frame.power_slot_indices) if frame.hotkeys_enabled else (-1,) * 9
            catalog = (frame.power_scope, tuple(names), slots, frame.hotkeys_enabled)
            if catalog != shown:
                powers.delete(0, "end")
                for index, name in enumerate(names):
                    labels = []
                    for slot, value in enumerate(slots):
                        if value != index:
                            continue
                        if frame.hotkeys_enabled & 1:
                            labels.append(f"Ctrl+Alt+{slot + 1}")
                        if frame.hotkeys_enabled & 2 and slot < 8:
                            labels.append(f"Power slot {slot + 1}")
                    shortcut = f" [{', '.join(labels)}]" if labels else ""
                    powers.insert("end", f"{index + 1}. {name or '(unnamed)'}{shortcut}")
                shown, last_scope = catalog, frame.power_scope
            if pending is not None and frame.ack_id == pending:
                result.set({1: "Request accepted for processing", 2: "Request refused",
                            3: "Request expired before delivery; send a fresh request"}.get(
                                frame.ack_result, "Unknown request result"))
                pending = None
            for button, code in controls:
                allowed = code in (5, 6, 8) or (frame.enabled and (
                    (code in (1, 7) and frame.role == 1 and frame.netmode == 0) or
                    (code in (2, 3) and frame.role == 2 and frame.netmode == 0) or
                    (code == 4 and frame.role == 2 and frame.control_state == 1 and
                     frame.power_scope and frame.power_count)))
                button.configure(state="normal" if allowed else "disabled")
        except (OSError, RuntimeError) as error:
            status.set(str(error))
            guidance.set("Live status is unavailable. Controls will return only after a valid current game snapshot.")
            for button, _ in controls:
                button.configure(state="disabled")
        root.after(250, refresh)

    try:
        refresh()
        root.mainloop()
    finally:
        session.close()


if __name__ == "__main__":
    main()
