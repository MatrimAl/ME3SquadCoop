"""Authored format and descriptor checks; no Mod Manager or game execution."""
import configparser
import importlib.util
from pathlib import Path
import struct
import sys
import unittest

PROJECT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PROJECT / "src"))
import package_manifest as package


class ActivationDlcTests(unittest.TestCase):
    def builder(self):
        spec = importlib.util.spec_from_file_location("activation_builder", PROJECT / "tools/build_shareable.py")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        return module

    def test_mount_matches_reviewed_le3_binary_layout(self):
        # Independent fixed byte vector for priority 5100; see pinned primary
        # layout, offsets and interpretation in docs/DLC-PACKAGING.md.
        header = bytes.fromhex("01000000 ad020000 cd000000 6b000300 ec130000 "
                               "00000000 0c000000 00000000 00000000")
        expected = header + bytes(72)
        self.assertEqual(package.make_activation_mount(), expected)
        self.assertEqual(self.builder().generate_mount_dlc(), expected)
        readback = package.read_activation_mount(expected)
        self.assertEqual(readback["game"], "LE3")
        self.assertEqual(readback["mount_priority"], 5100)
        self.assertFalse(readback["save_dependency"])
        self.assertTrue(readback["loads_in_singleplayer"])
        self.assertTrue(readback["loads_in_multiplayer"])

    def test_mount_rejects_legacy_versions_flags_tlk_and_reserved_data(self):
        for offset, value in ((0, 0), (4, 0x2AC), (8, 0xC2), (12, 0), (16, 65536),
                              (20, 1), (24, 0x14), (24, 0x0D), (28, 1), (32, 1), (44, 1), (104, 1)):
            raw = bytearray(package.make_activation_mount())
            struct.pack_into("<I", raw, offset, value)
            with self.subTest(offset=offset, value=value), self.assertRaises(ValueError):
                package.read_activation_mount(bytes(raw))

    def test_mount_rejects_truncation_trailing_data_and_invalid_priority(self):
        raw = package.make_activation_mount()
        for bad in (b"", raw[:-1], raw + b"\0", None):
            with self.subTest(size=len(bad) if bad is not None else None), self.assertRaises(ValueError):
                package.read_activation_mount(bad)
        for priority in (False, True, 0, -1, 65536, 1.5, "5100"):
            with self.subTest(priority=priority), self.assertRaises(ValueError):
                package.make_activation_mount(priority)
        for priority in (1, 65535):
            self.assertEqual(package.read_activation_mount(package.make_activation_mount(priority))["mount_priority"], priority)

    def test_descriptor_uses_le3_custom_dlc_and_no_native_install_job(self):
        parser = configparser.ConfigParser(interpolation=None)
        parser.read_string(self.builder().generate_moddesc_ini())
        self.assertEqual(set(parser.sections()), {"ModManager", "ModInfo", "CUSTOMDLC"})
        self.assertEqual(parser["ModManager"]["cmmver"], "7.0")
        self.assertEqual(parser["ModInfo"]["game"], "LE3")
        self.assertEqual(parser["CUSTOMDLC"]["sourcedirs"], "DLC_MOD_ME3SquadCoop")
        self.assertEqual(parser["CUSTOMDLC"]["destdirs"], "DLC_MOD_ME3SquadCoop")
        self.assertIn("Unfinished", parser["ModInfo"]["moddesc"])
        self.assertNotIn("requireddlc", parser["ModInfo"])
        self.assertNotIn("asi", parser["ModManager"])

    def test_descriptor_rejects_old_game_id_unsupported_version_and_ini_injection(self):
        module = self.builder()
        for kwargs in ({"game": "ME3LE"}, {"game": "ME3"}, {"cmmver": "6.0"},
                       {"version": "0.1.0\n[BASEGAME]\nmoddir=bad"}, {"version": None}):
            with self.subTest(kwargs=kwargs), self.assertRaises(ValueError):
                module.generate_moddesc_ini(**kwargs)

    def test_automatic_archive_cannot_declare_release(self):
        with self.assertRaisesRegex(ValueError, "candidate-only"):
            self.builder().build_deployment_archive("unused.7z", channel="release")


if __name__ == "__main__":
    unittest.main()
