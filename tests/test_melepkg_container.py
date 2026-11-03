"""Container-model regression tests that do not need a game install.

The chunk finder used to bound each candidate at the next package-magic hit. A compressed payload can
legitimately contain those four bytes - the signature is arbitrary data inside a compressed stream - and
when it does, the next hit lands inside the chunk, the candidate's span stops before its own compressed
size is consumed, and a real chunk disappears from the result. That is a structural synthetic reproduction,
not a reported failure in a shipped package.

These cases are in `tests/` so CI discovers them; the reader itself lives in `tools/melepkg/`.
"""
import importlib.util
import struct
import sys
import tempfile
import unittest
from pathlib import Path

PROJECT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PROJECT / "tools" / "melepkg"))

spec = importlib.util.spec_from_file_location(
    "melepkg", PROJECT / "tools" / "melepkg" / "melepkg.py")
melepkg = importlib.util.module_from_spec(spec)
spec.loader.exec_module(melepkg)


class ChunkFinderTests(unittest.TestCase):
    """`find_chunks` must not let a magic value inside a payload remove the real chunk."""

    def _chunk(self, payload):
        total_c = len(payload)
        return (struct.pack("<IIII", melepkg.PKG_MAGIC, 0x10000, total_c, total_c)
                + struct.pack("<II", total_c, total_c)
                + payload)

    def test_magic_inside_a_payload_does_not_remove_the_chunk(self):
        """The maintainer's case: a payload containing the package magic."""
        payload = b"\x00" * 8 + struct.pack("<I", melepkg.PKG_MAGIC) + b"\x00" * 4
        data = b"\x00" * 0x400 + self._chunk(payload)
        hits = melepkg.signature_hits(data)
        self.assertEqual(len(hits), 2, "expected the chunk header and the payload hit: %s"
                         % [hex(h) for h in hits])
        chunks = melepkg.find_chunks(data)
        self.assertEqual(len(chunks), 1,
                         "the payload's magic hit removed the real chunk (hits at %s)"
                         % [hex(h) for h in hits])
        chunk = chunks[0]
        self.assertEqual(chunk["total_csize"], len(payload))
        self.assertEqual(chunk["blocks"], 1)
        # The extent must cover the whole chunk, including the payload that contains the magic.
        self.assertEqual(chunk["start"] + chunk["span"], 0x400 + len(self._chunk(payload)),
                         "the span stopped before the payload was consumed")

    def test_a_bare_magic_hit_is_not_a_chunk(self):
        """A signature hit with no valid block table is still not a chunk - the fix must not loosen that."""
        data = b"\x00" * 0x400 + struct.pack("<I", melepkg.PKG_MAGIC) + b"\x00" * 16
        self.assertEqual(melepkg.find_chunks(data), [],
                         "a bare magic hit was accepted as a chunk")

    def test_the_package_header_at_zero_is_not_a_chunk(self):
        """Offset 0 is the package header's own magic, not a chunk."""
        payload = b"\x00" * 16
        data = self._chunk(payload)
        self.assertEqual(melepkg.find_chunks(data), [],
                         "the header magic at offset 0 was accepted as a chunk")

    def test_adjacent_chunks_are_all_found(self):
        """Real packages hold several chunks; the fix must not find fewer than before."""
        first = self._chunk(b"\x11" * 32)
        second = self._chunk(b"\x22" * 32)
        data = b"\x00" * 0x400 + first + second
        chunks = melepkg.find_chunks(data)
        self.assertEqual(len(chunks), 2, "expected both adjacent chunks, got %d" % len(chunks))
        self.assertEqual([c["start"] for c in chunks],
                         [0x400, 0x400 + len(first)],
                         "the second chunk's start moved")

    def test_a_chunk_whose_table_size_is_wrong_is_rejected(self):
        """The validation still has to close: a bad total_csize must not validate."""
        # total_csize larger than the payload present: the sums cannot match, so no chunk.
        bad = (struct.pack("<IIII", melepkg.PKG_MAGIC, 0x10000, 0x1000, 0x10)
               + struct.pack("<II", 0x1000, 0x10)
               + b"\x00" * 32)
        data = b"\x00" * 0x400 + bad
        self.assertEqual(melepkg.find_chunks(data), [],
                         "a chunk whose declared compressed size exceeds the file was accepted")


if __name__ == "__main__":
    unittest.main()
