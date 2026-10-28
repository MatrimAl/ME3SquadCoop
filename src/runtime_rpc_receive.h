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
static int runtime_initial_reference_installed;
#define ME3_INITIAL_OBJECT_ORDINARY
#define initial_actor_trace_base runtime_rpc_receive_base
#define initial_actor_trace_thread runtime_rpc_receive_thread
#define initial_actor_trace_installed runtime_rpc_receive_installed
#include "initial_object_trace.h"
#undef initial_actor_trace_base
#undef initial_actor_trace_thread
#undef initial_actor_trace_installed
#undef ME3_INITIAL_OBJECT_ORDINARY
#include "runtime_initial_reference.h"
static unsigned runtime_initial_receive_events;
static void runtime_rpc_receive(void *self,void *bunch) {
    DWORD error=GetLastError();NativeRpcReceiveFrame frame;
    InitialObjectFrame initial={0};int initial_thread=GetCurrentThreadId()==runtime_rpc_receive_thread;
    uintptr_t base=runtime_rpc_receive_base,channel=(uintptr_t)self;
    uint32_t flags_before=0;unsigned char initial_byte=0;
    if(initial_thread&&initial_object_installed&&runtime_initial_receive_events<128&&
       is_live(base,channel)&&read_ptr(channel)==base+0x11a2150&&!read_ptr(channel+0xa0)&&
       read_at((uintptr_t)bunch+0xc4,&initial_byte,1)&&initial_byte&&
       read_at(channel+0x68,&flags_before,4)) {
        initial.active=1;initial.event=++runtime_initial_receive_events;
        initial.channel=channel;initial.archive=(uintptr_t)bunch;
        read_at(channel+0x6c,&initial.index,4);
    }
    if(initial_thread){initial.previous=initial_object_frame;initial_object_frame=&initial;}
    native_rpc_receive_begin(&frame,runtime_rpc_receive_thread,
        runtime_control_enabled&&runtime_scope_ready?(uintptr_t)self:0);
    SetLastError(error);runtime_rpc_receive_original(self,bunch);
    error=GetLastError();native_rpc_receive_end(&frame);
    if(initial_thread)initial_object_frame=initial.previous;
    if(initial.active) {
        uint32_t flags_after=0;read_at(channel+0x68,&flags_after,4);
        char row[340];snprintf(row,sizeof row,"{\"runtime_initial_actor\":\"after-native-receive\",\"event\":%u,\"channel_index\":%d,\"flags_before\":%u,\"flags_after\":%u,\"actor_present\":%s,\"class_present\":%s,\"forwarded_unchanged\":true}\r\n",initial.event,initial.index,flags_before,flags_after,read_ptr(channel+0xa0)?"true":"false",read_ptr(channel+0xa8)?"true":"false");line(row);
    }
    SetLastError(error);
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
#include "runtime_owned_optimized_receive.h"
#include "runtime_delayed_fire_stop.h"
static void runtime_rpc_receive_prepare(uintptr_t base) {
    /* The scalar presentation RPC uses the same forwarding-only provenance
     * frame. Reconciliation retains its separate explicit opt-in. */
    static int opted=-1,presentation=-1,revive=-1;
    if(opted<0){wchar_t value[8]={0};opted=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_AMMO_RECONCILE",value,8)==1&&value[0]==L'1';}
    if(presentation<0){wchar_t value[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_REMOTE_PRESENTATION",value,8);
        presentation=!n||(n==1&&value[0]==L'1');}
    if(revive<0){wchar_t value[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_REVIVE",value,8);
        revive=!n||(n==1&&value[0]==L'1');}
    int owned=g_runtime_config.role==ME3_ROLE_GUEST&&runtime_owned_optimized_opted();
    int delayed=g_runtime_config.role==ME3_ROLE_HOST&&runtime_delayed_fire_stop_enabled();
    int hit=g_runtime_config.role==ME3_ROLE_GUEST&&runtime_hit_enabled();
    if(((opted||presentation||revive||owned||delayed||hit)&&runtime_control_enabled&&runtime_scope_ready)||
       (g_runtime_config.role==ME3_ROLE_GUEST&&initial_object_opted()&&runtime_control_enabled)) {
        if(runtime_rpc_receive_install(base)){initial_object_install(base);runtime_initial_reference_install(base);}
    }
    runtime_owned_optimized_prepare(base);
    if(runtime_rpc_receive_installed)runtime_delayed_fire_stop_prepare(base);
}
#endif
