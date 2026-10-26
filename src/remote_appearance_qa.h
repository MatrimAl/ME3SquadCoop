#ifndef ME3_REMOTE_APPEARANCE_QA_H
#define ME3_REMOTE_APPEARANCE_QA_H
/* Scoped ordinary presentation repair; protected entry still requires opt-in.
 * The owned remote player had a null mesh asset despite a valid channel.
 * The normal zero-argument UpdateAppearance wrapper blocks for asset load.
 * It also validates IDs and updates personalization effects. Never call it
 * on the host, local pawn, a scene, or a customized/pending appearance.
 * No raw mesh, appearance-ID, actor flag or profile writes here. */
typedef struct RemoteAppearanceAttempt {
    int valid,apply_pending,metadata_valid;
    int appearance_ids[15];
    GuestCameraStamp pawn,world;
    uint32_t group[2];
} RemoteAppearanceAttempt;
static RemoteAppearanceAttempt remote_appearance_attempt;
static int remote_appearance_call_active;
static DWORD remote_appearance_scan_at;
static int remote_appearance_option(const wchar_t *name) {
    wchar_t text[8]={0};DWORD count=GetEnvironmentVariableW(name,text,8);
#ifdef ME3_REMOTE_APPEARANCE_ORDINARY
    if(!count)return 1;
#endif
    return count==1&&text[0]==L'1';
}
static int remote_appearance_attempt_matches(const RemoteAppearanceAttempt *attempt,
        const GuestCameraStamp *pawn,const GuestCameraStamp *world) {
    return attempt->valid&&guest_camera_stamp_core_equal(&attempt->pawn,pawn)&&
        guest_camera_stamp_core_equal(&attempt->world,world)&&
        !strcmp(attempt->pawn.full_name,pawn->full_name)&&
        !strcmp(attempt->world.full_name,world->full_name);
}
/* A stock request may normalize IDs. A retry must match the metadata captured
 * after that request, not the pre-request defaults or a later customization. */
static int remote_appearance_metadata_matches(const RemoteAppearanceAttempt *attempt,const int ids[15]) {
    return attempt->metadata_valid&&!memcmp(attempt->appearance_ids,ids,sizeof attempt->appearance_ids);
}
static int remote_appearance_group_matches(const RemoteAppearanceAttempt *attempt,const uint32_t group[2],int retry) {
    return (group[0]||group[1])&&(!retry||(attempt->apply_pending&&!memcmp(attempt->group,group,8)));
}
/* The native ready result may also accept an empty matching record. Require
 * at least one readable live reference in the same stable loader group. */
static int remote_appearance_group_assets_live(uintptr_t base,uintptr_t loader,const uint32_t group[2]) {
    GameArray packages={0},last={0};int matches=0,assets=0;
    if(!read_at(loader+0x68,&packages,sizeof packages)||!packages.data||packages.count<=0||
       packages.count>256||packages.max<packages.count||packages.max>4096)return 0;
    for(int i=0;i<packages.count;++i){
        uintptr_t entry=packages.data+(uintptr_t)i*32;uint32_t name[2]={0};GameArray requests={0},again={0};
        if(!read_at(entry+24,name,8))return 0;
        if(memcmp(name,group,8))continue;
        ++matches;
        if(!read_at(entry,&requests,sizeof requests)||requests.count<0||requests.count>256||
           requests.max<requests.count||requests.max>4096||(requests.count&&!requests.data))return 0;
        for(int j=0;j<requests.count;++j){uintptr_t asset=0;
            if(!read_at(requests.data+(uintptr_t)j*40+32,&asset,8)||!is_live(base,asset))return 0;
            ++assets;
        }
        if(!read_at(entry,&again,sizeof again)||memcmp(&requests,&again,sizeof requests)||
           !read_at(entry+24,name,8)||memcmp(name,group,8))return 0;
    }
    return matches>0&&assets>0&&read_at(loader+0x68,&last,sizeof last)&&!memcmp(&packages,&last,sizeof packages);
}
static int remote_appearance_case(int enabled,int guest,int ordinary,int distinct,
        int channel,int role,int mesh_missing,int default_ids,int pending,int female,int morph) {
    return enabled==1&&guest==1&&ordinary==1&&distinct==1&&channel==1&&role==1&&
        mesh_missing==1&&default_ids==1&&pending==0&&female==0&&morph==0;
}
static void remote_appearance_qa_tick(uintptr_t base,uintptr_t engine,uintptr_t pc,uintptr_t local,
        uintptr_t info,int ordinary,int enabled,DWORD now) {
    static int loaded,opted;
    if(!loaded){loaded=1;opted=remote_appearance_option(L"ME3_SQUAD_COOP_REMOTE_APPEARANCE_QA");}
    if(!opted||!enabled||!ordinary||remote_appearance_call_active||
#ifndef ME3_REMOTE_APPEARANCE_ORDINARY
       !has_marker(root)||test_action!=5||
#endif
       now-remote_appearance_scan_at<1000)return;
    remote_appearance_scan_at=now;
    unsigned char net=255;
    if(!guest_camera_actor_live(base,local)||!is_kind_of(base,local,"SFXPawn_Ashley")||
       read_ptr(local+0x374)!=pc||read_ptr(local+0x198)!=info||
       !read_at(info+0x7c8,&net,1)||net!=3)return;
    /* A scene or temporary channel loss does not authorize a second attempt
     * on the same player. A new player/world identity must pass all gates. */
    if(remote_appearance_attempt.valid&&!remote_appearance_attempt.apply_pending&&remote_appearance_attempt.world.pointer==info&&
       guest_camera_stamp_matches(base,&remote_appearance_attempt.world,1)&&
       guest_camera_stamp_matches(base,&remote_appearance_attempt.pawn,1)&&
       read_ptr(remote_appearance_attempt.pawn.pointer+0x198)==info&&
       network_actor_channel_id(base,remote_appearance_attempt.pawn.pointer)>=0)return;
    GameArray array={0};uintptr_t *objects=identity_object_list(base,&array),target=0;int found=0;
    if(!objects)return;
    for(int i=0;i<array.count;++i){uintptr_t p=objects[i];
        if(p&&p!=local&&guest_camera_actor_live(base,p)&&read_ptr(p+0x198)==info&&
           is_kind_of(base,p,"SFXPawn_Player")&&network_actor_channel_id(base,p)>=0){target=p;++found;}}
    HeapFree(GetProcessHeap(),0,objects);if(found!=1)return;
    uintptr_t mesh=read_ptr(target+0x3f4),asset=read_ptr(mesh+0x258);unsigned char role=255;
    if(is_live(base,asset)&&remote_appearance_attempt.valid&&remote_appearance_attempt.pawn.pointer==target&&
       remote_appearance_attempt.world.pointer==info&&guest_camera_stamp_matches(base,&remote_appearance_attempt.pawn,1)&&
       guest_camera_stamp_matches(base,&remote_appearance_attempt.world,1))remote_appearance_attempt.apply_pending=0;
    uint32_t flags=0,sex=0;int ids[15],defaults=1;
    if(!guest_camera_actor_live(base,target)||!narrative_probe_object(base,mesh,"SkeletalMeshComponent")||
       !read_at(target+0x26a,&role,1)||!read_at(target+0x1560,&flags,4)||
       !read_at(target+0x578,&sex,4)||!read_at(target+0x14dc,ids,sizeof ids))return;
    for(int i=0;i<15;++i)if(ids[i]!=(i==1||i==14?-1:0))defaults=0;
    int continuing=remote_appearance_attempt.valid&&remote_appearance_attempt.apply_pending&&
        remote_appearance_attempt.pawn.pointer==target&&remote_appearance_attempt.world.pointer==info&&
        guest_camera_stamp_matches(base,&remote_appearance_attempt.pawn,1)&&
        guest_camera_stamp_matches(base,&remote_appearance_attempt.world,1);
    if(continuing)defaults=remote_appearance_metadata_matches(&remote_appearance_attempt,ids);
    if(!remote_appearance_case(1,net==3,ordinary,target!=local,network_actor_channel_id(base,target)>=0,
        role,asset==0,defaults,(flags&0x1880)!=0,(sex&0x20)!=0,read_ptr(target+0xb40)!=0))return;
    GuestCameraStamp pawn_stamp,mesh_stamp,world_stamp,fn_stamp;
    uintptr_t fn=guest_player_context_find_method(base,read_ptr(target+0x50),"SFXPawn_Player","UpdateAppearance");
    GameArray code={0};
    if(!guest_camera_stamp_read(base,target,&pawn_stamp,1)||!guest_camera_stamp_read(base,mesh,&mesh_stamp,1)||
       !guest_camera_stamp_read(base,info,&world_stamp,1)||!guest_camera_stamp_read(base,fn,&fn_stamp,1)||
       strcmp(fn_stamp.full_name,"SFXGame.SFXPawn_Player.UpdateAppearance")||
       !guest_player_context_function_abi(base,fn,"SFXPawn_Player","UpdateAppearance",0x20102,0,65535,0,0xf52b0,0)||
       !read_at(fn+0x7c,&code,sizeof code)||code.count!=31||code.max<31||code.max>4096)return;
    uintptr_t pe=read_ptr(read_ptr(target)+0x230);
    if(pe!=base+0x40d450||!guest_camera_stamp_matches(base,&pawn_stamp,1)||
       !guest_camera_stamp_matches(base,&mesh_stamp,1)||!guest_camera_stamp_matches(base,&world_stamp,1)||
       !guest_camera_stamp_matches(base,&fn_stamp,1)||read_ptr(target+0x3f4)!=mesh||read_ptr(mesh+0x258))return;
    int retry=remote_appearance_attempt_matches(&remote_appearance_attempt,&pawn_stamp,&world_stamp);
    if(retry&&!remote_appearance_attempt.apply_pending)return;
    /* Call UpdateAppearance once per admitted identity. Only the original
     * asset group may subsequently be polled, with all admission gates rerun. */
    if(!retry){
        remote_appearance_attempt.valid=1;remote_appearance_attempt.apply_pending=0;
        remote_appearance_attempt.metadata_valid=0;
        remote_appearance_attempt.pawn=pawn_stamp;remote_appearance_attempt.world=world_stamp;
        memset(remote_appearance_attempt.group,0,sizeof remote_appearance_attempt.group);
    }
    remote_appearance_call_active=1;
    if(!retry){
        line("{\"remote_appearance_qa\":\"calling-normal-UpdateAppearance\",\"default_metadata_only\":true,\"full_host_appearance_verified\":false}\r\n");
        ((ProcessEvent)pe)((void*)target,(void*)fn,NULL,NULL);
    }
    int same=guest_camera_stamp_matches(base,&pawn_stamp,1)&&guest_camera_stamp_matches(base,&world_stamp,1);
    /* Capture the request before optional completion ABI discovery. Missing
     * methods or a not-yet-ready group must not mark this request complete. */
    if(!retry&&same&&guest_camera_stamp_matches(base,&mesh_stamp,1)&&read_ptr(target+0x3f4)==mesh){
        uint32_t group[2]={0};
        if(read_at(target+0x148c,group,8)&&(group[0]||group[1])&&
           read_at(target+0x14dc,remote_appearance_attempt.appearance_ids,sizeof remote_appearance_attempt.appearance_ids)){
            memcpy(remote_appearance_attempt.group,group,8);
            remote_appearance_attempt.metadata_valid=1;
            remote_appearance_attempt.apply_pending=1;
        }
    }
    uintptr_t loader=narrative_probe_object(base,engine,"SFXEngine")?read_ptr(engine+0xd90):0;
    if(same&&remote_appearance_option(L"ME3_SQUAD_COOP_REMOTE_APPEARANCE_APPLY_QA")&&
       read_ptr(read_ptr(target)+0x248)==base+0x70a250&&
       narrative_probe_object(base,loader,"SFXAsyncAssetLoader")&&read_ptr(engine+0xd90)==loader){
        /* Actor callspace70a250 returns skip for role<=1 without flags500.
         * The async completion bridge lacks those flags; ApplyAppearance
         * itself is simulated. Admit only a fully loaded existing group. */
        uintptr_t ready=guest_player_context_find_method(base,read_ptr(loader+0x50),"SFXAsyncAssetLoader","IsAsyncGroupLoaded");
        uintptr_t apply=guest_player_context_find_method(base,read_ptr(target+0x50),"SFXPawn_Player","ApplyAppearance");
        uint32_t ready_flags=0;uint16_t ready_size=0,ready_return=0,ready_index=65535;unsigned char ready_num=0;
        uintptr_t loader_pe=read_ptr(read_ptr(loader)+0x230);GuestCameraStamp ready_stamp,apply_stamp;
        int abi=guest_camera_stamp_read(base,ready,&ready_stamp,1)&&!strcmp(ready_stamp.full_name,"SFXGame.SFXAsyncAssetLoader.IsAsyncGroupLoaded")&&
            read_at(ready+0xe0,&ready_flags,4)&&ready_flags==0x20501&&read_at(ready+0xe6,&ready_size,2)&&ready_size==12&&
            read_at(ready+0xe8,&ready_return,2)&&ready_return==8&&read_at(ready+0xe4,&ready_index,2)&&ready_index==0&&
            read_at(ready+0xea,&ready_num,1)&&ready_num==2&&loader_pe==base+0xf4f80&&
            guest_camera_stamp_read(base,apply,&apply_stamp,1)&&!strcmp(apply_stamp.full_name,"SFXGame.SFXPawn_Player.ApplyAppearance")&&
            guest_player_context_function_abi(base,apply,"SFXPawn_Player","ApplyAppearance",0x40103,0,65535,0,0xf52b0,0);
        struct {uint32_t group[2],result;} loaded_group={{0},0};
        _Static_assert(sizeof loaded_group==12,"IsAsyncGroupLoaded parameter ABI");
        if(abi&&read_at(target+0x148c,loaded_group.group,8)&&
           remote_appearance_group_matches(&remote_appearance_attempt,loaded_group.group,1)){
            ((ProcessEvent)loader_pe)((void*)loader,(void*)ready,&loaded_group,NULL);
            uint32_t current_group[2]={0};int current_ids[15];
            if(loaded_group.result==1&&read_ptr(engine+0xd90)==loader&&guest_camera_stamp_matches(base,&pawn_stamp,1)&&guest_camera_stamp_matches(base,&world_stamp,1)&&
               guest_camera_stamp_matches(base,&mesh_stamp,1)&&read_ptr(target+0x3f4)==mesh&&!read_ptr(mesh+0x258)&&
               guest_camera_stamp_matches(base,&apply_stamp,1)&&guest_camera_stamp_matches(base,&ready_stamp,1)&&
               read_at(target+0x148c,current_group,8)&&!memcmp(current_group,loaded_group.group,8)&&
               read_at(target+0x14dc,current_ids,sizeof current_ids)&&
               remote_appearance_metadata_matches(&remote_appearance_attempt,current_ids)&&
               remote_appearance_group_assets_live(base,loader,loaded_group.group)&&
               network_actor_channel_id(base,target)>=0){
                line("{\"remote_appearance_qa\":\"calling-loaded-ApplyAppearance\",\"group_loaded\":true,\"role_written\":false}\r\n");
                ((ProcessEvent)pe)((void*)target,(void*)apply,NULL,NULL);
                same=guest_camera_stamp_matches(base,&pawn_stamp,1)&&guest_camera_stamp_matches(base,&world_stamp,1);
            }else line("{\"remote_appearance_qa\":\"apply-not-admitted\",\"reason\":\"group-not-loaded-or-identity-changed\"}\r\n");
        }else {line("{\"remote_appearance_qa\":\"apply-not-admitted\",\"reason\":\"loaded-group-ABI-or-identity\"}\r\n");}
    }
    uintptr_t after_mesh=same?read_ptr(target+0x3f4):0;
    uintptr_t after_asset=narrative_probe_object(base,after_mesh,"SkeletalMeshComponent")?read_ptr(after_mesh+0x258):0;
    /* Dispatch success alone does not establish model creation. Retain the
     * original group for a later attempt when ApplyAppearance produced none. */
    if(same&&is_live(base,after_asset))remote_appearance_attempt.apply_pending=0;
    char row[500];snprintf(row,sizeof row,
        "{\"remote_appearance_qa\":\"returned\",\"same_pawn_world\":%s,\"asset_live\":%s,\"full_host_appearance_verified\":false,\"monotonic_ms\":%lu}\r\n",
        same?"true":"false",is_live(base,after_asset)?"true":"false",now);line(row);
    remote_appearance_call_active=0;
}
static int remote_appearance_option_selftest(void) {
    wchar_t name[96];_snwprintf(name,96,L"ME3_SQUAD_COOP_APPEARANCE_OPTION_%lu",GetCurrentProcessId());
    SetLastError(0);
    if(GetEnvironmentVariableW(name,NULL,0)||GetLastError()!=ERROR_ENVVAR_NOT_FOUND)return 0;
#ifdef ME3_REMOTE_APPEARANCE_ORDINARY
    int ok=remote_appearance_option(name)==1;
#else
    int ok=remote_appearance_option(name)==0;
#endif
    ok=SetEnvironmentVariableW(name,L"0")&&ok&&!remote_appearance_option(name);
    ok=SetEnvironmentVariableW(name,L"1")&&ok&&remote_appearance_option(name);
    ok=SetEnvironmentVariableW(name,L"invalid")&&ok&&!remote_appearance_option(name);
    return SetEnvironmentVariableW(name,NULL)&&ok;
}
static int remote_appearance_case_selftest(void) {
    if(!remote_appearance_option_selftest())return 0;
    int admitted[]={1,1,1,1,1,1,1,1,0,0,0};
    if(!remote_appearance_case(1,1,1,1,1,1,1,1,0,0,0))return 0;
    for(unsigned i=0;i<11;++i){int changed[11];memcpy(changed,admitted,sizeof changed);changed[i]=i<8?0:1;
        if(remote_appearance_case(changed[0],changed[1],changed[2],changed[3],changed[4],changed[5],changed[6],changed[7],changed[8],changed[9],changed[10]))return 0;}
    GuestCameraStamp pawn={0},world={0};pawn.pointer=0x100;pawn.index=7;world.pointer=0x200;world.index=9;
    strcpy(pawn.full_name,"World.Player");strcpy(world.full_name,"World.Info");
    RemoteAppearanceAttempt attempt={0};attempt.pawn=pawn;attempt.world=world;
    if(remote_appearance_attempt_matches(&attempt,&pawn,&world))return 0;
    attempt.valid=1;if(!remote_appearance_attempt_matches(&attempt,&pawn,&world))return 0;
    GuestCameraStamp changed=pawn;changed.index++;
    if(remote_appearance_attempt_matches(&attempt,&changed,&world))return 0;
    changed=world;changed.index++;
    if(remote_appearance_attempt_matches(&attempt,&pawn,&changed))return 0;
    changed=pawn;strcpy(changed.full_name,"OtherWorld.Player");
    if(remote_appearance_attempt_matches(&attempt,&changed,&world))return 0;
    int ids[15]={0};
    if(remote_appearance_metadata_matches(&attempt,ids))return 0;
    attempt.metadata_valid=1;ids[14]=2;
    memcpy(attempt.appearance_ids,ids,sizeof ids);
    if(!remote_appearance_metadata_matches(&attempt,ids))return 0;
    ids[0]=1;if(remote_appearance_metadata_matches(&attempt,ids))return 0;
    uint32_t group[2]={123,0},other[2]={124,0},empty[2]={0,0};
    if(!remote_appearance_group_matches(&attempt,group,0)||remote_appearance_group_matches(&attempt,empty,0)||
       remote_appearance_group_matches(&attempt,group,1))return 0;
    attempt.apply_pending=1;memcpy(attempt.group,group,8);
    return remote_appearance_attempt_matches(&attempt,&pawn,&world)&&
        remote_appearance_group_matches(&attempt,group,1)&&
        !remote_appearance_group_matches(&attempt,other,1)&&!remote_appearance_group_matches(&attempt,empty,1);
}
#endif
