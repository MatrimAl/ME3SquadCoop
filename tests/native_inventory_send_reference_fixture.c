/* Authored sender metadata only; no game or process access. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"CHECK failed line%d: %s\n",__LINE__,#x);exit(2);}}while(0)
typedef struct {uintptr_t data;int count,max;} GameArray;
static uintptr_t initial_actor_trace_base=0x10000000;
static DWORD initial_actor_trace_thread=7,current_thread=7;
static DWORD fixture_thread(void){return current_thread;}
#define GetCurrentThreadId fixture_thread
static int inventory_decode_installed=1,progress_enabled=1,test_action=6,marker=1,property_valid=1,pawn_valid=1,records,scan_read_fail;
static char test_role[8]="host";static wchar_t root[]=L"fixture";
static unsigned char property[0xe0],pawn[0x800],world[0x100],level[0x100],info[0x200],manager[0x200];
static unsigned char archive[0x80],map[0x120],connection[0x4000],channel[0x100],other_channel[0x100];
static uintptr_t actors[1],writer[4];static uint8_t scalar_tag;static uint32_t scalar_index;static int bitcalls,intcalls,cas_calls,cas_fail,image_valid=1,pin_bad;static char row[2000];
static void put(void *p,size_t off,uintptr_t value){memcpy((char*)p+off,&value,8);}
static int read_at(uintptr_t at,void *out,size_t n){
 SetLastError(999);
 if((at==initial_actor_trace_base+0xc2bd0||at==initial_actor_trace_base+0xc3110)&&n==16){
  static const unsigned char bp[16]={0x48,0x89,0x5c,0x24,8,0x57,0x48,0x83,0xec,0x30,0x41,0x8b,0xf8,0x48,0x8b,0xd9};
  static const unsigned char ip[16]={0x40,0x57,0x45,0x8b,0xd0,0x48,0x8b,0xfa,0x4c,0x8b,0xd9,0x44,0x39,2,0x76,3};
  memcpy(out,at==initial_actor_trace_base+0xc2bd0?bp:ip,16);if(pin_bad)((unsigned char*)out)[0]^=1;return 1;}
 if(scan_read_fail&&at==(uintptr_t)connection+0xf84+10*8)return 0;
 if(at>=initial_actor_trace_base+0x119dcc0&&at<initial_actor_trace_base+0x119dce0&&n==8){memcpy(out,(char*)writer+at-initial_actor_trace_base-0x119dcc0,8);return 1;}
 if(at==initial_actor_trace_base+0x18b6638&&n==8){uintptr_t value=(uintptr_t)world;memcpy(out,&value,8);return 1;}
 void *buf[]={property,pawn,world,level,info,manager,archive,map,connection,channel,other_channel,actors,writer,&scalar_tag,&scalar_index};
 size_t sizes[]={sizeof property,sizeof pawn,sizeof world,sizeof level,sizeof info,sizeof manager,sizeof archive,sizeof map,sizeof connection,sizeof channel,sizeof other_channel,sizeof actors,sizeof writer,1,4};
 for(size_t i=0;i<sizeof sizes/sizeof *sizes;i++)if(at>=(uintptr_t)buf[i]&&n<=sizes[i]&&at-(uintptr_t)buf[i]<=sizes[i]-n){memcpy(out,(void*)at,n);return 1;}
 return 0;
}
static uintptr_t read_ptr(uintptr_t at){uintptr_t v=0;read_at(at,&v,8);return v;}
static int is_live(uintptr_t base,uintptr_t p){(void)base;return p==(uintptr_t)pawn||p==(uintptr_t)world||p==(uintptr_t)level||p==(uintptr_t)info||p==(uintptr_t)manager;}
static int is_kind_of(uintptr_t base,uintptr_t p,const char *kind){(void)base;return pawn_valid&&p==(uintptr_t)pawn&&(!strcmp(kind,"Pawn")||!strcmp(kind,"SFXPawn_Ashley"));}
static int has_marker(const wchar_t *p){(void)p;return marker;}
static int inventory_decode_property(uintptr_t p){return property_valid&&p==(uintptr_t)property;}
static void line(const char *s){++records;snprintf(row,sizeof row,"%s",s);SetLastError(998);}

static int supported_image(uintptr_t b){return image_valid&&b==initial_actor_trace_base;}
static BOOL fake_protect(void *p,SIZE_T n,DWORD prot,DWORD *old){(void)p;(void)n;(void)prot;*old=PAGE_READONLY;return TRUE;}
static PVOID fake_exchange(PVOID volatile *slot,PVOID replacement,PVOID expected){
 uintptr_t off=(uintptr_t)slot-initial_actor_trace_base-0x119dcc0;CHECK(off==0x10||off==0x18);uintptr_t *p=&writer[off/8];PVOID old=(PVOID)*p;
 ++cas_calls;if(cas_calls==cas_fail)return (PVOID)0xdead;if(old==expected)*p=(uintptr_t)replacement;return old;}
#define VirtualProtect fake_protect
#undef InterlockedCompareExchangePointer
#define InterlockedCompareExchangePointer fake_exchange
#include "inventory_send_reference_trace.h"
static void original_bits(void *self,void *source,int32_t count){CHECK(self==archive&&source==&scalar_tag&&count>0&&GetLastError()==123);++bitcalls;SetLastError(501);}
static void original_index(void *self,uint32_t *source,uint32_t max){CHECK(self==archive&&source==&scalar_index&&GetLastError()==123);++intcalls;if(*source>max)*source=max;SetLastError(502);}
static void tagcall(uintptr_t site,int count){SetLastError(123);int n=bitcalls;inventory_send_bits_forward(archive,&scalar_tag,count,site);CHECK(bitcalls==n+1&&GetLastError()==501);}
static void indexcall(uintptr_t site,uint32_t max){SetLastError(123);int n=intcalls;inventory_send_index_forward(archive,&scalar_index,max,site);CHECK(intcalls==n+1&&GetLastError()==502);}
int main(int argc,char **argv){
 if(argc>1&&!strcmp(argv[1],"canary"))CHECK(0);
 uintptr_t base=initial_actor_trace_base;actors[0]=(uintptr_t)info;
 put(world,0x88,(uintptr_t)level);GameArray a={(uintptr_t)actors,1,1};memcpy(level+0x60,&a,sizeof a);
 put(pawn,0x198,(uintptr_t)info);put(pawn,0x414,(uintptr_t)manager);put(manager,0x50,0x1234);
 put(map,0,base+0x11a0d20);put(map,0x108,(uintptr_t)connection);put(archive,0,base+0x119dcc0);
 put(connection,0xf84+9*8,(uintptr_t)channel);put(channel,0xa0,(uintptr_t)manager);uint32_t type=2;memcpy(channel+0x78,&type,4);
 writer[2]=base+0xc2bd0;writer[3]=base+0xc3110;
 image_valid=0;inventory_send_reference_install(base);CHECK(!inventory_send_reference_installed&&cas_calls==0);image_valid=1;
 pin_bad=1;inventory_send_reference_install(base);CHECK(!inventory_send_reference_installed&&cas_calls==0);pin_bad=0;
 cas_fail=1;inventory_send_reference_install(base);CHECK(!inventory_send_reference_installed&&cas_calls==1&&writer[2]==base+0xc2bd0&&writer[3]==base+0xc3110);
 cas_calls=0;cas_fail=2;inventory_send_reference_install(base);CHECK(!inventory_send_reference_installed&&cas_calls==3&&writer[2]==base+0xc2bd0&&writer[3]==base+0xc3110);
 cas_calls=cas_fail=0;inventory_send_reference_install(base);CHECK(inventory_send_reference_installed&&cas_calls==2&&writer[2]==(uintptr_t)inventory_send_bits&&writer[3]==(uintptr_t)inventory_send_index);
 CHECK(records==1&&strstr(row,"\"inventory_send_reference\":\"installed\""));records=0;
 inventory_send_bits_original=original_bits;inventory_send_index_original=original_index;
 InventorySendScope outer,nested;SetLastError(123);inventory_send_reference_begin(&outer,property,archive,map,pawn+0x414,base+0x48d433);CHECK(GetLastError()==123&&outer.active&&outer.channel_index==9);
 scalar_tag=1;scalar_index=26;tagcall(base+0x602ed2,1);indexcall(base+0x602f12,0x3ff);
 CHECK(records==2&&strstr(row,"\"scalar_after\":26")&&strstr(row,"\"branch\":1")&&strstr(row,"\"property_id\":1"));
 int before=records;indexcall(base+0x602f12,0x3ff);tagcall(base+0x602ed3,1);tagcall(base+0x602ed2,8);CHECK(records==before);
 scalar_tag=0;scalar_index=13579432;tagcall(base+0x602f57,1);indexcall(base+0x602f89,0x80000000);CHECK(records==before+2&&strstr(row,"\"branch\":3")&&strstr(row,"13579432"));
 scalar_tag=1;tagcall(base+0x602e8d,1);scalar_index=0;indexcall(base+0x602ea0,0x3ff);CHECK(strstr(row,"\"branch\":2")&&strstr(row,"\"scalar_after\":0"));
 tagcall(base+0x602f76,1);scalar_index=0;indexcall(base+0x602f89,0x3ff);CHECK(strstr(row,"\"branch\":4"));
 tagcall(base+0x602ed2,1);before=records;indexcall(base+0x602f12,0x400);indexcall(base+0x602f89,0x3ff);CHECK(records==before);
 scalar_index=2000;indexcall(base+0x602f12,0x3ff);CHECK(scalar_index==0x3ff&&strstr(row,"\"scalar_before\":2000")&&strstr(row,"\"scalar_after\":1023"));
 before=records;current_thread=8;tagcall(base+0x602ed2,1);inventory_send_reference_begin(&nested,property,archive,map,pawn+0x414,base+0x48d433);CHECK(inventory_send_scope==&outer);inventory_send_reference_end(&nested);current_thread=7;CHECK(records==before);
 inventory_send_reference_begin(&nested,property,archive,map,pawn+0x414,base+0x48d434);tagcall(base+0x602ed2,1);CHECK(!nested.active&&records==before);inventory_send_reference_end(&nested);CHECK(inventory_send_scope==&outer);
 property_valid=0;inventory_send_reference_begin(&nested,property,archive,map,pawn+0x414,base+0x48d433);tagcall(base+0x602ed2,1);inventory_send_reference_end(&nested);property_valid=1;CHECK(records==before);
 put(archive,0,base+0x119dda8);tagcall(base+0x602ed2,1);put(archive,0,base+0x119dcc0);CHECK(records==before);
 put(map,0x108,0);tagcall(base+0x602ed2,1);put(map,0x108,(uintptr_t)connection);CHECK(records==before);
 put(pawn,0x414,0);tagcall(base+0x602ed2,1);put(pawn,0x414,(uintptr_t)manager);CHECK(records==before);
 inventory_send_reference_rows=128;tagcall(base+0x602ed2,1);indexcall(base+0x602f12,0x3ff);CHECK(records==before&&inventory_send_reference_rows==128);
 inventory_send_reference_end(&outer);CHECK(!inventory_send_scope);tagcall(base+0x602ed2,1);CHECK(records==before);
 puts("PASS writer scalar observer: branches, normalization, scopes, forwarding, LastError, metadata and budget");return 0;
}
