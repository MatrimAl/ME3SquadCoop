#ifndef ME3_RUNTIME_SCENE_SPECTATOR_H
#define ME3_RUNTIME_SCENE_SPECTATOR_H
#include "scene_spectator_protocol.h"
/* Full host scenes can release the human companion. Viewing therefore binds
 * the current local PC and native owner connection, independently of pawn
 * possession and ordinary combat context. Ambient dialogue keeps gameplay.
 * Only camera outputs travel; no actor, story, possession or mode is written. */
typedef struct RuntimeSceneScope {
    GuestCameraStamp pc,world,root,driver,connection,peer,channel,camera,manager;
    int valid;
} RuntimeSceneScope;
typedef struct RuntimeSceneView {
    RuntimeSceneScope scope;
    SceneSpectatorMessage message;
    DWORD received_at;
    int valid;
} RuntimeSceneView;
static RuntimeSceneView runtime_scene_view;
static RuntimeSceneScope runtime_scene_host_scope;
static uint64_t runtime_scene_host_nonce,runtime_scene_host_sequence,runtime_scene_serial;
static DWORD runtime_scene_host_sent;
static int runtime_scene_host_active,runtime_scene_view_reported;
static DWORD runtime_scene_host_polled;
static unsigned runtime_scene_events;
static unsigned runtime_scene_samples;
static DWORD runtime_scene_sampled;
static int runtime_scene_camera_value(uintptr_t,SceneSpectatorMessage*);
static int runtime_scene_camera_requested(void);
#include "runtime_scene_camera.h"
static int runtime_scene_spectator_opted(void) {
    static int value=-1;
    if(value<0){wchar_t text[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_SCENE_SPECTATOR",text,8);
        value=!n||(n==1&&text[0]==L'1');}
    return value;
}
static void runtime_scene_note(const char *event,DWORD now) {
    if(runtime_scene_events++>=96)return;
    char row[300];snprintf(row,sizeof row,"{\"runtime_scene_spectator\":\"%s\",\"role\":%d,\"actors_moved\":false,\"gameplay_verified\":false,\"monotonic_ms\":%lu}\r\n",
        event,(int)g_runtime_config.role,now);line(row);
}
static int runtime_scene_fresh(int enabled,int active,DWORD received,DWORD now) {
    return enabled&&active&&now-received<=500;
}
static int runtime_scene_camera_requested(void) {
    return g_runtime_config.role==ME3_ROLE_GUEST&&runtime_scene_view.valid&&
        runtime_scene_fresh(runtime_scene_spectator_opted()&&runtime_control_enabled&&runtime_scope_ready,
            runtime_scene_view.message.active,runtime_scene_view.received_at,GetTickCount());
}
static int runtime_scene_mode(int mode) {return mode==7||mode==8;}
static int runtime_scene_message_order(const SceneSpectatorMessage *previous,int valid,const SceneSpectatorMessage *next) {
    return scene_spectator_message_valid(next)&&(!valid||next->nonce>previous->nonce||
        (next->nonce==previous->nonce&&scene_spectator_message_newer(next,previous->nonce,previous->sequence)));
}
static int runtime_scene_scope_live(uintptr_t base,const RuntimeSceneScope *s) {
    if(!s->valid||!runtime_control_enabled||!runtime_scope_ready||!runtime_scene_spectator_opted()||
       GetCurrentThreadId()!=session_game_thread)return 0;
    const GuestCameraStamp *a[]={&s->pc,&s->world,&s->root,&s->driver,&s->connection,&s->peer,&s->channel,&s->camera,&s->manager};
    for(unsigned i=0;i<9;++i)if(!guest_camera_stamp_matches(base,a[i],0))return 0;
    if(local_controller(session_engine)!=s->pc.pointer||read_ptr(base+0x18b6638)!=s->root.pointer||
       read_ptr(s->root.pointer+0x120)!=s->driver.pointer||read_ptr(s->pc.pointer+0x198)!=s->world.pointer||
       read_ptr(s->pc.pointer+0x684)!=s->camera.pointer||read_ptr(s->pc.pointer+0xe28)!=s->camera.pointer||
       read_ptr(s->camera.pointer+0x4a8)!=s->pc.pointer||read_ptr(s->camera.pointer+0x198)!=s->world.pointer||
       read_ptr(s->pc.pointer+0xc38)!=s->manager.pointer||read_ptr(s->connection.pointer+0x68)!=s->peer.pointer)return 0;
    NetworkActorIdentityEntry current={0};
    return network_actor_channel_valid(base,s->peer.pointer,s->channel.pointer,s->driver.pointer,&current)&&
        current.connection==s->connection.pointer;
}
static int runtime_scene_context(uintptr_t base,SessionContext c,NetworkActorScope *world) {
    uintptr_t player=read_ptr(c.pc+0x67c);
    return runtime_control_enabled&&runtime_scope_ready&&runtime_scene_spectator_opted()&&
        GetCurrentThreadId()==session_game_thread&&session_actor_live(base,c.pc)&&
        is_kind_of(base,c.pc,"SFXPlayerController")&&session_actor_live(base,c.world)&&
        is_kind_of(base,c.world,"BioWorldInfo")&&narrative_probe_object(base,player,"LocalPlayer")&&
        read_ptr(player+0x68)==c.pc&&network_actor_read_scope(base,c.pc,world)&&world->info==c.world;
}
static int runtime_scene_capture(uintptr_t base,SessionContext c,const NetworkActorScope *world,
        const NetworkActorIdentityEntry *peer,RuntimeSceneScope *out) {
    uintptr_t camera=read_ptr(c.pc+0x684),manager=read_ptr(c.pc+0xc38);
    if(!session_actor_live(base,camera)||!is_kind_of(base,camera,"SFXPlayerCamera")||
       read_ptr(c.pc+0xe28)!=camera||read_ptr(camera+0x4a8)!=c.pc||read_ptr(camera+0x198)!=c.world||
       !narrative_probe_object(base,manager,"SFXGameModeManager")||
       !runtime_power_property(base,camera,"Camera","CameraCache","StructProperty",0x448,32)||
       !runtime_power_property(base,manager,"SFXGameModeManager","CurrentMode","ByteProperty",0xa8,1))return 0;
    RuntimeSceneScope s={0};
    uintptr_t objects[]={c.pc,c.world,world->world,world->driver,peer->connection,peer->pawn,peer->channel,camera,manager};
    GuestCameraStamp *stamps[]={&s.pc,&s.world,&s.root,&s.driver,&s.connection,&s.peer,&s.channel,&s.camera,&s.manager};
    for(unsigned i=0;i<9;++i)if(!guest_camera_stamp_read(base,objects[i],stamps[i],1))return 0;
    s.valid=1;if(!runtime_scene_scope_live(base,&s))return 0;*out=s;return 1;
}
/* Queues scalars only in a real ReceivedBunch owner-PC frame. It never invokes
 * the camera or broadens helmet/revive reception to an unpossessed context. */
static int runtime_scene_receive(uintptr_t self,uintptr_t frame,const wchar_t *text,size_t count) {
    if(g_runtime_config.role!=ME3_ROLE_GUEST||!runtime_scene_spectator_opted())return 1;
    NativeRpcReceiveFrame *r=native_rpc_receive_current(session_game_thread);
    SceneSpectatorMessage message={0};NetworkActorScope world;NetworkActorIdentityEntry source={0};
    SessionContext c=session_context(session_engine);
    if(!r||!r->channel||!presentation_method_valid(1)||read_ptr(frame+0x18)!=presentation_methods[1].function.pointer||
       read_ptr(frame+0x20)!=self||self!=c.pc||!scene_spectator_parse(text,count,&message)||
       !runtime_scene_context(presentation_base,c,&world)||
       !network_actor_channel_valid(presentation_base,self,r->channel,world.driver,&source)||
       read_ptr(world.driver+0x80)!=source.connection||read_ptr(source.connection+0x68)!=self)return 1;
    RuntimeSceneView *v=&runtime_scene_view;
    int same=runtime_scene_scope_live(presentation_base,&v->scope)&&v->scope.channel.pointer==source.channel;
    if(!same){RuntimeSceneScope next;
        if(!runtime_scene_capture(presentation_base,c,&world,&source,&next))return 1;
        memset(v,0,sizeof *v);v->scope=next;
    }
    if(!runtime_scene_message_order(&v->message,v->valid,&message))return 1;
    v->message=message;v->received_at=GetTickCount();v->valid=1;return 1;
}
static int runtime_scene_camera_value(uintptr_t camera,SceneSpectatorMessage *out) {
    RuntimeSceneView *v=&runtime_scene_view;
    if(g_runtime_config.role!=ME3_ROLE_GUEST||camera!=v->scope.camera.pointer||!v->valid||
       !runtime_scene_fresh(runtime_scene_spectator_opted()&&runtime_control_enabled&&runtime_scope_ready,
           v->message.active,v->received_at,GetTickCount())||
       !runtime_scene_scope_live(presentation_base,&v->scope))return 0;
    *out=v->message;return 1;
}
static int runtime_scene_host_peer(uintptr_t base,SessionContext c,RuntimeSceneScope *out) {
    NetworkActorScope world;GameArray peers={0};NetworkActorIdentityEntry target={0};
    if(!runtime_scene_context(base,c,&world)||read_ptr(world.driver+0x80)||
       !identity_array(world.driver+0x70,&peers,64)||peers.count!=1)return 0;
    uintptr_t connection=read_ptr(peers.data),pc=read_ptr(connection+0x68);
    if(!network_actor_connection_in_scope(base,world.driver,connection)||!session_actor_live(base,pc)||
       pc==c.pc||!is_kind_of(base,pc,"SFXPlayerController")||read_ptr(pc+0x67c)!=connection||
       read_ptr(pc+0x198)!=c.world||network_actor_scan_driver(base,pc,world.driver,&target)<0||
       target.connection!=connection)return 0;
    return runtime_scene_capture(base,c,&world,&target,out);
}
static int runtime_scene_full_conversation(uintptr_t base,SessionContext c) {
    uintptr_t manager=read_ptr(c.world+0x948);GameArray active={0},after={0};
    if(!narrative_probe_object(base,manager,"BioConversationManager")||
       !scene_readiness_array(manager+0xc8,&active,16))return 0;
    int full=0;for(int i=0;i<active.count;++i){
        uintptr_t controller=read_ptr(active.data+(size_t)i*8);uint32_t flags=0;
        if(!narrative_probe_object(base,controller,"BioConversationController")||
           read_ptr(controller+0x28c)!=manager||!runtime_ambient_conversation_layout(base,controller)||
           !read_at(controller+0x358,&flags,4))return 0;
        /* Ended/failed/interrupted/over and camera-free ambient lines do not
         * create exclusive viewing. Require the actual host story player. */
        if(!(flags&(2|0x80|0x1000|0x2000|0x8000))&&read_ptr(controller+0x29c)==c.pawn&&
           session_actor_live(base,c.pawn)&&is_kind_of(base,c.pawn,"SFXPawn_Player"))full=1;
    }
    return full&&read_at(manager+0xc8,&after,sizeof after)&&!memcmp(&active,&after,sizeof active);
}
static void runtime_scene_spectator_tick(uintptr_t base,uintptr_t engine,int after,DWORD now) {
    presentation_scene_receive=runtime_scene_receive;
    runtime_scene_camera_retire(base);
    int enabled=runtime_scene_spectator_opted()&&runtime_control_enabled&&runtime_scope_ready;
    SessionContext c=session_context(engine);
    if(!enabled) {
        if(runtime_scene_view_reported)runtime_scene_note("native-view-restored-scope-ended",now);
        runtime_scene_view_reported=0;memset(&runtime_scene_view,0,sizeof runtime_scene_view);
        memset(&runtime_scene_host_scope,0,sizeof runtime_scene_host_scope);runtime_scene_host_active=0;return;
    }
    /* Existing receiver wrapper remains resident across scenes. Install the
     * owner-PC methods even when combat presentation opts out or is blocked. */
    presentation_install(base,c.pc);
    if(!presentation_installed)return;
    if(g_runtime_config.role==ME3_ROLE_GUEST) {
        if(!after)runtime_scene_camera_prepare(base,c.pc);
        SceneSpectatorMessage value;int viewing=runtime_scene_camera_value(read_ptr(c.pc+0x684),&value);
        if(viewing!=runtime_scene_view_reported){runtime_scene_note(viewing?"host-view-admitted":"host-view-admission-ended",now);
            runtime_scene_view_reported=viewing;}
        if(runtime_scene_view.valid&&!runtime_scene_scope_live(base,&runtime_scene_view.scope))
            memset(&runtime_scene_view,0,sizeof runtime_scene_view);
        return;
    }
    if(g_runtime_config.role!=ME3_ROLE_HOST||!after)return;
    if(runtime_scene_host_polled&&now-runtime_scene_host_polled<(runtime_scene_host_active?50u:100u))return;
    runtime_scene_host_polled=now;
    RuntimeSceneScope *s=&runtime_scene_host_scope;
    if(!runtime_scene_scope_live(base,s)) {
        memset(s,0,sizeof *s);runtime_scene_host_active=0;
        if(!runtime_scene_host_peer(base,c,s)||runtime_scene_serial==UINT64_MAX)return;
        runtime_scene_host_nonce=++runtime_scene_serial;runtime_scene_host_sequence=0;runtime_scene_host_sent=0;
    }
    unsigned char mode=255;
    if(!read_at(s->manager.pointer+0xa8,&mode,1))return;
    int active=runtime_scene_mode(mode);
    /* A full conversation can own the camera before its mode transition.
     * Ambient-only mode0 remains playable. Never use a movement warning as
     * evidence that the host has actually started a scene. */
    if(!active&&mode==0)active=runtime_scene_full_conversation(base,c);
    if(active==runtime_scene_host_active&&runtime_scene_host_sent&&
       (!active||now-runtime_scene_host_sent<50))return;
    SceneSpectatorMessage message={0};message.nonce=runtime_scene_host_nonce;
    if(runtime_scene_host_sequence==UINT64_MAX)return;
    message.sequence=++runtime_scene_host_sequence;message.active=active;
    if(active) {
        unsigned char pov[28];
        /* Native GetCameraViewPoint reads448/454; GetFOVAngle reads460.
         * CameraCache is POV then timestamp (not timestamp then POV). */
        if(!read_at(s->camera.pointer+0x448,pov,sizeof pov))return;
        memcpy(message.position,pov,12);memcpy(message.rotation,pov+12,12);memcpy(&message.fov,pov+24,4);
        uint32_t flags=0;
        if(!read_at(s->camera.pointer+0x504,&flags,4)||
           ((flags&1)&&!read_at(s->camera.pointer+0x4d4,&message.fov,4)))return;
    }
    wchar_t text[SCENE_SPECTATOR_MESSAGE_CAPACITY];int count=scene_spectator_format(&message,text);
    if(!count||!runtime_scene_scope_live(base,s)||!presentation_send_text(s->peer.pointer,1,text,count))return;
    if(active&&runtime_scene_samples<32&&(!runtime_scene_host_active||!runtime_scene_sampled||now-runtime_scene_sampled>=1000)) {
        ++runtime_scene_samples;runtime_scene_sampled=now;char row[600];
        snprintf(row,sizeof row,"{\"runtime_scene_spectator\":\"host-view-sent\",\"nonce\":%llu,\"sequence\":%llu,\"position\":[%.6f,%.6f,%.6f],\"rotation\":[%d,%d,%d],\"fov\":%.6f,\"monotonic_ms\":%lu}\r\n",
            (unsigned long long)message.nonce,(unsigned long long)message.sequence,message.position[0],message.position[1],message.position[2],
            message.rotation[0],message.rotation[1],message.rotation[2],message.fov,now);line(row);
    }
    if(active!=runtime_scene_host_active)runtime_scene_note(active?"host-scene-started":"host-scene-ended",now);
    runtime_scene_host_active=active;runtime_scene_host_sent=now;
}
#ifdef ME3_RUNTIME_FIXTURE
static int runtime_scene_spectator_selftest(void) {
    SceneSpectatorMessage a={1,2,0},b={1,3,0};
    if(!runtime_scene_message_order(&a,0,&b)||!runtime_scene_message_order(&a,1,&b))return 0;
    b.sequence=2;if(runtime_scene_message_order(&a,1,&b))return 0;
    b.sequence=1;if(runtime_scene_message_order(&a,1,&b))return 0;
    b.nonce=2;if(!runtime_scene_message_order(&a,1,&b))return 0;
    b.nonce=1;a.nonce=2;if(runtime_scene_message_order(&a,1,&b))return 0;
    return scene_spectator_protocol_selftest()&&runtime_scene_camera_selftest()&&
        runtime_scene_mode(7)&&runtime_scene_mode(8)&&!runtime_scene_mode(0)&&!runtime_scene_mode(3)&&
        !runtime_scene_mode(5)&&!runtime_scene_mode(255)&&runtime_scene_fresh(1,1,100,600)&&
        !runtime_scene_fresh(1,1,100,601)&&!runtime_scene_fresh(1,0,100,100)&&
        !runtime_scene_fresh(0,1,100,100)&&!runtime_scene_fresh(1,1,100,99)&&
        runtime_scene_fresh(1,1,UINT32_MAX-9,0);
}
#endif
#endif
