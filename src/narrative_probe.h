#ifndef ME3_SQUAD_COOP_NARRATIVE_PROBE_H
#define ME3_SQUAD_COOP_NARRATIVE_PROBE_H

/* Read-only observations, called by PROGRESS_TEST after scene_readiness helpers.
 * No setters, seek, replay, story activation or dialogue-success verdict.
 * Local LE3 2.0.0.48602 SDK provenance:
 * Core_classes:844..863 UField.Next68 / UStruct.Children70;
 * SFXGame_classes:7610 PC.GameModeManager2c38;8201 CurrentModea8;
 * 12333 World.ConversationManager948;18408..18479 conversation controller;
 * 18508 manager.Activec8; SFXGame_f_structs:46750 GetFullConversation native,
 * return pointer0,size8,flags20400. Only this ordinary getter is dispatched.
 * Engine_classes:5543 MatineeActor270..288;23242 SeqAct_Interp12c..180;
 * 24968 InterpData.Groups90;15423 Group.Tracks68;15500 GroupInst;
 * 15572 Director.clientCuts a0/1;16060 TrackInstDirector.OldViewTarget60;
 * 8263 PC.ControllingDirTrackInst6ac.
 * Movement observations: Engine_classes:9599 Track.ActiveCondition78,
 * bDisableTrack8c/4;15799 Move.bDisableMovementd8/4,MoveFramedc,RotModedd;
 * 16165 MoveInst.ResetLocationb0/ResetRotationbc; Actor108/114/198/25c/269;
 * 32282 CameraActor.FOV364;3456 Actor.LatentActionsf0;3603 Physics268.
 * These are fields, NOT proven native branch rules.
 * Native channels use the separately pinned network_actor_identity.h layout.
 * Object pointers/indices below are process-local observations, NOT shared IDs.
 * A channel number is meaningful only in its validated connection lifetime.
 */

static DWORD narrative_probe_last = 0;
static int narrative_probe_sampled = 0;
static uintptr_t narrative_probe_last_world = 0;
static DWORD narrative_probe_started = 0;
static unsigned int narrative_probe_rows = 0, narrative_probe_dropped = 0;
static uintptr_t narrative_probe_registration_seen[16];
static unsigned int narrative_probe_registration_seen_count = 0;
static int narrative_probe_budget(void) {
    return narrative_probe_rows<160 && GetTickCount()-narrative_probe_started<100;
}
static void narrative_probe_emit(const char *row) {
    if(!narrative_probe_budget()) { ++narrative_probe_dropped;return; }
    ++narrative_probe_rows;line(row);
}

static void narrative_probe_float(char out[48], int valid, float value) {
    if (valid && isfinite(value)) snprintf(out,48,"%.6f",value);
    else snprintf(out,48,"null");
}
/* Writes a complete JSON value: absent, unreadable, and valid are distinct. */
static void narrative_probe_identity(uintptr_t base, uintptr_t object, int pointer_read,
                                     char *out, size_t capacity) {
    if (!pointer_read) { snprintf(out,capacity,"{\"status\":\"unknown\"}"); return; }
    if (!object) { snprintf(out,capacity,"null"); return; }
    char path[1200], raw[96]={0}, klass[200]; int32_t index=-1;
    int valid=is_live(base,object);
    scene_readiness_path(base,object,path,sizeof path);
    if(valid) { object_name(base,read_ptr(object+0x50),raw,sizeof raw); read_at(object+8,&index,4); }
    identity_quote(raw,klass,sizeof klass);
    snprintf(out,capacity,"{\"status\":\"%s\",\"pointer\":\"0x%llx\",\"object_index\":%d,\"class\":\"%s\",\"path\":\"%s\"}",
        valid?"live":"invalid",(unsigned long long)object,index,klass,path);
}

typedef struct NarrativeProbeGetter {
    uintptr_t base, klass, function; int32_t index; uint64_t name;
} NarrativeProbeGetter;
static NarrativeProbeGetter narrative_probe_getter = {0};

/* Bounded class-child lookup avoids function_named/invoke's GObjects scans.
 * Revalidate the exact native getter, owner, identity and parameter size on
 * every call. This is the same size check as scene_readiness_getter_abi,
 * with an additional exact flags check and explicit owner/name validation.
 */
static int narrative_probe_get_full(uintptr_t base, uintptr_t manager, uintptr_t *full) {
    *full=0;
    if(!narrative_probe_object(base,manager,"BioConversationManager")) return 0;
    uintptr_t klass=read_ptr(manager+0x50),fn=0;
    NarrativeProbeGetter *cache=&narrative_probe_getter;
    int32_t index=-1;uint64_t name=0;uint16_t size=0;uint32_t flags=0;
    if(cache->base==base && cache->klass==klass && is_live(base,cache->function) &&
       read_at(cache->function+8,&index,4) && index==cache->index &&
       read_at(cache->function+0x48,&name,8) && name==cache->name) fn=cache->function;
    if(!fn) {
        uintptr_t seen_classes[16];int classes=0;
        while(is_live(base,klass) && classes<16 && !fn) {
            for(int n=0;n<classes;++n) if(seen_classes[n]==klass) return 0;
            seen_classes[classes++]=klass;
            char owner[96];object_name(base,klass,owner,sizeof owner);
            if(!strcmp(owner,"BioConversationManager")) {
                uintptr_t child=read_ptr(klass+0x70),seen[256];int count=0;
                while(child && count<256) {
                    if(!is_live(base,child)) return 0;
                    for(int n=0;n<count;++n) if(seen[n]==child) return 0;
                    seen[count++]=child;
                    char text[96];object_name(base,child,text,sizeof text);
                    if(!strcmp(text,"GetFullConversation") && is_kind_of(base,child,"Function") &&
                       read_ptr(child+0x40)==klass) { fn=child;break; }
                    child=read_ptr(child+0x68);
                }
            }
            klass=read_ptr(klass+0x60);
        }
    }
    char fn_name[96]={0},owner_name[96]={0};
    if(!narrative_probe_object(base,fn,"Function")) return 0;
    object_name(base,fn,fn_name,sizeof fn_name);
    uintptr_t owner=read_ptr(fn+0x40);
    if(is_live(base,owner)) object_name(base,owner,owner_name,sizeof owner_name);
    if(strcmp(fn_name,"GetFullConversation") || strcmp(owner_name,"BioConversationManager") ||
       !read_at(fn+0xe6,&size,2) || size!=8 || !read_at(fn+0xe0,&flags,4) || flags!=0x20400 ||
       !read_at(fn+8,&index,4) || !read_at(fn+0x48,&name,8)) return 0;
    uintptr_t pe=read_ptr(read_ptr(manager)+0x230);
    if(pe<base || pe>=base+0x1300000) return 0;
    cache->base=base;cache->klass=read_ptr(manager+0x50);cache->function=fn;cache->index=index;cache->name=name;
    ((ProcessEvent)pe)((void*)manager,(void*)fn,full,NULL);
    return 1;
}

static void narrative_probe_matinee(uintptr_t base,uintptr_t actor,uintptr_t world,
                                     int pointer_read,const char *source,int channel,DWORD now) {
    uintptr_t action=0;float position=0,rate=0;uint32_t flags=0;unsigned char dummy=0;
    int valid=scene_readiness_actor(base,actor)&&is_kind_of(base,actor,"MatineeActor");
    int action_read=valid&&read_at(actor+0x270,&action,8);
    int fields=valid&&read_at(actor+0x278,&rate,4)&&read_at(actor+0x27c,&position,4)&&
        read_at(actor+0x284,&flags,4)&&read_at(actor+0x288,&dummy,1)&&isfinite(rate)&&isfinite(position);
    char object[1600],action_id[1600],p[48],r[48],row[4000];
    narrative_probe_identity(base,actor,pointer_read,object,sizeof object);
    narrative_probe_identity(base,action,action_read,action_id,sizeof action_id);
    narrative_probe_float(p,fields,position);narrative_probe_float(r,fields,rate);
    snprintf(row,sizeof row,"{\"narrative_probe\":\"replicated-matinee\",\"source\":\"%s\",\"channel_index\":%d,\"actor\":%s,\"matinee_class_valid\":%s,\"world_matches\":%s,\"interp_action\":%s,\"position\":%s,\"play_rate\":%s,\"fields_valid\":%s,\"flags_raw\":%u,\"playing\":%s,\"paused\":%s,\"reverse\":%s,\"replication_trigger\":%u,\"monotonic_ms\":%lu}\r\n",
        source,channel,object,narrative_probe_bool(1,valid),narrative_probe_bool(valid,read_ptr(actor+0x198)==world),action_id,p,r,
        narrative_probe_bool(1,fields),flags,narrative_probe_bool(fields,flags&1),narrative_probe_bool(fields,flags&4),narrative_probe_bool(fields,flags&2),dummy,now);narrative_probe_emit(row);
}

static void narrative_probe_action_state(uintptr_t base,uintptr_t action,int pointer_read,
                                         const char *source,DWORD now) {
    int valid=pointer_read&&narrative_probe_object(base,action,"SeqAct_Interp");
    uint32_t flags=0;float position=0,rate=0;uintptr_t replicated=0;
    int fields=valid&&read_at(action+0x180,&flags,4)&&read_at(action+0x170,&position,4)&&
        read_at(action+0x16c,&rate,4)&&isfinite(position)&&isfinite(rate);
    int rep_read=valid&&read_at(action+0x15c,&replicated,8);
    char object[1600],rep_id[1600],p[48],r[48],row[4200];
    narrative_probe_identity(base,action,pointer_read,object,sizeof object);
    narrative_probe_identity(base,replicated,rep_read,rep_id,sizeof rep_id);
    narrative_probe_float(p,fields,position);narrative_probe_float(r,fields,rate);
    snprintf(row,sizeof row,"{\"narrative_probe\":\"line-action\",\"source\":\"%s\",\"action\":%s,\"action_class_valid\":%s,\"fields_valid\":%s,\"flags_raw\":%u,\"position\":%s,\"play_rate\":%s,\"playing\":%s,\"paused\":%s,\"client_side_only\":%s,\"skip_update_if_not_visible\":%s,\"replicated_actor\":%s,\"monotonic_ms\":%lu}\r\n",
        source,object,narrative_probe_bool(1,valid),narrative_probe_bool(1,fields),flags,p,r,
        narrative_probe_bool(fields,flags&2),narrative_probe_bool(fields,flags&4),narrative_probe_bool(fields,flags&0x2000),narrative_probe_bool(fields,flags&0x4000),rep_id,now);narrative_probe_emit(row);
}

static void narrative_probe_camera_move(uintptr_t base,uintptr_t pc,uintptr_t action,uintptr_t instance,
                                        uintptr_t group,uintptr_t actor,int group_index,DWORD now) {
    if(!narrative_probe_budget()||!scene_readiness_actor(base,actor)||
       !is_kind_of(base,actor,"CameraActor")||!narrative_probe_object(base,group,"InterpGroup"))return;
    GameArray tracks={0},track_instances={0};
    if(!scene_readiness_array(group+0x68,&tracks,64))return;
    int instances_valid=scene_readiness_array(instance+0x60,&track_instances,64);
    uintptr_t world=read_ptr(pc+0x198),actor_world=0;unsigned char role=255,remote=255,net=255;
    uint32_t actor_flags=0;float position[3]={0},fov=0;int32_t rotation[3]={0};
    int transform=read_at(actor+0x108,position,12)&&read_at(actor+0x114,rotation,12)&&scene_readiness_finite3(position);
    int scope=read_at(actor+0x198,&actor_world,8)&&read_at(actor+0x269,&remote,1)&&
        read_at(actor+0x26a,&role,1)&&read_at(actor+0x25c,&actor_flags,4)&&read_at(world+0x7c8,&net,1);
    int fov_read=read_at(actor+0x364,&fov,4)&&isfinite(fov);
    char actor_id[1600],actor_world_id[1600],row[8192];
    narrative_probe_identity(base,actor,1,actor_id,sizeof actor_id);
    narrative_probe_identity(base,actor_world,scope,actor_world_id,sizeof actor_world_id);
    /* Observe the current ordinary action membership; never activate a track,
     * alter Physics, or replay a sequence to make the camera move. */
    GameArray latent={0};uintptr_t latent_actions[64];unsigned char physics=255;
    int physics_valid=read_at(actor+0x268,&physics,1);
    int latent_valid=scene_readiness_array(actor+0xf0,&latent,64)&&
        (!latent.count||read_at(latent.data,latent_actions,(SIZE_T)latent.count*8));
    int contains_action=0;
    if(latent_valid)for(int i=0;i<latent.count;++i)contains_action|=latent_actions[i]==action;
    char action_id[1600];narrative_probe_identity(base,action,1,action_id,sizeof action_id);
    snprintf(row,sizeof row,"{\"narrative_probe\":\"camera-interpolation-membership\",\"group_instance_index\":%d,\"bound_camera\":%s,\"action\":%s,\"physics_read_valid\":%s,\"physics\":%u,\"latent_actions_valid\":%s,\"latent_action_count\":%d,\"contains_current_action\":%s,\"monotonic_ms\":%lu}\r\n",
        group_index,actor_id,action_id,narrative_probe_bool(1,physics_valid),physics,
        narrative_probe_bool(1,latent_valid),latent_valid?latent.count:-1,narrative_probe_bool(latent_valid,contains_action),now);
    narrative_probe_emit(row);
    for(int t=0;t<tracks.count&&narrative_probe_budget();++t) {
        uintptr_t track=read_ptr(tracks.data+(SIZE_T)t*8),move_instance=0;
        if(!narrative_probe_object(base,track,"InterpTrackMove"))continue;
        uint32_t common=0,move=0;unsigned char condition=255,frame=255,rot_mode=255;
        int flags_read=read_at(track+0x8c,&common,4)&&read_at(track+0xd8,&move,4)&&
            read_at(track+0x78,&condition,1)&&read_at(track+0xdc,&frame,1)&&read_at(track+0xdd,&rot_mode,1);
        int instance_read=instances_valid&&t<track_instances.count&&read_at(track_instances.data+(SIZE_T)t*8,&move_instance,8);
        int instance_valid=instance_read&&narrative_probe_object(base,move_instance,"InterpTrackInstMove");
        float reset[3]={0};int32_t reset_rotation[3]={0};
        int reset_read=instance_valid&&read_at(move_instance+0xb0,reset,12)&&read_at(move_instance+0xbc,reset_rotation,12)&&scene_readiness_finite3(reset);
        char track_id[1600],px[48],py[48],pz[48],fv[48];
        narrative_probe_identity(base,track,1,track_id,sizeof track_id);
        narrative_probe_float(px,transform,position[0]);narrative_probe_float(py,transform,position[1]);narrative_probe_float(pz,transform,position[2]);narrative_probe_float(fv,fov_read,fov);
        snprintf(row,sizeof row,"{\"narrative_probe\":\"camera-movement-track\",\"group_instance_index\":%d,\"track_index\":%d,\"track\":%s,\"bound_camera\":%s,\"actor_world\":%s,\"scope_fields_valid\":%s,\"world_matches_pc\":%s,\"net_mode\":%u,\"actor_role\":%u,\"actor_remote_role\":%u,\"actor_flags_raw\":%u,\"actor_static\":%s,\"actor_no_delete\":%s,\"replicate_movement\":%s,\"skip_actor_property_replication\":%s,\"transform_valid\":%s,\"position\":[%s,%s,%s],\"rotation\":[%d,%d,%d],\"fov\":%s,\"track_fields_valid\":%s,\"track_flags_raw\":%u,\"movement_flags_raw\":%u,\"track_disabled\":%s,\"movement_disabled\":%s,\"active_condition_raw\":%u,\"move_frame_raw\":%u,\"rotation_mode_raw\":%u,\"matching_index_move_instance_valid\":%s,\"reset_transform_valid\":%s,\"reset_position\":[%.6f,%.6f,%.6f],\"reset_rotation\":[%d,%d,%d],\"monotonic_ms\":%lu}\r\n",
            group_index,t,track_id,actor_id,actor_world_id,narrative_probe_bool(1,scope),narrative_probe_bool(scope,actor_world==world),net,role,remote,actor_flags,
            narrative_probe_bool(scope,actor_flags&1),narrative_probe_bool(scope,actor_flags&4),narrative_probe_bool(scope,actor_flags&0x800000),narrative_probe_bool(scope,actor_flags&0x1000000),
            narrative_probe_bool(1,transform),px,py,pz,rotation[0],rotation[1],rotation[2],fv,narrative_probe_bool(1,flags_read),common,move,narrative_probe_bool(flags_read,common&4),narrative_probe_bool(flags_read,move&4),condition,frame,rot_mode,
            narrative_probe_bool(1,instance_valid),narrative_probe_bool(1,reset_read),reset_read?reset[0]:0,reset_read?reset[1]:0,reset_read?reset[2]:0,reset_rotation[0],reset_rotation[1],reset_rotation[2],now);
        narrative_probe_emit(row);
    }
}

/* Native provenance for the otherwise missing FSeqVarLink SDK structure:
 * 72ac10: imul link_index,4c; add [SequenceOp+a8].
 * 72aff7/72b010: linked variables count+8/data+0; pointers at index*8.
 * 72adb2/72af71: test link byte+48,mask4 (meaning deliberately not inferred).
 * 72adbe loads link+20;72add0..72addc tests that UClass against
 * SequenceVariable::StaticClass and traverses its SuperField+60.
 * 72af79..72afbe compares the same expected-type constraint with the
 * SeqVar_Object class hierarchy. StaticClass1b3480 uses registration731a50,
 * whose UTF16 class name at1265eba is SequenceVariable.
 * 72b024: variable->vtable[3b0](index), yielding an object-reference address.
 * SeqVar_Object registration/ctor730e5d/73ea10 installs vtable1263d60;
 * its slot3b0 is72a750: index0 returns &ObjValue+ac, other indices NULL.
 * Engine_classes:22596 VariableLinks;23105 LatentActors;25136 ObjValue.
 * Do not call getters or assume subclasses use this same reference source.
 */
static int narrative_probe_variable_layout(uintptr_t base) {
    static const unsigned char outer[]={0x4c,0x6b,0xe1,0x4c,0x4c,0x03,0xa7,0xa8,0,0,0};
    static const unsigned char inner[]={0x49,0x8b,0x04,0x24,0x49,0x8b,0x0c,0x06,0x48,0x85,0xc9,0x74,0x70,0x33,0xf6,0x48,0x8b,0x01,0x33,0xd2,0xff,0x90,0xb0,0x03};
    static const unsigned char getter[]={0x85,0xd2,0x74,0x03,0x33,0xc0,0xc3,0x48,0x8d,0x81,0xac,0,0,0,0xc3};
    unsigned char observed[24];
    return read_at(base+0x72ac10,observed,sizeof outer)&&!memcmp(observed,outer,sizeof outer)&&
        read_at(base+0x72b010,observed,sizeof inner)&&!memcmp(observed,inner,sizeof inner)&&
        read_at(base+0x72a750,observed,sizeof getter)&&!memcmp(observed,getter,sizeof getter);
}

static void narrative_probe_registration(uintptr_t base,uintptr_t pc,uintptr_t action,
                                         const GameArray *instances,int instances_valid,DWORD now) {
    if(!instances_valid||!narrative_probe_budget()||!narrative_probe_object(base,action,"SeqAct_Interp"))return;
    for(unsigned int i=0;i<narrative_probe_registration_seen_count;++i)
        if(narrative_probe_registration_seen[i]==action)return;
    if(narrative_probe_registration_seen_count>=16)return;
    narrative_probe_registration_seen[narrative_probe_registration_seen_count++]=action;
    uintptr_t cameras[8],camera_instances[8];unsigned int camera_count=0;
    int cameras_complete=1,cameras_scanned=0;
    unsigned int dropped_before=narrative_probe_dropped;
    for(;cameras_scanned<instances->count&&narrative_probe_budget();++cameras_scanned) {
        int i=cameras_scanned;
        uintptr_t instance=read_ptr(instances->data+(SIZE_T)i*8),actor=0;
        if(!narrative_probe_object(base,instance,"InterpGroupInst")||!read_at(instance+0x78,&actor,8)) {
            cameras_complete=0;continue;
        }
        if(!scene_readiness_actor(base,actor)||!is_kind_of(base,actor,"CameraActor"))continue;
        unsigned int c=0;for(;c<camera_count;++c)if(cameras[c]==actor)break;
        if(c<camera_count)continue;
        if(camera_count>=8){cameras_complete=0;break;}
        cameras[camera_count]=actor;camera_instances[camera_count++]=instance;
        if(camera_count==8){cameras_complete=0;break;}
    }
    if(cameras_scanned<instances->count)cameras_complete=0;
    if(!camera_count)return;
    uintptr_t actors[64];GameArray latent={0},links={0};
    int latent_valid=scene_readiness_array(action+0xf8,&latent,64)&&
        (!latent.count||read_at(latent.data,actors,(SIZE_T)latent.count*8));
    int pins=narrative_probe_variable_layout(base);
    int links_valid=pins&&scene_readiness_array(action+0xa8,&links,32);
    unsigned int known_matches[8]={0},base_matches[8]={0};
    unsigned int variables_seen=0,unresolved_variables=0,links_seen=0;
    int linked_arrays_valid=links_valid,complete=links_valid&&cameras_complete;
    char action_id[1600],row[6500];narrative_probe_identity(base,action,1,action_id,sizeof action_id);
    if(links_valid)for(int l=0;l<links.count;++l) {
        if(!narrative_probe_budget()||variables_seen>=32) {complete=0;break;}
        uintptr_t at=links.data+(SIZE_T)l*0x4c,expected_type=0;GameArray variables={0};uintptr_t values[64];uint32_t flags=0;
        int fields=read_at(at+0x20,&expected_type,8)&&read_at(at+0x48,&flags,4);
        if(!fields)complete=0;
        int linked_valid=scene_readiness_array(at,&variables,64)&&
            (!variables.count||read_at(variables.data,values,(SIZE_T)variables.count*8));
        ++links_seen;if(!linked_valid){linked_arrays_valid=0;complete=0;}
        char expected_id[1600];narrative_probe_identity(base,expected_type,fields,expected_id,sizeof expected_id);
        snprintf(row,sizeof row,"{\"narrative_probe\":\"registration-variable-link\",\"action\":%s,\"link_index\":%d,\"native_stride\":76,\"link_fields_valid\":%s,\"link_flags_raw\":%u,\"link_flag_mask4_set\":%s,\"expected_type\":%s,\"linked_variables_valid\":%s,\"linked_variable_count\":%d,\"monotonic_ms\":%lu}\r\n",
            action_id,l,narrative_probe_bool(1,fields),flags,narrative_probe_bool(fields,flags&4),expected_id,narrative_probe_bool(1,linked_valid),linked_valid?variables.count:-1,now);narrative_probe_emit(row);
        if(!linked_valid)continue;
        for(int v=0;v<variables.count;++v) {
            if(!narrative_probe_budget()||variables_seen>=32){complete=0;break;}
            ++variables_seen;uintptr_t variable=values[v],value=0,method=0;
            int object_variable=narrative_probe_object(base,variable,"SeqVar_Object");
            int value_read=object_variable&&read_at(variable+0xac,&value,8);
            int method_read=object_variable&&read_at(read_ptr(variable)+0x3b0,&method,8);
            int native_ref_known=value_read&&method_read&&method==base+0x72a750;
            if(!native_ref_known)++unresolved_variables;
            unsigned int matching_cameras=0;
            for(unsigned int c=0;c<camera_count;++c)if(value_read&&value==cameras[c]) {
                ++base_matches[c];++matching_cameras;if(native_ref_known)++known_matches[c];
            }
            char variable_id[1600],value_id[1600];
            narrative_probe_identity(base,variable,1,variable_id,sizeof variable_id);
            narrative_probe_identity(base,value,value_read,value_id,sizeof value_id);
            snprintf(row,sizeof row,"{\"narrative_probe\":\"registration-object-variable\",\"action\":%s,\"link_index\":%d,\"variable_index\":%d,\"variable\":%s,\"seqvar_object_class_valid\":%s,\"base_obj_value\":%s,\"obj_value_read_valid\":%s,\"reference_getter_read_valid\":%s,\"reference_getter_rva\":\"0x%llx\",\"native_reference_value_known\":%s,\"base_value_matches_camera_count\":%u,\"native_link_filter_eligibility\":null,\"monotonic_ms\":%lu}\r\n",
                action_id,l,v,variable_id,narrative_probe_bool(1,object_variable),value_id,narrative_probe_bool(1,value_read),narrative_probe_bool(1,method_read),
                (unsigned long long)(method_read&&method>=base?method-base:0),narrative_probe_bool(1,native_ref_known),matching_cameras,now);narrative_probe_emit(row);
        }
    }
    for(unsigned int c=0;c<camera_count&&narrative_probe_budget();++c) {
        int memberships=0;
        if(latent_valid)for(int a=0;a<latent.count;++a)if(actors[a]==cameras[c])++memberships;
        char camera_id[1600],instance_id[1600];narrative_probe_identity(base,cameras[c],1,camera_id,sizeof camera_id);
        narrative_probe_identity(base,camera_instances[c],1,instance_id,sizeof instance_id);
        snprintf(row,sizeof row,"{\"narrative_probe\":\"camera-registration-candidates\",\"action\":%s,\"bound_camera\":%s,\"group_instance\":%s,\"camera_world_matches_pc\":%s,\"action_latent_actors_valid\":%s,\"action_latent_actor_count\":%d,\"camera_action_latent_memberships\":%d,\"known_reference_camera_matches\":%u,\"base_obj_value_camera_matches\":%u,\"variable_enumeration_complete\":%s,\"unresolved_variable_count\":%u,\"native_link_filter_eligibility\":null,\"monotonic_ms\":%lu}\r\n",
            action_id,camera_id,instance_id,narrative_probe_bool(1,read_ptr(cameras[c]+0x198)==read_ptr(pc+0x198)),narrative_probe_bool(1,latent_valid),latent_valid?latent.count:-1,latent_valid?memberships:-1,
            known_matches[c],base_matches[c],narrative_probe_bool(1,complete&&narrative_probe_budget()&&narrative_probe_dropped==dropped_before),unresolved_variables,now);narrative_probe_emit(row);
    }
    snprintf(row,sizeof row,"{\"narrative_probe\":\"registration-candidate-summary\",\"action\":%s,\"native_layout_pins_match\":%s,\"action_latent_actors_valid\":%s,\"action_latent_actor_count\":%d,\"variable_links_valid\":%s,\"variable_link_count\":%d,\"links_observed\":%u,\"linked_arrays_valid\":%s,\"variables_observed\":%u,\"unresolved_variables\":%u,\"bound_camera_count\":%u,\"camera_set_complete\":%s,\"enumeration_complete\":%s,\"monotonic_ms\":%lu}\r\n",
        action_id,narrative_probe_bool(1,pins),narrative_probe_bool(1,latent_valid),latent_valid?latent.count:-1,narrative_probe_bool(1,links_valid),links_valid?links.count:-1,links_seen,
        narrative_probe_bool(1,linked_arrays_valid),variables_seen,unresolved_variables,camera_count,narrative_probe_bool(1,cameras_complete),
        narrative_probe_bool(1,complete&&narrative_probe_budget()&&narrative_probe_dropped==dropped_before),now);narrative_probe_emit(row);
}

static void narrative_probe_directors(uintptr_t base,uintptr_t action,uintptr_t pc,DWORD now) {
    if(!narrative_probe_budget())return;
    uintptr_t data=0,pc_director=0;GameArray groups={0},instances={0};
    int valid=narrative_probe_object(base,action,"SeqAct_Interp");
    int data_read=valid&&read_at(action+0x154,&data,8);
    int data_valid=data_read&&narrative_probe_object(base,data,"InterpData");
    int groups_valid=data_valid&&scene_readiness_array(data+0x90,&groups,32);
    int instances_valid=valid&&scene_readiness_array(action+0x12c,&instances,32);
    int pc_read=scene_readiness_actor(base,pc)&&is_kind_of(base,pc,"PlayerController")&&read_at(pc+0x6ac,&pc_director,8);
    char data_id[1600],director_id[1600],row[4200];
    narrative_probe_identity(base,data,data_read,data_id,sizeof data_id);
    narrative_probe_identity(base,pc_director,pc_read,director_id,sizeof director_id);
    snprintf(row,sizeof row,"{\"narrative_probe\":\"director-summary\",\"interp_data\":%s,\"group_array_valid\":%s,\"group_count\":%d,\"instance_array_valid\":%s,\"instance_count\":%d,\"pc_director_instance\":%s,\"monotonic_ms\":%lu}\r\n",
        data_id,narrative_probe_bool(1,groups_valid),groups_valid?groups.count:-1,narrative_probe_bool(1,instances_valid),instances_valid?instances.count:-1,director_id,now);narrative_probe_emit(row);
    narrative_probe_registration(base,pc,action,&instances,instances_valid,now);
    int tracks_seen=0;
    if(groups_valid) for(int g=0;g<groups.count&&narrative_probe_budget();++g) {
        uintptr_t group=read_ptr(groups.data+(SIZE_T)g*8);GameArray tracks={0};
        if(!narrative_probe_object(base,group,"InterpGroup")||!scene_readiness_array(group+0x68,&tracks,64))continue;
        for(int t=0;t<tracks.count&&tracks_seen<512&&narrative_probe_budget();++t,++tracks_seen) {
            uintptr_t track=read_ptr(tracks.data+(SIZE_T)t*8);uint32_t flags=0;
            if(!narrative_probe_object(base,track,"InterpTrackDirector"))continue;
            int flags_read=read_at(track+0xa0,&flags,4);char track_id[1600];
            narrative_probe_identity(base,track,1,track_id,sizeof track_id);
            snprintf(row,sizeof row,"{\"narrative_probe\":\"director-track\",\"group_index\":%d,\"track_index\":%d,\"track\":%s,\"flags_read_valid\":%s,\"flags_raw\":%u,\"simulate_camera_cuts_on_clients\":%s,\"monotonic_ms\":%lu}\r\n",
                g,t,track_id,narrative_probe_bool(1,flags_read),flags,narrative_probe_bool(flags_read,flags&1),now);narrative_probe_emit(row);
        }
    }
    if(instances_valid) for(int i=0;i<instances.count&&narrative_probe_budget();++i) {
        uintptr_t instance=read_ptr(instances.data+(SIZE_T)i*8),group=0,actor=0;GameArray tracks={0};
        int instance_valid=narrative_probe_object(base,instance,"InterpGroupInst");
        int fields=instance_valid&&read_at(instance+0x70,&group,8)&&read_at(instance+0x78,&actor,8);
        char group_id[1600],actor_id[1600];
        narrative_probe_identity(base,group,fields,group_id,sizeof group_id);narrative_probe_identity(base,actor,fields,actor_id,sizeof actor_id);
        snprintf(row,sizeof row,"{\"narrative_probe\":\"group-binding\",\"instance_index\":%d,\"instance_valid\":%s,\"director_group\":%s,\"group\":%s,\"group_actor\":%s,\"group_actor_is_local_pc\":%s,\"monotonic_ms\":%lu}\r\n",
            i,narrative_probe_bool(1,instance_valid),narrative_probe_bool(instance_valid,is_kind_of(base,instance,"InterpGroupInstDirector")),group_id,actor_id,narrative_probe_bool(fields,actor==pc),now);narrative_probe_emit(row);
        if(fields)narrative_probe_camera_move(base,pc,action,instance,group,actor,i,now);
        if(!instance_valid||!scene_readiness_array(instance+0x60,&tracks,64))continue;
        for(int t=0;t<tracks.count&&narrative_probe_budget();++t) {
            uintptr_t track=read_ptr(tracks.data+(SIZE_T)t*8),old=0;
            if(!narrative_probe_object(base,track,"InterpTrackInstDirector"))continue;
            int old_read=read_at(track+0x60,&old,8);char old_id[1600];narrative_probe_identity(base,old,old_read,old_id,sizeof old_id);
            snprintf(row,sizeof row,"{\"narrative_probe\":\"director-binding\",\"instance_index\":%d,\"track_index\":%d,\"is_pc_controlling_director\":%s,\"old_view_target\":%s,\"monotonic_ms\":%lu}\r\n",
                i,t,narrative_probe_bool(pc_read,track==pc_director),old_id,now);narrative_probe_emit(row);
        }
    }
}

/* Observe actual network Matinee actors even when the guest has no local
 * BioConversationController. Reuse verified scopes only; never discover via
 * a full object scan. The diagnostic scope can exist before pawn possession.
 */
static void narrative_probe_network_matinees(uintptr_t base,uintptr_t pc,uintptr_t world,DWORD now) {
#ifdef ME3_SQUAD_COOP_NETWORK_ACTOR_IDENTITY_H
    uintptr_t driver=0,early_connection=0,connections[64];int count=0,ambiguous=0;
    const char *discovery_mode="pawn-identity-cache";
#ifdef ME3_SQUAD_COOP_ACTOR_CHANNEL_DIAGNOSTICS_H
    if(test_action==5&&actor_channel_diagnostic_base==base&&
       actor_channel_diagnostic_world.object==world&&actor_channel_diagnostic_scope_matches(base)&&
       network_actor_layout_matches(base)&&read_ptr(base+0x11a2150+0x2b0)==base+0x4840a0&&
       network_actor_received_handler_matches(base)) {
        driver=actor_channel_diagnostic_driver.object;
        early_connection=actor_channel_diagnostic_connection.object;
        discovery_mode="verified-early-diagnostic-cache";
    }
#endif
    for(unsigned int i=0;i<sizeof network_actor_identity_cache/sizeof network_actor_identity_cache[0];++i) {
        NetworkActorIdentityEntry *entry=&network_actor_identity_cache[i];
        NetworkActorIdentityEntry verified={0};
        if(entry->base!=base||!scene_readiness_actor(base,entry->pawn)||read_ptr(entry->pawn+0x198)!=world||
           !network_actor_channel_valid(base,entry->pawn,entry->channel,entry->driver,&verified)||
           verified.connection!=entry->connection||verified.index!=entry->index)continue;
        if(driver&&driver!=entry->driver)ambiguous=1;
        driver=entry->driver;
    }
    if(!driver&&test_action==6&&is_live(base,identity_host_driver))driver=identity_host_driver;
    if(ambiguous||!driver||!network_actor_layout_matches(base)||
       read_ptr(base+0x11a2150+0x2b0)!=base+0x4840a0||!network_actor_received_handler_matches(base)) {
        char row[260];snprintf(row,sizeof row,"{\"narrative_probe\":\"peer-matinee-inventory\",\"scope_valid\":false,\"reason\":\"%s\",\"actor_count\":null,\"monotonic_ms\":%lu}\r\n",ambiguous?"ambiguous-cached-scope":"scope-or-layout-unavailable",now);narrative_probe_emit(row);return;
    }
    if(test_action==5) { connections[0]=early_connection?early_connection:read_ptr(driver+0x80);count=1; }
    else if(test_action==6) {
        GameArray clients={0};if(!scene_readiness_array(driver+0x70,&clients,64))return;
        count=clients.count;if(count&&!read_at(clients.data,connections,(SIZE_T)count*8))return;
    }
    int rows=0,scopes_valid=1;
    for(int c=0;c<count&&narrative_probe_budget();++c) {
        GameArray open={0};uintptr_t channels[2048];
        if(!network_actor_connection_in_scope(base,driver,connections[c])||
           !scene_readiness_array(connections[c]+0x5f90,&open,2048)||
           (open.count&&!read_at(open.data,channels,(SIZE_T)open.count*8))) { scopes_valid=0;continue; }
        for(int n=0;n<open.count&&rows<32&&narrative_probe_budget();++n) {
            uintptr_t channel=channels[n],actor=read_ptr(channel+0xa0);NetworkActorIdentityEntry verified={0};
            if(!scene_readiness_actor(base,actor)||!is_kind_of(base,actor,"MatineeActor")||
               !network_actor_channel_valid(base,actor,channel,driver,&verified)||verified.connection!=connections[c])continue;
            ++rows;narrative_probe_matinee(base,actor,world,1,"current-peer-channel",verified.index,now);
            uintptr_t action=0;int action_read=read_at(actor+0x270,&action,8);
            narrative_probe_action_state(base,action,action_read,"current-peer-channel",now);
            if(action_read&&narrative_probe_object(base,action,"SeqAct_Interp"))narrative_probe_directors(base,action,pc,now);
        }
    }
    char row[400];snprintf(row,sizeof row,"{\"narrative_probe\":\"peer-matinee-inventory\",\"scope_valid\":%s,\"discovery_mode\":\"%s\",\"connection_count\":%d,\"actor_count\":%d,\"output_cap_reached\":%s,\"monotonic_ms\":%lu}\r\n",
        narrative_probe_bool(1,scopes_valid),discovery_mode,count,rows,narrative_probe_bool(1,rows>=32||!narrative_probe_budget()),now);narrative_probe_emit(row);
#else
    (void)base;(void)pc;(void)world;(void)now;
#endif
}

static void narrative_probe_sample(uintptr_t base,uintptr_t pc,uintptr_t world,DWORD now) {
    if(world!=narrative_probe_last_world) { narrative_probe_last_world=world;narrative_probe_sampled=0; }
    if(narrative_probe_sampled&&now-narrative_probe_last<1000)return;
    narrative_probe_sampled=1;narrative_probe_last=now;DWORD started=GetTickCount();
    narrative_probe_started=started;narrative_probe_rows=0;narrative_probe_dropped=0;
    narrative_probe_registration_seen_count=0;
    char row[7300];
    int context=scene_readiness_actor(base,pc)&&is_kind_of(base,pc,"BioPlayerController")&&
        scene_readiness_actor(base,world)&&is_kind_of(base,world,"BioWorldInfo")&&read_ptr(pc+0x198)==world;
    if(!context) { snprintf(row,sizeof row,"{\"narrative_probe\":\"sample\",\"context_valid\":false,\"monotonic_ms\":%lu}\r\n",now);line(row);return; }
    uintptr_t modes=0,manager=0,full=0,action=0,data=0,replicated=0;unsigned char mode=255,net=255;
    int modes_read=read_at(pc+0xc38,&modes,8),manager_read=read_at(world+0x948,&manager,8);
    int mode_read=modes_read&&narrative_probe_object(base,modes,"SFXGameModeManager")&&read_at(modes+0xa8,&mode,1);
    int net_read=read_at(world+0x7c8,&net,1);
    int full_read=manager_read&&narrative_probe_get_full(base,manager,&full);
    int full_valid=full_read&&narrative_probe_object(base,full,"BioConversationController");
    int action_read=full_valid&&read_at(full+0x2dc,&action,8),data_read=full_valid&&read_at(full+0x284,&data,8);
    int32_t entry=-1;uint32_t conv_flags=0;
    int fields=full_valid&&read_at(full+0x2ec,&entry,4)&&read_at(full+0x358,&conv_flags,4);
    GameArray active={0};int active_valid=narrative_probe_object(base,manager,"BioConversationManager")&&scene_readiness_array(manager+0xc8,&active,128),members=0;
    if(active_valid)for(int i=0;i<active.count;++i) {uintptr_t member=0;if(!read_at(active.data+(SIZE_T)i*8,&member,8)){active_valid=0;break;}if(member==full&&full)++members;}
    char manager_id[1600],full_id[1600],data_id[1600],action_id[1600];
    narrative_probe_identity(base,manager,manager_read,manager_id,sizeof manager_id);narrative_probe_identity(base,full,full_read,full_id,sizeof full_id);
    narrative_probe_identity(base,data,data_read,data_id,sizeof data_id);narrative_probe_identity(base,action,action_read,action_id,sizeof action_id);
    snprintf(row,sizeof row,"{\"narrative_probe\":\"conversation\",\"context_valid\":true,\"net_mode_read_valid\":%s,\"net_mode\":%u,\"mode_read_valid\":%s,\"current_mode\":%u,\"conversation_mode\":%s,\"cinematic_mode\":%s,\"manager\":%s,\"full_getter_dispatched\":%s,\"full_controller\":%s,\"conversation_data\":%s,\"line_action\":%s,\"controller_fields_valid\":%s,\"entry\":%d,\"controller_flags_raw\":%u,\"active_array_valid\":%s,\"active_count\":%d,\"full_active_memberships\":%d,\"monotonic_ms\":%lu}\r\n",
        narrative_probe_bool(1,net_read),net,narrative_probe_bool(1,mode_read),mode,narrative_probe_bool(mode_read,mode==7),narrative_probe_bool(mode_read,mode==8),manager_id,narrative_probe_bool(1,full_read),full_id,data_id,action_id,
        narrative_probe_bool(1,fields),entry,conv_flags,narrative_probe_bool(1,active_valid),active_valid?active.count:-1,active_valid?members:-1,now);narrative_probe_emit(row);
    int action_valid=action_read&&narrative_probe_object(base,action,"SeqAct_Interp");
    int rep_read=action_valid&&read_at(action+0x15c,&replicated,8);
    narrative_probe_action_state(base,action,action_read,"current-conversation-line",now);
    if(action_valid)narrative_probe_directors(base,action,pc,now);
    narrative_probe_matinee(base,replicated,world,rep_read,"current-conversation-line",-1,now);
    narrative_probe_network_matinees(base,pc,world,now);
    snprintf(row,sizeof row,"{\"narrative_probe\":\"sample\",\"context_valid\":true,\"sampling_duration_ms\":%lu,\"observation_rows\":%u,\"dropped_rows\":%u,\"sampling_truncated\":%s,\"read_only\":true,\"monotonic_ms\":%lu}\r\n",GetTickCount()-started,narrative_probe_rows,narrative_probe_dropped,narrative_probe_bool(1,!narrative_probe_budget()||narrative_probe_dropped),now);line(row);
}
#endif
