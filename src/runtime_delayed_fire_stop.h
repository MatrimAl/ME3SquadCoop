#ifndef ME3_RUNTIME_DELAYED_FIRE_STOP_H
#define ME3_RUNTIME_DELAYED_FIRE_STOP_H
/* Ordinary host candidate: a real incoming stop must complete unchanged before
 * cancelling the stock Active.CheckBeginFire timer left by port-arms delay.
 * No sampled input substitutes for that RPC and no fire/ammo state is written. */
typedef void (*RuntimeDelayedFireExec)(void*,void*,void*);
typedef struct RuntimeDelayedFireParms {uint32_t name[2];uintptr_t target;} RuntimeDelayedFireParms;
_Static_assert(sizeof(RuntimeDelayedFireParms)==16,"Actor.ClearTimer ABI16");
typedef struct RuntimeDelayedFireScope {
    GuestCameraStamp objects[9]; /* weapon, pawn, remote PC, info, root, driver, connection, channel, inventory */
    NetworkActorScope world;
    NetworkActorIdentityEntry channel;
    uint32_t epoch;
} RuntimeDelayedFireScope;
typedef struct RuntimeDelayedFireCapture {
    RuntimeDelayedFireScope scope;
    NativeRpcReceiveFrame *receive;
    uintptr_t frame;
    int valid;
    unsigned mode;
} RuntimeDelayedFireCapture;
static GuestPlayerContextMethod runtime_delayed_fire_method;
static RuntimeDelayedFireExec runtime_delayed_fire_original;
static uintptr_t runtime_delayed_fire_base;
static int runtime_delayed_fire_installed,runtime_delayed_fire_busy;
static unsigned runtime_delayed_fire_rows;
static int runtime_delayed_fire_stop_enabled(void) {
    static int enabled=-1;
    if(enabled<0){wchar_t value[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_DELAYED_FIRE_STOP",value,8);
        enabled=!n||(n==1&&value[0]==L'1');}
    return enabled;
}
static int runtime_delayed_fire_decision(int captured,int same,int active,int cursor,
        unsigned requested,unsigned default_mode,unsigned pending,unsigned current) {
    return captured&&same&&active&&cursor==228&&requested==2&&default_mode==2&&pending==2&&!current;
}
static int runtime_delayed_fire_dispatch(int eligible,ProcessEvent pe,uintptr_t weapon,uintptr_t method,const uint32_t name[2]) {
    DWORD error=GetLastError();int called=0;
    if(eligible&&pe&&weapon&&method&&name[0]&&!name[1]) {
        RuntimeDelayedFireParms p={{name[0],name[1]},0};pe((void*)weapon,(void*)method,&p,NULL);called=1;
    }
    SetLastError(error);return called;
}
static int runtime_delayed_fire_offset(uintptr_t code,uintptr_t script,int length) {
    if(!code||!script||length<0||length>4096||script>UINTPTR_MAX-(uintptr_t)length||
       code<script||code>script+(uintptr_t)length)return -1;
    return (int)(code-script);
}
static int runtime_delayed_fire_property(uintptr_t base,uintptr_t property,const char *kind,
        const char *path,int offset,int size,uint64_t flags) {
    GuestCameraStamp stamp;int32_t actual=-1,bytes=0,dim=0;uint64_t pf=0;
    return narrative_probe_object(base,property,kind)&&guest_camera_stamp_read(base,property,&stamp,1)&&
        !strcmp(stamp.full_name,path)&&read_at(property+0x94,&actual,4)&&actual==offset&&
        read_at(property+0x74,&bytes,4)&&bytes==size&&read_at(property+0x70,&dim,4)&&dim==1&&
        read_at(property+0x78,&pf,8)&&pf==flags;
}
static int runtime_delayed_fire_contract(uintptr_t base,const GuestPlayerContextMethod *method,uintptr_t expected) {
    uintptr_t fn=method->function.pointer,property=read_ptr(fn+0x70);GameArray script={0};
    uint32_t flags=0;uint16_t bytes=0,ret=0,index=1;unsigned char count=0;
    return guest_player_context_method_matches(base,method,1)&&
        !strcmp(method->function.full_name,"SFXGame.SFXWeapon.ServerStopFire")&&
        read_at(fn+0xe0,&flags,4)&&flags==0x2200c2&&read_at(fn+0xe4,&index,2)&&!index&&
        read_at(fn+0xe6,&bytes,2)&&bytes==1&&read_at(fn+0xe8,&ret,2)&&ret==65535&&
        read_at(fn+0xea,&count,1)&&count==1&&read_ptr(fn+0xd8)==expected&&
        read_at(fn+0x7c,&script,sizeof script)&&script.data&&script.count==229&&script.max>=229&&script.max<=4096&&
        read_ptr(property+0x40)==fn&&!read_ptr(property+0x68)&&
        runtime_delayed_fire_property(base,property,"ByteProperty","SFXGame.SFXWeapon.ServerStopFire.FireModeNum",0,1,0x80);
}
static int runtime_delayed_fire_pins(uintptr_t base) {
    static const unsigned char wrapper[32]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x18,0x57,0x48,0x83,0xec,0x20,0x48,0x8b,0xf9,0x48,0x8b,0xda,0x48,0x8b,0x0d,0x64,0x9f,0x2e,0x01,0xe8,0xdf,0x87,0xbd};
    static const unsigned char native[32]={0x48,0x89,0x5c,0x24,0x08,0x4d,0x85,0xc0,0x48,0x89,0x54,0x24,0x10,0x4c,0x8b,0xd1,0x4c,0x8b,0xc9,0x4d,0x0f,0x45,0xd0,0x33,0xdb,0x44,0x8b,0xc3,0x39,0x99,0xa8,0x00};
    unsigned char bytes[32],call[5];int32_t rel=0;
    if(!read_at(base+0x4bdbe0,bytes,32)||memcmp(bytes,wrapper,32)||
       !read_at(base+0x702120,bytes,32)||memcmp(bytes,native,32)||
       !read_at(base+0x4bdca1,call,5)||call[0]!=0xe8)return 0;
    memcpy(&rel,call+1,4);return base+0x4bdca6+rel==base+0x702120;
}
static int runtime_delayed_fire_timer(uintptr_t base,uintptr_t weapon,uintptr_t state,
        GuestPlayerContextMethod *clear,GuestPlayerContextMethod *check,uint32_t name[2]) {
    uintptr_t fn=guest_player_context_find_method(base,read_ptr(weapon+0x50),"Actor","ClearTimer");
    if(!guest_player_context_method_stamp(base,fn,clear)||
       !guest_player_context_function_abi(base,fn,"Actor","ClearTimer",0x24401,16,65535,0,0x4bdbe0,2)||
       strcmp(clear->function.full_name,"Engine.Actor.ClearTimer"))return 0;
    uintptr_t prop=read_ptr(fn+0x70),obj=read_ptr(prop+0x68);
    if(read_ptr(prop+0x40)!=fn||read_ptr(obj+0x40)!=fn||read_ptr(obj+0x68)||
       !runtime_delayed_fire_property(base,prop,"NameProperty","Engine.Actor.ClearTimer.inTimerFunc",0,8,0x90)||
       !runtime_delayed_fire_property(base,obj,"ObjectProperty","Engine.Actor.ClearTimer.inObj",8,8,0x90))return 0;
    uintptr_t child=read_ptr(state+0x70),seen[256];unsigned count=0;fn=0;
    while(child&&count<256) {
        if(!is_live(base,child)||read_ptr(child+0x40)!=state)return 0;
        for(unsigned i=0;i<count;++i)if(seen[i]==child)return 0;seen[count++]=child;
        char text[96];object_name(base,child,text,sizeof text);
        if(!strcmp(text,"CheckBeginFire")){if(fn)return 0;fn=child;}
        child=read_ptr(child+0x68);
    }
    if(child||!fn||!narrative_probe_object(base,fn,"Function")||!guest_player_context_method_stamp(base,fn,check)||
       strcmp(check->function.full_name,"SFXGame.SFXWeapon.Active.CheckBeginFire")||check->owner.pointer!=state)return 0;
    uint32_t flags=0;uint16_t bytes=1,ret=0,index=1;unsigned char num=1;GameArray script={0};
    return read_at(fn+0xe0,&flags,4)&&flags==0x20102&&read_at(fn+0xe4,&index,2)&&!index&&
        read_at(fn+0xe6,&bytes,2)&&!bytes&&read_at(fn+0xe8,&ret,2)&&ret==65535&&
        read_at(fn+0xea,&num,1)&&!num&&read_ptr(fn+0xd8)==base+0xf52b0&&
        read_at(fn+0x7c,&script,sizeof script)&&script.data&&script.count==99&&script.max>=99&&script.max<=4096&&
        read_at(fn+0x48,name,8)&&name[0]&&!name[1];
}
static int runtime_delayed_fire_scope(uintptr_t base,uintptr_t weapon,uintptr_t channel,RuntimeDelayedFireScope *out) {
    memset(out,0,sizeof *out);SessionContext host=session_context(session_engine);uint32_t life=0;unsigned char role=0;
    if(!runtime_delayed_fire_stop_enabled()||runtime_delayed_fire_busy||GetCurrentThreadId()!=session_game_thread||
       !runtime_control_enabled||!runtime_scope_ready||g_config_status!=ME3_CONFIG_READY||!InterlockedCompareExchange(&g_enable_requested,0,0)||
       g_runtime_config.role!=ME3_ROLE_HOST||!session_active||session_releasing||session_fault||session_cleanup_pending||
       session_pending_reason||session_travel_waiting||!session_epoch||session_engine_blocked(host)||
       host.pc!=session_host||host.pawn!=session_shepard||host.world!=session_world||!session_story_valid(base)||
       !session_handle_matches(session_pc_handle)||session_pc_handle.pointer!=session_remote||
       !session_handle_matches(session_pawn_handle)||session_pawn_handle.pointer!=session_pawn||
       !session_handle_matches(session_world_handle)||session_world_handle.pointer!=session_world||
       !session_handle_matches(session_connection_handle)||session_connection_handle.pointer!=session_connection||
       !session_peer_connected(base,session_remote,session_world)||
       !guest_camera_actor_live(base,session_pawn)||!is_kind_of(base,session_pawn,"SFXPawn_Ashley")||
       read_ptr(session_remote+0x3bc)!=session_pawn||read_ptr(session_pawn+0x374)!=session_remote||
       read_ptr(session_remote+0x67c)!=session_connection||read_ptr(session_connection+0x68)!=session_remote||
       !read_at(session_pawn+0xe20,&life,4)||(life&0x300)||
       !guest_camera_actor_live(base,weapon)||!is_kind_of(base,weapon,"SFXWeapon")||
       !read_at(weapon+0x26a,&role,1)||role!=3||!network_actor_layout_matches(base)||
       !network_actor_read_scope(base,weapon,&out->world)||out->world.info!=session_world||
       !network_actor_channel_valid(base,weapon,channel,out->world.driver,&out->channel)||
       out->channel.connection!=session_connection)return 0;
    uintptr_t inventory=read_ptr(session_pawn+0x414);
    if(!guest_camera_actor_live(base,inventory)||!is_kind_of(base,inventory,"SFXInventoryManager")||
       read_ptr(session_pawn+0x41c)!=weapon||read_ptr(weapon+0x180)!=session_pawn||
       read_ptr(weapon+0x190)!=session_pawn||read_ptr(weapon+0x2b0)!=inventory||
       read_ptr(inventory+0x180)!=session_pawn||read_ptr(inventory+0x190)!=session_pawn)return 0;
    uintptr_t objects[]={weapon,session_pawn,session_remote,session_world,out->world.world,
        out->world.driver,session_connection,channel,inventory};
    for(unsigned i=0;i<9;++i)if(!guest_camera_stamp_read(base,objects[i],&out->objects[i],1))return 0;
    NetworkActorScope again;NetworkActorIdentityEntry checked={0};
    if(!network_actor_read_scope(base,weapon,&again)||memcmp(&out->world,&again,sizeof again)||
       !network_actor_channel_valid(base,weapon,channel,again.driver,&checked)||
       memcmp(&out->channel,&checked,sizeof checked)||!session_story_valid(base)||
       read_ptr(session_pawn+0x41c)!=weapon||read_ptr(session_pawn+0x414)!=inventory||
       read_ptr(weapon+0x180)!=session_pawn||read_ptr(weapon+0x190)!=session_pawn||read_ptr(weapon+0x2b0)!=inventory||
       read_ptr(session_remote+0x3bc)!=session_pawn||read_ptr(session_pawn+0x374)!=session_remote||
       read_ptr(session_remote+0x67c)!=session_connection||read_ptr(session_connection+0x68)!=session_remote||
       read_ptr(inventory+0x180)!=session_pawn||read_ptr(inventory+0x190)!=session_pawn||
       !read_at(session_pawn+0xe20,&life,4)||(life&0x300))return 0;
    for(unsigned i=0;i<9;++i)if(!guest_camera_stamp_matches(base,&out->objects[i],1))return 0;
    out->epoch=session_epoch;return 1;
}
static void runtime_delayed_fire_exec(void *self,void *frame,void *result);
static RuntimeDelayedFireCapture runtime_delayed_fire_capture(void *self,void *frame) {
    RuntimeDelayedFireCapture c={0};uintptr_t base=runtime_delayed_fire_base;
    NativeRpcReceiveFrame *receive=native_rpc_receive_current(session_game_thread);GameArray script={0};unsigned char mode=0;
    if(!runtime_delayed_fire_installed||!receive||!receive->channel||
       !runtime_delayed_fire_contract(base,&runtime_delayed_fire_method,(uintptr_t)runtime_delayed_fire_exec)||
       read_ptr((uintptr_t)frame+0x18)!=runtime_delayed_fire_method.function.pointer||
       read_ptr((uintptr_t)frame+0x20)!=(uintptr_t)self||
       !read_at(runtime_delayed_fire_method.function.pointer+0x7c,&script,sizeof script)||
       runtime_delayed_fire_offset(read_ptr((uintptr_t)frame+0x28),script.data,229)!=0||
       !read_at(read_ptr((uintptr_t)frame+0x30),&mode,1)||mode!=2||
       !runtime_delayed_fire_scope(base,(uintptr_t)self,receive->channel,&c.scope))return c;
    c.valid=1;c.frame=(uintptr_t)frame;c.receive=receive;c.mode=mode;return c;
}
static void runtime_delayed_fire_after(RuntimeDelayedFireCapture *c) {
    if(!c->valid)return;
    uintptr_t base=runtime_delayed_fire_base,weapon=c->scope.objects[0].pointer;
    NativeRpcReceiveFrame *receive=native_rpc_receive_current(session_game_thread);RuntimeDelayedFireScope current;
    if(receive!=c->receive||!receive||receive->channel!=c->scope.channel.channel||
       !runtime_delayed_fire_scope(base,weapon,receive->channel,&current)||memcmp(&current,&c->scope,sizeof current)||
       !runtime_delayed_fire_contract(base,&runtime_delayed_fire_method,(uintptr_t)runtime_delayed_fire_exec)||
       read_ptr(c->frame+0x18)!=runtime_delayed_fire_method.function.pointer||read_ptr(c->frame+0x20)!=weapon)return;
    GameArray script={0};uintptr_t state=0;uint32_t state_name[2];GuestCameraStamp stamp;
    unsigned char pending=0,firing=255,mode=0;
    if(!read_at(runtime_delayed_fire_method.function.pointer+0x7c,&script,sizeof script))return;
    int active=native_state_fields(weapon,&state,state_name)&&guest_camera_stamp_read(base,state,&stamp,1)&&
        narrative_probe_object(base,state,"State")&&!strcmp(stamp.full_name,"SFXGame.SFXWeapon.Active");
    if(!read_at(weapon+0x10c2,&pending,1)||!read_at(weapon+0x3b0,&firing,1)||!read_at(weapon+0x10be,&mode,1)||
       !runtime_delayed_fire_decision(1,1,active,
          runtime_delayed_fire_offset(read_ptr(c->frame+0x28),script.data,229),c->mode,mode,pending,firing))return;
    GuestPlayerContextMethod clear,check;uint32_t name[2];uintptr_t pe=read_ptr(read_ptr(weapon)+0x230);
    if(!runtime_delayed_fire_pins(base)||!guest_equipment_dispatch_pins(base)||
       !guest_equipment_weapon_pe_valid(base,weapon,pe)||
       !runtime_delayed_fire_timer(base,weapon,state,&clear,&check,name)||
       !guest_player_context_method_matches(base,&clear,1)||!guest_player_context_method_matches(base,&check,1)||
       !guest_camera_stamp_matches(base,&stamp,1))return;
    runtime_delayed_fire_busy=1;
    int called=runtime_delayed_fire_dispatch(1,(ProcessEvent)pe,weapon,clear.function.pointer,name);
    runtime_delayed_fire_busy=0;
    if(called&&runtime_delayed_fire_rows++<16)
        line("{\"runtime_delayed_fire_stop\":\"stock-check-begin-fire-timer-cleared\",\"native_stop_forwarded\":true,\"gameplay_verified\":false}\r\n");
}
static void runtime_delayed_fire_forward(void *self,void *frame,void *result,RuntimeDelayedFireExec stock,
        RuntimeDelayedFireCapture (*capture)(void*,void*),void (*after)(RuntimeDelayedFireCapture*)) {
    DWORD entry=GetLastError();RuntimeDelayedFireCapture c=capture(self,frame);
    SetLastError(entry);stock(self,frame,result);DWORD error=GetLastError();
    after(&c);SetLastError(error);
}
static void runtime_delayed_fire_exec(void *self,void *frame,void *result) {
    runtime_delayed_fire_forward(self,frame,result,runtime_delayed_fire_original,
        runtime_delayed_fire_capture,runtime_delayed_fire_after);
}
static void runtime_delayed_fire_stop_prepare(uintptr_t base) {
    if(runtime_delayed_fire_installed||!runtime_delayed_fire_stop_enabled()||g_runtime_config.role!=ME3_ROLE_HOST||
       !runtime_control_enabled||!runtime_scope_ready||!session_active||GetCurrentThreadId()!=session_game_thread)return;
    uintptr_t fn=function_named(base,"SFXWeapon","ServerStopFire");GuestPlayerContextMethod method;
    if(!supported_image(base)||!guest_player_context_method_stamp(base,fn,&method)||
       !runtime_delayed_fire_contract(base,&method,base+0xf52b0)||
       !runtime_delayed_fire_pins(base)||!guest_equipment_dispatch_pins(base))return;
    runtime_delayed_fire_base=base;runtime_delayed_fire_method=method;
    runtime_delayed_fire_original=(RuntimeDelayedFireExec)(base+0xf52b0);
    if(InterlockedCompareExchangePointer((PVOID volatile*)(fn+0xd8),(PVOID)runtime_delayed_fire_exec,
        (PVOID)runtime_delayed_fire_original)!=(PVOID)runtime_delayed_fire_original)return;
    runtime_delayed_fire_installed=1;
    line("{\"runtime_delayed_fire_stop\":\"native-stop-wrapper-installed\",\"gameplay_verified\":false}\r\n");
}
#ifdef ME3_RUNTIME_FIXTURE
static void *runtime_delayed_fixture_args[3];
static unsigned runtime_delayed_fixture_calls,runtime_delayed_fixture_order;
static int runtime_delayed_fixture_valid,runtime_delayed_fixture_bad;
static RuntimeDelayedFireCapture runtime_delayed_fixture_capture(void *self,void *frame) {
    RuntimeDelayedFireCapture c={0};c.valid=runtime_delayed_fixture_valid;
    if(self!=runtime_delayed_fixture_args[0]||frame!=runtime_delayed_fixture_args[1]||runtime_delayed_fixture_order++)runtime_delayed_fixture_bad=1;
    SetLastError(11);return c;
}
static void runtime_delayed_fixture_stock(void *self,void *frame,void *result) {
    if(self!=runtime_delayed_fixture_args[0]||frame!=runtime_delayed_fixture_args[1]||result!=runtime_delayed_fixture_args[2]||
       GetLastError()!=37||runtime_delayed_fixture_order++!=1)runtime_delayed_fixture_bad=1;
    ++runtime_delayed_fixture_calls;SetLastError(53);
}
static void runtime_delayed_fixture_after(RuntimeDelayedFireCapture *c) {
    if(c->valid!=runtime_delayed_fixture_valid||GetLastError()!=53||runtime_delayed_fixture_order++!=2)runtime_delayed_fixture_bad=1;
    SetLastError(79);
}
static void runtime_delayed_fixture_clear(void *self,void *fn,void *parms,void *result) {
    RuntimeDelayedFireParms *p=parms;
    if((uintptr_t)self!=123||(uintptr_t)fn!=456||result||!p||p->name[0]!=789||p->name[1]||p->target)runtime_delayed_fixture_bad=1;
    ++runtime_delayed_fixture_calls;SetLastError(99);
}
static int runtime_delayed_fire_stop_selftest(void) {
    DWORD saved=GetLastError();int a,b,c,ok=1;
    runtime_delayed_fixture_args[0]=&a;runtime_delayed_fixture_args[1]=&b;runtime_delayed_fixture_args[2]=&c;
    runtime_delayed_fixture_bad=0;
    for(int valid=0;valid<2;++valid) {
        runtime_delayed_fixture_valid=valid;runtime_delayed_fixture_calls=runtime_delayed_fixture_order=0;SetLastError(37);
        runtime_delayed_fire_forward(&a,&b,&c,runtime_delayed_fixture_stock,runtime_delayed_fixture_capture,runtime_delayed_fixture_after);
        ok=ok&&GetLastError()==53&&runtime_delayed_fixture_calls==1&&runtime_delayed_fixture_order==3&&!runtime_delayed_fixture_bad;
    }
    ok=ok&&runtime_delayed_fire_decision(1,1,1,228,2,2,2,0)&&
        !runtime_delayed_fire_decision(0,1,1,228,2,2,2,0)&&!runtime_delayed_fire_decision(1,0,1,228,2,2,2,0)&&
        !runtime_delayed_fire_decision(1,1,0,228,2,2,2,0)&&!runtime_delayed_fire_decision(1,1,1,227,2,2,2,0)&&
        !runtime_delayed_fire_decision(1,1,1,228,1,2,2,0)&&!runtime_delayed_fire_decision(1,1,1,228,2,1,2,0)&&
        !runtime_delayed_fire_decision(1,1,1,228,2,2,0,0)&&!runtime_delayed_fire_decision(1,1,1,228,2,2,2,2);
    uint32_t name[2]={789,0};runtime_delayed_fixture_calls=0;SetLastError(43);
    ok=ok&&!runtime_delayed_fire_dispatch(0,runtime_delayed_fixture_clear,123,456,name)&&!runtime_delayed_fixture_calls&&
        runtime_delayed_fire_dispatch(1,runtime_delayed_fixture_clear,123,456,name)&&runtime_delayed_fixture_calls==1&&GetLastError()==43;
    name[1]=1;ok=ok&&!runtime_delayed_fire_dispatch(1,runtime_delayed_fixture_clear,123,456,name)&&runtime_delayed_fixture_calls==1;
    name[1]=0;name[0]=0;
    ok=ok&&!runtime_delayed_fire_dispatch(1,runtime_delayed_fixture_clear,123,456,name)&&runtime_delayed_fixture_calls==1;
    ok=ok&&!runtime_delayed_fixture_bad;SetLastError(saved);return ok;
}
#endif
#endif
