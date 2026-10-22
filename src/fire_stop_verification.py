"""Read-only pulse/release evidence; a bFire reset alone cannot pass this gate."""


def _id(value):
    if isinstance(value, dict) and value.get('status') == 'live':
        return value.get('pointer'), value.get('object_index'), value.get('path')
    return None


def _stable_counts(rows):
    if len(rows) < 3 or rows[-1]['monotonic_ms'] - rows[0]['monotonic_ms'] < 2000:
        return None
    if any(b['monotonic_ms'] - a['monotonic_ms'] > 2200 for a, b in zip(rows, rows[1:])):
        return None
    values = [r['ammo_used_count'] for r in rows]
    if any(b < a for a, b in zip(values, values[1:])):
        return None  # Reload/correction needs separate interpretation.
    if len({r.get('current_spare_ammo') for r in rows}) != 1:
        return None
    return len(set(values)) == 1


def fire_stop_evidence(host_rows, guest_rows):
    pulses = [r for r in guest_rows if r.get('combat_fire_bot') == 'pressed'
              and r.get('ordinary_script_dispatched') is True
              and r.get('bfire_before_valid') is True and r.get('bfire_before') == 0
              and r.get('bfire_after_valid') is True and r.get('bfire_after') == 1]
    results = []
    for press in pulses:
        start = press['monotonic_ms']
        releases = [r for r in guest_rows if r.get('combat_fire_bot') == 'released'
                    and r.get('lease_token') == press.get('lease_token')
                    and r.get('monotonic_ms', 0) > start
                    and r.get('ordinary_script_dispatched') is True
                    and r.get('bfire_after_valid') is True and r.get('bfire_after') == 0]
        release = min((r['monotonic_ms'] for r in releases), default=None)
        acquire = max((r['monotonic_ms'] for r in host_rows if r.get('session') == 'acquired'
                       and r.get('remote_owns_ashley') is True and r.get('host_story_preserved') is True
                       and r['monotonic_ms'] <= start), default=None)
        end_control = min((r['monotonic_ms'] for r in host_rows if r.get('session') == 'released'
                           and acquire is not None and r['monotonic_ms'] > acquire), default=None)
        next_press = min((r['monotonic_ms'] for r in pulses if r['monotonic_ms'] > start), default=None)
        end = min([release + 6000 if release is not None else start] +
                  [v for v in (end_control, next_press) if v is not None])
        valid_control = acquire is not None and end_control is not None and release is not None and release < end_control
        sides = {}
        for role, source in (('host-ashley', host_rows), ('guest-local', guest_rows)):
            equipment = [r for r in source if r.get('combat_equipment_probe') == 'weapon'
                         and r.get('role') == role and r.get('ammo_counts_read_valid') is True
                         and _id(r.get('weapon')) is not None]
            baselines = [r for r in equipment if start - 3000 <= r['monotonic_ms'] <= start]
            baseline = max(baselines, key=lambda r: r['monotonic_ms'], default=None)
            identity = _id(baseline.get('weapon')) if baseline else None
            observations = [r for r in equipment if release is not None
                            and release + 1000 <= r['monotonic_ms'] <= end]
            same = bool(identity and observations and all(_id(r.get('weapon')) == identity for r in observations))
            if role == 'guest-local' and baseline:
                same = same and baseline['weapon'].get('path') == press.get('weapon_path')
            stable = _stable_counts(observations) if same else None
            shot = bool(baseline and any(r['ammo_used_count'] > baseline['ammo_used_count']
                        for r in equipment if start < r['monotonic_ms'] <= end
                        and _id(r.get('weapon')) == identity))
            sides[role] = {'same_weapon_observed': same, 'shot_consumption_observed': shot,
                           'post_release_counts': [r['ammo_used_count'] for r in observations],
                           'post_release_sample_times': [r['monotonic_ms'] for r in observations],
                           'stable_after_one_second_grace': stable}
        stopped = valid_control and all(s['stable_after_one_second_grace'] is True for s in sides.values())
        continued = valid_control and any(s['stable_after_one_second_grace'] is False for s in sides.values())
        results.append({'lease_token': press.get('lease_token'), 'pressed_at': start, 'released_at': release,
                        'same_host_control_window': valid_control, 'sides': sides,
                        'post_release_ammo_agreement': bool(stopped and
                            sides['host-ashley']['post_release_counts'][-1] == sides['guest-local']['post_release_counts'][-1]),
                        'stop_verified': stopped, 'continued_consumption_observed': continued})
    return {'pulses': results, 'one_second_release_grace_ms': 1000,
            'fire_stop_verified': bool(results) and all(r['stop_verified'] for r in results),
            'continued_consumption_observed': any(r['continued_consumption_observed'] for r in results),
            'enemy_damage_verified': False, 'full_combat_verified': False}
