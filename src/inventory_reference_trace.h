#ifndef ME3_INVENTORY_REFERENCE_TRACE_H
#define ME3_INVENTORY_REFERENCE_TRACE_H
/* Include after inventory_decode_property. Observe original outputs, never archive payload. */
typedef void (*InventoryReferenceBits)(void*,void*,int32_t);
typedef void (*InventoryReferenceIndex)(void*,uint32_t*,uint32_t);
typedef struct InventoryReferenceScope {
    struct InventoryReferenceScope *previous;
    InventoryDecodeFrame *receive;
    uintptr_t archive,map,pawn,requested_class;
    unsigned id;
    int active,tag_known;
    uint8_t tag;
} InventoryReferenceScope;
static InventoryReferenceScope *inventory_reference_scope;
static InventoryReferenceBits inventory_reference_bits_original;
static InventoryReferenceIndex inventory_reference_index_original;
static unsigned inventory_reference_rows;
static int inventory_reference_installed;
static int inventory_reference_current(InventoryReferenceScope *s,void *archive) {
    uintptr_t base=initial_actor_trace_base;
    return inventory_reference_installed&&s&&s==inventory_reference_scope&&s->active&&
        GetCurrentThreadId()==initial_actor_trace_thread&&s->receive==inventory_decode_frame&&
        s->receive&&s->receive->before.valid&&s->id==s->receive->id&&
        s->archive==(uintptr_t)archive&&read_ptr(s->archive)==base+0x119dda8&&
        read_ptr(s->map)==base+0x11a0d20&&s->pawn==s->receive->before.pawn&&
        read_ptr(s->receive->channel+0xa0)==s->pawn;
}
/* Always push, including rejected nested properties, to mask an outer valid scope. */
static void inventory_reference_begin(InventoryReferenceScope *s,InventoryDecodeFrame *f,
        void *property,void *archive,void *map,uintptr_t caller,int eligible) {
    DWORD error=GetLastError();memset(s,0,sizeof *s);s->previous=inventory_reference_scope;
    if(GetCurrentThreadId()!=initial_actor_trace_thread){SetLastError(error);return;}
    s->receive=f;s->archive=(uintptr_t)archive;s->map=(uintptr_t)map;
    if(f){s->id=f->id;s->pawn=f->before.pawn;}
    s->active=eligible&&caller==initial_actor_trace_base+0x488ed4&&
        inventory_decode_property((uintptr_t)property);
    if(s->active)s->requested_class=read_ptr((uintptr_t)property+0xd0);
    inventory_reference_scope=s;SetLastError(error);
}
static void inventory_reference_end(InventoryReferenceScope *s) {
    if(inventory_reference_scope==s)inventory_reference_scope=s->previous;
}
static void inventory_reference_record(InventoryReferenceScope *s,int index,uint32_t value) {
    if(inventory_reference_rows>=128)return;
    uintptr_t connection=0,channel=0,actor=0,klass=0,receive_connection=0,index_connection=0;int snapshot_valid=0;
    int receive_connection_valid=read_at(s->receive->channel+0x60,&receive_connection,8);
    int map_connection_valid=read_at(s->map+0x108,&connection,8);
    int index_connection_valid=0;
    uint32_t type=0,flags=0,error=0;
    if(index&&s->tag==1&&value>0&&value<0x3ff) {
        if(map_connection_valid&&connection&&
           read_at(connection+0xf84+(uintptr_t)value*8,&channel,8)) {
            snapshot_valid=1;
            if(channel){index_connection_valid=read_at(channel+0x60,&index_connection,8);
                snapshot_valid=read_at(channel+0x78,&type,4)&&read_at(channel+0x68,&flags,4)&&
                read_at(channel+0xa0,&actor,8);if(actor)snapshot_valid=snapshot_valid&&read_at(actor+0x50,&klass,8);}
        }
    }
    int error_valid=read_at(s->archive+0x38,&error,4);++inventory_reference_rows;char row[1600];
    snprintf(row,sizeof row,
      "{\"inventory_reference_trace\":\"%s\",\"receive_id\":%u,\"tag\":%u,\"index\":%u,"
      "\"channel_snapshot_valid\":%s,\"archive_error_read_valid\":%s,\"archive_error\":%u,\"channel\":\"0x%llx\",\"channel_type\":%u,\"channel_flags\":%u,"
      "\"actor\":\"0x%llx\",\"actor_class\":\"0x%llx\",\"requested_class\":\"0x%llx\","
      "\"map_connection_valid\":%s,\"map_connection\":\"0x%llx\",\"receive_connection_valid\":%s,\"receive_connection\":\"0x%llx\","
      "\"indexed_channel_connection_valid\":%s,\"indexed_channel_connection\":\"0x%llx\","
      "\"original_forwarded\":true,\"payload_modified\":false,\"monotonic_ms\":%lu}\r\n",
      index?"index":"tag",s->id,(unsigned)s->tag,value,snapshot_valid?"true":"false",error_valid?"true":"false",error,(unsigned long long)channel,
      type,flags,(unsigned long long)actor,(unsigned long long)klass,
      (unsigned long long)s->requested_class,map_connection_valid?"true":"false",(unsigned long long)connection,
      receive_connection_valid?"true":"false",(unsigned long long)receive_connection,
      index_connection_valid?"true":"false",(unsigned long long)index_connection,GetTickCount());line(row);
}
static void inventory_reference_bits_forward(void *archive,void *dest,int32_t count,uintptr_t caller) {
    DWORD before=GetLastError();InventoryReferenceScope *s=inventory_reference_scope;
    int capture=caller==initial_actor_trace_base+0x602c40&&count==1&&
        inventory_reference_current(s,archive);
    SetLastError(before);inventory_reference_bits_original(archive,dest,count);DWORD after=GetLastError();
    if(capture&&inventory_reference_current(s,archive)&&dest) {
        uint8_t tag=0;if(read_at((uintptr_t)dest,&tag,1)&&tag<=1){s->tag=tag;s->tag_known=1;
            inventory_reference_record(s,0,0);}
    }
    SetLastError(after);
}
static void inventory_reference_index_forward(void *archive,uint32_t *dest,uint32_t maximum,uintptr_t caller) {
    DWORD before=GetLastError();InventoryReferenceScope *s=inventory_reference_scope;
    int capture=inventory_reference_current(s,archive)&&s->tag_known&&
        ((s->tag==1&&maximum==0x3ff&&caller==initial_actor_trace_base+0x602c5d)||
         (s->tag==0&&maximum==UINT32_C(0x80000000)&&caller==initial_actor_trace_base+0x602cc3));
    SetLastError(before);inventory_reference_index_original(archive,dest,maximum);DWORD after=GetLastError();
    if(capture&&inventory_reference_current(s,archive)&&dest) {
        uint32_t index=0;if(read_at((uintptr_t)dest,&index,4))inventory_reference_record(s,1,index);
    }
    SetLastError(after);
}
static void inventory_reference_bits(void *archive,void *dest,int32_t count) {
    inventory_reference_bits_forward(archive,dest,count,(uintptr_t)__builtin_return_address(0));
}
static void inventory_reference_index(void *archive,uint32_t *dest,uint32_t maximum) {
    inventory_reference_index_forward(archive,dest,maximum,(uintptr_t)__builtin_return_address(0));
}
/* Both slots share one page. On second CAS failure roll back only our first slot. */
static void inventory_reference_install(uintptr_t base) {
    if(inventory_reference_installed||!inventory_decode_installed||!position_receive_opted()||
       !progress_enabled||test_action!=5||strcmp(test_role,"client")||!has_marker(root)||
       GetCurrentThreadId()!=initial_actor_trace_thread)return;
    static const unsigned char bits_pin[16]={0x48,0x89,0x5c,0x24,8,0x48,0x89,0x74,0x24,0x10,0x57,0x48,0x83,0xec,0x30,0x41};
    static const unsigned char index_pin[16]={0x48,0x89,0x5c,0x24,0x10,0x57,0x45,0x33,0xc9,0x41,0xbb,1,0,0,0,0x44};
    unsigned char bytes[16];uintptr_t slots=base+0x119ddb8;DWORD old,ignored;
    if(!supported_image(base)||read_ptr(slots)!=base+0xc2b00||read_ptr(slots+8)!=base+0xc3060||
       !read_at(base+0xc2b00,bytes,16)||memcmp(bytes,bits_pin,16)||
       !read_at(base+0xc3060,bytes,16)||memcmp(bytes,index_pin,16)||
       !VirtualProtect((void*)slots,16,PAGE_READWRITE,&old))return;
    inventory_reference_bits_original=(InventoryReferenceBits)(base+0xc2b00);
    inventory_reference_index_original=(InventoryReferenceIndex)(base+0xc3060);
    PVOID first=InterlockedCompareExchangePointer((PVOID volatile*)slots,(PVOID)inventory_reference_bits,(PVOID)(base+0xc2b00));
    if((uintptr_t)first==base+0xc2b00) {
        PVOID second=InterlockedCompareExchangePointer((PVOID volatile*)(slots+8),(PVOID)inventory_reference_index,(PVOID)(base+0xc3060));
        if((uintptr_t)second==base+0xc3060)inventory_reference_installed=1;
        else InterlockedCompareExchangePointer((PVOID volatile*)slots,(PVOID)(base+0xc2b00),(PVOID)inventory_reference_bits);
    }
    VirtualProtect((void*)slots,16,old,&ignored);
    if(inventory_reference_installed)line("{\"inventory_reference_trace\":\"installed\",\"limit\":128,\"observational\":true}\r\n");
}
#endif
