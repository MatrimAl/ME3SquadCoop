"""Compile/export a static feature using a retained local Ghidra project.

Never executes the analyzed program. Keep projects, manifests and results in
private workspace storage. Existing projects are processed with -noanalysis.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
import re
from pathlib import Path
import subprocess


def digest(path):
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def save(path, value):
    staged = path.with_suffix(path.suffix + ".partial")
    staged.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")
    staged.replace(path)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for flag in ("source", "manifest", "project-root", "output", "ghidra", "java-home"):
        parser.add_argument("--" + flag, type=Path, required=True)
    parser.add_argument("--prepare-only", action="store_true")
    parser.add_argument("--reuse-from", type=Path,
                        help="Prior successful export directory required before reusing a saved project")
    args = parser.parse_args()
    source = args.source.resolve(strict=True)
    manifest_path = args.manifest.resolve(strict=True)
    ghidra = args.ghidra.resolve(strict=True)
    java = args.java_home.resolve(strict=True)
    root = args.project_root.resolve()
    output = args.output.resolve()
    root.mkdir(parents=True, exist_ok=True)
    output.mkdir(parents=True, exist_ok=True)
    manifest = json.loads(manifest_path.read_text(encoding="utf-8-sig"))
    expected = digest(source)
    if manifest["sha256"] != expected:
        raise RuntimeError("Source digest differs from manifest")
    targets = manifest["functions"]
    if not 1 <= len(targets) <= 128:
        raise ValueError("Select between1 and128 function entries")
    roles = [target["role"] for target in targets]
    if len(set(roles)) != len(roles):
        raise ValueError("Duplicate function role")
    # Validate destinations before native analysis starts or private output can
    # overwrite an artifact. The Java exporter applies the same role grammar.
    if any(not re.fullmatch(r"[a-z][a-z0-9_-]{0,79}", role) for role in roles):
        raise ValueError("Invalid function role")
    for target in targets:
        if not 0 <= int(target["rva"], 0) < 0x100000000:
            raise ValueError("RVA outside bounded image range")
    if (output / "feature.json").exists() or (output / "launcher.log").exists():
        raise RuntimeError("Choose a fresh output directory; retained results must not be overwritten")
    identity_path = root / "source-identity.json"
    identity = {"sha256": expected, "program": source.name}
    project = root / "feature.gpr"
    if identity_path.exists():
        if json.loads(identity_path.read_text()) != identity:
            raise RuntimeError("Retained project belongs to a different source")
    elif project.exists():
        raise RuntimeError("Refusing an unidentified existing project")
    else:
        save(identity_path, identity)
    if project.exists():
        if args.reuse_from is None:
            raise RuntimeError("Existing project requires --reuse-from a verified terminal export")
        previous = args.reuse_from.resolve(strict=True)
        receipt = json.loads((previous / "run-status.json").read_text())
        report = json.loads((previous / "feature.json").read_text())
        prior_rows = report.get("functions", [])
        prior_roles = [row.get("role") for row in prior_rows]
        if not (receipt.get("status") == "export-complete" and receipt.get("exit_code") == 0 and
                receipt.get("project_retained") is True and receipt.get("sha256") == expected and
                report.get("status") == "export-complete" and report.get("sha256") == expected and
                report.get("program") == source.name and prior_rows and
                len(prior_rows) == receipt.get("functions_requested") and
                len(set(prior_roles)) == len(prior_rows) and
                receipt.get("project_root", str(root)) == str(root)):
            raise RuntimeError("Prior export does not establish completed source-matching analysis")
    script = Path(__file__).with_name("ExportFeature.java").resolve()
    classes = root / "compiled-exporter"
    classes.mkdir(exist_ok=True)
    jars = sorted((ghidra / "Ghidra").rglob("*.jar"))
    if not jars:
        raise RuntimeError("Ghidra classpath unavailable")
    classpath = os.pathsep.join(str(path) for path in jars)
    compile_args = root / "javac.args"
    def quote(value):
        return '"' + str(value).replace("\\", "/").replace('"', '\\"') + '"'
    compile_args.write_text("\n".join([
        "-proc:none", "-classpath", quote(classpath), "-d", quote(classes), quote(script)
    ]), encoding="utf-8")
    javac = java / "bin" / ("javac.exe" if os.name == "nt" else "javac")
    subprocess.run([str(javac), "@" + str(compile_args)], check=True)
    launcher = ghidra / "support" / ("analyzeHeadless.bat" if os.name == "nt" else "analyzeHeadless")
    command = [str(launcher), str(root), "feature"]
    mode = "reuse-noanalysis" if project.exists() else "import-and-analyze"
    if project.exists():
        command += ["-process", source.name, "-noanalysis", "-readOnly"]
    else:
        command += ["-import", str(source)]
    command += ["-scriptPath", str(script.parent), "-postScript", script.name,
                str(manifest_path), str(output), "-max-cpu", "2",
                "-log", str(output / "headless.log"), "-scriptlog", str(output / "script.log")]
    status = {"status": "prepared", "mode": mode, "sha256": expected,
              "project_root": str(root),
              "manifest_sha256": digest(manifest_path), "script_sha256": digest(script),
              "functions_requested": len(targets), "game_executed": False, "tests_run": False}
    if args.prepare_only:
        save(output / "run-status.json", status)
        print(json.dumps(status))
        return
    # A process lock prevents two copies of this runner sharing one project.
    with (root / "runner.lock").open("a+b") as lock:
        if os.name == "nt":
            import msvcrt
            lock.seek(0)
            if lock.read(1) == b"":
                lock.write(b"0")
                lock.flush()
            lock.seek(0)
            msvcrt.locking(lock.fileno(), msvcrt.LK_NBLCK, 1)
        else:
            import fcntl
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        env = dict(os.environ, JAVA_HOME=str(java), JAVA_HOME_OVERRIDE=str(java))
        launch = command
        if os.name == "nt":
            # Official Windows launcher is a batch file. Reject shell syntax
            # in its arguments before invoking cmd; paths are never commands.
            if any(any(char in arg for char in '%!&|<>^\r\n') for arg in command):
                raise ValueError("Shell metacharacter in launcher argument")
            launch = ["cmd.exe", "/d", "/s", "/c", subprocess.list2cmdline(command)]
        with (output / "launcher.log").open("wb") as log:
            process = subprocess.Popen(launch, env=env, stdout=log, stderr=subprocess.STDOUT,
                                       creationflags=subprocess.CREATE_NO_WINDOW if os.name == "nt" else 0)
            status.update(status="running", pid=process.pid, started=datetime.now(timezone.utc).isoformat())
            save(output / "run-status.json", status)
            code = process.wait()
        status.update(exit_code=code, finished=datetime.now(timezone.utc).isoformat())
        report_path = output / "feature.json"
        try:
            report = json.loads(report_path.read_text()) if report_path.exists() else {}
        except (ValueError, UnicodeError):
            report = {}
        rows = report.get("functions", [])
        valid = (code == 0 and digest(source) == expected and report.get("sha256") == expected and
                 report.get("status") == "export-complete" and
                 [row.get("role") for row in rows] == roles and
                 [row.get("requested_rva") for row in rows] ==
                 [hex(int(target["rva"], 0)) for target in targets])
        status.update(status="export-complete" if valid else "failed-or-incomplete",
                      functions_decompiled=sum(row.get("status") == "decompiled" for row in rows),
                      project_retained=project.exists())
        save(output / "run-status.json", status)
        print(json.dumps(status))
        if not valid:
            raise SystemExit(1)


if __name__ == "__main__":
    main()
