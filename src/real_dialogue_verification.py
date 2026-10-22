"""Require an actual full conversation during a guest lease, not a synthetic gate."""


def real_dialogue_evidence(host_rows, guest_rows):
    def full(row):
        return (row.get('narrative_probe') == 'conversation'
                and row.get('current_mode') == 7 and row.get('mode_read_valid') is True
                and row.get('active_array_valid') is True and row.get('active_count', 0) > 0
                and row.get('full_getter_dispatched') is True
                and isinstance(row.get('full_controller'), dict)
                and row['full_controller'].get('status') == 'live'
                and row.get('controller_fields_valid') is True
                and row.get('full_active_memberships') == 1)
    observed = [r for r in host_rows if full(r)]
    successes = []
    for release in host_rows:
        if release.get('session') != 'released' or release.get('reason') != 'scene':
            continue
        at, epoch = release['monotonic_ms'], release.get('epoch')
        actual = [r for r in observed if abs(r['monotonic_ms'] - at) <= 1500]
        engine_gate = any(r.get('session') == 'gate-observation' and r.get('source') == 'engine'
                          and r.get('blocked') is True and r.get('mode') == 7
                          and abs(r['monotonic_ms'] - at) <= 1500 for r in host_rows)
        reacquire = min((r['monotonic_ms'] for r in host_rows if r.get('session') == 'acquired'
                         and r.get('monotonic_ms', 0) > at and r.get('epoch', 0) > epoch
                         and r.get('host_story_preserved') is True
                         and r.get('remote_owns_ashley') is True), default=None)
        detached = bool(reacquire and any(r.get('session') == 'guest-idle' and r.get('has_pawn') is False
                                          and at <= r.get('monotonic_ms', -1) < reacquire for r in guest_rows))
        returned = bool(reacquire and any(r.get('session') == 'guest-observation'
                                         and r.get('reciprocal_controller') is True
                                         and r.get('camera_targets_ashley') is True
                                         and reacquire <= r.get('monotonic_ms', -1) <= reacquire + 5000 for r in guest_rows))
        if (actual and engine_gate and release.get('ai_restored') is True
                and release.get('host_story_preserved') is True and detached and returned):
            successes.append(epoch)
    return {'full_conversation_mode7_samples': len(observed),
            'real_dialogue_control_return_verified': bool(successes),
            'verified_release_epochs': successes,
            'dialogue_camera_playability_verified': False,
            'synthetic_gate_is_dialogue_proof': False,
            'reason': None if successes else 'No verified full dialogue during guest control followed by detach and reacquire.'}
