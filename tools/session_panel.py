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
            "power_scope", "power_count", "ack_id", "ack_result", "tick_ms")] + [
        ("power_names", (C.c_char * 80) * 64)]


class Memory(C.LittleEndianStructure):
    _pack_ = 4
    _fields_ = [("frame", Frame), ("command", C.c_int32), ("command_id", C.c_uint32),
                ("scope", C.c_uint32), ("index", C.c_uint32)]


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
        if C.sizeof(Frame) != 5192 or C.sizeof(Memory) != 5208:
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
                    0x4d334350, 1, 5208, self.pid, self.created):
                raise RuntimeError("Panel identity/version unavailable or different")
            if frame.role not in (1, 2) or frame.power_count > 64:
                raise RuntimeError("Invalid panel frame")
            if (self.kernel.GetTickCount() - frame.tick_ms) & 0xffffffff > 3000:
                raise RuntimeError("Game status is stale; controls unavailable")
            return frame
        raise RuntimeError("Game status is updating")

    def command(self, code, scope=0, index=0):
        frame = self.snapshot()
        if code not in range(1, 7):
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
           12: "Waiting to retry", 13: "Retry refused", 14: "Retry expired"}
POWER = {0: "Idle", 1: "Queued", 2: "Rejected", 3: "Native order accepted", 4: "Native order refused"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--pid", required=True, type=int, help="PID of the already running ordinary game")
    args = parser.parse_args()
    session = Session(args.pid)
    root = tk.Tk()
    root.title(f"ME3 Squad Co-op — {args.pid}")
    root.geometry("590x440")
    status, result = tk.StringVar(value="Waiting for game status"), tk.StringVar(value="")
    ttk.Label(root, textvariable=status, wraplength=550).pack(padx=15, pady=12, anchor="w")
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

    for text, code in (("Host / Listen", 1), ("Join", 2), ("Retry join", 3), ("Disable mod", 5), ("Enable mod", 6)):
        button = ttk.Button(buttons, text=text, command=lambda value=code: send(value))
        button.pack(side="left", padx=2)
        controls.append((button, code))
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
            role = "Host" if frame.role == 1 else "Guest"
            net = {0: "Standalone", 2: "Listen server", 3: "Client"}.get(frame.netmode, "Unavailable")
            status.set(f"{role} | {net} | {'Enabled' if frame.enabled else 'Disabled'}\n"
                       f"Connection: {STARTUP.get(frame.startup, 'Unknown')} | Companion ownership: {bool(frame.companion_owned)}\n"
                       f"Power: {POWER.get(frame.power_status, 'Unknown')}")
            names = [bytes(frame.power_names[i]).split(b'\0', 1)[0].decode('utf-8', errors='replace')
                     for i in range(frame.power_count)]
            catalog = (frame.power_scope, tuple(names))
            if catalog != shown:
                powers.delete(0, "end")
                for index, name in enumerate(names):
                    powers.insert("end", f"{index + 1}. {name or '(unnamed)'}")
                shown, last_scope = catalog, frame.power_scope
            if pending is not None and frame.ack_id == pending:
                result.set("Request accepted for processing" if frame.ack_result == 1 else "Request refused")
                pending = None
            for button, code in controls:
                allowed = code in (5, 6) or (frame.enabled and (
                    (code == 1 and frame.role == 1 and frame.netmode == 0) or
                    (code in (2, 3) and frame.role == 2 and frame.netmode == 0) or
                    (code == 4 and frame.role == 2 and frame.power_scope and frame.power_count)))
                button.configure(state="normal" if allowed else "disabled")
        except (OSError, RuntimeError) as error:
            status.set(str(error))
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
