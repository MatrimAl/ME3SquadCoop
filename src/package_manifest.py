"""Reversible authored-file packaging. Never installs into an original game tree.

The release gate is deliberately closed for research and empty/scaffold payloads.
Hashes protect integrity/ownership, not publisher authenticity. No native code is run.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import stat
import struct

PACKAGE_ID = "ME3SquadCoop"
RECEIPT = "ME3SquadCoop.install.json"
SANDBOX_MARKER = ".me3-coop-sandbox.json"
EXE = "Binaries/Win64/MassEffect3.exe"
GAME_SHA256 = "b453871eb6eacff0c646490ffbc588f743399a829b1f9e31cd7775dc95da8001"
REQUIRED_QA = (
    "remote_input", "dialogue", "combat", "disconnect_restore", "transition",
    "private_saves", "radmin", "native_disable", "mod_manager_import",
)
FIXED_TARGETS = {
    "Binaries/Win64/ASI/ME3SquadCoop.asi": "native",
    "BioGame/DLC/DLC_MOD_ME3SquadCoop/CookedPCConsole/Mount.dlc": "dlc",
}
DLC_PREFIX = "BioGame/DLC/DLC_MOD_ME3SquadCoop/CookedPCConsole/"
MOUNT_TARGET = DLC_PREFIX + "Mount.dlc"
ACTIVATION_PREFIX = "BioGame/DLC/DLC_MOD_ME3SquadCoop/"
ACTIVATION_FILES = (
    "PCConsoleTOC.bin", "CookedPCConsole/Mount.dlc",
    "CookedPCConsole/Default_DLC_MOD_ME3SquadCoop.bin",
    "CookedPCConsole/DLC_MOD_ME3SquadCoop_INT.tlk",
)
ACTIVATION_TARGETS = frozenset(ACTIVATION_PREFIX + name for name in ACTIVATION_FILES)
ACTIVATION_STRING_ID = 1915100
FIXED_TARGETS.update(dict.fromkeys(ACTIVATION_TARGETS, "dlc"))
BASE_TOC_TARGET = "BioGame/PCConsoleTOC.bin"
HASH = re.compile(r"[0-9a-f]{64}\Z")
VERSION = re.compile(r"[0-9]+\.[0-9]+\.[0-9]+(?:-[a-z0-9.]+)?\Z")


def make_activation_mount(priority=5100):
    """Author our native-only activation container's LE3 mount metadata.

    Binary layout provenance: docs/DLC-PACKAGING.md. There are no startup
    packages, config changes, copied GUIDs or game assets in this container.
    Readback validates the authored format; it does not prove engine loading.
    """
    if type(priority) is not int or not 1 <= priority <= 65535:
        raise ValueError("LE3 mount priority must be an integer from 1 to 65535")
    raw = struct.pack("<9I", 1, 0x2AD, 0xCD, 0x3006B, priority, 0, 0x0C, 0, 0) + bytes(0x48)
    read_activation_mount(raw)
    return raw


def read_activation_mount(raw, *, string_id=0):
    """Strict bounded readback for this project's asset-free activation DLC."""
    if not isinstance(raw, bytes) or len(raw) != 0x6C:
        raise ValueError("activation Mount.dlc must contain exactly 108 bytes")
    version, package, licensee, cooked, priority, build, flags, content, name = struct.unpack_from("<9I", raw)
    if (version, package, licensee, cooked) != (1, 0x2AD, 0xCD, 0x3006B):
        raise ValueError("Mount.dlc is not the supported LE3 mount format")
    if not 1 <= priority <= 65535:
        raise ValueError("invalid LE3 mount priority")
    if string_id not in (0, ACTIVATION_STRING_ID) or type(string_id) is not int:
        raise ValueError("unsupported authored activation string identity")
    if build or flags != 0x0C or content != string_id or name != string_id or any(raw[0x24:]):
        raise ValueError("unexpected activation mount flags, TLK IDs or reserved build/GUID bytes")
    return {"game": "LE3", "mount_priority": priority, "loads_in_singleplayer": True,
            "loads_in_multiplayer": True, "save_dependency": False, "tlk_id": string_id}


def verify_activation_toc(root):
    """Bounded readback of the three-file authored LE3 activation TOC.

    TOCBinFile stores relative bucket offsets and variable-size entries. The
    last entry's size is zero. No arbitrary package filename is admitted.
    """
    root = checked_root(root)
    raw = safe_path(root, "PCConsoleTOC.bin").read_bytes()
    if len(raw) < 36 or len(raw) > 4096 or struct.unpack_from("<3I", raw) != (0x3AB70C13, 0, 3):
        raise ValueError("unsupported activation TOC header")
    expected = {name.replace("/", "\\"): safe_path(root, name).stat().st_size
                for name in ACTIVATION_FILES if name != "PCConsoleTOC.bin"}
    table = []
    for index in range(256):
        crc = index << 24
        for _ in range(8):
            crc = ((crc << 1) ^ (0x04C11DB7 if crc & 0x80000000 else 0)) & 0xffffffff
        table.append(crc)
    found, ranges = {}, []
    for bucket in range(3):
        start = 12 + bucket * 8
        relative, count = struct.unpack_from("<2I", raw, start)
        if count > 3 or (count == 0) != (relative == 0):
            raise ValueError("invalid activation TOC bucket")
        at = start + relative
        for entry in range(count):
            if at < 36 or at + 29 > len(raw):
                raise ValueError("activation TOC entry outside payload")
            length, flags, size = struct.unpack_from("<HHI", raw, at)
            end = at + length if length else len(raw)
            if end > len(raw) or end < at + 29 or flags or any(raw[at+8:at+28]):
                raise ValueError("invalid authored activation TOC entry")
            nul = raw.find(b"\0", at + 28, end)
            if nul < 0 or any(raw[nul+1:end]):
                raise ValueError("invalid activation TOC filename termination")
            try:
                name = raw[at+28:nul].decode("ascii")
            except UnicodeDecodeError as error:
                raise ValueError("ASCII activation TOC filename required") from error
            if name not in expected or name in found or size != expected[name]:
                raise ValueError("activation TOC filename/size differs from payload")
            hashed = 0
            for value in name.rsplit("\\", 1)[-1].upper().encode("ascii"):
                hashed = (hashed >> 8) ^ table[(hashed ^ value) & 255]
                hashed = (hashed >> 8) ^ table[hashed & 255]
            if hashed % 3 != bucket:
                raise ValueError("activation TOC filename hash bucket mismatch")
            if not length and entry + 1 != count:
                raise ValueError("early terminal activation TOC entry")
            found[name] = size
            ranges.append((at, end))
            at = end
    ordered = sorted(ranges)
    if set(found) != set(expected) or not ordered or ordered[0][0] != 36 or ordered[-1][1] != len(raw):
        raise ValueError("incomplete activation TOC")
    if any(a[1] != b[0] for a,b in zip(ordered,ordered[1:])):
        raise ValueError("overlapping or unaccounted activation TOC bytes")
    return found


def parse_base_toc(raw):
    """Read both native bucket and linear walks without changing a game file."""
    if len(raw) < 20 or len(raw) > 64 * 1024 * 1024:
        raise ValueError("base TOC size outside bounds")
    magic, media, buckets = struct.unpack_from("<III", raw)
    if magic != 0x3ab70c13 or media or not 0 < buckets <= 100000:
        raise ValueError("unsupported base TOC header")
    start = 12 + buckets * 8
    if start >= len(raw):
        raise ValueError("truncated base TOC buckets")
    entries, visited = {}, set()
    for bucket in range(buckets):
        table = 12 + bucket * 8
        offset, count = struct.unpack_from("<ii", raw, table)
        if not 0 <= count <= 100000 or (count == 0 and offset):
            raise ValueError("invalid base TOC bucket")
        at = table + offset
        for index in range(count):
            if at < start or at + 29 > len(raw) or at in visited:
                raise ValueError("overlapping base TOC entry")
            visited.add(at)
            step, flags, size = struct.unpack_from("<hHi", raw, at)
            end = raw.find(b"\0", at + 28, min(len(raw), at + 4096))
            if end < 0 or size < 0 or (step and (step < end+1-at or step % 4)):
                raise ValueError("invalid base TOC entry fields")
            try:
                name = raw[at+28:end].decode("ascii")
            except UnicodeDecodeError as error:
                raise ValueError("ASCII base TOC filenames required") from error
            key = name.casefold()
            if not name or key in entries or (index+1 < count and step <= 0):
                raise ValueError("duplicate or early terminal base TOC entry")
            entries[key] = (name, size, flags, raw[at+8:at+28], bucket)
            at += step
    at, linear = start, set()
    while at in visited and at not in linear:
        linear.add(at)
        step = struct.unpack_from("<h", raw, at)[0]
        if not step:
            break
        at += step
    if not linear or linear != visited or at not in visited or struct.unpack_from("<h", raw, at)[0]:
        raise ValueError("base TOC linear walk differs from buckets")
    return buckets, entries


def toc_filename_hash(name):
    table = []
    for index in range(256):
        crc = index << 24
        for _ in range(8):
            crc = ((crc << 1) ^ (0x04c11db7 if crc & 0x80000000 else 0)) & 0xffffffff
        table.append(crc)
    value = 0
    for char in name.upper().encode("ascii"):
        value = (value >> 8) ^ table[(value ^ char) & 255]
        value = (value >> 8) ^ table[value & 255]
    return value


def encode_base_toc(buckets, entries):
    """Return derived bytes only; retain every supplied entry's native bucket."""
    table = bytearray(struct.pack("<III", 0x3ab70c13, 0, buckets) + bytes(buckets*8))
    records = []
    groups = {}
    for entry in entries.values():
        groups.setdefault(entry[4], []).append(entry)
    for bucket in range(buckets):
        group = groups.get(bucket, [])
        if group:
            struct.pack_into("<ii", table, 12+bucket*8, len(table)-(12+bucket*8), len(group))
        for name, size, flags, hashed, _ in group:
            encoded = name.encode("ascii") + b"\0"
            length = (28 + len(encoded) + 3) & ~3
            if length > 32767:
                raise ValueError("base TOC filename stride outside bounds")
            records.append(len(table))
            table += struct.pack("<HHi", length, flags, size) + hashed + encoded
            table += bytes(length - 28 - len(encoded))
    if not records:
        raise ValueError("empty base TOC")
    struct.pack_into("<H", table, records[-1], 0)
    return bytes(table)


def derived_base_toc(before, manifest):
    buckets, original = parse_base_toc(before)
    expected = {}
    if {item["path"] for item in manifest["files"] if item["kind"] == "dlc"} != ACTIVATION_TARGETS:
        raise ValueError("complete authored activation required for base TOC preparation")
    for item in manifest["files"]:
        if item["kind"] != "dlc" or item["path"].endswith("/PCConsoleTOC.bin"):
            continue
        name = "BIOGame\\DLC\\DLC_MOD_ME3SquadCoop\\" + item["path"][len(ACTIVATION_PREFIX):].replace("/", "\\")
        expected[name.casefold()] = (name, item["size"], 0, bytes(20), toc_filename_hash(name.rsplit("\\",1)[-1]) % buckets)
    if len(expected) != 3:
        raise ValueError("exactly three authored content additions required")
    existing = {key:value for key,value in original.items() if key.startswith("biogame\\dlc\\dlc_mod_me3squadcoop\\")}
    if existing:
        if existing != expected:
            raise ValueError("existing co-op base TOC entries conflict or are incomplete")
        return before
    revised = dict(original)
    revised.update(expected)
    after = encode_base_toc(buckets, revised)
    new_buckets, checked = parse_base_toc(after)
    if buckets != new_buckets or checked != revised or any(checked.get(key) != value for key,value in original.items()):
        raise ValueError("derived TOC changed an existing entry")
    return after


def prepare_base_toc(package_root, manifest, baseline_path, output_path):
    """Create a new derived artifact. Never install, replace or restore a TOC."""
    verify_payload(package_root, manifest, installable=False)
    baseline = Path(baseline_path).absolute()
    safe_path(baseline.parent, baseline.name)
    output = Path(output_path).absolute()
    safe_path(output.parent, output.name)
    if os.path.lexists(output):
        raise FileExistsError("derived TOC output already exists")
    before = baseline.read_bytes()
    after = derived_base_toc(before, manifest)
    if baseline.read_bytes() != before:
        raise ValueError("baseline TOC changed during preparation")
    with output.open("xb") as stream:
        stream.write(after); stream.flush(); os.fsync(stream.fileno())
    if baseline.read_bytes() != before or output.read_bytes() != after:
        raise ValueError("TOC artifact readback failed; baseline was never written")
    return {"status":"derived-toc-prepared-not-installed", "baseline_sha256":hashlib.sha256(before).hexdigest(),
            "output_sha256":hashlib.sha256(after).hexdigest(), "added_entries":0 if before == after else 3,
            "game_file_written":False}


def digest(path):
    h = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for data in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(data)
    return h.hexdigest()


def _plain_path(value):
    if not isinstance(value, str) or len(value) > 220 or not value:
        raise ValueError("invalid relative path")
    if any(c in value for c in "\\:\x00") or value.startswith("/"):
        raise ValueError("absolute, alternate-stream and Windows separator paths forbidden")
    parts = value.split("/")
    devices = {"CON", "PRN", "AUX", "NUL", *(f"COM{i}" for i in range(1, 10)),
               *(f"LPT{i}" for i in range(1, 10))}
    for part in parts:
        if (not re.fullmatch(r"[A-Za-z0-9_.-]+", part) or part in (".", "..")
                or part.endswith((".", " ")) or part.split(".")[0].upper() in devices):
            raise ValueError("unsafe path component")
    return parts


def _is_link(path):
    info = path.lstat()
    return stat.S_ISLNK(info.st_mode) or bool(getattr(info, "st_file_attributes", 0) & 0x400)


def checked_root(root):
    root = Path(root).absolute()
    for path in (root, *root.parents):
        if os.path.lexists(path) and _is_link(path):
            raise ValueError("symlink/junction ancestry forbidden")
    return root.resolve(strict=True)


def safe_path(root, relative):
    parts = _plain_path(relative)
    root = checked_root(root)
    path = root
    for part in parts:
        path = path / part
        if os.path.lexists(path) and _is_link(path):
            raise ValueError("symlink/junction target forbidden")
    if not path.resolve().is_relative_to(root):
        raise ValueError("path escaped root")
    return path


def allowed_target(target, kind):
    _plain_path(target)
    if FIXED_TARGETS.get(target) == kind:
        return
    if (kind == "dlc" and target.startswith(DLC_PREFIX)
            and re.fullmatch(r"ME3SquadCoop_[A-Za-z0-9_]+\.pcc", target[len(DLC_PREFIX):])):
        return
    raise ValueError("target is not an authored co-op component in the fixed allowlist")


def validate_manifest(manifest, *, allow_fixture=False, allow_candidate=False, installable=True):
    if not isinstance(manifest, dict) or manifest.get("schema") != 1:
        raise ValueError("unsupported package schema")
    fields = {"schema", "package_id", "version", "channel", "release_ready", "qa", "qa_evidence", "supported_game_sha256", "files"}
    if set(manifest) != fields:
        raise ValueError("unexpected manifest metadata; private paths/source data must not be bundled")
    if (manifest.get("package_id") != PACKAGE_ID or not isinstance(manifest.get("version"), str)
            or not VERSION.fullmatch(manifest["version"])):
        raise ValueError("invalid package identity")
    if manifest.get("channel") not in ("candidate", "release", "fixture"):
        raise ValueError("unknown release channel")
    if type(manifest.get("release_ready")) is not bool or not isinstance(manifest.get("qa"), dict):
        raise ValueError("invalid release gate metadata")
    if any(key not in REQUIRED_QA or type(value) is not bool for key, value in manifest["qa"].items()):
        raise ValueError("invalid QA gate metadata")
    evidence = manifest.get("qa_evidence")
    if (not isinstance(evidence, dict) or any(key not in REQUIRED_QA or not isinstance(value, str)
            or not HASH.fullmatch(value) for key, value in evidence.items())):
        raise ValueError("invalid QA evidence fingerprints")
    fixture = allow_fixture and manifest.get("channel") == "fixture"
    candidate = manifest.get("channel") == "candidate"
    if candidate:
        if manifest["release_ready"] is not False or set(manifest["qa"]) != set(REQUIRED_QA):
            raise ValueError("candidate must explicitly declare unfinished release status and every QA gate")
        if installable and not allow_candidate:
            raise ValueError("candidate requires explicit QA-only installation into an independent test copy")
    if manifest.get("channel") == "release":
        if manifest.get("release_ready") is not True:
            raise ValueError("release readiness must be declared")
        qa = manifest.get("qa", {})
        if not isinstance(qa, dict) or any(qa.get(key) is not True for key in REQUIRED_QA):
            raise ValueError("release QA is incomplete; do not distribute as a playable mod")
        if set(evidence) != set(REQUIRED_QA):
            raise ValueError("release requires an actual evidence artifact fingerprint for every QA gate")
    elif installable and not fixture and not candidate:
        raise ValueError("scaffold/research packages cannot be installed")
    game_hash = manifest.get("supported_game_sha256")
    if not isinstance(game_hash, str) or not HASH.fullmatch(game_hash):
        raise ValueError("missing exact supported executable fingerprint")
    if not fixture and game_hash != GAME_SHA256:
        raise ValueError("unsupported game build")
    files = manifest.get("files")
    if not isinstance(files, list) or not files or len(files) > 128:
        raise ValueError("empty or excessive component list")
    seen = set()
    for item in files:
        if not isinstance(item, dict) or set(item) != {"path", "sha256", "size", "kind", "origin"}:
            raise ValueError("unexpected file metadata")
        allowed_target(item["path"], item["kind"])
        key = item["path"].casefold()
        if key in seen:
            raise ValueError("duplicate or case-colliding component path")
        seen.add(key)
        if (item["origin"] != "authored" or type(item["size"]) is not int
                or not 0 < item["size"] <= 64 * 1024 * 1024
                or not isinstance(item["sha256"], str) or not HASH.fullmatch(item["sha256"])):
            raise ValueError("invalid authored file size/hash")
    if not fixture:
        kinds = {item["kind"] for item in files}
        if kinds != {"native", "dlc"}:
            raise ValueError("release requires real authored DLC and a release native companion")
        if not any(item["path"] == MOUNT_TARGET for item in files):
            raise ValueError("authored DLC mount is missing")
        dlc_targets = {item["path"] for item in files if item["kind"] == "dlc"}
        # Preserve hash-checked inspection/removal of old Mount-only receipts.
        # New automatic bundles require the complete authored activation set.
        if dlc_targets not in ({MOUNT_TARGET}, ACTIVATION_TARGETS):
            raise ValueError("activation DLC requires its complete authored file set; no PCC/assets")
    return manifest


def manifest_hash(manifest):
    return hashlib.sha256(json.dumps(manifest, sort_keys=True, separators=(",", ":")).encode()).hexdigest()


def validate_native(path, *, fixture=False):
    raw = Path(path).read_bytes()
    if b"SquadProbe" in raw or b"ME3SquadCoopProbe" in raw:
        raise ValueError("research probe is not a release native component")
    if fixture:
        return
    if len(raw) < 512 or raw[:2] != b"MZ":
        raise ValueError("native companion is not a built x64 DLL")
    at = struct.unpack_from("<I", raw, 0x3C)[0]
    if (at > len(raw) - 26 or raw[at:at + 4] != b"PE\0\0"
            or struct.unpack_from("<H", raw, at + 4)[0] != 0x8664
            or not struct.unpack_from("<H", raw, at + 22)[0] & 0x2000
            or struct.unpack_from("<H", raw, at + 24)[0] != 0x20B):
        raise ValueError("native companion must be an x64 PE32+ DLL, never a game EXE")


def verify_payload(package_root, manifest, *, allow_fixture=False, allow_candidate=False, installable=True):
    validate_manifest(manifest, allow_fixture=allow_fixture, allow_candidate=allow_candidate, installable=installable)
    root = safe_path(package_root, "payload")
    checked_root(root)
    expected = {item["path"] for item in manifest["files"]}
    actual = set()
    for folder, dirs, files in os.walk(root, followlinks=False):
        for name in dirs + files:
            if _is_link(Path(folder, name)):
                raise ValueError("payload contains symlink/junction")
        actual.update(Path(folder, name).relative_to(root).as_posix() for name in files)
    if actual != expected:
        raise ValueError("payload file set differs from manifest")
    for item in manifest["files"]:
        path = safe_path(root, item["path"])
        if path.stat().st_size != item["size"] or digest(path) != item["sha256"]:
            raise ValueError("payload hash/size mismatch")
        if item["kind"] == "native":
            # Source-name checks alone are insufficient: reject the actual diagnostic binary marker too.
            validate_native(path, fixture=manifest.get("channel") == "fixture")
        elif manifest.get("channel") != "fixture" and item["path"] == MOUNT_TARGET:
            modern = {entry["path"] for entry in manifest["files"] if entry["kind"] == "dlc"} == ACTIVATION_TARGETS
            read_activation_mount(path.read_bytes(), string_id=ACTIVATION_STRING_ID if modern else 0)
    if manifest.get("channel") != "fixture" and any(entry["path"] == ACTIVATION_PREFIX + "PCConsoleTOC.bin" for entry in manifest["files"]):
        verify_activation_toc(safe_path(root, ACTIVATION_PREFIX.rstrip("/")))
    return root


def ensure_game_closed():
    """Conservatively refuse mutation while any MassEffect3.exe process exists."""
    if os.name != "nt":
        return
    import ctypes
    from ctypes import wintypes
    class Entry(ctypes.Structure):
        _fields_ = [("size", wintypes.DWORD), ("usage", wintypes.DWORD),
                    ("pid", wintypes.DWORD), ("heap", ctypes.c_size_t),
                    ("module", wintypes.DWORD), ("threads", wintypes.DWORD),
                    ("parent", wintypes.DWORD), ("priority", wintypes.LONG),
                    ("flags", wintypes.DWORD), ("name", wintypes.WCHAR * 260)]
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel.CreateToolhelp32Snapshot.argtypes = [wintypes.DWORD, wintypes.DWORD]
    kernel.CreateToolhelp32Snapshot.restype = wintypes.HANDLE
    for name in ("Process32FirstW", "Process32NextW"):
        fn = getattr(kernel, name)
        fn.argtypes = [wintypes.HANDLE, ctypes.POINTER(Entry)]
        fn.restype = wintypes.BOOL
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    kernel.CloseHandle.restype = wintypes.BOOL
    snapshot = kernel.CreateToolhelp32Snapshot(2, 0)
    if snapshot == ctypes.c_void_p(-1).value:
        raise OSError("cannot verify that the game is closed")
    try:
        entry = Entry()
        entry.size = ctypes.sizeof(Entry)
        if not kernel.Process32FirstW(snapshot, ctypes.byref(entry)):
            raise OSError("cannot enumerate game processes")
        while True:
            if entry.name.casefold() == "masseffect3.exe":
                raise RuntimeError("close every MassEffect3.exe process before changing installed components")
            if not kernel.Process32NextW(snapshot, ctypes.byref(entry)):
                if ctypes.get_last_error() != 18:  # ERROR_NO_MORE_FILES
                    raise OSError("process enumeration failed; no files changed")
                break
    finally:
        kernel.CloseHandle(snapshot)


def _sandbox(root, manifest, allow_fixture):
    root = checked_root(root)
    marker = json.loads(safe_path(root, SANDBOX_MARKER).read_text(encoding="utf-8"))
    if (marker.get("status") != "verified-copy" or marker.get("destination") != str(root)
            or marker.get("source") == str(root)):
        raise ValueError("installation requires a verified independent test copy")
    if manifest.get("channel") == "fixture" and not (allow_fixture and marker.get("fixture_test") is True):
        raise ValueError("synthetic fixture package requires a synthetic fixture target")
    exe = safe_path(root, EXE)
    if digest(exe) != manifest["supported_game_sha256"]:
        raise ValueError("target executable differs from supported build")
    return root


def _write_json_new(path, value):
    with path.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(value, stream, indent=2)
        stream.write("\n")
        stream.flush()
        os.fsync(stream.fileno())


def install(package_root, manifest, game_root, *, allow_fixture=False, allow_candidate=False):
    """Preflight every conflict; copy only new allowlisted files. No game launch."""
    payload = verify_payload(package_root, manifest, allow_fixture=allow_fixture, allow_candidate=allow_candidate)
    root = _sandbox(game_root, manifest, allow_fixture)
    ensure_game_closed()
    receipt_path = safe_path(root, RECEIPT)
    if os.path.lexists(receipt_path):
        raise FileExistsError("installation receipt already exists; inspect/remove that installation first")
    targets = [(item, safe_path(root, item["path"])) for item in manifest["files"]]
    for item, target in targets:
        if os.path.lexists(target):
            raise FileExistsError(f"refusing to overwrite {item['path']}")
        for ancestor in target.parents:
            if ancestor == root:
                break
            if ancestor.exists() and not ancestor.is_dir():
                raise FileExistsError("target ancestor is not a directory")
    if {item["path"] for item in manifest["files"] if item["kind"] == "dlc"} == ACTIVATION_TARGETS:
        base_toc = safe_path(root, BASE_TOC_TARGET)
        if not base_toc.is_file():
            raise ValueError("base TOC is missing; no components installed")
        current = base_toc.read_bytes()
        if derived_base_toc(current, manifest) != current or base_toc.read_bytes() != current:
            raise ValueError("base TOC lacks current authored entries; prepare-base-toc creates a separate artifact only; no components installed")
    receipt = {"schema": 1, "package_id": PACKAGE_ID, "manifest_sha256": manifest_hash(manifest),
               "manifest": manifest, "status": "incomplete", "created_directories": [],
               "qa_candidate": manifest.get("channel") == "candidate"}
    # Durable ownership journal before copying; an interrupted copy is never represented as installed.
    _write_json_new(receipt_path, receipt)
    try:
        for item, target in targets:
            missing = []
            for ancestor in target.parents:
                if ancestor == root or ancestor.exists():
                    break
                missing.append(ancestor)
            for directory in reversed(missing):
                directory.mkdir()
                receipt["created_directories"].append(directory.relative_to(root).as_posix())
            with safe_path(payload, item["path"]).open("rb") as source, target.open("xb") as output:
                for data in iter(lambda: source.read(1024 * 1024), b""):
                    output.write(data)
                output.flush()
                os.fsync(output.fileno())
            if digest(target) != item["sha256"]:
                raise ValueError("new file failed post-copy hash verification")
        receipt["status"] = "installed"
        # Receipt is our newly created file and remains a recovery journal on failure.
        committed = safe_path(root, RECEIPT + ".committed.tmp")
        _write_json_new(committed, receipt)
        os.replace(committed, receipt_path)
    except Exception:
        # Keep the incomplete journal and any partial file; removal refuses changed/unverified bytes.
        raise
    return {"status": "installed", "channel": manifest["channel"], "release_ready": manifest["release_ready"],
            "files": len(targets), "manifest_sha256": receipt["manifest_sha256"]}


def remove(game_root, *, expected_manifest_sha256, allow_fixture=False):
    """Remove only receipt-listed files matching the caller's trusted manifest hash.

    All hashes are checked before any deletion; modified components are preserved.
    Empty directories are intentionally retained to avoid touching another mod's directories.
    """
    root = checked_root(game_root)
    path = safe_path(root, RECEIPT)
    receipt = json.loads(path.read_text(encoding="utf-8"))
    manifest = receipt.get("manifest")
    # A candidate's hash-checked removal must remain available after its explicit QA install.
    validate_manifest(manifest, allow_fixture=allow_fixture, allow_candidate=receipt.get("qa_candidate") is True)
    if (receipt.get("package_id") != PACKAGE_ID or receipt.get("status") not in ("installed", "incomplete")
            or manifest_hash(manifest) != expected_manifest_sha256
            or receipt.get("manifest_sha256") != expected_manifest_sha256):
        raise ValueError("receipt does not match the trusted package manifest")
    _sandbox(root, manifest, allow_fixture)
    ensure_game_closed()
    existing = []
    for item in manifest["files"]:
        target = safe_path(root, item["path"])
        if not os.path.lexists(target):
            continue
        if not target.is_file() or digest(target) != item["sha256"]:
            raise ValueError(f"changed component preserved: {item['path']}")
        existing.append(target)
    for target in existing:
        target.unlink()
    path.unlink()
    return {"status": "removed", "files": len(existing)}
