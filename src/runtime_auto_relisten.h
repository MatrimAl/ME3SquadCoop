#ifndef ME3_RUNTIME_AUTO_RELISTEN_H
#define ME3_RUNTIME_AUTO_RELISTEN_H
/* Optional host continuation policy, not native travel or guest reconnect.
 * Schedule at most one request after each successful runtime listener. */
static void runtime_auto_relisten_tick(uintptr_t base,uintptr_t engine,DWORD now) {
    static int configured,opted;
    static DWORD checked;
    static SessionHandle attempted_origin;
    if(!configured) {
        wchar_t value[8]={0};DWORD length=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_AUTO_RELISTEN",value,8);
        opted=length==1&&value[0]==L'1';configured=1;
    }
    runtime_auto_relisten_enabled=opted&&g_runtime_config.role==ME3_ROLE_HOST;
    if(!opted||runtime_auto_relisten_suspended||!runtime_control_enabled||
       g_runtime_config.role!=ME3_ROLE_HOST||g_config_status!=ME3_CONFIG_READY||
       !InterlockedCompareExchange(&runtime_listen_successful,0,0)||
       !InterlockedCompareExchange(&runtime_listen_attempted,0,0)||
       InterlockedCompareExchange(&runtime_listen_requested,0,0)||
       InterlockedCompareExchange(&runtime_relisten_requested,0,0))return;
    /* Compare captured values, not liveness: this origin must be disposed.
     * A failed candidate cannot trigger a loop through other temporary roots. */
    if(attempted_origin.pointer==runtime_listen_origin_world.pointer&&
       attempted_origin.index==runtime_listen_origin_world.index&&
       attempted_origin.name==runtime_listen_origin_world.name)return;
    if(checked&&now-checked<500)return;
    checked=now;
    CombatContextChain chain,again;combat_context_read_chain(base,&chain);unsigned char net=255;
    if(!chain.world_valid||!chain.info_valid||chain.world==runtime_listen_origin_world.pointer||
       !read_at(chain.info+0x7c8,&net,1)||net!=0||!runtime_relisten_previous_gone(base)||
       !runtime_rejoin_cleanup_idle()||runtime_host_capacity.active||
       !runtime_join_travel_idle(chain.world,engine))return;
    SessionContext c=session_context(engine);
    if(c.world!=chain.info||!runtime_host_story_idle(base,c))return;
    SessionHandle target=session_capture_handle(chain.world);
    combat_context_read_chain(base,&again);
    if(!combat_context_chain_equal(&chain,&again)||!session_handle_matches(target)||
       !narrative_probe_object(base,chain.world,"World"))return;
    if(!runtime_request_relisten())return;
    attempted_origin=runtime_listen_origin_world;runtime_auto_relisten_target=target;
    line("{\"runtime_network\":\"automatic-new-mission-listen-queued\",\"native_travel_requested\":false,\"guest_reconnected\":false}\r\n");
}
#endif
