#ifndef ME3_RUNTIME_HOST_PACKAGE_PRELOAD_H
#define ME3_RUNTIME_HOST_PACKAGE_PRELOAD_H
/* Focused default-off candidate for the observed resident morph GUID mismatch.
 * Uses the shipped public static SFXGame.LoadPackage API. Never writes GUIDs,
 * package maps, files or validation flags. This is not a general preload list. */
static int runtime_host_package_preload_attempted,runtime_host_package_preload_dispatched;
static int runtime_host_package_preload_opted(void) {
    wchar_t flag[2]={0};
    return GetEnvironmentVariableW(L"ME3_SQUAD_COOP_HOST_PACKAGE_PRELOAD",flag,2)==1&&flag[0]==L'1';
}
static uintptr_t runtime_host_morph_package(uintptr_t base) {
    GameArray array={0};uintptr_t *objects=identity_object_list(base,&array),found=0;
    if(!objects)return 0;
    for(int i=0;i<array.count;++i) {
        uintptr_t object=objects[i];
        if(!object||read_ptr(object+0x40)||!is_live(base,object)||!is_kind_of(base,object,"Package"))continue;
        char name[96];object_name(base,object,name,sizeof name);
        if(!strcmp(name,"BIOG_HMF_HED_PROMorph_R")) {
            if(found){found=0;break;}found=object;
        }
    }
    HeapFree(GetProcessHeap(),0,objects);return found;
}
static int runtime_host_package_preload_abi(uintptr_t base,uintptr_t fn) {
    uint32_t flags=0;uint16_t bytes=0,result=0,native_index=0;unsigned char count=0;
    uintptr_t native=0,property=read_ptr(fn+0x70);char name[96];
    if(!is_live(base,fn)||!is_kind_of(base,fn,"Function")||!is_live(base,property)||
       !is_kind_of(base,property,"StrProperty")||read_ptr(property+0x40)!=fn||read_ptr(property+0x68))return 0;
    object_name(base,property,name,sizeof name);
    int dim=0,size=0,offset=-1;uint64_t property_flags=0;
    return !strcmp(name,"PackageName")&&
        read_at(fn+0xe0,&flags,4)&&flags==0x22401&&read_at(fn+0xe4,&native_index,2)&&!native_index&&
        read_at(fn+0xe6,&bytes,2)&&bytes==sizeof(GameString)&&read_at(fn+0xe8,&result,2)&&result==65535&&
        read_at(fn+0xea,&count,1)&&count==1&&read_at(fn+0xd8,&native,8)&&native>=base&&native<base+0x1300000&&
        read_at(property+0x70,&dim,4)&&dim==1&&read_at(property+0x74,&size,4)&&size==16&&
        read_at(property+0x94,&offset,4)&&!offset&&read_at(property+0x78,&property_flags,8)&&property_flags==UINT64_C(0x400080);
}
static int runtime_host_package_preload(uintptr_t base,SessionContext context) {
    if(!runtime_host_package_preload_opted())return 1;
    if(runtime_host_package_preload_attempted)return runtime_host_package_preload_dispatched;
    uintptr_t game=read_ptr(context.world+0x6b0),fn=function_named(base,"SFXGame","LoadPackage");
    if(!session_actor_live(base,game)||!is_kind_of(base,game,"SFXGame")||
       read_ptr(game+0x198)!=context.world||!runtime_host_package_preload_abi(base,fn)) {
        static int logged;if(!logged++)line("{\"runtime_network\":\"host-package-preload-wait\",\"reason\":\"game-or-native-api-contract\"}\r\n");
        return 0;
    }
    uintptr_t before_package=runtime_host_morph_package(base);uint32_t before[4]={0},after[4]={0};
    if(!before_package||!read_at(before_package+0x6c,before,16))return 0;
    wchar_t name[]=L"BIOG_HMF_HED_PROMorph_R";
    GameString params={name,(int32_t)wcslen(name)+1,(int32_t)wcslen(name)+1};
    uintptr_t native_before=read_ptr(fn+0xd8);
    runtime_host_package_preload_attempted=1; /* Native calls may re-enter. */
    runtime_host_package_preload_dispatched=invoke(base,game,"SFXGame","LoadPackage",&params);
    uintptr_t after_package=runtime_host_morph_package(base);
    int readable=after_package&&read_at(after_package+0x6c,after,16);char row[540];
    snprintf(row,sizeof row,"{\"runtime_network\":\"host-package-preload\",\"package\":\"BIOG_HMF_HED_PROMorph_R\",\"native_api_dispatched\":%s,\"guid_before\":\"%08x%08x%08x%08x\",\"guid_after\":\"%08x%08x%08x%08x\",\"after_readable\":%s,\"native_function_rva\":\"0x%llx\",\"guid_written_by_mod\":false,\"connection_verified\":false}\r\n",runtime_host_package_preload_dispatched?"true":"false",before[0],before[1],before[2],before[3],after[0],after[1],after[2],after[3],readable?"true":"false",(unsigned long long)(native_before-base));line(row);
    return 0; /* Refresh player/world context on the following native Tick. */
}
#endif
