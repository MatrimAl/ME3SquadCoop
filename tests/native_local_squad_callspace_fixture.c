/* Exercise the actual scope-filtering callback; no game or hooks are installed. */
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef struct GuestPlayerContextMethod {struct {uintptr_t pointer;} function;} GuestPlayerContextMethod;
static struct {GuestPlayerContextMethod set_pawn;} guest_player_context_methods;
static int guest_player_context_methods_valid(uintptr_t b,int f){(void)b;(void)f;return 0;}
static int guest_camera_actor_live(uintptr_t b,uintptr_t a){(void)b;(void)a;return 0;}
static uintptr_t read_ptr(uintptr_t a){(void)a;return 0;}
static int read_at(uintptr_t a,void *v,size_t n){(void)a;(void)v;(void)n;return 0;}
static int guest_player_context_dispatch(uintptr_t b,uintptr_t a,GuestPlayerContextMethod *m,void *p){(void)b;(void)a;(void)m;(void)p;return 0;}
static void line(const char *s){(void)s;}
#include "runtime_local_squad_set.h"
static int native_result=1,native_calls;
static int original(void *a,void *f,void *p,void *c){(void)a;(void)f;(void)p;(void)c;++native_calls;return native_result;}
static DWORD WINAPI other_thread(void *p){(void)p;return (DWORD)local_squad_callspace((void*)1,(void*)2,NULL,NULL);}
#define CHECK(x) do {if(!(x)){printf("FAIL local-squad line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){
    local_squad_tls=TlsAlloc();CHECK(local_squad_tls!=TLS_OUT_OF_INDEXES);local_squad_original=original;
    CHECK(local_squad_callspace((void*)1,(void*)2,NULL,NULL)==1);
    LocalSquadCall scope={1,2,0};CHECK(TlsSetValue(local_squad_tls,&scope));
    CHECK(local_squad_callspace((void*)3,(void*)2,NULL,NULL)==1&&!scope.adapted);
    CHECK(local_squad_callspace((void*)1,(void*)3,NULL,NULL)==1&&!scope.adapted);
    CHECK(local_squad_callspace((void*)1,(void*)2,NULL,(void*)1)==1&&!scope.adapted);
    native_result=0;CHECK(local_squad_callspace((void*)1,(void*)2,NULL,NULL)==0&&!scope.adapted);
    native_result=1;HANDLE t=CreateThread(NULL,0,other_thread,NULL,0,NULL);CHECK(t);
    CHECK(WaitForSingleObject(t,5000)==WAIT_OBJECT_0);DWORD result=0;CHECK(GetExitCodeThread(t,&result)&&result==1);CloseHandle(t);
    CHECK(local_squad_callspace((void*)1,(void*)2,NULL,NULL)==0&&scope.adapted);
    CHECK(local_squad_callspace((void*)1,(void*)2,NULL,NULL)==1);
    CHECK(TlsSetValue(local_squad_tls,NULL));CHECK(TlsFree(local_squad_tls));
    CHECK(native_calls==8);
    puts("PASS local squad callspace scope, original forwarding, one-shot and other-thread isolation; no game launched");return 0;
}
