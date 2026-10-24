/* Authored memory only. Actual observer header, no game or process access. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"CHECK failed line%d: %s\n",__LINE__,#x);exit(2);}}while(0)
typedef struct {uintptr_t data;int count,max;} GameArray;
typedef struct {int valid;uintptr_t pawn;} PositionReceiveSnapshot;
typedef struct InventoryDecodeFrame {struct InventoryDecodeFrame *previous;PositionReceiveSnapshot before;uintptr_t channel;unsigned id;} InventoryDecodeFrame;
static uintptr_t initial_actor_trace_base=0x10000000,slot_value;
static DWORD initial_actor_trace_thread=7,current_thread=7;
static DWORD fixture_thread(void){return current_thread;}
#define GetCurrentThreadId fixture_thread
static int progress_enabled=1,test_action=5,marker=1,opted=1,live=1,image=1,pin_valid=1,protect=1;
static int calls,records,entry_ok,change_frame;static char row[1500],test_role[8]="client",field_path[400]="Engine.Actor.Location";
static wchar_t root[]=L"fixture";static unsigned char property[0xe0],archive[0x40],channel[0xb0],pawn[0x800],map[0x120],world[0x100],level[0x100],info[0x200];
static float *data=(float*)(pawn+0x15c);static uintptr_t actors[1];
static InventoryDecodeFrame frame,*inventory_decode_frame=&frame;
static void (*vector_decode_index_observer)(void*,uint32_t*,uint32_t,uintptr_t);
static uint32_t scalar_value;
static void put(void *p,size_t off,uintptr_t v){memcpy((char*)p+off,&v,8);}
static int read_at(uintptr_t at,void *out,size_t n){
 SetLastError(999);
 if(at==initial_actor_trace_base+0x146fb0&&n==20){const unsigned char pin[20]={0x48,0x8b,0xc4,0x4c,0x89,0x40,0x18,0x48,0x89,0x48,0x08,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56};memcpy(out,pin,20);if(!pin_valid)((char*)out)[0]=0;return 1;}
 void *buf[]={property,archive,channel,pawn,&scalar_value,map,world,level,info,actors};size_t size[]={sizeof property,sizeof archive,sizeof channel,sizeof pawn,sizeof scalar_value,sizeof map,sizeof world,sizeof level,sizeof info,sizeof actors};
 for(unsigned i=0;i<10;i++)if(at>=(uintptr_t)buf[i]&&n<=size[i]&&at-(uintptr_t)buf[i]<=size[i]-n){memcpy(out,(void*)at,n);return 1;}
 return 0;
}
static uintptr_t read_ptr(uintptr_t at){if(at==initial_actor_trace_base+0x10784a8+0x2f8)return slot_value;if(at==initial_actor_trace_base+0x18b6638)return (uintptr_t)world;uintptr_t v=0;read_at(at,&v,8);return v;}
static int is_live(uintptr_t b,uintptr_t p){(void)b;return live&&(p==(uintptr_t)property||p==(uintptr_t)pawn||p==(uintptr_t)world||p==(uintptr_t)level||p==(uintptr_t)info);}
static int is_kind_of(uintptr_t b,uintptr_t p,const char *k){(void)b;return p==(uintptr_t)pawn&&(!strcmp(k,"SFXPawn_Ashley")||!strcmp(k,"Pawn"));}
static void identity_path(uintptr_t b,uintptr_t p,char *s,size_t n){(void)b;(void)p;snprintf(s,n,"%s",field_path);}
static int has_marker(const wchar_t *p){(void)p;return marker;}
static int position_receive_opted(void){return opted;}
static int supported_image(uintptr_t b){(void)b;return image;}
static void line(const char *s){records++;snprintf(row,sizeof row,"%s",s);SetLastError(998);}
static BOOL fixture_protect(void *at,SIZE_T n,DWORD flags,DWORD *old){(void)at;(void)n;(void)flags;*old=PAGE_READONLY;return protect;}
#define VirtualProtect fixture_protect
static PVOID fixture_cas(PVOID volatile *at,PVOID value,PVOID expected){(void)at;uintptr_t prev=slot_value;if(prev==(uintptr_t)expected)slot_value=(uintptr_t)value;return (PVOID)prev;}
#define InterlockedCompareExchangePointer fixture_cas
#include "vector_decode_trace.h"
static int original(void *p,void *a,void *m,void *d){
 calls++;entry_ok=GetLastError()==123;
 CHECK(p==property&&a==archive&&m==map&&d==data);
 const uintptr_t sites[4]={0x10cc41,0x10cc7d,0x10cc91,0x10cca2};const uint32_t values[4]={13,17,103,0};
 scalar_value=13;
 vector_decode_index_observer(a,&scalar_value,20,initial_actor_trace_base+0x10cc40);
 vector_decode_index_observer((char*)a+4,&scalar_value,20,initial_actor_trace_base+sites[0]);
 vector_decode_index_observer(a,&scalar_value,19,initial_actor_trace_base+sites[0]);
 for(unsigned i=0;i<4;i++){
  scalar_value=values[i];
  if(i)vector_decode_index_observer(a,&scalar_value,32767,initial_actor_trace_base+sites[i]);
  vector_decode_index_observer(a,&scalar_value,i?32768:20,initial_actor_trace_base+sites[i]);
 }
 data[0]=-16367;data[1]=-16281;data[2]=-16384;
 if(change_frame)inventory_decode_frame=NULL;
 SetLastError(456);return 27;
}
static void invoke(uintptr_t caller){int old=calls;SetLastError(123);CHECK(vector_decode_forward(property,archive,map,data,caller)==27);CHECK(calls==old+1&&entry_ok&&GetLastError()==456&&vector_decode_scope==NULL);}
int main(int argc,char **argv){
 if(argc>1&&!strcmp(argv[1],"canary"))CHECK(0);
 uintptr_t b=initial_actor_trace_base;slot_value=b+0x146fb0;
 pin_valid=0;vector_decode_install(b);CHECK(!vector_decode_installed);pin_valid=1;
 image=0;vector_decode_install(b);CHECK(!vector_decode_installed);image=1;
 slot_value++;vector_decode_install(b);CHECK(!vector_decode_installed);slot_value=b+0x146fb0;
 protect=0;vector_decode_install(b);CHECK(!vector_decode_installed);protect=1;
 vector_decode_install(b);CHECK(vector_decode_installed&&slot_value==(uintptr_t)vector_decode_serialize&&records==1);records=0;
 vector_decode_original=original;put(property,0,b+0x10784a8);int dim=1,size=12,offset=0x108;
 memcpy(property+0x70,&dim,4);memcpy(property+0x74,&size,4);memcpy(property+0x94,&offset,4);
 frame.before.valid=1;frame.before.pawn=0x789;frame.channel=(uintptr_t)channel;frame.id=19;put(channel,0xa0,frame.before.pawn);
 data[0]=-22031;data[1]=49555;data[2]=225;invoke(b+0x488ed4);
 CHECK(records==1&&strstr(row,"Engine.Actor.Location")&&strstr(row,"-16367.000")&&strstr(row,"\"receive_id\":19"));
 CHECK(strstr(row,"\"scalar_count\":4")&&strstr(row,"\"scalar_values\":[13,17,103,0]"));
 int count=records;invoke(b+0x488ed5);CHECK(records==count);
 marker=0;invoke(b+0x488ed4);CHECK(records==count);marker=1;
 opted=0;invoke(b+0x488ed4);CHECK(records==count);opted=1;
 current_thread=8;invoke(b+0x488ed4);CHECK(records==count);current_thread=7;
 test_action=6;invoke(b+0x488ed4);CHECK(records==count);test_action=5;
 dim=2;memcpy(property+0x70,&dim,4);invoke(b+0x488ed4);CHECK(records==count);dim=1;memcpy(property+0x70,&dim,4);
 offset=0x138;memcpy(property+0x94,&offset,4);invoke(b+0x488ed4);CHECK(records==count);
 strcpy(field_path,"Engine.Actor.Velocity");invoke(b+0x488ed4);CHECK(records==++count);
 offset=0x15c;memcpy(property+0x94,&offset,4);strcpy(field_path,"Engine.Actor.RelativeLocation");invoke(b+0x488ed4);CHECK(records==++count);
 change_frame=1;invoke(b+0x488ed4);CHECK(records==count);change_frame=0;inventory_decode_frame=&frame;
 vector_decode_rows=128;invoke(b+0x488ed4);CHECK(records==count);
 vector_decode_rows=0;test_action=6;strcpy(test_role,"host");inventory_decode_frame=NULL;
 put(archive,0,b+0x119dcc0);put(map,0,b+0x11a0d20);put(map,0x108,0xabcdef);
 put(world,0x88,(uintptr_t)level);actors[0]=(uintptr_t)info;
 GameArray ar={(uintptr_t)actors,1,1};memcpy(level+0x60,&ar,sizeof ar);put(pawn,0x198,(uintptr_t)info);
 invoke(b+0x48d433);CHECK(records==++count&&strstr(row,"vector_send_trace")&&strstr(row,"\"scalar_count\":4"));
 uint32_t loading=1;memcpy(archive+0x14,&loading,4);invoke(b+0x48d433);CHECK(records==count);
 loading=0;memcpy(archive+0x14,&loading,4);
 put(map,0x108,0);invoke(b+0x48d433);CHECK(records==count);put(map,0x108,0xabcdef);
 invoke(b+0x48d434);CHECK(records==count);
 put(pawn,0x198,0);invoke(b+0x48d433);CHECK(records==count);put(pawn,0x198,(uintptr_t)info);
 vector_decode_rows=128;invoke(b+0x48d433);CHECK(records==count);
 puts("PASS actual vector observer: forwarding, scope, property identity, frame change, pins, install and budget");return 0;
}
