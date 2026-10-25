#ifndef ME3_SQUAD_COOP_INITIAL_ACTOR_TRACE_H
#define ME3_SQUAD_COOP_INITIAL_ACTOR_TRACE_H

/* Research-only, opt-in receive observation. Include AFTER progress_probe.h.
 * Requires existing supported_image/read_at/read_ptr/is_live helpers and line,
 * and main's static uintptr_t network_actor_received_bunch_observer = 0.
 * Call initial_actor_trace_install(base) from the FIRST game-thread tick,
 * before its original engine tick can dispatch the join.
 *
 * Exact b453871e... LE3 image native evidence (read-only disassembly):
 * ActorChannel vtable 11a2150 +2d0 -> 488700. Dispatcher 48bb40 saves
 * RCX=this and RDX=FInBunch*, calls +2d0 at 48bb78, ignores its return.
 * ReceivedBunch 488700 saves RCX/RDX; no third argument is consumed.
 * Therefore Win64 void(this,bunch) is the ABI; no engine-call guessing.
 * Actor+a0 NULL -> requires bunch byte+c4 at 488754. Archive virtual+30
 * decodes the initial UObject at 48876e; NULL/non-Actor sets channel+68 bit8
 * at 4887bf. UObject flags+c &600 selects archetype spawn 589ee0; otherwise
 * it binds an existing Actor. Binding 48f800 writes Actor+a0, Class+a8.
 * No archive seek/read/deserialize/spawn/bind call is performed by this hook.
 * FInBunch byte/bit-position layouts are NOT pinned here and are omitted.
 *
 * INTEGRATION REQUIREMENT: existing actor identity/diagnostic guards require
 * slot+2d0 == original. Main's network_actor_received_handler_matches helper
 * must accept ONLY original OR exactly network_actor_received_bunch_observer;
 * this header assigns that observer ONLY following successful original->hook
 * CAS. All original code-byte pins remain required. No arbitrary hook acceptance.
 * This header is inactive until explicitly installed. No DLL unload support:
 * bounded research process lifetime must own the hook as with the tick hook.
 */
typedef void (*InitialActorReceivedBunch)(void *channel, void *bunch);
static InitialActorReceivedBunch initial_actor_original_bunch = NULL;
static uintptr_t initial_actor_trace_base = 0;
static DWORD initial_actor_trace_thread = 0;
static unsigned int initial_actor_trace_count = 0;
static int initial_actor_trace_installed = 0, initial_actor_trace_attempted = 0;

/* Default-off diagnostic on the existing receive hook, protected probe only.
 * Never decodes/consumes a packet or corrects a pawn. Keep the original call
 * and its LastError unchanged. Log only discontinuities, bounded to64 records. */
static int position_receive_opted(void) {
    static int ready,value;
    if(!ready) {wchar_t text[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_POSITION_RECEIVE_TRACE",text,8);
        value=n==1&&text[0]==L'1';ready=1;}
    return value;
}
typedef struct PositionReceiveSnapshot {
    uintptr_t pawn,klass,world,base,controller,inventory,weapon;
    int32_t index;
    float position[3],relative[3],velocity[3];
    uint32_t actor_flags;
    unsigned char physics;
    unsigned char roles[2];
    int valid;
} PositionReceiveSnapshot;
static unsigned int position_receive_events;
static PositionReceiveSnapshot position_receive_snapshot_actor(uintptr_t channel,uintptr_t retained) {
    PositionReceiveSnapshot s={0};uintptr_t base=initial_actor_trace_base;
    if(!position_receive_opted()||!progress_enabled||test_action!=5||strcmp(test_role,"client")||
       GetCurrentThreadId()!=initial_actor_trace_thread||position_receive_events>=64||!has_marker(root)||
       !is_live(base,channel)||read_ptr(channel)!=base+0x11a2150)return s;
    s.pawn=read_ptr(channel+0xa0);
    /* ReceivedBunch clears Actor+a0 on its native tear-off path. Observe the
     * retained identity only when that slot became null, never a replacement. */
    if(!s.pawn&&retained)s.pawn=retained;
    if(!is_live(base,s.pawn)||!is_kind_of(base,s.pawn,"SFXPawn_Ashley"))return s;
    s.klass=read_ptr(s.pawn+0x50);s.world=read_ptr(s.pawn+0x198);
    uintptr_t world=read_ptr(base+0x18b6638);
    uintptr_t level=read_ptr(world+0x88);GameArray actors={0};unsigned char net=0;
    if(!is_live(base,world)||!is_live(base,level)||!read_at(level+0x60,&actors,sizeof actors)||
       actors.count<1||actors.count>100000||actors.max<actors.count||actors.max>1000000||
       read_ptr(actors.data)!=s.world||!is_live(base,s.world)||
       !read_at(s.world+0x7c8,&net,1)||net!=3)return s;
    if(!read_at(s.pawn+0x38,&s.index,4)||!read_at(s.pawn+0x108,s.position,12)||
       !read_at(s.pawn+0x15c,s.relative,12)||!read_at(s.pawn+0x138,s.velocity,12)||
       !read_at(s.pawn+0x25c,&s.actor_flags,4)||!read_at(s.pawn+0x268,&s.physics,1)||
       !read_at(s.pawn+0x188,&s.base,8)||
       !read_at(s.pawn+0x414,&s.inventory,8)||!read_at(s.pawn+0x41c,&s.weapon,8)||
       !read_at(s.pawn+0x374,&s.controller,8)||!read_at(s.pawn+0x269,s.roles,2))return s;
    for(int i=0;i<3;++i)if(!isfinite(s.position[i])||!isfinite(s.relative[i])||!isfinite(s.velocity[i]))return s;
    s.valid=1;return s;
}
static PositionReceiveSnapshot position_receive_snapshot(uintptr_t channel) {
    return position_receive_snapshot_actor(channel,0);
}
static void position_receive_finish(uintptr_t channel,PositionReceiveSnapshot before) {
    if(!before.valid)return;
    PositionReceiveSnapshot after=position_receive_snapshot_actor(channel,before.pawn);
    if(!after.valid||before.pawn!=after.pawn||before.klass!=after.klass||before.index!=after.index||before.world!=after.world)return;
    double delta=0,velocity_delta=0,relative_delta=0;
    for(int i=0;i<3;++i){double d=(double)after.position[i]-before.position[i];delta+=d*d;
        d=(double)after.velocity[i]-before.velocity[i];velocity_delta+=d*d;
        d=(double)after.relative[i]-before.relative[i];relative_delta+=d*d;}
    if(delta<256.0*256.0&&velocity_delta<256.0*256.0&&relative_delta<256.0*256.0&&
       before.physics==after.physics&&!((before.actor_flags^after.actor_flags)&0x04000008)&&read_ptr(channel+0xa0)==before.pawn&&before.base==after.base&&before.controller==after.controller&&
       before.inventory==after.inventory&&before.weapon==after.weapon&&
       !memcmp(before.roles,after.roles,2))return;
    ++position_receive_events;
    char row[2300];snprintf(row,sizeof row,
        "{\"position_receive_trace\":\"change-in-original-receive\",\"event\":%u,\"pawn\":\"0x%llx\","
        "\"before\":[%.3f,%.3f,%.3f],\"after\":[%.3f,%.3f,%.3f],"
        "\"relative_before\":[%.3f,%.3f,%.3f],\"relative_after\":[%.3f,%.3f,%.3f],"
        "\"velocity_before\":[%.3f,%.3f,%.3f],\"velocity_after\":[%.3f,%.3f,%.3f],"
        "\"physics_before\":%u,\"physics_after\":%u,\"actor_flags_before\":%u,\"actor_flags_after\":%u,"
        "\"channel_actor_cleared\":%s,"
        "\"base_before\":\"0x%llx\",\"base_after\":\"0x%llx\","
        "\"inventory_before\":\"0x%llx\",\"inventory_after\":\"0x%llx\","
        "\"weapon_before\":\"0x%llx\",\"weapon_after\":\"0x%llx\","
        "\"controller_before\":\"0x%llx\",\"controller_after\":\"0x%llx\","
        "\"roles_before\":[%u,%u],\"roles_after\":[%u,%u],\"original_forwarded\":true,"
        "\"bunch_modified_by_probe\":false,\"position_written_by_probe\":false,\"monotonic_ms\":%lu}\r\n",
        position_receive_events,(unsigned long long)before.pawn,
        before.position[0],before.position[1],before.position[2],after.position[0],after.position[1],after.position[2],
        before.relative[0],before.relative[1],before.relative[2],after.relative[0],after.relative[1],after.relative[2],
        before.velocity[0],before.velocity[1],before.velocity[2],after.velocity[0],after.velocity[1],after.velocity[2],
        before.physics,after.physics,before.actor_flags,after.actor_flags,read_ptr(channel+0xa0)?"false":"true",
        (unsigned long long)before.base,(unsigned long long)after.base,
        (unsigned long long)before.inventory,(unsigned long long)after.inventory,
        (unsigned long long)before.weapon,(unsigned long long)after.weapon,
        (unsigned long long)before.controller,(unsigned long long)after.controller,
        before.roles[0],before.roles[1],after.roles[0],after.roles[1],GetTickCount());line(row);
}

#include "inventory_decode_trace.h"
#include "initial_object_trace.h"

static void initial_actor_received_bunch(void *self, void *bunch) {
    /* Invariant: the original is assigned before the only CAS that publishes
     * this function. Never change or consume any bytes in the incoming bunch.
     */
    InitialActorReceivedBunch forward = initial_actor_original_bunch;
    uintptr_t channel = (uintptr_t)self, incoming = (uintptr_t)bunch;
    DWORD error_before = GetLastError();
    uintptr_t actor_before = 0, class_before = 0;
    uint32_t flags_before = 0, archive_before = 0;
    int32_t index = -1; unsigned char initial_byte = 0;
    int capture = progress_enabled && test_action == 5 && !strcmp(test_role, "client") &&
        GetCurrentThreadId() == initial_actor_trace_thread && initial_actor_trace_count < 128 && has_marker(root) &&
        is_live(initial_actor_trace_base, channel) &&
        read_ptr(channel) == initial_actor_trace_base + 0x11a2150 &&
        read_at(channel + 0xa0, &actor_before, sizeof actor_before) && !actor_before &&
        read_at(channel + 0xa8, &class_before, sizeof class_before) &&
        read_at(channel + 0x6c, &index, sizeof index) && index >= 0 && index < 2048 &&
        read_at(channel + 0x68, &flags_before, sizeof flags_before) &&
        read_at(incoming + 0xc4, &initial_byte, sizeof initial_byte) &&
        read_at(incoming + 0x38, &archive_before, sizeof archive_before);
    unsigned int event = capture ? ++initial_actor_trace_count : 0;
    PositionReceiveSnapshot position_before=position_receive_snapshot(channel);
    InventoryDecodeFrame inventory_frame;inventory_decode_begin(&inventory_frame,channel,position_before);
    AmmoWireReceive wire_frame;ammo_wire_receive_begin(&wire_frame,channel);
    int object_same_thread=GetCurrentThreadId()==initial_actor_trace_thread;
    InitialObjectFrame object_frame={0};
    if(object_same_thread){
        object_frame.previous=initial_object_frame;object_frame.channel=channel;object_frame.archive=incoming;
        object_frame.event=event;object_frame.index=index;object_frame.active=capture&&initial_byte!=0;
        initial_object_frame=&object_frame;
    }
    SetLastError(error_before);
    forward(self, bunch); /* Always exactly one original call, unchanged args. */
    DWORD error_after = GetLastError();
    if(object_same_thread&&initial_object_frame==&object_frame)initial_object_frame=object_frame.previous;
    ammo_wire_receive_end(&wire_frame);
    inventory_decode_end(&inventory_frame);
    position_receive_finish(channel,position_before);
    if (capture) {
        uintptr_t actor_after = 0, class_after = 0;
        uint32_t flags_after = 0, archive_after = 0;
        int after_valid = is_live(initial_actor_trace_base, channel) &&
            read_ptr(channel) == initial_actor_trace_base + 0x11a2150 &&
            read_at(channel + 0xa0, &actor_after, sizeof actor_after) &&
            read_at(channel + 0xa8, &class_after, sizeof class_after) &&
            read_at(channel + 0x68, &flags_after, sizeof flags_after) &&
            read_at(incoming + 0x38, &archive_after, sizeof archive_after);
        char row[1100];
        snprintf(row, sizeof row,
            "{\"initial_actor_trace\":\"received-bunch\",\"event\":%u,\"channel_index\":%d,\"callback_thread_id\":%lu,"
            "\"initial_byte_c4\":%u,\"channel_flags_68_before\":%u,\"channel_flags_68_after\":%u,"
            "\"actor_a0_before\":\"0x%llx\",\"actor_a0_after\":\"0x%llx\","
            "\"class_a8_before\":\"0x%llx\",\"class_a8_after\":\"0x%llx\","
            "\"archive_38_before\":%u,\"archive_38_after\":%u,\"after_valid\":%s,"
            "\"original_forwarded\":true,\"bunch_modified_by_probe\":false,\"monotonic_ms\":%lu}\r\n",
            event, index, GetCurrentThreadId(), (unsigned)initial_byte, flags_before, flags_after,
            (unsigned long long)actor_before, (unsigned long long)actor_after,
            (unsigned long long)class_before, (unsigned long long)class_after,
            archive_before, archive_after, after_valid ? "true" : "false", GetTickCount());
        line(row);
    }
    SetLastError(error_after);
}

static int initial_actor_trace_owns_slot(uintptr_t base) {
    return initial_actor_trace_installed && initial_actor_trace_base == base &&
        (uintptr_t)initial_actor_original_bunch == base + 0x488700 &&
        read_ptr(base + 0x11a2150 + 0x2d0) == (uintptr_t)initial_actor_received_bunch;
}

static int initial_actor_trace_install(uintptr_t base) {
    if (!progress_enabled || !((test_action==5&&!strcmp(test_role,"client"))||
        (test_action==6&&!strcmp(test_role,"host")&&(ammo_rpc_opted()||fire_rpc_opted())))) return 0;
    if (initial_actor_trace_installed) return initial_actor_trace_owns_slot(base);
    if (!has_marker(root)) return 0;
    if (initial_actor_trace_attempted) return 0;
    initial_actor_trace_attempted = 1;
    static const unsigned char prologue[32] = {
        0x48,0x89,0x54,0x24,0x10,0x48,0x89,0x4c,0x24,0x08,0x55,0x53,0x56,0x57,0x41,0x54,
        0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x8d,0xac,0x24,0x58,0xff,0xff,0xff,0x48,0x81};
    static const unsigned char dispatcher[15] = {
        0xf6,0x41,0x68,0x02,0x75,0x09,0x48,0x8b,0x01,0xff,0x90,0xd0,0x02,0x00,0x00};
    static const unsigned char initial_check[13] = {
        0x44,0x38,0xa2,0xc4,0x00,0x00,0x00,0x0f,0x84,0xbb,0x13,0x00,0x00};
    unsigned char bytes[32]; uintptr_t slot = base + 0x11a2150 + 0x2d0;
    uintptr_t expected = base + 0x488700;
    if (!supported_image(base) || read_ptr(slot) != expected ||
        !read_at(expected, bytes, sizeof prologue) || memcmp(bytes, prologue, sizeof prologue) ||
        !read_at(base + 0x48bb6f, bytes, sizeof dispatcher) || memcmp(bytes, dispatcher, sizeof dispatcher) ||
        !read_at(base + 0x488754, bytes, sizeof initial_check) || memcmp(bytes, initial_check, sizeof initial_check)) {
        line("{\"initial_actor_trace\":\"not-installed\",\"reason\":\"exact-native-pins\"}\r\n"); return 0;
    }
    DWORD protection;
    if (!VirtualProtect((void*)slot, sizeof(uintptr_t), PAGE_READWRITE, &protection)) {
        line("{\"initial_actor_trace\":\"not-installed\",\"reason\":\"slot-protection\"}\r\n"); return 0;
    }
    initial_actor_trace_base = base; initial_actor_trace_thread = GetCurrentThreadId();
    initial_actor_original_bunch = (InitialActorReceivedBunch)expected;
    PVOID previous = InterlockedCompareExchangePointer((PVOID volatile*)slot,
        (PVOID)initial_actor_received_bunch, (PVOID)expected);
    DWORD ignored; VirtualProtect((void*)slot, sizeof(uintptr_t), protection, &ignored);
    if ((uintptr_t)previous != expected) {
        initial_actor_original_bunch = NULL;
        line("{\"initial_actor_trace\":\"not-installed\",\"reason\":\"slot-changed\"}\r\n"); return 0;
    }
    network_actor_received_bunch_observer = (uintptr_t)initial_actor_received_bunch;
    initial_actor_trace_installed = 1;
    initial_object_install(base);
    inventory_decode_install(base);
    line("{\"initial_actor_trace\":\"installed\",\"original_rva\":\"0x488700\",\"slot\":\"0x2d0\",\"limit\":128,\"unverified_position_fields_omitted\":true}\r\n");
    return 1;
}

#endif
