"""Unit tests for the bounded authoritative enemy health and shield sampler."""
from __future__ import annotations

import io
import json
import math
from pathlib import Path
import struct
import sys
import tempfile
import unittest

# Ensure src/ is on search path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "src"))

from authoritative_enemy_sampler import (
    AuthoritativeEnemySampler,
    MockMemoryReader,
    compute_file_sha256,
    evaluate_damage_decrease,
    verify_isolated_executable,
    OFFSET_ACTOR_FLAGS,
    OFFSET_ACTOR_LOCATION,
    OFFSET_ACTOR_MODULES,
    OFFSET_ACTOR_OWNER,
    OFFSET_ACTOR_WORLD_INFO,
    OFFSET_BIOPAWN_DEAD_FLAGS,
    OFFSET_BIOPAWN_SQUAD,
    OFFSET_CONTROLLER_PRI,
    OFFSET_DAMAGE_CURRENT_HEALTH,
    OFFSET_DAMAGE_MAX_HEALTH,
    OFFSET_DAMAGE_REPLICATED_HEALTH,
    OFFSET_GNAMES,
    OFFSET_GOBJECTS,
    OFFSET_GWORLD,
    OFFSET_INVENTORY_CHAIN,
    OFFSET_INVENTORY_INSTIGATOR,
    OFFSET_INVENTORY_MANAGER,
    OFFSET_INVENTORY_NEXT,
    OFFSET_INVENTORY_OWNER,
    OFFSET_LEVELSTREAMING_FLAGS,
    OFFSET_LEVELSTREAMING_LOADED_LEVEL,
    OFFSET_MODULE_OWNER,
    OFFSET_PAWN_CONTROLLER,
    OFFSET_PAWN_HEALTH_LEGACY,
    OFFSET_PAWN_INVENTORY,
    OFFSET_PAWN_PRI,
    OFFSET_PRI_TEAM,
    OFFSET_SHIELD_CURRENT_VALUE,
    OFFSET_SHIELD_MAX_VALUE,
    OFFSET_SHIELD_RESISTANCE,
    OFFSET_TEAMINFO_TEAM_INDEX,
    OFFSET_UCLASS_CLASS_DEFAULT_OBJECT,
    OFFSET_UCLASS_SUPER_FIELD,
    OFFSET_ULEVEL_ACTORS,
    OFFSET_UOBJECT_CLASS,
    OFFSET_UOBJECT_FNAME,
    OFFSET_UOBJECT_INTERNAL_INDEX,
    OFFSET_UOBJECT_OUTER,
    OFFSET_UWORLD_PERSISTENT_LEVEL,
    OFFSET_WORLDINFO_STREAMING_LEVELS,
    SUPPORTED_EXECUTABLE_SHA256,
)


class TestWorldFixture:
    """Helper to construct a complete, valid synthetic UE3 object/world layout in MockMemoryReader."""

    def __init__(self, reader: MockMemoryReader, base: int = 0x7FF74A890000):
        self.reader = reader
        self.base = base
        self.name_offsets: dict[str, int] = {}
        self.objects: list[int] = []

        # Name pool at base + 0x17B33D0
        self.name_pool_data = 0x10001000
        # Zero-initialize 64KB for name pool
        self.reader.write_bytes(self.name_pool_data, b"\x00" * 0x10000)
        self.reader.write_ptr(self.base + OFFSET_GNAMES, self.name_pool_data)

        # GObjects at base + 0x1887E40
        self.gobjects_data_ptr = 0x20000000
        # Zero-initialize 64KB for GObjects
        self.reader.write_bytes(self.gobjects_data_ptr, b"\x00" * 0x10000)

        # Core Classes
        self.class_object = self.create_class("Object", super_class=0)
        self.class_class = self.create_class("Class", super_class=self.class_object)
        self.class_actor = self.create_class("Actor", super_class=self.class_object)
        self.class_pawn = self.create_class("Pawn", super_class=self.class_actor)
        self.class_biopawn = self.create_class("BioPawn", super_class=self.class_pawn)
        self.class_controller = self.create_class("Controller", super_class=self.class_actor)
        self.class_aicontroller = self.create_class("AIController", super_class=self.class_controller)
        self.class_playercontroller = self.create_class("SFXPlayerController", super_class=self.class_controller)
        self.class_henchmancontroller = self.create_class("SFXAI_Henchman", super_class=self.class_aicontroller)
        self.class_troopercontroller = self.create_class("SFXAI_Trooper", super_class=self.class_aicontroller)
        self.class_world = self.create_class("World", super_class=self.class_object)
        self.class_level = self.create_class("Level", super_class=self.class_object)
        self.class_worldinfo = self.create_class("WorldInfo", super_class=self.class_actor)
        self.class_levelstreaming = self.create_class("LevelStreaming", super_class=self.class_object)
        self.class_squad = self.create_class("BioBaseSquad", super_class=self.class_actor)
        self.class_playersquad = self.create_class("BioPlayerSquad", super_class=self.class_squad)
        self.class_combatsquad = self.create_class("BioSquadCombat", super_class=self.class_squad)
        self.class_pri = self.create_class("PlayerReplicationInfo", super_class=self.class_actor)
        self.class_teaminfo = self.create_class("TeamInfo", super_class=self.class_actor)
        self.class_module = self.create_class("SFXModule", super_class=self.class_object)
        self.class_damage_native = self.create_class("SFXModule_DamageNativeBase", super_class=self.class_module)
        self.class_damage_party = self.create_class("SFXModule_DamageParty", super_class=self.class_damage_native)
        self.class_damage_player = self.create_class("SFXModule_DamagePlayer", super_class=self.class_damage_native)
        self.class_damage_enemy = self.create_class("SFXModule_Damage", super_class=self.class_damage_native)
        self.class_inv_manager = self.create_class("InventoryManager", super_class=self.class_actor)
        self.class_sfx_inv_manager = self.create_class("SFXInventoryManager", super_class=self.class_inv_manager)
        self.class_inventory = self.create_class("Inventory", super_class=self.class_actor)
        self.class_shield_base = self.create_class("SFXShield_Base", super_class=self.class_inventory)
        self.class_shield_energy = self.create_class("SFXShield_Energy_Player", super_class=self.class_shield_base)
        self.class_playersoldier = self.create_class("SFXPawn_PlayerSoldier", super_class=self.class_biopawn)
        self.class_ashley = self.create_class("SFXPawn_Ashley", super_class=self.class_biopawn)
        self.class_trooper = self.create_class("SFXPawn_Trooper", super_class=self.class_biopawn)

        # World setup
        self.world_ptr = 0x30000000
        self.level_ptr = 0x30001000
        self.world_info_ptr = 0x30002000

        self.setup_object(self.world_ptr, "TheWorld", self.class_world)
        self.setup_object(self.level_ptr, "PersistentLevel", self.class_level, outer=self.world_ptr)
        self.setup_object(self.world_info_ptr, "BioWorldInfo_0", self.class_worldinfo, outer=self.level_ptr)

        self.reader.write_ptr(self.base + OFFSET_GWORLD, self.world_ptr)
        self.reader.write_ptr(self.world_ptr + OFFSET_UWORLD_PERSISTENT_LEVEL, self.level_ptr)

        # Actor array in PersistentLevel
        self.level_actors: list[int] = [self.world_info_ptr]
        self.sync_level_actors(self.level_ptr, self.level_actors, data_ptr=0x30005000)

        # WorldInfo pointers
        self.reader.write_ptr(self.world_info_ptr + OFFSET_ACTOR_WORLD_INFO, self.world_info_ptr)
        self.reader.write_tarray(self.world_info_ptr + OFFSET_WORLDINFO_STREAMING_LEVELS, [], 0x30006000)

        self.sync_gobjects()

    def add_name(self, name: str) -> int:
        if name in self.name_offsets:
            return self.name_offsets[name]
        off = len(self.name_offsets) * 128
        self.name_offsets[name] = off
        # Write to name table pool
        entry_addr = self.name_pool_data + off
        raw_name = name.encode("ascii") + b"\0"
        self.reader.write_bytes(entry_addr + 12, raw_name)
        return off

    def create_class(self, name: str, super_class: int = 0) -> int:
        addr = 0x15000000 + len(self.objects) * 0x1000
        self.setup_object(addr, name, 0)
        self.reader.write_ptr(addr + OFFSET_UCLASS_SUPER_FIELD, super_class)
        cdo_addr = addr + 0x500
        self.setup_object(cdo_addr, f"Default__{name}", addr)
        self.reader.write_ptr(addr + OFFSET_UCLASS_CLASS_DEFAULT_OBJECT, cdo_addr)
        return addr

    def setup_object(self, addr: int, name: str, class_ptr: int, outer: int = 0) -> int:
        # Zero-initialize 0x1000 for this object
        self.reader.write_bytes(addr, b"\x00" * 0x1000)
        name_idx = self.add_name(name)
        idx = len(self.objects)
        self.objects.append(addr)
        self.reader.write_int32(addr + OFFSET_UOBJECT_INTERNAL_INDEX, idx)
        self.reader.write_ptr(addr + OFFSET_UOBJECT_OUTER, outer)
        self.reader.write_uint32(addr + OFFSET_UOBJECT_FNAME, name_idx)
        self.reader.write_uint32(addr + OFFSET_UOBJECT_FNAME + 4, 0)
        self.reader.write_ptr(addr + OFFSET_UOBJECT_CLASS, class_ptr)
        self.sync_gobjects()
        return addr

    def sync_gobjects(self):
        self.reader.write_ptr(self.base + OFFSET_GOBJECTS, self.gobjects_data_ptr)
        self.reader.write_int32(self.base + OFFSET_GOBJECTS + 8, len(self.objects))
        self.reader.write_int32(self.base + OFFSET_GOBJECTS + 12, max(len(self.objects), 64))
        for i, addr in enumerate(self.objects):
            self.reader.write_ptr(self.gobjects_data_ptr + i * 8, addr)

    def sync_level_actors(self, level_ptr: int, actors: list[int], data_ptr: int):
        self.reader.write_tarray(level_ptr + OFFSET_ULEVEL_ACTORS, actors, data_ptr)

    def add_damage_module(self, actor_ptr: int, current_health: float = 500.0, max_health: float = 500.0, reciprocal_owner: int = None) -> int:
        mod_ptr = 0x40000000 + len(self.objects) * 0x1000
        self.setup_object(mod_ptr, "DmgMod0", self.class_damage_enemy, outer=actor_ptr)
        owner = actor_ptr if reciprocal_owner is None else reciprocal_owner
        self.reader.write_ptr(mod_ptr + OFFSET_MODULE_OWNER, owner)
        self.reader.write_float(mod_ptr + OFFSET_DAMAGE_CURRENT_HEALTH, current_health)
        self.reader.write_float(mod_ptr + OFFSET_DAMAGE_MAX_HEALTH, max_health)
        self.reader.write_byte(mod_ptr + OFFSET_DAMAGE_REPLICATED_HEALTH, 10)
        self.sync_gobjects()
        return mod_ptr

    def add_shield(self, actor_ptr: int, inv_manager_ptr: int, current_shields: float = 250.0, max_shields: float = 250.0,
                   owner: int = None, instigator: int = None, inv_man: int = None) -> int:
        shield_ptr = 0x50000000 + len(self.objects) * 0x1000
        self.setup_object(shield_ptr, "Shield0", self.class_shield_base, outer=actor_ptr)
        self.reader.write_ptr(shield_ptr + OFFSET_INVENTORY_OWNER, actor_ptr if owner is None else owner)
        self.reader.write_ptr(shield_ptr + OFFSET_INVENTORY_INSTIGATOR, actor_ptr if instigator is None else instigator)
        self.reader.write_ptr(shield_ptr + OFFSET_INVENTORY_MANAGER, inv_manager_ptr if inv_man is None else inv_man)
        self.reader.write_ptr(shield_ptr + OFFSET_INVENTORY_NEXT, 0)
        self.reader.write_float(shield_ptr + OFFSET_SHIELD_CURRENT_VALUE, current_shields)
        self.reader.write_float(shield_ptr + OFFSET_SHIELD_MAX_VALUE, max_shields)
        self.reader.write_byte(shield_ptr + OFFSET_SHIELD_RESISTANCE, 1)
        self.sync_gobjects()
        return shield_ptr


class AuthoritativeEnemySamplerTests(unittest.TestCase):
    def setUp(self):
        self.reader = MockMemoryReader()
        self.fixture = TestWorldFixture(self.reader)
        self.sampler = AuthoritativeEnemySampler(self.reader, self.fixture.base)

    def test_verify_isolated_executable_requires_file(self):
        valid, reason = verify_isolated_executable(Path("non_existent_exe_12345.exe"))
        self.assertFalse(valid)
        self.assertIn("not found", reason)

    def test_verify_isolated_executable_checks_sha256_and_sandbox(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            tmppath = Path(tmpdir)
            sandbox = tmppath / "sandbox"
            sandbox.mkdir()
            fake_exe = sandbox / "MassEffect3.exe"
            fake_exe.write_bytes(b"test game binary content")

            # Check mismatched hash
            valid, reason = verify_isolated_executable(fake_exe, expected_sha=SUPPORTED_EXECUTABLE_SHA256, sandbox_dir=sandbox)
            self.assertFalse(valid)
            self.assertIn("SHA-256 mismatch", reason)

            # Check matching hash
            correct_sha = compute_file_sha256(fake_exe)
            valid, reason = verify_isolated_executable(fake_exe, expected_sha=correct_sha, sandbox_dir=sandbox)
            self.assertTrue(valid)

            # Check sandbox escape
            outside_exe = tmppath / "outside.exe"
            outside_exe.write_bytes(b"test game binary content")
            valid, reason = verify_isolated_executable(outside_exe, expected_sha=correct_sha, sandbox_dir=sandbox)
            self.assertFalse(valid)
            self.assertIn("outside isolated sandbox", reason)

    def test_reject_out_of_bounds_parameters(self):
        with self.assertRaises(ValueError):
            self.sampler.run_bounded_sampling(duration_seconds=0.01, interval_seconds=0.5)
        with self.assertRaises(ValueError):
            self.sampler.run_bounded_sampling(duration_seconds=150.0, interval_seconds=0.5)
        with self.assertRaises(ValueError):
            self.sampler.run_bounded_sampling(duration_seconds=10.0, interval_seconds=0.01)
        with self.assertRaises(ValueError):
            self.sampler.run_bounded_sampling(duration_seconds=10.0, interval_seconds=10.0)

    def test_stale_identity_rejected(self):
        # Create actor in memory
        pawn_ptr = 0x60001000
        self.fixture.setup_object(pawn_ptr, "SFXPawn_Trooper_1", self.fixture.class_trooper, outer=self.fixture.level_ptr)
        self.reader.write_ptr(pawn_ptr + OFFSET_ACTOR_WORLD_INFO, self.fixture.world_info_ptr)
        self.reader.write_uint32(pawn_ptr + OFFSET_ACTOR_FLAGS, 0)  # bDeleteMe = False

        self.fixture.level_actors.append(pawn_ptr)
        self.fixture.sync_level_actors(self.fixture.level_ptr, self.fixture.level_actors, data_ptr=0x30005000)

        # Corrupt InternalIndex in GObjects to simulate stale/unmapped object
        self.reader.write_ptr(self.fixture.gobjects_data_ptr + (len(self.fixture.objects) - 1) * 8, 0x99999999)

        actors, _ = self.sampler.enumerate_actors(self.fixture.world_ptr)
        actor_ptrs = [a["pointer"] for a in actors]
        self.assertNotIn(pawn_ptr, actor_ptrs, "Stale actor with mismatched GObjects entry must be rejected")

    def test_reject_wrong_world_actors(self):
        # Create actor with different WorldInfo
        pawn_ptr = 0x60002000
        self.fixture.setup_object(pawn_ptr, "SFXPawn_Trooper_2", self.fixture.class_trooper, outer=self.fixture.level_ptr)
        self.reader.write_ptr(pawn_ptr + OFFSET_ACTOR_WORLD_INFO, 0xDEADBEEF)  # Wrong world info!
        self.reader.write_uint32(pawn_ptr + OFFSET_ACTOR_FLAGS, 0)

        self.fixture.level_actors.append(pawn_ptr)
        self.fixture.sync_level_actors(self.fixture.level_ptr, self.fixture.level_actors, data_ptr=0x30005000)

        actors, _ = self.sampler.enumerate_actors(self.fixture.world_ptr)
        actor_ptrs = [a["pointer"] for a in actors]
        self.assertNotIn(pawn_ptr, actor_ptrs, "Actor from another world must be rejected")

    def test_reject_deleted_actors(self):
        pawn_ptr = 0x60003000
        self.fixture.setup_object(pawn_ptr, "SFXPawn_Trooper_3", self.fixture.class_trooper, outer=self.fixture.level_ptr)
        self.reader.write_ptr(pawn_ptr + OFFSET_ACTOR_WORLD_INFO, self.fixture.world_info_ptr)
        self.reader.write_uint32(pawn_ptr + OFFSET_ACTOR_FLAGS, 0x8)  # bDeleteMe is bit 3 (0x8)

        self.fixture.level_actors.append(pawn_ptr)
        self.fixture.sync_level_actors(self.fixture.level_ptr, self.fixture.level_actors, data_ptr=0x30005000)

        actors, _ = self.sampler.enumerate_actors(self.fixture.world_ptr)
        actor_ptrs = [a["pointer"] for a in actors]
        self.assertNotIn(pawn_ptr, actor_ptrs, "Actor marked bDeleteMe must be rejected")

    def test_reject_class_default_object(self):
        # Add CDO directly to level actors
        cdo_ptr = self.reader.read_ptr(self.fixture.class_trooper + OFFSET_UCLASS_CLASS_DEFAULT_OBJECT)
        self.reader.write_ptr(cdo_ptr + OFFSET_ACTOR_WORLD_INFO, self.fixture.world_info_ptr)
        self.reader.write_uint32(cdo_ptr + OFFSET_ACTOR_FLAGS, 0)

        self.fixture.level_actors.append(cdo_ptr)
        self.fixture.sync_level_actors(self.fixture.level_ptr, self.fixture.level_actors, data_ptr=0x30005000)

        actors, _ = self.sampler.enumerate_actors(self.fixture.world_ptr)
        actor_ptrs = [a["pointer"] for a in actors]
        self.assertNotIn(cdo_ptr, actor_ptrs, "Class Default Object (CDO) must be rejected")

    def test_reject_player_and_squad_members(self):
        # 1. Shepard (SFXPawn_PlayerSoldier)
        shepard = 0x60010000
        self.fixture.setup_object(shepard, "SFXPawn_PlayerSoldier_1", self.fixture.class_playersoldier, outer=self.fixture.level_ptr)
        self.reader.write_ptr(shepard + OFFSET_ACTOR_WORLD_INFO, self.fixture.world_info_ptr)
        is_hostile, reason, _ = self.sampler.classify_pawn(shepard)
        self.assertFalse(is_hostile)
        self.assertEqual(reason, "player_pawn")

        # 2. Squad Companion (Ashley on BioPlayerSquad)
        ashley = 0x60011000
        self.fixture.setup_object(ashley, "SFXPawn_Ashley_1", self.fixture.class_ashley, outer=self.fixture.level_ptr)
        self.reader.write_ptr(ashley + OFFSET_ACTOR_WORLD_INFO, self.fixture.world_info_ptr)

        player_squad = 0x60012000
        self.fixture.setup_object(player_squad, "BioPlayerSquad_0", self.fixture.class_playersquad, outer=self.fixture.level_ptr)
        self.reader.write_ptr(ashley + OFFSET_BIOPAWN_SQUAD, player_squad)

        is_hostile, reason, _ = self.sampler.classify_pawn(ashley)
        self.assertFalse(is_hostile)
        self.assertEqual(reason, "player_squad_member")

    def test_reject_gatehouse_dummy_biopawn(self):
        # Construct Gatehouse dummy BioPawn
        gatehouse_level = 0x30010000
        self.fixture.setup_object(gatehouse_level, "BioD_ProMar_120Gatehouse", self.fixture.class_level, outer=self.fixture.world_ptr)

        dummy = 0x60020000
        self.fixture.setup_object(dummy, "BioPawn_0", self.fixture.class_biopawn, outer=gatehouse_level)
        self.reader.write_ptr(dummy + OFFSET_ACTOR_WORLD_INFO, self.fixture.world_info_ptr)
        self.reader.write_uint32(dummy + OFFSET_ACTOR_FLAGS, 0)

        # Plain AIController, no squad, no team
        ai_ctrl = 0x60021000
        self.fixture.setup_object(ai_ctrl, "AIController_1", self.fixture.class_aicontroller, outer=self.fixture.level_ptr)
        self.reader.write_ptr(dummy + OFFSET_PAWN_CONTROLLER, ai_ctrl)
        self.reader.write_ptr(dummy + OFFSET_BIOPAWN_SQUAD, 0)

        self.fixture.sync_gobjects()

        is_hostile, reason, _ = self.sampler.classify_pawn(dummy)
        self.assertFalse(is_hostile, "Gatehouse BioPawn dummy must NOT be treated as hostile")
        self.assertEqual(reason, "gatehouse_dummy_biopawn")

    def test_accept_genuine_hostile_trooper(self):
        # Construct genuine hostile Trooper
        trooper = 0x60030000
        self.fixture.setup_object(trooper, "SFXPawn_Trooper_0", self.fixture.class_trooper, outer=self.fixture.level_ptr)
        self.reader.write_ptr(trooper + OFFSET_ACTOR_WORLD_INFO, self.fixture.world_info_ptr)
        self.reader.write_uint32(trooper + OFFSET_ACTOR_FLAGS, 0)

        # Combat squad
        combat_squad = 0x60031000
        self.fixture.setup_object(combat_squad, "BioSquadCombat_0", self.fixture.class_combatsquad, outer=self.fixture.level_ptr)
        self.reader.write_ptr(trooper + OFFSET_BIOPAWN_SQUAD, combat_squad)

        # Combat controller with hostile team
        trooper_ctrl = 0x60032000
        self.fixture.setup_object(trooper_ctrl, "SFXAI_Trooper_0", self.fixture.class_troopercontroller, outer=self.fixture.level_ptr)
        pri = 0x60033000
        self.fixture.setup_object(pri, "PlayerReplicationInfo_1", self.fixture.class_pri, outer=self.fixture.level_ptr)
        team = 0x60034000
        self.fixture.setup_object(team, "SFXTeamInfo_Cerberus", self.fixture.class_teaminfo, outer=self.fixture.level_ptr)
        self.reader.write_int32(team + OFFSET_TEAMINFO_TEAM_INDEX, 1)  # Team 1 = Hostile

        self.reader.write_ptr(trooper + OFFSET_PAWN_CONTROLLER, trooper_ctrl)
        self.reader.write_ptr(trooper_ctrl + OFFSET_CONTROLLER_PRI, pri)
        self.reader.write_ptr(pri + OFFSET_PRI_TEAM, team)

        self.fixture.sync_gobjects()

        is_hostile, reason, meta = self.sampler.classify_pawn(trooper)
        self.assertTrue(is_hostile, "Genuine Trooper must be classified as hostile")
        self.assertEqual(reason, "genuine_hostile")
        self.assertEqual(meta["team_index"], 1)
        self.assertTrue(meta["hostile_indicators"]["hostile_team"])
        self.assertTrue(meta["hostile_indicators"]["combat_squad"])
        self.assertTrue(meta["hostile_indicators"]["hostile_class"])

    def test_damage_module_reciprocal_ownership_verified(self):
        trooper = 0x60040000
        self.fixture.setup_object(trooper, "SFXPawn_Trooper_1", self.fixture.class_trooper, outer=self.fixture.level_ptr)

        # Valid module with reciprocal owner pointing to trooper
        valid_mod = self.fixture.add_damage_module(trooper, current_health=450.0, max_health=500.0, reciprocal_owner=trooper)
        # Invalid module with mismatched owner
        invalid_mod = self.fixture.add_damage_module(trooper, current_health=300.0, max_health=500.0, reciprocal_owner=0xDEADBEEF)

        # Link modules to trooper
        self.reader.write_tarray(trooper + OFFSET_ACTOR_MODULES, [valid_mod, invalid_mod], 0x60045000)

        # Legacy pawn health has different value (must not be authoritative)
        self.reader.write_int32(trooper + OFFSET_PAWN_HEALTH_LEGACY, 100)

        modules, failures = self.sampler.sample_damage_modules(trooper)
        self.assertEqual(len(modules), 1, "Only reciprocally owned module should be sampled")
        self.assertEqual(modules[0]["current_health"], 450.0)
        self.assertEqual(modules[0]["max_health"], 500.0)
        self.assertTrue(modules[0]["owner_reciprocal"])
        self.assertEqual(len(failures), 1)
        self.assertIn("ownership mismatch", failures[0])

    def test_inventory_shields_reciprocal_ownership_verified(self):
        trooper = 0x60050000
        self.fixture.setup_object(trooper, "SFXPawn_Trooper_2", self.fixture.class_trooper, outer=self.fixture.level_ptr)

        inv_man = 0x60051000
        self.fixture.setup_object(inv_man, "SFXInventoryManager_1", self.fixture.class_sfx_inv_manager, outer=trooper)
        self.reader.write_ptr(trooper + OFFSET_PAWN_INVENTORY, inv_man)

        # Valid shield: owner, instigator, inv_man all match
        valid_shield = self.fixture.add_shield(trooper, inv_man, current_shields=250.0, max_shields=250.0)

        # Invalid shield: owner points elsewhere
        invalid_shield = self.fixture.add_shield(trooper, inv_man, current_shields=150.0, max_shields=250.0, owner=0xBAADF00D)

        # Link inventory chain: inv_man -> valid_shield -> invalid_shield -> 0
        self.reader.write_ptr(inv_man + OFFSET_INVENTORY_CHAIN, valid_shield)
        self.reader.write_ptr(valid_shield + OFFSET_INVENTORY_NEXT, invalid_shield)

        shields, failures = self.sampler.sample_inventory_shields(trooper)
        self.assertEqual(len(shields), 1, "Only shield with verified reciprocal links should be accepted")
        self.assertEqual(shields[0]["current_shields"], 250.0)
        self.assertEqual(shields[0]["max_shields"], 250.0)
        self.assertTrue(shields[0]["owner_matches"])
        self.assertTrue(shields[0]["instigator_matches"])
        self.assertTrue(shields[0]["inv_manager_matches"])
        self.assertEqual(len(failures), 1)
        self.assertIn("reciprocal link mismatch", failures[0])

    def test_unreadable_or_nan_health_rejected(self):
        trooper = 0x60060000
        self.fixture.setup_object(trooper, "SFXPawn_Trooper_3", self.fixture.class_trooper, outer=self.fixture.level_ptr)

        # Damage module with NaN health
        mod = self.fixture.add_damage_module(trooper, current_health=100.0, max_health=100.0, reciprocal_owner=trooper)
        self.reader.write_float(mod + OFFSET_DAMAGE_CURRENT_HEALTH, float("nan"))
        self.reader.write_tarray(trooper + OFFSET_ACTOR_MODULES, [mod], 0x60065000)

        modules, failures = self.sampler.sample_damage_modules(trooper)
        self.assertEqual(len(modules), 0, "Module with NaN health must be rejected")
        self.assertEqual(len(failures), 1)
        self.assertIn("CurrentHealth at +0x98 is unreadable or non-finite", failures[0])

    def test_world_pointer_change_detected(self):
        # Force world pointer to change
        snapshot1 = self.sampler.sample_snapshot()
        self.assertIn(snapshot1["status"], ("ok", "no_hostiles_found"))

        # Now simulate world pointer becoming invalid before snapshot
        self.reader.write_ptr(self.fixture.base + OFFSET_GWORLD, 0x12345678)
        snapshot2 = self.sampler.sample_snapshot()
        self.assertEqual(snapshot2["status"], "world_unavailable")
        self.assertIn("GWorld pointer is null, unreadable, or not a World", snapshot2["read_failures"][0])

        # Restore valid world
        self.reader.write_ptr(self.fixture.base + OFFSET_GWORLD, self.fixture.world_ptr)

        # Test mid-snapshot world change: wrap reader.read_ptr to change GWORLD after initial read
        original_read_ptr = self.reader.read_ptr
        changed = False

        def hook_read_ptr(addr):
            nonlocal changed
            val = original_read_ptr(addr)
            if addr == self.fixture.base + OFFSET_GWORLD and not changed:
                changed = True
                # Next read of GWORLD will see different address
                self.reader.write_ptr(self.fixture.base + OFFSET_GWORLD, 0x99990000)
            return val

        self.reader.read_ptr = hook_read_ptr
        snapshot3 = self.sampler.sample_snapshot()
        self.reader.read_ptr = original_read_ptr
        self.assertEqual(snapshot3["status"], "world_changed")
        self.assertTrue(any("changed during snapshot" in f for f in snapshot3["read_failures"]))

    def test_complete_snapshot_and_jsonl_output(self):
        # Set up a complete hostile enemy in the world
        trooper = 0x60070000
        self.fixture.setup_object(trooper, "SFXPawn_Trooper_4", self.fixture.class_trooper, outer=self.fixture.level_ptr)
        self.reader.write_ptr(trooper + OFFSET_ACTOR_WORLD_INFO, self.fixture.world_info_ptr)
        self.reader.write_uint32(trooper + OFFSET_ACTOR_FLAGS, 0)
        self.reader.write_bytes(trooper + OFFSET_ACTOR_LOCATION, struct.pack("<3f", 100.0, 200.0, 300.0))

        # Squad & controller
        combat_squad = 0x60071000
        self.fixture.setup_object(combat_squad, "BioSquadCombat_1", self.fixture.class_combatsquad, outer=self.fixture.level_ptr)
        self.reader.write_ptr(trooper + OFFSET_BIOPAWN_SQUAD, combat_squad)

        # Damage module and shields
        mod = self.fixture.add_damage_module(trooper, current_health=500.0, max_health=500.0, reciprocal_owner=trooper)
        self.reader.write_tarray(trooper + OFFSET_ACTOR_MODULES, [mod], 0x60072000)

        inv_man = 0x60073000
        self.fixture.setup_object(inv_man, "SFXInventoryManager_2", self.fixture.class_sfx_inv_manager, outer=trooper)
        self.reader.write_ptr(trooper + OFFSET_PAWN_INVENTORY, inv_man)
        shield = self.fixture.add_shield(trooper, inv_man, current_shields=200.0, max_shields=200.0)
        self.reader.write_ptr(inv_man + OFFSET_INVENTORY_CHAIN, shield)

        self.fixture.level_actors.append(trooper)
        self.fixture.sync_level_actors(self.fixture.level_ptr, self.fixture.level_actors, data_ptr=0x30005000)
        self.fixture.sync_gobjects()

        # Run bounded sampling into string buffer
        buf = io.StringIO()
        samples = self.sampler.run_bounded_sampling(duration_seconds=0.15, interval_seconds=0.06, output_stream=buf)

        self.assertGreaterEqual(len(samples), 1)
        first = samples[0]
        self.assertEqual(first["status"], "ok")
        self.assertEqual(first["hostiles_count"], 1)
        h = first["hostiles"][0]
        self.assertEqual(h["class"], "SFXPawn_Trooper")
        self.assertEqual(h["damage_modules"][0]["current_health"], 500.0)
        self.assertEqual(h["shields"][0]["current_shields"], 200.0)

        # Validate JSONL content
        lines = buf.getvalue().strip().split("\n")
        self.assertEqual(len(lines), len(samples))
        for line in lines:
            parsed = json.loads(line)
            self.assertIn("monotonic_time_s", parsed)
            self.assertIn("hostiles", parsed)

    def test_damage_attribution_skepticism(self):
        # Test evaluate_damage_decrease behavior
        obs1 = {
            "monotonic_time_s": 10.0,
            "hostiles": [{
                "pointer": "0x60070000",
                "path": "TheWorld.PersistentLevel.SFXPawn_Trooper_4",
                "damage_modules": [{"current_health": 500.0}],
                "shields": [{"current_shields": 200.0}],
            }],
        }
        obs2 = {
            "monotonic_time_s": 10.5,
            "hostiles": [{
                "pointer": "0x60070000",
                "path": "TheWorld.PersistentLevel.SFXPawn_Trooper_4",
                "damage_modules": [{"current_health": 350.0}],
                "shields": [{"current_shields": 0.0}],
            }],
        }

        analysis = evaluate_damage_decrease([obs1, obs2])
        self.assertTrue(analysis["decrease_observed"])
        self.assertEqual(analysis["health_decrease"], 150.0)
        self.assertEqual(analysis["shield_decrease"], 200.0)
        # CRITICAL SKEPTICISM CHECK: guest damage attribution MUST NOT be claimed!
        self.assertFalse(analysis["guest_damage_attribution_proven"])
        self.assertIn("Without observed hit/weapon attribution", analysis["attribution_note"])


if __name__ == "__main__":
    unittest.main()
