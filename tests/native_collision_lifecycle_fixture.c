/* Offline fixture: exercise the real collision adapter against temporary
 * objects. Engine/object discovery and ProcessEvent are explicit stand-ins;
 * memory reads, masked CAS, scene fields and the adapter's state machine run
 * normally. This does not launch or load a game or prove engine compatibility. */
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <limits.h>

typedef struct GameArray {uintptr_t data;int count,max;} GameArray;
typedef struct GuestCameraStamp {uintptr_t pointer,klass;} GuestCameraStamp;
typedef struct GuestPlayerContextMethod {GuestCameraStamp function,owner;} GuestPlayerContextMethod;
typedef struct NetworkActorIdentityEntry {
    uintptr_t base,pawn,channel,connection,driver;int index;
} NetworkActorIdentityEntry;
typedef struct FixtureContext {
    int registered;unsigned long token;GuestCameraStamp pc,pawn;
} FixtureContext;
static FixtureContext guest_player_context_lease;
static unsigned presentation_queries;
static int guest_player_context_registered_for(uintptr_t pc_value,uintptr_t pawn_value) {
    ++presentation_queries;
    return guest_player_context_lease.registered&&guest_player_context_lease.pc.pointer==pc_value&&guest_player_context_lease.pawn.pointer==pawn_value;
}
static unsigned long guest_player_context_registration_token(void) {++presentation_queries;return guest_player_context_lease.token;}
typedef struct FixtureObject {_Alignas(8) unsigned char bytes[0x1000];} FixtureObject;
static FixtureObject pc,pawn,world,component,cdo,templ,pawn_arch,component_arch,klass,channel,connection,driver,other,fn,field,manager,conversation;
static _Alignas(8) unsigned char image[0x4ba140];
static uintptr_t members[2],invalid_stamp;
static int owned,channel_changed,abi_valid,camera_valid,dispatches,repair_logs;
static uint32_t dispatch_actor_before,dispatch_block_before;

static uintptr_t address(FixtureObject *p){return (uintptr_t)p->bytes;}
static void put(uintptr_t at,const void *value,SIZE_T size){memcpy((void*)at,value,size);}
static void ptr(uintptr_t at,uintptr_t value){put(at,&value,sizeof value);}
static void u32(uintptr_t at,uint32_t value){put(at,&value,sizeof value);}
static void put_byte(uintptr_t at,unsigned char value){put(at,&value,1);}
static int read_at(uintptr_t at,void *out,SIZE_T size){SIZE_T got=0;
    return at&&ReadProcessMemory(GetCurrentProcess(),(void*)at,out,size,&got)&&got==size;}
static uintptr_t read_ptr(uintptr_t at){uintptr_t value=0;read_at(at,&value,sizeof value);return value;}
static int guest_camera_stamp_read(uintptr_t base,uintptr_t at,GuestCameraStamp *out,int full){
    (void)base;(void)full;if(!at||at==invalid_stamp)return 0;
    out->pointer=at;out->klass=read_ptr(at+0x50);return out->klass!=0;}
static int guest_camera_stamp_matches(uintptr_t base,const GuestCameraStamp *s,int full){
    GuestCameraStamp actual={0};return guest_camera_stamp_read(base,s->pointer,&actual,full)&&actual.klass==s->klass;}
static int guest_camera_actor_live(uintptr_t base,uintptr_t at){
    GuestCameraStamp s={0};return guest_camera_stamp_read(base,at,&s,1);}
static int is_kind_of(uintptr_t base,uintptr_t at,const char *kind){(void)base;
    return (at==address(&cdo)||at==address(&pawn_arch))&&!strcmp(kind,"SFXPawn_Ashley");}
static int narrative_probe_object(uintptr_t base,uintptr_t at,const char *kind){(void)base;
    if(!strcmp(kind,"CylinderComponent"))return at==address(&component)||at==address(&templ)||at==address(&component_arch);
    if(!strcmp(kind,"ActorComponent"))return at==address(&component)||at==address(&other);
    if(!strcmp(kind,"ByteProperty"))return at==address(&field);
    if(!strcmp(kind,"SFXGameModeManager"))return at==address(&manager);
    if(!strcmp(kind,"BioConversationManager"))return at==address(&conversation);
    return 0;}
static void object_name(uintptr_t base,uintptr_t at,char *out,size_t size){(void)base;
    snprintf(out,size,"%s",at==address(&field)?"NewCollisionType":"unexpected");}
static uintptr_t function_named(uintptr_t base,const char *owner,const char *name){(void)base;
    return !strcmp(owner,"Actor")&&!strcmp(name,"SetCollisionType")?address(&fn):0;}
static int guest_player_context_function_abi(uintptr_t base,uintptr_t at,const char *owner,const char *name,
    uint32_t flags,uint16_t size,uint16_t result,uint16_t native_index,uintptr_t native,unsigned char num){
    (void)base;return abi_valid&&at==address(&fn)&&!strcmp(owner,"Actor")&&!strcmp(name,"SetCollisionType")&&
        flags==0x20401&&size==1&&result==65535&&native_index==0&&native==0x4ba0c0&&num==1;}
static int guest_player_context_method_stamp(uintptr_t base,uintptr_t at,GuestPlayerContextMethod *method){
    return guest_camera_stamp_read(base,at,&method->function,1);}
static int guest_player_context_basic_owned(uintptr_t base,uintptr_t p,uintptr_t a,uintptr_t w){(void)base;
    return owned&&p==address(&pc)&&a==address(&pawn)&&w==address(&world);}
static int guest_camera_conditions(uintptr_t base,uintptr_t p,uintptr_t a,uintptr_t w,
    int ordinary,int enabled,uintptr_t *camera,uintptr_t *mode){
    ++presentation_queries;
    *camera=*mode=0;return camera_valid&&ordinary&&enabled&&guest_player_context_basic_owned(base,p,a,w);}
static int guest_player_context_channel(uintptr_t base,uintptr_t a,NetworkActorIdentityEntry *out){
    if(a!=address(&pawn))return 0;
    *out=(NetworkActorIdentityEntry){base,a,channel_changed?address(&other):address(&channel),address(&connection),address(&driver),13};return 1;}
static int guest_player_context_dispatch(uintptr_t base,uintptr_t at,const GuestPlayerContextMethod *method,void *parms){
    (void)base;if(at!=address(&pawn)||method->function.pointer!=address(&fn)||*(unsigned char*)parms!=6)return 0;
    /* Exact native type6 sets bCollideActors/bBlockActors only, preserving
     * bCollideWorld and the unrelated neighboring Actor flag. */
    ++dispatches;uint32_t value=0;read_at(at+0x260,&value,4);dispatch_actor_before=value;u32(at+0x260,value|0x20000000);
    read_at(at+0x264,&value,4);dispatch_block_before=value;u32(at+0x264,value|1);
    read_at(address(&component)+0x16c,&value,4);u32(address(&component)+0x16c,(value&~UINT32_C(0xba))|0xaa);
    put_byte(at+0x26b,6);put_byte(at+0x26c,6);return 1;
}
static void line(const char *row){if(strstr(row,"\"guest_collision_restore\":\"repair-attempt\""))++repair_logs;}
#include "guest_collision_restore.h"

#define CHECK(condition) do {if(!(condition)){fprintf(stderr,"line %d: %s\n",__LINE__,#condition);return 0;}}while(0)
static void set_shape(uintptr_t actor,uintptr_t cylinder,int healthy){
    u32(actor+0x260,healthy?0x69690000:0x09690100);
    u32(actor+0x264,healthy?0x444003:0x444000);
    u32(cylinder+0x16c,healthy?0x8600aa:0x860020);
    put_byte(actor+0x26b,6);put_byte(actor+0x26c,6);
}
static void reset_fixture(void){
    FixtureObject *objects[]={&pc,&pawn,&world,&component,&cdo,&templ,&pawn_arch,&component_arch,&klass,&channel,&connection,&driver,&other,&fn,&field,&manager,&conversation};
    for(unsigned i=0;i<sizeof objects/sizeof *objects;++i){memset(objects[i],0,sizeof *objects[i]);ptr(address(objects[i])+0x50,address(&klass));}
    memset(&guest_collision_restore,0,sizeof guest_collision_restore);
    guest_collision_restore.configured=guest_collision_restore.opted=1;
    guest_player_context_lease=(FixtureContext){1,1,{address(&pc),address(&klass)},{address(&pawn),address(&klass)}};
    owned=abi_valid=camera_valid=1;channel_changed=dispatches=repair_logs=0;invalid_stamp=0;presentation_queries=0;
    ptr(address(&pc)+0x198,address(&world));ptr(address(&pawn)+0x198,address(&world));
    ptr(address(&pc)+0xc38,address(&manager));
    ptr(address(&pawn)+0x1e0,address(&component));ptr(address(&klass)+0x13c,address(&cdo));
    ptr(address(&cdo)+0x1e0,address(&templ));ptr(address(&component)+0x58,address(&templ));
    ptr(address(&pawn)+0x58,address(&cdo));ptr(address(&templ)+0x40,address(&cdo));
    ptr(address(&component_arch)+0x40,address(&pawn_arch));
    set_shape(address(&pawn),address(&component),1);set_shape(address(&cdo),address(&templ),1);
    put_byte(address(&pawn)+0x269,3);put_byte(address(&pawn)+0x26a,2);put_byte(address(&pawn)+0x268,1);
    u32(address(&pawn)+0xe18,0x40);members[0]=address(&component);members[1]=address(&other);
    GameArray components={ (uintptr_t)members,2,2 };put(address(&pawn)+0x60,&components,sizeof components);
    static const unsigned char entry[]={0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20,0x48,0x8b,0x42,0x28};
    static const unsigned char call[]={0x0f,0xb6,0x54,0x24,0x38,0x48,0x8b,0xcf,0xe8,0xe4,0x55,0xf5,0xff};
    memcpy(image+0x4ba0c0,entry,sizeof entry);memcpy(image+0x4ba11f,call,sizeof call);
    ptr(address(&fn)+0x70,address(&field));ptr(address(&field)+0x40,address(&fn));
    u32(address(&field)+0x94,0);u32(address(&field)+0x74,1);u32(address(&field)+0x70,1);
    uint64_t flags=0x80;put(address(&field)+0x78,&flags,8);
}
static void frame(int ordinary,int session_enabled,DWORD now){
    guest_collision_adapt((uintptr_t)image,address(&pc),owned?address(&pawn):0,address(&world),ordinary,session_enabled,now);
}
static int baseline(void){reset_fixture();frame(1,1,100);
    CHECK(guest_collision_restore.tracked&&guest_collision_restore.was_owned&&dispatches==0);return 1;}
static int release(void){
    owned=0;guest_player_context_lease.registered=0;u32(address(&world)+0x7c0,0x400);
    set_shape(address(&pawn),address(&component),0);
    frame(0,1,200);CHECK(guest_collision_restore.tracked&&guest_collision_restore.pending&&!guest_collision_restore.released);
    frame(0,1,210);CHECK(guest_collision_restore.released&&dispatches==0);return 1;
}
static void reacquire(void){owned=1;guest_player_context_lease.registered=1;guest_player_context_lease.token=2;}
static int restored(void){GuestCollisionShape shape={0};
    CHECK(guest_collision_shape(address(&pawn),address(&component),&shape));
    CHECK(guest_collision_healthy(&shape)&&dispatches==1&&repair_logs==1&&guest_collision_restore.token==2&&guest_collision_restore.was_owned&&presentation_queries==0);
    uint32_t a=0,b=0;read_at(address(&pawn)+0x260,&a,4);read_at(address(&pawn)+0x264,&b,4);
    CHECK(a==(dispatch_actor_before|0x60000000)&&b==(dispatch_block_before|1));return 1;
}
static int lifecycle_cases(void){
    CHECK(baseline());frame(0,1,150);CHECK(guest_collision_restore.tracked&&dispatches==0);
    CHECK(release());reacquire();
    /* PlayersOnly blocks the native call even if the caller's ordinary flag
     * is accidentally optimistic; it must retain the two released samples. */
    frame(1,1,300);CHECK(guest_collision_restore.released&&dispatches==0);
    u32(address(&world)+0x7c0,0);frame(1,1,310);CHECK(restored());
    frame(1,1,320);CHECK(dispatches==1&&repair_logs==1);

    /* A true session disable retires the baseline. A later broken pawn must
     * not manufacture a healthy baseline or inherit repair permission. */
    CHECK(baseline());CHECK(release());frame(0,0,220);CHECK(!guest_collision_restore.tracked);
    reacquire();u32(address(&world)+0x7c0,0);frame(1,1,300);CHECK(!guest_collision_restore.tracked&&dispatches==0);

    CHECK(baseline());CHECK(release());reacquire();u32(address(&world)+0x7c0,0);
    /* The measured return has no UI re-registration, the old UI token and
     * a camera target that still belongs to something else. Physical repair
     * must use actual ownership/channel, never these presentation adapters. */
    guest_player_context_lease.registered=0;guest_player_context_lease.token=1;camera_valid=0;
    ptr(address(&pc)+0x3f4,address(&other));frame(1,1,300);CHECK(restored());

    reset_fixture();guest_player_context_lease.registered=0;guest_player_context_lease.token=0;camera_valid=0;
    frame(1,1,100);CHECK(guest_collision_restore.tracked&&guest_collision_restore.token==1);
    CHECK(release());reacquire();u32(address(&world)+0x7c0,0);
    guest_player_context_lease.registered=0;guest_player_context_lease.token=0;frame(1,1,300);CHECK(restored());

    CHECK(baseline());CHECK(release());reacquire();u32(address(&world)+0x7c0,0);
    channel_changed=1;frame(1,1,300);CHECK(dispatches==0&&guest_collision_restore.token==1&&!guest_collision_restore.was_owned);
    /* A foreign discovered channel grants nothing and does not consume the
     * captured channel's later valid ownership return. */
    channel_changed=0;frame(1,1,310);CHECK(restored());

    CHECK(baseline());CHECK(release());reacquire();u32(address(&world)+0x7c0,0);
    invalid_stamp=address(&component);frame(1,1,300);CHECK(guest_collision_restore.suppressed&&dispatches==0);
    invalid_stamp=0;frame(1,1,310);CHECK(dispatches==0);

    CHECK(baseline());CHECK(release());reacquire();u32(address(&world)+0x7c0,0);
    ptr(address(&component)+0x58,address(&other));frame(1,1,300);CHECK(guest_collision_restore.suppressed&&dispatches==0);

    CHECK(baseline());CHECK(release());reacquire();u32(address(&world)+0x7c0,0);
    u32(address(&component)+0x16c,0x860000);frame(1,1,300);CHECK(dispatches==0&&guest_collision_restore.token==2&&guest_collision_restore.was_owned);

    CHECK(baseline());CHECK(release());reacquire();u32(address(&world)+0x7c0,0);
    abi_valid=0;frame(1,1,300);CHECK(dispatches==0&&guest_collision_restore.token==2&&guest_collision_restore.was_owned);

    CHECK(baseline());CHECK(release());reacquire();u32(address(&world)+0x7c0,0);
    image[0x4ba11f]^=1;frame(1,1,300);CHECK(dispatches==0);

    CHECK(baseline());owned=0;set_shape(address(&pawn),address(&component),0);frame(0,1,200);
    reacquire();frame(1,1,300);CHECK(dispatches==0); /* one disabled sample is insufficient */
    /* A later refused release shape cancels BOTH the candidate and its stable
     * flag. A stale snapshot must not consume the next ownership attempt. */
    CHECK(baseline());CHECK(release());u32(address(&pawn)+0x260,0x49690100);
    frame(0,1,220);CHECK(!guest_collision_restore.pending&&!guest_collision_restore.released);
    reacquire();u32(address(&world)+0x7c0,0);frame(1,1,300);
    CHECK(dispatches==0&&guest_collision_restore.token==1&&!guest_collision_restore.was_owned);
    CHECK(release());reacquire();u32(address(&world)+0x7c0,0);frame(1,1,400);CHECK(restored());
    return 1;
}
static int physical_guard_cases(void) {
    /* Each guard is a real field consumed by guest_collision_ordinary. The
     * caller intentionally supplies ordinary=true: native scene/action/input
     * state must still refuse, retaining the candidate until it is safe. */
    struct Blocker {FixtureObject *object;unsigned offset,size;uint64_t value,ordinary;} blockers[]={
        {&world,0x7c0,4,0x400,0},{&world,0x7c0,4,0x800,0},
        {&world,0x670,8,1,0},
        {&pawn,0xe20,4,0x40000,0},
        {&pawn,0xe18,4,0,0x40},{&pawn,0xe18,4,0x200040,0x40},
        {&pawn,0xe18,4,0x20000040,0x40},
        {&pawn,0xc20,4,9,0},{&pawn,0xc28,4,4,0},
        {&pc,0x781,1,1,0},{&pc,0x77c,4,0x800,0},{&pc,0x77c,4,0x20000,0},
        {&manager,0xa8,1,3,0},{&manager,0xa8,1,5,0},
        {&manager,0xa8,1,7,0},{&manager,0xa8,1,8,0},{&manager,0xa4,4,1,0},
        {&pawn,0x25c,4,0x4000000,0},{&pawn,0x268,1,0,1},{&pawn,0x268,1,3,1}
    };
    for(unsigned n=0;n<sizeof blockers/sizeof *blockers;++n) {
        CHECK(baseline());CHECK(release());reacquire();u32(address(&world)+0x7c0,0);
        struct Blocker *b=blockers+n;uintptr_t at=address(b->object)+b->offset;
        put(at,&b->value,b->size);frame(1,1,300);
        CHECK(dispatches==0&&guest_collision_restore.released&&guest_collision_restore.token==1&&!guest_collision_restore.was_owned);
        put(at,&b->ordinary,b->size);frame(1,1,310);CHECK(restored());
    }
    CHECK(baseline());CHECK(release());reacquire();u32(address(&world)+0x7c0,0);
    ptr(address(&pc)+0xc38,address(&other));frame(1,1,300);CHECK(dispatches==0);
    ptr(address(&pc)+0xc38,address(&manager));frame(1,1,310);CHECK(restored());

    for(unsigned flag=0;flag<2;++flag) {
        CHECK(baseline());CHECK(release());reacquire();u32(address(&world)+0x7c0,0);
        ptr(address(&world)+0x948,address(&conversation));
        GameArray active={0,flag?0:1,1};put(address(&conversation)+0xc8,&active,sizeof active);
        u32(address(&conversation)+0xe8,flag?1:0);frame(1,1,300);
        CHECK(dispatches==0&&guest_collision_restore.released&&guest_collision_restore.token==1&&!guest_collision_restore.was_owned);
        active.count=0;put(address(&conversation)+0xc8,&active,sizeof active);
        u32(address(&conversation)+0xe8,0);frame(1,1,310);CHECK(restored());
    }
    /* Death/downed is not a temporary scene guard. It discards the captured
     * release; becoming alive cannot replay that candidate without another
     * real ownership release and stable disabled collision observation. */
    for(uint32_t life=0x100;life<=0x200;life+=0x100) {
        CHECK(baseline());CHECK(release());reacquire();u32(address(&world)+0x7c0,0);
        u32(address(&pawn)+0xe20,life);frame(1,1,300);
        CHECK(dispatches==0&&!guest_collision_restore.released&&!guest_collision_restore.pending);
        u32(address(&pawn)+0xe20,0);frame(1,1,310);CHECK(dispatches==0);
        CHECK(release());reacquire();u32(address(&world)+0x7c0,0);frame(1,1,410);CHECK(restored());
    }
    return 1;
}
static void use_variant(void) {
    ptr(address(&pawn)+0x58,address(&pawn_arch));ptr(address(&component)+0x58,address(&component_arch));
    put_byte(address(&pawn)+0x26c,10);put_byte(address(&cdo)+0x26c,10);
}
static int variant_cases(void) {
    /* Observed first-lease shape: type6, initial replicated sentinel10,
     * same physical class default but a component from a pawn archetype. */
    reset_fixture();use_variant();frame(1,1,100);
    CHECK(guest_collision_restore.tracked&&guest_collision_restore.healthy.replicated==10&&dispatches==0);
    CHECK(release());reacquire();u32(address(&world)+0x7c0,0);frame(1,1,310);CHECK(restored());

    reset_fixture();use_variant();ptr(address(&component_arch)+0x40,address(&other));frame(1,1,100);
    CHECK(!guest_collision_restore.tracked&&dispatches==0);
    reset_fixture();use_variant();ptr(address(&component_arch)+0x50,address(&other));frame(1,1,100);
    CHECK(!guest_collision_restore.tracked&&dispatches==0);
    reset_fixture();use_variant();ptr(address(&pawn_arch)+0x50,address(&other));frame(1,1,100);
    CHECK(!guest_collision_restore.tracked&&dispatches==0);
    reset_fixture();use_variant();put_byte(address(&pawn)+0x26c,11);frame(1,1,100);
    CHECK(!guest_collision_restore.tracked&&dispatches==0);
    reset_fixture();use_variant();u32(address(&templ)+0x16c,0x8600ba);frame(1,1,100);
    CHECK(!guest_collision_restore.tracked&&dispatches==0);

    reset_fixture();use_variant();frame(1,1,100);CHECK(guest_collision_restore.tracked);
    CHECK(release());reacquire();u32(address(&world)+0x7c0,0);
    ptr(address(&pawn)+0x58,address(&other));frame(1,1,310);
    CHECK(guest_collision_restore.suppressed&&dispatches==0);
    /* Captured control return: local autonomous2, RemoteRole2. No role write;
     * the same native owned pawn/channel and real return are still required. */
    reset_fixture();use_variant();frame(1,1,100);CHECK(guest_collision_restore.tracked);
    CHECK(release());reacquire();u32(address(&world)+0x7c0,0);
    put_byte(address(&pawn)+0x269,2);frame(1,1,310);CHECK(restored());
    CHECK(*(unsigned char*)(address(&pawn)+0x269)==2&&*(unsigned char*)(address(&pawn)+0x26a)==2);
    /* Measured ordinary return keeps localRole2 but RemoteRole1. It cannot
     * acquire a baseline; captured reciprocal ownership permits repair. */
    reset_fixture();use_variant();frame(1,1,100);CHECK(guest_collision_restore.tracked);
    CHECK(release());reacquire();u32(address(&world)+0x7c0,0);
    put_byte(address(&pawn)+0x269,1);frame(1,1,310);CHECK(restored());
    CHECK(*(unsigned char*)(address(&pawn)+0x269)==1&&*(unsigned char*)(address(&pawn)+0x26a)==2);
    for(unsigned char role=0;role<=3;++role)if(role!=2){
        reset_fixture();use_variant();put_byte(address(&pawn)+0x26a,role);frame(1,1,100);
        CHECK(!guest_collision_restore.tracked&&dispatches==0);
    }
    for(unsigned char remote=0;remote<=4;++remote)if(remote!=2&&remote!=3){
        reset_fixture();use_variant();put_byte(address(&pawn)+0x269,remote);frame(1,1,100);
        CHECK(!guest_collision_restore.tracked&&dispatches==0);
    }
    /* Actor collision can be absent while the cylinder retains its healthy
     * response. Recover via the same native type6 path; never force a type
     * toggle, touch role bytes or arm a state that retains world collision. */
    for(unsigned char type=0;type<=6;type+=6){
        reset_fixture();use_variant();frame(1,1,100);CHECK(guest_collision_restore.tracked);
        owned=0;guest_player_context_lease.registered=0;set_shape(address(&pawn),address(&component),0);
        u32(address(&pawn)+0x264,0x444002);u32(address(&component)+0x16c,0x8600aa);
        put_byte(address(&pawn)+0x26b,type);put_byte(address(&pawn)+0x26c,type==0?0:10);
        frame(0,1,200);frame(0,1,210);CHECK(guest_collision_restore.released);
        reacquire();put_byte(address(&pawn)+0x269,2);frame(1,1,310);CHECK(restored());
    }
    GuestCollisionShape partial={0x40000000,1,0xaa,0,0},unknown={0,0,0xba,0,0};
    CHECK(guest_collision_disabled(&partial)&&!guest_collision_disabled(&unknown));
    for(uint32_t blocking=0;blocking<=1;++blocking)for(uint32_t adjacent=0;adjacent<=2;adjacent+=2){
        reset_fixture();use_variant();frame(1,1,100);CHECK(guest_collision_restore.tracked);
        owned=0;guest_player_context_lease.registered=0;set_shape(address(&pawn),address(&component),0);
        u32(address(&pawn)+0x260,0x49690100);u32(address(&pawn)+0x264,0x444000|adjacent|blocking);
        u32(address(&component)+0x16c,0x8600aa);put_byte(address(&pawn)+0x26b,0);put_byte(address(&pawn)+0x26c,0);
        frame(0,1,200);frame(0,1,210);CHECK(guest_collision_restore.released);
        reacquire();frame(1,1,310);CHECK(restored());
        uint32_t after=0;read_at(address(&pawn)+0x264,&after,4);CHECK((after&2)==adjacent);
    }
    /* Actual scene return: fully disabled Actor with retained block latch.
     * Two stable samples, a native ownership return and identity guards are required. */
    for(uint32_t adjacent=0;adjacent<=2;adjacent+=2){
        reset_fixture();use_variant();frame(1,1,100);CHECK(guest_collision_restore.tracked);
        owned=0;guest_player_context_lease.registered=0;set_shape(address(&pawn),address(&component),0);
        u32(address(&pawn)+0x260,0x09690100);u32(address(&pawn)+0x264,0x444001|adjacent);
        u32(address(&component)+0x16c,0x8600aa);put_byte(address(&pawn)+0x26b,0);put_byte(address(&pawn)+0x26c,0);
        frame(0,1,200);CHECK(!guest_collision_restore.released);
        frame(0,1,210);CHECK(guest_collision_restore.released&&dispatches==0);
        reacquire();frame(1,1,310);CHECK(restored());
        uint32_t after=0;read_at(address(&pawn)+0x264,&after,4);CHECK((after&2)==adjacent);
        int calls=dispatches;frame(1,1,320);CHECK(dispatches==calls);
    }
    GuestCollisionShape unknown_type={0x40000000,0,0xaa,12,12},unknown_response={0x40000000,0,0xba,0,0};
    CHECK(!guest_collision_disabled(&unknown_type)&&!guest_collision_disabled(&unknown_response));
    /* Ordinary landing return: only world collision is missing; retain actor
     * collision/blocking/cylinder and require the same captured release path. */
    for(uint32_t adjacent=0;adjacent<=2;adjacent+=2){
        reset_fixture();use_variant();frame(1,1,100);CHECK(guest_collision_restore.tracked);
        owned=0;guest_player_context_lease.registered=0;set_shape(address(&pawn),address(&component),0);
        u32(address(&pawn)+0x260,0x29690100);u32(address(&pawn)+0x264,0x444001|adjacent);
        u32(address(&component)+0x16c,0x8600aa);put_byte(address(&pawn)+0x26b,0);put_byte(address(&pawn)+0x26c,0);
        frame(0,1,200);CHECK(!guest_collision_restore.released);
        frame(0,1,210);CHECK(guest_collision_restore.released&&dispatches==0);
        reacquire();frame(1,1,310);CHECK(restored());
        uint32_t after=0;read_at(address(&pawn)+0x264,&after,4);CHECK((after&2)==adjacent);
        int calls=dispatches;frame(1,1,320);CHECK(dispatches==calls);
    }
    for(unsigned int field=0;field<4;++field){
        GuestCollisionShape bad={0x20000000,1,0xaa,0,0};
        if(field==0)bad.block=0;if(field==1)bad.component=0xba;if(field==2)bad.type=1;if(field==3)bad.replicated=10;
        CHECK(!guest_collision_disabled(&bad));
    }
    /* Live landing return: the same reciprocal owned pawn/channel, role0/2,
     * type8/response12 and no world/blocking collision. The initial role0
     * baseline is still refused above. No role byte may be rewritten. */
    reset_fixture();use_variant();frame(1,1,100);CHECK(guest_collision_restore.tracked);
    owned=0;guest_player_context_lease.registered=0;set_shape(address(&pawn),address(&component),0);
    u32(address(&pawn)+0x260,0x29690100);u32(address(&pawn)+0x264,0x444000);
    u32(address(&component)+0x16c,0x860012);put_byte(address(&pawn)+0x26b,8);put_byte(address(&pawn)+0x26c,8);
    frame(0,1,200);frame(0,1,210);CHECK(guest_collision_restore.released);
    reacquire();put_byte(address(&pawn)+0x269,0);frame(1,1,310);CHECK(restored());
    CHECK(*(unsigned char*)(address(&pawn)+0x269)==0&&*(unsigned char*)(address(&pawn)+0x26a)==2);
    for(unsigned int field=0;field<4;++field){
        GuestCollisionShape bad={0x20000000,0,0x12,8,8};
        if(field==0)bad.block=1;if(field==1)bad.component=0xaa;if(field==2)bad.type=0;if(field==3)bad.replicated=10;
        CHECK(!guest_collision_disabled(&bad));
    }
    return 1;
}
int main(void){if(!guest_collision_selftest()||!lifecycle_cases()||!physical_guard_cases()||!variant_cases())return 1;
    puts("PASS collision adapter lifecycle: native ownership return without UI/camera, physical guards, variants and identity refusal");return 0;}
