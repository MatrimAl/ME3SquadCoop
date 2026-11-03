#ifndef ME3_VECTOR_DECODE_TRACE_H
#define ME3_VECTOR_DECODE_TRACE_H
/* Protected field observer. Receive identity comes from ReceivedBunch; sender
 * identity requires the exact native caller, live Ashley and current world.
 * Never consumes the archive or writes serialized/decoded data. */
typedef int (*VectorDecodeOriginal)(void*,void*,void*,void*);
static VectorDecodeOriginal vector_decode_original;
static unsigned vector_decode_rows;
static int vector_decode_installed;
typedef struct VectorDecodeScope {
    struct VectorDecodeScope *previous;
    InventoryDecodeFrame *receive;
    uintptr_t archive,pawn,map,connection;
    int sending;
    unsigned count;
    uint32_t value[4],maximum[4];
    int active;
} VectorDecodeScope;
static VectorDecodeScope *vector_decode_scope;
static int vector_decode_scope_current(VectorDecodeScope *s) {
    if(!s||!s->active||GetCurrentThreadId()!=initial_actor_trace_thread)return 0;
    if(!s->sending)return inventory_decode_frame==s->receive;
    uintptr_t base=initial_actor_trace_base;
    return read_ptr(s->archive)==base+0x119dcc0&&read_ptr(s->map)==base+0x11a0d20&&
        read_ptr(s->map+0x108)==s->connection&&is_live(base,s->pawn)&&
        is_kind_of(base,s->pawn,"SFXPawn_Ashley");
}
static int vector_decode_sender(uintptr_t property,uintptr_t archive,uintptr_t map,
        uintptr_t data,uintptr_t caller,const char *path,VectorDecodeScope *s) {
    uintptr_t base=initial_actor_trace_base;uint32_t loading=1;int32_t offset=0;
    if(test_action!=6||strcmp(test_role,"host")||caller!=base+0x48d433||
       (strcmp(path,"Engine.Actor.RelativeLocation")&&strcmp(path,"Engine.Actor.Velocity"))||
       !read_at(property+0x94,&offset,4)||
       offset!=(!strcmp(path,"Engine.Actor.Velocity")?0x138:0x15c)||data<(uintptr_t)offset||
       read_ptr(archive)!=base+0x119dcc0||read_ptr(map)!=base+0x11a0d20||
       !read_at(archive+0x14,&loading,4)||loading)return 0;
    uintptr_t pawn=data-offset,world=read_ptr(base+0x18b6638),level=0,info=0;GameArray actors={0};
    if(!is_live(base,pawn)||!is_kind_of(base,pawn,"SFXPawn_Ashley")||
       !is_kind_of(base,pawn,"Pawn")||!is_live(base,world)||
       !(level=read_ptr(world+0x88))||!is_live(base,level)||
       !read_at(level+0x60,&actors,sizeof actors)||actors.count<1||actors.count>100000||
       actors.max<actors.count||actors.max>1000000||
       !(info=read_ptr(actors.data))||!is_live(base,info)||read_ptr(pawn+0x198)!=info||
       !(s->connection=read_ptr(map+0x108)))return 0;
    s->sending=1;s->pawn=pawn;s->map=map;return 1;
}
static void vector_decode_index_observe(void *archive,uint32_t *dest,uint32_t maximum,uintptr_t caller) {
    VectorDecodeScope *s=vector_decode_scope;
    static const uintptr_t sites[4]={0x10cc41,0x10cc7d,0x10cc91,0x10cca2};
    uint32_t value=0;
    if(!s||!s->active||s->count>=4||s->archive!=(uintptr_t)archive||
       !vector_decode_scope_current(s)||
       caller!=initial_actor_trace_base+sites[s->count]||!read_at((uintptr_t)dest,&value,4)||
       value>=maximum)return;
    if((s->count==0&&maximum!=20)||(s->count>0&&
       (s->value[0]>=20||maximum!=(UINT32_C(1)<<(s->value[0]+2)))))return;
    s->value[s->count]=value;s->maximum[s->count]=maximum;++s->count;
}
static int vector_decode_finite(const float value[3]) {
    return isfinite(value[0])&&isfinite(value[1])&&isfinite(value[2]);
}
static int vector_decode_property(uintptr_t property,char path[400]) {
    uintptr_t base=initial_actor_trace_base;int32_t dim=0,size=0,offset=0;
    if(!is_live(base,property)||read_ptr(property)!=base+0x10784a8||
       !read_at(property+0x70,&dim,4)||dim!=1||!read_at(property+0x74,&size,4)||size!=12||
       !read_at(property+0x94,&offset,4))return 0;
    identity_path(base,property,path,400);
    return (offset==0x108&&!strcmp(path,"Engine.Actor.Location"))||
        (offset==0x138&&!strcmp(path,"Engine.Actor.Velocity"))||
        (offset==0x15c&&!strcmp(path,"Engine.Actor.RelativeLocation"));
}
static int vector_decode_forward(void *property,void *archive,void *map,void *data,uintptr_t caller) {
    DWORD entry_error=GetLastError();
    if(GetCurrentThreadId()!=initial_actor_trace_thread){SetLastError(entry_error);return vector_decode_original(property,archive,map,data);}
    InventoryDecodeFrame *f=inventory_decode_frame;
    float before[3]={0},after[3]={0};char path[400]={0};uint32_t error_before=0,error_after=0;
    VectorDecodeScope scope={0};scope.previous=vector_decode_scope;scope.receive=f;
    scope.archive=(uintptr_t)archive;
    int capture=vector_decode_installed&&position_receive_opted()&&progress_enabled&&has_marker(root)&&
        GetCurrentThreadId()==initial_actor_trace_thread&&vector_decode_rows<128&&
        vector_decode_property((uintptr_t)property,path)&&
        read_at((uintptr_t)data,before,sizeof before)&&vector_decode_finite(before)&&
        read_at((uintptr_t)archive+0x38,&error_before,4)&&
        ((test_action==5&&!strcmp(test_role,"client")&&caller==initial_actor_trace_base+0x488ed4&&
          f&&f->before.valid&&read_ptr(f->channel+0xa0)==f->before.pawn)||
         vector_decode_sender((uintptr_t)property,(uintptr_t)archive,(uintptr_t)map,(uintptr_t)data,caller,path,&scope));
    scope.active=capture;vector_decode_scope=&scope;
    SetLastError(entry_error);
    int result=vector_decode_original(property,archive,map,data);
    DWORD exit_error=GetLastError();
    vector_decode_scope=scope.previous;
    if(capture&&vector_decode_scope_current(&scope)&&
       (scope.sending||read_ptr(f->channel+0xa0)==f->before.pawn)&&
       read_at((uintptr_t)data,after,sizeof after)&&vector_decode_finite(after)&&
       read_at((uintptr_t)archive+0x38,&error_after,4)) {
        ++vector_decode_rows;char row[1200];snprintf(row,sizeof row,
          "{\"%s\":\"field\",\"%s\":%u,\"field\":\"%s\","
          "\"scratch_before\":[%.3f,%.3f,%.3f],\"scratch_after\":[%.3f,%.3f,%.3f],"
          "\"pawn\":\"0x%llx\",\"map_connection\":\"0x%llx\",\"native_result\":%d,\"archive_error_before\":%u,\"archive_error_after\":%u,"
          "\"scalar_count\":%u,\"scalar_values\":[%u,%u,%u,%u],\"scalar_maxima\":[%u,%u,%u,%u],"
          "\"original_forwarded\":true,\"payload_modified\":false,\"position_written\":false,"
          "\"final_application_verified\":false,\"monotonic_ms\":%lu}\r\n",
          scope.sending?"vector_send_trace":"vector_decode_trace",scope.sending?"send_id":"receive_id",
          scope.sending?vector_decode_rows:f->id,path,before[0],before[1],before[2],after[0],after[1],after[2],
          (unsigned long long)(scope.sending?scope.pawn:f->before.pawn),(unsigned long long)scope.connection,
          result,error_before,error_after,scope.count,scope.value[0],scope.value[1],scope.value[2],scope.value[3],
          scope.maximum[0],scope.maximum[1],scope.maximum[2],scope.maximum[3],GetTickCount());line(row);
    }
    SetLastError(exit_error);return result;
}
static int vector_decode_serialize(void *property,void *archive,void *map,void *data) {
    return vector_decode_forward(property,archive,map,data,(uintptr_t)__builtin_return_address(0));
}
static void vector_decode_install(uintptr_t base) {
    int side=(test_action==5&&!strcmp(test_role,"client"))||(test_action==6&&!strcmp(test_role,"host"));
    if(vector_decode_installed||!position_receive_opted()||!progress_enabled||!side||!has_marker(root))return;
    static const unsigned char pin[20]={0x48,0x8b,0xc4,0x4c,0x89,0x40,0x18,0x48,0x89,0x48,
        0x08,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56};
    uintptr_t slot=base+0x10784a8+0x2f8,expected=base+0x146fb0;unsigned char bytes[20];DWORD old,ignored;
    if(!supported_image(base)||read_ptr(slot)!=expected||!read_at(expected,bytes,sizeof bytes)||
       memcmp(bytes,pin,sizeof pin)||!VirtualProtect((void*)slot,8,PAGE_READWRITE,&old))return;
    vector_decode_original=(VectorDecodeOriginal)expected;
    PVOID previous=InterlockedCompareExchangePointer((PVOID volatile*)slot,(PVOID)vector_decode_serialize,(PVOID)expected);
    VirtualProtect((void*)slot,8,old,&ignored);
    if((uintptr_t)previous!=expected){vector_decode_original=NULL;return;}
    vector_decode_installed=1;
    vector_decode_index_observer=vector_decode_index_observe;
    line("{\"vector_decode_trace\":\"installed\",\"limit\":128,\"observational\":true}\r\n");
}
#endif
