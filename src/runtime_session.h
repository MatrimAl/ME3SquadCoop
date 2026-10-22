#ifndef ME3_RUNTIME_SESSION_H
#define ME3_RUNTIME_SESSION_H
/* Ordinary-entry policy. Networking must already belong to the current
 * native world; this file never starts travel, synthesizes input or sets
 * story, permission, time, animation or replicated ownership flags. */
static int runtime_scope_ready;
static int runtime_control_enabled;
static int native_session_role(void) {
    return g_runtime_config.role==ME3_ROLE_HOST?6:g_runtime_config.role==ME3_ROLE_GUEST?5:0;
}
#include "native_remote_controller.h"
#include "network_actor_identity.h"
#include "squad_context_support.h"
#include "combat_context_support.h"
#include "native_state_reader.h"
#include "squad_pawn_readiness.h"
#include "guest_camera_adapter.h"
#include "guest_player_context_adapter.h"
#include "guest_cinematic_registration_adapter.h"
#include "guest_equipment_bootstrap.h"
#include "squad_session_core.h"

static int session_policy_is_host(void) {return g_runtime_config.role==ME3_ROLE_HOST;}
static int session_policy_enabled(void) {return runtime_control_enabled;}
static int session_policy_host_ready(void) {return runtime_scope_ready;}
static int session_policy_synthetic(DWORD first,DWORD now) {(void)first;(void)now;return 0;}
static int session_policy_walk_started(void) {return 0;}
static int session_policy_acquire_ready(DWORD now) {(void)now;return 1;}
static void session_policy_companion_far(uintptr_t base,uintptr_t pc,uintptr_t pawn,uintptr_t ai) {
    /* Wait for native squad following; no QA bot or catch-up experiment. */
    (void)base;(void)pc;(void)pawn;(void)ai;
}
static void session_policy_acquired(DWORD now) {(void)now;}
static void session_policy_held(uintptr_t base,uintptr_t pc,uintptr_t host_pawn,uintptr_t remote,
    uintptr_t connection,uintptr_t pawn,uintptr_t world,uint32_t epoch,int ordinary,DWORD now) {
    guest_equipment_bootstrap_tick(base,pc,host_pawn,remote,connection,pawn,world,epoch,
        ordinary,runtime_control_enabled,now);
}

/* Bind the driver through native GWorld -> PersistentLevel -> WorldInfo.
 * Pending worlds and drivers with a similarly named object are insufficient.
 * Invalidation leaves captured lease identities intact for safe cleanup. */
static int runtime_session_scope(uintptr_t base,uintptr_t engine) {
    if(g_runtime_config.role==ME3_ROLE_HOST&&!InterlockedCompareExchange(&runtime_host_start_gate,0,0))return 0;
    CombatContextChain first,last;combat_context_read_chain(base,&first);
    if(!first.info_valid||!network_actor_world_pins(base))return 0;
    SessionContext c=session_context(engine);unsigned char mode=255;
    uintptr_t driver=read_ptr(first.world+0x120);
    if(!session_actor_live(base,c.pc)||!is_kind_of(base,c.pc,"SFXPlayerController")||
       c.world!=first.info||!read_at(first.info+0x7c8,&mode,1)||
       mode!=(g_runtime_config.role==ME3_ROLE_HOST?2:3)||
       !is_live(base,driver)||!is_kind_of(base,driver,"NetDriver")||
       !runtime_transport_endpoint(base,driver,&g_runtime_config))return 0;
    if(g_runtime_config.role==ME3_ROLE_HOST) {
        GameArray peers={0};
        if(read_ptr(driver+0x80)||!read_at(driver+0x70,&peers,sizeof peers)||
           session_peer_array_member(peers,0)<0)return 0;
    } else {
        uintptr_t connection=read_ptr(driver+0x80);
        if(!is_live(base,connection)||!is_kind_of(base,connection,"NetConnection")||
           read_ptr(connection+0x8c)!=driver)return 0;
    }
    combat_context_read_chain(base,&last);
    if(!combat_context_chain_equal(&first,&last)||read_ptr(first.world+0x120)!=driver)return 0;
    identity_host_driver=g_runtime_config.role==ME3_ROLE_HOST?driver:0;
    return 1;
}
static void runtime_guest_frame(uintptr_t base,uintptr_t engine,int after,DWORD now) {
    SessionContext c=session_context(engine);
    uint32_t world_flags=0;
    int unpaused=c.world&&read_at(c.world+0x7c0,&world_flags,4)&&!(world_flags&0x400);
    int enabled=runtime_control_enabled&&runtime_scope_ready&&unpaused;
    int ordinary=enabled&&!session_engine_blocked(c);
    /* Resolve channels at a bounded cadence, rather than scanning every Tick.
     * Adapters independently validate their cached reciprocal identities. */
    static DWORD last_channels;
    if(enabled&&(!last_channels||now-last_channels>=1000)) {
        last_channels=now;
        if(session_actor_live(base,c.pawn))network_actor_channel_id(base,c.pawn);
    }
    if(!after)guest_cinematic_registration_adapt(base,engine,c.pc,c.world,enabled,now);
    guest_camera_adapt(base,c.pc,c.pawn,c.world,ordinary,enabled,now);
    guest_player_context_adapt(base,c.pc,c.pawn,c.world,ordinary,enabled,now);
}
static void runtime_session_frame(uintptr_t base,uintptr_t engine,int after,DWORD now) {
    session_engine=engine;session_game_thread=GetCurrentThreadId();
    runtime_scope_ready=runtime_session_scope(base,engine);
    if(g_runtime_config.role==ME3_ROLE_GUEST) {
        runtime_guest_frame(base,engine,after,now);return;
    }
    /* The core must see disable/world invalidation while a lease exists.
     * Its pending native-disconnect path continues proving actual removal. */
    if(session_active&&(!runtime_control_enabled||!runtime_scope_ready)) {
        session_release(base,session_context(engine),runtime_control_enabled?"world-change":"disabled",now);
        return;
    }
    session_host_frame(base,engine,after);
}
#endif
