#ifndef ME3_RUNTIME_ARCHETYPE_RETENTION_H
#define ME3_RUNTIME_ARCHETYPE_RETENTION_H
/* Keep already loaded stock player templates through ordinary join travel.
 * The native network still selects the host archetype. We retain every loaded
 * player template in BioChar_Player, never substitute the guest's own pawn.
 * Only the stock loader owns references; no root flags or package-map writes. */
#define RUNTIME_ARCHETYPE_LIMIT 64
typedef struct RuntimeAssetRequest {
    GameString path;
    uintptr_t klass;
    uint32_t package[2];
    uintptr_t asset;
} RuntimeAssetRequest;
_Static_assert(sizeof(RuntimeAssetRequest)==40,"SFXAsyncAssetRequest ABI");
typedef struct RuntimeArchetypeRetention {
    int active,inflight,connected,count;
    GuestCameraStamp engine,loader,world;
    GuestCameraStamp assets[RUNTIME_ARCHETYPE_LIMIT];
    uint32_t group[2];
} RuntimeArchetypeRetention;
static RuntimeArchetypeRetention runtime_archetypes;
static int runtime_archetype_path(const char *path) {
    static const char prefix[]="BioChar_Player.Archetypes.";
    if(strncmp(path,prefix,sizeof prefix-1)||!path[sizeof prefix-1])return 0;
    for(const char *c=path;*c;++c)
        if(!((*c>='A'&&*c<='Z')||(*c>='a'&&*c<='z')||(*c>='0'&&*c<='9')||*c=='_'||*c=='.'))return 0;
    return 1;
}
static uintptr_t runtime_asset_method(uintptr_t base,uintptr_t loader,const char *name,
        uint32_t flags,uint16_t bytes,unsigned char count,uintptr_t rva) {
    uintptr_t fn=guest_player_context_find_method(base,read_ptr(loader+0x50),"SFXAsyncAssetLoader",name);
    GuestCameraStamp stamp={0};char path[128];snprintf(path,sizeof path,"SFXGame.SFXAsyncAssetLoader.%s",name);
    if(!guest_camera_stamp_read(base,fn,&stamp,1)||strcmp(path,stamp.full_name)||
       !guest_player_context_function_abi(base,fn,"SFXAsyncAssetLoader",name,flags,bytes,65535,0,rva,count))return 0;
    return fn;
}
/* -1 means unreadable, never an empty group. Check callback records too. */
static int runtime_asset_group_count(uintptr_t loader,const uint32_t group[2]) {
    int found=0;
    for(int kind=0;kind<2;++kind){
        GameArray list={0},again={0};int stride=kind?24:32,offset=kind?16:24;
        if(!read_at(loader+(kind?0x78:0x68),&list,sizeof list)||list.count<0||list.count>4096||
           list.max<list.count||list.max>16384||(list.count&&!list.data))return -1;
        for(int i=0;i<list.count;++i){uint32_t name[2];
            if(!read_at(list.data+(uintptr_t)i*stride+offset,name,8))return -1;
            if(!memcmp(name,group,8))++found;
        }
        if(!read_at(loader+(kind?0x78:0x68),&again,sizeof again)||memcmp(&list,&again,sizeof list))return -1;
    }
    return found;
}
static int runtime_archetype_loader_matches(uintptr_t base,uintptr_t engine) {
    return runtime_archetypes.active&&runtime_archetypes.engine.pointer==engine&&
        guest_camera_stamp_matches(base,&runtime_archetypes.engine,1)&&
        guest_camera_stamp_matches(base,&runtime_archetypes.loader,1)&&
        read_ptr(engine+0xd90)==runtime_archetypes.loader.pointer;
}
static int runtime_archetype_group_owned(uintptr_t base,uintptr_t engine,int loaded) {
    if(!runtime_archetypes.active||!guest_camera_stamp_matches(base,&runtime_archetypes.loader,1)||
       (engine&&!runtime_archetype_loader_matches(base,engine)))return 0;
    uintptr_t loader=runtime_archetypes.loader.pointer;
    GameArray packages={0},after={0};unsigned char found[RUNTIME_ARCHETYPE_LIMIT]={0};int total=0;
    if(!read_at(loader+0x68,&packages,sizeof packages)||packages.count<1||packages.count>4096||
       packages.max<packages.count||packages.max>16384||!packages.data)return 0;
    for(int i=0;i<packages.count;++i){uintptr_t entry=packages.data+(uintptr_t)i*32;uint32_t group[2];GameArray requests={0},requests_after={0};
        if(!read_at(entry+24,group,8))return 0;
        if(memcmp(group,runtime_archetypes.group,8))continue;
        if(!read_at(entry,&requests,sizeof requests)||requests.count<1||requests.count>RUNTIME_ARCHETYPE_LIMIT||
           requests.max<requests.count||requests.max>4096||!requests.data)return 0;
        for(int j=0;j<requests.count;++j){RuntimeAssetRequest request={0};int match=-1;
            if(!read_at(requests.data+(uintptr_t)j*40,&request,sizeof request))return 0;
            wchar_t path[640],expected[640];
            if(!request.path.data||request.path.count<2||request.path.count>640||
               request.path.max<request.path.count||request.path.max>4096||
               !read_at((uintptr_t)request.path.data,path,(size_t)request.path.count*sizeof(wchar_t))||path[request.path.count-1])return 0;
            for(int k=0;k<runtime_archetypes.count;++k){
                int length=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,runtime_archetypes.assets[k].full_name,-1,expected,640);
                if(length==request.path.count&&!memcmp(path,expected,(size_t)length*sizeof(wchar_t)))match=k;
            }
            if(match<0||found[match]||request.klass!=runtime_archetypes.assets[match].klass||
               (request.asset&&request.asset!=runtime_archetypes.assets[match].pointer)||
               ((loaded||request.asset)&&(!request.asset||!guest_camera_stamp_matches(base,&runtime_archetypes.assets[match],1))))return 0;
            found[match]=1;++total;
        }
        if(!read_at(entry,&requests_after,sizeof requests_after)||memcmp(&requests,&requests_after,sizeof requests)||
           !read_at(entry+24,group,8)||memcmp(group,runtime_archetypes.group,8))return 0;
    }
    return total==runtime_archetypes.count&&total>0&&read_at(loader+0x68,&after,sizeof after)&&
        !memcmp(&packages,&after,sizeof after);
}
static void runtime_archetype_release(uintptr_t base,uintptr_t engine) {
    if(!runtime_archetypes.active||runtime_archetypes.inflight)return;
    (void)engine;
    if(!guest_camera_stamp_matches(base,&runtime_archetypes.loader,1)){
        /* The owning loader no longer exists; never touch its replacement. */
        memset(&runtime_archetypes,0,sizeof runtime_archetypes);return;
    }
    uintptr_t loader=runtime_archetypes.loader.pointer;
    int present=runtime_asset_group_count(loader,runtime_archetypes.group);
    if(!present){memset(&runtime_archetypes,0,sizeof runtime_archetypes);return;}
    uintptr_t fn=runtime_asset_method(base,loader,"ClearAsyncGroup",0x20501,8,1,0xb73800);
    if(present<0||!runtime_archetype_group_owned(base,0,0)||!fn||read_ptr(read_ptr(loader)+0x230)!=base+0xf4f80)return;
    runtime_archetypes.inflight=1;
    ((ProcessEvent)(base+0xf4f80))((void*)loader,(void*)fn,runtime_archetypes.group,NULL);
    runtime_archetypes.inflight=0;
    if(guest_camera_stamp_matches(base,&runtime_archetypes.loader,1)&&!runtime_asset_group_count(loader,runtime_archetypes.group)){
        line("{\"runtime_archetype_retention\":\"released\",\"stock_loader\":true}\r\n");
        memset(&runtime_archetypes,0,sizeof runtime_archetypes);
    }
}
static int runtime_archetype_retain(uintptr_t base,uintptr_t engine,uintptr_t world) {
    if(runtime_archetypes.inflight)return 0;
    if(runtime_archetypes.active)return runtime_archetype_group_owned(base,engine,1);
    uintptr_t loader=read_ptr(engine+0xd90);
    if(!narrative_probe_object(base,engine,"SFXEngine")||!narrative_probe_object(base,loader,"SFXAsyncAssetLoader")||
       read_ptr(read_ptr(loader)+0x230)!=base+0xf4f80)return 0;
    uintptr_t fn=runtime_asset_method(base,loader,"AsyncLoadAssets",0x424501,40,3,0xb734b0);
    if(!fn||!runtime_asset_method(base,loader,"ClearAsyncGroup",0x20501,8,1,0xb73800))return 0;
    RuntimeArchetypeRetention next={0};
    if(!guest_camera_stamp_read(base,engine,&next.engine,1)||!guest_camera_stamp_read(base,loader,&next.loader,1)||
       !guest_camera_stamp_read(base,world,&next.world,1))return 0;
    GameArray list={0};uintptr_t *objects=identity_object_list(base,&list);uintptr_t package=0;int overflow=0;
    if(!objects)return 0;
    for(int i=0;i<list.count;++i){uintptr_t obj=objects[i];GuestCameraStamp stamp={0};
        if(!obj||!is_live(base,obj))continue;
        if(!read_ptr(obj+0x40)&&is_kind_of(base,obj,"Package")){
            char name[64];object_name(base,obj,name,sizeof name);if(!strcmp(name,"BioChar_Player")){
                if(package){overflow=1;break;}package=obj;
            }
        }
        if(!is_kind_of(base,obj,"SFXPawn_Player")||!guest_camera_stamp_read(base,obj,&stamp,1)||!runtime_archetype_path(stamp.full_name))continue;
        if(next.count==RUNTIME_ARCHETYPE_LIMIT){overflow=1;break;}
        next.assets[next.count++]=stamp;
    }
    HeapFree(GetProcessHeap(),0,objects);
    if(overflow||!package||!next.count||!read_at(package+0x48,next.group,8))return 0;
    /* Process-local FName suffix, admitted only if neither loader array uses it. */
    next.group[1]=0x4d330000u;
    for(int i=0;i<128;++i,++next.group[1]){
        int count=runtime_asset_group_count(loader,next.group);if(count<0)return 0;if(!count)break;if(i==127)return 0;
    }
    RuntimeAssetRequest requests[RUNTIME_ARCHETYPE_LIMIT]={0};wchar_t paths[RUNTIME_ARCHETYPE_LIMIT][640];
    for(int i=0;i<next.count;++i){
        if(!guest_camera_stamp_matches(base,&next.assets[i],1))return 0;
        int length=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,next.assets[i].full_name,-1,paths[i],640);
        if(length<=1)return 0;
        requests[i].path=(GameString){paths[i],length,length};requests[i].klass=next.assets[i].klass;
        requests[i].asset=next.assets[i].pointer;
    }
    struct {uint32_t group[2];GameArray assets;uintptr_t callback[2];} params={0};
    _Static_assert(sizeof params==40,"AsyncLoadAssets ABI");
    memcpy(params.group,next.group,8);params.assets=(GameArray){(uintptr_t)requests,next.count,next.count};
    if(!guest_camera_stamp_matches(base,&next.engine,1)||!guest_camera_stamp_matches(base,&next.loader,1)||
       !guest_camera_stamp_matches(base,&next.world,1)||read_ptr(engine+0xd90)!=loader||runtime_asset_group_count(loader,next.group)!=0)return 0;
    /* The retained native wrapper calls this exact four-argument method:
     * loader, group by value, request-array reference, delegate reference.
     * Passing the const/out array through external ProcessEvent did not
     * establish an owned group in the observed ordinary run. Invoke the same
     * stock method with an explicit array rather than fabricating a VM frame. */
    static const unsigned char load_body_pin[]={0x4c,0x89,0x44,0x24,0x18,0x48,0x89,0x54,
        0x24,0x10,0x48,0x89,0x4c,0x24,0x08,0x55,0x53,0x56,0x57,0x41,0x54,0x41,0x55,0x41};
    unsigned char actual[sizeof load_body_pin];
    if(!read_at(base+0xb03c10,actual,sizeof actual)||memcmp(actual,load_body_pin,sizeof actual))return 0;
    uint64_t group_value=0;memcpy(&group_value,params.group,8);
    typedef void (*LoadAssetsNative)(void*,uint64_t,const GameArray*,const void*);
    next.active=next.inflight=1;runtime_archetypes=next;
    ((LoadAssetsNative)(base+0xb03c10))((void*)loader,group_value,&params.assets,params.callback);
    runtime_archetypes.inflight=0;
    int retained=runtime_archetype_group_owned(base,engine,1);char row[220];
    snprintf(row,sizeof row,"{\"runtime_archetype_retention\":\"requested\",\"templates\":%d,\"references_verified\":%s,\"travel_verified\":false}\r\n",next.count,retained?"true":"false");line(row);
    return 0; /* Refresh controller/world state before travel on another Tick. */
}
/* Cleanup is based on explicit cancellation/disable or an observed return
 * from a connected client to an idle standalone world, never a time limit. */
static void runtime_archetype_retention_tick(uintptr_t base,uintptr_t engine,DWORD now) {
    static DWORD checked;
    if(!runtime_archetypes.active||runtime_archetypes.inflight)return;
    if(checked&&now-checked<250)return;checked=now;
    if(!runtime_archetype_loader_matches(base,engine)){runtime_archetype_release(base,engine);return;}
    int status=(int)InterlockedCompareExchange(&runtime_network_start_status,0,0);
    if(!runtime_control_enabled||g_runtime_config.role!=ME3_ROLE_GUEST||status==RUNTIME_START_CANCELLED||
       status==RUNTIME_START_ABI_REFUSED||status==RUNTIME_START_BIND_REFUSED){
        runtime_archetype_release(base,engine);return;
    }
    SessionContext c=session_context(engine);unsigned char net=255;
    if(!session_actor_live(base,c.world)||!read_at(c.world+0x7c8,&net,1))return;
    if(net==3)runtime_archetypes.connected=1;
    CombatContextChain chain;combat_context_read_chain(base,&chain);
    if(net==0&&runtime_archetypes.connected&&chain.info_valid&&chain.info==c.world&&
       runtime_join_travel_idle(chain.world,engine))runtime_archetype_release(base,engine);
}
#endif
