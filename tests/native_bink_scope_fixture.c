#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "bink_scope_types.h"

static struct { Me3RuntimeRole role; } g_runtime_config;
static Me3RuntimeConfigStatus g_config_status;
static DWORD session_game_thread;
static int runtime_control_enabled,opted,slot_owned,flushes;
static volatile LONG g_enable_requested,g_bridge_installed;
static int runtime_bink_movie_multi(void *movie,const uint32_t *ids,int count) {
    (void)movie;(void)ids;(void)count;return 0;
}
static int runtime_bink_movie_opted(void) {return opted;}
static uintptr_t read_ptr(uintptr_t at) {
    return at==0x10000000+0x1069430&&slot_owned?(uintptr_t)runtime_bink_movie_multi:0;
}
static void runtime_bink_movie_flush(DWORD now) {(void)now;++flushes;}
#include "bink_scope_prefix.h"

static unsigned checks;
static int expect(int value,const char *name,int role) {
    ++checks;
    if(value)return 1;
    fprintf(stderr,"FAIL %s role=%d\n",name,role);return 0;
}
static void reset(int role) {
    memset(&runtime_bink_movie,0,sizeof runtime_bink_movie);
    runtime_bink_movie.installed=1;
    g_runtime_config.role=(Me3RuntimeRole)role;g_config_status=ME3_CONFIG_READY;
    session_game_thread=GetCurrentThreadId();runtime_control_enabled=opted=slot_owned=1;
    g_enable_requested=g_bridge_installed=1;flushes=0;
}
static int run(int expected,const char *name,int role) {
    runtime_bink_movie_prepare(0x10000000,100);
    return expect(runtime_bink_movie.enabled==expected&&flushes==1,name,role);
}
int main(void) {
    int ok=1;
    for(int role=-1;role<=3;++role) {
        int admitted=role==1||role==2;
        reset(role);ok=run(admitted,"configured role",role)&&ok;
        reset(role);opted=0;ok=run(0,"explicit opt-out",role)&&ok;
        reset(role);runtime_control_enabled=0;ok=run(0,"control disabled",role)&&ok;
        reset(role);g_enable_requested=0;ok=run(0,"runtime enable absent",role)&&ok;
        reset(role);g_bridge_installed=0;ok=run(0,"bridge absent",role)&&ok;
        reset(role);runtime_bink_movie.installed=0;ok=run(0,"hook not installed",role)&&ok;
        reset(role);slot_owned=0;ok=run(0,"hook slot replaced",role)&&ok;
        for(int status=0;status<=6;++status)if(status!=1) {
            reset(role);g_config_status=(Me3RuntimeConfigStatus)status;
            ok=run(0,"configuration not ready",role)&&ok;
        }
        reset(role);session_game_thread=0;runtime_bink_movie.enabled=7;
        runtime_bink_movie_prepare(0x10000000,100);
        ok=expect(runtime_bink_movie.enabled==7&&flushes==0,"no game thread",role)&&ok;
        reset(role);++session_game_thread;runtime_bink_movie.enabled=7;
        runtime_bink_movie_prepare(0x10000000,100);
        ok=expect(runtime_bink_movie.enabled==7&&flushes==0,"foreign thread",role)&&ok;
    }
    if(ok)printf("PASS production Bink preparation scope: %u checks\n",checks);
    return ok?0:1;
}
