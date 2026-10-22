"""DLL harness only: no game starts and no engine memory is written."""
import ctypes
import json
import subprocess
import tempfile
import uuid
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "local" / "build"

probe = ctypes.CDLL(str(BUILD / "ME3SquadCoopProbe.asi"))
probe.SquadProbeSelfTest.restype = ctypes.c_int
assert probe.SquadProbeSelfTest() == 1, "safe memory-read positive/negative controls failed"
probe.SquadStateReaderSelfTest.restype = ctypes.c_int
assert probe.SquadStateReaderSelfTest() == 1, "state reader accepted missing, invalid or special native state nodes"
probe.SquadCatchupGroundSelfTest.restype = ctypes.c_int
assert probe.SquadCatchupGroundSelfTest() == 1, "catch-up ground checks accepted invalid surfaces or rejected world-floor readiness"
probe.SquadAmmoTraceSelfTest.restype = ctypes.c_int
assert probe.SquadAmmoTraceSelfTest() == 1, "ammo trace merged weapon contexts or exceeded its process-wide sample budget"
probe.SquadDirectIPSelfTest.restype = ctypes.c_int
assert probe.SquadDirectIPSelfTest() == 1, "native endpoint validation failed"
probe.SquadGuestCameraSelfTest.restype = ctypes.c_int
assert probe.SquadGuestCameraSelfTest() == 1, "guest camera identity and restoration guards failed"
probe.SquadGuestCinematicSelfTest.restype = ctypes.c_int
assert probe.SquadGuestCinematicSelfTest() == 1, "cinematic reciprocal membership transaction/rollback guards failed"
probe.SquadSessionHandleSelfTest.restype = ctypes.c_int
assert probe.SquadSessionHandleSelfTest() == 1, "stale/reused object identity guard failed"
probe.SquadProbeMutexTest.restype = ctypes.c_int
assert probe.SquadProbeMutexTest() == 1, "test-instance mutex scope changed an unrelated or activation mutex"
probe.SquadProbeFolderTest.restype = ctypes.c_int
assert probe.SquadProbeFolderTest() == 1, "Documents redirect changed another folder"
probe.SquadProbePathTest.restype = ctypes.c_int
assert probe.SquadProbePathTest() == 1, "file-write guard accepted an outside path"
probe.SquadProbeEnabled.argtypes = [ctypes.c_wchar_p]
probe.SquadProbeEnabled.restype = ctypes.c_int
with tempfile.TemporaryDirectory(dir=ROOT / "local") as tmp:
    folder = Path(tmp)
    assert probe.SquadProbeEnabled(str(folder)) == 0
    (folder / ".me3-coop-sandbox.json").write_text("{}")
    assert probe.SquadProbeEnabled(str(folder)) == 0
    (folder / "ME3SquadCoop.probe.enabled").write_text("stage-0")
    assert probe.SquadProbeEnabled(str(folder)) == 1
    (folder / "ME3SquadCoop.probe.enabled").unlink()
    assert probe.SquadProbeEnabled(str(folder)) == 0
print("PASS native probe memory reader and opt-in gate (no game launched)")

# Controlled Windows integrity test. Only the new fixture's ACL is changed.
# Explicit ownership on new fixture directories avoids changing the ACL of
# any existing workspace or game directory. No administrator is required.
kernel = ctypes.WinDLL("kernel32", use_last_error=True)
advapi = ctypes.WinDLL("advapi32", use_last_error=True)
kernel.GetCurrentProcess.restype = ctypes.c_void_p
kernel.CloseHandle.argtypes = [ctypes.c_void_p]
kernel.LocalFree.argtypes = [ctypes.c_void_p]
advapi.OpenProcessToken.argtypes = [ctypes.c_void_p, ctypes.c_uint32, ctypes.POINTER(ctypes.c_void_p)]
advapi.GetTokenInformation.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_void_p, ctypes.c_uint32, ctypes.POINTER(ctypes.c_uint32)]
advapi.ConvertSidToStringSidW.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_wchar_p)]
advapi.ConvertStringSecurityDescriptorToSecurityDescriptorW.argtypes = [ctypes.c_wchar_p, ctypes.c_uint32, ctypes.POINTER(ctypes.c_void_p), ctypes.c_void_p]

token = ctypes.c_void_p()
assert advapi.OpenProcessToken(kernel.GetCurrentProcess(), 8, ctypes.byref(token)), ctypes.get_last_error()
needed = ctypes.c_uint32()
advapi.GetTokenInformation(token, 1, None, 0, ctypes.byref(needed))
user = ctypes.create_string_buffer(needed.value)
assert advapi.GetTokenInformation(token, 1, user, needed, ctypes.byref(needed))
sid = ctypes.c_void_p.from_buffer(user).value
sid_text = ctypes.c_wchar_p()
assert advapi.ConvertSidToStringSidW(sid, ctypes.byref(sid_text))
identity = sid_text.value
kernel.LocalFree(ctypes.cast(sid_text, ctypes.c_void_p))
kernel.CloseHandle(token)

class SecurityAttributes(ctypes.Structure):
    _fields_ = [("length", ctypes.c_uint32), ("descriptor", ctypes.c_void_p), ("inherit", ctypes.c_int)]

kernel.CreateDirectoryW.argtypes = [ctypes.c_wchar_p, ctypes.POINTER(SecurityAttributes)]

def owned_directory(path, low=False):
    descriptor = ctypes.c_void_p()
    sddl = f"O:{identity}D:(A;OICI;FA;;;{identity})" + ("S:(ML;OICI;NW;;;LW)" if low else "")
    assert advapi.ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl, 1, ctypes.byref(descriptor), None)
    sa = SecurityAttributes(ctypes.sizeof(SecurityAttributes), descriptor, False)
    try:
        if not kernel.CreateDirectoryW(str(path), ctypes.byref(sa)):
            raise ctypes.WinError(ctypes.get_last_error())
    finally:
        kernel.LocalFree(descriptor)

shield = ROOT / "local" / ("integrity-test-" + uuid.uuid4().hex[:8])
owned_directory(shield)
medium = shield / "medium"
owned_directory(medium)
protected = medium / "protected.txt"
protected.write_text("ORIGINAL TEST CONTENT", encoding="utf-8")
allowed = shield / "low-output"
owned_directory(allowed, low=True)
result = allowed / "result.json"
run = subprocess.run([str(BUILD / "sandbox_launch.exe"), "--selftest", str(protected), str(result)],
                     capture_output=True, text=True, timeout=25)
print(run.stdout.strip())
if run.returncode != 0:
    print(run.stderr.strip())
    raise SystemExit(run.returncode)
data = json.loads(result.read_text())
assert data["pass"] and data["protected_write_denied"] and data["integrity"] == 4096, data
assert protected.read_text(encoding="utf-8") == "ORIGINAL TEST CONTENT"
print("PASS low-integrity child denied medium-file write and wrote only its permitted output")
