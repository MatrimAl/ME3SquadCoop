#ifndef ME3_RUNTIME_NATIVE_HUD_H
#define ME3_RUNTIME_NATIVE_HUD_H
/* Bind only the existing PC Scaleform HUD's player display source. The two
 * original native selectors still run. Their following native health/shield,
 * weapon and animation updates render the stock Shepard UI for owned Ashley.
 * Never change a squad/world cache, movie, gameplay permission or pawn class. */
typedef void (*RuntimeHudSelector)(void*);
typedef struct RuntimeNativeHud {
    SessionHandle handler,pc,pawn,world,baseline;
    uintptr_t base;
    RuntimeHudSelector original[2];
    DWORD scanned,observed;
    int installed,active,changed,busy,reported;
} RuntimeNativeHud;
static RuntimeNativeHud runtime_native_hud;
static int runtime_native_hud_enabled(void) {
    static int enabled=-1;
    if(enabled<0){wchar_t value[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_NATIVE_HUD",value,8);
        enabled=!n||(n==1&&value[0]==L'1');}
    return enabled;
}
static uintptr_t runtime_native_hud_field(uintptr_t base,uintptr_t owner,const char *name,const char *kind,int offset,int size) {
    uintptr_t field=read_ptr(owner+0x70);char text[96];
    for(unsigned n=0;field&&n<1024;++n) {
        if(!is_live(base,field)||read_ptr(field+0x40)!=owner)return 0;
        object_name(base,field,text,sizeof text);
        if(!strcmp(text,name)) {
            int got=-1,bytes=0,dim=0;
            return narrative_probe_object(base,field,kind)&&read_at(field+0x94,&got,4)&&got==offset&&
                read_at(field+0x74,&bytes,4)&&bytes==size&&read_at(field+0x70,&dim,4)&&dim==1?field:0;
        }
        if(!read_at(field+0x68,&field,8))return 0;
    }
    return 0;
}
static int runtime_native_hud_layout(uintptr_t base,uintptr_t handler) {
    uintptr_t klass=read_ptr(handler+0x50),owner=0;char name[96];
    if(!narrative_probe_object(base,handler,"SFXSFHandler_PCHUD")||read_ptr(handler)!=base+0x14f58b0)return 0;
    for(unsigned n=0;klass&&n<48;++n) {
        if(!narrative_probe_object(base,klass,"Class"))return 0;
        object_name(base,klass,name,sizeof name);
        if(!strcmp(name,"SFXSFHandler_HUD")){owner=klass;break;}
        klass=read_ptr(klass+0x60);
    }
    uintptr_t info=runtime_native_hud_field(base,owner,"m_oShepardInfo","StructProperty",0x334,0xc0);
    uintptr_t type=read_ptr(info+0xd0),target=read_ptr(runtime_native_hud_field(base,type,"pPawn","ObjectProperty",0x98,8)+0xd0);
    uintptr_t invalid=runtime_native_hud_field(base,type,"bInvalidated","BoolProperty",0xb8,4);uint64_t mask=0;
    object_name(base,target,name,sizeof name);
    return owner&&info&&narrative_probe_object(base,type,"ScriptStruct")&&read_ptr(type+0x40)==owner&&
        narrative_probe_object(base,target,"Class")&&!strcmp(name,"BioPawn")&&invalid&&
        read_at(invalid+0xd0,&mask,8)&&mask==0x200&&
        runtime_power_property(base,handler,"SFXSFHandler_HUD","m_pPlayerController","ObjectProperty",0x708,8)&&
        runtime_power_property(base,handler,"SFXSFHandler_HUD","m_pPlayerSquad","ObjectProperty",0x710,8)&&
        runtime_power_property(base,handler,"SFXGUIMovie","oWorldInfo","ObjectProperty",0x2bc,8);
}
static int runtime_native_hud_scope(uintptr_t base,SessionContext c) {
    NetworkActorIdentityEntry channel={0};
    return runtime_native_hud_enabled()&&runtime_control_enabled&&runtime_scope_ready&&
        g_runtime_config.role==ME3_ROLE_GUEST&&InterlockedCompareExchange(&g_enable_requested,0,0)&&
        !session_environment_block_reason(c)&&(!c.mode||runtime_power_menu_context_held(base,c))&&
        guest_player_context_basic_owned(base,c.pc,c.pawn,c.world)&&guest_player_context_channel(base,c.pawn,&channel);
}
static int runtime_native_hud_owner(uintptr_t base,SessionContext c) {
    RuntimeNativeHud *q=&runtime_native_hud;
    return q->active&&c.pc==q->pc.pointer&&c.pawn==q->pawn.pointer&&c.world==q->world.pointer&&
        session_handle_matches(q->handler)&&session_handle_matches(q->pc)&&session_handle_matches(q->pawn)&&
        session_handle_matches(q->world)&&read_ptr(q->handler.pointer)==base+0x14f58b0&&
        read_ptr(q->handler.pointer+0x708)==c.pc&&read_ptr(q->handler.pointer+0x2bc)==c.world;
}
/* +3cc is intentionally unaligned. Game-thread conditional eight-byte UI
 * write; never use an interlocked pointer operation on this field. */
static int runtime_native_hud_pointer(uintptr_t at,uintptr_t expected,uintptr_t desired) {
    uintptr_t actual=0;SIZE_T count=0;
    if(!read_at(at,&actual,8)||actual!=expected)return 0;
    if(actual==desired)return 1;
    return background_focus_writable(at,8)&&
        WriteProcessMemory(GetCurrentProcess(),(void*)at,&desired,8,&count)&&count==8&&read_ptr(at)==desired;
}
static void runtime_native_hud_invalidate(uintptr_t handler) {
    uintptr_t at=handler+0x3ec;
    if(!(at&3)&&background_focus_writable(at,4))InterlockedOr((LONG volatile*)at,0x200);
}
static void runtime_native_hud_retire(void) {
    RuntimeNativeHud *q=&runtime_native_hud;
    if(q->active&&q->changed&&session_handle_matches(q->handler)&&
       read_ptr(q->handler.pointer)==q->base+0x14f58b0&&read_ptr(q->handler.pointer+0x708)==q->pc.pointer&&
       read_ptr(q->handler.pointer+0x2bc)==q->world.pointer) {
        uintptr_t original=0;
        if(q->baseline.pointer&&session_handle_matches(q->baseline)&&guest_camera_actor_live(q->base,q->baseline.pointer)&&
           read_ptr(q->baseline.pointer+0x198)==q->world.pointer)original=q->baseline.pointer;
        if(runtime_native_hud_pointer(q->handler.pointer+0x3cc,q->pawn.pointer,original))
            runtime_native_hud_invalidate(q->handler.pointer);
    }
    if(q->active)line("{\"runtime_native_hud\":\"display-source-retired\",\"squad_written\":false}\r\n");
    q->active=q->changed=q->reported=0;q->observed=0;
}
static void runtime_native_hud_selected(void *self,unsigned which,uintptr_t caller) {
    RuntimeNativeHud *q=&runtime_native_hud;
    q->original[which](self);DWORD after=GetLastError();
    uintptr_t base=q->base,handler=(uintptr_t)self;
    if(!q->busy&&GetCurrentThreadId()==session_game_thread&&
       caller==base+(which?0xc57d0f:0xc57d17)&&handler==q->handler.pointer) {
        q->busy=1;SessionContext c=session_context(session_engine);
        /* The native hench selector can dispatch callbacks. Recheck current
         * ownership and the complete captured UI scope after it returns. */
        if(runtime_native_hud_scope(base,c)&&runtime_native_hud_owner(base,c)) {
            uintptr_t selected=0;
            if(read_at(handler+0x3cc,&selected,8)&&(!selected||
               (guest_camera_actor_live(base,selected)&&is_kind_of(base,selected,"BioPawn")&&read_ptr(selected+0x198)==c.world))) {
                SessionHandle baseline=session_capture_handle(selected);
                if((!selected||session_handle_matches(baseline))&&
                   runtime_native_hud_pointer(handler+0x3cc,selected,c.pawn)) {
                    if(selected!=c.pawn){q->baseline=baseline;q->changed=1;runtime_native_hud_invalidate(handler);}
                    q->observed=GetTickCount();
                    if(!q->reported){q->reported=1;line("{\"runtime_native_hud\":\"owned-player-source-bound\",\"stock_movie\":true,\"squad_written\":false,\"gameplay_verified\":false}\r\n");}
                }
            }
        }
        q->busy=0;
    }
    SetLastError(after);
}
static void runtime_native_hud_squad(void *self) {runtime_native_hud_selected(self,0,(uintptr_t)__builtin_return_address(0));}
static void runtime_native_hud_player(void *self) {runtime_native_hud_selected(self,1,(uintptr_t)__builtin_return_address(0));}
static int runtime_native_hud_install(uintptr_t base) {
    RuntimeNativeHud *q=&runtime_native_hud;
    uintptr_t table=base+0x14f58b0,slots[]={table+0x488,table+0x490};
    uintptr_t expected[]={base+0xc51e60,base+0xc519e0};
    uintptr_t wrappers[]={(uintptr_t)runtime_native_hud_squad,(uintptr_t)runtime_native_hud_player};
    if(q->installed)return q->base==base&&read_ptr(slots[0])==wrappers[0]&&read_ptr(slots[1])==wrappers[1];
    static const unsigned char pins[2][16]={
        {0x40,0x53,0x48,0x83,0xec,0x50,0x48,0x8b,0xd9,0x48,0x8b,0x89,0x10,0x07,0,0},
        {0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0x81,0x10,0x07,0,0,0x48,0x8b,0xd9}};
    static const unsigned char calls[]={0x48,0x8b,0xcf,0xf6,0x87,0xb8,0x08,0,0,0x08,0x48,0x8b,0x07,
        0x75,0x08,0xff,0x90,0x90,0x04,0,0,0xeb,0x06,0xff,0x90,0x88,0x04,0,0};
    unsigned char bytes[sizeof calls];
    if(!supported_image(base)||read_ptr(table+0x3f8)!=base+0xc57af0||
       !read_at(base+0xc57cfa,bytes,sizeof calls)||memcmp(bytes,calls,sizeof calls))return 0;
    for(unsigned i=0;i<2;++i)if(read_ptr(slots[i])!=expected[i]||!read_at(expected[i],bytes,16)||memcmp(bytes,pins[i],16))return 0;
    DWORD old=0,ignored=0;if(!VirtualProtect((void*)slots[0],16,PAGE_READWRITE,&old))return 0;
    q->base=base;q->original[0]=(RuntimeHudSelector)expected[0];q->original[1]=(RuntimeHudSelector)expected[1];
    int first=(uintptr_t)InterlockedCompareExchangePointer((PVOID volatile*)slots[0],(PVOID)wrappers[0],(PVOID)expected[0])==expected[0];
    int second=first&&(uintptr_t)InterlockedCompareExchangePointer((PVOID volatile*)slots[1],(PVOID)wrappers[1],(PVOID)expected[1])==expected[1];
    if(first&&!second)InterlockedCompareExchangePointer((PVOID volatile*)slots[0],(PVOID)expected[0],(PVOID)wrappers[0]);
    VirtualProtect((void*)slots[0],16,old,&ignored);q->installed=second;
    if(second)line("{\"runtime_native_hud\":\"stock-player-selectors-installed\",\"health_written\":false}\r\n");
    return second;
}
static void runtime_native_hud_tick(uintptr_t base,uintptr_t engine,DWORD now) {
    RuntimeNativeHud *q=&runtime_native_hud;
    if(GetCurrentThreadId()!=session_game_thread||g_runtime_config.role!=ME3_ROLE_GUEST)return;
    SessionContext c=session_context(engine);
    if(q->active&&(!runtime_native_hud_scope(base,c)||!runtime_native_hud_owner(base,c)))runtime_native_hud_retire();
    if(q->active||!runtime_native_hud_scope(base,c)||c.mode||(q->scanned&&now-q->scanned<500))return;
    q->scanned=now;if(!runtime_power_wheel_registry_pins(base))return;
    uintptr_t registry=read_ptr(base+0x18b6be0),found=0;GameArray lists[2]={0};
    uintptr_t records[2][128]={{0}},movies[2][128]={{0}};
    for(unsigned g=0;g<2;++g) {
        if(!scene_readiness_array(registry+(g?0xbc:0x4c),&lists[g],128)||
           (lists[g].count&&!read_at(lists[g].data,records[g],(SIZE_T)lists[g].count*8)))return;
        for(int i=0;i<lists[g].count;++i) {
            uintptr_t movie=read_ptr(records[g][i]+0x5c);movies[g][i]=movie;
            if(!movie||read_ptr(movie)!=base+0x14f58b0||read_ptr(movie+0x708)!=c.pc||read_ptr(movie+0x2bc)!=c.world)continue;
            if((found&&found!=movie)||!runtime_native_hud_layout(base,movie))return;found=movie;
        }
    }
    if(!found||read_ptr(base+0x18b6be0)!=registry)return;
    for(unsigned g=0;g<2;++g) {
        GameArray after={0};if(!read_at(registry+(g?0xbc:0x4c),&after,sizeof after)||memcmp(&after,&lists[g],sizeof after))return;
        for(int i=0;i<lists[g].count;++i)if(read_ptr(lists[g].data+(SIZE_T)i*8)!=records[g][i]||read_ptr(records[g][i]+0x5c)!=movies[g][i])return;
    }
    if(!runtime_native_hud_scope(base,session_context(engine))||!runtime_native_hud_install(base))return;
    q->handler=session_capture_handle(found);q->pc=session_capture_handle(c.pc);q->pawn=session_capture_handle(c.pawn);q->world=session_capture_handle(c.world);
    q->active=session_handle_matches(q->handler)&&session_handle_matches(q->pc)&&session_handle_matches(q->pawn)&&session_handle_matches(q->world);
}
static int runtime_native_hud_showing(uintptr_t base,SessionContext c) {
    RuntimeNativeHud *q=&runtime_native_hud;uint32_t flags=0;GameArray modes={0};
    uintptr_t mode=0;uint32_t permission=0;
    return q->observed&&GetTickCount()-q->observed<=1000&&runtime_native_hud_scope(base,c)&&runtime_native_hud_owner(base,c)&&
        read_ptr(q->handler.pointer+0x3cc)==c.pawn&&read_at(q->handler.pointer+0x8b8,&flags,4)&&(flags&0x100)&&!(flags&0x4000)&&
        narrative_probe_object(base,c.manager,"SFXGameModeManager")&&identity_array(c.manager+0x70,&modes,64)&&
        c.mode>=0&&c.mode<modes.count&&read_at(modes.data+(size_t)c.mode*8,&mode,8)&&
        narrative_probe_object(base,mode,"SFXGameModeBase")&&read_ptr(mode+0x40)==c.pc&&
        read_at(mode+0xd0,&permission,4)&&(permission&0x84)==0x84;
}
static int runtime_native_hud_teammate_showing(uintptr_t base,SessionContext c) {
    if(!runtime_native_hud_showing(base,c)||!presentation_guest_valid(base))return 0;
    uintptr_t handler=runtime_native_hud.handler.pointer,pawn=presentation_guest.objects[2].pointer;
    static const int fields[]={0x48c,0x54c};
    for(unsigned n=0;n<2;++n){uint32_t flags=0;
        if(read_ptr(handler+fields[n])==pawn&&read_at(handler+fields[n]+0x20,&flags,4)&&
           (flags&0x8400)==0x8400)return 1;}
    return 0;
}
static unsigned runtime_native_hud_fixture_calls[2];
static void runtime_native_hud_fixture_squad(void *self) {
    if((uintptr_t)self==17&&GetLastError()==601)++runtime_native_hud_fixture_calls[0];SetLastError(602);
}
static void runtime_native_hud_fixture_player(void *self) {
    if((uintptr_t)self==19&&GetLastError()==603)++runtime_native_hud_fixture_calls[1];SetLastError(604);
}
static int runtime_native_hud_selftest(void) {
    /* Conditional unaligned display-source writes preserve adjacent fields
     * and refuse a native replacement instead of overwriting it. */
    _Alignas(8) unsigned char memory[24];memset(memory,0xa5,sizeof memory);
    uintptr_t before=11,after=0;memcpy(memory+4,&before,8);
    if(runtime_native_hud_pointer((uintptr_t)memory+4,12,22)||
       !runtime_native_hud_pointer((uintptr_t)memory+4,11,22))return 0;
    memcpy(&after,memory+4,8);if(after!=22)return 0;
    for(unsigned i=0;i<24;++i)if((i<4||i>=12)&&memory[i]!=0xa5)return 0;
    if(runtime_native_hud_pointer((uintptr_t)memory+4,11,0)||!runtime_native_hud_pointer((uintptr_t)memory+4,22,0))return 0;
    RuntimeNativeHud saved=runtime_native_hud;DWORD error=GetLastError();
    runtime_native_hud.busy=1;runtime_native_hud.original[0]=runtime_native_hud_fixture_squad;
    runtime_native_hud.original[1]=runtime_native_hud_fixture_player;
    runtime_native_hud_fixture_calls[0]=runtime_native_hud_fixture_calls[1]=0;
    SetLastError(601);runtime_native_hud_selected((void*)17,0,0);int ok=GetLastError()==602;
    SetLastError(603);runtime_native_hud_selected((void*)19,1,0);ok&=GetLastError()==604;
    ok&=runtime_native_hud_fixture_calls[0]==1&&runtime_native_hud_fixture_calls[1]==1;
    runtime_native_hud=saved;SetLastError(error);return ok;
}
#endif
