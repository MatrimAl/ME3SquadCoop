"""Build/test the inert runtime entry in Python; never starts a game or hooks it."""
import argparse
import ctypes
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src"))
from build_state import verify_current


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("zig", type=Path)
    args = parser.parse_args()
    verify_current(ROOT)
    folder = ROOT / "local/runtime-entry-fixture"
    folder.mkdir(parents=True, exist_ok=True)
    dll = folder / "runtime-entry-fixture.dll"
    env = dict(os.environ, ZIG_GLOBAL_CACHE_DIR=str(ROOT / "local/zig-cache"),
               ZIG_LOCAL_CACHE_DIR=str(ROOT / "local/zig-local-cache"),
               TMP=str(ROOT / "local/compiler-tmp"), TEMP=str(ROOT / "local/compiler-tmp"))
    subprocess.run([str(args.zig.resolve(strict=True)), "cc", "-target", "x86_64-windows-gnu",
                    "-std=c17", "-O2", "-shared", "-DME3_RUNTIME_FIXTURE",
                    str(ROOT / "src/squad_coop.c"), "-o", str(dll)], env=env, check=True)
    entry = ctypes.CDLL(str(dll))
    for name in ("ME3RuntimeIdentitySelfTest", "ME3RuntimeConfigSelfTest", "ME3RuntimeSupportSelfTest"):
        fixture = getattr(entry, name)
        fixture.restype = ctypes.c_int
        assert fixture() == 1, name
    entry.ME3SquadCoopActive.restype = ctypes.c_int
    entry.ME3SquadCoopConfigStatus.restype = ctypes.c_int
    assert entry.ME3SquadCoopActive() == 0 and entry.ME3SquadCoopConfigStatus() == 0
    entry.ME3SquadCoopVerifyDlcGate.argtypes = [ctypes.c_wchar_p]
    entry.ME3SquadCoopVerifyDlcGate.restype = ctypes.c_int
    with tempfile.TemporaryDirectory(dir=folder) as temporary:
        root = Path(temporary)
        assert entry.ME3SquadCoopVerifyDlcGate(str(root)) == 0
        mount = root / "BioGame/DLC/DLC_MOD_ME3SquadCoop/CookedPCConsole/Mount.dlc"
        mount.parent.mkdir(parents=True)
        mount.write_bytes(b"file-presence-fixture-not-a-game-mount")
        assert entry.ME3SquadCoopVerifyDlcGate(str(root)) == 1
        mount.parents[1].rename(mount.parents[1].with_name("DLC_MOD_ME3SquadCoop_disabled"))
        assert entry.ME3SquadCoopVerifyDlcGate(str(root)) == 0
    clean = {k: v for k, v in os.environ.items() if not k.startswith("ME3_SQUAD_COOP_")}
    code = ("import ctypes,sys; p=ctypes.CDLL(sys.argv[1]); "
            "f=p.ME3RuntimeConfigEnvironmentFixture; f.restype=ctypes.c_int; "
            "assert f()==int(sys.argv[2]); assert p.ME3SquadCoopActive()==0")
    cases = [({}, 0), ({"ME3_SQUAD_COOP_TEST_ROLE": "host", "ME3_SQUAD_COOP_LOOPBACK_QA": "1"}, 0),
             ({"ME3_SQUAD_COOP_ENABLED": "1", "ME3_SQUAD_COOP_ROLE": "host"}, 5),
             ({"ME3_SQUAD_COOP_ENABLED": "1", "ME3_SQUAD_COOP_ROLE": "host",
               "ME3_SQUAD_COOP_BIND_IPV4": "26.1.2.3", "ME3_SQUAD_COOP_PORT": "17779"}, 1),
             ({"ME3_SQUAD_COOP_ENABLED": "1", "ME3_SQUAD_COOP_ROLE": "host",
               "ME3_SQUAD_COOP_BIND_IPV4": "127.0.0.1", "ME3_SQUAD_COOP_PORT": "17779",
               "ME3_SQUAD_COOP_LOOPBACK_QA": "1"}, 5),
             ({"ME3_SQUAD_COOP_ENABLED": "111111111"}, 6)]
    for values, expected in cases:
        subprocess.run([sys.executable, "-c", code, str(dll), str(expected)],
                       env=dict(clean, **values), check=True, timeout=15)
    print("PASS shared runtime identity/configuration, inert entry and DLC file-presence fixtures; no game launched")


if __name__ == "__main__":
    main()
