# Native activation DLC packaging

A separate [authored-script candidate](SCRIPT-DLC-CANDIDATE.md) now stages the
revive package, minimal startup config and generated/read-back TOC privately.
It is not installed or engine-validated and does not change the asset-free
distribution/installer contract described here.

A protected test has now received an owner-reported corrupt-DLC dialog naming
`DLC_MOD_ME3SquadCoop`. The screenshot did not capture process attribution.
**The Mount-only activation container is not an engine-validated package and
must not be distributed as a working install.** The completed test removed it. A later PID-bound guest recording reproduced
the same named warning after ordinary join; a readiness delay did not resolve
package acceptance.

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


## Offline TOC-only isolation candidate

`tools/prepare_activation_dlc.ps1` stages an asset-free candidate containing
only the existing Mount and a DLC-root PCConsoleTOC.bin. It uses the already
installed LegendaryExplorerCore TOC generator and reads the result back, requiring
exactly one CookedPCConsole\Mount.dlc entry of108bytes. A separate project
reader/byte comparison confirmed the staged Mount is identical to
make_activation_mount(); its versions, priority, flags and reserved bytes are
unchanged. No startup PCC, config, TLK or owner game asset is added.

```powershell
& tools/prepare_activation_dlc.ps1 -LegendaryExplorerPath C:\Tools\LegendaryExplorer `
  -OutputDirectory C:\ResearchRepo\local\activation-dlc\toc-candidate
```

PowerShell7 and a fresh directory below this checkout's local/ are required.
Existing outputs and reparse ancestors are rejected. The receipt records file
sizes/digests, tool identity and prepared-not-installed status. Failed outputs
retain failed status. The tool never installs or launches a game; engine
acceptance remains false. This candidate isolates the missing root TOC from
other packaging changes. It has not been promoted into the existing archive
or receipt-installer allowlist. A protected engine acceptance observation is
required before treating this as the corrupt-DLC repair.


## TOC-only engine result

A protected ordinary pair installed the exact unchanged108-byte Mount plus
its generated76-byte DLC-root TOC. Independent byte parsing also checked the
TOC magic, bucket/entry bounds, Mount path and size. Guest video nevertheless
reproduced the corrupt-content warning naming DLC_MOD_ME3SquadCoop after the
ordinary join request. No native connection interval was observed. The missing
TOC alone is therefore insufficient to explain or repair this candidate's
rejection. The exact engine rejection branch remains unknown; coalesced/TLK
requirements remain hypotheses, not justification for arbitrary added files.
The candidate remains unaccepted and is not promoted to distribution.

The test removed the temporary ASI, Mount and TOC, restored the probe gate,
recorded original profiles unchanged and no native exception, and left no
game process. Both window encoders exited0. This result does not invalidate
the TOC format readback; it separates structural consistency from actual
engine acceptance.


## Named configuration follow-up

Focused offline disassembly of the supported executable separates two paths
to the same error-record routine/code0x51692. The Mount caller records it when
the Mount loader returns false. Another caller invokes a loader that constructs
Default_ plus the DLC-derived name plus .bin; resolution/file-load failure also
records that code. The latter path is conditional on DLC state flags and a
global enable. This static finding does not prove which branch fired live.
Chained x64 unwind fragments were included rather than mistaking the first
runtime-function fragment for a complete body. No game was launched for this
inspection; Ghidra's timed-out search supplied no decompilation evidence.

The staging tool now offers explicit -IncludeMinimalConfig for a separate
mount-toc-minimal-config candidate. It compiles one newly authored BioEngine
core.system seekfreepcpaths property, type2, naming only this mod's own cooked
folder. Those property names/operation type were independently observed in
retained base and LEMP configuration reads; no binary config was copied.
There is no startup PCC, startup registration, TLK, animation or owner asset.
Readback requires exactly that one property, and regenerated TOC readback
requires both the unchanged Mount and the correctly sized named coalesced.
The candidate receipt still records EngineAccepted=false and Installed=false.
This narrows the next experiment; it does not establish the corruption fix.


## Named config result and English metadata candidate

The protected named-config pair changed the visible rejection: guest video
showed "DLC does not run in English" naming this mod, rather than the preceding
corrupt-content warning. No native connection interval was established. This
supports the relevance of the named config, but is not full package acceptance
or proof of which exact internal branch ran. The supported base TLK independently
maps0x51692 to the observed corruption message, with input hashes unchanged.
The run cleaned up all three temporary DLC files and the ASI, left original
profiles unchanged, recorded zero native exceptions and left no game process.

An explicit -IncludeEnglishTlk option, requiring -IncludeMinimalConfig, now
prepares a separate English-only metadata candidate. Official retained starter
code specifies own-name, folder, language and Male/Female metadata records.
The locally read LEMP English metadata uses en-us; only that metadata value was
used as a format reference, with no game TLK bytes or dialogue copied. The new
TLK contains project-authored strings and is encoded/read back using existing
LEC. Both gender-table readbacks and the regenerated TOC entry are checked.

The private candidate uses research string ID1915100, absent from the examined
base English table; a registered allocation or compatibility with every mod is
not established. Mount's two TLK ID fields now reference that ID. All other
Mount bytes and the preceding named config remain identical. The generated
receipt labels the distinct variant and records allocation verification=false,
Installed=false and EngineAccepted=false. This is an English research candidate,
not support for every language or a release/install payload. No new game run
has evaluated its language acceptance.
