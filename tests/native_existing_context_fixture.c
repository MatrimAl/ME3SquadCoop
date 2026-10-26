/* Actual existing-context lease and CAS cleanup on authored memory. Object
 * lookup, native ownership checks and SetPlayerPawn are explicit stand-ins.
 * This fixture cannot establish compatibility with the game engine. */
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef struct GuestCameraStamp {uintptr_t pointer,klass;char full_name[600];} GuestCameraStamp;
typedef struct NetworkActorIdentityEntry {uintptr_t channel,connection,driver;int index;} NetworkActorIdentityEntry;
typedef struct CombatContextChain {uintptr_t world,level,info,squad;int world_valid,level_valid,info_valid;} CombatContextChain;
typedef struct FixtureObject {_Alignas(8) unsigned char data[0x1000];} FixtureObject;
static FixtureObject objects[12];
static unsigned long guest_player_context_token=1;
static struct {int tracked,pending_cleanup,registered;unsigned long token;GuestCameraStamp pc,pawn;} guest_player_context_lease;
static struct {GuestCameraStamp klass;int set_pawn;} guest_player_context_methods;
static CombatContextChain current_chain;
static NetworkActorIdentityEntry current_channel;
static int native_calls,clear_calls,dispatch_mode,conditions_valid=1,getter_valid=1;
static uintptr_t addr(unsigned i){return (uintptr_t)objects[i].data;}
static int read_at(uintptr_t at,void *out,SIZE_T count){SIZE_T got=0;
    return at&&ReadProcessMemory(GetCurrentProcess(),(void*)at,out,count,&got)&&got==count;}
static uintptr_t read_ptr(uintptr_t at){uintptr_t out=0;read_at(at,&out,8);return out;}
static void put(uintptr_t at,uintptr_t value){memcpy((void*)at,&value,8);}
static int guest_camera_stamp_read(uintptr_t base,uintptr_t at,GuestCameraStamp *out,int full){
    (void)base;(void)full;if(!at)return 0;memset(out,0,sizeof *out);out->pointer=at;out->klass=read_ptr(at+0x50);return out->klass!=0;}
static int guest_camera_stamp_core_equal(const GuestCameraStamp *a,const GuestCameraStamp *b){return a->pointer==b->pointer&&a->klass==b->klass;}
static int guest_camera_stamp_matches(uintptr_t base,const GuestCameraStamp *s,int full){GuestCameraStamp actual={0};
    return guest_camera_stamp_read(base,s->pointer,&actual,full)&&guest_camera_stamp_core_equal(s,&actual);}
static int guest_camera_actor_live(uintptr_t base,uintptr_t at){GuestCameraStamp actual;return guest_camera_stamp_read(base,at,&actual,1);}
static int is_kind_of(uintptr_t base,uintptr_t at,const char *kind){(void)base;return at==addr(6)&&!strcmp(kind,"BioPlayerSquad");}
static int is_live(uintptr_t base,uintptr_t at){return guest_camera_actor_live(base,at);}
static int guest_player_context_field_writable(uintptr_t at){MEMORY_BASIC_INFORMATION m;
    if((at&7)||VirtualQuery((void*)at,&m,sizeof m)!=sizeof m||m.State!=MEM_COMMIT||
       (m.Protect&(PAGE_NOACCESS|PAGE_GUARD)))return 0;
    DWORD p=m.Protect&0xff;
    return p==PAGE_READWRITE||p==PAGE_WRITECOPY||p==PAGE_EXECUTE_READWRITE||p==PAGE_EXECUTE_WRITECOPY;}
static void combat_context_read_chain(uintptr_t base,CombatContextChain *out){(void)base;*out=current_chain;}
static int combat_context_get(uintptr_t base,uintptr_t input,uint32_t *out){(void)base;(void)input;
    *out=read_ptr(addr(6)+0x2a8)==addr(1)&&getter_valid;return 1;}
static int guest_player_context_pins(uintptr_t base){(void)base;return 1;}
static int guest_player_context_resolve(uintptr_t base,uintptr_t pc,DWORD now){(void)base;(void)pc;(void)now;return 1;}
static int guest_player_context_conditions(uintptr_t base,uintptr_t pc,uintptr_t pawn,uintptr_t info,
    int ordinary,int enabled,int acquiring,NetworkActorIdentityEntry *channel,CombatContextChain *chain){
    (void)base;(void)pc;(void)pawn;(void)info;(void)acquiring;*channel=current_channel;*chain=current_chain;
    return conditions_valid&&ordinary&&enabled;}
static int guest_player_context_dispatch(uintptr_t base,uintptr_t squad,const int *method,void *parms){
    (void)base;(void)method;uintptr_t pawn=*(uintptr_t*)parms;if(pawn)++native_calls;else ++clear_calls;
    if(dispatch_mode==1)return 0;
    put(squad+0x2cc,pawn);if(dispatch_mode!=2)put(squad+0x2a8,pawn);return 1;}
static char verification_detail[320];
static void line(const char *text){if(strstr(text,"verification-detail"))snprintf(verification_detail,sizeof verification_detail,"%s",text);}
#include "guest_existing_context.h"
#define CHECK(x) do {if(!(x)){printf("FAIL existing-context line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static void reset(void){
    memset(objects,0,sizeof objects);memset(&guest_existing_context,0,sizeof guest_existing_context);
    verification_detail[0]=0;
    memset(&guest_player_context_lease,0,sizeof guest_player_context_lease);
    for(unsigned i=0;i<12;++i)put(addr(i)+0x50,addr(11));
    current_chain=(CombatContextChain){addr(3),addr(4),addr(2),addr(6),1,1,1};
    current_channel=(NetworkActorIdentityEntry){addr(7),addr(8),addr(9),12};
    guest_player_context_methods.klass.pointer=addr(11);
    put(addr(0)+0x67c,addr(5));put(addr(0)+0x6dc,addr(10));
    put(addr(6)+0x198,addr(2));put(addr(6)+0x40,addr(4));put(addr(2)+0x8e8,addr(6));put(addr(1)+0xba8,addr(6));
    put(addr(6)+0x2d4,addr(10));
    uint32_t flags=14;memcpy((void*)(addr(6)+0x2b0),&flags,4);
    guest_player_context_token=1;native_calls=clear_calls=dispatch_mode=0;conditions_valid=getter_valid=1;
}
static void adapt(int eligible){guest_existing_context_adapt(0,addr(0),addr(1),addr(2),eligible,&current_channel,&current_chain,10000);}
int main(void){
    _Alignas(64) unsigned char edge[128]={0};uintptr_t value=addr(1);memcpy(edge+60,&value,8);
    CHECK(!guest_existing_context_restore_field((uintptr_t)edge+60,value,0)&&read_ptr((uintptr_t)edge+60)==value);
    void *readonly=VirtualAlloc(NULL,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);DWORD previous=0;
    CHECK(readonly);put((uintptr_t)readonly,value);
    CHECK(VirtualProtect(readonly,4096,PAGE_READONLY,&previous));
    CHECK(!guest_existing_context_restore_field((uintptr_t)readonly,value,0)&&read_ptr((uintptr_t)readonly)==value);
    CHECK(VirtualFree(readonly,0,MEM_RELEASE));
    reset();put(addr(6)+0x2a8,addr(1));put(addr(6)+0x2cc,addr(1));adapt(1);
    CHECK(guest_player_context_registered_for(addr(0),addr(1))&&!native_calls&&!clear_calls&&!guest_existing_context.cache_writes_owned);
    adapt(1);adapt(0);
    CHECK(!guest_existing_context.active&&!native_calls&&!clear_calls&&read_ptr(addr(6)+0x2a8)==addr(1)&&read_ptr(addr(6)+0x2cc)==addr(1));
    reset();put(addr(6)+0x2a8,addr(1));put(addr(6)+0x2cc,addr(1));getter_valid=0;adapt(1);
    CHECK(!guest_existing_context.registered&&!native_calls&&!clear_calls&&read_ptr(addr(6)+0x2a8)==addr(1));
    reset();put(addr(6)+0x2a8,addr(10));put(addr(6)+0x2cc,addr(10));adapt(1);
    CHECK(!guest_existing_context.registered&&!native_calls&&!clear_calls&&read_ptr(addr(6)+0x2a8)==addr(10));
    reset();adapt(0);CHECK(!native_calls);adapt(1);
    CHECK(native_calls==1&&guest_player_context_registered_for(addr(0),addr(1))&&guest_player_context_registration_token()==1);
    CHECK(read_ptr(addr(6)+0x2a8)==addr(1)&&read_ptr(addr(6)+0x2cc)==addr(1));
    adapt(1);CHECK(native_calls==1);adapt(0);
    CHECK(!guest_existing_context.active&&!read_ptr(addr(6)+0x2a8)&&!read_ptr(addr(6)+0x2cc)&&clear_calls==1);
    CHECK(read_ptr(addr(6)+0x2d4)==addr(10)&&read_ptr(addr(2)+0x8e8)==addr(6)&&read_ptr(addr(1)+0xba8)==addr(6));
    adapt(1);CHECK(native_calls==1);++guest_player_context_token;adapt(1);CHECK(native_calls==2);
    put(addr(6)+0x2a8,addr(10));adapt(1);
    CHECK(read_ptr(addr(6)+0x2a8)==addr(10)&&read_ptr(addr(6)+0x2cc)==addr(1)&&!guest_existing_context.active);
    reset();adapt(1);put(addr(6)+0x2cc,addr(10));adapt(1);
    CHECK(!read_ptr(addr(6)+0x2a8)&&read_ptr(addr(6)+0x2cc)==addr(10)&&!guest_existing_context.active&&!clear_calls);
    reset();dispatch_mode=2;adapt(1);CHECK(native_calls==1&&!guest_existing_context.active&&!read_ptr(addr(6)+0x2cc));
    CHECK(strstr(verification_detail,"\"cached_player\":0")&&strstr(verification_detail,"\"combat_getter\":-1"));
    adapt(1);CHECK(native_calls==1);
    reset();dispatch_mode=1;adapt(1);CHECK(native_calls==1&&!guest_existing_context.active&&!read_ptr(addr(6)+0x2a8));
    CHECK(strstr(verification_detail,"\"dispatch\":0")&&strstr(verification_detail,"\"conditions\":-1"));
    reset();getter_valid=0;adapt(1);CHECK(native_calls==1&&!guest_existing_context.active&&!read_ptr(addr(6)+0x2a8)&&!read_ptr(addr(6)+0x2cc));
    CHECK(strstr(verification_detail,"\"current_player\":1")&&strstr(verification_detail,"\"combat_getter\":0"));
    reset();put(addr(6)+0x2cc,addr(10));adapt(1);CHECK(!native_calls&&read_ptr(addr(6)+0x2cc)==addr(10));
    reset();put(addr(6)+0x2cc,addr(1));adapt(1);adapt(0);CHECK(!read_ptr(addr(6)+0x2a8)&&read_ptr(addr(6)+0x2cc)==addr(1));
    reset();adapt(1);current_chain.world_valid=0;adapt(0);
    CHECK(guest_existing_context.active&&!guest_existing_context.registered&&read_ptr(addr(6)+0x2a8)==addr(1));
    current_chain.world_valid=1;adapt(0);CHECK(!guest_existing_context.active&&!read_ptr(addr(6)+0x2a8));
    reset();adapt(1);current_chain.world=addr(10);adapt(0);
    CHECK(!guest_existing_context.active&&read_ptr(addr(6)+0x2a8)==addr(1));
    reset();adapt(1);put(addr(6)+0x50,addr(10));adapt(0);
    CHECK(!guest_existing_context.active&&read_ptr(addr(6)+0x2a8)==addr(1));
    reset();adapt(1);put(addr(2)+0x50,addr(10));adapt(0);
    CHECK(!guest_existing_context.active&&read_ptr(addr(6)+0x2a8)==addr(1));
    reset();adapt(1);dispatch_mode=1;
    for(unsigned i=0;i<12;++i)guest_existing_context_cleanup(0,"fixture-no-op",10000+i*1000);
    CHECK(clear_calls==8&&guest_existing_context.active&&!guest_existing_context.registered);
    reset();put(addr(1)+0xba8,addr(10));adapt(1);CHECK(!native_calls);
    reset();current_channel.index=0;adapt(1);CHECK(!native_calls);
    puts("PASS existing native squad lease: cache application, scope cleanup, partial/no-op dispatch, native replacements and unknown-world handling; no game launched");
    return 0;
}
