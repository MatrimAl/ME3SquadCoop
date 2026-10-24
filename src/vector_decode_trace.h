#ifndef ME3_VECTOR_DECODE_TRACE_H
#define ME3_VECTOR_DECODE_TRACE_H
/* Protected receive observer only. The existing ReceivedBunch frame supplies
 * Ashley identity. Never consumes the archive or writes decoded data. */
typedef int (*VectorDecodeOriginal)(void*,void*,void*,void*);
static VectorDecodeOriginal vector_decode_original;
static unsigned vector_decode_rows;
static int vector_decode_installed;
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
    DWORD entry_error=GetLastError();InventoryDecodeFrame *f=inventory_decode_frame;
    float before[3]={0},after[3]={0};char path[400]={0};uint32_t error_before=0,error_after=0;
    int capture=vector_decode_installed&&position_receive_opted()&&progress_enabled&&
        test_action==5&&!strcmp(test_role,"client")&&has_marker(root)&&
        GetCurrentThreadId()==initial_actor_trace_thread&&vector_decode_rows<128&&
        caller==initial_actor_trace_base+0x488ed4&&f&&f->before.valid&&
        read_ptr(f->channel+0xa0)==f->before.pawn&&
        vector_decode_property((uintptr_t)property,path)&&
        read_at((uintptr_t)data,before,sizeof before)&&vector_decode_finite(before)&&
        read_at((uintptr_t)archive+0x38,&error_before,4);
    SetLastError(entry_error);
    int result=vector_decode_original(property,archive,map,data);
    DWORD exit_error=GetLastError();
    if(capture&&inventory_decode_frame==f&&read_ptr(f->channel+0xa0)==f->before.pawn&&
       read_at((uintptr_t)data,after,sizeof after)&&vector_decode_finite(after)&&
       read_at((uintptr_t)archive+0x38,&error_after,4)) {
        ++vector_decode_rows;char row[1100];snprintf(row,sizeof row,
          "{\"vector_decode_trace\":\"field\",\"receive_id\":%u,\"field\":\"%s\","
          "\"scratch_before\":[%.3f,%.3f,%.3f],\"scratch_after\":[%.3f,%.3f,%.3f],"
          "\"native_result\":%d,\"archive_error_before\":%u,\"archive_error_after\":%u,"
          "\"original_forwarded\":true,\"payload_modified\":false,\"position_written\":false,"
          "\"final_application_verified\":false,\"monotonic_ms\":%lu}\r\n",
          f->id,path,before[0],before[1],before[2],after[0],after[1],after[2],
          result,error_before,error_after,GetTickCount());line(row);
    }
    SetLastError(exit_error);return result;
}
static int vector_decode_serialize(void *property,void *archive,void *map,void *data) {
    return vector_decode_forward(property,archive,map,data,(uintptr_t)__builtin_return_address(0));
}
static void vector_decode_install(uintptr_t base) {
    if(vector_decode_installed||!position_receive_opted()||!progress_enabled||test_action!=5||
       strcmp(test_role,"client")||!has_marker(root))return;
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
    line("{\"vector_decode_trace\":\"installed\",\"limit\":128,\"observational\":true}\r\n");
}
#endif
