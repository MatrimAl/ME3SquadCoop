/* Bounded two-controller probe. Ordinary possession/input only; no actor
   position writes, package identity changes, or replication bypasses. */
static int pair_control_enabled = 0, pair_control_stage = 0;
static uintptr_t pair_remote = 0, pair_ashley = 0, pair_ai = 0, pair_host = 0, pair_shepard = 0;
static uintptr_t pair_story_player = 0, pair_story_initial = 0;
static DWORD pair_possessed_at = 0, pair_skip_at = 0, pair_guest_input_at = 0;
static float pair_initial_ashley[3], pair_initial_shepard[3];
#include "native_remote_controller.h"
static void pair_guest_input(uintptr_t base, uintptr_t engine) {
    if (!pair_control_enabled || test_action != 5) return;
    uintptr_t pc = local_controller(engine), pawn = pc ? read_ptr(pc + 0x3bc) : 0;
    if (!is_live(base, pawn) || !is_kind_of(base, pawn, "SFXPawn_Ashley")) return;
    uintptr_t input = read_ptr(pc + 0x6dc);
    if (!is_live(base, input) || !is_kind_of(base, input, "PlayerInput")) return;
    DWORD now = GetTickCount(); if (!pair_guest_input_at) pair_guest_input_at = now;
    DWORD elapsed = now - pair_guest_input_at;
    float value = elapsed >= 2000 && elapsed < 4000 ? 1.0f : 0.0f; SIZE_T wrote;
    WriteProcessMemory(GetCurrentProcess(), (void *)(input + 0x194), &value, 4, &wrote);
    WriteProcessMemory(GetCurrentProcess(), (void *)(input + 0x1a4), &value, 4, &wrote);
}
static void pair_control_probe(uintptr_t base, uintptr_t pc, uintptr_t pawn, uintptr_t world,
                               uintptr_t squad, uintptr_t manager, unsigned char mode, DWORD now) {
    if (!pair_control_enabled) return;
    char text[1100];
    if (test_action == 5) {
        unsigned char net = 255; if (world) read_at(world + 0x7c8, &net, 1);
        uintptr_t camera = pc ? read_ptr(pc + 0x684) : 0;
        int ashley = pawn && is_kind_of(base, pawn, "SFXPawn_Ashley");
        float xyz[3] = {0}; if (pawn) read_at(pawn + 0x108, xyz, sizeof xyz);
        snprintf(text, sizeof text,
            "{\"pair_control\":\"guest-observation\",\"net_mode\":%u,\"ashley_pawn\":%s,\"reciprocal_controller\":%s,\"camera_targets_ashley\":%s,\"input_elapsed_ms\":%lu,\"position\":[%.3f,%.3f,%.3f]}\r\n",
            net, ashley ? "true" : "false", ashley && read_ptr(pawn + 0x374) == pc ? "true" : "false",
            ashley && camera && read_ptr(camera + 0x3f4) == pawn ? "true" : "false",
            pair_guest_input_at ? now - pair_guest_input_at : 0, xyz[0], xyz[1], xyz[2]); line(text);
        return;
    }
    if (test_action != 6 || !host_map_refreshed || !pc || !pawn || !squad || !world || !manager || pair_control_stage >= 2) return;
    unsigned char scratch[64] = {0};
    if (pair_control_stage == 1) {
        int live = is_live(base, pair_remote) && is_live(base, pair_ashley) && is_live(base, pair_host) && is_live(base, pair_shepard);
        if (!live) { line("{\"pair_control\":\"restore-aborted-stale-object\"}\r\n"); pair_control_stage = 3; return; }
        int story = read_ptr(pair_host + 0x3bc) == pair_shepard && read_ptr(pair_shepard + 0x374) == pair_host &&
                    read_ptr(squad + 0x2cc) == pair_story_player && read_ptr(squad + 0x2d4) == pair_story_initial;
        int owned = read_ptr(pair_remote + 0x3bc) == pair_ashley && read_ptr(pair_ashley + 0x374) == pair_remote;
        float guest_xyz[3] = {0}, host_xyz[3] = {0};
        read_at(pair_ashley + 0x108, guest_xyz, sizeof guest_xyz); read_at(pair_shepard + 0x108, host_xyz, sizeof host_xyz);
        snprintf(text, sizeof text,
            "{\"pair_control\":\"host-sample\",\"held_ms\":%lu,\"remote_owns_ashley\":%s,\"host_story_preserved\":%s,\"ashley_distance\":%.3f,\"shepard_distance\":%.3f,\"host_pawn_preserved\":%s,\"host_controller_preserved\":%s,\"story_player_preserved\":%s,\"story_initial_preserved\":%s}\r\n",
            now - pair_possessed_at, owned ? "true" : "false", story ? "true" : "false",
            distance(guest_xyz, pair_initial_ashley), distance(host_xyz, pair_initial_shepard),
            read_ptr(pair_host + 0x3bc) == pair_shepard ? "true" : "false", read_ptr(pair_shepard + 0x374) == pair_host ? "true" : "false",
            read_ptr(squad + 0x2cc) == pair_story_player ? "true" : "false", read_ptr(squad + 0x2d4) == pair_story_initial ? "true" : "false"); line(text);
        if (story && owned && now - pair_possessed_at < 8000 && mode != 7 && mode != 8 && mode != 9) return;
        invoke(base, pair_remote, "Controller", "UnPossess", scratch);
        struct { uintptr_t pawn; uint32_t vehicle; } params = {pair_shepard, 0};
        if (!story) invoke(base, pair_host, "BioPlayerController", "Possess", &params);
        params.pawn = pair_ashley;
        if (is_live(base, pair_ai)) invoke(base, pair_ai, "BioAiController", "Possess", &params);
        int restored = read_ptr(pair_host + 0x3bc) == pair_shepard && read_ptr(pair_shepard + 0x374) == pair_host &&
                       read_ptr(squad + 0x2cc) == pair_story_player && read_ptr(squad + 0x2d4) == pair_story_initial &&
                       read_ptr(pair_ashley + 0x374) == pair_ai && read_ptr(pair_ai + 0x3bc) == pair_ashley;
        snprintf(text, sizeof text, "{\"pair_control\":\"restored\",\"host_and_ai_restored\":%s,\"story_changed_during_test\":%s}\r\n", restored ? "true" : "false", story ? "false" : "true");
        line(text); pair_control_stage = 2; return;
    }
    if (read_ptr(world + 0x670)) {
        invoke(base, pc, "PlayerController", "SetPause", scratch);
        line("{\"pair_control\":\"unpause-request\"}\r\n");
    }
    if (mode == 8 && now - pair_skip_at >= 5000) {
        pair_skip_at = now;
        uintptr_t cinematic = mode_object(base, manager, "SFXGameModeCinematic");
        int ok = invoke(base, cinematic, "SFXGameModeCinematic", "SkipCinematic", scratch);
        line(ok ? "{\"pair_control\":\"cinematic-skip-dispatched\"}\r\n" : "{\"pair_control\":\"cinematic-skip-unavailable\"}\r\n"); return;
    }
    if (!is_live(base, pair_remote)) pair_remote = pair_find_remote(base, pc, world);
    if (!pair_remote) return;
    uintptr_t remote_pawn = read_ptr(pair_remote + 0x3bc);
    char remote_kind[96]; object_name(base, remote_pawn ? read_ptr(remote_pawn + 0x50) : 0, remote_kind, sizeof remote_kind);
    snprintf(text, sizeof text,
        "{\"pair_control\":\"remote-state\",\"remote_has_pawn\":%s,\"remote_pawn_is_shepard\":%s,\"remote_pawn_class\":\"%s\",\"host_owns_shepard\":%s,\"squad_main_is_shepard\":%s,\"mode\":%u}\r\n",
        remote_pawn ? "true" : "false", remote_pawn == pawn ? "true" : "false", remote_kind,
        read_ptr(pawn + 0x374) == pc ? "true" : "false", read_ptr(squad + 0x2cc) == pawn ? "true" : "false", mode); line(text);
    if (mode == 7 || mode == 8 || mode == 9 || remote_pawn) return;
    uint32_t allows = 0;
    if (!invoke(base, manager, "SFXGameModeManager", "AllowsMovement", &allows) || !(allows & 1)) return;
    GameArray members = {0}; if (!identity_array(squad + 0x270, &members, 4)) return;
    uintptr_t ashley = 0;
    for (int i = 0; i < members.count; ++i) {
        uintptr_t member = read_ptr(members.data + (SIZE_T)i * 8);
        if (member != pawn && is_kind_of(base, member, "SFXPawn_Ashley")) ashley = member;
    }
    uintptr_t ai = ashley ? read_ptr(ashley + 0x374) : 0;
    if (!is_live(base, ai) || !is_kind_of(base, ai, "BioAiController")) return;
    pair_host = pc; pair_shepard = pawn; pair_ashley = ashley; pair_ai = ai;
    pair_story_player = read_ptr(squad + 0x2cc); pair_story_initial = read_ptr(squad + 0x2d4);
    read_at(ashley + 0x108, pair_initial_ashley, sizeof pair_initial_ashley);
    read_at(pawn + 0x108, pair_initial_shepard, sizeof pair_initial_shepard);
    if (!invoke(base, ai, "BioAiController", "UnPossess", scratch)) return;
    struct { uintptr_t pawn; uint32_t vehicle; } params = {ashley, 0};
    /* Execute the base UFunction explicitly. BioPlayerController.Possess
       updates the singleton story squad even for a remote controller. */
    invoke(base, pair_remote, "PlayerController", "Possess", &params);
    pair_possessed_at = now; pair_control_stage = 1;
    line("{\"pair_control\":\"remote-possession-dispatched\",\"target\":\"Ashley\",\"function_owner\":\"PlayerController\"}\r\n");
}
