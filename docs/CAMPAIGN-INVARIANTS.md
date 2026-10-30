# Campaign invariants read offline from the shipped packages

These are **offline observations** of the LE1/LE2/LE3 campaign packages, not in-game results. They
constrain what a guest implementation can assume, and several contradict assumptions the guest work in
this repository is built around. Nothing here proves guest behaviour; it narrows where to look.

Reproduce every row: `python -I tools/melepkg/mele_props.py coopcheck` (sections 1-5),
`savecheck`, `squadadd` and `maplist`. The reader and its scope limit are described in
[offline package reader](OFFLINE-PACKAGE-READER.md). Tests:
`python -m unittest tests.test_mele_invariants -v`.

Labels follow [VALIDATION.md](VALIDATION.md): **observed** means read from a file with a check that
can fail; **derived** means it follows from observations plus engine behaviour; **hypothesis** means
untested.

## 1. There is no seamless travel

`bUseSeamlessTravel` is declared on `Engine.GameInfo` in all three builds and is **set by none of the
6,685 class default objects that close exactly**:

| build | CDOs closing exactly | CDOs setting `bUseSeamlessTravel` |
| --- | --- | --- |
| LE1 | 2,154 | **0** |
| LE2 | 2,211 | **0** |
| LE3 | 2,320 | **0** |

An un-set property means the class default applies, and the engine default is false. Two independent
confirmations: `SeamlessTravelTo` is declared only on base `PlayerController` and
`PlayerReplicationInfo` across all 19,430 packages and is overridden on no `BioPlayerController` or
`SFXPlayerController`, while `PreClientTravel` is overridden on LE2/LE3.

**Consequences for this project** (derived):

- `GetSeamlessTravelActorList` is declared on `PlayerController`, `GameInfo` and `Mutator` and is
  **never called by this campaign**. A plan that carries guest state through it is carrying state
  through a function the game does not use.
- Every level change constructs a new `GameInfo` and destroys the old world. The guest's possessed
  pawn does not survive it and must be re-created and re-possessed on the far side.
- A mod-level mutator installed in one world is not present in the next. Installation is per-world,
  not per-session.

This intersects the collision and continuity work in
[COLLISION-RESTORATION.md](COLLISION-RESTORATION.md): a scene change is a new world, not a
re-possession inside the same one.

## 2. LE3 clamps capacity to one on its campaign class

| build | base `Default__GameInfo` | campaign game class |
| --- | --- | --- |
| LE1 | `MaxPlayers=16`, `MaxPlayersAllowed=32` | `Default__BioSPGame` sets neither |
| LE2 | `MaxPlayers=16`, `MaxPlayersAllowed=32` | `Default__BioSPGame` sets neither |
| LE3 | `MaxPlayers=4`, `MaxPlayersAllowed=32` | `Default__SFXGame` sets **`MaxPlayersAllowed=1`** |

LE3 does not merely allow fewer players; its campaign game class serialises a `1` that clamps
admission. Reading the declaration gives 32 and is the wrong answer. This is consistent with the
`MaxPlayers`/`MaxAllowed` lease that `src/session_control.h` already raises, and it names the exact
field pair to raise on LE3 and explains why LE1/LE2 need no equivalent clamp.

## 3. The squad roster changes shape between builds, and recruitment is table-driven

`BioBaseSquad.Members` is an `ArrayProperty` everywhere, but its element type is not the same:

| build | `Members` element | `MemberData` struct |
| --- | --- | --- |
| LE1 | `TArray<MemberData>` | present, **17 fields** |
| LE2 | `TArray<MemberData>` | present, **7 fields** |
| LE3 | `TArray<Object>` | **absent** |

`MemberData` on LE1 carries per-member order state - `SquadMember`, `vMoveLocation`,
`vHoldPosition`, `vTarget`, `stCoverToUse`, `EscapeLocation`, `nmPower`, `eAction`, `eSwitchWeapon`,
`eFormation`. LE3 has none of that in a struct.

The campaign's own recruitment action does not simply append. `BioSeqAct_AddToParty.Activated`:

| build | body size | `Bio2DA` members referenced |
| --- | --- | --- |
| LE1 | 842 bytes | **0** |
| LE2 | 1,195 bytes | **5** (`GetNumColumns`, `GetStringEntryNumI`, `GetStringEntryNumN`, `GetFloatEntryII`, …) plus `BioBaseSquad.GetSquadSize` |
| LE3 | 1,186 bytes | **3** (`GetRowNames`, `GetNameEntryIN`, …) |

So appending to `Members` by hand skips bookkeeping the campaign relies on, and the symptom is
defaulted per-member state rather than an error. **The safe route is to make the campaign recruit the
guest** - fire `BioSeqAct_AddToParty` or the equivalent - rather than write the array. Nothing in the
declarations caps the roster at two; whether a third entry is *tolerated* is a **hypothesis** and the
first thing a guest roster test should answer.

Note the scope limit: the reference scan establishes that a name is referenced inside the body, not
what the code does with it. The opcode table for this engine version remains unproven, so if a hard
cap of 2 exists on this path it was not located.

## 4. Only two things in the guest plan are genuinely shared across builds

Everything squad-related above differs per build. Two things do not:

| surface | LE1 | LE2 | LE3 |
| --- | --- | --- | --- |
| `Controller.Possess` / `UnPossess` | present | present | present |
| `AIController.Possess` / `UnPossess` | present | present | present |
| `BioAiController.Possess` / `UnPossess` | **overrides both** | **overrides both** | **overrides both** |
| `BioSFHandler_Conversation.SelectConversationEntry` | present | present | present |

Taking a squadmate from its AI and handing it to a player controller is a first-class engine operation
on all three builds. Write it once. Likewise the dialogue selection call exists everywhere, so "a guest
cannot choose dialogue" is not an API gap - it is a contention question, and a different piece of work.

## 5. LE3 does not persist the level's actors; LE1 does

| build | classes ending in `SaveObject` | how a level is written |
| --- | --- | --- |
| LE1 | **23** in `SFXGame.pcc` (34 install-wide) | `BioLevelSaveObject.m_LevelObjects`, one entry per saveable actor |
| LE2 | **0** | behind `SFXSFHandler_Save` / `SFXSaveGame`, driven by `SFXSeqAct_SaveGame` |
| LE3 | **0** | same |

LE3 is the build this project targets, and its save path does **not** enumerate the level's actors, so
a guest pawn standing in the host's level is not captured by an actor-graph walk. LE1 would have that
problem; it is recorded here because the project plans LE2 and LE1 after LE3.

`HenchmanSaveRecord` is 28 fields on LE1, 32 on LE2 and **9** on LE3, which is why a squadmate's
persisted progression does not port between builds in either direction.

## 6. Scope: 6,644 maps

"Campaign co-op works" needs a denominator. A **map** is a package containing a `PersistentLevel`
export - a structural test, because the naming convention is wrong here (`BIOG_HMF_ARM_CTH_R` is an
armour mesh, `BIOG_UIWorld` is a map, and both match a `^BIOG_` level pattern).

| build | maps | base | DLC |
| --- | --- | --- | --- |
| LE1 | 1,811 | 1,811 | 0 |
| LE2 | 2,156 | 1,278 | 878 |
| LE3 | **2,677** | 1,442 | 1,235 |

## 7. The net driver is native on every build

`TcpNetDriver`, `UdpNetDriver`, `DemoRecDriver` and `SFXOnlineNetDriver` exist in **zero of the 12,704
packages**. They are native classes, so a mod can create a driver but can never adjust its properties
from script. This is consistent with the ordinary listen path this repository already uses, and it
means there is no script-level driver class to subclass as a way of adding a transport.

The UE3 replication model itself is intact on all three builds - `Actor.Role`/`RemoteRole`, 117/113/113
net-named `PlayerController` functions, the full listen-server admin surface on `GameInfo` - so the
work is in what replicates and when, not in providing a transport.

## What this does not tell you

- Nothing here is a game result. No guest ran.
- Whether a guest pawn survives a real level transition is untested. Section 1 says it cannot survive
  by seamless travel; it says nothing about what a from-scratch re-spawn does.
- The recruitment path's control flow is not decoded; only the references inside it are measured.
- Cross-build facts about LE1/LE2 come from their packages, not from a running LE1/LE2 session.
