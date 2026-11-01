# Campaign invariants read offline from the shipped packages

These are **offline observations** of the LE1/LE2/LE3 campaign packages, not in-game results. They
constrain what a guest implementation can assume, and several contradict assumptions the guest work in
this repository is built around. Nothing here proves guest behaviour; it narrows where to look.

Reproduce every row: `python -I tools/melepkg/mele_props.py coopcheck` (sections 1-5),
`savecheck`, `squadadd`, `storycheck` (7), `plotcheck` (8), `spawncheck` (9), `joincheck` (10),
`rostercheck` (3), `startcheck` (11), `gricheck` (12), `prefixcheck` (13), `netcheck` (14),
`travelcheck` (15), `capacitycheck` (2), `mutatorcheck` (4b), `transitioncheck` (7), `scenecheck` (18) and
`maplist`. The reader
and its scope limit are described in [offline package reader](OFFLINE-PACKAGE-READER.md). Tests:
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

**Where the clamp actually bites, measured with `capacitycheck` - identical on all three builds:**

| step | function | reads |
| --- | --- | --- |
| 1 | `GameInfo.InitGame(Options, out ErrorMessage)` - 1,454/1,454/**1,467** bytes, literals `'MaxPlayers'`, `'MaxSpectators'` | **`MaxPlayers` and `MaxPlayersAllowed`** |
| 2 | `GameInfo.AtCapacity(bSpectator) -> bool` - 138/138/**139** bytes | **`NumPlayers` vs `MaxPlayers`**, `NumSpectators` vs `MaxSpectators`; never `MaxPlayersAllowed` |
| 3 | `GameInfo.Login(...)` - 2,079/2,079/**2,127** bytes, literal `'.MaxedOutMessage'` | the refusal |

`InitGame` parses `?MaxPlayers=` from the options string and clamps the requested value against
`MaxPlayersAllowed`; `AtCapacity` compares `NumPlayers` against the resulting `MaxPlayers`; `Login` refuses
with `MaxedOutMessage`. LE3's `MaxPlayers` is the inherited 4 until `InitGame` writes 1 into it.

Two consequences. **`?MaxPlayers=2` in a travel URL does nothing on its own** - it is parsed and clamped back
inside the same function, so the obvious first experiment is pre-defeated. And the fix is the one int this
section already named, but for a measured reason: `MaxPlayersAllowed` is what `InitGame` clamps against.

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

## 4b. There is a supported hook into the join path, and it ships: the mutator chain

This is the most useful structural finding here, because it means the guest work does not need a binary patch.
`GameInfo` calls into the UE3 `Mutator` chain at every moment the plan needs, and the chain is populated from
the travel URL (`InitGame` parses `Mutator` / `Mutators` and calls `AddMutator` + `InitMutator`).

The wiring, measured by scanning each body for the hook's **name index** — a virtual call emits an `FName`,
not a package reference, so an export-reference scan reports no mutator calls at all:

| `GameInfo` function | calls |
| --- | --- |
| `InitGame` | `AddMutator`, `InitMutator` |
| `Login` | **`Mutator.ModifyLogin`**, `FindPlayerStart` |
| `PostLogin` → `GenericPlayerInitialization` | **`Mutator.NotifyLogin`** |
| `RestartPlayer` → `SetPlayerDefaults` | **`Mutator.ModifyPlayer`** |
| `Logout` | `Mutator.NotifyLogout` |
| `ServerMutate` → `GameInfo.Mutate` | `Mutator.Mutate` |

**The gate is open:** `Mutator.MutatorIsAllowed` is 29 bytes on all three builds and is
`return !WorldInfo.IsDemoBuild()` — stock UE3, so mutators are permitted in a retail build.

**And the signatures are identical on every build**, which is what makes one implementation portable:

| hook | signature | parms |
| --- | --- | --- |
| `ModifyLogin` | `(Portal: Str, Options: Str)` | 0x20 |
| `InitMutator` | `(Options: Str, ErrorMessage: Str)` | 0x20 |
| `ModifyPlayer` | `(Other: Object)` | 0x8 |
| `NotifyLogin` | `(NewPlayer: Object)` | 0x8 |
| `NotifyLogout` | `(Exiting: Object)` | 0x8 |
| `Mutate` | `(MutateString: Str, Sender: Object)` | 0x18 |
| `FindPlayerStart` | `(Player, InTeam, IncomingName, out ReturnValue)` | 0x24 |

So the three things the guest plan needs are already hook names: accept the guest is `ModifyLogin`, give it a
pawn is `ModifyPlayer`, set up its per-player state is `NotifyLogin`.

**And `InitMutator` runs after the capacity clamp** (section 2), measured by bytecode offset inside `InitGame`:

| build | clamp at | `AddMutator` at | `InitMutator` at |
| --- | --- | --- | --- |
| LE1 / LE2 | `+0x29` | `+0x302` | **`+0x487`** |
| LE3 | `+0x2b` | `+0x31c` | **`+0x4b0`** |

So a mutator can raise `MaxPlayers` *after* the ceiling has clamped it, from inside the supported path and with
no int patching. That makes the whole guest plan one deliverable: a `Mutator` subclass whose `InitMutator` sets
the player count, `ModifyLogin` accepts the guest and `ModifyPlayer` hands it a squadmate pawn.

**Three risks, stated rather than waved away:** the mutator class has to be reachable from a cooked package;
the campaign builds its own travel URLs, so whether `?Mutator=` survives a map change is unmeasured; and
nothing here proves a mutator's effect is *replicated* to a client, only that the hook is called.

**And LE3 already ships a working example, which removes most of the first risk.** `SFXGame.pcc` defines two
subclasses:

| class | super | overrides |
| --- | --- | --- |
| `SFXMutator` | `Engine.Mutator` | `SetGameSpeed(NewSpeed, NextMut)` |
| `SFXMutator_DifficultySpeed` | `SFXMutator` | `MutatorIsAllowed()`, `SetGameSpeed(NewSpeed, NextMut, GRI, GlobalSpeed)` |

`Default__SFXMutator_DifficultySpeed` sets exactly one property, `GroupNames`, whose single element decodes to
the UTF-16 string **`DifficultySpeed`** — and **`GameInfo.AddMutator` reads `Mutator.GroupNames`** (verified by
locating the property's dword inside its 619-byte body). So the whole chain is demonstrated by shipped content:

```
?Mutator=DifficultySpeed
  -> InitGame parses the 'Mutator' option
  -> AddMutator resolves the name via Mutator.GroupNames
  -> MutatorIsAllowed (!WorldInfo.IsDemoBuild()) permits it
  -> InitMutator runs, after the MaxPlayers clamp
  -> ModifyLogin / ModifyPlayer / NotifyLogin fire
```

That means the class *can* be cooked and the name *is* resolved — by the same code path a mod would use.
What is still unmeasured is only whether the campaign's own travel URLs carry the option forward.

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

**And reading the real instances turns five candidate identity fields into one.** `transitioncheck` decodes
every `BioSeqAct_PMExecuteTransition` that closes exactly - **15,625 on LE3, 1 skipped**:

| field | type | presence | value |
| --- | --- | --- | --- |
| **`m_nIndex`** | `Int` | 14,918 | **-1 … 7,416, 2,858 distinct** |
| `Region` | `Byte` (`EBioRegionAutoSet`) | 11,216 | `None` |
| `Plot` | `Byte` (`EBioPlotAutoSet`) | 10,108 | `None` |
| `Transition` | `Byte` (`EBioAutoSet`) | 9,676 | `None` |
| `Param` | `Int` | 673 | -1 … 5 |
| `m_sObjectType` | `Str` | **0** | never set |

So the progression step is identified by an **integer index**. `m_sObjectType` exists on the class and appears
on zero real instances; `Region`/`Plot`/`Transition` read `None`; `Param` is an occasional argument on 4%. A
declaration lists what a field could be; the data says what it is.

**And the three fields LE1/LE2 add are declared, not used.** `BioSeqAct_PMBase` declares `m_sVarName`,
`m_nPrevRegionIndex` and `m_nPrevPlotIndex` on LE1/LE2 only, which reads like more story state. Measured:

| build | instances | `m_nIndex` | distinct | `m_sVarName` | `m_nPrevRegionIndex` | `m_nPrevPlotIndex` |
| --- | --- | --- | --- | --- | --- | --- |
| LE1 | **8,758** | 205 … 6,540 | 1,455 | **0** | 8,751, all **−1** | 8,751, all **−1** |
| LE2 | **32,214** | −1 … 7,170 | 2,145 | **0** | 32,210 — **32,203 at −1** | 32,180 — 32,146 at −1 |
| LE3 | **15,625** | −1 … 7,416 | 2,858 | field absent | — | — |

`m_sVarName` is never set on any build, and the two `m_nPrev*` fields are **sentinel −1** on essentially every
instance (LE2 has 7 non-sentinel region values and 34 plot values out of ~32,200). So there is no second
identity to carry on the older builds and no per-build field set to reconcile — **the same rule holds on all
three builds**, across 66,597 exactly-closing instances with 3 skipped.

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

## 15. A level change is a scripted broadcast, and the seamless path is dead but complete

Section 1 established there is no seamless travel, so a level change is a hard reload. The follow-up is
whether the reload moves *every* connected player or only the host.

On all three builds, with real bodies:

| function | body | signature |
| --- | --- | --- |
| `GameInfo.ProcessServerTravel` | 704 / 704 / **718** | `(URL, bAbsolute, LocalPlayer, bSeamless, NextMap, NextMapGuid, OptionStart)` |
| `GameInfo.ProcessClientTravel` | 180 / 180 / **185** | `(URL, NextMapGuid, bSeamless, bAbsolute, out ReturnValue, out P, out LP)` |
| `GameInfo.PostLogin` | 894 / 894 / 915 | `(NewPlayer, Address, ...)` |
| `WorldInfo.ServerTravel` | 363 / 363 / 365 | `(URL, bAbsolute, bShouldSkipGameNotify)` |

`ProcessClientTravel` takes the `PlayerController` to move as an out parameter, and `ProcessServerTravel` is
the loop above it. **So a level change is already a broadcast: a connected guest is carried to the next map
by the engine**, and its pawn is re-created on the ordinary `PostLogin`/`RestartPlayer` path the same way a
late joiner's would be.

**The travel call itself is native.** `PlayerController.ClientTravel(URL, TravelType, bSeamless,
MapPackageGuid)` has a 28-byte body on LE1/LE2, and LE3's `BioPlayerController.ClientTravel` override is 24
bytes; the only package references inside are the function's own four parameters. That is the shape of a
forwarder onto native code - a **hypothesis**, since the opcode table is unproven, but 24-28 bytes cannot
hold a loop over players. The practical rule: **observe a map change at `Process*Travel`, not at
`ClientTravel`.**

**And the seamless path is scripted, complete and switched off:**

| function | body |
| --- | --- |
| `GameInfo.GetSeamlessTravelActorList` | 274-279 bytes |
| `GameInfo.HandleSeamlessTravelPlayer` | **1,315-1,343 bytes** |
| `WorldInfo.SeamlessTravel` | 19 bytes |

`HandleSeamlessTravelPlayer` is the third-largest travel function in the engine, its whole purpose is
carrying player state across a map change, and nothing reaches it because `bUseSeamlessTravel` is set by 0 of
6,685 class default objects. That is the second dormant mechanism found here, after LE1's spare-controller
trio in section 3.

## 16. The net driver is native on every build
`TcpNetDriver`, `UdpNetDriver`, `DemoRecDriver` and `SFXOnlineNetDriver` exist in **zero of the 12,704
packages**. They are native classes, so a mod can create a driver but can never adjust its properties
from script. This is consistent with the ordinary listen path this repository already uses, and it
means there is no script-level driver class to subclass as a way of adding a transport.

The UE3 replication model itself is intact on all three builds - `Actor.Role`/`RemoteRole`, 117/113/113
net-named `PlayerController` functions, the full listen-server admin surface on `GameInfo` - so the
work is in what replicates and when, not in providing a transport.

## 17. Bytecode: a real bug worth 2x, and ME3's table is genuinely extended

Everything above was read from tables and property streams. Function **bodies** were only ever sized, never
decoded. This records the attempt, a bug found in it, and its honest limit.

The real grammar came from UE3 source - `UStruct::SerializeExpr` in `CodeRedModding/UnrealEngine3`
(`ScriptSerialization.h` blob ee67f58, `UnStack.h` blob 774de7f) - so the operand rules are not guesses. Three
facts were then **measured** by requiring a walk to land exactly on the end of a body.

**Script pointers are 32-bit**, even though the game is 64-bit:

| build | `ptr=8` closed | `ptr=4` closed |
| --- | --- | --- |
| ME1 | 6.7% | **39.5%** |
| ME3 | 2.4% | **14.2%** |

**`FName` in bytecode is 8 bytes**, matching the property tags (ME1: 39.5% at name=8 vs 28.9% at name=4).

**And a closure rate split by body length found a bug in the decoder itself:** closure fell monotonically to
**0%** for bodies over 64 bytes. That pattern is a per-item offset error, not a missing opcode. The cause was an
early `raise` in the token dispatcher that fired for every native final function (`token >= 0x70`) *before* the
handler that consumes its parameters could run. Fixing it took ME1 from 21.9% to 39.5% and ME2 from 22.5% to
41.0%.

**ME3's opcode table genuinely differs from stock UE3.** ME1/ME2 close walks containing native tokens up to
`0xFF`, so that path works there. ME3 additionally uses `0x5b`-`0x65`, `0x2b` and `0x4c`-`0x50` - gaps in the
stock enum - which is why it sits at 14.2%. Deriving their operand sizes empirically **did not converge**: trying
every length 0..12 gave near-ties (token `0x62`: 82 bodies at length 5, 78 at length 10), the signature of a
desync rather than an identifiable operand. No operand sizes were invented from those numbers.

So `tools/melepkg/ue3_decode.py` is a deliberately **partial** decoder - ~40% on ME1/ME2, ~14% on ME3 - with
token names, the closure test and a token inventory, and **no per-function disassembler**. `AtCapacity` (139
bytes), the `ClientTravel` forwarder, `ModifyPlayer` (41) and `ProcessClientTravel` (185) remain sized but
**unread**, and are labelled as such wherever they are described.

Nothing in this document rests on a decode.

## 18. A conversation takes the camera and input, and nothing tells the client

Section [4](../README.md) of the design treats cinematics as "host drives, client is a passenger". That is a
description of what should happen. This is what the packages say, from `scenecheck`.

**Scene state is per-controller and it does not replicate.** On `PlayerController`, `bCinematicMode` and
`ViewTarget` carry no `CPF_Net`; the only two replicated properties on that class are `TargetEyeHeight` and
`TargetViewRotation`. On `WorldInfo`, `bPlayersOnly` carries none either - its five replicated properties are
`Pauser`, `ReplicatedMusicTrack`, `TimeDilation`, `WorldGravityZ` and `bHighPriorityLoading`. **So a client is
never told by the engine that the host entered a scene.** The two replicated rows are checked as positive
controls in the same run, on the same classes, so an unreadable flag would fail the control rather than
silently support the negative.

**The engine already ships the per-client call - and nothing reaches it for a guest.**
`PlayerController.ClientSetCinematicMode(bInCinematicMode, bAffectsMovement, bAffectsTurning, bAffectsHUD)` is
real script on all three builds (116 / 116 / 119 bytes) and does the right per-client work: `bCinematicMode`,
`myHUD.bShowHUD`, and the movement/turning input gates. It is called from `PlayerController.SetCinematicMode`
at **+0xef / +0xef / +0xfc**.

That offset is found by **FName match, not export index**. A virtual call in UE3 emits a name, not a package
reference, so an index scan reports that *nothing* calls this function - the exact false negative that would
make writing a new RPC look necessary when the shipped one is fine. `scenecheck` reports both counts and labels
the index count a lower bound.

**The campaign's own scene toggle already loops the squad on LE2 and LE3.** The action the campaign places is
`BioSeqAct_BioToggleCinematicMode` - **728 / 2,264 / 2,051** instances. The stock `SeqAct_ToggleCinematicMode`
is placed **once per build**, in `Engine.pcc`: a declaration with no campaign use. Its path ends in
`BioPlayerController.SetCinematicMode`, overridden per build:

| build | body | squad references inside it | parms |
| --- | --- | --- | --- |
| ME1 | 131 B | **none** | 6 |
| ME2 | 747 B | `m_playerSquad`, `GetMember`, `ClearRBCallbacks`, `SetWeaponImmediately`, `m_oBehavior` | 10 |
| ME3 | 775 B | `Squad`, `Members`, `ForceEndRagdoll` | 13 |

LE2/LE3 already fetch every squad member and clear its behaviour on scene entry, so a guest possessing a
squadmate rides a path the campaign runs on every scene. **LE1's 131-byte body does not mention the squad at
all**, so the loop has to be written there.

**This reverses the usual per-build ordering for this one seam.** Elsewhere LE1 is the cheapest target because
it is closest to stock `Engine.GameInfo`. For scenes it is the most expensive, and a table written left to
right would have assigned the most new code to the build that needs it.

**What is still unread:** `BioWorldInfo.GetLocalPlayerController` is a **2-byte native stub** on all three
builds, so how the campaign's scene action picks its controller is not readable from the packages. That is what
stands between "the campaign puts the squad in the scene" and "the campaign puts the guest's squadmate in the
scene", and it is a live question.

## 19. Two native shapes, not one — and the collision repair is writing a replicated bit

Two corrections to this repository's own numbers, both found by reversing the shipped packages.

### A native function that takes parameters is not an empty stub

Every "N of M functions are implemented" count here was produced by testing `len(body) > 4`. UE3 gives a
native function an empty body so the interpreter has a return to execute, and this was written down as the
*only* native shape. It is not:

| shape | body | meaning |
| --- | --- | --- |
| stub | `0b 53`, `04 0b 53`, `0b 04 0b 53` (2–3 bytes) | empty implementation |
| **native declaration** | `29 <u32> 0b 53` (7 + 5 per parameter) | `EX_NativeParm`, index, nothing, end |
| script | anything else | a body worth reading |

`0x29` is `EX_NativeParm` and `0x0b` is `EX_Nothing`, so the body reads "declare this native parameter, do
nothing, end". The dword is a **per-build native function index**, and that is decidable without decoding
anything, because it has three properties a name index cannot have:

| test | ME1 | ME2 | ME3 |
| --- | --- | --- | --- |
| all values distinct | 590 / 590 | 571 / 571 | 552 / 552 |
| outside the name table entirely | 105 | 181 | 191 |
| same-named function shared with another build carrying the same value | **0 of 455** (ME1 vs ME3) | | |

Corrected counts — these replace the earlier table:

| measurement | published | actually |
| --- | --- | --- |
| ME3 `BioPlayerController` script functions | 236 | **198** |
| ME3 `BioWorldInfo` script functions | 46 | **28** |
| ME3 `Actor` script functions | 180 | **121** |
| ME1 `PlayerController` script functions | 329 | **305** |
| ME1 `BioBaseSquad` script functions | 194 | **119** |
| LE3 `BioBaseSquad` script functions | 28 | **10** |

About **1,000 functions per build** move from "script" to "native". No finding in this document rests on a
count — they all rest on a specific function's measured size — but the *engine-wide* claim "the script layer
does this" was overstated by roughly a fifth, on top of the fifth the stub test already removed.

### The guest collision repair writes a bit the engine already replicates

The live work recorded a real failure: on release and reacquire the guest's pawn lost world and actor
collision, stayed non-colliding, fell, and could not move. The repair calls native `Actor.SetCollisionType`
and then writes `Actor.bCollideWorld` directly, because `SetCollisionType` does not restore it.

That is true about the method and the wrong layer to fix it at. The collision contract on `Actor`:

| property | `CPF_Net` |
| --- | --- |
| `bCollideWorld` | **yes** |
| `bCollideActors` | **yes** |
| `bBlockActors` | **yes** |
| `bProjTarget` | yes |
| `Role`, `RemoteRole` | yes |
| `CollisionType` | no |
| `CollisionComponent` | no |

**Every flag the repair writes locally is a replicated property.** So the guest's value is not the engine's
idea of the guest's value: on an authority-side release and reacquisition the property stream re-sends these
bits, and a local write is either overwritten by the next update or diverges until one arrives. A repair that
works for a moment and then is not there is exactly what a local write of a replicated property looks like.

**There is no shipped force-update call.** `Actor.bForceNetUpdate` and `Actor.bNetDirty` exist on all three
builds (both `CPF_RepNotify`, both correctly local), but neither is read by any script function:
`bNetDirty`'s only reader is `Actor.ForceNetRelevant`; `bForceNetUpdate` has no reader by export index and no
FName mention outside `GameInfo.HandleSeamlessTravelPlayer` on LE3. `ForceNetUpdate`, `ForceRelevant` and
`SetNetUpdateFrequency` are not exported from `Engine.pcc` at all. Consistent with section 14: 98% of engine
properties are local and the replication decisions are native.

**So the repair is a display fix, not the fix.** The durable correction is on the authority — make the host's
copy of the guest pawn's collision right and let the existing replication carry it, which is the same free
path sections 14 and 15 rely on. If the host's copy is already correct and the guest still shows the wrong
value, that is a channel/relevancy question, and no guest-side write settles it.

**What is not established:** whether the host's copy is in fact correct at that moment. That needs a
two-process read of three bits, four times (healthy, released, reacquired, post-repair), on both processes.
It is the one measurement the repair needs and the packages cannot supply.

## 20. The guest's team is the right lever, and half of the team path is local

The live work's most concrete combat finding: a guest's `PRI.Team` was null, `GetTeamNum` mapped that to 255,
stock hostility logic rejected an unknown team, and after registering the guest as host team 0 through
`SFXGame.ChangeTeam` near-hits started reducing a real `SFXPawn_Trooper_0` from 512.5 to 194.05. Measuring the
contract around that:

| item | location | size / flags | replicates |
| --- | --- | --- | --- |
| `PlayerReplicationInfo.Team` | `Engine.pcc` | ObjectProperty | **yes, all builds** |
| `Pawn.Health` | `Engine.pcc` | IntProperty | yes |
| `Actor.bProjTarget` | `Engine.pcc` | BoolProperty | yes |
| `RestartPlayer.TeamNum` | `Engine.pcc` | IntProperty | **no** |
| `ChoosePlayerStart.Team` | `Engine.pcc` | ByteProperty | **no** |
| `Actor.GetTeamNum` | `Engine.pcc` | **2-byte native stub** | — |
| `Pawn.GetTeamNum` | `Engine.pcc` | **2-byte native stub** | — |
| `PlayerReplicationInfo.GetTeamNum` | `Engine.pcc` | **2-byte native stub** | — |
| `PlayerController.ChangeTeam` | `Engine.pcc` | 188 / 188 / 194 B script | — |
| `GameInfo.ChangeTeam` | `Engine.pcc` | 9 B script — a forwarder | — |
| `SFXGame.ChangeTeam` | `SFXGame.pcc` | **358 B** script (LE3) | — |
| `PlayerReplicationInfo.SetPlayerTeam` | `Engine.pcc` | 43 / 43 / 46 B script | — |
| `SFXGRI.ModifyDamage` | `SFXGame.pcc` | 178 B script (LE3) | server-side gate |
| `SFXGame.ModifyDamage` | `SFXGame.pcc` | 2,483 B script | server-side gate |
| `BioPawn.ModifyDamage` | `SFXGame.pcc` | 561 B script (LE3) | per-pawn |

Four things follow, and they are cheap to act on:

1. **`PRI.Team` replicates, so the team registration was always going to travel.** Setting it on the host
   is sufficient; the guest's copy follows. That is why the approach worked after registration, and it means
   the guest-side re-registration the live work performs on scene release and reacquisition is belt and
   braces rather than the mechanism. Worth saying so, because a repair that is re-run "because it worked" is a
   repair nobody can reason about later.
2. **Every `GetTeamNum` is native.** The team decision is not readable and not overridable in script on any
   build — it is a native stub on `Actor`, `Pawn`, `Controller`, `PlayerReplicationInfo`, `Projectile` and
   `TeamInfo`. So the only script-level lever is setting `Team`, which is exactly what the code does. That
   makes `ChangeTeam` the whole mechanism, not one option among several.
3. **`RestartPlayer.TeamNum` and `ChoosePlayerStart.Team` are local.** Anything that derives a spawn from a
   team is a host-only decision, so a guest's own copy of those fields will disagree and must not be trusted
   on the guest.
4. **`SFXGame.ChangeTeam` is the LE3 override at 358 bytes, over `GameInfo.ChangeTeam`'s 9-byte forwarder.**
   So the call the code makes reaches the campaign's own logic, not the stock stub — the right entry point.

**What this still does not establish**, and it is the same gap the live work reports: whether a guest hit is
admitted by the server in a full combat sequence. `ModifyDamage` is a 2,483-byte server-side gate whose inputs
are `injured`, `instigatedBy`, `DamageCauser`, `Info` and `PC`, and none of those is replicated to the guest
in the direction that would let a guest decide. That is consistent with "the host must adjudicate", and it is
the design the mod already has — but it is a design statement, not a measurement.

## 21. The installed executable's code is encrypted at rest, so no offline pin check can run on it

A finding that changes what "the build does not match" means. Measured on the installed
`MassEffect3.exe` (`72047e65…`, 26,896,672 bytes):

| section | entropy | reading |
| --- | --- | --- |
| `.text` (17 MB) | **8.0000 bits/byte** | random — not code |
| `.data` (7.4 MB) | **7.9996** | random — not data |
| `.rdata` (7.2 MB) | 4.54 | **plaintext** — holds Bioware source paths |
| `.pdata`, `.reloc` | 6.77, 5.73 | structured |

Controls on the same machine: `notepad.exe` `.text` 6.24, `kernel32.dll` 6.26, with hundreds of
`48 83 ec 28` prologues. The installed LE3 `.text` has zero of those in 17 MB, zero duplicated 32-byte windows
in 4 MB, and 38.3% printable bytes — the random baseline (95/256 = 37.1%), not text.

**The file on disk is not the code the game runs.** It decrypts in memory, through an exe wrapper or the EA
App's licence path. Consequences, in order of how much they change:

1. **"9 pins mismatch" against this file is a statement about the file, not about the build.** No offline byte
   comparison of this executable can validate anything about the mod. `tools/check_pins.py` now detects the
   condition and reports `UNREADABLE` (exit 2) instead of mismatches, and `--generate` refuses rather than
   writing random bytes as pins — a generated pin from an encrypted image would pass, which is worse than none.
2. **The build-identity gap is not "find the right exe".** Even a correct Steam install of this game has
   encrypted `.text`. The supported build can only be identified from a **decrypted image**, which means the
   supported SHA-256 `b453871e…` describes a build the owner has in a form this machine does not — or the pins
   were taken from a decrypted image the repository does not record how it obtained.
3. **The in-process guard is the only check that can work.** `supported_image()` reads memory after the game
   has decrypted itself, so its timestamp and 7-byte check operate on real code. The launcher's SHA-256 gate
   plus the in-process guard together are therefore the design, and neither alone is sufficient.

The classifier that decides "encrypted" is tested, because its failure mode is silently wrong in both
directions: a section below 64 KiB is *unknown* not encrypted, a uniform byte ramp has entropy 8.0 but is
plaintext (detected by small-lag self-match), and a printable-ratio test was rejected because random bytes and
a uniform ramp both measure 37%.

## What this does not tell you
- Nothing here is a game result. No guest ran.
- Whether a guest pawn survives a real level transition is untested. Section 1 says it cannot survive
  by seamless travel; it says nothing about what a from-scratch re-spawn does.
- The recruitment path's control flow is not decoded; only the references inside it are measured.
- Cross-build facts about LE1/LE2 come from their packages, not from a running LE1/LE2 session.
- Section 18 measures that the engine has a per-client cinematic call and that nothing reaches it for a
  guest; it does not measure what happens to the guest's pawn when the scene *ends*. The way out is where
  this project has recorded the damage before.
