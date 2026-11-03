/* Small local helper: Win32 interlocked operations are compiler intrinsics
 * on systems which do not export them from kernel32 for ctypes clients. */
#include <windows.h>
__declspec(dllexport) LONG PanelCompareExchange(volatile LONG *p,LONG exchange,LONG compare) {
    return InterlockedCompareExchange(p,exchange,compare);
}
__declspec(dllexport) LONG PanelExchange(volatile LONG *p,LONG exchange) {
    return InterlockedExchange(p,exchange);
}
