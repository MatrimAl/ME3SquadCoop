# Current authored activation bundle

The candidate archive builder now packages the same four authored activation
files used by the protected ordinary runtime: root `PCConsoleTOC.bin`,
`CookedPCConsole/Mount.dlc`, `Default_DLC_MOD_ME3SquadCoop.bin` and
`DLC_MOD_ME3SquadCoop_INT.tlk`. It requires an explicit preparation receipt,
LegendaryExplorer directory and pinned upstream starter-config template. The
existing PowerShell verifier checks the exact file set, hashes, Mount fields,
TOC names/sizes/invariant buckets, config semantics and authored English strings
before staging. These build inputs and private receipts are not redistributed.

```powershell
python tools/build_shareable.py build candidate.7z --activation-receipt <preparation.json> --legendary-explorer <tool-directory> --starter-config-template <starter-template.bin> --powershell <pwsh.exe>
```

The resulting portable payload contains the normal ASI and those four DLC
files; the separate Mod Manager archive contains only the descriptor and DLC.
No PCC, game assets, saves, loader, diagnostic ASI or decompiled source is
included. Portable verification additionally checks the complete modern file
set, exact Mount string identity and bounded TOC structure against payload
sizes. Hash-checked inspection/removal of older Mount-only receipts remains
available; automatic generation no longer silently produces that older variant.

Candidate installation remains explicit and restricted to an independently
verified test copy. Packaging and archive integrity do not establish a complete
campaign, Mod Manager import, runtime disable/rollback or two-PC support.
The portable installer also reads the existing base `BioGame/PCConsoleTOC.bin`
and refuses before copying any component unless it already indexes all three
authored content paths with their exact sizes, flags, hash fields and buckets.
It never changes or restores that existing game file. The existing management
tool can prepare a separate artifact for review:

```powershell
python manage.py prepare-base-toc <unpacked-package> <baseline-PCConsoleTOC.bin> <new-output.bin>
```

This reads the baseline and creates a new output only. It preserves every
existing entry and bucket and adds exactly the three authored paths. Conflicts,
partial existing co-op entries and existing output files are refused. It does
not install that artifact; base-TOC setup remains a separate prerequisite.
The historical distribution contract below describes the earlier Mount-only
generation and is superseded by this section for new automatic bundles.

# Native activation DLC packaging

## Config resolution restored with combined TOC candidate

An ordinary pair from `d517c7a` used the invariant starter DLC TOC together with
the additive basegame TOC. Both native config-name lookups returned1 and the
correct resolved path, followed by successful reads of the authored config.
The guest also opened the authored English TLK. Its inspected80s frame shows
EntryMenu without the previously persistent corrupt-content modal. This is
positive evidence for config resolution/opening and a sampled visible-warning
change; it does not establish every engine validation stage or independently
attribute success to either TOC change. Preparation receipts remain immutable
offline artifacts with EngineAccepted=false.

Guest join dispatched after five samples/1000ms at world time67.954, but no
native connection interval was observed. The viewport reported Connecting and
the guest returned to EntryMenu; the exact connection failure remains unknown.
Do not classify this run as playable co-op or complete package acceptance.

Both window encoders exited0. Terminal audit and SHA-256 readback confirmed
the private basegame TOC restored byte-identical to its retained baseline.
Temporary authored files were removed; original profile unchanged; no game
processes, native exceptions, forced child cleanup or cleanup errors remained.
Only the independent copy's TOC was temporarily changed. The player installer
and distribution allowlist still require a reviewed supported TOC workflow.

## Confirmed config lookup failure and deterministic TOC preparation

The corrected observer run (`7743c40`) first logged manager-not-ready, then
installed successfully on both roles. The exact authored Default config lookup
returned0 with no resolved output on each process. A stable host snapshot
decoded21 base/official directory entries and no authored DLC directory. The
corrected string fields are+16/+32 within56-byte directory records; an earlier
exploratory+16/+40 read is superseded. Guest join dispatched and its inspected
80s frame showed the same corrupt-content warning. Cleanup restored all
temporary authored state with no remaining games, profile change, native
exception or cleanup error; both window encoders exited0.

Selected native TOC parser/registration/load functions exported successfully
from the unchanged retained database. Static count/linear-walk checks match
the candidate TOC's three entries, but that is not a live registration result.
The retained official TOCCreator includes DLC content in the LE2/LE3 basegame
TOC as well as generating individual DLC TOCs. Prior experiments changed only
the DLC TOC. A new private additive base-TOC candidate preserves all4090
baseline semantic entries and adds exactly the three authored content paths.
An independent parser checks original entries, exact additions, flags, sizes,
SHA fields, bucket placement and the native-style linear entry count. The
private runner retains a durable original backup and uses exact-content atomic
replacement/restoration only in the independent copy after original-file locks
are established. Synthetic-copy installation/restoration passed, and external
content changes were preserved/refused rather than overwritten. No actual game
has yet evaluated this base-TOC candidate; its successful acceptance cannot be
assumed from these checks.

The independent check also exposed culture-sensitive hashing in retained LEC:
its filename uppercasing uses current culture. Turkish uppercasing changes the
`i` in `.bin`, yielding a different hash bucket from invariant ASCII `.BIN`.
The first private base-TOC preparation also used doubled path separators;
both mistakes were rejected before installation. Revised preparations use
normal separators and invariant culture, preserving all other content.
The public DLC preparation now temporarily uses invariant culture for TOC
generation and restores the caller's culture afterward. The verifier independently
checks every authored filename's invariant CRC bucket, instead of relying only
on parser readback. The corrected starter and freshly generated minimal
English variants passed; the previous self-consistent wrong-bucket starter
candidate was rejected. Distribution/installer allowlists remain unchanged.

## Resolver startup observation

The first resolver-instrumented pair (`257597b`) reported installation refusal
on both processes, so it produced no resolver result. A later read-only host
snapshot matched the expected vtable, original target and wrapper bytes. The
initial revision did not distinguish refusal conditions; startup timing is a
lead, not an established cause. Guest join dispatched and an inspected80s
frame again showed the named corrupt-DLC warning. Cleanup left no game
processes, preserved the original profile and removed temporary authored files
without native exceptions or cleanup errors. Both recorders exited0.

Installation now distinguishes missing manager, vtable/slot identity, wrapper
pins, protection failure and concurrent slot change. It retries only within
the existing bounded worker loop, at most140 attempts, logging reason changes;
all identity checks remain required on every attempt. Successful installation
stops retries. Synthetic allocated-image checks exercised missing-manager and
wrong-pin refusal, successful actual slot installation, no second installation
and the attempt cap. Three native targets rebuilt and the full no-game probe
harness passed. The corrected installer has not yet been run in-game.

## File-open trace result and selected resolver

The first IO-observed ordinary pair (`d613e93`) again dispatched guest join and
showed the named corrupt-content dialog in its inspected guest80s frame. Both
roles recorded successful opens of the authored Mount and TOC via the sandbox
game path. Failed profile-prefixed probes preceded those successful fallbacks.
The captured import-only rows contain no config open; this does not establish
whether the native config lookup ran or failed. Terminal cleanup preserved the
original profile, removed temporary authored files and left no game processes,
native exceptions or cleanup errors.

A read-only host snapshot identified the file-manager vtable and its open and
resolve targets, with stable readback. An initial snapshot made with an unopened
reader was invalidated and superseded; its zero addresses are not evidence.
Four further selected functions exported successfully from the unchanged
retained database. The resolve wrapper forwards its three arguments to a
locked directory-list lookup and converts its returned pointer to a boolean;
the open wrapper first checks an archive/cache helper before a fallback.
Decompiler output has unresolved globals/forwarded arguments, so exact ABI and
forwarding decisions use the supported instruction bytes as well.

The same default-off IO flag now also installs a pinned, process-lifetime
forwarding observer at file-manager virtual+e8 in guards-only mode. It records
only Default_DLC_MOD_ME3SquadCoop.bin, at most16 rows, with the native result and
bounded copied output path. It preserves self/name/output arguments, the
return value and LastError. The exact vtable, original target and wrapper bytes
must match; install refusal is logged. It does not alter a lookup result or
cache and cannot be unloaded during the process. Three targets built and the
native probe harness passed including actual callback forwarding, output,
LastError, unrelated-name filtering and record-cap checks. This resolver
observer still requires a live run.

## Starter config runtime result and file-access observation

A subsequent ordinary pair from revision `2c5c36b` used the pinned eight-asset
starter config. Guest join dispatched with five samples/1000ms at world
time68.380; an inspected guest80s frame again showed corrupt content naming
this mod. No native connection interval was observed. Both recording encoders
exited0; cleanup preserved the original profile, removed temporary authored
files and left no game processes, native exceptions or cleanup errors. Full
starter config alone did not fix engine rejection.

Four selected native functions were exported from the retained read-only
database, with terminal success and unchanged database hashes. Their static
path constructs the named Default config, resolves it through the native file
manager and opens/reads its bytes before marking config-loaded. An eligible
record whose load fails is associated with the corruption-message ID. This
does not prove which stage failed in either live run, or establish validation
of the coalesced contents at that stage.

The protected probe now supports default-off `ME3_SQUAD_COOP_DLC_IO_TRACE=1`
only in guards-only mode. It observes the existing CreateFileW import guard
after unchanged forwarding, and logs at most64 records for paths inside the
independent copy's authored DLC. Records include requested path, open result,
access/creation values, monotonic time and LastError (meaningful on failure).
Arguments, return handle and LastError are preserved. Native file-manager
cache lookups, other file APIs and early events before the log opens are not
covered; missing rows are not proof of no engine request. Three targets built
and the no-game native probe harness passed, including actual forwarding and
opt-in/cap checks. The observer has not yet been exercised in-game.

## Latest English candidate result

An ordinary-entry pair from revision `67d87b4` dispatched guest join after five
readiness samples spanning1000ms, at world time68.211. The native viewport
reported Connecting, then a decoded PID-bound guest frame showed the corrupt
downloadable-content dialog naming this mod. The external observer recorded
EntryMenu, with no observed native connection interval. The four-file English
candidate therefore failed engine acceptance; its successful offline verifier
does not establish a valid runtime package. The exact internal rejection branch
was not observed.

Both window recorders exited0, and inspected20s frames show the Mars cinematic.
The guest80s frame contains the rejection. Cleanup preserved the original
profile, removed temporary authored files, left no game processes, recorded no
native exceptions and reported no cleanup errors.

A separate private offline comparison candidate now uses the retained official
ME3Tweaks starter config template (Git blob
`171989e363c5d3f28e7e32e680b50b5b7774f1d9`). The template contains eight config
assets, whereas the preceding minimal candidate contains only BioEngine. Only
the starter's mod-name placeholder is replaced, with all eight assets checked
on compiled readback. Mount and English TLK are unchanged and the TOC is
regenerated. This difference is a lead, not a proven cause or fix. The candidate
has not been installed, accepted by the production verifier or run in-game.
The existing distribution and installer allowlists remain unchanged.

The offline verifier now recognizes this separate starter research variant only
with an explicit `-StarterConfigTemplate` input matching the retained template's
SHA-256. It compares all eight actual config assets against the pinned template,
allowing only the BioEngine mod-name substitution. Existing Mount, English TLK,
TOC, receipt and payload checks still apply. The positive candidate passed;
consistent receipt/TOC copies with missing config assets or a changed template
identity were rejected, as was omission of the explicit template. The previous
minimal English variant still verified. These are offline results only.

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


## Offline candidate verification

`tools/verify_activation_dlc.ps1` reads a private preparation receipt and the
exact candidate files without installing or launching anything. It rejects
unsupported variants, unverified preparation status, redirected paths, extra
files, duplicate receipt paths and digest/size changes. It checks every Mount
word, including the selected variant's TLK references and zero reserved tail,
and reads the actual TOC for an exact set of correctly sized content entries.
Named config must decompile to the single intended property; English metadata
must resolve the authored strings from the actual TLK, including both gender
markers. Receipt, payload and tool hashes are rechecked after inspection.

```powershell
& tools/verify_activation_dlc.ps1 -Receipt C:\ResearchRepo\local\activation-dlc\english-candidate\preparation.json `
  -LegendaryExplorerPath C:\Tools\LegendaryExplorer
```

The English candidate passed. Three authored negative copies were rejected:
a changed Mount TLK reference with updated receipt hash, a duplicate receipt
path, and changed config length with an updated receipt hash but stale TOC.
These are offline artifact checks, not game or installation tests. The verifier
is separate from the unchanged distribution installer allowlist; verification
never changes EngineAccepted or grants release readiness.

Additional static review maps the visible English-language message to base
TLK ID0x54c14. The supported native DLC routine at RVA0xafd960 records this ID
when its conditional metadata lookup fails. It passes selector2 and a DLC
record identifier derived from the Mount alongside the DLC-derived name. This
supports checking the TLK's language metadata and references together; it does
not prove that an arbitrary TLK file alone is sufficient or that this exact
branch was captured at runtime. The new metadata candidate remains untested
in the engine.


## Prepared English acceptance experiment

The private ordinary-pair experiment has a distinct four-file English variant.
Preparation validates the selected receipt through the semantic verifier before
any temporary installation. It permits only the declared TLK-reference changes
to Mount, records the TLK/config/TOC/Mount hashes and verifier fingerprint, and
uses exclusive file creation with exact-content cleanup for all four files.
A generic Mount-only runner must not be reused for this candidate: its old
zero-TLK contract would reject the intended metadata or omit cleanup ownership.
Only parser/help preparation ran; no game launched and no file was installed.
The production archive/installer allowlist remains unchanged pending actual
engine acceptance. A future experiment must distinguish a changed visible
warning from complete acceptance and from successful native guest connection.
