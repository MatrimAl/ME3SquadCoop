"""Reversible authored-file packaging. Never installs into an original game tree.

The release gate is deliberately closed for research and empty/scaffold payloads.
Hashes protect integrity/ownership, not publisher authenticity. No native code is run.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import stat
import struct

PACKAGE_ID = "ME3SquadCoop"
RECEIPT = "ME3SquadCoop.install.json"
TRANSACTION = "ME3SquadCoop.reinstall"
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
HASH = re.compile(r"[0-9a-f]{64}\Z")
VERSION = re.compile(r"[0-9]+\.[0-9]+\.[0-9]+(?:-[a-z0-9.]+)?\Z")


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
        if kinds != {"native", "dlc"} or not any(item["path"].endswith(".pcc") for item in files):
            raise ValueError("release requires real authored DLC and a release native companion")
        if not any(item["path"].endswith("/Mount.dlc") for item in files):
            raise ValueError("authored DLC mount is missing")
    return manifest


def manifest_hash(manifest):
    return hashlib.sha256(json.dumps(manifest, sort_keys=True, separators=(",", ":")).encode()).hexdigest()


def validate_native(path, *, fixture=False):
    raw = Path(path).read_bytes()
    probe_markers = (
        b"SquadProbe", b"ME3SquadCoopProbe", b"SquadDirectIPSelfTest",
        b"SquadGuestCameraSelfTest", b"SquadGuestCinematicSelfTest",
        b"SquadSessionHandleSelfTest", b".me3-coop-sandbox.json",
    )
    if any(marker in raw for marker in probe_markers):
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


def _replace_json(path, value):
    """Replace an already-owned journal without truncating its recoverable version."""
    temporary = safe_path(path.parent, path.name + ".committed.tmp")
    created = False
    try:
        with temporary.open("x", encoding="utf-8", newline="\n") as stream:
            created = True
            json.dump(value, stream, indent=2)
            stream.write("\n")
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, path)
    except BaseException:
        if created:
            temporary.unlink(missing_ok=True)
        raise


def _component_directories(manifest):
    # The executable proves Binaries and Win64 predate every permitted installation.
    protected = {EXE.rsplit("/", 1)[0], EXE.split("/")[0]}
    result = set()
    for item in manifest["files"]:
        parts = item["path"].split("/")
        result.update("/".join(parts[:end]) for end in range(1, len(parts)))
    return result - protected


def _validated_receipt(receipt, *, expected_manifest_sha256=None, allow_fixture=False):
    if not isinstance(receipt, dict):
        raise ValueError("invalid installation receipt")
    manifest = receipt.get("manifest")
    validate_manifest(manifest, allow_fixture=allow_fixture,
                      allow_candidate=receipt.get("qa_candidate") is True)
    actual_hash = manifest_hash(manifest)
    if (receipt.get("schema") != 1 or receipt.get("package_id") != PACKAGE_ID
            or receipt.get("status") not in ("installed", "incomplete")
            or receipt.get("manifest_sha256") != actual_hash
            or (expected_manifest_sha256 is not None and actual_hash != expected_manifest_sha256)
            or type(receipt.get("qa_candidate")) is not bool
            or receipt["qa_candidate"] != (manifest["channel"] == "candidate")):
        raise ValueError("receipt does not match the trusted package manifest")
    directories = receipt.get("created_directories")
    if (not isinstance(directories, list) or any(not isinstance(value, str) for value in directories)
            or len(directories) != len(set(directories))
            or not set(directories).issubset(_component_directories(manifest))):
        raise ValueError("receipt directory is not an owned component ancestor")
    planned = [{key: item[key] for key in ("path", "sha256", "size")} for item in manifest["files"]]
    installed = receipt.get("installed_files")
    if installed is None and receipt["status"] == "installed":
        installed = planned  # Completed receipts from the preceding source version.
    if (not isinstance(installed, list) or len(installed) > len(planned)
            or installed != planned[:len(installed)]
            or (receipt["status"] == "installed" and installed != planned)):
        raise ValueError("invalid installed-file ownership ledger")
    return manifest, installed


def _remove_empty_directories(root, directories):
    removed = 0
    for relative in sorted(directories, key=lambda value: value.count("/"), reverse=True):
        path = safe_path(root, relative)
        if path.is_dir():
            try:
                path.rmdir()
                removed += 1
            except OSError as error:
                if error.errno not in (39, 66) and getattr(error, "winerror", None) != 145:
                    raise
    return removed


def install(package_root, manifest, game_root, *, allow_fixture=False, allow_candidate=False):
    """Preflight every conflict; copy only new allowlisted files. No game launch."""
    payload = verify_payload(package_root, manifest, allow_fixture=allow_fixture, allow_candidate=allow_candidate)
    root = _sandbox(game_root, manifest, allow_fixture)
    ensure_game_closed()
    receipt_path = safe_path(root, RECEIPT)
    if os.path.lexists(receipt_path):
        raise FileExistsError("installation receipt already exists; inspect/remove that installation first")
    if os.path.lexists(safe_path(root, TRANSACTION)):
        raise FileExistsError("reinstall recovery is pending")
    if os.path.lexists(safe_path(root, RECEIPT + ".committed.tmp")):
        raise FileExistsError("receipt temporary file requires inspection")
    targets = [(item, safe_path(root, item["path"])) for item in manifest["files"]]
    for item, target in targets:
        if os.path.lexists(target):
            raise FileExistsError(f"refusing to overwrite {item['path']}")
        for ancestor in target.parents:
            if ancestor == root:
                break
            if ancestor.exists() and not ancestor.is_dir():
                raise FileExistsError("target ancestor is not a directory")
    receipt = {"schema": 1, "package_id": PACKAGE_ID, "manifest_sha256": manifest_hash(manifest),
               "manifest": manifest, "status": "incomplete", "created_directories": [],
               "installed_files": [],
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
                _replace_json(receipt_path, receipt)
            with safe_path(payload, item["path"]).open("rb") as source, target.open("xb") as output:
                for data in iter(lambda: source.read(1024 * 1024), b""):
                    output.write(data)
                output.flush()
                os.fsync(output.fileno())
            if digest(target) != item["sha256"]:
                raise ValueError("new file failed post-copy hash verification")
            receipt["installed_files"].append({key: item[key] for key in ("path", "sha256", "size")})
            _replace_json(receipt_path, receipt)
        receipt["status"] = "installed"
        # Receipt is our newly created file and remains a recovery journal on failure.
        _replace_json(receipt_path, receipt)
    except Exception:
        # Keep the incomplete journal and any partial file; removal refuses changed/unverified bytes.
        raise
    return {"status": "installed", "channel": manifest["channel"], "release_ready": manifest["release_ready"],
            "files": len(targets), "manifest_sha256": receipt["manifest_sha256"]}


def remove(game_root, *, expected_manifest_sha256, allow_fixture=False):
    """Remove only receipt-listed files matching the caller's trusted manifest hash.

    All hashes are checked before any deletion; modified components are preserved.
    Empty directories created during installation are cleaned up, while directories containing
    other files or existing game files are preserved.
    """
    if not isinstance(expected_manifest_sha256, str) or not HASH.fullmatch(expected_manifest_sha256):
        raise ValueError("trusted package manifest fingerprint required")
    root = checked_root(game_root)
    path = safe_path(root, RECEIPT)
    receipt = json.loads(path.read_text(encoding="utf-8"))
    manifest, installed = _validated_receipt(receipt, expected_manifest_sha256=expected_manifest_sha256,
                                             allow_fixture=allow_fixture)
    if os.path.lexists(safe_path(root, TRANSACTION)):
        raise ValueError("recover interrupted reinstall before removing components")
    _sandbox(root, manifest, allow_fixture)
    ensure_game_closed()
    existing = []
    owned = {item["path"] for item in installed}
    for value in receipt["created_directories"]:
        safe_path(root, value)  # Validate every directory before deleting any component.
    for item in manifest["files"]:
        target = safe_path(root, item["path"])
        if not os.path.lexists(target):
            continue
        if item["path"] not in owned:
            raise ValueError(f"unverified incomplete component preserved: {item['path']}")
        if not target.is_file() or digest(target) != item["sha256"]:
            raise ValueError(f"changed component preserved: {item['path']}")
        existing.append(target)
    for target in existing:
        target.unlink()
    removed_dirs = _remove_empty_directories(root, receipt["created_directories"])
    path.unlink()
    return {"status": "removed", "files": len(existing), "removed_directories": removed_dirs}


def inspect_installation(game_root):
    """Inspect installation status and component integrity in game_root without mutating anything."""
    root = checked_root(game_root)
    receipt_path = safe_path(root, RECEIPT)
    if not receipt_path.is_file():
        return {"status": "not_installed", "receipt_present": False}
    try:
        receipt = json.loads(receipt_path.read_text(encoding="utf-8"))
        manifest, _ = _validated_receipt(receipt, allow_fixture=True)
    except Exception as err:
        return {"status": "corrupt_receipt", "receipt_present": True, "error": str(err)}

    files = manifest["files"]
    verified, modified, missing = [], [], []
    for item in files:
        target = safe_path(root, item["path"])
        if not os.path.lexists(target):
            missing.append(item["path"])
        elif not target.is_file():
            modified.append(item["path"])
        else:
            if digest(target) == item["sha256"]:
                verified.append(item["path"])
            else:
                modified.append(item["path"])

    receipt_status = receipt.get("status", "unknown")
    recovery_pending = os.path.lexists(safe_path(root, TRANSACTION))
    is_healthy = (receipt_status == "installed" and len(verified) == len(files)
                  and not modified and not missing and not recovery_pending)
    return {
        "status": "recovery_pending" if recovery_pending else ("installed" if is_healthy else
                  ("incomplete" if receipt_status == "incomplete" else "diverged")),
        "receipt_present": True,
        "receipt_status": receipt_status,
        "package_id": receipt.get("package_id"),
        "manifest_sha256": receipt.get("manifest_sha256"),
        "channel": manifest.get("channel"),
        "version": manifest.get("version"),
        "total_files": len(files),
        "files_verified": verified,
        "files_modified": modified,
        "files_missing": missing,
        "is_clean": is_healthy,
    }


def _matches(path, item):
    return path.is_file() and path.stat().st_size == item["size"] and digest(path) == item["sha256"]


def _copy_verified(source, destination, item):
    destination.parent.mkdir(parents=True, exist_ok=True)
    with source.open("rb") as incoming, destination.open("xb") as outgoing:
        shutil.copyfileobj(incoming, outgoing, 1024 * 1024)
        outgoing.flush()
        os.fsync(outgoing.fileno())
    if not _matches(destination, item):
        raise ValueError("staged component hash/size mismatch")


def _transaction_contents(transaction, old_manifest, new_manifest):
    """Preflight every staging path before touching a recovery target or backup."""
    allowed = {"transaction.json", "transaction.json.committed.tmp", "commit-receipt.json",
               "restore-receipt.json"}
    for category, manifest in (("backup", old_manifest), ("restore", old_manifest), ("new", new_manifest)):
        allowed.update(category + "/" + item["path"] for item in manifest["files"])
    allowed_directories = set()
    for relative in allowed:
        parts = relative.split("/")
        allowed_directories.update("/".join(parts[:end]) for end in range(1, len(parts)))
    files, directories = [], []
    for folder, dirs, names in os.walk(checked_root(transaction), followlinks=False):
        for name in dirs + names:
            child = Path(folder, name)
            relative = child.relative_to(transaction).as_posix()
            safe_path(transaction, relative)
            if relative not in (allowed_directories if name in dirs else allowed):
                raise ValueError("unrecognized transaction content preserved")
            (directories if name in dirs else files).append(child)
    return files, directories


def _transaction_cleanup(transaction, old_manifest, new_manifest):
    """Delete only this transaction's fixed control files and declared staging files."""
    files, directories = _transaction_contents(transaction, old_manifest, new_manifest)
    journal = transaction / "transaction.json"
    for child in files:
        if child != journal:
            child.unlink()
    for child in sorted(directories, key=lambda path: len(path.parts), reverse=True):
        child.rmdir()
    journal.unlink()
    transaction.rmdir()


def recover_reinstall(game_root, *, expected_manifest_sha256, allow_fixture=False):
    """Restore a prepared replacement's verified backups; finish committed cleanup.

    The caller supplies the old package's trusted manifest fingerprint. A changed
    component, receipt, backup or unknown staging file is preserved for inspection.
    """
    if not isinstance(expected_manifest_sha256, str) or not HASH.fullmatch(expected_manifest_sha256):
        raise ValueError("trusted old package manifest fingerprint required")
    root = checked_root(game_root)
    transaction = safe_path(root, TRANSACTION)
    journal = json.loads(safe_path(transaction, "transaction.json").read_text(encoding="utf-8"))
    if (not isinstance(journal, dict) or journal.get("schema") != 1
            or journal.get("package_id") != PACKAGE_ID
            or journal.get("status") not in ("staging", "prepared", "committed")):
        raise ValueError("invalid reinstall recovery journal; backups preserved")
    old_receipt, new_receipt = journal.get("old_receipt"), journal.get("new_receipt")
    old, _ = _validated_receipt(old_receipt, expected_manifest_sha256=expected_manifest_sha256,
                               allow_fixture=allow_fixture)
    new, _ = _validated_receipt(new_receipt, allow_fixture=allow_fixture)
    old_text = journal.get("old_receipt_text")
    if not isinstance(old_text, str) or json.loads(old_text) != old_receipt:
        raise ValueError("invalid original receipt backup")
    old_bytes = old_text.encode("utf-8")
    new_bytes = (json.dumps(new_receipt, indent=2) + "\n").encode("utf-8")
    _transaction_contents(transaction, old, new)
    _sandbox(root, old, allow_fixture)
    ensure_game_closed()
    receipt_path = safe_path(root, RECEIPT)
    if receipt_path.read_bytes() not in (old_bytes, new_bytes):
        raise ValueError("changed installation receipt preserved; backups retained")
    old_files = {item["path"]: item for item in old["files"]}
    new_files = {item["path"]: item for item in new["files"]}
    committed = journal["status"] == "committed"
    if journal["status"] != "staging":
        for category, manifest in (("backup", old), ("restore", old), ("new", new)):
            for item in manifest["files"]:
                staged = safe_path(transaction, category + "/" + item["path"])
                if staged.exists() and not _matches(staged, item):
                    raise ValueError("backup verification failed or changed staging file preserved")
    for relative in old_files.keys() | new_files.keys():
        target = safe_path(root, relative)
        if committed:
            if relative in new_files and not _matches(target, new_files[relative]):
                raise ValueError("changed committed replacement preserved; backups retained")
            if relative not in new_files and os.path.lexists(target):
                raise ValueError("unexpected old component preserved; backups retained")
        elif os.path.lexists(target):
            staged = safe_path(transaction, "new/" + relative) if relative in new_files else None
            # A still-staged file proves this transaction has not installed it.
            if staged is not None and staged.exists() and (relative not in old_files
                    or not _matches(target, old_files[relative])):
                raise ValueError("unowned or modified components preserved; backups retained")
            if not any(_matches(target, item) for item in
                       (old_files.get(relative), new_files.get(relative)) if item):
                raise ValueError("modified components preserved; backups retained")
    if committed:
        if receipt_path.read_bytes() != new_bytes:
            raise ValueError("committed replacement receipt mismatch")
    elif journal["status"] == "staging":
        if receipt_path.read_bytes() != old_bytes or any(
                not _matches(safe_path(root, relative), item) for relative, item in old_files.items()):
            raise ValueError("original changed during staging; backups retained")
    else:
        # Validate every backup before changing any installed file.
        already_restored = (receipt_path.read_bytes() == old_bytes and all(
            _matches(safe_path(root, relative), item) for relative, item in old_files.items())
            and not any(os.path.lexists(safe_path(root, relative))
                        for relative in new_files.keys() - old_files.keys()))
        for relative, item in old_files.items():
            if not already_restored and not _matches(safe_path(transaction, "backup/" + relative), item):
                raise ValueError("backup verification failed; recovery made no changes")
        for relative, item in old_files.items():
            target = safe_path(root, relative)
            if not _matches(target, item):
                staged = safe_path(transaction, "restore/" + relative)
                if staged.exists():
                    if not _matches(staged, item):
                        raise ValueError("changed recovery scratch file preserved")
                else:
                    _copy_verified(safe_path(transaction, "backup/" + relative), staged, item)
                os.replace(staged, target)
        for relative in new_files.keys() - old_files.keys():
            target = safe_path(root, relative)
            if os.path.lexists(target):
                target.unlink()
        if receipt_path.read_bytes() != old_bytes:
            restored = safe_path(transaction, "restore-receipt.json")
            if restored.exists():
                if restored.read_bytes() != old_bytes:
                    raise ValueError("changed recovery receipt scratch file preserved")
            else:
                with restored.open("xb") as stream:
                    stream.write(old_bytes)
                    stream.flush()
                    os.fsync(stream.fileno())
            os.replace(restored, receipt_path)
    _transaction_cleanup(transaction, old, new)
    return {"status": "replacement_committed" if committed else "previous_install_restored"}


def reinstall(package_root, manifest, game_root, *, allow_fixture=False, allow_candidate=False,
              expected_installed_manifest_sha256=None):
    """Stage verified old/new files before replacement; retain backups until commit.

    Refreshing the same package trusts the caller's supplied manifest. Upgrading to
    a different manifest additionally requires the old package's trusted hash.
    """
    payload = verify_payload(package_root, manifest, allow_fixture=allow_fixture,
                             allow_candidate=allow_candidate)
    root = _sandbox(game_root, manifest, allow_fixture)
    ensure_game_closed()
    receipt_path = safe_path(root, RECEIPT)
    if not receipt_path.exists():
        return install(package_root, manifest, root, allow_fixture=allow_fixture, allow_candidate=allow_candidate)
    old_text = receipt_path.read_bytes().decode("utf-8")
    old_receipt = json.loads(old_text)
    trusted = expected_installed_manifest_sha256 or manifest_hash(manifest)
    old, _ = _validated_receipt(old_receipt, expected_manifest_sha256=trusted, allow_fixture=allow_fixture)
    if old_receipt["status"] != "installed":
        raise ValueError("remove incomplete installation before replacement")
    transaction = safe_path(root, TRANSACTION)
    if os.path.lexists(transaction):
        raise ValueError("recover interrupted reinstall before starting another replacement")
    old_files = {item["path"]: item for item in old["files"]}
    new_files = {item["path"]: item for item in manifest["files"]}
    for relative in old_files.keys() | new_files.keys():
        target = safe_path(root, relative)
        if relative in old_files:
            if not _matches(target, old_files[relative]):
                raise ValueError("modified components preserved; replacement rejected")
        elif os.path.lexists(target):
            raise FileExistsError("replacement would overwrite an unowned component")
        if not target.parent.is_dir():
            # All real manifests share the fixed ASI and CookedPCConsole parents.
            raise ValueError("replacement component parent is missing")
    new_receipt = {"schema": 1, "package_id": PACKAGE_ID, "manifest_sha256": manifest_hash(manifest),
                   "manifest": manifest, "status": "installed",
                   "created_directories": [value for value in old_receipt["created_directories"]
                                           if value in _component_directories(manifest)],
                   "installed_files": [{key: item[key] for key in ("path", "sha256", "size")}
                                       for item in manifest["files"]],
                   "qa_candidate": manifest["channel"] == "candidate"}
    journal = {"schema": 1, "package_id": PACKAGE_ID, "status": "staging",
               "old_receipt": old_receipt, "old_receipt_text": old_text, "new_receipt": new_receipt}
    transaction.mkdir()
    journal_path = safe_path(transaction, "transaction.json")
    _write_json_new(journal_path, journal)
    try:
        for relative, item in old_files.items():
            _copy_verified(safe_path(root, relative), safe_path(transaction, "backup/" + relative), item)
        for relative, item in new_files.items():
            _copy_verified(safe_path(payload, relative), safe_path(transaction, "new/" + relative), item)
        _write_json_new(safe_path(transaction, "commit-receipt.json"), new_receipt)
        ensure_game_closed()
        if receipt_path.read_bytes() != old_text.encode("utf-8"):
            raise ValueError("installation receipt changed during staging")
        for relative, item in old_files.items():
            if not _matches(safe_path(root, relative), item):
                raise ValueError("modified components preserved; replacement rejected")
        journal["status"] = "prepared"
        _replace_json(journal_path, journal)
        for relative, item in new_files.items():
            target = safe_path(root, relative)
            if relative not in old_files and os.path.lexists(target):
                raise FileExistsError("replacement target appeared during staging")
            staged = safe_path(transaction, "new/" + relative)
            if not _matches(staged, item):
                raise ValueError("staged replacement changed; backups retained")
            os.replace(staged, target)
        for relative in old_files.keys() - new_files.keys():
            safe_path(root, relative).unlink()
        os.replace(safe_path(transaction, "commit-receipt.json"), receipt_path)
        journal["status"] = "committed"
        _replace_json(journal_path, journal)
    except Exception as error:
        try:
            recover_reinstall(root, expected_manifest_sha256=trusted, allow_fixture=allow_fixture)
        except Exception as recovery_error:
            raise RuntimeError("replacement failed; recovery is pending and backups are retained: "
                               + str(recovery_error)) from error
        raise
    _transaction_cleanup(transaction, old, manifest)
    return {"status": "installed", "channel": manifest["channel"], "release_ready": manifest["release_ready"],
            "files": len(new_files), "manifest_sha256": manifest_hash(manifest)}
