"""Independent game-copy preparation. No game launch, source mutation or hardlinks."""
import argparse
import hashlib
import json
import os
import shutil
import stat
import time
from pathlib import Path

MARKER = ".me3-coop-sandbox.json"


def digest(path):
    h = hashlib.sha256()
    with Path(path).open("rb") as f:
        for chunk in iter(lambda: f.read(4 * 1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def reparse(path):
    s = Path(path).lstat()
    return stat.S_ISLNK(s.st_mode) or bool(getattr(s, "st_file_attributes", 0) & 0x400)


def safe_path(root, rel):
    """Do not let a manifest name, symlink or junction escape its sandbox."""
    if not isinstance(rel, str) or not rel or "\\" in rel or ":" in rel:
        raise ValueError("invalid relative path")
    parts = rel.split("/")
    if any(p in ("", ".", "..") for p in parts):
        raise ValueError("invalid path component")
    root = Path(root).resolve(strict=True)
    p = root.joinpath(*parts)
    for parent in (p, *p.parents):
        if parent == root:
            break
        if parent.exists() and reparse(parent):
            raise ValueError("reparse points are forbidden")
    if not p.resolve().is_relative_to(root):
        raise ValueError("path outside sandbox")
    return p


def atomic_json(path, data):
    p = Path(path)
    tmp = p.with_name(p.name + ".tmp")
    with tmp.open("x", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)
        f.write("\n")
        f.flush()
        os.fsync(f.fileno())
    os.replace(tmp, p)


def inventory(root):
    root = Path(root)
    if reparse(root):
        raise ValueError("source root is a reparse point")
    result = []
    for folder, dirs, files in os.walk(root, followlinks=False):
        for name in dirs + files:
            if reparse(Path(folder, name)):
                raise ValueError("source contains a reparse point")
        for name in files:
            p = Path(folder, name)
            s = p.stat()
            result.append({"path": p.relative_to(root).as_posix(), "size": s.st_size,
                           "mtime_ns": s.st_mtime_ns})
    return sorted(result, key=lambda x: x["path"])


def prepare(source, destination):
    source_arg, dest_arg = Path(source).absolute(), Path(destination).absolute()
    # Check lexical ancestors as well: resolve() alone would hide a junction.
    for p in (source_arg, dest_arg):
        for ancestor in (p, *p.parents):
            if ancestor.exists() and reparse(ancestor):
                raise ValueError("source/destination ancestry contains a reparse point")
    src = source_arg.resolve(strict=True)
    dst = dest_arg.resolve()
    if src == dst or src.is_relative_to(dst) or dst.is_relative_to(src):
        raise ValueError("source and destination overlap")
    if dst.exists():
        raise FileExistsError("destination already exists; no files were overwritten")
    entries = inventory(src)
    size = sum(e["size"] for e in entries)
    dst.parent.mkdir(parents=True, exist_ok=True)
    if shutil.disk_usage(dst.parent).free < size + 1024**3:
        raise OSError("insufficient space for an independent copy plus 1 GiB margin")
    dst.mkdir()
    marker = {"schema": 1, "status": "incomplete", "source": str(src),
              "destination": str(dst), "files": [], "total_bytes": size,
              "created_unix": int(time.time()), "game_launched": False}
    atomic_json(dst / MARKER, marker)
    total = 0
    for i, e in enumerate(entries, 1):
        original, target = safe_path(src, e["path"]), safe_path(dst, e["path"])
        before = original.stat()
        if (before.st_size, before.st_mtime_ns) != (e["size"], e["mtime_ns"]):
            raise RuntimeError("source changed after inventory: " + e["path"])
        target.parent.mkdir(parents=True, exist_ok=True)
        h = hashlib.sha256()
        with original.open("rb") as input_file, target.open("xb") as output_file:
            for chunk in iter(lambda: input_file.read(4 * 1024 * 1024), b""):
                h.update(chunk)
                output_file.write(chunk)
            output_file.flush()
        after = original.stat()
        if (after.st_size, after.st_mtime_ns) != (e["size"], e["mtime_ns"]):
            raise RuntimeError("source changed while copying: " + e["path"])
        if digest(target) != h.hexdigest():
            raise RuntimeError("copy hash mismatch: " + e["path"])
        # Do not preserve a write-protection bit on the independent test copy.
        os.utime(target, ns=(after.st_atime_ns, after.st_mtime_ns))
        marker["files"].append(dict(e, sha256=h.hexdigest()))
        total += e["size"]
        if i == 1 or i % 200 == 0 or i == len(entries):
            print(f"COPY {i}/{len(entries)}  {total / 1024**3:.2f}/{size / 1024**3:.2f} GiB", flush=True)
    if inventory(src) != entries:
        raise RuntimeError("source changed during preparation")
    marker["status"] = "verified-copy"
    marker["completed_unix"] = int(time.time())
    atomic_json(dst / MARKER, marker)
    print("VERIFIED independent copy; game not launched", flush=True)
    return marker


def verify(sandbox, original=False):
    sandbox = Path(sandbox).resolve(strict=True)
    marker_path = safe_path(sandbox, MARKER)
    marker = json.loads(marker_path.read_text(encoding="utf-8"))
    if marker.get("status") != "verified-copy" or marker.get("destination") != str(sandbox):
        raise ValueError("not a verified sandbox at its recorded location")
    root = Path(marker["source"]) if original else sandbox
    problems = []
    for e in marker["files"]:
        p = safe_path(root, e["path"])
        if not p.is_file() or p.stat().st_size != e["size"] or digest(p) != e["sha256"]:
            problems.append(e["path"])
    expected = {e["path"] for e in marker["files"]}
    extras = {e["path"] for e in inventory(root)} - expected
    if not original:
        extras.discard(MARKER)
    problems += sorted(extras)
    print(json.dumps({"target": "original" if original else "test-copy", "checked": len(expected),
                      "unchanged": not problems, "differences": problems}, indent=2), flush=True)
    return problems


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    subs = parser.add_subparsers(dest="command", required=True)
    p = subs.add_parser("prepare")
    p.add_argument("source", type=Path)
    p.add_argument("destination", type=Path)
    for command in ("verify", "verify-source"):
        p = subs.add_parser(command)
        p.add_argument("sandbox", type=Path)
    args = parser.parse_args()
    if args.command == "prepare":
        prepare(args.source, args.destination)
    elif verify(args.sandbox, original=args.command == "verify-source"):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
