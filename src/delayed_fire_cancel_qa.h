#ifndef ME3_DELAYED_FIRE_CANCEL_QA_H
#define ME3_DELAYED_FIRE_CANCEL_QA_H
/* Default-off protected QA experiment. Included before fire_rpc_exec, after
 * its method/scope helpers and wrapper-array declaration. Capture/post-step
 * surround the unchanged original call; observer snapshots stay read-only.
 * Eligibility is independent of the observational record budget. */
typedef struct DelayedFireParms { uint32_t name[2];uintptr_t target; } DelayedFireParms;
_Static_assert(sizeof(DelayedFireParms)==16,"Actor.ClearTimer ABI16");
static int delayed_fire_decision(int captured,int same_scope,int active,int cursor,
                                 unsigned requested,unsigned default_mode,unsigned pending,unsigned current) {
    return captured&&same_scope&&active&&cursor==228&&requested==2&&default_mode==2&&pending==2&&!current;
}
static int delayed_fire_dispatch(int eligible,ProcessEvent pe,uintptr_t weapon,uintptr_t fn,const uint32_t name[2]) {
    DWORD error=GetLastError();int called=0;
    if(eligible&&pe&&weapon&&fn&&name[0]&&!name[1]){
        DelayedFireParms parms={{name[0],name[1]},0};pe((void*)weapon,(void*)fn,&parms,NULL);called=1;
    }
    SetLastError(error);return called;
}
#ifndef ME3_DELAYED_FIRE_FIXTURE
/* All dependencies below are the existing runtime's exact-build readers. */
typedef struct DelayedFireContext {
    int valid;uintptr_t frame,receive;unsigned mode;AmmoWireScope scope;
    GuestPlayerContextMethod stop;
} DelayedFireContext;
static int delayed_fire_opted(void) {
    wchar_t value[8]={0};return progress_enabled&&test_action==6&&has_marker(root)&&
        GetEnvironmentVariableW(L"ME3_SQUAD_COOP_DELAYED_FIRE_CANCEL_QA",value,8)==1&&value[0]==L'1';
}
static int delayed_fire_pins(uintptr_t base) {
    static const unsigned char wrapper[32]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x18,0x57,0x48,0x83,0xec,0x20,0x48,0x8b,0xf9,0x48,0x8b,0xda,0x48,0x8b,0x0d,0x64,0x9f,0x2e,0x01,0xe8,0xdf,0x87,0xbd};
    static const unsigned char native[32]={0x48,0x89,0x5c,0x24,0x08,0x4d,0x85,0xc0,0x48,0x89,0x54,0x24,0x10,0x4c,0x8b,0xd1,0x4c,0x8b,0xc9,0x4d,0x0f,0x45,0xd0,0x33,0xdb,0x44,0x8b,0xc3,0x39,0x99,0xa8,0x00};
    unsigned char b[32],call[5];int32_t rel=0;
    if(!read_at(base+0x4bdbe0,b,32)||memcmp(b,wrapper,32)||
       !read_at(base+0x702120,b,32)||memcmp(b,native,32)||!read_at(base+0x4bdca1,call,5)||call[0]!=0xe8)return 0;
    memcpy(&rel,call+1,4);return base+0x4bdca6+rel==base+0x702120;
}
static int delayed_fire_prop(uintptr_t base,uintptr_t p,const char *kind,const char *path,int offset) {
    int dim=0,size=0;return combat_fire_property(base,p,kind,path,offset,0x90)&&
        read_at(p+0x70,&dim,4)&&dim==1&&read_at(p+0x74,&size,4)&&size==8;
}
static int delayed_fire_timer(uintptr_t base,uintptr_t weapon,uintptr_t state,
                              GuestPlayerContextMethod *clear,GuestPlayerContextMethod *check,uint32_t name[2]) {
    uintptr_t fn=guest_player_context_find_method(base,read_ptr(weapon+0x50),"Actor","ClearTimer");
    if(!guest_player_context_method_stamp(base,fn,clear)||
       !guest_player_context_function_abi(base,fn,"Actor","ClearTimer",0x24401,16,65535,0,0x4bdbe0,2)||
       strcmp(clear->function.full_name,"Engine.Actor.ClearTimer"))return 0;
    uintptr_t prop=read_ptr(fn+0x70),obj=read_ptr(prop+0x68);
    if(read_ptr(prop+0x40)!=fn||read_ptr(obj+0x40)!=fn||read_ptr(obj+0x68)||
       !delayed_fire_prop(base,prop,"NameProperty","Engine.Actor.ClearTimer.inTimerFunc",0)||
       !delayed_fire_prop(base,obj,"ObjectProperty","Engine.Actor.ClearTimer.inObj",8))return 0;
    uintptr_t child=read_ptr(state+0x70),seen[256];int count=0;fn=0;
    while(child&&count<256){
        if(!is_live(base,child)||read_ptr(child+0x40)!=state)return 0;
        for(int i=0;i<count;++i)if(seen[i]==child)return 0;seen[count++]=child;
        char text[96];object_name(base,child,text,sizeof text);
        if(!strcmp(text,"CheckBeginFire")){if(fn)return 0;fn=child;}
        child=read_ptr(child+0x68);
    }
    if(child||!fn||!narrative_probe_object(base,fn,"Function")||!guest_player_context_method_stamp(base,fn,check)||
       strcmp(check->function.full_name,"SFXGame.SFXWeapon.Active.CheckBeginFire")||check->owner.pointer!=state)return 0;
    uint32_t flags=0;uint16_t size=1,ret=0,index=1;unsigned char num=1;GameArray script={0};
    return read_at(fn+0xe0,&flags,4)&&flags==0x20102&&read_at(fn+0xe4,&index,2)&&!index&&
        read_at(fn+0xe6,&size,2)&&!size&&read_at(fn+0xe8,&ret,2)&&ret==65535&&
        read_at(fn+0xea,&num,1)&&!num&&read_ptr(fn+0xd8)==base+0xf52b0&&
        read_at(fn+0x7c,&script,sizeof script)&&script.count==99&&script.max>=99&&script.max<=4096&&
        read_at(fn+0x48,name,8)&&name[0]&&!name[1];
}
static DelayedFireContext delayed_fire_capture(int k,void *self,void *frame) {
    DelayedFireContext c={0};AmmoWireScope *s=&ammo_wire_scope;unsigned char mode=0,default_mode=0,role=0;
    if(k!=14||!delayed_fire_opted()||GetCurrentThreadId()!=ammo_wire_thread||!ammo_wire_scope_valid()||
       !ammo_wire_receive||ammo_wire_receive->channel!=s->channel.channel||ammo_wire_receive->generation!=s->generation||
       (uintptr_t)self!=s->weapon.pointer||read_ptr((uintptr_t)frame+0x20)!=(uintptr_t)self||
       read_ptr((uintptr_t)frame+0x18)!=fire_rpc_methods[14].function.pointer||
       !guest_player_context_method_matches(s->base,&fire_rpc_methods[14],1)||
       strcmp(fire_rpc_methods[14].function.full_name,"SFXGame.SFXWeapon.ServerStopFire")||
       !read_at(read_ptr((uintptr_t)frame+0x30),&mode,1)||mode!=2||
       !read_at(s->weapon.pointer+0x10be,&default_mode,1)||default_mode!=2||
       !read_at(s->weapon.pointer+0x26a,&role,1)||role!=3)return c;
    uint32_t flags=0;uint16_t size=0,ret=0,index=1;unsigned char num=0;GameArray script={0};
    uintptr_t fn=fire_rpc_methods[14].function.pointer;
    if(!read_at(fn+0xe0,&flags,4)||flags!=0x2200c2||!read_at(fn+0xe4,&index,2)||index||
       !read_at(fn+0xe6,&size,2)||size!=1||!read_at(fn+0xe8,&ret,2)||ret!=65535||
       !read_at(fn+0xea,&num,1)||num!=1||read_ptr(fn+0xd8)!=(uintptr_t)fire_rpc_wrappers[14]||
       !read_at(fn+0x7c,&script,sizeof script)||script.count!=229||script.max<229||script.max>4096||
       fire_rpc_code_offset(read_ptr((uintptr_t)frame+0x28),script.data,229)!=0)return c;
    c.valid=1;c.frame=(uintptr_t)frame;c.receive=(uintptr_t)ammo_wire_receive;c.mode=mode;c.scope=*s;c.stop=fire_rpc_methods[14];return c;
}
static void delayed_fire_after(DelayedFireContext *c) {
    if(!c->valid)return;AmmoWireScope *s=&ammo_wire_scope;uintptr_t base=c->scope.base;
    int same=delayed_fire_opted()&&GetCurrentThreadId()==ammo_wire_thread&&ammo_wire_scope_valid()&&
        s->weapon.pointer==c->scope.weapon.pointer&&s->pawn.pointer==c->scope.pawn.pointer&&
        s->pc.pointer==c->scope.pc.pointer&&s->world.pointer==c->scope.world.pointer&&
        s->generation==c->scope.generation&&s->epoch==c->scope.epoch&&s->base==base&&s->map==c->scope.map&&
        s->channel.channel==c->scope.channel.channel&&s->channel.connection==c->scope.channel.connection&&
        s->channel.driver==c->scope.channel.driver&&s->channel.index==c->scope.channel.index&&
        (uintptr_t)ammo_wire_receive==c->receive&&ammo_wire_receive->generation==s->generation&&
        ammo_wire_receive->channel==s->channel.channel&&guest_camera_stamp_matches(base,&c->scope.weapon,1)&&
        guest_camera_stamp_matches(base,&c->scope.pawn,1)&&guest_camera_stamp_matches(base,&c->scope.pc,1)&&
        guest_camera_stamp_matches(base,&c->scope.world,1)&&guest_player_context_method_matches(base,&c->stop,1)&&
        read_ptr(c->frame+0x18)==c->stop.function.pointer&&read_ptr(c->frame+0x20)==c->scope.weapon.pointer;
    if(!same)return;
    GameArray script={0};int cursor=-1;uintptr_t weapon=c->scope.weapon.pointer,state=0;uint32_t state_name[2];
    GuestCameraStamp state_stamp;unsigned char pending=0,current=255,default_mode=0;
    if(!read_at(c->stop.function.pointer+0x7c,&script,sizeof script)||script.count!=229||script.max<229||script.max>4096)return;
    cursor=fire_rpc_code_offset(read_ptr(c->frame+0x28),script.data,script.count);
    int active=native_state_fields(weapon,&state,state_name)&&guest_camera_stamp_read(base,state,&state_stamp,1)&&
        narrative_probe_object(base,state,"State")&&!strcmp(state_stamp.full_name,"SFXGame.SFXWeapon.Active");
    if(!read_at(weapon+0x10c2,&pending,1)||!read_at(weapon+0x3b0,&current,1)||
       !read_at(weapon+0x10be,&default_mode,1)||!delayed_fire_decision(c->valid,same,active,cursor,c->mode,default_mode,pending,current))return;
    line("{\"delayed_fire_cancel_qa\":\"eligible-pending-release\",\"timer\":\"CheckBeginFire\",\"gameplay_verified\":false}\r\n");
    GuestPlayerContextMethod clear,check;uint32_t name[2];uintptr_t pe=read_ptr(read_ptr(weapon)+0x230);
    if(!delayed_fire_pins(base)||!guest_equipment_dispatch_pins(base)||!guest_equipment_weapon_pe_valid(base,weapon,pe)||
       !delayed_fire_timer(base,weapon,state,&clear,&check,name)||
       !guest_player_context_method_matches(base,&clear,1)||!guest_player_context_method_matches(base,&check,1)){
        line("{\"delayed_fire_cancel_qa\":\"binding-rejected\",\"gameplay_verified\":false}\r\n");return;}
    if(delayed_fire_dispatch(1,(ProcessEvent)pe,weapon,clear.function.pointer,name))
        line("{\"delayed_fire_cancel_qa\":\"ordinary-clear-timer-dispatched\",\"timer\":\"CheckBeginFire\",\"gameplay_verified\":false}\r\n");
}
#endif
/* Stock call is unconditional and exactly once; side effects preserve its error.
 * Fixture substitutes only engine readers/capture, not this ordering/helper. */
static void delayed_fire_exec_stock(int k,void *self,void *frame,void *result,FireRpcExec stock) {
    DWORD before=GetLastError();DelayedFireContext c=delayed_fire_capture(k,self,frame);
    SetLastError(before);stock(self,frame,result);DWORD after=GetLastError();
    delayed_fire_after(&c);SetLastError(after);
}
#endif
