/* Inert experimental runtime entry, not a playable co-op initializer.
 * File presence is an installation gate, not proof of an engine-mounted DLC.
 */
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <stdint.h>
#include <wchar.h>
#include <string.h>
#include "me3_runtime_identity.h"
#include "runtime_config.h"

static int g_dlc_active = 0;
static wchar_t g_game_root[MAX_PATH] = {0};
static Me3RuntimeConfig g_runtime_config;
static Me3RuntimeConfigStatus g_config_status=ME3_CONFIG_DISABLED;

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
    return 1; /* Active-DLC verified */
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
__declspec(dllexport) int ME3RuntimeIdentitySelfTest(void) {return me3_runtime_identity_selftest();}
__declspec(dllexport) int ME3RuntimeConfigSelfTest(void) {return runtime_config_selftest();}
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
    wchar_t *slash = wcsrchr(g_game_root, L'\\');
    if (!slash || _wcsicmp(slash + 1, L"MassEffect3.exe") != 0) return TRUE;

    /* Normalize to game root directory (up 3 levels from Binaries\\Win64\\MassEffect3.exe) */
    for (int i = 0; i < 3; ++i) {
        slash = wcsrchr(g_game_root, L'\\');
        if (!slash) return TRUE;
        *slash = 0;
    }
    if(!supported_image((uintptr_t)GetModuleHandleW(NULL)))return TRUE;

    /* Active-DLC runtime gate verification:
     * If DLC_MOD_ME3SquadCoop is disabled or unmounted in ME3Tweaks Mod Manager,
     * the ASI plugin remains strictly inert with zero hooks installed.
     */
    if (!check_active_dlc_gate(g_game_root)) {
        g_dlc_active = 0;
        return TRUE;
    }

    g_dlc_active = 1;
    g_config_status=runtime_config_load_environment(&g_runtime_config);
    /* No co-op core is started here. Configuration readiness is separate
     * from DLC file presence and must not be advertised as initialization. */
    return TRUE;
}
