#ifndef ME3_RUNTIME_SCENE_INTERP_H
#define ME3_RUNTIME_SCENE_INTERP_H
#include "scene_interp_capability_protocol.h"
/* Enable the existing native Matinee transport for the selected authored
 * Mars scenes. Never replay activation, spawn a transport actor manually,
 * set a track clock/pose, or run guest sequence event outputs. */
typedef void (*RuntimeSceneInterpStart)(void*);
typedef void (*RuntimeSceneInterpEvent)(void*,void*,int);
typedef int (*RuntimeSceneInterpOutput)(void*,int);
typedef struct RuntimeSceneInterpHook {
    uintptr_t address,original;
    unsigned char entry[32],hook[16],trampoline[64];
    int installed;
} RuntimeSceneInterpHook;
typedef struct RuntimeSceneInterpLease {
    GuestCameraStamp action,world,root,level;
    int used;
} RuntimeSceneInterpLease;
typedef struct RuntimeSceneInterp {
    uintptr_t base,engine;
    RuntimeSceneInterpHook activated,start,event,output;
    RuntimeSceneScope capability;
    RuntimeSceneInterpLease leases[64];
    DWORD received,sent,checked,retired;
    unsigned rows;
    int capability_valid,attempted,busy;
} RuntimeSceneInterp;
static RuntimeSceneInterp runtime_scene_interp;
static void runtime_scene_interp_activated(void*);
static void runtime_scene_interp_start(void*);
static void runtime_scene_interp_event(void*,void*,int);
static int runtime_scene_interp_output(void*,int);
static int runtime_scene_interp_enabled(void) {
    static int enabled=-1;
    if(enabled<0){wchar_t text[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_SCENE_INTERP",text,8);
        enabled=!n||(n==1&&text[0]==L'1');}
    return enabled;
}
static void runtime_scene_interp_note(const char *event,uintptr_t action) {
    RuntimeSceneInterp *s=&runtime_scene_interp;if(s->rows++>=128)return;
    char row[1300],path[600]={0},safe[900];
    if(action&&is_live(s->base,action))identity_path(s->base,action,path,sizeof path);
    identity_quote(path,safe,sizeof safe);
    snprintf(row,sizeof row,"{\"runtime_scene_interp\":\"%s\",\"role\":%d,\"action_path\":\"%s\",\"manual_spawn\":false,\"activation_replayed\":false,\"track_clock_written\":false,\"full_cinematic_parity_verified\":false,\"monotonic_ms\":%lu}\r\n",
        event,(int)g_runtime_config.role,safe,GetTickCount());line(row);
}
static int runtime_scene_interp_path(const char *path) {
    static const char *packages[]={"BioD_ProMar_205ALiaraIntro.","BioD_ProMar_205ALiaraIntro_LOC_INT.",
        "BioD_ProMar_205BLiaraConv.","BioD_ProMar_205BLiaraConv_LOC_INT."};
    if(!path)return 0;
    for(unsigned i=0;i<4;++i)if(!strncmp(path,packages[i],strlen(packages[i])))return 1;
    return 0;
}
static int runtime_scene_interp_host_line(uintptr_t base,uintptr_t info,uintptr_t action) {
    uintptr_t manager=read_ptr(info+0x948);GameArray active={0},after={0};int found=0;
    if(!narrative_probe_object(base,manager,"BioConversationManager")||
       !scene_readiness_array(manager+0xc8,&active,16))return 0;
    for(int i=0;i<active.count;++i){uintptr_t controller=read_ptr(active.data+(SIZE_T)i*8);uint32_t flags=0;
        if(!narrative_probe_object(base,controller,"BioConversationController")||read_ptr(controller+0x28c)!=manager||
           !runtime_ambient_conversation_layout(base,controller)||!read_at(controller+0x358,&flags,4))return 0;
        if(!(flags&(2|0x80|0x1000|0x2000|0x8000))&&read_ptr(controller+0x2dc)==action)++found;
    }
    return found==1&&read_at(manager+0xc8,&after,sizeof after)&&!memcmp(&active,&after,sizeof active);
}
static int runtime_scene_interp_guest_channel(uintptr_t base,uintptr_t action,uintptr_t root) {
    uintptr_t driver=read_ptr(root+0x120),connection=read_ptr(driver+0x80);GameArray channels={0},after={0};
    int found=0;
    if(!network_actor_connection_in_scope(base,driver,connection)||
       !scene_readiness_array(connection+0x5f90,&channels,2048))return 0;
    for(int i=0;i<channels.count;++i){uintptr_t channel=read_ptr(channels.data+(SIZE_T)i*8),actor=read_ptr(channel+0xa0);
        if(!actor||read_ptr(actor+0x270)!=action)continue;
        NetworkActorIdentityEntry entry={0};
        if(!guest_camera_actor_live(base,actor)||!is_kind_of(base,actor,"MatineeActor")||
           !network_actor_channel_valid(base,actor,channel,driver,&entry)||entry.connection!=connection)return 0;
        ++found;
    }
    return found==1&&read_at(connection+0x5f90,&after,sizeof after)&&!memcmp(&channels,&after,sizeof channels);
}
static int runtime_scene_interp_action(uintptr_t base,uintptr_t action,uintptr_t info,
        GuestCameraStamp *stamp,uintptr_t *level_out,uintptr_t *root_out) {
    uintptr_t root=0,level=0;
    if(!narrative_probe_object(base,action,"SeqAct_Interp")||read_ptr(action)!=base+0x11e9d18||
       !guest_camera_stamp_read(base,action,stamp,1)||!runtime_scene_interp_path(stamp->full_name)||
       !runtime_power_property(base,action,"SeqAct_Interp","bClientSideOnly","BoolProperty",0x180,4)||
       !runtime_native_hud_bool(base,read_ptr(action+0x50),"bClientSideOnly",0x180,0x2000)||
       !runtime_power_property(base,action,"SeqAct_Interp","GroupInst","ArrayProperty",0x12c,16)||
       !runtime_power_property(base,action,"SeqAct_Interp","ReplicatedActor","ObjectProperty",0x15c,8)||
       !runtime_power_property(base,action,"SeqAct_Latent","LatentActors","ArrayProperty",0xf8,16))return 0;
    CombatContextChain chain;combat_context_read_chain(base,&chain);
    if(!chain.info_valid||chain.info!=info)return 0;
    level=runtime_story_graph_level(base,action,&root);
    if(level){if(chain.world!=root)return 0;}
    else {
        /* Localized line actions live under a conversation package rather
         * than Level. Bind them to the actual current host line or the
         * guest's actual native transport channel, never a guessed level. */
        root=chain.world;
        if(g_runtime_config.role==ME3_ROLE_HOST){if(!runtime_scene_interp_host_line(base,info,action))return 0;}
        else if(g_runtime_config.role==ME3_ROLE_GUEST){if(!runtime_scene_interp_guest_channel(base,action,root))return 0;}
        else return 0;
    }
    if(read_ptr(base+0x18b6638)!=root)return 0;
    if(level_out)*level_out=level;if(root_out)*root_out=root;return 1;
}
static int runtime_scene_interp_hook_live(const RuntimeSceneInterpHook *h) {
    unsigned char current[16],tail[16],trampoline[64];
    return h->installed&&h->address&&h->original&&
        read_at(h->address,current,16)&&!memcmp(current,h->hook,16)&&
        read_at(h->address+16,tail,16)&&!memcmp(tail,h->entry+16,16)&&
        read_at(h->original,trampoline,sizeof trampoline)&&!memcmp(trampoline,h->trampoline,sizeof trampoline);
}
static int runtime_scene_interp_hook_install(uintptr_t base,RuntimeSceneInterpHook *h,uintptr_t rva,
        const unsigned char entry[32],void *replacement) {
    if(h->installed)return runtime_scene_interp_hook_live(h);
    unsigned char current[32];
    if(!read_at(base+rva,current,sizeof current)||memcmp(current,entry,sizeof current))return 0;
    MH_STATUS status=MH_Initialize();if(status!=MH_OK&&status!=MH_ERROR_ALREADY_INITIALIZED)return 0;
    LPVOID original=NULL;
    if(MH_CreateHook((void*)(base+rva),replacement,&original)!=MH_OK)return 0;
    h->address=base+rva;h->original=(uintptr_t)original;memcpy(h->entry,entry,32);
    if(!original||!read_at((uintptr_t)original,h->trampoline,64)||MH_EnableHook((void*)(base+rva))!=MH_OK||
       !read_at(base+rva,h->hook,16)){
        MH_DisableHook((void*)(base+rva));MH_RemoveHook((void*)(base+rva));h->original=0;return 0;
    }
    h->installed=1;return runtime_scene_interp_hook_live(h);
}
static int runtime_scene_interp_cap_fresh(int valid,DWORD received,DWORD now) {
    return valid&&now-received<=2000;
}
static int runtime_scene_interp_capability_live(DWORD now) {
    RuntimeSceneInterp *s=&runtime_scene_interp;
    return runtime_scene_interp_enabled()&&g_runtime_config.role==ME3_ROLE_HOST&&
        runtime_scene_interp_cap_fresh(s->capability_valid,s->received,now)&&
        runtime_scene_interp_hook_live(&s->activated)&&runtime_scene_interp_hook_live(&s->start)&&
        runtime_scene_scope_live(s->base,&s->capability);
}
static int runtime_scene_interp_receive(uintptr_t self,uintptr_t frame,const wchar_t *text,size_t count) {
    RuntimeSceneInterp *s=&runtime_scene_interp;NativeRpcReceiveFrame *r=native_rpc_receive_current(session_game_thread);
    NetworkActorIdentityEntry source={0};RuntimeSceneScope scope={0};SessionContext c=session_context(s->engine);
    if(!runtime_scene_interp_enabled()||g_runtime_config.role!=ME3_ROLE_HOST||!scene_interp_capability_parse(text,count)||
       !r||!r->channel||!presentation_method_valid(0)||read_ptr(frame+0x18)!=presentation_methods[0].function.pointer||
       read_ptr(frame+0x20)!=self||!runtime_scene_host_peer(s->base,c,&scope)||self!=scope.peer.pointer||
       r->channel!=scope.channel.pointer||
       !network_actor_channel_valid(s->base,self,r->channel,scope.driver.pointer,&source)||
       source.connection!=scope.connection.pointer||read_ptr(source.connection+0x68)!=self)return 1;
    s->capability=scope;s->received=GetTickCount();
    if(!s->capability_valid)runtime_scene_interp_note("guest-event-guard-ready",0);
    s->capability_valid=1;return 1;
}
static int runtime_scene_interp_bit(uintptr_t address,int restore) {
    uint32_t before=0;
    if((address&3)||!background_focus_writable(address,4)||!read_at(address,&before,4))return 0;
    if(!restore&&!(before&0x2000))return 0;
    uint32_t desired=restore?before|0x2000:before&~UINT32_C(0x2000);
    return (uint32_t)InterlockedCompareExchange((LONG volatile*)address,(LONG)desired,(LONG)before)==before;
}
static int runtime_scene_interp_lease_live(const RuntimeSceneInterpLease *l) {
    RuntimeSceneInterp *s=&runtime_scene_interp;
    return l->used&&guest_camera_stamp_matches(s->base,&l->action,1)&&
        guest_camera_stamp_matches(s->base,&l->world,0)&&guest_camera_stamp_matches(s->base,&l->root,0)&&
        (!l->level.pointer||guest_camera_stamp_matches(s->base,&l->level,0))&&read_ptr(s->base+0x18b6638)==l->root.pointer&&
        (!l->level.pointer||runtime_story_graph_same_level(s->base,l->action.pointer,l->level.pointer,l->root.pointer));
}
static void runtime_scene_interp_admit(uintptr_t action) {
    RuntimeSceneInterp *s=&runtime_scene_interp;DWORD now=GetTickCount();
    if(!runtime_scene_interp_capability_live(now)||GetCurrentThreadId()!=session_game_thread||s->busy)return;
    GuestCameraStamp stamp={0};uintptr_t level=0,root=0;
    if(!runtime_scene_interp_action(s->base,action,s->capability.world.pointer,&stamp,&level,&root))return;
    for(unsigned i=0;i<64;++i)if(s->leases[i].used&&s->leases[i].action.pointer==action)return;
    uint32_t flags=0,actor_flags=0;uintptr_t actor=1;GameArray groups={0};
    if(!read_at(action+0x180,&flags,4)||!(flags&0x2000)||(flags&0x400000)||
       !read_at(action+0x15c,&actor,8)||!scene_readiness_array(action+0x12c,&groups,128)||groups.count)return;
    /* Native actions may retain and reuse a finished transport actor. */
    if(actor&&(!guest_camera_actor_live(s->base,actor)||!is_kind_of(s->base,actor,"MatineeActor")||
       read_ptr(actor+0x270)!=action||read_ptr(actor+0x198)!=s->capability.world.pointer||
       !read_at(actor+0x284,&actor_flags,4)||(actor_flags&1)))return;
    RuntimeSceneInterpLease next={0};next.action=stamp;
    if(!guest_camera_stamp_read(s->base,s->capability.world.pointer,&next.world,0)||
       !guest_camera_stamp_read(s->base,root,&next.root,0)||(level&&!guest_camera_stamp_read(s->base,level,&next.level,0)))return;
    unsigned slot=64;for(unsigned i=0;i<64;++i)if(!s->leases[i].used){slot=i;break;}if(slot==64)return;
    next.used=1;
    if(!runtime_scene_interp_capability_live(GetTickCount())||!guest_camera_stamp_matches(s->base,&stamp,1)||
       !runtime_scene_interp_bit(action+0x180,0))return;
    s->leases[slot]=next;runtime_scene_interp_note("native-scene-transport-admitted",action);
}
static void runtime_scene_interp_start_forward(void *self,RuntimeSceneInterpStart original,
        void (*admit)(uintptr_t)) {
    DWORD error=GetLastError();admit((uintptr_t)self);SetLastError(error);original(self);
}
static void runtime_scene_interp_activated(void *self) {
    runtime_scene_interp_start_forward(self,(RuntimeSceneInterpStart)runtime_scene_interp.activated.original,runtime_scene_interp_admit);
}
static void runtime_scene_interp_start(void *self) {
    runtime_scene_interp_start_forward(self,(RuntimeSceneInterpStart)runtime_scene_interp.start.original,runtime_scene_interp_admit);
}
/* Story outputs have no client authority. Keep the native EventTrack update
 * itself running so its crossed-key/last-position bookkeeping is unchanged.
 * Exact authored selected actions in the current guest world are refused even
 * before their first channel can be correlated; no local replay is granted. */
static int runtime_scene_interp_event_owned(uintptr_t action,uintptr_t track,int key) {
    RuntimeSceneInterp *s=&runtime_scene_interp;
    if(GetCurrentThreadId()!=session_game_thread||g_runtime_config.role!=ME3_ROLE_GUEST)return 0;
    GuestCameraStamp stamp={0};
    /* Shared output dispatch is hot for unrelated sequence nodes. Reject by
     * exact native table before any ownership/world/class traversal. */
    if(read_ptr(action)!=s->base+0x11e9d18||!narrative_probe_object(s->base,action,"SeqAct_Interp")||
       !guest_camera_stamp_read(s->base,action,&stamp,1)||!runtime_scene_interp_path(stamp.full_name))return 0;
    SessionContext c=session_context(s->engine);NetworkActorScope world={0};
    uintptr_t connection=0;
    /* The resident authority guard outlives opt-out/control release. Otherwise
     * disabling during a replicated line could re-enable its story outputs. */
    if(!session_actor_live(s->base,c.pc)||!is_kind_of(s->base,c.pc,"SFXPlayerController")||
       !network_actor_read_scope(s->base,c.pc,&world)||world.info!=c.world||world.mode!=3||
       !(connection=read_ptr(world.driver+0x80))||
       !network_actor_connection_in_scope(s->base,world.driver,connection)||read_ptr(connection+0x68)!=c.pc)return 0;
    (void)track;(void)key;
    /* All EventTrack outputs for this exact selected action are host-owned,
     * including malformed keys. Other track methods and assets stay native. */
    runtime_scene_interp_note("guest-sequence-event-withheld",action);return 1;
}
static void runtime_scene_interp_event_forward(void *self,void *track,int key,RuntimeSceneInterpEvent original,
        int (*owned)(uintptr_t,uintptr_t,int)) {
    DWORD error=GetLastError();int consume=owned((uintptr_t)self,(uintptr_t)track,key);
    SetLastError(error);if(!consume)original(self,track,key);
}
static void runtime_scene_interp_event(void *self,void *track,int key) {
    runtime_scene_interp_event_forward(self,track,key,(RuntimeSceneInterpEvent)runtime_scene_interp.event.original,
        runtime_scene_interp_event_owned);
}
/* Native ActivateOutputLink returns a Boolean and sets only the selected
 * output's impulse. Suppress that mutation for guest scene actions, including
 * Completed/Reversed; keep its real return value for every other caller. */
static int runtime_scene_interp_output_forward(void *self,int index,RuntimeSceneInterpOutput original,
        int (*owned)(uintptr_t,uintptr_t,int)) {
    DWORD error=GetLastError();int consume=owned((uintptr_t)self,0,index);
    SetLastError(error);return consume?0:original(self,index);
}
static int runtime_scene_interp_output(void *self,int index) {
    return runtime_scene_interp_output_forward(self,index,(RuntimeSceneInterpOutput)runtime_scene_interp.output.original,
        runtime_scene_interp_event_owned);
}
static void runtime_scene_interp_retire(DWORD now) {
    RuntimeSceneInterp *s=&runtime_scene_interp;if(s->retired&&now-s->retired<100)return;s->retired=now;
    for(unsigned i=0;i<64;++i){RuntimeSceneInterpLease *l=&s->leases[i];if(!l->used)continue;
        if(!runtime_scene_interp_lease_live(l)){
            /* Never dereference/rewrite an expired world/action identity. */
            if(!guest_camera_stamp_matches(s->base,&l->action,0)||!guest_camera_stamp_matches(s->base,&l->root,0))memset(l,0,sizeof *l);
            continue;
        }
        uint32_t flags=0,actor_flags=0;uintptr_t actor=0;GameArray groups={0},latent={0};
        if(!read_at(l->action.pointer+0x180,&flags,4)||(flags&(2|0x400000))||
           !scene_readiness_array(l->action.pointer+0x12c,&groups,128)||groups.count||
           !scene_readiness_array(l->action.pointer+0xf8,&latent,256)||latent.count||
           !read_at(l->action.pointer+0x15c,&actor,8))continue;
        if(actor&&(!guest_camera_actor_live(s->base,actor)||!is_kind_of(s->base,actor,"MatineeActor")||
           read_ptr(actor+0x270)!=l->action.pointer||!read_at(actor+0x284,&actor_flags,4)||(actor_flags&1)))continue;
        if((flags&0x2000)||runtime_scene_interp_bit(l->action.pointer+0x180,1)){
            runtime_scene_interp_note("native-scene-flag-retired",l->action.pointer);memset(l,0,sizeof *l);
        }
    }
}
static void runtime_scene_interp_tick(uintptr_t base,uintptr_t engine,DWORD now) {
    DWORD error=GetLastError();RuntimeSceneInterp *s=&runtime_scene_interp;
    s->base=base;s->engine=engine;presentation_interp_receive=runtime_scene_interp_receive;
    if(GetCurrentThreadId()!=session_game_thread)goto done;
    runtime_scene_interp_retire(now);
    if(!runtime_scene_interp_enabled()||!runtime_control_enabled||!runtime_scope_ready){s->capability_valid=0;goto done;}
    if(!s->attempted){
        static const unsigned char activated[32]={0x48,0x8b,0xc4,0x55,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x8d,0x68,0xa1,0x48,0x81,0xec,0xb0,0x00,0x00,0x00,0x48,0xc7,0x45,0xf7,0xfe,0xff,0xff,0xff,0x48};
        static const unsigned char start[32]={0x48,0x8b,0xc4,0x53,0x48,0x81,0xec,0xa0,0x00,0x00,0x00,0xf6,0x81,0x80,0x01,0x00,0x00,0x02,0x48,0x8b,0xd9,0x0f,0x84,0xbe,0x01,0x00,0x00,0x48,0x89,0x70,0x18,0x4c};
        static const unsigned char event[32]={0x48,0x85,0xd2,0x0f,0x84,0xc3,0x00,0x00,0x00,0x57,0x48,0x83,0xec,0x40,0x48,0xc7,0x44,0x24,0x20,0xfe,0xff,0xff,0xff,0x48,0x89,0x5c,0x24,0x50,0x4d,0x63,0xc8,0x48};
        static const unsigned char output[32]={0x85,0xd2,0x78,0x29,0x3b,0x91,0xa0,0x00,0x00,0x00,0x7d,0x21,0x48,0x63,0xc2,0x48,0x6b,0xd0,0x34,0x48,0x03,0x91,0x98,0x00,0x00,0x00,0x8b,0x42,0x30,0xa8,0x02,0x75};
        s->attempted=1;
        if(!supported_image(base))goto done;
        int prepared=g_runtime_config.role==ME3_ROLE_GUEST?
            runtime_scene_interp_hook_install(base,&s->event,0x56a740,event,(void*)runtime_scene_interp_event)&&
            runtime_scene_interp_hook_install(base,&s->output,0x71afb0,output,(void*)runtime_scene_interp_output):
            g_runtime_config.role==ME3_ROLE_HOST&&
            runtime_scene_interp_hook_install(base,&s->activated,0x555850,activated,(void*)runtime_scene_interp_activated)&&
            runtime_scene_interp_hook_install(base,&s->start,0x564c00,start,(void*)runtime_scene_interp_start);
        runtime_scene_interp_note(prepared?"native-transport-boundary-prepared":"native-transport-boundary-refused",0);
    }
    if(g_runtime_config.role!=ME3_ROLE_GUEST||!runtime_scene_interp_hook_live(&s->event)||!runtime_scene_interp_hook_live(&s->output)||
       (s->sent&&now-s->sent<500))goto done;
    RuntimeSceneScope scope={0};wchar_t text[SCENE_INTERP_CAPABILITY_CAPACITY];
    if(!runtime_scene_actor_observer_scope(base,engine,&scope))goto done;
    int count=scene_interp_capability_format(text);
    if(count&&runtime_scene_scope_live(base,&scope)&&presentation_send_text(scope.pc.pointer,0,text,count))s->sent=now;
 done:SetLastError(error);
}
#ifdef ME3_RUNTIME_FIXTURE
static unsigned runtime_scene_interp_fixture_calls,runtime_scene_interp_fixture_admits;
static int runtime_scene_interp_fixture_bad,runtime_scene_interp_fixture_consume;
static void runtime_scene_interp_fixture_admit(uintptr_t self){if(self!=16)runtime_scene_interp_fixture_bad=1;++runtime_scene_interp_fixture_admits;SetLastError(11);}
static void runtime_scene_interp_fixture_start(void *self){if(self!=(void*)16||GetLastError()!=37)runtime_scene_interp_fixture_bad=1;++runtime_scene_interp_fixture_calls;SetLastError(53);}
static int runtime_scene_interp_fixture_owned(uintptr_t self,uintptr_t track,int key){if(self!=16||track!=24||key!=3)runtime_scene_interp_fixture_bad=1;SetLastError(11);return runtime_scene_interp_fixture_consume;}
static void runtime_scene_interp_fixture_event(void *self,void *track,int key){if(self!=(void*)16||track!=(void*)24||key!=3||GetLastError()!=37)runtime_scene_interp_fixture_bad=1;++runtime_scene_interp_fixture_calls;SetLastError(53);}
static int runtime_scene_interp_fixture_output_owned(uintptr_t self,uintptr_t track,int key){if(self!=16||track||key!=3)runtime_scene_interp_fixture_bad=1;SetLastError(11);return runtime_scene_interp_fixture_consume;}
static int runtime_scene_interp_fixture_output(void *self,int index){if(self!=(void*)16||index!=3||GetLastError()!=37)runtime_scene_interp_fixture_bad=1;++runtime_scene_interp_fixture_calls;SetLastError(53);return 7;}
static int runtime_scene_interp_selftest(void) {
    DWORD error=GetLastError();_Alignas(4) uint32_t flags=0x2000|0x40000|2;int ok=1;
    ok=ok&&runtime_scene_interp_bit((uintptr_t)&flags,0)&&flags==(0x40000|2);
    flags|=0x10000;ok=ok&&runtime_scene_interp_bit((uintptr_t)&flags,1)&&flags==(0x2000|0x40000|2|0x10000)&&
        !runtime_scene_interp_bit((uintptr_t)&flags+1,0)&&
        runtime_scene_interp_path("BioD_ProMar_205ALiaraIntro.TheWorld.PersistentLevel.Action")&&
        runtime_scene_interp_path("BioD_ProMar_205BLiaraConv_LOC_INT.Conv.Line")&&
        !runtime_scene_interp_path("BioD_ProMar_205BLiaraConvForeign.Conv.Line")&&
        !runtime_scene_interp_path("Other.TheWorld.Action")&&
        runtime_scene_interp_cap_fresh(1,100,2100)&&!runtime_scene_interp_cap_fresh(1,100,2101)&&
        !runtime_scene_interp_cap_fresh(0,100,100)&&runtime_scene_interp_cap_fresh(1,UINT32_MAX-9,0);
    runtime_scene_interp_fixture_calls=runtime_scene_interp_fixture_admits=0;runtime_scene_interp_fixture_bad=0;
    SetLastError(37);runtime_scene_interp_start_forward((void*)16,runtime_scene_interp_fixture_start,runtime_scene_interp_fixture_admit);
    ok=ok&&runtime_scene_interp_fixture_calls==1&&runtime_scene_interp_fixture_admits==1&&!runtime_scene_interp_fixture_bad&&GetLastError()==53;
    for(int consume=0;consume<2;++consume){runtime_scene_interp_fixture_calls=0;runtime_scene_interp_fixture_consume=consume;
        SetLastError(37);runtime_scene_interp_event_forward((void*)16,(void*)24,3,runtime_scene_interp_fixture_event,runtime_scene_interp_fixture_owned);
        ok=ok&&runtime_scene_interp_fixture_calls==(unsigned)!consume&&!runtime_scene_interp_fixture_bad&&GetLastError()==(consume?37u:53u);
        runtime_scene_interp_fixture_calls=0;SetLastError(37);
        int result=runtime_scene_interp_output_forward((void*)16,3,runtime_scene_interp_fixture_output,runtime_scene_interp_fixture_output_owned);
        ok=ok&&result==(consume?0:7)&&runtime_scene_interp_fixture_calls==(unsigned)!consume&&
            !runtime_scene_interp_fixture_bad&&GetLastError()==(consume?37u:53u);
    }
    SetLastError(error);return ok&&scene_interp_capability_protocol_selftest();
}
#endif
#endif
