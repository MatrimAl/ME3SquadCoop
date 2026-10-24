/* Authored memory only: compiles the actual draft header; never opens a game. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do {if(!(x)){fprintf(stderr,"CHECK failed at line %d: %s\n",__LINE__,#x);exit(2);}}while(0)
typedef struct {int valid;uintptr_t pawn;} PositionReceiveSnapshot;
typedef struct {PositionReceiveSnapshot before;uintptr_t channel;unsigned id;} InventoryDecodeFrame;
static InventoryDecodeFrame *inventory_decode_frame;
static uintptr_t initial_actor_trace_base=0x10000000;
static DWORD initial_actor_trace_thread=7,current_thread=7;
static int inventory_decode_installed=1,progress_enabled=1,test_action=5;
static char test_role[]="client";static wchar_t root[]=L"fixture";
static unsigned char property[0xe0],archive[0xc0],other_archive[0xc0],map[0x120],channel[0x100],pawn[0x80],connection[0x4000];
static uint8_t tag_output,native_tag=1;static uint32_t index_output,native_index=9;
static uintptr_t slots[2];static int records,bits_calls,index_calls,property_valid=1,image_valid=1,marker=1,cas_calls,cas_fail,error_read_fail;
static char last_row[1500];static void *last_self,*last_dest;static uint32_t last_arg;
static DWORD fake_thread(void){return current_thread;}
#define GetCurrentThreadId fake_thread
static const unsigned char bitpin[16]={0x48,0x89,0x5c,0x24,8,0x48,0x89,0x74,0x24,0x10,0x57,0x48,0x83,0xec,0x30,0x41};
static const unsigned char intpin[16]={0x48,0x89,0x5c,0x24,0x10,0x57,0x45,0x33,0xc9,0x41,0xbb,1,0,0,0,0x44};
static void put(unsigned char *p,size_t off,uintptr_t v){memcpy(p+off,&v,8);}
static int read_at(uintptr_t at,void *out,size_t n){
    SetLastError(999);
    if(error_read_fail&&at==(uintptr_t)archive+0x38)return 0;
    if(at==initial_actor_trace_base+0xc2b00&&n==16){memcpy(out,bitpin,n);return 1;}
    if(at==initial_actor_trace_base+0xc3060&&n==16){memcpy(out,intpin,n);return 1;}
    if(at>=initial_actor_trace_base+0x119ddb8&&at<initial_actor_trace_base+0x119ddc8&&n==8){memcpy(out,(char*)slots+(at-initial_actor_trace_base-0x119ddb8),8);return 1;}
    void *buf[]={property,archive,other_archive,map,channel,pawn,connection,&tag_output,&index_output};
    size_t size[]={sizeof property,sizeof archive,sizeof other_archive,sizeof map,sizeof channel,sizeof pawn,sizeof connection,1,4};
    for(size_t i=0;i<sizeof size/sizeof *size;i++)if(at>=(uintptr_t)buf[i]&&n<=size[i]&&at-(uintptr_t)buf[i]<=size[i]-n){memcpy(out,(void*)at,n);return 1;}
    return 0;
}
static uintptr_t read_ptr(uintptr_t at){uintptr_t v=0;read_at(at,&v,8);return v;}
static int inventory_decode_property(uintptr_t p){return property_valid&&p==(uintptr_t)property;}
static int position_receive_opted(void){return 1;}
static int supported_image(uintptr_t b){return image_valid&&b==initial_actor_trace_base;}
static int has_marker(const wchar_t *p){(void)p;return marker;}
static void line(const char *s){++records;snprintf(last_row,sizeof last_row,"%s",s);SetLastError(998);}
static BOOL protect(void *p,SIZE_T n,DWORD permission,DWORD *old){(void)p;(void)n;(void)permission;*old=PAGE_READONLY;return TRUE;}
static PVOID exchange(PVOID volatile *p,PVOID replacement,PVOID expected){
    uintptr_t off=(uintptr_t)p-initial_actor_trace_base-0x119ddb8;CHECK(off==0||off==8);
    uintptr_t *slot=&slots[off/8];PVOID old=(PVOID)*slot;++cas_calls;
    if(cas_calls==cas_fail)return (PVOID)0xdead;
    if(old==expected)*slot=(uintptr_t)replacement;return old;
}
#define VirtualProtect protect
#undef InterlockedCompareExchangePointer
#define InterlockedCompareExchangePointer exchange
#include "inventory_reference_trace.h"
static void bits_original(void *self,void *dest,int32_t count){CHECK(GetLastError()==123);++bits_calls;last_self=self;last_dest=dest;last_arg=(uint32_t)count;*(uint8_t*)dest=native_tag;SetLastError(501);}
static void index_original(void *self,uint32_t *dest,uint32_t maximum){CHECK(GetLastError()==123);++index_calls;last_self=self;last_dest=dest;last_arg=maximum;*dest=native_index;SetLastError(502);}
static void reset_slots(void){slots[0]=initial_actor_trace_base+0xc2b00;slots[1]=initial_actor_trace_base+0xc3060;inventory_reference_installed=0;cas_calls=cas_fail=0;}
static void bitcall(uintptr_t site,int32_t count,void *ar){SetLastError(123);int n=bits_calls;inventory_reference_bits_forward(ar,&tag_output,count,site);CHECK(bits_calls==n+1&&last_self==ar&&last_dest==&tag_output&&last_arg==(uint32_t)count&&tag_output==native_tag&&GetLastError()==501);}
static void intcall(uintptr_t site,uint32_t max,void *ar){SetLastError(123);int n=index_calls;inventory_reference_index_forward(ar,&index_output,max,site);CHECK(index_calls==n+1&&last_self==ar&&last_dest==&index_output&&last_arg==max&&index_output==native_index&&GetLastError()==502);}
int main(int argc,char **argv){
    if(argc>1&&!strcmp(argv[1],"canary"))CHECK(0);
    InventoryDecodeFrame f={{1,(uintptr_t)pawn},(uintptr_t)channel,19},inner_f=f;
    inventory_decode_frame=&f;put(archive,0,initial_actor_trace_base+0x119dda8);put(other_archive,0,initial_actor_trace_base+0x119dda8);
    put(map,0,initial_actor_trace_base+0x11a0d20);put(map,0x108,(uintptr_t)connection);put(channel,0xa0,(uintptr_t)pawn);
    put(connection,0xf84+9*8,(uintptr_t)channel);put(property,0xd0,0x1111);put(pawn,0x50,0x2222);uint32_t type=2,flags=2;memcpy(channel+0x78,&type,4);memcpy(channel+0x68,&flags,4);
    reset_slots();inventory_reference_install(initial_actor_trace_base);CHECK(inventory_reference_installed&&slots[0]==(uintptr_t)inventory_reference_bits&&slots[1]==(uintptr_t)inventory_reference_index);
    reset_slots();cas_fail=2;inventory_reference_install(initial_actor_trace_base);CHECK(!inventory_reference_installed&&slots[0]==initial_actor_trace_base+0xc2b00&&slots[1]==initial_actor_trace_base+0xc3060&&cas_calls==3);
    reset_slots();cas_fail=1;inventory_reference_install(initial_actor_trace_base);CHECK(!inventory_reference_installed&&slots[0]==initial_actor_trace_base+0xc2b00&&slots[1]==initial_actor_trace_base+0xc3060&&cas_calls==1);
    reset_slots();image_valid=0;inventory_reference_install(initial_actor_trace_base);CHECK(!inventory_reference_installed&&cas_calls==0);image_valid=1;
    inventory_reference_installed=1;inventory_reference_bits_original=bits_original;inventory_reference_index_original=index_original;records=0;
    uintptr_t base=initial_actor_trace_base;InventoryReferenceScope outer,nested;SetLastError(123);
    inventory_reference_begin(&outer,&f,property,archive,map,base+0x488ed4,1);CHECK(GetLastError()==123&&inventory_reference_scope==&outer);
    bitcall(base+0x602c40,1,archive);CHECK(records==1&&outer.tag_known&&outer.tag==1);
    intcall(base+0x602c5d,0x3ff,archive);CHECK(records==2&&strstr(last_row,"\"index\":9")&&strstr(last_row,"\"channel_snapshot_valid\":true")&&strstr(last_row,"\"channel_type\":2")&&strstr(last_row,"\"channel_flags\":2"));
    native_index=0;intcall(base+0x602c5d,0x3ff,archive);CHECK(strstr(last_row,"\"index\":0")&&strstr(last_row,"\"channel\":\"0x0\""));
    native_index=10;intcall(base+0x602c5d,0x3ff,archive);CHECK(strstr(last_row,"\"index\":10")&&strstr(last_row,"\"channel_snapshot_valid\":true")&&strstr(last_row,"\"channel\":\"0x0\""));
    native_index=9;error_read_fail=1;intcall(base+0x602c5d,0x3ff,archive);CHECK(strstr(last_row,"\"archive_error_read_valid\":false"));error_read_fail=0;
    int before=records;bitcall(base+0x602c41,1,archive);bitcall(base+0x602c40,8,archive);bitcall(base+0x602c40,1,other_archive);
    intcall(base+0x602c5e,0x3ff,archive);intcall(base+0x602c5d,0x400,archive);intcall(base+0x602c5d,0x3ff,other_archive);CHECK(records==before);
    current_thread=8;bitcall(base+0x602c40,1,archive);inventory_reference_begin(&nested,&f,property,archive,map,base+0x488ed4,1);CHECK(inventory_reference_scope==&outer);inventory_reference_end(&nested);CHECK(records==before&&inventory_reference_scope==&outer);current_thread=7;
    inventory_reference_begin(&nested,&f,property,archive,map,base+0x488ed5,1);bitcall(base+0x602c40,1,archive);CHECK(records==before);inventory_reference_end(&nested);CHECK(inventory_reference_scope==&outer);
    property_valid=0;inventory_reference_begin(&nested,&f,property,archive,map,base+0x488ed4,1);bitcall(base+0x602c40,1,archive);inventory_reference_end(&nested);property_valid=1;CHECK(records==before);
    inventory_decode_frame=&inner_f;bitcall(base+0x602c40,1,archive);inventory_decode_frame=&f;CHECK(records==before);
    ++f.id;bitcall(base+0x602c40,1,archive);--f.id;CHECK(records==before);
    put(channel,0xa0,0);bitcall(base+0x602c40,1,archive);put(channel,0xa0,(uintptr_t)pawn);CHECK(records==before);
    inventory_reference_begin(&nested,&f,property,archive,map,base+0x488ed4,1);intcall(base+0x602c5d,0x3ff,archive);CHECK(records==before);native_tag=0;bitcall(base+0x602c40,1,archive);intcall(base+0x602c5d,0x3ff,archive);CHECK(records==before+1);intcall(base+0x602cc3,0x80000000,archive);CHECK(records==before+2);inventory_reference_end(&nested);CHECK(outer.tag==1);
    inventory_reference_rows=128;before=records;bitcall(base+0x602c40,1,archive);intcall(base+0x602c5d,0x3ff,archive);CHECK(records==before&&inventory_reference_rows==128);
    inventory_reference_end(&outer);CHECK(!inventory_reference_scope);bitcall(base+0x602c40,1,archive);CHECK(records==before);
    printf("PASS authored reference observer: forwarding, LastError, scope, outputs, channel snapshot, budget and transactional slots\n");return 0;
}
