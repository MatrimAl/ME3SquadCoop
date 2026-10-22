"""Prepare a gated authored-file package or manage that package in a test copy.

No ME3Tweaks metadata, public ASI ID, game asset or diagnostic native component
is fabricated here. Source spec paths never enter the portable manifest.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
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
    return package.make_activation_mount(priority)


def generate_moddesc_ini(cmmver="7.0", game="LE3", version="0.1.0"):
    if cmmver != "7.0" or game != "LE3" or not isinstance(version, str) or not package.VERSION.fullmatch(version):
        raise ValueError("activation descriptor requires cmmver=7.0, game=LE3 and a valid version")
    return (
        f"[ModManager]\n"
        f"cmmver = {cmmver}\n\n"
        f"[ModInfo]\n"
        f"game = {game}\n"
        f"modname = Mass Effect 3 Squad Co-op\n"
        f"moddev = ME3SquadCoop contributors\n"
        f"moddesc = Unfinished native co-op research activation DLC. Gameplay, import and disable testing remain incomplete. The ASI companion is installed separately.\n"
        f"modver = {version}\n"
        f"modsite = https://github.com/MatrimAl/ME3SquadCoop\n\n"
        f"[CUSTOMDLC]\n"
        f"sourcedirs = DLC_MOD_ME3SquadCoop\n"
        f"destdirs = DLC_MOD_ME3SquadCoop\n"
    )


def build_deployment_archive(archive_path, channel="candidate"):
    """Bundle a separate Mod Manager DLC archive and an unfinished native payload.

    The inner DLC archive contains no ASI, executable, PCC or copied assets.
    Neither this builder nor our format readback is a Mod Manager/engine test.
    """
    if channel != "candidate":
        raise ValueError("automatic archive generation is candidate-only; real release QA is required")
    archive_path = Path(archive_path).absolute()
    package.checked_root(archive_path.parent)
    if archive_path.suffix.lower() != ".7z":
        raise ValueError("deployment archive must have a .7z extension")
    if archive_path.exists() or archive_path.is_symlink():
        raise FileExistsError("archive destination already exists")
    me3_coop_root = HERE.parents[1]
    # Never silently compile a different entry or package a stale native DLL.
    from build_state import verify_current
    verify_current(me3_coop_root)
    asi_built = package.safe_path(me3_coop_root, "local/build/ME3SquadCoop.asi")
    if asi_built.stat().st_nlink != 1:
        raise ValueError("shared hardlinked native binaries cannot be packaged")
    package.validate_native(asi_built)
    seven_zip = find_seven_zip()
    if not seven_zip:
        raise RuntimeError("7-Zip (7z.exe) not found on system PATH or Program Files")

    def archive(source, destination):
        command = [seven_zip, "a", "-t7z", "-mx9", str(destination), "."]
        result = subprocess.run(command, cwd=source, capture_output=True, text=True,
                                encoding="utf-8", errors="replace", timeout=120)
        if result.returncode:
            raise RuntimeError(f"7-Zip creation failed (exit {result.returncode})")
        result = subprocess.run([seven_zip, "t", str(destination)], capture_output=True,
                                text=True, encoding="utf-8", errors="replace", timeout=120)
        if result.returncode:
            raise RuntimeError(f"7-Zip integrity test failed (exit {result.returncode})")

    scratch = me3_coop_root / "local/staging-7z"
    scratch.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=scratch) as stage_temp:
        stage_root = Path(stage_temp)
        stage = stage_root / "bundle"
        stage.mkdir()
        manager = stage_root / "mod-manager"
        dlc_dir = manager / "DLC_MOD_ME3SquadCoop/CookedPCConsole"
        dlc_dir.mkdir(parents=True)
        mount_bytes = generate_mount_dlc()
        mount_readback = package.read_activation_mount(mount_bytes)
        (dlc_dir / "Mount.dlc").write_bytes(mount_bytes)
        (manager / "moddesc.ini").write_text(generate_moddesc_ini(), encoding="utf-8")
        manager_archive = "ME3SquadCoop-DLC.7z"
        archive(manager, stage / manager_archive)

        files = []
        payload = stage / "payload"
        payload.mkdir()
        for target, kind, raw in (
                ("Binaries/Win64/ASI/ME3SquadCoop.asi", "native", asi_built.read_bytes()),
                (package.MOUNT_TARGET, "dlc", mount_bytes)):
            path = package.safe_path(payload, target)
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(raw)
            files.append({"path": target, "kind": kind, "origin": "authored",
                          "size": len(raw), "sha256": hashlib.sha256(raw).hexdigest()})
        manifest = {"schema": 1, "package_id": package.PACKAGE_ID, "version": "0.1.0",
                    "channel": "candidate", "release_ready": False,
                    "qa": dict.fromkeys(package.REQUIRED_QA, False), "qa_evidence": {},
                    "supported_game_sha256": package.GAME_SHA256, "files": files}
        package.verify_payload(stage, manifest, installable=False)
        (stage / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
        shutil.copyfile(package.__file__, stage / "package_manifest.py")
        shutil.copyfile(HERE, stage / "manage.py")
        (stage / "PACKAGE-STATUS.md").write_text(
            "# Unfinished native co-op QA candidate\n\n"
            "All gameplay, Radmin, Mod Manager import, engine DLC loading, and runtime disable gates remain unverified. "
            "The mount metadata uses the reviewed LE3 layout; it is not engine validation. "
            "There is no startup PCC, config override, TLK content, or game asset. "
            "Priority 5100 is an unregistered research value, not a conflict-free allocation.\n",
            encoding="utf-8")
        (stage / "README.md").write_text(
            "# ME3 Squad Co-op: unfinished research candidate\n\n"
            "This is not a validated player release. Keep testing inside an independently verified game copy.\n\n"
            "The separate ME3SquadCoop-DLC.7z contains only a Mod Manager descriptor and native-activation Mount.dlc. "
            "Import that inner archive for the DLC job; this does not install the ASI companion. "
            "Actual Mod Manager acceptance and engine behavior remain untested.\n\n"
            "The native companion is under payload/Binaries/Win64/ASI. Mod Manager's ordinary DLC job rejects ASIs. "
            "The portable manage.py supports verification and explicit --qa-candidate installation into verified test copies only. "
            "Keep its manifest hash for hash-checked removal. Installation requires the game to be closed. "
            "Do not mix managed/manual installation methods; the receipt only owns files it installed. "
            "Removing or disabling the DLC is intended to close the native presence gate, with cleanup on the next game tick; "
            "that lifecycle still needs protected live verification. A compatible ASI loader is a separate prerequisite.\n",
            encoding="utf-8")
        sums = [f"{package.digest(path)}  {path.relative_to(stage).as_posix()}"
                for path in sorted(stage.rglob("*")) if path.is_file()]
        (stage / "SHA256SUMS.txt").write_text("\n".join(sums) + "\n", encoding="utf-8")
        archive(stage, archive_path)
    return {"status": "candidate_archive_built", "archive": str(archive_path),
            "size": archive_path.stat().st_size, "sha256": package.digest(archive_path),
            "channel": "candidate", "release_ready": False,
            "manifest_sha256": package.manifest_hash(manifest),
            "components": [item["path"] for item in files], "mount_readback": mount_readback,
            "mod_manager_archive": manager_archive, "archive_verified": True,
            "mod_manager_import_verified": False, "engine_dlc_load_verified": False,
            "native_disable_verified": False}


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
