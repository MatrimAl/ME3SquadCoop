#ifndef ME3_RUNTIME_RELISTEN_H
#define ME3_RUNTIME_RELISTEN_H
/* Explicit current-mission listener request after a previously verified
 * runtime-owned listener world AND driver have been natively retired. */
static int runtime_relisten_previous_gone(uintptr_t base) {
    GameArray objects={0},again={0};
    if(!read_at(base+OBJECTS_RVA,&objects,sizeof objects)||!objects.data||objects.count<=0||
       objects.count>2000000||objects.max<objects.count||objects.max>4000000)return 0;
    return session_registry_retired(base,runtime_listen_origin_world,&objects)&&
        session_registry_retired(base,runtime_listen_origin_driver,&objects)&&
        read_at(base+OBJECTS_RVA,&again,sizeof again)&&!memcmp(&objects,&again,sizeof objects)&&
        session_registry_retired(base,runtime_listen_origin_world,&again)&&
        session_registry_retired(base,runtime_listen_origin_driver,&again);
}
static int runtime_relisten_target_valid(uintptr_t base,const CombatContextChain *chain) {
    return chain->world_valid&&chain->info_valid&&chain->world==runtime_relisten_target.pointer&&
        session_handle_matches(runtime_relisten_target)&&narrative_probe_object(base,chain->world,"World");
}
static void runtime_relisten_finish(int status,const char *reason,DWORD now) {
    char row[350];snprintf(row,sizeof row,
        "{\"runtime_network\":\"relisten-request\",\"reason\":\"%s\",\"status\":%d,\"implicit_travel\":false,\"connection_verified\":false,\"monotonic_ms\":%lu}\r\n",
        reason,status,now);line(row);
    runtime_relisten_started=0;InterlockedExchange(&runtime_network_start_status,status);
    memset(&runtime_auto_relisten_target,0,sizeof runtime_auto_relisten_target);
    InterlockedExchange(&runtime_relisten_requested,0);
}
static void runtime_relisten_tick(uintptr_t base,uintptr_t engine,DWORD now) {
    if(!InterlockedCompareExchange(&runtime_relisten_requested,0,0))return;
    if(!runtime_relisten_started) {
        runtime_relisten_started=now;
        InterlockedExchange(&runtime_network_start_status,RUNTIME_START_RELISTEN_WAITING);
    }
    if(!runtime_control_enabled||g_runtime_config.role!=ME3_ROLE_HOST||
       !InterlockedCompareExchange(&runtime_listen_successful,0,0)||
       InterlockedCompareExchange(&runtime_listen_requested,0,0)) {
        runtime_relisten_finish(RUNTIME_START_RELISTEN_REFUSED,"disabled-or-conflicting-request",now);return;
    }
    if(now-runtime_relisten_started>=30000) {
        runtime_relisten_finish(RUNTIME_START_RELISTEN_EXPIRED,"new-mission-readiness-timeout",now);return;
    }
    CombatContextChain chain;combat_context_read_chain(base,&chain);unsigned char net=255;
    if(runtime_auto_relisten_target.pointer&&
       (!chain.world_valid||chain.world!=runtime_auto_relisten_target.pointer||
        !session_handle_matches(runtime_auto_relisten_target))) {
        runtime_relisten_finish(RUNTIME_START_RELISTEN_REFUSED,"automatic-target-changed",now);return;
    }
    if(chain.info_valid&&read_at(chain.info+0x7c8,&net,1)&&net!=0) {
        runtime_relisten_finish(RUNTIME_START_RELISTEN_REFUSED,"existing-network-world-preserved",now);return;
    }
    if(net!=0||!chain.world_valid||chain.world==runtime_listen_origin_world.pointer||
       !runtime_relisten_previous_gone(base)||!runtime_rejoin_cleanup_idle()||runtime_host_capacity.active||
       !runtime_join_travel_idle(chain.world,engine))return;
    SessionContext c=session_context(engine);
    if(c.world!=chain.info||!runtime_host_story_idle(base,c))return;
    runtime_relisten_target=session_capture_handle(chain.world);
    if(!runtime_relisten_target_valid(base,&chain))return;
    runtime_relisten_armed=1;runtime_join_last_check=0;
    InterlockedExchange(&runtime_host_start_gate,0);
    InterlockedExchange(&runtime_listen_requested,1);InterlockedExchange(&runtime_listen_attempted,0);
    runtime_relisten_finish(RUNTIME_START_WAITING,"new-mission-listen-queued",now);
}
#endif
