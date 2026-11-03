#ifndef ME3_RUNTIME_AMBIENT_CONVERSATION_H
#define ME3_RUNTIME_AMBIENT_CONVERSATION_H
/* Keep a human squadmate possessed during proven ambient conversation lines.
 * The stock HenchmanUseInteractionPoint CanDo/Start scripts require an
 * SFXAI_Henchman controller. Retaining the existing player controller therefore
 * denies that AI-only movement/teleport path before it can take the pawn.
 * Full/unknown conversations and every other scene/pause/mode gate still yield.
 * Include after runtime_story_gate.h for the existing reflection helpers. */
static uintptr_t runtime_ambient_conversation_base;
static int runtime_ambient_conversation_policy(int count,uint32_t activating,const uint32_t *flags,uint32_t known) {
    if(count<1||count>16||!flags||(activating&1)||known!=((1u<<count)-1))return 0;
    for(int i=0;i<count;++i)
        if(!(flags[i]&0x80)||(flags[i]&0xc))return 0;
    return 1;
}
static int runtime_ambient_conversation_layout(uintptr_t base,uintptr_t controller) {
    uintptr_t owner=runtime_story_graph_class(base,controller,"BioConversationController");
    return owner&&runtime_native_hud_bool(base,owner,"m_bCurrentlyAmbient",0x358,0x80)&&
        runtime_native_hud_bool(base,owner,"m_bHasAttachedCameraTrack",0x358,4)&&
        runtime_native_hud_bool(base,owner,"m_bHasAttachedDOFTrack",0x358,8)&&
        runtime_native_hud_field(base,owner,"m_pManager","ObjectProperty",0x28c,8)&&
        runtime_native_hud_field(base,owner,"m_pConvData","ObjectProperty",0x284,8);
}
static int runtime_ambient_conversation_owner(uintptr_t base,SessionContext c) {
    if(g_runtime_config.role==ME3_ROLE_GUEST) {
        NetworkActorIdentityEntry channel={0};
        return guest_player_context_basic_owned(base,c.pc,c.pawn,c.world)&&
            guest_player_context_channel(base,c.pawn,&channel);
    }
    /* Retain an existing human lease. This must never acquire a squadmate
     * from an AI that has already started an ambient interaction. */
    return g_runtime_config.role==ME3_ROLE_HOST&&session_active&&!session_fault&&!session_releasing&&
        !session_cleanup_pending&&!session_pending_reason&&!session_travel_waiting&&
        c.pc==session_host&&c.pawn==session_shepard&&c.world==session_world&&
        session_handle_matches(session_pc_handle)&&session_handle_matches(session_pawn_handle)&&
        session_actor_live(base,session_remote)&&session_actor_live(base,session_pawn)&&
        read_ptr(session_remote+0x3bc)==session_pawn&&read_ptr(session_pawn+0x374)==session_remote&&
        session_peer_connected(base,session_remote,c.world)&&session_story_valid(base);
}
static int runtime_ambient_conversation_held(SessionContext c,uintptr_t manager,int active) {
    uintptr_t base=runtime_ambient_conversation_base;GameArray list={0},after={0};
    uintptr_t controllers[16]={0},data[16]={0};uint32_t flags[16]={0},again=0,activating=0,known=0;
    SessionHandle identities[16],resources[16];
    if(!base||!runtime_control_enabled||!runtime_scope_ready||c.mode!=0||active<1||active>16||
       !InterlockedCompareExchange(&g_enable_requested,0,0)||
       (g_runtime_config.role!=ME3_ROLE_HOST&&g_runtime_config.role!=ME3_ROLE_GUEST)||
       !session_actor_live(base,c.pc)||!is_kind_of(base,c.pc,"SFXPlayerController")||
       !session_actor_live(base,c.pawn)||!is_kind_of(base,c.pawn,"SFXPawn_PlayerParty")||
       !session_actor_live(base,c.world)||!runtime_ambient_conversation_owner(base,c)||
       read_ptr(c.pc+0x3bc)!=c.pawn||read_ptr(c.pawn+0x374)!=c.pc||
       read_ptr(c.pc+0x198)!=c.world||read_ptr(c.pawn+0x198)!=c.world||
       !narrative_probe_object(base,manager,"BioConversationManager")||read_ptr(c.world+0x948)!=manager||
       !runtime_power_property(base,c.world,"BioWorldInfo","m_pConversationManager","ObjectProperty",0x948,8)||
       !runtime_power_property(base,manager,"BioConversationManager","m_aActiveConversations","ArrayProperty",0xc8,16)||
       !runtime_power_property(base,manager,"BioConversationManager","m_bActivatingConversation","BoolProperty",0xe8,4)||
       !runtime_native_hud_bool(base,runtime_story_graph_class(base,manager,"BioConversationManager"),"m_bActivatingConversation",0xe8,1)||
       !read_at(manager+0xe8,&activating,4)||(activating&1)||!read_at(manager+0xc8,&list,sizeof list)||
       list.count!=active||list.max<active||list.max>256||!list.data||(list.data&7)||
       !read_at(list.data,controllers,(SIZE_T)active*8))return 0;
    SessionHandle world=session_capture_handle(c.world),pc=session_capture_handle(c.pc),pawn=session_capture_handle(c.pawn);
    SessionHandle owner=session_capture_handle(manager);
    if(!session_handle_matches(world)||!session_handle_matches(pc)||!session_handle_matches(pawn)||!session_handle_matches(owner))return 0;
    for(int i=0;i<active;++i) {
        uintptr_t current=controllers[i];
        for(int j=0;j<i;++j)if(controllers[j]==current)return 0;
        if(!narrative_probe_object(base,current,"BioConversationController")||
           read_ptr(current+0x28c)!=manager||!runtime_ambient_conversation_layout(base,current)||
           !read_at(current+0x358,&flags[i],4))return 0;
        data[i]=read_ptr(current+0x284);
        if(!narrative_probe_object(base,data[i],"BioConversation"))return 0;
        identities[i]=session_capture_handle(current);resources[i]=session_capture_handle(data[i]);
        if(!session_handle_matches(identities[i])||!session_handle_matches(resources[i]))return 0;
        known|=1u<<i;
    }
    if(!runtime_ambient_conversation_policy(active,activating,flags,known)||
       !read_at(manager+0xc8,&after,sizeof after)||memcmp(&list,&after,sizeof list)||
       !read_at(manager+0xe8,&again,4)||again!=activating||read_ptr(c.world+0x948)!=manager||
       !session_handle_matches(world)||!session_handle_matches(pc)||!session_handle_matches(pawn)||!session_handle_matches(owner)||
       read_ptr(c.pc+0x3bc)!=c.pawn||read_ptr(c.pawn+0x374)!=c.pc||
       read_ptr(c.pc+0x198)!=c.world||read_ptr(c.pawn+0x198)!=c.world||!runtime_ambient_conversation_owner(base,c))return 0;
    for(int i=0;i<active;++i)
        if(read_ptr(list.data+(SIZE_T)i*8)!=controllers[i]||!session_handle_matches(identities[i])||
           !session_handle_matches(resources[i])||read_ptr(controllers[i]+0x28c)!=manager||
           read_ptr(controllers[i]+0x284)!=data[i]||!read_at(controllers[i]+0x358,&again,4)||again!=flags[i])return 0;
    static SessionHandle last;static unsigned reported;
    if(reported<32&&(last.pointer!=identities[0].pointer||last.index!=identities[0].index||last.name!=identities[0].name)) {
        last=identities[0];++reported;
        line("{\"runtime_ambient_conversation\":\"existing-human-control-retained\",\"camera_attached\":false,\"actors_moved\":false,\"gameplay_verified\":false}\r\n");
    }
    return 1;
}
static void runtime_ambient_conversation_configure(uintptr_t base) {
    runtime_ambient_conversation_base=base;
    session_ambient_conversation_held=runtime_ambient_conversation_held;
}
#ifdef ME3_RUNTIME_FIXTURE
static int runtime_ambient_fixture_proof(SessionContext c,uintptr_t manager,int count) {(void)c;return manager&&count==1;}
static int runtime_ambient_conversation_selftest(void) {
    uint32_t flags[16]={0x80,0x80};
    if(!runtime_ambient_conversation_policy(1,0,flags,1)||!runtime_ambient_conversation_policy(2,0,flags,3)||
       runtime_ambient_conversation_policy(2,0,flags,1)||runtime_ambient_conversation_policy(1,1,flags,1)||
       runtime_ambient_conversation_policy(0,0,flags,0)||runtime_ambient_conversation_policy(17,0,flags,0xffff)||
       runtime_ambient_conversation_policy(1,0,NULL,1))return 0;
    flags[1]=0;
    if(runtime_ambient_conversation_policy(2,0,flags,3))return 0;
    flags[1]=0x84;
    if(runtime_ambient_conversation_policy(2,0,flags,3))return 0;
    flags[1]=0x88;
    if(runtime_ambient_conversation_policy(2,0,flags,3))return 0;
    for(int i=0;i<16;++i)flags[i]=0x80;
    if(!runtime_ambient_conversation_policy(16,0,flags,0xffff))return 0;
    /* Even a positive ambient proof cannot bypass the original environmental
     * and full-scene gates. The probe's absent callback keeps its old policy. */
    unsigned char pc[0x800]={0},world[0x960]={0},manager[16]={0},conversation[0xf0]={0};
    SessionContext c={(uintptr_t)pc,0,(uintptr_t)world,0,(uintptr_t)manager,0};
    uintptr_t conversation_pointer=(uintptr_t)conversation;int count=1;uint32_t word=0;
    memcpy(world+0x948,&conversation_pointer,8);memcpy(conversation+0xd0,&count,4);
    int (*saved)(SessionContext,uintptr_t,int)=session_ambient_conversation_held;
    session_ambient_conversation_held=NULL;int ok=session_environment_block_reason(c)!=NULL;
    session_ambient_conversation_held=runtime_ambient_fixture_proof;
    ok&=!session_environment_block_reason(c);
    word=1;memcpy(conversation+0xe8,&word,4);ok&=session_environment_block_reason(c)!=NULL;
    word=0;memcpy(conversation+0xe8,&word,4);c.mode=8;ok&=session_engine_blocked(c);c.mode=0;
    word=0x20000;memcpy(pc+0x77c,&word,4);ok&=session_engine_blocked(c);
    word=0;memcpy(pc+0x77c,&word,4);word=0x400;memcpy(world+0x7c0,&word,4);ok&=session_engine_blocked(c);
    word=0;memcpy(world+0x7c0,&word,4);count=2;memcpy(conversation+0xd0,&count,4);ok&=session_engine_blocked(c);
    session_ambient_conversation_held=saved;return ok;
}
#endif
#endif
