import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'src'))
from fire_stop_verification import fire_stop_evidence


def fixture():
    host = [{'session': 'acquired', 'monotonic_ms': 0, 'remote_owns_ashley': True, 'host_story_preserved': True},
            {'session': 'released', 'monotonic_ms': 10000}]
    guest = [{'combat_fire_bot': 'pressed', 'monotonic_ms': 1000, 'lease_token': 1,
              'ordinary_script_dispatched': True, 'bfire_before_valid': True, 'bfire_before': 0,
              'bfire_after_valid': True, 'bfire_after': 1, 'weapon_path': 'World.Weapon'},
             {'combat_fire_bot': 'released', 'monotonic_ms': 1250, 'lease_token': 1,
              'ordinary_script_dispatched': True, 'bfire_after_valid': True, 'bfire_after': 0}]
    for role, rows in [('host-ashley', host), ('guest-local', guest)]:
        for at, count in [(900, 0), (3000, 1), (4000, 1), (5000, 1), (6000, 1)]:
            rows.append({'combat_equipment_probe': 'weapon', 'role': role, 'monotonic_ms': at,
                         'ammo_counts_read_valid': True, 'ammo_used_count': count, 'current_spare_ammo': 210,
                         'weapon': {'status': 'live', 'pointer': '0x100', 'object_index': 1, 'path': 'World.Weapon'}})
    return host, guest


class FireStopTests(unittest.TestCase):
    def test_stable_real_counts_after_release(self):
        result = fire_stop_evidence(*fixture())
        self.assertTrue(result['fire_stop_verified'])
        self.assertTrue(result['pulses'][0]['post_release_ammo_agreement'])

    def test_equal_magazine_counts_with_different_reserves_are_not_agreement(self):
        host, guest = fixture()
        for row in guest:
            if 'current_spare_ammo' in row:
                row['current_spare_ammo'] = 90
        result = fire_stop_evidence(host, guest)
        self.assertTrue(result['fire_stop_verified'])
        pulse = result['pulses'][0]
        self.assertFalse(pulse['post_release_ammo_agreement'])
        self.assertEqual(pulse['sides']['guest-local']['post_release_spare_counts'], [90] * 4)

    def test_bfire_reset_with_continued_authoritative_consumption_fails(self):
        host, guest = fixture()
        for r in host:
            if r.get('combat_equipment_probe') == 'weapon' and r['monotonic_ms'] >= 3000:
                r['ammo_used_count'] = r['monotonic_ms'] // 1000
        result = fire_stop_evidence(host, guest)
        self.assertFalse(result['fire_stop_verified'])
        self.assertTrue(result['continued_consumption_observed'])

    def test_no_authoritative_control_window(self):
        host, guest = fixture()
        host = [r for r in host if r.get('session') != 'acquired']
        self.assertFalse(fire_stop_evidence(host, guest)['fire_stop_verified'])

    def test_weapon_swap_is_unknown(self):
        host, guest = fixture()
        host[-1] = copy.deepcopy(host[-1]); host[-1]['weapon']['pointer'] = '0x200'
        self.assertIsNone(fire_stop_evidence(host, guest)['pulses'][0]['sides']['host-ashley']['stable_after_one_second_grace'])

    def test_reload_is_not_stopping_proof(self):
        host, guest = fixture(); host[-1]['ammo_used_count'] = 0
        self.assertFalse(fire_stop_evidence(host, guest)['fire_stop_verified'])

    def test_empty_success_labels_are_insufficient(self):
        self.assertFalse(fire_stop_evidence([], [{'firing_verified': True}])['fire_stop_verified'])

    def test_stopped_does_not_mean_ammo_agreement(self):
        host, guest = fixture()
        for r in guest:
            if r.get('ammo_used_count') == 1:
                r['ammo_used_count'] = 3
        result = fire_stop_evidence(host, guest)
        self.assertTrue(result['fire_stop_verified'])
        self.assertFalse(result['pulses'][0]['post_release_ammo_agreement'])

    def test_no_shots_cannot_prove_stopping(self):
        host,guest=fixture()
        for row in host+guest:
            if 'ammo_used_count' in row:row['ammo_used_count']=0
        result=fire_stop_evidence(host,guest)
        self.assertFalse(result['fire_stop_verified'])
        self.assertTrue(result['pulses'][0]['post_release_counts_stable'])

    def test_client_prediction_without_authoritative_shot_cannot_pass(self):
        host,guest=fixture()
        for row in host:
            if 'ammo_used_count' in row:row['ammo_used_count']=0
        result=fire_stop_evidence(host,guest)
        self.assertFalse(result['fire_stop_verified'])
        self.assertFalse(result['pulses'][0]['authoritative_and_guest_firing_observed'])


if __name__ == '__main__':
    unittest.main()
