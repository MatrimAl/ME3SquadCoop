# Packaging and reversible installation

This is packaging research. There is no verified playable campaign package or verified ME3Tweaks import/disable path yet. The native `squad_coop.c` companion is a **DLC-presence gate scaffold**: it exports gate diagnostics but installs no campaign or network hooks. The gameplay research runtime is separate.

## Generated archive: a non-installable fixture

```powershell
python tools/build_shareable.py build outputs/ME3SquadCoop-format-fixture.7z
```

This compiles `squad_coop.c` afresh with Zig 0.15.2 into `local/packaging-build/`, separately from `build.py`'s fingerprinted research outputs. The tool locates the compiler through `ME3_SQUAD_COOP_ZIG`, the verified GitHub Actions `RUNNER_TEMP` toolchain, the sibling workspace toolchain, or `PATH`. Its source/compiler/artifact hashes are recorded in `native/BUILD-INFO.json`; a stale source or binary fingerprint is rejected.

The archive contains a generated mount/header experiment and gate scaffold, not the campaign implementation. These headers have **not** passed a UE3 package parser, game startup, Mod Manager import, or DLC-disable test. The descriptor is stored as `research/moddesc.example.ini`, with no importable root `moddesc.ini`. The manifest uses the `fixture` channel, and public installation rejects it even with `--qa-candidate`. Do not copy these generated components into a game.

Archive generation, extraction, hash inspection and the fixture-install rejection are automated checks. Listing the archive alone does not prove that a DLC works. No user-facing ASI group ID or verified mount compatibility is claimed.

## Packaging real authored components

The existing specification-based builder remains available for actual authored components:

```powershell
python tools/build_shareable.py build <source_root> <spec.json> <new_package_directory> --baseline-manifest <verified_game_inventory.json>
```

It requires the original-game hash inventory, rejects hashes copied from that inventory, enforces fixed mod-owned target paths, rejects diagnostic probe binaries, and records the real component hashes. A candidate must declare unfinished QA. Release promotion additionally requires every release gate and its evidence fingerprint. This source-based path does not certify the content's in-game behavior merely by packaging it.

## Installation into an independent test copy

For a real authored candidate, the portable `manage.py` and repository tool expose the same lifecycle commands:

```powershell
python tools/build_shareable.py verify <package_root>
python tools/build_shareable.py status <test_game_root>
python tools/build_shareable.py install <package_root> <test_game_root> --qa-candidate
python tools/build_shareable.py reinstall <package_root> <test_game_root> --qa-candidate
python tools/build_shareable.py remove <package_root> <test_game_root>
```

All mutation requires a verified independent-copy marker and the exact supported executable hash. The process guard refuses mutation while any `MassEffect3.exe` is running. No game is launched by these commands.

Installation preflights every target, ancestor, reparse point and receipt before copying. An existing file is a conflict, regardless of whether its bytes match. Each new file is created exclusively; no original game component, loader, third-party ASI, save, or unrelated configuration is an allowed target.

The initial receipt declares an incomplete installation. Each created directory and each fully copied/hash-verified file is journaled durably before advancing. Only the verified files enter the installed-file ownership ledger. The final receipt is replaced atomically. `status` validates receipt identity, manifest hash and ledger rather than trusting its status label.

Removal requires the caller's trusted package-manifest hash. It validates every receipt directory against the component ancestors and checks all files before deleting any. Changed files, unjournaled partial files, symlinks/junctions and invalid ownership metadata stop removal. Unmodified owned files are removed; recorded empty directories are removed deepest-first, while nonempty directories remain.

An interruption before a file is created, or after a verified file is journaled, can be cleaned with `remove`. A process termination during a write may leave unverified partial bytes; those bytes and the receipt are preserved for inspection, rather than guessing ownership. Power loss between directory creation and its journal update may leave an empty directory. SHA-256 establishes consistency with a supplied manifest, not publisher authenticity or protection against hostile concurrent filesystem modification.

## Replacement and recovery

`reinstall` validates the replacement payload, target identity and explicit candidate permission **before** touching installed components. It requires an unmodified completed installation. Replacing a different manifest additionally requires the original package's trusted manifest hash:

```powershell
python tools/build_shareable.py reinstall <new_package_root> <test_game_root> --qa-candidate --installed-manifest-sha256 <trusted_old_manifest_hash>
```

The transaction stages verified backups of every old component and verified copies of every new component in `ME3SquadCoop.reinstall/`. Only then does it replace owned targets and commit the new receipt. A normal I/O failure triggers restoration of the original verified bytes and receipt. Backups remain until commit and cleanup.

After process termination or a storage failure that also prevents automatic rollback, stop further installation/removal and recover using the **old trusted package**:

```powershell
python tools/build_shareable.py recover <old_package_root> <test_game_root>
```

Recovery validates the journal, target ownership and every required backup before restoration. A committed replacement finishes cleanup without reverting. A prepared replacement restores the old installation. If a component, receipt, backup or transaction directory has foreign or changed content, recovery preserves it and reports the conflict. Backups are retained when recovery cannot finish. There is no promise of continued writes succeeding on a failed disk, or of a filesystem transaction that remains invisible to an external concurrent installer.

## Automated validation

`tests/test_package_manifest.py` uses owned temporary fixtures, never an installed game. Coverage includes collision preflight, original-file preservation, invalid receipt/cleanup-path rejection, changed and unjournaled-file preservation, real copy-error journaling, rejected replacement preservation, staged upgrade, failure rollback, process-termination recovery, rollback retry, modified backup rejection, committed cleanup, compiler discovery and archive extraction/installation rejection. Symlink fixtures can skip when Windows lacks the required privilege. The archive fixture requires Zig and 7-Zip.

Actual Mod Manager import, end-user installation/disable/rollback, native DLC activation, real conversations, reliable combat and cross-PC Radmin remain separate unverified requirements. See [VALIDATION.md](VALIDATION.md).
