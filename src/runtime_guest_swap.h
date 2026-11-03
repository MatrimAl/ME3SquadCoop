#ifndef ME3_RUNTIME_GUEST_SWAP_H
#define ME3_RUNTIME_GUEST_SWAP_H
/* The stock X alias invokes TrySwapWeapon, whose Pawn cast accepts Shepard
 * only. Forward that script exactly once. For the unchanged owned companion
 * case, let stock NextWeapon perform its combat/action/cover/equip checks and
 * native inventory/network path. Never write WeaponOnDeck, input or ammo. */
typedef void (*RuntimeGuestSwapExec)(void*,void*,void*);
typedef struct RuntimeGuestSwapScope {
    SessionHandle pc,pawn,world,manager,mode,inventory,weapon,channel,connection,driver;
    GameArray modes;
    unsigned long token;
} RuntimeGuestSwapScope;
typedef struct RuntimeGuestSwapCapture {RuntimeGuestSwapScope scope;uintptr_t frame;int valid;} RuntimeGuestSwapCapture;
typedef struct RuntimeGuestSwap {
    uintptr_t base;GuestVitalsMethod swap,next;RuntimeGuestSwapExec original;
    DWORD checked;unsigned rows;int installed,busy;
} RuntimeGuestSwap;
static RuntimeGuestSwap runtime_guest_swap;
static int runtime_guest_swap_enabled(void) {
    static int enabled=-1;
    if(enabled<0){wchar_t value[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_WEAPON_SWAP",value,8);
        enabled=!n||(n==1&&value[0]==L'1');}return enabled;
}
static int runtime_guest_swap_same(const RuntimeGuestSwapScope *a,const RuntimeGuestSwapScope *b) {
    const SessionHandle *aa[]={&a->pc,&a->pawn,&a->world,&a->manager,&a->mode,&a->inventory,&a->weapon,&a->channel,&a->connection,&a->driver};
    const SessionHandle *bb[]={&b->pc,&b->pawn,&b->world,&b->manager,&b->mode,&b->inventory,&b->weapon,&b->channel,&b->connection,&b->driver};
    if(!a->token||a->token!=b->token||memcmp(&a->modes,&b->modes,sizeof a->modes))return 0;
    for(unsigned i=0;i<10;++i)if(!runtime_power_same_handle(*aa[i],*bb[i]))return 0;
    return 1;
}
static int runtime_guest_swap_scope(uintptr_t base,uintptr_t receiver,RuntimeGuestSwapScope *out) {
    memset(out,0,sizeof *out);SessionContext c=session_context(session_engine);NetworkActorIdentityEntry channel={0};
    GameArray modes={0},again={0};uint32_t life=0;unsigned char mode=255;
    if(GetCurrentThreadId()!=session_game_thread||!runtime_guest_swap_enabled()||
       !runtime_control_enabled||!runtime_scope_ready||g_runtime_config.role!=ME3_ROLE_GUEST||
       !InterlockedCompareExchange(&g_enable_requested,0,0)||c.mode!=0||session_engine_blocked(c)||
       !guest_player_context_registered_for(c.pc,c.pawn)||!guest_player_context_basic_owned(base,c.pc,c.pawn,c.world)||
       !read_at(c.pawn+0xe20,&life,4)||(life&0x300)||
       !narrative_probe_object(base,receiver,"SFXGameModeDefault")||read_ptr(receiver+0x40)!=c.pc||
       !narrative_probe_object(base,c.manager,"SFXGameModeManager")||read_ptr(c.pc+0xc38)!=c.manager||
       !runtime_power_property(base,c.manager,"SFXGameModeManager","GameModes","ArrayProperty",0x70,16)||
       !identity_array(c.manager+0x70,&modes,64)||read_ptr(modes.data)!=receiver||
       !read_at(c.manager+0xa8,&mode,1)||mode||
       !guest_player_context_channel(base,c.pawn,&channel)||channel.index<=0)return 0;
    uintptr_t inventory=read_ptr(c.pawn+0x414),weapon=read_ptr(c.pawn+0x41c);
    if(!runtime_guest_weapon_chain(base,c.pc,c.pawn,weapon,inventory,c.world,1)||read_ptr(inventory+0x288))return 0;
    out->pc=session_capture_handle(c.pc);out->pawn=session_capture_handle(c.pawn);out->world=session_capture_handle(c.world);
    out->manager=session_capture_handle(c.manager);out->mode=session_capture_handle(receiver);
    out->inventory=session_capture_handle(inventory);out->weapon=session_capture_handle(weapon);
    out->channel=session_capture_handle(channel.channel);out->connection=session_capture_handle(channel.connection);
    out->driver=session_capture_handle(channel.driver);out->modes=modes;out->token=guest_player_context_registration_token();
    SessionHandle *handles[]={&out->pc,&out->pawn,&out->world,&out->manager,&out->mode,&out->inventory,&out->weapon,
        &out->channel,&out->connection,&out->driver};
    for(unsigned i=0;i<10;++i)if(!session_handle_matches(*handles[i]))return 0;
    return out->token&&read_at(c.manager+0x70,&again,sizeof again)&&!memcmp(&modes,&again,sizeof modes)&&
        read_ptr(modes.data)==receiver&&read_ptr(c.pc+0xc38)==c.manager&&read_ptr(receiver+0x40)==c.pc;
}
static int runtime_guest_swap_method(uintptr_t base,const GuestVitalsMethod *method,uintptr_t expected,int length) {
    GameArray script={0};uint32_t flags=0;uint16_t native=1,size=1,ret=0;unsigned char count=1;
    return guest_camera_stamp_matches(base,&method->object,1)&&
        read_ptr(method->object.pointer+0xd8)==expected&&
        read_at(method->object.pointer+0xe0,&flags,4)&&flags==0x20202&&
        read_at(method->object.pointer+0xe4,&native,2)&&!native&&
        read_at(method->object.pointer+0xe6,&size,2)&&!size&&
        read_at(method->object.pointer+0xe8,&ret,2)&&ret==65535&&
        read_at(method->object.pointer+0xea,&count,1)&&!count&&
        read_at(method->object.pointer+0x7c,&script,sizeof script)&&script.data&&script.count==length&&
        script.max>=length&&script.max<=4096;
}
static void runtime_guest_swap_exec(void *self,void *frame,void *result);
static RuntimeGuestSwapCapture runtime_guest_swap_capture(void *self,void *frame) {
    RuntimeGuestSwap *s=&runtime_guest_swap;RuntimeGuestSwapCapture c={0};GameArray script={0};
    if(!s->installed||s->busy||!runtime_guest_swap_method(s->base,&s->swap,(uintptr_t)runtime_guest_swap_exec,135)||
       !runtime_guest_swap_method(s->base,&s->next,s->base+0xf52b0,536)||
       read_ptr((uintptr_t)frame+0x18)!=s->swap.object.pointer||read_ptr((uintptr_t)frame+0x20)!=(uintptr_t)self||
       !read_at(s->swap.object.pointer+0x7c,&script,sizeof script)||read_ptr((uintptr_t)frame+0x28)!=script.data||
       !runtime_guest_swap_scope(s->base,(uintptr_t)self,&c.scope))return c;
    c.frame=(uintptr_t)frame;c.valid=1;return c;
}
static void runtime_guest_swap_after(RuntimeGuestSwapCapture *capture) {
    RuntimeGuestSwap *s=&runtime_guest_swap;RuntimeGuestSwapScope current;
    if(!capture->valid||s->busy||!runtime_guest_swap_method(s->base,&s->swap,(uintptr_t)runtime_guest_swap_exec,135)||
       !runtime_guest_swap_method(s->base,&s->next,s->base+0xf52b0,536)||
       read_ptr(capture->frame+0x18)!=s->swap.object.pointer||read_ptr(capture->frame+0x20)!=capture->scope.mode.pointer||
       !runtime_guest_swap_scope(s->base,capture->scope.mode.pointer,&current)||
       !runtime_guest_swap_same(&capture->scope,&current))return;
    uintptr_t pe=read_ptr(read_ptr(current.mode.pointer)+0x230);
    if(pe!=s->base+0xf4f80)return;
    s->busy=1;
    ((ProcessEvent)pe)((void*)current.mode.pointer,(void*)s->next.object.pointer,NULL,NULL);
    s->busy=0;
    if(s->rows++<32)line("{\"runtime_guest_swap\":\"stock-next-weapon-dispatched\",\"input_synthesized\":false,\"ammo_written\":false,\"gameplay_verified\":false}\r\n");
}
static void runtime_guest_swap_forward(void *self,void *frame,void *result,RuntimeGuestSwapExec stock,
        RuntimeGuestSwapCapture (*capture)(void*,void*),void (*after)(RuntimeGuestSwapCapture*)) {
    DWORD error=GetLastError();RuntimeGuestSwapCapture c=capture(self,frame);
    SetLastError(error);stock(self,frame,result);error=GetLastError();after(&c);SetLastError(error);
}
static void runtime_guest_swap_exec(void *self,void *frame,void *result) {
    runtime_guest_swap_forward(self,frame,result,runtime_guest_swap.original,runtime_guest_swap_capture,runtime_guest_swap_after);
}
static void runtime_guest_swap_prepare(uintptr_t base,uintptr_t engine,DWORD now) {
    RuntimeGuestSwap *s=&runtime_guest_swap;
    if(GetCurrentThreadId()!=session_game_thread||g_runtime_config.role!=ME3_ROLE_GUEST||
       !runtime_guest_swap_enabled()||s->installed||(s->checked&&now-s->checked<2000))return;
    s->checked=now;SessionContext c=session_context(engine);
    if(!runtime_control_enabled||!runtime_scope_ready||session_engine_blocked(c)||
       !supported_image(base)||!guest_equipment_dispatch_pins(base)||
       !guest_vitals_method(base,"SFXGameModeDefault","TrySwapWeapon",0x20202,0,NULL,0,&s->swap)||
       !guest_vitals_method(base,"SFXGameModeDefault","NextWeapon",0x20202,0,NULL,0,&s->next)||
       !runtime_guest_swap_method(base,&s->swap,base+0xf52b0,135)||
       !runtime_guest_swap_method(base,&s->next,base+0xf52b0,536))return;
    uintptr_t slot=s->swap.object.pointer+0xd8;
    if(!background_focus_writable(slot,8))return;
    s->base=base;s->original=(RuntimeGuestSwapExec)(base+0xf52b0);
    if(InterlockedCompareExchangePointer((PVOID volatile*)slot,(PVOID)runtime_guest_swap_exec,
       (PVOID)s->original)!=(PVOID)s->original)return;
    s->installed=1;
    line("{\"runtime_guest_swap\":\"stock-try-swap-wrapper-installed\",\"gameplay_verified\":false}\r\n");
}
#ifdef ME3_RUNTIME_FIXTURE
static unsigned runtime_swap_fixture_order,runtime_swap_fixture_calls;
static int runtime_swap_fixture_bad;static void *runtime_swap_fixture_args[3];
static RuntimeGuestSwapCapture runtime_swap_fixture_capture(void *self,void *frame) {
    RuntimeGuestSwapCapture c={0};
    if(self!=runtime_swap_fixture_args[0]||frame!=runtime_swap_fixture_args[1]||runtime_swap_fixture_order++)runtime_swap_fixture_bad=1;
    SetLastError(11);return c;
}
static void runtime_swap_fixture_stock(void *self,void *frame,void *result) {
    if(self!=runtime_swap_fixture_args[0]||frame!=runtime_swap_fixture_args[1]||result!=runtime_swap_fixture_args[2]||
       GetLastError()!=37||runtime_swap_fixture_order++!=1)runtime_swap_fixture_bad=1;
    ++runtime_swap_fixture_calls;SetLastError(53);
}
static void runtime_swap_fixture_after(RuntimeGuestSwapCapture *capture) {
    if(capture->valid||GetLastError()!=53||runtime_swap_fixture_order++!=2)runtime_swap_fixture_bad=1;
    SetLastError(79);
}
static int runtime_guest_swap_selftest(void) {
    DWORD error=GetLastError();int a,b,c;
    runtime_swap_fixture_args[0]=&a;runtime_swap_fixture_args[1]=&b;runtime_swap_fixture_args[2]=&c;
    runtime_swap_fixture_order=runtime_swap_fixture_calls=0;runtime_swap_fixture_bad=0;SetLastError(37);
    runtime_guest_swap_forward(&a,&b,&c,runtime_swap_fixture_stock,runtime_swap_fixture_capture,runtime_swap_fixture_after);
    int ok=GetLastError()==53&&runtime_swap_fixture_calls==1&&runtime_swap_fixture_order==3&&!runtime_swap_fixture_bad;
    RuntimeGuestSwapScope first={0},second;
    SessionHandle *handles[]={&first.pc,&first.pawn,&first.world,&first.manager,&first.mode,&first.inventory,&first.weapon,
        &first.channel,&first.connection,&first.driver};
    for(unsigned i=0;i<10;++i)*handles[i]=(SessionHandle){0x100+i*8,(int32_t)i+1,i+2};
    first.token=1;first.modes=(GameArray){0x1000,8,8};second=first;
    ok=ok&&runtime_guest_swap_same(&first,&second);
    for(unsigned i=0;i<10;++i){++handles[i]->index;ok=ok&&!runtime_guest_swap_same(&first,&second);--handles[i]->index;}
    ++first.token;ok=ok&&!runtime_guest_swap_same(&first,&second);first=second;++first.modes.data;
    ok=ok&&!runtime_guest_swap_same(&first,&second);SetLastError(error);return ok;
}
#endif
#endif
