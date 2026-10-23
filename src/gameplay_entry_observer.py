"""Read-only current-map/control observations; never a playable-co-op verdict.

Offsets belong to the exact executable identity required by WindowsProcessMemory.
Unlike UObject counts, the current GWorld chain identifies the loaded map. All
structural fields and identities are checked again before accepting a snapshot.
"""
from __future__ import annotations

import math
import struct
import time

from authoritative_enemy_sampler import (
    AuthoritativeEnemySampler, BudgetedReader, SamplingBudgetExceeded,
    OFFSET_GOBJECTS, OFFSET_GWORLD,
)

OFFSET_GENGINE = 0x18B31D0


class Unavailable(RuntimeError):
    pass


def observe_entry(source, base: int, expected_map: str = "BioP_ProMar", *, deadline=None):
    """One bounded snapshot, with unreadable/changing state treated as unknown."""
    reader = BudgetedReader(source, time.monotonic() + 1.0 if deadline is None else deadline)
    sampler = AuthoritativeEnemySampler(reader, base)
    fields = {}
    identities = {}
    map_fields = map_identities = None
    map_matches = False
    out = {"status": "unknown", "current_map_verified": False,
           "control_gates_clear": False, "ready_for_in_place_listen": False,
           "playable_coop_verified": False, "blockers": [], "read_only": True}

    def field(address, fmt):
        length = struct.calcsize(fmt)
        raw = reader.read(address, length)
        if len(raw) != length:
            raise Unavailable("unreadable field")
        fields[address] = raw
        values = struct.unpack(fmt, raw)
        return values[0] if len(values) == 1 else values

    def obj(pointer, kind=None):
        identity = sampler.object_identity(pointer)
        if identity is None or (kind and not sampler.is_a(pointer, kind)) or sampler.is_cdo(pointer):
            raise Unavailable("invalid " + (kind or "object"))
        identities[pointer] = identity
        return pointer

    def actor(pointer, kind):
        obj(pointer, kind)
        if (field(pointer + 0x0C, "<Q") & (1 << 61) or
                field(pointer + 0x25C, "<I") & 8 or
                field(pointer + 0x260, "<I") & 0x800000):
            raise Unavailable("disposing actor")
        return pointer

    try:
        # Pin the global table header too: relocation/growth during loading
        # invalidates a snapshot rather than accepting mixed object generations.
        field(base + OFFSET_GOBJECTS, "<Qii")
        world = obj(field(base + OFFSET_GWORLD, "<Q"), "World")
        level = obj(field(world + 0x88, "<Q"), "Level")
        if field(level + 0x40, "<Q") != world:
            raise Unavailable("level/world mismatch")
        data, count, capacity = field(level + 0x60, "<Qii")
        if not data or not 1 <= count <= 100000 or not count <= capacity <= 500000:
            raise Unavailable("invalid actor array")
        # This is the exact first-actor chain used by the native runtime.
        info = actor(field(data, "<Q"), "BioWorldInfo")
        if field(info + 0x40, "<Q") != level or field(info + 0x198, "<Q") != info:
            raise Unavailable("WorldInfo reciprocity mismatch")
        root, seen = world, {world}
        for _ in range(16):
            outer = field(root + 0x40, "<Q")
            if not outer:
                if not sampler.is_a(root, "Package"):
                    raise Unavailable("current world has no root package")
                break
            if outer in seen:
                raise Unavailable("cyclic world outer chain")
            seen.add(outer)
            root = obj(outer)
        else:
            raise Unavailable("world outer chain exceeds bound")
        world_path = sampler.get_full_path(world)
        out.update(world_path=world_path, world_identity=identities[world],
                   world_info_identity=identities[info])
        # FName/map comparison is case-insensitive. The live supported Mars
        # root is spelled Biop_ProMar although the travel target is BioP_ProMar.
        map_matches = (sampler.get_object_name(root).casefold() == expected_map.casefold() and
                       field(root + 0x4C, "<I") == 0)
        map_fields, map_identities = dict(fields), dict(identities)

        engine = obj(field(base + OFFSET_GENGINE, "<Q"), "Engine")
        players, player_count, player_capacity = field(engine + 0x498, "<Qii")
        if not players or player_count != 1 or not 1 <= player_capacity <= 128:
            raise Unavailable("requires one local player")
        player = obj(field(players, "<Q"), "LocalPlayer")
        pc = actor(field(player + 0x68, "<Q"), "SFXPlayerController")
        if field(pc + 0x67C, "<Q") != player or field(pc + 0x198, "<Q") != info:
            raise Unavailable("local-controller reciprocity mismatch")
        # The supported live Shepard inherits SFXPawn_Player, not BioPlayerPawn.
        pawn = actor(field(pc + 0x3BC, "<Q"), "SFXPawn_Player")
        if field(pawn + 0x374, "<Q") != pc or field(pawn + 0x198, "<Q") != info:
            raise Unavailable("pawn reciprocity mismatch")
        manager = obj(field(pc + 0xC38, "<Q"), "SFXGameModeManager")
        mode = field(manager + 0xA8, "<B")
        flags = field(info + 0x7C0, "<I")
        cinematic = bool(field(pc + 0x77C, "<I") & 0x20000)
        pauser = field(info + 0x670, "<Q")
        driver = field(world + 0x120, "<Q")
        pending = field(engine + 0x9FC, "<Q")
        net_mode = field(info + 0x7C8, "<B")
        conversation = field(info + 0x948, "<Q")
        active = activating = 0
        if conversation:
            obj(conversation, "BioConversationManager")
            _, active, maximum = field(conversation + 0xC8, "<Qii")
            if not 0 <= active <= maximum <= 4096:
                raise Unavailable("invalid active-conversation array")
            activating = field(conversation + 0xE8, "<I") & 1
        # Time is expected to advance, so do not require it to remain identical
        # while the structural snapshot is checked. Never turn read failure into 0.
        world_time = reader.read_float(info + 0x74C)
        if world_time is None:
            raise Unavailable("unreadable world time")
        for address, raw in fields.items():
            if reader.read(address, len(raw)) != raw:
                raise Unavailable("context changed during snapshot")
        for pointer, identity in identities.items():
            if sampler.object_identity(pointer) != identity:
                raise Unavailable("object identity changed during snapshot")
        blockers = []
        for blocked, reason in (
                (not map_matches, "different_current_map"), (mode != 0, "non_ordinary_mode"),
                (flags & 0x400, "players_only"), (flags & 0x800, "world_paused_flag"),
                (cinematic, "cinematic_mode"), (pauser, "pauser_present"),
                (active, "active_conversation"), (activating, "conversation_activating")):
            if blocked:
                blockers.append(reason)
        clear = not blockers
        out.update(status="ok", current_map_verified=map_matches,
                   control_gates_clear=clear, ready_for_in_place_listen=clear and net_mode == 0 and not driver and not pending,
                   blockers=blockers, net_mode=net_mode, mode=mode,
                   players_only=bool(flags & 0x400), world_paused_flag=bool(flags & 0x800),
                   cinematic_mode=cinematic, pauser_present=bool(pauser),
                   active_conversations=active, conversation_activating=bool(activating),
                   driver_present=bool(driver), pending_travel_present=bool(pending),
                   world_time=world_time, controller_identity=identities[pc],
                   pawn_identity=identities[pawn])
    except (Unavailable, SamplingBudgetExceeded) as exc:
        out["blockers"] = [str(exc)]
        # A loaded map can be established before a controller/pawn exists. A
        # failed later control read must neither invent control nor hide that
        # narrower observation. Recheck only the captured current-map chain.
        try:
            if map_fields is not None:
                out["current_map_verified"] = (map_matches and
                    all(reader.read(address, len(raw)) == raw for address, raw in map_fields.items()) and
                    all(sampler.object_identity(pointer) == identity for pointer, identity in map_identities.items()))
        except SamplingBudgetExceeded:
            out["current_map_verified"] = False
    return out


def evaluate_entry_interval(samples, minimum_seconds=2.0, maximum_gap_seconds=1.5):
    """Require uninterrupted identity, gates and advancing native world time.

    A short clear interval helps decide when to request native Listen. It does
    not prove that Listen, networking, combat, or the mission works.
    """
    if not math.isfinite(minimum_seconds) or minimum_seconds <= 0:
        raise ValueError("minimum_seconds must be finite and positive")
    if not math.isfinite(maximum_gap_seconds) or maximum_gap_seconds <= 0:
        raise ValueError("maximum_gap_seconds must be finite and positive")
    start = previous = None
    for row in samples:
        identity = tuple(row.get(key) for key in
                         ("world_identity", "world_info_identity", "controller_identity", "pawn_identity"))
        now, native_time = row.get("elapsed_seconds"), row.get("world_time")
        valid = (row.get("status") == "ok" and row.get("ready_for_in_place_listen") is True
                 and all(identity) and isinstance(now, (int, float)) and math.isfinite(now)
                 and isinstance(native_time, (int, float)) and math.isfinite(native_time))
        if not valid:
            start = previous = None
            continue
        if (previous is None or previous[0] != identity or now <= previous[1] or
                now - previous[1] > maximum_gap_seconds or native_time < previous[2]):
            start = (now, native_time)
        previous = (identity, now, native_time)
        if now - start[0] >= minimum_seconds and native_time > start[1]:
            return True
    return False
