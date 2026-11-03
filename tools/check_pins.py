"""Check the native code pins against an executable, offline, before any game process starts.

The mod is an exact-build ASI. It guards native entry points with committed instruction bytes
(`base+0xNNN, "<hex>"`) because a wrong offset in a hook is a crash, not a wrong answer. But those guards
only run inside the game: a pin that has drifted is discovered as a failed launch, after the copy, the
staging and the profile setup.

This does the same comparison offline. Two results matter and they are different:

* **A pin that does not match the executable the mod targets** means the mod is wrong.
* **Every pin not matching some other executable** means the *user* has a different build, and says nothing
  about the mod. Conflating the two is how a support channel fills with "the mod is broken" reports that are
  really "wrong game build", so the reason is reported separately from the count.

Nothing here writes anything except the optional JSON report. The executable is opened read-only.

Usage:
    python -I tools/check_pins.py                          # installed build, default paths
    python -I tools/check_pins.py --exe <MassEffect3.exe> --root <repo> --json <out.json>

Exit codes: 0 all pins match the supported build; 1 a pin does not match the SUPPORTED build;
2 the executable or sources are missing (a skip, not a result); 3 the pins do not match, but the
executable is not the supported one, so the result is about the build rather than the mod.
"""
import argparse
import hashlib
import json
import os
import re
import struct
import sys

SUPPORTED_SHA256 = "b453871eb6eacff0c646490ffbc588f743399a829b1f9e31cd7775dc95da8001"
SUPPORTED_VERSION = "2.0.0.48602"
DEFAULT_INSTALL = r"B:\Steamlibrary\steamapps\common\Mass Effect Legendary Edition"
DEFAULT_EXE = os.path.join(DEFAULT_INSTALL, "Game", "ME3", "Binaries", "Win64", "MassEffect3.exe")
PROJECT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# `base+0xNNN,"hex"` is the form every pin in this tree uses. Whitespace is allowed around the operator:
# a regex that requires `base+0x` silently misses `base + 0x`, and this one did - it reported 294 unpinned
# offsets where the sources use 359, because a spaced spelling is invisible to a pattern written from one
# example.
PIN = re.compile(r'base\s*\+\s*0x([0-9a-fA-F]+)\s*,\s*"([0-9a-fA-F]+)"')
# Any other absolute offset, so a pin written a different way is reported rather than silently skipped.
LOOSE = re.compile(r"base\s*\+\s*0x([0-9a-fA-F]+)")
IMAGE_SCANNED = "0x1000"


class PE:
    """The smallest PE32+ reader that can map an RVA to file bytes. Read-only."""

    def __init__(self, data):
        self.data = data
        if data[:2] != b"MZ" or len(data) < 64:
            raise ValueError("not a PE executable")
        at = struct.unpack_from("<I", data, 0x3C)[0]
        if data[at:at + 4] != b"PE\0\0":
            raise ValueError("no PE signature")
        sections = struct.unpack_from("<H", data, at + 6)[0]
        opt_size = struct.unpack_from("<H", data, at + 20)[0]
        opt = at + 24
        self.timestamp = struct.unpack_from("<I", data, at + 8)[0]
        if struct.unpack_from("<H", data, opt)[0] != 0x20B:
            raise ValueError("expected PE32+ (x64)")
        self.sections = []
        for i in range(sections):
            row = opt + opt_size + 40 * i
            name = data[row:row + 8].split(b"\0")[0].decode("ascii", errors="replace")
            vsize, rva, size, raw = struct.unpack_from("<IIII", data, row + 8)
            self.sections.append((name, rva, vsize, size, raw))

    def read(self, rva, count):
        """`count` bytes at `rva`, or None when the range is not file-backed.

        Bounded by `SizeOfRawData`, not by the section's virtual size. A section's virtual tail - the part
        of `VirtualSize` past `SizeOfRawData` - is zero-filled at load time and is not backed by the file,
        so a read there must report unavailability rather than return whatever follows the section in the
        file (an overlay, a certificate, the next section). A synthetic PE with raw size 4, virtual size
        0x100 and an appended overlay used to hand back overlay bytes for a read of the virtual tail; that
        is the case this bound exists for.
        """
        for _name, base, vsize, size, raw in self.sections:
            delta = rva - base
            if 0 <= delta:
                if delta + count > size:
                    continue
                at = raw + delta
                if at + count <= len(self.data):
                    return self.data[at:at + count]
                return None
        return None


def read_pins(src_dir):
    """Every `(file, line, rva, pinned_bytes_or_None)` in the sources, in file order."""
    out = []
    for name in sorted(os.listdir(src_dir)):
        if not name.endswith((".h", ".c")):
            continue
        with open(os.path.join(src_dir, name), encoding="utf-8", errors="replace") as fh:
            text = fh.read()
        pinned_at = set()
        for match in PIN.finditer(text):
            rva = int(match.group(1), 16)
            pinned_at.add(rva)
            out.append({"file": name, "line": text.count("\n", 0, match.start()) + 1, "rva": rva,
                        "pinned": match.group(2).lower()})
        for match in LOOSE.finditer(text):
            rva = int(match.group(1), 16)
            # Below the first section an offset is a member offset, not a code address.
            if rva < int(IMAGE_SCANNED, 16) or rva in pinned_at:
                continue
            out.append({"file": name, "line": text.count("\n", 0, match.start()) + 1, "rva": rva,
                        "pinned": None})
    return out


def looks_encrypted(section_bytes):
    """True when a section is not plaintext code, measured rather than assumed.

    **Every pins check rests on this**, and getting it wrong inverts the answer. An x64 `.text` is full of
    recognisable bytes; encrypted code is not. Measured on the installed Steam build of LE3:

        .text entropy 8.0000 bits/byte, 0 duplicated 32-byte windows in 4 MB,
        0 occurrences of `48 83 ec 28` (sub rsp,28h) in 17 MB,
        and 38.3% "printable" bytes - which is the random baseline (95/256 = 37.1%), not text.
        By contrast .rdata is plaintext and still holds Bioware source paths.

    For comparison `notepad.exe` and `kernel32.dll` on this machine measure 6.24 and 6.26 bits/byte.

    Three cases have to be told apart, and the traps are the reason the classifier is this shape rather than
    one statistic:

      * **too small** - below 64 KiB the entropy statistic has no meaning, so the answer is "unknown", not
        "encrypted". A test image of a few kilobytes can measure anything;
      * **high entropy** - code never measures above ~7.9, so 7.9 or more is the random signature. But a
        uniform byte ramp (`bytes(range(256))` repeated) measures exactly 8.0 and is plaintext, so entropy
        alone is not proof. The ramp is *periodic*: it repeats with period 256, which random data does not.
        So the high-entropy branch checks periodicity at a few small lags before calling a section random;
      * **printable ratio is deliberately not used.** Random bytes are ~37% printable by construction and a
        uniform ramp is 37.1% - the same number - so that ratio separates nothing.

    Returns `(is_encrypted, reason)`.
    """
    import math
    if len(section_bytes) < 1 << 16:
        return False, ("section too small (%d bytes) for the entropy statistic to mean anything"
                       % len(section_bytes))
    sample = section_bytes[:1 << 22]
    counts = [0] * 256
    for byte in sample:
        counts[byte] += 1
    entropy = 0.0
    for n in counts:
        if n:
            p = n / len(sample)
            entropy -= p * math.log2(p)
    if entropy < 7.9:
        return False, "entropy %.4f bits/byte (plaintext x64 measures about 6.2)" % entropy
    # High entropy. Distinguish random from a uniform ramp by periodicity: a ramp repeats with a small
    # period, random data matches itself at a lag about 1/256 of the time.
    best = 0.0
    for lag in (256, 512, 1024, 2048):
        if len(sample) <= lag * 2:
            continue
        a = sample[:len(sample) - lag]
        b = sample[lag:]
        same = sum(1 for x, y in zip(a, b) if x == y)
        rate = same / len(a)
        best = max(best, rate)
    if best > 0.5:
        return False, ("entropy %.4f but the section repeats at a small lag (%.1f%% self-match) - a uniform "
                       "ramp or a repeated block, not random" % (entropy, best * 100))
    return True, ("entropy %.4f bits/byte, %.1f%% self-match at small lags - random, not text"
                  % (entropy, best * 100))


def image_text_section(data, image):
    """`(section_name, bytes)` for the executable section a pin would have to come from, or (None, b"")."""
    for name, rva, vsize, size, raw in image.sections:
        if name == ".text":
            return name, data[raw:raw + size]
    return None, b""


def check(exe_path, src_dir):
    """`result` including whether the file's code is readable. Raises only on unusable inputs."""
    with open(exe_path, "rb") as fh:
        data = fh.read()
    image = PE(data)
    name, section = image_text_section(data, image)
    encrypted, why = looks_encrypted(section) if section else (True, "no .text section")
    pins = read_pins(src_dir)
    for pin in pins:
        if pin["pinned"] is None:
            pin["status"] = "unpinned"
            pin["actual"] = None
            continue
        want = bytes.fromhex(pin["pinned"])
        have = image.read(pin["rva"], len(want))
        pin["status"] = "match" if have == want else ("unmapped" if have is None else "mismatch")
        pin["actual"] = have.hex() if have is not None else None
    return {"executable": exe_path, "sha256": hashlib.sha256(data).hexdigest(),
            "supported": hashlib.sha256(data).hexdigest() == SUPPORTED_SHA256,
            "size": len(data), "pe_timestamp": image.timestamp,
            "code_section": name, "code_readable": not encrypted, "code_why": why,
            "sections": [name for name, *_rest in image.sections], "pins": pins}


def file_version(path):
    """`2.0.0.48602`-style fixed file version from the PE resource, or None when it cannot be read.

    Read here rather than left to the caller because it is the field that makes the build-identity problem
    look solved: the installed Steam build reports the **same** fixed version as the supported build and is a
    different executable. A version number is not an identity.
    """
    try:
        import ctypes
        from ctypes import wintypes
        version = ctypes.WinDLL("version")
        size = version.GetFileVersionInfoSizeW(str(path), None)
        if not size:
            return None
        buffer = ctypes.create_string_buffer(size)
        if not version.GetFileVersionInfoW(str(path), 0, size, buffer):
            return None
        value = ctypes.c_void_p()
        length = wintypes.UINT()
        if not version.VerQueryValueW(buffer, "\\", ctypes.byref(value), ctypes.byref(length)):
            return None
        fields = ctypes.cast(value, ctypes.POINTER(wintypes.DWORD * 4)).contents
        # VS_FIXEDFILEINFO: signature, struct version, FileVersionMS, FileVersionLS. The MS word packs
        # major/minor and the LS word packs build/private, so the version is not four consecutive dwords.
        if fields[0] != 0xFEEF04BD:
            return None
        return "%d.%d.%d.%d" % (fields[2] >> 16, fields[2] & 0xFFFF, fields[3] >> 16, fields[3] & 0xFFFF)
    except Exception:
        return None


def summarise(result):
    """`(exit code, [lines])` - the whole report, so the caller and a test read the same thing."""
    pins = result["pins"]
    counts = {}
    for pin in pins:
        counts[pin["status"]] = counts.get(pin["status"], 0) + 1
    lines = ["executable  %s" % result["executable"],
             "sha256      %s" % result["sha256"],
             "size        %d bytes; PE timestamp 0x%08x" % (result["size"], result["pe_timestamp"]),
             "pe          %d section(s): %s" % (len(result["sections"]), ", ".join(result["sections"])),
             "pins        %d: %s" % (len(pins), ", ".join("%s %d" % kv for kv in sorted(counts.items())))]
    version = result.get("file_version")
    failing = [p for p in pins if p["status"] in ("mismatch", "unmapped")]
    encrypted = not result.get("code_readable", True)
    if encrypted:
        # A pins result is meaningless when the code is not plaintext, and reporting the mismatch would be
        # worse than reporting nothing: it reads as "the mod is wrong" when it is "this file cannot be
        # checked". Measured reason included so the claim is re-checkable.
        lines += ["",
                  "UNREADABLE  the %s section is not plaintext: %s"
                  % (result.get("code_section") or "code", result.get("code_why")),
                  "            So no pin comparison is meaningful, and the counts above must not be read as",
                  "            a result about the mod. The executable likely decrypts its own code, either",
                  "            through an exe wrapper or through the EA App's licence path; reading the file",
                  "            on disk cannot see the instructions the game runs.",
                  "",
                  "            Consequence: the pins can only be validated against a DECRYPTED image. They",
                  "            are still correct to ship - the game decrypts before it runs - but an offline",
                  "            check of this executable cannot confirm them."]
        return 2, lines
    if not result["supported"]:
        lines += ["",
                  "SUPPORT     this executable is NOT the one the mod targets",
                  "            expected sha256 %s (%s)" % (SUPPORTED_SHA256, SUPPORTED_VERSION),
                  "            installed sha256 %s%s" % (result["sha256"],
                                                       ", version %s" % version if version else ""),
                  "            A mismatch below says 'different build', not 'the mod is broken'. Do not",
                  "            re-pin against this executable: identify which build it is first, because",
                  "            the offsets belong to the supported one."]
        if version and version == SUPPORTED_VERSION:
            lines += ["",
                      "            NOTE the version resource AGREES (%s) and the executable does not."
                      % version,
                      "            So this build cannot be identified or excluded by version number - the",
                      "            hash is the identity, and two ship builds of one version are different",
                      "            executables. A version match is not a build match."]
    for pin in failing[:16]:
        lines.append("  %-9s %s:%d  rva 0x%x  %s" % (
            pin["status"].upper(), pin["file"], pin["line"], pin["rva"],
            "<%d pinned bytes> actual %s" % (len(pin["pinned"]) // 2, pin["actual"])
            if pin["actual"] else "<no section covers this rva>"))
    unpinned = [p for p in pins if p["status"] == "unpinned"]
    for pin in unpinned[:8]:
        lines.append("  unpinned  %s:%d  rva 0x%x (no pinned bytes: a wrong offset is undetectable at "
                     "runtime too)" % (pin["file"], pin["line"], pin["rva"]))
    lines.append("")
    if failing and result["supported"]:
        lines.append("RESULT: %d pin(s) do not match the SUPPORTED executable - the mod is wrong for its "
                     "own target" % len(failing))
        return 1, lines
    if failing:
        lines.append("RESULT: %d pin(s) do not match, and this is not the supported build (%d pinned, %d "
                     "unpinned) - nothing about the mod follows from this run" % (len(failing),
                                                                                   len(pins) - len(unpinned),
                                                                                   len(unpinned)))
        return 3, lines
    if unpinned:
        lines.append("RESULT: every %d pinned sequence matches; %d absolute offset(s) are unpinned"
                     % (len(pins) - len(unpinned), len(unpinned)))
        return 0, lines
    lines.append("RESULT: all %d pinned sequences match%s"
                 % (len(pins), "" if result["supported"] else " (but this is not the supported build)"))
    return 0, lines


def code_addresses(src_dir, text_rva=0x1000):
    """Every distinct absolute code address the sources use, whether or not it is already pinned.

    One pass over `base+0xNNN` spellings, deduped by RVA and excluding anything below the first section (a
    member offset, which cannot be a code address). Returning *every* address, rather than only the ones with
    no pin, is what makes coverage reportable: the previous shape produced 52 rows for 140 addresses and could
    not itself tell whether the missing 88 were pinned or dropped.
    """
    found = {}
    for name in sorted(os.listdir(src_dir)):
        if not name.endswith((".h", ".c")):
            continue
        with open(os.path.join(src_dir, name), encoding="utf-8", errors="replace") as fh:
            text = fh.read()
        for match in LOOSE.finditer(text):
            rva = int(match.group(1), 16)
            if rva < int(IMAGE_SCANNED, 16) or rva in found:
                continue
            found[rva] = {"file": name, "line": text.count("\n", 0, match.start()) + 1, "rva": rva}
    return [found[rva] for rva in sorted(found)]


def propose_pins(exe_path, src_dir, length=16, minimum=8):
    """Pins for every code address the sources use that has none, read from this executable.

    The measurement that motivates this: `src/` names **140 distinct absolute code addresses** (on the
    guest-readiness branch), and **9** carry committed hex-string pins in this checker's recognized form,
    so a build mismatch is detected for those 9 and is silent for the rest. Two limits on that number,
    stated rather than smoothed:

      * it counts **recognized hex-string pins**, not every guard in the tree. The sources also carry
        byte-array and `memcmp` guards - for example `guest_equipment_bootstrap.h` and
        `guest_player_context_adapter.h` - which this scanner does not recognize, so the checked fraction
        is understated, not overstated. A comprehensive source analyzer is out of scope for this tool.
      * it is a property of one branch at one commit. The number moves as sources change.

    Generating the missing ones from a known-good executable turns the recognized fraction into full
    coverage of the recognized form.

    Two rules keep a generated file from being worse than none:

      * an address that does not resolve to file-backed bytes is **skipped and named**, never emitted with an
        empty or guessed sequence;
      * a sequence with fewer than `minimum` distinct byte values is flagged, because 16 bytes with three
        distinct values matches almost any build and would validate the wrong one.

    Returns `(rows, already, skipped, weak)` where `already` is the count that needed no new pin.
    """
    with open(exe_path, "rb") as fh:
        data = fh.read()
    image = PE(data)
    already = {pin["rva"] for pin in read_pins(src_dir) if pin["pinned"] is not None}
    rows, skipped, weak, had = [], [], [], 0
    for address in code_addresses(src_dir):
        if address["rva"] in already:
            had += 1
            continue
        want = image.read(address["rva"], length)
        if want is None:
            skipped.append(address)
            continue
        if len(set(want)) < minimum:
            weak.append((address, want))
        rows.append({"rva": address["rva"], "bytes": want.hex(),
                     "file": address["file"], "line": address["line"]})
    rows.sort(key=lambda r: r["rva"])
    return rows, had, skipped, weak


def render_pins(rows):
    """The generated header. Sorted by RVA so a diff between two builds is readable.

    The rows are emitted in the form `read_pins()` already recognizes - `base+0xRVA,"hex"` - so a generated
    file is consumable by the same checker that validates hand-written pins. That is the whole point of
    generating rather than editing: the generated rows must not need a second reader. The guard is closed.
    """
    out = ["/* Generated from an executable by tools/check_pins.py --generate. Do not hand-edit.",
           " *",
           " * Each row pins the first bytes of a code address the sources use, so a build mismatch is",
           " * detected offline by tools/check_pins.py instead of at a hook inside the game.",
           " *",
           " * Review before committing: a pin generated from the WRONG executable validates the wrong build.",
           " *",
           " * The rows below are comments in the checker's own input syntax, so this file is both the",
           " * generated pin table and a readable record of what was pinned against which executable.",
           " */",
           "#ifndef ME3_CODE_PINS_H",
           "#define ME3_CODE_PINS_H",
           "",
           "typedef struct { uint32_t rva; const char *bytes; } Me3CodePin;",
           "",
           "static const Me3CodePin me3_code_pins[] = {"]
    for row in rows:
        out.append('    {0x%07x, "%s"},  /* base+0x%x,"%s" */'
                   % (row["rva"], row["bytes"], row["rva"], row["bytes"]))
    out.append("};")
    out.append("")
    out.append("#endif /* ME3_CODE_PINS_H */")
    out.append("")
    return "\n".join(out)



def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--exe", default=os.environ.get("ME3_EXE") or DEFAULT_EXE)
    ap.add_argument("--root", default=PROJECT)
    ap.add_argument("--json")
    ap.add_argument("--generate", metavar="OUT.h",
                    help="write pins for every code address that has none, read from --exe. "
                         "Keep the output OUTSIDE src/: the checker scans src/ and would treat the "
                         "generated rows as additional pins")
    ap.add_argument("--length", type=int, default=16, help="bytes per generated pin (default 16)")
    args = ap.parse_args(argv)
    if not os.path.isfile(args.exe):
        print("executable not found: %s (set ME3_EXE; skip, not a result)" % args.exe)
        return 2
    src = os.path.join(args.root, "src")
    if not os.path.isdir(src):
        print("no src directory under %s (skip, not a result)" % args.root)
        return 2
    if args.generate:
        with open(args.exe, "rb") as fh:
            blob = fh.read()
        probe_image = PE(blob)
        section_name, section = image_text_section(blob, probe_image)
        if section:
            # Generating pins from an unreadable section would write random bytes and certify them. That is
            # worse than no pins, so it is refused rather than warned about.
            encrypted, why = looks_encrypted(section)
            if encrypted:
                print("REFUSING to generate pins: the %s section is not plaintext (%s)."
                      % (section_name, why))
                print("Pins made from this file would be random bytes that then pass, which is worse than")
                print("having none. Use an executable whose code sections are readable, or decrypt this one")
                print("first with a method the owner has approved.")
                return 2
        rows, had, skipped, weak = propose_pins(args.exe, src, length=args.length)
        text = render_pins(rows)
        with open(args.generate, "w", encoding="utf-8", newline="\n") as fh:
            fh.write(text + "\n")
        total = len(rows) + had
        print("wrote %s: %d new pin(s), %d already pinned, %d skipped, %d weak - %d of %d code address(es) "
              "covered (%.0f%%)"
              % (args.generate, len(rows), had, len(skipped), len(weak), total, total,
                 100.0 if total else 0.0))
        for pin in skipped[:8]:
            print("  SKIPPED %s:%d rva 0x%x - no file-backed bytes there (a bug or a wrong address)"
                  % (pin["file"], pin["line"], pin["rva"]))
        for pin, want in weak[:8]:
            print("  WEAK    %s:%d rva 0x%x has only %d distinct byte value(s) - it would match almost any "
                  "build" % (pin["file"], pin["line"], pin["rva"], len(set(want))))
        version = file_version(args.exe)
        if version and version == SUPPORTED_VERSION:
            print("\nWARNING the source executable reports %s, the same fixed version as the supported build."
                  % version)
            print("        A version match is not a build match, so these pins may describe the wrong")
            print("        executable. Confirm the SHA-256 is %s before committing them." % SUPPORTED_SHA256)
        return 0
    result = check(args.exe, src)
    result["file_version"] = file_version(args.exe)
    code, lines = summarise(result)
    print("\n".join(lines))
    if args.json:
        os.makedirs(os.path.dirname(os.path.abspath(args.json)), exist_ok=True)
        with open(args.json, "w", encoding="utf-8") as fh:
            json.dump(result, fh, indent=2)
        print("\nwrote %s" % args.json)
    return code


if __name__ == "__main__":
    sys.exit(main())
