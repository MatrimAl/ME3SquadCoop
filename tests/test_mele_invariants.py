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


class SkipBehaviourTests(unittest.TestCase):
    """The commands must refuse to report a measurement when there is no install to measure."""

    def test_commands_exit_two_without_an_install(self):
        saved = os.environ.get("MELE_GAME_DIR")
        os.environ["MELE_GAME_DIR"] = str(Path(PROJECT) / "local" / "definitely-not-a-game")
        try:
            for command in (mele_props.cmd_coopcheck, mele_props.cmd_savecheck,
                            mele_props.cmd_squadadd):
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
