#ifndef ME3_INVENTORY_SEND_REFERENCE_TRACE_H
#define ME3_INVENTORY_SEND_REFERENCE_TRACE_H
/* Protected research observer: caller scalars only, never archive payload. */
typedef void (*InventorySendBits)(void*,void*,int32_t);
typedef void (*InventorySendIndex)(void*,uint32_t*,uint32_t);
typedef struct InventorySendScope {
    struct InventorySendScope *previous;
    uintptr_t archive,map,data,pawn,value,connection;
    unsigned id,channel_index;
    int active,tag_known,branch;
    uint8_t tag;
} InventorySendScope;
static InventorySendScope *inventory_send_scope;
static InventorySendBits inventory_send_bits_original;
static InventorySendIndex inventory_send_index_original;
static unsigned inventory_send_reference_rows,inventory_send_reference_next;
static int inventory_send_reference_installed;
static int inventory_send_reference_current(InventorySendScope *s,void *archive) {
    uintptr_t base=initial_actor_trace_base;
    return s&&s==inventory_send_scope&&s->active&&inventory_send_reference_installed&&
        GetCurrentThreadId()==initial_actor_trace_thread&&s->archive==(uintptr_t)archive&&
        read_ptr(s->archive)==base+0x119dcc0&&read_ptr(s->map)==base+0x11a0d20&&
        read_ptr(s->map+0x108)==s->connection&&read_ptr(s->data)==s->value;
}
static void inventory_send_reference_begin(InventorySendScope *s,void *property,void *archive,
        void *map,void *data,uintptr_t caller) {
    DWORD saved=GetLastError();memset(s,0,sizeof *s);
    if(GetCurrentThreadId()!=initial_actor_trace_thread){SetLastError(saved);return;}
    s->previous=inventory_send_scope;inventory_send_scope=s;
    uintptr_t base=initial_actor_trace_base,world=0,level=0,info=0;GameArray actors={0};uint32_t loading=1;
    if(!inventory_send_reference_installed||!inventory_decode_installed||!progress_enabled||
       test_action!=6||strcmp(test_role,"host")||!has_marker(root)||caller!=base+0x48d433||
       !inventory_decode_property((uintptr_t)property)||(uintptr_t)data<0x414)goto done;
    s->archive=(uintptr_t)archive;s->map=(uintptr_t)map;s->data=(uintptr_t)data;s->pawn=s->data-0x414;
    if(!is_live(base,s->pawn)||!is_kind_of(base,s->pawn,"SFXPawn_Ashley")||!is_kind_of(base,s->pawn,"Pawn")||
       read_ptr(s->archive)!=base+0x119dcc0||read_ptr(s->map)!=base+0x11a0d20||
       !read_at(s->archive+0x14,&loading,4)||loading||!read_at(s->data,&s->value,8)||
       (s->value&&!is_live(base,s->value)))goto done;
    world=read_ptr(base+0x18b6638);level=read_ptr(world+0x88);
    if(!is_live(base,world)||!is_live(base,level)||!read_at(level+0x60,&actors,sizeof actors)||
       actors.count<1||actors.count>100000||actors.max<actors.count||actors.max>1000000||
       !(info=read_ptr(actors.data))||!is_live(base,info)||read_ptr(s->pawn+0x198)!=info||
       !(s->connection=read_ptr(s->map+0x108)))goto done;
    unsigned matches=0;
    for(unsigned i=1;i<0x3ff;++i){uintptr_t c=read_ptr(s->connection+0xf84+(uintptr_t)i*8);uint32_t type=0;
        if(c&&read_at(c+0x78,&type,4)&&type==2&&s->value&&read_ptr(c+0xa0)==s->value){++matches;s->channel_index=i;}}
    if(matches!=1)s->channel_index=0; /* metadata hint only, never used as encoded index */
    s->id=++inventory_send_reference_next;s->active=1;
 done:SetLastError(saved);
}
static void inventory_send_reference_end(InventorySendScope *s) {
    if(inventory_send_scope==s)inventory_send_scope=s->previous;
}
static void inventory_send_reference_record(InventorySendScope *s,int index,uint32_t before,
        uint32_t after,int before_valid,int after_valid) {
    if(inventory_send_reference_rows>=128)return;
    uint32_t error=0;int valid=read_at(s->archive+0x38,&error,4);char row[1200];++inventory_send_reference_rows;
    snprintf(row,sizeof row,
      "{\"inventory_send_reference\":\"%s\",\"property_id\":%u,\"branch\":%d,\"tag\":%u,"
      "\"value_before_valid\":%s,\"value_after_valid\":%s,\"scalar_before\":%u,\"scalar_after\":%u,"
      "\"pawn\":\"0x%llx\",\"inventory\":\"0x%llx\",\"map_connection\":\"0x%llx\","
      "\"metadata_channel_index\":%u,\"archive_error_read_valid\":%s,\"archive_error\":%u,"
      "\"original_forwarded\":true,\"payload_modified\":false,\"monotonic_ms\":%lu}\r\n",
      index?"index":"tag",s->id,s->branch,(unsigned)s->tag,before_valid?"true":"false",
      after_valid?"true":"false",before,after,(unsigned long long)s->pawn,
      (unsigned long long)s->value,(unsigned long long)s->connection,s->channel_index,
      valid?"true":"false",error,GetTickCount());line(row);
}
static int inventory_send_tag_branch(uintptr_t caller) {
    uintptr_t base=initial_actor_trace_base;
    if(caller==base+0x602ed2)return 1; /* dynamic actor */
    if(caller==base+0x602e8d)return 2; /* relevance denied */
    if(caller==base+0x602f57)return 3; /* package index */
    if(caller==base+0x602f76)return 4; /* null or package lookup failure */
    return 0;
}
static void inventory_send_bits_forward(void *archive,void *source,int32_t count,uintptr_t caller) {
    DWORD saved=GetLastError();InventorySendScope *s=inventory_send_scope;
    int branch=inventory_send_tag_branch(caller),capture=branch&&count==1&&inventory_send_reference_current(s,archive);
    if(capture)s->tag_known=0;
    uint8_t before=0;int bv=capture&&read_at((uintptr_t)source,&before,1);
    SetLastError(saved);inventory_send_bits_original(archive,source,count);DWORD after_error=GetLastError();
    if(capture&&inventory_send_reference_current(s,archive)){
        uint8_t after=0;int av=read_at((uintptr_t)source,&after,1);
        if(av&&after<=1&&((branch==3&&after==0)||(branch!=3&&after==1))){
            s->tag=after;s->tag_known=1;s->branch=branch;inventory_send_reference_record(s,0,before,after,bv,av);}}
    SetLastError(after_error);
}
static void inventory_send_index_forward(void *archive,uint32_t *source,uint32_t maximum,uintptr_t caller) {
    DWORD saved=GetLastError();InventorySendScope *s=inventory_send_scope;uintptr_t base=initial_actor_trace_base;
    int capture=inventory_send_reference_current(s,archive)&&s->tag_known&&
      ((s->branch==1&&maximum==0x3ff&&caller==base+0x602f12)||
       (s->branch==2&&maximum==0x3ff&&caller==base+0x602ea0)||
       (s->branch==3&&maximum==UINT32_C(0x80000000)&&caller==base+0x602f89)||
       (s->branch==4&&maximum==0x3ff&&caller==base+0x602f89));
    uint32_t before=0;int bv=capture&&read_at((uintptr_t)source,&before,4);
    SetLastError(saved);inventory_send_index_original(archive,source,maximum);DWORD after_error=GetLastError();
    if(capture&&inventory_send_reference_current(s,archive)){
        uint32_t after=0;int av=read_at((uintptr_t)source,&after,4);
        inventory_send_reference_record(s,1,before,after,bv,av);s->tag_known=0;}
    if(vector_decode_index_observer)vector_decode_index_observer(archive,source,maximum,caller);
    SetLastError(after_error);
}
static void inventory_send_bits(void *archive,void *source,int32_t count) {
    inventory_send_bits_forward(archive,source,count,(uintptr_t)__builtin_return_address(0));
}
static void inventory_send_index(void *archive,uint32_t *source,uint32_t maximum) {
    inventory_send_index_forward(archive,source,maximum,(uintptr_t)__builtin_return_address(0));
}
/* Same guarded two-slot transaction as inbound observer, different exact table/functions. */
static void inventory_send_reference_install(uintptr_t base) {
    if(inventory_send_reference_installed||!inventory_decode_installed||!progress_enabled||test_action!=6||
       strcmp(test_role,"host")||!has_marker(root)||GetCurrentThreadId()!=initial_actor_trace_thread)return;
    static const unsigned char bits[16]={0x48,0x89,0x5c,0x24,8,0x57,0x48,0x83,0xec,0x30,0x41,0x8b,0xf8,0x48,0x8b,0xd9};
    static const unsigned char index[16]={0x40,0x57,0x45,0x8b,0xd0,0x48,0x8b,0xfa,0x4c,0x8b,0xd9,0x44,0x39,2,0x76,3};
    uintptr_t slots=base+0x119dcd0;unsigned char bytes[16];DWORD old,ignored;
    if(!supported_image(base)||read_ptr(slots)!=base+0xc2bd0||read_ptr(slots+8)!=base+0xc3110||
       !read_at(base+0xc2bd0,bytes,16)||memcmp(bytes,bits,16)||
       !read_at(base+0xc3110,bytes,16)||memcmp(bytes,index,16)||!VirtualProtect((void*)slots,16,PAGE_READWRITE,&old))return;
    inventory_send_bits_original=(InventorySendBits)(base+0xc2bd0);inventory_send_index_original=(InventorySendIndex)(base+0xc3110);
    PVOID first=InterlockedCompareExchangePointer((PVOID volatile*)slots,(PVOID)inventory_send_bits,(PVOID)(base+0xc2bd0));
    if((uintptr_t)first==base+0xc2bd0){
        PVOID second=InterlockedCompareExchangePointer((PVOID volatile*)(slots+8),(PVOID)inventory_send_index,(PVOID)(base+0xc3110));
        if((uintptr_t)second==base+0xc3110)inventory_send_reference_installed=1;
        else InterlockedCompareExchangePointer((PVOID volatile*)slots,(PVOID)(base+0xc2bd0),(PVOID)inventory_send_bits);}
    VirtualProtect((void*)slots,16,old,&ignored);
    if(inventory_send_reference_installed)line("{\"inventory_send_reference\":\"installed\",\"limit\":128,\"observational\":true}\r\n");
}
#endif
