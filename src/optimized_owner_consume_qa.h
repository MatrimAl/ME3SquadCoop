#ifndef ME3_OPTIMIZED_OWNER_CONSUME_CANDIDATE_H
#define ME3_OPTIMIZED_OWNER_CONSUME_CANDIDATE_H
/* Default-off protected QA hypothesis only.
 * Include after optimized_owner_snapshot(), before optimized_owner_forward().
 * This changes one transient owner bit around one unchanged original call.
 * It is not an observer and must not be reported as owner_bit_written=false. */
typedef struct OptimizedOwnerConsumeLease {
    uintptr_t pawn,klass,world;
    int32_t index;
    uint32_t flags_before;
    int changed,restore_status;
} OptimizedOwnerConsumeLease;
static int optimized_owner_consume_active,optimized_owner_consume_failed;
static int optimized_owner_consume_opted(void) {
    static int loaded,value;
    if(!loaded){wchar_t text[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_OPTIMIZED_OWNER_CONSUME_QA",text,8);
        value=n==1&&text[0]==L'1';loaded=1;}
    return value;
}
static OptimizedOwnerConsumeLease optimized_owner_consume_begin(InventoryDecodeFrame *f,
        uintptr_t pawn,uintptr_t archive,uintptr_t shadow,uintptr_t map,uintptr_t channel,uintptr_t caller) {
    DWORD saved=GetLastError();OptimizedOwnerConsumeLease lease={0};uint32_t actor_flags=0,flags=0;
    if(!optimized_owner_consume_opted()||optimized_owner_consume_active||optimized_owner_consume_failed||
       !optimized_owner_installed||!position_receive_opted()||!progress_enabled||test_action!=5||
       strcmp(test_role,"client")||!has_marker(root)||GetCurrentThreadId()!=initial_actor_trace_thread||
       (caller!=initial_actor_trace_base+0x488aeb&&caller!=initial_actor_trace_base+0x489adf))goto done;
    /* Independent of observer row budget. The original native call site already
     * established optimized presence==1 and archive error==0 before dispatch. */
    OptimizedOwnerSnapshot before=optimized_owner_snapshot(f,pawn,archive,shadow,map,channel);
    if(!before.valid||before.error||before.cursor>before.bit_count||before.bit_count-before.cursor<5||
       !(before.flags264&0x4000)||!read_at(pawn+0x25c,&actor_flags,4)||(actor_flags&0x04000008)||
       !read_at(pawn+0x264,&flags,4)||!(flags&0x4000)||
       !optimized_owner_identity(f,pawn,archive,shadow,map,channel))goto done;
    lease.pawn=pawn;lease.klass=f->before.klass;lease.world=f->before.world;lease.index=f->before.index;
    /* A failed CAS makes no change. Do not overwrite concurrent unrelated flags. */
    LONG previous=InterlockedCompareExchange((volatile LONG*)(pawn+0x264),(LONG)(flags&~UINT32_C(0x4000)),(LONG)flags);
    if((uint32_t)previous==flags){lease.flags_before=flags;lease.changed=1;optimized_owner_consume_active=1;}
done:SetLastError(saved);return lease;
}
static int optimized_owner_consume_same_pawn(const OptimizedOwnerConsumeLease *lease) {
    uintptr_t base=initial_actor_trace_base;int32_t index=-1;uint32_t flags=0;
    return GetCurrentThreadId()==initial_actor_trace_thread&&is_live(base,lease->pawn)&&
        read_ptr(lease->pawn)==base+0x133c3f0&&is_kind_of(base,lease->pawn,"SFXPawn_Ashley")&&
        read_at(lease->pawn+0x38,&index,4)&&index==lease->index&&
        read_ptr(lease->pawn+0x50)==lease->klass&&read_ptr(lease->pawn+0x198)==lease->world&&
        read_at(lease->pawn+0x25c,&flags,4)&&!(flags&8);
}
static void optimized_owner_consume_end(OptimizedOwnerConsumeLease *lease) {
    DWORD saved=GetLastError();
    if(!lease->changed)goto done;
    /* Native PostNetReceive may clear/change the channel. Restore only to the
     * same still-live pawn; never write to a replacement UObject or stale address. */
    if(!optimized_owner_consume_same_pawn(lease)){
        lease->restore_status=-1;optimized_owner_consume_failed=1;
    }else{
        InterlockedOr((volatile LONG*)(lease->pawn+0x264),(LONG)0x4000);
        uint32_t flags=0;
        lease->restore_status=read_at(lease->pawn+0x264,&flags,4)&&(flags&0x4000)?1:-1;
        if(lease->restore_status!=1)optimized_owner_consume_failed=1;
    }
    optimized_owner_consume_active=0;
done:SetLastError(saved);
}
#endif
