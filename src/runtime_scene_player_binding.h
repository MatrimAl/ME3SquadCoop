#ifndef ME3_RUNTIME_SCENE_PLAYER_BINDING_H
#define ME3_RUNTIME_SCENE_PLAYER_BINDING_H
#include "runtime_scene_owner_binding.h"
/* The native Player selector returns WorldInfo.Squad.PlayerPawn, which is
 * Ashley during the registered guest context. Reuse the existing authenticated
 * host-Shepard channel proof; no new role RPC or guessed controller is needed.
 * Include after runtime_scene_interp.h. */
typedef uintptr_t (*RuntimeSceneActorResolve)(void*,void*);
typedef uintptr_t (*RuntimeSceneGroupResolve)(void*);
typedef struct RuntimeScenePlayerBinding {
    uintptr_t base,engine;
    RuntimeSceneInterpHook resolver,group_resolver;
    RuntimeSceneScope scope;
    GuestCameraStamp actor,channel;
    int index,valid,attempted,busy;
    unsigned rows;
} RuntimeScenePlayerBinding;
static RuntimeScenePlayerBinding runtime_scene_player_binding;
static uintptr_t runtime_scene_player_resolve(void*,void*);
static uintptr_t runtime_scene_player_group_resolve(void*);
static void runtime_scene_player_note(const char *event,uintptr_t actor) {
    RuntimeScenePlayerBinding *s=&runtime_scene_player_binding;if(s->rows++>=64)return;
    char row[450];snprintf(row,sizeof row,
        "{\"runtime_scene_player_binding\":\"%s\",\"actor_index\":%d,\"actor_channel\":%d,\"actor\":\"%p\",\"group_actor_written\":false,\"tree_reset\":false,\"track_clock_written\":false,\"cinematic_parity_verified\":false,\"monotonic_ms\":%lu}\r\n",
        event,s->actor.index,s->index,(void*)actor,GetTickCount());line(row);
}
static int runtime_scene_player_live(void) {
    RuntimeScenePlayerBinding *s=&runtime_scene_player_binding;NetworkActorIdentityEntry current={0};unsigned char role=255;
    return s->valid&&runtime_scene_interp_enabled()&&g_runtime_config.role==ME3_ROLE_GUEST&&
        runtime_scene_scope_live(s->base,&s->scope)&&guest_camera_stamp_matches(s->base,&s->actor,0)&&
        guest_camera_stamp_matches(s->base,&s->channel,0)&&guest_camera_actor_live(s->base,s->actor.pointer)&&
        is_kind_of(s->base,s->actor.pointer,"SFXPawn_Player")&&read_ptr(s->actor.pointer+0x198)==s->scope.world.pointer&&
        read_at(s->actor.pointer+0x26a,&role,1)&&role==1&&
        network_actor_channel_valid(s->base,s->actor.pointer,s->channel.pointer,s->scope.driver.pointer,&current)&&
        current.connection==s->scope.connection.pointer&&current.index==s->index;
}
/* Called only after the ordinary appearance receiver has authenticated its
 * native owner-PC source and matched the outstanding Q/channel request to the
 * host's actual session_shepard. This captures identities only, with no event
 * invocation. A scene may release Ashley afterwards without retiring them. */
static void runtime_scene_player_confirmed(const PresentationGuestScope *proof,const NetworkActorIdentityEntry *source) {
    RuntimeScenePlayerBinding *s=&runtime_scene_player_binding;SessionContext c=session_context(s->engine);
    NetworkActorScope world={0};RuntimeSceneScope scope={0};NetworkActorIdentityEntry actor={0};unsigned char role=255;
    if(!runtime_scene_interp_enabled()||g_runtime_config.role!=ME3_ROLE_GUEST||GetCurrentThreadId()!=session_game_thread||
       !proof||!proof->valid||!source||!presentation_guest_valid(s->base)||
       c.pc!=proof->objects[0].pointer||c.world!=proof->objects[3].pointer||source->pawn!=c.pc||
       source->connection!=proof->objects[4].pointer||source->driver!=proof->objects[5].pointer||
       !network_actor_read_scope(s->base,c.pc,&world)||world.info!=c.world||world.driver!=source->driver||
       !runtime_scene_capture(s->base,c,&world,source,&scope)||
       !guest_camera_stamp_matches(s->base,&proof->objects[2],1)||!guest_camera_stamp_matches(s->base,&proof->objects[6],1)||
       !guest_camera_actor_live(s->base,proof->objects[2].pointer)||!is_kind_of(s->base,proof->objects[2].pointer,"SFXPawn_Player")||
       !read_at(proof->objects[2].pointer+0x26a,&role,1)||role!=1||
       !network_actor_channel_valid(s->base,proof->objects[2].pointer,proof->objects[6].pointer,world.driver,&actor)||
       actor.connection!=source->connection||actor.index!=proof->index)return;
    int changed=!s->valid||!guest_camera_stamp_core_equal(&s->actor,&proof->objects[2])||
        !guest_camera_stamp_core_equal(&s->channel,&proof->objects[6])||s->scope.connection.pointer!=source->connection;
    s->scope=scope;s->actor=proof->objects[2];s->channel=proof->objects[6];s->index=proof->index;s->valid=1;
    if(changed)runtime_scene_player_note("authenticated-host-shepard-captured",s->actor.pointer);
}
static int runtime_scene_player_ready(void) {
    return runtime_scene_interp_hook_live(&runtime_scene_player_binding.resolver)&&
        runtime_scene_interp_hook_live(&runtime_scene_player_binding.group_resolver)&&runtime_scene_player_live();
}
static int runtime_scene_player_kind(int native,int mode,uint32_t number,const char *name) {
    if(!native||mode||number||!name)return 0;
    return !strcmp(name,"Player")?1:!strcmp(name,"Owner")?2:0;
}
static int runtime_scene_player_selector(uintptr_t base,uintptr_t track) {
    unsigned char mode=255;uint32_t name[2]={0};char text[80];
    if(!narrative_probe_object(base,track,"SFXGameActorInterpTrack")||
       read_ptr(read_ptr(track)+0x360)!=base+0xbb4700||
       !runtime_power_property(base,track,"SFXGameActorInterpTrack","m_nmFindActor","NameProperty",0xa8,8)||
       !runtime_power_property(base,track,"SFXGameActorInterpTrack","m_eFindActorMode","ByteProperty",0xb0,1)||
       !read_at(track+0xb0,&mode,1)||mode||!read_at(track+0xa8,name,8)||name[1])return 0;
    name_of(base,name[0],text,sizeof text);
    return runtime_scene_player_kind(1,mode,name[1],text);
}
/* Natural initialization supplies both the actual track and track instance.
 * Require the reciprocal action/group/parallel track arrays, and a unique
 * native host Matinee actor channel for this exact selected scene. */
static int runtime_scene_player_context(uintptr_t track,uintptr_t instance,uintptr_t *action_out) {
    RuntimeScenePlayerBinding *s=&runtime_scene_player_binding;uintptr_t group_instance=read_ptr(instance+0x40);
    uintptr_t action=read_ptr(group_instance+0x40),group=read_ptr(group_instance+0x70),data=read_ptr(action+0x154);
    GameArray groups={0},tracks={0},instances={0},after={0};int found_group=0,found_track=0;
    GuestCameraStamp stamp={0};
    if(!runtime_scene_player_ready()||!narrative_probe_object(s->base,instance,"SFXGameActorInterpTrackInst")||
       !narrative_probe_object(s->base,group_instance,"InterpGroupInst")||
       !narrative_probe_object(s->base,group,"InterpGroup")||read_ptr(track+0x40)!=group||
       !narrative_probe_object(s->base,data,"InterpData")||read_ptr(group+0x40)!=data||
       !runtime_scene_interp_action(s->base,action,s->scope.world.pointer,&stamp,NULL,NULL)||
       !runtime_scene_interp_guest_channel(s->base,action,s->scope.root.pointer)||
       !scene_readiness_array(action+0x12c,&groups,128)||!scene_readiness_array(group+0x68,&tracks,64)||
       !scene_readiness_array(group_instance+0x60,&instances,64)||tracks.count!=instances.count)return 0;
    for(int i=0;i<groups.count;++i)if(read_ptr(groups.data+(SIZE_T)i*8)==group_instance)++found_group;
    for(int i=0;i<tracks.count;++i)if(read_ptr(tracks.data+(SIZE_T)i*8)==track&&
        read_ptr(instances.data+(SIZE_T)i*8)==instance)++found_track;
    if(found_group!=1||found_track!=1||!read_at(action+0x12c,&after,sizeof after)||memcmp(&groups,&after,sizeof after)||
       !read_at(group+0x68,&after,sizeof after)||memcmp(&tracks,&after,sizeof after)||
       !read_at(group_instance+0x60,&after,sizeof after)||memcmp(&instances,&after,sizeof after))return 0;
    if(action_out)*action_out=action;return 1;
}
typedef uintptr_t (*RuntimeScenePlayerSelect)(void*,void*,uintptr_t);
static uintptr_t runtime_scene_player_forward(void *track,void *instance,RuntimeSceneActorResolve original,
        RuntimeScenePlayerSelect select) {
    uintptr_t actor=original(track,instance);DWORD outgoing=GetLastError();
    uintptr_t selected=select(track,instance,actor);SetLastError(outgoing);return selected;
}
static uintptr_t runtime_scene_player_select(void *track,void *instance,uintptr_t original) {
    RuntimeScenePlayerBinding *s=&runtime_scene_player_binding;uintptr_t action=0;
    if(s->busy||GetCurrentThreadId()!=session_game_thread||g_runtime_config.role!=ME3_ROLE_GUEST||
       !runtime_scene_interp_enabled()||!runtime_control_enabled||!runtime_scope_ready||!instance||
       !runtime_scene_interp_hook_live(&s->resolver))return original;
    int selector=runtime_scene_player_selector(s->base,(uintptr_t)track);
    if((selector!=1&&(selector!=2||original))||!runtime_scene_player_context((uintptr_t)track,(uintptr_t)instance,&action))return original;
    uintptr_t player=s->actor.pointer;
    if(selector==2){NetworkActorScope world={0};
        if(!network_actor_read_scope(s->base,s->scope.pc.pointer,&world))return original;
        player=runtime_scene_owner_actor(s->base,action,s->scope.world.pointer,&world);
        if(!player)return original;
    }
    if(original==player)return original;
    /* Native content validation runs after this resolver returns. Keep every
     * non-Player selector, UseGroupActor mode and engine group binding stock. */
    if(!runtime_scene_player_live()||runtime_scene_player_selector(s->base,(uintptr_t)track)!=selector)return original;
    runtime_scene_player_note(selector==1?"native-player-selector-bound-to-host":"native-owner-selector-resolved",player);return player;
}
static uintptr_t runtime_scene_player_resolve(void *track,void *instance) {
    return runtime_scene_player_forward(track,instance,(RuntimeSceneActorResolve)runtime_scene_player_binding.resolver.original,
        runtime_scene_player_select);
}
/* InitGroupInst normally resolves the authored group selector before it
 * stores GroupActor and creates/initializes tracks. The original getter may
 * cache its native result, but this exact caller stores the returned actor
 * again. Returning the selected existing actor here lets native initialization
 * own that assignment; every later getter and mode2 path remain stock. */
static uintptr_t runtime_scene_player_group_select(void *self,uintptr_t original,uintptr_t caller) {
    RuntimeScenePlayerBinding *s=&runtime_scene_player_binding;uintptr_t instance=(uintptr_t)self;
    uintptr_t action=read_ptr(instance+0x40),group=read_ptr(instance+0x70),data=read_ptr(action+0x154);
    GameArray groups={0},data_groups={0},after={0};unsigned char mode=255;uint32_t name[2]={0};char text[80];
    GuestCameraStamp stamp={0};int found_instance=0,found_group=0;
    if(caller!=s->base+0x5630dc||GetCurrentThreadId()!=session_game_thread||g_runtime_config.role!=ME3_ROLE_GUEST||
       !runtime_scene_player_ready()||!narrative_probe_object(s->base,instance,"InterpGroupInst")||
       read_ptr(read_ptr(instance)+0x2b0)!=s->base+0x55e620||
       !narrative_probe_object(s->base,group,"InterpGroup")||!narrative_probe_object(s->base,data,"InterpData")||
       read_ptr(group+0x40)!=data||!runtime_power_property(s->base,group,"InterpGroup","m_nmSFXFindActor","NameProperty",0x88,8)||
       !runtime_power_property(s->base,group,"InterpGroup","m_eSFXFindActorMode","ByteProperty",0xa4,1)||
       !read_at(group+0xa4,&mode,1)||mode||!read_at(group+0x88,name,8)||name[1]||
       !runtime_scene_interp_action(s->base,action,s->scope.world.pointer,&stamp,NULL,NULL)||
       !runtime_scene_interp_guest_channel(s->base,action,s->scope.root.pointer)||
       !scene_readiness_array(action+0x12c,&groups,128)||!scene_readiness_array(data+0x90,&data_groups,128))return original;
    name_of(s->base,name[0],text,sizeof text);int selector=runtime_scene_player_kind(1,mode,name[1],text);
    if(selector!=1&&(selector!=2||original))return original;
    for(int i=0;i<groups.count;++i)if(read_ptr(groups.data+(SIZE_T)i*8)==instance)++found_instance;
    for(int i=0;i<data_groups.count;++i)if(read_ptr(data_groups.data+(SIZE_T)i*8)==group)++found_group;
    if(found_instance!=1||found_group!=1||!read_at(action+0x12c,&after,sizeof after)||memcmp(&groups,&after,sizeof after)||
       !read_at(data+0x90,&after,sizeof after)||memcmp(&data_groups,&after,sizeof after))return original;
    uintptr_t actor=s->actor.pointer;
    if(selector==2){NetworkActorScope world={0};
        if(!network_actor_read_scope(s->base,s->scope.pc.pointer,&world))return original;
        actor=runtime_scene_owner_actor(s->base,action,s->scope.world.pointer,&world);
    }
    if(!actor||!runtime_scene_player_live())return original;
    if(actor!=original)runtime_scene_player_note(selector==1?"native-player-group-bound-to-host":"native-owner-group-resolved",actor);
    return actor;
}
typedef uintptr_t (*RuntimeSceneGroupSelect)(void*,uintptr_t,uintptr_t);
static uintptr_t runtime_scene_player_group_forward(void *self,uintptr_t caller,RuntimeSceneGroupResolve original,RuntimeSceneGroupSelect select) {
    uintptr_t actor=original(self);DWORD error=GetLastError();uintptr_t result=select(self,actor,caller);SetLastError(error);return result;
}
static uintptr_t runtime_scene_player_group_resolve(void *self) {
    return runtime_scene_player_group_forward(self,(uintptr_t)__builtin_return_address(0),
        (RuntimeSceneGroupResolve)runtime_scene_player_binding.group_resolver.original,runtime_scene_player_group_select);
}
static void runtime_scene_player_tick(uintptr_t base,uintptr_t engine) {
    RuntimeScenePlayerBinding *s=&runtime_scene_player_binding;s->base=base;s->engine=engine;
    presentation_player_confirmed=runtime_scene_player_confirmed;runtime_scene_interp_player_ready=runtime_scene_player_ready;
    presentation_player_identity_enabled=runtime_scene_interp_enabled;
    if(GetCurrentThreadId()!=session_game_thread||g_runtime_config.role!=ME3_ROLE_GUEST)return;
    if(s->valid&&!runtime_scene_player_live())s->valid=0;
    if(s->valid){NetworkActorScope world={0};
        if(network_actor_read_scope(base,s->scope.pc.pointer,&world))runtime_scene_owner_prepare(base,s->scope.world.pointer,&world);
    }
    if(!runtime_scene_interp_enabled()||!runtime_control_enabled||!runtime_scope_ready||s->attempted)return;
    static const unsigned char entry[32]={0x40,0x53,0x48,0x83,0xec,0x30,0x48,0x8b,0xd9,0x48,0x85,0xd2,0x74,0x0b,0x48,0x8b,0x4a,0x40,0xe8,0xa9,0xda,0x99,0xff,0xeb,0x17,0x48,0x8b,0x49,0x40,0xe8,0x5e,0xda};
    static const unsigned char group_entry[32]={0x40,0x53,0x48,0x83,0xec,0x30,0xf6,0x81,0x80,0x00,0x00,0x00,0x04,0x48,0x8b,0xd9,0x75,0x50,0x48,0x8b,0x41,0x78,0x48,0x85,0xc0,0x74,0x09,0xf6,0x80,0x5c,0x02,0x00};
    s->attempted=1;
    if(supported_image(base)&&runtime_scene_interp_hook_install(base,&s->resolver,0xbb4700,entry,(void*)runtime_scene_player_resolve)&&
       runtime_scene_interp_hook_install(base,&s->group_resolver,0x55e620,group_entry,(void*)runtime_scene_player_group_resolve))
        runtime_scene_player_note("native-player-resolver-prepared",0);
}
#ifdef ME3_RUNTIME_FIXTURE
static unsigned runtime_scene_player_fixture_calls;
static int runtime_scene_player_fixture_bad,runtime_scene_player_fixture_replace;
static uintptr_t runtime_scene_player_fixture_original(void *track,void *instance) {
    ++runtime_scene_player_fixture_calls;if(track!=(void*)16||instance!=(void*)24||GetLastError()!=37)runtime_scene_player_fixture_bad=1;
    SetLastError(53);return 32;
}
static uintptr_t runtime_scene_player_fixture_select(void *track,void *instance,uintptr_t original) {
    if(track!=(void*)16||instance!=(void*)24||original!=32||GetLastError()!=53)runtime_scene_player_fixture_bad=1;
    SetLastError(71);return runtime_scene_player_fixture_replace?48:original;
}
static uintptr_t runtime_scene_group_fixture_original(void *self){++runtime_scene_player_fixture_calls;if(self!=(void*)16||GetLastError()!=37)runtime_scene_player_fixture_bad=1;SetLastError(53);return 32;}
static uintptr_t runtime_scene_group_fixture_select(void *self,uintptr_t original,uintptr_t caller){if(self!=(void*)16||original!=32||caller!=99||GetLastError()!=53)runtime_scene_player_fixture_bad=1;SetLastError(71);return runtime_scene_player_fixture_replace?48:original;}
static int runtime_scene_player_selftest(void) {
    DWORD error=GetLastError();int ok=1;
    ok=ok&&runtime_scene_player_kind(1,0,0,"Player")==1&&runtime_scene_player_kind(1,0,0,"Owner")==2&&
        !runtime_scene_player_kind(0,0,0,"Player")&&!runtime_scene_player_kind(1,1,0,"Player")&&
        !runtime_scene_player_kind(1,2,0,"Player")&&!runtime_scene_player_kind(1,0,1,"Player")&&
        !runtime_scene_player_kind(1,0,0,"Player0")&&!runtime_scene_player_kind(1,0,0,"Listener")&&
        !runtime_scene_player_kind(1,0,0,NULL);
    for(int change=0;change<2;++change){runtime_scene_player_fixture_calls=0;runtime_scene_player_fixture_bad=0;runtime_scene_player_fixture_replace=change;
        SetLastError(37);uintptr_t result=runtime_scene_player_forward((void*)16,(void*)24,runtime_scene_player_fixture_original,runtime_scene_player_fixture_select);
        ok=ok&&result==(change?48u:32u)&&runtime_scene_player_fixture_calls==1&&!runtime_scene_player_fixture_bad&&GetLastError()==53;
        runtime_scene_player_fixture_calls=0;SetLastError(37);
        result=runtime_scene_player_group_forward((void*)16,99,runtime_scene_group_fixture_original,runtime_scene_group_fixture_select);
        ok=ok&&result==(change?48u:32u)&&runtime_scene_player_fixture_calls==1&&!runtime_scene_player_fixture_bad&&GetLastError()==53;
    }
    SetLastError(error);return ok&&runtime_scene_owner_binding_selftest();
}
#endif
#endif
