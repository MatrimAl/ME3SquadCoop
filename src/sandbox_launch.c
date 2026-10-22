/* Low-integrity bounded launcher. Never changes the user's account or registry.
 * Used only after a write-denial self-test; no elevation, no original game writes.
 */
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <sddl.h>
#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include "direct_ip_native.h"

static int fail(const char *what) {
    fprintf(stderr, "%s: Windows error %lu\n", what, GetLastError());
    return 1;
}

static HANDLE low_token(void) {
    HANDLE current = NULL, restricted = NULL;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE | TOKEN_QUERY |
                          TOKEN_ASSIGN_PRIMARY | TOKEN_ADJUST_DEFAULT, &current)) return NULL;
    if (!CreateRestrictedToken(current, DISABLE_MAX_PRIVILEGE, 0, NULL, 0, NULL,
                               0, NULL, &restricted)) { CloseHandle(current); return NULL; }
    CloseHandle(current);
    PSID sid = NULL;
    if (!ConvertStringSidToSidW(L"S-1-16-4096", &sid)) { CloseHandle(restricted); return NULL; }
    TOKEN_MANDATORY_LABEL label;
    ZeroMemory(&label, sizeof label);
    label.Label.Attributes = SE_GROUP_INTEGRITY;
    label.Label.Sid = sid;
    BOOL ok = SetTokenInformation(restricted, TokenIntegrityLevel, &label,
                                  (DWORD)(sizeof label + GetLengthSid(sid)));
    LocalFree(sid);
    if (!ok) { CloseHandle(restricted); return NULL; }
    return restricted;
}

static DWORD integrity_level(void) {
    HANDLE token = NULL; DWORD size = 0, rid = 0;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) return 0;
    GetTokenInformation(token, TokenIntegrityLevel, NULL, 0, &size);
    TOKEN_MANDATORY_LABEL *label = (TOKEN_MANDATORY_LABEL*)HeapAlloc(GetProcessHeap(), 0, size);
    if (label && GetTokenInformation(token, TokenIntegrityLevel, label, size, &size)) {
        DWORD n = *GetSidSubAuthorityCount(label->Label.Sid);
        rid = *GetSidSubAuthority(label->Label.Sid, n - 1);
    }
    if (label) HeapFree(GetProcessHeap(), 0, label);
    CloseHandle(token);
    return rid;
}

static int child_check(const wchar_t *protected_file, const wchar_t *result_file) {
    DWORD il = integrity_level();
    HANDLE f = CreateFileW(protected_file, GENERIC_WRITE, FILE_SHARE_READ, NULL,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    BOOL denied = f == INVALID_HANDLE_VALUE && GetLastError() == ERROR_ACCESS_DENIED;
    if (f != INVALID_HANDLE_VALUE) CloseHandle(f); /* no write, even on failed protection */
    char result[180];
    int len = snprintf(result, sizeof result,
        "{\"integrity\":%lu,\"protected_write_denied\":%s,\"pass\":%s}\n",
        il, denied ? "true" : "false", (il == 4096 && denied) ? "true" : "false");
    HANDLE out = CreateFileW(result_file, GENERIC_WRITE, 0, NULL, CREATE_NEW,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    if (out == INVALID_HANDLE_VALUE) return fail("selftest result");
    DWORD wrote;
    BOOL ok = WriteFile(out, result, len, &wrote, NULL) && wrote == (DWORD)len;
    CloseHandle(out);
    return ok && il == 4096 && denied ? 0 : 2;
}

static int start_child(wchar_t *command, const wchar_t *directory, DWORD seconds, BOOL low) {
    HANDLE token = low ? low_token() : NULL;
    if (low && !token) return fail("restricted low-integrity token");
    if (!low && integrity_level() != 8192) return fail("medium-integrity parent required");
    HANDLE job = CreateJobObjectW(NULL, NULL);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits;
    ZeroMemory(&limits, sizeof limits);
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!job || !SetInformationJobObject(job, JobObjectExtendedLimitInformation,
                                         &limits, sizeof limits)) {
        if (job) CloseHandle(job);
        if (token) CloseHandle(token); return fail("bounded process job");
    }
    STARTUPINFOW si; PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof si); si.cb = sizeof si;
    si.dwFlags = STARTF_USESHOWWINDOW; si.wShowWindow = SW_SHOWMINNOACTIVE;
    ZeroMemory(&pi, sizeof pi);
    BOOL ok = low ? CreateProcessAsUserW(token, NULL, command, NULL, NULL, FALSE,
                                   CREATE_SUSPENDED, NULL, directory, &si, &pi)
                  : CreateProcessW(NULL, command, NULL, NULL, FALSE,
                                   CREATE_SUSPENDED, NULL, directory, &si, &pi);
    if (token) CloseHandle(token);
    if (!ok) { CloseHandle(job); return fail("CreateProcessAsUserW (no unrestricted fallback)"); }
    if (!AssignProcessToJobObject(job, pi.hProcess)) {
        TerminateProcess(pi.hProcess, 3);
        CloseHandle(pi.hThread); CloseHandle(pi.hProcess); CloseHandle(job);
        return fail("AssignProcessToJobObject");
    }
    printf("Started %s child PID %lu for at most %lu seconds\n", low ? "low-integrity" : "share-lock-guarded medium-integrity", pi.dwProcessId, seconds);
    fflush(stdout);
    if (ResumeThread(pi.hThread) == (DWORD)-1) {
        CloseHandle(job); CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
        return fail("ResumeThread");
    }
    CloseHandle(pi.hThread);
    DWORD wait = WaitForSingleObject(pi.hProcess, seconds * 1000);
    DWORD exit_code = 1;
    if (wait == WAIT_TIMEOUT) {
        TerminateJobObject(job, 124);
        WaitForSingleObject(pi.hProcess, 5000);
    }
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hProcess); CloseHandle(job);
    printf("Child exit %lu\n", exit_code);
    return (int)exit_code;
}

int wmain(int argc, wchar_t **argv) {
    if (argc == 4 && wcscmp(argv[1], L"--child-check") == 0)
        return child_check(argv[2], argv[3]);
    wchar_t cmd[32760], own[MAX_PATH];
    if (argc == 4 && wcscmp(argv[1], L"--selftest") == 0) {
        GetModuleFileNameW(NULL, own, MAX_PATH);
        if (_snwprintf(cmd, 32760, L"\"%ls\" --child-check \"%ls\" \"%ls\"", own, argv[2], argv[3]) < 0) return 2;
        return start_child(cmd, NULL, 15, TRUE);
    }
    if ((argc == 4 || argc == 5) && (wcscmp(argv[1], L"--boot-probe") == 0 || wcscmp(argv[1], L"--boot-probe-sharelock") == 0)) {
        /* The caller has just run --selftest. Require its positive result plus
           both opt-in markers. Never run an arbitrary original installation. */
        HANDLE check = CreateFileW(argv[3], GENERIC_READ, FILE_SHARE_READ, NULL,
                                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        char data[256] = {0}; DWORD got = 0;
        if (check == INVALID_HANDLE_VALUE) return fail("protection result missing");
        BOOL read_ok = ReadFile(check, data, sizeof data - 1, &got, NULL);
        CloseHandle(check);
        BOOL low = wcscmp(argv[1], L"--boot-probe") == 0;
        if (!read_ok || !strstr(data, "\"pass\":true")) return 2;
        if (low && !strstr(data, "\"integrity\":4096")) return 2;
        if (!low && (!strstr(data, "\"integrity\":8192") || !strstr(data, "\"share_lock\":true"))) return 2;
        if (wcslen(argv[2]) + 80 >= MAX_PATH) return 2;
        wchar_t marker[MAX_PATH], exe[MAX_PATH], directory[MAX_PATH];
        _snwprintf(marker, MAX_PATH, L"%ls\\.me3-coop-sandbox.json", argv[2]);
        if (GetFileAttributesW(marker) == INVALID_FILE_ATTRIBUTES) return 2;
        _snwprintf(marker, MAX_PATH, L"%ls\\ME3SquadCoop.probe.enabled", argv[2]);
        if (GetFileAttributesW(marker) == INVALID_FILE_ATTRIBUTES) return 2;
        _snwprintf(marker, MAX_PATH, L"%ls\\Binaries\\Win64\\ASI\\lemp.asi", argv[2]);
        if (GetFileAttributesW(marker) != INVALID_FILE_ATTRIBUTES) return 2;
        _snwprintf(exe, MAX_PATH, L"%ls\\Binaries\\Win64\\MassEffect3.exe", argv[2]);
        _snwprintf(directory, MAX_PATH, L"%ls\\Binaries\\Win64", argv[2]);
        DirectIPNativeConfig endpoint;
        wchar_t role[16] = {0}; GetEnvironmentVariableW(L"ME3_SQUAD_COOP_TEST_ROLE",role,16);
        if (direct_ip_native_load_environment(&endpoint,wcscmp(role,L"client")==0) != DIRECT_IP_NATIVE_OK) return 87;
        if (_snwprintf(cmd, 32760, L"\"%ls\" -windowed -ResX=960 -ResY=540 -NoMovies -NoSplash -MULTIHOME=%ls -PORT=%u", exe,endpoint.bind_ipv4,(unsigned)endpoint.port) < 0) return 2;
        DWORD seconds = 35;
        if (argc == 5) {
            wchar_t *end = NULL;
            unsigned long requested = wcstoul(argv[4], &end, 10);
            if (!end || *end || requested < 20 || requested > 480) return 2;
            seconds = (DWORD)requested;
        }
        return start_child(cmd, directory, seconds, low);
    }
    fprintf(stderr, "Usage: sandbox_launch --selftest <medium-file> <low-dir/result.json>\n"
                    "       sandbox_launch --boot-probe <verified-copy> <passed-result.json>\n");
    return 2;
}
