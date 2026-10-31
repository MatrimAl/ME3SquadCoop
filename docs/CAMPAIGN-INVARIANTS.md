# Campaign invariants read offline from the shipped packages

These are **offline observations** of the LE1/LE2/LE3 campaign packages, not in-game results. They
constrain what a guest implementation can assume, and several contradict assumptions the guest work in
this repository is built around. Nothing here proves guest behaviour; it narrows where to look.

Reproduce every row: `python -I tools/melepkg/mele_props.py coopcheck` (sections 1-5),
`savecheck`, `squadadd`, `storycheck` (7), `plotcheck` (8), `spawncheck` (9), `joincheck` (10),
`rostercheck` (3), `startcheck` (11), `gricheck` (12), `prefixcheck` (13), `netcheck` (14) and `maplist`.
The reader and its scope limit are described in [offline package reader](OFFLINE-PACKAGE-READER.md). Tests:
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

## 3. The squad roster stores a record on LE1/LE2 and the pawn itself on LE3, behind one shared call

`BioBaseSquad.Members` is an `ArrayProperty` everywhere, but its element is not the same thing. The
element type is a child property of the array, and resolving *that* property's `+44` field names it:

| build | `Members` element | what one entry **is** |
| --- | --- | --- |
| LE1 | struct `MemberData` | a **record** describing a squad member (17 fields) |
| LE2 | struct `MemberData` | a **record** (7 fields) |
| LE3 | class **`Pawn`** | **the pawn itself** |

`MemberData` on LE1 carries per-member order state - `SquadMember`, `vMoveLocation`,
`vHoldPosition`, `vTarget`, `stCoverToUse`, `EscapeLocation`, `nmPower`, `eAction`, `eSwitchWeapon`,
`eFormation`. LE3 has none of that in a struct, because its roster entry is the pawn.

**That reads like three different edits and it is not.** The API in front of the store is the same:

| build | `AddMember` |
| --- | --- |
| LE1 | `(Pawn) -> int` |
| LE2 | `(Pawn, bCheckPlaypens) -> int` |
| LE3 | `(Pawn, bCheckPlaypens) -> int` |

`AddMember` takes a pawn on every build and returns its index, so adding the guest is one call whichever
build is running; the pawn to `MemberData` conversion on LE1/LE2 happens inside the engine. What the
difference decides is what the entry *means*: on LE3 the roster holds the guest pawn alive, on LE1/LE2 it
holds a description of it.

Where the builds genuinely diverge is notification and control transfer:

| capability | LE1 | LE2 | LE3 |
| --- | --- | --- | --- |
| `MemberAdded` notify | **base + player squad** | none | none |
| `MemberRemoved` notify | yes | yes | yes |
| `GetSpareController` / `SetSpareController(newCtrl)` / `SwapSpareController(oPawn, oOriginalController, oNewController)` | **yes** | no | no |
| `BioPlayerSquad.SetPlayerPawn(PlayerPawn)` | no | no | **yes** |

Two matter. **LE1 has an explicit spare-controller slot**, and the word `SpareController` appears in no
other name in any of the 19,430 packages - a second-player mechanism sitting dormant in the campaign's own
squad class (a name search cannot exclude a bytecode call site, so "declared and named nowhere else" is
the claim). And **LE1 notifies on add while LE2/LE3 do not**: on those builds `MemberAdded` does not
exist, so anything reacting to a squad change has to be driven explicitly rather than observed.

The campaign's own recruitment action does not simply append either. `BioSeqAct_AddToParty.Activated`:

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

| build | campaign class | script overrides (real bodies) | touches the join chain |
| --- | --- | --- | --- |
| LE1 | `BioSPGame` | 17 of 18 declared | `SpawnDefaultPawnFor`, `FindPlayerStart` |
| LE2 | `BioSPGame` | 4 of 4 | nothing (stock `GameInfo` path) |
| LE3 | `SFXGame` | **70 of 80 declared** | `RestartPlayer`, `SpawnDefaultPawnFor` |

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

## 11. A guest shares the host's spawn point, because most maps have none or exactly one

`startcheck` answers where a second player could stand. Counting only `PlayerStart` gives zero spawn
points in 94-99% of maps, which is a fact about `PlayerStart` and not about the game: the campaign spawns
from **`BioStartLocation`** (plus `BioStartPoint` on LE1/LE2), which is also why LE1's campaign class
overrides `FindPlayerStart` and adds `FindStartingNavPoint`.

| build | maps | with a spawn point | exactly one | two or more |
| --- | --- | --- | --- | --- |
| LE1 | 1,811 | 129 (7.1%) | 61 (47.3%) | 68 (52.7%) |
| LE2 | 2,156 | 250 (11.6%) | 121 (48.4%) | 129 (51.6%) |
| LE3 | 2,677 | 114 (4.3%) | **69 (60.5%)** | 45 (39.5%) |

By class - `BioStartLocation` / `BioStartPoint` / `PlayerStart`: LE1 251 / 202 / 165, LE2 338 / 33 / 147,
LE3 255 / – / 44.

The class is confirmed by the campaign's own `FindPlayerStart` rather than by a name match - LE3's takes an
out parameter called **`ChkBioStartLocation`**:

| build | `FindPlayerStart` |
| --- | --- |
| LE1 | `BioSPGame(Player, InTeam, IncomingName, out ReturnValue, oNav, ATStartPoint)` |
| LE2 | `SFXGame(Player, InTeam, IncomingName, out ReturnValue, ChkActor, ATStart, Engine)` |
| LE3 | `SFXGameInfoSP(Player, InTeam, IncomingName, out ReturnValue, Engine, ATStart, ChkActor, ChkBioStartLocation)` |

Most of the 6,644 packages are streaming sublevels (`_ART`, `_SND`, `_LAY`, `_DSG`) that never receive a
player, which is why the 7%/12%/4% figures are small. The useful number is the last two columns: **of the
maps that do have a spawn point, LE3 has exactly one in 60% of them**, so a guest has no start of its own
in most places a player can appear.

That settles guest placement without a new mechanism: share the host's start and pass
`bNoCollisionFail`, which ME3's `Engine.GameInfo.SpawnDefaultPawnFor` takes and ME1/ME2's does not - the
build added "spawn even though something is standing there" precisely because two things can want the
same spot.

## 12. LE3's campaign replication info is multiplayer-shaped, but its capability flags are local

`Default__SFXGameInfoSP` sets exactly one property, `GameReplicationInfoClass=SFXGRISP`, and `SFXGRISP` is
a **zero-override alias of `SFXGRI`** - 35 properties and 32 script functions - which is the per-session
state the campaign is built on.

| build | campaign `GameReplicationInfoClass` | that class |
| --- | --- | --- |
| LE1 | **`BioGameReplicationInfo`** | 3 properties, 1 function |
| LE2 | *inherited* - stock `GameReplicationInfo` | `BioGameReplicationInfo` exists in `SFXGame.pcc` but the CDO does not point at it |
| LE3 | **`SFXGRISP`** | 35 properties, 32 functions |

`SFXGRI` carries capability switches (`bCanSave`, `bCanShowJournal`, `bCanShowCodex`, `bCanShowMap`,
`bPlayerCanChangeSquad`, `bCanSpawnHenchmen`, `bPauseForCommand`, `bAllowTimeDilation`), multiplayer state
(`MaxUniquePlayers`, `bMultiplayer`, `bIsMultiplayerCharacter`), combat state (`bInCombat`,
`bForceCombat`, `bAlwaysInCombat`), and the functions `NumPlayersInGame`, `NumLivingPlayers`,
`IsJoinInProgress`, `NextLivingPlayer`, `GetPlayerLevel`, `IsGameOver`, `GetTeamScore`,
`CopyProperties(OldGRI)`. `Default__SFXGRI` closes exactly with 18 properties; all eight capability
switches default to True, and `bMultiplayer` / `bIsMultiplayerCharacter` / `MaxUniquePlayers` are unset.

**The capability switches do not replicate, and a co-op plan must not assume they do.** Exactly four of
the 35 properties carry `CPF_Net`:

```
replicated:  MaxUniquePlayers, WaveCoordinator, bForceCombat, bInCombat
local-only:  bCanSave, bCanShowJournal, bCanShowCodex, bCanShowMap,
             bPlayerCanChangeSquad, bCanSpawnHenchmen, bPauseForCommand, bAllowTimeDilation
```

So "the guest cannot save" has to be enforced where the save happens (the `SFXSeqAct_SaveGame` surface in
section 7), not by flipping a bool the host owns. What *is* replicated is combat state and the player
ceiling - the half that must not diverge, which is the useful half to get for free.

`CopyProperties(OldGRI)` is a state-transfer primitive LE1/LE2 do not have: one GRI can be handed
another's state wholesale, which is what a late join or a level transition needs.

An earlier pass concluded LE1/LE2 have no replication-info class at all. Both define
`BioGameReplicationInfo` and ME1's campaign points at it; the difference is scale and whether the campaign
uses it.

## 13. Routing knows no multiplayer class, and the campaign class already supplies the player stack

`prefixcheck` reads the engine's map-prefix table. ME3's `Default__GameInfo.DefaultMapPrefixes` has
**three rows**, and every one is marked `bIsMultiplayer=False`:

| `Prefix` | `GameType` | `bUsesCommonPackage` | `bIsMultiplayer` |
| --- | --- | --- | --- |
| `EntryMenu` | `SFXGameContent.SFXGameInfoEntryMenu` | False | **False** |
| `BIOG_UIWorld` | `SFXGame.SFXGame` | False | **False** |
| `Bio` | `SFXGameContent.SFXGameInfoSP` | **True** | **False** |

`GameInfoClassAliases` holds exactly one alias, `SP -> SFXGameContent.SFXGameInfoSP`, and `DefaultGameType`
names the same class. So nothing in the engine's routing hands a campaign map to a networked game class -
the multiplayer entry points were removed from the table, not disabled.

One prefix covers the whole campaign: `Bio` catches every `Bio*` package into `SFXGameInfoSP` with
`bUsesCommonPackage=True`, which is how `BIO_COMMON.pcc` - the MPContent-derived package from section 12 -
is loaded for every campaign map. `bIsMultiplayer` is a `GameTypePrefix` field on LE3 only (7 fields versus
5 on LE1/LE2).

The player stack is already wired: `Default__SFXGame` sets `PlayerControllerClass=<SFXPlayerController>`,
`PlayerReplicationInfoClass=<SFXPRI>`, `HUDType=<BioHUD>` and `MaxPlayersAllowed=1`. Since every campaign map
routes to `SFXGameInfoSP : SFXGame`, any player the engine spawns gets a Bio controller, PRI and HUD with no
co-op code - which is the asset that decides whether a second player has a working HUD.

**The per-player record replicates, and it is multiplayer-shaped.** `SFXPRI` has 10 properties and 27
functions: `CreditsEarned`, `PointsEarned`, `ScoreLog`, `ScoreReplicationPeriod`, `ActiveMatchConsumables`,
`ReplicatedScoreInfo`; `AddPoints`, `AddPlayerMedal`, `ReplicateScoreInfo`, `CanSetReadyInLobby`,
`SetReadyInLobby`; and `ServerAddActiveMatchConsumable` / `ServerRemoveActiveMatchConsumable`, which are the
server-authoritative RPC pattern already in the shipped code. Three properties replicate.
`FindLocalPRI` distinguishes *the local player's* record from everyone else's, which only makes sense with
more than one player.

So the split is backwards from what a networked game would want, and worth stating plainly: **per-player data
replicates, per-session permissions do not.** A guest's score syncs for free; a guest's inability to save has
to be enforced by the mod (section 12).

Two reader notes for anyone repeating this: `ArrayProperty` values decode into a tag **list per element**,
so a row is folded by hand; and for an import, `describe_index`'s package prefix is the cook **source**, not
the package. `SFXGameInfoSP`'s super reads `Core.SFXGame` and is really `SFXGame.SFXGame`;
`mele_tables.qualified_name()` gets it right.

## 14. What the engine already replicates to a guest

`netcheck` counts `CPF_Net` on real property exports across every class in `Engine.pcc`.

| build | properties | replicated |
| --- | --- | --- |
| LE1 | 7,922 | **145 (1.83%)** |
| LE2 | 7,929 | **147 (1.85%)** |
| LE3 | 7,923 | **146 (1.84%)** |

Only ~1.8% of the engine's properties replicate, and they are the right ones.

**`Actor` (20-21):** `Role`, `RemoteRole`, `bNetOwner`, `bProjTarget`, `Location`/`location`, `Rotation`,
`Velocity`, `RelativeLocation`, `RelativeRotation`, `DrawScale`, `bHidden`, `Base`, `Owner`, `Instigator`,
`Physics`, `bCollideActors`, `bBlockActors`, `bCollideWorld`, `bHardAttach`, `bTearOff`,
`ReplicatedCollisionType`. The authority model and the transform both replicate.

**`Pawn` (25-26):** `Controller`, `PlayerReplicationInfo`, `Health`, `FiringMode`, `FlashCount`,
`FlashLocation`, `TakeHitLocation`, `HitDamageType`, `bIsCrouched`, `bIsWalking`, `RemoteViewPitch`,
`DrivenVehicle`, `InvManager`, and the movement rates. A complete player pawn.

**The stock client-join handshake is in script on `PlayerController` on every build, 5/5:**
`ServerGivePawn`, `ServerAcknowledgePossession`, `ServerMove`, `ServerSetSpectatorLocation`,
`ServerUpdatePing`, plus 62-65 `Client*` functions with bytecode. That is unmodified UE3 - the host assigns a
pawn, the client acknowledges possession, the client sends its own movement.

**And the campaign's own controller chain never replaces that transport.** Following the class each campaign
instantiates:

| build | chain, outermost last | `ServerMove` replaced |
| --- | --- | --- |
| LE1 | `PlayerController` 364 fn -> `BioBasePlayerController` 2 -> `BioPlayerController` 119 | **no** |
| LE2 | `PlayerController` 362 -> `BioBasePlayerController` 0 -> `BioPlayerController` 166 -> `SFXPlayerController` 14 | **no** |
| LE3 | `PlayerController` 361 -> `BioPlayerController` 272 -> `SFXPlayerController` 76 | **no** |

`ServerMove` and `ClientRestart` are declared once, on `Engine.PlayerController`, and never overridden
anywhere in the Bio chain on any build. The chain replaces `Possess` (on `BioPlayerController` everywhere and
again on `SFXPlayerController` on LE2/LE3), `UnPossess` (LE2/LE3) and `SpawnPlayerCamera` (LE3).

So the movement transport a guest gets is exactly the engine's own, and **possession is the single overridden
seam in the presence path** - which is also the seam "the client takes a squadmate" has to hook. On LE3 the
whole of co-op becomes: lift `MaxPlayersAllowed` from 1 on `SFXGame`, let the stock `Login`/`PostLogin` path
hand the guest a controller, and override `Possess` so it takes a squadmate pawn. No packet format, no
movement code, no snapshot system.

**And the campaign's possession is already squad-aware on LE2/LE3**, which is the signature an override has
to match:

| build | `BioPlayerController.Possess` |
| --- | --- |
| LE1 | `(aPawn, bVehicleTransition, V)` - 0x18, no squad |
| LE2 | `(aPawn, bVehicleTransition, V, **PlayerSquad**, oBioPawn, oBioWorldInfo)` - 0x30 |
| LE3 | `(aPawn, bVehicleTransition, V, **PlayerSquad**, oBioWorldInfo)` - 0x28 |

LE2/LE3 take the `PlayerSquad` the pawn is being placed into, which matches the recruitment difference
measured in section 3 (LE1's `AddToParty` touches no `Bio2DA`, LE2/LE3 go through the table). And the class
the campaign actually instantiates, `SFXPlayerController`, has a **stock-shaped**
`Possess(aPawn, bVehicleTransition)` at 0xc - so the override matches the engine's own signature and the
squad-aware version sits one level down. On the pawn side, `SFXPawn_Player.PossessedBy(C,
bVehicleTransition, Manager)` adds one parameter the engine lacks.

**`Projectile` replicates nothing, on all three builds.** Weapon fire travels as `Pawn.FlashCount` (an
increment) and `Pawn.FlashLocation` (a position), and each client spawns its own projectile locally. So
tracers and muzzle effects are free and **damage is not** - the guest's projectile is a different object from
the host's, so every damage path has to be authority-checked.

Two traps for anyone repeating this measurement. A class's children include its **enums**, and counting them
as properties inflated `Actor` by 15 fields and the density figure from 1.8% to 4.7% - filter on the child's
class being a `UProperty` subclass. And `Actor.Location` is spelled with a capital on LE1/LE2 but
`Actor.location` on LE3, so a cross-build name lookup must be case-insensitive.

**A third trap, which cost six counts in this document before it was caught:** a UE3 **native** function
still carries a 2-3 byte stub body (`0b 53`, `04 0b 53`, `0b 04 0b 53`), so `serial_size > 4` cannot tell an
implementation from a declaration. **1,070 of the 5,279 functions in LE3's `Engine.pcc` are that shape -
about 20%.** `mele_tables.is_implemented()` uses the proven bytecode extent instead. The counts in this
document are the corrected ones; every structural conclusion survived, because each rested on a specific
function whose body is hundreds of bytes rather than on a total.

## 15. The net driver is native on every build
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
