#ifndef ME3_RUNTIME_NETWORK_BOOTSTRAP_H
#define ME3_RUNTIME_NETWORK_BOOTSTRAP_H
/* Explicit one-shot guest join, through the already exercised ordinary engine
 * console. A dispatch is not connection success. Host in-place listening still
 * needs its transport ABI/bind/capacity contract completed; do not reload a map
 * as an implicit substitute. */
typedef enum RuntimeNetworkStartStatus {
    RUNTIME_START_IDLE=0,RUNTIME_START_WAITING=1,RUNTIME_START_DISPATCHED=2,
    RUNTIME_START_ALREADY_NETWORKED=3,RUNTIME_START_ABI_REFUSED=4,
    RUNTIME_START_BAD_REQUEST=5,RUNTIME_START_HOST_UNAVAILABLE=6
} RuntimeNetworkStartStatus;
static volatile LONG runtime_join_requested,runtime_join_attempted,runtime_network_start_status;
static DWORD runtime_join_last_check;
static int runtime_network_start_parse(Me3RuntimeRole role,const wchar_t *value) {
    if(!value)return 0;
    if(!wcscmp(value,L"join"))return role==ME3_ROLE_GUEST?1:-1;
    if(!wcscmp(value,L"listen")&&role==ME3_ROLE_HOST)return -2;
    return -1;
}
static int runtime_request_join(void) {
    if(g_config_status!=ME3_CONFIG_READY||g_runtime_config.role!=ME3_ROLE_GUEST||
       !InterlockedCompareExchange(&g_enable_requested,0,0)||
       InterlockedCompareExchange(&runtime_join_attempted,0,0))return 0;
    if(InterlockedCompareExchange(&runtime_join_requested,1,0))return 0;
    InterlockedExchange(&runtime_network_start_status,RUNTIME_START_WAITING);return 1;
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
static void runtime_network_start_tick(uintptr_t base,uintptr_t engine,DWORD now) {
    if(!runtime_control_enabled||!InterlockedCompareExchange(&runtime_join_requested,0,0)||
       InterlockedCompareExchange(&runtime_join_attempted,0,0))return;
    if(runtime_join_last_check&&now-runtime_join_last_check<250)return;
    runtime_join_last_check=now;
    if(!network_actor_world_pins(base)||!combat_context_layout(base))return;
    SessionContext c=session_context(engine);CombatContextChain before,after;
    combat_context_read_chain(base,&before);unsigned char net=255;
    uintptr_t local_pc=0,player=read_ptr(c.pc+0x67c);
    if(!before.info_valid||c.world!=before.info||!session_actor_live(base,c.pc)||
       !combat_context_local_pc(base,engine,&local_pc)||local_pc!=c.pc||
       !is_live(base,player)||!is_kind_of(base,player,"LocalPlayer")||read_ptr(player+0x68)!=c.pc||
       read_ptr(c.pc+0x198)!=c.world||!read_at(c.world+0x7c8,&net,1))return;
    if(net!=0) {
        InterlockedExchange(&runtime_join_attempted,1);InterlockedExchange(&runtime_join_requested,0);
        InterlockedExchange(&runtime_network_start_status,RUNTIME_START_ALREADY_NETWORKED);return;
    }
    /* Wait for ordinary native control; never clear pause, scene or mode flags.
     * Current-world driver/pending travel must both be absent before the call. */
    if(session_engine_blocked(c)||!runtime_join_travel_idle(before.world,engine)||session_active)return;
    wchar_t command[64];
    if(!runtime_join_command(&g_runtime_config,command,64)||!runtime_join_console_abi(base,c.pc)) {
        InterlockedExchange(&runtime_join_attempted,1);InterlockedExchange(&runtime_join_requested,0);
        InterlockedExchange(&runtime_network_start_status,RUNTIME_START_ABI_REFUSED);return;
    }
    SessionHandle controller=session_capture_handle(c.pc);
    combat_context_read_chain(base,&after);
    if(!session_handle_matches(controller)||!combat_context_chain_equal(&before,&after)||local_controller(engine)!=c.pc)return;
    /* Mark before invoking: native travel can invalidate the captured PC/world.
     * Nothing from that old context is dereferenced after the engine call. */
    InterlockedExchange(&runtime_join_attempted,1);InterlockedExchange(&runtime_join_requested,0);
    ConsoleParms params={0};params.command.data=command;params.command.count=params.command.max=(int32_t)wcslen(command)+1;
    params.write_log=1;int dispatched=invoke(base,c.pc,"PlayerController","ConsoleCommand",&params);
    if(params.result.data&&guest_cinematic_allocator_pins(base)) {
        typedef void (*EngineFree)(void*,uint32_t);
        ((EngineFree)(base+0xbf0d0))(params.result.data,0);
    }
    InterlockedExchange(&runtime_network_start_status,dispatched?RUNTIME_START_DISPATCHED:RUNTIME_START_ABI_REFUSED);
    char row[220];snprintf(row,sizeof row,"{\"runtime_network\":\"join-request\",\"ordinary_console_dispatched\":%s,\"connection_verified\":false,\"monotonic_ms\":%lu}\r\n",dispatched?"true":"false",now);line(row);
}
static int runtime_network_start_selftest(void) {
    Me3RuntimeConfig config={0};wchar_t command[64];
    if(runtime_network_start_parse(ME3_ROLE_GUEST,NULL)||runtime_network_start_parse(ME3_ROLE_GUEST,L"join")!=1||
       runtime_network_start_parse(ME3_ROLE_HOST,L"join")!=-1||runtime_network_start_parse(ME3_ROLE_HOST,L"listen")!=-2||
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
