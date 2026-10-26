#ifndef ME3_RUNTIME_RPC_RECEIVE_H
#define ME3_RUNTIME_RPC_RECEIVE_H
#include "native_rpc_receive_context.h"
#include "native_actor_receive_pins.h"
/* Forwarding-only ordinary entry. No bunch decoding, message consumption,
 * counter write, synthetic input or package/engine validation replacement. */
typedef void (*RuntimeRpcReceive)(void*,void*);
static RuntimeRpcReceive runtime_rpc_receive_original;
static uintptr_t runtime_rpc_receive_base;
static DWORD runtime_rpc_receive_thread;
static int runtime_rpc_receive_attempted,runtime_rpc_receive_installed;
static void runtime_rpc_receive(void *self,void *bunch) {
    DWORD error=GetLastError();NativeRpcReceiveFrame frame;
    native_rpc_receive_begin(&frame,runtime_rpc_receive_thread,
        runtime_control_enabled&&runtime_scope_ready?(uintptr_t)self:0);
    SetLastError(error);runtime_rpc_receive_original(self,bunch);
    error=GetLastError();native_rpc_receive_end(&frame);SetLastError(error);
}
static int runtime_rpc_receive_install(uintptr_t base) {
    uintptr_t slot=base+0x11a2150+0x2d0,expected=base+0x488700;
    if(runtime_rpc_receive_installed)return runtime_rpc_receive_base==base&&
        (uintptr_t)runtime_rpc_receive_original==expected&&read_ptr(slot)==(uintptr_t)runtime_rpc_receive;
    if(runtime_rpc_receive_attempted)return 0;
    runtime_rpc_receive_attempted=1;
    if(!supported_image(base)||!native_actor_receive_pins(base)||read_ptr(slot)!=expected)return 0;
    DWORD old=0;if(!VirtualProtect((void*)slot,8,PAGE_READWRITE,&old))return 0;
    runtime_rpc_receive_base=base;runtime_rpc_receive_thread=GetCurrentThreadId();
    runtime_rpc_receive_original=(RuntimeRpcReceive)expected;
    PVOID previous=InterlockedCompareExchangePointer((PVOID volatile*)slot,(PVOID)runtime_rpc_receive,(PVOID)expected);
    DWORD ignored;VirtualProtect((void*)slot,8,old,&ignored);
    if((uintptr_t)previous!=expected){runtime_rpc_receive_original=NULL;return 0;}
    network_actor_received_bunch_observer=(uintptr_t)runtime_rpc_receive;
    runtime_rpc_receive_installed=1;
    line("{\"runtime\":\"rpc-receive-context-installed\",\"forwarding_only\":true}\r\n");return 1;
}
static void runtime_rpc_receive_prepare(uintptr_t base) {
    /* Default off until ordinary reconciliation is integrated and validated.
     * Sharing the existing explicit reconciliation opt-in adds no QA mode. */
    static int opted=-1;
    if(opted<0){wchar_t value[8]={0};opted=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_AMMO_RECONCILE",value,8)==1&&value[0]==L'1';}
    if(opted&&runtime_control_enabled&&runtime_scope_ready)runtime_rpc_receive_install(base);
}
#endif
