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
`2` the executable or sources are missing (a skip, not a result); `3` pins do not match but the executable is
not the supported one.

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
unpinned offset is reported rather than counted as passing, and the unsupported-executable case gets its own
exit code. Twelve cases, no game, no network, no modification of an installed copy.

## What this says about the rest of the native surface

The pin failure is the smaller half of the finding. **9 pinned sequences, all in one file, is thin cover for
294 absolute offsets across 43 files** — and an unpinned offset cannot be validated at runtime either, since
nothing checks the bytes there. Two directions follow, and they are independent:

1. Pin the entry points that matter. Every offset used as a *code* address (a function pointer, a
   `ProcessEvent` target, a hook site) can carry its first N instruction bytes the same way
   `guest_cinematic_registration_adapter.h` does, and then `check_pins.py` covers it for free. This is
   mechanical and needs no new knowledge.
2. What cannot be pinned is the object layout — `read_ptr(pawn+0x1e0)` and its relatives. Those are member
   offsets, not code addresses, and instruction bytes cannot validate them. They need the class-default
   offsets this repository's offline package reader derives from the shipped `.pcc` files, which is the other
   half of the work: `tools/melepkg/` reads a class's real field list and declaration order out of
   `Engine.pcc`/`SFXGame.pcc`, and `mele_tables.class_fields()` is the entry point. A generated header of
   field offsets, checked against the live values the mod already logs, would replace the hand-written ones.
