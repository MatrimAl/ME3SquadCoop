"""Test the ordinary runtime in a Python process; never starts a game or hooks it."""
import argparse
import ctypes
import hashlib
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
    ordinary = ROOT / "local/build/ME3SquadCoop.asi"
    binary = ordinary.read_bytes()
    for sentinel in ("test-action.txt", "ME3_SQUAD_COOP_TEST_ROLE", "ME3SquadCoop.probe.enabled",
                     ".me3-coop-sandbox.json", "ME3_SQUAD_COOP_PROGRESS_TEST"):
        assert sentinel.encode() not in binary and sentinel.encode("utf-16-le") not in binary, sentinel
    loaded = ctypes.CDLL(str(ordinary))
    assert loaded.ME3SquadCoopBridgeInstalled() == 0
    assert not hasattr(loaded, "SquadProbeEnabled") and not hasattr(loaded, "ME3RuntimeCoreCleanupSelfTest")
    folder = ROOT / "local/runtime-entry-fixture"
    folder.mkdir(parents=True, exist_ok=True)
    dll = folder / "runtime-entry-fixture.dll"
    env = dict(os.environ, ZIG_GLOBAL_CACHE_DIR=str(ROOT / "local/zig-cache"),
               ZIG_LOCAL_CACHE_DIR=str(ROOT / "local/zig-local-cache"),
               TMP=str(ROOT / "local/compiler-tmp"), TEMP=str(ROOT / "local/compiler-tmp"))
    subprocess.run([str(args.zig.resolve(strict=True)), "cc", "-target", "x86_64-windows-gnu",
                    "-std=c17", "-O2", "-shared", "-DME3_RUNTIME_FIXTURE",
                    str(ROOT / "src/squad_coop.c"), "-lbcrypt", "-o", str(dll)], env=env, check=True)
    entry = ctypes.CDLL(str(dll))
    for name in ("ME3RuntimeIdentitySelfTest", "ME3RuntimeConfigSelfTest", "ME3RuntimeSupportSelfTest",
                 "ME3RuntimePolicySelfTest", "ME3RuntimeSessionGateSelfTest", "ME3RuntimeCameraSelfTest",
                 "ME3RuntimeCinematicSelfTest", "ME3RuntimeCoreCleanupSelfTest",
                 "ME3RuntimeTickForwardingSelfTest", "ME3RuntimeNetworkStartSelfTest",
                 "ME3RuntimeJoinRequestSelfTest", "ME3RuntimeTravelIdleSelfTest",
                 "ME3RuntimeJoinReadinessSelfTest",
                 "ME3RuntimeJoinDiagnosticsSelfTest",
                 "ME3RuntimeJoinProgressSelfTest",
                 "ME3RuntimeTransportSelfTest", "ME3RuntimeHostStartupSelfTest",
                 "ME3RuntimeListenRequestSelfTest"):
        fixture = getattr(entry, name)
        fixture.restype = ctypes.c_int
        assert fixture() == 1, name
    entry.ME3SquadCoopActive.restype = ctypes.c_int
    entry.ME3SquadCoopConfigStatus.restype = ctypes.c_int
    assert entry.ME3SquadCoopActive() == 0 and entry.ME3SquadCoopConfigStatus() == 0
    assert entry.ME3SquadCoopBridgeInstalled() == 0 and entry.ME3SquadCoopRuntimeError() == 0
    assert entry.ME3SquadCoopSetEnabled(2) == 0 and entry.ME3SquadCoopSetEnabled(0) == 1
    assert entry.ME3SquadCoopBridgeInstalled() == 0
    assert entry.ME3SquadCoopSetEnabled(1) == 1
    entry.ME3SquadCoopVerifyDlcGate.argtypes = [ctypes.c_wchar_p]
    entry.ME3SquadCoopVerifyDlcGate.restype = ctypes.c_int
    with tempfile.TemporaryDirectory(dir=folder) as temporary:
        root = Path(temporary)
        entry.ME3RuntimeHashFileFixture.argtypes = [ctypes.c_wchar_p, ctypes.c_void_p]
        entry.ME3RuntimeHashFileFixture.restype = ctypes.c_int
        file = root / "hash-fixture.bin"
        for content in (b"", b"abc", bytes(range(256)) * 600):
            file.write_bytes(content)
            digest = ctypes.create_string_buffer(hashlib.sha256(content).digest())
            assert entry.ME3RuntimeHashFileFixture(str(file), digest) == 1
            wrong = ctypes.create_string_buffer(bytes(32))
            assert entry.ME3RuntimeHashFileFixture(str(file), wrong) == 0
        assert entry.ME3RuntimeHashFileFixture(str(root / "absent.bin"), digest) == 0
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
    print("PASS ordinary runtime policies, shared cleanup identity/order, pause/camera/cinematic guards, SHA-256, configuration and DLC fixtures; no game launched")


if __name__ == "__main__":
    main()
