"""DLL harness only: no game starts and no engine memory is written."""
import ctypes
import json
import os
import subprocess
import tempfile
import uuid
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "local" / "build"

probe = ctypes.CDLL(str(BUILD / "ME3SquadCoopProbe.asi"))
probe.SquadProbeSelfTest.restype = ctypes.c_int
assert probe.SquadProbeSelfTest() == 1, "safe memory-read positive/negative controls failed"
probe.SquadRuntimeIdentitySelfTest.restype = ctypes.c_int
assert probe.SquadRuntimeIdentitySelfTest() == 1, "shared image landmarks accepted an invalid timestamp, signature or address"
probe.SquadRuntimeConfigSelfTest.restype = ctypes.c_int
assert probe.SquadRuntimeConfigSelfTest() == 1, "explicit runtime configuration accepted QA defaults or invalid role/endpoints"
probe.SquadStateReaderSelfTest.restype = ctypes.c_int
assert probe.SquadStateReaderSelfTest() == 1, "state reader accepted missing, invalid or special native state nodes"
probe.SquadCatchupGroundSelfTest.restype = ctypes.c_int
assert probe.SquadCatchupGroundSelfTest() == 1, "catch-up ground checks accepted invalid surfaces or rejected world-floor readiness"
probe.SquadCombatRouteSelfTest.restype = ctypes.c_int
assert probe.SquadCombatRouteSelfTest() == 1, "combat route accepted unsafe context, invalid steering or off-corridor movement"
probe.SquadCombatActionSelfTest.restype = ctypes.c_int
assert probe.SquadCombatActionSelfTest() == 1, "normal action pulse lost forwarding, bounded release or captured input identity"
probe.SquadAmmoTraceSelfTest.restype = ctypes.c_int
assert probe.SquadAmmoTraceSelfTest() == 1, "ammo trace merged weapon contexts or exceeded its process-wide sample budget"
probe.SquadFunctionLookupSelfTest.restype = ctypes.c_int
assert probe.SquadFunctionLookupSelfTest() == 1, "function cache accepted a changed/reused function, owner, class or object-table entry"
probe.SquadNetworkActorSelfTest.restype = ctypes.c_int
assert probe.SquadNetworkActorSelfTest() == 1, "channel lookup accepted a stale world, duplicate peer/channel or broken reciprocal identity"
probe.SquadAmmoWireSelfTest.restype = ctypes.c_int
assert probe.SquadAmmoWireSelfTest() == 1, "ammo wire observer changed forwarding, nested receive scope or its record budget"
probe.SquadAmmoReconcileSelfTest.restype = ctypes.c_int
assert probe.SquadAmmoReconcileSelfTest() == 1, "ammo reconciliation accepted a malformed/stale reply or changed unrelated script forwarding"
probe.SquadFireRpcTraceSelfTest.restype = ctypes.c_int
assert probe.SquadFireRpcTraceSelfTest() == 1, "weapon script observer changed forwarding, receive classification or its record limit"
probe.SquadNativeFireTraceSelfTest.restype = ctypes.c_int
assert probe.SquadNativeFireTraceSelfTest() == 1, "native fire observer changed forwarding, accepted another identity/native body or exceeded its record limit"
probe.SquadGuestHitTraceSelfTest.restype = ctypes.c_int
assert probe.SquadGuestHitTraceSelfTest() == 1, "hit observer changed forwarding/nesting, admitted stale targets/callbacks, or exceeded its paired record budget"
probe.SquadDirectIPSelfTest.restype = ctypes.c_int
assert probe.SquadDirectIPSelfTest() == 1, "native endpoint validation failed"
probe.SquadGuestCameraSelfTest.restype = ctypes.c_int
assert probe.SquadGuestCameraSelfTest() == 1, "guest camera identity and restoration guards failed"
probe.SquadGuestCollisionSelfTest.restype = ctypes.c_int
assert probe.SquadGuestCollisionSelfTest() == 1, "guest collision baseline/release and masked compare-exchange guards failed"
probe.SquadGuestCinematicSelfTest.restype = ctypes.c_int
assert probe.SquadGuestCinematicSelfTest() == 1, "cinematic reciprocal membership transaction/rollback guards failed"
probe.SquadSessionAdmissionDiagnosticsSelfTest.restype = ctypes.c_int
assert probe.SquadSessionAdmissionDiagnosticsSelfTest() == 1, "session admission diagnostic exceeded its clock/cap or misclassified a rejection"
probe.SquadExceptionContextSelfTest.restype = ctypes.c_int
assert probe.SquadExceptionContextSelfTest() == 1, "exception observation falsely attributed an unknown/private allocation or missing parameters"
probe.SquadSessionGateSelfTest.restype = ctypes.c_int
assert probe.SquadSessionGateSelfTest() == 1, "session gate accepted PlayersOnly, unreadable, cinematic or conversation control"
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
probe.SquadProbeGuardsOnlySelfTest.argtypes=[ctypes.c_wchar_p]
probe.SquadProbeGuardsOnlySelfTest.restype=ctypes.c_int
with tempfile.TemporaryDirectory(dir=ROOT / "local") as tmp:
    folder=Path(tmp)
    (folder / ".me3-coop-sandbox.json").write_text("{}")
    (folder / "ME3SquadCoop.probe.enabled").write_text("stage-0")
    runtime=folder / "ME3SquadCoopRuntime"
    (runtime / "Documents-client").mkdir(parents=True)
    (runtime / "test-action.txt").write_text("host-pair")
    assert probe.SquadProbeGuardsOnlySelfTest(str(folder))==1, "guards-only skipped isolation or admitted orchestration/engine bridge"
    (folder / "ME3SquadCoop.probe.enabled").unlink()
    assert probe.SquadProbeGuardsOnlySelfTest(str(folder))==0, "guards-only accepted a missing private marker"
clean_environment={k:v for k,v in os.environ.items() if k!="ME3_SQUAD_COOP_GUARDS_ONLY_QA"}
code=("import ctypes,sys; p=ctypes.CDLL(sys.argv[1]); "
      "f=p.SquadProbeGuardsEnvironmentFixture; f.restype=ctypes.c_int; "
      "assert f()==int(sys.argv[2])")
for value,expected in ((None,0),("0",0),("1",1),("",-1),("true",-1),("11",-1)):
    environment=dict(clean_environment)
    if value is not None:environment["ME3_SQUAD_COOP_GUARDS_ONLY_QA"]=value
    subprocess.run(["python","-c",code,str(BUILD / "ME3SquadCoopProbe.asi"),str(expected)],
                   env=environment,check=True,timeout=15)
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
