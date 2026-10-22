"""Unit tests for the bounded authoritative enemy health and shield sampler."""
from __future__ import annotations

import io
import copy
import json
import math
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest import mock
import importlib.util
import contextlib

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
    OFFSET_UWORLD_LEVELS,
    OFFSET_UWORLD_PERSISTENT_LEVEL,
    OFFSET_WORLDINFO_STREAMING_LEVELS,
    SUPPORTED_EXECUTABLE_SHA256,
)


def qualified_observations(*observations):
    """Explicit synthetic identities for offline evaluator fixtures."""
    def identity(pointer):
        return {"pointer": pointer, "internal_index": int(pointer, 16),
                "fname_index": 10, "fname_number": 0, "class_pointer": "0xc000", "outer_pointer": "0xa000"}
    out = copy.deepcopy(observations)
    for observation in out:
        observation.update(status="ok", authoritative=True, valid_for_decrease=True,
                           netmode=2, read_failures=[], world_pointer="0x7000",
                           world_identity=identity("0x7000"), world_info_identity=identity("0x8000"))
        for hostile in observation["hostiles"]:
            hostile.update(identity=identity(hostile["pointer"]), role=3, measurement_complete=True,
                           measurement_failures=[], world_info="0x8000", inventory_identity=None)
            for i, module in enumerate(hostile.get("damage_modules", [])):
                pointer = hex(int(hostile["pointer"], 16) + 0x100000 + i)
                module.update(pointer=pointer, identity=identity(pointer), module_owner=hostile["pointer"])
            for i, shield in enumerate(hostile.get("shields", [])):
                pointer = hex(int(hostile["pointer"], 16) + 0x200000 + i)
                shield.update(pointer=pointer, identity=identity(pointer), owner=hostile["pointer"],
                              instigator=hostile["pointer"], inv_manager="0x9000")
    return out


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
        self.class_bioaicontroller = self.create_class("BioAiController", super_class=self.class_aicontroller)
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
        self.class_guardian = self.create_class("SFXPawn_Guardian", super_class=self.class_biopawn)
        self.class_phantom = self.create_class("SFXPawn_Phantom", super_class=self.class_biopawn)
        self.class_civilian = self.create_class("SFXPawn_Civilian", super_class=self.class_biopawn)

        # World setup
        self.world_ptr = 0x30000000
        self.level_ptr = 0x30001000
        self.world_info_ptr = 0x30002000

        self.setup_object(self.world_ptr, "TheWorld", self.class_world)
        self.setup_object(self.level_ptr, "PersistentLevel", self.class_level, outer=self.world_ptr)
        self.setup_object(self.world_info_ptr, "BioWorldInfo_0", self.class_worldinfo, outer=self.level_ptr)

        self.reader.write_ptr(self.base + OFFSET_GWORLD, self.world_ptr)
        self.reader.write_ptr(self.world_ptr + OFFSET_UWORLD_PERSISTENT_LEVEL, self.level_ptr)
        self.reader.write_tarray(self.world_ptr + OFFSET_UWORLD_LEVELS, [], 0x30007000)

        # Actor array in PersistentLevel
        self.level_actors: list[int] = [self.world_info_ptr]
        self.sync_level_actors(self.level_ptr, self.level_actors, data_ptr=0x30005000)

        # WorldInfo pointers
        self.reader.write_ptr(self.world_info_ptr + OFFSET_ACTOR_WORLD_INFO, self.world_info_ptr)
        self.reader.write_byte(self.world_info_ptr + 0x7C8, 2)
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
        self.reader.write_byte(addr + 0x26A, 3)
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
        self.assertEqual(reason, "hostile_candidate")
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
        self.assertTrue(any("CurrentHealth at +0x98 is unreadable or non-finite" in f for f in failures))

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
        original_read = self.reader.read
        changed = False

        def hook_read(addr, size):
            nonlocal changed
            val = original_read(addr, size)
            if addr == self.fixture.base + OFFSET_GWORLD and not changed:
                changed = True
                # Next read of GWORLD will see different address
                self.reader.write_ptr(self.fixture.base + OFFSET_GWORLD, 0x99990000)
            return val

        self.reader.read = hook_read
        snapshot3 = self.sampler.sample_snapshot()
        self.reader.read = original_read
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

        analysis = evaluate_damage_decrease(qualified_observations(obs1, obs2))
        self.assertTrue(analysis["decrease_observed"])
        self.assertEqual(analysis["health_decrease"], 150.0)
        self.assertEqual(analysis["shield_decrease"], 200.0)
        # CRITICAL SKEPTICISM CHECK: guest damage attribution MUST NOT be claimed!
        self.assertFalse(analysis["guest_damage_attribution_proven"])
        self.assertIn("Without observed hit/weapon attribution", analysis["attribution_note"])

    def test_evaluate_damage_decrease_rejects_comparing_different_hostiles(self):
        # Sample 0 has Hostile A at 0x60010000 with HP 500
        obs1 = {
            "monotonic_time_s": 10.0,
            "hostiles": [{
                "pointer": "0x60010000",
                "path": "TheWorld.PersistentLevel.SFXPawn_Trooper_1",
                "damage_modules": [{"current_health": 500.0}],
                "shields": [{"current_shields": 200.0}],
            }],
        }
        # Sample 1 has Hostile B at 0x60020000 with HP 200 (Hostile A was culled/moved)
        obs2 = {
            "monotonic_time_s": 10.5,
            "hostiles": [{
                "pointer": "0x60020000",
                "path": "TheWorld.PersistentLevel.SFXPawn_Trooper_2",
                "damage_modules": [{"current_health": 200.0}],
                "shields": [{"current_shields": 0.0}],
            }],
        }
        # Must NOT compare Hostile A with Hostile B and report false damage!
        analysis = evaluate_damage_decrease(qualified_observations(obs1, obs2))
        self.assertFalse(analysis["decrease_observed"])
        self.assertIn("insufficient complete samples", analysis["reason"])

    def test_evaluate_damage_decrease_detects_transient_damage_with_regeneration(self):
        # Hostile takes 150 shield damage in sample 2, but regenerates by sample 3
        obs1 = {"monotonic_time_s": 1.0, "hostiles": [{"pointer": "0x60010000", "shields": [{"current_shields": 200.0}], "damage_modules": [{"current_health": 500.0}]}]}
        obs2 = {"monotonic_time_s": 1.5, "hostiles": [{"pointer": "0x60010000", "shields": [{"current_shields": 50.0}], "damage_modules": [{"current_health": 500.0}]}]}
        obs3 = {"monotonic_time_s": 2.0, "hostiles": [{"pointer": "0x60010000", "shields": [{"current_shields": 200.0}], "damage_modules": [{"current_health": 500.0}]}]}
        analysis = evaluate_damage_decrease(qualified_observations(obs1, obs2, obs3))
        self.assertTrue(analysis["decrease_observed"])
        self.assertEqual(analysis["shield_decrease"], 150.0)
        self.assertEqual(analysis["per_target_evaluations"]["0x60010000"]["net_shield_decrease"], 0.0)

    def test_reject_civilian_with_generic_bioaicontroller(self):
        # Neutral/civilian pawn with generic BioAiController, no combat squad, no hostile team
        civilian = 0x60080000
        self.fixture.setup_object(civilian, "SFXPawn_Civilian_1", self.fixture.class_civilian, outer=self.fixture.level_ptr)
        self.reader.write_ptr(civilian + OFFSET_ACTOR_WORLD_INFO, self.fixture.world_info_ptr)
        ctrl = 0x60081000
        self.fixture.setup_object(ctrl, "BioAiController_0", self.fixture.class_bioaicontroller, outer=self.fixture.level_ptr)
        self.reader.write_ptr(civilian + OFFSET_PAWN_CONTROLLER, ctrl)
        self.reader.write_ptr(civilian + OFFSET_BIOPAWN_SQUAD, 0)
        self.fixture.sync_gobjects()

        is_hostile, reason, _ = self.sampler.classify_pawn(civilian)
        self.assertFalse(is_hostile, "Civilian with BioAiController must not be classified as hostile")
        self.assertEqual(reason, "unverified_non_hostile_pawn")

    def test_accept_mars_cerberus_guardian_and_phantom_archetypes(self):
        guardian = 0x60090000
        self.fixture.setup_object(guardian, "SFXPawn_Guardian_0", self.fixture.class_guardian, outer=self.fixture.level_ptr)
        self.reader.write_ptr(guardian + OFFSET_ACTOR_WORLD_INFO, self.fixture.world_info_ptr)
        is_hostile, reason, meta = self.sampler.classify_pawn(guardian)
        self.assertTrue(is_hostile, "Cerberus Guardian must be recognized as hostile archetype")
        self.assertEqual(reason, "hostile_candidate")

        phantom = 0x60091000
        self.fixture.setup_object(phantom, "SFXPawn_Phantom_0", self.fixture.class_phantom, outer=self.fixture.level_ptr)
        self.reader.write_ptr(phantom + OFFSET_ACTOR_WORLD_INFO, self.fixture.world_info_ptr)
        is_hostile, reason, meta = self.sampler.classify_pawn(phantom)
        self.assertTrue(is_hostile, "Cerberus Phantom must be recognized as hostile archetype")
        self.assertEqual(reason, "hostile_candidate")

    def test_level_actors_changed_during_snapshot_detected(self):
        snap1 = self.sampler.sample_snapshot()
        self.assertIn(snap1["status"], ("ok", "no_hostiles_found"))

        # Mutate level actor table memory mid-snapshot via classify_pawn hook
        orig_classify = self.sampler.classify_pawn

        def hook_classify(ptr):
            # Mutate the level actor table in memory while actors are being scanned
            self.reader.write_bytes(self.fixture.level_ptr + OFFSET_ULEVEL_ACTORS, b"\xff" * 16)
            return orig_classify(ptr)

        self.sampler.classify_pawn = hook_classify
        snap2 = self.sampler.sample_snapshot()
        self.sampler.classify_pawn = orig_classify
        self.assertEqual(snap2["status"], "level_actors_changed")
        self.assertTrue(any("actor table changed during snapshot" in f for f in snap2["read_failures"]))

    def test_damage_modules_array_unreadable_records_failure(self):
        trooper = 0x600A0000
        self.fixture.setup_object(trooper, "SFXPawn_Trooper_5", self.fixture.class_trooper, outer=self.fixture.level_ptr)
        self.reader.set_fail_address(trooper + OFFSET_ACTOR_MODULES)

        modules, failures = self.sampler.sample_damage_modules(trooper)
        self.assertEqual(len(modules), 0)
        self.assertEqual(len(failures), 1)
        self.assertIn("Modules array at +0x70 is unreadable", failures[0])

    def test_inventory_chain_corrupted_pointer_records_failure(self):
        trooper = 0x600B0000
        self.fixture.setup_object(trooper, "SFXPawn_Trooper_6", self.fixture.class_trooper, outer=self.fixture.level_ptr)
        inv_man = 0x600B1000
        self.fixture.setup_object(inv_man, "SFXInventoryManager_3", self.fixture.class_sfx_inv_manager, outer=trooper)
        self.reader.write_ptr(trooper + OFFSET_PAWN_INVENTORY, inv_man)
        # Write corrupted item pointer that is not a live object in GObjects
        self.reader.write_ptr(inv_man + OFFSET_INVENTORY_CHAIN, 0xBAADF00D)

        shields, failures = self.sampler.sample_inventory_shields(trooper)
        self.assertEqual(len(shields), 0)
        self.assertTrue(any("not a valid live Inventory object" in f for f in failures))

    def test_enumerate_levels_with_world_levels_and_worldinfo_streaming(self):
        # Level in StreamingLevels
        stream_lvl = 0x30020000
        self.fixture.setup_object(stream_lvl, "StreamingLevel_1", self.fixture.class_level, outer=self.fixture.world_ptr)
        self.fixture.sync_level_actors(stream_lvl, [], data_ptr=0x30021000)

        stream_obj = 0x30022000
        self.fixture.setup_object(stream_obj, "LevelStreaming_1", self.fixture.class_levelstreaming, outer=self.fixture.world_info_ptr)
        self.reader.write_ptr(stream_obj + OFFSET_LEVELSTREAMING_LOADED_LEVEL, stream_lvl)
        self.reader.write_uint32(stream_obj + OFFSET_LEVELSTREAMING_FLAGS, 1)  # Visible
        self.reader.write_tarray(self.fixture.world_info_ptr + OFFSET_WORLDINFO_STREAMING_LEVELS, [stream_obj], 0x30006000)

        # Additional level in World.Levels (+0x78)
        world_lvl = 0x30030000
        self.fixture.setup_object(world_lvl, "WorldLevel_2", self.fixture.class_level, outer=self.fixture.world_ptr)
        self.fixture.sync_level_actors(world_lvl, [], data_ptr=0x30031000)
        self.reader.write_tarray(self.fixture.world_ptr + OFFSET_UWORLD_LEVELS, [world_lvl], 0x30007000)

        levels, world_info, failures = self.sampler.enumerate_levels(self.fixture.world_ptr)
        self.assertEqual(len(failures), 0)
        level_ptrs = [lvl for lvl, vis in levels]
        self.assertIn(self.fixture.level_ptr, level_ptrs)
        self.assertIn(stream_lvl, level_ptrs)
        self.assertIn(world_lvl, level_ptrs)
        self.assertEqual(len(levels), 3)

    def test_worldinfo_not_first_actor_still_discovered(self):
        # Put a static actor at Actors[0], WorldInfo at Actors[1]
        other_actor = 0x30040000
        self.fixture.setup_object(other_actor, "StaticMeshActor_0", self.fixture.class_actor, outer=self.fixture.level_ptr)
        self.fixture.level_actors = [other_actor, self.fixture.world_info_ptr]
        self.fixture.sync_level_actors(self.fixture.level_ptr, self.fixture.level_actors, data_ptr=0x30005000)

        levels, world_info, failures = self.sampler.enumerate_levels(self.fixture.world_ptr)
        self.assertEqual(len(failures), 0)
        self.assertEqual(world_info, self.fixture.world_info_ptr)

    def test_evaluate_damage_decrease_single_target_filter(self):
        obs1 = {
            "monotonic_time_s": 1.0,
            "hostiles": [
                {"pointer": "0x1000", "damage_modules": [{"current_health": 500.0}], "shields": []},
                {"pointer": "0x2000", "damage_modules": [{"current_health": 400.0}], "shields": []},
            ],
        }
        obs2 = {
            "monotonic_time_s": 2.0,
            "hostiles": [
                {"pointer": "0x1000", "damage_modules": [{"current_health": 500.0}], "shields": []},
                {"pointer": "0x2000", "damage_modules": [{"current_health": 250.0}], "shields": []},
            ],
        }
        res_1000 = evaluate_damage_decrease(qualified_observations(obs1, obs2), target_pointer="0x1000")
        self.assertFalse(res_1000["decrease_observed"])
        self.assertEqual(res_1000["health_decrease"], 0.0)

        res_2000 = evaluate_damage_decrease(qualified_observations(obs1, obs2), target_pointer="0x2000")
        self.assertTrue(res_2000["decrease_observed"])
        self.assertEqual(res_2000["health_decrease"], 150.0)

    def make_sampled_target(self):
        pawn = 0x61000000
        self.fixture.setup_object(pawn, "SFXPawn_Trooper_20", self.fixture.class_trooper, outer=self.fixture.level_ptr)
        self.reader.write_ptr(pawn + OFFSET_ACTOR_WORLD_INFO, self.fixture.world_info_ptr)
        module = self.fixture.add_damage_module(pawn, 100.0, 100.0)
        self.reader.write_tarray(pawn + OFFSET_ACTOR_MODULES, [module], 0x61010000)
        self.fixture.level_actors.append(pawn)
        self.fixture.sync_level_actors(self.fixture.level_ptr, self.fixture.level_actors, 0x30005000)
        return pawn, module

    def test_real_snapshot_schema_detects_only_valid_stable_delta(self):
        _, module = self.make_sampled_target()
        first = self.sampler.sample_snapshot()
        self.assertTrue(first["valid_for_decrease"], first["read_failures"])
        self.reader.write_float(module + OFFSET_DAMAGE_CURRENT_HEALTH, 70.0)
        second = self.sampler.sample_snapshot()
        second["monotonic_time_s"] = first["monotonic_time_s"] + 0.05
        result = evaluate_damage_decrease([first, second])
        self.assertEqual(result["health_decrease"], 30.0)
        self.assertFalse(result["guest_damage_attribution_proven"])

    def test_unreadable_module_never_becomes_zero_health(self):
        _, module = self.make_sampled_target()
        first = self.sampler.sample_snapshot()
        self.reader.set_fail_address(module + OFFSET_DAMAGE_CURRENT_HEALTH)
        second = self.sampler.sample_snapshot()
        self.assertFalse(second["valid_for_decrease"])
        self.assertFalse(evaluate_damage_decrease([first, second])["decrease_observed"])

    def test_module_replacement_same_array_header_starts_new_epoch(self):
        pawn, _ = self.make_sampled_target()
        first = self.sampler.sample_snapshot()
        replacement = self.fixture.add_damage_module(pawn, 20.0, 100.0)
        self.reader.write_ptr(0x61010000, replacement)
        second = self.sampler.sample_snapshot()
        second["monotonic_time_s"] = first["monotonic_time_s"] + 0.05
        self.assertTrue(second["valid_for_decrease"], second["read_failures"])
        self.assertFalse(evaluate_damage_decrease([first, second])["decrease_observed"])

    def test_owner_mutation_during_health_read_invalidates_sample(self):
        _, module = self.make_sampled_target()
        original_read = self.reader.read
        def mutate(address, size):
            value = original_read(address, size)
            if address == module + OFFSET_DAMAGE_CURRENT_HEALTH:
                self.reader.write_ptr(module + OFFSET_MODULE_OWNER, 0x99990000)
            return value
        self.reader.read = mutate
        result = self.sampler.sample_snapshot()
        self.assertFalse(result["valid_for_decrease"])
        self.assertTrue(any("changed identity/owner" in f for f in result["read_failures"]))

    def test_delta_rejects_each_identity_or_validity_change(self):
        self.make_sampled_target()
        first = self.sampler.sample_snapshot()
        mutations = {
            "world": lambda x: x.update(world_pointer="0x99990000"),
            "world_generation": lambda x: x["world_identity"].update(internal_index=999),
            "actor_generation": lambda x: x["hostiles"][0]["identity"].update(internal_index=999),
            "actor_fname": lambda x: x["hostiles"][0]["identity"].update(fname_number=999),
            "actor_path": lambda x: x["hostiles"][0].update(path="OtherWorld.Pawn"),
            "module": lambda x: x["hostiles"][0]["damage_modules"][0].update(pointer="0x88880000"),
            "module_generation": lambda x: x["hostiles"][0]["damage_modules"][0]["identity"].update(fname_number=999),
            "module_owner": lambda x: x["hostiles"][0]["damage_modules"][0].update(module_owner="0x88880000"),
            "missing_module": lambda x: x["hostiles"][0].update(damage_modules=[]),
            "world_changed": lambda x: x.update(status="world_changed"),
            "level_changed": lambda x: x.update(status="level_actors_changed"),
            "read_failure": lambda x: x.update(read_failures=["partial read"]),
            "incomplete": lambda x: x["hostiles"][0].update(measurement_complete=False),
            "client": lambda x: x.update(netmode=3),
            "non_authority_role": lambda x: x["hostiles"][0].update(role=2),
        }
        for name, mutate in mutations.items():
            with self.subTest(name=name):
                changed = copy.deepcopy(first)
                changed["monotonic_time_s"] += 1
                changed["hostiles"][0]["damage_modules"][0]["current_health"] = 20.0
                mutate(changed)
                self.assertFalse(evaluate_damage_decrease([first, changed])["decrease_observed"])

    def test_shield_replacement_and_partial_chain_do_not_prove_delta(self):
        raw = {"monotonic_time_s": 1.0, "hostiles": [{"pointer": "0x1000", "damage_modules": [{"current_health": 100.0}], "shields": [{"current_shields": 200.0}]}]}
        first = qualified_observations(raw)[0]
        for mutation in (
            lambda x: x["hostiles"][0].update(shields=[]),
            lambda x: x["hostiles"][0]["shields"][0]["identity"].update(fname_number=2),
            lambda x: x["hostiles"][0]["shields"][0].update(owner="0x9999"),
            lambda x: x["hostiles"][0]["shields"][0].update(inv_manager="0x9999"),
        ):
            second = copy.deepcopy(first)
            second["monotonic_time_s"] = 2.0
            second["hostiles"][0]["shields"][0]["current_shields"] = 0.0
            mutation(second)
            self.assertFalse(evaluate_damage_decrease([first, second])["decrease_observed"])

    def test_invalid_gap_breaks_epoch_without_masking_later_real_delta(self):
        raw = {"monotonic_time_s": 1., "hostiles": [{"pointer": "0x1000", "damage_modules": [{"current_health": 100.}], "shields": []}]}
        first = qualified_observations(raw)[0]
        later = copy.deepcopy(first)
        later["monotonic_time_s"] = 3.
        later["hostiles"][0]["damage_modules"][0]["current_health"] = 50.
        gap = {"status": "world_unavailable", "hostiles": []}
        self.assertFalse(evaluate_damage_decrease([first, gap, later])["decrease_observed"])
        final = copy.deepcopy(later)
        final["monotonic_time_s"] = 4.
        final["hostiles"][0]["damage_modules"][0]["current_health"] = 40.
        self.assertEqual(evaluate_damage_decrease([first, gap, later, final])["health_decrease"], 10.)

    def test_client_or_non_authoritative_role_is_not_authoritative_health(self):
        pawn, _ = self.make_sampled_target()
        self.reader.write_byte(self.fixture.world_info_ptr + 0x7C8, 3)
        sample = self.sampler.sample_snapshot()
        self.assertFalse(sample["authoritative"])
        self.assertFalse(sample["valid_for_decrease"])
        self.reader.write_byte(self.fixture.world_info_ptr + 0x7C8, 2)
        self.reader.write_byte(pawn + 0x26A, 2)
        self.assertFalse(self.sampler.sample_snapshot()["valid_for_decrease"])

    def test_same_array_header_with_changed_contents_is_invalid(self):
        pawn, _ = self.make_sampled_target()
        original_read = self.reader.read
        reads = 0
        def mutate(address, size):
            nonlocal reads
            data = original_read(address, size)
            if address == 0x30005000 and size == 16:
                reads += 1
                if reads == 2:
                    self.reader.write_ptr(0x30005000 + 8, 0)
            return data
        self.reader.read = mutate
        result = self.sampler.sample_snapshot()
        self.assertEqual(result["status"], "level_actors_changed")
        self.assertFalse(result["valid_for_decrease"])

    def test_inventory_unreadable_head_next_and_cycles_are_unknown(self):
        pawn, _ = self.make_sampled_target()
        manager = 0x62000000
        self.fixture.setup_object(manager, "SFXInventoryManager", self.fixture.class_sfx_inv_manager, outer=pawn)
        self.reader.write_ptr(pawn + OFFSET_PAWN_INVENTORY, manager)
        shield = self.fixture.add_shield(pawn, manager)
        self.reader.write_ptr(manager + OFFSET_INVENTORY_CHAIN, shield)
        self.reader.set_fail_address(manager + OFFSET_INVENTORY_CHAIN)
        self.assertFalse(self.sampler.sample_snapshot()["valid_for_decrease"])
        self.reader._read_failure_addresses.clear()
        self.reader.set_fail_address(shield + OFFSET_INVENTORY_NEXT)
        self.assertFalse(self.sampler.sample_snapshot()["valid_for_decrease"])
        self.reader._read_failure_addresses.clear()
        self.reader.write_ptr(shield + OFFSET_INVENTORY_NEXT, shield)
        self.assertFalse(self.sampler.sample_snapshot()["valid_for_decrease"])

    def test_pending_delete_and_class_substring_are_not_live_hostiles(self):
        pawn, _ = self.make_sampled_target()
        self.reader.write_uint32(pawn + 0x260, 0x800000)
        self.assertEqual(self.sampler.sample_snapshot()["hostiles_count"], 0)
        self.reader.write_uint32(pawn + 0x260, 0)
        neutral = self.fixture.create_class("SFXPawn_EngineeringCivilian", self.fixture.class_biopawn)
        self.reader.write_ptr(pawn + OFFSET_UOBJECT_CLASS, neutral)
        self.assertFalse(self.sampler.classify_pawn(pawn)[0])

    def test_budget_deadline_returns_explicit_invalid_status(self):
        sample = self.sampler.sample_snapshot(deadline=0.0)
        self.assertEqual(sample["status"], "budget_exhausted")
        self.assertFalse(sample["valid_for_decrease"])

    def test_cli_world_unavailable_does_not_raise_keyerror(self):
        spec = importlib.util.spec_from_file_location("sample_cli", Path(__file__).resolve().parents[1] / "tools/sample_authoritative_combat.py")
        cli = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(cli)
        class EmptyProcess(MockMemoryReader):
            base_address, module_path = 0x10000000, "synthetic-only"
            def __enter__(self): return self
            def __exit__(self, *args): pass
        with tempfile.TemporaryDirectory() as folder:
            output = Path(folder) / "private.jsonl"
            args = ["sampler", "--pid", "123", "--seconds", "0.1", "--interval", "0.05", "--output", str(output)]
            with mock.patch.object(sys, "argv", args), mock.patch.object(cli, "WindowsProcessMemory", return_value=EmptyProcess()), contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(cli.main(), 0)
            self.assertEqual(json.loads(output.read_text().splitlines()[0])["status"], "world_unavailable")

    def test_executable_requires_sandbox_and_rejects_hardlinks(self):
        with tempfile.TemporaryDirectory() as folder:
            executable = Path(folder) / "MassEffect3.exe"
            executable.write_bytes(b"synthetic fixture")
            sha = compute_file_sha256(executable)
            self.assertFalse(verify_isolated_executable(executable, sha)[0])
            from types import SimpleNamespace
            metadata = executable.stat()
            # The desktop sandbox denies creating actual hardlinks. Exercise
            # the filesystem metadata branch without requesting wider access.
            with mock.patch.object(Path, "stat", return_value=SimpleNamespace(st_mode=metadata.st_mode, st_nlink=2)):
                valid, reason = verify_isolated_executable(executable, sha, Path(folder))
            self.assertFalse(valid)
            self.assertIn("hardlinks", reason)


if __name__ == "__main__":
    unittest.main()
