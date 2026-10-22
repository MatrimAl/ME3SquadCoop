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

static int g_dlc_active = 0;
static wchar_t g_game_root[MAX_PATH] = {0};
static Me3RuntimeConfig g_runtime_config;
static Me3RuntimeConfigStatus g_config_status=ME3_CONFIG_DISABLED;
static volatile LONG g_enable_requested=1;
static volatile LONG g_bridge_installed;
static volatile LONG g_runtime_error;
static wchar_t g_executable[MAX_PATH];
#include "runtime_session.h"
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
    runtime_control_enabled=g_dlc_active&&InterlockedCompareExchange(&g_enable_requested,0,0)&&
        g_config_status==ME3_CONFIG_READY;
    runtime_session_frame(base,(uintptr_t)self,0,now);
    runtime_original_tick(self,delta);
    runtime_session_frame(base,(uintptr_t)self,1,GetTickCount());
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
    line("{\"runtime\":\"validated\",\"full-executable-sha256\":true,\"network-bootstrap\":false,\"playable-campaign-verified\":false}\r\n");
    for(int attempt=0;attempt<120;++attempt) {
        if(!check_active_dlc_gate(g_game_root)||!InterlockedCompareExchange(&g_enable_requested,0,0))return 0;
        if(runtime_install_bridge(base))return 0;
        Sleep(500);
    }
    InterlockedExchange(&g_runtime_error,2);line("{\"runtime\":\"bridge-unavailable\"}\r\n");return 0;
}
__declspec(dllexport) int ME3SquadCoopBridgeInstalled(void) {return InterlockedCompareExchange(&g_bridge_installed,0,0)!=0;}
__declspec(dllexport) int ME3SquadCoopRuntimeError(void) {return InterlockedCompareExchange(&g_runtime_error,0,0);}
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
#include "runtime_core_fixture.h"
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
    Me3RuntimeConfig saved=g_runtime_config;int enabled=runtime_control_enabled,ready=runtime_scope_ready,ok=1;
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
__declspec(dllexport) int ME3RuntimeCameraSelfTest(void) {return guest_camera_adapter_selftest();}
__declspec(dllexport) int ME3RuntimeCinematicSelfTest(void) {return guest_cinematic_registration_selftest();}
__declspec(dllexport) int ME3RuntimeHashFileFixture(const wchar_t *path,const unsigned char *expected) {
    return path&&expected&&runtime_hash_file_matches(path,expected);
}
__declspec(dllexport) int ME3RuntimeIdentitySelfTest(void) {return me3_runtime_identity_selftest();}
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
        HANDLE worker=CreateThread(NULL,0,runtime_start,NULL,0,NULL);
        if(worker)CloseHandle(worker);else InterlockedExchange(&g_runtime_error,3);
    }
    return TRUE;
}
