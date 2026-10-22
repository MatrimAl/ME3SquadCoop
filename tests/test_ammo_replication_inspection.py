import copy
from pathlib import Path
import struct
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'src'))
from ammo_replication_inspection import inspect_ammo_replication


def fixture():
    # Synthetic metadata and process-local pointers; no extracted game script.
    code = bytearray(163)
    tokens = {0: 0x82, 1: 0x82, 2: 0x2D, 3: 1, 12: 0x18, 13: 11,
              15: 0x2D, 16: 1, 25: 0x16, 26: 0x18, 27: 18, 29: 0x9A,
              30: 0x38, 31: 0x3A, 32: 0x61, 41: 0x38, 42: 0x3A,
              43: 0x24, 44: 3, 45: 0x16, 46: 0x16}
    for at, token in tokens.items():
        code[66 + at] = token
    operands = {}
    for at, pointer, name, kind, offset, flags, mask in (
            (4, 0x1000, 'bNetOwner', 'BoolProperty', 0x264, '0x22', '0x4000'),
            (17, 0x2000, 'bNetDirty', 'BoolProperty', 0x25C, '0x2000', '0x100000'),
            (33, 0x3000, 'Role', 'ByteProperty', 0x26A, '0x20', None)):
        struct.pack_into('<Q', code, 66 + at, pointer)
        operands[str(66 + at)] = {'path': 'Engine.Actor.' + name, 'class': kind,
                                 'offset': offset, 'flags': flags, 'bool_mask': mask,
                                 'pointer': hex(pointer)}
    entries = []
    for name, offset, index in (('AmmoUsedCount', 0x3B8, 23), ('CurrentSpareAmmo', 0x3B4, 22)):
        entries.append({'property': {'path': 'SFXGame.SFXWeapon_NativeBase.' + name,
                                     'class': 'IntProperty', 'offset': offset, 'flags': '0x20'},
                        'owner': {'class': 'Class', 'path': 'SFXGame.SFXWeapon_NativeBase'},
                        'rep_index': index, 'rep_offset': 66, 'class_script_size': 163,
                        'class_script_hex': code.hex(), 'condition_operands': copy.deepcopy(operands)})
    return {'read_only': True, 'ammo_replication_properties': entries}


class AmmoReplicationInspectionTests(unittest.TestCase):
    def test_recognizes_only_the_condition_without_claiming_runtime_sync(self):
        result = inspect_ammo_replication(fixture())
        self.assertTrue(result['condition_recognized'])
        self.assertFalse(result['owner_is_excluded_by_condition'])
        self.assertFalse(result['live_connection_gate_evaluation_verified'])
        self.assertFalse(result['dirty_gate_failure_verified'])
        self.assertFalse(result['runtime_ammo_synchronization_verified'])
        self.assertNotIn('pointer', str(result))
        self.assertNotIn('class_script_hex', result)

    def test_unknown_and_missing_captures_fail_closed(self):
        for value in (None, [], {}, {'read_only': True},
                      {'read_only': True, 'ammo_replication_properties': None}):
            self.assertFalse(inspect_ammo_replication(value)['condition_recognized'])

    def test_mutated_expression_is_rejected(self):
        for at, value in ((0, 0x84), (13, 10), (29, 0x9B), (44, 2), (46, 0)):
            data = fixture()
            entry = data['ammo_replication_properties'][0]
            code = bytearray.fromhex(entry['class_script_hex'])
            code[66 + at] = value
            entry['class_script_hex'] = code.hex()
            with self.subTest(at=at):
                self.assertFalse(inspect_ammo_replication(data)['condition_recognized'])

    def test_wrong_reference_layout_or_pointer_is_rejected(self):
        for field, value in (('offset', 0), ('bool_mask', '0x1'), ('class', 'IntProperty'),
                             ('path', 'Engine.Actor.bNetInitial'), ('pointer', '0x9999')):
            data = fixture()
            data['ammo_replication_properties'][0]['condition_operands']['70'][field] = value
            with self.subTest(field=field):
                self.assertFalse(inspect_ammo_replication(data)['condition_recognized'])

    def test_duplicate_and_missing_properties_are_rejected(self):
        for delta in ('duplicate', 'missing'):
            data = fixture()
            if delta == 'duplicate':
                data['ammo_replication_properties'].append(copy.deepcopy(data['ammo_replication_properties'][0]))
            else:
                data['ammo_replication_properties'].pop()
            self.assertFalse(inspect_ammo_replication(data)['condition_recognized'])

    def test_changed_property_or_class_metadata_is_rejected(self):
        for field, value in (('rep_offset', 0), ('rep_index', 24), ('class_script_size', 162),
                             ('class_script_hex', 'not hex'), ('owner', None)):
            data = fixture()
            data['ammo_replication_properties'][0][field] = value
            with self.subTest(field=field):
                self.assertFalse(inspect_ammo_replication(data)['condition_recognized'])
        data = fixture()
        data['ammo_replication_properties'][0]['property']['offset'] = 0x3B4
        self.assertFalse(inspect_ammo_replication(data)['condition_recognized'])

    def test_capture_requires_read_only_provenance_label(self):
        data = fixture()
        data['read_only'] = False
        self.assertFalse(inspect_ammo_replication(data)['condition_recognized'])

    def test_malformed_pointer_does_not_echo_private_capture_content(self):
        data = fixture()
        secret = 'private-capture-content'
        data['ammo_replication_properties'][0]['condition_operands']['70']['pointer'] = secret
        result = inspect_ammo_replication(data)
        self.assertFalse(result['condition_recognized'])
        self.assertNotIn(secret, str(result))


if __name__ == '__main__':
    unittest.main()
