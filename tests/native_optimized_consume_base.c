/* Authored-memory checks only: no process access or game code execution. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"CHECK failed line%d: %s\n",__LINE__,#x);exit(2);}}while(0)
typedef struct {uintptr_t pawn,klass,world;int32_t index;int valid;} PositionReceiveSnapshot;
typedef struct InventoryDecodeFrame {struct InventoryDecodeFrame *previous;PositionReceiveSnapshot before;uintptr_t channel;unsigned id;} InventoryDecodeFrame;
static uintptr_t initial_actor_trace_base=0x10000000,slot_value;
static DWORD initial_actor_trace_thread=7,current_thread=7;
static DWORD fixture_thread(void){return current_thread;}
#define GetCurrentThreadId fixture_thread
static int progress_enabled=1,test_action=5,marker=1,opted=1,live=1,image=1,pin_valid=1,protect=1;
static int initial_actor_trace_installed=1,calls,records,mode,entry_ok;
static char row[2000],test_role[8]="client";static wchar_t root[]=L"fixture";
static unsigned char pawn[0x500],archive[0x100],channel[0x100],shadow[0x144],map[0x120];
static InventoryDecodeFrame frame,*inventory_decode_frame=&frame;
static void put(void *p,size_t off,uintptr_t v){memcpy((char*)p+off,&v,8);}
static void put32(void *p,size_t off,uint32_t v){memcpy((char*)p+off,&v,4);}
static int read_at(uintptr_t at,void *out,size_t n){
 SetLastError(999);
 if(at==initial_actor_trace_base+0xa89c20&&n==20){const unsigned char pin[20]={0x40,0x55,0x57,0x41,0x56,0x48,0x8b,0xec,0x48,0x83,0xec,0x50,0x48,0xc7,0x45,0xe8,0xfe,0xff,0xff,0xff};memcpy(out,pin,20);if(!pin_valid)((char*)out)[0]=0;return 1;}
 void *buf[]={pawn,archive,channel,shadow,map};size_t size[]={sizeof pawn,sizeof archive,sizeof channel,sizeof shadow,sizeof map};
 for(unsigned i=0;i<5;i++)if(at>=(uintptr_t)buf[i]&&n<=size[i]&&at-(uintptr_t)buf[i]<=size[i]-n){memcpy(out,(void*)at,n);return 1;}
 return 0;
}
static uintptr_t read_ptr(uintptr_t at){if(at==initial_actor_trace_base+0x133c3f0+0x7c8)return slot_value;uintptr_t v=0;read_at(at,&v,8);return v;}
static int is_live(uintptr_t b,uintptr_t p){(void)b;return live&&(p==(uintptr_t)pawn||p==(uintptr_t)channel);}
static int is_kind_of(uintptr_t b,uintptr_t p,const char *k){(void)b;return p==(uintptr_t)pawn&&!strcmp(k,"SFXPawn_Ashley");}
static int has_marker(const wchar_t *p){(void)p;return marker;}
static int position_receive_opted(void){return opted;}
static int supported_image(uintptr_t b){(void)b;return image;}
static void line(const char *s){records++;snprintf(row,sizeof row,"%s",s);SetLastError(998);}
static BOOL fixture_protect(void *at,SIZE_T n,DWORD flags,DWORD *old){(void)at;(void)n;(void)flags;*old=PAGE_READONLY;return protect;}
#define VirtualProtect fixture_protect
static PVOID fixture_cas(PVOID volatile *at,PVOID value,PVOID expected){(void)at;uintptr_t prev=slot_value;if(prev==(uintptr_t)expected)slot_value=(uintptr_t)value;return (PVOID)prev;}
#undef InterlockedCompareExchangePointer
#define InterlockedCompareExchangePointer fixture_cas
#include "optimized_owner_trace_base.h"
static void original(void *p,void *a,void *s,void *m,void *c){
 calls++;entry_ok=GetLastError()==123;
 CHECK(p==pawn&&a==archive&&s==shadow&&m==map&&c==channel);
 if(mode==1){put32(archive,0xa0,120);float xyz[3]={11,20,30};memcpy(pawn+0x108,xyz,12);memcpy(shadow+0x108,xyz,12);}
 if(mode==2)put32(archive,0x38,1);
 if(mode==3){float xyz[3]={1010,20,30};memcpy(pawn+0x108,xyz,12);}
 if(mode==4)inventory_decode_frame=NULL;
 SetLastError(456);
}
static void reset(void){
 uintptr_t b=initial_actor_trace_base;memset(pawn,0,sizeof pawn);memset(archive,0,sizeof archive);memset(channel,0,sizeof channel);memset(shadow,0,sizeof shadow);memset(map,0,sizeof map);
 put(pawn,0,b+0x133c3f0);put(pawn,0x50,0x45678);put(pawn,0x198,0x56789);put32(pawn,0x38,19);
 put(channel,0,b+0x11a2150);put(channel,0xa0,(uintptr_t)pawn);put(channel,0xc4,(uintptr_t)shadow);put32(channel,0xcc,sizeof shadow);put(channel,0x60,0xabcde);
 put(archive,0,b+0x119dda8);put32(archive,0x14,1);put32(archive,0xa0,100);put32(archive,0x9c,200);
 put(map,0,b+0x11a0d20);put(map,0x108,0xabcde);
 frame.before=(PositionReceiveSnapshot){(uintptr_t)pawn,0x45678,0x56789,19,1};frame.channel=(uintptr_t)channel;frame.id=31;inventory_decode_frame=&frame;
 float xyz[3]={10,20,30};memcpy(pawn+0x108,xyz,12);memcpy(shadow+0x108,xyz,12);mode=0;
}
static void invoke(uintptr_t caller){int old=calls;SetLastError(123);optimized_owner_forward(pawn,archive,shadow,map,channel,caller);CHECK(calls==old+1&&entry_ok&&GetLastError()==456);}
int main(int argc,char **argv){
 if(argc>1&&!strcmp(argv[1],"canary"))CHECK(0);
 uintptr_t b=initial_actor_trace_base;slot_value=b+0xa89c20;reset();
 pin_valid=0;optimized_owner_install(b);CHECK(!optimized_owner_installed);pin_valid=1;
 image=0;optimized_owner_install(b);CHECK(!optimized_owner_installed);image=1;
 slot_value++;optimized_owner_install(b);CHECK(!optimized_owner_installed);slot_value=b+0xa89c20;
 protect=0;optimized_owner_install(b);CHECK(!optimized_owner_installed);protect=1;
 SetLastError(123);optimized_owner_install(b);CHECK(optimized_owner_installed&&slot_value==(uintptr_t)optimized_owner_serialize&&records==1&&GetLastError()==123);records=0;optimized_owner_original=original;
 for(int i=0;i<7;i++){reset();mode=1;invoke(b+0x489adf);}CHECK(records==4&&optimized_owner_normal_rows==4&&strstr(row,"\"cursor_after\":120"));
 reset();put32(pawn,0x264,0x4000);invoke(b+0x489adf);CHECK(records==5&&strstr(row,"\"entry_net_owner\":true")&&strstr(row,"\"cursor_before\":100,\"cursor_after\":100"));
 reset();mode=2;invoke(b+0x489adf);CHECK(records==6&&strstr(row,"\"archive_error_after\":1"));
 reset();mode=3;invoke(b+0x489adf);CHECK(records==7&&strstr(row,"\"large_position_jump\":true"));
 reset();mode=4;invoke(b+0x489adf);CHECK(records==8&&strstr(row,"\"after_valid\":false"));
 int count=records;
 reset();invoke(b+0x489ade);CHECK(records==count);
 reset();current_thread=8;invoke(b+0x489adf);CHECK(records==count);current_thread=7;
 reset();marker=0;invoke(b+0x489adf);CHECK(records==count);marker=1;
 reset();opted=0;invoke(b+0x489adf);CHECK(records==count);opted=1;
 reset();frame.before.valid=0;invoke(b+0x489adf);CHECK(records==count);
 reset();frame.before.index++;invoke(b+0x489adf);CHECK(records==count);
 reset();put(channel,0xc4,(uintptr_t)shadow+8);invoke(b+0x489adf);CHECK(records==count);
 reset();put32(channel,0xcc,0x143);invoke(b+0x489adf);CHECK(records==count);
 reset();put(pawn,0,b+0x133c3f8);invoke(b+0x489adf);CHECK(records==count);
 reset();put(archive,0,b+0x119dcc0);invoke(b+0x489adf);CHECK(records==count);
 reset();put32(archive,0x14,0);invoke(b+0x489adf);CHECK(records==count);
 reset();put(map,0x108,0xabcdf);invoke(b+0x489adf);CHECK(records==count);
 reset();test_action=6;invoke(b+0x489adf);CHECK(records==count);test_action=5;
 reset();put32(pawn,0x264,0x4000);optimized_owner_rows=128;invoke(b+0x489adf);CHECK(records==count);
 puts("PASS optimized owner observer: five unchanged args/one original call, LastError, normal/anomaly budgets, identity/archive/header guards, pins and install");return 0;
}
