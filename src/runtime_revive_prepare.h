#ifndef ME3_RUNTIME_REVIVE_PREPARE_H
#define ME3_RUNTIME_REVIVE_PREPARE_H
/* Explicit preparation-only candidate. No loader, Activate call, RPC, input
 * or claim of peer agreement. The protected probe does not include it. */
typedef struct {
    uint16_t bytes,offset[6];
    uint32_t bool_mask[6];
} RuntimeReviveLayout;
typedef struct {
    GuestCameraStamp factory,cdo,service_class,create,status,retire,handoff,destroy,animset;
} RuntimeReviveApi;
typedef struct {
    GuestCameraStamp pc,pawn,world,channel,connection,driver;
    unsigned long registration;
    int channel_index;
} RuntimeReviveAdmission;
typedef struct {
    RuntimeReviveAdmission scope;
    GuestCameraStamp service;
    uintptr_t uncertain_result;
    int token,attempted,retiring,last_status,busy;
    DWORD poll_at,resolve_at;
} RuntimeRevivePreparation;
static RuntimeRevivePreparation runtime_revive_preparations[2];
static int runtime_revive_dispatching;
static int runtime_revive_serial;
static unsigned runtime_revive_dispatch_generation;
static RuntimeReviveApi runtime_revive_cached_api;
static int runtime_revive_api_cached;

static int runtime_revive_opted(void) {
    static int configured,opted;
    if(!configured) {
        wchar_t value[2]={0};int present=0;
        opted=direct_ip_native_read_environment(L"ME3_SQUAD_COOP_REVIVE_PREPARE_QA",value,2,&present)&&
            present&&!wcscmp(value,L"1");
        configured=1;
    }
    return opted;
}
static void runtime_revive_note(RuntimeRevivePreparation *q,const char *event,int status) {
    char row[420];
    snprintf(row,sizeof row,"{\"runtime_revive_prepare\":\"%s\",\"slot\":%d,\"token\":%d,\"status\":%d,"
        "\"role\":%d,\"scope_generation\":%lu,\"channel_index\":%d,"
        "\"activation_dispatched_by_bridge\":false,\"monotonic_ms\":%llu}\r\n",event,
        (int)(q-runtime_revive_preparations),q->token,status,(int)g_runtime_config.role,
        q->scope.registration,q->scope.channel_index,(unsigned long long)GetTickCount64());line(row);
}
static int runtime_revive_path(uintptr_t base,uintptr_t object,const char *path) {
    GuestCameraStamp stamp;
    return guest_camera_stamp_read(base,object,&stamp,1)&&!strcmp(stamp.full_name,path);
}
/* Find a unique loaded object, including its full outer chain. A matching
 * filename, short owner name or stale cache cannot authorize a call. */
static int runtime_revive_find_classes(uintptr_t base,RuntimeReviveApi *api) {
    static const char *names[]={"SFXSquadCoop_ReviveFactory","Default__SFXSquadCoop_ReviveFactory",
        "SFXSquadCoop_ReviveService","ReviveAnimSet"};
    static const char *paths[]={"ME3SquadCoopActions.SFXSquadCoop_ReviveFactory",
        "ME3SquadCoopActions.Default__SFXSquadCoop_ReviveFactory",
        "ME3SquadCoopActions.SFXSquadCoop_ReviveService","ME3SquadCoopActions.ReviveAnimSet"};
    GuestCameraStamp *outputs[]={&api->factory,&api->cdo,&api->service_class,&api->animset};int found[4]={0};
    GameArray objects={0};uintptr_t *snapshot=identity_object_list(base,&objects);
    if(!snapshot)return 0;
    int valid=1;
    for(int i=0;i<objects.count&&valid;++i) {
        uintptr_t object=snapshot[i];char text[96];
        object_name(base,object,text,sizeof text);
        for(unsigned j=0;j<4;++j) {
            if(strcmp(text,names[j]))continue;
            GuestCameraStamp stamp;
            if(guest_camera_stamp_read(base,object,&stamp,1)&&!strcmp(stamp.full_name,paths[j])) {
                if(found[j]++&&j<3){valid=0;break;}
                *outputs[j]=stamp;
            }
        }
    }
    HeapFree(GetProcessHeap(),0,snapshot);
    if(found[3]!=1||!runtime_revive_path(base,api->animset.klass,"Engine.BioDynamicAnimSet")||
       !is_kind_of(base,api->animset.pointer,"AnimSet"))memset(&api->animset,0,sizeof api->animset);
    return valid&&found[0]==1&&found[1]==1&&found[2]==1;
}
static int runtime_revive_method(uintptr_t base,const GuestCameraStamp *owner,const char *name,
    GuestCameraStamp *out) {
    uintptr_t field=read_ptr(owner->pointer+0x70);int found=0;
    for(int n=0;field&&n<128;++n) {
        if(!is_live(base,field)||read_ptr(field+0x40)!=owner->pointer)return 0;
        char text[96];object_name(base,field,text,sizeof text);
        if(!strcmp(text,name)) {
            if(found++||!runtime_revive_path(base,read_ptr(field+0x50),"Core.Function")||
               !guest_camera_stamp_read(base,field,out,1))return 0;
        }
        uintptr_t next=read_ptr(field+0x68);if(next==field)return 0;field=next;
    }
    return !field&&found==1;
}
/* UFunction/UProperty layout comes from the supported Core SDK. Parameter
 * offsets and bool masks come from the actual loaded function, never inferred
 * from the serialized PCC or the C compiler's struct padding. */
static int runtime_revive_layout(uintptr_t base,const GuestCameraStamp *fn,uintptr_t owner,
    int create,int integer,RuntimeReviveLayout *out) {
    static const char *create_names[]={"Candidate","Admission","LoadedSet","Token","bSimulatedPresentation","ReturnValue"};
    static const char *create_types[]={"SFXGame.SFXPawn_PlayerParty","SFXGame.SFXPlayerController","Engine.AnimSet"};
    unsigned expected=create?6:2,seen=0;uint32_t flags=0;uint16_t bytes=0,ret=0;unsigned char parms=0;
    memset(out,0,sizeof *out);
    if(!guest_camera_stamp_matches(base,fn,1)||read_ptr(fn->pointer+0x40)!=owner||
       !runtime_revive_path(base,read_ptr(fn->pointer+0x50),"Core.Function")||
       !read_at(fn->pointer+0xe0,&flags,4)||(flags&(0x400|0x40))||
       !!(flags&0x2000)!=!!create||!read_at(fn->pointer+0xe6,&bytes,2)||!bytes||bytes>128||
       !read_at(fn->pointer+0xe8,&ret,2)||!read_at(fn->pointer+0xea,&parms,1)||parms!=expected)return 0;
    uintptr_t field=read_ptr(fn->pointer+0x70);
    for(int n=0;field&&n<64;++n) {
        uint64_t pf=0;int32_t dim=0,size=0,offset=-1;
        if(!is_live(base,field)||read_ptr(field+0x40)!=fn->pointer||
           !is_kind_of(base,field,"Property")||!read_at(field+0x78,&pf,8))return 0;
        if(pf&0x80) {
            char name[96];object_name(base,field,name,sizeof name);unsigned which=expected;
            for(unsigned i=0;i<expected;++i)
                if(!strcmp(name,create?create_names[i]:(i?"ReturnValue":"Token")))which=i;
            if(which==expected||(seen&(1u<<which))||!!(pf&0x400)!=(which==expected-1)||
               (which!=expected-1&&(pf&0x100))||!read_at(field+0x70,&dim,4)||dim!=1||
               !read_at(field+0x74,&size,4)||!read_at(field+0x94,&offset,4)||offset<0)return 0;
            int object=create&&(which<3||which==5),boolean=create?which==4:(which==1&&!integer);
            const char *kind=object?"Core.ObjectProperty":boolean?"Core.BoolProperty":"Core.IntProperty";
            if(!runtime_revive_path(base,read_ptr(field+0x50),kind)||size!=(object?8:4)||
               offset>bytes-size||(which==expected-1&&ret!=offset))return 0;
            if(object&&!runtime_revive_path(base,read_ptr(field+0xd0),which==5?"Engine.Actor":create_types[which]))return 0;
            if(boolean) {
                uint32_t mask=0;
                if(!read_at(field+0xd0,&mask,4)||!mask||(mask&(mask-1)))return 0;
                out->bool_mask[which]=mask;
            }
            for(unsigned i=0;i<expected;++i)if(seen&(1u<<i)) {
                int old_size=create&&(i<3||i==5)?8:4;
                if(offset<out->offset[i]+old_size&&out->offset[i]<offset+size)return 0;
            }
            out->offset[which]=(uint16_t)offset;seen|=1u<<which;
        }
        uintptr_t next=read_ptr(field+0x68);if(next==field)return 0;field=next;
    }
    out->bytes=bytes;return !field&&seen==((1u<<expected)-1);
}
static int runtime_revive_resolve(uintptr_t base,RuntimeReviveApi *api) {
    static DWORD last_scan;
    DWORD now=GetTickCount();
    if(runtime_revive_api_cached) {
        RuntimeReviveApi *saved=&runtime_revive_cached_api;
        if(guest_camera_stamp_matches(base,&saved->factory,1)&&guest_camera_stamp_matches(base,&saved->cdo,1)&&
           guest_camera_stamp_matches(base,&saved->service_class,1)&&guest_camera_stamp_matches(base,&saved->create,1)&&
           guest_camera_stamp_matches(base,&saved->status,1)&&guest_camera_stamp_matches(base,&saved->retire,1)&&
           guest_camera_stamp_matches(base,&saved->handoff,1)&&guest_camera_stamp_matches(base,&saved->destroy,1)) {
            if(saved->animset.pointer&&!guest_camera_stamp_matches(base,&saved->animset,1))
                memset(&saved->animset,0,sizeof saved->animset);
            /* Missing animation must not prevent retiring an existing service.
             * Discover a later-loaded asset on the normal bounded cadence. */
            if(saved->animset.pointer||(last_scan&&now-last_scan<5000)){*api=*saved;return 1;}
        }
        runtime_revive_api_cached=0;
    }
    if(last_scan&&now-last_scan<5000)return 0;
    last_scan=now;
    memset(api,0,sizeof *api);RuntimeReviveLayout layout;
    if(!runtime_revive_find_classes(base,api)||
       !runtime_revive_path(base,api->factory.klass,"Core.Class")||
       api->cdo.klass!=api->factory.pointer||
       !runtime_revive_path(base,api->service_class.klass,"Core.Class")||
       !runtime_revive_method(base,&api->factory,"CreateFor",&api->create)||
       !runtime_revive_method(base,&api->service_class,"Status",&api->status)||
       !runtime_revive_method(base,&api->service_class,"Retire",&api->retire)||
       !runtime_revive_method(base,&api->service_class,"RetireForHandoff",&api->handoff)||
       !runtime_revive_method(base,&api->service_class,"DestroyRetired",&api->destroy))return 0;
    int ready=runtime_revive_layout(base,&api->create,api->factory.pointer,1,0,&layout)&&
        runtime_revive_layout(base,&api->status,api->service_class.pointer,0,1,&layout)&&
        runtime_revive_layout(base,&api->retire,api->service_class.pointer,0,0,&layout)&&
        runtime_revive_layout(base,&api->handoff,api->service_class.pointer,0,0,&layout)&&
        runtime_revive_layout(base,&api->destroy,api->service_class.pointer,0,0,&layout);
    if(ready){runtime_revive_cached_api=*api;runtime_revive_api_cached=1;}
    return ready;
}
static int runtime_revive_current_world(uintptr_t base,uintptr_t engine,const GuestCameraStamp *world) {
    CombatContextChain chain;combat_context_read_chain(base,&chain);
    return session_game_thread==GetCurrentThreadId()&&session_engine==engine&&chain.info_valid&&
        chain.info==world->pointer&&guest_camera_stamp_matches(base,world,1)&&
        guest_camera_actor_live(base,world->pointer);
}
static int runtime_revive_dispatch(uintptr_t base,uintptr_t engine,RuntimeRevivePreparation *q,const GuestCameraStamp *world,
    const GuestCameraStamp *object,const GuestCameraStamp *fn,void *params) {
    if(runtime_revive_dispatching||q->busy||!runtime_revive_current_world(base,engine,world)||!guest_camera_stamp_matches(base,object,1)||
       !guest_camera_stamp_matches(base,fn,1))return 0;
    uintptr_t pe=read_ptr(read_ptr(object->pointer)+0x230);
    if(pe<base||pe>=base+0x1300000)return 0;
    runtime_revive_dispatching=q->busy=1;++runtime_revive_dispatch_generation;
    ((ProcessEvent)pe)((void*)object->pointer,(void*)fn->pointer,params,NULL);
    runtime_revive_dispatching=q->busy=0;return 1;
}
static int runtime_revive_service_call(uintptr_t base,uintptr_t engine,RuntimeRevivePreparation *q,const RuntimeReviveApi *api,
    const GuestCameraStamp *fn,int integer,int *result) {
    RuntimeReviveLayout layout;unsigned char params[128]={0};
    if(q->service.klass!=api->service_class.pointer||!guest_camera_actor_live(base,q->service.pointer)||
       read_ptr(q->service.pointer+0x198)!=q->scope.world.pointer||
       !runtime_revive_layout(base,fn,api->service_class.pointer,0,integer,&layout))return 0;
    memcpy(params+layout.offset[0],&q->token,4);
    if(!runtime_revive_dispatch(base,engine,q,&q->scope.world,&q->service,fn,params))return 0;
    uint32_t raw=0;memcpy(&raw,params+layout.offset[1],4);
    if(integer) {if(raw>4)return 0;*result=(int)raw;}
    else *result=(raw&layout.bool_mask[1])!=0;
    return 1;
}
/* Forget a handle only after positive registry evidence of disposal, not
 * merely a failed memory read, elapsed time or a change to GWorld. */
static int runtime_revive_disposed(uintptr_t base,const GuestCameraStamp *stamp) {
    GameArray objects={0};uintptr_t slot=0;
    if(!stamp->pointer||stamp->index<0||!read_at(base+OBJECTS_RVA,&objects,sizeof objects)||
       !objects.data||objects.count<0||objects.count>500000||objects.max<objects.count)return 0;
    if(stamp->index>=objects.count)return 1;
    if(!read_at(objects.data+(SIZE_T)stamp->index*8,&slot,8))return 0;
    return slot!=stamp->pointer;
}
static int runtime_revive_scope_same(const RuntimeReviveAdmission *a,const RuntimeReviveAdmission *b) {
    const GuestCameraStamp *left[]={&a->pc,&a->pawn,&a->world,&a->channel,&a->connection,&a->driver};
    const GuestCameraStamp *right[]={&b->pc,&b->pawn,&b->world,&b->channel,&b->connection,&b->driver};
    if(!a->registration||a->registration!=b->registration||a->channel_index!=b->channel_index)return 0;
    for(unsigned i=0;i<6;++i)if(!guest_camera_stamp_core_equal(left[i],right[i])||
        strcmp(left[i]->full_name,right[i]->full_name))return 0;
    return 1;
}
/* Host candidates come only from the captured story/possession lease, never
 * a pawn class/name scan. Both host pawns must map to that exact guest's
 * reciprocal native channel/connection lifetime. */
static int runtime_revive_read_admission(uintptr_t base,SessionContext c,uintptr_t candidate,
    unsigned long registration,RuntimeReviveAdmission *out) {
    memset(out,0,sizeof *out);NetworkActorIdentityEntry network={0};
    if(!registration||!session_actor_live(base,candidate)||!is_kind_of(base,candidate,"SFXPawn_PlayerParty")||
       read_ptr(candidate+0x198)!=c.world||session_engine_blocked(c))return 0;
    if(g_runtime_config.role==ME3_ROLE_GUEST) {
        if(candidate!=c.pawn||registration!=guest_player_context_registration_token()||
           !guest_player_context_registered_for(c.pc,c.pawn)||
           !guest_player_context_basic_owned(base,c.pc,c.pawn,c.world))return 0;
    } else if(g_runtime_config.role==ME3_ROLE_HOST) {
        if(!session_active||session_releasing||session_fault||session_cleanup_pending||session_pending_reason||
           registration!=session_epoch||c.pc!=session_host||c.pawn!=session_shepard||c.world!=session_world||
           c.squad!=session_squad||(candidate!=session_shepard&&candidate!=session_pawn)||
           !session_story_valid(base)||!session_handle_matches(session_world_handle)||
           !session_handle_matches(session_pc_handle)||!session_handle_matches(session_pawn_handle)||
           !session_handle_matches(session_connection_handle)||
           read_ptr(session_remote+0x67c)!=session_connection||
           !session_peer_connected(base,session_remote,session_world)||
           read_ptr(session_remote+0x3bc)!=session_pawn||read_ptr(session_pawn+0x374)!=session_remote)return 0;
    } else return 0;
    if(!guest_player_context_channel(base,candidate,&network)||
       (g_runtime_config.role==ME3_ROLE_HOST&&network.connection!=session_connection))return 0;
    if(!guest_camera_stamp_read(base,c.pc,&out->pc,1)||!guest_camera_stamp_read(base,candidate,&out->pawn,1)||
       !guest_camera_stamp_read(base,c.world,&out->world,1)||!guest_camera_stamp_read(base,network.channel,&out->channel,1)||
       !guest_camera_stamp_read(base,network.connection,&out->connection,1)||
       !guest_camera_stamp_read(base,network.driver,&out->driver,1))return 0;
    out->registration=registration;out->channel_index=network.index;return 1;
}
static void runtime_revive_prepare_one(uintptr_t base,uintptr_t engine,SessionContext c,uintptr_t candidate,
    unsigned long registration,int ordinary,DWORD now,RuntimeRevivePreparation *q) {
    if(!runtime_revive_opted()||runtime_revive_dispatching||q->busy||session_game_thread!=GetCurrentThreadId()||session_engine!=engine)return;
    if(q->uncertain_result)return; /* Never replace an unvalidated factory result. */
    if(q->service.pointer) {
        if(runtime_revive_disposed(base,&q->service)) {
            memset(&q->service,0,sizeof q->service);runtime_revive_note(q,"service-disposed",0);
        } else {
            if(q->poll_at&&now-q->poll_at<250)return;q->poll_at=now;
            RuntimeReviveAdmission current;
            if(!ordinary||!runtime_control_enabled||!runtime_scope_ready||
               !runtime_revive_read_admission(base,c,candidate,registration,&current)||
               !runtime_revive_scope_same(&current,&q->scope))q->retiring=1;
            if(!runtime_revive_current_world(base,engine,&q->scope.world))return;
            RuntimeReviveApi api;int status=0,result=0;
            if(!runtime_revive_resolve(base,&api)||!runtime_revive_service_call(base,engine,q,&api,&api.status,1,&status)) {
                if(q->last_status!=-2){q->last_status=-2;runtime_revive_note(q,"service-contract-unavailable",-2);}
                return;
            }
            if(status!=q->last_status){q->last_status=status;runtime_revive_note(q,"service-status",status);}
            if(status!=1)q->retiring=1; /* Includes unexpected active admission. */
            if(q->retiring) {
                runtime_revive_service_call(base,engine,q,&api,&api.handoff,0,&result);
                /* A handoff result is not disposal permission. */
                if(runtime_revive_service_call(base,engine,q,&api,&api.status,1,&status)&&status==4&&
                   runtime_revive_service_call(base,engine,q,&api,&api.destroy,0,&result)&&result)
                    runtime_revive_note(q,"destruction-requested",status);
            }
            return;
        }
    }
    if(!ordinary||!runtime_control_enabled||!runtime_scope_ready)return;
    if(q->resolve_at&&now-q->resolve_at<5000)return;q->resolve_at=now;
    RuntimeReviveAdmission admitted;
    if(!runtime_revive_read_admission(base,c,candidate,registration,&admitted))return;
    if(q->attempted&&runtime_revive_scope_same(&admitted,&q->scope))return;
    RuntimeReviveApi api;RuntimeReviveLayout layout;unsigned char params[128]={0};
    if(!runtime_revive_resolve(base,&api)) {
        if(q->last_status!=-2){q->last_status=-2;runtime_revive_note(q,"loaded-contract-unavailable",-2);}
        return;
    }
    if(!api.animset.pointer||!guest_camera_stamp_matches(base,&api.animset,1)) {
        if(q->last_status!=-3){q->last_status=-3;runtime_revive_note(q,"owned-animation-unavailable",-3);}
        return;
    }
    if(!runtime_revive_current_world(base,engine,&admitted.world)||
       !runtime_revive_layout(base,&api.create,api.factory.pointer,1,0,&layout)||runtime_revive_serial==INT32_MAX)return;
    q->scope=admitted;q->token=++runtime_revive_serial;
    q->attempted=1;q->retiring=0;q->last_status=-1;
    memcpy(params+layout.offset[0],&candidate,8);memcpy(params+layout.offset[1],&c.pc,8);
    memcpy(params+layout.offset[2],&api.animset.pointer,8);
    memcpy(params+layout.offset[3],&q->token,4);
    /* Only the exact already-loaded owner-derived set is supplied. Setup
     * validates its unique sequence and registration; no loader is called. */
    RuntimeReviveAdmission checked;
    if(!runtime_revive_read_admission(base,c,candidate,registration,&checked)||
       !runtime_revive_scope_same(&checked,&admitted)||
       !guest_camera_stamp_matches(base,&api.animset,1)||
       !runtime_revive_dispatch(base,engine,q,&q->scope.world,&api.cdo,&api.create,params))return;
    uintptr_t created=0;memcpy(&created,params+layout.offset[5],8);
    if(!created){runtime_revive_note(q,"factory-returned-none",0);return;}
    q->uncertain_result=created;
    if(guest_camera_stamp_read(base,created,&q->service,1)&&q->service.klass==api.service_class.pointer&&
       guest_camera_actor_live(base,created)&&read_ptr(created+0x198)==q->scope.world.pointer&&
       read_ptr(created+0x180)==q->scope.pc.pointer) {
        q->uncertain_result=0;runtime_revive_note(q,"factory-returned-service",0);
    } else runtime_revive_note(q,"factory-result-unvalidated",0);
}
static void runtime_revive_prepare_tick(uintptr_t base,uintptr_t engine,SessionContext c,int ordinary,DWORD now) {
    runtime_revive_prepare_one(base,engine,c,c.pawn,guest_player_context_registration_token(),ordinary,now,
        &runtime_revive_preparations[0]);
}
static void runtime_revive_host_tick(uintptr_t base,uintptr_t engine,DWORD now) {
    if(!runtime_revive_opted()||runtime_revive_dispatching||session_game_thread!=GetCurrentThreadId())return;
    for(unsigned slot=0;slot<2;++slot) {
        SessionContext c=session_context(engine);
        int ordinary=runtime_control_enabled&&runtime_scope_ready&&!session_engine_blocked(c);
        unsigned before=runtime_revive_dispatch_generation;
        runtime_revive_prepare_one(base,engine,c,slot?session_pawn:c.pawn,session_epoch,ordinary,now,
            &runtime_revive_preparations[slot]);
        if(before!=runtime_revive_dispatch_generation)runtime_scope_ready=runtime_session_scope(base,engine);
    }
}
/* Preparation-only cleanup request before the core's UnPossess. It never
 * blocks a narrative handoff, clears a scene gate or opens action admission. */
static void runtime_revive_before_host_release(uintptr_t base,uintptr_t engine) {
    if(!runtime_revive_opted()||runtime_revive_dispatching||session_game_thread!=GetCurrentThreadId())return;
    for(unsigned slot=0;slot<2;++slot) {
        RuntimeRevivePreparation *q=&runtime_revive_preparations[slot];
        q->retiring=1;q->poll_at=0;
        runtime_revive_prepare_one(base,engine,session_context(engine),0,0,0,GetTickCount(),q);
    }
}
#endif
