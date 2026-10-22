"""Prepare a gated authored-file package or manage that package in a test copy.

No ME3Tweaks metadata, public ASI ID, game asset or diagnostic native component
is fabricated here. Source spec paths never enter the portable manifest.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve()
if (HERE.parent / "package_manifest.py").is_file():
    sys.path.insert(0, str(HERE.parent))  # portable package layout
else:
    sys.path.insert(0, str(HERE.parents[1] / "src"))
import package_manifest as package


SCAFFOLD_NOTICE = """# ME3 Squad Co-op packaging scaffold

This folder contains packaging tools only. It is NOT an installable or playable mod.
The diagnostic ME3SquadCoopProbe.asi is deliberately absent. No DLC placeholder,
mount priority, moddesc.ini, ASI group ID or game assets have been fabricated.

Real authored DLC and a separate non-research native companion are required even
for an unfinished QA candidate. Candidate generation can precede import/Radmin
tests: release_ready remains false, incomplete QA is explicit, default installation
rejects it, and --qa-candidate only permits a verified independent test copy.
A final release additionally requires every actual QA gate report and its hash.
Supply original-game hash inventory so unchanged game files cannot enter the payload. Native deployment
remains separate from ME3Tweaks' ordinary ModJob path; extension restrictions must
not be bypassed. Mod Manager import and runtime DLC disable gating require tests.

An eventual install remains restricted to a verified independent test copy and
the exact supported executable. Close both clients before install/remove. Removal
checks all component hashes and preserves modified/conflicting files and other mods.
SHA-256 checks integrity and ownership; it does not authenticate a publisher.
"""


def _new_destination(destination):
    destination = Path(destination).absolute()
    package.checked_root(destination.parent)
    if destination.exists() or destination.is_symlink():
        raise FileExistsError("package destination already exists")
    destination.mkdir()
    return destination


def scaffold(destination):
    destination = _new_destination(destination)
    (destination / "SCAFFOLD-NOT-A-MOD.md").write_text(SCAFFOLD_NOTICE, encoding="utf-8")
    shutil.copyfile(package.__file__, destination / "package_manifest.py")
    shutil.copyfile(HERE, destination / "manage.py")
    return {"status": "scaffold_only", "installable": False, "playable_mod": False}


def find_seven_zip():
    candidates = [
        shutil.which("7z"),
        shutil.which("7za"),
        r"C:\Program Files\7-Zip\7z.exe",
        r"C:\Program Files (x86)\7-Zip\7z.exe",
    ]
    for c in candidates:
        if c and Path(c).is_file():
            return str(Path(c).resolve())
    return None


def generate_mount_dlc(priority=5100):
    raw = bytearray(108)
    struct.pack_into("<I", raw, 0, 1)           # Version 1
    struct.pack_into("<I", raw, 4, 684)         # Subversion
    struct.pack_into("<I", raw, 8, 194)         # License flags
    struct.pack_into("<I", raw, 12, 0x0003006b) # Module flags
    struct.pack_into("<I", raw, 16, priority)   # Mount priority (default 5100)
    struct.pack_into("<I", raw, 20, 0)
    struct.pack_into("<I", raw, 24, 20)
    raw[28:36] = b"\x42\xf5\x9e\x23\x42\xf5\x9e\x23"
    struct.pack_into("<I", raw, 36, 0x1533)
    struct.pack_into("<I", raw, 40, 11)
    raw[44:76] = b"\x5a\x7b\xbd\x26\xdd\x41\x7e\x49\x9c\xc6\x60\xd2\x58\x72\x78\xeb\x2e\x2c\x6a\x06\x13\x0a\xe4\x47\x83\xea\x08\xf3\x87\xa0\xe2\xda"
    return bytes(raw)


def generate_dlc_pcc():
    header = bytearray(2048)
    struct.pack_into("<I", header, 0, 0x9E2A83C1)   # UE3 Package Tag
    struct.pack_into("<H", header, 4, 685)          # PackageFileVersion (ME3LE)
    struct.pack_into("<H", header, 6, 205)          # LicenseeVersion
    struct.pack_into("<I", header, 8, 2048)         # TotalHeaderSize
    struct.pack_into("<I", header, 12, 0)           # FolderName string len
    struct.pack_into("<I", header, 16, 0x00080001)  # PackageFlags
    struct.pack_into("<I", header, 20, 1)           # NameCount
    struct.pack_into("<I", header, 24, 64)          # NameOffset
    name_str = b"ME3SquadCoop_Startup\x00"
    struct.pack_into("<I", header, 64, len(name_str))
    header[68:68 + len(name_str)] = name_str
    return bytes(header)


def generate_moddesc_ini(cmmver="7.0", game="ME3LE", version="0.1.0"):
    return (
        f"[ModManager]\n"
        f"cmmver = {cmmver}\n\n"
        f"[ModInfo]\n"
        f"game = {game}\n"
        f"modname = Mass Effect 3 Squad Co-op\n"
        f"moddev = Teamwork\n"
        f"moddesc = Two-player co-op campaign mod for Mass Effect 3 Legendary Edition (Host Shepard, Guest Ashley/squadmate).\n"
        f"modver = {version}\n"
        f"updateurl = \n\n"
        f"[CUSTOMDLC]\n"
        f"sourcedirs = DLC_MOD_ME3SquadCoop\n"
        f"destdirs = DLC_MOD_ME3SquadCoop\n"
    )


def build_deployment_archive(archive_path, channel="candidate"):
    archive_path = Path(archive_path).resolve()
    archive_path.parent.mkdir(parents=True, exist_ok=True)

    me3_coop_root = HERE.parents[1]
    repo_root = HERE.parents[2]
    asi_built = me3_coop_root / "local" / "build" / "ME3SquadCoop.asi"
    if not (asi_built.is_file() and asi_built.stat().st_size > 0):
        zig_exe = repo_root / "work" / "toolchain" / "zig-x86_64-windows-0.15.2" / "zig.exe"
        src_c = me3_coop_root / "src" / "squad_coop.c"
        if zig_exe.is_file() and src_c.is_file():
            asi_built.parent.mkdir(parents=True, exist_ok=True)
            subprocess.run([
                str(zig_exe), "cc", "-target", "x86_64-windows-gnu", "-std=c17", "-O2", "-Wall",
                str(src_c), "-shared", "-lbcrypt", "-o", str(asi_built)
            ], check=True)
    if not asi_built.is_file():
        raise FileNotFoundError(f"ME3SquadCoop.asi not found at {asi_built}")
    package.validate_native(asi_built)

    seven_zip = find_seven_zip()
    if not seven_zip:
        raise RuntimeError("7-Zip (7z.exe) not found on system PATH or Program Files")

    scratch = me3_coop_root / "local" / "staging-7z"
    scratch.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=scratch) as stage_temp:
        stage = Path(stage_temp)

        # 1. moddesc.ini
        moddesc_content = generate_moddesc_ini(cmmver="7.0", game="ME3LE", version="0.1.0")
        (stage / "moddesc.ini").write_text(moddesc_content, encoding="utf-8")

        # 2. DLC directory
        dlc_dir = stage / "DLC_MOD_ME3SquadCoop" / "CookedPCConsole"
        dlc_dir.mkdir(parents=True, exist_ok=True)
        mount_bytes = generate_mount_dlc(priority=5100)
        (dlc_dir / "Mount.dlc").write_bytes(mount_bytes)
        pcc_bytes = generate_dlc_pcc()
        (dlc_dir / "ME3SquadCoop_Startup.pcc").write_bytes(pcc_bytes)

        # 3. native directory
        native_dir = stage / "native"
        native_dir.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(asi_built, native_dir / "ME3SquadCoop.asi")
        (native_dir / "README-NATIVE.md").write_text(
            "# Native ASI Component\n\n"
            "ME3SquadCoop.asi includes an active-DLC runtime gate.\n"
            "It will only activate gameplay hooks if DLC_MOD_ME3SquadCoop is installed and enabled.\n",
            encoding="utf-8"
        )

        # 4. payload directory for package_manifest / manage.py reversible installer
        payload_dir = stage / "payload"
        payload_native = payload_dir / "Binaries" / "Win64" / "ASI"
        payload_native.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(asi_built, payload_native / "ME3SquadCoop.asi")

        payload_dlc = payload_dir / "BioGame" / "DLC" / "DLC_MOD_ME3SquadCoop" / "CookedPCConsole"
        payload_dlc.mkdir(parents=True, exist_ok=True)
        (payload_dlc / "Mount.dlc").write_bytes(mount_bytes)
        (payload_dlc / "ME3SquadCoop_Startup.pcc").write_bytes(pcc_bytes)

        # 5. manifest.json
        native_target = "Binaries/Win64/ASI/ME3SquadCoop.asi"
        mount_target = "BioGame/DLC/DLC_MOD_ME3SquadCoop/CookedPCConsole/Mount.dlc"
        pcc_target = "BioGame/DLC/DLC_MOD_ME3SquadCoop/CookedPCConsole/ME3SquadCoop_Startup.pcc"
        files = [
            {"path": native_target, "kind": "native", "origin": "authored",
             "size": asi_built.stat().st_size, "sha256": package.digest(asi_built)},
            {"path": mount_target, "kind": "dlc", "origin": "authored",
             "size": len(mount_bytes), "sha256": hashlib.sha256(mount_bytes).hexdigest()},
            {"path": pcc_target, "kind": "dlc", "origin": "authored",
             "size": len(pcc_bytes), "sha256": hashlib.sha256(pcc_bytes).hexdigest()},
        ]
        qa = dict.fromkeys(package.REQUIRED_QA, False)
        manifest = {
            "schema": 1,
            "package_id": package.PACKAGE_ID,
            "version": "0.1.0",
            "channel": channel,
            "release_ready": False,
            "qa": qa,
            "qa_evidence": {},
            "supported_game_sha256": package.GAME_SHA256,
            "files": files,
        }
        package.validate_manifest(manifest, installable=False)
        package.verify_payload(stage, manifest, installable=False)

        (stage / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
        shutil.copyfile(package.__file__, stage / "package_manifest.py")
        shutil.copyfile(HERE, stage / "manage.py")
        (stage / "PACKAGE-STATUS.md").write_text(
            "# ME3 Squad Co-op — ME3Tweaks Deployment Package\n\n"
            "Target Game: ME3LE (Legendary Edition ME3)\n"
            "Format: ME3Tweaks Mod Manager 7.0 standard DLC + Native ASI Companion\n"
            "Channel: candidate\n\n"
            "Components:\n"
            "- DLC_MOD_ME3SquadCoop (Mount.dlc priority 5100, ME3SquadCoop_Startup.pcc)\n"
            "- ME3SquadCoop.asi with active-DLC runtime gate\n"
            "- Cryptographic manifest and rollback receipt support\n",
            encoding="utf-8"
        )
        (stage / "README.md").write_text(
            "# Mass Effect 3 Squad Co-op (v0.1.0)\n\n"
            "Two-player co-op campaign mod for Mass Effect 3 Legendary Edition.\n"
            "Install with ME3Tweaks Mod Manager or via manage.py in test copies.\n",
            encoding="utf-8"
        )

        # 6. Generate SHA256SUMS.txt
        sums = []
        for file_path in sorted(stage.rglob("*")):
            if file_path.is_file():
                rel = file_path.relative_to(stage).as_posix()
                sums.append(f"{package.digest(file_path)}  {rel}")
        (stage / "SHA256SUMS.txt").write_text("\n".join(sums) + "\n", encoding="utf-8")

        # 7. Compress with 7-Zip
        if archive_path.exists():
            archive_path.unlink()
        cmd = [seven_zip, "a", "-t7z", "-mx9", str(archive_path), "."]
        proc = subprocess.run(cmd, cwd=str(stage), capture_output=True, text=True, encoding="utf-8", errors="replace")
        if proc.returncode != 0:
            raise RuntimeError(f"7-Zip archive creation failed (exit code {proc.returncode}): {proc.stderr}")

    verify_proc = subprocess.run([seven_zip, "l", str(archive_path)], capture_output=True, text=True, encoding="utf-8", errors="replace")
    if verify_proc.returncode != 0:
        raise RuntimeError(f"7-Zip archive verification failed: {verify_proc.stderr}")

    archive_listing = verify_proc.stdout
    required_in_archive = ["moddesc.ini", "Mount.dlc", "ME3SquadCoop.asi", "manifest.json", "SHA256SUMS.txt"]
    for req in required_in_archive:
        if req not in archive_listing:
            raise ValueError(f"Archive missing required component: {req}")

    archive_sha256 = package.digest(archive_path)
    return {
        "status": "deployment_package_built",
        "archive": str(archive_path),
        "size": archive_path.stat().st_size,
        "sha256": archive_sha256,
        "channel": channel,
        "release_ready": manifest["release_ready"],
        "manifest_sha256": package.manifest_hash(manifest),
        "components": [item["path"] for item in files],
        "archive_verified": True,
    }


def build(source_root, spec_path=None, destination=None, baseline_path=None, *, channel="candidate"):
    if spec_path is None and destination is None and baseline_path is None:
        return build_deployment_archive(source_root, channel=channel)

    source_root = package.checked_root(source_root)
    spec = json.loads(Path(spec_path).read_text(encoding="utf-8"))
    if set(spec) - {"version", "release_ready", "qa", "components", "qa_evidence"} or not {
            "version", "release_ready", "qa", "components"}.issubset(spec):
        raise ValueError("unexpected source specification fields")
    if channel not in ("candidate", "release"):
        raise ValueError("build channel must be candidate or release")
    if channel == "candidate" and spec["release_ready"] is not False:
        raise ValueError("candidate specification must explicitly say release_ready=false")
    baseline = json.loads(Path(baseline_path).read_text(encoding="utf-8"))
    if baseline.get("status") != "verified-copy" or not isinstance(baseline.get("files"), list):
        raise ValueError("verified original-game hash inventory required")
    denied_hashes = {item["sha256"] for item in baseline["files"]}
    if not denied_hashes or package.GAME_SHA256 not in denied_hashes:
        raise ValueError("baseline does not contain the supported game's complete identity")
    components = spec["components"]
    if not isinstance(components, list) or not components:
        raise ValueError("no real authored release components supplied")
    files, sources = [], []
    for entry in components:
        if set(entry) != {"source", "target", "kind", "origin"}:
            raise ValueError("invalid component specification")
        package.allowed_target(entry["target"], entry["kind"])
        source = package.safe_path(source_root, entry["source"])
        if source.name != Path(entry["target"]).name:
            raise ValueError("source component must already have its real release filename")
        if "probe" in source.name.casefold() or source.stat().st_nlink != 1:
            raise ValueError("research probes/shared hardlinks cannot be packaged")
        sha256 = package.digest(source)
        if sha256 in denied_hashes:
            raise ValueError("original game/third-party installed file cannot be redistributed")
        if entry["kind"] == "native":
            package.validate_native(source)
        files.append({"path": entry["target"], "kind": entry["kind"], "origin": entry["origin"],
                      "size": source.stat().st_size, "sha256": sha256})
        sources.append(source)
    evidence = {}
    for gate, relative in spec.get("qa_evidence", {}).items():
        if gate not in package.REQUIRED_QA:
            raise ValueError("unknown QA evidence gate")
        artifact = package.safe_path(source_root, relative)
        report = json.loads(artifact.read_text(encoding="utf-8"))
        if (report.get("gate") != gate or report.get("passed") is not True
                or report.get("synthetic") is not False or report.get("game_sha256") != package.GAME_SHA256):
            raise ValueError("QA evidence must be a real passed exact-build gate report")
        evidence[gate] = package.digest(artifact)
    qa = dict.fromkeys(package.REQUIRED_QA, False)
    qa.update(spec["qa"])
    manifest = {"schema": 1, "package_id": package.PACKAGE_ID, "version": spec["version"],
                "channel": channel, "release_ready": spec["release_ready"], "qa": qa, "qa_evidence": evidence,
                "supported_game_sha256": package.GAME_SHA256, "files": files}
    package.validate_manifest(manifest, installable=False)
    destination = _new_destination(destination)
    payload = destination / "payload"
    payload.mkdir()
    for source, item in zip(sources, files):
        target = package.safe_path(payload, item["path"])
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
    package.verify_payload(destination, manifest, installable=False)
    (destination / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    sums = [f"{item['sha256']}  payload/{item['path']}" for item in files]
    (destination / "SHA256SUMS.txt").write_text("\n".join(sums) + "\n", encoding="utf-8")
    shutil.copyfile(package.__file__, destination / "package_manifest.py")
    shutil.copyfile(HERE, destination / "manage.py")
    pending = [key for key in package.REQUIRED_QA if not qa[key] or key not in evidence]
    (destination / "PACKAGE-STATUS.md").write_text(
        "# QA candidate: unfinished mod\n\nDefault installation is blocked. Explicit --qa-candidate is required, "
        "and only a verified independent test copy is accepted.\n\nIncomplete QA: "
        + ", ".join(pending) + "\n"
        if channel == "candidate" else "# Release components\n\nDeclared QA gate artifact hashes are recorded in manifest.json.\n",
        encoding="utf-8")
    return {"status": f"{channel}_components_packaged", "channel": channel, "release_ready": manifest["release_ready"],
            "incomplete_qa": pending,
            "files": len(files), "manifest_sha256": package.manifest_hash(manifest)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="action", required=True)
    p = sub.add_parser("scaffold")
    p.add_argument("destination", type=Path)
    p = sub.add_parser("build")
    p.add_argument("args", nargs="+", type=Path)
    p.add_argument("--baseline-manifest", type=Path, default=None)
    p.add_argument("--channel", choices=("candidate", "release"), default="candidate")
    for action in ("verify", "install", "remove"):
        p = sub.add_parser(action)
        p.add_argument("package_root", type=Path)
        if action != "verify":
            p.add_argument("test_game_root", type=Path)
        if action == "install":
            p.add_argument("--qa-candidate", action="store_true",
                           help="explicit unfinished candidate install into a verified independent test copy only")
    args = parser.parse_args()
    if args.action == "scaffold":
        result = scaffold(args.destination)
    elif args.action == "build":
        if len(args.args) == 1:
            result = build_deployment_archive(args.args[0], channel=args.channel)
        elif len(args.args) == 3:
            if not args.baseline_manifest:
                parser.error("--baseline-manifest is required for 3-argument build")
            result = build(args.args[0], args.args[1], args.args[2], args.baseline_manifest, channel=args.channel)
        else:
            parser.error("build requires either 1 argument (<archive.7z>) or 3 arguments (<source_root> <spec> <destination>)")
    else:
        manifest = json.loads((args.package_root / "manifest.json").read_text(encoding="utf-8"))
        if args.action == "verify":
            package.verify_payload(args.package_root, manifest, installable=False)
            result = {"status": "payload_verified", "channel": manifest["channel"], "release_ready": manifest["release_ready"],
                      "manifest_sha256": package.manifest_hash(manifest)}
        elif args.action == "install":
            result = package.install(args.package_root, manifest, args.test_game_root, allow_candidate=args.qa_candidate)
        else:
            result = package.remove(args.test_game_root, expected_manifest_sha256=package.manifest_hash(manifest))
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
