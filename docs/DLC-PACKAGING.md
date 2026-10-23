# Native activation DLC packaging

A protected test has now received an owner-reported corrupt-DLC dialog naming
`DLC_MOD_ME3SquadCoop`. The screenshot did not capture process attribution.
**The Mount-only activation container is not an engine-validated package and
must not be distributed as a working install.** The completed test removed it.

Read-only native review confirms the authored Mount versions and priority fit
the supported reader; its complete byte layout also matches the official LE3
writer. Do not invent different versions or nonzero GUID data to hide this
failure. The container lacks a DLC-root `PCConsoleTOC.bin`, which the official
TOC generator produces and uses to list `CookedPCConsole\\Mount.dlc`.
The native engine can report corruption when its virtual file manager cannot
open the Mount. A separate conditional coalesced-file failure reaches the same
dialog. Which path caused the reported error remains unresolved; adding a TOC
alone is an isolation experiment, not a proven complete fix.

The candidate now authors LE3 mount metadata instead of a fabricated startup
package. It contains no UnrealScript, package exports, coalesced/config changes,
TLK strings, copied GUIDs, or game assets. The ordinary ASI currently uses the
presence of this enabled DLC folder as its activation gate. An engine-mounted
DLC and a validated player installation remain separate, unfinished checks.

The official starter-kit generator treats a startup package as an optional
add-in. Our native-only activation container has no package to register, so a
startup PCC serves no purpose. The old zero-filled PCC generator and the
requirement that every candidate contain a PCC were removed. The official
general-purpose starter kit also generates coalesced files and language TLKs;
we do not claim that omitting them from this asset-free container has been
verified in the engine. If protected testing demonstrates a real requirement,
author and validate that component before adding it.

## Reviewed binary contract

`Mount.dlc` is 108 bytes, little endian. This project's strict reader accepts
only its authored native-activation variant:

| Offset | Type | Value | Meaning |
| --- | --- | --- | --- |
| `0x00` | uint32 | `1` | Mounting information version |
| `0x04` | uint32 | `0x2AD` | LE3 package version |
| `0x08` | uint32 | `0xCD` | LE3 licensee version |
| `0x0C` | uint32 | `0x3006B` | Cooked content version |
| `0x10` | uint32 | `1..65535` | Mount priority; compatible with the upstream uint16 reader |
| `0x14` | uint32 | `0` | Version |
| `0x18` | uint32 | `0x0C` | Singleplayer and multiplayer loading; no save dependency or matchmaking flag |
| `0x1C`, `0x20` | int32 | `0`, `0` | No content-name/package-name TLK strings authored |
| `0x24..0x6B` | 72 bytes | zero | Reserved build and GUID area |

Priority 5100 is a research value, with no registered or conflict-free allocation
claim. The old writer used original ME3 versions and unrelated nonzero fields;
matching length alone could not establish LE3 validity. Strict readback now
rejects those bytes, truncation, extra data, unsupported flags and altered
reserved fields, even when a package's SHA-256 matches them.

## Distribution contract

The outer candidate bundle contains `ME3SquadCoop-DLC.7z`, the portable
hash-checked payload/receipt tooling, and an explicit unfinished status. The
inner archive contains only `moddesc.ini` and
`DLC_MOD_ME3SquadCoop/CookedPCConsole/Mount.dlc`. Its descriptor targets
`cmmver=7.0` and **`game=LE3`**, with a `CUSTOMDLC` source/destination mapping.
`ME3LE` is not a recognized game identifier in the official parser.

The ASI remains outside the Mod Manager DLC job. Ordinary `ModJob` validation
rejects `.asi` files; no extension rename, invented ASI ID, basegame write job,
or loader distribution circumvents that restriction. The native companion and
a compatible loader still need a supported, validated end-user workflow.
Do not mix the portable receipt installer with a separate manual/managed
installation and assume its receipt owns both.

The automatic archive builder requires current native build fingerprints,
rejects existing destinations, generates only the candidate channel, verifies
the staged payload and runs 7-Zip integrity checks on both archives. Candidate
installation still requires explicit QA opt-in and an independent test copy.
Release promotion still requires all real QA gates and evidence hashes.

## Primary format provenance

Reviewed 2026-10-23. Links are pinned to the source revisions actually read:

- [LegendaryExplorer `MountFile.cs`](https://github.com/ME3Tweaks/LegendaryExplorer/blob/b5e746233491ec5a7cbfc077685996af37a3d125/LegendaryExplorer/LegendaryExplorerCore/GameFilesystem/MountFile.cs#L319-L338): LE3 writer layout. Its game-three enum defines the loading/save flags and its reader reads priority at `0x10`, flags at `0x18`, and TLK ID at `0x1C`.
- [ME3TweaksCore `DLCModGenerator.cs`](https://github.com/ME3Tweaks/ME3TweaksCore/blob/b5f006add38dbaea91dde3f16f45ecbc3b0155a9/ME3TweaksCore/ME3Tweaks/StarterKit/DLCModGenerator.cs#L102-L185): mount generation, general-purpose config/TLK generation, and optional startup add-in.
- [Mod Manager game parser](https://github.com/ME3Tweaks/ME3TweaksModManager/blob/f97b1ece73a59884a72b339aae3d92d3eada496a/MassEffectModManagerCore/modmanager/objects/mod/Mod-Parsing.cs#L670-L725): `LE3` identifier and minimum descriptor version 7.0. Its `CUSTOMDLC` block validates the folder mapping.
- [Official descriptor generator](https://github.com/ME3Tweaks/ME3TweaksModManager/blob/f97b1ece73a59884a72b339aae3d92d3eada496a/MassEffectModManagerCore/modmanager/windows/StarterKitGeneratorWindow.xaml.cs#L657-L680) and [CUSTOMDLC documentation](https://github.com/ME3Tweaks/ME3TweaksModManager/blob/f97b1ece73a59884a72b339aae3d92d3eada496a/documentation/customdlc.md): descriptor fields and source/destination layout.
- [ModJob extension validation](https://github.com/ME3Tweaks/ME3TweaksModManager/blob/f97b1ece73a59884a72b339aae3d92d3eada496a/MassEffectModManagerCore/modmanager/objects/ModJob.cs#L285-L301): ordinary native-extension rejection.

Those upstream repositories use GPLv3. Their implementations and assets are
not included in this project. The Python generation and strict project policy
were independently written from the reviewed binary-format facts; they remain
under this project's MIT license.

## Verification boundary

Fixed-byte format tests, descriptor parsing, rejection tests, archive extraction,
and receipt rollback on owned synthetic targets verify those specific contracts.
They do not establish actual Mod Manager import, engine DLC mounting, native
activation/disable during a real session, usable campaign co-op, or Radmin play.
Those gates remain false in generated candidate manifests.
