#ifndef ME3_RUNTIME_BINK_MOVIE_H
#define ME3_RUNTIME_BINK_MOVIE_H
/* Exact guest movie-worker repair. The observed native Multi caller ignores
 * failure, so a refused asynchronous dispatch cannot simply return zero.
 * For its observed zero-ID / missing-worker / no-pending-jobs case only, use
 * the original synchronous decoder. Its native preparation and completion
 * remain intact; subsequent native AsyncWait accepts the empty job list.
 * No movie, job, worker-ID, UObject, packet, profile or asset field is written.
 * Include after the ordinary session helpers. Prepare runs on Engine.Tick;
 * the callback runs on the movie worker and reads only immutable pins, its
 * real native arguments and atomic enable/counter cells. */
typedef int (*RuntimeBinkMulti)(void*,const uint32_t*,int);
typedef int (*RuntimeBinkFrame)(void*);
typedef struct RuntimeBinkMovie {
    uintptr_t base,native,proxy;
    HMODULE native_reference,proxy_reference;
    RuntimeBinkMulti original;
    RuntimeBinkFrame sync;
    volatile LONG installed,enabled;
    volatile LONG64 sync_calls,sync_zero_returns;
    LONG64 reported_calls,reported_zero_returns;
    DWORD checked,logged_at;
    unsigned rows;
    int refused;
} RuntimeBinkMovie;
static RuntimeBinkMovie runtime_bink_movie;
static int runtime_bink_movie_multi(void*,const uint32_t*,int);
static int runtime_bink_movie_bytes(uintptr_t at,const unsigned char *expected,size_t count) {
    unsigned char actual[1024];
    return count<=sizeof actual&&read_at(at,actual,count)&&!memcmp(actual,expected,count);
}
#include "runtime_bink_movie_pins.h"
static int runtime_bink_movie_opted(void) {
    static int enabled=-1;
    if(enabled<0){wchar_t value[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_BINK_MOVIE_SYNC_GUARD",value,8);
        enabled=!n||(n==1&&value[0]==L'1');}
    return enabled;
}
static int runtime_bink_movie_inputs(uintptr_t base,uintptr_t native,void *movie,const uint32_t *ids,int count) {
    uint32_t workers[4],jobs[8];uintptr_t worker=1,handle=(uintptr_t)movie;
    if(!handle||handle<0x10000||handle>UINTPTR_MAX-0x134||count!=4||
       (uintptr_t)ids!=base+0x18942c0||!read_at((uintptr_t)ids,workers,sizeof workers)||
       workers[0]||workers[1]||workers[2]||workers[3]||
       !read_at(native+0x5c5c0,&worker,8)||worker||
       !read_at(handle+0xf0,jobs,sizeof jobs))return 0;
    for(unsigned i=0;i<8;++i)if(jobs[i])return 0;
    return 1;
}
static int runtime_bink_movie_owned(const RuntimeBinkMovie *s) {
    return s->base&&s->native&&s->proxy&&s->original==(RuntimeBinkMulti)(s->native+0xc050)&&
        s->sync==(RuntimeBinkFrame)(s->native+0x8370)&&
        read_ptr(s->base+0x1069430)==(uintptr_t)runtime_bink_movie_multi&&
        runtime_bink_movie_pins(s->base,s->native,s->proxy);
}
static int runtime_bink_movie_admit(void *movie,const uint32_t *ids,int count,uintptr_t caller) {
    RuntimeBinkMovie *s=&runtime_bink_movie;
    if(!InterlockedCompareExchange(&s->installed,0,0)||!InterlockedCompareExchange(&s->enabled,0,0)||
       !InterlockedCompareExchange(&g_enable_requested,0,0)||caller!=s->base+0x481acf||
       !runtime_bink_movie_inputs(s->base,s->native,movie,ids,count)||!runtime_bink_movie_owned(s))return 0;
    /* Re-read the actual mutable inputs after validating the owned hook and
     * original code. No cached UObject/session lookup runs on this worker. */
    return InterlockedCompareExchange(&s->enabled,0,0)&&InterlockedCompareExchange(&g_enable_requested,0,0)&&
        runtime_bink_movie_inputs(s->base,s->native,movie,ids,count)&&
        read_ptr(s->base+0x1069430)==(uintptr_t)runtime_bink_movie_multi;
}
static int runtime_bink_movie_forward(void *movie,const uint32_t *ids,int count,uintptr_t caller,
        RuntimeBinkMulti original,RuntimeBinkFrame sync,
        int (*admit)(void*,const uint32_t*,int,uintptr_t),int *used_sync) {
    DWORD error=GetLastError();int selected=admit(movie,ids,count,caller);
    *used_sync=selected;SetLastError(error);
    return selected?sync(movie):original(movie,ids,count);
}
static int runtime_bink_movie_multi(void *movie,const uint32_t *ids,int count) {
    RuntimeBinkMovie *s=&runtime_bink_movie;int selected=0;
    int result=runtime_bink_movie_forward(movie,ids,count,(uintptr_t)__builtin_return_address(0),
        s->original,s->sync,runtime_bink_movie_admit,&selected);
    DWORD error=GetLastError();
    if(selected){InterlockedIncrement64(&s->sync_calls);if(!result)InterlockedIncrement64(&s->sync_zero_returns);}
    SetLastError(error);return result;
}
static int runtime_bink_movie_path(HMODULE module,const wchar_t *name,wchar_t path[MAX_PATH]) {
    wchar_t expected[MAX_PATH],parents[MAX_PATH];
    DWORD n=GetModuleFileNameW(module,path,MAX_PATH);
    int size=_snwprintf(expected,MAX_PATH,L"%ls\\Binaries\\Win64\\%ls",g_game_root,name);
    if(!n||n>=MAX_PATH||size<=0||size>=MAX_PATH||_wcsicmp(path,expected)||
       path[1]!=L':'||path[2]!=L'\\')return 0;
    wcscpy(parents,path);
    for(;;){DWORD flags=GetFileAttributesW(parents);
        if(flags==INVALID_FILE_ATTRIBUTES||(flags&FILE_ATTRIBUTE_REPARSE_POINT))return 0;
        wchar_t *slash=wcsrchr(parents,L'\\');if(!slash||slash<=parents+2)break;*slash=0;}
    MEMORY_BASIC_INFORMATION memory;
    return VirtualQuery(module,&memory,sizeof memory)==sizeof memory&&memory.Type==MEM_IMAGE&&memory.AllocationBase==module;
}
static void runtime_bink_movie_flush(DWORD now) {
    RuntimeBinkMovie *s=&runtime_bink_movie;
    if(s->logged_at&&now-s->logged_at<1000)return;s->logged_at=now;
    LONG64 calls=InterlockedCompareExchange64(&s->sync_calls,0,0),zeros=InterlockedCompareExchange64(&s->sync_zero_returns,0,0);
    if(calls==s->reported_calls&&zeros==s->reported_zero_returns)return;
    s->reported_calls=calls;s->reported_zero_returns=zeros;
    if(s->rows++<32){char row[520];snprintf(row,sizeof row,
        "{\"runtime_bink_movie\":\"native-sync-fallback\",\"calls\":%llu,\"native_zero_returns\":%llu,\"worker_thread_callback\":true,\"logged_on_game_thread\":true,\"native_return_preserved\":true,\"movie_fields_written\":false,\"worker_ids_written\":false,\"gameplay_verified\":false,\"monotonic_ms\":%lu}\r\n",
        (unsigned long long)calls,(unsigned long long)zeros,now);line(row);}
}
static void runtime_bink_movie_prepare(uintptr_t base,DWORD now) {
    RuntimeBinkMovie *s=&runtime_bink_movie;
    if(!session_game_thread||GetCurrentThreadId()!=session_game_thread)return;
    int enabled=runtime_bink_movie_opted()&&g_runtime_config.role==ME3_ROLE_GUEST&&g_config_status==ME3_CONFIG_READY&&
        runtime_control_enabled&&InterlockedCompareExchange(&g_enable_requested,0,0)&&
        InterlockedCompareExchange(&g_bridge_installed,0,0);
    InterlockedExchange(&s->enabled,enabled&&InterlockedCompareExchange(&s->installed,0,0)&&
        read_ptr(base+0x1069430)==(uintptr_t)runtime_bink_movie_multi);
    runtime_bink_movie_flush(now);
    if(!enabled||s->installed||s->refused||(s->checked&&now-s->checked<1000))return;s->checked=now;
    HMODULE proxy=NULL,native=NULL;wchar_t proxy_path[MAX_PATH],native_path[MAX_PATH],exe_path[MAX_PATH];
    /* Both modules must already be loaded. Retain references for this resident
     * slot hook; do not load a DLL, create a decoder thread or change assets. */
    if(!GetModuleHandleExW(0,L"bink2w64.dll",&proxy))return;
    if(!GetModuleHandleExW(0,L"bink2w64_original.dll",&native)){FreeLibrary(proxy);return;}
    int paths=runtime_bink_movie_path((HMODULE)base,L"MassEffect3.exe",exe_path)&&!_wcsicmp(exe_path,g_executable)&&
        runtime_bink_movie_path(proxy,L"bink2w64.dll",proxy_path)&&runtime_bink_movie_path(native,L"bink2w64_original.dll",native_path);
    if(!paths||!supported_image(base)||!runtime_hash_file_matches(proxy_path,runtime_bink_movie_proxy_sha)||
       !runtime_hash_file_matches(native_path,runtime_bink_movie_native_sha)||
       !runtime_bink_movie_pins(base,(uintptr_t)native,(uintptr_t)proxy)){
        s->refused=1;FreeLibrary(native);FreeLibrary(proxy);
        line("{\"runtime_bink_movie\":\"identity-or-code-refused\",\"native_forwarding_unchanged\":true}\r\n");return;}
    FARPROC original=GetProcAddress(proxy,"BinkDoFrameAsyncMulti");
    if((uintptr_t)original!=(uintptr_t)native+0xc050||
       (uintptr_t)GetProcAddress(native,"BinkDoFrame")!=(uintptr_t)native+0x8370||
       (uintptr_t)GetProcAddress(native,"BinkDoFrameAsyncWait")!=(uintptr_t)native+0xbeb0||
       read_ptr(base+0x1069430)!=(uintptr_t)original){FreeLibrary(native);FreeLibrary(proxy);return;}
    DWORD protection=0;
    if(!VirtualProtect((void*)(base+0x1069430),8,PAGE_READWRITE,&protection)){FreeLibrary(native);FreeLibrary(proxy);return;}
    s->base=base;s->native=(uintptr_t)native;s->proxy=(uintptr_t)proxy;
    s->original=(RuntimeBinkMulti)original;s->sync=(RuntimeBinkFrame)((uintptr_t)native+0x8370);
    s->native_reference=native;s->proxy_reference=proxy;
    /* Publish immutable targets and admission before the slot becomes visible
     * to an already running movie worker. The callback also requires ownership
     * of the actual slot, so it cannot adapt a pre-install call. */
    InterlockedExchange(&s->installed,1);InterlockedExchange(&s->enabled,1);
    PVOID previous=InterlockedCompareExchangePointer((PVOID volatile*)(base+0x1069430),
        (PVOID)runtime_bink_movie_multi,(PVOID)original);
    DWORD ignored;VirtualProtect((void*)(base+0x1069430),8,protection,&ignored);
    if(previous!=(PVOID)original){InterlockedExchange(&s->enabled,0);InterlockedExchange(&s->installed,0);s->original=NULL;s->sync=NULL;
        s->native_reference=s->proxy_reference=NULL;FreeLibrary(native);FreeLibrary(proxy);return;}
    InterlockedExchange(&s->enabled,1);
    line("{\"runtime_bink_movie\":\"guest-native-sync-guard-installed\",\"iat_slot_owned\":true,\"native_forwarder_verified\":true,\"gameplay_verified\":false}\r\n");
}
#ifdef ME3_RUNTIME_FIXTURE
static int runtime_bink_fixture_selected,runtime_bink_fixture_calls,runtime_bink_fixture_bad,runtime_bink_fixture_result;
static void *runtime_bink_fixture_movie;
static const uint32_t *runtime_bink_fixture_ids;
static int runtime_bink_fixture_admit(void *movie,const uint32_t *ids,int count,uintptr_t caller) {
    if(movie!=runtime_bink_fixture_movie||ids!=runtime_bink_fixture_ids||count!=4||caller!=77)runtime_bink_fixture_bad=1;
    SetLastError(11);return runtime_bink_fixture_selected;
}
static int runtime_bink_fixture_multi(void *movie,const uint32_t *ids,int count) {
    if(movie!=runtime_bink_fixture_movie||ids!=runtime_bink_fixture_ids||count!=4||GetLastError()!=37||runtime_bink_fixture_selected)runtime_bink_fixture_bad=1;
    ++runtime_bink_fixture_calls;SetLastError(53);return runtime_bink_fixture_result;
}
static int runtime_bink_fixture_frame(void *movie) {
    if(movie!=runtime_bink_fixture_movie||GetLastError()!=37||!runtime_bink_fixture_selected)runtime_bink_fixture_bad=1;
    ++runtime_bink_fixture_calls;SetLastError(53);return runtime_bink_fixture_result;
}
static int runtime_bink_movie_selftest(void) {
    DWORD error=GetLastError();int ok=1;
    uint32_t ids[4]={0},movie[0x134/4]={0};uintptr_t worker=0;
    uintptr_t base=(uintptr_t)ids-0x18942c0,native=(uintptr_t)&worker-0x5c5c0;
    ok=ok&&runtime_bink_movie_inputs(base,native,movie,ids,4)&&!runtime_bink_movie_inputs(base,native,movie,ids,3)&&
        !runtime_bink_movie_inputs(base,native,movie,ids+1,4)&&!runtime_bink_movie_inputs(base,native,NULL,ids,4);
    for(unsigned i=0;i<4;++i){ids[i]=1;ok=ok&&!runtime_bink_movie_inputs(base,native,movie,ids,4);ids[i]=0;}
    worker=1;ok=ok&&!runtime_bink_movie_inputs(base,native,movie,ids,4);worker=0;
    for(unsigned i=0;i<8;++i){movie[0xf0/4+i]=1;ok=ok&&!runtime_bink_movie_inputs(base,native,movie,ids,4);movie[0xf0/4+i]=0;}
    runtime_bink_fixture_movie=movie;runtime_bink_fixture_ids=ids;
    for(int selected=0;selected<2;++selected)for(int zero=0;zero<2;++zero){int used=-1;
        runtime_bink_fixture_selected=selected;runtime_bink_fixture_calls=runtime_bink_fixture_bad=0;
        runtime_bink_fixture_result=zero?0:17;SetLastError(37);
        int result=runtime_bink_movie_forward(movie,ids,4,77,runtime_bink_fixture_multi,runtime_bink_fixture_frame,runtime_bink_fixture_admit,&used);
        ok=ok&&result==runtime_bink_fixture_result&&used==selected&&runtime_bink_fixture_calls==1&&!runtime_bink_fixture_bad&&GetLastError()==53;}
    SetLastError(error);return ok;
}
#endif
#endif
