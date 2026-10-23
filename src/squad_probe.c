/* Sandbox-only research probe, NOT a playable co-op mod.
 * Private Documents and original-file protection remain required.
 * Stage 1 adds a checked game-thread bridge. Independent of LEMP.
 * Research layouts: PayDay42/lemp-asi (MIT), LExSDKv2 LE3 headers.
 */
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <math.h>
#include "direct_ip_native.h"
#include "me3_game_support.h"
#include "runtime_config.h"
#include "probe_exception_context.h"
#include "collision_baseline_qa.h"
static DirectIPNativeConfig direct_ip_config;
static uintptr_t probe_module_base,probe_game_module_base;

static wchar_t root[MAX_PATH];
static wchar_t documents[MAX_PATH];
static char test_role[8] = "single";
static int probe_guards_only=0,probe_private_guards_ready=0;
typedef HANDLE (WINAPI *MutexW)(LPSECURITY_ATTRIBUTES,BOOL,LPCWSTR);
static MutexW original_mutex = NULL;
static volatile LONG instance_mutex_isolations = 0;
static LPCWSTR scoped_mutex_name(LPCWSTR name, int allowed, wchar_t *out, size_t capacity) {
    /* Only UE3's ordinary application-instance mutex. In particular never
       change Core/Activation's SingleMgrMutex or any unknown mutex. */
    if (!allowed || !name || wcscmp(name, L"UnrealEngine3_6") != 0) return name;
    _snwprintf(out, capacity, L"UnrealEngine3_6_SquadCoopTest_%lu", GetCurrentProcessId());
    out[capacity - 1] = 0;
    return out;
}
static HANDLE WINAPI private_engine_mutex(LPSECURITY_ATTRIBUTES sa, BOOL owner, LPCWSTR name) {
    uintptr_t caller = (uintptr_t)__builtin_return_address(0);
    uintptr_t base = (uintptr_t)GetModuleHandleW(NULL);
    int allowed = strcmp(test_role, "single") != 0 && caller == base + 0x9be1c5;
    wchar_t scoped[96];
    LPCWSTR selected = scoped_mutex_name(name, allowed, scoped, 96);
    HANDLE result = original_mutex(sa, owner, selected);
    DWORD error = GetLastError();
    if (selected != name) InterlockedIncrement(&instance_mutex_isolations);
    SetLastError(error); /* WinMain immediately tests ERROR_ALREADY_EXISTS. */
    return result;
}
typedef HRESULT (WINAPI *FolderW)(HWND,int,HANDLE,DWORD,LPWSTR);
static FolderW original_folder = NULL;
static volatile LONG folder_redirects = 0;
typedef HANDLE (WINAPI *FileW)(LPCWSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,DWORD,DWORD,HANDLE);
typedef BOOL (WINAPI *DirectoryW)(LPCWSTR,LPSECURITY_ATTRIBUTES);
typedef BOOL (WINAPI *DeleteW)(LPCWSTR);
static FileW original_file = NULL;
static volatile LONG capture_pending = 0, capture_file_redirects = 0;
static DirectoryW original_directory = NULL;
static DeleteW original_delete = NULL;
static volatile LONG blocked_writes = 0;
static int test_action = 0; /* 0 observe, 1 travel, 2 possess, 3 input, 4 listen, 5 join, 6 host pair */
static SRWLOCK blocked_lock = SRWLOCK_INIT;
static wchar_t last_blocked_path[MAX_PATH];
static void blocked_path(const wchar_t *path) {
    AcquireSRWLockExclusive(&blocked_lock);
    wcsncpy(last_blocked_path, path ? path : L"<null>", MAX_PATH - 1);
    last_blocked_path[MAX_PATH - 1] = 0;
    InterlockedIncrement(&blocked_writes);
    ReleaseSRWLockExclusive(&blocked_lock);
}

static int path_inside_test(const wchar_t *path) {
    wchar_t full[MAX_PATH];
    if (!path || !path[0]) return 0;
    DWORD count = GetFullPathNameW(path, MAX_PATH, full, NULL);
    if (!count || count >= MAX_PATH) return 0;
    size_t n = wcslen(root);
    return _wcsnicmp(root, full, n) == 0 && (full[n] == 0 || full[n] == L'\\');
}
static int private_capture_path(const wchar_t *path,wchar_t *out,size_t size) {
    if (!capture_pending || !path || !wcsstr(path,L"\\ScreenShots\\")) return 0;
    const wchar_t *name=wcsrchr(path,L'\\');if(!name)return 0;name++;
    wchar_t prefix[70];_snwprintf(prefix,70,L"host-progress_%lu_",GetCurrentProcessId());
    if(wcsncmp(name,prefix,wcslen(prefix)))_snwprintf(prefix,70,L"guest-progress_%lu_",GetCurrentProcessId());
    size_t start=wcslen(prefix),length=wcslen(name);
    if(wcsncmp(name,prefix,start)||length<=start+4||length>90||wcscmp(name+length-4,L".bmp")) return 0;
    for(size_t i=start;i<length-4;++i) if((name[i]<L'0'||name[i]>L'9')&&name[i]!=L'_')return 0;
    int n=_snwprintf(out,size,L"%ls\\ME3SquadCoopRuntime\\captures\\%ls",root,name);
    return n>0&&(size_t)n<size&&path_inside_test(out);
}
static HANDLE WINAPI private_file(LPCWSTR path, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES sa,
                                  DWORD creation, DWORD attributes, HANDLE template_file) {
    DWORD writes = GENERIC_WRITE | GENERIC_ALL | DELETE | WRITE_DAC | WRITE_OWNER |
                   FILE_WRITE_DATA | FILE_APPEND_DATA | FILE_WRITE_EA | FILE_WRITE_ATTRIBUTES;
    wchar_t screenshot[MAX_PATH];
    if ((access & writes) && creation != OPEN_EXISTING && private_capture_path(path,screenshot,MAX_PATH)) {
        HANDLE output=original_file(screenshot,access,share,sa,CREATE_NEW,attributes,template_file);
        DWORD error=GetLastError();
        if(output!=INVALID_HANDLE_VALUE) { InterlockedExchange(&capture_pending,0);InterlockedIncrement(&capture_file_redirects); }
        SetLastError(error);return output;
    }
    if (((access & writes) || creation != OPEN_EXISTING) && !path_inside_test(path)) {
        blocked_path(path); SetLastError(ERROR_ACCESS_DENIED); return INVALID_HANDLE_VALUE;
    }
    return original_file(path, access, share, sa, creation, attributes, template_file);
}
static BOOL WINAPI private_directory(LPCWSTR path, LPSECURITY_ATTRIBUTES sa) {
    if (!path_inside_test(path)) {
        /* The game's recursive mkdir visits existing ancestors. Match the
           Win32 result without attempting any write outside the sandbox. */
        DWORD attrs = GetFileAttributesW(path);
        if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
            SetLastError(ERROR_ALREADY_EXISTS); return FALSE;
        }
        blocked_path(path); SetLastError(ERROR_ACCESS_DENIED); return FALSE;
    }
    return original_directory(path, sa);
}
static BOOL WINAPI private_delete(LPCWSTR path) {
    if (!path_inside_test(path)) { blocked_path(path); SetLastError(ERROR_ACCESS_DENIED); return FALSE; }
    return original_delete(path);
}
static int guard_file_imports(uintptr_t base) {
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER*)base;
    IMAGE_NT_HEADERS64 *nt = (IMAGE_NT_HEADERS64*)(base + dos->e_lfanew);
    DWORD rva = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    if (!rva) return 0;
    int hooked_file = 0;
    IMAGE_IMPORT_DESCRIPTOR *entry = (IMAGE_IMPORT_DESCRIPTOR*)(base + rva);
    for (; entry->Name; ++entry) {
        if (!entry->OriginalFirstThunk) continue;
        IMAGE_THUNK_DATA64 *names = (IMAGE_THUNK_DATA64*)(base + entry->OriginalFirstThunk);
        IMAGE_THUNK_DATA64 *iat = (IMAGE_THUNK_DATA64*)(base + entry->FirstThunk);
        for (; names->u1.AddressOfData; ++names, ++iat) {
            if (IMAGE_SNAP_BY_ORDINAL64(names->u1.Ordinal)) continue;
            IMAGE_IMPORT_BY_NAME *name = (IMAGE_IMPORT_BY_NAME*)(base + names->u1.AddressOfData);
            PVOID replacement = NULL;
            if (strcmp((char*)name->Name, "CreateFileW") == 0) {
                original_file = (FileW)(uintptr_t)iat->u1.Function; replacement = (PVOID)private_file;
            } else if (strcmp((char*)name->Name, "CreateDirectoryW") == 0) {
                original_directory = (DirectoryW)(uintptr_t)iat->u1.Function; replacement = (PVOID)private_directory;
            } else if (strcmp((char*)name->Name, "DeleteFileW") == 0) {
                original_delete = (DeleteW)(uintptr_t)iat->u1.Function; replacement = (PVOID)private_delete;
            } else if (strcmp((char*)name->Name, "CreateMutexW") == 0) {
                original_mutex = (MutexW)(uintptr_t)iat->u1.Function; replacement = (PVOID)private_engine_mutex;
            }
            if (!replacement) continue;
            DWORD protection;
            if (!VirtualProtect(&iat->u1.Function, sizeof(uintptr_t), PAGE_READWRITE, &protection)) return 0;
            InterlockedExchangePointer((PVOID volatile*)&iat->u1.Function, replacement);
            VirtualProtect(&iat->u1.Function, sizeof(uintptr_t), protection, &protection);
            if (replacement == (PVOID)private_file) hooked_file = 1;
        }
    }
    return hooked_file;
}

static void log_blocked_path(void) {
    char utf8[MAX_PATH * 3 + 1] = {0}, escaped[MAX_PATH * 6 + 1] = {0}, message[MAX_PATH * 6 + 100];
    AcquireSRWLockShared(&blocked_lock);
    WideCharToMultiByte(CP_UTF8, 0, last_blocked_path, -1, utf8, sizeof utf8, NULL, NULL);
    ReleaseSRWLockShared(&blocked_lock);
    size_t j = 0;
    for (size_t i = 0; utf8[i] && j + 2 < sizeof escaped; ++i) {
        unsigned char c = (unsigned char)utf8[i];
        if (c < 32) { escaped[j++] = '?'; continue; }
        if (c == '\\' || c == '"') escaped[j++] = '\\';
        escaped[j++] = (char)c;
    }
    snprintf(message, sizeof message, "{\"blocked_write_count\":%ld,\"private_blocked_path\":\"%s\"}\r\n", blocked_writes, escaped);
    line(message);
}
static HRESULT WINAPI private_folder(HWND hwnd, int folder, HANDLE token, DWORD flags, LPWSTR out) {
    if ((folder & 0xff) == 5 && out) {
        InterlockedIncrement(&folder_redirects);
        wcscpy(out, documents);
        return S_OK;
    }
    return original_folder ? original_folder(hwnd, folder, token, flags, out) : E_FAIL;
}
static int redirect_documents(uintptr_t base) {
    /* Known, validated game image only. This is an IAT redirect, not a code
       detour. Low integrity still blocks original-profile writes if any
       call site does not use this imported function. */
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER*)base;
    IMAGE_NT_HEADERS64 *nt = (IMAGE_NT_HEADERS64*)(base + dos->e_lfanew);
    DWORD import_rva = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
    if (!import_rva) return 0;
    IMAGE_IMPORT_DESCRIPTOR *entry = (IMAGE_IMPORT_DESCRIPTOR*)(base + import_rva);
    for (; entry->Name; ++entry) {
        if (!entry->OriginalFirstThunk) continue;
        IMAGE_THUNK_DATA64 *names = (IMAGE_THUNK_DATA64*)(base + entry->OriginalFirstThunk);
        IMAGE_THUNK_DATA64 *iat = (IMAGE_THUNK_DATA64*)(base + entry->FirstThunk);
        for (; names->u1.AddressOfData; ++names, ++iat) {
            if (IMAGE_SNAP_BY_ORDINAL64(names->u1.Ordinal)) continue;
            IMAGE_IMPORT_BY_NAME *name = (IMAGE_IMPORT_BY_NAME*)(base + names->u1.AddressOfData);
            if (strcmp((char*)name->Name, "SHGetFolderPathW") != 0) continue;
            DWORD protection;
            if (!VirtualProtect(&iat->u1.Function, sizeof(uintptr_t), PAGE_READWRITE, &protection)) return 0;
            original_folder = (FolderW)(uintptr_t)iat->u1.Function;
            InterlockedExchangePointer((PVOID volatile*)&iat->u1.Function, (PVOID)private_folder);
            VirtualProtect(&iat->u1.Function, sizeof(uintptr_t), protection, &protection);
            return 1;
        }
    }
    return 0;
}
static LONG WINAPI exception_observer(PEXCEPTION_POINTERS info) {
    if (info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
        char text[768];
        uintptr_t fault = (uintptr_t)info->ExceptionRecord->ExceptionAddress;
        MEMORY_BASIC_INFORMATION memory;
        ZeroMemory(&memory, sizeof memory);
        int queried=VirtualQuery((void*)fault,&memory,sizeof memory)==sizeof memory;
        int parameters=info->ExceptionRecord->NumberParameters>=2;
        if(probe_exception_format(text,sizeof text,fault,queried,(uintptr_t)memory.AllocationBase,memory.Type,
            probe_module_base,probe_game_module_base,parameters,
            parameters?info->ExceptionRecord->ExceptionInformation[0]:0,
            parameters?info->ExceptionRecord->ExceptionInformation[1]:0,GetTickCount()))line(text);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
static int has_marker(const wchar_t *folder) {
    wchar_t path[MAX_PATH];
    if (wcslen(folder) + 40 >= MAX_PATH) return 0;
    _snwprintf(path, MAX_PATH, L"%ls\\.me3-coop-sandbox.json", folder);
    if (GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES) return 0;
    _snwprintf(path, MAX_PATH, L"%ls\\ME3SquadCoop.probe.enabled", folder);
    return GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES;
}

static EngineTick original_tick = NULL;
static EngineProgress original_progress = NULL;
static DWORD bridge_started = 0, last_state = 0;
static int action_sent = 0;
static int client_prewarm = 0, client_prewarm_joined = 0;
static int host_reload_mars = 0, host_reload_sent = 0;
static uintptr_t saved_pc = 0, saved_player = 0, saved_guest = 0, saved_ai = 0;
static DWORD possession_at = 0, last_skip = 0;
static int possession_stage = 0;
static float initial_guest[3], initial_player[3], baseline_guest[3];
static int baseline_taken = 0, shot_taken = 0;
static uintptr_t initial_squad_player = 0, initial_squad_pawn = 0;
static float initial_camera[3];
static int host_capacity_set = 0;
typedef int (*FindPackageFile)(void*, const wchar_t*, const void*, void*, const void*);
static FindPackageFile original_find_package = NULL;
static void identity_snapshot(uintptr_t base, const char *phase);
static void game_progress(void *self, int32_t kind, void *title, void *error) {
    wchar_t wide[512]; char escaped[2200], text[2350];
    game_string((uintptr_t)error, wide, 512); json_text(wide, escaped, sizeof escaped);
    snprintf(text, sizeof text, "{\"network\":\"progress\",\"progress_kind\":%d,\"pending_error\":\"%s\"}\r\n", kind, escaped);
    line(text);
    static int global_failure_sampled = 0;
    if (test_action == 5 && kind == 4 && wcsstr(wide, L"BioP_Global") && !global_failure_sampled) {
        global_failure_sampled = 1;
        identity_snapshot((uintptr_t)GetModuleHandleW(NULL), "client-global-mismatch");
    }
    original_progress(self, kind, title, error);
}
static int package_file_trace(void *self, const wchar_t *name, const void *guid, void *filename, const void *language) {
    uintptr_t caller = (uintptr_t)__builtin_return_address(0);
    uintptr_t base = (uintptr_t)GetModuleHandleW(NULL);
    int result = original_find_package(self, name, guid, filename, language);
    DWORD error = GetLastError();
    /* Read-only observation at the exact package-map validation call.
       Preserve its return and all output data, including a refusal. */
    if (caller == base + 0x65fbdc || (name && _wcsicmp(name, L"BioP_Global") == 0)) {
        wchar_t input[400] = {0}, output[400]; uint32_t id[4] = {0};
        if (name) { wcsncpy(input, name, 399); input[399] = 0; }
        if (guid) read_at((uintptr_t)guid, id, sizeof id);
        game_string((uintptr_t)filename, output, 400);
        char escaped[1700], output_path[1700], text[3750];
        json_text(input, escaped, sizeof escaped); json_text(output, output_path, sizeof output_path);
        snprintf(text, sizeof text,
            "{\"network\":\"package-file-check\",\"package\":\"%s\",\"remote_guid\":\"%08x-%08x-%08x-%08x\",\"guid_supplied\":%s,\"caller_rva\":\"0x%llx\",\"file_found\":%s,\"private_resolved_path\":\"%s\",\"validation_modified\":false}\r\n",
            escaped, id[0], id[1], id[2], id[3], guid ? "true" : "false", (unsigned long long)(caller - base), result ? "true" : "false", output_path);
        line(text);
        if (!result && wcscmp(input, L"SFXGameContent") == 0 && !identity_client_done) {
            identity_client_done = 1;
            identity_snapshot(base, "client-lookup-failure");
        }
    }
    SetLastError(error); return result;
}
static void install_package_trace(uintptr_t base) {
    if (original_find_package || (test_action != 5 && test_action != 6)) return;
    /* The call at 65fbd3/65fbda loads GPackageFileCache and invokes slot 2
       with (name, guid, output FString, optional language). Do not treat the
       old source comment 'NotifyReceivedText' as a UTF-16 network message:
       this build's handler takes a message-type byte plus an FInBunch. */
    const unsigned char expected[] = {0x48,0x8b,0x0d,0xd6,0xde,0x19,0x01,0xff,0xd3};
    unsigned char observed[sizeof expected];
    if (!read_at(base + 0x65fbd3, observed, sizeof observed) || memcmp(observed, expected, sizeof expected)) return;
    uintptr_t cache = read_ptr(base + 0x17fdab0), vt = cache ? read_ptr(cache) : 0;
    if (vt < base + 0x1069000 || vt >= base + 0x1700000) return;
    uintptr_t function = read_ptr(vt + 0x10);
    /* RVA observed and exercised on this exact image in the package trace
       run (Core through SFXGameContent). Refuse another implementation. */
    if (function != base + 0xabfc40) return;
    DWORD old;
    if (!VirtualProtect((void*)(vt + 0x10), 8, PAGE_READWRITE, &old)) return;
    original_find_package = (FindPackageFile)function;
    PVOID previous = InterlockedCompareExchangePointer((PVOID volatile*)(vt + 0x10), (PVOID)package_file_trace, (PVOID)function);
    VirtualProtect((void*)(vt + 0x10), 8, old, &old);
    if ((uintptr_t)previous != function) { original_find_package = NULL; return; }
    char text[160];
    snprintf(text, sizeof text, "{\"package_trace\":\"installed\",\"original_rva\":\"0x%llx\",\"validation_modified\":false}\r\n", (unsigned long long)(function - base));
    line(text);
}

static void input_bot(uintptr_t base, uintptr_t engine) {
    if (test_action != 3 || possession_stage != 1 || !is_live(base, saved_guest)) return;
    uintptr_t pc = local_controller(engine);
    if (pc != saved_pc || read_ptr(pc + 0x3bc) != saved_guest) return;
    uintptr_t input = read_ptr(pc + 0x6dc);
    if (!is_live(base, input) || !is_kind_of(base, input, "PlayerInput")) return;
    DWORD elapsed = GetTickCount() - possession_at;
    /* Two seconds neutral, then a two-second forward pulse, then neutral.
       No actor position/velocity edits: only the player's normal axes. */
    float value = elapsed >= 2000 && elapsed < 4000 ? 1.0f : 0.0f;
    SIZE_T wrote;
    WriteProcessMemory(GetCurrentProcess(), (void*)(input + 0x194), &value, 4, &wrote);
    WriteProcessMemory(GetCurrentProcess(), (void*)(input + 0x1a4), &value, 4, &wrote);
}
static void possession_test(uintptr_t base, uintptr_t pc, uintptr_t pawn, uintptr_t world,
                            uintptr_t squad, uintptr_t manager, unsigned char mode, DWORD now) {
    unsigned char scratch[64] = {0};
    if (!pc || !pawn || !world || !squad || !manager || now - bridge_started < 22000) return;
    /* Exercise the game's existing skip/unpause interfaces in this private
       synthetic mission, rather than manipulating Kismet or plot flags. */
    if (read_ptr(world + 0x670)) {
        invoke(base, pc, "PlayerController", "SetPause", scratch);
        line("{\"autotest\":\"unpause-request\"}\r\n");
    }
    if (!possession_stage && mode == 8 && now - last_skip >= 5000) {
        last_skip = now;
        uintptr_t cinematic = mode_object(base, manager, "SFXGameModeCinematic");
        int ok = invoke(base, cinematic, "SFXGameModeCinematic", "SkipCinematic", scratch);
        line(ok ? "{\"autotest\":\"cinematic-skip-dispatched\"}\r\n" : "{\"autotest\":\"cinematic-skip-unavailable\"}\r\n");
        return;
    }
    if (possession_stage == 1) {
        float guest_xyz[3] = {0}, player_xyz[3] = {0};
        read_at(saved_guest + 0x108, guest_xyz, sizeof guest_xyz);
        read_at(saved_player + 0x108, player_xyz, sizeof player_xyz);
        uintptr_t camera = read_ptr(pc + 0x684);
        /* FTViewTarget is POV first (0x1c bytes), THEN Target. Reading 0x3d8
           as a pointer is a broken oracle: it reads POV.Location. */
        int camera_match = camera && read_ptr(camera + 0x3f4) == saved_guest;
        float camera_xyz[3] = {0};
        if (camera) read_at(camera + 0x3d8, camera_xyz, sizeof camera_xyz);
        char measure[360];
        snprintf(measure, sizeof measure,
            "{\"possession\":\"sample\",\"held_ms\":%lu,\"guest_distance\":%.3f,\"shepard_distance\":%.3f,\"camera_targets_guest\":%s,\"camera_distance\":%.3f,\"camera_to_guest\":%.3f}\r\n",
            now - possession_at, distance(guest_xyz, initial_guest), distance(player_xyz, initial_player), camera_match ? "true" : "false", distance(camera_xyz, initial_camera), distance(camera_xyz, guest_xyz));
        line(measure);
        if (!baseline_taken && now - possession_at < 2000) {
            memcpy(baseline_guest, guest_xyz, sizeof baseline_guest); baseline_taken = 1;
        }
        if (test_action == 3 && !shot_taken && now - possession_at >= 5000) {
            shot_taken = 1; wchar_t shot[] = L"shot";
            int requested = console(base, pc, shot);
            line(requested ? "{\"screenshot\":\"engine-shot-requested\"}\r\n" : "{\"screenshot\":\"unavailable\"}\r\n");
            snprintf(measure, sizeof measure,
                "{\"movement\":\"result\",\"neutral_distance\":%.3f,\"input_distance\":%.3f,\"shepard_distance\":%.3f,\"camera_targets_guest\":%s,\"camera_distance\":%.3f,\"camera_to_guest\":%.3f}\r\n",
                distance(baseline_guest, initial_guest), distance(guest_xyz, baseline_guest), distance(player_xyz, initial_player), camera_match ? "true" : "false", distance(camera_xyz, initial_camera), distance(camera_xyz, guest_xyz));
            line(measure);
        }
        if (now - possession_at < 8000 && mode != 7 && mode != 8 && pc == saved_pc) return;
        if (!is_live(base, saved_pc) || !is_live(base, saved_player) || !is_live(base, saved_guest)) {
            line("{\"possession\":\"restore-aborted-stale-object\"}\r\n"); possession_stage = 3; return;
        }
        struct { uintptr_t pawn; uint32_t vehicle; } params = {saved_player, 0};
        invoke(base, saved_pc, "BioPlayerController", "Possess", &params);
        int player_ok = read_ptr(saved_pc + 0x3bc) == saved_player && read_ptr(saved_player + 0x374) == saved_pc;
        params.pawn = saved_guest;
        if (is_live(base, saved_ai)) invoke(base, saved_ai, "BioAiController", "Possess", &params);
        int ai_ok = saved_ai && read_ptr(saved_ai + 0x3bc) == saved_guest && read_ptr(saved_guest + 0x374) == saved_ai;
        int squad_ok = read_ptr(squad + 0x2cc) == initial_squad_player && read_ptr(squad + 0x2d4) == initial_squad_pawn;
        uintptr_t restored_camera = read_ptr(saved_pc + 0x684);
        int camera_ok = restored_camera && read_ptr(restored_camera + 0x3f4) == saved_player;
        char text[320];
        snprintf(text, sizeof text, "{\"possession\":\"restored\",\"shepard_controller_match\":%s,\"ashley_ai_match\":%s,\"squad_identity_restored\":%s,\"camera_targets_shepard\":%s}\r\n", player_ok ? "true" : "false", ai_ok ? "true" : "false", squad_ok ? "true" : "false", camera_ok ? "true" : "false");
        line(text); possession_stage = 2; return;
    }
    if (possession_stage || mode == 7 || mode == 8 || mode == 9) return;
    uint32_t allows = 0;
    if (!invoke(base, manager, "SFXGameModeManager", "AllowsMovement", &allows) || !(allows & 1)) return;
    GameArray members = {0};
    if (!read_at(squad + 0x270, &members, sizeof members) || members.count < 2 || members.count > 4) return;
    uintptr_t guest = 0;
    for (int i = 0; i < members.count; ++i) {
        uintptr_t candidate = read_ptr(members.data + (SIZE_T)i * 8);
        if (candidate != pawn && is_kind_of(base, candidate, "SFXPawn_Ashley")) { guest = candidate; break; }
    }
    if (!guest || !is_kind_of(base, pawn, "SFXPawn_Player")) return;
    uintptr_t ai = read_ptr(guest + 0x374);
    if (!is_live(base, ai) || !is_kind_of(base, ai, "BioAiController")) return;
    saved_pc = pc; saved_player = pawn; saved_guest = guest; saved_ai = ai;
    read_at(guest + 0x108, initial_guest, sizeof initial_guest);
    read_at(pawn + 0x108, initial_player, sizeof initial_player);
    initial_squad_player = read_ptr(squad + 0x2cc);
    initial_squad_pawn = read_ptr(squad + 0x2d4);
    uintptr_t camera = read_ptr(pc + 0x684);
    if (camera) read_at(camera + 0x3d8, initial_camera, sizeof initial_camera);
    line(camera && read_ptr(camera + 0x3f4) == pawn
        ? "{\"camera_baseline\":\"shepard-positive\"}\r\n" : "{\"camera_baseline\":\"shepard-negative\"}\r\n");
    line("{\"possession\":\"dispatch\",\"target\":\"Ashley\"}\r\n");
    if (!invoke(base, ai, "BioAiController", "UnPossess", scratch)) return;
    struct { uintptr_t pawn; uint32_t vehicle; } params = {guest, 0};
    invoke(base, pc, "BioPlayerController", "Possess", &params);
    int ok = read_ptr(pc + 0x3bc) == guest && read_ptr(guest + 0x374) == pc;
    char text[160];
    snprintf(text, sizeof text, "{\"possession\":\"returned\",\"ashley_controller_match\":%s}\r\n", ok ? "true" : "false");
    line(text); possession_at = now; possession_stage = 1;
}
static int native_session_role(void) {return test_action;}
#define ME3_PROTECTED_SESSION_ADMISSION_DIAGNOSTICS 1
#include "pair_control.h"
#include "network_actor_identity.h"
#include "network_actor_identity_selftest.h"
#include "actor_channel_diagnostics.h"
#include "render_capture.h"
#include "scene_readiness_diagnostics.h"
#include "narrative_probe.h"
#include "combat_context_probe.h"
#include "native_state_reader.h"
#include "combat_equipment_probe.h"
#include "catchup_ground.h"
#include "progress_probe.h"
#include "guest_camera_adapter.h"
#include "guest_player_context_adapter.h"
#include "guest_collision_restore.h"
#include "guest_cinematic_registration_adapter.h"
#include "combat_fire_bot.h"
#define ME3_SQUAD_COOP_PROTECTED_HIT_TRACE 1
#include "guest_equipment_bootstrap.h"
#include "ammo_change_trace.h"
#include "ammo_wire_trace.h"
#include "ammo_reconcile.h"
#include "fire_rpc_trace.h"
#include "native_fire_trace.h"
#include "guest_hit_trace.h"
#include "guest_hit_trace_selftest.h"
#include "initial_actor_trace.h"
#include "session_control.h"
#include "combat_route_qa.h"
static void game_tick_diagnostics(void *self,uintptr_t base) {
    DWORD now = GetTickCount();
    if (now - last_state < 1000) return;
    last_state = now;
    uintptr_t pc = local_controller((uintptr_t)self);
    uintptr_t pawn = pc ? read_ptr(pc + 0x3bc) : 0;
    uintptr_t world = pc ? read_ptr(pc + 0x198) : 0;
    uintptr_t squad = world ? read_ptr(world + 0x8e8) : 0;
    uintptr_t manager = pc ? read_ptr(pc + 0xc38) : 0;
    unsigned char mode = 255;
    if (manager) read_at(manager + 0xa8, &mode, 1);
    float world_time = 0; if (world) read_at(world + 0x74c, &world_time, 4);
    char pc_name[96], pawn_name[96], world_name[96], pawn_class[96], message[750];
    object_name(base, pc, pc_name, sizeof pc_name);
    object_name(base, pawn, pawn_name, sizeof pawn_name);
    object_name(base, world, world_name, sizeof world_name);
    object_name(base, pawn ? read_ptr(pawn + 0x50) : 0, pawn_class, sizeof pawn_class);
    snprintf(message, sizeof message,
        "{\"game_thread\":%lu,\"elapsed_ms\":%lu,\"local_pc\":\"%s\",\"local_pawn\":\"%s\",\"pawn_class\":\"%s\",\"world\":\"%s\",\"squad_present\":%s,\"mode\":%u,\"world_time\":%.3f}\r\n",
        GetCurrentThreadId(), now - bridge_started, pc_name, pawn_name, pawn_class, world_name, squad ? "true" : "false", mode, world_time);
    line(message);
    if (test_action >= 4) {
        uintptr_t game = world ? read_ptr(world + 0x6b0) : 0;
        unsigned char netmode = 255;
        int32_t max_players = -1, num_players = -1;
        if (world) read_at(world + 0x7c8, &netmode, 1);
        if (game) { read_at(game + 0x42c, &max_players, 4); read_at(game + 0x434, &num_players, 4); }
        if (test_action == 6 && netmode == 2 && game && !host_capacity_set && is_kind_of(base, game, "GameInfo")) {
            int32_t old_allowed = 0;
            read_at(game + 0x430, &old_allowed, 4);
            if (old_allowed < 2) InterlockedExchange((LONG volatile*)(game + 0x430), 2);
            InterlockedExchange((LONG volatile*)(game + 0x42c), 2);
            host_capacity_set = 1;
            snprintf(message, sizeof message, "{\"network\":\"private-capacity-change\",\"previous_max_players\":%d,\"previous_max_allowed\":%d,\"max_players\":2,\"disk_changed\":false}\r\n", max_players, old_allowed);
            line(message); max_players = 2;
        }
        char game_class[96]; object_name(base, game ? read_ptr(game + 0x50) : 0, game_class, sizeof game_class);
        if (test_action == 6 && refresh_host_map && !host_map_attempted && netmode == 2 && pawn && squad && now - bridge_started >= 16000) {
            host_map_attempted = 1;
            host_map_refreshed = identity_refresh_master(base);
        }
        snprintf(message, sizeof message, "{\"network\":\"state\",\"net_mode\":%u,\"game_class\":\"%s\",\"max_players\":%d,\"num_players\":%d,\"native_network_patches\":false}\r\n", netmode, game_class, max_players, num_players);
        line(message);
        if (test_action == 6 && netmode == 2 && !identity_host_done && now - bridge_started >= 16000) {
            identity_host_done = 1;
            identity_snapshot(base, "host-listening");
        }
        uintptr_t pending = read_ptr((uintptr_t)self + 0x9fc);
        if (pending) {
            wchar_t wide[512]; char escaped[2200], text[2350];
            game_string(pending + 0xf4, wide, 512); json_text(wide, escaped, sizeof escaped);
            snprintf(text, sizeof text, "{\"network\":\"pending\",\"pending_error\":\"%s\"}\r\n", escaped);
            line(text);
        }
    }
    if (session_enabled) session_sample(base, (uintptr_t)self, now);
    else pair_control_probe(base, pc, pawn, world, squad, manager, mode, now);
    progress_sample(base, pc, pawn, squad, now);
    if ((test_action == 2 || test_action == 3) && action_sent) possession_test(base, pc, pawn, world, squad, manager, mode, now);
    if (test_action == 6 && host_reload_mars && action_sent && !host_reload_sent && pc && pawn && squad && now - bridge_started >= 24000) {
        unsigned char local_mode = 255; read_at(world + 0x7c8, &local_mode, 1);
        if (local_mode == 0) {
            host_reload_sent = 1;
            line("{\"action\":\"listen-after-mars-prewarm\",\"phase\":\"dispatch\"}\r\n");
            wchar_t listen[] = L"open BioP_ProMar?listen";
            console(base, pc, listen);
            line("{\"action\":\"listen-after-mars-prewarm\",\"phase\":\"returned\"}\r\n");
        }
    }
    if (test_action == 5 && client_prewarm && action_sent && !client_prewarm_joined &&
        pc && pawn && squad && now - bridge_started >= 24000) {
        unsigned char local_mode = 255; read_at(world + 0x7c8, &local_mode, 1);
        if (local_mode == 0) {
            client_prewarm_joined = 1;
            identity_snapshot(base, "client-prewarm");
            line("{\"action\":\"join-after-mars-prewarm\",\"phase\":\"dispatch\"}\r\n");
            wchar_t join[80]; _snwprintf(join,80,L"open %ls:%u",direct_ip_config.peer_ipv4,(unsigned)direct_ip_config.port);
            console(base, pc, join);
            line("{\"action\":\"join-after-mars-prewarm\",\"phase\":\"returned\"}\r\n");
        }
    }
    if (!test_action || action_sent || !pc || now - bridge_started < 12000) return;
    /* No arbitrary commands from files: this test has one fixed travel target. */
    uintptr_t fn = function_named(base, "PlayerController", "ConsoleCommand");
    uintptr_t pe = read_ptr(read_ptr(pc) + 0x230);
    if (!fn || pe < base || pe >= base + 0x1300000) {
        line("{\"action\":\"travel-mars\",\"error\":\"ConsoleCommand or ProcessEvent validation failed\"}\r\n");
        action_sent = 1; return;
    }
    wchar_t command[80];
    wcscpy(command, test_action == 4 || (test_action == 6 && !host_reload_mars) ? L"open BioP_ProMar?listen" :
                    L"open BioP_ProMar");
    if (test_action == 5 && !client_prewarm) _snwprintf(command,80,L"open %ls:%u",direct_ip_config.peer_ipv4,(unsigned)direct_ip_config.port);
    ConsoleParms parms = {0};
    parms.command.data = command;
    parms.command.count = parms.command.max = (int32_t)wcslen(command) + 1;
    parms.write_log = 1;
    action_sent = 1; /* Set before dispatch; map travel may re-enter the engine. */
    line(test_action == 5 && !client_prewarm ? "{\"action\":\"join-loopback\",\"phase\":\"dispatch\"}\r\n" : "{\"action\":\"travel-mars\",\"phase\":\"dispatch\"}\r\n");
    ((ProcessEvent)pe)((void*)pc, (void*)fn, &parms, NULL);
    line(test_action == 5 && !client_prewarm ? "{\"action\":\"join-loopback\",\"phase\":\"returned\"}\r\n" : "{\"action\":\"travel-mars\",\"phase\":\"returned\"}\r\n");
    /* A single diagnostic call. Any engine-owned return string lives only
       until this bounded process exits; never free it with the CRT heap. */
}
static void game_tick(void *self,float delta) {
    uintptr_t base=(uintptr_t)GetModuleHandleW(NULL);
    lookup_profile_emit();unsigned long long whole=lookup_clock(),part=lookup_clock();
    if(!collision_baseline_qa_opted()){initial_actor_trace_install(base);install_package_trace(base);}
    ammo_wire_install(base);
    input_bot(base,(uintptr_t)self);
    if(!combat_route_qa_host_mode())progress_input(base,(uintptr_t)self);
    combat_route_qa_tick(base,(uintptr_t)self,0,GetTickCount());
    if(session_enabled)session_frame(base,(uintptr_t)self,0);else pair_guest_input(base,(uintptr_t)self);
    lookup_record("Probe","PreTick",part,0,0);part=lookup_clock();
    original_tick(self,delta);lookup_record("Engine","OriginalTick",part,0,0);part=lookup_clock();
    if(session_enabled)session_frame(base,(uintptr_t)self,1);
    combat_route_qa_tick(base,(uintptr_t)self,1,GetTickCount());
    ammo_wire_prepare(base);
    ammo_reconcile_tick();
    fire_rpc_trace_prepare();
    native_fire_trace_prepare();
    guest_hit_trace_prepare();
    lookup_record("Probe","PostTick",part,0,0);part=lookup_clock();
    game_tick_diagnostics(self,base);lookup_record("Probe","Diagnostics",part,0,0);
    lookup_record("Probe","TotalTick",whole,0,0);
}
static int install_tick_bridge(uintptr_t base) {
    if(probe_guards_only)return 0;
    uintptr_t engine = read_ptr(base + 0x18b31d0);
    uintptr_t vt = engine ? read_ptr(engine) : 0;
    if (vt != base + 0x13983a0) return 0;
    uintptr_t expected = base + 0xb9dda0;
    uintptr_t slot = vt + 0x2b0;
    if (read_ptr(slot) != expected) return 0;
    /* SFXEngine::Tick(float) prologue and its vtable slot checked against the
       exact executable. That override tail-calls UGameEngine::Tick. */
    unsigned char bytes[11], want[11] = {0x40,0x53,0x55,0x41,0x56,0x48,0x83,0xec,0x50,0x33,0xed};
    if (!read_at(expected, bytes, sizeof bytes) || memcmp(bytes, want, sizeof bytes)) return 0;
    DWORD old;
    if (!VirtualProtect((void*)slot, 8, PAGE_READWRITE, &old)) return 0;
    original_tick = (EngineTick)expected;
    bridge_started = GetTickCount();
    PVOID previous = InterlockedCompareExchangePointer((PVOID volatile*)slot, (PVOID)game_tick, (PVOID)expected);
    VirtualProtect((void*)slot, 8, old, &old);
    if ((uintptr_t)previous != expected) { original_tick = NULL; return 0; }
    line("{\"bridge\":\"installed\",\"slot\":\"0x2b0\",\"expected_rva\":\"0xb9dda0\"}\r\n");
    if (test_action >= 4 && read_ptr(vt + 0x2e0) == base + 0xb9c5c0) {
        unsigned char head[5], expect[5] = {0x48,0x89,0x5c,0x24,0x08};
        if (read_at(base + 0xb9c5c0, head, sizeof head) && memcmp(head, expect, sizeof head) == 0 &&
            VirtualProtect((void*)(vt + 0x2e0), 8, PAGE_READWRITE, &old)) {
            original_progress = (EngineProgress)(base + 0xb9c5c0);
            InterlockedCompareExchangePointer((PVOID volatile*)(vt + 0x2e0), (PVOID)game_progress, (PVOID)original_progress);
            VirtualProtect((void*)(vt + 0x2e0), 8, old, &old);
        }
    }
    return 1;
}
static DWORD WINAPI observe(void *unused) {
    (void)unused;
    uintptr_t base = (uintptr_t)GetModuleHandleW(NULL);
    probe_game_module_base=base;
    if (!supported_image(base)) return 0;
    wchar_t path[MAX_PATH];
    _snwprintf(path, MAX_PATH, L"%ls\\ME3SquadCoopRuntime\\squad_probe_%lu.jsonl", root, GetCurrentProcessId());
    log_file = CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
    if (log_file == INVALID_HANDLE_VALUE) return 0;
    char header[220];
    snprintf(header, sizeof header, "{\"stage\":%d,\"pid\":%lu,\"role\":\"%s\",\"engine_state_read_only\":%s,\"observer_loaded\":true,\"private_documents\":true,\"campaign_coop\":false}\r\n", test_action, GetCurrentProcessId(), test_role, test_action ? "false" : "true");
    line(header);
    if(collision_baseline_qa_opted())line("{\"collision_baseline_qa\":true,\"route_stop\":\"first-host-acquisition\",\"guest_synthetic_input\":false,\"synthetic_scene\":true,\"reacquisition\":true,\"combat_tested\":false}\r\n");
    if(probe_guards_only)line("{\"guards_only_qa\":true,\"engine_bridge_disabled\":true,\"action_plan_ignored\":true,\"private_guards_ready\":true}\r\n");
    AddVectoredExceptionHandler(0, exception_observer);
    int bridge_installed = 0, pe_identity_logged = 0; LONG logged_blocks = -1, logged_isolations = 0;
    for (int tick = 0; tick < 140 && has_marker(root); ++tick) {
        Sleep(1000);
        if (blocked_writes != logged_blocks) { log_blocked_path(); logged_blocks = blocked_writes; }
        if (instance_mutex_isolations != logged_isolations) {
            line("{\"engine_instance_mutex_isolated\":true,\"scope\":\"verified-test-role-and-exact-callsite\",\"activation_modified\":false}\r\n");
            logged_isolations = instance_mutex_isolations;
        }
        if (test_action && !bridge_installed && tick >= 4) bridge_installed = install_tick_bridge(base);
        struct { uintptr_t data; int32_t count, max; } array = {0};
        if (!read_at(base + OBJECTS_RVA, &array, sizeof array) ||
            !array.data || array.count <= 0 || array.count > 500000 || array.max < array.count) continue;
        int controllers = 0, pawns = 0, squads = 0, worlds = 0, conversations = 0, pri = 0, drivers = 0, connections = 0;
        for (int i = 0; i < array.count; ++i) {
            uintptr_t object = read_ptr(array.data + (SIZE_T)i * 8);
            uintptr_t klass = object ? read_ptr(object + 0x50) : 0;
            if (!klass) continue;
            char classname[96], name[96];
            object_name(base, klass, classname, sizeof classname);
            object_name(base, object, name, sizeof name);
            if (!name[0] || strncmp(name, "Default__", 9) == 0) continue;
            if (strstr(classname, "PlayerController")) {
                controllers++;
                if (!pe_identity_logged && strcmp(classname, "SFXPlayerController") == 0) {
                    uintptr_t vt = read_ptr(object), pe = vt ? read_ptr(vt + 0x230) : 0;
                    unsigned char head[16]; char hex[33] = {0}, identity[300];
                    if (pe >= base && pe < base + 0x1300000 && read_at(pe, head, sizeof head)) {
                        for (int k = 0; k < 16; ++k) snprintf(hex + k * 2, 3, "%02x", head[k]);
                        snprintf(identity, sizeof identity,
                            "{\"process_event_identity\":true,\"class\":\"SFXPlayerController\",\"vtable_rva\":\"0x%llx\",\"function_rva\":\"0x%llx\",\"first16\":\"%s\"}\r\n",
                            (unsigned long long)(vt - base), (unsigned long long)(pe - base), hex); line(identity); pe_identity_logged = 1;
                    }
                }
            }
            if (strstr(classname, "Pawn")) pawns++;
            if (strcmp(classname, "BioPlayerSquad") == 0) squads++;
            if (strcmp(classname, "BioWorldInfo") == 0) worlds++;
            if (strcmp(classname, "BioConversationController") == 0) conversations++;
            if ((strstr(classname, "PRI") || strstr(classname, "ReplicationInfo")) && is_kind_of(base, object, "PlayerReplicationInfo")) pri++;
            if (strstr(classname, "NetDriver")) drivers++;
            if (strstr(classname, "Connection")) connections++;
        }
        char message[480];
        snprintf(message, sizeof message, "{\"tick\":%d,\"objects\":%d,\"controllers\":%d,\"pawns\":%d,\"squads\":%d,\"worlds\":%d,\"conversation_objects\":%d,\"pri\":%d,\"net_drivers\":%d,\"net_connections\":%d,\"folder_redirects\":%ld,\"blocked_writes\":%ld}\r\n",
                 tick, array.count, controllers, pawns, squads, worlds, conversations, pri, drivers, connections, folder_redirects, blocked_writes);
        line(message);
    }
    /* The bounded launcher owns process lifetime; the game-thread callback
       can still log until process termination. */
    return 0;
}

/* Explicit protected QA mode: all original isolation gates must succeed
 * before any orchestration decision. Isolation role still chooses private
 * Documents/mutex names; it never forces client gameplay in guards-only mode. */
static int probe_apply_private_guards(uintptr_t base) {
    probe_private_guards_ready=0;
    if(!has_marker(root)||!supported_image(base)||GetFileAttributesW(documents)==INVALID_FILE_ATTRIBUTES||
       !redirect_documents(base)||!guard_file_imports(base))return 0;
    probe_private_guards_ready=1;return 1;
}
static int probe_guards_only_environment(void) {
    wchar_t value[2];int present=0;
    if(!direct_ip_native_read_environment(L"ME3_SQUAD_COOP_GUARDS_ONLY_QA",value,2,&present))return -1;
    if(!present||!wcscmp(value,L"0"))return 0;
    return !wcscmp(value,L"1")?1:-1;
}
static int probe_configure_orchestration(int guards_only) {
    if(!probe_private_guards_ready||(guards_only!=0&&guards_only!=1))return 0;
    if(guards_only) {
        probe_guards_only=1;test_action=0;client_prewarm=client_prewarm_joined=0;
        host_reload_mars=host_reload_sent=0;refresh_host_map=0;
        pair_control_enabled=session_enabled=progress_enabled=0;
        return 1;
    }
    wchar_t plan[MAX_PATH];
    _snwprintf(plan, MAX_PATH, L"%ls\\ME3SquadCoopRuntime\\test-action.txt", root);
    HANDLE action = CreateFileW(plan, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (action != INVALID_HANDLE_VALUE) {
        char value[64] = {0}; DWORD count = 0;
        if (ReadFile(action, value, sizeof value - 1, &count, NULL)) {
            if (strcmp(value, "travel-mars") == 0) test_action = 1;
            else if (strcmp(value, "possess-ashley") == 0) test_action = 2;
            else if (strcmp(value, "move-ashley") == 0) test_action = 3;
            else if (strcmp(value, "host-mars") == 0) test_action = 4;
            else if (strcmp(value, "host-pair") == 0) test_action = 6;
        }
        CloseHandle(action);
    }
    if (strcmp(test_role, "client") == 0) test_action = 5;
    wchar_t prewarm[8] = {0};
    client_prewarm = test_action == 5 && GetEnvironmentVariableW(L"ME3_SQUAD_COOP_PREWARM_MARS", prewarm, 8) == 1 && prewarm[0] == L'1';
    prewarm[0] = 0;
    refresh_host_map = test_action == 6 && GetEnvironmentVariableW(L"ME3_SQUAD_COOP_REFRESH_HOST_MAP", prewarm, 8) == 1 && prewarm[0] == L'1';
    prewarm[0] = 0;
    host_reload_mars = test_action == 6 && GetEnvironmentVariableW(L"ME3_SQUAD_COOP_RELOAD_HOST_MARS", prewarm, 8) == 1 && prewarm[0] == L'1';
    prewarm[0] = 0;
    pair_control_enabled = GetEnvironmentVariableW(L"ME3_SQUAD_COOP_PAIR_CONTROL", prewarm, 8) == 1 && prewarm[0] == L'1';
    prewarm[0] = 0;
    session_enabled = GetEnvironmentVariableW(L"ME3_SQUAD_COOP_SESSION_TEST", prewarm, 8) == 1 && prewarm[0] == L'1';
    progress_enabled = GetEnvironmentVariableW(L"ME3_SQUAD_COOP_PROGRESS_TEST", prewarm, 8) == 1 && prewarm[0] == L'1';
    return 1;
}

#ifndef ME3_SQUAD_COOP_RELEASE
#include "probe_guards_fixture.h"
#endif
#ifndef ME3_SQUAD_COOP_RELEASE
__declspec(dllexport) int SquadRuntimeIdentitySelfTest(void) {return me3_runtime_identity_selftest();}
__declspec(dllexport) int SquadRuntimeConfigSelfTest(void) {return runtime_config_selftest();}
__declspec(dllexport) int SquadProbeSelfTest(void) {
    int value = 0x13579, copy = 0;
    if (!read_at((uintptr_t)&value, &copy, sizeof copy) || copy != value) return 0;
    if (read_at(1, &copy, sizeof copy)) return 0;
    return 1;
}
#endif
__declspec(dllexport) int SquadDirectIPSelfTest(void) { return direct_ip_native_selftest(); }
__declspec(dllexport) int SquadStateReaderSelfTest(void) { return native_state_reader_selftest(); }
__declspec(dllexport) int SquadCatchupGroundSelfTest(void) { return catchup_ground_selftest(); }
__declspec(dllexport) int SquadCombatRouteSelfTest(void) { return combat_route_qa_selftest(); }
__declspec(dllexport) int SquadCombatActionSelfTest(void) { return combat_action_qa_selftest(); }
__declspec(dllexport) int SquadAmmoTraceSelfTest(void) { return ammo_trace_selftest(); }
__declspec(dllexport) int SquadFunctionLookupSelfTest(void) { return function_lookup_selftest(); }
__declspec(dllexport) int SquadNetworkActorSelfTest(void) { return network_actor_identity_selftest(); }
__declspec(dllexport) int SquadAmmoWireSelfTest(void) { return ammo_wire_selftest(); }
__declspec(dllexport) int SquadAmmoReconcileSelfTest(void) { return ammo_rpc_selftest(); }
__declspec(dllexport) int SquadFireRpcTraceSelfTest(void) { return fire_rpc_selftest(); }
__declspec(dllexport) int SquadNativeFireTraceSelfTest(void) { return native_fire_selftest(); }
__declspec(dllexport) int SquadGuestHitTraceSelfTest(void) { return guest_hit_selftest(); }
__declspec(dllexport) int SquadGuestCameraSelfTest(void) { return guest_camera_adapter_selftest(); }
__declspec(dllexport) int SquadGuestCollisionSelfTest(void) { return guest_collision_selftest(); }
__declspec(dllexport) int SquadGuestCinematicSelfTest(void) { return guest_cinematic_registration_selftest(); }
__declspec(dllexport) int SquadSessionGateSelfTest(void) { return session_gate_selftest(); }
__declspec(dllexport) int SquadSessionAdmissionDiagnosticsSelfTest(void) { return session_admission_diagnostics_selftest(); }
__declspec(dllexport) int SquadExceptionContextSelfTest(void) { return probe_exception_context_selftest(); }
__declspec(dllexport) int SquadCollisionBaselineQASelfTest(void) { return collision_baseline_qa_selftest(); }
__declspec(dllexport) int SquadSessionHandleSelfTest(void) {
    unsigned char fake[0x60] = {0}; int32_t index = 27; uint64_t name = 0x1234567800000021ULL;
    memcpy(fake + 8, &index, 4); memcpy(fake + 0x48, &name, 8);
    SessionHandle h = session_capture_handle((uintptr_t)fake);
    if (!session_handle_matches(h)) return 0;
    index++; memcpy(fake + 8, &index, 4);
    if (session_handle_matches(h)) return 0;
    index--; memcpy(fake + 8, &index, 4); name++; memcpy(fake + 0x48, &name, 8);
    if (session_handle_matches(h)) return 0;
    name--; memcpy(fake + 0x48, &name, 8);
    if (!session_handle_matches(h)) return 0;
    h.pointer = 1; if (session_handle_matches(h)) return 0;
    uintptr_t peers[2] = {0x100, 0x200};
    GameArray list = {(uintptr_t)peers, 2, 2};
    if (session_peer_array_member(list, 0x100) != 1 || session_peer_array_member(list, 0x300) != 0) return 0;
    peers[1] = peers[0]; if (session_peer_array_member(list, 0x100) != -1) return 0;
    list.data = 8; if (session_peer_array_member(list, 0x100) != -1) return 0;
    list.count = -1; if (session_peer_array_member(list, 0x100) != -1) return 0;
    list.count = 0; list.data = 0;
    return session_peer_array_member(list, 0x100) == 0;
}
#ifndef ME3_SQUAD_COOP_RELEASE
__declspec(dllexport) int SquadProbeMutexTest(void) {
    wchar_t alias[96];
    const wchar_t *engine = L"UnrealEngine3_6", *activation = L"SingleMgrMutex00000000", *other = L"UnrealEngine3_5";
    const wchar_t *result = scoped_mutex_name(engine, 1, alias, 96);
    const wchar_t *prefix = L"UnrealEngine3_6_SquadCoopTest_";
    return result != engine && wcsncmp(result, prefix, wcslen(prefix)) == 0 &&
           scoped_mutex_name(engine, 0, alias, 96) == engine &&
           scoped_mutex_name(activation, 1, alias, 96) == activation &&
           scoped_mutex_name(other, 1, alias, 96) == other && scoped_mutex_name(NULL, 1, alias, 96) == NULL;
}
__declspec(dllexport) int SquadProbeEnabled(const wchar_t *folder) { return has_marker(folder); }
__declspec(dllexport) int SquadProbePathTest(void) {
    wchar_t saved[MAX_PATH]; wcscpy(saved, root); wcscpy(root, L"X:\\SANDBOX");
    int ok = path_inside_test(L"X:\\SANDBOX\\private.txt") &&
             !path_inside_test(L"X:\\SANDBOX-EVIL\\private.txt") &&
             !path_inside_test(L"X:\\SANDBOX\\..\\original.txt") &&
             !path_inside_test(L"C:\\Users\\original-save.txt");
    wchar_t input[MAX_PATH], mapped[MAX_PATH];LONG previous=capture_pending;
    _snwprintf(input,MAX_PATH,L"X:\\SANDBOX\\ScreenShots\\host-progress_%lu_00000.bmp",GetCurrentProcessId());
    capture_pending=0;ok=ok&&!private_capture_path(input,mapped,MAX_PATH);
    capture_pending=1;ok=ok&&private_capture_path(input,mapped,MAX_PATH)&&path_inside_test(mapped)&&wcsstr(mapped,L"\\captures\\");
    _snwprintf(input,MAX_PATH,L"X:\\SANDBOX\\ScreenShots\\guest-progress_%lu_00000.bmp",GetCurrentProcessId());
    ok=ok&&private_capture_path(input,mapped,MAX_PATH)&&path_inside_test(mapped);
    _snwprintf(input,MAX_PATH,L"X:\\SANDBOX\\ScreenShots\\guest-progress_%lu_00000.bmp",GetCurrentProcessId()+1);
    ok=ok&&!private_capture_path(input,mapped,MAX_PATH);
    ok=ok&&!private_capture_path(L"X:\\SANDBOX\\ScreenShots\\unrelated-user-image.bmp",mapped,MAX_PATH);
    _snwprintf(input,MAX_PATH,L"X:\\SANDBOX\\ScreenShots\\host-progress_%lu_evil.bmp",GetCurrentProcessId());
    ok=ok&&!private_capture_path(input,mapped,MAX_PATH);capture_pending=previous;
    wcscpy(root, saved); return ok;
}
static HRESULT WINAPI fake_other_folder(HWND h, int f, HANDLE t, DWORD flags, LPWSTR out) {
    (void)h; (void)f; (void)t; (void)flags;
    wcscpy(out, L"UNCHANGED_OTHER_FOLDER"); return S_OK;
}
__declspec(dllexport) int SquadProbeFolderTest(void) {
    wchar_t saved[MAX_PATH], output[MAX_PATH];
    wcscpy(saved, documents);
    FolderW previous = original_folder;
    wcscpy(documents, L"X:\\PRIVATE_TEST_DOCUMENTS");
    original_folder = fake_other_folder;
    int ok = private_folder(NULL, 5, NULL, 0, output) == S_OK &&
             wcscmp(output, L"X:\\PRIVATE_TEST_DOCUMENTS") == 0;
    ok = ok && private_folder(NULL, 0, NULL, 0, output) == S_OK &&
              wcscmp(output, L"UNCHANGED_OTHER_FOLDER") == 0;
    original_folder = previous;
    wcscpy(documents, saved);
    return ok;
}
#endif

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason != DLL_PROCESS_ATTACH) return TRUE;
    probe_module_base=(uintptr_t)instance;
    DisableThreadLibraryCalls(instance);
    if (!GetModuleFileNameW(NULL, root, MAX_PATH)) return TRUE;
    wchar_t *slash = wcsrchr(root, L'\\');
    if (!slash || _wcsicmp(slash + 1, L"MassEffect3.exe") != 0) return TRUE;
    for (int i = 0; i < 3; ++i) {
        slash = wcsrchr(root, L'\\'); if (!slash) return TRUE; *slash = 0;
    }
    if (!has_marker(root)) return TRUE;
    if (!supported_image((uintptr_t)GetModuleHandleW(NULL))) return TRUE;
    wchar_t role[16] = {0};
    GetEnvironmentVariableW(L"ME3_SQUAD_COOP_TEST_ROLE", role, 16);
    if (wcscmp(role, L"host") == 0) strcpy(test_role, "host");
    else if (wcscmp(role, L"client") == 0) strcpy(test_role, "client");
    if (direct_ip_native_load_environment(&direct_ip_config,strcmp(test_role,"client")==0) != DIRECT_IP_NATIVE_OK) {
        TerminateProcess(GetCurrentProcess(), 87); return TRUE;
    }
    _snwprintf(documents, MAX_PATH, L"%ls\\ME3SquadCoopRuntime\\Documents%ls%ls", root,
        strcmp(test_role, "single") ? L"-" : L"", strcmp(test_role, "single") ? role : L"");
    if(!probe_apply_private_guards((uintptr_t)GetModuleHandleW(NULL))) {
        TerminateProcess(GetCurrentProcess(),5);return TRUE;
    }
    if(!probe_configure_orchestration(probe_guards_only_environment())) {
        TerminateProcess(GetCurrentProcess(),87);return TRUE;
    }
    HANDLE thread = CreateThread(NULL, 0, observe, NULL, 0, NULL);
    if (thread) CloseHandle(thread);
    return TRUE;
}
