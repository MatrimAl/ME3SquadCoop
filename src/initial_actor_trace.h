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
    AmmoWireReceive wire_frame;ammo_wire_receive_begin(&wire_frame,channel);
    SetLastError(error_before);
    forward(self, bunch); /* Always exactly one original call, unchanged args. */
    DWORD error_after = GetLastError();
    ammo_wire_receive_end(&wire_frame);
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
    if (!progress_enabled || test_action != 5 || strcmp(test_role, "client")) return 0;
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
    line("{\"initial_actor_trace\":\"installed\",\"original_rva\":\"0x488700\",\"slot\":\"0x2d0\",\"limit\":128,\"unverified_position_fields_omitted\":true}\r\n");
    return 1;
}

#endif
