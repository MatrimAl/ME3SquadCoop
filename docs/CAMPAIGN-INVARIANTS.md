# Campaign invariants read offline from the shipped packages

These are **offline observations** of the LE1/LE2/LE3 campaign packages, not in-game results. They
constrain what a guest implementation can assume, and several contradict assumptions the guest work in
this repository is built around. Nothing here proves guest behaviour; it narrows where to look.

Reproduce every row: `python -I tools/melepkg/mele_props.py coopcheck` (sections 1-5),
`savecheck`, `squadadd`, `storycheck` (7), `plotcheck` (8), `spawncheck` (9), `joincheck` (10) and
`maplist`. The reader and its scope limit are described in
[offline package reader](OFFLINE-PACKAGE-READER.md). Tests:
`python -I -m unittest discover -s tests -p "test_mele_invariants.py" -v`.

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

## 7. Story progression has one shared interception point, and it is not a trigger volume

Four classes carry the campaign's story progression, with **identical class names and identical
property names on all three builds**:

```
BioSeqAct_PMBase              m_sObjectType(Str)  m_nIndex(Int)  Region(Byte)  Plot(Byte)
BioSeqAct_PMCheckState        OutValue(Bool)      State(Byte)
BioSeqAct_PMCheckConditional  Param(Int)          OutValue(Bool)  Conditional(Byte)
BioSeqAct_PMExecuteTransition Param(Int)          Transition(Byte)
```

ME1/ME2's base adds `m_sVarName`, `m_nPrevRegionIndex`, `m_nPrevPlotIndex`; ME3 drops those three.
`PMExecuteConsequence` is ME1/ME2 only. `PMBase` is declared and has a class default object but
declares no functions and is placed in zero maps - the three subclasses are what a level uses.

Counted in every package holding a `PersistentLevel` (1,811 / 2,156 / 2,677 maps):

| action | LE1 | LE2 | LE3 |
| --- | --- | --- | --- |
| `BioSeqAct_PMCheckState` | 4,913 / 473 maps | 12,019 / 815 | 14,422 / 1,014 |
| `BioSeqAct_PMCheckConditional` | 1,824 / 227 | 12,613 / 386 | 12,686 / 722 |
| **`BioSeqAct_PMExecuteTransition`** | 8,752 / 453 | 32,209 / 801 | **15,607 / 1,058** |
| `BioSeqAct_AreaTransition` | 837 / 43 | 271 / 157 | 30 / 22 |
| **`BioAreaTransition`** (trigger volume) | **1 / 1** | **2 / 2** | **0 / 0** |
| `BioSeqAct_StartConversation` | 762 / 283 | 821 / 280 | - |
| `SFXSeqAct_StartConversation` | - | 940 / 389 | 1,944 / 692 |
| `BioSeqAct_AddToParty` | 426 / 52 | 402 / 138 | 244 / 201 |
| `BioSeqAct_AutoSave` | 178 / 159 | - | - |
| `SFXSeqAct_SaveGame` | - | 800 / 413 | 983 / 499 |

Two things follow, and the second corrects a natural assumption:

1. **The host-advances-the-story / guest-observes rule has one interception point on LE3, not three.**
   It sits at the story layer rather than the engine layer, which is a stronger place to enforce
   authority than anything that only decides what an actor is.
2. **Walking into a trigger volume changes almost no level.** `BioAreaTransition` - a `TriggerVolume`
   carrying `sMoveToArea` / `sMoveToStartPoint` - is used **once in LE1, twice in LE2, never in LE3**
   across 6,644 maps. A hook on trigger volumes follows essentially nothing. The campaign reaches the
   plot manager 56,568 times by comparison.

Conversations and saves *do* split per build, and LE2 is the awkward case: it carries **both**
`BioSeqAct_StartConversation` and `SFXSeqAct_StartConversation`, so hooking one covers part of that game
and silently misses the rest.

**Caveat on what this does not say.** These are class-instance counts from export tables, so they
establish how often a node is *placed*, not that a given node fires or what it does when it fires. The
`Region`/`Plot` byte properties are typed by `EBioRegionAutoSet`/`EBioPlotAutoSet`, which have two
members each (`*_Unset`, `*_MAX`) - stubs for a value space defined in the plot manager's own data - so
the size of story state **cannot** be read from those enums. Section 8 reads it from the accessors
instead, which do bound it.

## 8. Story state is small enough to ship whole, on all three builds

Section 7 says where the campaign advances. This says what the advancement costs to synchronise, which
decides whether a guest needs a delta log or can simply be handed the whole thing.

`PlotManager.pcc` on LE1 and LE2 defines exactly one class, `BioAutoConditionals`, and its entire body
is plot-flag accessors shaped `F(id)(Argument: Int) -> ReturnValue: Bool`. Each flag is an integer
addressed by a small id, and the id space is bounded by the largest id present:

| build | shape | size |
| --- | --- | --- |
| LE1 | 1,066 accessors, ids 113..2018 | ~8 KB as int32 over 2,019 slots |
| LE2 | 1,356 accessors, ids 1..2050 | ~8 KB as int32 over 2,051 slots |
| LE3 | one save struct, no accessor class | ~69 KB dense over 17,655 slots |

At most ~8 KB on the first two. That is the number that matters: a host can ship the entire flag set on
every level change rather than maintaining a delta log, and a guest can be handed a complete story
state instead of a replay of how it got there. It also means story state is cheap enough to include in
a desync check - comparing two 8 KB blobs every N seconds is free.

**LE3 has no `BioAutoConditionals` in any of its 7,943 packages and ships no `PlotManager.pcc`, and
that is not the same as having no story state.** It keeps the same state in one place: `SFXGame.pcc`
declares `PlotTableSaveRecord` inside `SFXSaveGame`, and the save object holds it as `PlotRecord` (with
`ME1PlotRecord` beside it for imported LE1 state). The struct is seven arrays - `BoolVariables`,
`IntVariables`, `FloatVariables`, `QuestProgress`, `QuestIDs`, `CodexEntries`, `CodexIDs` - plus a
`QuestProgressCounter` int. Entries are index-keyed pairs, not names: `IntVariablePair` is
`{Index: int32, Value: int32}` and `FloatVariablePair` is `{Index: int32, Value: float32}`, 8 bytes each.

The size comes from constants the runtime plot manager ships with. `BioGlobalVariableTable` - the plot
manager, under a different name - carries on its class default object
`ME1PlotTable_IndexOffset=10000`, `Float_CutoffIndex=10039`, `Int_CutoffIndex=10148`,
`Bool_CutoffIndex=17655`, so the index space runs to 17,655 slots: ~69 KB dense, ~138 KB as pairs.

Two consequences that change the design rather than decorating it:

1. **"Ship the whole state" holds on all three builds, and on LE3 it is the easier case**, because the
   payload is already a save struct the engine knows how to write. The per-build difference is the
   *shape* - named flags on LE1/LE2, an index-addressed blob on LE3 - not whether it is affordable.
2. **Plot state gates level streaming.** `BioSeqAct_GetIsLevelLoaded.GetLevelStreamingObject` takes
   `(LevelName: Name, oWorldInfo: Object, i: Int, PlotStreamingElements: Array, Element: Struct)` and
   each level package declares its own `PlotStreamingElements`. A guest must receive the host's plot
   state **before** its streaming is applied, not after.

The names are only part of the set: `oNameLookupTable` points at `PlotManagerGameData`, a `Bio2DA`
cooked inside `SFXGame.pcc` under the package group `BIOG_2DA_PlotManager_X` with 176 rows naming the
*named* integer variables. Counting names therefore undercounts; the count came from the constants.

An earlier version of this section said LE3's flag set could not be enumerated and called it the hard
build. The absence of the accessor class is real; the conclusion drawn from it was not.

**The sync itself is not a getter/setter pair, and the first reading of it was wrong.** An earlier
version of this section named `GetPlayerPlotData` / `SetPlayerPlotData` on `BioGlobalVariableTable` as the
plot sync, on the strength of their names. Resolving the parameter's `StructProperty` type shows both
take **`PlayerInfoEx`** - the player *identity* block (`FirstName`, `FaceCode`, `CharacterClass`,
`MorphHead`, `BonusTalentClass`, `bIsFemale`, `Origin`, `Notoriety`, plus `CharacterGUID` on LE3). The
name says plot; the payload is character.

The plot payload is `PlotTableSaveRecord`, held by `SFXSaveGame.PlotRecord` and by **nothing else** on
every build, and **no function in any build takes it as a parameter** - so it is reached through the save
object rather than a getter. It is also the same struct on all three builds: `BoolVariables`,
`IntVariables`, `FloatVariables`, `QuestProgress`, `QuestIDs`, `CodexEntries`, `CodexIDs`,
`QuestProgressCounter`. LE3 declares `IntVariablePair` and `FloatVariablePair` inside it as nested types,
which is why its field count reads 12 rather than 10.

## 9. A guest pawn needs a character description, and LE1 does not have one

`spawncheck` answers the question behind "the client takes a squadmate": does the campaign's own pawn
spawn take a *character description*, or only a controller?

| build | campaign spawn | params | parms | takes `PlayerInfo` |
| --- | --- | --- | --- | --- |
| LE1 | `BioSPGame.SpawnDefaultPawnFor(NewPlayer, SpawnPoint, BioPlayerController, Pawn)` | 5 | 0x28 | **no** |
| LE2 | `SFXGame.SpawnDefaultPawnFor(NewPlayer, StartSpot, ..., PlayerInfo, ...)` | 29 | 0xdc | yes |
| LE3 | `SFXGame.SpawnDefaultPawnFor(NewPlayer, StartSpot, ..., PlayerInfo, ResetPlayerInfoEx, ..., CharacterGUID)` | 33 | 0xf4 | yes |

The identity payload is `PlayerInfoEx`, held by `SFXSaveGame`, at **7 / 8 / 9 fields** across the builds.

So on LE2 and LE3 the guest pawn is the campaign's own spawn called with a different character
description, and the struct it wants is one the save system already writes. **LE1's campaign spawn takes
four opaque objects and no identity at all**; its identity levers are the other functions on the same
class (`SpawnPlayer`, `SpawnPawn`, `SpawnHenchman`, `SpawnPlayerSquadMembers`). That is a real difference
in effort, not a detail - and it is the reason LE1 is not simply the same work again.

Both payloads were resolved with `mele_tables.property_struct()`, which reads the struct reference at
serial +44 of a 48-byte `UStructProperty`. That offset is measured, not assumed: it resolves for
2,539/2,539 struct properties on ME1, 2,752/2,752 on ME2 and 4,480/4,503 on ME3, against a 1.5% rate for
every other offset. The 23 ME3 misses are 50-byte struct properties, a wider form, reported as unreadable
rather than guessed.

## 10. Accepting a second player is a class override on every build

`joincheck` answers the question the rest of the plan rests on: is the code that decides about a joining
player still in script, and does a second player have anywhere to come from?

The whole UE3 join chain is implemented **in script** on `Engine.GameInfo` on all three builds -
`PreLogin`, `Login`, `PostLogin`, `AtCapacity`, `RestartPlayer`, `SpawnDefaultPawnFor`,
`ChoosePlayerStart`, `FindPlayerStart`, `SpawnPlayerController`, 9 of 9, with bytecode rather than
native stubs. So co-op does not need a native patch to decide about a second player.

| build | campaign class | script overrides | touches the join chain |
| --- | --- | --- | --- |
| LE1 | `BioSPGame` | 18 | `SpawnDefaultPawnFor`, `FindPlayerStart` |
| LE2 | `BioSPGame` | 4 | nothing (stock `GameInfo` path) |
| LE3 | `SFXGame` | 80 | `RestartPlayer`, `SpawnDefaultPawnFor` |

On LE3 the runtime campaign GameInfo is `SFXGameInfoSP` (super `SFXGame`) in `BIO_COMMON.pcc`, whose
export list begins with the path `..\BIOGame\MPContent\BIO_COMMON.upk` - the campaign game class ships
inside a multiplayer content package. `Default__SFXGameInfoSP` sets exactly one property,
`GameReplicationInfoClass=SFXGRISP`, and inherits the `MaxPlayersAllowed=1` clamp from `SFXGame`.

`SFXGame` still carries teams (`InitializeTeams`, `ChangeTeam(Other, N, bNewTeam) -> bool`), a player
count (`GetPlayerCount`), `GetDownedPlayerCount`, join-in-progress delegates, `CalculatedNetSpeed()`,
and **`SpawnNewPlayerPawn`** - 17 parameters, parms block 0x90, `-> Object`:

```
SpawnNewPlayerPawn(bCombatPawn, bInjuredPawn, bUseCasualAppearance, ForcedCasualID,
                   bCreateAndShowWeapons, PC, SpawnLocation, SpawnRotation, ReturnValue,
                   Engine, firstName, Origin, Notoriety, ResultPawn, PlayerArchetype,
                   faceCode, CharacterGUID)
```

It takes a `PlayerController` and a `PlayerArchetype` and returns a pawn, which is precisely what a
squadmate-as-player needs. The guest's pawn does not have to be invented; what is missing is a caller
that accepts a second player. `SFXGameInfoMP_LobbyUnusedDummy` also ships, which says MP was disabled
deliberately rather than removed.

## 11. The net driver is native on every build
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
