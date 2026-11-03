#ifndef ME3_SCENE_READINESS_DIAGNOSTICS_H
#define ME3_SCENE_READINESS_DIAGNOSTICS_H

/* Read-only diagnostics; include after package_identity.h and memory/object
 * helpers. Caller selects the diagnostic run. No readiness verdict, actor
 * writes, visibility acknowledgments, scene activation or input overrides.
 * Private LExSDKv2 LE3 provenance:
 * Engine_classes:5072 WorldInfo.StreamingLevels; 9725..9747 LevelStreaming;
 * 8258 PlayerController.PlayerCamera; 32209 Camera.ViewTarget.
 * Engine_f_structs:16322 GetPlayerViewPoint (Vector0, Rotator12, size24).
 * SFXGame_classes:12333 conversation manager; 18408..18479 controller;
 * 18508..18514 manager arrays. SFXGame_f_structs:46750 getter returnptr0,size8.
 * Camera ViewTarget.POV+0/Target+0x1c agrees with the existing validated
 * camera-target probe (+0x3f4). UWorld's manually documented native layouts
 * and unverified connection-visible-level offsets are intentionally unused.
 * Additional camera provenance: SFXGame_classes:29091 PC.MyCamera+e28;
 * 16966+ SFXCameraNativeBase.CurrentCameraMode+558/flags57c;
 * 29666+ SFXPlayerCamera.CameraDesiredPosition+594;16628+ mode fields.
 * Engine_classes:32217 Camera.PCOwner+4a8; Engine_f_structs:29487
 * Pawn.GetActorEyesViewPoint (out Vector0, out Rotator12, size24).
 */
static DWORD scene_readiness_last_sample = 0;

static const char *scene_readiness_bool(int value) { return value ? "true" : "false"; }
#include "squad_context_support.h"
static void scene_readiness_path(uintptr_t base, uintptr_t object, char *out, size_t cap) {
    char raw[600]={0}; out[0]=0;
    if(is_live(base,object)) identity_path(base,object,raw,sizeof raw);
    identity_quote(raw,out,cap);
}
static uintptr_t scene_readiness_owner_level(uintptr_t base, uintptr_t object) {
    uintptr_t seen[24]; int count=0;
    while(object && count<24) {
        if(!is_live(base,object)) return 0;
        for(int i=0;i<count;++i) if(seen[i]==object) return 0;
        seen[count++]=object;
        if(is_kind_of(base,object,"Level")) return object;
        if(!read_at(object+0x40,&object,sizeof object)) return 0;
    }
    return 0;
}
static void scene_readiness_level_row(uintptr_t base, uintptr_t actor, const char *role, DWORD now) {
    uintptr_t level=scene_readiness_owner_level(base,actor);
    char actor_path[1200],level_path[1200],row[2700];
    scene_readiness_path(base,actor,actor_path,sizeof actor_path);
    scene_readiness_path(base,level,level_path,sizeof level_path);
    snprintf(row,sizeof row,"{\"scene_readiness\":\"actor-level\",\"role\":\"%s\",\"actor_live\":%s,\"actor_path\":\"%s\",\"owning_level_valid\":%s,\"level_package_path\":\"%s\",\"monotonic_ms\":%lu}\r\n",
        role,scene_readiness_bool(scene_readiness_actor(base,actor)),actor_path,
        scene_readiness_bool(level!=0),level_path,now); line(row);
}
static int scene_readiness_getter_abi(uintptr_t base,const char *owner,const char *name,uint16_t size) {
    uintptr_t fn=function_named(base,owner,name); uint16_t parms=0;
    return is_live(base,fn) && is_kind_of(base,fn,"Function") &&
        read_at(fn+0xe6,&parms,2) && parms==size;
}
static int scene_readiness_finite3(const float *v) {
    return isfinite(v[0])&&isfinite(v[1])&&isfinite(v[2]);
}
static void scene_readiness_camera_details(uintptr_t base,uintptr_t pc,uintptr_t pawn,
                                           uintptr_t camera,const float *pov,int pov_valid,
                                           const float *pawn_xyz,int pawn_position_valid,DWORD now) {
    char row[5000],raw_class[96]={0},camera_class[200],owner_path[1200];
    uintptr_t mine=0,owner=0,mode=0;
    int camera_live=scene_readiness_actor(base,camera)&&is_kind_of(base,camera,"Camera");
    if(camera_live) object_name(base,read_ptr(camera+0x50),raw_class,sizeof raw_class);
    identity_quote(raw_class,camera_class,sizeof camera_class);
    int mine_read=is_kind_of(base,pc,"SFXPlayerController")&&read_at(pc+0xe28,&mine,8);
    int mine_valid=scene_readiness_actor(base,mine)&&is_kind_of(base,mine,"SFXPlayerCamera");
    int owner_read=camera_live&&read_at(camera+0x4a8,&owner,8);
    scene_readiness_path(base,owner,owner_path,sizeof owner_path);
    int native_camera=camera_live&&is_kind_of(base,camera,"SFXCameraNativeBase");
    int player_camera=native_camera&&is_kind_of(base,camera,"SFXPlayerCamera");
    uint32_t native_flags=0;int native_flags_valid=native_camera&&read_at(camera+0x57c,&native_flags,4);
    float desired[3]={0},desired_fov=0;int32_t desired_rot[3]={0};
    int desired_valid=player_camera&&read_at(camera+0x594,desired,12)&&read_at(camera+0x5a0,desired_rot,12)&&
        read_at(camera+0x5ac,&desired_fov,4)&&scene_readiness_finite3(desired)&&isfinite(desired_fov);
    snprintf(row,sizeof row,"{\"scene_readiness\":\"camera-pipeline\",\"camera_valid\":%s,\"camera_class\":\"%s\",\"my_camera_read_valid\":%s,\"my_camera_valid\":%s,\"my_camera_matches_player_camera\":%s,\"owner_read_valid\":%s,\"owner_path\":\"%s\",\"owner_is_local_pc\":%s,\"native_flags_valid\":%s,\"native_flags\":%u,\"disabled\":%s,\"switch_enabled\":%s,\"use_camera_rotation\":%s,\"desired_pov_valid\":%s,\"desired_pov\":[%.3f,%.3f,%.3f],\"desired_rotation\":[%d,%d,%d],\"desired_fov\":%.3f,\"desired_to_final_distance\":%.3f,\"desired_to_pawn_distance\":%.3f,\"monotonic_ms\":%lu}\r\n",
        scene_readiness_bool(camera_live),camera_class,scene_readiness_bool(mine_read),scene_readiness_bool(mine_valid),scene_readiness_bool(mine_read&&camera_live&&mine==camera),
        scene_readiness_bool(owner_read),owner_path,scene_readiness_bool(owner_read&&owner==pc),scene_readiness_bool(native_flags_valid),native_flags,
        scene_readiness_bool(native_flags_valid&&(native_flags&1)),scene_readiness_bool(native_flags_valid&&(native_flags&4)),scene_readiness_bool(native_flags_valid&&(native_flags&8)),
        scene_readiness_bool(desired_valid),desired_valid?desired[0]:0,desired_valid?desired[1]:0,desired_valid?desired[2]:0,desired_rot[0],desired_rot[1],desired_rot[2],desired_valid?desired_fov:0,
        desired_valid&&pov_valid?distance(desired,pov):-1,desired_valid&&pawn_position_valid?distance(desired,pawn_xyz):-1,now);line(row);

    int mode_read=native_camera&&read_at(camera+0x558,&mode,8);
    int mode_valid=mode_read&&is_live(base,mode)&&is_kind_of(base,mode,"SFXCameraMode");
    float offset[3]={0},hook[3]={0},collision=0;uint32_t flags=0;uintptr_t target=0;
    int fields_valid=mode_valid&&read_at(mode+0xb4,offset,12)&&read_at(mode+0xc0,hook,12)&&
        read_at(mode+0x114,&collision,4)&&read_at(mode+0x11c,&flags,4)&&read_at(mode+0x7c,&target,8)&&
        scene_readiness_finite3(offset)&&scene_readiness_finite3(hook)&&isfinite(collision);
    char mode_path[1200],mode_class[200],target_path[1200];raw_class[0]=0;
    if(mode_valid) object_name(base,read_ptr(mode+0x50),raw_class,sizeof raw_class);
    identity_quote(raw_class,mode_class,sizeof mode_class);scene_readiness_path(base,mode,mode_path,sizeof mode_path);scene_readiness_path(base,target,target_path,sizeof target_path);
    snprintf(row,sizeof row,"{\"scene_readiness\":\"camera-mode\",\"mode_read_valid\":%s,\"mode_present\":%s,\"mode_valid\":%s,\"mode_class\":\"%s\",\"mode_path\":\"%s\",\"fields_valid\":%s,\"offset\":[%.3f,%.3f,%.3f],\"hook_offset\":[%.3f,%.3f,%.3f],\"collision_distance\":%.3f,\"flags\":%u,\"collision_enabled\":%s,\"first_person\":%s,\"mode_target_path\":\"%s\",\"mode_target_is_local_pawn\":%s,\"monotonic_ms\":%lu}\r\n",
        scene_readiness_bool(mode_read),scene_readiness_bool(mode!=0),scene_readiness_bool(mode_valid),mode_class,mode_path,scene_readiness_bool(fields_valid),
        fields_valid?offset[0]:0,fields_valid?offset[1]:0,fields_valid?offset[2]:0,fields_valid?hook[0]:0,fields_valid?hook[1]:0,fields_valid?hook[2]:0,fields_valid?collision:0,flags,
        scene_readiness_bool(fields_valid&&(flags&2)),scene_readiness_bool(fields_valid&&(flags&0x20)),target_path,scene_readiness_bool(fields_valid&&target==pawn&&pawn),now);line(row);

    float pawn_hook[3]={0},pawn_arm[3]={0},hook_scale=0,arm_scale=0;
    int pawn_offsets_valid=scene_readiness_actor(base,pawn)&&is_kind_of(base,pawn,"BioPawn")&&read_at(pawn+0xad8,pawn_hook,12)&&read_at(pawn+0xae4,pawn_arm,12)&&
        read_at(pawn+0xe10,&hook_scale,4)&&read_at(pawn+0xe14,&arm_scale,4)&&scene_readiness_finite3(pawn_hook)&&scene_readiness_finite3(pawn_arm)&&isfinite(hook_scale)&&isfinite(arm_scale);
    snprintf(row,sizeof row,"{\"scene_readiness\":\"pawn-camera-offsets\",\"fields_valid\":%s,\"hook_offset\":[%.3f,%.3f,%.3f],\"arm_offset\":[%.3f,%.3f,%.3f],\"hook_scale\":%.3f,\"arm_scale\":%.3f,\"monotonic_ms\":%lu}\r\n",
        scene_readiness_bool(pawn_offsets_valid),pawn_offsets_valid?pawn_hook[0]:0,pawn_offsets_valid?pawn_hook[1]:0,pawn_offsets_valid?pawn_hook[2]:0,
        pawn_offsets_valid?pawn_arm[0]:0,pawn_offsets_valid?pawn_arm[1]:0,pawn_offsets_valid?pawn_arm[2]:0,pawn_offsets_valid?hook_scale:0,pawn_offsets_valid?arm_scale:0,now);line(row);
#pragma pack(push, 1)
    struct { float location[3]; int32_t rotation[3]; } eyes={{0},{0}};
#pragma pack(pop)
    _Static_assert(sizeof eyes==24,"Pawn.GetActorEyesViewPoint ABI");
    int eyes_read=scene_readiness_actor(base,pawn)&&is_kind_of(base,pawn,"Pawn")&&
        scene_readiness_getter_abi(base,"Pawn","GetActorEyesViewPoint",24)&&invoke(base,pawn,"Pawn","GetActorEyesViewPoint",&eyes);
    int eyes_valid=eyes_read&&scene_readiness_finite3(eyes.location);
    snprintf(row,sizeof row,"{\"scene_readiness\":\"pawn-eyes-viewpoint\",\"getter_dispatched\":%s,\"location_valid\":%s,\"location\":[%.3f,%.3f,%.3f],\"rotation\":[%d,%d,%d],\"eyes_to_final_pov_distance\":%.3f,\"eyes_to_pawn_distance\":%.3f,\"monotonic_ms\":%lu}\r\n",
        scene_readiness_bool(eyes_read),scene_readiness_bool(eyes_valid),eyes_valid?eyes.location[0]:0,eyes_valid?eyes.location[1]:0,eyes_valid?eyes.location[2]:0,eyes.rotation[0],eyes.rotation[1],eyes.rotation[2],
        eyes_valid&&pov_valid?distance(eyes.location,pov):-1,eyes_valid&&pawn_position_valid?distance(eyes.location,pawn_xyz):-1,now);line(row);
}
static void scene_readiness_sample(uintptr_t base,uintptr_t pc,uintptr_t pawn,uintptr_t world,DWORD now) {
    if(scene_readiness_last_sample && now-scene_readiness_last_sample<5000) return;
    scene_readiness_last_sample=now;
    if(!scene_readiness_actor(base,pc) || !is_kind_of(base,pc,"PlayerController") ||
       !scene_readiness_actor(base,world) || !is_kind_of(base,world,"WorldInfo") ||
       read_ptr(pc+0x198)!=world) {
        char row[180];snprintf(row,sizeof row,"{\"scene_readiness\":\"snapshot\",\"context_valid\":false,\"monotonic_ms\":%lu}\r\n",now);line(row);return;
    }
    GameArray streams={0}; int streams_valid=scene_readiness_array(world+0x3e4,&streams,1024);
    uintptr_t manager=read_ptr(world+0x948),camera=read_ptr(pc+0x684),actor_base=0;
    char world_path[1200],row[5000];scene_readiness_path(base,world,world_path,sizeof world_path);
    snprintf(row,sizeof row,"{\"scene_readiness\":\"snapshot\",\"context_valid\":true,\"world_path\":\"%s\",\"pawn_world_matches\":%s,\"streaming_array_valid\":%s,\"streaming_count\":%d,\"monotonic_ms\":%lu}\r\n",
        world_path,scene_readiness_bool(scene_readiness_actor(base,pawn)&&read_ptr(pawn+0x198)==world),scene_readiness_bool(streams_valid),streams_valid?streams.count:-1,now);line(row);
    if(streams_valid) for(int i=0;i<streams.count;++i) {
        uintptr_t entry=0,loaded=0;uint32_t name[2]={0},flags=0;
        int valid=read_at(streams.data+(SIZE_T)i*8,&entry,8) && is_live(base,entry) &&
            is_kind_of(base,entry,"LevelStreaming") && read_at(entry+0x98,name,8) &&
            read_at(entry+0xa0,&loaded,8) && read_at(entry+0xcc,&flags,4);
        char raw[200]={0},package[400],level_path[1200];
        if(valid) name_of(base,name[0],raw,sizeof raw);
        identity_quote(raw,package,sizeof package);scene_readiness_path(base,loaded,level_path,sizeof level_path);
        int loaded_valid=valid && is_live(base,loaded) && is_kind_of(base,loaded,"Level");
        snprintf(row,sizeof row,"{\"scene_readiness\":\"streaming-level\",\"index\":%d,\"entry_valid\":%s,\"package\":\"%s\",\"package_name_number\":%u,\"loaded_pointer_present\":%s,\"loaded_level_valid\":%s,\"loaded_level_path\":\"%s\",\"flags\":%u,\"visible\":%s,\"load_pending\":%s,\"unload_pending\":%s,\"should_load\":%s,\"should_be_visible\":%s,\"removal_requested\":%s,\"monotonic_ms\":%lu}\r\n",
            i,scene_readiness_bool(valid),package,name[1],scene_readiness_bool(loaded!=0),scene_readiness_bool(loaded_valid),level_path,flags,
            scene_readiness_bool(valid&&(flags&1)),scene_readiness_bool(valid&&(flags&4)),scene_readiness_bool(valid&&(flags&8)),
            scene_readiness_bool(valid&&(flags&0x200)),scene_readiness_bool(valid&&(flags&0x400)),scene_readiness_bool(valid&&(flags&0x2000)),now);line(row);
    }
    scene_readiness_level_row(base,pawn,"pawn",now);
    if(scene_readiness_actor(base,pawn)) read_at(pawn+0x188,&actor_base,8);
    scene_readiness_level_row(base,actor_base,"pawn-base",now);

    int manager_valid=is_live(base,manager)&&is_kind_of(base,manager,"BioConversationManager");
    GameArray active={0};uint32_t activating=0;uintptr_t full=0;
    int active_valid=manager_valid&&scene_readiness_array(manager+0xc8,&active,128);
    int activating_valid=manager_valid&&read_at(manager+0xe8,&activating,4);
    int full_read=manager_valid&&scene_readiness_getter_abi(base,"BioConversationManager","GetFullConversation",8)&&
        invoke(base,manager,"BioConversationManager","GetFullConversation",&full);
    int full_valid=full_read&&is_live(base,full)&&is_kind_of(base,full,"BioConversationController"),member=0,members_read=active_valid;
    if(active_valid) for(int i=0;i<active.count;++i) {uintptr_t item=0;if(!read_at(active.data+(SIZE_T)i*8,&item,8))members_read=0;else if(item==full&&full)member++;}
    uintptr_t data=0,player=0,speaker=0,listener=0,matinee=0;uint32_t flags=0;int32_t entry=-1;unsigned char type=255;
    int fields_valid=full_valid&&read_at(full+0x284,&data,8)&&read_at(full+0x29c,&player,8)&&
        read_at(full+0x2a4,&speaker,8)&&read_at(full+0x2b4,&listener,8)&&read_at(full+0x2dc,&matinee,8)&&
        read_at(full+0x2ec,&entry,4)&&read_at(full+0x358,&flags,4)&&read_at(full+0x35c,&type,1);
    char full_path[1200],data_path[1200];scene_readiness_path(base,full,full_path,sizeof full_path);scene_readiness_path(base,data,data_path,sizeof data_path);
    snprintf(row,sizeof row,"{\"scene_readiness\":\"conversation\",\"manager_valid\":%s,\"active_array_valid\":%s,\"active_count\":%d,\"activating_valid\":%s,\"activating\":%s,\"full_getter_dispatched\":%s,\"full_present\":%s,\"full_valid\":%s,\"active_membership_valid\":%s,\"full_active_memberships\":%d,\"controller_path\":\"%s\",\"data_path\":\"%s\",\"fields_valid\":%s,\"controller_type\":%u,\"entry\":%d,\"flags\":%u,\"ended\":%s,\"ambient\":%s,\"failed\":%s,\"interrupted\":%s,\"over\":%s,\"player_is_local_pawn\":%s,\"speaker_is_local_pawn\":%s,\"listener_is_local_pawn\":%s,\"monotonic_ms\":%lu}\r\n",
        scene_readiness_bool(manager_valid),scene_readiness_bool(active_valid),active_valid?active.count:-1,scene_readiness_bool(activating_valid),scene_readiness_bool(activating_valid&&(activating&1)),
        scene_readiness_bool(full_read),scene_readiness_bool(full!=0),scene_readiness_bool(full_valid),scene_readiness_bool(members_read),member,full_path,data_path,scene_readiness_bool(fields_valid),type,entry,flags,
        scene_readiness_bool(fields_valid&&(flags&2)),scene_readiness_bool(fields_valid&&(flags&0x80)),scene_readiness_bool(fields_valid&&(flags&0x1000)),scene_readiness_bool(fields_valid&&(flags&0x2000)),scene_readiness_bool(fields_valid&&(flags&0x8000)),
        scene_readiness_bool(player&&player==pawn),scene_readiness_bool(speaker&&speaker==pawn),scene_readiness_bool(listener&&listener==pawn),now);line(row);
    if(full_valid) {
        const uintptr_t actors[]={player,speaker,listener};const char *roles[]={"conversation-player","conversation-speaker","conversation-listener"};
        for(int i=0;i<3;++i) scene_readiness_level_row(base,actors[i],roles[i],now);
        uint32_t interp_flags=0;float position=0;int interp_valid=is_live(base,matinee)&&is_kind_of(base,matinee,"SeqAct_Interp")&&read_at(matinee+0x180,&interp_flags,4)&&read_at(matinee+0x170,&position,4)&&isfinite(position);
        char path[1200];scene_readiness_path(base,matinee,path,sizeof path);
        snprintf(row,sizeof row,"{\"scene_readiness\":\"conversation-line\",\"matinee_valid\":%s,\"matinee_path\":\"%s\",\"position\":%.3f,\"playing\":%s,\"paused\":%s,\"monotonic_ms\":%lu}\r\n",scene_readiness_bool(interp_valid),path,isfinite(position)?position:0,scene_readiness_bool(interp_valid&&(interp_flags&2)),scene_readiness_bool(interp_valid&&(interp_flags&4)),now);line(row);
    }

    float pov[3]={0},fov=0,pawn_xyz[3]={0};int32_t rotation[3]={0};uintptr_t target=0;
    int camera_valid=scene_readiness_actor(base,camera)&&is_kind_of(base,camera,"Camera");
    int pov_valid=camera_valid&&read_at(camera+0x3d8,pov,12)&&read_at(camera+0x3e4,rotation,12)&&read_at(camera+0x3f0,&fov,4)&&
        isfinite(pov[0])&&isfinite(pov[1])&&isfinite(pov[2])&&isfinite(fov);
    int target_read=camera_valid&&read_at(camera+0x3f4,&target,8);
    int pawn_position_valid=scene_readiness_actor(base,pawn)&&read_at(pawn+0x108,pawn_xyz,12)&&isfinite(pawn_xyz[0])&&isfinite(pawn_xyz[1])&&isfinite(pawn_xyz[2]);
    double camera_distance=pov_valid&&pawn_position_valid?distance(pov,pawn_xyz):-1;
    char target_path[1200];scene_readiness_path(base,target,target_path,sizeof target_path);
    snprintf(row,sizeof row,"{\"scene_readiness\":\"camera\",\"camera_valid\":%s,\"pov_valid\":%s,\"pov\":[%.3f,%.3f,%.3f],\"rotation\":[%d,%d,%d],\"fov\":%.3f,\"target_read_valid\":%s,\"target_path\":\"%s\",\"target_is_local_pawn\":%s,\"pawn_position_valid\":%s,\"camera_to_pawn_distance\":%.3f,\"monotonic_ms\":%lu}\r\n",
        scene_readiness_bool(camera_valid),scene_readiness_bool(pov_valid),pov_valid?pov[0]:0,pov_valid?pov[1]:0,pov_valid?pov[2]:0,rotation[0],rotation[1],rotation[2],pov_valid?fov:0,
        scene_readiness_bool(target_read),target_path,scene_readiness_bool(target_read&&target==pawn&&pawn),scene_readiness_bool(pawn_position_valid),camera_distance,now);line(row);
    scene_readiness_camera_details(base,pc,pawn,camera,pov,pov_valid,pawn_xyz,pawn_position_valid,now);
#pragma pack(push, 1)
    struct { float location[3]; int32_t rotation[3]; } viewpoint={{0},{0}};
#pragma pack(pop)
    _Static_assert(sizeof viewpoint==24,"GetPlayerViewPoint ABI");
    int view_read=scene_readiness_getter_abi(base,"PlayerController","GetPlayerViewPoint",24)&&invoke(base,pc,"PlayerController","GetPlayerViewPoint",&viewpoint);
    int view_valid=view_read&&isfinite(viewpoint.location[0])&&isfinite(viewpoint.location[1])&&isfinite(viewpoint.location[2]);
    snprintf(row,sizeof row,"{\"scene_readiness\":\"player-viewpoint\",\"getter_dispatched\":%s,\"location_valid\":%s,\"location\":[%.3f,%.3f,%.3f],\"rotation\":[%d,%d,%d],\"monotonic_ms\":%lu}\r\n",
        scene_readiness_bool(view_read),scene_readiness_bool(view_valid),view_valid?viewpoint.location[0]:0,view_valid?viewpoint.location[1]:0,view_valid?viewpoint.location[2]:0,viewpoint.rotation[0],viewpoint.rotation[1],viewpoint.rotation[2],now);line(row);
}
#endif
