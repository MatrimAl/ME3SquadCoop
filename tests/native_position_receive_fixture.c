/* Actual receive wrapper on authored memory only. No game/process access. */
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
typedef struct {uintptr_t data;int count,max;} GameArray;
typedef struct {int unused;} AmmoWireReceive;
static int progress_enabled=1,test_action=5;
static char test_role[8]="client";
static wchar_t root[]=L"fixture";
static uintptr_t network_actor_received_bunch_observer;
static unsigned char pawn[0x800],channel[0x100],world[0x100],level[0x100],info[0x800],bunch[0x100];
static uintptr_t actors[1],fake_base=0x10000000;
static int marker=1,ashley=1,forward_calls,mode,records;
static char logged[2400];
static int read_at(uintptr_t at,void *out,size_t n) {
    SetLastError(999);
    if(at==fake_base+0x18b6638&&n==8){uintptr_t w=(uintptr_t)world;memcpy(out,&w,8);return 1;}
    unsigned char *buffers[]={pawn,channel,world,level,info,bunch,(unsigned char*)actors};
    size_t sizes[]={sizeof pawn,sizeof channel,sizeof world,sizeof level,sizeof info,sizeof bunch,sizeof actors};
    for(size_t i=0;i<sizeof sizes/sizeof sizes[0];++i)
        if(at>=(uintptr_t)buffers[i]&&n<=sizes[i]&&at-(uintptr_t)buffers[i]<=sizes[i]-n){memcpy(out,(void*)at,n);return 1;}
    return 0;
}
static uintptr_t read_ptr(uintptr_t at){uintptr_t v=0;read_at(at,&v,8);return v;}
static int is_live(uintptr_t base,uintptr_t p){(void)base;return p==(uintptr_t)pawn||p==(uintptr_t)channel||p==(uintptr_t)world||p==(uintptr_t)level||p==(uintptr_t)info;}
static int is_kind_of(uintptr_t base,uintptr_t p,const char *name){(void)base;return ashley&&p==(uintptr_t)pawn&&!strcmp(name,"SFXPawn_Ashley");}
static int has_marker(const wchar_t *p){(void)p;return marker;}
static void line(const char *s){++records;snprintf(logged,sizeof logged,"%s",s);SetLastError(998);}
static void ammo_wire_receive_begin(AmmoWireReceive *s,uintptr_t c){(void)s;(void)c;SetLastError(997);}
static void ammo_wire_receive_end(AmmoWireReceive *s){(void)s;SetLastError(996);}
static int ammo_rpc_opted(void){return 0;}
static int fire_rpc_opted(void){return 0;}
static int supported_image(uintptr_t base){(void)base;return 0;}
#include "initial_actor_trace.h"
static void putptr(unsigned char *b,size_t off,uintptr_t v){memcpy(b+off,&v,8);}
static void original(void *self,void *incoming) {
    assert(self==channel&&incoming==bunch&&GetLastError()==123);
    ++forward_calls;
    if(mode==1){float v[3]={78,-128,-72};memcpy(pawn+0x108,v,12);}
    if(mode==2)pawn[0x269]=0;
    if(mode==3){int i=999;memcpy(pawn+0x38,&i,4);}
    if(mode==4){float v=NAN;memcpy(pawn+0x108,&v,4);}
    if(mode==5){float v[3]={-57118,-105857,71};memcpy(pawn+0x138,v,12);}
    if(mode==6)pawn[0x268]=10;
    if(mode==7){putptr(channel,0xa0,0);uint32_t flags=0x4000000;memcpy(pawn+0x25c,&flags,4);pawn[0x268]=10;}
    if(mode==8)putptr(channel,0xa0,(uintptr_t)world);
    SetLastError(456);
}
static void setup(void) {
    memset(pawn,0,sizeof pawn);memset(channel,0,sizeof channel);memset(world,0,sizeof world);
    memset(level,0,sizeof level);memset(info,0,sizeof info);memset(bunch,0xa5,sizeof bunch);
    putptr(channel,0,fake_base+0x11a2150);putptr(channel,0xa0,(uintptr_t)pawn);
    putptr(pawn,0x50,0x12345);putptr(pawn,0x198,(uintptr_t)info);
    putptr(world,0x88,(uintptr_t)level);actors[0]=(uintptr_t)info;
    GameArray a={(uintptr_t)actors,1,1};memcpy(level+0x60,&a,sizeof a);info[0x7c8]=3;
    int ix=42;memcpy(pawn+0x38,&ix,4);float pos[3]={-22319,49125,230};memcpy(pawn+0x108,pos,12);
    pawn[0x269]=3;pawn[0x26a]=2;
    initial_actor_trace_base=fake_base;initial_actor_trace_thread=GetCurrentThreadId();
    initial_actor_original_bunch=original;position_receive_events=0;
    progress_enabled=1;marker=ashley=1;test_action=5;mode=records=forward_calls=0;logged[0]=0;
}
static void run(int expected_records) {
    unsigned char old_bunch[sizeof bunch],old_channel[sizeof channel];
    memcpy(old_bunch,bunch,sizeof bunch);memcpy(old_channel,channel,sizeof channel);
    SetLastError(123);initial_actor_received_bunch(channel,bunch);
    assert(forward_calls==1&&GetLastError()==456&&records==expected_records);
    if(mode==7)putptr(old_channel,0xa0,0);
    if(mode==8)putptr(old_channel,0xa0,(uintptr_t)world);
    assert(!memcmp(old_bunch,bunch,sizeof bunch)&&!memcmp(old_channel,channel,sizeof channel));
}
int main(int argc,char **argv) {
    /* Separate processes exercise the cached opt-in parser. */
    assert(argc==2);SetEnvironmentVariableW(L"ME3_SQUAD_COOP_POSITION_RECEIVE_TRACE",argv[1][0]=='1'?L"1":L"0");
    setup();mode=1;run(argv[1][0]=='1');
    if(argv[1][0]!='1'){puts("PASS default-off: forward once, LastError and packet preserved");return 0;}
    assert(strstr(logged,"change-in-original-receive")&&strstr(logged,"78.000,-128.000,-72.000"));
    setup();run(0);
    setup();mode=5;run(1);assert(strstr(logged,"-57118.000,-105857.000,71.000"));
    setup();mode=6;run(1);assert(strstr(logged,"\"physics_after\":10"));
    setup();mode=7;run(1);assert(strstr(logged,"\"channel_actor_cleared\":true")&&strstr(logged,"\"actor_flags_after\":67108864"));
    setup();mode=8;run(0);
    setup();mode=2;run(1);
    setup();mode=3;run(0);
    setup();mode=4;run(0);
    setup();mode=1;position_receive_events=64;run(0);
    setup();mode=1;progress_enabled=0;run(0);
    setup();mode=1;marker=0;run(0);
    setup();mode=1;ashley=0;run(0);
    setup();mode=1;test_action=6;run(0);
    setup();mode=1;initial_actor_trace_thread++;run(0);
    setup();mode=1;info[0x7c8]=0;run(0);
    puts("PASS 16 receive-wrapper cases: changed/unchanged, identity/nonfinite/budget/context guards, forward once, LastError and packet preserved");
    return 0;
}
