"""Evaluate native camera observations separately from adapter success labels."""
import math


def _key(identity):
    if not isinstance(identity, dict) or identity.get('status') != 'live':
        return None
    return identity.get('pointer'), identity.get('object_index'), identity.get('path')


def cinematic_motion_evidence(host_rows, guest_rows):
    samples = {}
    for row in guest_rows:
        at = row.get('monotonic_ms')
        if type(at) is int:
            samples.setdefault(at, []).append(row)
    cameras = {}
    for at, rows in samples.items():
        playing = {_key(r.get('action')) for r in rows
                   if r.get('narrative_probe') == 'line-action'
                   and r.get('fields_valid') is True and r.get('playing') is True
                   and r.get('paused') is False}
        members = {_key(r.get('bound_camera')) for r in rows
                   if r.get('narrative_probe') == 'camera-interpolation-membership'
                   and _key(r.get('action')) in playing
                   and r.get('physics_read_valid') is True and r.get('physics') == 7
                   and r.get('latent_actions_valid') is True
                   and r.get('contains_current_action') is True
                   and r.get('latent_action_count', 0) >= 1}
        for r in rows:
            key = _key(r.get('bound_camera'))
            if r.get('narrative_probe') != 'camera-movement-track' or key is None or key not in members:
                continue
            required = ('scope_fields_valid', 'world_matches_pc', 'transform_valid',
                        'track_fields_valid', 'matching_index_move_instance_valid')
            if not all(r.get(k) is True for k in required) or r.get('net_mode') != 3:
                continue
            if r.get('track_disabled') is not False or r.get('movement_disabled') is not False:
                continue
            position = r.get('position')
            if (not isinstance(position, list) or len(position) != 3
                    or not all(type(v) in (int, float) and math.isfinite(v) for v in position)):
                continue
            cameras.setdefault(key, {})[at] = tuple(position)
    results = []
    for key, positions in cameras.items():
        values = list(positions.values())
        extent = math.sqrt(sum((max(p[i] for p in values) - min(p[i] for p in values))**2 for i in range(3)))
        span = max(positions) - min(positions)
        verified = len(positions) >= 3 and len(set(values)) >= 3 and span >= 2000 and extent >= 5
        results.append({'camera_path': key[2], 'native_registered_samples': len(positions),
                        'sample_span_ms': span, 'distinct_positions': len(set(values)),
                        'position_extent': extent, 'motion_verified': verified})
    events = [r for r in guest_rows if 'guest_cinematic_registration_adapter' in r]
    applies = [r for r in events if r.get('guest_cinematic_registration_adapter') == 'applied']
    tokens = {r.get('lease_token') for r in applies}
    cleaned = {r.get('lease_token') for r in events
               if r.get('guest_cinematic_registration_adapter') == 'cleanup-complete'
               and r.get('verified') is True}
    expired = sum(r.get('guest_cinematic_registration_adapter') == 'cleanup-expired' for r in events)
    rejected = sum(r.get('guest_cinematic_registration_adapter') in ('suppressed', 'cleanup-rejected') for r in events)
    host_mutations = sum(r.get('guest_cinematic_registration_adapter') == 'applied' for r in host_rows)
    exceptions = sum('exception' in r for r in host_rows + guest_rows)
    motion = sum(r['motion_verified'] for r in results) >= 2 and not exceptions and not host_mutations
    return {'cameras': results, 'guest_camera_motion_while_registered_verified': motion,
            'applied_leases': len(applies), 'cleanup_verified_leases': len(tokens & cleaned),
            'expired_unverified_cleanup_events': expired, 'rejected_cleanup_or_suppression_events': rejected,
            'all_applied_leases_cleaned': bool(tokens) and tokens <= cleaned and not expired and not rejected,
            'host_mutation_events': host_mutations, 'native_exception_count': exceptions,
            'real_dialogue_control_return_verified': False, 'campaign_coop_verified': False}
