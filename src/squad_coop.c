/* Ordinary experimental runtime entry. Initialization is not evidence of a
 * complete playable campaign. File presence does not prove an engine mount. */
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <stdint.h>
#include <wchar.h>
#include <string.h>
#include "me3_game_support.h"
#include "runtime_config.h"
#include "runtime_fingerprint.h"
#include "runtime_transport.h"

static int g_dlc_active = 0;
static wchar_t g_game_root[MAX_PATH] = {0};
static Me3RuntimeConfig g_runtime_config;
static Me3RuntimeConfigStatus g_config_status=ME3_CONFIG_DISABLED;
static volatile LONG g_enable_requested=1;
static volatile LONG g_bridge_installed;
static volatile LONG g_runtime_error;
/* Default existing-session attachment remains available. An explicit host
 * startup closes admission until its whole native setup has been verified. */
static volatile LONG runtime_host_start_gate=1;
static wchar_t g_executable[MAX_PATH];
#include "runtime_session.h"
#include "runtime_network_bootstrap.h"
#include "runtime_join_event_trace.h"
#include "runtime_panel.h"
#include "runtime_ready_policy.h"
#include "runtime_stunt_actor_retention.h"
static EngineTick runtime_original_tick;
static DWORD runtime_last_gate;


/* Active-DLC Runtime Gate: Checks if DLC_MOD_ME3SquadCoop is installed and enabled */
static int check_active_dlc_gate(const wchar_t *root) {
    if (!root || !root[0]) return 0;
    wchar_t dlc_path[MAX_PATH];
    int n = _snwprintf(dlc_path, MAX_PATH, L"%ls\\BioGame\\DLC\\DLC_MOD_ME3SquadCoop\\CookedPCConsole\\Mount.dlc", root);
    if (n <= 0 || n >= MAX_PATH) return 0;
    DWORD attrs = GetFileAttributesW(dlc_path);
    if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        return 0; /* DLC absent or disabled: fail closed, stay inert */
    }
    return 1; /* Enabled DLC file present; engine mount is a separate check. */
}

/* All engine calls and transient adapter writes run on the native Tick
 * thread. The worker only validates files and atomically installs this bridge.
 * Disabling leaves the tiny forwarding bridge resident until game shutdown. */
static void runtime_tick(void *self,float delta) {
    uintptr_t base=(uintptr_t)GetModuleHandleW(NULL);DWORD now=GetTickCount();
    if(!runtime_last_gate||now-runtime_last_gate>=1000) {
        runtime_last_gate=now;g_dlc_active=check_active_dlc_gate(g_game_root);
    }
    runtime_panel_commands();
    runtime_control_enabled=g_dlc_active&&InterlockedCompareExchange(&g_enable_requested,0,0)&&
        g_config_status==ME3_CONFIG_READY;
    runtime_stunt_actor_prepare(base,(uintptr_t)self);
    runtime_ready_policy_tick(base,(uintptr_t)self);
    runtime_network_start_tick(base,(uintptr_t)self,now);
    runtime_session_frame(base,(uintptr_t)self,0,now);
    runtime_original_tick(self,delta);
    runtime_join_progress_poll(base,(uintptr_t)self,GetTickCount());
    runtime_session_frame(base,(uintptr_t)self,1,GetTickCount());
    runtime_join_outcome_tick(runtime_scope_ready,GetTickCount());
    runtime_host_capacity_poll(base,(uintptr_t)self);
    runtime_panel_publish(base,(uintptr_t)self,GetTickCount());
}
static int runtime_install_bridge(uintptr_t base) {
    uintptr_t engine=read_ptr(base+0x18b31d0),vt=read_ptr(engine);
    uintptr_t expected=base+0xb9dda0,slot=vt+0x2b0;
    const unsigned char want[]={0x40,0x53,0x55,0x41,0x56,0x48,0x83,0xec,0x50,0x33,0xed};
    unsigned char got[sizeof want];
    if(vt!=base+0x13983a0||read_ptr(slot)!=expected||
       !read_at(expected,got,sizeof got)||memcmp(got,want,sizeof want))return 0;
    DWORD old=0;if(!VirtualProtect((void*)slot,8,PAGE_READWRITE,&old))return 0;
    runtime_original_tick=(EngineTick)expected;
    PVOID previous=InterlockedCompareExchangePointer((PVOID volatile*)slot,(PVOID)runtime_tick,(PVOID)expected);
    VirtualProtect((void*)slot,8,old,&old);
    if((uintptr_t)previous!=expected){runtime_original_tick=NULL;return 0;}
    session_enabled=1;InterlockedExchange(&g_bridge_installed,1);
    line("{\"runtime\":\"bridge-installed\",\"implicit-travel\":false,\"automated-input\":false}\r\n");
    if(runtime_join_event_opted()&&!runtime_join_event_install(base,engine))
        line("{\"runtime_network\":\"join-event-observer-refused\",\"native_progress_unchanged\":true}\r\n");
    return 1;
}
static DWORD WINAPI runtime_start(void *unused) {
    (void)unused;uintptr_t base=(uintptr_t)GetModuleHandleW(NULL);
    if(!runtime_executable_fingerprint(g_executable,base)) {InterlockedExchange(&g_runtime_error,1);return 0;}
    wchar_t path[MAX_PATH];int n=_snwprintf(path,MAX_PATH,L"%ls\\ME3SquadCoopRuntime",g_game_root);
    if(n>0&&n<MAX_PATH) {
        CreateDirectoryW(path,NULL);
        n=_snwprintf(path,MAX_PATH,L"%ls\\ME3SquadCoopRuntime\\squad_runtime_%lu.jsonl",g_game_root,GetCurrentProcessId());
        if(n>0&&n<MAX_PATH)log_file=CreateFileW(path,FILE_APPEND_DATA,FILE_SHARE_READ,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);
    }
    line("{\"runtime\":\"validated\",\"full-executable-sha256\":true,\"playable-campaign-verified\":false}\r\n");
    runtime_panel_prepare();
    for(int attempt=0;attempt<120;++attempt) {
        if(!check_active_dlc_gate(g_game_root)||!InterlockedCompareExchange(&g_enable_requested,0,0))return 0;
        if(runtime_install_bridge(base))return 0;
        Sleep(500);
    }
    InterlockedExchange(&g_runtime_error,2);line("{\"runtime\":\"bridge-unavailable\"}\r\n");return 0;
}
__declspec(dllexport) int ME3SquadCoopBridgeInstalled(void) {return InterlockedCompareExchange(&g_bridge_installed,0,0)!=0;}
__declspec(dllexport) int ME3SquadCoopRuntimeError(void) {return InterlockedCompareExchange(&g_runtime_error,0,0);}
__declspec(dllexport) int ME3SquadCoopRequestJoin(void) {return runtime_request_join();}
__declspec(dllexport) int ME3SquadCoopRequestRejoin(void) {return runtime_request_rejoin();}
__declspec(dllexport) int ME3SquadCoopRequestListen(void) {return runtime_request_listen();}
__declspec(dllexport) int ME3SquadCoopRequestRelisten(void) {return runtime_request_relisten();}
__declspec(dllexport) int ME3SquadCoopCancelPendingConnection(void) {return runtime_request_cancel_start();}
__declspec(dllexport) unsigned ME3SquadCoopPowerScope(void) {return (unsigned)InterlockedCompareExchange(&runtime_power_scope,0,0);}
__declspec(dllexport) unsigned ME3SquadCoopPowerCount(void) {return (unsigned)InterlockedCompareExchange(&runtime_power_count,0,0);}
__declspec(dllexport) int ME3SquadCoopRequestPower(unsigned scope,unsigned index) {return runtime_request_power(scope,index);}
__declspec(dllexport) int ME3SquadCoopRequestMappedPower(unsigned scope,unsigned action) {return runtime_request_mapped_power(scope,action);}
__declspec(dllexport) int ME3SquadCoopPowerStatus(void) {
    return InterlockedCompareExchange64(&runtime_power_request,0,0)?RUNTIME_POWER_QUEUED:
        InterlockedCompareExchange(&runtime_power_status,0,0);
}
__declspec(dllexport) int ME3SquadCoopNetworkStartStatus(void) {
    return InterlockedCompareExchange(&runtime_network_start_status,0,0);
}
/* Callers request a policy change; cleanup is deferred to the game thread.
 * Never unload the ASI while the game is running. */
__declspec(dllexport) int ME3SquadCoopSetEnabled(int enabled) {
    if(enabled!=0&&enabled!=1)return 0;InterlockedExchange(&g_enable_requested,enabled);return 1;
}
__declspec(dllexport) int ME3SquadCoopActive(void) {
    return g_dlc_active;
}

__declspec(dllexport) int ME3SquadCoopVerifyDlcGate(const wchar_t *custom_root) {
    return check_active_dlc_gate(custom_root ? custom_root : g_game_root);
}

__declspec(dllexport) const char* ME3SquadCoopVersion(void) {
    return "0.1.0";
}
__declspec(dllexport) int ME3SquadCoopConfigStatus(void) {return g_config_status;}
#ifdef ME3_RUNTIME_FIXTURE
__declspec(dllexport) int ME3RuntimeScriptSlotSelfTest(void) {return native_script_slot_selftest();}
#include "runtime_rpc_receive_selftest.h"
__declspec(dllexport) int ME3RuntimeRpcReceiveSelfTest(void) {return runtime_rpc_receive_selftest();}
#include "runtime_core_fixture.h"
#include "runtime_auto_rejoin_fixture.h"
__declspec(dllexport) int ME3RuntimeAttachRecoverySelfTest(void) {return runtime_attach_recovery_selftest();}
__declspec(dllexport) int ME3RuntimeMappedPowerInputSelfTest(void) {return runtime_mapped_power_input_selftest();}
__declspec(dllexport) int ME3RuntimePowerActionContextSelfTest(void) {return runtime_power_action_context_selftest();}
__declspec(dllexport) int ME3RuntimeDelayedFireStopSelfTest(void) {return runtime_delayed_fire_stop_selftest();}
__declspec(dllexport) int ME3RuntimeGuestFireReleaseSelfTest(void) {return runtime_guest_fire_release_selftest();}
__declspec(dllexport) int ME3RuntimeGuestMeleeSelfTest(void) {return runtime_guest_melee_selftest();}
__declspec(dllexport) int ME3RuntimeGuestTraversalSelfTest(void) {return runtime_guest_traversal_selftest();}
__declspec(dllexport) int ME3RuntimeGuestReactionsSelfTest(void) {return runtime_guest_reactions_selftest();}
__declspec(dllexport) int ME3RuntimeGuestCoverKnockbackSelfTest(void) {return runtime_guest_cover_knockback_selftest();}
__declspec(dllexport) int ME3RuntimeGuestHitFeedbackSelfTest(void) {return runtime_guest_hit_feedback_selftest();}
__declspec(dllexport) int ME3RuntimeGuestAmmoPickupSelfTest(void) {return runtime_guest_ammo_pickup_selftest();}
__declspec(dllexport) int ME3RuntimeAirlockPlayerSelfTest(void) {return runtime_airlock_player_selftest();}
__declspec(dllexport) int ME3RuntimeSupplySleepSelfTest(void) {return runtime_supply_sleep_selftest();}
__declspec(dllexport) int ME3RuntimeAmmoAwardSelfTest(void) {return runtime_ammo_award_selftest();}
__declspec(dllexport) int ME3RuntimeAmmoReceiveSelfTest(void) {return runtime_ammo_receive_selftest();}
__declspec(dllexport) int ME3RuntimeBinkMovieSelfTest(void) {return runtime_bink_movie_selftest();}
__declspec(dllexport) int ME3RuntimeSceneSpectatorSelfTest(void) {return runtime_scene_spectator_selftest();}
__declspec(dllexport) int ME3RuntimeDialogueProtocolSelfTest(void) {return dialogue_choice_protocol_selftest();}
__declspec(dllexport) int ME3RuntimeDialogueSelfTest(void) {return runtime_dialogue_selftest();}
__declspec(dllexport) int ME3RuntimeSceneActorObserverSelfTest(void) {return runtime_scene_actor_observer_selftest();}
__declspec(dllexport) int ME3RuntimeAmmoSettleSelfTest(void) {return runtime_ammo_settle_selftest();}
__declspec(dllexport) int ME3RuntimeGuestSceneWeaponSelfTest(void) {return runtime_guest_scene_weapon_selftest();}
__declspec(dllexport) int ME3RuntimeHeavyPlotGuardSelfTest(void) {return runtime_heavy_plot_guard_selftest();}
#include "runtime_revive_pair_selftest.h"
__declspec(dllexport) int ME3RuntimeReviveSelfTest(void) {return runtime_revive_pair_selftest()&&runtime_revive_peer_selftest();}
static unsigned guest_vitals_fixture_calls;
static void guest_vitals_fixture_forward(void *a,void *b,void *c,void *d) {
    if((uintptr_t)a==1&&(uintptr_t)b==2&&(uintptr_t)c==3&&(uintptr_t)d==4&&GetLastError()==411)++guest_vitals_fixture_calls;
    SetLastError(412);
}
__declspec(dllexport) int ME3RuntimeGuestVitalsSelfTest(void) {
    float fraction=-1;
    if(!guest_vitals_fraction(25,100,&fraction)||fraction!=0.25f||
       !guest_vitals_fraction(0,100,&fraction)||fraction!=0||
       !guest_vitals_fraction(150,100,&fraction)||fraction!=1||
       guest_vitals_fraction(NAN,100,&fraction)||guest_vitals_fraction(10,NAN,&fraction)||
       guest_vitals_fraction(10,0,&fraction)||guest_vitals_fraction(-1,100,&fraction)||
       guest_vitals_fraction(500,100,&fraction))return 0;
    GuestVitalsHud saved=guest_vitals;DWORD error=GetLastError();
    memset(&guest_vitals,0,sizeof guest_vitals);guest_vitals.original=guest_vitals_fixture_forward;guest_vitals_fixture_calls=0;
    SetLastError(411);guest_vitals_process_event((void*)1,(void*)2,(void*)3,(void*)4);
    int ok=guest_vitals_fixture_calls==1&&GetLastError()==412;
    guest_vitals=saved;SetLastError(error);return ok&&runtime_team_hud_selftest();
}
__declspec(dllexport) int ME3RuntimeNetworkStartSelfTest(void) {return runtime_network_start_selftest()&&runtime_startup_focus_selftest()&&runtime_package_preload_selftest();}
__declspec(dllexport) int ME3RuntimeTravelIdleSelfTest(void) {return runtime_join_travel_idle_selftest();}
__declspec(dllexport) int ME3RuntimeJoinReadinessSelfTest(void) {return runtime_join_readiness_selftest();}
__declspec(dllexport) int ME3RuntimeJoinDiagnosticsSelfTest(void) {return runtime_join_diagnostics_selftest();}
__declspec(dllexport) int ME3RuntimeJoinProgressSelfTest(void) {return runtime_join_progress_selftest()&&runtime_join_outcome_selftest();}
__declspec(dllexport) int ME3RuntimeJoinEventSelfTest(void) {return runtime_join_event_selftest();}
#include "runtime_transport_fixture.h"
__declspec(dllexport) int ME3RuntimeTransportSelfTest(void) {return runtime_transport_selftest();}
#include "runtime_host_fixture.h"
__declspec(dllexport) int ME3RuntimeHostStartupSelfTest(void) {return runtime_host_startup_selftest();}
__declspec(dllexport) int ME3RuntimeListenRequestSelfTest(void) {
    Me3RuntimeConfig saved=g_runtime_config;Me3RuntimeConfigStatus status=g_config_status;
    LONG enabled=g_enable_requested,requested=runtime_listen_requested,attempted=runtime_listen_attempted,
        network_status=runtime_network_start_status,gate=runtime_host_start_gate;
    runtime_listen_requested=runtime_listen_attempted=0;g_enable_requested=1;g_config_status=ME3_CONFIG_DISABLED;
    int ok=!runtime_request_listen();g_config_status=ME3_CONFIG_READY;g_runtime_config.role=ME3_ROLE_GUEST;
    ok=ok&&!runtime_request_listen();g_runtime_config.role=ME3_ROLE_HOST;g_enable_requested=0;
    ok=ok&&!runtime_request_listen();g_enable_requested=1;
    ok=ok&&runtime_request_listen()&&!runtime_request_listen()&&runtime_listen_requested==1&&
        !runtime_host_start_gate&&runtime_network_start_status==RUNTIME_START_WAITING;
    runtime_listen_requested=0;runtime_listen_attempted=1;ok=ok&&!runtime_request_listen();
    g_runtime_config=saved;g_config_status=status;g_enable_requested=enabled;runtime_listen_requested=requested;
    runtime_listen_attempted=attempted;runtime_network_start_status=network_status;runtime_host_start_gate=gate;return ok;
}
__declspec(dllexport) int ME3RuntimeJoinRequestSelfTest(void) {
    Me3RuntimeConfig saved=g_runtime_config;Me3RuntimeConfigStatus status=g_config_status;
    LONG enabled=g_enable_requested,requested=runtime_join_requested,attempted=runtime_join_attempted,
        network_status=runtime_network_start_status;int control=runtime_control_enabled;
    runtime_join_requested=runtime_join_attempted=0;g_enable_requested=1;runtime_control_enabled=1;
    g_config_status=ME3_CONFIG_DISABLED;int ok=!runtime_request_join();
    g_config_status=ME3_CONFIG_READY;g_runtime_config.role=ME3_ROLE_HOST;ok=ok&&!runtime_request_join();
    g_runtime_config.role=ME3_ROLE_GUEST;g_enable_requested=0;ok=ok&&!runtime_request_join();
    g_enable_requested=1;ok=ok&&runtime_request_join()&&!runtime_request_join()&&
        runtime_join_requested==1&&runtime_network_start_status==RUNTIME_START_WAITING;
    runtime_network_start_tick((uintptr_t)GetModuleHandleW(NULL),1,10);
    ok=ok&&runtime_join_requested==1&&!runtime_join_attempted&&runtime_network_start_status==RUNTIME_START_WAITING;
    runtime_join_requested=0;runtime_join_attempted=1;ok=ok&&!runtime_request_join();
    g_runtime_config=saved;g_config_status=status;g_enable_requested=enabled;runtime_join_requested=requested;
    runtime_join_attempted=attempted;runtime_network_start_status=network_status;runtime_control_enabled=control;return ok;
}
__declspec(dllexport) int ME3RuntimeRejoinRequestSelfTest(void) {
    Me3RuntimeConfig saved=g_runtime_config;Me3RuntimeConfigStatus status=g_config_status;
    LONG enabled=g_enable_requested,requested=runtime_join_requested,attempted=runtime_join_attempted,
        retry=runtime_rejoin_requested,network_status=runtime_network_start_status,
        origin=runtime_rejoin_verified_origin;
    runtime_join_requested=runtime_join_attempted=runtime_rejoin_requested=runtime_rejoin_verified_origin=0;
    g_enable_requested=1;g_config_status=ME3_CONFIG_READY;g_runtime_config.role=ME3_ROLE_GUEST;
    int ok=!runtime_request_rejoin();runtime_join_attempted=1;
    g_runtime_config.role=ME3_ROLE_HOST;ok=ok&&!runtime_request_rejoin();
    g_runtime_config.role=ME3_ROLE_GUEST;g_enable_requested=0;ok=ok&&!runtime_request_rejoin();
    g_enable_requested=1;runtime_join_requested=1;ok=ok&&!runtime_request_rejoin();
    runtime_join_requested=0;
    ok=ok&&runtime_request_rejoin()&&!runtime_request_rejoin()&&!runtime_request_join()&&
        runtime_rejoin_requested==1&&runtime_join_attempted==1&&!runtime_join_requested;
    g_runtime_config=saved;g_config_status=status;g_enable_requested=enabled;
    runtime_join_requested=requested;runtime_join_attempted=attempted;runtime_rejoin_requested=retry;
    runtime_rejoin_verified_origin=origin;runtime_network_start_status=network_status;return ok;
}
static unsigned runtime_fixture_tick_count;
static uintptr_t runtime_fixture_tick_self;
static float runtime_fixture_tick_delta;
static void runtime_fixture_tick(void *self,float delta) {
    ++runtime_fixture_tick_count;runtime_fixture_tick_self=(uintptr_t)self;runtime_fixture_tick_delta=delta;
}
__declspec(dllexport) int ME3RuntimeTickForwardingSelfTest(void) {
    EngineTick saved=runtime_original_tick;runtime_original_tick=runtime_fixture_tick;
    runtime_fixture_tick_count=0;runtime_fixture_tick_self=0;runtime_fixture_tick_delta=0;
    runtime_tick((void*)(uintptr_t)0x1234,0.125f);runtime_original_tick=saved;
    return runtime_fixture_tick_count==1&&runtime_fixture_tick_self==0x1234&&runtime_fixture_tick_delta==0.125f&&
        !runtime_control_enabled&&!runtime_scope_ready&&!g_bridge_installed;
}
__declspec(dllexport) int ME3RuntimeCoreCleanupSelfTest(void) {return runtime_core_cleanup_selftest();}
__declspec(dllexport) int ME3RuntimePolicySelfTest(void) {
    Me3RuntimeConfig saved=g_runtime_config;int enabled=runtime_control_enabled,ready=runtime_scope_ready,ok=runtime_command_menu_mode_selftest()&&runtime_story_gate_policy_selftest()&&runtime_story_warning_selftest()&&runtime_story_gate_selftest()&&runtime_ambient_conversation_selftest()&&runtime_guest_team_selftest()&&runtime_guest_weapon_selftest()&&runtime_guest_shield_feedback_selftest()&&runtime_guest_movement_selftest()&&runtime_guest_swap_selftest()&&runtime_power_assignment_selftest()&&runtime_layout_cache_selftest()&&runtime_guest_fire_audit_selftest();
    g_runtime_config.role=ME3_ROLE_GUEST;ok=ok&&native_session_role()==5&&!session_policy_is_host();
    g_runtime_config.role=ME3_ROLE_HOST;ok=ok&&native_session_role()==6&&session_policy_is_host();
    runtime_control_enabled=0;runtime_scope_ready=0;
    ok=ok&&!session_policy_enabled()&&!session_policy_host_ready();
    runtime_control_enabled=1;runtime_scope_ready=1;
    ok=ok&&session_policy_enabled()&&session_policy_host_ready()&&session_policy_acquire_ready(0);
    for(DWORD now=0;now<100000;now+=100)ok=ok&&!session_policy_synthetic(1,now);
    g_runtime_config=saved;runtime_control_enabled=enabled;runtime_scope_ready=ready;return ok;
}
__declspec(dllexport) int ME3RuntimeSessionGateSelfTest(void) {return session_gate_selftest();}
__declspec(dllexport) int ME3RuntimeCameraSelfTest(void) {return guest_camera_adapter_selftest()&&remote_appearance_case_selftest()&&remote_weapon_presentation_selftest();}
__declspec(dllexport) int ME3RuntimePresentationSelfTest(void) {return runtime_presentation_selftest()&&runtime_native_hud_selftest();}
__declspec(dllexport) int ME3RuntimeStuntRetentionSelfTest(void) {return runtime_stunt_selftest();}
__declspec(dllexport) int ME3RuntimeCinematicSelfTest(void) {return guest_cinematic_registration_selftest();}
__declspec(dllexport) int ME3RuntimeHashFileFixture(const wchar_t *path,const unsigned char *expected) {
    return path&&expected&&runtime_hash_file_matches(path,expected);
}
__declspec(dllexport) int ME3RuntimeIdentitySelfTest(void) {return me3_runtime_identity_selftest();}
static unsigned runtime_owned_fixture_calls;
static void runtime_owned_fixture_original(void *a,void *b,void *c,void *d,void *e) {
    if((uintptr_t)a==11&&(uintptr_t)b==22&&(uintptr_t)c==33&&(uintptr_t)d==44&&(uintptr_t)e==55&&GetLastError()==607)
        ++runtime_owned_fixture_calls;
    SetLastError(609);
}
__declspec(dllexport) int ME3RuntimeOwnedOptimizedSelfTest(void) {
    for(unsigned role=0;role<=3;++role)for(int direct=0;direct<2;++direct)for(int captured=0;captured<2;++captured) {
        int wanted=(direct&&role==2)||(captured&&(role==1||role==2));
        if(runtime_owned_receive_permission(direct,captured,(unsigned char)role)!=wanted)return 0;
    }
    if(!runtime_owned_optimized_case(1,1,1,1,1,1,0x4000,0,10,15)||
       runtime_owned_optimized_case(1,1,0,1,1,1,0x4000,0,10,15)||
       runtime_owned_optimized_case(1,1,1,0,1,1,0x4000,0,10,15)||
       runtime_owned_optimized_case(1,1,1,1,0,1,0x4000,0,10,15)||
       runtime_owned_optimized_case(1,1,1,1,1,0,0x4000,0,10,15)||
       runtime_owned_optimized_case(1,1,1,1,1,1,0,0,10,15)||
       runtime_owned_optimized_case(1,1,1,1,1,1,0x4000,1,10,15)||
       runtime_owned_optimized_case(1,1,1,1,1,1,0x4000,0,16,15)||
       runtime_owned_optimized_case(1,1,1,1,1,1,0x4000,0,11,15))return 0;
    volatile LONG flags=0xc000;
    if(runtime_owned_optimized_clear(&flags,0x4000)||flags!=0xc000||
       !runtime_owned_optimized_clear(&flags,0xc000)||flags!=0x8000)return 0;
    InterlockedOr(&flags,0x100);runtime_owned_optimized_restore_bit(&flags);
    if(flags!=0xc100)return 0; /* Retain a native change to another bit. */
    RuntimeOwnedOptimizedOriginal saved=runtime_owned_optimized_original;
    int enabled=runtime_control_enabled;DWORD error=GetLastError();
    runtime_control_enabled=0;runtime_owned_fixture_calls=0;runtime_owned_optimized_original=runtime_owned_fixture_original;
    SetLastError(607);runtime_owned_optimized_forward((void*)11,(void*)22,(void*)33,(void*)44,(void*)55,0);
    int ok=runtime_owned_fixture_calls==1&&GetLastError()==609;
    runtime_owned_optimized_original=saved;runtime_control_enabled=enabled;SetLastError(error);return ok;
}
__declspec(dllexport) int ME3RuntimeConfigSelfTest(void) {return runtime_config_selftest();}
__declspec(dllexport) int ME3RuntimeSupportSelfTest(void) {
    unsigned char engine[0x510]={0},player[0x80]={0};uint32_t marker=17;
    uintptr_t players[1]={(uintptr_t)player};GameArray array={(uintptr_t)players,1,1};
    uintptr_t controller=(uintptr_t)&marker;
    memcpy(engine+0x498,&array,sizeof array);memcpy(player+0x68,&controller,8);
    if(local_controller((uintptr_t)engine)!=controller)return 0;
    array.count=5;memcpy(engine+0x498,&array,sizeof array);
    return !local_controller((uintptr_t)engine)&&sizeof(ConsoleParms)==0x24&&function_lookup_selftest();
}
__declspec(dllexport) int ME3RuntimeConfigEnvironmentFixture(void) {
    Me3RuntimeConfig config;return runtime_config_load_environment(&config);
}
#endif

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    (void)instance; (void)reserved;
    if (reason != DLL_PROCESS_ATTACH) return TRUE;
    DisableThreadLibraryCalls(instance);

    /* Determine game executable path */
    if (!GetModuleFileNameW(NULL, g_game_root, MAX_PATH)) return TRUE;
    wcscpy(g_executable,g_game_root);
    wchar_t *slash = wcsrchr(g_game_root, L'\\');
    if (!slash || _wcsicmp(slash + 1, L"MassEffect3.exe") != 0) return TRUE;

    /* Normalize to game root directory (up 3 levels from Binaries\\Win64\\MassEffect3.exe) */
    for (int i = 0; i < 3; ++i) {
        slash = wcsrchr(g_game_root, L'\\');
        if (!slash) return TRUE;
        *slash = 0;
    }
    if(!supported_image((uintptr_t)GetModuleHandleW(NULL)))return TRUE;

    /* A missing/disabled DLC folder at startup installs no hooks. File
     * presence does not prove the engine mounted a valid DLC package.
     */
    if (!check_active_dlc_gate(g_game_root)) {
        g_dlc_active = 0;
        return TRUE;
    }

    g_dlc_active = 1;
    g_config_status=runtime_config_load_environment(&g_runtime_config);
    if(g_config_status==ME3_CONFIG_READY) {
        wchar_t start[8];int present=0;
        int requested=-1;
        if(direct_ip_native_read_environment(L"ME3_SQUAD_COOP_START",start,8,&present))
            requested=runtime_network_start_parse(g_runtime_config.role,present?start:NULL);
        if(requested==1)runtime_request_join();
        else if(requested==2)runtime_request_listen();
        else if(requested) {
            InterlockedExchange(&runtime_network_start_status,RUNTIME_START_BAD_REQUEST);
            InterlockedExchange(&g_runtime_error,4);return TRUE;
        }
        HANDLE worker=CreateThread(NULL,0,runtime_start,NULL,0,NULL);
        if(worker)CloseHandle(worker);else InterlockedExchange(&g_runtime_error,3);
    }
    return TRUE;
}
