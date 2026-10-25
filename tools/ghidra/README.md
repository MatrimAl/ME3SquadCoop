# Retained offline feature analysis

`feature_analysis.py` uses an existing Ghidra installation and JDK. It imports
an owner-selected executable into private project storage, exports selected
functions and keeps the project. Subsequent invocations use `-process`,
`-noanalysis` and `-readOnly`. The source executable is never run or changed.

Supply a private JSON manifest containing the source's full `sha256` and a
`functions` array of objects with unique `role` names and hexadecimal `rva`
strings. Entries must identify exact analyzed function starts. Missing entries
are reported; the exporter does not create functions or silently substitute a
containing function. Keep manifests, databases and game-derived output outside
Git. Nothing is downloaded.

```powershell
python tools/ghidra/feature_analysis.py --source $sourcePath `
  --manifest $manifestPath --project-root $privateProject `
  --output $freshOutput --ghidra $ghidraPath --java-home $jdkPath
```

`--prepare-only` verifies input identity and manifest and compiles the exporter
against the supplied installation without starting headless analysis. It
creates private identity/compiler metadata. That prepared output path can be
used for the first actual run. After an actual launch, choose a fresh output
directory to prevent old results being confused with a new operation.

A runner process lock and Ghidra's project lock prevent concurrent project
use. Do not remove locks or start another import after an observation timeout.
`run-status.json` records launcher PID, terminal exit code, target/manifest
identities and decompiled count. A saved running status alone does not prove
the process is live: observe the same command handle or actual process.

Import/analysis has no runner deadline. Per-function decompilation uses zero
timeout; the operator can cancel the owned command. Each completed entry is
saved separately and the aggregate report updated. Earlier entries survive a
later failure. `export-complete` means the manifest was processed, not that
every function decompiled; inspect each status and `functions_decompiled`.

Results include analyzed body ranges, no-return annotations, call instructions,
unresolved targets, diagnostics and available pseudocode. A `.gpr` file alone
does not prove successful initial analysis. Reuse only after reviewing terminal
status and export/save logs. Diagnose interrupted imports before reusing them
with `-noanalysis`. The exporter does not repair no-return assumptions or prove
complete control flow, ABI safety, rendering, casting or campaign behavior.

Saved-project reuse requires `--reuse-from` pointing to a prior successful
export directory. The runner checks terminal exit0, completed report,
retained-project flag, requested entry count, unique roles and matching source
identity before allowing `-noanalysis`. This refuses incomplete/failed import
state instead of treating a `.gpr` filename as a readiness receipt. Keep the
prior export with its project. On first receipt files produced before the
project-root field was introduced, source/report identity is still required;
later receipts must match the recorded project root as well.
