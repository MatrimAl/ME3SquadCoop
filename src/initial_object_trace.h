#ifndef ME3_INITIAL_OBJECT_TRACE_H
#define ME3_INITIAL_OBJECT_TRACE_H
/* Protected, default-off observation of the first UObject decoded by an
 * initial ActorChannel bunch. Exact LE3 FInBunch virtual30 ->464910 takes
 * (archive,UObject**), forwards to PackageMap virtual2c8, returns archive.
 * Never replay a decoder, change an output, or repair channel fields. */
typedef void *(*InitialObjectOriginal)(void*,void*);
typedef struct InitialObjectFrame {
    struct InitialObjectFrame *previous;
    uintptr_t channel,archive;unsigned event;int index,active;
    unsigned tag,index_value;int tag_known,index_known;
} InitialObjectFrame;
static InitialObjectFrame *initial_object_frame;
static InitialObjectFrame *initial_object_decoding;
static InitialObjectOriginal initial_object_original;
static int initial_object_installed;
static unsigned initial_object_rows;
static int initial_object_opted(void) {
    static int loaded,value;
    if(!loaded){wchar_t text[8]={0};loaded=1;value=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_INITIAL_OBJECT_TRACE",text,8)==1&&text[0]==L'1';}
    return value;
}
static int initial_object_scalar_current(void *archive) {
    InitialObjectFrame *f=initial_object_decoding;
    return initial_object_installed&&GetCurrentThreadId()==initial_actor_trace_thread&&
        f&&f==initial_object_frame&&f->active&&f->archive==(uintptr_t)archive&&
        read_ptr((uintptr_t)archive)==initial_actor_trace_base+0x119dda8;
}
static void initial_object_bits_observe(void *archive,void *dest,int32_t count,uintptr_t caller) {
    if(caller!=initial_actor_trace_base+0x602c40||count!=1||!initial_object_scalar_current(archive))return;
    uint8_t tag=0;if(read_at((uintptr_t)dest,&tag,1)&&tag<=1){
        initial_object_decoding->tag=tag;initial_object_decoding->tag_known=1;
    }
}
static void initial_object_index_observe(void *archive,uint32_t *dest,uint32_t maximum,uintptr_t caller) {
    if(!initial_object_scalar_current(archive)||!initial_object_decoding->tag_known)return;
    InitialObjectFrame *f=initial_object_decoding;
    if(!((f->tag==1&&maximum==0x3ff&&caller==initial_actor_trace_base+0x602c5d)||
         (f->tag==0&&maximum==UINT32_C(0x80000000)&&caller==initial_actor_trace_base+0x602cc3)))return;
    uint32_t index=0;if(read_at((uintptr_t)dest,&index,4)){f->index_value=index;f->index_known=1;}
}
static void *initial_object_forward(void *archive,void *output,uintptr_t caller) {
    DWORD before=GetLastError();int same_thread=GetCurrentThreadId()==initial_actor_trace_thread;
    InitialObjectFrame *f=same_thread?initial_object_frame:NULL;
    uintptr_t base=initial_actor_trace_base;
    int capture=initial_object_installed&&same_thread&&
        f&&f->active&&initial_object_rows<128&&caller==base+0x488771&&
        f->archive==(uintptr_t)archive&&read_ptr((uintptr_t)archive)==base+0x119dda8;
    InitialObjectFrame *previous=initial_object_decoding;
    if(same_thread)initial_object_decoding=capture?f:NULL;
    SetLastError(before);void *result=initial_object_original(archive,output);DWORD after=GetLastError();
    if(same_thread)initial_object_decoding=previous;
    if(capture&&initial_object_frame==f){
        uintptr_t object=0;int valid=read_at((uintptr_t)output,&object,8);
        int live=valid&&is_live(base,object),actor=live&&is_kind_of(base,object,"Actor");
        char path[700]={0},quoted[1400],kind[96]={0};uint32_t flags=0;
        if(live){identity_path(base,object,path,sizeof path);object_name(base,read_ptr(object+0x50),kind,sizeof kind);read_at(object+0xc,&flags,4);}
        identity_quote(path,quoted,sizeof quoted);++initial_object_rows;char row[2200];
        snprintf(row,sizeof row,"{\"initial_object_trace\":\"decoded\",\"receive_event\":%u,\"channel_index\":%d,\"output_read_valid\":%s,\"object\":\"0x%llx\",\"live\":%s,\"is_actor\":%s,\"class\":\"%s\",\"path\":\"%s\",\"object_flags\":%u,\"archive_return_matches\":%s,\"original_forwarded\":true,\"payload_modified\":false,\"monotonic_ms\":%lu}\r\n",
            f->event,f->index,valid?"true":"false",(unsigned long long)object,live?"true":"false",actor?"true":"false",kind,quoted,flags,result==archive?"true":"false",GetTickCount());line(row);
        snprintf(row,sizeof row,"{\"initial_object_trace\":\"reference-inputs\",\"receive_event\":%u,\"channel_index\":%d,\"tag_known\":%s,\"tag\":%u,\"index_known\":%s,\"index\":%u,\"scalar_hooks_installed\":%s,\"output_null\":%s,\"payload_modified\":false,\"monotonic_ms\":%lu}\r\n",
            f->event,f->index,f->tag_known?"true":"false",f->tag,f->index_known?"true":"false",f->index_value,
            inventory_reference_installed?"true":"false",valid&&!object?"true":"false",GetTickCount());line(row);
    }
    SetLastError(after);return result;
}
static void *initial_object_read(void *archive,void *output) {
    return initial_object_forward(archive,output,(uintptr_t)__builtin_return_address(0));
}
static void initial_object_install(uintptr_t base) {
    if(initial_object_installed||!initial_object_opted()||!initial_actor_trace_installed||
       test_action!=5||strcmp(test_role,"client")||!progress_enabled||!has_marker(root)||
       GetCurrentThreadId()!=initial_actor_trace_thread)return;
    static const unsigned char pin[20]={0x48,0x89,0x5c,0x24,8,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57,0x48,0x83,0xec,0x20};
    uintptr_t slot=base+0x119dda8+0x30,expected=base+0x464910;unsigned char bytes[20];DWORD old,ignored;
    if(!supported_image(base)||read_ptr(slot)!=expected||!read_at(expected,bytes,20)||memcmp(bytes,pin,20)||
       !VirtualProtect((void*)slot,8,PAGE_READWRITE,&old))return;
    initial_object_original=(InitialObjectOriginal)expected;
    PVOID previous=InterlockedCompareExchangePointer((PVOID volatile*)slot,(PVOID)initial_object_read,(PVOID)expected);
    VirtualProtect((void*)slot,8,old,&ignored);
    if((uintptr_t)previous!=expected){initial_object_original=NULL;return;}
    initial_reference_bits_observer=initial_object_bits_observe;
    initial_reference_index_observer=initial_object_index_observe;
    initial_object_installed=1;line("{\"initial_object_trace\":\"installed\",\"limit\":128,\"output_only\":true}\r\n");
}
static unsigned initial_object_test_calls;
static void *initial_object_test_original(void *archive,void *output) {
    ++initial_object_test_calls;*(uintptr_t*)output=0x456;SetLastError(1234);return archive;
}
static int initial_object_selftest(void) {
    InitialObjectOriginal saved=initial_object_original;InitialObjectFrame *frame=initial_object_frame;
    DWORD error=GetLastError();uintptr_t output=0;initial_object_test_calls=0;initial_object_original=initial_object_test_original;
    InitialObjectFrame inactive={0};initial_object_frame=&inactive;
    void *result=initial_object_forward((void*)0x123,&output,0);
    int pass=result==(void*)0x123&&output==0x456&&initial_object_test_calls==1&&GetLastError()==1234;
    uintptr_t saved_base=initial_actor_trace_base;DWORD saved_thread=initial_actor_trace_thread;
    int saved_installed=initial_object_installed;InitialObjectFrame *saved_decoding=initial_object_decoding;
    unsigned char archive[256]={0};uintptr_t base=0x100000,table=base+0x119dda8;memcpy(archive,&table,8);
    InitialObjectFrame active={0};active.active=1;active.archive=(uintptr_t)archive;
    initial_actor_trace_base=base;initial_actor_trace_thread=GetCurrentThreadId();initial_object_installed=1;
    initial_object_frame=initial_object_decoding=&active;
    uint8_t tag=1;uint32_t index=37;
    initial_object_bits_observe(archive,&tag,2,base+0x602c40);pass=pass&&!active.tag_known;
    initial_object_bits_observe(archive,&tag,1,base+0x602c41);pass=pass&&!active.tag_known;
    initial_object_bits_observe(archive,&tag,1,base+0x602c40);pass=pass&&active.tag_known&&active.tag==1;
    initial_object_index_observe(archive,&index,0x80000000,base+0x602cc3);pass=pass&&!active.index_known;
    initial_object_index_observe(archive,&index,0x3ff,base+0x602c5d);pass=pass&&active.index_known&&active.index_value==37;
    active.index_known=0;tag=0;initial_object_bits_observe(archive,&tag,1,base+0x602c40);
    initial_object_index_observe(archive,&index,0x80000000,base+0x602cc3);pass=pass&&active.index_known;
    active.tag_known=0;initial_object_frame=&inactive;
    initial_object_bits_observe(archive,&tag,1,base+0x602c40);pass=pass&&!active.tag_known;
    initial_object_frame=&active;active.active=0;
    initial_object_bits_observe(archive,&tag,1,base+0x602c40);pass=pass&&!active.tag_known;
    initial_object_original=saved;initial_object_frame=frame;initial_object_decoding=saved_decoding;
    initial_actor_trace_base=saved_base;initial_actor_trace_thread=saved_thread;initial_object_installed=saved_installed;
    SetLastError(error);return pass;
}
#endif
