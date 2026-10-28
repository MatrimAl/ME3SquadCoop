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
#define ME3_RUNTIME_LOCAL_SQUAD_SET
#include "guest_player_context_adapter.h"
#undef ME3_RUNTIME_LOCAL_SQUAD_SET
#include "guest_power_order_contract.h"
#define ME3_ORDINARY_COLLISION_RESTORE
#include "guest_collision_restore.h"
#undef ME3_ORDINARY_COLLISION_RESTORE
#include "guest_cinematic_registration_adapter.h"
#include "guest_equipment_bootstrap.h"
#include "squad_session_core.h"
static int runtime_startup_focus_pending(void);
#define ME3_ORDINARY_STARTUP_FOCUS
#include "background_focus.h"
#undef ME3_ORDINARY_STARTUP_FOCUS
#include "runtime_rpc_receive.h"

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
#include "runtime_guest_power.h"
#include "runtime_power_icons.h"
#include "runtime_power_menu.h"
#include "runtime_power_wheel_context.h"
#include "runtime_power_assignment.h"
#include "runtime_power_caster.h"
#include "runtime_native_quickslot.h"
#include "runtime_input_bootstrap.h"
#include "runtime_command_pause.h"
#include "runtime_revive_prepare.h"
#ifdef ME3_ORDINARY_FIRE_QA
/* Compiled only into the private lab candidate, never normal player ASIs. */
static int ordinary_fire_qa_allowed(void) {
    wchar_t enabled[4]={0},marker[MAX_PATH];
    if(g_runtime_config.role!=ME3_ROLE_GUEST||GetEnvironmentVariableW(L"ME3_SQUAD_COOP_ORDINARY_FIRE_QA",enabled,4)!=1||enabled[0]!=L'1')return 0;
    int n=_snwprintf(marker,MAX_PATH,L"%ls\\.me3-coop-sandbox.json",g_game_root);
    return n>0&&n<MAX_PATH&&GetFileAttributesW(marker)!=INVALID_FILE_ATTRIBUTES;
}
#include "combat_fire_bot.h"
#endif
#define ME3_REMOTE_APPEARANCE_ORDINARY
#include "remote_appearance_qa.h"
#undef ME3_REMOTE_APPEARANCE_ORDINARY
#include "runtime_remote_helmet.h"
#include "remote_weapon_presentation.h"
#include "runtime_presentation_rpc.h"
#include "runtime_revive_peer.h"
#include "runtime_native_hud.h"
#include "runtime_guest_vitals_hud.h"
#include "runtime_story_warning.h"
#include "runtime_team_hud.h"
#include "runtime_story_gate.h"
#include "runtime_ambient_conversation.h"
#include "runtime_guest_team.h"
#include "runtime_guest_weapon.h"
#include "runtime_guest_shield_feedback.h"
#include "runtime_power_action_context.h"
#include "runtime_guest_movement.h"
#include "runtime_guest_swap.h"
static SessionContext runtime_host_power_context(uintptr_t engine) {
    SessionContext c=session_context(engine);
    c.pc=session_remote;c.pawn=session_pawn;return c;
}
static void runtime_host_power_context_tick(uintptr_t base,uintptr_t engine) {
    SessionContext host=session_context(engine),remote=runtime_host_power_context(engine);
    int enabled=runtime_control_enabled&&runtime_scope_ready;
    int ordinary=enabled&&!session_engine_blocked(host);
    runtime_power_action_context_retire(base,remote,ordinary,enabled,0);
    runtime_power_action_context_prepare(base,remote,ordinary,enabled);
}
static void runtime_before_host_control_release(uintptr_t base,uintptr_t engine) {
    /* Conditional cache cleanup never delays the core's story/AI handoff.
     * Busy actions retain pending cleanup until their ordinary idle state. */
    runtime_guest_weapon_retire(base,engine,GetTickCount());
    runtime_guest_movement_retire(base,engine,GetTickCount());
    runtime_power_action_context_retire(base,runtime_host_power_context(engine),0,0,0);
    if(runtime_revive_opted())runtime_revive_before_host_release(base,engine);
}
static void runtime_guest_frame(uintptr_t base,uintptr_t engine,int after,DWORD now) {
    SessionContext c=session_context(engine);
    uint32_t world_flags=0;
    int unpaused=c.world&&read_at(c.world+0x7c0,&world_flags,4)&&!(world_flags&0x400);
    int enabled=runtime_control_enabled&&runtime_scope_ready&&unpaused;
    int ordinary=enabled&&!session_engine_blocked(c);
    /* Poll authored preparation/retirement before context release. Admission
     * is opened only by the peer agreement on the after-Tick path. */
    if(!after) {
        unsigned before=runtime_revive_dispatch_generation;
        runtime_revive_prepare_tick(base,engine,c,ordinary,now);
        if(before!=runtime_revive_dispatch_generation) {
            /* Spawn/Status/retirement can re-enter native code. Refresh the
             * whole transport/world gate, not just the controller pointer. */
            runtime_scope_ready=runtime_session_scope(base,engine);
            c=session_context(engine);world_flags=0;
            unpaused=c.world&&read_at(c.world+0x7c0,&world_flags,4)&&!(world_flags&0x400);
            enabled=runtime_control_enabled&&runtime_scope_ready&&unpaused;
            ordinary=enabled&&!session_engine_blocked(c);
        }
    }
    /* Resolve channels at a bounded cadence, rather than scanning every Tick.
     * Adapters independently validate their cached reciprocal identities. */
    static DWORD last_channels;
    if(enabled&&(!last_channels||now-last_channels>=1000)) {
        last_channels=now;
        if(session_actor_live(base,c.pawn))network_actor_channel_id(base,c.pawn);
    }
    if(!after)guest_cinematic_registration_adapt(base,engine,c.pc,c.world,enabled,now);
    guest_camera_adapt(base,c.pc,c.pawn,c.world,ordinary,enabled,now);
    runtime_power_action_context_retire(base,c,ordinary,enabled,runtime_power_menu_context_held(base,c));
    /* Context retention is independent of optional icon initialization.
     * This admits no orders and never rebuilds context inside the menu. */
    if(!runtime_power_menu_context_held(base,c))
        guest_player_context_adapt(base,c.pc,c.pawn,c.world,ordinary,enabled,now);
    /* Native registration can change context. Bind only the resulting
     * current idle action instances before normal power input is consumed. */
    c=session_context(engine);
    ordinary=enabled&&!session_engine_blocked(c);
    if(!after)runtime_power_action_context_prepare(base,c,ordinary,enabled);
    /* A scene/PlayersOnly interval suspends writes, not the captured healthy
     * collision lifetime. Keep observing the same pawn while it is released;
     * only an actual session disable/scope loss retires that baseline. */
    guest_collision_adapt(base,c.pc,c.pawn,c.world,ordinary,
        runtime_control_enabled&&runtime_scope_ready,now);
#ifdef ME3_ORDINARY_FIRE_QA
    combat_fire_bot_tick(base,c.pc,c.pawn,c.world,ordinary,enabled,after,now);
#endif
    if(!after)runtime_native_hud_tick(base,engine,now);
    if(!after)runtime_input_bootstrap_tick(base,engine,now);
    if(!after)runtime_power_wheel_context_tick(base,engine);
    if(!after)runtime_power_icons_tick(base,engine,ordinary);
    /* Collision repair, input initialization and icon updates can invoke native code. Resolve the current control
     * context again before considering a power order. Keep the order last:
     * its ProcessEvent may start an action or change the world/ownership, so
     * no later adapter may consume this frame's pre-order context. */
    c=session_context(engine);
    ordinary=runtime_control_enabled&&runtime_scope_ready&&!session_engine_blocked(c);
    runtime_guest_power_tick(base,engine,c,ordinary,after,now);
    if(after) {
        /* Appearance loading may invoke native code. Run last with a fresh
         * connection and local context; no later adapter reuses this frame. */
        c=session_context(engine);
        remote_appearance_qa_tick(base,engine,c.pc,c.pawn,c.world,!session_engine_blocked(c),
            runtime_control_enabled&&runtime_session_scope(base,engine),GetTickCount());
        c=session_context(engine);
        runtime_scope_ready=runtime_session_scope(base,engine);
        remote_weapon_presentation_tick(base,c.pc,c.pawn,c.world,!session_engine_blocked(c),
            runtime_control_enabled&&runtime_scope_ready,GetTickCount());
        runtime_scope_ready=runtime_session_scope(base,engine);
        runtime_presentation_tick(base,engine,GetTickCount());
        runtime_revive_peer_tick(base,engine,GetTickCount());
        runtime_native_hud_tick(base,engine,GetTickCount());
        runtime_guest_vitals_tick(base,engine,GetTickCount());
    }
}
static void runtime_session_frame(uintptr_t base,uintptr_t engine,int after,DWORD now) {
    session_engine=engine;session_game_thread=GetCurrentThreadId();
    runtime_scope_ready=runtime_session_scope(base,engine);
    runtime_ambient_conversation_configure(base);
    if(!after)runtime_guest_weapon_tick(base,engine,now);
    if(!after)runtime_guest_movement_tick(base,engine,now);
    if(!after&&runtime_guest_team_tick(base,engine,now))runtime_scope_ready=runtime_session_scope(base,engine);
    runtime_revive_peer_configure();
    runtime_team_hud_configure();
    if(!after)runtime_rpc_receive_prepare(base);
    if(!after){runtime_story_gate_tick(base,engine,now);runtime_scope_ready=runtime_session_scope(base,engine);}
    runtime_command_pause_prepare(base,engine,runtime_control_enabled&&runtime_scope_ready);
    session_command_menu_retained=runtime_host_command_menu_held;
    if(!after)background_focus_prepare(base,engine,runtime_control_enabled&&
        (runtime_scope_ready||runtime_startup_focus_pending()));
    if(g_runtime_config.role==ME3_ROLE_GUEST) {
        runtime_guest_frame(base,engine,after,now);
        runtime_scope_ready=runtime_session_scope(base,engine);
        runtime_guest_weapon_tick(base,engine,GetTickCount());
        runtime_guest_movement_tick(base,engine,GetTickCount());
        if(after)runtime_guest_shield_feedback_tick(base,engine,GetTickCount());
        if(after)runtime_guest_swap_prepare(base,engine,GetTickCount());
        if(after)runtime_layout_cache_flush(GetTickCount());
        return;
    }
    session_before_control_release=runtime_before_host_control_release;
    if(!after){runtime_host_power_context_tick(base,engine);runtime_revive_host_tick(base,engine,now);}
    /* The core must see disable/world invalidation while a lease exists.
     * Its pending native-disconnect path continues proving actual removal. */
    if(session_active&&(!runtime_control_enabled||!runtime_scope_ready)) {
        session_release(base,session_context(engine),runtime_control_enabled?"world-change":"disabled",now);
        return;
    }
    session_host_frame(base,engine,after);
    if(after) {
        runtime_scope_ready=runtime_session_scope(base,engine);
        if(runtime_guest_team_tick(base,engine,GetTickCount()))runtime_scope_ready=runtime_session_scope(base,engine);
        runtime_guest_weapon_tick(base,engine,GetTickCount());
        runtime_guest_movement_tick(base,engine,GetTickCount());
        runtime_host_power_context_tick(base,engine);
        runtime_presentation_tick(base,engine,GetTickCount());
        runtime_revive_peer_tick(base,engine,GetTickCount());
        runtime_guest_vitals_tick(base,engine,GetTickCount());
        runtime_layout_cache_flush(GetTickCount());
    }
}
#endif
