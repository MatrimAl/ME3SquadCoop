#ifndef ME3_RUNTIME_REVIVE_PREPARE_H
#define ME3_RUNTIME_REVIVE_PREPARE_H
/* Local preparation/activation half. Peer agreement belongs to the ordinary
 * native owner-PC bridge; these helpers never infer it from local readiness.
 * No loader or use/revive input is dispatched here. */
typedef struct {
    uint16_t bytes,offset[6];
    uint32_t bool_mask[6];
} RuntimeReviveLayout;
typedef struct {
    GuestCameraStamp factory,cdo,service_class,create,status,activate,can_activate,suspend,retire,handoff,destroy,animset;
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
    int token,attempted,retiring,last_status,busy,activation_dispatched;
    DWORD poll_at,resolve_at,status_at;
} RuntimeRevivePreparation;
typedef struct {
    RuntimeReviveAdmission scope;
    GuestCameraStamp local_pawn;
    DWORD offered_at;
    int valid;
} RuntimeReviveGuestOffer;
/* Canonical wire order is story Shepard, then guest Ashley on both peers.
 * Stamps are process-local guards and must never be serialized. */
typedef struct {
    RuntimeReviveAdmission scope[2];
    GuestCameraStamp service[2];
    GuestCameraStamp package;
    uint32_t package_guid[4];
    int token[2],status[2];
    uint64_t generation;
    DWORD sampled_at;
} RuntimeRevivePair;
static RuntimeRevivePreparation runtime_revive_preparations[2];
static RuntimeReviveGuestOffer runtime_revive_guest_offer;
static RuntimeRevivePair runtime_revive_pair_current,runtime_revive_pair_armed;
static uint64_t runtime_revive_pair_serial;
static int (*runtime_revive_peer_enabled)(void);
static int runtime_revive_dispatching;
static int runtime_revive_serial;
static unsigned runtime_revive_dispatch_generation;
static RuntimeReviveApi runtime_revive_cached_api;
static int runtime_revive_api_cached;

static int runtime_revive_player_opted(void) {
    static int value=-1;
    if(value<0){wchar_t text[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_REVIVE",text,8);
        value=!n||(n==1&&text[0]==L'1');}
    return value;
}
static int runtime_revive_opted(void) {
    static int configured,opted;
    if(!configured) {
        wchar_t value[2]={0};int present=0;
        opted=direct_ip_native_read_environment(L"ME3_SQUAD_COOP_REVIVE_PREPARE_QA",value,2,&present)&&
            present&&!wcscmp(value,L"1");
        configured=1;
    }
    return opted||(runtime_revive_player_opted()&&runtime_revive_peer_enabled&&runtime_revive_peer_enabled());
}
static void runtime_revive_note(RuntimeRevivePreparation *q,const char *event,int status) {
    char row[420];
    snprintf(row,sizeof row,"{\"runtime_revive_prepare\":\"%s\",\"slot\":%d,\"token\":%d,\"status\":%d,"
        "\"role\":%d,\"scope_generation\":%lu,\"channel_index\":%d,"
        "\"activation_dispatched_by_bridge\":%s,\"monotonic_ms\":%llu}\r\n",event,
        (int)(q-runtime_revive_preparations),q->token,status,(int)g_runtime_config.role,
        q->scope.registration,q->scope.channel_index,q->activation_dispatched?"true":"false",
        (unsigned long long)GetTickCount64());line(row);
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
           guest_camera_stamp_matches(base,&saved->status,1)&&guest_camera_stamp_matches(base,&saved->activate,1)&&
           guest_camera_stamp_matches(base,&saved->can_activate,1)&&guest_camera_stamp_matches(base,&saved->suspend,1)&&
           guest_camera_stamp_matches(base,&saved->retire,1)&&
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
       !runtime_revive_method(base,&api->service_class,"Activate",&api->activate)||
       !runtime_revive_method(base,&api->service_class,"CanActivate",&api->can_activate)||
       !runtime_revive_method(base,&api->service_class,"Suspend",&api->suspend)||
       !runtime_revive_method(base,&api->service_class,"Retire",&api->retire)||
       !runtime_revive_method(base,&api->service_class,"RetireForHandoff",&api->handoff)||
       !runtime_revive_method(base,&api->service_class,"DestroyRetired",&api->destroy))return 0;
    int ready=runtime_revive_layout(base,&api->create,api->factory.pointer,1,0,&layout)&&
        runtime_revive_layout(base,&api->status,api->service_class.pointer,0,1,&layout)&&
        runtime_revive_layout(base,&api->activate,api->service_class.pointer,0,0,&layout)&&
        runtime_revive_layout(base,&api->can_activate,api->service_class.pointer,0,0,&layout)&&
        runtime_revive_layout(base,&api->suspend,api->service_class.pointer,0,0,&layout)&&
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
       read_ptr(q->service.pointer+0x198)!=q->scope.world.pointer||read_ptr(q->service.pointer+0x180)!=q->scope.pc.pointer||
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
static void runtime_revive_clear_guest_offer(void) {
    memset(&runtime_revive_guest_offer,0,sizeof runtime_revive_guest_offer);
}
/* Called only by the later presentation layer after its native remote-story
 * identity was admitted. Revalidate the offered channel and local admission;
 * an offer expires instead of becoming a second pawn-discovery mechanism. */
static int runtime_revive_guest_offer_valid(uintptr_t base,SessionContext c,uintptr_t candidate,
    unsigned long registration,DWORD now,NetworkActorIdentityEntry *network) {
    RuntimeReviveGuestOffer *offer=&runtime_revive_guest_offer;RuntimeReviveAdmission *s=&offer->scope;
    const GuestCameraStamp *stamps[]={&s->pc,&s->pawn,&s->world,&s->channel,&s->connection,&s->driver,&offer->local_pawn};
    unsigned char role=255;
    if(g_runtime_config.role!=ME3_ROLE_GUEST||!offer->valid||now-offer->offered_at>1500||
       !registration||registration!=s->registration||registration!=guest_player_context_registration_token()||
       c.pc!=s->pc.pointer||c.pawn!=offer->local_pawn.pointer||c.world!=s->world.pointer||
       candidate!=s->pawn.pointer||candidate==c.pawn||session_engine_blocked(c)||
       !guest_player_context_registered_for(c.pc,c.pawn)||
       !guest_player_context_basic_owned(base,c.pc,c.pawn,c.world))return 0;
    for(unsigned i=0;i<7;++i)if(!guest_camera_stamp_matches(base,stamps[i],1))return 0;
    return session_actor_live(base,candidate)&&is_kind_of(base,candidate,"SFXPawn_Player")&&
        read_ptr(candidate+0x198)==c.world&&!read_ptr(candidate+0x374)&&
        read_at(candidate+0x26a,&role,1)&&role==1&&read_ptr(s->driver.pointer+0x80)==s->connection.pointer&&
        network_actor_channel_valid(base,candidate,s->channel.pointer,s->driver.pointer,network)&&
        network->index==s->channel_index&&network->connection==s->connection.pointer;
}
static int runtime_revive_offer_guest(uintptr_t base,SessionContext c,uintptr_t replica,
    uintptr_t channel,uintptr_t connection,uintptr_t driver,int index,DWORD now) {
    RuntimeReviveGuestOffer offer={0};NetworkActorIdentityEntry network={0};
    runtime_revive_clear_guest_offer();
    if(session_game_thread!=GetCurrentThreadId()||g_runtime_config.role!=ME3_ROLE_GUEST||index<=0||index>2047||
       !runtime_control_enabled||!runtime_scope_ready||!guest_player_context_registration_token())return 0;
    offer.scope.registration=guest_player_context_registration_token();offer.scope.channel_index=index;
    uintptr_t values[]={c.pc,replica,c.world,channel,connection,driver,c.pawn};
    GuestCameraStamp *stamps[]={&offer.scope.pc,&offer.scope.pawn,&offer.scope.world,&offer.scope.channel,
        &offer.scope.connection,&offer.scope.driver,&offer.local_pawn};
    for(unsigned i=0;i<7;++i)if(!guest_camera_stamp_read(base,values[i],stamps[i],1))return 0;
    offer.valid=1;offer.offered_at=now;runtime_revive_guest_offer=offer;
    if(!runtime_revive_guest_offer_valid(base,c,replica,offer.scope.registration,now,&network)) {
        runtime_revive_clear_guest_offer();return 0;
    }
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
        if(registration!=guest_player_context_registration_token()||
           !guest_player_context_registered_for(c.pc,c.pawn)||
           !guest_player_context_basic_owned(base,c.pc,c.pawn,c.world))return 0;
        if(candidate!=c.pawn&&!runtime_revive_guest_offer_valid(base,c,candidate,registration,GetTickCount(),&network))return 0;
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
    if((!network.channel&&!guest_player_context_channel(base,candidate,&network))||
       (g_runtime_config.role==ME3_ROLE_HOST&&network.connection!=session_connection))return 0;
    if(!guest_camera_stamp_read(base,c.pc,&out->pc,1)||!guest_camera_stamp_read(base,candidate,&out->pawn,1)||
       !guest_camera_stamp_read(base,c.world,&out->world,1)||!guest_camera_stamp_read(base,network.channel,&out->channel,1)||
       !guest_camera_stamp_read(base,network.connection,&out->connection,1)||
       !guest_camera_stamp_read(base,network.driver,&out->driver,1))return 0;
    out->registration=registration;out->channel_index=network.index;return 1;
}
/* Creation is one attempt per admitted scope. Defer that attempt while an
 * ordinary action/downed state would make the authored Bind refuse it. This
 * gate is never used to invalidate an existing prepared/active service. */
static int runtime_revive_creation_idle(uintptr_t pawn) {
    int32_t action=-1,power=-1;uint32_t life=0;
    return read_at(pawn+0xc20,&action,4)&&!action&&read_at(pawn+0xc28,&power,4)&&!power&&
        read_at(pawn+0xe20,&life,4)&&!(life&0x300);
}
static void runtime_revive_prepare_one(uintptr_t base,uintptr_t engine,SessionContext c,uintptr_t candidate,
    unsigned long registration,int ordinary,DWORD now,RuntimeRevivePreparation *q) {
    if(runtime_revive_dispatching||q->busy||session_game_thread!=GetCurrentThreadId()||session_engine!=engine)return;
    if(q->uncertain_result)return; /* Never replace an unvalidated factory result. */
    if(q->service.pointer) {
        if(runtime_revive_disposed(base,&q->service)) {
            memset(&q->service,0,sizeof q->service);runtime_revive_note(q,"service-disposed",0);
        } else {
            if(q->poll_at&&now-q->poll_at<250)return;q->poll_at=now;
            RuntimeReviveAdmission current;
            if(!runtime_revive_opted()||!ordinary||!runtime_control_enabled||!runtime_scope_ready||
               !runtime_revive_read_admission(base,c,candidate,registration,&current)||
               !runtime_revive_scope_same(&current,&q->scope))q->retiring=1;
            if(!runtime_revive_current_world(base,engine,&q->scope.world))return;
            RuntimeReviveApi api;int status=0,result=0;
            if(!runtime_revive_resolve(base,&api)||!runtime_revive_service_call(base,engine,q,&api,&api.status,1,&status)) {
                if(q->last_status!=-2){q->last_status=-2;runtime_revive_note(q,"service-contract-unavailable",-2);}
                return;
            }
            if(status!=q->last_status){q->last_status=status;runtime_revive_note(q,"service-status",status);}
            q->status_at=now;
            if(status!=1&&status!=2)q->retiring=1;
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
    if(!runtime_revive_opted()||!ordinary||!runtime_control_enabled||!runtime_scope_ready)return;
    if(q->resolve_at&&now-q->resolve_at<5000)return;q->resolve_at=now;
    RuntimeReviveAdmission admitted;
    if(!runtime_revive_read_admission(base,c,candidate,registration,&admitted))return;
    if(!runtime_revive_creation_idle(candidate))return;
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
    q->attempted=1;q->retiring=0;q->last_status=-1;q->status_at=0;q->activation_dispatched=0;
    memcpy(params+layout.offset[0],&candidate,8);memcpy(params+layout.offset[1],&c.pc,8);
    memcpy(params+layout.offset[2],&api.animset.pointer,8);
    memcpy(params+layout.offset[3],&q->token,4);
    if(g_runtime_config.role==ME3_ROLE_GUEST&&candidate!=c.pawn)
        memcpy(params+layout.offset[4],&layout.bool_mask[4],4);
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
    unsigned before=runtime_revive_dispatch_generation;
    runtime_revive_prepare_one(base,engine,c,c.pawn,guest_player_context_registration_token(),ordinary,now,
        &runtime_revive_preparations[0]);
    if(before!=runtime_revive_dispatch_generation){runtime_scope_ready=runtime_session_scope(base,engine);c=session_context(engine);}
    runtime_revive_prepare_one(base,engine,c,runtime_revive_guest_offer.valid?runtime_revive_guest_offer.scope.pawn.pointer:0,
        guest_player_context_registration_token(),ordinary&&!session_engine_blocked(c),now,&runtime_revive_preparations[1]);
}
static void runtime_revive_host_tick(uintptr_t base,uintptr_t engine,DWORD now) {
    if(runtime_revive_dispatching||session_game_thread!=GetCurrentThreadId())return;
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
    if(runtime_revive_dispatching||session_game_thread!=GetCurrentThreadId())return;
    memset(&runtime_revive_pair_armed,0,sizeof runtime_revive_pair_armed);
    for(unsigned slot=0;slot<2;++slot) {
        RuntimeRevivePreparation *q=&runtime_revive_preparations[slot];
        q->retiring=1;q->poll_at=0;
        runtime_revive_prepare_one(base,engine,session_context(engine),0,0,0,GetTickCount(),q);
    }
}
static int runtime_revive_status_ready(int status) {return status==1||status==2;}
static int runtime_revive_pair_identity_same(const RuntimeRevivePair *a,const RuntimeRevivePair *b) {
    if(!guest_camera_stamp_core_equal(&a->package,&b->package)||strcmp(a->package.full_name,b->package.full_name)||
       memcmp(a->package_guid,b->package_guid,sizeof a->package_guid))return 0;
    for(unsigned i=0;i<2;++i)if(a->token[i]<=0||a->token[i]!=b->token[i]||
       !runtime_revive_scope_same(&a->scope[i],&b->scope[i])||
       !guest_camera_stamp_core_equal(&a->service[i],&b->service[i])||
       strcmp(a->service[i].full_name,b->service[i].full_name))return 0;
    return 1;
}
static int runtime_revive_pair_matches(const RuntimeRevivePair *a,const RuntimeRevivePair *b,DWORD now) {
    return a->generation&&a->generation==b->generation&&now-a->sampled_at<=3000&&
        now-b->sampled_at<=3000&&runtime_revive_status_ready(a->status[0])&&runtime_revive_status_ready(a->status[1])&&
        runtime_revive_status_ready(b->status[0])&&runtime_revive_status_ready(b->status[1])&&runtime_revive_pair_identity_same(a,b);
}
static void runtime_revive_pair_invalidate(void) {
    memset(&runtime_revive_pair_current,0,sizeof runtime_revive_pair_current);
    memset(&runtime_revive_pair_armed,0,sizeof runtime_revive_pair_armed);
}
static unsigned runtime_revive_canonical_slot(unsigned i) {
    return g_runtime_config.role==ME3_ROLE_GUEST?1u-i:i;
}
/* Read actual script readiness, not the previous 250ms log sample. Every
 * ProcessEvent may re-enter: re-read local/world/channel ownership before
 * reading the next service and again before returning the pair. */
static int runtime_revive_pair_snapshot(uintptr_t base,uintptr_t engine,DWORD now,RuntimeRevivePair *out) {
    RuntimeRevivePair pair={0};RuntimeReviveApi api;uint32_t final_guid[4]={0};
    if(!out)return 0;memset(out,0,sizeof *out);
    if(!runtime_revive_opted()||runtime_revive_dispatching||session_game_thread!=GetCurrentThreadId()||
       engine!=session_engine||!runtime_control_enabled||!runtime_scope_ready||!runtime_revive_resolve(base,&api)||
       !api.animset.pointer||!guest_camera_stamp_matches(base,&api.animset,1))goto invalid;
    uintptr_t package=read_ptr(api.factory.pointer+0x40);
    if(!runtime_revive_path(base,package,"ME3SquadCoopActions")||
       !runtime_revive_path(base,read_ptr(package+0x50),"Core.Package")||
       !guest_camera_stamp_read(base,package,&pair.package,1)||!read_at(package+0x6c,pair.package_guid,16)||
       !(pair.package_guid[0]|pair.package_guid[1]|pair.package_guid[2]|pair.package_guid[3]))goto invalid;
    for(unsigned i=0;i<2;++i) {
        RuntimeRevivePreparation *q=&runtime_revive_preparations[runtime_revive_canonical_slot(i)];
        SessionContext c=session_context(engine);RuntimeReviveAdmission admitted;int status=0;
        runtime_scope_ready=runtime_session_scope(base,engine);
        if(!runtime_scope_ready||q->retiring||q->busy||!q->token||q->uncertain_result||!q->service.pointer||
           !runtime_revive_read_admission(base,c,q->scope.pawn.pointer,q->scope.registration,&admitted)||
           !runtime_revive_scope_same(&admitted,&q->scope)||!guest_camera_stamp_matches(base,&q->service,1)||
           !runtime_revive_service_call(base,engine,q,&api,&api.status,1,&status)||!runtime_revive_status_ready(status))goto invalid;
        if(status!=q->last_status){q->last_status=status;runtime_revive_note(q,"service-status",status);}
        q->status_at=GetTickCount();
        pair.scope[i]=q->scope;pair.service[i]=q->service;pair.token[i]=q->token;pair.status[i]=status;
    }
    if(pair.scope[0].registration!=pair.scope[1].registration||
       pair.scope[0].channel_index==pair.scope[1].channel_index||
       !guest_camera_stamp_core_equal(&pair.scope[0].pc,&pair.scope[1].pc)||
       !guest_camera_stamp_core_equal(&pair.scope[0].world,&pair.scope[1].world)||
       !guest_camera_stamp_core_equal(&pair.scope[0].connection,&pair.scope[1].connection)||
       !guest_camera_stamp_core_equal(&pair.scope[0].driver,&pair.scope[1].driver))goto invalid;
    for(unsigned i=0;i<2;++i) {
        SessionContext c=session_context(engine);RuntimeReviveAdmission admitted;
        RuntimeRevivePreparation *q=&runtime_revive_preparations[runtime_revive_canonical_slot(i)];
        if(q->retiring||q->token!=pair.token[i]||!guest_camera_stamp_matches(base,&pair.service[i],1)||
           !runtime_revive_read_admission(base,c,pair.scope[i].pawn.pointer,pair.scope[i].registration,&admitted)||
           !runtime_revive_scope_same(&admitted,&pair.scope[i]))goto invalid;
    }
    if(!guest_camera_stamp_matches(base,&pair.package,1)||!read_at(package+0x6c,final_guid,16)||
       memcmp(pair.package_guid,final_guid,16))goto invalid;
    if(runtime_revive_pair_current.generation&&runtime_revive_pair_identity_same(&pair,&runtime_revive_pair_current))
        pair.generation=runtime_revive_pair_current.generation;
    else {
        if(runtime_revive_pair_serial==UINT64_MAX)goto invalid;
        pair.generation=++runtime_revive_pair_serial;
        memset(&runtime_revive_pair_armed,0,sizeof runtime_revive_pair_armed);
    }
    pair.sampled_at=GetTickCount();runtime_revive_pair_current=pair;*out=pair;(void)now;return 1;
invalid:
    runtime_revive_pair_invalidate();return 0;
}
/* Arm records a local two-phase identity only. It neither opens admission nor
 * treats service tokens/package GUIDs as peer authentication. */
static int runtime_revive_pair_arm(uintptr_t base,uintptr_t engine,const RuntimeRevivePair *expected,DWORD now) {
    RuntimeRevivePair current;
    if(!expected||!runtime_revive_pair_snapshot(base,engine,now,&current)||
       !runtime_revive_pair_matches(expected,&current,GetTickCount()))return 0;
    runtime_revive_pair_armed=current;return 1;
}
static void runtime_revive_pair_revoke(uintptr_t base,uintptr_t engine,DWORD now) {
    runtime_revive_pair_invalidate();
    for(unsigned slot=0;slot<2;++slot) {
        RuntimeRevivePreparation *q=&runtime_revive_preparations[slot];q->retiring=1;q->poll_at=0;
        /* Normal authored retirement closes admission and owns action cleanup;
         * this does not force an action stop or call an old-world service. */
        runtime_revive_prepare_one(base,engine,session_context(engine),0,0,0,now,q);
    }
}
static int runtime_revive_pair_preflight_ready(const int status[2],const int ready[2]) {
    for(unsigned i=0;i<2;++i)if(!runtime_revive_status_ready(status[i])||(status[i]==1&&!ready[i]))return 0;
    return 1;
}
static int runtime_revive_pair_can_activate(uintptr_t base,uintptr_t engine,const RuntimeRevivePair *expected,DWORD now) {
    RuntimeRevivePair current;int ready[2]={0};
    if(!expected||!runtime_revive_pair_snapshot(base,engine,now,&current)||
       !runtime_revive_pair_matches(expected,&current,GetTickCount()))return 0;
    for(unsigned i=0;i<2;++i) {
        RuntimeReviveApi api;
        if(current.status[i]==2){ready[i]=1;continue;}
        RuntimeRevivePreparation *q=&runtime_revive_preparations[runtime_revive_canonical_slot(i)];
        if(!runtime_revive_resolve(base,&api)||
           !runtime_revive_service_call(base,engine,q,&api,&api.can_activate,0,&ready[i])||!ready[i]||
           !runtime_revive_pair_snapshot(base,engine,GetTickCount(),&current)||
           !runtime_revive_pair_matches(expected,&current,GetTickCount()))return 0;
    }
    return runtime_revive_pair_preflight_ready(current.status,ready);
}
/* A timeout closes new admissions without discarding the prepared lifetimes.
 * New agreement must use a new generation; existing actions retain their own
 * normal completion/cancel cleanup. A failed close falls back to retirement. */
static int runtime_revive_pair_suspend(uintptr_t base,uintptr_t engine,DWORD now) {
    runtime_revive_pair_invalidate();
    for(unsigned i=0;i<2;++i) {
        RuntimeRevivePreparation *q=&runtime_revive_preparations[runtime_revive_canonical_slot(i)];
        runtime_scope_ready=runtime_session_scope(base,engine);
        SessionContext c=session_context(engine);RuntimeReviveAdmission admitted;RuntimeReviveApi api;int closed=0,status=0;
        if(!runtime_control_enabled||!runtime_scope_ready||!q->service.pointer||q->retiring||q->uncertain_result||
           !runtime_revive_read_admission(base,c,q->scope.pawn.pointer,q->scope.registration,&admitted)||
           !runtime_revive_scope_same(&admitted,&q->scope)||!runtime_revive_resolve(base,&api)||
           !runtime_revive_service_call(base,engine,q,&api,&api.suspend,0,&closed)||!closed||
           !runtime_revive_service_call(base,engine,q,&api,&api.status,1,&status)||status!=1)goto failed;
        q->last_status=status;q->status_at=GetTickCount();runtime_revive_note(q,"admission-suspended",status);
    }
    return 1;
failed:
    runtime_revive_pair_revoke(base,engine,now);return 0;
}
static int runtime_revive_pair_activate(uintptr_t base,uintptr_t engine,const RuntimeRevivePair *expected,
    int peer_agreed,DWORD now) {
    RuntimeRevivePair current,armed=runtime_revive_pair_armed;int activation_attempted=0;
    if(!peer_agreed||!expected||!runtime_revive_pair_matches(expected,&armed,now)||
       !runtime_revive_pair_snapshot(base,engine,now,&current)||
       !runtime_revive_pair_matches(expected,&current,GetTickCount()))return 0;
    /* A busy closed slot is a normal retry, not a partially opened pair.
     * Check both before the first Activate can change admission. */
    if(!runtime_revive_pair_can_activate(base,engine,expected,GetTickCount()))return 0;
    for(unsigned i=0;i<2;++i) {
        RuntimeReviveApi api;int result=0;
        /* A previous Activate may change the native world or either slot.
         * Recheck the full pair, not just this function's return value. */
        if(!runtime_revive_pair_snapshot(base,engine,GetTickCount(),&current)||
           !runtime_revive_pair_matches(expected,&current,GetTickCount())||
           !runtime_revive_pair_matches(&armed,&current,GetTickCount())||!runtime_revive_resolve(base,&api))goto failed;
        RuntimeRevivePreparation *q=&runtime_revive_preparations[runtime_revive_canonical_slot(i)];
        if(current.status[i]==1) {
            activation_attempted=1;
            q->activation_dispatched=1;
            if(!runtime_revive_service_call(base,engine,q,&api,&api.activate,0,&result)||!result)goto failed;
            runtime_revive_note(q,"activation-dispatched",1);
        }
    }
    if(!runtime_revive_pair_snapshot(base,engine,GetTickCount(),&current)||
       !runtime_revive_pair_matches(expected,&current,GetTickCount())||current.status[0]!=2||current.status[1]!=2)goto failed;
    memset(&runtime_revive_pair_armed,0,sizeof runtime_revive_pair_armed);return 1;
failed:
    if(activation_attempted)runtime_revive_pair_revoke(base,engine,GetTickCount());return 0;
}
#endif
