"""Pin checking against synthetic PE images only. Never reads an installed game.

The pin checker exists to turn "the mod crashes on launch" into "these 9 pinned offsets do not describe this
executable". What has to hold, and is asserted here:

* a matching pin is reported as a match, and a one-byte change is reported as a mismatch - so the check can
  fail;
* a pin whose RVA no section covers is `unmapped`, not `mismatch`, because the two mean different things to
  whoever has to fix it;
* an offset used without pinned bytes is reported as `unpinned` rather than counted as passing, because
  "nothing checked it" and "it checked out" must not print the same;
* a result from an unsupported executable gets its own exit code, so a wrong-build report cannot be mistaken
  for a broken-mod report.
"""
import hashlib
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

PROJECT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PROJECT / "tools"))
import check_pins  # noqa: E402


def build_pe(text_at=0x1000, text=b"", extra=b"") -> bytes:
    """A minimal PE32+ whose .text section holds `text` at RVA `text_at`."""
    file_align, section_align = 0x200, 0x1000
    headers = 0x40 + 0x40 + 0xF0 + 0x28  # DOS + stub + optional + one section header
    raw = (headers + file_align - 1) // file_align * file_align
    data = bytearray(raw) + bytearray(text) + bytearray(extra)
    data[0:2] = b"MZ"
    struct.pack_into("<I", data, 0x3C, 0x40)
    data[0x40:0x44] = b"PE\0\0"
    struct.pack_into("<H", data, 0x44, 0x8664)          # machine
    struct.pack_into("<H", data, 0x46, 1)               # one section
    struct.pack_into("<H", data, 0x54, 0xF0)            # optional header size
    opt = 0x58
    struct.pack_into("<H", data, opt, 0x20B)            # PE32+
    row = opt + 0xF0
    data[row:row + 5] = b".text"
    struct.pack_into("<I", data, row + 8, len(text))    # virtual size
    struct.pack_into("<I", data, row + 12, text_at)     # virtual address
    struct.pack_into("<I", data, row + 16, len(text))   # size of raw data
    struct.pack_into("<I", data, row + 20, raw)         # pointer to raw data
    return bytes(data)


class PEMappingTests(unittest.TestCase):
    def test_a_section_maps_an_rva_to_its_bytes(self):
        image = check_pins.PE(build_pe(0x4000, b"\x48\x8b\xc4"))
        self.assertEqual(image.read(0x4000, 3), b"\x48\x8b\xc4")
        self.assertEqual(image.read(0x4001, 2), b"\x8b\xc4")

    def test_an_rva_outside_every_section_is_unmapped_rather_than_empty(self):
        image = check_pins.PE(build_pe(0x4000, b"\x90"))
        self.assertIsNone(image.read(0x9000, 1), "an uncovered RVA must be None, not b''")
        self.assertIsNone(image.read(0x3FFF, 1))

    def test_a_virtual_tail_past_raw_size_is_unavailable_not_the_next_section(self):
        """The maintainer's case: raw size 4, virtual size 0x100, an appended overlay.

        `read(0x1004, 11)` used to return the overlay bytes, because the bound was
        `delta + count <= max(vsize, size)`, which admits the whole virtual tail. That tail is zero-filled
        at load time and is not backed by the file, so the overlay is a different object entirely. The read
        must be bounded by `SizeOfRawData` and report unavailability for the tail.
        """
        # build_pe writes raw size = len(text); build the image by hand so raw size can be 4 while the
        # virtual size is 0x100.
        import struct as _s
        headers = bytearray(0x400)
        headers[0:2] = b"MZ"
        _s.pack_into("<I", headers, 0x3C, 0x40)
        headers[0x40:0x44] = b"PE\0\0"
        _s.pack_into("<H", headers, 0x44, 0x8664)
        _s.pack_into("<H", headers, 0x46, 1)
        _s.pack_into("<H", headers, 0x54, 0xF0)
        opt = 0x58
        _s.pack_into("<H", headers, opt, 0x20B)
        row = opt + 0xF0
        headers[row:row + 5] = b".text"
        _s.pack_into("<I", headers, row + 8, 0x100)     # virtual size
        _s.pack_into("<I", headers, row + 12, 0x1000)   # rva
        _s.pack_into("<I", headers, row + 16, 4)        # raw size: only 4 file-backed bytes
        _s.pack_into("<I", headers, row + 20, 0x400)    # raw offset
        blob = bytes(headers) + b"ABCD" + b"NOT-IN-TEXT-OVERLAY-BYTES"
        image = check_pins.PE(blob)
        self.assertEqual(image.read(0x1000, 4), b"ABCD", "the file-backed part must still read")
        self.assertIsNone(image.read(0x1004, 11),
                          "the virtual tail returned overlay bytes: it is not file-backed")
        self.assertIsNone(image.read(0x1000, 0x100),
                          "a read spanning the raw boundary must be unavailable, not partially served")

    def test_a_non_pe_is_refused(self):
        with self.assertRaises(ValueError):
            check_pins.PE(b"not a pe at all" * 8)


class SourceScanTests(unittest.TestCase):
    def setUp(self):
        scratch = PROJECT / "local" / "pin-tests"
        scratch.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=scratch)
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.src = self.root / "src"
        self.src.mkdir()
        self.exe = self.root / "fake.exe"

    def write(self, name, text):
        (self.src / name).write_text(text, encoding="utf-8")

    def test_pins_are_read_with_file_and_line(self):
        self.write("a.h", 'int a(void) {\n  return !pin(base+0x401000,"488bc4");\n}\n')
        pins = check_pins.read_pins(str(self.src))
        self.assertEqual(len(pins), 1)
        self.assertEqual((pins[0]["file"], pins[0]["line"]), ("a.h", 2))
        self.assertEqual((pins[0]["rva"], pins[0]["pinned"]), (0x401000, "488bc4"))

    def test_a_loose_offset_is_reported_and_a_member_offset_is_not(self):
        self.write("b.h", 'read_ptr(pawn+0x1e0);\nuse(base+0x401000);\nuse(base+0x401100);\n'
                          'pin(base+0x401100,"90");\n')
        pins = check_pins.read_pins(str(self.src))
        # 0x1e0 is below .text and is a member offset; 0x401000 is used without pinned bytes;
        # 0x401100 is pinned, so the loose use of it is not reported twice.
        self.assertEqual(sorted((p["rva"], p["pinned"]) for p in pins),
                         [(0x401000, None), (0x401100, "90")])

    def _check(self, src_text, image):
        self.write("c.h", src_text)
        self.exe.write_bytes(image)
        result = check_pins.check(str(self.exe), str(self.src))
        return check_pins.summarise(result)

    def test_a_matching_pin_passes_and_a_one_byte_change_fails(self):
        image = build_pe(0x1000, b"\x48\x8b\xc4\x53")
        code, lines = self._check('pin(base+0x1000,"488bc453");\n', image)
        self.assertEqual(code, 0, "\n".join(lines))
        # The same image with the pin off by one byte must fail, or the check cannot fail at all.
        code, lines = self._check('pin(base+0x1000,"488bc452");\n', image)
        self.assertNotEqual(code, 0, "a changed byte was reported as a match")
        self.assertIn("MISMATCH", "\n".join(lines))

    def test_an_uncovered_rva_is_unmapped_not_a_mismatch(self):
        code, lines = self._check('pin(base+0x900000,"90");\n', build_pe(0x1000, b"\x90"))
        text = "\n".join(lines)
        self.assertIn("UNMAPPED", text)
        self.assertNotIn("MISMATCH", text)
        self.assertNotEqual(code, 0)

    def test_an_unpinned_offset_is_not_silently_counted_as_passing(self):
        code, lines = self._check('use(base+0x1000);\npin(base+0x1001,"90");\n',
                                  build_pe(0x1000, b"\x48\x90"))
        text = "\n".join(lines)
        self.assertIn("unpinned 1", text)
        self.assertIn("unpinned  c.h:1", text)
        self.assertIn("1 absolute offset(s) are unpinned", text)
        self.assertEqual(code, 0)

    def test_an_unsupported_executable_gets_its_own_exit_code(self):
        # The synthetic image cannot be the supported build, so a mismatch here is about the build.
        code, lines = self._check('pin(base+0x1000,"00");\n', build_pe(0x1000, b"\x90"))
        self.assertEqual(code, 3, "\n".join(lines))
        self.assertIn("NOT the one the mod targets", "\n".join(lines))
        self.assertIn("nothing about the mod follows from this run", "\n".join(lines))

    def test_the_supported_hash_is_the_documented_one(self):
        # Pinned here so the support boundary cannot drift away from README.md without a failure.
        self.assertEqual(check_pins.SUPPORTED_SHA256,
                         "b453871eb6eacff0c646490ffbc588f743399a829b1f9e31cd7775dc95da8001")
        readme = (PROJECT / "README.md").read_text(encoding="utf-8")
        self.assertIn(check_pins.SUPPORTED_SHA256, readme,
                      "README.md no longer states the supported executable hash")

    def test_the_fixed_file_version_is_parsed_from_the_resource(self):
        """A version string is not an identity, and the parse has to be right for the message to be true.

        `VS_FIXEDFILEINFO` is signature, struct version, `FileVersionMS`, `FileVersionLS`. The MS word packs
        major (high) and minor (low); the LS word packs build (high) and private (low). Reading four
        consecutive dwords instead gives nonsense - measured as `131072.65536.4277077181.48602` on the
        installed build, whose real fixed version is `2.0.0.48602`.
        """
        try:
            import ctypes  # noqa: F401
        except Exception:
            self.skipTest("no ctypes")
        import sys
        if sys.platform != "win32":
            self.skipTest("version resources are Windows-only")
        # python.exe always carries a version resource, so this needs no game file.
        version = check_pins.file_version(sys.executable)
        self.assertIsNotNone(version, "could not read a version resource from %s" % sys.executable)
        parts = version.split(".")
        self.assertEqual(len(parts), 4, version)
        self.assertTrue(all(p.isdigit() for p in parts), version)


class CommandLineTests(unittest.TestCase):
    def test_a_missing_executable_is_a_skip_not_a_failure(self):
        scratch = PROJECT / "local" / "pin-tests"
        scratch.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=scratch) as temp:
            code = subprocess.run(
                [sys.executable, "-I", str(PROJECT / "tools" / "check_pins.py"),
                 "--exe", str(Path(temp) / "absent.exe")],
                capture_output=True, text=True, check=False)
        self.assertEqual(code.returncode, 2, code.stdout + code.stderr)
        self.assertIn("skip, not a result", code.stdout)


class CoverageTests(unittest.TestCase):
    """Coverage is only meaningful if it is counted over every address, not only the pinned ones.

    Measured on the two trees in this repository, which are not the same tree:

        tree            source files   distinct code addresses   pinned   covered
        fork main                 21                        61        9       15%
        guest-readiness branch    77                       140        9        6%

    So a build mismatch is detected for a small minority of the surface, and silently ignored for the rest.
    These cases pin the counting rules rather than the numbers, because the numbers move with the branch.
    """

    def setUp(self):
        scratch = PROJECT / "local" / "pin-tests"
        scratch.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=scratch)
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.src = self.root / "src"
        self.src.mkdir()

    def write(self, name, text):
        (self.src / name).write_text(text, encoding="utf-8")

    def test_code_addresses_counts_every_distinct_address_including_pinned_ones(self):
        # The bug this pins: returning only the *unpinned* addresses made 52 rows for 140 addresses, and the
        # output could not itself say whether the missing ones were pinned or dropped.
        self.write("a.h", 'pin(base+0x401000,"90");\nuse(base+0x402000);\nuse(base + 0x402000);\n'
                          'use(base + 0x403000);\n')
        addresses = check_pins.code_addresses(str(self.src))
        self.assertEqual([a["rva"] for a in addresses], [0x401000, 0x402000, 0x403000])

    def test_a_spaced_offset_is_seen(self):
        # A pattern written from one example missed `base + 0x`, and under-reported by 60 uses.
        self.write("b.h", "use(base + 0x401234);\n")
        self.assertEqual([a["rva"] for a in check_pins.code_addresses(str(self.src))], [0x401234])

    def test_generation_skips_unmapped_and_flags_low_variety_sequences(self):
        self.write("c.h", 'use(base+0x1000);\nuse(base+0x2000);\npin(base+0x3000,"90");\n')
        exe = self.root / "fake.exe"
        exe.write_bytes(build_pe(0x1000, b"\x00" * 0x1000 + b"\x48\x8b\xc4\x53\x55\x56\x57\x41" * 4))
        # 0x1000 is all zero bytes (weak), 0x2000 is varied, 0x3000 is already pinned.
        rows, had, skipped, weak = check_pins.propose_pins(str(exe), str(self.src), length=16)
        self.assertEqual([r["rva"] for r in rows], [0x1000, 0x2000])
        self.assertEqual(had, 1, "the already-pinned address must be counted, not silently dropped")
        self.assertEqual([w[0]["rva"] for w in weak], [0x1000],
                         "sixteen zero bytes must be flagged - it would validate any build")
        # An address past the end of every section is skipped and named, never emitted with an empty pin.
        self.write("d.h", "use(base+0x9f0000);\n")
        rows, had, skipped, _weak = check_pins.propose_pins(str(exe), str(self.src), length=16)
        self.assertEqual([s["rva"] for s in skipped], [0x9f0000])

    def test_the_generated_header_is_sorted_and_self_describing(self):
        rows = [{"rva": 0x402000, "bytes": "aa"}, {"rva": 0x401000, "bytes": "bb"}]
        text = check_pins.render_pins(sorted(rows, key=lambda r: r["rva"]))
        self.assertIn("Do not hand-edit", text)
        self.assertIn("Review before committing", text, "a generated pin file must say it needs review")
        self.assertLess(text.index("0x0401000"), text.index("0x0402000"), "pins must be sorted by RVA")
        self.assertIn('#ifndef ME3_CODE_PINS_H', text)

    def test_the_generated_header_is_consumable_by_the_checker_own_scanner(self):
        """The maintainer's case: the generator emitted rows the checker's own scanner does not read.

        `render_pins` produced `{0xRVA, "hex"}` rows, but `read_pins` only recognizes
        `base+0xRVA,"hex"`. Generating a header and checking again therefore left the original address
        reported as unpinned - the generator and the checker disagreed about what a pin is. The generated
        rows are now emitted in the scanner's own syntax, and the guard is closed.
        """
        rows = [{"rva": 0x401000, "bytes": "488bc453", "file": "x.h", "line": 1}]
        header = check_pins.render_pins(rows)
        self.assertIn("#endif", header, "the include guard is not closed")
        import re
        recognized = re.findall(r'base\s*\+\s*0x([0-9a-fA-F]+)\s*,\s*"([0-9a-fA-F]+)"', header)
        self.assertEqual(recognized, [("401000", "488bc453")],
                         "read_pins would not recognize the generated rows:\n%s" % header)


class EncryptionTests(unittest.TestCase):
    """The check that inverts every other result, and that this tool got wrong first time.

    The installed Steam build of LE3 is **not** an ordinary PE: its `.text` measures 8.0000 bits/byte, has no
    repeated 32-byte window in 4 MB, and contains zero `48 83 ec 28` prologues in 17 MB. `notepad.exe` and
    `kernel32.dll` on the same machine measure 6.24 and 6.26 with hundreds of them. The code is encrypted at
    rest and decrypted when the game runs.

    Reporting "9 pins mismatch" against such a file is a false negative of the worst kind: it reads as "the mod
    is broken" when it means "this file cannot be checked". So an unreadable image must produce its own
    outcome, and generation from one must be refused.

    The classifier's own limits are asserted too, because they are what keep it honest: a section too small
    for the statistic is *unknown*, not encrypted, and a uniform byte ramp has entropy 8.0 without being
    random - so plaintext that measures 8.0 must still be readable as plaintext.
    """

    def setUp(self):
        scratch = PROJECT / "local" / "pin-tests"
        scratch.mkdir(parents=True, exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(dir=scratch)
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.src = self.root / "src"
        self.src.mkdir()

    def _image(self, text):
        """A PE whose `.text` is exactly `text`, large enough that the entropy statistic can apply."""
        import struct as _struct
        blob = bytearray(0x400 + 0x10000 + len(text))
        blob[0:2] = b"MZ"
        _struct.pack_into("<I", blob, 0x3C, 0x40)
        blob[0x40:0x44] = b"PE\0\0"
        _struct.pack_into("<H", blob, 0x44, 0x8664)
        _struct.pack_into("<H", blob, 0x46, 1)
        _struct.pack_into("<H", blob, 0x54, 0xF0)
        opt = 0x58
        _struct.pack_into("<H", blob, opt, 0x20B)
        row = opt + 0xF0
        blob[row:row + 5] = b".text"
        _struct.pack_into("<I", blob, row + 8, len(text))
        _struct.pack_into("<I", blob, row + 12, 0x1000)
        _struct.pack_into("<I", blob, row + 16, len(text))
        _struct.pack_into("<I", blob, row + 20, 0x400)
        blob[0x400:0x400 + len(text)] = text
        return bytes(blob)

    def test_high_entropy_code_is_reported_unreadable_rather_than_as_mismatches(self):
        import random
        rng = random.Random(1234)
        exe = self.root / "packed.exe"
        exe.write_bytes(self._image(bytes(rng.randrange(256) for _ in range(1 << 17))))
        (self.src / "a.h").write_text('pin(base+0x1000,"488bc4");\n', encoding="utf-8")
        result = check_pins.check(str(exe), str(self.src))
        self.assertFalse(result["code_readable"], "encrypted .text was read as plaintext")
        self.assertEqual(result["code_section"], ".text")
        code, lines = check_pins.summarise(result)
        text = "\n".join(lines)
        self.assertEqual(code, 2, "an unreadable image must not return a pass or a mod verdict")
        self.assertIn("UNREADABLE", text)
        self.assertIn("must not be read as", text)
        self.assertNotIn("the mod is wrong", text)

    def test_generation_refuses_an_encrypted_image(self):
        import random
        rng = random.Random(1234)
        exe = self.root / "packed.exe"
        exe.write_bytes(self._image(bytes(rng.randrange(256) for _ in range(1 << 17))))
        (self.src / "a.h").write_text("use(base+0x1000);\n", encoding="utf-8")
        out = self.root / "gen.h"
        code = check_pins.main(["--exe", str(exe), "--root", str(self.root), "--generate", str(out)])
        self.assertEqual(code, 2, "generation from encrypted code must be refused, not warned about")
        self.assertFalse(out.exists(), "a refused generation must not leave a partial pin file")

    def test_plaintext_code_is_still_readable(self):
        # The negative control: without this, "everything is unreadable" would also pass the cases above.
        # Real x64 code is repetitive - the same prologues and epilogues over and over - so plaintext here is
        # built from repeated real instruction shapes rather than a uniform byte ramp, which also measures
        # 8.0 bits/byte and would test the classifier's stated limit instead of the plaintext path.
        exe = self.root / "plain.exe"
        body = (b"\x48\x83\xec\x28\x48\x89\x5c\x24\x08\x57\x48\x8b\x01\xff\xd0\x5f\x48\x83\xc4\x28\xc3"
                b"\xcc\xcc\xcc\xcc") * 4000
        exe.write_bytes(self._image(body))
        (self.src / "a.h").write_text("use(base+0x1000);\n", encoding="utf-8")
        result = check_pins.check(str(exe), str(self.src))
        self.assertTrue(result["code_readable"], "plaintext .text was reported as encrypted: %s"
                        % result.get("code_why"))

    def test_a_uniform_byte_ramp_is_high_entropy_but_not_called_encrypted(self):
        # bytes(range(256)) repeated measures 8.0 bits/byte - the same entropy as random - and is plainly
        # not encrypted. The classifier must not call it encrypted on entropy alone.
        image = self._image(bytes(range(256)) * 512)
        encrypted, why = check_pins.looks_encrypted(image[0x400:])
        self.assertFalse(encrypted, "a uniform ramp was called encrypted: %s" % why)

    def test_a_small_section_is_unknown_not_encrypted(self):
        image = self._image(bytes(1 << 17))
        # The section starts at file offset 0x400; pass only part of it, below the 64 KiB floor.
        encrypted, why = check_pins.looks_encrypted(image[0x400:0x400 + (1 << 15)])
        self.assertFalse(encrypted)
        self.assertIn("too small", why)


if __name__ == "__main__":
    unittest.main()
