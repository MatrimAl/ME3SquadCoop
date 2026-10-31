# Offline package reader

`tools/melepkg/` reads a Legendary Edition `.pcc` package without launching the game: the Oodle
container, the name table, the import and export tables, function signatures with byte offsets, enum
members, UnrealScript bytecode extents, and the tagged property stream inside a class default object.

It exists because every claim in [CAMPAIGN-INVARIANTS.md](CAMPAIGN-INVARIANTS.md) is a statement about
shipped data, and a statement about shipped data should be checkable without a running process.

## What it is for

The question this answers is not "does this class declare `DefaultGameType`" but **"what is it set to
in the object the campaign actually constructs"**. `mele_tables.py` answers the first; `mele_props.py`
answers the second, and only for class default objects.

## Scope limit, measured

This is a **class default object reader**. It is correct on `Default__X` exports and is not usable on
an actor spawned in a level:

| package | objects closing exactly |
| --- | --- |
| `ME3/SFXGame.pcc` | **40/40** (100%) |
| `ME3/BioA_Cat002.pcc` | **9/40** (22.5%) |
| `ME2/BioA_BchLmL.pcc` | **0/40** (0%) |

A spawned actor's serial block is not a plain property tag stream. The failure is quiet: a level query
returns a handful of plausible tags before desynchronising, so a wrong answer looks like a measurement.
`tests/test_mele_invariants.py` asserts both halves of this so the limit cannot be quietly widened.

Anything about a *spawned* object needs a live read. That is what this repository's native probes are
for, and none of this module replaces them.

## Why the closure check matters

A `Default__X` export's serial data is exactly the property tag stream, so a correct parse consumes it
completely and ends precisely at `serial_offset + serial_size`. A wrong layout desynchronises within a
few tags and almost never lands on the exact end. `probe` runs the measured layout against the refuted
one and reports both, so "the layout closes" is evidence only because the alternative closes visibly
less (40/40 against 0/40). A check that cannot fail is worse than no check.

## Usage

```powershell
# Which tag layout closes this package's objects, and does the rival close fewer?
python -I tools/melepkg/mele_props.py probe <package> [--count 40]

# Print a class default object's properties
python -I tools/melepkg/mele_props.py cdo <package> <class>          # reads Default__<class>
python -I tools/melepkg/mele_props.py dump <package> <object> [--grep text]
python -I tools/melepkg/mele_props.py value <package> <object> <property>

# The campaign invariants, across all three installs
python -I tools/melepkg/mele_props.py coopcheck    # travel, capacity, roster, possession, dialogue
python -I tools/melepkg/mele_props.py savecheck    # how each build persists a level
python -I tools/melepkg/mele_props.py squadadd     # how the campaign recruits a squad member
python -I tools/melepkg/mele_props.py storycheck   # what advances the campaign, in every map
python -I tools/melepkg/mele_props.py plotcheck    # how much story state that advancement produces
python -I tools/melepkg/mele_props.py maplist      # how many maps a full pass covers
python -I tools/melepkg/mele_props.py routing      # map prefix -> game class, and the driver check

# One package's tables, signatures and enums
python -I tools/melepkg/mele_tables.py summary  <package>
python -I tools/melepkg/mele_tables.py fields   <package> <class>
python -I tools/melepkg/mele_tables.py signature <package> <function>
python -I tools/melepkg/mele_tables.py enum     <package> <enum>
python -I tools/melepkg/mele_tables.py chain    <package> <class>
```

`coopcheck`, `savecheck`, `squadadd` and `routing` exit non-zero when a measurement contradicts what
the documentation claims, so they are usable as checks rather than as printouts. `routing` exits 1
when the transport half is skipped for lack of an index, which is a "not checked" signal rather than a
failure - read the `skipped:` line.

## Where the game is found

`melepkg.game_dirs()` locates the installs automatically (a Steam library search that matches the
folder name case-insensitively and de-duplicates, so one install is not walked twice). Override it
with `MELE_GAME_DIR` when the library is somewhere unexpected.

Two checks want to look inside **every** package rather than a named one, and they need an optional
pre-built index for that:

| command | without an index |
| --- | --- |
| `maplist` | walks the install and parses each package - correct, and minutes per build |
| `storycheck` | refuses, and says so; it counts actions in every map and a partial count is misleading |
| `routing` (driver half only) | prints `skipped: no package index` and reports the rest |

`plotcheck` deliberately needs no index: it opens two named packages, so it answers in seconds.

Set `MELE_INDEX_DIR` to a folder holding `ME1.json`/`ME2.json`/`ME3.json` to avoid the walk. Both
refuse to report a zero they could not have measured; neither is silently fast.

No install at all is a **skip, not a finding**: `coopcheck`, `savecheck`, `squadadd` and `maplist`
exit 2 and print `skip, not a result` rather than a page of zeroes, and the test module skips too.

`melepkg.find_oodle()` uses the game's own `oo2core_8_win64.dll`, read in place; set `MELE_ODLE_DLL`
to point at it explicitly. No file in the game tree is modified.

## Files

| File | Purpose |
| --- | --- |
| `melepkg.py` | container chunks, Oodle decompression, name table, install discovery |
| `mele_tables.py` | import and export tables, `class_fields`, `signature`, `enum_members`, bytecode extents |
| `mele_props.py` | property tag reader, struct-array decoder, and the invariant checks above |

## Limits that are not going to be fixed by more work

- The opcode table for UnrealScript bytecode is **unproven** for this engine version and no
  disassembler is shipped. Function *extents* are proven (`32 + u32(+28) + trailer == serial_size` on
  every function in all three builds); instruction *decoding* is not. A walk that "closes" proves
  nothing here, because a partition into instructions exists for almost any byte string.
- The reader is read-only and takes no locks on game files.
- Nothing in this folder launches, patches or installs anything.
