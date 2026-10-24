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
static uintptr_t actors[1],writer[4];static char row[2000];
static void put(void *p,size_t off,uintptr_t value){memcpy((char*)p+off,&value,8);}
static int read_at(uintptr_t at,void *out,size_t n){
 SetLastError(999);
 if(scan_read_fail&&at==(uintptr_t)connection+0xf84+10*8)return 0;
 if(at==initial_actor_trace_base+0x18b6638&&n==8){uintptr_t value=(uintptr_t)world;memcpy(out,&value,8);return 1;}
 void *buf[]={property,pawn,world,level,info,manager,archive,map,connection,channel,other_channel,actors,writer};
 size_t sizes[]={sizeof property,sizeof pawn,sizeof world,sizeof level,sizeof info,sizeof manager,sizeof archive,sizeof map,sizeof connection,sizeof channel,sizeof other_channel,sizeof actors,sizeof writer};
 for(size_t i=0;i<sizeof sizes/sizeof *sizes;i++)if(at>=(uintptr_t)buf[i]&&n<=sizes[i]&&at-(uintptr_t)buf[i]<=sizes[i]-n){memcpy(out,(void*)at,n);return 1;}
 return 0;
}
static uintptr_t read_ptr(uintptr_t at){uintptr_t v=0;read_at(at,&v,8);return v;}
static int is_live(uintptr_t base,uintptr_t p){(void)base;return p==(uintptr_t)pawn||p==(uintptr_t)world||p==(uintptr_t)level||p==(uintptr_t)info||p==(uintptr_t)manager;}
static int is_kind_of(uintptr_t base,uintptr_t p,const char *kind){(void)base;return pawn_valid&&p==(uintptr_t)pawn&&(!strcmp(kind,"Pawn")||!strcmp(kind,"SFXPawn_Ashley"));}
static int has_marker(const wchar_t *p){(void)p;return marker;}
static int inventory_decode_property(uintptr_t p){return property_valid&&p==(uintptr_t)property;}
static void line(const char *s){++records;snprintf(row,sizeof row,"%s",s);SetLastError(998);}
#include "inventory_send_metadata.h"
static int capture(uintptr_t site){SetLastError(123);int result=inventory_send_metadata_capture((uintptr_t)property,(uintptr_t)archive,(uintptr_t)map,(uintptr_t)pawn+0x414,site);CHECK(GetLastError()==123);return result;}
int main(int argc,char **argv){
 if(argc>1&&!strcmp(argv[1],"canary"))CHECK(0);
 uintptr_t base=initial_actor_trace_base;actors[0]=(uintptr_t)info;
 put(world,0x88,(uintptr_t)level);GameArray a={(uintptr_t)actors,1,1};memcpy(level+0x60,&a,sizeof a);
 put(pawn,0x198,(uintptr_t)info);put(pawn,0x414,(uintptr_t)manager);put(manager,0x50,0x1234);
 put(map,0,base+0x11a0d20);put(map,0x108,(uintptr_t)connection);put(archive,0,(uintptr_t)writer);
 writer[2]=base+0xc2bd0;writer[3]=base+0xc3110;
 put(connection,0xf84+9*8,(uintptr_t)channel);put(channel,0xa0,(uintptr_t)manager);
 uint32_t type=2,flags=1;memcpy(channel+0x78,&type,4);memcpy(channel+0x68,&flags,4);
 CHECK(capture(base+0x48d433)&&records==1&&strstr(row,"\"channel_matches\":1")&&strstr(row,"\"channel_index\":9")&&strstr(row,"\"channel_details_valid\":true"));
 CHECK(strstr(row,"\"archive_bits_rva\":\"0xc2bd0\"")&&strstr(row,"\"archive_index_rva\":\"0xc3110\""));
 CHECK(!capture(base+0x48d434));test_action=5;CHECK(!capture(base+0x48d433));test_action=6;
 current_thread=8;CHECK(!capture(base+0x48d433));current_thread=7;
 marker=0;CHECK(!capture(base+0x48d433));marker=1;property_valid=0;CHECK(!capture(base+0x48d433));property_valid=1;
 pawn_valid=0;CHECK(!capture(base+0x48d433));pawn_valid=1;
 put(pawn,0x198,0);CHECK(!capture(base+0x48d433));put(pawn,0x198,(uintptr_t)info);
 put(pawn,0x414,0);CHECK(capture(base+0x48d433)&&strstr(row,"\"channel_matches\":0"));
 put(pawn,0x414,0xdead);CHECK(!capture(base+0x48d433));put(pawn,0x414,(uintptr_t)manager);
 put(map,0x108,0);CHECK(!capture(base+0x48d433));put(map,0x108,(uintptr_t)connection);
 put(connection,0xf84+9*8,0);CHECK(capture(base+0x48d433)&&strstr(row,"\"channel_scan_valid\":true")&&strstr(row,"\"channel_matches\":0"));
 put(connection,0xf84+9*8,(uintptr_t)channel);scan_read_fail=1;CHECK(capture(base+0x48d433)&&strstr(row,"\"channel_scan_valid\":false"));scan_read_fail=0;
 put(connection,0xf84+10*8,(uintptr_t)other_channel);memcpy(other_channel,channel,sizeof channel);
 CHECK(capture(base+0x48d433)&&strstr(row,"\"channel_matches\":2")&&strstr(row,"\"channel_details_valid\":false"));
 type=1;memcpy(other_channel+0x78,&type,4);CHECK(capture(base+0x48d433)&&strstr(row,"\"channel_matches\":1"));
 inventory_send_metadata_rows=32;int before=records;CHECK(!capture(base+0x48d433)&&records==before);
 puts("PASS authored sender metadata: scope, identity, ambiguity, unreadable scan, nonactor channels, LastError and budget");return 0;
}
