#ifndef ME3_OPTIMIZED_OWNER_TRACE_H
#define ME3_OPTIMIZED_OWNER_TRACE_H
/* Receiver-only observation on the existing protected
 * Ashley ReceivedBunch frame. Does not consume archive data or alter actor state.
 * The vtable slot installation is the only mutation. Bounded process lifetime
 * owns the hook; no DLL unload support, matching the existing research hooks. */
typedef void (*OptimizedOwnerOriginal)(void*,void*,void*,void*,void*);
static OptimizedOwnerOriginal optimized_owner_original;
static int optimized_owner_installed;
static unsigned optimized_owner_rows,optimized_owner_normal_rows,optimized_owner_rejected_rows;

typedef struct OptimizedOwnerSnapshot {
    float position[3],shadow_position[3];
    uint32_t flags264,cursor,bit_count,error;
    int valid;
} OptimizedOwnerSnapshot;

static int optimized_owner_identity(InventoryDecodeFrame *f,uintptr_t pawn,
        uintptr_t archive,uintptr_t shadow,uintptr_t map,uintptr_t channel) {
    uintptr_t base=initial_actor_trace_base;int32_t index=-1,size=0;uint32_t loading=0;
    return f&&inventory_decode_frame==f&&f->before.valid&&f->channel==channel&&
        f->before.pawn==pawn&&GetCurrentThreadId()==initial_actor_trace_thread&&
        is_live(base,channel)&&read_ptr(channel)==base+0x11a2150&&
        read_ptr(channel+0xa0)==pawn&&is_live(base,pawn)&&
        read_ptr(pawn)==base+0x133c3f0&&is_kind_of(base,pawn,"SFXPawn_Ashley")&&
        read_at(pawn+0x38,&index,4)&&index==f->before.index&&
        read_ptr(pawn+0x50)==f->before.klass&&read_ptr(pawn+0x198)==f->before.world&&
        read_ptr(channel+0xc4)==shadow&&shadow>=0x10000&&
        read_at(channel+0xcc,&size,4)&&size>=0x144&&size<=0x100000&&
        read_ptr(archive)==base+0x119dda8&&read_at(archive+0x14,&loading,4)&&loading==1&&
        read_ptr(map)==base+0x11a0d20&&read_ptr(map+0x108)!=0&&
        read_ptr(map+0x108)==read_ptr(channel+0x60);
}
static OptimizedOwnerSnapshot optimized_owner_snapshot(InventoryDecodeFrame *f,
        uintptr_t pawn,uintptr_t archive,uintptr_t shadow,uintptr_t map,uintptr_t channel) {
    OptimizedOwnerSnapshot s={0};
    if(!optimized_owner_identity(f,pawn,archive,shadow,map,channel)||
       !read_at(pawn+0x108,s.position,sizeof s.position)||
       !read_at(shadow+0x108,s.shadow_position,sizeof s.shadow_position)||
       !read_at(pawn+0x264,&s.flags264,4)||!read_at(archive+0xa0,&s.cursor,4)||
       !read_at(archive+0x9c,&s.bit_count,4)||!read_at(archive+0x38,&s.error,4))return s;
    for(int i=0;i<3;++i)if(!isfinite(s.position[i])||!isfinite(s.shadow_position[i]))return s;
    /* Recheck identity/header after reading. Never treat the shadow as UObject. */
    s.valid=optimized_owner_identity(f,pawn,archive,shadow,map,channel);return s;
}
static void optimized_owner_forward(void *pawn,void *archive,void *shadow,void *map,
        void *channel,uintptr_t caller) {
    DWORD entry_error=GetLastError();InventoryDecodeFrame *f=inventory_decode_frame;
    OptimizedOwnerSnapshot before={0},after={0};
    int capture=optimized_owner_installed&&position_receive_opted()&&progress_enabled&&
        test_action==5&&!strcmp(test_role,"client")&&has_marker(root)&&
        GetCurrentThreadId()==initial_actor_trace_thread&&optimized_owner_rows<128&&
        caller==initial_actor_trace_base+0x489adf;
    if(capture)before=optimized_owner_snapshot(f,(uintptr_t)pawn,(uintptr_t)archive,
                                            (uintptr_t)shadow,(uintptr_t)map,(uintptr_t)channel);
    if(capture&&!before.valid&&optimized_owner_rejected_rows<4){
        ++optimized_owner_rejected_rows;char row[750];uint32_t loading=0;int32_t size=0;
        int loading_valid=read_at((uintptr_t)archive+0x14,&loading,4);
        int size_valid=read_at((uintptr_t)channel+0xcc,&size,4);
        snprintf(row,sizeof row,"{\"optimized_owner_trace\":\"scope-rejected\",\"frame_valid\":%s,\"pawn_matches\":%s,\"channel_matches\":%s,\"pawn_vtable_rva\":\"0x%llx\",\"archive_vtable_rva\":\"0x%llx\",\"map_vtable_rva\":\"0x%llx\",\"loading_valid\":%s,\"loading\":%u,\"shadow_size_valid\":%s,\"shadow_size\":%d,\"monotonic_ms\":%lu}\r\n",
          f&&f->before.valid?"true":"false",f&&f->before.pawn==(uintptr_t)pawn?"true":"false",
          f&&f->channel==(uintptr_t)channel?"true":"false",
          (unsigned long long)(read_ptr((uintptr_t)pawn)-initial_actor_trace_base),
          (unsigned long long)(read_ptr((uintptr_t)archive)-initial_actor_trace_base),
          (unsigned long long)(read_ptr((uintptr_t)map)-initial_actor_trace_base),
          loading_valid?"true":"false",loading,size_valid?"true":"false",size,GetTickCount());line(row);
    }
    SetLastError(entry_error);
    optimized_owner_original(pawn,archive,shadow,map,channel); /* Exactly once, all five args unchanged. */
    DWORD exit_error=GetLastError();
    if(before.valid){
        after=optimized_owner_snapshot(f,(uintptr_t)pawn,(uintptr_t)archive,
                                      (uintptr_t)shadow,(uintptr_t)map,(uintptr_t)channel);
        double jump_squared=0;
        if(after.valid)for(int i=0;i<3;++i){double d=(double)after.position[i]-before.position[i];jump_squared+=d*d;}
        int owner=(before.flags264&0x4000)!=0;
        int abnormal=owner||before.error||(after.valid&&after.error)||!after.valid||jump_squared>=256.0*256.0;
        if(abnormal||optimized_owner_normal_rows<4){
            if(!abnormal)++optimized_owner_normal_rows;
            ++optimized_owner_rows;char row[1800];
            snprintf(row,sizeof row,
              "{\"optimized_owner_trace\":\"receive\",\"receive_id\":%u,\"sample\":%u,"
              "\"pawn\":\"0x%llx\",\"channel\":\"0x%llx\",\"shadow\":\"0x%llx\","
              "\"entry_flags264\":%u,\"entry_net_owner\":%s,\"after_valid\":%s,\"exit_flags264\":%u,"
              "\"cursor_before\":%u,\"cursor_after\":%u,\"bit_count_before\":%u,\"bit_count_after\":%u,"
              "\"archive_error_before\":%u,\"archive_error_after\":%u,"
              "\"position_before\":[%.3f,%.3f,%.3f],\"position_after\":[%.3f,%.3f,%.3f],"
              "\"shadow_before\":[%.3f,%.3f,%.3f],\"shadow_after\":[%.3f,%.3f,%.3f],"
              "\"large_position_jump\":%s,\"original_forwarded\":true,\"payload_modified\":false,"
              "\"position_written\":false,\"owner_bit_written\":false,\"monotonic_ms\":%lu}\r\n",
              f->id,optimized_owner_rows,(unsigned long long)(uintptr_t)pawn,
              (unsigned long long)(uintptr_t)channel,(unsigned long long)(uintptr_t)shadow,
              before.flags264,owner?"true":"false",after.valid?"true":"false",after.flags264,
              before.cursor,after.cursor,before.bit_count,after.bit_count,before.error,after.error,
              before.position[0],before.position[1],before.position[2],after.position[0],after.position[1],after.position[2],
              before.shadow_position[0],before.shadow_position[1],before.shadow_position[2],
              after.shadow_position[0],after.shadow_position[1],after.shadow_position[2],
              jump_squared>=256.0*256.0?"true":"false",GetTickCount());line(row);
        }
    }
    SetLastError(exit_error);
}
static void optimized_owner_serialize(void *pawn,void *archive,void *shadow,void *map,void *channel) {
    optimized_owner_forward(pawn,archive,shadow,map,channel,(uintptr_t)__builtin_return_address(0));
}
static void optimized_owner_install(uintptr_t base) {
    DWORD saved=GetLastError();
    if(optimized_owner_installed||!position_receive_opted()||!progress_enabled||test_action!=5||
       strcmp(test_role,"client")||!has_marker(root)||!initial_actor_trace_installed||
       GetCurrentThreadId()!=initial_actor_trace_thread)goto done;
    static const unsigned char pin[20]={0x40,0x55,0x57,0x41,0x56,0x48,0x8b,0xec,0x48,0x83,
        0xec,0x50,0x48,0xc7,0x45,0xe8,0xfe,0xff,0xff,0xff};
    uintptr_t slot=base+0x133c3f0+0x7c8,expected=base+0xa89c20;unsigned char bytes[20];DWORD old,ignored;
    if(!supported_image(base)||read_ptr(slot)!=expected||!read_at(expected,bytes,sizeof bytes)||
       memcmp(bytes,pin,sizeof pin)||!VirtualProtect((void*)slot,8,PAGE_READWRITE,&old))goto done;
    optimized_owner_original=(OptimizedOwnerOriginal)expected;
    PVOID previous=InterlockedCompareExchangePointer((PVOID volatile*)slot,(PVOID)optimized_owner_serialize,(PVOID)expected);
    VirtualProtect((void*)slot,8,old,&ignored);
    if((uintptr_t)previous!=expected){optimized_owner_original=NULL;goto done;}
    optimized_owner_installed=1;
    line("{\"optimized_owner_trace\":\"installed\",\"limit\":128,\"normal_limit\":4,\"observational\":true}\r\n");
done:SetLastError(saved);
}
#endif
