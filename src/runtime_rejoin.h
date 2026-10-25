#ifndef ME3_RUNTIME_REJOIN_H
#define ME3_RUNTIME_REJOIN_H
/* Explicit recovery after an ordinary join attempt. No automatic reconnect,
 * forced disconnect, fault reset, mission selection or stale-world cleanup. */
static void runtime_rejoin_finish(int status,const char *reason,DWORD now) {
    char row[350];snprintf(row,sizeof row,
        "{\"runtime_network\":\"rejoin-request\",\"reason\":\"%s\",\"status\":%d,\"connection_verified\":false,\"monotonic_ms\":%lu}\r\n",
        reason,status,now);line(row);
    runtime_rejoin_started=0;
    InterlockedExchange(&runtime_network_start_status,status);
    InterlockedExchange(&runtime_rejoin_requested,0);
}
static int runtime_rejoin_cleanup_idle(void) {
    if(session_active||session_releasing||session_cleanup_pending||session_fault||
       guest_camera_lease.applied||guest_player_context_lease.registered||
       guest_player_context_lease.pending_cleanup||guest_player_context_lease.squad.pointer||
       guest_existing_context.active||guest_existing_context.registered)return 0;
    for(unsigned i=0;i<16;++i)if(guest_cinematic_leases[i].used)return 0;
    return 1;
}
static void runtime_rejoin_tick(uintptr_t base,uintptr_t engine,DWORD now) {
    if(!InterlockedCompareExchange(&runtime_rejoin_requested,0,0))return;
    if(!runtime_rejoin_started) {
        runtime_rejoin_started=now;
        InterlockedExchange(&runtime_network_start_status,RUNTIME_START_RETRY_WAITING);
    }
    if(!runtime_control_enabled||g_runtime_config.role!=ME3_ROLE_GUEST||
       !InterlockedCompareExchange(&runtime_join_attempted,0,0)||
       InterlockedCompareExchange(&runtime_join_requested,0,0)) {
        runtime_rejoin_finish(RUNTIME_START_RETRY_REFUSED,"disabled-or-conflicting-request",now);return;
    }
    CombatContextChain chain;combat_context_read_chain(base,&chain);
    unsigned char net=255;
    if(chain.info_valid&&read_at(chain.info+0x7c8,&net,1)&&net!=0) {
        runtime_rejoin_finish(RUNTIME_START_RETRY_REFUSED,"existing-network-world-preserved",now);return;
    }
    if(now-runtime_rejoin_started>=30000) {
        runtime_rejoin_finish(RUNTIME_START_RETRY_EXPIRED,"standalone-cleanup-readiness-timeout",now);return;
    }
    SessionContext context=session_context(engine);RuntimeJoinSample ready={0};
    if(net!=0||!chain.world_valid||context.world!=chain.info||
       !runtime_join_travel_idle(chain.world,engine)||!runtime_rejoin_cleanup_idle()||
       !runtime_join_context_sample(base,context,&chain,&ready))return;
    /* Rearm only startup policy; the ordinary stable-player window starts
     * from zero, and its final native admission checks run again. */
    memset(&runtime_join_readiness,0,sizeof runtime_join_readiness);
    memset(&runtime_join_progress,0,sizeof runtime_join_progress);
    runtime_join_last_check=0;
    InterlockedExchange(&runtime_join_requested,1);
    InterlockedExchange(&runtime_join_attempted,0);
    runtime_rejoin_finish(RUNTIME_START_WAITING,"standalone-retry-queued",now);
}
#endif
