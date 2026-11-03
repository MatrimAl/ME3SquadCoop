"""Exercise the actual panel atomic helper without starting or attaching a game."""
import ctypes as C
import os
from pathlib import Path
import sys
import threading

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
from session_panel import Session

dll = C.WinDLL(str(ROOT / 'local/build/ME3PanelAtomic.dll'))
dll.PanelCompareExchange.argtypes = [C.POINTER(C.c_int32), C.c_int32, C.c_int32]
dll.PanelCompareExchange.restype = C.c_int32
dll.PanelExchange.argtypes = [C.POINTER(C.c_int32), C.c_int32]
dll.PanelExchange.restype = C.c_int32
word = C.c_int32(0)
barrier = threading.Barrier(9)
results = []
def reserve():
    barrier.wait()
    results.append(dll.PanelCompareExchange(C.byref(word), -1, 0))
threads = [threading.Thread(target=reserve) for _ in range(8)]
for thread in threads: thread.start()
barrier.wait()
for thread in threads: thread.join()
assert results.count(0) == 1 and results.count(-1) == 7 and word.value == -1
assert dll.PanelExchange(C.byref(word), 5) == -1 and word.value == 5
try:
    Session(os.getpid())
except RuntimeError as exc:
    assert str(exc) == 'Selected process is not MassEffect3.exe', str(exc)
else:
    raise AssertionError('panel accepted the Python process as a game')
print('PASS native panel atomic reservation and process identity rejection; no game launched')
