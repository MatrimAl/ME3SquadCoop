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


def verified_activation_files(receipt_path, legendary_explorer, starter_config_template, *, powershell="pwsh"):
    """Use the existing semantic verifier; retain no private paths in bundles."""
    if not receipt_path or not legendary_explorer or not starter_config_template:
        raise ValueError("current activation requires explicit receipt, LegendaryExplorer and starter template")
    receipt_path = Path(receipt_path).resolve(strict=True)
    package.checked_root(receipt_path.parent)
    receipt_hash = package.digest(receipt_path)
    receipt = json.loads(receipt_path.read_text(encoding="utf-8-sig"))
    if receipt.get("Variant") != "mount-toc-starter-config-english-tlk":
        raise ValueError("the current authored starter-config activation is required")
    verifier = HERE.parent / "verify_activation_dlc.ps1"
    command = [str(powershell), "-NoProfile", "-File", str(verifier), "-Receipt", str(receipt_path),
               "-LegendaryExplorerPath", str(Path(legendary_explorer).resolve(strict=True)),
               "-StarterConfigTemplate", str(Path(starter_config_template).resolve(strict=True))]
    process = subprocess.run(command, capture_output=True, text=True, encoding="utf-8", errors="strict", timeout=120)
    if process.returncode:
        raise ValueError("activation semantic verification failed; no archive created")
    try:
        result = json.loads(process.stdout.lstrip("\ufeff"))
    except ValueError as error:
        raise ValueError("activation verifier did not return a complete receipt") from error
    required = ("MountContractVerified", "TOCVerified", "TOCHashBucketsVerified", "ConfigVerified",
                "EnglishMetadataVerified", "StarterConfigVerified")
    if (result.get("Status") != "offline-verified-not-installed" or result.get("Files") != 4 or
            result.get("Variant") != receipt["Variant"] or any(result.get(key) is not True for key in required) or
            result.get("ReceiptSHA256", "").lower() != receipt_hash or package.digest(receipt_path) != receipt_hash):
        raise ValueError("activation semantic verification receipt mismatch")
    root = package.safe_path(receipt_path.parent, "DLC_MOD_ME3SquadCoop")
    files = []
    for relative in package.ACTIVATION_FILES:
        matches = [row for row in receipt["Files"] if row["RelativePath"].replace("\\", "/") == relative]
        source = package.safe_path(root, relative)
        if (len(matches) != 1 or source.stat().st_nlink != 1 or source.stat().st_size != matches[0]["Bytes"] or
                package.digest(source) != matches[0]["SHA256"].lower()):
            raise ValueError("activation payload changed after semantic verification")
        raw = source.read_bytes()
        if len(raw) != matches[0]["Bytes"] or hashlib.sha256(raw).hexdigest() != matches[0]["SHA256"].lower():
            raise ValueError("activation bytes changed while staging")
        files.append((relative, raw))
    package.verify_activation_toc(root)
    return files


def build_deployment_archive(archive_path, channel="candidate", *, activation_receipt=None,
        legendary_explorer=None, starter_config_template=None, powershell="pwsh"):
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
    activation = verified_activation_files(activation_receipt, legendary_explorer, starter_config_template, powershell=powershell)
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
        dlc_dir = manager / "DLC_MOD_ME3SquadCoop"
        for relative, raw in activation:
            destination = dlc_dir / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes(raw)
        mount_bytes = dict(activation)["CookedPCConsole/Mount.dlc"]
        mount_readback = package.read_activation_mount(mount_bytes, string_id=package.ACTIVATION_STRING_ID)
        (manager / "moddesc.ini").write_text(generate_moddesc_ini(), encoding="utf-8")
        manager_archive = "ME3SquadCoop-DLC.7z"
        archive(manager, stage / manager_archive)

        files = []
        payload = stage / "payload"
        payload.mkdir()
        for target, kind, raw in (
                ("Binaries/Win64/ASI/ME3SquadCoop.asi", "native", asi_built.read_bytes()),
                *((package.ACTIVATION_PREFIX + relative, "dlc", raw) for relative, raw in activation)):
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
            "The activation includes only project-authored Mount, named starter config, English metadata TLK and root TOC. "
            "No PCC, save, game asset, loader, decompiled source or private receipt is included. "
            "Base-game TOC setup is a separate prerequisite; installation never updates or restores an existing base TOC. "
            "Priority 5100 is an unregistered research value, not a conflict-free allocation.\n",
            encoding="utf-8")
        (stage / "README.md").write_text(
            "# ME3 Squad Co-op: unfinished research candidate\n\n"
            "This is not a validated player release. Keep testing inside an independently verified game copy.\n\n"
            "The separate ME3SquadCoop-DLC.7z contains a Mod Manager descriptor and the complete authored activation DLC. "
            "Import that inner archive for the DLC job; this does not install the ASI companion. "
            "Actual Mod Manager acceptance and engine behavior remain untested.\n\n"
            "The native companion is under payload/Binaries/Win64/ASI. Mod Manager's ordinary DLC job rejects ASIs. "
            "The portable manage.py supports verification and explicit --qa-candidate installation into verified test copies only. "
            "Keep its manifest hash for hash-checked removal. Installation requires the game to be closed. "
            "Before copying any components, installation requires the existing BioGame/PCConsoleTOC.bin to already index the three authored DLC content paths. "
            "Run `python manage.py prepare-base-toc <package-root> <baseline-PCConsoleTOC.bin> <new-output.bin>` to create a separate derived artifact for review; this command never installs it or changes the baseline. "
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
    p.add_argument("--activation-receipt", type=Path, help="explicit current authored activation preparation receipt")
    p.add_argument("--legendary-explorer", type=Path, help="LegendaryExplorer folder for semantic verification")
    p.add_argument("--starter-config-template", type=Path, help="explicit pinned upstream authored starter template")
    p.add_argument("--powershell", default="pwsh", help="PowerShell 7 executable; used only while building")
    p = sub.add_parser("prepare-base-toc", help="create a separate derived TOC artifact; never changes the baseline/game")
    p.add_argument("package_root", type=Path)
    p.add_argument("baseline_toc", type=Path)
    p.add_argument("output", type=Path, help="new output file; existing destinations are refused")
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
            result = build_deployment_archive(args.args[0], channel=args.channel, activation_receipt=args.activation_receipt,
                legendary_explorer=args.legendary_explorer, starter_config_template=args.starter_config_template,
                powershell=args.powershell)
        elif len(args.args) == 3:
            if any((args.activation_receipt, args.legendary_explorer, args.starter_config_template)):
                parser.error("activation options apply only to the one-argument archive build")
            if not args.baseline_manifest:
                parser.error("--baseline-manifest is required for 3-argument build")
            result = build(args.args[0], args.args[1], args.args[2], args.baseline_manifest, channel=args.channel)
        else:
            parser.error("build requires either 1 argument (<archive.7z>) or 3 arguments (<source_root> <spec> <destination>)")
    elif args.action == "prepare-base-toc":
        manifest = json.loads((args.package_root / "manifest.json").read_text(encoding="utf-8"))
        result = package.prepare_base_toc(args.package_root, manifest, args.baseline_toc, args.output)
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
