"""Compare complete, read-only package identity JSONL captures (stdlib only).

This is an evidence comparator, not a metadata repair tool. It never writes to
the game and never authorizes a repair. Only --output receives runtime output.
"""
import argparse
import hashlib
import json
import ntpath
from pathlib import Path

PACKAGES = ("Core", "SFXGame", "SFXGameContent")


def select_snapshot(path, phase):
    active = None
    completed = []
    malformed = 0
    with Path(path).open(encoding="utf-8-sig") as stream:
        for number, text in enumerate(stream, 1):
            try:
                row = json.loads(text)
            except (ValueError, TypeError):
                malformed += 1
                if active is not None:
                    active["errors"].append(f"malformed JSON line {number}")
                continue
            if not isinstance(row, dict):
                continue
            if row.get("identity") == "snapshot-begin":
                active = ({"phase": phase, "begin_line": number, "rows": [], "errors": []}
                          if row.get("phase") == phase else None)
            elif active is not None and row.get("identity") == "snapshot-end":
                if row.get("phase") == phase:
                    active["end_line"] = number
                    completed.append(active)
                active = None
            elif active is not None and row.get("phase") == phase:
                active["rows"].append(row)
    if not completed:
        return {"phase": phase, "complete": False, "errors": ["no complete snapshot"], "rows": []}
    chosen = completed[-1]
    chosen.update(complete=True, complete_snapshots=len(completed), malformed_lines=malformed)
    return chosen


def integer(value):
    return isinstance(value, int) and not isinstance(value, bool)


def canonical_hash(value):
    encoded = json.dumps(value, ensure_ascii=True, sort_keys=True, separators=(",", ":")).encode()
    return hashlib.sha256(encoded).hexdigest()


def inspect_table(header, rows, package):
    errors = []
    count = header.get("count")
    offset = header.get("offset")
    if not integer(count) or not 0 < count <= 100000:
        errors.append("invalid table count")
    if not integer(offset) or not 0x60 <= offset <= 0x100 or offset % 4:
        errors.append("invalid table offset")
    slots = {}
    for row in rows:
        slot = row.get("slot")
        if not integer(slot) or not integer(count) or not 0 <= slot < count:
            errors.append("out-of-range or invalid slot")
            continue
        if slot in slots:
            errors.append(f"duplicate slot {slot}")
        slots[slot] = row
    if not integer(count) or len(slots) != count:
        errors.append("missing table rows")
    semantic = []
    rooted = nonnull = zero = one = 0
    for slot, row in sorted(slots.items()):
        path, kind, net, present = (row.get(key) for key in ("path", "class", "net_index", "nonnull"))
        if not isinstance(path, str) or not isinstance(kind, str) or not integer(net) or not isinstance(present, bool):
            errors.append(f"invalid semantic row {slot}")
            continue
        nonnull += present
        owned = present and path.startswith(package + ".") and bool(kind) and "<unreadable>" not in path
        if owned:
            rooted += 1
            zero += net == slot
            one += net == slot + 1
        elif present and (not path or not kind or net < 0):
            errors.append(f"unresolved non-null semantic row {slot}")
        elif not present and (path or kind or net != -1):
            errors.append(f"nonnull=false has object metadata at slot {slot}")
        semantic.append({"offset": offset, "slot": slot, "path": path,
                         "class": kind, "net_index": net, "nonnull": present})
    for field, actual in (("nonnull", nonnull), ("rooted", rooted),
                          ("zero_based_matches", zero), ("one_based_matches", one)):
        if header.get(field) != actual or not integer(header.get(field)):
            errors.append(f"{field} does not match semantic rows")
    if rooted < 3 or rooted * 10 < nonnull * 9:
        errors.append("fewer than three rooted rows or less than 90% rooted")
    convention = "zero-based" if rooted and zero == rooted else "one-based" if rooted and one == rooted else None
    if convention is None:
        errors.append("NetIndex convention is inconsistent")
    return {"offset": offset, "count": count, "nonnull": nonnull, "rooted": rooted,
            "net_index_convention": convention, "valid": not errors, "errors": sorted(set(errors)),
            "semantic_sha256": canonical_hash(semantic), "semantic_rows": semantic}


def inspect_package(snapshot, package):
    rows = [row for row in snapshot["rows"] if row.get("package") == package]
    metadata = [row for row in rows if row.get("identity") == "package"]
    errors = list(snapshot["errors"])
    if len(metadata) != 1:
        errors.append("missing package" if not metadata else "duplicate package metadata")
    info = metadata[0] if len(metadata) == 1 else {}
    generations = info.get("generation_counts")
    generation_valid = (isinstance(generations, list) and bool(generations)
                        and len(generations) <= 32
                        and all(integer(x) and x >= 0 for x in generations))
    guid = info.get("guid")
    try:
        guid_valid = (isinstance(guid, str) and len(guid) == 35
                      and [len(part) for part in guid.split("-")] == [8] * 4)
        if guid_valid:
            [int(part, 16) for part in guid.split("-")]
    except ValueError:
        guid_valid = False
    if not generation_valid:
        errors.append("missing or invalid generation array")
    if not guid_valid:
        errors.append("missing or invalid GUID")
    headers = [row for row in rows if row.get("identity") == "net-table"]
    offsets = [header.get("offset") for header in headers]
    if len({str(x) for x in offsets}) != len(offsets):
        errors.append("duplicate table offset")
    if not headers:
        errors.append("missing net table")
    objects = [row for row in rows if row.get("identity") == "net-object"]
    if any(row.get("offset") not in offsets for row in objects):
        errors.append("object rows without table header")
    tables = [inspect_table(header, [row for row in objects if row.get("offset") == header.get("offset")], package)
              for header in headers]
    summaries = [row for row in rows if row.get("identity") == "package-summary"]
    if len(summaries) != 1 or summaries[0].get("candidate_tables") != len(headers):
        errors.append("missing or inconsistent package summary")
    children = [{key: row.get(key) for key in ("path", "class", "net_index", "linker_index", "linker")}
                for row in rows if row.get("identity") == "child"]
    return {"present": bool(metadata), "guid": guid, "generation_counts": generations,
            "generation_valid": generation_valid, "metadata_valid": not errors,
            "errors": sorted(set(errors)), "tables": tables, "linker": info.get("linker", {}),
            "sampled_children": children}


def calibrate_linker(packages, linker_cwd=None):
    """Require both file-backed controls, exact root and readable known filename."""
    control_results = {}
    for package in ("Core", "SFXGame"):
        result = packages[package]
        sources = [result["linker"]] + [row.get("linker") or {} for row in result["sampled_children"]]
        good = set()
        for linker in sources:
            if not linker.get("present") or linker.get("root") != package or not str(linker.get("class", "")).startswith("Linker"):
                continue
            for candidate in ("194", "198"):
                filename = linker.get("candidate_" + candidate)
                if not isinstance(filename, str) or ntpath.basename(filename).lower() != package.lower() + ".pcc":
                    continue
                if linker.get("candidate_" + candidate + "_exists") is not True:
                    continue
                try:
                    control_path = Path(filename)
                    if linker_cwd is not None and not control_path.is_absolute():
                        control_path = Path(linker_cwd) / control_path
                    with control_path.open("rb") as stream:
                        readable = bool(stream.read(1))
                except (OSError, ValueError):
                    readable = False
                if readable:
                    good.add(candidate)
        control_results[package] = sorted(good)
    common = set(control_results["Core"]) & set(control_results["SFXGame"])
    selected = next(iter(common)) if len(common) == 1 else None
    return {"calibrated": selected is not None, "candidate": selected,
            "relative_filename_cwd": str(Path(linker_cwd).resolve()) if linker_cwd is not None else None,
            "controls": control_results, "oracle": "readable Core/SFXGame filename and matching LinkerRoot",
            "reason": None if selected else "filename oracle uncalibrated or ambiguous"}


def net_convention(packages):
    controls = {}
    for name in ("Core", "SFXGame"):
        package = packages[name]
        tables = package["tables"]
        conventions = {table["net_index_convention"] for table in tables if table["valid"]}
        controls[name] = (next(iter(conventions)) if package["metadata_valid"] and tables
                          and all(table["valid"] for table in tables) and len(conventions) == 1 else None)
    values = set(controls.values())
    selected = next(iter(values)) if len(values) == 1 and None not in values else None
    return {"calibrated": selected is not None, "convention": selected, "controls": controls,
            "reason": None if selected else "NetIndex oracle uncalibrated"}


def provenance(package, calibration):
    candidate = calibration["candidate"]
    def chosen(linker):
        linker = linker or {}
        return {"present": linker.get("present", False), "class": linker.get("class"),
                "root": linker.get("root"), "selected_filename": linker.get("candidate_" + candidate) if candidate else None,
                "selected_exists": linker.get("candidate_" + candidate + "_exists") if candidate else None}
    return {"package": chosen(package["linker"]), "child_sampling_is_exhaustive": False,
            "sampled_children": [{**{k: row.get(k) for k in ("path", "class", "net_index", "linker_index")},
                                  "linker": chosen(row.get("linker"))} for row in package["sampled_children"]]}


def compare(host, client, host_linker_cwd=None, client_linker_cwd=None):
    sides = {name: {package: inspect_package(snapshot, package) for package in PACKAGES}
             for name, snapshot in (("host", host), ("client", client))}
    conventions = {name: net_convention(packages) for name, packages in sides.items()}
    linkers = {"host": calibrate_linker(sides["host"], host_linker_cwd),
               "client": calibrate_linker(sides["client"], client_linker_cwd)}
    oracle = (all(item["calibrated"] for item in conventions.values())
              and conventions["host"]["convention"] == conventions["client"]["convention"])
    results = {}
    for name in PACKAGES:
        a, b = sides["host"][name], sides["client"][name]
        tables_a, tables_b = a["tables"], b["tables"]
        comparable = (host["complete"] and client["complete"] and oracle
                      and a["metadata_valid"] and b["metadata_valid"]
                      and all(t["valid"] and t["net_index_convention"] == conventions["host"]["convention"]
                              for t in tables_a + tables_b))
        maps_a = {t["offset"]: t["semantic_rows"] for t in tables_a}
        maps_b = {t["offset"]: t["semantic_rows"] for t in tables_b}
        same_map = maps_a == maps_b if comparable else None
        same_guid = a["guid"] == b["guid"] if a["guid"] and b["guid"] else None
        same_generations = a["generation_counts"] == b["generation_counts"] if a["generation_valid"] and b["generation_valid"] else None
        if not a["present"] or not b["present"]:
            status = "missing-package"
        elif not tables_a or not tables_b:
            status = "missing-table"
        elif not comparable:
            status = "oracle-uncalibrated" if not oracle else "incomplete-or-invalid-evidence"
        elif not same_map:
            status = "different-semantic-map"
        elif not same_generations:
            status = "different-generations"
        elif not same_guid:
            status = "guid-only-mismatch"
        else:
            status = "equivalent-observed-identity"
        differences = []
        if comparable and not same_map:
            keyed_a = {(row["offset"], row["slot"]): row for rows in maps_a.values() for row in rows}
            keyed_b = {(row["offset"], row["slot"]): row for rows in maps_b.values() for row in rows}
            differences = [{"offset": key[0], "slot": key[1], "host": keyed_a.get(key), "client": keyed_b.get(key)}
                           for key in sorted(keyed_a.keys() | keyed_b.keys()) if keyed_a.get(key) != keyed_b.get(key)]
        results[name] = {"status": status, "fully_comparable": bool(comparable),
                         "semantic_map_equal": same_map, "guid_equal": same_guid,
                         "generation_arrays_equal": same_generations, "map_differences": differences,
                         "host": {k: v for k, v in a.items() if k not in ("sampled_children", "linker", "tables")},
                         "client": {k: v for k, v in b.items() if k not in ("sampled_children", "linker", "tables")},
                         "table_evidence": {side: [{k: v for k, v in table.items() if k != "semantic_rows"}
                                                    for table in sides[side][name]["tables"]] for side in sides},
                         "provenance": {side: provenance(sides[side][name], linkers[side]) for side in sides}}
    return {"schema": 1, "campaign_coop_verified": False, "repair_authorized": False,
            "repair_note": "Read-only evidence only; this comparator does not authorize metadata writes or bypasses.",
            "snapshots": {side: {k: v for k, v in snap.items() if k != "rows"}
                          for side, snap in (("host", host), ("client", client))},
            "net_index_oracle_calibrated": oracle, "net_index_calibration": conventions,
            "linker_filename_calibration": linkers, "packages": results,
            "all_tables_and_generations_comparable": all(row["fully_comparable"] for row in results.values()),
            "semantic_hash_policy": "SHA-256 of sorted offset/slot/path/class/net_index/nonnull rows; no process pointers"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("host", type=Path)
    parser.add_argument("client", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--host-linker-cwd", type=Path, help="Actual host game cwd for relative linker filenames")
    parser.add_argument("--client-linker-cwd", type=Path, help="Actual client game cwd for relative linker filenames")
    args = parser.parse_args()
    if args.output.resolve() in (args.host.resolve(), args.client.resolve()):
        parser.error("output must not overwrite an input capture")
    report = compare(select_snapshot(args.host, "host-listening"), select_snapshot(args.client, "client-lookup-failure"),
                     args.host_linker_cwd, args.client_linker_cwd)
    with args.output.open("x", encoding="utf-8") as stream:
        json.dump(report, stream, indent=2, ensure_ascii=True)
        stream.write("\n")


if __name__ == "__main__":
    main()
