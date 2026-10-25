#ifndef ME3_RUNTIME_READY_POLICY_H
#define ME3_RUNTIME_READY_POLICY_H
#include "runtime_ready_channel.h"
/* Ordinary entry only. Readiness is advisory synchronization for automatic
 * recovery; native join/package/ownership validation remains authoritative. */
static void runtime_ready_policy_tick(uintptr_t base,uintptr_t engine) {
    static int configured,opted,reported;
    static SessionHandle sampled_root;
    static uint64_t context;
    runtime_peer_ready=0;
    runtime_ready_channel_status=RUNTIME_READY_OFF;
    if(!configured) {
        wchar_t value[8]={0};DWORD length=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_READY_CHANNEL",value,8);
        opted=length==1&&value[0]==L'1';configured=1;
    }
    int host=g_runtime_config.role==ME3_ROLE_HOST;
    runtime_peer_ready_required=opted&&!host;
    if(!opted)return;
    if(!runtime_control_enabled) {runtime_ready_channel_status=RUNTIME_READY_DISABLED;me3_ready_close();return;}
    unsigned char bind[4],peer[4]={0};
    if(!direct_ip_native_parse_ipv4(g_runtime_config.endpoint.bind_ipv4,0,bind)||
       (!host&&!direct_ip_native_parse_ipv4(g_runtime_config.endpoint.peer_ipv4,0,peer))) {
        runtime_ready_channel_status=RUNTIME_READY_FAILED;return;
    }
    CombatContextChain chain;combat_context_read_chain(base,&chain);unsigned char net=255;
    int known=chain.world_valid&&chain.info_valid&&read_at(chain.info+0x7c8,&net,1);
    SessionContext c=session_context(engine);
    int ready=host&&known&&net==2&&!session_fault&&!session_travel_waiting&&
        InterlockedCompareExchange(&runtime_listen_successful,0,0)&&
        chain.world==runtime_listen_origin_world.pointer&&session_handle_matches(runtime_listen_origin_world)&&
        session_handle_matches(runtime_listen_origin_driver)&&
        read_ptr(chain.world+0x120)==runtime_listen_origin_driver.pointer&&runtime_session_scope(base,engine)&&
        c.world==chain.info&&runtime_host_story_idle(base,c);
    int query=!host&&known&&net==0&&!runtime_auto_rejoin_suspended&&
        !InterlockedCompareExchange(&runtime_cancel_start_requested,0,0)&&
        runtime_auto_rejoin_epoch&&runtime_auto_rejoin_attempted_epoch!=runtime_auto_rejoin_epoch&&
        runtime_auto_rejoin_old_gone(base)&&runtime_join_travel_idle(chain.world,engine);
    /* Keep querying through the stable-player window of an automatic intent. */
    if(!host&&runtime_auto_rejoin_target.pointer&&known&&net==0&&
       !runtime_auto_rejoin_suspended&&runtime_auto_rejoin_target_valid(&chain)&&
       !InterlockedCompareExchange(&runtime_cancel_start_requested,0,0)&&
       runtime_auto_rejoin_old_gone(base)&&runtime_join_travel_idle(chain.world,engine))query=1;
    uint64_t tag=0;
    if(query) {
        SessionHandle root=session_capture_handle(chain.world);
        if(!session_handle_matches(root))query=0;
        else {
            if(root.pointer!=sampled_root.pointer||root.index!=sampled_root.index||root.name!=sampled_root.name) {
                if(context==UINT64_MAX)query=0;
                else {sampled_root=root;++context;}
            }
            if(query)tag=context;
        }
    }
    int result=me3_ready_poll(bind,peer,g_runtime_config.endpoint.port,host,ready,query,tag,GetTickCount64());
    runtime_peer_ready=!host&&query&&result==1;
    runtime_ready_channel_status=result<0?RUNTIME_READY_FAILED:
        host?(ready?RUNTIME_READY_HOST_READY:RUNTIME_READY_HOST_WAIT):
        query?(runtime_peer_ready?RUNTIME_READY_PEER_READY:RUNTIME_READY_WAIT_HOST):RUNTIME_READY_GUEST_IDLE;
    if(result<0&&!reported) {
        reported=1;line("{\"runtime_network\":\"readiness-channel-unavailable\",\"automatic_recovery_waits\":true,\"manual_requests_available\":true}\r\n");
    }
}
#endif
