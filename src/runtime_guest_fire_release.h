#ifndef ME3_RUNTIME_GUEST_FIRE_RELEASE_H
#define ME3_RUNTIME_GUEST_FIRE_RELEASE_H
/* StopFiringWeapon normally clears only Controller.bFire; the pawn's later
 * Tick clears inventory pending fire. On the exact local human FullAuto2
 * release, complete that same stock Pawn.StopFire path before another weapon
 * timer can consume a round. Preserve semi/burst and unrelated controllers.
 * A real release while Active has a deferred port-arms CheckBeginFire timer;
 * cancel only that named stock timer before the same ordinary stop. No ammo,
 * fire input, pending mode, timer-array or network-role field is written. */
typedef void (*RuntimeGuestReleaseExec)(void*,void*,void*);
typedef struct RuntimeGuestReleaseScope {
    RuntimeGuestFireScope fire;
    SessionHandle manager,mode;
    GameArray modes;
} RuntimeGuestReleaseScope;
typedef struct RuntimeGuestReleaseCapture {RuntimeGuestReleaseScope scope;uintptr_t frame;int valid,kind;} RuntimeGuestReleaseCapture;
typedef struct RuntimeGuestRelease {
    uintptr_t base;GuestPlayerContextMethod release,stop;
    RuntimeGuestReleaseExec original;DWORD checked;
    int installed,busy;unsigned rows;
} RuntimeGuestRelease;
static RuntimeGuestRelease runtime_guest_release;
static int runtime_guest_fire_release_enabled(void) {
    static int enabled=-1;
    if(enabled<0){wchar_t value[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_FIRE_RELEASE",value,8);
        enabled=!n||(n==1&&value[0]==L'1');}return enabled;
}
/* Active.BeginFire's port-arms branch sets weapon PendingFireMode before it
 * calls the normal inventory BeginFire path. Inventory pending may still be0. */
static int runtime_guest_release_kind(unsigned authored,unsigned current,unsigned mode,int pending,
        unsigned weapon_pending,int state) {
    if(authored!=2||mode!=2)return 0;
    if(state==1&&current==2&&pending==1)return 1;
    if(state==2&&!current&&weapon_pending==2&&(pending==0||pending==1))return 2;
    return 0;
}
static int runtime_guest_release_completed(int captured,int same,int cursor,unsigned before,unsigned after,int old_kind,int kind) {
    return captured&&same&&cursor==35&&before==1&&!after&&old_kind>0&&old_kind<=2&&old_kind==kind;
}
static int runtime_guest_release_pending(const GameArray *requests,int32_t *pending) {
    if(requests->count<0||requests->count>32)return 0;
    *pending=0;
    return requests->count<=2||read_at(requests->data+8,pending,4);
}
static int runtime_guest_release_method(uintptr_t base,const GuestPlayerContextMethod *method,uintptr_t expected,
        int release) {
    uintptr_t fn=method->function.pointer;uint32_t flags=0;uint16_t index=1,size=0,ret=0;unsigned char count=0;GameArray script={0};
    if(!guest_player_context_method_matches(base,method,0)||read_ptr(fn+0xd8)!=expected||
       strcmp(method->function.full_name,release?"SFXGame.SFXGameModeBase.StopFiringWeapon":"Engine.Pawn.StopFire")||
       !read_at(fn+0xe0,&flags,4)||flags!=(release?0x20202:0x20102)||!read_at(fn+0xe4,&index,2)||index||
       !read_at(fn+0xe6,&size,2)||size!=(release?0:1)||!read_at(fn+0xe8,&ret,2)||ret!=65535||
       !read_at(fn+0xea,&count,1)||count!=(release?0:1)||!read_at(fn+0x7c,&script,sizeof script)||
       !script.data||script.count!=(release?36:57)||script.max<script.count||script.max>4096)return 0;
    uintptr_t field=read_ptr(fn+0x70);
    return release?!field:read_ptr(field+0x40)==fn&&!read_ptr(field+0x68)&&
        runtime_delayed_fire_property(base,field,"ByteProperty","Engine.Pawn.StopFire.FireModeNum",0,1,0x80);
}
static int runtime_guest_release_bodies(uintptr_t base,uintptr_t release,uintptr_t stop) {
    static const unsigned char release_code[36]={
        0x0f,0x19,0x62,0,0,0,0,0,0,0,0,0x09,0,0,0,0,0,0,0,0,0,0,0x61,0,0,0,0,0,0,0,0,0x24,0,0x04,0x0b,0x53};
    static const GuestEquipmentOperand release_ops[]={
        {3,"ObjectProperty","Core.Object.Outer",0x40,UINT64_C(0x21002)},
        {13,"ByteProperty","Engine.Controller.bFire",0x480,UINT64_C(4)},
        {23,"ByteProperty","Engine.Controller.bFire",0x480,UINT64_C(4)},{0,NULL,NULL,0,0}};
    static const unsigned char stop_code[57]={
        0x64,0,0,0,0,0,0,0,0,0x01,0x36,0,0x19,0x62,0,0,0,0,0,0,0,0,0x15,0,0,0,0,0,0,0,0,0,0,0x65,
        0,0,0,0,0,0,0,0,0xb3,0,0x5d,0,0,0,0,0,0,0,0,0x16,0x04,0x0b,0x53};
    static const GuestEquipmentOperand stop_ops[]={
        {1,"ObjectProperty","Engine.Pawn.InvManager",0x414,UINT64_C(0x100000020)},
        {14,"ObjectProperty","Engine.Pawn.InvManager",0x414,UINT64_C(0x100000020)},
        {45,"ByteProperty","Engine.Pawn.StopFire.FireModeNum",0,UINT64_C(0x80)},{0,NULL,NULL,0,0}};
    static const GuestEquipmentName none[]={{0,NULL}},stop_names[]={{34,"StopFire"},{0,NULL}};
    return guest_equipment_body(base,release,36,release_code,sizeof release_code,release_ops,none)&&
        guest_equipment_body(base,stop,57,stop_code,sizeof stop_code,stop_ops,stop_names);
}
static int runtime_guest_release_held(uintptr_t base,const RuntimeGuestReleaseScope *s,unsigned *fire,int *kind) {
    uintptr_t weapon=s->fire.lease.weapon.pointer,pc=s->fire.lease.pc.pointer;
    unsigned char current=0,mode=0,authored=0,input=255,weapon_pending=255;int32_t pending=0,action=-1,power=-1;
    GameArray requests={0};uintptr_t state=0;uint32_t name[2];GuestCameraStamp stamp;
    if(!read_at(pc+0x480,&input,1)||!read_at(weapon+0x3b0,&current,1)||!read_at(weapon+0x10be,&mode,1)||
       !read_at(weapon+0x10c2,&weapon_pending,1)||
       !read_at(s->fire.lease.cdo.pointer+0x10be,&authored,1)||
       !runtime_guest_team_field(base,s->fire.lease.inventory.pointer,"InventoryManager","PendingFire","ArrayProperty",0x270,16,0x400000)||
       !scene_readiness_array(s->fire.lease.inventory.pointer+0x270,&requests,32)||
       !runtime_guest_release_pending(&requests,&pending)||
       !read_at(s->fire.lease.pawn.pointer+0xc20,&action,4)||action||
       !read_at(s->fire.lease.pawn.pointer+0xc28,&power,4)||power||
       !native_state_fields(weapon,&state,name)||!guest_camera_stamp_read(base,state,&stamp,1)||
       !narrative_probe_object(base,state,"State"))return 0;
    int state_kind=!strcmp(stamp.full_name,"SFXGame.SFXWeapon.WeaponFiring")?1:
        !strcmp(stamp.full_name,"SFXGame.SFXWeapon.Active")?2:0;
    *fire=input;*kind=runtime_guest_release_kind(authored,current,mode,pending,weapon_pending,state_kind);return *kind!=0;
}
static int runtime_guest_release_scope(uintptr_t base,uintptr_t receiver,RuntimeGuestReleaseScope *out) {
    memset(out,0,sizeof *out);SessionContext c=session_context(session_engine);unsigned char mode=255;GameArray again={0};
    if(GetCurrentThreadId()!=session_game_thread||!runtime_guest_fire_release_enabled()||runtime_guest_release.busy||
       g_runtime_config.role!=ME3_ROLE_GUEST||c.mode||
       !runtime_guest_fire_scope(base,session_engine,&out->fire)||c.pc!=out->fire.lease.pc.pointer||
       c.pawn!=out->fire.lease.pawn.pointer||c.world!=out->fire.lease.world.pointer||
       !narrative_probe_object(base,receiver,"SFXGameModeDefault")||read_ptr(receiver+0x40)!=c.pc||
       !narrative_probe_object(base,c.manager,"SFXGameModeManager")||read_ptr(c.pc+0xc38)!=c.manager||
       !runtime_power_property(base,c.manager,"SFXGameModeManager","GameModes","ArrayProperty",0x70,16)||
       !identity_array(c.manager+0x70,&out->modes,64)||out->modes.count<=0||read_ptr(out->modes.data)!=receiver||
       !read_at(c.manager+0xa8,&mode,1)||mode)return 0;
    out->manager=session_capture_handle(c.manager);out->mode=session_capture_handle(receiver);
    return session_handle_matches(out->manager)&&session_handle_matches(out->mode)&&
        read_at(c.manager+0x70,&again,sizeof again)&&!memcmp(&out->modes,&again,sizeof again)&&
        read_ptr(out->modes.data)==receiver&&read_ptr(receiver+0x40)==c.pc;
}
static int runtime_guest_release_same(const RuntimeGuestReleaseScope *a,const RuntimeGuestReleaseScope *b) {
    return runtime_guest_fire_scope_same(&a->fire,&b->fire)&&runtime_power_same_handle(a->manager,b->manager)&&
        runtime_power_same_handle(a->mode,b->mode)&&!memcmp(&a->modes,&b->modes,sizeof a->modes);
}
static void runtime_guest_fire_release_exec(void *self,void *frame,void *result);
static RuntimeGuestReleaseCapture runtime_guest_release_capture(void *self,void *frame) {
    RuntimeGuestReleaseCapture c={0};RuntimeGuestRelease *s=&runtime_guest_release;GameArray script={0};unsigned fire=0;
    if(!s->installed||s->busy||GetCurrentThreadId()!=session_game_thread||
       !runtime_guest_release_method(s->base,&s->release,(uintptr_t)runtime_guest_fire_release_exec,1)||
       !runtime_guest_release_method(s->base,&s->stop,s->base+0xf52b0,0)||
       read_ptr((uintptr_t)frame+0x18)!=s->release.function.pointer||read_ptr((uintptr_t)frame+0x20)!=(uintptr_t)self||
       !read_at(s->release.function.pointer+0x7c,&script,sizeof script)||read_ptr((uintptr_t)frame+0x28)!=script.data||
       !runtime_guest_release_scope(s->base,(uintptr_t)self,&c.scope)||
       !runtime_guest_release_held(s->base,&c.scope,&fire,&c.kind)||fire!=1)return c;
    c.valid=1;c.frame=(uintptr_t)frame;return c;
}
static int runtime_guest_release_dispatch(int eligible,ProcessEvent pe,uintptr_t pawn,uintptr_t method,unsigned mode) {
    DWORD error=GetLastError();int called=0;
    if(eligible&&pe&&pawn&&method&&mode==2) {
        unsigned char param=2;pe((void*)pawn,(void*)method,&param,NULL);called=1;
    }
    SetLastError(error);return called;
}
static void runtime_guest_release_after(RuntimeGuestReleaseCapture *capture) {
    RuntimeGuestRelease *s=&runtime_guest_release;RuntimeGuestReleaseScope current;GameArray script={0};unsigned fire=255;int kind=0;
    if(!capture->valid||s->busy||!runtime_guest_release_method(s->base,&s->release,(uintptr_t)runtime_guest_fire_release_exec,1)||
       !runtime_guest_release_method(s->base,&s->stop,s->base+0xf52b0,0)||
       read_ptr(capture->frame+0x18)!=s->release.function.pointer||read_ptr(capture->frame+0x20)!=capture->scope.mode.pointer||
       !runtime_guest_release_scope(s->base,capture->scope.mode.pointer,&current)||
       !runtime_guest_release_same(&capture->scope,&current)||!runtime_guest_release_held(s->base,&current,&fire,&kind)||
       !read_at(s->release.function.pointer+0x7c,&script,sizeof script)||
       !runtime_guest_release_completed(1,1,runtime_delayed_fire_offset(read_ptr(capture->frame+0x28),script.data,36),
           1,fire,capture->kind,kind)||
       guest_equipment_find_first(s->base,read_ptr(current.fire.lease.pawn.pointer+0x50),"StopFire")!=s->stop.function.pointer||
       !guest_equipment_dispatch_pins(s->base))return;
    uintptr_t pawn=current.fire.lease.pawn.pointer,pe=read_ptr(read_ptr(pawn)+0x230);
    if(pe!=s->base+0x40d450)return;
    if(kind==2){
        uintptr_t weapon=current.fire.lease.weapon.pointer,state=0,weapon_pe=read_ptr(read_ptr(weapon)+0x230);uint32_t state_name[2],timer_name[2];
        GuestCameraStamp stamp;GuestPlayerContextMethod clear,check;
        if(!native_state_fields(weapon,&state,state_name)||!guest_camera_stamp_read(s->base,state,&stamp,1)||
           strcmp(stamp.full_name,"SFXGame.SFXWeapon.Active")||!runtime_delayed_fire_pins(s->base)||
           !guest_equipment_weapon_pe_valid(s->base,weapon,weapon_pe)||
           !runtime_delayed_fire_timer(s->base,weapon,state,&clear,&check,timer_name)||
           !guest_player_context_method_matches(s->base,&clear,1)||!guest_player_context_method_matches(s->base,&check,1)||
           !guest_camera_stamp_matches(s->base,&stamp,1))return;
        s->busy=1;int cleared=runtime_delayed_fire_dispatch(1,(ProcessEvent)weapon_pe,weapon,clear.function.pointer,timer_name);s->busy=0;
        RuntimeGuestReleaseScope refreshed;
        if(!cleared||!runtime_guest_release_scope(s->base,capture->scope.mode.pointer,&refreshed)||
           !runtime_guest_release_same(&capture->scope,&refreshed)||!runtime_guest_release_held(s->base,&refreshed,&fire,&kind)||
           fire||kind!=2||guest_equipment_find_first(s->base,read_ptr(pawn+0x50),"StopFire")!=s->stop.function.pointer||
           !runtime_guest_release_method(s->base,&s->stop,s->base+0xf52b0,0)||read_ptr(read_ptr(pawn)+0x230)!=pe)return;
        if(s->rows++<32)line("{\"runtime_guest_fire_release\":\"stock-local-pending-timer-clear-dispatched\",\"real_input_release_forwarded\":true,\"pending_mode_written\":false,\"ammo_written\":false,\"gameplay_verified\":false}\r\n");
    }
    s->busy=1;int called=runtime_guest_release_dispatch(1,(ProcessEvent)pe,pawn,s->stop.function.pointer,2);s->busy=0;
    if(called&&s->rows++<32)
        line("{\"runtime_guest_fire_release\":\"stock-pawn-stop-dispatched\",\"real_input_release_forwarded\":true,\"fire_mode\":2,\"ammo_written\":false,\"gameplay_verified\":false}\r\n");
}
static void runtime_guest_release_forward(void *self,void *frame,void *result,RuntimeGuestReleaseExec stock,
        RuntimeGuestReleaseCapture (*capture)(void*,void*),void (*after)(RuntimeGuestReleaseCapture*)) {
    DWORD error=GetLastError();RuntimeGuestReleaseCapture c=capture(self,frame);
    SetLastError(error);stock(self,frame,result);error=GetLastError();after(&c);SetLastError(error);
}
static void runtime_guest_fire_release_exec(void *self,void *frame,void *result) {
    runtime_guest_release_forward(self,frame,result,runtime_guest_release.original,runtime_guest_release_capture,runtime_guest_release_after);
}
static void runtime_guest_fire_release_prepare(uintptr_t base,uintptr_t engine,DWORD now) {
    RuntimeGuestRelease *s=&runtime_guest_release;
    if(GetCurrentThreadId()!=session_game_thread||g_runtime_config.role!=ME3_ROLE_GUEST||
       !runtime_guest_fire_release_enabled()||s->installed||(s->checked&&now-s->checked<2000))return;
    s->checked=now;SessionContext c=session_context(engine);GameArray modes={0};
    if(c.mode||!runtime_guest_weapon.valid||!runtime_control_enabled||!runtime_scope_ready||session_engine_blocked(c)||
       !narrative_probe_object(base,c.manager,"SFXGameModeManager")||!identity_array(c.manager+0x70,&modes,64)||modes.count<=0)return;
    uintptr_t receiver=read_ptr(modes.data);RuntimeGuestReleaseScope scope;
    uintptr_t release=guest_player_context_find_method(base,read_ptr(receiver+0x50),"SFXGameModeBase","StopFiringWeapon");
    uintptr_t stop=guest_player_context_find_method(base,read_ptr(c.pawn+0x50),"Pawn","StopFire");
    if(!runtime_guest_release_scope(base,receiver,&scope)||!supported_image(base)||!guest_equipment_dispatch_pins(base)||
       !guest_player_context_method_stamp(base,release,&s->release)||!guest_player_context_method_stamp(base,stop,&s->stop)||
       !runtime_guest_release_method(base,&s->release,base+0xf52b0,1)||!runtime_guest_release_method(base,&s->stop,base+0xf52b0,0)||
       !runtime_guest_release_bodies(base,release,stop)||
       guest_equipment_find_first(base,read_ptr(c.pawn+0x50),"StopFire")!=stop||
       guest_equipment_find_first(base,read_ptr(receiver+0x50),"StopFiringWeapon")!=release||
       read_ptr(read_ptr(c.pawn)+0x230)!=base+0x40d450||!background_focus_writable(release+0xd8,8))return;
    s->base=base;s->original=(RuntimeGuestReleaseExec)(base+0xf52b0);
    if(InterlockedCompareExchangePointer((PVOID volatile*)(release+0xd8),(PVOID)runtime_guest_fire_release_exec,
        (PVOID)s->original)!=(PVOID)s->original)return;
    s->installed=1;
    line("{\"runtime_guest_fire_release\":\"stock-release-wrapper-installed\",\"full_auto_only\":true,\"ammo_written\":false,\"gameplay_verified\":false}\r\n");
}
#ifdef ME3_RUNTIME_FIXTURE
static void *runtime_release_fixture_args[3];static unsigned runtime_release_fixture_calls,runtime_release_fixture_order;
static int runtime_release_fixture_bad,runtime_release_fixture_valid;
static RuntimeGuestReleaseCapture runtime_release_fixture_capture(void *self,void *frame) {
    RuntimeGuestReleaseCapture c={0};c.valid=runtime_release_fixture_valid;
    if(self!=runtime_release_fixture_args[0]||frame!=runtime_release_fixture_args[1]||runtime_release_fixture_order++)runtime_release_fixture_bad=1;
    SetLastError(11);return c;
}
static void runtime_release_fixture_stock(void *self,void *frame,void *result) {
    if(self!=runtime_release_fixture_args[0]||frame!=runtime_release_fixture_args[1]||result!=runtime_release_fixture_args[2]||
       GetLastError()!=37||runtime_release_fixture_order++!=1)runtime_release_fixture_bad=1;
    ++runtime_release_fixture_calls;SetLastError(53);
}
static void runtime_release_fixture_after(RuntimeGuestReleaseCapture *c) {
    if(c->valid!=runtime_release_fixture_valid||GetLastError()!=53||runtime_release_fixture_order++!=2)runtime_release_fixture_bad=1;
    SetLastError(79);
}
static void runtime_release_fixture_dispatch(void *self,void *method,void *parms,void *result) {
    if((uintptr_t)self!=123||(uintptr_t)method!=456||!parms||*(unsigned char*)parms!=2||result)runtime_release_fixture_bad=1;
    ++runtime_release_fixture_calls;SetLastError(97);
}
static int runtime_guest_fire_release_selftest(void) {
    DWORD saved=GetLastError();int a,b,c,ok=1;
    runtime_release_fixture_args[0]=&a;runtime_release_fixture_args[1]=&b;runtime_release_fixture_args[2]=&c;
    runtime_release_fixture_bad=0;
    for(int valid=0;valid<2;++valid) {
        runtime_release_fixture_valid=valid;runtime_release_fixture_calls=runtime_release_fixture_order=0;SetLastError(37);
        runtime_guest_release_forward(&a,&b,&c,runtime_release_fixture_stock,runtime_release_fixture_capture,runtime_release_fixture_after);
        ok=ok&&runtime_release_fixture_calls==1&&runtime_release_fixture_order==3&&!runtime_release_fixture_bad&&GetLastError()==53;
    }
    ok=ok&&runtime_guest_release_kind(2,2,2,1,0,1)==1&&
        runtime_guest_release_kind(2,0,2,0,2,2)==2&&runtime_guest_release_kind(2,0,2,1,2,2)==2&&
        !runtime_guest_release_kind(1,0,2,0,2,2)&&!runtime_guest_release_kind(3,0,2,0,2,2)&&
        !runtime_guest_release_kind(2,0,1,0,2,2)&&!runtime_guest_release_kind(2,2,2,0,0,1)&&
        !runtime_guest_release_kind(2,1,2,0,2,2)&&!runtime_guest_release_kind(2,0,2,-1,2,2)&&
        !runtime_guest_release_kind(2,0,2,2,2,2)&&!runtime_guest_release_kind(2,0,2,0,0,2)&&
        !runtime_guest_release_kind(2,0,2,0,1,2)&&!runtime_guest_release_kind(2,0,2,0,3,2)&&
        !runtime_guest_release_kind(2,0,2,0,4,2)&&!runtime_guest_release_kind(2,0,2,0,2,0)&&
        runtime_guest_release_completed(1,1,35,1,0,1,1)&&runtime_guest_release_completed(1,1,35,1,0,2,2)&&
        !runtime_guest_release_completed(1,1,35,1,0,1,2)&&!runtime_guest_release_completed(1,1,35,1,0,2,1)&&
        !runtime_guest_release_completed(0,1,35,1,0,2,2)&&!runtime_guest_release_completed(1,0,35,1,0,2,2)&&
        !runtime_guest_release_completed(1,1,34,1,0,2,2)&&!runtime_guest_release_completed(1,1,35,0,0,2,2)&&
        !runtime_guest_release_completed(1,1,35,1,1,2,2)&&!runtime_guest_release_completed(1,1,35,1,0,0,0)&&
        !runtime_guest_release_completed(1,1,35,1,0,3,3);
    int32_t pending=-1,pending_cells[3]={0,0,1};GameArray pending_array={0,0,0};
    ok=ok&&runtime_guest_release_pending(&pending_array,&pending)&&!pending;
    pending_array=(GameArray){(uintptr_t)pending_cells,2,3};pending=7;
    ok=ok&&runtime_guest_release_pending(&pending_array,&pending)&&!pending;
    pending_array.count=3;
    ok=ok&&runtime_guest_release_pending(&pending_array,&pending)&&pending==1;
    pending_array.count=-1;ok=ok&&!runtime_guest_release_pending(&pending_array,&pending);
    runtime_release_fixture_calls=0;SetLastError(43);
    ok=ok&&!runtime_guest_release_dispatch(0,runtime_release_fixture_dispatch,123,456,2)&&!runtime_release_fixture_calls&&
        runtime_guest_release_dispatch(1,runtime_release_fixture_dispatch,123,456,2)&&runtime_release_fixture_calls==1&&GetLastError()==43&&
        !runtime_guest_release_dispatch(1,runtime_release_fixture_dispatch,123,456,1)&&
        !runtime_guest_release_dispatch(1,runtime_release_fixture_dispatch,123,456,3)&&runtime_release_fixture_calls==1;
    RuntimeGuestReleaseScope first={0},second;
    first.fire.token=1;first.fire.side=ME3_ROLE_GUEST;SessionHandle h={1,2,3};
    first.fire.lease.pc=first.fire.lease.pawn=first.fire.lease.weapon=first.fire.lease.inventory=first.fire.lease.world=
        first.fire.lease.root=first.fire.lease.klass=first.fire.lease.cdo=first.fire.lease.damage_field=first.fire.lease.mode_field=
        first.fire.lease.pawn_channel=first.fire.lease.weapon_channel=first.fire.lease.connection=first.fire.lease.driver=h;
    first.manager=first.mode=h;first.modes=(GameArray){1,8,8};second=first;
    ok=ok&&runtime_guest_release_same(&first,&second);second.mode.index++;ok=ok&&!runtime_guest_release_same(&first,&second);
    second=first;second.fire.token++;ok=ok&&!runtime_guest_release_same(&first,&second);
    second=first;second.modes.data++;ok=ok&&!runtime_guest_release_same(&first,&second);
    ok=ok&&!runtime_release_fixture_bad;SetLastError(saved);return ok;
}
#endif
#endif
