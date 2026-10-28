#ifndef ME3_RUNTIME_AIRLOCK_PLAYER_H
#define ME3_RUNTIME_AIRLOCK_PLAYER_H
/* The authored Mars entrance airlock anchors its transition through one
 * SeqVar_Player. Its stock getter selects Players[0], which is native list
 * order, not campaign protagonist identity. Keep Shepard first only when the
 * fresh two-controller scratch lists of this exact variable put the guest first.
 * No graph, plot, teleport, actor transform or player-selection flag is written. */
typedef void (*RuntimeAirlockNative)(void*);
typedef struct RuntimeAirlockScope {
    SessionHandle host,pawn,info,world,driver,connection,peer,peer_pawn,variable,level;
} RuntimeAirlockScope;
typedef struct RuntimeAirlockCapture {RuntimeAirlockScope scope;int valid;} RuntimeAirlockCapture;
typedef struct RuntimeAirlockPlayer {
    uintptr_t base;
    GuestPlayerContextMethod update,get;
    RuntimeAirlockNative original;
    unsigned char original_entry[16],hook_entry[16],trampoline_entry[64];
    DWORD checked;
    unsigned rows;
    int installed,busy;
} RuntimeAirlockPlayer;
static RuntimeAirlockPlayer runtime_airlock_player;
static void runtime_airlock_player_native(void*);
static int runtime_airlock_player_body_pin(uintptr_t base,const unsigned char *expected,size_t size) {
    RuntimeAirlockPlayer *s=&runtime_airlock_player;unsigned char actual[384],prefix[16],trampoline[64];
    if(size>sizeof actual)return 0;
    if(!s->installed)return read_at(base+0x74f2d0,actual,size)&&!memcmp(actual,expected,size);
    if(base!=s->base||!s->original||size<16||!read_at(base+0x74f2d0,prefix,16)||memcmp(prefix,s->hook_entry,16)||
       !read_at((uintptr_t)s->original,trampoline,sizeof trampoline)||memcmp(trampoline,s->trampoline_entry,sizeof trampoline))return 0;
    memcpy(actual,s->original_entry,16);
    return read_at(base+0x74f2e0,actual+16,size-16)&&!memcmp(actual,expected,size);
}
#include "runtime_airlock_player_pins.h"
static int runtime_airlock_player_enabled(void) {
    static int enabled=-1;if(enabled<0){wchar_t text[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_AIRLOCK_PLAYER",text,8);
        enabled=!n||(n==1&&text[0]==L'1');}return enabled;
}
static int runtime_airlock_scope_same(const RuntimeAirlockScope *a,const RuntimeAirlockScope *b) {
    const SessionHandle *aa[]={&a->host,&a->pawn,&a->info,&a->world,&a->driver,&a->connection,&a->peer,&a->peer_pawn,&a->variable,&a->level};
    const SessionHandle *bb[]={&b->host,&b->pawn,&b->info,&b->world,&b->driver,&b->connection,&b->peer,&b->peer_pawn,&b->variable,&b->level};
    for(unsigned i=0;i<10;++i)if(!runtime_movement_same_handle(*aa[i],*bb[i]))return 0;return 1;
}
static int runtime_airlock_player_array_layout(uintptr_t base,uintptr_t variable,const char *name,int offset) {
    uintptr_t prop=runtime_guest_team_field(base,variable,"SeqVar_Player",name,"ArrayProperty",offset,16,0x402000),
        inner=read_ptr(prop+0xd0),klass=read_ptr(inner+0xd0);int32_t size=0,dim=0,at=-1;GuestCameraStamp stamp={0};
    return prop&&narrative_probe_object(base,inner,"ObjectProperty")&&read_at(inner+0x74,&size,4)&&size==8&&
        read_at(inner+0x70,&dim,4)&&dim==1&&read_at(inner+0x94,&at,4)&&!at&&
        narrative_probe_object(base,klass,"Class")&&guest_camera_stamp_read(base,klass,&stamp,1)&&!strcmp(stamp.full_name,"Core.Object");
}
static int runtime_airlock_player_scope(uintptr_t base,uintptr_t variable,RuntimeAirlockScope *out) {
    SessionContext c=session_context(session_engine);NetworkActorScope world={0};GameArray peers={0},again={0};uint32_t flags=0;int32_t index=-1;
    uintptr_t story_world=0,level=0,connection=0,peer=0,peer_pawn=0;GuestCameraStamp stamp={0};
    memset(out,0,sizeof *out);
    if(!runtime_airlock_player_enabled()||g_runtime_config.role!=ME3_ROLE_HOST||GetCurrentThreadId()!=session_game_thread||
       !runtime_control_enabled||!runtime_scope_ready||!InterlockedCompareExchange(&g_enable_requested,0,0)||
       !guest_camera_actor_live(base,c.pc)||!is_kind_of(base,c.pc,"SFXPlayerController")||
       !guest_camera_actor_live(base,c.pawn)||!is_kind_of(base,c.pawn,"SFXPawn_Player")||
       read_ptr(c.pc+0x3bc)!=c.pawn||read_ptr(c.pawn+0x374)!=c.pc||read_ptr(c.pc+0x198)!=c.world||
       !network_actor_read_scope(base,c.pawn,&world)||world.info!=c.world||
       !scene_readiness_array(world.driver+0x70,&peers,32)||peers.count!=1||
       !read_at(peers.data,&connection,8)||!narrative_probe_object(base,connection,"NetConnection")||read_ptr(connection+0x8c)!=world.driver||
       !(peer=read_ptr(connection+0x68))||peer==c.pc||!guest_camera_actor_live(base,peer)||!is_kind_of(base,peer,"SFXPlayerController")||
       read_ptr(peer+0x67c)!=connection||read_ptr(peer+0x198)!=c.world||
       !guest_camera_stamp_read(base,variable,&stamp,1)||
       strcmp(stamp.full_name,"BioD_ProMar_125Elevator.TheWorld.PersistentLevel.Main_Sequence.Base_Entrance_Airlock.SeqVar_Player_2")||
       !narrative_probe_object(base,variable,"SeqVar_Player")||
       !(level=runtime_story_graph_level(base,variable,&story_world))||story_world!=world.world||
       !runtime_airlock_player_array_layout(base,variable,"Players",0xb8)||
       !runtime_airlock_player_array_layout(base,variable,"PlayersPawns",0xc8)||
       !runtime_power_property(base,variable,"SeqVar_Player","PlayerIdx","IntProperty",0xd8,4)||
       !runtime_native_hud_bool(base,read_ptr(variable+0x50),"bAllPlayers",0xdc,1)||
       !runtime_native_hud_bool(base,read_ptr(variable+0x50),"bReturnPawns",0xdc,2)||
       !read_at(variable+0xdc,&flags,4)||(flags&3)!=3||!read_at(variable+0xd8,&index,4)||index)return 0;
    peer_pawn=read_ptr(peer+0x3bc);
    if(peer_pawn&&(!guest_camera_actor_live(base,peer_pawn)||!is_kind_of(base,peer_pawn,"SFXPawn_Ashley")||
       read_ptr(peer_pawn+0x198)!=c.world||read_ptr(peer_pawn+0x374)!=peer))return 0;
    out->host=session_capture_handle(c.pc);out->pawn=session_capture_handle(c.pawn);out->info=session_capture_handle(c.world);
    out->world=session_capture_handle(world.world);out->driver=session_capture_handle(world.driver);out->connection=session_capture_handle(connection);
    out->peer=session_capture_handle(peer);out->variable=session_capture_handle(variable);out->level=session_capture_handle(level);
    if(peer_pawn)out->peer_pawn=session_capture_handle(peer_pawn);
    const SessionHandle *handles[]={&out->host,&out->pawn,&out->info,&out->world,&out->driver,&out->connection,&out->peer,&out->variable,&out->level};
    for(unsigned i=0;i<9;++i)if(!session_handle_matches(*handles[i]))return 0;
    return (!peer_pawn||session_handle_matches(out->peer_pawn))&&guest_camera_stamp_matches(base,&stamp,1)&&
        read_ptr(peer+0x3bc)==peer_pawn&&read_at(world.driver+0x70,&again,sizeof again)&&!memcmp(&peers,&again,sizeof peers)&&
        read_ptr(peers.data)==connection;
}
static int runtime_airlock_player_methods(uintptr_t base) {
    RuntimeAirlockPlayer *s=&runtime_airlock_player;GameArray script={0};unsigned char prefix[10];uintptr_t called=0;
    if(!guest_player_context_method_matches(base,&s->update,1)||!guest_player_context_method_matches(base,&s->get,1)||
       strcmp(s->update.function.full_name,"Engine.SeqVar_Player.UpdatePlayersList")||strcmp(s->get.function.full_name,"Engine.SeqVar_Player.GetObjectValue")||
       s->update.owner.pointer!=s->get.owner.pointer||
       !guest_player_context_function_abi(base,s->update.function.pointer,"SeqVar_Player","UpdatePlayersList",0x20401,0,65535,0,0x4b4cc0,0)||
       !guest_player_context_function_abi(base,s->get.function.pointer,"SeqVar_Player","GetObjectValue",0x20002,8,0,0,0xf52b0,1)||
       !read_at(s->get.function.pointer+0x7c,&script,sizeof script)||script.count!=261||script.max<261||script.max>4096||
       !read_at(script.data,prefix,sizeof prefix)||prefix[0]!=0x1c||prefix[9]!=0x16)return 0;
    memcpy(&called,prefix+1,8);return called==s->update.function.pointer&&runtime_airlock_player_native_pins(base);
}
static RuntimeAirlockCapture runtime_airlock_player_capture(void *self) {
    RuntimeAirlockPlayer *s=&runtime_airlock_player;RuntimeAirlockCapture c={0};
    if(!s->installed||s->busy||GetCurrentThreadId()!=session_game_thread||
       !runtime_airlock_player_methods(s->base)||
       !runtime_airlock_player_scope(s->base,(uintptr_t)self,&c.scope))return c;
    c.valid=1;return c;
}
static int runtime_airlock_player_order(const uintptr_t controllers[2],const uintptr_t pawns[2],
        uintptr_t host,uintptr_t shepard,uintptr_t peer,uintptr_t peer_pawn) {
    if(!host||!shepard||!peer||host==peer||shepard==peer_pawn)return 0;
    if(controllers[0]==host&&controllers[1]==peer&&pawns[0]==shepard&&pawns[1]==peer_pawn)return 2;
    return controllers[0]==peer&&controllers[1]==host&&pawns[0]==peer_pawn&&pawns[1]==shepard?1:0;
}
static int runtime_airlock_player_reorder(const uintptr_t cells[4],const uintptr_t expected[4],const uintptr_t desired[4]) {
    unsigned changed=0;
    for(;changed<4;++changed)if(!runtime_power_action_context_pointer(cells[changed],expected[changed],desired[changed]))break;
    if(changed==4)return 1;
    while(changed){--changed;runtime_power_action_context_pointer(cells[changed],desired[changed],expected[changed]);}
    return 0;
}
static void runtime_airlock_player_after(RuntimeAirlockCapture *capture) {
    RuntimeAirlockPlayer *s=&runtime_airlock_player;RuntimeAirlockScope current={0};GameArray players={0},pawns={0},again={0};uintptr_t pc[2],pawn[2];
    if(!capture->valid||s->busy||!runtime_airlock_player_methods(s->base)||
       !runtime_airlock_player_scope(s->base,capture->scope.variable.pointer,&current)||!runtime_airlock_scope_same(&capture->scope,&current)||
       !scene_readiness_array(current.variable.pointer+0xb8,&players,32)||players.count!=2||
       !scene_readiness_array(current.variable.pointer+0xc8,&pawns,32)||pawns.count!=2||
       !read_at(players.data,pc,sizeof pc)||!read_at(pawns.data,pawn,sizeof pawn))return;
    int order=runtime_airlock_player_order(pc,pawn,current.host.pointer,current.pawn.pointer,current.peer.pointer,current.peer_pawn.pointer);
    if(!order)return;
    if(order==1){
        if(!read_at(current.variable.pointer+0xb8,&again,sizeof again)||memcmp(&players,&again,sizeof players)||
           !read_at(current.variable.pointer+0xc8,&again,sizeof again)||memcmp(&pawns,&again,sizeof pawns))return;
        uintptr_t cells[]={players.data,players.data+8,pawns.data,pawns.data+8};
        uintptr_t expected[]={pc[0],pc[1],pawn[0],pawn[1]},desired[]={pc[1],pc[0],pawn[1],pawn[0]};
        s->busy=1;int changed=runtime_airlock_player_reorder(cells,expected,desired);s->busy=0;if(!changed)return;
    }
    if(s->rows++<16){char row[300];snprintf(row,sizeof row,
        "{\"runtime_airlock_player\":\"%s\",\"host_controller_index\":%d,\"shepard_index\":%d,\"scratch_player_lists_only\":true,\"actor_position_written\":false,\"gameplay_verified\":false,\"monotonic_ms\":%lu}\r\n",
        order==1?"primary-player-order-corrected":"native-primary-player-retained",current.host.index,current.pawn.index,GetTickCount());line(row);}
}
static void runtime_airlock_player_forward(void *self,RuntimeAirlockNative stock,
        RuntimeAirlockCapture (*capture)(void*),void (*after)(RuntimeAirlockCapture*)) {
    DWORD entry=GetLastError();RuntimeAirlockCapture c=capture(self);SetLastError(entry);stock(self);
    DWORD error=GetLastError();after(&c);SetLastError(error);
}
static void runtime_airlock_player_native(void *self) {
    RuntimeAirlockPlayer *s=&runtime_airlock_player;
    /* Both script and direct native callers reach this builder. Only the
     * authored variable suffix pays for metadata/scope discovery. */
    if(s->installed&&self&&
       *(uintptr_t*)((unsigned char*)self+0x50)==s->get.owner.pointer&&*(uint32_t*)((unsigned char*)self+0x4c)==3)
        runtime_airlock_player_forward(self,s->original,runtime_airlock_player_capture,runtime_airlock_player_after);
    else s->original(self);
}
static void runtime_airlock_player_prepare(uintptr_t base,DWORD now) {
    RuntimeAirlockPlayer *s=&runtime_airlock_player;
    if(s->installed||g_runtime_config.role!=ME3_ROLE_HOST||GetCurrentThreadId()!=session_game_thread||
       !runtime_airlock_player_enabled()||!runtime_control_enabled||!runtime_scope_ready||(s->checked&&now-s->checked<2000))return;s->checked=now;
    uintptr_t update=function_named(base,"SeqVar_Player","UpdatePlayersList"),get=function_named(base,"SeqVar_Player","GetObjectValue");
    if(!supported_image(base)||!guest_player_context_method_stamp(base,update,&s->update)||!guest_player_context_method_stamp(base,get,&s->get)||
       !runtime_airlock_player_methods(base)||!read_at(base+0x74f2d0,s->original_entry,16))return;
    MH_STATUS status=MH_Initialize();if(status!=MH_OK&&status!=MH_ERROR_ALREADY_INITIALIZED)return;
    LPVOID original=NULL;s->base=base;
    if(MH_CreateHook((void*)(base+0x74f2d0),(void*)runtime_airlock_player_native,&original)!=MH_OK)return;
    s->original=(RuntimeAirlockNative)original;
    if(!original||!read_at((uintptr_t)original,s->trampoline_entry,sizeof s->trampoline_entry)||MH_EnableHook((void*)(base+0x74f2d0))!=MH_OK||
       !read_at(base+0x74f2d0,s->hook_entry,16)){
        MH_DisableHook((void*)(base+0x74f2d0));MH_RemoveHook((void*)(base+0x74f2d0));s->original=NULL;return;
    }
    s->installed=1;line("{\"runtime_airlock_player\":\"scoped-player-list-guard-installed\",\"actor_position_written\":false,\"gameplay_verified\":false}\r\n");
}
#ifdef ME3_RUNTIME_FIXTURE
static int runtime_airlock_fixture_order,runtime_airlock_fixture_bad,runtime_airlock_fixture_valid;
static void *runtime_airlock_fixture_self;
static RuntimeAirlockCapture runtime_airlock_fixture_capture(void *self) {
    RuntimeAirlockCapture c={0};c.valid=runtime_airlock_fixture_valid;
    if(runtime_airlock_fixture_order++||self!=runtime_airlock_fixture_self)runtime_airlock_fixture_bad=1;
    SetLastError(11);return c;
}
static void runtime_airlock_fixture_stock(void *self) {
    if(runtime_airlock_fixture_order++!=1||GetLastError()!=37||self!=runtime_airlock_fixture_self)runtime_airlock_fixture_bad=1;SetLastError(53);
}
static void runtime_airlock_fixture_passthrough(void *self) {
    if(runtime_airlock_fixture_order++||GetLastError()!=37||self!=runtime_airlock_fixture_self)runtime_airlock_fixture_bad=1;SetLastError(53);
}
static void runtime_airlock_fixture_after(RuntimeAirlockCapture *c) {
    if(runtime_airlock_fixture_order++!=2||GetLastError()!=53||c->valid!=runtime_airlock_fixture_valid)runtime_airlock_fixture_bad=1;SetLastError(79);
}
static int runtime_airlock_player_selftest(void) {
    DWORD error=GetLastError();int a,ok=1;
    uintptr_t pc[]={10,20},pawn[]={30,40},empty[]={0,30};
    ok=ok&&runtime_airlock_player_order(pc,pawn,10,30,20,40)==2&&runtime_airlock_player_order(pc,pawn,20,40,10,30)==1&&
        !runtime_airlock_player_order(pc,pawn,10,40,20,30)&&!runtime_airlock_player_order(pc,pawn,10,30,10,40)&&
        runtime_airlock_player_order(pc,empty,20,30,10,0)==1;
    _Alignas(8) uintptr_t values[]={10,20,30,40},cells[4],expected[]={10,20,30,40},desired[]={20,10,40,30};
    for(unsigned i=0;i<4;++i)cells[i]=(uintptr_t)&values[i];
    ok=ok&&runtime_airlock_player_reorder(cells,expected,desired)&&!memcmp(values,desired,sizeof values);
    memcpy(values,expected,sizeof values);values[2]=99;
    ok=ok&&!runtime_airlock_player_reorder(cells,expected,desired)&&values[0]==10&&values[1]==20&&values[2]==99&&values[3]==40;
    runtime_airlock_fixture_self=&a;
    for(int valid=0;valid<2;++valid){runtime_airlock_fixture_valid=valid;runtime_airlock_fixture_order=runtime_airlock_fixture_bad=0;
        SetLastError(37);runtime_airlock_player_forward(&a,runtime_airlock_fixture_stock,runtime_airlock_fixture_capture,runtime_airlock_fixture_after);
        ok=ok&&runtime_airlock_fixture_order==3&&!runtime_airlock_fixture_bad&&GetLastError()==53;}
    RuntimeAirlockPlayer saved=runtime_airlock_player;DWORD thread=session_game_thread;
    memset(&runtime_airlock_player,0,sizeof runtime_airlock_player);
    unsigned char live[32]={0},original[32]={0},trampoline[64]={0};uintptr_t fake=(uintptr_t)live-0x74f2d0;
    ok=ok&&runtime_airlock_player_body_pin(fake,original,sizeof original);
    runtime_airlock_player.installed=1;runtime_airlock_player.base=fake;runtime_airlock_player.original=(RuntimeAirlockNative)trampoline;
    live[0]=0xe9;runtime_airlock_player.hook_entry[0]=0xe9;
    ok=ok&&runtime_airlock_player_body_pin(fake,original,sizeof original);
    live[22]=1;ok=ok&&!runtime_airlock_player_body_pin(fake,original,sizeof original);live[22]=0;
    live[0]=0xcc;ok=ok&&!runtime_airlock_player_body_pin(fake,original,sizeof original);live[0]=0xe9;
    trampoline[63]=1;ok=ok&&!runtime_airlock_player_body_pin(fake,original,sizeof original);trampoline[63]=0;
    unsigned char object[0x60]={0};uintptr_t klass=0x123;uint32_t number=2;memcpy(object+0x50,&klass,8);memcpy(object+0x4c,&number,4);
    runtime_airlock_player.original=runtime_airlock_fixture_passthrough;runtime_airlock_player.get.owner.pointer=klass;
    runtime_airlock_fixture_self=object;runtime_airlock_fixture_order=runtime_airlock_fixture_bad=0;
    SetLastError(37);runtime_airlock_player_native(object);ok=ok&&runtime_airlock_fixture_order==1&&!runtime_airlock_fixture_bad&&GetLastError()==53;
    number=3;memcpy(object+0x4c,&number,4);session_game_thread=0;runtime_airlock_fixture_order=runtime_airlock_fixture_bad=0;
    SetLastError(37);runtime_airlock_player_native(object);ok=ok&&runtime_airlock_fixture_order==1&&!runtime_airlock_fixture_bad&&GetLastError()==53;
    runtime_airlock_player=saved;session_game_thread=thread;
    SetLastError(error);return ok;
}
#endif
#endif
