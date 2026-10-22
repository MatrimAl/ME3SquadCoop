# Reversible Packaging and Delivery

This document describes the packaging, delivery, and reversible lifecycle architecture for the **ME3 Squad Co-op** mod in **Mass Effect 3 Legendary Edition (ME3LE)**.

---

## 1. Package Architecture

The deployment package is delivered as a solid LZMA2 `.7z` archive conforming to **ME3Tweaks Mod Manager 7.0** custom DLC conventions alongside a gated native ASI companion.

```
ME3SquadCoop-v0.1.0.7z
├── moddesc.ini                                          # ME3Tweaks Mod Manager descriptor
├── DLC_MOD_ME3SquadCoop/
│   └── CookedPCConsole/
│       ├── Mount.dlc                                    # Mount priority 5100
│       └── ME3SquadCoop_Startup.pcc                     # UE3 package (v685 / l205)
├── native/
│   ├── ME3SquadCoop.asi                                 # Release native companion with active-DLC gate
│   └── README-NATIVE.md                                 # Native companion deployment documentation
├── payload/                                             # Authorised staging tree for portable manager
│   ├── Binaries/Win64/ASI/ME3SquadCoop.asi
│   └── BioGame/DLC/DLC_MOD_ME3SquadCoop/CookedPCConsole/
│       ├── Mount.dlc
│       └── ME3SquadCoop_Startup.pcc
├── manifest.json                                        # Cryptographic component ledger and QA fingerprints
├── package_manifest.py                                  # Reversible installer engine
├── manage.py                                            # Standalone CLI manager for test installations
├── PACKAGE-STATUS.md                                    # Channel readiness and testing disclaimer
├── README.md                                            # Mod summary and instructions
└── SHA256SUMS.txt                                       # Authoritative checksum manifest
```

### 1.1 Custom DLC (`DLC_MOD_ME3SquadCoop`)
- **Mount Priority**: `5100` (defined in `Mount.dlc`, evaluated by the game engine at startup).
- **Startup Package**: `ME3SquadCoop_Startup.pcc` (valid Unreal Engine 3 package header for ME3LE, version 685, licensee version 205).
- **Mod Manager Descriptor**: `moddesc.ini` targeting `game = ME3LE` with `[CUSTOMDLC]` entries pointing to `DLC_MOD_ME3SquadCoop`.

### 1.2 Native ASI Companion & Active-DLC Runtime Gate
- Standard ME3Tweaks ModJob tasks do not install arbitrary `.asi` or `.dll` binaries into custom DLC folders.
- The companion binary is supplied in `native/ME3SquadCoop.asi` for loader execution (such as Bink2w64 ASI loader).
- To prevent unintended execution when the mod is disabled in ME3Tweaks Mod Manager, `ME3SquadCoop.asi` embeds an active-DLC runtime gate (`check_active_dlc_gate` / `ME3SquadCoopVerifyDlcGate`).
- At runtime (`DllMain`), the ASI checks whether `BioGame\DLC\DLC_MOD_ME3SquadCoop\CookedPCConsole\Mount.dlc` exists and is accessible. If the DLC is absent or disabled by Mod Manager, the ASI fails closed and remains completely inert with zero installed hooks.

---

## 2. Reversible Lifecycle & Safety Guarantees

All packaging operations adhere to strict safety invariants to guarantee zero risk of corruption to original base game files, user save profiles, or other mods:

### 2.1 Preflight Conflict Detection (No Overwriting)
- Before creating any file, directory, or receipt, `install()` preflights every target file path.
- If **any** target path already exists on disk (whether from the base game or a third-party mod), `install()` halts immediately with `FileExistsError(f"refusing to overwrite {item['path']}")`.
- No target files are overwritten, no partial files are created, and no installation receipt is committed.

### 2.2 Installation Ledger (`ME3SquadCoop.install.json`)
- Upon passing preflight checks, the installer writes a durable ownership journal (`ME3SquadCoop.install.json`) with `"status": "incomplete"`.
- The ledger explicitly records:
  - Exact package identity and version
  - Authoritative manifest SHA-256 hash
  - List of installed file paths, sizes, and SHA-256 hashes
  - List of directories created solely for this installation
- Files are stream-copied and verified with post-copy SHA-256 checks. Once all files are copied and verified, the receipt is atomically transitioned to `"status": "installed"`.

### 2.3 Safe Uninstallation
- `remove()` inspects the installation receipt and validates the package identity and manifest hash.
- Every installed file is hashed before any deletion:
  - If a file is missing (e.g. from an aborted partial install), it is safely skipped.
  - If a file exists on disk and its SHA-256 matches the ledger, it is marked for deletion.
  - If a file exists on disk but its SHA-256 **does not match** (indicating user customization or external modification), `remove()` **immediately aborts** with `ValueError(f"changed component preserved: {path}")`. No files are deleted.
- Unmodified mod files are unlinked.
- Directories listed in `created_directories` are checked in reverse order: if and only if a directory is completely empty, it is safely removed. Any directory containing user files or files from other mods is preserved intact.
- The installation receipt is unlinked last.

### 2.4 Interrupted Install Recovery
- If an installation is interrupted halfway (due to power loss, process termination, or I/O failure), the receipt remains on disk with `"status": "incomplete"`.
- A subsequent `install()` refuses to run over an existing receipt, preventing state corruption.
- Calling `remove()` on an incomplete installation safely audits all partially written files, deletes those that match the manifest, cleans up empty directories, and removes the incomplete receipt, restoring the game directory to a clean state.

### 2.5 Safe Reinstallation Flow
- Calling `reinstall()` validates the existing installation:
  - If the existing installation is clean and unmodified, it invokes `remove()` to clean up previous files, then runs `install()` to deploy fresh components.
  - If any existing file has been modified, `reinstall()` halts and refuses to overwrite user data.

---

## 3. CLI and Tooling Usage

The packaging and management tool is available at `tools/build_shareable.py` (and deployed as `manage.py` inside packages).

### Build Deployment Archive (.7z)
```powershell
python tools/build_shareable.py build outputs/ME3SquadCoop-v0.1.0.7z
```

### Inspect Installation Status
```powershell
python tools/build_shareable.py status <path_to_test_game_root>
```

### Install into a Test Copy (QA Candidate)
```powershell
python tools/build_shareable.py install <package_root> <path_to_test_game_root> --qa-candidate
```

### Safe Reinstall
```powershell
python tools/build_shareable.py reinstall <package_root> <path_to_test_game_root> --qa-candidate
```

### Clean Uninstallation
```powershell
python tools/build_shareable.py remove <package_root> <path_to_test_game_root>
```

---

## 4. Testing & Verification Summary

All packaging, installation, reinstallation, collision rejection, interrupted recovery, and directory cleanup behaviors are verified through automated unit and integration tests in `tests/test_package_manifest.py`:

| Test Case | Scenario Verified | Result |
|---|---|:---:|
| `test_install_remove_preserves_every_preexisting_file` | Clean install followed by clean uninstall leaves pre-existing files unchanged | PASS |
| `test_reinstall_rejected_if_installed_and_reinstall_flow` | Duplicate install blocked by receipt; reinstall helper cleanly rolls back and redeploys | PASS |
| `test_preexisting_collision_aborts_without_creating_receipt` | Colliding pre-existing file halts install; target untouched, no receipt created | PASS |
| `test_interrupted_incomplete_install_rollback_and_recovery` | Incomplete install journaled; rollback cleans partial files and permits clean reinstall | PASS |
| `test_changed_component_blocks_entire_remove` | Modified mod component aborts uninstall and preserves all files | PASS |
| `test_remove_cleans_empty_created_directories_and_preserves_non_empty` | Empty created directories removed; directories with third-party files preserved | PASS |
| `test_inspect_installation_accurately_detects_all_states` | Status reporter distinguishes uninstalled, clean, incomplete, and diverged states | PASS |
| `test_validate_native_rejects_diagnostic_probe_symbols` | Reject binaries containing research/probe symbols or sandbox markers | PASS |
| `test_deployment_archive_and_reversibility` | Complete LZMA2 solid .7z archive built with valid moddesc.ini and release companion | PASS |

---

## 5. Testing Notice & Mod Manager Status

> [!IMPORTANT]
> **Real in-game ME3Tweaks Mod Manager execution has NOT been live tested with human players.**
> 
> All tests documented here were executed programmatically against synthetic and verified test sandbox copies.
> This package is provided as a **reversible QA candidate** for independent test installations.
> **Do not treat or distribute this package as a finalized, playable co-op release.**
