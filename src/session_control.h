#include "squad_session_core.h"
#include "session_admission_diagnostics.h"
#include "background_focus.h"
/* Protected probe orchestration. These policies do not enter the player ASI. */
static int combat_action_opted(void);
static int session_manual_guest_input(void) {
    static int loaded,value;
    if(!loaded){wchar_t text[8]={0};loaded=1;value=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_MANUAL_GUEST_QA",text,8)==1&&text[0]==L'1';}
    return value&&progress_enabled&&has_marker(root);
}
static int session_policy_is_host(void) {return test_action==6;}
static int session_policy_enabled(void) {
    return has_marker(root);
}
static int session_policy_host_ready(void) {return host_map_refreshed;}
static int session_policy_synthetic(DWORD first,DWORD now) {
    /* The normal-action gameplay trial follows real scene gates only. A
     * synthetic release made the detached guest fall before reacquisition. */
    if(combat_action_opted())return 0;
    DWORD at=progress_enabled?30000:18000;return first&&now-first>=at&&now-first<at+3000;
}
static int session_policy_walk_started(void) {return progress_walk_at!=0;}
static int session_policy_acquire_ready(DWORD now) {return !progress_enabled||(progress_walk_at&&now-progress_walk_at>=14000);}
static void session_policy_companion_far(uintptr_t base,uintptr_t pc,uintptr_t pawn,uintptr_t ai) {
    progress_follow(base,pc,pawn);progress_native_catchup(base,pc,pawn,ai);
}
static void session_policy_acquired(DWORD now) {if(progress_enabled&&!progress_lease_at)progress_lease_at=now;}
static void session_policy_held(uintptr_t base,uintptr_t pc,uintptr_t host_pawn,uintptr_t remote,
    uintptr_t connection,uintptr_t pawn,uintptr_t world,uint32_t epoch,int ordinary,DWORD now) {
    guest_equipment_bootstrap_tick(base,pc,host_pawn,remote,connection,pawn,world,epoch,
        ordinary,progress_enabled&&has_marker(root),now);
    ammo_change_sample(base,remote,pawn,world,epoch,now);
}
static uintptr_t session_guest_last_pawn = 0;
static uint32_t session_guest_epoch = 0;
static DWORD session_guest_at = 0, session_guest_first_at = 0;
static int session_disconnect_sent = 0;
static DWORD session_guest_disconnect_delay(int progress,int action) {
    return progress?(action==1?150000:60000):40000;
}
static void session_guest_frame(uintptr_t base, uintptr_t engine, int after) {
    SessionContext c = session_context(engine); DWORD now = GetTickCount();
    if(after)remote_appearance_qa_tick(base,c.pc,c.pawn,c.world,!session_engine_blocked(c),progress_enabled&&has_marker(root),now);
    if(!after)guest_cinematic_registration_adapt(base,engine,c.pc,c.world,progress_enabled&&has_marker(root),now);
    combat_fire_bot_tick(base,c.pc,c.pawn,c.world,!session_engine_blocked(c),progress_enabled&&has_marker(root)&&!collision_baseline_qa_opted()&&!session_manual_guest_input(),after,now);
    guest_camera_adapt(base,c.pc,c.pawn,c.world,!session_engine_blocked(c),progress_enabled&&has_marker(root),now);
    guest_player_context_adapt(base,c.pc,c.pawn,c.world,!session_engine_blocked(c),progress_enabled&&has_marker(root),now);
    guest_collision_adapt(base,c.pc,c.pawn,c.world,!session_engine_blocked(c),progress_enabled&&has_marker(root),now);
    if(after&&!session_engine_blocked(c))ammo_change_sample(base,c.pc,c.pawn,c.world,guest_player_context_lease.token,now);
    int ashley = session_actor_live(base, c.pawn) && is_kind_of(base, c.pawn, "SFXPawn_Ashley");
    if (ashley && session_guest_last_pawn != c.pawn) {
        session_guest_epoch++; session_guest_at = now;
        if (!session_guest_first_at) session_guest_first_at = now;
    }
    session_guest_last_pawn = ashley ? c.pawn : 0;
    if (!after) {
        if (!session_manual_guest_input()&&ashley&&(!collision_baseline_qa_opted()||session_guest_epoch>=2)) {
            uintptr_t input = read_ptr(c.pc + 0x6dc);
            if (is_live(base, input) && is_kind_of(base, input, "PlayerInput")) {
                DWORD elapsed = now - session_guest_at;
                float value = elapsed >= 2000 && elapsed < 4000 && !session_disconnect_sent ? 1.0f : 0.0f; SIZE_T wrote;
                WriteProcessMemory(GetCurrentProcess(), (void *)(input + 0x194), &value, 4, &wrote);
                WriteProcessMemory(GetCurrentProcess(), (void *)(input + 0x1a4), &value, 4, &wrote);
                if (!session_disconnect_sent&&!collision_baseline_qa_opted()) progress_guest_follow_input(base,c.pc,c.pawn,input,elapsed);
            }
        }
    } else if (!session_manual_guest_input() && session_guest_first_at && now - session_guest_first_at >= session_guest_disconnect_delay(progress_enabled,combat_action_opted()) && !session_disconnect_sent && c.pc) {
        session_disconnect_sent = 1;
        char text[250]; snprintf(text, sizeof text,
            "{\"session\":\"disconnect-request\",\"source\":\"ordinary-engine-console\",\"monotonic_ms\":%lu}\r\n", now); line(text);
        /* Release the guest-only native context while this world/PC is still
           current. Native travel may dispose them inside ConsoleCommand. */
        combat_fire_bot_tick(base,c.pc,c.pawn,c.world,0,0,after,now);
        guest_player_context_adapt(base,c.pc,c.pawn,c.world,0,0,now);
        guest_cinematic_registration_adapt(base,engine,c.pc,c.world,0,now);
        wchar_t command[] = L"disconnect"; console(base, c.pc, command);
    }
}
static void session_frame(uintptr_t base,uintptr_t engine,int after) {
    if(!after) {
        static int focus_opted=-1;
        if(focus_opted<0) {wchar_t value[8]={0};focus_opted=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_BACKGROUND_FOCUS",value,8)==1&&value[0]==L'1';}
        background_focus_prepare(base,engine,session_enabled&&focus_opted&&progress_enabled&&has_marker(root));
    }
    if(!session_enabled)return;
    session_engine=engine;session_game_thread=GetCurrentThreadId();
    if(test_action==5)session_guest_frame(base,engine,after);
    else session_host_frame(base,engine,after);
}
static void session_sample(uintptr_t base, uintptr_t engine, DWORD now) {
    if (!session_enabled) return;
    SessionContext c = session_context(engine); char text[650];
    if (test_action == 5) {
        unsigned char net = 255; if (c.world) read_at(c.world + 0x7c8, &net, 1);
        uintptr_t camera = read_ptr(c.pc + 0x684); float xyz[3] = {0}; read_at(c.pawn + 0x108, xyz, sizeof xyz);
        if (!session_actor_live(base, c.pawn) || !is_kind_of(base, c.pawn, "SFXPawn_Ashley")) {
            uintptr_t ack = read_ptr(c.pc + 0x694), view = camera ? read_ptr(camera + 0x3f4) : 0;
            snprintf(text, sizeof text,
                "{\"session\":\"guest-idle\",\"net_mode\":%u,\"has_pawn\":%s,\"acknowledged_ashley\":%s,\"camera_targets_ashley\":%s,\"monotonic_ms\":%lu}\r\n",
                net, c.pawn ? "true" : "false", ack && is_kind_of(base, ack, "SFXPawn_Ashley") ? "true" : "false",
                view && is_kind_of(base, view, "SFXPawn_Ashley") ? "true" : "false", now); line(text); return;
        }
        snprintf(text, sizeof text,
            "{\"session\":\"guest-observation\",\"epoch\":%u,\"local_epoch\":true,\"net_mode\":%u,\"ashley_pawn\":true,\"reciprocal_controller\":%s,\"camera_targets_ashley\":%s,\"held_ms\":%lu,\"monotonic_ms\":%lu,\"position\":[%.3f,%.3f,%.3f]}\r\n",
            session_guest_epoch, net, read_ptr(c.pawn + 0x374) == c.pc ? "true" : "false",
            camera && read_ptr(camera + 0x3f4) == c.pawn ? "true" : "false", now - session_guest_at, now, xyz[0], xyz[1], xyz[2]); line(text);
    } else if (test_action == 6 && session_active && !session_pending_reason) {
        float world_time = 0; read_at(c.world + 0x74c, &world_time, 4);
        int owned = read_ptr(session_remote + 0x3bc) == session_pawn && read_ptr(session_pawn + 0x374) == session_remote;
        snprintf(text, sizeof text,
            "{\"session\":\"sample\",\"epoch\":%u,\"held_ms\":%lu,\"remote_owns_ashley\":%s,\"host_story_preserved\":%s,\"mode\":%u,\"world_time\":%.3f,\"monotonic_ms\":%lu}\r\n",
            session_epoch, now - session_acquired_at, owned ? "true" : "false", session_story_valid(base) ? "true" : "false", c.mode, world_time, now); line(text);
    }
}
