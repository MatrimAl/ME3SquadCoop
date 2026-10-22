import copy
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'src'))
from guest_context_verification import guest_context_evidence


def identity(pointer, kind):
    return {'status': 'live', 'pointer': pointer, 'class': kind}


def fixture():
    rows = []
    for token, start in ((1, 1000), (2, 21000)):
        squad, pawn, info = hex(0x100 + token), '0x200', '0x300'
        def event(name, **fields):
            return dict(guest_player_context_adapter=name, lease_token=token,
                        monotonic_ms=start, **fields)
        rows += [event('gate-observation', phase='before-apply', world_slot='0x0',
                       getter_dispatched=True, getter_result=False, derived_gate=False, getter_derived_agree=True),
                 event('gate-observation', phase='after-apply', world_slot=squad, cached_pawn=pawn,
                       getter_dispatched=True, getter_result=True, derived_gate=True, getter_derived_agree=True,
                       flags_unchanged=True, squad_flags_raw=14),
                 event('applied', squad_pointer=squad, controlled_pawn=pawn, world_info=info,
                       pawn_squad_baseline='0x400', world_slot_baseline='0x0', verified=True,
                       registered=True, getter_before=False, getter_after=True)]
        for i in range(10):
            at = start + i * 1000
            rows += [dict(combat_context_probe='local-context', monotonic_ms=at,
                          pc_class_valid=True, pawn_class_valid=True, input_class_valid=True,
                          reciprocal_possession=True, acknowledged_pawn_matches=True, local_snapshot_stable=True,
                          net_mode=3, current_mode=0, pawn=identity(pawn, 'SFXPawn_Ashley'),
                          pc_world_info=identity(info, 'BioWorldInfo')),
                     dict(combat_context_probe='native-world-chain', monotonic_ms=at,
                          layout_pinned=True, world_info_class_valid=True, same_as_pc_world_info=True,
                          native_dereferences_safe=True, pre_getter_stable=True, post_getter_stable=True),
                     dict(combat_context_probe='native-gate', monotonic_ms=at,
                          getter_dispatched=True, getter_result_valid=True, getter_result=True,
                          derived_gate=True, getter_agrees_with_derived=True, squad_flags_read_valid=True,
                          squad_flags_raw=14, cached_is_local_pawn=True,
                          world_player_squad=identity(squad, 'BioPlayerSquad'), cached_player_pawn=identity(pawn, 'SFXPawn_Ashley')),
                     dict(combat_context_probe='context-comparison', monotonic_ms=at,
                          controlled_pawn_squad=identity('0x400', 'BioPlayerSquad'))]
        tail = [event('world-slot-restored', verified=True),
                event('destroy-observation', destroy_dispatched=True, destroy_return=True, disposed_observed=True),
                event('cleanup-success', verified=True),
                event('gate-observation', phase='after-cleanup', world_slot='0x0', getter_dispatched=True,
                      getter_result=False, derived_gate=False, getter_derived_agree=True)]
        for r in tail:
            r['monotonic_ms'] = start + 12000
        rows += tail
    return rows


class GuestContextEvidenceTests(unittest.TestCase):
    def test_two_real_gate_windows_and_ordinary_cleanup(self):
        result = guest_context_evidence([], fixture())
        self.assertTrue(result['guest_context_roundtrip_verified'])
        self.assertFalse(result['firing_or_damage_verified'])

    def assert_rejected(self, rows):
        self.assertFalse(guest_context_evidence([], rows)['guest_context_roundtrip_verified'])

    def test_success_messages_without_native_samples(self):
        self.assert_rejected([r for r in fixture() if not r.get('combat_context_probe')])

    def test_prewarm_positive_getter_is_not_connected_proof(self):
        rows = fixture()
        for r in rows:
            if r.get('combat_context_probe') == 'local-context': r['net_mode'] = 0
        self.assert_rejected(rows)

    def test_true_message_with_false_actual_getter(self):
        rows = fixture()
        next(r for r in rows if r.get('combat_context_probe') == 'native-gate')['getter_result'] = False
        self.assert_rejected(rows)

    def test_changed_original_pawn_membership(self):
        rows = fixture()
        next(r for r in rows if r.get('combat_context_probe') == 'context-comparison')['controlled_pawn_squad'] = None
        self.assert_rejected(rows)

    def test_destroy_dispatch_without_actual_disposal(self):
        rows = fixture()
        next(r for r in rows if r.get('guest_player_context_adapter') == 'destroy-observation')['disposed_observed'] = False
        self.assert_rejected(rows)

    def test_missing_null_slot_restore(self):
        self.assert_rejected([r for r in fixture() if r.get('guest_player_context_adapter') != 'world-slot-restored'])

    def test_no_before_getter_proof(self):
        self.assert_rejected([r for r in fixture() if r.get('phase') != 'before-apply'])

    def test_duplicate_epoch(self):
        rows = fixture()
        rows.append(copy.deepcopy(next(r for r in rows if r.get('guest_player_context_adapter') == 'applied')))
        self.assert_rejected(rows)

    def test_guest_adapter_must_not_mutate_host_context(self):
        rows = fixture()
        host = [next(r for r in rows if r.get('guest_player_context_adapter') == 'applied')]
        self.assertFalse(guest_context_evidence(host, rows)['guest_context_roundtrip_verified'])


if __name__ == '__main__':
    unittest.main()
