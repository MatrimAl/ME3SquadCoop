#ifndef ME3_RUNTIME_OWNED_OPTIMIZED_RECEIVE_H
#define ME3_RUNTIME_OWNED_OPTIMIZED_RECEIVE_H
/* The native channel has already consumed the positive optimized-payload
 * marker at these two calls. bNetOwner makes the stock pawn callback skip
 * that payload, leaving the following property decoder at the wrong bit.
 * Reuse the protected correction: clear only that bit for this original
 * callback, then restore it on the same live pawn. Never decode, skip, copy
 * or repair archive data, positions, inventory pointers or actor roles here.
 * Include after the ordinary receive wrapper and channel identity helpers. */
typedef void (*RuntimeOwnedOptimizedOriginal)(void*,void*,void*,void*,void*);
typedef struct RuntimeOwnedOptimizedLease {
    GuestCameraStamp pawn,world;
    uintptr_t base,channel;
    uint32_t cursor;
    int changed,handoff;
} RuntimeOwnedOptimizedLease;
static RuntimeOwnedOptimizedOriginal runtime_owned_optimized_original;
static int runtime_owned_optimized_installed,runtime_owned_optimized_attempted;
static int runtime_owned_optimized_busy,runtime_owned_optimized_failed;
static unsigned runtime_owned_optimized_rows;
static int runtime_owned_optimized_opted(void) {
    static int value=-1;
    if(value<0){wchar_t text[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_OWNED_OPTIMIZED_RECEIVE",text,8);
        value=!n||(n==1&&text[0]==L'1');}
    return value;
}
static int runtime_owned_optimized_case(int enabled,int guest,int frame,int caller,
        int owned,int identity,uint32_t flags,uint32_t error,uint32_t cursor,uint32_t bits) {
    return enabled==1&&guest==1&&frame==1&&caller==1&&owned==1&&identity==1&&
        (flags&0x4000)&&!error&&cursor<=bits&&bits-cursor>=5;
}
static int runtime_owned_optimized_clear(volatile LONG *flags,uint32_t expected) {
    return (uint32_t)InterlockedCompareExchange(flags,(LONG)(expected&~UINT32_C(0x4000)),(LONG)expected)==expected;
}
static void runtime_owned_optimized_restore_bit(volatile LONG *flags) {
    InterlockedOr(flags,(LONG)0x4000);
}
/* A scene detaches the local PC before its actor channel stops receiving
 * owner-marked payloads. Retain only a previously proved native owner/channel
 * identity, independently of camera/UI registration and current possession. */
typedef struct RuntimeOwnedReceiveScope {
    GuestCameraStamp pc,pawn,world,channel,connection,driver;
    int active,index;
} RuntimeOwnedReceiveScope;
static RuntimeOwnedReceiveScope runtime_owned_receive_scope;
static DWORD runtime_owned_receive_refused;
static int runtime_owned_receive_permission(int direct,int captured,unsigned char role) {
    return (direct&&role==2)||(captured&&(role==1||role==2));
}
static int runtime_owned_receive_scope_same(uintptr_t base,SessionContext c,uintptr_t pawn,
        const NetworkActorIdentityEntry *entry) {
    RuntimeOwnedReceiveScope *s=&runtime_owned_receive_scope;
    return s->active&&s->pc.pointer==c.pc&&s->pawn.pointer==pawn&&s->world.pointer==c.world&&
        s->channel.pointer==entry->channel&&s->connection.pointer==entry->connection&&s->driver.pointer==entry->driver&&
        s->index==entry->index&&session_actor_live(base,c.pc)&&read_ptr(c.pc+0x198)==c.world&&
        guest_camera_stamp_matches(base,&s->pc,0)&&guest_camera_stamp_matches(base,&s->pawn,0)&&
        guest_camera_stamp_matches(base,&s->world,0)&&guest_camera_stamp_matches(base,&s->channel,0)&&
        guest_camera_stamp_matches(base,&s->connection,0)&&guest_camera_stamp_matches(base,&s->driver,0);
}
static void runtime_owned_receive_capture(uintptr_t base) {
    RuntimeOwnedReceiveScope *s=&runtime_owned_receive_scope;
    if(!runtime_control_enabled||!runtime_scope_ready||g_runtime_config.role!=ME3_ROLE_GUEST||!runtime_owned_optimized_opted()) {
        s->active=0;return;
    }
    SessionContext c=session_context(session_engine);NetworkActorIdentityEntry entry={0};
    if(s->active&&(s->world.pointer!=c.world||s->pc.pointer!=c.pc||
       !guest_camera_stamp_matches(base,&s->pawn,0)||!guest_camera_stamp_matches(base,&s->channel,0)||
       !guest_camera_stamp_matches(base,&s->connection,0)||!guest_camera_stamp_matches(base,&s->driver,0)))s->active=0;
    if(!guest_player_context_basic_owned(base,c.pc,c.pawn,c.world)||
       !is_kind_of(base,c.pawn,"SFXPawn_Ashley")||read_ptr(c.pawn)!=base+0x133c3f0||
       !guest_player_context_channel(base,c.pawn,&entry))return;
    unsigned char role=255;
    if(!read_at(c.pawn+0x26a,&role,1)||role!=2||runtime_owned_receive_scope_same(base,c,c.pawn,&entry))return;
    RuntimeOwnedReceiveScope next={0};
    uintptr_t values[]={c.pc,c.pawn,c.world,entry.channel,entry.connection,entry.driver};
    GuestCameraStamp *stamps[]={&next.pc,&next.pawn,&next.world,&next.channel,&next.connection,&next.driver};
    for(unsigned n=0;n<6;++n)if(!guest_camera_stamp_read(base,values[n],stamps[n],0))return;
    next.index=entry.index;next.active=entry.index>0;*s=next;
    if(next.active)line("{\"runtime_owned_optimized\":\"native-owner-channel-captured\",\"handoff_payloads_only\":true}\r\n");
}
static RuntimeOwnedOptimizedLease runtime_owned_optimized_begin(uintptr_t pawn,uintptr_t archive,
        uintptr_t shadow,uintptr_t map,uintptr_t channel,uintptr_t caller) {
    RuntimeOwnedOptimizedLease lease={0};uintptr_t base=runtime_rpc_receive_base;
    NativeRpcReceiveFrame *frame=native_rpc_receive_current(runtime_rpc_receive_thread);
    if(!runtime_owned_optimized_installed||!runtime_owned_optimized_opted()||runtime_owned_optimized_busy||
       runtime_owned_optimized_failed||!runtime_control_enabled||!runtime_scope_ready||g_runtime_config.role!=ME3_ROLE_GUEST||
       !frame||frame->channel!=channel||(caller!=base+0x488aeb&&caller!=base+0x489adf))return lease;
    SessionContext c=session_context(session_engine);NetworkActorScope before,after;NetworkActorIdentityEntry entry={0};
    uint32_t flags=0,actor_flags=0,loading=0,error=1,cursor=0,bits=0;int32_t size=0;unsigned char role=255;
    /* Positive native payload framing remains required during story release.
     * Losing reciprocal possession does not make an already consumed marker
     * safe to skip. Retained identity grants receive parsing only, no input. */
    if(local_controller(session_engine)!=c.pc||!guest_camera_actor_live(base,c.pc)||
       !guest_camera_actor_live(base,pawn)||read_ptr(pawn)!=base+0x133c3f0||!is_kind_of(base,pawn,"SFXPawn_Ashley")||
       !read_at(pawn+0x26a,&role,1)||
       !read_at(pawn+0x25c,&actor_flags,4)||(actor_flags&0x04000008)||
       !network_actor_read_scope(base,pawn,&before)||before.info!=c.world||
       !network_actor_channel_valid(base,pawn,channel,before.driver,&entry)||
       read_ptr(before.driver+0x80)!=entry.connection||read_ptr(channel+0xc4)!=shadow||shadow<0x10000||
       !read_at(channel+0xcc,&size,4)||size<0x144||size>0x100000||
       read_ptr(archive)!=base+0x119dda8||!read_at(archive+0x14,&loading,4)||loading!=1||
       read_ptr(map)!=base+0x11a0d20||read_ptr(map+0x108)!=entry.connection||
       !read_at(archive+0x38,&error,4)||!read_at(archive+0xa0,&cursor,4)||!read_at(archive+0x9c,&bits,4)||
       !read_at(pawn+0x264,&flags,4)||!runtime_owned_optimized_case(1,1,1,1,1,1,flags,error,cursor,bits)||
       !guest_camera_stamp_read(base,pawn,&lease.pawn,0)||!guest_camera_stamp_read(base,c.world,&lease.world,0)||
       !network_actor_read_scope(base,pawn,&after)||memcmp(&before,&after,sizeof before))return lease;
    int direct=c.pawn==pawn&&read_ptr(pawn+0x374)==c.pc&&read_ptr(c.pc+0x3bc)==pawn;
    int captured=runtime_owned_receive_scope_same(base,c,pawn,&entry);
    if(!runtime_owned_receive_permission(direct,captured,role)) {
        DWORD now=GetTickCount();
        if(!runtime_owned_receive_refused||now-runtime_owned_receive_refused>=5000) {
            runtime_owned_receive_refused=now;
            line("{\"runtime_owned_optimized\":\"positive-payload-refused\",\"reason\":\"no-current-or-captured-owner-scope\",\"payload_written\":false}\r\n");
        }
        return lease;
    }
    lease.handoff=!direct;
    if(runtime_owned_optimized_clear((volatile LONG*)(pawn+0x264),flags)) {
        lease.base=base;lease.channel=channel;lease.cursor=cursor;lease.changed=1;runtime_owned_optimized_busy=1;
    }
    return lease;
}
static int runtime_owned_optimized_end(RuntimeOwnedOptimizedLease *lease) {
    if(!lease->changed)return 0;
    uintptr_t pawn=lease->pawn.pointer;int restored=0;
    if(guest_camera_stamp_matches(lease->base,&lease->pawn,0)&&guest_camera_stamp_matches(lease->base,&lease->world,0)&&
       guest_camera_actor_live(lease->base,pawn)&&read_ptr(pawn)==lease->base+0x133c3f0&&
       read_ptr(pawn+0x198)==lease->world.pointer) {
        /* Preserve every unrelated bit changed by native PostNetReceive. */
        runtime_owned_optimized_restore_bit((volatile LONG*)(pawn+0x264));
        uint32_t flags=0;restored=read_at(pawn+0x264,&flags,4)&&(flags&0x4000);
    }
    if(!restored)runtime_owned_optimized_failed=1;
    runtime_owned_optimized_busy=0;return restored;
}
static void runtime_owned_optimized_forward(void *pawn,void *archive,void *shadow,void *map,void *channel,uintptr_t caller) {
    DWORD entry_error=GetLastError();
    RuntimeOwnedOptimizedLease lease=runtime_owned_optimized_begin((uintptr_t)pawn,(uintptr_t)archive,
        (uintptr_t)shadow,(uintptr_t)map,(uintptr_t)channel,caller);
    SetLastError(entry_error);runtime_owned_optimized_original(pawn,archive,shadow,map,channel);
    DWORD exit_error=GetLastError();int restored=runtime_owned_optimized_end(&lease);
    if(lease.changed&&runtime_owned_optimized_rows++<16) {
        uint32_t cursor=0,error=1;int valid=read_at((uintptr_t)archive+0xa0,&cursor,4)&&read_at((uintptr_t)archive+0x38,&error,4);
        char row[480];snprintf(row,sizeof row,"{\"runtime_owned_optimized\":\"native-payload-consumed\",\"story_handoff\":%s,\"owner_bit_restored\":%s,\"archive_read_valid\":%s,\"cursor_advanced\":%s,\"archive_error\":%u,\"payload_written\":false,\"position_written\":false,\"monotonic_ms\":%lu}\r\n",
            lease.handoff?"true":"false",restored?"true":"false",valid?"true":"false",valid&&cursor>lease.cursor?"true":"false",error,GetTickCount());line(row);
    }
    SetLastError(exit_error);
}
static void runtime_owned_optimized_receive(void *pawn,void *archive,void *shadow,void *map,void *channel) {
    runtime_owned_optimized_forward(pawn,archive,shadow,map,channel,(uintptr_t)__builtin_return_address(0));
}
static void runtime_owned_optimized_prepare(uintptr_t base) {
    runtime_owned_receive_capture(base);
    if(runtime_owned_optimized_attempted||!runtime_owned_optimized_opted()||g_runtime_config.role!=ME3_ROLE_GUEST||
       !runtime_control_enabled||!runtime_scope_ready||!runtime_rpc_receive_installed)return;
    runtime_owned_optimized_attempted=1;
    static const unsigned char pin[]={0x40,0x55,0x57,0x41,0x56,0x48,0x8b,0xec,0x48,0x83,
        0xec,0x50,0x48,0xc7,0x45,0xe8,0xfe,0xff,0xff,0xff};
    uintptr_t slot=base+0x133c3f0+0x7c8,expected=base+0xa89c20;unsigned char bytes[sizeof pin];DWORD old=0,ignored=0;
    if(!supported_image(base)||!native_actor_receive_pins(base)||read_ptr(slot)!=expected||
       !read_at(expected,bytes,sizeof bytes)||memcmp(bytes,pin,sizeof pin)||!VirtualProtect((void*)slot,8,PAGE_READWRITE,&old))return;
    runtime_owned_optimized_original=(RuntimeOwnedOptimizedOriginal)expected;
    PVOID previous=InterlockedCompareExchangePointer((PVOID volatile*)slot,(PVOID)runtime_owned_optimized_receive,(PVOID)expected);
    VirtualProtect((void*)slot,8,old,&ignored);
    if((uintptr_t)previous!=expected){runtime_owned_optimized_original=NULL;return;}
    runtime_owned_optimized_installed=1;
    line("{\"runtime_owned_optimized\":\"installed\",\"scoped_native_receive_only\":true,\"position_written\":false}\r\n");
}
#endif
