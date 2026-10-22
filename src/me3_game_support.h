#ifndef ME3_GAME_SUPPORT_H
#define ME3_GAME_SUPPORT_H
/* Shared exact-layout UE3 support, extracted unchanged from the protected
 * probe. No front-end startup, file guards, QA bots or travel orchestration. */
#include <stdio.h>
#include <wchar.h>
#include <math.h>
#include "me3_runtime_identity.h"
#define NAME_POOLS_RVA 0x17B33D0
#define OBJECTS_RVA 0x1887E40
static HANDLE log_file = INVALID_HANDLE_VALUE;
/* Layouts checked against LExSDKv2 LE3. The packed return FString starts at
   +0x14, not a C compiler's default +0x18. */
#pragma pack(push, 1)
typedef struct { wchar_t *data; int32_t count, max; } GameString;
typedef struct { GameString command; uint32_t write_log; GameString result; } ConsoleParms;
typedef struct { uintptr_t data; int32_t count, max; } GameArray;
#pragma pack(pop)
_Static_assert(sizeof(ConsoleParms) == 0x24, "ConsoleCommand ABI");
typedef void (*EngineTick)(void*, float);
typedef void (*ProcessEvent)(void*, void*, void*, void*);
typedef void (*EngineProgress)(void*, int32_t, void*, void*);
static void name_of(uintptr_t base, uint32_t index, char *out, size_t size) {
    out[0] = 0;
    uintptr_t pool = read_ptr(base + NAME_POOLS_RVA + (index >> 29) * 8);
    if (!pool) return;
    char text[96] = {0};
    if (!read_at(pool + (index & 0x1fffffff) + 12, text, sizeof text - 1)) return;
    text[sizeof text - 1] = 0;
    snprintf(out, size, "%s", text);
}
static void object_name(uintptr_t base, uintptr_t object, char *out, size_t size) {
    uint32_t index = 0;
    out[0] = 0;
    if (read_at(object + 0x48, &index, 4)) name_of(base, index, out, size);
}
static void line(const char *text) {
    DWORD wrote;
    if (log_file != INVALID_HANDLE_VALUE) WriteFile(log_file, text, (DWORD)strlen(text), &wrote, NULL);
}
static void json_text(const wchar_t *wide, char *out, size_t capacity) {
    char utf8[1600] = {0}; size_t j = 0;
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8, sizeof utf8, NULL, NULL);
    for (size_t i = 0; utf8[i] && j + 2 < capacity; ++i) {
        unsigned char c = (unsigned char)utf8[i];
        if (c < 32) { out[j++] = ' '; continue; }
        if (c == '\\' || c == '"') out[j++] = '\\';
        out[j++] = (char)c;
    }
    out[j] = 0;
}
static void game_string(uintptr_t address, wchar_t *out, size_t capacity) {
    GameString value = {0}; out[0] = 0;
    if (!read_at(address, &value, sizeof value) || !value.data || value.count <= 1 || value.count > 4096) return;
    size_t n = (size_t)value.count < capacity ? (size_t)value.count : capacity;
    if (!read_at((uintptr_t)value.data, out, n * sizeof(wchar_t))) out[0] = 0;
    out[n - 1] = 0;
}
static int is_kind_of(uintptr_t base, uintptr_t object, const char *classname);
#include "package_identity.h"
static uintptr_t local_controller(uintptr_t engine) {
    GameArray players = {0};
    if (!read_at(engine + 0x498, &players, sizeof players) ||
        !players.data || players.count < 1 || players.count > 4 || players.max < players.count) return 0;
    uintptr_t player = read_ptr(players.data);
    return player ? read_ptr(player + 0x68) : 0;
}
#include "lookup_profile.h"
#include "function_lookup_cache.h"
static uintptr_t function_named_scan(uintptr_t base, const char *owner, const char *name,unsigned *examined) {
    GameArray objects = {0};
    if (!read_at(base + OBJECTS_RVA, &objects, sizeof objects) ||
        !objects.data || objects.count < 1 || objects.count > 500000 || objects.max < objects.count) return 0;
    for (int i = 0; i < objects.count; ++i) {
        *examined=(unsigned)i+1;
        uintptr_t object = read_ptr(objects.data + (SIZE_T)i * 8);
        if (!object) continue;
        char text[96]; object_name(base, object, text, sizeof text);
        if (strcmp(text, name) != 0) continue;
        object_name(base, read_ptr(object + 0x50), text, sizeof text);
        if (strcmp(text, "Function") != 0) continue;
        object_name(base, read_ptr(object + 0x40), text, sizeof text);
        if (strcmp(text, owner) == 0) return object;
    }
    return 0;
}
static uintptr_t function_named(uintptr_t base,const char *owner,const char *name) {
    unsigned examined=0;unsigned long long started=lookup_clock();
    uintptr_t found=function_lookup_get(base,owner,name);
    if(found){lookup_record(owner,name,started,1,0);return found;}
    found=function_named_scan(base,owner,name,&examined);
    if(found)function_lookup_put(base,owner,name,found);
    lookup_record(owner,name,started,0,examined);return found;
}
static int is_kind_of(uintptr_t base, uintptr_t object, const char *classname) {
    uintptr_t cls = object ? read_ptr(object + 0x50) : 0;
    for (int depth = 0; cls && depth < 48; ++depth) {
        char name[96]; object_name(base, cls, name, sizeof name);
        if (strcmp(name, classname) == 0) return 1;
        cls = read_ptr(cls + 0x60);
    }
    return 0;
}
static int is_live(uintptr_t base, uintptr_t object) {
    GameArray objects = {0}; int32_t index = -1;
    if (!object || !read_at(object + 8, &index, 4) ||
        !read_at(base + OBJECTS_RVA, &objects, sizeof objects) ||
        index < 0 || index >= objects.count || objects.count > 500000) return 0;
    return read_ptr(objects.data + (SIZE_T)index * 8) == object;
}
static int invoke(uintptr_t base, uintptr_t object, const char *owner, const char *name, void *parms) {
    if (!is_live(base, object) || !is_kind_of(base, object, owner)) return 0;
    uintptr_t fn = function_named(base, owner, name);
    uintptr_t pe = read_ptr(read_ptr(object) + 0x230);
    if (!fn || pe < base || pe >= base + 0x1300000) return 0;
    ((ProcessEvent)pe)((void*)object, (void*)fn, parms, NULL);
    return 1;
}
static int console(uintptr_t base, uintptr_t pc, wchar_t *command) {
    ConsoleParms parms = {0};
    parms.command.data = command;
    parms.command.count = parms.command.max = (int32_t)wcslen(command) + 1;
    parms.write_log = 1;
    return invoke(base, pc, "PlayerController", "ConsoleCommand", &parms);
}
static double distance(const float *a, const float *b) {
    double x = a[0] - b[0], y = a[1] - b[1], z = a[2] - b[2];
    return sqrt(x*x + y*y + z*z);
}
static uintptr_t mode_object(uintptr_t base, uintptr_t manager, const char *classname) {
    GameArray modes = {0};
    if (!read_at(manager + 0x70, &modes, sizeof modes) || modes.count < 0 || modes.count > 64) return 0;
    for (int i = 0; i < modes.count; ++i) {
        uintptr_t object = read_ptr(modes.data + (SIZE_T)i * 8);
        if (is_kind_of(base, object, classname)) return object;
    }
    return 0;
}

#endif
