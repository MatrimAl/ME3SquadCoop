#ifndef ME3_RUNTIME_REMOTE_HELMET_H
#define ME3_RUNTIME_REMOTE_HELMET_H
/* Ordinary remote-Shepard presentation only. The caller supplies fresh host
 * controller slots from its validated native owner-PC exchange. IDs, meshes,
 * roles, animation trees and profile preferences are never copied or written.
 * Stock ForceSquadHelmet derives the required variant and requests its assets;
 * the simulated helmet-only builder completes that existing loaded group. */
typedef struct RuntimeRemoteHelmetPending {
    int valid,pending,requested;
    GuestCameraStamp pawn,world;
    unsigned char states[4];
    uint32_t group[2];
    int ids[15];
    DWORD last_tick;
} RuntimeRemoteHelmetPending;
static RuntimeRemoteHelmetPending runtime_remote_helmet_pending;
static unsigned runtime_remote_helmet_events;
static int runtime_remote_helmet_active;
static int runtime_remote_helmet_states_valid(const unsigned char states[4]) {
    if(!states)return 0;
    for(int i=0;i<4;++i)if(states[i]>3)return 0;
    return 1;
}
static unsigned runtime_remote_helmet_effective(const unsigned char states[4]) {
    for(int i=0;i<4;++i)if(states[i])return states[i];
    return 0;
}
static int runtime_remote_helmet_default_metadata(uintptr_t pawn) {
    int ids[14];uint32_t sex=0,flags=0;
    if(!read_at(pawn+0x14dc,ids,sizeof ids)||!read_at(pawn+0x578,&sex,4)||
       !read_at(pawn+0x1560,&flags,4)||sex&0x20||flags&0x18c1||read_ptr(pawn+0xb40))return 0;
    for(int i=0;i<14;++i)if(ids[i]!=(i==1?-1:0))return 0;
    return 1;
}
static void runtime_remote_helmet_log(const char *event,const unsigned char states[4],int ready) {
    if(runtime_remote_helmet_events>=96)return;
    ++runtime_remote_helmet_events;char row[400];
    snprintf(row,sizeof row,"{\"remote_helmet\":\"%s\",\"slots\":[%u,%u,%u,%u],\"effective\":%u,\"ready\":%s,\"role_written\":false,\"direct_tree_write\":false,\"monotonic_ms\":%lu}\r\n",
        event,states[0],states[1],states[2],states[3],runtime_remote_helmet_effective(states),ready?"true":"false",GetTickCount());line(row);
}
static uintptr_t runtime_remote_helmet_script(uintptr_t base,uintptr_t pawn,const char *owner,
        const char *name,uint32_t flags,uint16_t size,unsigned char count) {
    uintptr_t fn=guest_player_context_find_method(base,read_ptr(pawn+0x50),owner,name);
    if(!guest_player_context_function_abi(base,fn,owner,name,flags,size,65535,0,0xf52b0,count))return 0;
    return fn;
}
static int runtime_remote_helmet_executable(uintptr_t base,uintptr_t native) {
    MEMORY_BASIC_INFORMATION page={0};
    return native>=base&&native<base+0x1100000&&VirtualQuery((void*)native,&page,sizeof page)&&
        page.AllocationBase==(void*)base&&
        (page.Protect&(PAGE_EXECUTE|PAGE_EXECUTE_READ|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY));
}
/* Native component operations use the reflected method in this exact supported
 * image, full declaring path, complete parameter layout and executable wrapper.
 * No guessed wrapper RVA or script flag/actor-role mutation authorizes a call. */
static uintptr_t runtime_remote_helmet_native(uintptr_t base,uintptr_t object,const char *owner,
        const char *name,uint32_t flags,uint16_t size,unsigned char count) {
    uintptr_t fn=guest_player_context_find_method(base,read_ptr(object+0x50),owner,name);
    uintptr_t native=read_ptr(fn+0xd8);GuestCameraStamp stamp;char path[180];
    snprintf(path,sizeof path,"Engine.%s.%s",owner,name);
    if(!guest_camera_stamp_read(base,fn,&stamp,1)||strcmp(stamp.full_name,path)||
       !runtime_remote_helmet_executable(base,native)||
       !guest_player_context_function_abi(base,fn,owner,name,flags,size,65535,0,native-base,count))return 0;
    return fn;
}
static int runtime_remote_helmet_param(uintptr_t base,uintptr_t fn,const char *name,const char *kind,
        int offset,uint64_t flags) {
    uintptr_t field=read_ptr(fn+0x70);int found=0;
    for(int i=0;field&&i<32;++i){
        char text[96];uint64_t field_flags=0;int32_t field_offset=-1;int32_t dim=0,element=0;
        if(!is_live(base,field)||read_ptr(field+0x40)!=fn||!read_at(field+0x78,&field_flags,8))return 0;
        object_name(base,field,text,sizeof text);
        if(!strcmp(text,name)){
            if(found++||!narrative_probe_object(base,field,kind)||field_flags!=flags||
               !read_at(field+0x94,&field_offset,4)||field_offset!=offset||
               !read_at(field+0x70,&dim,4)||dim!=1||!read_at(field+0x74,&element,4))return 0;
            if(!strcmp(kind,"BoolProperty")){uint64_t mask=0;if(element!=4||!read_at(field+0xd0,&mask,8)||mask!=1)return 0;}
            if(!strcmp(kind,"ObjectProperty")&&element!=8)return 0;
            if(!strcmp(kind,"NameProperty")&&element!=8)return 0;
            if(!strcmp(kind,"ByteProperty")&&element!=1)return 0;
        }
        field=read_ptr(field+0x68);
    }
    return !field&&found==1;
}
static int runtime_remote_helmet_dispatch(uintptr_t base,uintptr_t actor,uintptr_t fn,void *params) {
    uintptr_t pe=read_ptr(read_ptr(actor)+0x230);
    if(!is_live(base,actor)||!is_live(base,fn)||(pe!=base+0x40d450&&pe!=base+0xf4f80))return 0;
    ((ProcessEvent)pe)((void*)actor,(void*)fn,params,NULL);return 1;
}
/* Generic script entry 0xf52b0 calls Actor callspace with a null frame even
 * for internal final calls. RequiresFullHelmet lacks FUNC_Simulated. Admit
 * only that exact getter while the validated stock helmet request executes;
 * its original body still computes the result from pawn/world requirements. */
static int runtime_remote_helmet_stock_dispatch(uintptr_t base,uintptr_t pawn,uintptr_t fn,void *params) {
    uintptr_t required=guest_player_context_find_method(base,read_ptr(pawn+0x50),"SFXPawn_Player","RequiresFullHelmet");
    GuestCameraStamp stamp;unsigned char role=255;
    if(!guest_camera_stamp_read(base,required,&stamp,1)||strcmp(stamp.full_name,"SFXGame.SFXPawn_Player.RequiresFullHelmet")||
       !guest_player_context_function_abi(base,required,"SFXPawn_Player","RequiresFullHelmet",0x20003,4,0,0,0xf52b0,1)||
       !runtime_remote_helmet_param(base,required,"ReturnValue","BoolProperty",0,0x580)||
       !read_at(pawn+0x26a,&role,1)||role!=1||read_ptr(read_ptr(pawn)+0x248)!=base+0x70a250||
       !local_squad_hook_ready(base)||TlsGetValue(local_squad_tls))return 0;
    LocalSquadCall scope={pawn,required,0,8};
    if(!TlsSetValue(local_squad_tls,&scope))return 0;
    int dispatched=runtime_remote_helmet_dispatch(base,pawn,fn,params);
    int cleared=TlsSetValue(local_squad_tls,NULL);
    if(scope.adapted){unsigned char states[4]={0};read_at(pawn+0x105c,states,4);runtime_remote_helmet_log("requires-full-callspace-adapted",states,0);}
    return dispatched&&cleared;
}
static int runtime_remote_helmet_component(uintptr_t base,uintptr_t pawn,uintptr_t component) {
    return narrative_probe_object(base,component,"SkeletalMeshComponent")&&read_ptr(component+0x40)==pawn&&
        (!read_ptr(component+0x78)||read_ptr(component+0x78)==pawn);
}
static int runtime_remote_helmet_visibility(uintptr_t base,uintptr_t pawn,uintptr_t component,int hidden) {
    uint32_t flags=0;
    if(!runtime_remote_helmet_component(base,pawn,component)||!read_at(component+0x168,&flags,4))return 0;
    if(((flags&4)!=0)==hidden)return 1;
    uintptr_t fn=runtime_remote_helmet_native(base,component,"PrimitiveComponent","SetHidden",0x20401,4,1);
    uint32_t value=(uint32_t)hidden;
    if(!fn||!runtime_remote_helmet_param(base,fn,"NewHidden","BoolProperty",0,0x80)||
       !runtime_remote_helmet_dispatch(base,component,fn,&value)||!read_at(component+0x168,&flags,4))return 0;
    return ((flags&4)!=0)==hidden;
}
static int runtime_remote_helmet_attach(uintptr_t base,uintptr_t pawn,const unsigned char states[4]) {
    uintptr_t gear=read_ptr(pawn+0xb50),head=read_ptr(pawn+0xb38),hair=read_ptr(pawn+0xb48);
    uint32_t attached=0,helmet=0;
    if(!runtime_remote_helmet_component(base,pawn,gear)||!read_at(gear+0x80,&attached,4)||
       !read_at(pawn+0x1058,&helmet,4))return 0;
    unsigned state=runtime_remote_helmet_effective(states);uintptr_t asset=read_ptr(gear+0x258);
    if(state==2&&!is_live(base,asset))return 0;
    if(state==2&&!(helmet&8)){
        int id=-1;uint32_t flags=0;
        if(!read_at(pawn+0x1514,&id,4)||id<=0||!read_at(pawn+0x1560,&flags,4)||
           !(flags&2)||asset!=read_ptr(pawn+0x1454))return 0;
    }
    int show=(state==1||state==2)&&is_live(base,asset);
    if(((attached&1)!=0)!=show){
        const char *name=show?"AttachComponent":"DetachComponent";
        uintptr_t fn=runtime_remote_helmet_native(base,pawn,"Actor",name,0x20401,8,1);
        if(!fn||!runtime_remote_helmet_param(base,fn,show?"NewComponent":"ExComponent","ObjectProperty",0,0x4000080)||
           !runtime_remote_helmet_dispatch(base,pawn,fn,&gear)||read_ptr(pawn+0xb50)!=gear||
           !runtime_remote_helmet_component(base,pawn,gear)||!read_at(gear+0x80,&attached,4)||((attached&1)!=0)!=show)return 0;
    }
    return runtime_remote_helmet_visibility(base,pawn,head,show&&(helmet&1))&&
        runtime_remote_helmet_visibility(base,pawn,hair,show&&(helmet&3));
}
/* A pending load is identified by exact pawn/world/group/IDs. It may be
 * completed only while the same fresh host slots remain requested. */
static int runtime_remote_helmet_apply(uintptr_t base,uintptr_t engine,uintptr_t pc,uintptr_t local,
        uintptr_t world,uintptr_t pawn,const unsigned char states[4],DWORD now) {
    RuntimeRemoteHelmetPending *p=&runtime_remote_helmet_pending;unsigned char role=255,current[4],net=255;
    GuestCameraStamp pawn_stamp,world_stamp;
    if(runtime_remote_helmet_active||!runtime_remote_helmet_states_valid(states)||
       !guest_player_context_basic_owned(base,pc,local,world)||pawn==local||
       !guest_camera_actor_live(base,pawn)||!is_kind_of(base,pawn,"SFXPawn_Player")||
       read_ptr(pawn+0x198)!=world||!read_at(pawn+0x26a,&role,1)||role!=1||
       !read_at(world+0x7c8,&net,1)||net!=3||network_actor_channel_id(base,pawn)<0||
       !runtime_remote_helmet_default_metadata(pawn)||
       !read_at(pawn+0x105c,current,4)||!runtime_remote_helmet_states_valid(current)||
       !guest_camera_stamp_read(base,pawn,&pawn_stamp,1)||!guest_camera_stamp_read(base,world,&world_stamp,1))return 0;
    uintptr_t mesh=read_ptr(pawn+0x3f4),body=read_ptr(mesh+0x258),tree=read_ptr(mesh+0x270);
    if(!narrative_probe_object(base,mesh,"SkeletalMeshComponent")||!is_live(base,body))return 0;
    int same=p->valid&&guest_camera_stamp_core_equal(&p->pawn,&pawn_stamp)&&
        guest_camera_stamp_core_equal(&p->world,&world_stamp)&&!memcmp(p->states,states,4);
    if(!same){memset(p,0,sizeof *p);p->valid=1;p->pawn=pawn_stamp;p->world=world_stamp;memcpy(p->states,states,4);}
    if(p->last_tick&&now-p->last_tick<500)return 0;
    p->last_tick=now;runtime_remote_helmet_active=1;
    int changed=memcmp(current,states,4)!=0,ok=0;
    if(changed){
        uintptr_t force=runtime_remote_helmet_script(base,pawn,"SFXPawn_PlayerParty","ForceSquadHelmet",0x20103,2,2);
        if(!force||!runtime_remote_helmet_param(base,force,"InController","ByteProperty",0,0x80)||
           !runtime_remote_helmet_param(base,force,"InState","ByteProperty",1,0x80))goto done;
        int first=4;for(int i=0;i<4;++i)if(states[i]){first=i;break;}
        int order[5]={first,3,2,1,0};
        for(int n=0;n<5;++n){int slot=order[n];if(slot>=4||(n&&slot==first)||current[slot]==states[slot])continue;
            unsigned char params[2]={(unsigned char)slot,states[slot]};
            if(!guest_camera_stamp_matches(base,&pawn_stamp,1)||!guest_camera_stamp_matches(base,&world_stamp,1)||
               !runtime_remote_helmet_stock_dispatch(base,pawn,force,params)||!read_at(pawn+0x105c,current,4)||current[slot]!=states[slot])goto done;
        }
        if(memcmp(current,states,4))goto done;
        p->pending=read_at(pawn+0x148c,p->group,8)&&(p->group[0]||p->group[1])&&
            read_at(pawn+0x14dc,p->ids,sizeof p->ids);
        runtime_remote_helmet_log("stock-slots-applied",states,0);
    }
    /* Matching slots alone do not prove a prior role-1 load derived a full
     * variant. Retry the normal blocking request once for this exact context;
     * never pulse ForceSquadHelmet through an invented intermediate state. */
    int override_id=-1;uint32_t helmet_flags=0,presentation_flags=0;
    uintptr_t gear_asset=read_ptr(read_ptr(pawn+0xb50)+0x258);
    if(!read_at(pawn+0x1514,&override_id,4)||!read_at(pawn+0x1058,&helmet_flags,4)||
       !read_at(pawn+0x1560,&presentation_flags,4))goto done;
    int full_unresolved=!is_live(base,gear_asset)||(!(helmet_flags&8)&&
        (override_id<=0||!(presentation_flags&2)||gear_asset!=read_ptr(pawn+0x1454)));
    if(!p->pending&&!p->requested&&runtime_remote_helmet_effective(states)==2&&full_unresolved){
        uintptr_t update=runtime_remote_helmet_script(base,pawn,"SFXPawn_Player","UpdateAppearance",0x20102,0,0);
        if(!update)goto done;
        p->requested=1;
        if(!runtime_remote_helmet_stock_dispatch(base,pawn,update,NULL))goto done;
        if(!guest_camera_stamp_matches(base,&pawn_stamp,1)||!guest_camera_stamp_matches(base,&world_stamp,1))goto done;
        p->pending=read_at(pawn+0x148c,p->group,8)&&(p->group[0]||p->group[1])&&
            read_at(pawn+0x14dc,p->ids,sizeof p->ids);
        runtime_remote_helmet_log("matching-full-slots-requested",states,0);
    }
    if(p->pending){
        uint32_t group[2];int ids[15];
        uintptr_t loader=narrative_probe_object(base,engine,"SFXEngine")?read_ptr(engine+0xd90):0;
        if(!narrative_probe_object(base,loader,"SFXAsyncAssetLoader")||!read_at(pawn+0x148c,group,8)||
           memcmp(group,p->group,8)||!read_at(pawn+0x14dc,ids,sizeof ids)||memcmp(ids,p->ids,sizeof ids))goto done;
        uintptr_t ready=guest_player_context_find_method(base,read_ptr(loader+0x50),"SFXAsyncAssetLoader","IsAsyncGroupLoaded");
        uintptr_t native=read_ptr(ready+0xd8);GuestCameraStamp ready_stamp;
        if(!runtime_remote_helmet_executable(base,native)||
           !guest_camera_stamp_read(base,ready,&ready_stamp,1)||strcmp(ready_stamp.full_name,"SFXGame.SFXAsyncAssetLoader.IsAsyncGroupLoaded")||
           !guest_player_context_function_abi(base,ready,"SFXAsyncAssetLoader","IsAsyncGroupLoaded",0x20501,12,8,0,native-base,2)||
           !runtime_remote_helmet_param(base,ready,"nmAsyncGroupTag","NameProperty",0,0x80)||
           !runtime_remote_helmet_param(base,ready,"ReturnValue","BoolProperty",8,0x580))goto done;
        struct {uint32_t group[2],result;} loaded={{group[0],group[1]},0};
        if(!runtime_remote_helmet_dispatch(base,loader,ready,&loaded)||loaded.result!=1||
           !remote_appearance_group_assets_live(base,loader,group)||read_ptr(engine+0xd90)!=loader||
           !guest_camera_stamp_matches(base,&pawn_stamp,1)||!guest_camera_stamp_matches(base,&world_stamp,1)||
           !read_at(pawn+0x105c,current,4)||memcmp(current,states,4)||read_ptr(pawn+0x3f4)!=mesh||
           read_ptr(mesh+0x258)!=body||read_ptr(mesh+0x270)!=tree)goto done;
        uintptr_t build=runtime_remote_helmet_script(base,pawn,"SFXPawn_Player","UpdateHelmetComponent",0x40103,0,0);
        if(!build||!runtime_remote_helmet_dispatch(base,pawn,build,NULL))goto done;
        p->pending=0;runtime_remote_helmet_log("loaded-helmet-built",states,0);
    }
    if(!guest_camera_stamp_matches(base,&pawn_stamp,1)||!guest_camera_stamp_matches(base,&world_stamp,1)||
       read_ptr(pawn+0x3f4)!=mesh||read_ptr(mesh+0x258)!=body||read_ptr(mesh+0x270)!=tree)goto done;
    ok=runtime_remote_helmet_attach(base,pawn,states);
    if(changed||!same)runtime_remote_helmet_log(ok?"presentation-ready":"presentation-not-ready",states,ok);
done:
    runtime_remote_helmet_active=0;return ok;
}
static int runtime_remote_helmet_fixture_result,runtime_remote_helmet_fixture_calls;
static int runtime_remote_helmet_fixture_original(void *a,void *f,void *p,void *frame) {
    (void)a;(void)f;(void)p;(void)frame;++runtime_remote_helmet_fixture_calls;
    return runtime_remote_helmet_fixture_result;
}
static DWORD WINAPI runtime_remote_helmet_fixture_thread(void *unused) {
    (void)unused;return (DWORD)local_squad_callspace((void*)1,(void*)2,NULL,NULL);
}
static int runtime_remote_helmet_scope_selftest(void) {
    if(local_squad_installed)return 0; /* This fixture must never alter a live hook. */
    LocalSquadCallspace saved=local_squad_original;DWORD saved_tls=local_squad_tls,error=GetLastError();
    DWORD fixture_tls=TlsAlloc();if(fixture_tls==TLS_OUT_OF_INDEXES)return 0;
    local_squad_tls=fixture_tls;local_squad_original=runtime_remote_helmet_fixture_original;
    LocalSquadCall call={1,2,0,2};int ok=TlsSetValue(fixture_tls,&call);
    runtime_remote_helmet_fixture_result=1;runtime_remote_helmet_fixture_calls=0;
    ok=ok&&local_squad_callspace((void*)3,(void*)2,NULL,NULL)==1&&!call.adapted;
    ok=ok&&local_squad_callspace((void*)1,(void*)3,NULL,NULL)==1&&!call.adapted;
    ok=ok&&local_squad_callspace((void*)1,(void*)2,NULL,(void*)1)==1&&!call.adapted;
    runtime_remote_helmet_fixture_result=0;
    ok=ok&&local_squad_callspace((void*)1,(void*)2,NULL,NULL)==0&&!call.adapted;
    runtime_remote_helmet_fixture_result=1;
    HANDLE thread=CreateThread(NULL,0,runtime_remote_helmet_fixture_thread,NULL,0,NULL);DWORD result=0;
    if(!thread)ok=0;
    else {ok=WaitForSingleObject(thread,5000)==WAIT_OBJECT_0&&GetExitCodeThread(thread,&result)&&result==1&&ok;CloseHandle(thread);}
    ok=ok&&local_squad_callspace((void*)1,(void*)2,NULL,NULL)==0&&call.adapted==1;
    ok=ok&&local_squad_callspace((void*)1,(void*)2,NULL,NULL)==0&&call.adapted==2;
    ok=ok&&local_squad_callspace((void*)1,(void*)2,NULL,NULL)==1&&call.adapted==2;
    ok=runtime_remote_helmet_fixture_calls==8&&ok;
    int cleared=TlsSetValue(fixture_tls,NULL),freed=TlsFree(fixture_tls);
    local_squad_tls=saved_tls;local_squad_original=saved;SetLastError(error);
    return ok&&cleared&&freed;
}
static int runtime_remote_helmet_selftest(void) {
    unsigned char states[4]={0,2,3,1};
    if(!runtime_remote_helmet_states_valid(states)||runtime_remote_helmet_effective(states)!=2)return 0;
    states[0]=3;if(runtime_remote_helmet_effective(states)!=3)return 0;
    memset(states,0,4);if(runtime_remote_helmet_effective(states))return 0;
    for(int i=0;i<4;++i){states[i]=4;if(runtime_remote_helmet_states_valid(states))return 0;states[i]=0;}
    return !runtime_remote_helmet_states_valid(NULL)&&runtime_remote_helmet_scope_selftest();
}
#endif
