/* Actual receive wrapper on authored memory only. No game/process access. */
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
/* Zig ReleaseFast may compile libc assert to a no-op. These checks must run. */
#define CHECK(condition) do { if(!(condition)) { fprintf(stderr,"CHECK failed at line %d: %s\n",__LINE__,#condition); exit(2); } } while(0)
typedef struct {uintptr_t data;int count,max;} GameArray;
typedef struct {int unused;} AmmoWireReceive;
static int progress_enabled=1,test_action=5;
static char test_role[8]="client";
static wchar_t root[]=L"fixture";
static uintptr_t network_actor_received_bunch_observer;
static unsigned char pawn[0x800],channel[0x100],world[0x100],level[0x100],info[0x800],bunch[0x100];
static unsigned char property[0x100],archive_data[0x80],map_data[0x20];
static uintptr_t decode_scratch;
static int decode_calls,property_name_valid=1;
static uintptr_t actors[1],fake_base=0x10000000;
static int marker=1,ashley=1,forward_calls,mode,records;
static char logged[2400];
static int read_at(uintptr_t at,void *out,size_t n) {
    SetLastError(999);
    if(at==fake_base+0x18b6638&&n==8){uintptr_t w=(uintptr_t)world;memcpy(out,&w,8);return 1;}
    unsigned char *buffers[]={pawn,channel,world,level,info,bunch,(unsigned char*)actors,property,archive_data,map_data,(unsigned char*)&decode_scratch};
    size_t sizes[]={sizeof pawn,sizeof channel,sizeof world,sizeof level,sizeof info,sizeof bunch,sizeof actors,sizeof property,sizeof archive_data,sizeof map_data,sizeof decode_scratch};
    for(size_t i=0;i<sizeof sizes/sizeof sizes[0];++i)
        if(at>=(uintptr_t)buffers[i]&&n<=sizes[i]&&at-(uintptr_t)buffers[i]<=sizes[i]-n){memcpy(out,(void*)at,n);return 1;}
    return 0;
}
static uintptr_t read_ptr(uintptr_t at){uintptr_t v=0;read_at(at,&v,8);return v;}
static int is_live(uintptr_t base,uintptr_t p){(void)base;return p==(uintptr_t)pawn||p==(uintptr_t)channel||p==(uintptr_t)world||p==(uintptr_t)level||p==(uintptr_t)info||p==(uintptr_t)property;}
static int is_kind_of(uintptr_t base,uintptr_t p,const char *name){(void)base;return ashley&&p==(uintptr_t)pawn&&!strcmp(name,"SFXPawn_Ashley");}
static int has_marker(const wchar_t *p){(void)p;return marker;}
static void line(const char *s){++records;snprintf(logged,sizeof logged,"%s",s);SetLastError(998);}
static void ammo_wire_receive_begin(AmmoWireReceive *s,uintptr_t c){(void)s;(void)c;SetLastError(997);}
static void ammo_wire_receive_end(AmmoWireReceive *s){(void)s;SetLastError(996);}
static int ammo_rpc_opted(void){return 0;}
static int fire_rpc_opted(void){return 0;}
static int supported_image(uintptr_t base){(void)base;return 0;}
static void identity_path(uintptr_t base,uintptr_t object,char *out,size_t size){(void)base;(void)object;snprintf(out,size,"%s",property_name_valid?"Engine.Pawn.InvManager":"Engine.Pawn.Other");}
#include "initial_actor_trace.h"
static void putptr(unsigned char *b,size_t off,uintptr_t v){memcpy(b+off,&v,8);}
static int decode_original(void *prop,void *ar,void *map,void *data) {
    CHECK(prop==property&&ar==archive_data&&map==map_data&&data==&decode_scratch&&GetLastError()==321);
    ++decode_calls;decode_scratch=0;SetLastError(654);return 7;
}
static void original(void *self,void *incoming) {
    CHECK(self==channel&&incoming==bunch&&GetLastError()==123);
    ++forward_calls;
    if(mode==1){float v[3]={78,-128,-72};memcpy(pawn+0x108,v,12);}
    if(mode==2)pawn[0x269]=0;
    if(mode==3){int i=999;memcpy(pawn+0x38,&i,4);}
    if(mode==4){float v=NAN;memcpy(pawn+0x108,&v,4);}
    if(mode==5){float v[3]={-57118,-105857,71};memcpy(pawn+0x138,v,12);}
    if(mode==6)pawn[0x268]=10;
    if(mode==7){putptr(channel,0xa0,0);uint32_t flags=0x4000000;memcpy(pawn+0x25c,&flags,4);pawn[0x268]=10;}
    if(mode==8)putptr(channel,0xa0,(uintptr_t)world);
    if(mode==9)putptr(pawn,0x414,0);
    if(mode==10)putptr(pawn,0x41c,0x56789);
    if(mode>=11&&mode<=16) {
        InventoryDecodeFrame inner;
        if(mode==15){PositionReceiveSnapshot invalid={0};inventory_decode_begin(&inner,(uintptr_t)channel,invalid);}
        SetLastError(321);int result=inventory_decode_forward(property,archive_data,map_data,&decode_scratch,
            fake_base+(mode==13?0x488ed5:0x488ed4));
        CHECK(result==7&&GetLastError()==654&&decode_calls==1);
        if(mode==15){inventory_decode_end(&inner);CHECK(inventory_decode_frame!=NULL&&inventory_decode_frame->before.valid);}
        if(mode==11)putptr(pawn,0x414,decode_scratch);
    }
    SetLastError(456);
}
static void setup(void) {
    memset(pawn,0,sizeof pawn);memset(channel,0,sizeof channel);memset(world,0,sizeof world);
    memset(level,0,sizeof level);memset(info,0,sizeof info);memset(bunch,0xa5,sizeof bunch);
    putptr(channel,0,fake_base+0x11a2150);putptr(channel,0xa0,(uintptr_t)pawn);
    putptr(pawn,0x414,0x123456);putptr(pawn,0x41c,0x234567);
    putptr(pawn,0x50,0x12345);putptr(pawn,0x198,(uintptr_t)info);
    putptr(world,0x88,(uintptr_t)level);actors[0]=(uintptr_t)info;
    GameArray a={(uintptr_t)actors,1,1};memcpy(level+0x60,&a,sizeof a);info[0x7c8]=3;
    int ix=42;memcpy(pawn+0x38,&ix,4);float pos[3]={-22319,49125,230};memcpy(pawn+0x108,pos,12);
    pawn[0x269]=3;pawn[0x26a]=2;
    initial_actor_trace_base=fake_base;initial_actor_trace_thread=GetCurrentThreadId();
    initial_actor_original_bunch=original;position_receive_events=0;
    memset(property,0,sizeof property);memset(archive_data,0,sizeof archive_data);memset(map_data,0,sizeof map_data);
    putptr(property,0,fake_base+0x1077d88);int dim=1,size=8,offset=0x414;uint64_t flags=UINT64_C(0x100000020);
    memcpy(property+0x70,&dim,4);memcpy(property+0x74,&size,4);memcpy(property+0x78,&flags,8);memcpy(property+0x94,&offset,4);
    putptr(map_data,0,fake_base+0x11a1030);decode_scratch=0x123456;decode_calls=0;property_name_valid=1;
    inventory_decode_original=decode_original;inventory_decode_installed=0;inventory_decode_rows=0;inventory_decode_frame=NULL;
    inventory_decode_scoped_calls=inventory_decode_site_calls=inventory_decode_property_calls=0;
    inventory_decode_summaries=8;inventory_decode_summary_at=0;
    progress_enabled=1;marker=ashley=1;test_action=5;mode=records=forward_calls=0;logged[0]=0;
}
static void run(int expected_records) {
    unsigned char old_bunch[sizeof bunch],old_channel[sizeof channel];
    memcpy(old_bunch,bunch,sizeof bunch);memcpy(old_channel,channel,sizeof channel);
    SetLastError(123);initial_actor_received_bunch(channel,bunch);
    DWORD returned_error=GetLastError();
    if(forward_calls!=1||returned_error!=456||records!=expected_records)
        fprintf(stderr,"mode=%d calls=%d error=%lu records=%d expected=%d\n",mode,forward_calls,returned_error,records,expected_records);
    CHECK(forward_calls==1&&returned_error==456&&records==expected_records);
    if(mode==7)putptr(old_channel,0xa0,0);
    if(mode==8)putptr(old_channel,0xa0,(uintptr_t)world);
    CHECK(!memcmp(old_bunch,bunch,sizeof bunch)&&!memcmp(old_channel,channel,sizeof channel));
}
int main(int argc,char **argv) {
    /* Separate processes exercise the cached opt-in parser. */
    CHECK(argc==2);if(!strcmp(argv[1],"canary"))CHECK(0);
    SetEnvironmentVariableW(L"ME3_SQUAD_COOP_POSITION_RECEIVE_TRACE",argv[1][0]=='1'?L"1":L"0");
    setup();mode=1;run(argv[1][0]=='1');
    if(argv[1][0]!='1'){puts("PASS default-off: forward once, LastError and packet preserved");return 0;}
    CHECK(strstr(logged,"change-in-original-receive")&&strstr(logged,"78.000,-128.000,-72.000"));
    setup();run(0);
    setup();mode=11;inventory_decode_installed=1;run(3);
    setup();mode=12;inventory_decode_installed=1;run(1);CHECK(strstr(logged,"\"native_result\":7"));
    setup();mode=15;inventory_decode_installed=1;run(0);
    setup();mode=12;inventory_decode_installed=1;int invalid_offset=0x41c;memcpy(property+0x94,&invalid_offset,4);run(0);
    setup();mode=13;inventory_decode_installed=1;run(0);CHECK(inventory_decode_scoped_calls==1&&inventory_decode_site_calls==0&&inventory_decode_property_calls==1);
    setup();mode=14;inventory_decode_installed=1;property_name_valid=0;run(0);CHECK(inventory_decode_scoped_calls==1&&inventory_decode_site_calls==1&&inventory_decode_property_calls==0);
    setup();mode=12;inventory_decode_installed=1;inventory_decode_rows=64;run(0);
    setup();mode=12;run(0);
    setup();mode=12;inventory_decode_installed=1;inventory_decode_summaries=0;run(2);CHECK(strstr(logged,"\"scoped_serializer_calls\":1"));
    setup();mode=9;run(1);CHECK(strstr(logged,"\"inventory_before\":\"0x123456\",\"inventory_after\":\"0x0\""));
    setup();mode=10;run(1);CHECK(strstr(logged,"\"weapon_after\":\"0x56789\""));
    setup();mode=5;run(1);CHECK(strstr(logged,"-57118.000,-105857.000,71.000"));
    setup();mode=6;run(1);CHECK(strstr(logged,"\"physics_after\":10"));
    setup();mode=7;run(1);CHECK(strstr(logged,"\"channel_actor_cleared\":true")&&strstr(logged,"\"actor_flags_after\":67108864"));
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
    puts("PASS 27 receive-wrapper cases: changed/unchanged, identity/nonfinite/budget/context guards, forward once, LastError and packet preserved");
    return 0;
}
