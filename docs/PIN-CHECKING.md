# Offline pin checking

`tools/check_pins.py` compares the native code pins in `src/` against an executable, offline, before any
game process starts. It exists because the mod's guards are the right idea and run in the worst place.

## The problem it addresses

The ASI is an exact-build mod. A hook is written as a pinned sequence:

```c
!guest_cinematic_hex_pin(base+0x91860,"448bc93bca7d30...")
```

That guard is correct and necessary — a wrong offset in a hook is a crash, not a wrong answer. But every one
of them is evaluated **inside the game**, so a pin that has drifted is discovered as a failed launch, after
the copy, the staging, the profile setup and the colleague's time. Measured over `src/`: **9** such
committed-hex pins, all of them in one file (`src/guest_cinematic_registration_adapter.h`), plus **294**
absolute offsets across **43** files used with no pinned bytes at all — those cannot be validated at runtime
either, so a wrong one is silently wrong.

Checking the same bytes against the same executable, in Python, takes under a second and needs no game.

## What it distinguishes, and why that is the point

Three outcomes mean three different things, and collapsing them is how a support thread fills with the wrong
diagnosis:

| outcome | meaning | who acts |
| --- | --- | --- |
| `match` | the pinned bytes are at that RVA in this executable | nobody |
| `mismatch` on the **supported** executable | **the mod is wrong** for its own target | the mod's author |
| `mismatch` on any other executable | **the build is different**; the mod is not implicated | whoever is testing |

The supported executable is `MassEffect3.exe`, fixed file version `2.0.0.48602`, SHA-256
`b453871eb6eacff0c646490ffbc588f743399a829b1f9e31cd7775dc95da8001` — the value `README.md` already states and
`src/package_manifest.py` already enforces. The tool asserts that hash in its own tests, so the support
boundary cannot drift away from the documentation without a failure.

**A matching version number is not a matching build.** The Steam install on this machine reports the same
fixed version, `2.0.0.48602`, and is a different executable:

| | supported | installed here |
| --- | --- | --- |
| fixed file version | `2.0.0.48602` | `2.0.0.48602` |
| SHA-256 | `b453871e…95da8001` | `72047e65…215b9ec7` |
| size | — | 26,896,672 bytes |
| PE `TimeDateStamp` | — | `0x60aef0c4` (2021-05-27) |
| committed-hex pins matching | expected all | **0 of 9** |

So a build cannot be identified, or excluded, by its version string, and the tool prints the version
alongside the hash for exactly that reason. Until the `b453871e…` executable is located or reproduced, no
native claim in this repository can be tested on this machine — the pins name offsets in a binary that is not
here. That is a build-identification gap, and it is the first thing to close.

A fourth outcome, `unmapped`, is separate again: no PE section covers the RVA, which means the offset does
not address file-backed code at all. That is a different repair from a wrong pin.

Exit codes: `0` all pinned sequences match; `1` a pin does not match the **supported** build;
`2` the executable or sources are missing, **or the code sections are not plaintext** — both are a
skip, not a result, because neither can support a claim about the mod; `3` pins do not match but the
executable is not the supported one.

A note on exit code 2 and encryption: a file whose `.text` is random at 8.0000 bits/byte is not "a build the
mod does not support". It is a file the mod's own guards never see, because the game decrypts before it runs.
Telling those apart is the difference between "wrong build" and "cannot be checked offline", and the tool says
which one it means.

## Usage

```powershell
# The installed build, default paths
python -I tools/check_pins.py

# A specific executable and a specific checkout
python -I tools/check_pins.py --exe "<game dir>\Binaries\Win64\MassEffect3.exe" --root . --json local/pins.json

# The tests, against synthetic PE images only - no game needed
python -I tests/test_check_pins.py
```

`--json` writes every pin with its status and the actual bytes, so a run can be diffed against the next one.

`tools/Check-ME3.ps1` already produced a read-only compatibility report. It now runs this check and adds a
`nativePins` block — the supported hash, whether the executable matches it, the check's exit code and its
one-line result — so the report answers "will the mod's hooks land in *this* install" without a separate step.
It also means a compatibility report can no longer be produced without noticing that the executable is not
the supported one.

## What is checked and what is not

Checked: the PE section map (RVA to file offset), every `base+0xNNN,"hex"` pin in `src/*.h` and `src/*.c`,
and every absolute `base+0xNNN` offset that has no pin.

Not checked, and this matters: **a matching set of pins does not mean the mod works.** Pins prove the
instructions are where the code expects them. They say nothing about whether the hook is called, whether the
reflected offsets inside objects are right (those are member offsets, not code addresses, and this tool
ignores anything below the first section), or whether the behaviour is correct. The synthetic tests are not
game results.

## Tests

`tests/test_check_pins.py` builds minimal PE32+ images in the test scratch directory and asserts that a
matching pin passes, a one-byte change fails, an uncovered RVA is `unmapped` rather than `mismatch`, an
unpinned offset is reported rather than counted as passing, the unsupported-executable case gets its own exit
code, and an encrypted image is refused rather than compared. Twenty-one cases, no game, no network, no
modification of an installed copy.

Exit codes: `0` all pinned sequences match the supported build; `1` a pin does not match the **supported**
build; `2` the executable or sources are missing, **or the code sections are not plaintext** — both are a
skip, not a result, because neither can support a claim about the mod; `3` pins do not match but the
executable is not the supported one.

## What this says about the rest of the native surface

The pin failure is the smaller half of the finding. Measured over the two trees, which are **not** the same
tree:

| tree | source files | distinct code addresses | recognized hex-string pins | recognized coverage |
| --- | --- | --- | --- | --- |
| fork `main` | 21 | 61 | 9 | **15%** |
| `fix/guest-equipment-readiness` | 77 | 140 | 9 | **6%** |

Two limits on that table, stated because they bound what "6%" means:

* It counts **recognized hex-string pins** — the `base+0xNNN,"hex"` form this scanner reads. The sources also
  guard entry points with committed **byte arrays and `memcmp`** — for example
  `guest_equipment_bootstrap.h` and `guest_player_context_adapter.h` — which this scanner does not recognize.
  So the checked fraction is **understated**, and the table is a statement about one scanner, not about how
  much of the mod is guarded. A comprehensive source analyzer is out of scope here.
* It is a property of one branch at one commit. The numbers move as sources change.

What the table does support: on the branch the live work runs, most distinct code addresses carry no
hex-string pin this checker recognizes, and an address with no recognized pin is not compared offline.
Whether it is guarded some other way is exactly what the scanner cannot say.

That is why the tool can propose the missing recognized pins instead of only reporting the gap:

```powershell
python -I tools/check_pins.py --generate code_pins.h
```

It reads the first 16 bytes at every code address the sources use, skips (and names) any address with no
file-backed bytes, and **flags sequences with fewer than 8 distinct byte values** — sixteen zero bytes matches
almost any build and would validate the wrong one. On this machine that is 52 new pins covering 100% of the 61
addresses, with 2 flagged weak.

Two cautions are built into the output rather than left to the reader:

* the header says **"Review before committing"**, because a pin generated from the wrong executable validates
  the wrong build — and this machine's executable *is* the wrong build;
* when the source executable reports the supported fixed version, `--generate` prints a warning saying a
  version match is not a build match and asking for the SHA-256 to be confirmed first.

So use `--generate` against the **supported** executable. Against any other, the pins it produces are a
description of that other build, which is worse than no pins because they would pass. And keep the generated
file **outside `src/`**: the checker scans `src/`, and a generated table placed there would be scanned as if
its rows were hand-written pins in the sources.

**But `--generate` refuses to run against this machine's install, and that refusal is the finding.**

## The bigger finding: the installed executable's code is encrypted at rest

Measured on the installed `MassEffect3.exe` (`72047e65…`, 26,896,672 bytes):

| section | entropy | what it means |
| --- | --- | --- |
| `.text` (17 MB) | **8.0000 bits/byte** | random — not code |
| `.data` (7.4 MB) | **7.9996** | random — not data |
| `.rdata` (7.2 MB) | 4.54 | **plaintext** — holds Bioware source paths (`C:\metr\sfx3\Development\Src\...`) |
| `.pdata`, `.reloc` | 6.77, 5.73 | structured |

Controls on the same machine: `notepad.exe` `.text` 6.24, `kernel32.dll` 6.26, with hundreds of
`48 83 ec 28` prologues. The installed LE3 `.text` has **zero** `48 83 ec 28` in 17 MB, **zero** duplicated
32-byte windows in 4 MB, and 38.3% printable bytes — which is the random baseline (95/256 = 37.1%), not text.

**So the file on disk is not the code the game runs.** It decrypts in memory — **the mechanism is a
hypothesis**, and the measurement is scoped to the one executable inspected here: `.text` and `.data`
measure 8.0000/7.9996 bits/byte with no repeated 32-byte window and no `48 83 ec 28` prologue, while
`.rdata` is plaintext and still holds Bioware source paths. That is a measurement about this file, not a
claim about every LE3 install. The PE header narrows the hypothesis — the entry point is in `.ooa`, not
`.text`, and the image imports exactly one DLL, `Core/Activation64.dll` — but no wrapper code was read and
the decryption itself was not observed. Three consequences, in order of how much they change:

1. **No offline byte comparison of this executable can validate anything about the mod.** "9 pins mismatch"
   against this file is a statement about the file, not about the build. `check_pins.py` now detects this and
   reports `UNREADABLE` with exit code 2 instead of reporting mismatches, and `--generate` refuses rather than
   writing random bytes as pins — a generated pin from an encrypted image would pass, which is worse than none.
2. **The build-identity gap is not "find the right exe".** Even a correct Steam install of this game would have
   encrypted `.text`. The supported build can only be identified from a **decrypted image**, which means the
   supported SHA-256 `b453871e…` describes a build the owner has in a form this machine does not — or the pins
   were taken from a decrypted image the repository does not record how it obtained.
3. **The in-process guard is the only check that can work.** `supported_image()` reads memory after the game
   has decrypted itself, so its timestamp and 7-byte check operate on real code. That is why the launcher's
   SHA-256 gate plus the in-process guard together are the design, and why neither alone is sufficient: the
   hash gate refuses a different file, the guard refuses a wrong build at runtime, and neither can be replaced
   by an offline byte comparison of the on-disk file.

The classifier that decides "encrypted" is itself tested, because it is the one component whose failure mode
is silently wrong in both directions: a section below 64 KiB is *unknown* (the statistic has no meaning), a
uniform byte ramp has entropy 8.0 but is plaintext (detected by small-lag self-match), and a printable-ratio
test was rejected because random bytes and a uniform ramp both measure 37%.

## What cannot be pinned, and what to do about it instead

Object member offsets are below the first section and are therefore not code addresses — the checker excludes
them rather than emitting pins for them, and it is right to: instruction bytes cannot validate
`read_ptr(pawn+0x1e0)`. Measured: **97 distinct member offsets** across the branch, with a strong cluster
(`+0x230` eleven times — the `ProcessEvent` vtable slot, structural in UE3; then `+0x198`, `+0x374`, `+0x42c`,
`+0x3bc`).

Those come from the class layout, which the offline package reader derives from the shipped `.pcc` files:
`mele_tables.class_fields()` reads a class's real field list and declaration order from `Engine.pcc` /
`SFXGame.pcc`. A generated header of verified member offsets, cross-checked against the live values the mod
already logs, would replace the hand-written ones — the same shape as the pin generator, different source of
truth.

What has *not* been attempted and should not be claimed: that a generated member-offset header matches this
build. The offsets in the shipped `.pcc` are the *cooked* layout, and the mapping from there to runtime object
offsets is exactly what the earlier work found does not follow a constant (`PlayerController.Pawn` computed
0x3A0 vs measured 0x3DC on ME1). So member offsets need the live values to calibrate, and the mod already
collects them.
