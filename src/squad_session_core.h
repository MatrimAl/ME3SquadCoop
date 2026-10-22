#ifndef ME3_SQUAD_SESSION_CORE_H
#define ME3_SQUAD_SESSION_CORE_H
/* Entry policy is explicit: the core owns native possession and cleanup;
 * only the protected probe policy supplies bots, timed scenes or warmup. */
static int session_policy_is_host(void);
static int session_policy_enabled(void);
static int session_policy_host_ready(void);
static int session_policy_synthetic(DWORD first,DWORD now);
static int session_policy_walk_started(void);
static int session_policy_acquire_ready(DWORD now);
static void session_policy_companion_far(uintptr_t base,uintptr_t pc,uintptr_t pawn,uintptr_t ai);
static void session_policy_acquired(DWORD now);
static void session_policy_held(uintptr_t base,uintptr_t pc,uintptr_t host_pawn,uintptr_t remote,
    uintptr_t connection,uintptr_t pawn,uintptr_t world,uint32_t epoch,int ordinary,DWORD now);
/* Persistent companion lease. Native ownership is authoritative; epochs stop
   old owners being reused. Test-only synthetic gate is explicitly labelled. */
static int session_enabled = 0, session_active = 0, session_fault = 0;
static uint32_t session_epoch = 0;
static uintptr_t session_remote = 0, session_connection = 0, session_pawn = 0, session_ai = 0;
static uintptr_t session_host = 0, session_shepard = 0, session_world = 0, session_squad = 0;
static uintptr_t session_story_player = 0, session_story_initial = 0;
static DWORD session_acquired_at = 0, session_first_acquired = 0, session_find_at = 0, session_free_at = 0;
static int session_gate_last = -1, session_gate_source = -1, session_gate_mode = -1;
static int session_releasing = 0, session_release_failures = 0;
static const char *session_pending_reason = NULL;
typedef void (*SessionConnectionCleanup)(void *);
static SessionConnectionCleanup session_original_cleanup = NULL;
static uintptr_t session_engine = 0;
static DWORD session_game_thread = 0;
static int session_cleanup_pending = 0, session_disconnect_observed = 0;
typedef struct { uintptr_t pointer; int32_t index; uint64_t name; } SessionHandle;
static SessionHandle session_pc_handle = {0}, session_connection_handle = {0};
static SessionHandle session_pawn_handle = {0}, session_ai_handle = {0}, session_host_handle = {0};
static SessionHandle session_shepard_handle = {0}, session_world_handle = {0}, session_squad_handle = {0}, session_driver_handle = {0};
static SessionHandle session_capture_handle(uintptr_t pointer) {
    SessionHandle h = {0}; h.pointer = pointer; h.index = -1;
    if (!read_at(pointer + 8, &h.index, 4) || !read_at(pointer + 0x48, &h.name, 8)) h.pointer = 0;
    return h;
}
static int session_handle_matches(SessionHandle h) {
    int32_t index = -2; uint64_t name = 0;
    return h.pointer && read_at(h.pointer + 8, &index, 4) && index == h.index &&
           read_at(h.pointer + 0x48, &name, 8) && name == h.name;
}
typedef struct {
    uintptr_t pc, pawn, world, squad, manager;
    unsigned char mode;
} SessionContext;
static SessionContext session_context(uintptr_t engine) {
    SessionContext c = {0}; c.mode = 255;
    c.pc = local_controller(engine);
    if (c.pc) { c.pawn = read_ptr(c.pc + 0x3bc); c.world = read_ptr(c.pc + 0x198); c.manager = read_ptr(c.pc + 0xc38); }
    if (c.world) c.squad = read_ptr(c.world + 0x8e8);
    if (c.manager) read_at(c.manager + 0xa8, &c.mode, 1);
    return c;
}
static int session_actor_live(uintptr_t base, uintptr_t object) {
    uint32_t first = 0, second = 0; uint64_t flags = 0;
    return is_live(base, object) && read_at(object + 0xc, &flags, 8) && !(flags & UINT64_C(0x2000000000000000)) &&
           read_at(object + 0x25c, &first, 4) && !(first & 8) &&
           read_at(object + 0x260, &second, 4) && !(second & 0x800000);
}
/* -1 means unknown: it must never be reported as observed disconnection. */
static int session_peer_array_member(GameArray peers, uintptr_t connection) {
    if (peers.count < 0 || peers.count > 32 || peers.max < peers.count || peers.max > 500000 ||
        (peers.count && (!peers.data || (peers.data & 7)))) return -1;
    int found = 0;
    for (int i = 0; i < peers.count; ++i) {
        uintptr_t peer = 0;
        if (!read_at(peers.data + (SIZE_T)i * 8, &peer, 8)) return -1;
        found += peer == connection;
    }
    return found > 1 ? -1 : found;
}
static int session_connection_member(uintptr_t base, uintptr_t connection) {
    GameArray peers = {0};
    if (!is_live(base, identity_host_driver) ||
        (session_driver_handle.pointer && (session_driver_handle.pointer != identity_host_driver || !session_handle_matches(session_driver_handle))) ||
        !read_at(identity_host_driver + 0x70, &peers, sizeof peers)) return -1;
    return session_peer_array_member(peers, connection);
}
static int session_peer_connected(uintptr_t base, uintptr_t remote, uintptr_t world) {
    if (!session_actor_live(base, remote) || !is_kind_of(base, remote, "BioPlayerController") || read_ptr(remote + 0x198) != world) return 0;
    uint32_t pc_flags = 0;
    if (!read_at(remote + 0x77c, &pc_flags, 4) || (pc_flags & 0x800)) return 0;
    uintptr_t connection = read_ptr(remote + 0x67c);
    if (!is_live(base, connection) || !is_kind_of(base, connection, "NetConnection") || read_ptr(connection + 0x68) != remote) return 0;
    return session_connection_member(base, connection) == 1;
}
static int session_story_valid(uintptr_t base) {
    return session_handle_matches(session_host_handle) && session_handle_matches(session_shepard_handle) &&
           session_handle_matches(session_squad_handle) && session_actor_live(base, session_host) && session_actor_live(base, session_shepard) && is_live(base, session_squad) &&
           read_ptr(session_host + 0x3bc) == session_shepard && read_ptr(session_shepard + 0x374) == session_host &&
           read_ptr(session_squad + 0x2cc) == session_story_player && read_ptr(session_squad + 0x2d4) == session_story_initial;
}
static int session_engine_blocked(SessionContext c) {
    /* Only ordinary on-foot mode has been verified. Other modes are a
       conservative handoff to the game, not a claim of vehicle support. */
    if (!c.pc || !c.world || !c.manager || c.mode != 0) return 1;
    uint32_t flags = 0; read_at(c.pc + 0x77c, &flags, 4);
    if (flags & 0x20000) return 1; /* PlayerController.bCinematicMode */
    /* Exact-build native message-box paths can set WorldInfo.bPlayersOnly;
       the mesh animation tick then deliberately stops. Yield through normal
       lease cleanup until the engine restores control. Never clear this bit. */
    uint32_t world_flags=0;
    if(!read_at(c.world+0x7c0,&world_flags,4)||(world_flags&0x400))return 1;
    uintptr_t conversation = read_ptr(c.world + 0x948);
    if (conversation) {
        int32_t active = 0; uint32_t activating = 0;
        read_at(conversation + 0xd0, &active, 4); read_at(conversation + 0xe8, &activating, 4);
        if (active > 0 || (activating & 1)) return 1;
    }
    return 0;
}
static int session_gate_selftest(void) {
    unsigned char pc[0x800]={0},world[0x960]={0},manager[16]={0},conversation[0xf0]={0};
    SessionContext c={0};c.pc=(uintptr_t)pc;c.world=(uintptr_t)world;c.manager=(uintptr_t)manager;c.mode=0;
    if(session_engine_blocked(c))return 0;
    uint32_t value=0x400;memcpy(world+0x7c0,&value,4);if(!session_engine_blocked(c))return 0;
    value=0;memcpy(world+0x7c0,&value,4);if(session_engine_blocked(c))return 0;
    uintptr_t original=c.world;c.world=1;if(!session_engine_blocked(c))return 0;c.world=original;
    c.mode=8;if(!session_engine_blocked(c))return 0;c.mode=0;
    value=0x20000;memcpy(pc+0x77c,&value,4);if(!session_engine_blocked(c))return 0;
    value=0;memcpy(pc+0x77c,&value,4);
    uintptr_t cm=(uintptr_t)conversation;memcpy(world+0x948,&cm,8);
    value=1;memcpy(conversation+0xd0,&value,4);if(!session_engine_blocked(c))return 0;
    value=0;memcpy(conversation+0xd0,&value,4);value=1;memcpy(conversation+0xe8,&value,4);
    return session_engine_blocked(c);
}
static void session_gate(int blocked, int synthetic, unsigned char mode, DWORD now) {
    if (blocked == session_gate_last && synthetic == session_gate_source && mode == session_gate_mode) return;
    session_gate_last = blocked; session_gate_source = synthetic; session_gate_mode = mode;
    char text[400]; snprintf(text, sizeof text,
        "{\"session\":\"gate-observation\",\"blocked\":%s,\"source\":\"%s\",\"reason\":\"scene\",\"mode\":%u,\"monotonic_ms\":%lu}\r\n",
        blocked ? "true" : "false", synthetic ? "synthetic" : "engine", mode, now); line(text);
}
static void session_release(uintptr_t base, SessionContext c, const char *reason, DWORD now) {
    if (!session_active || session_releasing) return;
    session_releasing = 1;
    unsigned char scratch[64] = {0};
    int same_world = c.world == session_world && session_handle_matches(session_world_handle) && is_live(base, session_world);
    int pawn_live = same_world && session_handle_matches(session_pawn_handle) && session_actor_live(base, session_pawn);
    int pc_live = same_world && session_handle_matches(session_pc_handle) && session_actor_live(base, session_remote);
    if (pc_live && pawn_live && read_ptr(session_remote + 0x3bc) == session_pawn)
        invoke(base, session_remote, "PlayerController", "UnPossess", scratch);
    uintptr_t owner = pawn_live ? read_ptr(session_pawn + 0x374) : 0;
    int detached = !pc_live || read_ptr(session_remote + 0x3bc) != session_pawn;
    int ai_live = session_handle_matches(session_ai_handle) && session_actor_live(base, session_ai);
    if (pawn_live && detached && !owner && ai_live &&
        (!read_ptr(session_ai + 0x3bc) || read_ptr(session_ai + 0x3bc) == session_pawn)) {
        struct { uintptr_t pawn; uint32_t vehicle; } params = {session_pawn, 0};
        invoke(base, session_ai, "BioAiController", "Possess", &params);
        owner = read_ptr(session_pawn + 0x374);
    }
    int ai = pawn_live && ai_live && owner == session_ai && read_ptr(session_ai + 0x3bc) == session_pawn;
    int story = session_story_valid(base);
    /* The pre-cleanup handoff is not itself a disconnected observation. The
       original native cleanup must remove actual driver membership first. */
    if (session_cleanup_pending && (!session_disconnect_observed || session_connection_member(base, session_connection) != 0)) {
        session_pending_reason = reason;
        session_releasing = 0;
        return;
    }
    if (!ai && same_world && pawn_live && session_release_failures++ < 5) {
        session_pending_reason = reason;
        session_releasing = 0;
        return; /* Engine may finish retiring the old controller this frame. */
    }
    char text[650]; snprintf(text, sizeof text,
        "{\"session\":\"released\",\"epoch\":%u,\"reason\":\"%s\",\"ai_restored\":%s,\"host_story_preserved\":%s,\"remote_detached\":%s,\"world_unchanged\":%s,\"pawn_live\":%s,\"saved_ai_live\":%s,\"remote_pc_live\":%s,\"monotonic_ms\":%lu}\r\n",
        session_epoch, reason, ai ? "true" : "false", story ? "true" : "false", detached ? "true" : "false", same_world ? "true" : "false",
        pawn_live ? "true" : "false", ai_live ? "true" : "false", pc_live ? "true" : "false", now); line(text);
    session_active = 0; session_releasing = 0; session_pending_reason = NULL; session_release_failures = 0;
    session_cleanup_pending = 0;
    if (!ai || !story || !detached) session_fault = 1;
    session_remote = 0; session_connection = 0; session_free_at = 0;
}
static void session_connection_cleanup(void *self) {
    uintptr_t base = (uintptr_t)GetModuleHandleW(NULL);
    if (session_enabled && session_policy_is_host() && session_active && !session_releasing &&
        (uintptr_t)self == session_connection && GetCurrentThreadId() == session_game_thread &&
        session_handle_matches(session_pc_handle) && session_handle_matches(session_connection_handle) &&
        read_ptr(session_connection + 0x68) == session_remote && read_ptr(session_remote + 0x67c) == session_connection) {
        session_cleanup_pending = 1;
        session_pending_reason = "disconnect";
        DWORD now = GetTickCount();
        session_release(base, session_context(session_engine), "disconnect", now);
        char text[350]; snprintf(text, sizeof text,
            "{\"session\":\"cleanup-handoff\",\"epoch\":%u,\"source\":\"native-connection-cleanup\",\"ai_restored\":%s,\"host_story_preserved\":%s,\"monotonic_ms\":%lu}\r\n",
            session_epoch, read_ptr(session_pawn + 0x374) == session_ai && read_ptr(session_ai + 0x3bc) == session_pawn ? "true" : "false",
            session_story_valid(base) ? "true" : "false", now); line(text);
    }
    /* Always preserve the engine's connection cleanup, including failures. */
    session_original_cleanup(self);
}
static int session_install_cleanup(uintptr_t base, uintptr_t connection) {
    const unsigned char expected[16] = {0x40,0x53,0x56,0x57,0x41,0x57,0x48,0x83,0xec,0x28,0x45,0x33,0xff,0x4c,0x89,0x6c};
    unsigned char code[16];
    uintptr_t vt = read_ptr(connection), target = base + 0x47eae0;
    if (vt != base + 0x12bc620 || !read_at(target, code, sizeof code) || memcmp(code, expected, sizeof code)) return 0;
    uintptr_t slot = vt + 0x328;
    if (session_original_cleanup) return read_ptr(slot) == (uintptr_t)session_connection_cleanup;
    if (read_ptr(slot) != target) return 0;
    DWORD old;
    if (!VirtualProtect((void*)slot, 8, PAGE_READWRITE, &old)) return 0;
    session_original_cleanup = (SessionConnectionCleanup)target;
    PVOID previous = InterlockedCompareExchangePointer((PVOID volatile*)slot, (PVOID)session_connection_cleanup, (PVOID)target);
    VirtualProtect((void*)slot, 8, old, &old);
    if ((uintptr_t)previous != target) { session_original_cleanup = NULL; return 0; }
    line("{\"session\":\"cleanup-hook-installed\",\"vtable_rva\":\"0x12bc620\",\"slot\":808,\"target_rva\":\"0x47eae0\",\"prologue_verified\":true}\r\n");
    return 1;
}
static void session_acquire_wait(uintptr_t base,SessionContext c,const char *reason,uintptr_t pawn,uintptr_t ai,DWORD now) {
    static DWORD last;static const char *previous;
    if(previous&&!strcmp(previous,reason)&&now-last<5000)return;
    previous=reason;last=now;
    char ai_state[96],row[700];progress_state_name(base,ai,ai_state,sizeof ai_state);
    int32_t action=-1,power=-1;unsigned char ignored=255;
    read_at(pawn+0xc20,&action,4);read_at(pawn+0xc28,&power,4);read_at(c.pc+0x781,&ignored,1);
    uintptr_t camera=read_ptr(c.pc+0x684);
    snprintf(row,sizeof row,"{\"session\":\"acquisition-wait\",\"reason\":\"%s\",\"ai_state\":\"%s\",\"action\":%d,\"power_action\":%d,\"ignore_move\":%u,\"camera_targets_host\":%s,\"host_walk_started\":%s,\"read_only\":true,\"monotonic_ms\":%lu}\r\n",
        reason,ai_state,action,power,ignored,read_ptr(camera+0x3f4)==c.pawn?"true":"false",session_policy_walk_started()?"true":"false",now);line(row);
}
static void session_acquire(uintptr_t base, SessionContext c, DWORD now) {
    if (!session_actor_live(base, c.pawn) || !is_kind_of(base, c.pawn, "SFXPawn_Player") || !is_live(base, c.squad)) return;
    if (!session_peer_connected(base, session_remote, c.world) || read_ptr(session_remote + 0x3bc)) return;
    if (!session_install_cleanup(base, read_ptr(session_remote + 0x67c))) {
        session_fault = 1;
        line("{\"session\":\"cleanup-hook-refused\",\"acquisition_skipped\":true}\r\n"); return;
    }
    uint32_t allows = 0;
    if (!invoke(base, c.manager, "SFXGameModeManager", "AllowsMovement", &allows) || !(allows & 1)) {
        session_acquire_wait(base,c,"movement-not-allowed",0,0,now);return;
    }
    GameArray members = {0}; if (!identity_array(c.squad + 0x270, &members, 4)) return;
    uintptr_t ashley = 0;
    for (int i = 0; i < members.count; ++i) {
        uintptr_t candidate = read_ptr(members.data + (SIZE_T)i * 8);
        if (candidate != c.pawn && is_kind_of(base, candidate, "SFXPawn_Ashley")) ashley = candidate;
    }
    uintptr_t ai = ashley ? read_ptr(ashley + 0x374) : 0;
    if (!session_actor_live(base, ashley) || !session_actor_live(base, ai) || !is_kind_of(base, ai, "BioAiController") || read_ptr(ai + 0x3bc) != ashley) {
        session_acquire_wait(base,c,"companion-or-ai-not-ready",ashley,ai,now);return;
    }
    if (!progress_pawn_ready(base, ashley, ai)) {
        session_acquire_wait(base,c,"companion-action-or-physics",ashley,ai,now);return;
    }
    if (!progress_companion_near(c.pawn, ashley)) {
        session_policy_companion_far(base,c.pc,ashley,ai);
        session_acquire_wait(base,c,"companion-out-of-range",ashley,ai,now);
        return; /* An actor stranded in an intro set is not a playable guest. */
    }
    if (!session_policy_acquire_ready(now)) {
        session_acquire_wait(base,c,"host-input-warmup",ashley,ai,now);return;
    }
    session_host = c.pc; session_shepard = c.pawn; session_world = c.world; session_squad = c.squad;
    session_story_player = read_ptr(c.squad + 0x2cc); session_story_initial = read_ptr(c.squad + 0x2d4);
    session_pawn = ashley; session_ai = ai; session_connection = read_ptr(session_remote + 0x67c);
    session_pc_handle = session_capture_handle(session_remote);
    session_connection_handle = session_capture_handle(session_connection);
    session_pawn_handle = session_capture_handle(ashley); session_ai_handle = session_capture_handle(ai);
    session_host_handle = session_capture_handle(c.pc); session_shepard_handle = session_capture_handle(c.pawn);
    session_world_handle = session_capture_handle(c.world); session_squad_handle = session_capture_handle(c.squad);
    session_driver_handle = session_capture_handle(identity_host_driver);
    unsigned char scratch[64] = {0};
    if (!invoke(base, ai, "BioAiController", "UnPossess", scratch)) return;
    struct { uintptr_t pawn; uint32_t vehicle; } params = {ashley, 0};
    int dispatched = invoke(base, session_remote, "PlayerController", "Possess", &params);
    int owned = dispatched && read_ptr(session_remote + 0x3bc) == ashley && read_ptr(ashley + 0x374) == session_remote;
    session_epoch++; session_active = 1; session_acquired_at = now; session_disconnect_observed = 0;
    if (!session_first_acquired) session_first_acquired = now;
    session_policy_acquired(now);
    int story = session_story_valid(base);
    char text[480]; snprintf(text, sizeof text,
        "{\"session\":\"acquired\",\"epoch\":%u,\"remote_owns_ashley\":%s,\"host_story_preserved\":%s,\"monotonic_ms\":%lu}\r\n",
        session_epoch, owned ? "true" : "false", story ? "true" : "false", now); line(text);
    if (!owned || !story) { session_fault = 1; session_release(base, c, "ownership-lost", GetTickCount()); }
}
static void session_host_frame(uintptr_t base, uintptr_t engine, int after) {
    if (!session_enabled) return;
    session_engine = engine; session_game_thread = GetCurrentThreadId();
    if (!session_policy_is_host() || !session_policy_host_ready()) return;
    SessionContext c = session_context(engine); DWORD now = GetTickCount();
    if (session_pending_reason) {
        if (session_cleanup_pending && session_connection_member(base, session_connection) == 0 && !session_disconnect_observed) {
            char text[320]; snprintf(text, sizeof text,
                "{\"session\":\"disconnect-observation\",\"epoch\":%u,\"connected\":false,\"source\":\"engine\",\"evidence\":\"driver-membership-removed\",\"monotonic_ms\":%lu}\r\n", session_epoch, now); line(text);
            session_disconnect_observed = 1;
        }
        session_release(base, c, session_pending_reason, now); return;
    }
    int synthetic = session_policy_synthetic(session_first_acquired,now);
    int engine_gate = session_engine_blocked(c), blocked = engine_gate || synthetic;
    session_gate(blocked, synthetic && !engine_gate, c.mode, now);
    if (session_active) {
        if (c.pc != session_host || c.world != session_world || c.squad != session_squad ||
            !session_handle_matches(session_world_handle) || !session_handle_matches(session_pawn_handle) || !session_handle_matches(session_ai_handle)) {
            session_fault = 1; session_release(base, c, "ownership-lost", now); return;
        }
        if (!session_handle_matches(session_pc_handle) || !session_handle_matches(session_connection_handle) ||
            read_ptr(session_remote + 0x67c) != session_connection || !session_peer_connected(base, session_remote, session_world)) {
            /* Detach conservatively, but let the pending path prove removal
               from a readable driver array before any disconnect claim. */
            session_cleanup_pending = 1; session_pending_reason = "disconnect";
            session_release(base, c, "disconnect", now); return;
        }
        if (!session_policy_enabled()) { session_release(base, c, "disabled", now); return; }
        if (blocked) { session_release(base, c, "scene", now); return; }
        if (!session_story_valid(base) || read_ptr(session_remote + 0x3bc) != session_pawn || read_ptr(session_pawn + 0x374) != session_remote) {
            session_fault = 1; session_release(base, c, "ownership-lost", now); return;
        }
        if(after)session_policy_held(base,c.pc,c.pawn,session_remote,session_connection,
            session_pawn,c.world,session_epoch,!blocked,now);
        return;
    }
    if (session_fault || blocked || !session_policy_enabled() || !c.pc || !c.pawn || !c.squad) { session_free_at = 0; return; }
    if (!after) return;
    if (!session_free_at) session_free_at = now;
    if (now - session_free_at < 500) return;
    if (!session_peer_connected(base, session_remote, c.world)) {
        session_remote = 0;
        if (now - session_find_at < 1000) return;
        session_find_at = now; session_remote = pair_find_remote(base, c.pc, c.world);
    }
    if (session_remote) session_acquire(base, c, now);
}
#endif
