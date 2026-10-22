"""Read-only PE and LEMP networking-site inspection; never patches the game."""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path


class PE:
    def __init__(self, data):
        self.data = data
        if data[:2] != b"MZ" or len(data) < 64:
            raise ValueError("not a PE executable")
        at = struct.unpack_from("<I", data, 0x3C)[0]
        if data[at:at + 4] != b"PE\0\0":
            raise ValueError("invalid PE signature")
        self.machine, count, self.timestamp = struct.unpack_from("<HHI", data, at + 4)
        opt_size = struct.unpack_from("<H", data, at + 20)[0]
        opt = at + 24
        self.magic = struct.unpack_from("<H", data, opt)[0]
        if self.magic != 0x20B:
            raise ValueError("expected x64 PE32+")
        self.sections = []
        for i in range(count):
            offset = opt + opt_size + 40 * i
            name = data[offset:offset + 8].split(b"\0")[0].decode("ascii", errors="replace")
            vsize, rva, size, raw = struct.unpack_from("<IIII", data, offset + 8)
            if raw + size > len(data):
                raise ValueError("truncated section")
            self.sections.append({"name": name, "rva": rva, "virtual_size": vsize, "size": size, "raw": raw})

    def read_rva(self, rva, count):
        for section in self.sections:
            delta = rva - section["rva"]
            if 0 <= delta and delta + count <= section["size"]:
                offset = section["raw"] + delta
                return self.data[offset:offset + count]
        raise ValueError("RVA does not map to file-backed data")


def inspect(exe, lemp_sources):
    data = Path(exe).read_bytes()
    pe = PE(data)
    source = (Path(lemp_sources) / "join_patch.cpp").read_text(encoding="utf-8")
    # Extract only the expected bytes. Do not parse or apply replacement bytes.
    pattern = r'\{"([^"\n]+)",\s*(0x[0-9A-Fa-f]+),\s*\{([^}]+)\},\s*\{[^}]+\},\s*(\d+),\s*true\}'
    checks = []
    for name, address, expected, length in re.findall(pattern, source):
        raw = bytes(int(x.strip(), 16) for x in expected.split(","))[:int(length)]
        have = pe.read_rva(int(address, 16), len(raw))
        checks.append({"name": name, "rva": address, "expected": raw.hex(), "observed": have.hex(), "matches": raw == have})
    if len(checks) != 10:
        raise ValueError("unexpected source patch table; inspect manually before continuing")
    landmark = pe.read_rva(0x65FDE6, 7) == bytes.fromhex("48 8d 15 e3 f9 bb 00")
    return {
        "schema": 1,
        "executable_name": Path(exe).name,
        "sha256": hashlib.sha256(data).hexdigest(),
        "size": len(data), "machine": hex(pe.machine), "timestamp": pe.timestamp,
        "source_release": (Path(lemp_sources) / "VERSION").read_text().strip(),
        "network_sites": checks, "landmark_matches": landmark,
        "all_static_checks_match": landmark and all(x["matches"] for x in checks),
        "disk_modified": False,
        "runtime_campaign_coop_verified": False,
        "note": "Static byte compatibility only. Does not prove runtime replication, squad possession or campaign co-op.",
    }


if __name__ == "__main__":
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("exe", type=Path)
    p.add_argument("lemp_sources", type=Path)
    p.add_argument("--output", type=Path, required=True)
    args = p.parse_args()
    result = inspect(args.exe, args.lemp_sources)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"sha256": result["sha256"], "matched_network_sites": sum(c["matches"] for c in result["network_sites"]),
                      "checked_network_sites": len(result["network_sites"]), "landmark_matches": result["landmark_matches"],
                      "runtime_campaign_coop_verified": False}, indent=2))
