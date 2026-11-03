#ifndef ME3_RUNTIME_POWER_WHEEL_DISCOVERY_H
#define ME3_RUNTIME_POWER_WHEEL_DISCOVERY_H
/* Read the existing native GUI registry, never call GetMovie or load a movie.
 * GetMovie searches manager arrays+4c/+bc, then each record's movie+5c.
 * Only the already registered guest's exact PC wheel can be observed here. */
static int runtime_power_wheel_registry_pins(uintptr_t base) {
    static const unsigned char getter[]={0x48,0x8b,0x05,0x79,0x73,0xc9,0x00,0xc3};
    static const unsigned char primary[]={0x49,0x8b,0x47,0x4c,0x49,0x8b,0x0c,0x06,0x48,0x8b,0x79,0x5c};
    static const unsigned char secondary[]={0x49,0x8b,0x87,0xbc,0,0,0,0x48,0x8b,0x0c,0x28,0x48,0x8b,0x79,0x5c};
    unsigned char bytes[sizeof secondary];
    return read_at(base+0xc1f860,bytes,sizeof getter)&&!memcmp(bytes,getter,sizeof getter)&&
        read_at(base+0xc1efe0,bytes,sizeof primary)&&!memcmp(bytes,primary,sizeof primary)&&
        read_at(base+0xc1f090,bytes,sizeof secondary)&&!memcmp(bytes,secondary,sizeof secondary);
}
static void runtime_power_wheel_discover(uintptr_t base,SessionContext c,int ordinary) {
    static ULONGLONG last_scan;
    if(!ordinary||c.mode||GetCurrentThreadId()!=session_game_thread||
       !runtime_power_icons_enabled()||!runtime_native_power_input_active()||
       !runtime_control_enabled||!runtime_scope_ready||g_runtime_config.role!=ME3_ROLE_GUEST||
       !InterlockedCompareExchange(&g_enable_requested,0,0)||
       !guest_player_context_registered_for(c.pc,c.pawn)||
       !guest_player_context_basic_owned(base,c.pc,c.pawn,c.world))return;
    ULONGLONG now=GetTickCount64();
    if(last_scan&&now>=last_scan&&now-last_scan<250)return;
    last_scan=now;
    if(!runtime_power_wheel_registry_pins(base))return;
    uintptr_t manager=read_ptr(base+0x18b6be0);
    GameArray lists[2]={0},after={0};
    uintptr_t records[2][128]={{0}},movies[2][128]={{0}};
    RuntimeHotkeyWheel owner={0};
    owner.pc=session_capture_handle(c.pc);owner.pawn=session_capture_handle(c.pawn);
    owner.world=session_capture_handle(c.world);owner.token=guest_player_context_registration_token();
    if(!manager||!owner.token||!session_handle_matches(owner.pc)||
       !session_handle_matches(owner.pawn)||!session_handle_matches(owner.world))return;
    for(unsigned group=0;group<2;++group) {
        uintptr_t at=manager+(group?0xbc:0x4c);
        if(!scene_readiness_array(at,&lists[group],128)||
           (lists[group].count&&!read_at(lists[group].data,records[group],(SIZE_T)lists[group].count*8)))return;
        for(int i=0;i<lists[group].count;++i) {
            if(!records[group][i]||!read_at(records[group][i]+0x5c,&movies[group][i],8))return;
            uintptr_t movie=movies[group][i];
            if(!movie||read_ptr(movie)!=base+0x14f6700||
               read_ptr(movie+0x6f8)!=c.pc||read_ptr(movie+0x2bc)!=c.world)continue;
            /* Multiple distinct matching wheels are ambiguous. Do not take
             * whichever object happens to occur first in a native list. */
            if(owner.handler.pointer&&owner.handler.pointer!=movie)return;
            owner.handler=session_capture_handle(movie);
            if(!runtime_power_icon_owner(base,&owner,c))return;
            for(unsigned slot=0;slot<8;++slot) {
                SessionHandle icon=session_capture_handle(read_ptr(movie+0xbd8+slot*8));
                if(!runtime_power_icon_layout(base,&owner,slot,icon))return;
            }
        }
    }
    if(!owner.handler.pointer)return;
    /* Read-only inspection is still scoped to one registry/ownership epoch.
     * Recheck headers, records and movie links before publishing mod state. */
    for(unsigned group=0;group<2;++group) {
        if(!read_at(manager+(group?0xbc:0x4c),&after,sizeof after)||
           memcmp(&lists[group],&after,sizeof after))return;
        for(int i=0;i<lists[group].count;++i) {
            uintptr_t record=0,movie=0;
            if(!read_at(lists[group].data+(SIZE_T)i*8,&record,8)||record!=records[group][i]||
               !read_at(record+0x5c,&movie,8)||movie!=movies[group][i])return;
        }
    }
    if(read_ptr(base+0x18b6be0)!=manager||
       owner.token!=guest_player_context_registration_token()||
       !session_handle_matches(owner.pawn)||!runtime_power_icon_owner(base,&owner,c)||
       !guest_player_context_registered_for(c.pc,c.pawn)||
       !guest_player_context_basic_owned(base,c.pc,c.pawn,c.world))return;
    owner.observed=now;owner.registry_observation=1;runtime_hotkey_wheel=owner;
}
#endif
