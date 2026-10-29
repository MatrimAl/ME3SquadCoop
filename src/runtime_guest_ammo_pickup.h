#ifndef ME3_RUNTIME_GUEST_AMMO_PICKUP_H
#define ME3_RUNTIME_GUEST_AMMO_PICKUP_H
/* Human Ashley may use the stock dropped-ammo predicate even when her NPC
 * pickup flag is false. Select its own CanPickUpAmmo return, keeping native
 * capacity, quantity, Touch and authority rules. Never enable general pickups,
 * synthesize a touch, call GiveTo/AddAmmo or modify ammunition/return values. */
typedef void (*RuntimeAmmoPickupExec)(void*,void*,void*);
static int runtime_dialogue_node_match(uintptr_t);
static void runtime_dialogue_forward(void*,void*,void*,RuntimeAmmoPickupExec);
typedef struct RuntimeAmmoPickupScope {
    RuntimeMovementScope owner;
    GuestCameraStamp pickup,state,cdo;
} RuntimeAmmoPickupScope;
typedef struct RuntimeAmmoPickup {
    uintptr_t base;
    GuestPlayerContextMethod valid,can;
    GuestCameraStamp player_class,player_cdo;
    RuntimeAmmoPickupExec original;
    unsigned char original_entry[16],hook_entry[16],trampoline_entry[64];
    DWORD checked;
    unsigned rows;
    int installed,prepared;
} RuntimeAmmoPickup;
static RuntimeAmmoPickup runtime_ammo_pickup;
static uintptr_t runtime_ammo_award_node;
static uintptr_t runtime_scene_camera_node;
static uintptr_t runtime_scene_camera_fov_node;
static void runtime_scene_camera_forward(void*,void*,void*,RuntimeAmmoPickupExec);
static void runtime_scene_camera_fov_forward(void*,void*,void*,RuntimeAmmoPickupExec);
static void runtime_ammo_award_before(void*,void*);
static void runtime_ammo_pickup_exec(void*,void*,void*);
static int runtime_script_executor_prefix(uintptr_t base,const unsigned char *expected,size_t size) {
    RuntimeAmmoPickup *s=&runtime_ammo_pickup;unsigned char actual[256],prefix[16],trampoline[64];
    if(size>sizeof actual)return 0;
    if(!s->installed)return read_at(base+0xf52b0,actual,size)&&!memcmp(actual,expected,size);
    if(base!=s->base||!s->original||size<16||!read_at(base+0xf52b0,prefix,16)||memcmp(prefix,s->hook_entry,16)||
       !read_at((uintptr_t)s->original,trampoline,sizeof trampoline)||memcmp(trampoline,s->trampoline_entry,sizeof trampoline))return 0;
    memcpy(actual,s->original_entry,16);
    return read_at(base+0xf52c0,actual+16,size-16)&&!memcmp(actual,expected,size);
}
#include "runtime_guest_ammo_pickup_pins.h"
static int runtime_ammo_pickup_enabled(void) {
    static int enabled=-1;if(enabled<0){wchar_t text[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_GUEST_AMMO_PICKUP",text,8);
        enabled=!n||(n==1&&text[0]==L'1');}return enabled;
}
static int runtime_ammo_pickup_policy(int owned,int active,int disabled,int flag,int default_flag,int player_default,int controller) {
    return owned&&active&&!disabled&&!flag&&!default_flag&&player_default&&controller;
}
static int runtime_ammo_pickup_owner(uintptr_t base,RuntimeMovementScope *out) {
    SessionContext c=session_context(session_engine);NetworkActorScope world={0};NetworkActorIdentityEntry channel={0};uint32_t life=0;
    memset(out,0,sizeof *out);
    if(!runtime_ammo_pickup_enabled()||GetCurrentThreadId()!=session_game_thread||!runtime_control_enabled||!runtime_scope_ready||
       !InterlockedCompareExchange(&g_enable_requested,0,0)||session_engine_blocked(c))return 0;
    if(g_runtime_config.role==ME3_ROLE_HOST) {
        if(!session_active||session_fault||session_releasing||session_cleanup_pending||session_pending_reason||
           !session_story_valid(base)||c.world!=session_world||!session_handle_matches(session_pc_handle)||
           !session_handle_matches(session_pawn_handle)||!session_handle_matches(session_ai_handle)||
           !session_peer_connected(base,session_remote,c.world))return 0;
        c.pc=session_remote;c.pawn=session_pawn;out->token=session_epoch;
    } else if(g_runtime_config.role==ME3_ROLE_GUEST) {
        if(!guest_vitals_owned(base,c))return 0;out->token=guest_player_context_registration_token();
    } else return 0;
    if(!out->token||!guest_camera_actor_live(base,c.pc)||!is_kind_of(base,c.pc,"SFXPlayerController")||
       !guest_camera_actor_live(base,c.pawn)||!is_kind_of(base,c.pawn,"SFXPawn_Ashley")||
       read_ptr(c.pc+0x3bc)!=c.pawn||read_ptr(c.pawn+0x374)!=c.pc||read_ptr(c.pc+0x198)!=c.world||
       !read_at(c.pawn+0xe20,&life,4)||!(life&0x10)||(life&0x300)||
       !network_actor_read_scope(base,c.pawn,&world)||world.info!=c.world||
       !guest_player_context_channel(base,c.pawn,&channel)||channel.index<=0||channel.driver!=world.driver||
       (g_runtime_config.role==ME3_ROLE_HOST&&channel.connection!=read_ptr(c.pc+0x67c)))return 0;
    out->pc=session_capture_handle(c.pc);out->pawn=session_capture_handle(c.pawn);out->info=session_capture_handle(c.world);
    out->world=session_capture_handle(world.world);out->driver=session_capture_handle(world.driver);
    out->connection=session_capture_handle(channel.connection);out->channel=session_capture_handle(channel.channel);
    SessionHandle *handles[]={&out->pc,&out->pawn,&out->info,&out->world,&out->driver,&out->connection,&out->channel};
    for(unsigned i=0;i<7;++i)if(!session_handle_matches(*handles[i]))return 0;return 1;
}
static int runtime_ammo_pickup_bit(uintptr_t base,uintptr_t object,const char *owner,const char *name,int offset,uint64_t flags) {
    uintptr_t prop=runtime_guest_team_field(base,object,owner,name,"BoolProperty",offset,4,flags);uint64_t mask=0;
    return prop&&read_at(prop+0xd0,&mask,8)&&mask==0x40;
}
static int runtime_ammo_pickup_scope(uintptr_t base,uintptr_t pickup,uintptr_t other,RuntimeAmmoPickupScope *out) {
    RuntimeAmmoPickup *s=&runtime_ammo_pickup;uint32_t disabled=0,flag=0,default_flag=0,player_flag=0,name[2];uintptr_t state=0;
    memset(out,0,sizeof *out);
    if(!runtime_ammo_pickup_owner(base,&out->owner)||out->owner.pawn.pointer!=other||
       !guest_camera_actor_live(base,pickup)||!is_kind_of(base,pickup,"SFXDroppedAmmo")||read_ptr(pickup+0x198)!=out->owner.info.pointer||
       !guest_camera_stamp_read(base,pickup,&out->pickup,1)||
       !native_state_fields(pickup,&state,name)||state!=s->valid.owner.pointer||
       !guest_camera_stamp_read(base,state,&out->state,1)||strcmp(out->state.full_name,"SFXGame.SFXDroppedAmmo.Pickup")||
       !narrative_probe_object(base,state,"State")||
       !runtime_ammo_pickup_bit(base,pickup,"Actor","bTickIsDisabled",0x25c,2)||
       !runtime_ammo_pickup_bit(base,other,"Pawn","bCanPickupInventory",0x578,0)||
       !runtime_power_property(base,other,"Pawn","Controller","ObjectProperty",0x374,8)||
       !read_at(pickup+0x25c,&disabled,4)||!read_at(other+0x578,&flag,4))return 0;
    uintptr_t klass=read_ptr(other+0x50),cdo=read_ptr(klass+0x13c);
    if(!guest_camera_stamp_read(base,cdo,&out->cdo,1)||read_ptr(cdo+0x50)!=klass||!narrative_probe_object(base,cdo,"SFXPawn_Ashley")||
       !guest_camera_stamp_matches(base,&s->player_class,1)||!guest_camera_stamp_matches(base,&s->player_cdo,1)||
       read_ptr(s->player_class.pointer+0x13c)!=s->player_cdo.pointer||read_ptr(s->player_cdo.pointer+0x50)!=s->player_class.pointer||
       !read_at(cdo+0x578,&default_flag,4)||!read_at(s->player_cdo.pointer+0x578,&player_flag,4))return 0;
    /* Reciprocal possession supplies the native Controller-or-DrivenVehicle
     * condition. A live true flag already takes the unmodified stock path. */
    return runtime_ammo_pickup_policy(1,1,!!(disabled&0x40),!!(flag&0x40),!!(default_flag&0x40),!!(player_flag&0x40),read_ptr(other+0x374)==out->owner.pc.pointer);
}
static int runtime_ammo_pickup_state_abi(uintptr_t fn,uintptr_t expected) {
    uint32_t flags=0;uint16_t native=1,size=0,ret=0;unsigned char count=0;
    /* State-owned functions cannot use the class-only method ABI helper. */
    return read_ptr(fn+0xd8)==expected&&read_at(fn+0xe0,&flags,4)&&flags==0x20102&&
        read_at(fn+0xe4,&native,2)&&!native&&read_at(fn+0xe6,&size,2)&&size==12&&
        read_at(fn+0xe8,&ret,2)&&ret==8&&read_at(fn+0xea,&count,1)&&count==2;
}
static int runtime_ammo_pickup_methods(uintptr_t base,uintptr_t expected) {
    RuntimeAmmoPickup *s=&runtime_ammo_pickup;
    return guest_player_context_method_matches(base,&s->valid,1)&&guest_player_context_method_matches(base,&s->can,1)&&
        !strcmp(s->valid.function.full_name,"SFXGame.SFXDroppedAmmo.Pickup.ValidTouch")&&
        !strcmp(s->valid.owner.full_name,"SFXGame.SFXDroppedAmmo.Pickup")&&
        !strcmp(s->can.function.full_name,"SFXGame.SFXDroppedAmmo.CanPickUpAmmo")&&
        narrative_probe_object(base,s->valid.owner.pointer,"State")&&
        read_ptr(s->valid.owner.pointer+0x40)==s->can.owner.pointer&&
        runtime_ammo_pickup_state_abi(s->valid.function.pointer,expected)&&
        guest_player_context_function_abi(base,s->can.function.pointer,"SFXDroppedAmmo","CanPickUpAmmo",0x20103,12,8,0,0xf52b0,2)&&
        runtime_ammo_pickup_body(base,s->valid.function.pointer);
}
static int runtime_ammo_pickup_entry(uintptr_t frame,uintptr_t pickup,uintptr_t fn,GameArray *script,uintptr_t *other) {
    return read_ptr(frame+0x18)==fn&&read_ptr(frame+0x20)==pickup&&read_at(fn+0x7c,script,sizeof *script)&&
        script->data&&script->count==RUNTIME_AMMO_PICKUP_VM_SIZE&&script->max>=script->count&&script->max<=4096&&
        script->data<=UINTPTR_MAX-(uintptr_t)script->count&&read_ptr(frame+0x28)==script->data&&
        read_at(read_ptr(frame+0x30),other,8)&&*other;
}
static int runtime_ammo_pickup_redirect(int eligible,uintptr_t frame,const GameArray *script) {
    unsigned char code[2];
    return eligible&&script->data&&script->count==RUNTIME_AMMO_PICKUP_VM_SIZE&&
        script->data<=UINTPTR_MAX-(uintptr_t)script->count&&
        read_at(script->data+RUNTIME_AMMO_PICKUP_RETURN_OFFSET,code,2)&&code[0]==4&&code[1]==0x1c&&
        runtime_power_action_context_pointer(frame+0x28,script->data,script->data+RUNTIME_AMMO_PICKUP_RETURN_OFFSET);
}
static void runtime_ammo_pickup_before(void *self,void *frame) {
    RuntimeAmmoPickup *s=&runtime_ammo_pickup;RuntimeAmmoPickupScope scope={0},again={0};GameArray script={0},checked={0};uintptr_t other=0,next=0;
    if(!s->installed||GetCurrentThreadId()!=session_game_thread||!runtime_ammo_pickup_executor_pins(s->base)||
       !runtime_ammo_pickup_methods(s->base,s->base+0xf52b0)||
       !runtime_ammo_pickup_entry((uintptr_t)frame,(uintptr_t)self,s->valid.function.pointer,&script,&other)||
       !runtime_ammo_pickup_scope(s->base,(uintptr_t)self,other,&scope)||
       !runtime_ammo_pickup_scope(s->base,(uintptr_t)self,other,&again)||
       !runtime_movement_scope_same(&scope.owner,&again.owner)||
       !guest_camera_stamp_matches(s->base,&scope.pickup,1)||!guest_camera_stamp_matches(s->base,&scope.state,1)||
       !guest_camera_stamp_matches(s->base,&scope.cdo,1)||
       !runtime_ammo_pickup_methods(s->base,s->base+0xf52b0)||
       !runtime_ammo_pickup_entry((uintptr_t)frame,(uintptr_t)self,s->valid.function.pointer,&checked,&next)||
       next!=other||memcmp(&script,&checked,sizeof script))return;
    if(runtime_ammo_pickup_redirect(1,(uintptr_t)frame,&script)&&s->rows++<32){char row[320];
        snprintf(row,sizeof row,"{\"runtime_guest_ammo_pickup\":\"stock-capacity-check-selected\",\"role\":%d,\"scope_token\":%lu,\"pickup_flag_written\":false,\"ammo_written\":false,\"gameplay_verified\":false,\"monotonic_ms\":%lu}\r\n",(int)g_runtime_config.role,scope.owner.token,GetTickCount());line(row);}
}
static void runtime_ammo_pickup_forward(void *self,void *frame,void *result,RuntimeAmmoPickupExec stock,void (*before)(void*,void*)) {
    DWORD error=GetLastError();before(self,frame);SetLastError(error);stock(self,frame,result);
}
static void runtime_ammo_pickup_exec(void *self,void *frame,void *result) {
    RuntimeAmmoPickup *s=&runtime_ammo_pickup;
    /* The real executor ABI guarantees this frame. All other scripts take
     * only a direct node comparison and the unchanged trampoline call. */
    if(s->installed&&s->prepared&&frame&&*(uintptr_t*)((unsigned char*)frame+0x18)==s->valid.function.pointer)
        runtime_ammo_pickup_forward(self,frame,result,s->original,runtime_ammo_pickup_before);
    else if(s->installed&&frame&&runtime_ammo_award_node&&*(uintptr_t*)((unsigned char*)frame+0x18)==runtime_ammo_award_node)
        runtime_ammo_pickup_forward(self,frame,result,s->original,runtime_ammo_award_before);
    else if(s->installed&&frame&&runtime_scene_camera_node&&*(uintptr_t*)((unsigned char*)frame+0x18)==runtime_scene_camera_node)
        runtime_scene_camera_forward(self,frame,result,s->original);
    else if(s->installed&&frame&&runtime_scene_camera_fov_node&&*(uintptr_t*)((unsigned char*)frame+0x18)==runtime_scene_camera_fov_node)
        runtime_scene_camera_fov_forward(self,frame,result,s->original);
    else if(s->installed&&frame&&runtime_dialogue_node_match(*(uintptr_t*)((unsigned char*)frame+0x18)))
        runtime_dialogue_forward(self,frame,result,s->original);
    else s->original(self,frame,result);
}
static uintptr_t runtime_ammo_pickup_find(uintptr_t base,uintptr_t owner,const char *kind,const char *name) {
    uintptr_t field=read_ptr(owner+0x70),seen[2048];unsigned count=0;char text[96];
    while(field&&count<2048){if(!is_live(base,field)||read_ptr(field+0x40)!=owner)return 0;
        for(unsigned i=0;i<count;++i)if(seen[i]==field)return 0;seen[count++]=field;
        object_name(base,field,text,sizeof text);if(!strcmp(text,name)&&narrative_probe_object(base,field,kind))return field;
        field=read_ptr(field+0x68);}return 0;
}
static int runtime_ammo_pickup_params(uintptr_t base,uintptr_t fn,const char *name) {
    uintptr_t prop=read_ptr(fn+0x70);const char *names[]={name,"ReturnValue"},*types[]={"ObjectProperty","BoolProperty"};
    for(unsigned i=0;i<2;++i){char path[200];GuestCameraStamp stamp;uint64_t mask=0;
        if(!guest_camera_stamp_read(base,fn,&stamp,1))return 0;snprintf(path,sizeof path,"%.160s.%s",stamp.full_name,names[i]);
        if(!runtime_delayed_fire_property(base,prop,types[i],path,i?8:0,i?4:8,i?0x580:0x80)||
           (i&&(!read_at(prop+0xd0,&mask,8)||mask!=1)))return 0;prop=read_ptr(prop+0x68);}
    return !prop;
}
/* One existing script executor hook is shared by exact function-node filters.
 * Camera viewing can need it while no companion is possessed. Predicate
 * preparation remains independent, so early installation cannot skip ammo. */
static int runtime_script_executor_prepare(uintptr_t base) {
    RuntimeAmmoPickup *s=&runtime_ammo_pickup;
    if(s->installed)return s->base==base&&runtime_ammo_pickup_executor_pins(base);
    if(GetCurrentThreadId()!=session_game_thread||!supported_image(base)||!guest_equipment_dispatch_pins(base)||
       !runtime_ammo_pickup_executor_pins(base)||!read_at(base+0xf52b0,s->original_entry,16))return 0;
    MH_STATUS status=MH_Initialize();if(status!=MH_OK&&status!=MH_ERROR_ALREADY_INITIALIZED)return 0;
    LPVOID original=NULL;s->base=base;
    if(MH_CreateHook((void*)(base+0xf52b0),(void*)runtime_ammo_pickup_exec,&original)!=MH_OK)return 0;
    s->original=(RuntimeAmmoPickupExec)original;
    if(!original||!read_at((uintptr_t)original,s->trampoline_entry,sizeof s->trampoline_entry)||MH_EnableHook((void*)(base+0xf52b0))!=MH_OK||
       !read_at(base+0xf52b0,s->hook_entry,16)) {
        MH_DisableHook((void*)(base+0xf52b0));MH_RemoveHook((void*)(base+0xf52b0));s->original=NULL;return 0;
    }
    s->installed=1;return 1;
}
static void runtime_guest_ammo_pickup_prepare(uintptr_t base,DWORD now) {
    RuntimeAmmoPickup *s=&runtime_ammo_pickup;RuntimeMovementScope scope={0};
    if(s->prepared||GetCurrentThreadId()!=session_game_thread||!runtime_ammo_pickup_owner(base,&scope)||
       (s->checked&&now-s->checked<2000))return;s->checked=now;
    uintptr_t can=function_named(base,"SFXDroppedAmmo","CanPickUpAmmo"),klass=read_ptr(can+0x40),
        state=runtime_ammo_pickup_find(base,klass,"State","Pickup"),valid=runtime_ammo_pickup_find(base,state,"Function","ValidTouch"),
        player=read_ptr(function_named(base,"SFXPawn_Player","PlayHit")+0x40),cdo=read_ptr(player+0x13c);
    if(!supported_image(base)||!guest_equipment_dispatch_pins(base)||!runtime_ammo_pickup_executor_pins(base)||
       !guest_player_context_method_stamp(base,valid,&s->valid)||!guest_player_context_method_stamp(base,can,&s->can)||
       !guest_camera_stamp_read(base,player,&s->player_class,1)||strcmp(s->player_class.full_name,"SFXGame.SFXPawn_Player")||
       !guest_camera_stamp_read(base,cdo,&s->player_cdo,1)||!narrative_probe_object(base,cdo,"SFXPawn_Player")||read_ptr(cdo+0x50)!=player||
       !runtime_ammo_pickup_methods(base,base+0xf52b0)||!runtime_ammo_pickup_params(base,valid,"Other")||
       !runtime_script_executor_prepare(base))return;
    s->prepared=1;line("{\"runtime_guest_ammo_pickup\":\"stock-ammo-predicate-guard-installed\",\"general_pickup_flag_written\":false,\"gameplay_verified\":false}\r\n");
}
#ifdef ME3_RUNTIME_FIXTURE
static void *runtime_ammo_pickup_args[3];static int runtime_ammo_pickup_order,runtime_ammo_pickup_bad;
static void runtime_ammo_pickup_fixture_before(void *self,void *frame) {
    if(runtime_ammo_pickup_order++||self!=runtime_ammo_pickup_args[0]||frame!=runtime_ammo_pickup_args[1])runtime_ammo_pickup_bad=1;SetLastError(11);
}
static void runtime_ammo_pickup_fixture_stock(void *self,void *frame,void *result) {
    if(runtime_ammo_pickup_order++!=1||GetLastError()!=37||self!=runtime_ammo_pickup_args[0]||frame!=runtime_ammo_pickup_args[1]||result!=runtime_ammo_pickup_args[2])runtime_ammo_pickup_bad=1;SetLastError(53);
}
static void runtime_ammo_pickup_fixture_passthrough(void *self,void *frame,void *result) {
    if(runtime_ammo_pickup_order++||GetLastError()!=37||self!=runtime_ammo_pickup_args[0]||frame!=runtime_ammo_pickup_args[1]||result!=runtime_ammo_pickup_args[2])runtime_ammo_pickup_bad=1;SetLastError(53);
}
static int runtime_guest_ammo_pickup_selftest(void) {
    DWORD error=GetLastError();unsigned char frame[0x50]={0},code[RUNTIME_AMMO_PICKUP_VM_SIZE]={0},fn[0xf0]={0};uintptr_t start=(uintptr_t)code;int a,b,c,ok=1;
    uintptr_t exec=0x123;uint32_t flags=0x20102;uint16_t size=12,ret=8;unsigned char count=2;
    memcpy(fn+0xd8,&exec,8);memcpy(fn+0xe0,&flags,4);memcpy(fn+0xe6,&size,2);memcpy(fn+0xe8,&ret,2);memcpy(fn+0xea,&count,1);
    ok=ok&&runtime_ammo_pickup_state_abi((uintptr_t)fn,exec)&&!runtime_ammo_pickup_state_abi((uintptr_t)fn,exec+1);
    fn[0xe4]=1;ok=ok&&!runtime_ammo_pickup_state_abi((uintptr_t)fn,exec);fn[0xe4]=0;
    fn[0xe8]=0;ok=ok&&!runtime_ammo_pickup_state_abi((uintptr_t)fn,exec);fn[0xe8]=8;
    GameArray script={start,RUNTIME_AMMO_PICKUP_VM_SIZE,RUNTIME_AMMO_PICKUP_VM_SIZE};code[148]=4;code[149]=0x1c;memcpy(frame+0x28,&start,8);
    ok=ok&&runtime_ammo_pickup_policy(1,1,0,0,0,1,1)&&!runtime_ammo_pickup_policy(0,1,0,0,0,1,1)&&
        !runtime_ammo_pickup_policy(1,0,0,0,0,1,1)&&!runtime_ammo_pickup_policy(1,1,1,0,0,1,1)&&
        !runtime_ammo_pickup_policy(1,1,0,1,0,1,1)&&!runtime_ammo_pickup_policy(1,1,0,0,1,1,1)&&
        !runtime_ammo_pickup_policy(1,1,0,0,0,0,1)&&!runtime_ammo_pickup_policy(1,1,0,0,0,1,0);
    ok=ok&&!runtime_ammo_pickup_redirect(0,(uintptr_t)frame,&script)&&read_ptr((uintptr_t)frame+0x28)==start&&
        runtime_ammo_pickup_redirect(1,(uintptr_t)frame,&script)&&read_ptr((uintptr_t)frame+0x28)==start+148&&
        !runtime_ammo_pickup_redirect(1,(uintptr_t)frame,&script);
    memcpy(frame+0x28,&start,8);code[149]=0x28;ok=ok&&!runtime_ammo_pickup_redirect(1,(uintptr_t)frame,&script);
    runtime_ammo_pickup_args[0]=&a;runtime_ammo_pickup_args[1]=&b;runtime_ammo_pickup_args[2]=&c;runtime_ammo_pickup_order=runtime_ammo_pickup_bad=0;
    SetLastError(37);runtime_ammo_pickup_forward(&a,&b,&c,runtime_ammo_pickup_fixture_stock,runtime_ammo_pickup_fixture_before);
    ok=ok&&runtime_ammo_pickup_order==2&&!runtime_ammo_pickup_bad&&GetLastError()==53;
    RuntimeAmmoPickup saved=runtime_ammo_pickup;memset(&runtime_ammo_pickup,0,sizeof runtime_ammo_pickup);
    unsigned char live[32]={0},original[32]={0},trampoline[64]={0};uintptr_t fake=(uintptr_t)live-0xf52b0;
    ok=ok&&runtime_script_executor_prefix(fake,original,sizeof original);
    runtime_ammo_pickup.installed=1;runtime_ammo_pickup.base=fake;runtime_ammo_pickup.original=(RuntimeAmmoPickupExec)trampoline;
    live[0]=0xe9;runtime_ammo_pickup.hook_entry[0]=0xe9;
    ok=ok&&runtime_script_executor_prefix(fake,original,sizeof original);
    live[20]=1;ok=ok&&!runtime_script_executor_prefix(fake,original,sizeof original);live[20]=0;
    live[0]=0xcc;ok=ok&&!runtime_script_executor_prefix(fake,original,sizeof original);live[0]=0xe9;
    trampoline[63]=1;ok=ok&&!runtime_script_executor_prefix(fake,original,sizeof original);trampoline[63]=0;
    runtime_ammo_pickup.original=runtime_ammo_pickup_fixture_passthrough;runtime_ammo_pickup.valid.function.pointer=777;
    uintptr_t other_fn=778;memcpy(frame+0x18,&other_fn,8);runtime_ammo_pickup_args[1]=frame;
    runtime_ammo_pickup_order=runtime_ammo_pickup_bad=0;SetLastError(37);runtime_ammo_pickup_exec(&a,frame,&c);
    ok=ok&&runtime_ammo_pickup_order==1&&!runtime_ammo_pickup_bad&&GetLastError()==53;
    other_fn=777;memcpy(frame+0x18,&other_fn,8);runtime_ammo_pickup.base=0;runtime_ammo_pickup_order=runtime_ammo_pickup_bad=0;
    SetLastError(37);runtime_ammo_pickup_exec(&a,frame,&c);ok=ok&&runtime_ammo_pickup_order==1&&!runtime_ammo_pickup_bad&&GetLastError()==53;
    runtime_ammo_pickup=saved;SetLastError(error);return ok;
}
#endif
#endif
