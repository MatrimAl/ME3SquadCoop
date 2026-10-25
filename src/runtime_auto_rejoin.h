#ifndef ME3_RUNTIME_AUTO_REJOIN_H
#define ME3_RUNTIME_AUTO_REJOIN_H
/* Optional one-shot recovery after an observed owned Ashley client graph is
 * disposed. Never disconnect, choose a mission or retry a failed initial join. */
static SessionHandle runtime_auto_rejoin_origin_world,runtime_auto_rejoin_origin_driver;
static unsigned runtime_auto_rejoin_epoch,runtime_auto_rejoin_attempted_epoch;
static int runtime_auto_rejoin_client_scope(uintptr_t base,uintptr_t engine,
    const CombatContextChain *chain,SessionContext c) {
    uintptr_t local=0,driver=read_ptr(chain->world+0x120),connection=read_ptr(driver+0x80);
    NetworkActorIdentityEntry channel={0};
    return network_actor_world_pins(base)&&combat_context_layout(base)&&c.world==chain->info&&
        combat_context_local_pc(base,engine,&local)&&local==c.pc&&
        is_live(base,driver)&&is_kind_of(base,driver,"NetDriver")&&
        is_live(base,connection)&&is_kind_of(base,connection,"NetConnection")&&
        read_ptr(connection+0x8c)==driver&&runtime_transport_endpoint(base,driver,&g_runtime_config)&&
        guest_player_context_registered_for(c.pc,c.pawn)&&
        guest_player_context_basic_owned(base,c.pc,c.pawn,c.world)&&
        guest_player_context_channel(base,c.pawn,&channel)&&channel.driver==driver&&channel.connection==connection;
}
static int runtime_auto_rejoin_old_gone(uintptr_t base) {
    GameArray objects={0},again={0};
    if(!runtime_auto_rejoin_epoch||!read_at(base+OBJECTS_RVA,&objects,sizeof objects)||
       !objects.data||objects.count<=0||objects.count>2000000||objects.max<objects.count||objects.max>4000000)return 0;
    return session_registry_retired(base,runtime_auto_rejoin_origin_world,&objects)&&
        session_registry_retired(base,runtime_auto_rejoin_origin_driver,&objects)&&
        read_at(base+OBJECTS_RVA,&again,sizeof again)&&!memcmp(&objects,&again,sizeof objects)&&
        session_registry_retired(base,runtime_auto_rejoin_origin_world,&again)&&
        session_registry_retired(base,runtime_auto_rejoin_origin_driver,&again);
}
static void runtime_auto_rejoin_tick(uintptr_t base,uintptr_t engine,DWORD now) {
    static int configured,opted;static DWORD checked;
    if(!configured) {
        wchar_t value[8]={0};DWORD length=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_AUTO_REJOIN",value,8);
        opted=length==1&&value[0]==L'1';configured=1;
    }
    if(!opted||runtime_auto_rejoin_suspended||!runtime_control_enabled||
       g_runtime_config.role!=ME3_ROLE_GUEST||g_config_status!=ME3_CONFIG_READY||
       !InterlockedCompareExchange(&runtime_join_attempted,0,0)||
       InterlockedCompareExchange(&runtime_join_requested,0,0)||
       InterlockedCompareExchange(&runtime_rejoin_requested,0,0))return;
    if(checked&&now-checked<500)return;checked=now;
    CombatContextChain chain,again;combat_context_read_chain(base,&chain);unsigned char net=255;
    if(!chain.world_valid||!chain.info_valid||!read_at(chain.info+0x7c8,&net,1))return;
    SessionContext c=session_context(engine);
    if(net==3) {
        if(!runtime_auto_rejoin_client_scope(base,engine,&chain,c))return;
        uintptr_t driver=read_ptr(chain.world+0x120);
        SessionHandle world=session_capture_handle(chain.world),network=session_capture_handle(driver);
        combat_context_read_chain(base,&again);
        if(!combat_context_chain_equal(&chain,&again)||read_ptr(chain.world+0x120)!=driver||
           !session_handle_matches(world)||!session_handle_matches(network))return;
        if(runtime_auto_rejoin_epoch&&
           world.pointer==runtime_auto_rejoin_origin_world.pointer&&world.index==runtime_auto_rejoin_origin_world.index&&
           world.name==runtime_auto_rejoin_origin_world.name&&network.pointer==runtime_auto_rejoin_origin_driver.pointer&&
           network.index==runtime_auto_rejoin_origin_driver.index&&network.name==runtime_auto_rejoin_origin_driver.name)return;
        if(runtime_auto_rejoin_epoch==UINT32_MAX)return; /* Never reuse an origin epoch. */
        runtime_auto_rejoin_origin_world=world;runtime_auto_rejoin_origin_driver=network;++runtime_auto_rejoin_epoch;
        line("{\"runtime_network\":\"automatic-rejoin-origin-observed\",\"owned_registered_guest\":true,\"campaign_verified\":false}\r\n");return;
    }
    RuntimeJoinSample ready={0};
    if(net!=0||!runtime_auto_rejoin_epoch||runtime_auto_rejoin_attempted_epoch==runtime_auto_rejoin_epoch||
       (runtime_peer_ready_required&&!runtime_peer_ready)||
       chain.world==runtime_auto_rejoin_origin_world.pointer||!runtime_auto_rejoin_old_gone(base)||
       c.world!=chain.info||!runtime_rejoin_cleanup_idle()||!runtime_join_travel_idle(chain.world,engine)||
       !runtime_join_context_sample(base,c,&chain,&ready))return;
    SessionHandle target=session_capture_handle(chain.world);combat_context_read_chain(base,&again);
    if(!combat_context_chain_equal(&chain,&again)||!session_handle_matches(target)||
       !narrative_probe_object(base,chain.world,"World")||!runtime_request_rejoin())return;
    runtime_auto_rejoin_attempted_epoch=runtime_auto_rejoin_epoch;runtime_auto_rejoin_target=target;
    line("{\"runtime_network\":\"automatic-rejoin-queued\",\"native_disconnect_requested\":false,\"connection_verified\":false}\r\n");
}
#endif
