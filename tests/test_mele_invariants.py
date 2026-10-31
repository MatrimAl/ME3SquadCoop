"""Offline campaign-invariant checks. Read-only on the installed packages; launches nothing.

These are the checks behind [CAMPAIGN-INVARIANTS.md](../docs/CAMPAIGN-INVARIANTS.md). They read the
shipped `.pcc` packages and assert the measurements the guest work depends on, so a claim in the
documentation cannot drift away from the data without a failure here.

Two rules borrowed from the project's own history:

* **A check that cannot fail is worse than none.** Every assertion compares against a specific value
  or a specific ratio, never against "something was found".
* **A zero from a tool that could not have found the value is not a measurement.** The commands
  behind these tests exit 2 when no install is present, and that is translated into a skip rather
  than an assertion, because "0 CDOs closed exactly" is what a missing install looks like.

No game is modified. No process is started. When no install exists the whole module skips.
"""
import importlib.util
import io
import os
from pathlib import Path
import sys
import unittest
from contextlib import redirect_stdout

PROJECT = Path(__file__).resolve().parents[1]
TOOLS = PROJECT / "tools" / "melepkg"
sys.path.insert(0, str(TOOLS))

import mele_props  # noqa: E402


def load_tables():
    return mele_props.load_tables()


def install_present():
    """True when at least one build's Engine.pcc can be found, without touching any package."""
    try:
        tables = load_tables()
        package = tables.load_pkg()
    except Exception:
        return False
    for game in ("ME1", "ME2", "ME3"):
        for root in package.game_dirs():
            candidate = Path(root) / "Game" / game / "BioGame" / "CookedPCConsole" / "Engine.pcc"
            if candidate.is_file():
                return True
    return False


INSTALL = install_present()
needs_install = unittest.skipUnless(INSTALL, "no Legendary Edition install present")


def run(command):
    """Run one invariant command and return (exit code, captured stdout)."""
    buffer = io.StringIO()
    with redirect_stdout(buffer):
        code = command(None)
    return code, buffer.getvalue()


class PackageReaderScopeTests(unittest.TestCase):
    """The reader's own scope limit, which everything else depends on being honest about."""

    @needs_install
    def test_cdo_layout_closes_every_object_and_rival_does_not(self):
        tables = load_tables()
        package = tables.load_pkg()
        path = None
        for root in package.game_dirs():
            candidate = Path(root) / "Game" / "ME3" / "BioGame" / "CookedPCConsole" / "SFXGame.pcc"
            if candidate.is_file():
                path = candidate
                break
        self.assertIsNotNone(path, "ME3 SFXGame.pcc not found")
        table = tables.build(str(path))
        objects = mele_props._candidates(table, 40)
        self.assertGreaterEqual(len(objects), 10)
        best, results = mele_props.pick_layout(table, table["buf"], objects)
        exact, tried, _errors = results["start=+4 (measured)"]
        rival, rival_tried, _e = results["start=+0 (refuted)"]
        self.assertEqual(best, "start=+4 (measured)")
        self.assertEqual(exact, tried, "measured layout must close every CDO")
        # The rival must close visibly fewer, otherwise the check cannot discriminate.
        self.assertLessEqual(rival, exact * 0.10)

    @needs_install
    def test_live_level_actors_are_not_readable(self):
        tables = load_tables()
        package = tables.load_pkg()
        path = None
        for root in package.game_dirs():
            candidate = Path(root) / "Game" / "ME3" / "BioGame" / "CookedPCConsole" / "BioA_Cat002.pcc"
            if candidate.is_file():
                path = candidate
                break
        self.assertIsNotNone(path, "ME3 BioA_Cat002.pcc not found")
        table = tables.build(str(path))
        objects = mele_props._candidates(table, 40)
        _best, results = mele_props.pick_layout(table, table["buf"], objects)
        exact, tried, _errors = results["start=+4 (measured)"]
        self.assertGreater(tried, 0)
        self.assertLess(exact, tried, "a live level must NOT close fully - the scope limit is real")


class CampaignInvariantTests(unittest.TestCase):
    """The documented invariants, asserted with the command that prints them."""

    @needs_install
    def test_no_build_enables_seamless_travel(self):
        code, out = run(mele_props.cmd_coopcheck)
        self.assertEqual(code, 0, out)
        for game in ("ME1", "ME2", "ME3"):
            line = [l for l in out.splitlines() if l.strip().startswith(game) and "CDOs closed" in l]
            self.assertTrue(line, "%s line missing:\n%s" % (game, out))
            self.assertIn("declared=True", line[0])
            self.assertIn("0 set it", line[0])
            closed = int(line[0].split()[2])
            self.assertGreater(closed, 1000, "%s scan saw too few objects" % game)

    @needs_install
    def test_me3_clamps_capacity_and_me1_me2_do_not(self):
        code, out = run(mele_props.cmd_coopcheck)
        self.assertEqual(code, 0, out)
        self.assertIn("Default__SFXGame       MaxPlayers=-    MaxPlayersAllowed=1", out)
        for game in ("ME1", "ME2"):
            line = [l for l in out.splitlines() if l.strip().startswith(game) and "Default__GameInfo" in l]
            self.assertTrue(line, "%s base CDO line missing" % game)
            self.assertIn("MaxPlayersAllowed=32", line[0])

    @needs_install
    def test_roster_element_type_differs_and_memberdata_is_gone_on_le3(self):
        code, out = run(mele_props.cmd_coopcheck)
        self.assertEqual(code, 0, out)
        self.assertIn("ME1  Members is TArray<StructProperty>, MemberData present (17 fields)", out)
        self.assertIn("ME2  Members is TArray<StructProperty>, MemberData present (7 fields)", out)
        self.assertIn("ME3  Members is TArray<ObjectProperty>, MemberData ABSENT", out)

    @needs_install
    def test_possession_and_dialogue_selection_are_shared(self):
        code, out = run(mele_props.cmd_coopcheck)
        self.assertEqual(code, 0, out)
        for game in ("ME1", "ME2", "ME3"):
            possession = [l for l in out.splitlines()
                          if l.strip().startswith(game) and "BioAiController=" in l]
            self.assertTrue(possession, "%s possession line missing" % game)
            self.assertIn("Controller=PU", possession[0])
            self.assertIn("BioAiController=PU", possession[0])
            dialogue = [l for l in out.splitlines()
                        if l.strip().startswith(game) and "selection surface" in l]
            self.assertTrue(dialogue, "%s dialogue line missing" % game)
            self.assertIn("SelectConversationEntry", dialogue[0])

    @needs_install
    def test_le1_saves_the_actor_graph_and_le3_does_not(self):
        code, out = run(mele_props.cmd_savecheck)
        self.assertEqual(code, 0, out)
        self.assertIn("ME1 BioLevelSaveObject carries m_LevelObjects", out)
        self.assertIn("ME2   0 class(es) ending in SaveObject", out)
        self.assertIn("ME3   0 class(es) ending in SaveObject", out)
        self.assertIn("ME3  HenchmanSaveRecord 9 field(s)", out)

    @needs_install
    def test_le3_recruits_through_a_data_table_and_le1_does_not(self):
        code, out = run(mele_props.cmd_squadadd)
        self.assertEqual(code, 0, out)
        self.assertIn("ME1  AddToParty.Activated 842 bytes; Bio2DA refs 0", out)
        self.assertIn("ME3  AddToParty.Activated 1186 bytes; Bio2DA refs 3", out)
        # ME3 dropped the roster-size API that LE2 consults during recruitment.
        self.assertIn("GetSquadSize", out)


    @needs_install
    def test_story_surface_is_shared(self):
        """Where story progression lives at campaign scale, and where it does not.

        `BioSeqAct_PMBase` and its three concrete subclasses carry the same class and property names
        on all three builds, so "the host advances the story and the guest observes" has one
        interception point rather than three.

        The trigger-volume finding is asserted too because it corrects a natural assumption: a
        `TriggerVolume` that moves the player to another area is used three times in total across
        6,644 maps, against 56,568 placements of `PMExecuteTransition`. A hook on trigger volumes
        follows almost nothing.
        """
        import re
        index_dir = os.environ.get("MELE_INDEX_DIR") or str(
            PROJECT / "local" / "MELE" / "index")
        if not os.path.isfile(os.path.join(index_dir, "ME1.json")):
            self.skipTest("no package index (storycheck needs every package; set MELE_INDEX_DIR)")
        buffer = io.StringIO()
        with redirect_stdout(buffer):
            code = mele_props.cmd_storycheck(None)
        out = buffer.getvalue()
        self.assertEqual(code, 0, out)
        self.assertIn("PM action family is the campaign's story surface", out)
        for game in ("ME1", "ME2", "ME3"):
            match = re.search(r"=== %s:([^\n]*)\n(.*?)(?==== ME|\Z)" % game, out, re.S)
            self.assertIsNotNone(match, "%s block missing:\n%s" % (game, out))
            section = match.group(2)
            self.assertIn("BioSeqAct_PMExecuteTransition", section)
            self.assertIn("BioSeqAct_PMCheckState", section)
        le1 = re.search(r"=== ME1:.*?(?==== ME2|\Z)", out, re.S).group(0)
        volume = [l for l in le1.splitlines() if "BioAreaTransition" in l]
        if volume:
            placed = int(volume[0].split()[1])
            self.assertLessEqual(placed, 50,
                                 "LE1 trigger volume usage changed: %s" % volume[0].strip())

    @needs_install
    def test_story_state_is_bounded_and_le3_keeps_it_in_one_struct(self):
        """The sync budget: how much state the story surface in section 7 actually produces.

        `PlotManager.pcc` on LE1/LE2 defines one class whose body is `F<n>` plot-flag accessors, so the
        flag set is a bounded integer array and a guest can be handed all of it rather than a delta log.

        LE3 has no accessor class, and the first reading took that for absence of the state. It is
        asserted here that the state exists and has one serialization point, because "nothing to sync"
        would be a plan built on a gap that is not real.
        """
        code, out = run(mele_props.cmd_plotcheck)
        self.assertEqual(code, 0, out)
        import re
        for game in ("ME1", "ME2"):
            line = re.search(r"^%s\s+(\d+) plot flag" % game, out, re.M)
            self.assertIsNotNone(line, "%s row missing:\n%s" % (game, out))
            self.assertGreater(int(line.group(1)), 1000,
                               "%s flag count collapsed: %s" % (game, line.group(0)))
        self.assertIn("ME3  PlotManager.pcc present: False", out)
        self.assertIn("PlotTableSaveRecord", out)
        for array in ("BoolVariables", "IntVariables", "FloatVariables", "QuestIDs", "CodexIDs"):
            self.assertIn(array, out)
        self.assertIn("IntVariablePair = 8 bytes", out)
        self.assertIn("Bool_CutoffIndex=17655", out)
        # The budget claim itself: cheap enough to ship whole on every build.
        self.assertIn("~8 KB", out)
        self.assertIn("small enough to ship", out)


    @needs_install
    def test_join_chain_is_script_and_le3_has_a_guest_pawn_spawn(self):
        """Section 9: accepting a second player is a class override, not a native patch.

        This is the claim the whole guest plan rests on, so it is asserted at both ends: the chain
        functions must be in script on all three builds, and LE3 must still expose a spawn that takes a
        `PlayerController` and returns a pawn. Either one failing changes the plan rather than a detail.
        """
        code, out = run(mele_props.cmd_joincheck)
        self.assertEqual(code, 0, out)
        for game in ("ME1", "ME2", "ME3"):
            line = [l for l in out.splitlines() if l.strip().startswith(game) and "9/9 in script" in l]
            self.assertTrue(line, "%s join chain is not fully in script:\n%s" % (game, out))
            for fn in ("Login", "AtCapacity", "RestartPlayer"):
                self.assertIn(fn, line[0])
        self.assertIn("SFXGameInfoSP lives in BIO_COMMON.pcc", out)
        self.assertIn("MPContent", out)
        self.assertIn("SpawnNewPlayerPawn: 17 parameter(s)", out)
        for fn in ("InitializeTeams", "ChangeTeam", "GetPlayerCount"):
            self.assertIn(fn, out)


    @needs_install
    def test_guest_pawn_needs_a_character_description_and_le1_has_none(self):
        """Section 9: what "the client takes a squadmate" actually requires.

        Two payloads and one function decide it. `PlayerInfoEx` is the identity block (7/8/9 fields);
        `PlotTableSaveRecord` is the plot payload, the same struct on all three builds and held only by
        `SFXSaveGame.PlotRecord`.

        And the campaign's own spawn takes the identity block on LE2/LE3 but not on LE1. That asymmetry
        is asserted rather than noted, because it is the difference between "call the existing spawn with
        a different character" and "there is no such call to make".
        """
        code, out = run(mele_props.cmd_spawncheck)
        self.assertEqual(code, 0, out)
        for game, count in (("ME1", 7), ("ME2", 8), ("ME3", 9)):
            line = [l for l in out.splitlines()
                    if l.strip().startswith(game) and "PlayerInfoEx" in l]
            self.assertTrue(line, "%s PlayerInfoEx row missing:\n%s" % (game, out))
            self.assertIn("%2d field(s)" % count, line[0])
        for game in ("ME1", "ME2", "ME3"):
            holder = [l for l in out.splitlines()
                      if l.strip().startswith(game) and "SFXSaveGame.PlotRecord" in l]
            self.assertTrue(holder, "%s plot payload holder missing:\n%s" % (game, out))
        self.assertIn("ME1  BioSPGame .SpawnDefaultPawnFor  5 param(s), parms 0x28, PlayerInfo: False", out)
        self.assertIn("ME2  SFXGame   .SpawnDefaultPawnFor 29 param(s), parms 0xdc, PlayerInfo: True", out)
        self.assertIn("ME3  SFXGame   .SpawnDefaultPawnFor 33 param(s), parms 0xf4, PlayerInfo: True", out)


    @needs_install
    def test_roster_is_one_call_over_two_different_stores(self):
        """Section 3: the store differs per build, the call does not.

        `Members` holds a `MemberData` record on LE1/LE2 and the `Pawn` itself on LE3, which reads like
        three different edits. `AddMember(Pawn) -> int` exists on every build, so adding the guest is one
        call either way; the store decides what the entry *means*, not how much work it is.

        The notification differences are pinned too, because they are the part a co-op mod has to fill in:
        only LE1 has `MemberAdded`, only LE1 has a spare-controller slot, only LE3 has `SetPlayerPawn`.
        """
        code, out = run(mele_props.cmd_rostercheck)
        self.assertEqual(code, 0, out)
        for game in ("ME1", "ME2"):
            line = [l for l in out.splitlines() if l.strip().startswith(game) and "Members is" in l]
            self.assertTrue(line, "%s store row missing:\n%s" % (game, out))
            self.assertIn("element struct MemberData", line[0])
        me3 = [l for l in out.splitlines() if l.strip().startswith("ME3") and "Members is" in l]
        self.assertTrue(me3, "LE3 store row missing:\n%s" % out)
        self.assertIn("element class Pawn", me3[0])
        self.assertIn("BioBaseSquad   AddMember            (Pawn", out)
        self.assertIn("ME1  MemberAdded(notification); spare-controller slot", out)
        self.assertIn("ME3  no MemberAdded - adding a member is silent; SetPlayerPawn", out)


    @needs_install
    def test_guest_has_no_spawn_point_of_its_own(self):
        """Section 11: where a second player could stand.

        The campaign spawns from `BioStartLocation`, not `PlayerStart`, so a count of `PlayerStart` alone
        reported zero spawn points almost everywhere. Pinned here: the spawn class, that the map totals
        are plausible, and that of the maps that have a spawn point LE3 mostly has only one - which is
        what makes `bNoCollisionFail` necessary rather than optional.
        """
        index_dir = os.environ.get("MELE_INDEX_DIR") or str(
            PROJECT / "local" / "MELE" / "index")
        if not os.path.isfile(os.path.join(index_dir, "ME3.json")):
            self.skipTest("no package index (startcheck needs every map; set MELE_INDEX_DIR)")
        code, out = run(mele_props.cmd_startcheck)
        self.assertEqual(code, 0, out)
        import re
        lines = out.splitlines()
        for game in ("ME1", "ME2", "ME3"):
            idx = [i for i, l in enumerate(lines) if l.startswith(game)]
            self.assertTrue(idx, "%s row missing:\n%s" % (game, out))
            self.assertIn("BioStartLocation", lines[idx[0] + 1])
            maps = int(re.search(r"(\d+) map\(s\)", lines[idx[0]]).group(1))
            with_start = int(re.search(r"(\d+) with a spawn point", lines[idx[0]]).group(1))
            self.assertGreater(maps, 1000, "%s map count collapsed" % game)
            self.assertGreater(with_start, 0)
            self.assertLessEqual(with_start, maps)
        self.assertIn("SpawnDefaultPawnFor's bNoCollisionFail", out)


    @needs_install
    def test_capability_flags_are_local_and_combat_state_replicates(self):
        """Section 12: the campaign's replication info, and the one thing it does not give us.

        `SFXGRISP` is a zero-override alias of a 35-property, 32-function `SFXGRI`, so LE3's campaign
        runs on multiplayer-shaped state. The tempting conclusion - that `bCanSave` and friends are
        replicated capability switches a host can flip - is false, and this test asserts the negative so
        a plan cannot be built on it: exactly four properties carry `CPF_Net`, and none is a capability
        flag. If that ever changes, the gating plan gets simpler and this fails to say so.
        """
        code, out = run(mele_props.cmd_gricheck)
        self.assertEqual(code, 0, out)
        self.assertIn("GameReplicationInfoClass=<BioGameReplicationInfo>", out)
        self.assertIn("inherited (stock GameReplicationInfo)", out)
        self.assertIn("GameReplicationInfoClass=<SFXGRISP>", out)
        self.assertIn("SFXGRISP                  0 property(ies),  0 function(s)", out)
        self.assertIn("35 property(ies), 32 function(s)", out)
        self.assertIn("closure EXACT", out)
        self.assertIn("replicated: MaxUniquePlayers, WaveCoordinator, bForceCombat, bInCombat", out)
        local = [ln for ln in out.splitlines() if "capability flags present and local-only" in ln]
        self.assertTrue(local, "local-only capability flags were not reported:\n%s" % out)
        for flag in ("bCanSave", "bCanShowJournal", "bCanShowCodex", "bCanShowMap"):
            self.assertIn(flag, local[0])
        self.assertIn("CopyProperties       (OldGRI)", out)


    @needs_install
    def test_routing_knows_no_multiplayer_class(self):
        """Section 13: the map-prefix table, and the player stack it routes to.

        Three prefixes, all `bIsMultiplayer=False`, one `SP` alias. So the engine will not route a campaign
        map to a networked class by itself - and the `Bio` row with `bUsesCommonPackage=True` is why the
        MPContent-derived `BIO_COMMON.pcc` loads for every campaign map.

        The player stack is asserted in the same test because it is the other half of the same question:
        `SFXGame` sets the controller, PRI and HUD classes, so a player the engine spawns needs no co-op
        code to have a working HUD.
        """
        code, out = run(mele_props.cmd_prefixcheck)
        self.assertEqual(code, 0, out)
        self.assertIn("Default__GameInfo: 35 tag(s), closure EXACT", out)
        self.assertIn("DefaultMapPrefixes: 3 row(s)", out)
        self.assertNotIn("mp=True", out)
        self.assertIn("Bio            -> SFXGameContent.SFXGameInfoSP           common=True  mp=False", out)
        self.assertIn("GameInfoClassAliases: 1 row(s)", out)
        self.assertIn("SP     -> SFXGameContent.SFXGameInfoSP", out)
        self.assertIn("PlayerControllerClass        <SFXPlayerController>", out)
        self.assertIn("PlayerReplicationInfoClass   <SFXPRI>", out)
        self.assertIn("HUDType                      <BioHUD>", out)
        self.assertIn("MaxPlayersAllowed            1", out)


    @needs_install
    def test_pri_is_per_player_and_mp_shaped(self):
        """Section 12's other half: the per-player replication info.

        `SFXGame.PlayerReplicationInfoClass=SFXPRI`, so every player gets one. It is multiplayer-shaped:
        credits, points, medals, match consumables, a score-replication period, `FindLocalPRI` (a
        local-versus-remote distinction), and `Server*` RPCs - the server-authoritative pattern. Three of
        its 10 properties replicate.

        That means the engine already models "this player's state" separately from the session's, which is
        what a second player needs, and it is the asset that makes a guest's per-player data cheap.
        """
        code, out = run(mele_props.cmd_gricheck)
        self.assertEqual(code, 0, out)
        self.assertIn("5. The per-player replication info, SFXPRI", out)
        self.assertIn("super: export:SFXPRI_NativeBase", out)
        self.assertIn("10 property(ies), 27 function(s)", out)
        self.assertIn("ReplicatedScoreInfo", out)
        self.assertIn("ServerAddActiveMatchConsumable", out)


    @needs_install
    def test_engine_replicates_the_transform_and_a_player_pawn(self):
        """Section 14: what co-op gets for free.

        Only ~1.8% of the engine's properties replicate, and they are `Actor.Role`/`RemoteRole` with the
        transform plus a complete `Pawn` - so a replicated pawn moves, and the authority pair travels.
        The stock UE3 client-join handshake is also in script on every build.

        `Projectile` replicating nothing is asserted too: weapon fire travels as
        `Pawn.FlashCount`/`FlashLocation` and each client spawns its own projectile, so visuals are free and
        damage must be authority-checked. And the transform's spelling differs per build, which is why the
        lookup is case-insensitive.
        """
        code, out = run(mele_props.cmd_netcheck)
        self.assertEqual(code, 0, out)
        self.assertEqual(out.count("transform spelling"), 3, "spelling not reported per build:\n%s" % out)
        self.assertIn("transform spelling: Actor.Location", out)
        self.assertIn("transform spelling: Actor.location", out)
        self.assertIn("5/5 present: ServerGivePawn, ServerAcknowledgePossession, ServerMove", out)
        self.assertIn("Projectile replicates 0 propert(ies)", out)
        for game in ("ME1", "ME2", "ME3"):
            row = [l for l in out.splitlines() if l.startswith(game)]
            self.assertTrue(row, "%s density row missing" % game)
            self.assertIn("replicate (", row[0])


    @needs_install
    def test_movement_transport_is_inherited_and_possession_is_the_seam(self):
        """Section 14: the most actionable result — the campaign never replaces `ServerMove`.

        `BioPlayerController` has 119-272 script functions, which looks like it owns the presence path. It
        owns `Possess` and the camera; `ServerMove` and `ClientRestart` pass straight through from
        `Engine.PlayerController` on every build. So the movement transport a guest gets is the engine's own,
        and possession is the single overridden seam — which is where "the client takes a squadmate" has to
        hook.
        """
        code, out = run(mele_props.cmd_netcheck)
        self.assertEqual(code, 0, out)
        self.assertIn("the campaign's own controller chain, and what it replaces", out)
        replacing = [ln for ln in out.splitlines() if "replaces:" in ln]
        self.assertTrue(replacing, "no override list reported:\n%s" % out)
        for line in replacing:
            # Only Engine.PlayerController may replace ServerMove; anything further down means the
            # inherited-transport claim no longer holds.
            if "ServerMove" in line and not line.strip().startswith("PlayerController "):
                self.fail("a Bio/SP controller now replaces ServerMove: %s" % line.strip())
        self.assertTrue(any("replaces: Possess" in ln for ln in replacing),
                        "possession is no longer the overridden seam:\n%s" % out)
        self.assertIn("possession is the", out)
        # Squad awareness belongs to LE2/LE3 only, matching the recruitment-path difference in section 3.
        squad = [ln for ln in out.splitlines() if "Possess squad-aware:" in ln]
        self.assertEqual(len(squad), 3, "squad awareness not reported per build:\n%s" % out)
        self.assertTrue(squad[0].strip().endswith("False"), "LE1 became squad-aware: %s" % squad[0])
        self.assertTrue(squad[1].strip().endswith("True"), "LE2 lost PlayerSquad: %s" % squad[1])
        self.assertTrue(squad[2].strip().endswith("True"), "LE3 lost PlayerSquad: %s" % squad[2])
        # The class the campaign actually instantiates keeps a stock-shaped Possess - but only on LE3, where
        # SFXPlayerController overrides possession at all. Matched per line rather than by exact padding.
        stock = [ln for ln in out.splitlines()
                 if "SFXPlayerController" in ln and "Possess" in ln and "parms=0xc" in ln]
        self.assertEqual(len(stock), 1,
                         "SFXPlayerController.Possess shape changed: %r" % stock)
        self.assertIn("bVehicleTransition:BoolProperty", stock[0])
        self.assertNotIn("PlayerSquad", stock[0])


class SkipBehaviourTests(unittest.TestCase):
    """The commands must refuse to report a measurement when there is no install to measure."""

    def test_commands_exit_two_without_an_install(self):
        saved = os.environ.get("MELE_GAME_DIR")
        os.environ["MELE_GAME_DIR"] = str(Path(PROJECT) / "local" / "definitely-not-a-game")
        try:
            for command in (mele_props.cmd_coopcheck, mele_props.cmd_savecheck,
                            mele_props.cmd_squadadd, mele_props.cmd_plotcheck,
                            mele_props.cmd_joincheck, mele_props.cmd_spawncheck,
                            mele_props.cmd_rostercheck, mele_props.cmd_startcheck,
                            mele_props.cmd_gricheck, mele_props.cmd_prefixcheck,
                            mele_props.cmd_netcheck):
                code, out = run(command)
                self.assertEqual(code, 2, "%s did not skip: %s" % (command.__name__, out))
                self.assertIn("skip, not a result", out)
                self.assertNotIn("CDOs closed exactly", out)
        finally:
            if saved is None:
                os.environ.pop("MELE_GAME_DIR", None)
            else:
                os.environ["MELE_GAME_DIR"] = saved


if __name__ == "__main__":
    unittest.main()
