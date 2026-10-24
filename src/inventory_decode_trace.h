#ifndef ME3_INVENTORY_DECODE_TRACE_H
#define ME3_INVENTORY_DECODE_TRACE_H
/* Protected opt-in observer. Never reads or advances the archive itself. */
typedef int (*InventoryDecodeOriginal)(void*,void*,void*,void*);
typedef struct InventoryDecodeFrame {
    struct InventoryDecodeFrame *previous;
    PositionReceiveSnapshot before;
    uintptr_t channel;
    unsigned id;
} InventoryDecodeFrame;
static InventoryDecodeOriginal inventory_decode_original;
static InventoryDecodeFrame *inventory_decode_frame;
static unsigned inventory_decode_next,inventory_decode_rows;
static int inventory_decode_installed;
static unsigned inventory_decode_scoped_calls,inventory_decode_site_calls,inventory_decode_property_calls;
static unsigned inventory_decode_summaries;
static DWORD inventory_decode_summary_at;
static int inventory_decode_property(uintptr_t property) {
    uintptr_t base=initial_actor_trace_base;int32_t dim=0,size=0,offset=-1;uint64_t flags=0;char path[400];
    if(!is_live(base,property)||read_ptr(property)!=base+0x1077d88||
       !read_at(property+0x70,&dim,4)||dim!=1||!read_at(property+0x74,&size,4)||size!=8||
       !read_at(property+0x78,&flags,8)||flags!=UINT64_C(0x100000020)||
       !read_at(property+0x94,&offset,4)||offset!=0x414)return 0;
    identity_path(base,property,path,sizeof path);
    return !strcmp(path,"Engine.Pawn.InvManager");
}
static int inventory_decode_forward(void *property,void *archive,void *map,void *data,uintptr_t caller) {
    DWORD before_error=GetLastError();InventoryDecodeFrame *f=inventory_decode_frame;
    uintptr_t base=initial_actor_trace_base,before=0,after=0;uint32_t error_before=1,error_after=1;
    int scoped=inventory_decode_installed&&position_receive_opted()&&has_marker(root)&&
        GetCurrentThreadId()==initial_actor_trace_thread&&f&&f->before.valid;
    int property_match=0,site_match=0;
    if(scoped) {
        if(inventory_decode_scoped_calls!=UINT32_MAX)++inventory_decode_scoped_calls;
        site_match=caller==base+0x488ed4;
        if(site_match&&inventory_decode_site_calls!=UINT32_MAX)++inventory_decode_site_calls;
        property_match=inventory_decode_property((uintptr_t)property);
        if(property_match&&inventory_decode_property_calls!=UINT32_MAX)++inventory_decode_property_calls;
    }
    int capture=scoped&&inventory_decode_rows<64&&site_match&&property_match&&
        read_ptr(f->channel+0xa0)==f->before.pawn&&
        read_at((uintptr_t)data,&before,8)&&read_at((uintptr_t)archive+0x38,&error_before,4);
    SetLastError(before_error);
    int result=inventory_decode_original(property,archive,map,data);
    DWORD after_error=GetLastError();
    if(capture&&inventory_decode_frame==f&&
       read_at((uintptr_t)data,&after,8)&&read_at((uintptr_t)archive+0x38,&error_after,4)) {
        ++inventory_decode_rows;char row[1000];uintptr_t table=read_ptr((uintptr_t)map);
        snprintf(row,sizeof row,
          "{\"inventory_decode_trace\":\"field\",\"receive_id\":%u,\"pawn\":\"0x%llx\","
          "\"scratch_before\":\"0x%llx\",\"scratch_after\":\"0x%llx\",\"native_result\":%d,"
          "\"archive_error_before\":%u,\"archive_error_after\":%u,\"map_vtable_rva\":\"0x%llx\","
          "\"final_application_verified\":false,\"original_forwarded\":true,\"payload_modified\":false,\"monotonic_ms\":%lu}\r\n",
          f->id,(unsigned long long)f->before.pawn,(unsigned long long)before,(unsigned long long)after,
          result,error_before,error_after,(unsigned long long)(table>=base?table-base:0),GetTickCount());line(row);
    }
    SetLastError(after_error);return result;
}
static int inventory_decode_serialize(void *property,void *archive,void *map,void *data) {
    return inventory_decode_forward(property,archive,map,data,(uintptr_t)__builtin_return_address(0));
}
static void inventory_decode_begin(InventoryDecodeFrame *f,uintptr_t channel,PositionReceiveSnapshot before) {
    memset(f,0,sizeof *f);
    if(!inventory_decode_installed||GetCurrentThreadId()!=initial_actor_trace_thread)return;
    f->previous=inventory_decode_frame;f->channel=channel;f->before=before;
    f->id=++inventory_decode_next;inventory_decode_frame=f;
}
static void inventory_decode_end(InventoryDecodeFrame *f) {
    if(inventory_decode_frame!=f)return;
    if(f->before.valid&&inventory_decode_rows<64) {
        PositionReceiveSnapshot after=position_receive_snapshot_actor(f->channel,f->before.pawn);
        if(after.valid&&after.pawn==f->before.pawn&&after.index==f->before.index&&
           after.klass==f->before.klass&&after.world==f->before.world&&after.inventory!=f->before.inventory) {
            ++inventory_decode_rows;char row[500];snprintf(row,sizeof row,
              "{\"inventory_decode_trace\":\"receive-complete\",\"receive_id\":%u,\"inventory_before\":\"0x%llx\",\"inventory_after\":\"0x%llx\",\"monotonic_ms\":%lu}\r\n",
              f->id,(unsigned long long)f->before.inventory,(unsigned long long)after.inventory,GetTickCount());line(row);
        }
    }
    DWORD now=GetTickCount();
    if(f->before.valid&&inventory_decode_summaries<8&&
       (!inventory_decode_summary_at||now-inventory_decode_summary_at>=5000)) {
        inventory_decode_summary_at=now;++inventory_decode_summaries;char row[500];
        snprintf(row,sizeof row,
          "{\"inventory_decode_trace\":\"coverage\",\"scoped_serializer_calls\":%u,\"receive_site_calls\":%u,\"inventory_property_calls\":%u,\"monotonic_ms\":%lu}\r\n",
          inventory_decode_scoped_calls,inventory_decode_site_calls,inventory_decode_property_calls,now);line(row);
    }
    inventory_decode_frame=f->previous;
}
static void inventory_decode_install(uintptr_t base) {
    if(!position_receive_opted()||inventory_decode_installed||!progress_enabled||test_action!=5||strcmp(test_role,"client")||!has_marker(root))return;
    static const unsigned char pin[23]={0x49,0x8b,0,0x4d,0x8b,0xd0,0x4c,0x8b,0x81,0xd0,0,0,0,0x49,0x8b,0xca,0x48,0xff,0xa0,0xc8,2,0,0};
    unsigned char bytes[23];uintptr_t slot=base+0x1077d88+0x2f8,expected=base+0x146f60;DWORD old,ignored;
    if(!supported_image(base)||read_ptr(slot)!=expected||!read_at(expected,bytes,sizeof bytes)||memcmp(bytes,pin,sizeof pin)||
       !VirtualProtect((void*)slot,8,PAGE_READWRITE,&old))return;
    inventory_decode_original=(InventoryDecodeOriginal)expected;
    PVOID previous=InterlockedCompareExchangePointer((PVOID volatile*)slot,(PVOID)inventory_decode_serialize,(PVOID)expected);
    VirtualProtect((void*)slot,8,old,&ignored);
    if((uintptr_t)previous!=expected){inventory_decode_original=NULL;return;}
    inventory_decode_installed=1;
    line("{\"inventory_decode_trace\":\"installed\",\"limit\":64,\"observational\":true}\r\n");
}
#endif
