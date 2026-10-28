#ifndef ME3_RUNTIME_SUPPLY_SLEEP_H
#define ME3_RUNTIME_SUPPLY_SLEEP_H
/* Retained LEMP Cerberus/Noveria supply containers request a three-second
 * shared respawn; campaign containers inherit twenty seconds. Change only
 * that exact real native sleep argument for an existing respawning container
 * while the host has a connected campaign peer. All grant, capacity, sharing,
 * visibility and native state/timing behavior remains in the stock scripts. */
typedef void (*RuntimeSupplySleepNative)(void*,float);
typedef struct RuntimeSupplySleepCapture {GuestCameraStamp actor,state;uintptr_t frame;int valid;} RuntimeSupplySleepCapture;
typedef struct RuntimeSupplySleep {
    uintptr_t base;
    GuestPlayerContextMethod sleep;
    GuestCameraStamp klass,cdo,state;
    RuntimeSupplySleepNative original;
    unsigned char original_entry[16],hook_entry[16],trampoline_entry[64];
    DWORD checked;
    unsigned rows;
    int installed;
} RuntimeSupplySleep;
static RuntimeSupplySleep runtime_supply_sleep;
static void runtime_supply_sleep_native(void*,float);
static int runtime_supply_sleep_body_pin(uintptr_t base,const unsigned char *expected,size_t size) {
    RuntimeSupplySleep *s=&runtime_supply_sleep;unsigned char actual[32],entry[16],trampoline[64];
    if(size>sizeof actual)return 0;
    if(!s->installed)return read_at(base+0x70c2c0,actual,size)&&!memcmp(actual,expected,size);
    if(base!=s->base||!s->original||size<16||!read_at(base+0x70c2c0,entry,16)||memcmp(entry,s->hook_entry,16)||
       !read_at((uintptr_t)s->original,trampoline,sizeof trampoline)||memcmp(trampoline,s->trampoline_entry,sizeof trampoline))return 0;
    memcpy(actual,s->original_entry,16);
    return read_at(base+0x70c2d0,actual+16,size-16)&&!memcmp(actual,expected,size);
}
#include "runtime_supply_sleep_pins.h"
static int runtime_supply_sleep_enabled(void) {
    static int enabled=-1;if(enabled<0){wchar_t text[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_MP_SUPPLY_RESPAWN",text,8);
        enabled=!n||(n==1&&text[0]==L'1');}return enabled;
}
static float runtime_supply_sleep_seconds(int eligible,float seconds) {
    return eligible&&seconds==20.0f?3.0f:seconds;
}
static int runtime_supply_sleep_coop(uintptr_t base,SessionContext *out) {
    SessionContext c=session_context(session_engine);NetworkActorScope world={0};GameArray peers={0},again={0};
    if(!runtime_supply_sleep_enabled()||g_runtime_config.role!=ME3_ROLE_HOST||GetCurrentThreadId()!=session_game_thread||
       !runtime_control_enabled||!runtime_scope_ready||!InterlockedCompareExchange(&g_enable_requested,0,0)||
       !guest_camera_actor_live(base,c.pc)||!is_kind_of(base,c.pc,"SFXPlayerController")||
       !guest_camera_actor_live(base,c.pawn)||!is_kind_of(base,c.pawn,"SFXPawn_Player")||
       read_ptr(c.pc+0x3bc)!=c.pawn||read_ptr(c.pawn+0x374)!=c.pc||read_ptr(c.pc+0x198)!=c.world||
       !network_actor_read_scope(base,c.pawn,&world)||world.info!=c.world||
       !scene_readiness_array(world.driver+0x70,&peers,32)||peers.count!=1)return 0;
    uintptr_t connection=read_ptr(peers.data),pc=read_ptr(connection+0x68),pawn=read_ptr(pc+0x3bc);
    if(!narrative_probe_object(base,connection,"NetConnection")||read_ptr(connection+0x8c)!=world.driver||
       !guest_camera_actor_live(base,pc)||pc==c.pc||!is_kind_of(base,pc,"SFXPlayerController")||
       read_ptr(pc+0x67c)!=connection||read_ptr(pc+0x198)!=c.world||
       (pawn&&(!guest_camera_actor_live(base,pawn)||!is_kind_of(base,pawn,"SFXPawn_Ashley")||
                read_ptr(pawn+0x374)!=pc||read_ptr(pawn+0x198)!=c.world))||
       !read_at(world.driver+0x70,&again,sizeof again)||memcmp(&peers,&again,sizeof peers)||read_ptr(peers.data)!=connection)return 0;
    *out=c;return 1;
}
static uintptr_t runtime_supply_sleep_state(uintptr_t base,uintptr_t klass) {
    uintptr_t child=read_ptr(klass+0x70),seen[512];unsigned count=0;char name[96];
    while(child&&count<512){
        if(!is_live(base,child)||read_ptr(child+0x40)!=klass)return 0;
        for(unsigned i=0;i<count;++i)if(seen[i]==child)return 0;seen[count++]=child;
        object_name(base,child,name,sizeof name);
        if(!strcmp(name,"Sleeping")&&narrative_probe_object(base,child,"State"))return child;
        child=read_ptr(child+0x68);
    }
    return 0;
}
static int runtime_supply_sleep_contract(uintptr_t base) {
    RuntimeSupplySleep *s=&runtime_supply_sleep;GameArray script={0};unsigned char code[12];uintptr_t prop=0;uint64_t mask=0;
    if(!guest_player_context_method_matches(base,&s->sleep,1)||strcmp(s->sleep.function.full_name,"Engine.Actor.Sleep")||
       !guest_player_context_function_abi(base,s->sleep.function.pointer,"Actor","Sleep",0x20409,4,65535,256,0x4b9e80,1)||
       !guest_camera_stamp_matches(base,&s->klass,1)||!guest_camera_stamp_matches(base,&s->cdo,1)||!guest_camera_stamp_matches(base,&s->state,1)||
       strcmp(s->klass.full_name,"SFXGameContent.SFXAmmoContainer")||strcmp(s->state.full_name,"SFXGameContent.SFXAmmoContainer.Sleeping")||
       read_ptr(s->klass.pointer+0x13c)!=s->cdo.pointer||read_ptr(s->cdo.pointer+0x50)!=s->klass.pointer||
       read_ptr(s->state.pointer+0x40)!=s->klass.pointer||!narrative_probe_object(base,s->state.pointer,"State")||
       !read_at(s->state.pointer+0x7c,&script,sizeof script)||script.count!=57||script.max<57||script.max>4096||
       !read_at(script.data,code,sizeof code)||code[0]!=0x71||code[1]||code[2]!=0x5f||code[11]!=0x16)return 0;
    memcpy(&prop,code+3,8);
    if(!runtime_delayed_fire_property(base,prop,"FloatProperty","SFXGameContent.SFXAmmoContainer.RespawnTime",0x2ac,4,1)||
       read_ptr(prop+0x40)!=s->klass.pointer||
       !runtime_native_hud_bool(base,s->klass.pointer,"bRespawns",0x2b0,1)||
       !runtime_native_hud_bool(base,s->klass.pointer,"bAmmoMeshVisible",0x2b0,2)||
       !runtime_supply_sleep_pins(base))return 0;
    uintptr_t seconds=read_ptr(s->sleep.function.pointer+0x70);
    (void)mask;
    return runtime_delayed_fire_property(base,seconds,"FloatProperty","Engine.Actor.Sleep.Seconds",0,4,0x80);
}
static RuntimeSupplySleepCapture runtime_supply_sleep_capture(void *self,float seconds,uintptr_t caller) {
    RuntimeSupplySleep *s=&runtime_supply_sleep;RuntimeSupplySleepCapture capture={0};SessionContext c={0};
    uintptr_t actor=(uintptr_t)self,state=0,world=0;uint32_t state_name[2],flags=0;unsigned char role=0;float current=0,default_seconds=0;
    GameArray script={0};GuestCameraStamp archetype={0};
    if(!s->installed||caller!=s->base+0x4b9ef1||seconds!=20.0f||!runtime_supply_sleep_coop(s->base,&c)||
       !runtime_supply_sleep_contract(s->base)||!guest_camera_actor_live(s->base,actor)||read_ptr(actor+0x50)!=s->klass.pointer||
       read_ptr(actor+0x198)!=c.world||!runtime_story_graph_level(s->base,actor,&world)||world!=read_ptr(s->base+0x18b6638)||
       !runtime_power_property(s->base,actor,"Actor","Role","ByteProperty",0x26a,1)||!read_at(actor+0x26a,&role,1)||role!=3||
       !native_state_fields(actor,&state,state_name)||state!=s->state.pointer||
       !read_at(actor+0x2b0,&flags,4)||!(flags&1)||
       !read_at(actor+0x2ac,&current,4)||current!=seconds||!read_at(s->cdo.pointer+0x2ac,&default_seconds,4)||default_seconds!=20.0f||
       !guest_camera_stamp_read(s->base,read_ptr(actor+0x58),&archetype,1)||
       strcmp(archetype.full_name,"BioDpl_Int.TREASURE.TR_heatsinks_respawning.TR_heatsinks_respawning_Arc0")||
       !guest_camera_stamp_read(s->base,actor,&capture.actor,1))return capture;
    uintptr_t frame=read_ptr(actor+0x24);uint16_t latent=65535;
    if(!frame||read_ptr(frame+0x20)!=actor||read_ptr(frame+0x48)!=state||
       !read_at(state+0x7c,&script,sizeof script)||!script.data||script.count!=57||script.max<57||script.max>4096||
       script.data>UINTPTR_MAX-57||read_ptr(frame+0x28)!=script.data+12||
       !read_at(frame+0x58,&latent,2)||latent||
       !runtime_guest_team_field(s->base,actor,"Actor","LatentFloat","FloatProperty",0x23c,4,2))return capture;
    capture.frame=frame;capture.state=s->state;capture.valid=1;return capture;
}
static void runtime_supply_sleep_after(const RuntimeSupplySleepCapture *capture,float requested) {
    RuntimeSupplySleep *s=&runtime_supply_sleep;float latent=0;uint16_t action=0;
    if(!capture->valid||!guest_camera_stamp_matches(s->base,&capture->actor,1)||!guest_camera_stamp_matches(s->base,&capture->state,1)||
       read_ptr(capture->actor.pointer+0x24)!=capture->frame||read_ptr(capture->frame+0x48)!=capture->state.pointer||
       !read_at(capture->frame+0x58,&action,2)||action!=0x180||!read_at(capture->actor.pointer+0x23c,&latent,4)||latent!=requested)return;
    if(s->rows++<32){char row[320];snprintf(row,sizeof row,
        "{\"runtime_mp_supply\":\"native-shared-respawn-scheduled\",\"container_index\":%d,\"authored_seconds\":20,\"native_requested_seconds\":3,\"native_latent_seconds\":%.3f,\"ammo_written\":false,\"respawn_property_written\":false,\"gameplay_verified\":false,\"monotonic_ms\":%lu}\r\n",capture->actor.index,latent,GetTickCount());line(row);}
}
static void runtime_supply_sleep_forward(void *self,float seconds,uintptr_t caller,RuntimeSupplySleepNative stock,
        RuntimeSupplySleepCapture (*capture)(void*,float,uintptr_t),void (*after)(const RuntimeSupplySleepCapture*,float)) {
    DWORD entry=GetLastError();RuntimeSupplySleepCapture c=capture(self,seconds,caller);float desired=runtime_supply_sleep_seconds(c.valid,seconds);
    SetLastError(entry);stock(self,desired);DWORD error=GetLastError();after(&c,desired);SetLastError(error);
}
static void runtime_supply_sleep_native(void *self,float seconds) {
    RuntimeSupplySleep *s=&runtime_supply_sleep;
    if(s->installed&&seconds==20.0f&&self&&*(uintptr_t*)((unsigned char*)self+0x50)==s->klass.pointer)
        runtime_supply_sleep_forward(self,seconds,(uintptr_t)__builtin_return_address(0),s->original,runtime_supply_sleep_capture,runtime_supply_sleep_after);
    else s->original(self,seconds);
}
static void runtime_supply_sleep_prepare(uintptr_t base,DWORD now) {
    RuntimeSupplySleep *s=&runtime_supply_sleep;
    if(s->installed||!runtime_supply_sleep_enabled()||g_runtime_config.role!=ME3_ROLE_HOST||GetCurrentThreadId()!=session_game_thread||
       !runtime_control_enabled||!runtime_scope_ready||(s->checked&&now-s->checked<2000))return;s->checked=now;
    uintptr_t klass=read_ptr(function_named(base,"SFXAmmoContainer","SetRespawn")+0x40),cdo=read_ptr(klass+0x13c),state=runtime_supply_sleep_state(base,klass),sleep=function_named(base,"Actor","Sleep");
    if(!supported_image(base)||!guest_camera_stamp_read(base,klass,&s->klass,1)||!guest_camera_stamp_read(base,cdo,&s->cdo,1)||
       !guest_camera_stamp_read(base,state,&s->state,1)||!guest_player_context_method_stamp(base,sleep,&s->sleep)||
       !runtime_supply_sleep_contract(base)||!read_at(base+0x70c2c0,s->original_entry,16))return;
    MH_STATUS status=MH_Initialize();if(status!=MH_OK&&status!=MH_ERROR_ALREADY_INITIALIZED)return;
    LPVOID original=NULL;s->base=base;
    if(MH_CreateHook((void*)(base+0x70c2c0),(void*)runtime_supply_sleep_native,&original)!=MH_OK)return;
    s->original=(RuntimeSupplySleepNative)original;
    if(!original||!read_at((uintptr_t)original,s->trampoline_entry,sizeof s->trampoline_entry)||MH_EnableHook((void*)(base+0x70c2c0))!=MH_OK||
       !read_at(base+0x70c2c0,s->hook_entry,16)){
        MH_DisableHook((void*)(base+0x70c2c0));MH_RemoveHook((void*)(base+0x70c2c0));s->original=NULL;return;
    }
    s->installed=1;line("{\"runtime_mp_supply\":\"native-container-sleep-guard-installed\",\"respawn_property_written\":false,\"gameplay_verified\":false}\r\n");
}
#ifdef ME3_RUNTIME_FIXTURE
static void *runtime_supply_fixture_self;
static int runtime_supply_fixture_order,runtime_supply_fixture_bad,runtime_supply_fixture_valid;
static float runtime_supply_fixture_expected;
static RuntimeSupplySleepCapture runtime_supply_fixture_capture(void *self,float seconds,uintptr_t caller) {
    RuntimeSupplySleepCapture c={0};c.valid=runtime_supply_fixture_valid;
    if(runtime_supply_fixture_order++||self!=runtime_supply_fixture_self||seconds!=20.0f||caller!=77)runtime_supply_fixture_bad=1;SetLastError(11);return c;
}
static void runtime_supply_fixture_stock(void *self,float seconds) {
    if(runtime_supply_fixture_order++!=1||self!=runtime_supply_fixture_self||GetLastError()!=37||seconds!=runtime_supply_fixture_expected)runtime_supply_fixture_bad=1;SetLastError(53);
}
static void runtime_supply_fixture_after(const RuntimeSupplySleepCapture *c,float desired) {
    if(runtime_supply_fixture_order++!=2||GetLastError()!=53||c->valid!=runtime_supply_fixture_valid||desired!=runtime_supply_fixture_expected)runtime_supply_fixture_bad=1;SetLastError(79);
}
static int runtime_supply_sleep_selftest(void) {
    DWORD error=GetLastError();int a,ok=1;
    ok=ok&&runtime_supply_sleep_seconds(1,20.0f)==3.0f&&runtime_supply_sleep_seconds(0,20.0f)==20.0f&&
        runtime_supply_sleep_seconds(1,3.0f)==3.0f&&runtime_supply_sleep_seconds(1,8.0f)==8.0f&&
        runtime_supply_sleep_seconds(1,-1.0f)==-1.0f&&runtime_supply_sleep_seconds(1,0.0f)==0.0f&&isnan(runtime_supply_sleep_seconds(1,NAN));
    runtime_supply_fixture_self=&a;
    for(int valid=0;valid<2;++valid){runtime_supply_fixture_valid=valid;runtime_supply_fixture_expected=valid?3.0f:20.0f;runtime_supply_fixture_order=runtime_supply_fixture_bad=0;
        SetLastError(37);runtime_supply_sleep_forward(&a,20.0f,77,runtime_supply_fixture_stock,runtime_supply_fixture_capture,runtime_supply_fixture_after);
        ok=ok&&runtime_supply_fixture_order==3&&!runtime_supply_fixture_bad&&GetLastError()==53;}
    RuntimeSupplySleep saved=runtime_supply_sleep;memset(&runtime_supply_sleep,0,sizeof runtime_supply_sleep);
    unsigned char live[22]={0},original[22]={0},trampoline[64]={0};uintptr_t fake=(uintptr_t)live-0x70c2c0;
    ok=ok&&runtime_supply_sleep_body_pin(fake,original,sizeof original);
    runtime_supply_sleep.installed=1;runtime_supply_sleep.base=fake;runtime_supply_sleep.original=(RuntimeSupplySleepNative)trampoline;
    live[0]=0xe9;runtime_supply_sleep.hook_entry[0]=0xe9;ok=ok&&runtime_supply_sleep_body_pin(fake,original,sizeof original);
    live[20]=1;ok=ok&&!runtime_supply_sleep_body_pin(fake,original,sizeof original);live[20]=0;
    live[0]=0xcc;ok=ok&&!runtime_supply_sleep_body_pin(fake,original,sizeof original);live[0]=0xe9;
    trampoline[63]=1;ok=ok&&!runtime_supply_sleep_body_pin(fake,original,sizeof original);
    runtime_supply_sleep=saved;SetLastError(error);return ok;
}
#endif
#endif
