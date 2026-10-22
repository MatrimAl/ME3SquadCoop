"""Authoritative enemy health and shield sampler for Mass Effect 3 Legendary Edition.

Bounded, read-only Windows process-memory sampler. Does not launch, inject into,
or modify the game process. Enumerate actual current-world actors, validates
reciprocal ownership, and samples true damage-module health and inventory shields.
"""
from __future__ import annotations

import abc
import ctypes
from ctypes import wintypes
import hashlib
import json
import math
from pathlib import Path
import struct
import sys
import time
from typing import Any, Callable, Dict, List, Optional, Set, Tuple

# Supported MassEffect3.exe SHA-256 in isolated test installation
SUPPORTED_EXECUTABLE_SHA256 = "b453871eb6eacff0c646490ffbc588f743399a829b1f9e31cd7775dc95da8001"

# Bounded sampling limits
MIN_BOUNDED_DURATION_S = 0.1
MAX_BOUNDED_DURATION_S = 120.0
MIN_BOUNDED_INTERVAL_S = 0.05
MAX_BOUNDED_INTERVAL_S = 5.0
MAX_LIVE_OBJECTS_CAP = 500000
MAX_ARRAY_ELEMENT_LIMIT = 100000
MAX_INVENTORY_CHAIN_LIMIT = 128
MAX_ANCESTRY_DEPTH = 40
MAX_OUTER_CHAIN_DEPTH = 16

# Process memory offsets (Mass Effect 3 LE Windows x64)
OFFSET_GNAMES = 0x17B33D0
OFFSET_GOBJECTS = 0x1887E40
OFFSET_GWORLD = 0x18B6638

# UObject offsets
OFFSET_UOBJECT_INTERNAL_INDEX = 0x08
OFFSET_UOBJECT_FLAGS = 0x0C
OFFSET_UOBJECT_OUTER = 0x40
OFFSET_UOBJECT_FNAME = 0x48
OFFSET_UOBJECT_CLASS = 0x50

# UClass offsets
OFFSET_UCLASS_SUPER_FIELD = 0x60
OFFSET_UCLASS_CLASS_DEFAULT_OBJECT = 0x13C

# UWorld / ULevel offsets
OFFSET_UWORLD_LEVELS = 0x78
OFFSET_UWORLD_PERSISTENT_LEVEL = 0x88
OFFSET_ULEVEL_ACTORS = 0x60

# WorldInfo offsets
OFFSET_WORLDINFO_STREAMING_LEVELS = 0x3E4
OFFSET_WORLDINFO_NET_MODE = 0x7C8

# LevelStreaming offsets
OFFSET_LEVELSTREAMING_LOADED_LEVEL = 0xA0
OFFSET_LEVELSTREAMING_FLAGS = 0xCC

# Actor offsets
OFFSET_ACTOR_MODULES = 0x70
OFFSET_ACTOR_LOCATION = 0x108
OFFSET_ACTOR_ROTATION = 0x114
OFFSET_ACTOR_OWNER = 0x180
OFFSET_ACTOR_INSTIGATOR = 0x190
OFFSET_ACTOR_WORLD_INFO = 0x198
OFFSET_ACTOR_FLAGS = 0x25C
OFFSET_ACTOR_ROLE = 0x26A
OFFSET_ACTOR_PENDING_FLAGS = 0x260

# Pawn offsets
OFFSET_PAWN_CONTROLLER = 0x374
OFFSET_PAWN_PRI = 0x3CC
OFFSET_PAWN_INVENTORY = 0x414
OFFSET_PAWN_HEALTH_LEGACY = 0x514

# BioPawn offsets
OFFSET_BIOPAWN_SQUAD = 0xBA8
OFFSET_BIOPAWN_COMBAT_FLAGS = 0xE18
OFFSET_BIOPAWN_DEAD_FLAGS = 0xE20

# Controller offsets
OFFSET_CONTROLLER_PRI = 0x3C4

# PlayerReplicationInfo offsets
OFFSET_PRI_TEAM = 0x2D8

# TeamInfo offsets
OFFSET_TEAMINFO_TEAM_INDEX = 0x288

# SFXModule offsets
OFFSET_MODULE_OWNER = 0x60

# SFXModule_DamageNativeBase offsets
OFFSET_DAMAGE_MAX_HEALTH = 0x90
OFFSET_DAMAGE_CURRENT_HEALTH = 0x98
OFFSET_DAMAGE_REPLICATED_HEALTH = 0xAC
OFFSET_DAMAGE_HEALTH_TYPE = 0x214

# SFXInventoryManager offsets
OFFSET_INVENTORY_CHAIN = 0x280

# Inventory offsets
OFFSET_INVENTORY_OWNER = 0x180
OFFSET_INVENTORY_INSTIGATOR = 0x190
OFFSET_INVENTORY_NEXT = 0x2A8
OFFSET_INVENTORY_MANAGER = 0x2B0

# SFXShield_Base offsets
OFFSET_SHIELD_MAX_VALUE = 0x304
OFFSET_SHIELD_CURRENT_VALUE = 0x3C4
OFFSET_SHIELD_RESISTANCE = 0x3E4


def compute_file_sha256(path: Path) -> str:
    """Compute the SHA-256 hash of a file in 4MB chunks."""
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(4 * 1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def verify_isolated_executable(
    executable_path: Path,
    expected_sha: str = SUPPORTED_EXECUTABLE_SHA256,
    sandbox_dir: Optional[Path] = None,
) -> Tuple[bool, str]:
    """Verify that the target executable is in the isolated sandbox and matches SHA-256."""
    try:
        resolved = executable_path.resolve(strict=True)
    except (FileNotFoundError, OSError) as exc:
        return False, f"executable file not found: {exc}"

    if not resolved.is_file():
        return False, f"target is not a regular file: {resolved}"
    if resolved.stat().st_nlink != 1:
        return False, "executable has multiple hardlinks; independent sandbox identity is unproven"

    if sandbox_dir is None:
        return False, "an explicit isolated sandbox directory is required"
    if sandbox_dir is not None:
        try:
            resolved_sandbox = sandbox_dir.resolve(strict=True)
            if not resolved.is_relative_to(resolved_sandbox):
                return False, f"executable {resolved} is outside isolated sandbox {resolved_sandbox}"
            # Reject junction/symlink ambiguity rather than blessing a resolved
            # alias into the original game. Inspect lexical ancestors as well.
            for target in (executable_path.absolute(), sandbox_dir.absolute()):
                for part in (target, *target.parents):
                    metadata = part.lstat()
                    if part.is_symlink() or getattr(metadata, "st_file_attributes", 0) & 0x400:
                        return False, f"reparse/symlink path is not an isolated sandbox: {part}"
        except Exception as exc:
            return False, f"sandbox verification failed: {exc}"

    # Verify SHA-256 hash
    try:
        actual_sha = compute_file_sha256(resolved)
    except OSError as exc:
        return False, f"executable hashing failed: {exc}"
    if actual_sha.lower() != expected_sha.lower():
        return False, f"executable SHA-256 mismatch (actual={actual_sha}, expected={expected_sha})"

    return True, "verified isolated executable"


class MemoryReader(abc.ABC):
    """Abstract interface for process memory reads."""

    @abc.abstractmethod
    def read(self, address: int, size: int) -> bytes:
        """Read size bytes from address. Return b'' on failure."""
        raise NotImplementedError

    def read_ptr(self, address: int) -> int:
        """Read an unsigned 64-bit pointer."""
        raw = self.read(address, 8)
        if len(raw) != 8:
            return 0
        return struct.unpack("<Q", raw)[0]

    def read_int32(self, address: int) -> Optional[int]:
        """Read a signed 32-bit integer."""
        raw = self.read(address, 4)
        if len(raw) != 4:
            return None
        return struct.unpack("<i", raw)[0]

    def read_uint32(self, address: int) -> Optional[int]:
        """Read an unsigned 32-bit integer."""
        raw = self.read(address, 4)
        if len(raw) != 4:
            return None
        return struct.unpack("<I", raw)[0]

    def read_float(self, address: int) -> Optional[float]:
        """Read a 32-bit IEEE float, returning None if unreadable or non-finite."""
        raw = self.read(address, 4)
        if len(raw) != 4:
            return None
        val = struct.unpack("<f", raw)[0]
        if not math.isfinite(val):
            return None
        return val

    def read_byte(self, address: int) -> Optional[int]:
        """Read an unsigned 8-bit byte."""
        raw = self.read(address, 1)
        if len(raw) != 1:
            return None
        return raw[0]

    def read_tarray(self, address: int, limit: int = MAX_ARRAY_ELEMENT_LIMIT) -> Tuple[Optional[Tuple[int, int, int]], List[int]]:
        """Read a UE3 TArray<QWORD> header and elements.

        Returns (header_tuple(data_ptr, count, max), elements_list).
        On error, returns (None, []).
        """
        raw = self.read(address, 16)
        if len(raw) != 16:
            return None, []
        data_ptr, count, max_elems = struct.unpack("<Qii", raw)
        if count < 0 or count > limit or max_elems < count:
            return None, []
        if count == 0:
            return (data_ptr, count, max_elems), []
        if not data_ptr:
            return None, []
        elem_bytes = self.read(data_ptr, count * 8)
        if len(elem_bytes) != count * 8:
            return None, []
        elements = [ptr for (ptr,) in struct.iter_unpack("<Q", elem_bytes)]
        return (data_ptr, count, max_elems), elements


class WindowsProcessMemory(MemoryReader):
    """Windows ReadProcessMemory wrapper with STRICT read-only permissions (0x410)."""

    def __init__(self, pid: int, expected_sha: str = SUPPORTED_EXECUTABLE_SHA256, sandbox_dir: Optional[Path] = None):
        self.pid = pid
        self.expected_sha = expected_sha
        self.sandbox_dir = sandbox_dir
        self.handle = None
        self.base_address = 0
        self.module_path = ""
        self._kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
        self._setup_winapi()

    def _setup_winapi(self):
        w = wintypes
        c = ctypes
        self._kernel32.OpenProcess.argtypes = [w.DWORD, w.BOOL, w.DWORD]
        self._kernel32.OpenProcess.restype = w.HANDLE
        self._kernel32.CloseHandle.argtypes = [w.HANDLE]
        self._kernel32.ReadProcessMemory.argtypes = [w.HANDLE, c.c_void_p, c.c_void_p, c.c_size_t, c.POINTER(c.c_size_t)]
        self._kernel32.ReadProcessMemory.restype = w.BOOL
        self._kernel32.CreateToolhelp32Snapshot.argtypes = [w.DWORD, w.DWORD]
        self._kernel32.CreateToolhelp32Snapshot.restype = w.HANDLE

    def open(self):
        """Open the process with PROCESS_VM_READ | PROCESS_QUERY_INFORMATION (0x410)."""
        # PROCESS_VM_READ (0x0010) | PROCESS_QUERY_INFORMATION (0x0400) = 0x0410
        PROCESS_READ_ONLY = 0x0410
        self.handle = self._kernel32.OpenProcess(PROCESS_READ_ONLY, False, self.pid)
        if not self.handle:
            error_code = ctypes.get_last_error()
            raise PermissionError(f"OpenProcess failed for PID {self.pid} with error {error_code}")

        # Enumerate modules to locate MassEffect3.exe and verify path/SHA
        snap = None
        for _ in range(5):
            snap = self._kernel32.CreateToolhelp32Snapshot(0x18, self.pid)  # TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32
            if snap and snap != ctypes.c_void_p(-1).value:
                break
            err = ctypes.get_last_error()
            if err == 24:  # ERROR_BAD_LENGTH
                time.sleep(0.05)
                continue
            break

        if not snap or snap == ctypes.c_void_p(-1).value:
            self.close()
            error_code = ctypes.get_last_error()
            raise RuntimeError(f"CreateToolhelp32Snapshot failed with error {error_code}")

        class MODULEENTRY32W(ctypes.Structure):
            _fields_ = [
                ("dwSize", wintypes.DWORD),
                ("th32ModuleID", wintypes.DWORD),
                ("th32ProcessID", wintypes.DWORD),
                ("GlblcntUsage", wintypes.DWORD),
                ("ProccntUsage", wintypes.DWORD),
                ("modBaseAddr", ctypes.c_void_p),
                ("modBaseSize", wintypes.DWORD),
                ("hModule", wintypes.HMODULE),
                ("szModule", wintypes.WCHAR * 256),
                ("szExePath", wintypes.WCHAR * 260),
            ]

        self._kernel32.Module32FirstW.argtypes = [wintypes.HANDLE, ctypes.POINTER(MODULEENTRY32W)]
        self._kernel32.Module32NextW.argtypes = [wintypes.HANDLE, ctypes.POINTER(MODULEENTRY32W)]

        base = 0
        exe_path = ""
        try:
            entry = MODULEENTRY32W()
            entry.dwSize = ctypes.sizeof(entry)
            ok = self._kernel32.Module32FirstW(snap, ctypes.byref(entry))
            while ok:
                if entry.szModule.casefold() == "masseffect3.exe":
                    base = entry.modBaseAddr
                    exe_path = entry.szExePath
                    break
                ok = self._kernel32.Module32NextW(snap, ctypes.byref(entry))
        finally:
            self._kernel32.CloseHandle(snap)

        if not base or not exe_path:
            self.close()
            raise RuntimeError(f"MassEffect3.exe module not found in process PID {self.pid}")

        # Verify executable path and SHA-256
        valid, reason = verify_isolated_executable(Path(exe_path), self.expected_sha, self.sandbox_dir)
        if not valid:
            self.close()
            raise RuntimeError(f"Executable verification failed: {reason}")
        expected_path = self.sandbox_dir / "Binaries" / "Win64" / "MassEffect3.exe"
        if Path(exe_path).resolve() != expected_path.resolve():
            self.close()
            raise RuntimeError("module is not the exact sandbox Binaries/Win64/MassEffect3.exe")

        self.base_address = base if isinstance(base, int) else int(base)
        self.module_path = exe_path

    def close(self):
        if self.handle:
            self._kernel32.CloseHandle(self.handle)
            self.handle = None

    def __enter__(self):
        self.open()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    def read(self, address: int, size: int) -> bytes:
        if not self.handle or not address or address < 0x10000 or size <= 0 or size > 32 * 1024 * 1024:
            return b""
        buf = ctypes.create_string_buffer(size)
        bytes_read = ctypes.c_size_t(0)
        ok = self._kernel32.ReadProcessMemory(
            self.handle,
            ctypes.c_void_p(address),
            buf,
            ctypes.c_size_t(size),
            ctypes.byref(bytes_read),
        )
        if not ok or bytes_read.value != size:
            return b""
        return buf.raw


class MockMemoryReader(MemoryReader):
    """In-memory mock memory reader for hermetic unit testing without live game process."""

    def __init__(self):
        self._memory: Dict[int, bytes] = {}
        self._read_failure_addresses: Set[int] = set()

    def set_fail_address(self, address: int):
        """Force reads touching this address to fail."""
        self._read_failure_addresses.add(address)

    def write_bytes(self, address: int, data: bytes):
        for i, byte_val in enumerate(data):
            self._memory[address + i] = bytes([byte_val])

    def write_ptr(self, address: int, value: int):
        self.write_bytes(address, struct.pack("<Q", value))

    def write_int32(self, address: int, value: int):
        self.write_bytes(address, struct.pack("<i", value))

    def write_uint32(self, address: int, value: int):
        self.write_bytes(address, struct.pack("<I", value))

    def write_float(self, address: int, value: float):
        self.write_bytes(address, struct.pack("<f", value))

    def write_byte(self, address: int, value: int):
        self.write_bytes(address, bytes([value & 0xFF]))

    def write_tarray(self, address: int, elements: List[int], data_ptr: int):
        self.write_ptr(address, data_ptr)
        self.write_int32(address + 8, len(elements))
        self.write_int32(address + 12, max(len(elements), 16))
        for i, elem in enumerate(elements):
            self.write_ptr(data_ptr + i * 8, elem)

    def read(self, address: int, size: int) -> bytes:
        if not address or size <= 0:
            return b""
        for a in range(address, address + size):
            if a in self._read_failure_addresses:
                return b""
            if a not in self._memory:
                return b""
        return b"".join(self._memory[a] for a in range(address, address + size))


class SamplingBudgetExceeded(RuntimeError):
    pass


class BudgetedReader(MemoryReader):
    """Bound total remote reads, bytes and wall time of one snapshot."""

    def __init__(self, source, deadline):
        self.source, self.deadline = source, deadline
        self.calls = self.bytes = 0

    def read(self, address, size):
        self.calls += 1
        self.bytes += max(0, size)
        if self.calls > 120000 or self.bytes > 32 * 1024 * 1024 or time.monotonic() >= self.deadline:
            raise SamplingBudgetExceeded("snapshot exceeded read/byte/time budget")
        return self.source.read(address, size)


class AuthoritativeEnemySampler:
    """Bounded, read-only sampler for authoritative enemy health and shields."""

    def __init__(self, reader: MemoryReader, module_base: int):
        self.reader = reader
        self.base = module_base
        self._name_cache: Dict[int, str] = {}
        self._class_ancestry_cache: Dict[int, List[str]] = {}
        self._gobjects_info_cache: Optional[Tuple[int, int]] = None
        self._level_headers: Dict[int, bytes] = {}
        self._stable_arrays: Dict[int, Tuple[Any, Tuple[int, ...]]] = {}
        self._stable_fields: Dict[int, bytes] = {}

    def tracked_array(self, address: int, limit: int):
        header, items = self.reader.read_tarray(address, limit)
        self._stable_arrays[address] = (header, tuple(items))
        return header, items

    def object_identity(self, ptr: int) -> Optional[Dict[str, Any]]:
        """Bind identity to the live index, name, class and outer, not address alone."""
        raw = self.reader.read(ptr, 0x60)
        if len(raw) != 0x60 or not self.is_live_object(ptr):
            return None
        return {
            "pointer": hex(ptr),
            "internal_index": struct.unpack_from("<i", raw, 8)[0],
            "fname_index": struct.unpack_from("<I", raw, 0x48)[0],
            "fname_number": struct.unpack_from("<I", raw, 0x4C)[0],
            "class_pointer": hex(struct.unpack_from("<Q", raw, 0x50)[0]),
            "outer_pointer": hex(struct.unpack_from("<Q", raw, 0x40)[0]),
        }

    def get_name(self, index: int) -> str:
        """Resolve FName by index from the global names table."""
        if index in self._name_cache:
            return self._name_cache[index]
        chunk_offset = (index >> 29) * 8
        pool_ptr = self.reader.read_ptr(self.base + OFFSET_GNAMES + chunk_offset)
        if not pool_ptr:
            return ""
        entry_offset = (index & 0x1FFFFFFF) + 12
        raw = self.reader.read(pool_ptr + entry_offset, 128)
        if not raw:
            return ""
        name_str = raw.split(b"\0")[0].decode("ascii", errors="replace")
        self._name_cache[index] = name_str
        return name_str

    def get_object_name(self, object_ptr: int) -> str:
        """Get the FName string for an object (+ optional index number)."""
        raw = self.reader.read(object_ptr, 0x60)
        if len(raw) < 0x50:
            return ""
        fname_index, fname_num = struct.unpack_from("<II", raw, OFFSET_UOBJECT_FNAME)
        base_name = self.get_name(fname_index)
        if fname_num > 0:
            return f"{base_name}_{fname_num - 1}"
        return base_name

    def get_full_path(self, object_ptr: int) -> str:
        """Get full dotted path of an object by traversing Outer."""
        parts: List[str] = []
        seen: Set[int] = set()
        curr = object_ptr
        while curr and curr not in seen and len(parts) < MAX_OUTER_CHAIN_DEPTH:
            seen.add(curr)
            name = self.get_object_name(curr)
            if not name:
                break
            parts.append(name)
            curr = self.reader.read_ptr(curr + OFFSET_UOBJECT_OUTER)
        return ".".join(reversed(parts))

    def is_live_object(self, object_ptr: int) -> bool:
        """Verify that an address is a valid, live UObject in GObjects."""
        if not object_ptr or object_ptr < 0x10000:
            return False
        raw_obj = self.reader.read(object_ptr, 0x60)
        if len(raw_obj) < 0x10:
            return False
        internal_index = struct.unpack_from("<i", raw_obj, OFFSET_UOBJECT_INTERNAL_INDEX)[0]
        if self._gobjects_info_cache is not None:
            data_ptr, count = self._gobjects_info_cache
        else:
            gobjects_header = self.reader.read(self.base + OFFSET_GOBJECTS, 16)
            if len(gobjects_header) != 16:
                return False
            data_ptr, count, max_objs = struct.unpack("<Qii", gobjects_header)
            if count <= 0 or count > MAX_LIVE_OBJECTS_CAP or max_objs < count:
                return False
        if internal_index < 0 or internal_index >= count:
            return False
        live_ptr = self.reader.read_ptr(data_ptr + internal_index * 8)
        return live_ptr == object_ptr

    def is_a(self, object_ptr: int, target_class_name: str) -> bool:
        """Check if an object inherits from target_class_name via SuperField."""
        if not self.is_live_object(object_ptr):
            return False
        class_ptr = self.reader.read_ptr(object_ptr + OFFSET_UOBJECT_CLASS)
        if not class_ptr:
            return False
        target_lower = target_class_name.lower()
        if class_ptr in self._class_ancestry_cache:
            return any(target_lower == c.lower() for c in self._class_ancestry_cache[class_ptr])

        seen: Set[int] = set()
        curr = class_ptr
        ancestry: List[str] = []
        clean_termination = False
        while curr and curr not in seen and len(seen) < MAX_ANCESTRY_DEPTH:
            if not self.is_live_object(curr):
                break
            seen.add(curr)
            cname = self.get_object_name(curr)
            ancestry.append(cname)
            curr = self.reader.read_ptr(curr + OFFSET_UCLASS_SUPER_FIELD)
            if curr == 0:
                clean_termination = True
                break

        if clean_termination or any(target_lower == c.lower() for c in ancestry):
            self._class_ancestry_cache[class_ptr] = ancestry

        return any(target_lower == c.lower() for c in ancestry)

    def is_cdo(self, object_ptr: int) -> bool:
        """Check if an object is a Class Default Object (CDO)."""
        if not object_ptr:
            return False
        class_ptr = self.reader.read_ptr(object_ptr + OFFSET_UOBJECT_CLASS)
        if class_ptr:
            cdo_ptr = self.reader.read_ptr(class_ptr + OFFSET_UCLASS_CLASS_DEFAULT_OBJECT)
            if cdo_ptr == object_ptr:
                return True
        name = self.get_object_name(object_ptr)
        if name.startswith("Default__"):
            return True
        path = self.get_full_path(object_ptr)
        return "Default__" in path

    def enumerate_levels(self, world_ptr: int) -> Tuple[List[Tuple[int, bool]], int, List[str]]:
        """Enumerate PersistentLevel and all loaded streaming levels from WorldInfo / World.Levels.

        Returns ([(level_ptr, is_visible)], world_info_ptr, read_failures).
        """
        failures: List[str] = []
        if not self.is_a(world_ptr, "World"):
            return [], 0, ["GWorld pointer does not inherit from World"]

        persistent_level = self.reader.read_ptr(world_ptr + OFFSET_UWORLD_PERSISTENT_LEVEL)
        if not self.is_a(persistent_level, "Level"):
            return [], 0, ["PersistentLevel does not inherit from Level"]

        # Locate WorldInfo in PersistentLevel: check first 16 actors
        _, actors = self.tracked_array(persistent_level + OFFSET_ULEVEL_ACTORS, MAX_ARRAY_ELEMENT_LIMIT)
        if not actors:
            return [], 0, ["PersistentLevel actors array is empty or unreadable"]

        world_info = 0
        for act in actors[:16]:
            if act and self.is_a(act, "WorldInfo"):
                world_info = act
                break

        if not world_info:
            return [], 0, ["First actor in PersistentLevel is not WorldInfo"]

        levels: List[Tuple[int, bool]] = [(persistent_level, True)]
        seen_levels: Set[int] = {persistent_level}
        self._level_headers = {}
        self._level_headers[persistent_level] = self.reader.read(persistent_level + OFFSET_ULEVEL_ACTORS, 16)

        # 1. Enumerate streaming levels from WorldInfo.StreamingLevels (+0x3E4)
        stream_header, stream_ptrs = self.tracked_array(world_info + OFFSET_WORLDINFO_STREAMING_LEVELS, 512)
        if stream_header is None:
            failures.append("WorldInfo streaming levels array unreadable")
        for stream in stream_ptrs:
            if not self.is_a(stream, "LevelStreaming"):
                continue
            loaded_raw = self.reader.read(stream + OFFSET_LEVELSTREAMING_LOADED_LEVEL, 8)
            flags_raw = self.reader.read(stream + OFFSET_LEVELSTREAMING_FLAGS, 4)
            self._stable_fields[stream + OFFSET_LEVELSTREAMING_LOADED_LEVEL] = loaded_raw
            self._stable_fields[stream + OFFSET_LEVELSTREAMING_FLAGS] = flags_raw
            if len(loaded_raw) != 8 or len(flags_raw) != 4:
                failures.append(f"stream 0x{stream:x} loaded/visible state unreadable")
                continue
            loaded_level = struct.unpack("<Q", loaded_raw)[0]
            flags = struct.unpack("<I", flags_raw)[0]
            is_visible = bool(flags & 1) if flags is not None else False
            if loaded_level and loaded_level not in seen_levels and self.is_a(loaded_level, "Level"):
                seen_levels.add(loaded_level)
                levels.append((loaded_level, is_visible))
                self._level_headers[loaded_level] = self.reader.read(loaded_level + OFFSET_ULEVEL_ACTORS, 16)

        # 2. Also check World.Levels (+0x78) for loaded levels not in streaming list
        world_levels_header, world_level_ptrs = self.tracked_array(world_ptr + OFFSET_UWORLD_LEVELS, 256)
        if world_levels_header is None:
            failures.append("World levels array unreadable")
        for wlvl in world_level_ptrs:
            if wlvl and wlvl not in seen_levels and self.is_a(wlvl, "Level"):
                seen_levels.add(wlvl)
                levels.append((wlvl, True))
                self._level_headers[wlvl] = self.reader.read(wlvl + OFFSET_ULEVEL_ACTORS, 16)

        return levels, world_info, failures

    def enumerate_actors(self, world_ptr: int) -> Tuple[List[Dict[str, Any]], List[str]]:
        """Enumerate actual actors in loaded levels, rejecting CDOs, wrong-world, and deleted actors."""
        failures: List[str] = []
        levels, world_info, level_failures = self.enumerate_levels(world_ptr)
        failures.extend(level_failures)
        if not world_info:
            return [], failures

        valid_actors: List[Dict[str, Any]] = []
        seen_actors: Set[int] = set()

        for level_ptr, level_visible in levels:
            header, actor_ptrs = self.tracked_array(level_ptr + OFFSET_ULEVEL_ACTORS, MAX_ARRAY_ELEMENT_LIMIT)
            if header is None:
                failures.append(f"level 0x{level_ptr:x} actors array is unreadable")
                continue

            for actor in actor_ptrs:
                if not actor or actor in seen_actors:
                    continue
                seen_actors.add(actor)

                # 1. Must be live in GObjects
                if not self.is_live_object(actor):
                    continue

                # 2. Must inherit from Actor
                if not self.is_a(actor, "Actor"):
                    continue

                # 3. Reject Class Default Objects
                if self.is_cdo(actor):
                    continue

                # 4. Reject actors from another world (WorldInfo mismatch)
                actor_world_info = self.reader.read_ptr(actor + OFFSET_ACTOR_WORLD_INFO)
                if actor_world_info != world_info:
                    continue

                # 5. Reject deleted actors (bDeleteMe is bit 3 of actor_flags at 0x25C)
                actor_flags = self.reader.read_uint32(actor + OFFSET_ACTOR_FLAGS)
                if actor_flags is None:
                    failures.append(f"actor 0x{actor:x} flags unreadable")
                    continue
                if bool(actor_flags & 8):  # bDeleteMe
                    continue

                pending_flags = self.reader.read_uint32(actor + OFFSET_ACTOR_PENDING_FLAGS)
                if pending_flags is None:
                    failures.append(f"actor 0x{actor:x} pending-delete flags unreadable")
                    continue
                if pending_flags & 0x800000:
                    continue

                b_hidden = bool(actor_flags & 2)

                valid_actors.append({
                    "pointer": actor,
                    "level": level_ptr,
                    "level_visible": level_visible,
                    "hidden": b_hidden,
                    "flags": actor_flags,
                })

        return valid_actors, failures

    def classify_pawn(self, pawn_ptr: int) -> Tuple[bool, str, Dict[str, Any]]:
        """Identify genuine hostile pawns using verified live team/actor metadata.

        Returns (is_genuine_hostile, classification_reason, metadata_dict).
        Rejects:
        - Player pawn (SFXPawn_PlayerSoldier)
        - Player squad members (Ashley, Kaidan, Garrus, etc. on BioPlayerSquad)
        - Pre-existing Gatehouse BioPawn dummy
        - Friendly team pawns (TeamIndex == 0)
        - Neutral/ambient pawns with non-hostile AI controllers
        """
        if not self.is_a(pawn_ptr, "Pawn"):
            return False, "not_a_pawn", {}

        class_name = self.get_object_name(self.reader.read_ptr(pawn_ptr + OFFSET_UOBJECT_CLASS))
        full_path = self.get_full_path(pawn_ptr)

        # 1. Reject Player pawn
        if class_name == "SFXPawn_PlayerSoldier" or "PlayerSoldier" in class_name or self.is_a(pawn_ptr, "SFXPawn_PlayerSoldier"):
            return False, "player_pawn", {"class": class_name, "path": full_path}

        # 2. Check squad membership
        squad_ptr = self.reader.read_ptr(pawn_ptr + OFFSET_BIOPAWN_SQUAD) if self.is_a(pawn_ptr, "BioPawn") else 0
        squad_class = self.get_object_name(self.reader.read_ptr(squad_ptr + OFFSET_UOBJECT_CLASS)) if squad_ptr else ""
        squad_path = self.get_full_path(squad_ptr) if squad_ptr else ""

        if squad_ptr and (self.is_a(squad_ptr, "BioPlayerSquad") or "BioPlayerSquad" in squad_path):
            return False, "player_squad_member", {
                "class": class_name, "path": full_path, "squad": squad_path,
            }

        # 3. Check controller
        controller_ptr = self.reader.read_ptr(pawn_ptr + OFFSET_PAWN_CONTROLLER)
        controller_class = self.get_object_name(self.reader.read_ptr(controller_ptr + OFFSET_UOBJECT_CLASS)) if controller_ptr else ""
        controller_path = self.get_full_path(controller_ptr) if controller_ptr else ""

        if controller_ptr and (self.is_a(controller_ptr, "SFXPlayerController") or "SFXPlayerController" in controller_path):
            return False, "player_controller_pawn", {
                "class": class_name, "path": full_path, "controller": controller_path,
            }
        if controller_ptr and (self.is_a(controller_ptr, "SFXAI_Henchman") or "SFXAI_Henchman" in controller_path or "Henchman" in controller_class):
            return False, "player_henchman_pawn", {
                "class": class_name, "path": full_path, "controller": controller_path,
            }

        # Reject known player companion archetypes even if detached from BioPlayerSquad
        companion_archetypes = ("Ashley", "Kaidan", "Garrus", "Liara", "Tali", "James", "EDI", "Javik", "Marine")
        if any(comp.lower() in class_name.lower() for comp in companion_archetypes):
            if not (squad_ptr and "Combat" in squad_class):
                return False, "player_squad_member", {
                    "class": class_name, "path": full_path, "squad": squad_path,
                }

        # 4. Check team number via Controller PRI or Pawn PRI
        pri_ptr = 0
        if controller_ptr:
            pri_ptr = self.reader.read_ptr(controller_ptr + OFFSET_CONTROLLER_PRI)
        if not pri_ptr:
            pri_ptr = self.reader.read_ptr(pawn_ptr + OFFSET_PAWN_PRI)

        team_info_ptr = self.reader.read_ptr(pri_ptr + OFFSET_PRI_TEAM) if pri_ptr else 0
        team_index = None
        if team_info_ptr and self.is_a(team_info_ptr, "TeamInfo"):
            team_index = self.reader.read_int32(team_info_ptr + OFFSET_TEAMINFO_TEAM_INDEX)

        # TeamIndex 0 is the player/alliance team
        if team_index == 0:
            return False, "friendly_team", {
                "class": class_name, "path": full_path, "team_index": team_index,
            }

        # 5. Check Gatehouse BioPawn dummy & unassigned dummy pawns
        # Gatehouse dummy is: path contains BioD_ProMar_120Gatehouse, class is exactly BioPawn,
        # squad is None, team is None, controller is plain AIController or None.
        obj_name = self.get_object_name(pawn_ptr)
        is_gatehouse_dummy = False
        if "120Gatehouse" in full_path and (class_name == "BioPawn" or "BioPawn_0" in obj_name):
            if squad_ptr == 0 and (team_index is None or team_index == 0):
                is_gatehouse_dummy = True
        elif class_name == "BioPawn" and squad_ptr == 0 and team_index is None:
            # Base BioPawn without combat squad, without hostile team, and plain AIController/BioAiController
            if controller_class in ("AIController", "BioAiController", ""):
                is_gatehouse_dummy = True

        if is_gatehouse_dummy:
            return False, "gatehouse_dummy_biopawn", {
                "class": class_name, "path": full_path, "squad": squad_path, "team_index": team_index,
            }

        # 6. Verify positive hostile metadata
        # A genuine hostile must have at least one positive hostile indicator:
        # a) Hostile team index (team_index > 0)
        # b) Assigned to combat squad (e.g. BioSquadCombat)
        # c) Hostile enemy class archetype (Trooper, Centurion, Guardian, Phantom, Nemesis, Cannibal, etc.)
        # d) Hostile enemy AI controller (SFXAI_Trooper, SFXAI_Guardian, SFXAI_Enemy, SFXAI_Combat, etc.)
        # NOTE: Bare BioAiController is the base class for ALL BioWare AI controllers and is NOT hostile on its own.
        is_hostile_team = team_index is not None and team_index > 0
        is_combat_squad = bool(squad_ptr and self.is_a(squad_ptr, "BioSquadCombat"))
        hostile_class_archetypes = (
            "Trooper", "Centurion", "Nemesis", "Engineer",
            "Atlas", "Cannibal", "Marauder", "Husk",
            "Brute", "Banshee", "Ravager", "Geth",
            "Guardian", "Phantom", "Turret", "Cerberus",
            "Reaper", "Collector", "SFXPawn_Combat", "SFXPawn_Enemy",
        )
        is_hostile_class = any(self.is_a(pawn_ptr, "SFXPawn_" + arch) for arch in hostile_class_archetypes if not arch.startswith("SFXPawn_"))

        enemy_controllers = (
            "SFXAI_Trooper", "SFXAI_Enemy", "SFXAI_Combat", "SFXAI_Guardian",
            "SFXAI_Centurion", "SFXAI_Nemesis", "SFXAI_Turret", "BioAiCombat",
        )
        is_hostile_controller = bool(controller_ptr and any(self.is_a(controller_ptr, ec) for ec in enemy_controllers))
        if not is_hostile_controller and is_combat_squad and "BioAiController" in controller_class:
            is_hostile_controller = True

        if is_hostile_team or is_combat_squad or is_hostile_class or is_hostile_controller:
            return True, "hostile_candidate", {
                "class": class_name,
                "path": full_path,
                "squad": squad_path,
                "squad_class": squad_class,
                "controller": controller_path,
                "controller_class": controller_class,
                "team_index": team_index,
                "hostility_verified": False,
                "hostile_indicators": {
                    "hostile_team": is_hostile_team,
                    "combat_squad": is_combat_squad,
                    "hostile_class": is_hostile_class,
                    "hostile_controller": is_hostile_controller,
                },
            }

        return False, "unverified_non_hostile_pawn", {
            "class": class_name, "path": full_path, "team_index": team_index,
        }

    def sample_damage_modules(self, actor_ptr: int) -> Tuple[List[Dict[str, Any]], List[str]]:
        """Sample authoritative health from damage modules with reciprocal ownership verification.

        Legacy Pawn.Health at +0x514 is NOT authoritative. Authoritative health is CurrentHealth
        at +0x98 in SFXModule_DamageNativeBase.
        """
        modules: List[Dict[str, Any]] = []
        failures: List[str] = []

        header, mod_ptrs = self.reader.read_tarray(actor_ptr + OFFSET_ACTOR_MODULES, limit=128)
        if header is None:
            failures.append(f"actor 0x{actor_ptr:x} Modules array at +0x70 is unreadable")
            return [], failures

        for mod in mod_ptrs:
            if not mod:
                continue
            if not self.is_live_object(mod):
                failures.append(f"module 0x{mod:x} is not a live object")
                continue
            if not self.is_a(mod, "SFXModule_DamageNativeBase"):
                continue
            identity = self.object_identity(mod)

            # VERIFY RECIPROCAL OWNERSHIP: Module.ModuleOwner (+0x60) must point back to actor_ptr
            owner_ptr = self.reader.read_ptr(mod + OFFSET_MODULE_OWNER)
            if owner_ptr != actor_ptr:
                failures.append(f"damage module 0x{mod:x} ownership mismatch (owner=0x{owner_ptr:x}, expected=0x{actor_ptr:x})")
                continue

            current_health = self.reader.read_float(mod + OFFSET_DAMAGE_CURRENT_HEALTH)
            max_health = self.reader.read_float(mod + OFFSET_DAMAGE_MAX_HEALTH)
            replicated_health = self.reader.read_byte(mod + OFFSET_DAMAGE_REPLICATED_HEALTH)

            health_type = None
            if self.is_a(mod, "SFXModule_Damage"):
                health_type = self.reader.read_byte(mod + OFFSET_DAMAGE_HEALTH_TYPE)

            if current_health is None:
                failures.append(f"damage module 0x{mod:x} CurrentHealth at +0x98 is unreadable or non-finite")
                continue

            mod_class = self.get_object_name(self.reader.read_ptr(mod + OFFSET_UOBJECT_CLASS))
            mod_path = self.get_full_path(mod)

            modules.append({
                "pointer": hex(mod),
                "identity": identity,
                "class": mod_class,
                "path": mod_path,
                "module_owner": hex(owner_ptr),
                "owner_reciprocal": True,
                "current_health": current_health,
                "max_health": max_health,
                "replicated_health": replicated_health,
                "health_type": health_type,
            })

            if identity is None or self.object_identity(mod) != identity or self.reader.read_ptr(mod + OFFSET_MODULE_OWNER) != actor_ptr:
                failures.append(f"damage module 0x{mod:x} changed identity/owner during read")

        final_header, final_modules = self.reader.read_tarray(actor_ptr + OFFSET_ACTOR_MODULES, limit=128)
        if final_header != header or final_modules != mod_ptrs:
            failures.append("damage module membership changed during read")
        if not modules:
            failures.append("no readable authoritative damage module")

        return modules, failures

    def sample_inventory_shields(self, actor_ptr: int) -> Tuple[List[Dict[str, Any]], List[str]]:
        """Sample authoritative shields from inventory chain with reciprocal ownership verification.

        Traverses InventoryManager.InventoryChain (+0x280) via Inventory.Inventory (+0x2A8).
        Validates:
        - Shield.Owner (+0x180) == actor_ptr
        - Shield.Instigator (+0x190) == actor_ptr
        - Shield.InvManager (+0x2B0) == inv_manager_ptr
        Authoritative shield value is CurrentShields at +0x3C4.
        """
        shields: List[Dict[str, Any]] = []
        failures: List[str] = []

        inv_raw = self.reader.read(actor_ptr + OFFSET_PAWN_INVENTORY, 8)
        if len(inv_raw) != 8:
            return [], ["pawn inventory manager pointer unreadable"]
        inv_manager = struct.unpack("<Q", inv_raw)[0]
        if not inv_manager:
            return [], failures
        if not self.is_a(inv_manager, "SFXInventoryManager"):
            failures.append(f"actor 0x{actor_ptr:x} inventory manager 0x{inv_manager:x} does not inherit from InventoryManager")
            return [], failures

        manager_identity = self.object_identity(inv_manager)
        head_raw = self.reader.read(inv_manager + OFFSET_INVENTORY_CHAIN, 8)
        if len(head_raw) != 8:
            return [], ["inventory chain head unreadable"]
        curr_item = struct.unpack("<Q", head_raw)[0]
        initial_head = curr_item
        seen_items: Set[int] = set()
        links: List[Tuple[int, int, Optional[Dict[str, Any]]]] = []

        while curr_item and len(seen_items) < MAX_INVENTORY_CHAIN_LIMIT and curr_item not in seen_items:
            seen_items.add(curr_item)
            if not self.is_live_object(curr_item) or not self.is_a(curr_item, "Inventory"):
                if curr_item != 0:
                    failures.append(f"actor 0x{actor_ptr:x} inventory item 0x{curr_item:x} is not a valid live Inventory object")
                break

            item_identity = self.object_identity(curr_item)
            next_raw = self.reader.read(curr_item + OFFSET_INVENTORY_NEXT, 8)
            if len(next_raw) != 8:
                failures.append(f"inventory item 0x{curr_item:x} next pointer unreadable")
                break
            next_item = struct.unpack("<Q", next_raw)[0]
            links.append((curr_item, next_item, item_identity))

            if self.is_a(curr_item, "SFXShield_Base"):
                # VERIFY RECIPROCAL OWNERSHIP LINKS
                owner_ptr = self.reader.read_ptr(curr_item + OFFSET_INVENTORY_OWNER)
                instigator_ptr = self.reader.read_ptr(curr_item + OFFSET_INVENTORY_INSTIGATOR)
                inv_man_ptr = self.reader.read_ptr(curr_item + OFFSET_INVENTORY_MANAGER)

                owner_matches = (owner_ptr == actor_ptr)
                instigator_matches = (instigator_ptr == actor_ptr)
                inv_man_matches = (inv_man_ptr == inv_manager)

                if not (owner_matches and instigator_matches and inv_man_matches):
                    failures.append(
                        f"shield 0x{curr_item:x} reciprocal link mismatch: "
                        f"owner_matches={owner_matches}, instigator_matches={instigator_matches}, "
                        f"inv_man_matches={inv_man_matches}"
                    )
                    curr_item = next_item
                    continue

                current_shields = self.reader.read_float(curr_item + OFFSET_SHIELD_CURRENT_VALUE)
                max_shields = self.reader.read_float(curr_item + OFFSET_SHIELD_MAX_VALUE)
                resistance = self.reader.read_byte(curr_item + OFFSET_SHIELD_RESISTANCE)

                if current_shields is None:
                    failures.append(f"shield 0x{curr_item:x} CurrentShields at +0x3C4 is unreadable or non-finite")
                    curr_item = next_item
                    continue

                item_class = self.get_object_name(self.reader.read_ptr(curr_item + OFFSET_UOBJECT_CLASS))
                item_path = self.get_full_path(curr_item)

                shields.append({
                    "pointer": hex(curr_item),
                    "identity": item_identity,
                    "owner": hex(owner_ptr),
                    "instigator": hex(instigator_ptr),
                    "inv_manager": hex(inv_man_ptr),
                    "class": item_class,
                    "path": item_path,
                    "owner_matches": True,
                    "instigator_matches": True,
                    "inv_manager_matches": True,
                    "current_shields": current_shields,
                    "max_shields": max_shields,
                    "resistance": resistance,
                })
                if (self.object_identity(curr_item) != item_identity or
                    self.reader.read_ptr(curr_item + OFFSET_INVENTORY_OWNER) != owner_ptr or
                    self.reader.read_ptr(curr_item + OFFSET_INVENTORY_INSTIGATOR) != instigator_ptr or
                    self.reader.read_ptr(curr_item + OFFSET_INVENTORY_MANAGER) != inv_man_ptr):
                    failures.append(f"shield 0x{curr_item:x} changed identity/owner during read")

            curr_item = next_item

        if curr_item:
            failures.append("inventory chain cycle, invalid link or traversal limit")
        if (self.object_identity(inv_manager) != manager_identity or
            self.reader.read_ptr(actor_ptr + OFFSET_PAWN_INVENTORY) != inv_manager or
            self.reader.read_ptr(inv_manager + OFFSET_INVENTORY_CHAIN) != initial_head):
            failures.append("inventory manager or chain head changed during read")
        for item, next_item, identity in links:
            if self.object_identity(item) != identity or self.reader.read_ptr(item + OFFSET_INVENTORY_NEXT) != next_item:
                failures.append("inventory chain membership/identity changed during read")
                break

        return shields, failures

    def sample_snapshot(self, sample_index: int = 0, deadline: Optional[float] = None) -> Dict[str, Any]:
        source = self.reader
        now = time.monotonic()
        self.reader = BudgetedReader(source, min(deadline if deadline is not None else now + 3.0, now + 3.0))
        try:
            return self._sample_snapshot(sample_index)
        except SamplingBudgetExceeded as exc:
            return {"sample_index": sample_index, "monotonic_time_s": now,
                    "monotonic_ns": time.monotonic_ns(), "read_only": True,
                    "status": "budget_exhausted", "authoritative": False,
                    "valid_for_decrease": False, "hostiles_count": 0,
                    "hostiles": [], "read_failures": [str(exc)]}
        finally:
            self.reader = source
            self._gobjects_info_cache = None

    def _sample_snapshot(self, sample_index: int = 0) -> Dict[str, Any]:
        """Perform one authoritative sample snapshot of current world hostile health and shields."""
        now_monotonic = time.monotonic()
        now_ns = time.monotonic_ns()
        failures: List[str] = []
        self._class_ancestry_cache.clear()
        self._stable_arrays.clear()
        self._stable_fields.clear()
        self._level_headers.clear()

        # Cache GObjects header for the duration of this snapshot
        gobjects_header = self.reader.read(self.base + OFFSET_GOBJECTS, 16)
        if len(gobjects_header) == 16:
            data_ptr, count, max_objs = struct.unpack("<Qii", gobjects_header)
            if 0 < count <= MAX_LIVE_OBJECTS_CAP and max_objs >= count:
                self._gobjects_info_cache = (data_ptr, count)
            else:
                self._gobjects_info_cache = None
        else:
            self._gobjects_info_cache = None

        try:
            world_ptr = self.reader.read_ptr(self.base + OFFSET_GWORLD)
            if not world_ptr or not self.is_a(world_ptr, "World"):
                return {
                    "sample_index": sample_index,
                    "monotonic_time_s": now_monotonic,
                    "monotonic_ns": now_ns,
                    "read_only": True,
                    "status": "world_unavailable",
                    "hostiles_count": 0,
                    "authoritative": False,
                    "valid_for_decrease": False,
                    "hostiles": [],
                    "read_failures": ["GWorld pointer is null, unreadable, or not a World"],
                }

            # Enumerate actors
            actors, enum_failures = self.enumerate_actors(world_ptr)
            failures.extend(enum_failures)
            world_identity = self.object_identity(world_ptr)
            persistent_level = self.reader.read_ptr(world_ptr + OFFSET_UWORLD_PERSISTENT_LEVEL)
            world_info = next((a["pointer"] for a in actors if self.is_a(a["pointer"], "WorldInfo")), 0)
            world_info_identity = self.object_identity(world_info) if world_info else None
            netmode = self.reader.read_byte(world_info + OFFSET_WORLDINFO_NET_MODE) if world_info else None
            authoritative = netmode in (0, 2)
            if not authoritative:
                failures.append(f"host authority unverified: NetMode={netmode!r}; require standalone(0) or listen host(2)")

            hostiles: List[Dict[str, Any]] = []
            for actor_info in actors:
                actor = actor_info["pointer"]
                is_hostile, reason, meta = self.classify_pawn(actor)
                if not is_hostile:
                    continue
                if actor_info["hidden"] or not actor_info["level_visible"]:
                    continue
                identity = self.object_identity(actor)
                actor_role = self.reader.read_byte(actor + OFFSET_ACTOR_ROLE)
                actor_world = self.reader.read_ptr(actor + OFFSET_ACTOR_WORLD_INFO)
                inventory_identity = self.object_identity(self.reader.read_ptr(actor + OFFSET_PAWN_INVENTORY))
                local_failures: List[str] = []
                if actor_role != 3:
                    local_failures.append(f"pawn 0x{actor:x} is not authoritative Role3")

                # Read location
                loc_raw = self.reader.read(actor + OFFSET_ACTOR_LOCATION, 12)
                location = list(struct.unpack("<3f", loc_raw)) if len(loc_raw) == 12 else None
                if location is None or not all(math.isfinite(v) for v in location):
                    location = None
                    local_failures.append(f"pawn 0x{actor:x} location unreadable or non-finite")

                # Legacy pawn health (recorded for comparison/proof only; never authoritative)
                legacy_pawn_health = self.reader.read_int32(actor + OFFSET_PAWN_HEALTH_LEGACY)

                # BioPawn dead flag
                bio_dead = False
                if self.is_a(actor, "BioPawn"):
                    dead_flags = self.reader.read_uint32(actor + OFFSET_BIOPAWN_DEAD_FLAGS)
                    if dead_flags is None:
                        local_failures.append(f"pawn 0x{actor:x} dead flags unreadable")
                    bio_dead = bool(dead_flags and (dead_flags & 0x200))

                # Sample authoritative damage modules
                damage_modules, mod_failures = self.sample_damage_modules(actor)
                local_failures.extend(mod_failures)

                # Sample authoritative shields
                shields, shield_failures = self.sample_inventory_shields(actor)
                local_failures.extend(shield_failures)
                actor_flags_after = self.reader.read_uint32(actor + OFFSET_ACTOR_FLAGS)
                pending_after = self.reader.read_uint32(actor + OFFSET_ACTOR_PENDING_FLAGS)
                if (identity is None or self.object_identity(actor) != identity or
                    self.reader.read_ptr(actor + OFFSET_ACTOR_WORLD_INFO) != actor_world or
                    self.reader.read_byte(actor + OFFSET_ACTOR_ROLE) != actor_role or
                    actor_flags_after is None or actor_flags_after & 8 or
                    pending_after is None or pending_after & 0x800000 or
                    self.object_identity(self.reader.read_ptr(actor + OFFSET_PAWN_INVENTORY)) != inventory_identity):
                    local_failures.append(f"pawn 0x{actor:x} identity/lifecycle changed during sample")
                failures.extend(local_failures)

                hostiles.append({
                    "pointer": hex(actor),
                    "identity": identity,
                    "world_info": hex(actor_world),
                    "role": actor_role,
                    "inventory_identity": inventory_identity,
                    "measurement_complete": not local_failures,
                    "measurement_failures": local_failures,
                    "hostility_verified": meta.get("hostility_verified", False),
                    "path": meta.get("path", ""),
                    "class": meta.get("class", ""),
                    "level_visible": actor_info["level_visible"],
                    "hidden": actor_info["hidden"],
                    "bio_dead": bio_dead,
                    "location": location,
                    "team_index": meta.get("team_index"),
                    "squad": meta.get("squad"),
                    "controller": meta.get("controller"),
                    "legacy_pawn_health": legacy_pawn_health,
                    "damage_modules": damage_modules,
                    "shields": shields,
                    "hostile_indicators": meta.get("hostile_indicators", {}),
                })

            # Re-check world pointer and level actor table stability
            world_check = self.reader.read_ptr(self.base + OFFSET_GWORLD)
            level_tables_changed = False
            for lev_ptr, initial_header in self._level_headers.items():
                curr_header = self.reader.read(lev_ptr + OFFSET_ULEVEL_ACTORS, 16)
                if curr_header != initial_header:
                    level_tables_changed = True
                    failures.append(f"level 0x{lev_ptr:x} actor table changed during snapshot")
                    break
            for address, (initial_header, initial_items) in self._stable_arrays.items():
                current_header, current_items = self.reader.read_tarray(address, MAX_ARRAY_ELEMENT_LIMIT)
                if current_header != initial_header or tuple(current_items) != initial_items:
                    level_tables_changed = True
                    failures.append(f"world/level array 0x{address:x} contents changed during snapshot")
                    break
            for address, initial in self._stable_fields.items():
                if self.reader.read(address, len(initial)) != initial:
                    level_tables_changed = True
                    failures.append("streaming loaded/visible state changed during snapshot")
                    break
            if (self.reader.read(self.base + OFFSET_GOBJECTS, 16) != gobjects_header or
                self.object_identity(world_ptr) != world_identity or
                self.object_identity(world_info) != world_info_identity or
                self.reader.read_ptr(world_ptr + OFFSET_UWORLD_PERSISTENT_LEVEL) != persistent_level or
                self.reader.read_byte(world_info + OFFSET_WORLDINFO_NET_MODE) != netmode):
                failures.append("world/object table identity or authority changed during snapshot")

            if world_check != world_ptr:
                failures.append(f"GWorld pointer changed during snapshot (0x{world_ptr:x} -> 0x{world_check:x})")
                status = "world_changed"
            elif level_tables_changed:
                status = "level_actors_changed"
            elif failures:
                status = "sample_with_failures"
            elif not hostiles:
                status = "no_hostiles_found"
            else:
                status = "ok"

            return {
                "sample_index": sample_index,
                "monotonic_time_s": now_monotonic,
                "monotonic_ns": now_ns,
                "read_only": True,
                "status": status,
                "authoritative": authoritative,
                "valid_for_decrease": authoritative and not failures,
                "netmode": netmode,
                "world_identity": world_identity,
                "world_info_identity": world_info_identity,
                "world_pointer": hex(world_ptr),
                "actors_scanned": len(actors),
                "hostiles_count": len(hostiles),
                "hostiles": hostiles,
                "read_failures": failures,
            }
        finally:
            self._gobjects_info_cache = None

    def run_bounded_sampling(
        self,
        duration_seconds: float,
        interval_seconds: float,
        output_stream: Optional[Any] = None,
        callback: Optional[Callable[[Dict[str, Any]], None]] = None,
    ) -> List[Dict[str, Any]]:
        """Run bounded periodic sampling, streaming private JSONL records.

        Enforces duration in [0.1, 120.0] seconds and interval in [0.05, 5.0] seconds.
        """
        if not (MIN_BOUNDED_DURATION_S <= duration_seconds <= MAX_BOUNDED_DURATION_S):
            raise ValueError(f"duration_seconds must be between {MIN_BOUNDED_DURATION_S} and {MAX_BOUNDED_DURATION_S}")
        if not (MIN_BOUNDED_INTERVAL_S <= interval_seconds <= MAX_BOUNDED_INTERVAL_S):
            raise ValueError(f"interval_seconds must be between {MIN_BOUNDED_INTERVAL_S} and {MAX_BOUNDED_INTERVAL_S}")

        samples: List[Dict[str, Any]] = []
        start_time = time.monotonic()
        deadline = start_time + duration_seconds
        sample_index = 0

        while (time.monotonic() - start_time) < duration_seconds:
            sample = self.sample_snapshot(sample_index=sample_index, deadline=deadline)
            samples.append(sample)

            if output_stream is not None:
                line = json.dumps(sample, allow_nan=False) + "\n"
                output_stream.write(line)
                if hasattr(output_stream, "flush"):
                    output_stream.flush()

            if callback is not None:
                callback(sample)

            sample_index += 1
            remaining = deadline - time.monotonic()
            if remaining > 0:
                time.sleep(min(interval_seconds, remaining))

        return samples


def evaluate_damage_decrease(observations: List[Dict[str, Any]], target_pointer: Optional[str] = None) -> Dict[str, Any]:
    """Compare complete observations only within an uninterrupted identity epoch.

    Missing/unreadable data is unknown. A changed world, pawn generation, damage
    module or shield starts a new epoch. Health deltas never establish the shooter.
    """
    pointers = list(dict.fromkeys(
        h.get("pointer", "").lower() for o in observations for h in o.get("hostiles", []) if h.get("pointer")
    ))

    def observation_values(o, h):
        if (o.get("status") != "ok" or not o.get("valid_for_decrease") or
            not o.get("authoritative") or o.get("netmode") not in (0, 2) or
            o.get("read_failures") or not h.get("measurement_complete") or
            h.get("measurement_failures") or h.get("role") != 3):
            return None
        identities = [o.get("world_identity"), o.get("world_info_identity"), h.get("identity")]
        if any(not i or not all(k in i for k in ("pointer", "internal_index", "fname_index", "fname_number", "class_pointer", "outer_pointer")) for i in identities):
            return None
        modules, shields = h.get("damage_modules"), h.get("shields")
        if not isinstance(modules, list) or not modules or not isinstance(shields, list):
            return None
        parts, values = [], []
        for items, value_key, link_keys in (
            (modules, "current_health", ("module_owner",)),
            (shields, "current_shields", ("owner", "instigator", "inv_manager")),
        ):
            seen, total, component_ids = set(), 0.0, []
            for item in items:
                identity, value = item.get("identity"), item.get(value_key)
                pointer = item.get("pointer")
                if (not identity or not all(k in identity for k in ("internal_index", "fname_index", "fname_number", "class_pointer", "outer_pointer")) or
                    pointer != identity.get("pointer") or pointer in seen or
                    not isinstance(value, (float, int)) or not math.isfinite(value) or
                    any(k not in item for k in link_keys)):
                    return None
                if value_key == "current_health" and item["module_owner"] != h["pointer"]:
                    return None
                if value_key == "current_shields" and (item["owner"] != h["pointer"] or item["instigator"] != h["pointer"]):
                    return None
                seen.add(pointer)
                total += value
                component_ids.append((identity, tuple(item[k] for k in link_keys)))
            parts.append(sorted(component_ids, key=lambda x: x[0]["pointer"]))
            values.append(total)
        stamp = json.dumps((identities, o.get("world_pointer"), o["netmode"], h.get("path"),
                            h.get("class"), h.get("world_info"), h.get("inventory_identity"), parts), sort_keys=True)
        timestamp = o.get("monotonic_time_s")
        if not isinstance(timestamp, (int, float)) or not math.isfinite(timestamp):
            return None
        return stamp, timestamp, *values

    def evaluate_target(pointer):
        epochs, current, previous_stamp, previous_time = [], [], None, None
        qualified = 0
        for o in observations:
            matches = [h for h in o.get("hostiles", []) if h.get("pointer", "").lower() == pointer]
            v = observation_values(o, matches[0]) if len(matches) == 1 else None
            if v is None:
                if current:
                    epochs.append(current)
                current, previous_stamp, previous_time = [], None, None
                continue
            stamp, timestamp, health, shield = v
            if stamp != previous_stamp or (previous_time is not None and timestamp <= previous_time):
                if current:
                    epochs.append(current)
                current = []
            current.append((timestamp, health, shield, matches[0]))
            qualified += 1
            previous_stamp, previous_time = stamp, timestamp
        if current:
            epochs.append(current)
        results = []
        for epoch in epochs:
            if len(epoch) < 2:
                continue
            health_peak, shield_peak = epoch[0][1:3]
            health_decrease = shield_decrease = 0.0
            for _, health, shield, _ in epoch:
                health_decrease = max(health_decrease, health_peak - health)
                shield_decrease = max(shield_decrease, shield_peak - shield)
                health_peak, shield_peak = max(health_peak, health), max(shield_peak, shield)
            first, last = epoch[0], epoch[-1]
            results.append({
                "target_pointer": pointer, "target_path": first[3].get("path"),
                "samples_count": len(epoch), "initial_health": first[1], "final_health": last[1],
                "initial_shields": first[2], "final_shields": last[2],
                "health_decrease": health_decrease, "shield_decrease": shield_decrease,
                "net_health_decrease": max(0.0, first[1] - last[1]),
                "net_shield_decrease": max(0.0, first[2] - last[2]),
                "time_window_seconds": last[0] - first[0],
                "decrease_observed": health_decrease > 0.001 or shield_decrease > 0.001,
            })
        if not results:
            return {"target_pointer": pointer, "decrease_observed": False,
                    "health_decrease": 0.0, "shield_decrease": 0.0,
                    "reason": "insufficient complete samples in a stable identity epoch",
                    "samples_count": qualified, "epochs": []}
        result = dict(max(results, key=lambda r: r["health_decrease"] + r["shield_decrease"]))
        result["epochs"] = results
        return result

    primary = target_pointer.lower() if target_pointer else (pointers[0] if pointers else None)
    per_target = {p: evaluate_target(p) for p in pointers}
    result = dict(per_target.get(primary) or evaluate_target(primary))
    result.update(
        any_hostile_decrease_observed=any(r["decrease_observed"] for r in per_target.values()),
        per_target_evaluations=per_target,
        guest_damage_attribution_proven=False,
        attribution_note="A stable health/shield decrease establishes only that decrease. Without observed hit/weapon attribution from actual damage processing, the shooter is unproven. ServerStartFire or queued ProcessInstantHit entry alone is insufficient.",
        unresolved_limitations=[
            "Candidate classification is not a verified hostile relationship to Shepard.",
            "The shooter may be Shepard, AI, environment or script; this sampler does not instrument actual damage processing.",
            "Object address/index/name identity cannot prove uninterrupted lifetime between observations when exact identity fields are all reused.",
        ],
    )
    return result
