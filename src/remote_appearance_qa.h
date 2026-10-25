#ifndef ME3_REMOTE_APPEARANCE_QA_H
#define ME3_REMOTE_APPEARANCE_QA_H
/* Default-off protected experiment, not a general appearance synchronizer.
 * The owned remote player had a null mesh asset despite a valid channel.
 * The normal zero-argument UpdateAppearance wrapper blocks for asset load.
 * It also validates IDs and updates personalization effects. Never call it
 * on the host, local pawn, a scene, or a customized/pending appearance.
 * No raw mesh, appearance-ID, actor flag or profile writes here. */
static int remote_appearance_attempted;
static DWORD remote_appearance_scan_at;
static int remote_appearance_case(int enabled,int guest,int ordinary,int distinct,
        int channel,int role,int mesh_missing,int default_ids,int pending,int female,int morph) {
    return enabled==1&&guest==1&&ordinary==1&&distinct==1&&channel==1&&role==1&&
        mesh_missing==1&&default_ids==1&&pending==0&&female==0&&morph==0;
}
static void remote_appearance_qa_tick(uintptr_t base,uintptr_t engine,uintptr_t pc,uintptr_t local,
        uintptr_t info,int ordinary,int enabled,DWORD now) {
    static int loaded,opted;
    if(!loaded){wchar_t text[8]={0};loaded=1;opted=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_REMOTE_APPEARANCE_QA",text,8)==1&&text[0]==L'1';}
    if(!opted||!enabled||!ordinary||!has_marker(root)||test_action!=5||remote_appearance_attempted||now-remote_appearance_scan_at<1000)return;
    remote_appearance_scan_at=now;
    unsigned char net=255;
    if(!guest_camera_actor_live(base,local)||!is_kind_of(base,local,"SFXPawn_Ashley")||
       read_ptr(local+0x374)!=pc||read_ptr(local+0x198)!=info||
       !read_at(info+0x7c8,&net,1)||net!=3)return;
    GameArray array={0};uintptr_t *objects=identity_object_list(base,&array),target=0;int found=0;
    if(!objects)return;
    for(int i=0;i<array.count;++i){uintptr_t p=objects[i];
        if(p&&p!=local&&guest_camera_actor_live(base,p)&&read_ptr(p+0x198)==info&&
           is_kind_of(base,p,"SFXPawn_Player")&&network_actor_channel_id(base,p)>=0){target=p;++found;}}
    HeapFree(GetProcessHeap(),0,objects);if(found!=1)return;
    uintptr_t mesh=read_ptr(target+0x3f4),asset=read_ptr(mesh+0x258);unsigned char role=255;
    uint32_t flags=0,sex=0;int ids[15],defaults=1;
    if(!guest_camera_actor_live(base,target)||!narrative_probe_object(base,mesh,"SkeletalMeshComponent")||
       !read_at(target+0x26a,&role,1)||!read_at(target+0x1560,&flags,4)||
       !read_at(target+0x578,&sex,4)||!read_at(target+0x14dc,ids,sizeof ids))return;
    for(int i=0;i<15;++i)if(ids[i]!=(i==1||i==14?-1:0))defaults=0;
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
    /* One attempt in this process, even if loading fails. No retry loop. */
    remote_appearance_attempted=1;
    line("{\"remote_appearance_qa\":\"calling-normal-UpdateAppearance\",\"default_metadata_only\":true,\"full_host_appearance_verified\":false}\r\n");
    ((ProcessEvent)pe)((void*)target,(void*)fn,NULL,NULL);
    int same=guest_camera_stamp_matches(base,&pawn_stamp,1)&&guest_camera_stamp_matches(base,&world_stamp,1);
    uintptr_t loader=narrative_probe_object(base,engine,"SFXEngine")?read_ptr(engine+0xd90):0;
    wchar_t apply_flag[8]={0};
    if(same&&GetEnvironmentVariableW(L"ME3_SQUAD_COOP_REMOTE_APPEARANCE_APPLY_QA",apply_flag,8)==1&&apply_flag[0]==L'1'&&
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
        if(abi&&read_at(target+0x148c,loaded_group.group,8)){
            ((ProcessEvent)loader_pe)((void*)loader,(void*)ready,&loaded_group,NULL);
            if(loaded_group.result==1&&read_ptr(engine+0xd90)==loader&&guest_camera_stamp_matches(base,&pawn_stamp,1)&&guest_camera_stamp_matches(base,&world_stamp,1)&&
               guest_camera_stamp_matches(base,&apply_stamp,1)&&guest_camera_stamp_matches(base,&ready_stamp,1)){
                line("{\"remote_appearance_qa\":\"calling-loaded-ApplyAppearance\",\"group_loaded\":true,\"role_written\":false}\r\n");
                ((ProcessEvent)pe)((void*)target,(void*)apply,NULL,NULL);
                same=guest_camera_stamp_matches(base,&pawn_stamp,1)&&guest_camera_stamp_matches(base,&world_stamp,1);
            }else line("{\"remote_appearance_qa\":\"apply-not-admitted\",\"reason\":\"group-not-loaded-or-identity-changed\"}\r\n");
        }else line("{\"remote_appearance_qa\":\"apply-not-admitted\",\"reason\":\"loaded-group-ABI\"}\r\n");
    }
    uintptr_t after_mesh=same?read_ptr(target+0x3f4):0;
    uintptr_t after_asset=narrative_probe_object(base,after_mesh,"SkeletalMeshComponent")?read_ptr(after_mesh+0x258):0;
    char row[500];snprintf(row,sizeof row,
        "{\"remote_appearance_qa\":\"returned\",\"same_pawn_world\":%s,\"asset_live\":%s,\"full_host_appearance_verified\":false,\"monotonic_ms\":%lu}\r\n",
        same?"true":"false",is_live(base,after_asset)?"true":"false",now);line(row);
}
#endif
