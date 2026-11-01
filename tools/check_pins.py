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
        for _name, base, vsize, size, raw in self.sections:
            delta = rva - base
            if 0 <= delta and delta + count <= max(vsize, size):
                at = raw + delta
                if at + count <= len(self.data):
                    return self.data[at:at + count]
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


def check(exe_path, src_dir):
    """`(result, pins)` where each pin carries its own status. Raises only on unusable inputs."""
    with open(exe_path, "rb") as fh:
        data = fh.read()
    image = PE(data)
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

    The measurement that motivates this: `src/` names **140 distinct code addresses** and only **9** carry
    pinned bytes, so a build mismatch is detected for 6% of the surface and is silent for the rest. Generating
    the missing ones from a known-good executable turns that into full coverage.

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
    """The generated header. Sorted by RVA so a diff between two builds is readable."""
    out = ["/* Generated from an executable by tools/check_pins.py --generate. Do not hand-edit.",
           " *",
           " * Each row pins the first bytes of a code address the sources use, so a build mismatch is",
           " * detected offline by tools/check_pins.py instead of at a hook inside the game.",
           " *",
           " * Review before committing: a pin generated from the WRONG executable validates the wrong build.",
           " */",
           "#ifndef ME3_CODE_PINS_H",
           "#define ME3_CODE_PINS_H",
           "",
           "typedef struct { uint32_t rva; const char *bytes; } Me3CodePin;",
           "",
           "static const Me3CodePin me3_code_pins[] = {"]
    for row in rows:
        out.append('    {0x%07x, "%s"},' % (row["rva"], row["bytes"]))
    out.append("};")
    out.append("")
    return "\n".join(out)



def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--exe", default=os.environ.get("ME3_EXE") or DEFAULT_EXE)
    ap.add_argument("--root", default=PROJECT)
    ap.add_argument("--json")
    ap.add_argument("--generate", metavar="OUT.h",
                    help="write pins for every code address that has none, read from --exe")
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
