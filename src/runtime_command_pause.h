#ifndef ME3_RUNTIME_COMMAND_PAUSE_H
#define ME3_RUNTIME_COMMAND_PAUSE_H
/* Use the game's command-pause policy in a verified native network world.
 * Never clear PlayersOnly, Pauser, conversations or input/animation locks. */
typedef struct RuntimeCommandPause {
    SessionHandle world,gri;
    int active,changed;
} RuntimeCommandPause;
static RuntimeCommandPause runtime_command_pause;
static int runtime_command_pause_layout(uintptr_t base,uintptr_t gri) {
    if(!session_actor_live(base,gri)||!narrative_probe_object(base,gri,"SFXGRI")||
       !runtime_power_property(base,gri,"SFXGRI","bPauseForCommand","BoolProperty",0x378,4))return 0;
    uintptr_t klass=read_ptr(gri+0x50),seen[48]={0};char text[96];
    for(unsigned n=0;klass&&n<48;++n) {
        if(!narrative_probe_object(base,klass,"Class"))return 0;
        for(unsigned j=0;j<n;++j)if(seen[j]==klass)return 0;
        seen[n]=klass;object_name(base,klass,text,sizeof text);
        if(!strcmp(text,"SFXGRI")) {
            uintptr_t field=read_ptr(klass+0x70);
            for(unsigned k=0;field&&k<1024;++k) {
                if(!is_live(base,field))return 0;
                object_name(base,field,text,sizeof text);
                if(!strcmp(text,"bPauseForCommand")) {
                    uint64_t mask=0;return read_ptr(field+0x40)==klass&&
                        read_at(field+0xd0,&mask,8)&&mask==4;
                }
                if(!read_at(field+0x68,&field,8))return 0;
            }
            return 0;
        }
        if(!read_at(klass+0x60,&klass,8))return 0;
    }
    return 0;
}
static int runtime_command_pause_same(uintptr_t base,SessionContext c,RuntimeCommandPause *lease) {
    return c.world==lease->world.pointer&&session_handle_matches(lease->world)&&session_handle_matches(lease->gri)&&
        narrative_probe_object(base,c.world,"BioWorldInfo")&&read_ptr(c.world+0x6a8)==lease->gri.pointer&&
        read_ptr(lease->gri.pointer+0x198)==c.world&&
        runtime_power_property(base,c.world,"WorldInfo","GRI","ObjectProperty",0x6a8,8)&&
        runtime_command_pause_layout(base,lease->gri.pointer);
}
static void runtime_command_pause_retire(uintptr_t base,SessionContext c) {
    RuntimeCommandPause *lease=&runtime_command_pause;
    if(lease->active&&lease->changed&&runtime_command_pause_same(base,c,lease)) {
        uintptr_t at=lease->gri.pointer+0x378;uint32_t actual=0;
        if(!(at&3)&&background_focus_writable(at,4)&&read_at(at,&actual,4)&&!(actual&4))
            InterlockedCompareExchange((LONG volatile*)at,(LONG)(actual|4),(LONG)actual);
    }
    memset(lease,0,sizeof *lease);
}
static void runtime_command_pause_prepare(uintptr_t base,uintptr_t engine,int enabled) {
    if(GetCurrentThreadId()!=session_game_thread)return;
    RuntimeCommandPause *lease=&runtime_command_pause;SessionContext c=session_context(engine);
    if(lease->active&&(!enabled||!runtime_command_pause_same(base,c,lease)))
        runtime_command_pause_retire(base,c);
    if(!enabled)return;
    if(lease->active)return;
    /* Do not strand an already-paused command menu: its stock close still
     * needs the original policy to clear its own PlayersOnly state. Begin
     * prevention only from an ordinary, unpaused frame before menu entry. */
    if(session_engine_blocked(c))return;
    CombatContextChain chain;combat_context_read_chain(base,&chain);
    unsigned char net=255;
    if(!chain.info_valid||chain.info!=c.world||!network_actor_world_pins(base)||
       !session_actor_live(base,c.pc)||read_ptr(c.pc+0x198)!=c.world||
       !read_at(c.world+0x7c8,&net,1)||net!=(g_runtime_config.role==ME3_ROLE_HOST?2:3))return;
    uintptr_t gri=read_ptr(c.world+0x6a8),at=gri+0x378;uint32_t flags=0;
    RuntimeCommandPause captured={session_capture_handle(c.world),session_capture_handle(gri),1,0};
    if(!runtime_command_pause_same(base,c,&captured)||(at&3)||!background_focus_writable(at,4)||
       !read_at(at,&flags,4))return;
    if(flags&4) {
        /* Preserve every other SFXGRI boolean. A race refuses this sample. */
        if((uint32_t)InterlockedCompareExchange((LONG volatile*)at,(LONG)(flags&~4u),(LONG)flags)!=flags)return;
        captured.changed=1;
    }
    *lease=captured;
    line("{\"runtime_command_pause\":\"disabled-for-coop\",\"forced_unpause\":false,\"gameplay_verified\":false}\r\n");
}
#endif
