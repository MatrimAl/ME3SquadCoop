"""Negative controls for the current-world observer; no game is launched."""
import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))
from authoritative_enemy_sampler import MockMemoryReader, OFFSET_GWORLD
from gameplay_entry_observer import observe_entry, evaluate_entry_interval, OFFSET_GENGINE
from test_authoritative_enemy_sampler import TestWorldFixture


class GameplayEntryObserverTests(unittest.TestCase):
    def setUp(self):
        self.memory = MockMemoryReader()
        f = self.fixture = TestWorldFixture(self.memory)
        # Extend the existing live-object fixture with a real current-map chain.
        package_class = f.create_class("Package", f.class_object)
        package = self.package = f.setup_object(0x60000000, "BioP_ProMar", package_class)
        self.memory.write_ptr(f.world_ptr + 0x40, package)
        info_class = f.create_class("BioWorldInfo", f.class_worldinfo)
        self.memory.write_ptr(f.world_info_ptr + 0x50, info_class)
        engine_class = f.create_class("Engine", f.class_object)
        player_class = f.create_class("LocalPlayer", f.class_object)
        pawn_class = f.create_class("SFXPawn_Player", f.class_biopawn)
        manager_class = f.create_class("SFXGameModeManager", f.class_object)
        self.engine = f.setup_object(0x60001000, "Engine_0", engine_class)
        self.player = f.setup_object(0x60002000, "Player_0", player_class)
        self.pc = f.setup_object(0x60003000, "Controller_0", f.class_playercontroller, f.level_ptr)
        self.pawn = f.setup_object(0x60004000, "Shepard", pawn_class, f.level_ptr)
        self.manager = f.setup_object(0x60005000, "ModeManager", manager_class)
        self.memory.write_ptr(f.base + OFFSET_GENGINE, self.engine)
        self.memory.write_tarray(self.engine + 0x498, [self.player], 0x60006000)
        for address, value in ((self.player + 0x68, self.pc), (self.pc + 0x67C, self.player),
                               (self.pc + 0x198, f.world_info_ptr), (self.pc + 0x3BC, self.pawn),
                               (self.pawn + 0x374, self.pc), (self.pawn + 0x198, f.world_info_ptr),
                               (self.pc + 0xC38, self.manager)):
            self.memory.write_ptr(address, value)
        self.memory.write_byte(f.world_info_ptr + 0x7C8, 0)
        self.memory.write_float(f.world_info_ptr + 0x74C, 10)

    def sample(self):
        return observe_entry(self.memory, self.fixture.base)

    def test_current_chain_and_clear_gates_do_not_claim_campaign(self):
        row = self.sample()
        self.assertEqual(row["status"], "ok", row)
        self.assertTrue(row["current_map_verified"])
        self.assertEqual(row["world_path"], "BioP_ProMar.TheWorld")
        self.assertTrue(row["ready_for_in_place_listen"])
        self.assertFalse(row["playable_coop_verified"])

    def test_loaded_map_is_distinct_from_missing_controller(self):
        self.memory.write_ptr(self.player + 0x68, 0)
        row = self.sample()
        self.assertTrue(row["current_map_verified"])
        self.assertEqual(row["status"], "unknown")
        self.assertFalse(row["ready_for_in_place_listen"])

    def test_unreadable_pause_is_unknown_not_unpaused(self):
        self.memory.set_fail_address(self.fixture.world_info_ptr + 0x7C0)
        row = self.sample()
        self.assertEqual(row["status"], "unknown")
        self.assertFalse(row["control_gates_clear"])

    def test_current_world_is_required_not_unrelated_loaded_object(self):
        self.memory.write_ptr(self.fixture.base + OFFSET_GWORLD, 0)
        self.assertFalse(self.sample()["current_map_verified"])

    def test_similar_package_name_is_not_the_requested_map(self):
        self.memory.write_uint32(self.package + 0x48, self.fixture.add_name("BioP_ProMarExtra"))
        row = self.sample()
        self.assertEqual(row["status"], "ok")
        self.assertFalse(row["current_map_verified"])
        self.assertFalse(row["control_gates_clear"])

    def test_live_map_case_and_sfx_player_hierarchy_are_supported(self):
        self.memory.write_uint32(self.package + 0x48, self.fixture.add_name("Biop_ProMar"))
        row = self.sample()
        self.assertEqual(row["status"], "ok", row)
        self.assertTrue(row["current_map_verified"])
        self.assertTrue(row["ready_for_in_place_listen"])
        # A generic BioPawn is not enough to prove host player control.
        self.memory.write_ptr(self.pawn + 0x50, self.fixture.class_biopawn)
        row = self.sample()
        self.assertEqual(row["status"], "unknown")
        self.assertFalse(row["ready_for_in_place_listen"])

    def test_active_and_unreadable_conversation_are_not_clear_control(self):
        f = self.fixture
        klass = f.create_class("BioConversationManager", f.class_object)
        conversation = f.setup_object(0x60007000, "ConversationManager", klass)
        self.memory.write_ptr(f.world_info_ptr + 0x948, conversation)
        self.memory.write_tarray(conversation + 0xC8, [self.pc], 0x60008000)
        row = self.sample()
        self.assertEqual(row["status"], "ok")
        self.assertIn("active_conversation", row["blockers"])
        self.assertFalse(row["control_gates_clear"])
        self.memory.set_fail_address(conversation + 0xC8)
        row = self.sample()
        self.assertEqual(row["status"], "unknown")
        self.assertFalse(row["control_gates_clear"])

    def test_players_only_mode_cinematic_and_pauser_block(self):
        for address, value, kind, reason in (
                (self.fixture.world_info_ptr + 0x7C0, 0x400, "uint32", "players_only"),
                (self.manager + 0xA8, 8, "byte", "non_ordinary_mode"),
                (self.pc + 0x77C, 0x20000, "uint32", "cinematic_mode"),
                (self.fixture.world_info_ptr + 0x670, self.pc, "ptr", "pauser_present")):
            with self.subTest(reason=reason):
                getattr(self.memory, "write_" + kind)(address, value)
                row = self.sample()
                self.assertFalse(row["ready_for_in_place_listen"])
                self.assertIn(reason, row["blockers"])
                getattr(self.memory, "write_" + kind)(address, 0)

    def test_changed_identity_during_snapshot_is_rejected(self):
        original = self.memory.read
        count = 0
        def changing(address, size):
            nonlocal count
            if address == self.pawn and size == 0x60:
                count += 1
                if count == 4:
                    self.memory.write_uint32(self.pawn + 0x48, 0)
            return original(address, size)
        self.memory.read = changing
        row = self.sample()
        self.assertEqual(row["status"], "unknown", row)
        self.assertFalse(row["ready_for_in_place_listen"])

    def test_native_driver_or_pending_travel_prevents_listen_readiness(self):
        for address in (self.fixture.world_ptr + 0x120, self.engine + 0x9FC):
            self.memory.write_ptr(address, self.engine)
            row = self.sample()
            self.assertTrue(row["control_gates_clear"])
            self.assertFalse(row["ready_for_in_place_listen"])
            self.memory.write_ptr(address, 0)

    def test_interval_requires_identity_continuity_and_advancing_world_time(self):
        row = self.sample()
        rows = [dict(copy.deepcopy(row), elapsed_seconds=n * 0.5, world_time=10 + n * 0.5) for n in range(5)]
        self.assertTrue(evaluate_entry_interval(rows))
        frozen = [dict(item, world_time=10) for item in rows]
        self.assertFalse(evaluate_entry_interval(frozen))
        interrupted = copy.deepcopy(rows)
        interrupted[2]["ready_for_in_place_listen"] = False
        self.assertFalse(evaluate_entry_interval(interrupted))
        reused = copy.deepcopy(rows)
        reused[2]["pawn_identity"]["fname_number"] += 1
        self.assertFalse(evaluate_entry_interval(reused))
        gaps = [dict(rows[0], elapsed_seconds=0), dict(rows[1], elapsed_seconds=10)]
        self.assertFalse(evaluate_entry_interval(gaps))


if __name__ == "__main__":
    unittest.main()
