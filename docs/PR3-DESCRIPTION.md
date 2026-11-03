# PR #3 description (draft, paste-ready)

The GitHub MCP server available here is read-only for pull requests (get / get_diff / get_files /
get_commits / get_comments / get_review_comments / get_check_runs / get_status) and no `gh` CLI or token
is present in this environment, so the description was written here for the owner to paste rather than
posted under the owner's account.

Base: `dev/campaign-coop` (266e9d6). Head: `ThatEd:main` @ bd6cc71. 15 changed files, all additions to
tools/, tests/ and docs/ — no `src/`, no packaging, no game files.

---

## Problem and resulting behavior

The campaign co-op plan needs facts about the shipped packages that the game itself does not print: where
story progression happens, what a second player's pawn requires, whether the join path is reachable from
script, and what the engine already replicates. This PR adds a read-only offline reader for the `.pcc`
packages, plus the commands and regression tests that turn those questions into measurements with a check
that can fail behind each one. It changes no game file, launches nothing, and touches no native source.

## Approach and scope

* `tools/melepkg/` — a container, import/export-table, property-tag and CDO reader for all three
  Legendary Edition builds, derived from the shipped packages. The container model is verified by three
  closure invariants per chunk; the property reader by a 600/600 tag closure; every command refuses to run
  without an install or index rather than printing zeroes it could not have measured.
* 24 invariant commands on `mele_props.py` (`coopcheck`, `joincheck`, `spawncheck`, `rostercheck`,
  `startcheck`, `gricheck`, `prefixcheck`, `netcheck`, `travelcheck`, `capacitycheck`, `mutatorcheck`,
  `transitioncheck`, `scenecheck`, `plotcheck`, `storycheck`, `savecheck`, `squadadd`, `maplist`,
  `routing`, `mapgame`, `probe`, `dump`, `value`, `cdo`).
* `tools/check_pins.py` — compares the mod's committed instruction-byte pins against an executable
  offline, separating "the mod is wrong for its target" from "this is not the supported build", and
  refusing to call a mismatch a mod defect when the code sections are not plaintext.
* `docs/CAMPAIGN-INVARIANTS.md` — 21 sections, one per finding, each labelled observed/derived/hypothesis
  per `docs/VALIDATION.md`. `docs/OFFLINE-PACKAGE-READER.md` and `docs/PIN-CHECKING.md` say how to run
  it and what it cannot read.

Key measurements (all re-runnable with the commands above):

* No build enables seamless travel (`bUseSeamlessTravel` set by 0 of 6,685 CDOs), so a level change is a
  hard reload and the guest's pawn is re-created and re-possessed on the far side.
* LE3's campaign class clamps `MaxPlayersAllowed=1`; LE1/LE2 set neither. The clamp is `InitGame`, not
  `AtCapacity`, so `?MaxPlayers=2` alone does nothing.
* The whole UE3 join chain is script on all three builds; `ServerMove` and `ClientRestart` are never
  replaced, and possession is the one overridden seam.
* Story progression is the plot-manager Kismet family, dominated by `PMExecuteTransition`
  (8,758 / 32,214 / 15,625 instances); the payload is one integer `m_nIndex`, not a string or enum.
* Scene state (`bCinematicMode`, `ViewTarget`, `bPlayersOnly`) is per-controller and does not replicate;
  the engine already ships `ClientSetCinematicMode`, and LE2/LE3's cinematic override already loops the
  squad while LE1's does not.

## Validation actually run

- `python -I tools/melepkg/test_mele_props.py` — 35 cases, ALL OK.
- `python -I tests/test_mele_invariants.py` — 30 tests, OK, 4 skipped (no package index in CI).
- `python -I tests/test_check_pins.py` — 23 tests, OK, synthetic PE images only.
- `python -I tests/test_melepkg_container.py` — 5 tests, OK, synthetic containers only.
- `python -I tools/melepkg/test_container_format.py` — ALL OK, reads real `SFXGame.pcc` on all three
  builds when an install is present; skips cleanly without one.
- No game was launched. No game file, save or profile was modified. The reader opens packages read-only.

Skips and limits, stated honestly:

* The bytecode decoder (`ue3_decode.py`) is deliberately partial (~40% ME1/ME2, ~14% ME3) and exposes no
  per-function disassembler; ME3's opcode table is genuinely extended and its operand sizes were not
  invented.
* Commands that need every package require a pre-built package index and exit 2 without one rather than
  reporting zeroes.
* The installed Steam build's `MassEffect3.exe` has encrypted `.text` (entropy 8.0000), so no offline
  byte comparison of that file can validate a pin; `check_pins.py` reports that as UNREADABLE (exit 2)
  rather than as mismatches. The decryption mechanism is a hypothesis, narrowed by the PE header.
* The three mutator-route unknowns (cooking a mod's class in, whether `?Mutator=` survives the
  campaign's own travel URLs, whether a mutator's effect replicates) are outside what packages can answer
  and are labelled as such.

## AI assistance and sources

AI assistance was used extensively. The grammar and operand rules for the bytecode work came from
`CodeRedModding/UnrealEngine3` (`UnStack.h`, `ScriptSerialization.h`); the stock `AddMutator` behaviour
from `Development/Src/Engine/Classes/GameInfo.uc`. Every package-derived claim is re-runnable with the
commands above and is labelled observed/derived/hypothesis. No game assets, saves, private paths or
credentials are included.

## Checklist

- [x] Base branch is dev/campaign-coop.
- [x] I understand the change and have the right to contribute it under MIT.
- [x] Relevant tests ran; skipped/unrun checks are stated honestly.
- [x] Original games/saves stay protected; nothing writes to the install.
- [x] No private data, game assets or generated native binaries are included.
- [x] Documentation reflects the demonstrated behavior and known limits.
