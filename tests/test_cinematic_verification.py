from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'src'))
from cinematic_verification import cinematic_motion_evidence


def observed_rows():
    rows = []
    action = {'status': 'live', 'pointer': '0x100', 'object_index': 1, 'path': 'Level.Action'}
    for at in (1000, 2000, 3000):
        rows.append({'narrative_probe': 'line-action', 'monotonic_ms': at,
                     'action': action, 'fields_valid': True, 'playing': True, 'paused': False})
        for index in (2, 3):
            camera = {'status': 'live', 'pointer': hex(index * 256),
                      'object_index': index, 'path': f'Level.Camera{index}'}
            rows.extend([
                {'narrative_probe': 'camera-interpolation-membership', 'monotonic_ms': at,
                 'action': action, 'bound_camera': camera, 'physics_read_valid': True, 'physics': 7,
                 'latent_actions_valid': True, 'contains_current_action': True, 'latent_action_count': 1},
                {'narrative_probe': 'camera-movement-track', 'monotonic_ms': at,
                 'bound_camera': camera, 'scope_fields_valid': True, 'world_matches_pc': True,
                 'transform_valid': True, 'track_fields_valid': True,
                 'matching_index_move_instance_valid': True, 'net_mode': 3,
                 'track_disabled': False, 'movement_disabled': False, 'position': [at / 100, index, 0]}
            ])
    return rows


class CinematicEvidenceTests(unittest.TestCase):
    def test_two_independently_observed_moving_cameras(self):
        result = cinematic_motion_evidence([], observed_rows())
        self.assertTrue(result['guest_camera_motion_while_registered_verified'])
        self.assertFalse(result['all_applied_leases_cleaned'])

    def test_success_label_alone_proves_no_motion(self):
        result = cinematic_motion_evidence([], [{'guest_cinematic_registration_adapter': 'applied',
                                                 'lease_token': 1, 'verified': True}])
        self.assertFalse(result['guest_camera_motion_while_registered_verified'])

    def test_cut_target_change_without_transform_motion(self):
        rows = observed_rows()
        for row in rows:
            if 'position' in row:
                row['position'] = [0, 0, 0]
        self.assertFalse(cinematic_motion_evidence([], rows)['guest_camera_motion_while_registered_verified'])

    def test_unregistered_moving_camera_is_insufficient(self):
        rows = observed_rows()
        for row in rows:
            if 'contains_current_action' in row:
                row['contains_current_action'] = False
        self.assertFalse(cinematic_motion_evidence([], rows)['guest_camera_motion_while_registered_verified'])

    def test_wrong_world_or_paused_action_rejected(self):
        for field in ('world_matches_pc', 'playing'):
            rows = observed_rows()
            for row in rows:
                if field in row:
                    row[field] = False
            self.assertFalse(cinematic_motion_evidence([], rows)['guest_camera_motion_while_registered_verified'])

    def test_expired_cleanup_cannot_be_complete(self):
        rows = observed_rows() + [
            {'guest_cinematic_registration_adapter': 'applied', 'lease_token': 1},
            {'guest_cinematic_registration_adapter': 'cleanup-expired', 'lease_token': 1, 'verified': False}]
        self.assertFalse(cinematic_motion_evidence([], rows)['all_applied_leases_cleaned'])


if __name__ == '__main__':
    unittest.main()
