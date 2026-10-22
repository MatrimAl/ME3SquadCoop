"""Independent evidence gate for the guest-local player context experiment.

Requires observed native getter/ownership samples and ordinary teardown. An
adapter's success message alone cannot pass. This does not verify shooting.
"""


def _pointer(value):
    if not isinstance(value, str):
        return None
    try:
        return int(value, 16) if value.startswith('0x') else None
    except ValueError:
        return None


def _identity(value):
    if value is None:
        return 0
    if isinstance(value, dict) and value.get('status') == 'live':
        return _pointer(value.get('pointer'))
    return None


def guest_context_evidence(host_rows, guest_rows):
    events = [r for r in guest_rows if 'guest_player_context_adapter' in r]
    applied = [r for r in events if r.get('guest_player_context_adapter') == 'applied']
    snapshots = {}
    for r in guest_rows:
        kind, at = r.get('combat_context_probe'), r.get('monotonic_ms')
        if kind and type(at) is int:
            snapshots.setdefault(at, {})[kind] = r
    leases, reasons = [], []
    for apply in applied:
        token = apply.get('lease_token')
        own = [r for r in events if r.get('lease_token') == token]
        squad, pawn = _pointer(apply.get('squad_pointer')), _pointer(apply.get('controlled_pawn'))
        info, baseline = _pointer(apply.get('world_info')), _pointer(apply.get('pawn_squad_baseline'))
        start = apply.get('monotonic_ms')
        metadata = (type(token) is int and token > 0 and squad and pawn and info
                    and type(start) is int and apply.get('verified') is True
                    and apply.get('registered') is True
                    and _pointer(apply.get('world_slot_baseline')) == 0
                    and baseline is not None and apply.get('getter_before') is False
                    and apply.get('getter_after') is True)
        restores = [r for r in own if r.get('guest_player_context_adapter') == 'world-slot-restored'
                    and r.get('verified') is True and type(r.get('monotonic_ms')) is int
                    and type(start) is int and r['monotonic_ms'] >= start]
        stop = min((r['monotonic_ms'] for r in restores), default=None)
        after_apply = [r for r in own if r.get('guest_player_context_adapter') == 'gate-observation'
                       and r.get('phase') == 'after-apply']
        before_apply = any(r.get('guest_player_context_adapter') == 'gate-observation'
                           and r.get('phase') == 'before-apply' and r.get('getter_dispatched') is True
                           and r.get('getter_result') is False and r.get('derived_gate') is False
                           and r.get('getter_derived_agree') is True and _pointer(r.get('world_slot')) == 0 for r in own)
        transition = before_apply and any(r.get('getter_dispatched') is True and r.get('getter_result') is True
                         and r.get('derived_gate') is True and r.get('getter_derived_agree') is True
                         and r.get('flags_unchanged') is True and r.get('squad_flags_raw') == 14
                         and _pointer(r.get('world_slot')) == squad
                         and _pointer(r.get('cached_pawn')) == pawn for r in after_apply)
        positive, negative, membership_changes = [], 0, 0
        if metadata:
            for at, sample in snapshots.items():
                if at < start or (stop is not None and at >= stop):
                    continue
                local, world = sample.get('local-context', {}), sample.get('native-world-chain', {})
                gate, comparison = sample.get('native-gate', {}), sample.get('context-comparison', {})
                known = all(world.get(k) is True for k in (
                    'layout_pinned', 'world_info_class_valid', 'same_as_pc_world_info',
                    'native_dereferences_safe', 'pre_getter_stable', 'post_getter_stable'))
                known = known and all(local.get(k) is True for k in (
                    'pc_class_valid', 'pawn_class_valid', 'input_class_valid',
                    'reciprocal_possession', 'acknowledged_pawn_matches', 'local_snapshot_stable'))
                known = known and local.get('net_mode') == 3 and local.get('current_mode') == 0
                known = known and (local.get('pawn') or {}).get('class') == 'SFXPawn_Ashley'
                known = known and _identity(local.get('pawn')) == pawn and _identity(local.get('pc_world_info')) == info
                known = known and gate.get('getter_dispatched') is True and gate.get('getter_result_valid') is True
                if not known:
                    continue
                if _identity(comparison.get('controlled_pawn_squad')) != baseline:
                    membership_changes += 1
                    continue
                valid = (_identity(gate.get('world_player_squad')) == squad
                         and (gate.get('world_player_squad') or {}).get('class') == 'BioPlayerSquad'
                         and _identity(gate.get('cached_player_pawn')) == pawn
                         and gate.get('squad_flags_read_valid') is True and gate.get('squad_flags_raw') == 14
                         and gate.get('cached_is_local_pawn') is True
                         and gate.get('getter_result') is True and gate.get('derived_gate') is True
                         and gate.get('getter_agrees_with_derived') is True)
                if valid:
                    positive.append(at)
                else:
                    negative += 1
        duration = max(positive) - min(positive) if positive else 0
        destroyed = any(r.get('guest_player_context_adapter') == 'destroy-observation'
                        and r.get('destroy_dispatched') is True and r.get('destroy_return') is True
                        and r.get('disposed_observed') is True for r in own)
        cleaned = any(r.get('guest_player_context_adapter') == 'cleanup-success'
                      and r.get('verified') is True for r in own)
        final_gate = any(r.get('guest_player_context_adapter') == 'gate-observation'
                         and r.get('phase') == 'after-cleanup'
                         and _pointer(r.get('world_slot')) == 0
                         and r.get('getter_dispatched') is True and r.get('getter_result') is False
                         and r.get('derived_gate') is False and r.get('getter_derived_agree') is True for r in own)
        passed = bool(metadata and transition and len(positive) >= 10 and duration >= 9000
                      and not negative and not membership_changes and restores and destroyed and cleaned and final_gate)
        leases.append({'token': token, 'metadata_valid': bool(metadata), 'getter_transition_verified': transition,
                       'positive_native_samples': len(positive), 'positive_span_ms': duration,
                       'contradictory_native_samples': negative, 'pawn_membership_changes': membership_changes,
                       'world_slot_restored': bool(restores), 'ordinary_destroy_verified': destroyed,
                       'cleanup_success': cleaned, 'post_cleanup_gate_false_verified': final_gate,
                       'roundtrip_verified': passed})
    host_writes = [r for r in host_rows if r.get('guest_player_context_adapter') in
                   ('applied', 'world-slot-restored', 'destroy-observation')]
    if len(leases) < 2:
        reasons.append('fewer than two observed context applications')
    if any(not r['roundtrip_verified'] for r in leases):
        reasons.append('native context or ordinary teardown evidence incomplete')
    if len({r['token'] for r in leases}) != len(leases):
        reasons.append('duplicate context lease token')
    if host_writes:
        reasons.append('guest context mutation observed in host process')
    exceptions = sum('exception' in r for r in host_rows + guest_rows)
    if exceptions:
        reasons.append('native exception observed')
    return {'lease_count': len(leases), 'leases': leases, 'host_mutation_event_count': len(host_writes),
            'native_exception_count': exceptions, 'negative_reasons': reasons,
            'guest_context_roundtrip_verified': not reasons,
            'firing_or_damage_verified': False, 'campaign_coop_verified': False}
