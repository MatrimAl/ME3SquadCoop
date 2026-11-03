"""Inspect one observed LE3 replication expression, without executing it.

Input is a private, read-only reflection capture, never a game package. This
recognizer accepts only the observed owner/dirty/authority expression and its
resolved property operands. Unsupported metadata stays unknown. In particular,
the expression does not reveal whether its transient gates passed in a live
connection, whether a packet was sent, or whether client prediction reconciled.
"""
import struct


_FIELDS = {
    'AmmoUsedCount': ('IntProperty', 0x3B8, '0x20', None, 23),
    'CurrentSpareAmmo': ('IntProperty', 0x3B4, '0x20', None, 22),
}
_OPERANDS = {
    4: ('bNetOwner', 'BoolProperty', 0x264, '0x22', '0x4000'),
    17: ('bNetDirty', 'BoolProperty', 0x25C, '0x2000', '0x100000'),
    33: ('Role', 'ByteProperty', 0x26A, '0x20', None),
}
# The three process-local UObject pointers are resolved separately. All other
# bytes, including short-circuit lengths, conversion tokens and authority=3,
# must match. This is deliberately not a general UnrealScript decoder.
_TOKENS = {
    0: 0x82, 1: 0x82, 2: 0x2D, 3: 0x01,
    12: 0x18, 13: 0x0B, 14: 0, 15: 0x2D, 16: 0x01,
    25: 0x16, 26: 0x18, 27: 0x12, 28: 0, 29: 0x9A,
    30: 0x38, 31: 0x3A, 32: 0x61,
    41: 0x38, 42: 0x3A, 43: 0x24, 44: 3, 45: 0x16, 46: 0x16,
}


def _property(value, path, kind, offset, flags, mask=None):
    return (isinstance(value, dict) and value.get('path') == path and
            value.get('class') == kind and type(value.get('offset')) is int and
            value['offset'] == offset and value.get('flags') == flags and
            (mask is None or value.get('bool_mask') == mask))


def inspect_ammo_replication(capture):
    """Return a sanitized finding; input pointers and script bytes stay private."""
    failures, found = [], {}
    if not isinstance(capture, dict) or capture.get('read_only') is not True:
        failures.append('read-only capture required')
    entries = capture.get('ammo_replication_properties', []) if isinstance(capture, dict) else []
    if not isinstance(entries, list):
        entries = []
        failures.append('invalid property list')
    for name, (kind, offset, flags, mask, index) in _FIELDS.items():
        path = 'SFXGame.SFXWeapon_NativeBase.' + name
        matches = [entry for entry in entries if isinstance(entry, dict) and
                   isinstance(entry.get('property'), dict) and entry['property'].get('path') == path]
        try:
            if len(matches) != 1:
                raise ValueError('missing or duplicate property')
            entry = matches[0]
            if not _property(entry['property'], path, kind, offset, flags, mask):
                raise ValueError('property layout changed')
            owner = entry.get('owner', {})
            if owner.get('class') != 'Class' or owner.get('path') != 'SFXGame.SFXWeapon_NativeBase':
                raise ValueError('unexpected owner class')
            if type(entry.get('rep_index')) is not int or entry['rep_index'] != index:
                raise ValueError('replication index changed')
            start = entry.get('rep_offset')
            if type(start) is not int or start != 66:
                raise ValueError('replication offset changed')
            encoded = entry.get('class_script_hex')
            if not isinstance(encoded, str) or len(encoded) != 326:
                raise ValueError('invalid class script encoding')
            try:
                code = bytes.fromhex(encoded)
            except ValueError:
                raise ValueError('invalid class script encoding') from None
            if entry.get('class_script_size') != len(code) or len(code) != 163:
                raise ValueError('class script size changed')
            expression = code[start:start + 47]
            if any(expression[at] != token for at, token in _TOKENS.items()):
                raise ValueError('unsupported expression')
            operands = entry.get('condition_operands', {})
            for at, (field, prop_kind, prop_offset, prop_flags, prop_mask) in _OPERANDS.items():
                operand = operands.get(str(start + at), {})
                if not _property(operand, 'Engine.Actor.' + field, prop_kind, prop_offset, prop_flags, prop_mask):
                    raise ValueError('condition operand changed')
                encoded_pointer = operand.get('pointer')
                if (not isinstance(encoded_pointer, str) or not 3 <= len(encoded_pointer) <= 18 or
                        not encoded_pointer.startswith('0x') or
                        any(char not in '0123456789abcdefABCDEF' for char in encoded_pointer[2:])):
                    raise ValueError('invalid condition operand pointer')
                pointer = int(encoded_pointer, 16)
                if pointer <= 0 or struct.unpack_from('<Q', expression, at)[0] != pointer:
                    raise ValueError('condition operand pointer mismatch')
            found[name] = {'replication_index': index, 'replication_offset': start,
                           'condition': 'bNetOwner && bNetDirty && Role == ROLE_Authority'}
        except (ValueError, TypeError, KeyError, IndexError, AttributeError, struct.error) as error:
            failures.append(name + ': ' + str(error))
    recognized = len(found) == len(_FIELDS) and not failures
    return {'condition_recognized': recognized, 'properties': found,
            'owner_is_excluded_by_condition': False if recognized else None,
            'negative_reasons': failures,
            'live_connection_gate_evaluation_verified': False,
            'dirty_gate_failure_verified': False,
            'runtime_ammo_synchronization_verified': False}
