"""Hold read-only Windows sharing locks. Existing protected files cannot be rewritten or replaced."""
import ctypes
import json
import subprocess
import sys
import uuid
from pathlib import Path

kernel = ctypes.WinDLL("kernel32", use_last_error=True)
kernel.CreateFileW.argtypes = [ctypes.c_wchar_p, ctypes.c_uint32, ctypes.c_uint32,
                               ctypes.c_void_p, ctypes.c_uint32, ctypes.c_uint32, ctypes.c_void_p]
kernel.CreateFileW.restype = ctypes.c_void_p
kernel.CloseHandle.argtypes = [ctypes.c_void_p]
INVALID = ctypes.c_void_p(-1).value


class ReadLocks:
    def __init__(self, files):
        self.files = list(files)
        self.handles = []

    def __enter__(self):
        try:
            for path in self.files:
                handle = kernel.CreateFileW(str(path), 0x80000000, 1, None, 3, 0x80, None)
                if handle == INVALID:
                    raise ctypes.WinError(ctypes.get_last_error())
                self.handles.append(handle)
        except BaseException:
            self.__exit__(None, None, None)
            raise
        return self

    def __exit__(self, *_):
        for handle in self.handles:
            kernel.CloseHandle(handle)
        self.handles.clear()


def selftest(folder):
    folder = Path(folder)
    stem = "share-lock-" + uuid.uuid4().hex[:10]
    source = folder / (stem + ".txt")
    result = folder / (stem + ".json")
    source.write_text("PROTECTED TEST FILE", encoding="utf-8")
    child = r'''
import ctypes,json,sys
k=ctypes.WinDLL("kernel32",use_last_error=True)
a=ctypes.WinDLL("advapi32",use_last_error=True)
k.GetCurrentProcess.restype=ctypes.c_void_p
a.OpenProcessToken.argtypes=[ctypes.c_void_p,ctypes.c_uint32,ctypes.POINTER(ctypes.c_void_p)]
a.GetTokenInformation.argtypes=[ctypes.c_void_p,ctypes.c_int,ctypes.c_void_p,ctypes.c_uint32,ctypes.POINTER(ctypes.c_uint32)]
a.GetSidSubAuthorityCount.argtypes=[ctypes.c_void_p]
a.GetSidSubAuthorityCount.restype=ctypes.POINTER(ctypes.c_ubyte)
a.GetSidSubAuthority.argtypes=[ctypes.c_void_p,ctypes.c_uint32]
a.GetSidSubAuthority.restype=ctypes.POINTER(ctypes.c_uint32)
token=ctypes.c_void_p(); assert a.OpenProcessToken(k.GetCurrentProcess(),8,ctypes.byref(token))
size=ctypes.c_uint32(); a.GetTokenInformation(token,25,None,0,ctypes.byref(size))
buf=ctypes.create_string_buffer(size.value); assert a.GetTokenInformation(token,25,buf,size,ctypes.byref(size))
sid=ctypes.c_void_p.from_buffer(buf).value
integrity=a.GetSidSubAuthority(sid,a.GetSidSubAuthorityCount(sid)[0]-1)[0]
k.CreateFileW.argtypes=[ctypes.c_wchar_p,ctypes.c_uint32,ctypes.c_uint32,ctypes.c_void_p,ctypes.c_uint32,ctypes.c_uint32,ctypes.c_void_p]
k.CreateFileW.restype=ctypes.c_void_p
k.CloseHandle.argtypes=[ctypes.c_void_p]
h=k.CreateFileW(sys.argv[1],0x40000000,3,None,3,0x80,None)
error=ctypes.get_last_error()
denied=h==ctypes.c_void_p(-1).value and error==32
if h!=ctypes.c_void_p(-1).value:k.CloseHandle(h)
print(json.dumps({"pass":denied and integrity==8192,"share_lock":True,"integrity":integrity,"denial_error":error},separators=(",",":")))
sys.exit(0 if denied and integrity==8192 else 2)
'''
    with ReadLocks([source]):
        child_result = subprocess.run([sys.executable, "-c", child, str(source)],
                                      capture_output=True, text=True, timeout=15, check=True)
        data = json.loads(child_result.stdout)
        # A second process must also be unable to replace/delete the locked file.
        try:
            source.rename(folder / (stem + ".renamed"))
        except PermissionError:
            pass
        else:
            raise RuntimeError("protected file was renameable")
        assert source.read_text(encoding="utf-8") == "PROTECTED TEST FILE"
    # Releasing the guard must restore normal access to our synthetic file.
    source.write_text("GUARD RELEASED", encoding="utf-8")
    data["rename_denied"] = True
    result.write_text(json.dumps(data, separators=(",", ":")), encoding="utf-8")
    print("PASS cross-process write/replace denial and normal access after guard release", flush=True)
    return result
