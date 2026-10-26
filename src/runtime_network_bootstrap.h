#ifndef ME3_RUNTIME_NETWORK_BOOTSTRAP_H
#define ME3_RUNTIME_NETWORK_BOOTSTRAP_H
/* One-shot ordinary guest join or in-place host Listen, with optional checked
 * host scheduling after disposed-world travel. Startup
 * verification is not successful guest connection or campaign evidence. */
typedef enum RuntimeNetworkStartStatus {
    RUNTIME_START_IDLE=0,RUNTIME_START_WAITING=1,RUNTIME_START_DISPATCHED=2,
    RUNTIME_START_ALREADY_NETWORKED=3,RUNTIME_START_ABI_REFUSED=4,
    RUNTIME_START_BAD_REQUEST=5,RUNTIME_START_HOST_UNAVAILABLE=6,RUNTIME_START_BIND_REFUSED=7,
    RUNTIME_START_LISTENING=8,RUNTIME_START_CAPACITY_REFUSED=9,
    RUNTIME_START_HOST_NATIVE_FAILED=10,RUNTIME_START_HOST_VERIFY_FAILED=11,
    RUNTIME_START_RETRY_WAITING=12,RUNTIME_START_RETRY_REFUSED=13,RUNTIME_START_RETRY_EXPIRED=14,
    RUNTIME_START_RELISTEN_WAITING=15,RUNTIME_START_RELISTEN_REFUSED=16,RUNTIME_START_RELISTEN_EXPIRED=17,
    RUNTIME_START_CANCELLED=18,RUNTIME_START_JOIN_UNCONFIRMED=19
} RuntimeNetworkStartStatus;
static volatile LONG runtime_join_requested,runtime_join_attempted,runtime_network_start_status;
static volatile LONG runtime_listen_requested,runtime_listen_attempted;
static volatile LONG runtime_rejoin_requested;
static DWORD runtime_rejoin_started;
static int runtime_join_retry_armed;
static volatile LONG runtime_relisten_requested,runtime_listen_successful;
static SessionHandle runtime_listen_origin_world,runtime_listen_origin_driver,runtime_relisten_target;
static SessionHandle runtime_auto_relisten_target;
static int runtime_auto_relisten_suspended;
static SessionHandle runtime_auto_rejoin_target;
static int runtime_auto_rejoin_suspended;
/* Ordinary entry publishes a fresh side-channel observation each Tick.
 * The protected probe does not open a side channel. Manual join ignores it. */
static int runtime_peer_ready_required,runtime_peer_ready;
static int runtime_auto_relisten_enabled,runtime_auto_rejoin_enabled;
enum {RUNTIME_READY_OFF=0,RUNTIME_READY_HOST_WAIT=1,RUNTIME_READY_HOST_READY=2,
    RUNTIME_READY_GUEST_IDLE=3,RUNTIME_READY_WAIT_HOST=4,RUNTIME_READY_PEER_READY=5,
    RUNTIME_READY_FAILED=6,RUNTIME_READY_DISABLED=7};
static unsigned runtime_ready_channel_status;
static DWORD runtime_relisten_started;
static int runtime_relisten_armed;
static DWORD runtime_join_last_check;
static volatile LONG runtime_cancel_start_requested;
static int runtime_startup_focus_pending(void) {
    if(!runtime_control_enabled||InterlockedCompareExchange(&runtime_cancel_start_requested,0,0))return 0;
    if(g_runtime_config.role==ME3_ROLE_GUEST)
        return InterlockedCompareExchange(&runtime_join_requested,0,0)&&
            !InterlockedCompareExchange(&runtime_join_attempted,0,0);
    if(g_runtime_config.role==ME3_ROLE_HOST)
        return (InterlockedCompareExchange(&runtime_listen_requested,0,0)&&
                !InterlockedCompareExchange(&runtime_listen_attempted,0,0))||
            (InterlockedCompareExchange(&runtime_listen_successful,0,0)&&
                InterlockedCompareExchange(&runtime_host_start_gate,0,0));
    return 0;
}
#ifdef ME3_RUNTIME_FIXTURE
static int runtime_startup_focus_selftest(void) {
    Me3RuntimeRole role=g_runtime_config.role;int enabled=runtime_control_enabled;
    LONG requested=runtime_join_requested,attempted=runtime_join_attempted,cancel=runtime_cancel_start_requested;
    LONG listen=runtime_listen_requested,listen_attempted=runtime_listen_attempted,success=runtime_listen_successful,gate=runtime_host_start_gate;
    g_runtime_config.role=ME3_ROLE_GUEST;runtime_control_enabled=1;
    runtime_join_requested=1;runtime_join_attempted=runtime_cancel_start_requested=0;
    int ok=runtime_startup_focus_pending();
    runtime_join_attempted=1;ok=ok&&!runtime_startup_focus_pending();runtime_join_attempted=0;
    runtime_cancel_start_requested=1;ok=ok&&!runtime_startup_focus_pending();runtime_cancel_start_requested=0;
    runtime_join_requested=0;ok=ok&&!runtime_startup_focus_pending();runtime_join_requested=1;
    runtime_control_enabled=0;ok=ok&&!runtime_startup_focus_pending();runtime_control_enabled=1;
    g_runtime_config.role=ME3_ROLE_HOST;runtime_listen_requested=runtime_listen_attempted=runtime_listen_successful=runtime_host_start_gate=0;
    ok=ok&&!runtime_startup_focus_pending();runtime_listen_requested=1;ok=ok&&runtime_startup_focus_pending();
    runtime_listen_attempted=1;ok=ok&&!runtime_startup_focus_pending();
    runtime_listen_requested=0;runtime_listen_successful=1;ok=ok&&!runtime_startup_focus_pending();
    runtime_host_start_gate=1;ok=ok&&runtime_startup_focus_pending();
    runtime_cancel_start_requested=1;ok=ok&&!runtime_startup_focus_pending();
    g_runtime_config.role=role;runtime_control_enabled=enabled;runtime_join_requested=requested;
    runtime_join_attempted=attempted;runtime_cancel_start_requested=cancel;
    runtime_listen_requested=listen;runtime_listen_attempted=listen_attempted;runtime_listen_successful=success;runtime_host_start_gate=gate;return ok;
}
#endif
static int runtime_request_cancel_start(void) {
    if(g_config_status!=ME3_CONFIG_READY||
       (g_runtime_config.role!=ME3_ROLE_HOST&&g_runtime_config.role!=ME3_ROLE_GUEST))return 0;
    return InterlockedCompareExchange(&runtime_cancel_start_requested,1,0)==0;
}
static int runtime_network_start_parse(Me3RuntimeRole role,const wchar_t *value) {
    if(!value)return 0;
    if(!wcscmp(value,L"join"))return role==ME3_ROLE_GUEST?1:-1;
    if(!wcscmp(value,L"listen")&&role==ME3_ROLE_HOST)return 2;
    return -1;
}
static int runtime_request_join(void) {
    if(g_config_status!=ME3_CONFIG_READY||g_runtime_config.role!=ME3_ROLE_GUEST||
       !InterlockedCompareExchange(&g_enable_requested,0,0)||
       InterlockedCompareExchange(&runtime_join_attempted,0,0)||
       InterlockedCompareExchange(&runtime_rejoin_requested,0,0))return 0;
    if(InterlockedCompareExchange(&runtime_join_requested,1,0))return 0;
    InterlockedExchange(&runtime_network_start_status,RUNTIME_START_WAITING);return 1;
}
/* Export callers only enqueue intent. Native world/cleanup state belongs
 * exclusively to Tick, never an arbitrary caller thread. */
static int runtime_request_rejoin(void) {
    if(g_config_status!=ME3_CONFIG_READY||g_runtime_config.role!=ME3_ROLE_GUEST||
       !InterlockedCompareExchange(&g_enable_requested,0,0)||
       !InterlockedCompareExchange(&runtime_join_attempted,0,0)||
       InterlockedCompareExchange(&runtime_join_requested,0,0))return 0;
    return InterlockedCompareExchange(&runtime_rejoin_requested,1,0)==0;
}
static int runtime_request_listen(void) {
    if(g_config_status!=ME3_CONFIG_READY||g_runtime_config.role!=ME3_ROLE_HOST||
       !InterlockedCompareExchange(&g_enable_requested,0,0)||
       InterlockedCompareExchange(&runtime_listen_attempted,0,0)||
       InterlockedCompareExchange(&runtime_relisten_requested,0,0))return 0;
    if(InterlockedCompareExchange(&runtime_listen_requested,1,0))return 0;
    InterlockedExchange(&runtime_host_start_gate,0);
    InterlockedExchange(&runtime_network_start_status,RUNTIME_START_WAITING);return 1;
}
static int runtime_request_relisten(void) {
    if(g_config_status!=ME3_CONFIG_READY||g_runtime_config.role!=ME3_ROLE_HOST||
       !InterlockedCompareExchange(&g_enable_requested,0,0)||
       !InterlockedCompareExchange(&runtime_listen_attempted,0,0)||
       !InterlockedCompareExchange(&runtime_listen_successful,0,0)||
       InterlockedCompareExchange(&runtime_listen_requested,0,0))return 0;
    return InterlockedCompareExchange(&runtime_relisten_requested,1,0)==0;
}
static int runtime_join_command(const Me3RuntimeConfig *config,wchar_t *out,size_t capacity) {
    if(!config||config->role!=ME3_ROLE_GUEST||!out||!capacity||
       !direct_ip_native_parse_ipv4(config->endpoint.peer_ipv4,0,NULL)||!config->endpoint.port)return 0;
    int n=_snwprintf(out,capacity,L"open %ls:%u",config->endpoint.peer_ipv4,(unsigned)config->endpoint.port);
    return n>0&&(size_t)n<capacity;
}
static int runtime_join_console_abi(uintptr_t base,uintptr_t pc) {
    uintptr_t fn=function_named(base,"PlayerController","ConsoleCommand");
    uint32_t flags=0;uint16_t bytes=0,result=0;unsigned char count=0;
    return is_live(base,pc)&&is_kind_of(base,pc,"SFXPlayerController")&&fn&&
        read_at(fn+0xe0,&flags,4)&&flags==0x24400&&read_at(fn+0xe6,&bytes,2)&&bytes==sizeof(ConsoleParms)&&
        read_at(fn+0xe8,&result,2)&&result==0x14&&read_at(fn+0xea,&count,1)&&count==3&&
        guest_cinematic_allocator_pins(base);
}
static int runtime_join_travel_idle(uintptr_t world,uintptr_t engine) {
    uintptr_t driver=0,pending=0;
    /* Unknown loading state must not masquerade as an absent driver/travel. */
    return read_at(world+0x120,&driver,8)&&!driver&&
        read_at(engine+0x9fc,&pending,8)&&!pending;
}
static int runtime_auto_rejoin_target_valid(const CombatContextChain *chain) {
    return !runtime_auto_rejoin_target.pointer||(chain->world_valid&&
        chain->world==runtime_auto_rejoin_target.pointer&&session_handle_matches(runtime_auto_rejoin_target));
}
#include "runtime_host_bootstrap.h"
#include "runtime_host_package_preload.h"
#include "runtime_guest_archetype_preload.h"
#include "runtime_join_readiness.h"
#include "runtime_join_diagnostics.h"
#include "runtime_join_progress.h"
#include "runtime_join_outcome.h"
#include "runtime_rejoin.h"
#include "runtime_auto_rejoin.h"
#include "runtime_relisten.h"
#include "runtime_auto_relisten.h"
static void runtime_join_window_reset(void) {
    memset(&runtime_join_readiness,0,sizeof runtime_join_readiness);
}
static int runtime_cancel_start_tick(void) {
    if(!InterlockedExchange(&runtime_cancel_start_requested,0))return 0;
    /* A cancelled automatic intent must not silently reappear next Tick.
     * Manual Listen/relisten remains available for this process. */
    runtime_auto_relisten_suspended=1;
    runtime_auto_rejoin_suspended=1;
    memset(&runtime_auto_relisten_target,0,sizeof runtime_auto_relisten_target);
    memset(&runtime_auto_rejoin_target,0,sizeof runtime_auto_rejoin_target);
    LONG join=InterlockedExchange(&runtime_join_requested,0),listen=InterlockedExchange(&runtime_listen_requested,0),
        rejoin=InterlockedExchange(&runtime_rejoin_requested,0),relisten=InterlockedExchange(&runtime_relisten_requested,0);
    int pending=join||listen||rejoin||relisten;
    if(pending) {
        /* Rearmed retries originated from consumed one-shot attempts. Retain
         * that history; cancellation is not an initialization-failure reset. */
        if(runtime_join_retry_armed)InterlockedExchange(&runtime_join_attempted,1);
        if(runtime_relisten_armed)InterlockedExchange(&runtime_listen_attempted,1);
        runtime_join_retry_armed=runtime_relisten_armed=0;
        runtime_rejoin_started=runtime_relisten_started=0;
        memset(&runtime_relisten_target,0,sizeof runtime_relisten_target);
        runtime_join_window_reset();
        if(listen&&!session_fault&&!InterlockedCompareExchange(&runtime_listen_attempted,0,0))
            InterlockedExchange(&runtime_host_start_gate,1); /* Original existing-session attach policy. */
        InterlockedExchange(&runtime_network_start_status,RUNTIME_START_CANCELLED);
    }
    line(pending?"{\"runtime_network\":\"pending-start-cancelled\",\"automatic_relisten_suspended\":true,\"automatic_rejoin_suspended\":true,\"native_connection_touched\":false}\r\n":
        "{\"runtime_network\":\"cancel-no-pending-start\",\"automatic_relisten_suspended\":true,\"automatic_rejoin_suspended\":true,\"native_connection_touched\":false}\r\n");
    return 1; /* Concurrent fresh requests may be considered on the next Tick. */
}
static void runtime_network_start_tick(uintptr_t base,uintptr_t engine,DWORD now) {
    if(runtime_cancel_start_tick())return;
    runtime_auto_rejoin_tick(base,engine,now);
    runtime_rejoin_tick(base,engine,now);
    runtime_auto_relisten_tick(base,engine,now);
    runtime_relisten_tick(base,engine,now);
    int host=g_runtime_config.role==ME3_ROLE_HOST;
    volatile LONG *requested=host?&runtime_listen_requested:&runtime_join_requested,
        *attempted=host?&runtime_listen_attempted:&runtime_join_attempted;
    if(!runtime_control_enabled||!InterlockedCompareExchange(requested,0,0)||
       InterlockedCompareExchange(attempted,0,0)){runtime_join_window_reset();return;}
    if(runtime_join_last_check&&now-runtime_join_last_check<250)return;
    runtime_join_last_check=now;
    if(!network_actor_world_pins(base)||!combat_context_layout(base)){runtime_join_window_reset();return;}
    SessionContext c=session_context(engine);CombatContextChain before,after;
    combat_context_read_chain(base,&before);unsigned char net=255;
    uintptr_t local_pc=0,player=read_ptr(c.pc+0x67c);
    if(!before.info_valid||c.world!=before.info||!session_actor_live(base,c.pc)||
       !combat_context_local_pc(base,engine,&local_pc)||local_pc!=c.pc||
       !is_live(base,player)||!is_kind_of(base,player,"LocalPlayer")||read_ptr(player+0x68)!=c.pc||
       read_ptr(c.pc+0x198)!=c.world||!read_at(c.world+0x7c8,&net,1)){runtime_join_window_reset();return;}
    if(net!=0) {
        memset(&runtime_auto_rejoin_target,0,sizeof runtime_auto_rejoin_target);
        runtime_join_retry_armed=0;runtime_join_window_reset();
        runtime_relisten_armed=0;
        InterlockedExchange(attempted,1);InterlockedExchange(requested,0);
        /* Existing-session attachment is the default policy. A redundant
         * listen request must not revoke an already established host lease. */
        if(host)InterlockedExchange(&runtime_host_start_gate,1);
        InterlockedExchange(&runtime_network_start_status,RUNTIME_START_ALREADY_NETWORKED);return;
    }
    if(!host&&runtime_auto_rejoin_target.pointer&&
       (!runtime_auto_rejoin_target_valid(&before)||!runtime_auto_rejoin_old_gone(base))) {
        InterlockedExchange(&runtime_join_requested,0);InterlockedExchange(&runtime_join_attempted,1);
        runtime_join_retry_armed=0;
        runtime_rejoin_finish(RUNTIME_START_RETRY_REFUSED,"automatic-rejoin-context-changed",now);return;
    }
    /* Wait for ordinary native control; never clear pause, scene or mode flags.
     * Current-world driver/pending travel must both be absent before the call. */
    if(session_engine_blocked(c)||!runtime_join_travel_idle(before.world,engine)||session_active||
       (!host&&runtime_join_retry_armed&&!runtime_rejoin_cleanup_idle())) {
        runtime_join_window_reset();return;
    }
    /* Native transport consumes its own command line, not endpoint.bind_ipv4.
     * Refuse an unintended local interface before triggering ordinary travel. */
    if(!runtime_transport_bind_argument(base,g_runtime_config.endpoint.bind_ipv4)) {
        memset(&runtime_auto_rejoin_target,0,sizeof runtime_auto_rejoin_target);
        InterlockedExchange(attempted,1);InterlockedExchange(requested,0);
        InterlockedExchange(&runtime_network_start_status,RUNTIME_START_BIND_REFUSED);
        line("{\"runtime_network\":\"start-refused\",\"reason\":\"native-multihome-mismatch\",\"travel_dispatched\":false}\r\n");return;
    }
    if(host) {
        if(!runtime_host_story_idle(base,c))return;
        if(!runtime_host_package_preload(base,c))return;
        if(runtime_relisten_armed&&(!runtime_relisten_target_valid(base,&before)||
           !runtime_relisten_previous_gone(base)||!runtime_rejoin_cleanup_idle()||runtime_host_capacity.active)) {
            InterlockedExchange(&runtime_listen_requested,0);InterlockedExchange(&runtime_listen_attempted,1);
            runtime_relisten_armed=0;
            runtime_relisten_finish(RUNTIME_START_RELISTEN_REFUSED,"new-world-context-changed",now);return;
        }
        InterlockedExchange(attempted,1);InterlockedExchange(requested,0);
        runtime_relisten_armed=0;InterlockedExchange(&runtime_listen_successful,0);
        int status=runtime_host_start(base,engine,c,&before);
        if(status==RUNTIME_START_LISTENING) {
            runtime_listen_origin_world=session_capture_handle(before.world);
            runtime_listen_origin_driver=session_capture_handle(read_ptr(before.world+0x120));
            if(session_handle_matches(runtime_listen_origin_world)&&session_handle_matches(runtime_listen_origin_driver))
                InterlockedExchange(&runtime_listen_successful,1);
        }
        InterlockedExchange(&runtime_host_start_gate,status==RUNTIME_START_LISTENING);
        InterlockedExchange(&runtime_network_start_status,status);
        char row[280];snprintf(row,sizeof row,"{\"runtime_network\":\"listen-request\",\"status\":%d,\"native_start_verified\":%s,\"implicit_travel\":false,\"guest_connection_verified\":false,\"restart_required\":%s,\"monotonic_ms\":%lu}\r\n",status,runtime_host_start_gate?"true":"false",runtime_host_start_gate?"false":"true",now);line(row);return;
    }
    RuntimeJoinSample ready={0};
    int player_ready=runtime_join_context_sample(base,c,&before,&ready);
    if(!runtime_join_readiness_observe(&runtime_join_readiness,player_ready?&ready:NULL,now)) {
        static const char *last_ready_reason;
        const char *reason=player_ready?"stable-ordinary-player-window":runtime_join_context_last_reason;
        if(!last_ready_reason||strcmp(last_ready_reason,reason)) {
            last_ready_reason=reason;
            char row[300];snprintf(row,sizeof row,"{\"runtime_network\":\"join-wait\",\"reason\":\"%s\",\"join_dispatched\":false,\"monotonic_ms\":%lu}\r\n",reason,now);line(row);
        }
        return;
    }
    if(!runtime_guest_archetype_preload(base,c)){runtime_join_window_reset();return;}
    wchar_t command[64];
    if(runtime_auto_rejoin_target.pointer&&runtime_peer_ready_required&&!runtime_peer_ready)return;
    if(!runtime_join_command(&g_runtime_config,command,64)||!runtime_join_console_abi(base,c.pc)) {
        memset(&runtime_auto_rejoin_target,0,sizeof runtime_auto_rejoin_target);
        InterlockedExchange(&runtime_join_attempted,1);InterlockedExchange(&runtime_join_requested,0);
        InterlockedExchange(&runtime_network_start_status,RUNTIME_START_ABI_REFUSED);return;
    }
    SessionHandle controller=session_capture_handle(c.pc);
    combat_context_read_chain(base,&after);
    RuntimeJoinSample final_ready={0};SessionContext current_context=session_context(engine);
    if(!session_handle_matches(controller)||!combat_context_chain_equal(&before,&after)||local_controller(engine)!=c.pc||
       current_context.pc!=c.pc||current_context.pawn!=c.pawn||current_context.world!=c.world||
       current_context.manager!=c.manager||current_context.squad!=c.squad||
       !runtime_join_travel_idle(before.world,engine)||!runtime_join_context_sample(base,current_context,&after,&final_ready)||
       (runtime_join_retry_armed&&!runtime_rejoin_cleanup_idle())||
       (runtime_auto_rejoin_target.pointer&&(!runtime_auto_rejoin_target_valid(&after)||
        !runtime_auto_rejoin_old_gone(base)))||
       !runtime_join_sample_same(&ready,&final_ready)) {
        memset(&runtime_join_readiness,0,sizeof runtime_join_readiness);return;
    }
    /* Mark before invoking: native travel can invalidate the captured PC/world.
     * Nothing from that old context is dereferenced after the engine call. */
    InterlockedExchange(&runtime_join_attempted,1);InterlockedExchange(&runtime_join_requested,0);
    runtime_join_retry_armed=0;
    memset(&runtime_auto_rejoin_target,0,sizeof runtime_auto_rejoin_target);
    ConsoleParms params={0};params.command.data=command;params.command.count=params.command.max=(int32_t)wcslen(command)+1;
    params.write_log=1;runtime_join_progress_begin(engine,now);
    runtime_join_outcome=(RuntimeJoinOutcome){now,1};
    int dispatched=invoke(base,c.pc,"PlayerController","ConsoleCommand",&params);
    runtime_join_log_console_result(&params.result);
    if(params.result.data&&guest_cinematic_allocator_pins(base)) {
        typedef void (*EngineFree)(void*,uint32_t);
        ((EngineFree)(base+0xbf0d0))(params.result.data,0);
    }
    InterlockedExchange(&runtime_network_start_status,dispatched?RUNTIME_START_DISPATCHED:RUNTIME_START_ABI_REFUSED);
    char row[384];snprintf(row,sizeof row,"{\"runtime_network\":\"join-request\",\"ordinary_console_dispatched\":%s,\"connection_verified\":false,\"monotonic_ms\":%lu,\"ordinary_ready_samples\":%u,\"ordinary_ready_ms\":%lu,\"pre_dispatch_world_time\":%.3f}\r\n",dispatched?"true":"false",now,runtime_join_readiness.samples,now-runtime_join_readiness.first_at,final_ready.world_time);line(row);
}
static int runtime_network_start_selftest(void) {
    Me3RuntimeConfig config={0};wchar_t command[64];
    if(runtime_network_start_parse(ME3_ROLE_GUEST,NULL)||runtime_network_start_parse(ME3_ROLE_GUEST,L"join")!=1||
       runtime_network_start_parse(ME3_ROLE_HOST,L"join")!=-1||runtime_network_start_parse(ME3_ROLE_HOST,L"listen")!=2||
       runtime_network_start_parse(ME3_ROLE_GUEST,L"join extra")!=-1)return 0;
    if(runtime_config_parse(&config,L"1",L"guest",L"26.1.2.3",L"26.2.3.4",L"17779")!=ME3_CONFIG_READY||
       !runtime_join_command(&config,command,64)||wcscmp(command,L"open 26.2.3.4:17779")||
       runtime_join_command(&config,command,4))return 0;
    config.role=ME3_ROLE_HOST;if(runtime_join_command(&config,command,64))return 0;
    config.role=ME3_ROLE_GUEST;wcscpy(config.endpoint.peer_ipv4,L"127.0.0.1");
    return !runtime_join_command(&config,command,64);
}
static int runtime_join_travel_idle_selftest(void) {
    unsigned char world[0x130]={0},engine[0xa10]={0};uintptr_t value=1;
    if(!runtime_join_travel_idle((uintptr_t)world,(uintptr_t)engine)||
       runtime_join_travel_idle(1,(uintptr_t)engine)||runtime_join_travel_idle((uintptr_t)world,1))return 0;
    memcpy(world+0x120,&value,8);
    if(runtime_join_travel_idle((uintptr_t)world,(uintptr_t)engine))return 0;
    value=0;memcpy(world+0x120,&value,8);value=1;memcpy(engine+0x9fc,&value,8);
    return !runtime_join_travel_idle((uintptr_t)world,(uintptr_t)engine);
}
#endif
