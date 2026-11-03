#ifndef ME3_RUNTIME_INPUT_BOOTSTRAP_H
#define ME3_RUNTIME_INPUT_BOOTSTRAP_H
/* Optional ordinary-entry initialization of a genuinely empty local input.
 * Use the profile-aware native path; never synthesize bindings or unlock a
 * scene. A dispatch is not proof that gameplay input became functional. */
typedef struct RuntimeInputScope {
    SessionHandle pc,pawn,world,input,manager,pri,profile,method;
    SessionHandle channel,connection,driver;
    unsigned long registration_token;
    GameArray bindings,settings;
    uint64_t profile_hash;
} RuntimeInputScope;
static int runtime_input_bootstrap_enabled(void) {
    static int configured,opted;
    if(!configured) {
        wchar_t value[2]={0};int present=0;
        opted=direct_ip_native_read_environment(L"ME3_SQUAD_COOP_INPUT_BOOTSTRAP",value,2,&present)&&present&&!wcscmp(value,L"1");
        configured=1;
    }
    return opted;
}
static int runtime_input_scope_same(const RuntimeInputScope *a,const RuntimeInputScope *b) {
    return runtime_power_same_handle(a->pc,b->pc)&&runtime_power_same_handle(a->pawn,b->pawn)&&
        runtime_power_same_handle(a->world,b->world)&&runtime_power_same_handle(a->input,b->input)&&
        runtime_power_same_handle(a->manager,b->manager)&&runtime_power_same_handle(a->pri,b->pri)&&
        runtime_power_same_handle(a->profile,b->profile)&&runtime_power_same_handle(a->method,b->method)&&
        runtime_power_same_handle(a->channel,b->channel)&&runtime_power_same_handle(a->connection,b->connection)&&
        runtime_power_same_handle(a->driver,b->driver)&&a->registration_token==b->registration_token&&
        !memcmp(&a->bindings,&b->bindings,sizeof a->bindings)&&!memcmp(&a->settings,&b->settings,sizeof a->settings)&&
        a->profile_hash==b->profile_hash;
}
static int runtime_input_scope_read(uintptr_t base,uintptr_t engine,RuntimeInputScope *out) {
    SessionContext c=session_context(engine);NetworkActorIdentityEntry channel={0};CombatContextChain chain;
    uint32_t flags=0;uintptr_t pauser=1;unsigned char ignored=1;
    if(!runtime_control_enabled||!runtime_scope_ready||session_engine_blocked(c)||
       !guest_player_context_registered_for(c.pc,c.pawn)||
       !guest_player_context_conditions(base,c.pc,c.pawn,c.world,1,1,1,&channel,&chain)||
       !read_at(c.pc+0x782,&ignored,1)||ignored||
       !read_at(c.world+0x7c0,&flags,4)||(flags&0xc00)||!read_at(c.world+0x670,&pauser,8)||pauser)return 0;
    uintptr_t input=read_ptr(c.pc+0x6dc),manager=read_ptr(c.pc+0xc38),pri=read_ptr(c.pc+0x3c4),profile=read_ptr(c.pc+0xb68);
    if(!narrative_probe_object(base,input,"BioPlayerInput")||read_ptr(input+0x40)!=c.pc||
       !session_actor_live(base,pri)||!is_kind_of(base,pri,"SFXPRI")||read_ptr(pri+0x198)!=c.world||
       !narrative_probe_object(base,profile,"SFXProfileSettings")||
       !runtime_power_property(base,c.pc,"PlayerController","PlayerInput","ObjectProperty",0x6dc,8)||
       !runtime_power_property(base,c.pc,"Controller","PlayerReplicationInfo","ObjectProperty",0x3c4,8)||
       !runtime_power_property(base,c.pc,"BioPlayerController","ProfileSettings","ObjectProperty",0xb68,8)||
       !runtime_power_property(base,c.pc,"BioPlayerController","GameModeManager2","ObjectProperty",0xc38,8)||
       !runtime_power_property(base,input,"Input","Bindings","ArrayProperty",0xb0,16)||
       !runtime_power_property(base,profile,"OnlinePlayerStorage","ProfileSettings","ArrayProperty",0x60,16))return 0;
    memset(out,0,sizeof *out);
    if(!read_at(input+0xb0,&out->bindings,sizeof out->bindings)||out->bindings.count||
       out->bindings.max<0||out->bindings.max>8192||(out->bindings.max&&!out->bindings.data)||
       !read_at(profile+0x60,&out->settings,sizeof out->settings)||out->settings.count<1||
       out->settings.count>4096||out->settings.max<out->settings.count||out->settings.max>8192||!out->settings.data)return 0;
    /* Native loader walks 0x1c-byte profile rows without validating its
     * profile pointer. Prove the bounded rows readable before dispatch. */
    unsigned char row[0x1c];
    if(out->settings.data>UINTPTR_MAX-(size_t)out->settings.count*sizeof row)return 0;
    out->profile_hash=UINT64_C(14695981039346656037);
    for(int i=0;i<out->settings.count;++i) {
        if(!read_at(out->settings.data+(size_t)i*sizeof row,row,sizeof row))return 0;
        for(unsigned j=0;j<sizeof row;++j)out->profile_hash=(out->profile_hash^row[j])*UINT64_C(1099511628211);
    }
    uintptr_t method=guest_player_context_find_method(base,read_ptr(c.pc+0x50),"BioPlayerController","UpdateInputConfiguration");
    GameArray script={0};
    if(!guest_player_context_function_abi(base,method,"BioPlayerController","UpdateInputConfiguration",0x20002,0,65535,0,0xf52b0,0)||
       !read_at(method+0x7c,&script,sizeof script)||script.count!=374||script.max<374||script.max>4096||
       read_ptr(read_ptr(c.pc)+0xaf8)!=base+0xaa00b0||read_ptr(read_ptr(c.pc)+0x230)!=base+0x40d450)return 0;
    out->pc=session_capture_handle(c.pc);out->pawn=session_capture_handle(c.pawn);out->world=session_capture_handle(c.world);
    out->input=session_capture_handle(input);out->manager=session_capture_handle(manager);out->pri=session_capture_handle(pri);
    out->profile=session_capture_handle(profile);out->method=session_capture_handle(method);
    out->channel=session_capture_handle(channel.channel);out->connection=session_capture_handle(channel.connection);
    out->driver=session_capture_handle(channel.driver);out->registration_token=guest_player_context_registration_token();
    return session_handle_matches(out->pc)&&session_handle_matches(out->pawn)&&session_handle_matches(out->world)&&
        session_handle_matches(out->input)&&session_handle_matches(out->manager)&&session_handle_matches(out->pri)&&
        session_handle_matches(out->profile)&&session_handle_matches(out->method)&&
        session_handle_matches(out->channel)&&session_handle_matches(out->connection)&&session_handle_matches(out->driver)&&
        read_ptr(c.pc+0x6dc)==input&&read_ptr(c.pc+0xc38)==manager&&read_ptr(c.pc+0x3c4)==pri&&read_ptr(c.pc+0xb68)==profile;
}
static void runtime_input_bootstrap_tick(uintptr_t base,uintptr_t engine,DWORD now) {
    static RuntimeInputScope candidate;
    /* No eviction: a scene or temporary scope loss cannot rearm an old input.
     * Exhausting the process-wide bound disables further automatic attempts. */
    static RuntimeInputScope attempted[128];static unsigned attempts,samples;
    static DWORD sampled_at,first_at;
    if(!runtime_input_bootstrap_enabled()||g_runtime_config.role!=ME3_ROLE_GUEST)return;
    if(sampled_at&&now-sampled_at<250)return;
    DWORD gap=sampled_at?now-sampled_at:0;sampled_at=now;
    RuntimeInputScope current={0};
    if(!runtime_input_scope_read(base,engine,&current)){samples=0;return;}
    for(unsigned i=0;i<attempts;++i)
        if(runtime_power_same_handle(attempted[i].pc,current.pc)&&
           runtime_power_same_handle(attempted[i].input,current.input)&&
           runtime_power_same_handle(attempted[i].manager,current.manager))return;
    if(attempts>=128)return;
    if(!samples||gap>1000||!runtime_input_scope_same(&candidate,&current)) {
        candidate=current;samples=1;first_at=now;return;
    }
    if(samples<4)++samples;
    if(samples<4||now-first_at<750)return;
    RuntimeInputScope checked={0};
    if(!runtime_input_scope_read(base,engine,&checked)||!runtime_input_scope_same(&current,&checked)){samples=0;return;}
    /* Consume before native execution, including a no-op or partial result. */
    attempted[attempts++]=checked;samples=0;
    ((ProcessEvent)(base+0x40d450))((void*)checked.pc.pointer,(void*)checked.method.pointer,NULL,NULL);
    GameArray bindings={0};int valid=session_handle_matches(checked.pc)&&session_handle_matches(checked.input)&&
        read_ptr(checked.pc.pointer+0x6dc)==checked.input.pointer&&
        read_at(checked.input.pointer+0xb0,&bindings,sizeof bindings)&&bindings.count>=0&&bindings.count<=8192&&
        bindings.max>=bindings.count&&bindings.max<=8192&&(!bindings.count||bindings.data);
    char log[360];snprintf(log,sizeof log,"{\"runtime_input_bootstrap\":\"native-profile-update-dispatched\",\"post_binding_array_valid\":%s,\"post_binding_count\":%d,\"input_delivery_verified\":false,\"monotonic_ms\":%lu}\r\n",valid?"true":"false",valid?bindings.count:-1,now);line(log);
}
#endif
