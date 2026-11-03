/* Exact production intent helpers and complete retirement loop on temporary
 * objects. Scope/engine admission and UObject identity/type lookup are explicit
 * stand-ins. No game, RPC, target selection or native power action is invoked. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define RUNTIME_POWER_LIMIT 64
#define ME3_ROLE_HOST 1
#define ME3_ROLE_GUEST 2
typedef struct GameArray {uintptr_t data;int count,max;} GameArray;
typedef struct SessionHandle {uintptr_t pointer;int32_t index;uint64_t name;} SessionHandle;
typedef struct GuestCameraStamp {uintptr_t pointer;int32_t index;char full_name[600];} GuestCameraStamp;
typedef struct SessionContext {uintptr_t pc,pawn,world;unsigned char mode;} SessionContext;
typedef struct Object {_Alignas(8) unsigned char data[0x1300];} Object;
typedef struct Entry {Object *object;const char *name,*kind,*path;int live,generation;} Entry;
static Object pc,pawn,power,world,manager,channel,connection,driver,old_ai,foreign,klass,field;
static Entry entries[12];static int entry_count;
static struct {int role;} g_runtime_config;
static int runtime_control_enabled,runtime_scope_ready,configured_enabled,scope_ready,layout_ready,blocked,deny_power_type;
static DWORD session_game_thread;static uintptr_t session_engine;
static uintptr_t denied_at,unwritable_at,race_at;static uint32_t race_value;static unsigned log_rows,cases;
static SessionContext environment;
static uintptr_t at(Object *object){return(uintptr_t)object->data;}
static void p64(uintptr_t address,uint64_t value){memcpy((void*)address,&value,8);}
static void p32(uintptr_t address,uint32_t value){memcpy((void*)address,&value,4);}
static int read_at(uintptr_t address,void *out,SIZE_T size){SIZE_T got=0;
    if(!address||(denied_at&&address<=denied_at&&denied_at-address<size))return 0;
    return ReadProcessMemory(GetCurrentProcess(),(void*)address,out,size,&got)&&got==size;
}
static uintptr_t read_ptr(uintptr_t address){uintptr_t value=0;read_at(address,&value,8);return value;}
static int background_focus_writable(uintptr_t address,SIZE_T size){
    if(race_at==address){p32(race_at,race_value);race_at=0;}
    return address&&!(unwritable_at&&address<=unwritable_at&&unwritable_at-address<size);
}
static Entry *entry(uintptr_t address){for(int i=0;i<entry_count;++i)if(at(entries[i].object)==address)return&entries[i];return NULL;}
static int is_live(uintptr_t base,uintptr_t address){(void)base;Entry *e=entry(address);return e&&e->live;}
static int narrative_probe_object(uintptr_t base,uintptr_t address,const char *kind){
    (void)base;Entry *e=entry(address);if(!e||!e->live)return 0;
    if(address==at(&power)&&!strcmp(kind,"SFXPowerCustomAction"))return !deny_power_type;
    return !strcmp(e->kind,kind);
}
static int is_kind_of(uintptr_t base,uintptr_t address,const char *kind){return narrative_probe_object(base,address,kind);}
static int guest_camera_actor_live(uintptr_t base,uintptr_t address){return is_live(base,address);}
static void object_name(uintptr_t base,uintptr_t address,char *out,size_t size){(void)base;Entry *e=entry(address);snprintf(out,size,"%s",e?e->name:"");}
static int guest_camera_stamp_read(uintptr_t base,uintptr_t address,GuestCameraStamp *out,int full){
    (void)base;Entry *e=entry(address);if(!e||!e->live)return 0;
    memset(out,0,sizeof*out);out->pointer=address;out->index=e->generation;
    if(full)snprintf(out->full_name,sizeof out->full_name,"%s",e->path);return 1;
}
static int guest_camera_stamp_matches(uintptr_t base,const GuestCameraStamp *stamp,int full){GuestCameraStamp current;
    return guest_camera_stamp_read(base,stamp->pointer,&current,full)&&current.index==stamp->index&&
        (!full||!strcmp(current.full_name,stamp->full_name));
}
static SessionHandle handle(Object *object){Entry *e=entry(at(object));return(SessionHandle){at(object),e->generation,at(object)^0x1234};}
static int session_handle_matches(SessionHandle value){Entry *e=entry(value.pointer);
    return e&&e->live&&value.index==e->generation&&value.name==(value.pointer^0x1234);
}
static int runtime_power_action_context_enabled(void){return configured_enabled;}
static SessionContext session_context(uintptr_t engine){(void)engine;return environment;}
static int session_engine_blocked(SessionContext context){(void)context;return blocked;}
static int runtime_power_action_context_layout(uintptr_t base,uintptr_t action){(void)base;return layout_ready&&action==at(&power);}
static void line(const char *text){(void)text;++log_rows;}

/* Current production structs and helpers, generated without rewriting. */
#include "power_order_intent_types.h"
static RuntimePowerCatalog fixture_catalog;
static int runtime_power_catalog_same(const RuntimePowerCatalog *a,const RuntimePowerCatalog *b){return !memcmp(a,b,sizeof*a);}
static int runtime_power_action_context_scope(uintptr_t base,SessionContext context,RuntimePowerCatalog *out){
    (void)base;
    if(!scope_ready||context.pc!=at(&pc)||context.pawn!=at(&pawn)||context.world!=at(&world))return 0;
    *out=fixture_catalog;return 1;
}
#include "power_order_intent_helpers.h"

static void add(Object *object,const char *name,const char *kind,const char *path){entries[entry_count++]=(Entry){object,name,kind,path,1,1};}
static void reset(void){
    Object *objects[]={&pc,&pawn,&power,&world,&manager,&channel,&connection,&driver,&old_ai,&foreign,&klass,&field};
    for(unsigned i=0;i<sizeof objects/sizeof *objects;++i)memset(objects[i],0,sizeof*objects[i]);
    entry_count=0;
    add(&pc,"PC","SFXPlayerController","PC");add(&pawn,"Ashley","SFXPawn_Ashley","Ashley");
    add(&power,"InfernoGrenade","SFXPowerCustomAction","Power");add(&world,"World","BioWorldInfo","World");
    add(&manager,"Manager","SFXPowerManager","Manager");add(&channel,"Channel","ActorChannel","Channel");
    add(&connection,"Connection","NetConnection","Connection");add(&driver,"Driver","NetDriver","Driver");
    add(&old_ai,"AI","SFXAI_Core","AI");add(&foreign,"Foreign","SFXPlayerController","Foreign");
    add(&klass,"SFXPowerCustomActionBase","Class","SFXGame.SFXPowerCustomActionBase");
    add(&field,"m_bPlayerOrderedPowerUse","BoolProperty","SFXGame.SFXPowerCustomActionBase.m_bPlayerOrderedPowerUse");
    p64(at(&pc)+0x3bc,at(&pawn));p64(at(&pawn)+0x374,at(&pc));p64(at(&pawn)+0x198,at(&world));
    p64(at(&power)+0x98,at(&pawn));p64(at(&power)+0xa0,at(&pc));p64(at(&power)+0x50,at(&klass));
    p64(at(&klass)+0x70,at(&field));p64(at(&field)+0x40,at(&klass));p32(at(&field)+0x70,1);
    p32(at(&field)+0x74,4);p32(at(&field)+0x94,0x4dc);p64(at(&field)+0x78,0x2000);p64(at(&field)+0xd0,8);
    p32(at(&power)+0x4dc,0xb11);p64(at(&old_ai)+0x198,at(&world));
    g_runtime_config.role=ME3_ROLE_HOST;runtime_control_enabled=runtime_scope_ready=configured_enabled=scope_ready=layout_ready=1;
    session_game_thread=GetCurrentThreadId();session_engine=1;blocked=deny_power_type=0;
    denied_at=unwritable_at=race_at=0;race_value=0;log_rows=0;runtime_power_action_context_events=0;
    environment=(SessionContext){at(&pc),at(&pawn),at(&world),0};
    memset(&runtime_power_action_context,0,sizeof runtime_power_action_context);
    RuntimePowerActionContext *q=&runtime_power_action_context;q->active=1;q->count=1;
    q->catalog.pc=handle(&pc);q->catalog.pawn=handle(&pawn);q->catalog.info=handle(&world);
    q->catalog.manager=handle(&manager);q->catalog.channel=handle(&channel);q->catalog.connection=handle(&connection);q->catalog.driver=handle(&driver);
    q->catalog.registration_token=3;q->catalog.count=1;q->catalog.array=(GameArray){at(&manager)+0x200,1,1};
    q->catalog.powers[0]=handle(&power);q->catalog.names[0][0]=23;
    guest_camera_stamp_read(0,at(&power),&q->rows[0].action,1);q->rows[0].changed=1;
    fixture_catalog=q->catalog;
}
static int intent(void){return runtime_power_action_context_player_order(0,at(&pc),at(&pawn),at(&power));}
static int retire(void){return runtime_power_action_context_retire(0,environment,0,0,0);}
static uint32_t flags(void){uint32_t value=0;read_at(at(&power)+0x4dc,&value,4);return value;}
static int check(int condition,const char *name){++cases;if(!condition)fprintf(stderr,"FAIL %s (case%u)\n",name,cases);return condition;}
#define CHECK(condition,name) do{if(!check((condition),(name)))return 1;}while(0)
int main(void){
    reset();CHECK(intent()&&flags()==0xb19&&runtime_power_action_context.rows[0].changed==5,"owned intent installation");
    CHECK(intent()&&flags()==0xb19&&runtime_power_action_context.rows[0].changed==5,"repeat keeps one ownership");
    CHECK(runtime_power_action_context_retire(0,environment,1,1,0)&&flags()==0xb19&&runtime_power_action_context.active,"ordinary lease retains intent");
    p32(at(&pawn)+0xc20,132);p32(at(&pawn)+0xc28,23);
    CHECK(!retire()&&flags()==0xb19&&read_ptr(at(&power)+0xa0)==at(&pc),"running power defers cleanup");
    p32(at(&pawn)+0xc20,0);p32(at(&pawn)+0xc28,0);p32(at(&power)+0x100,0x10800000);
    CHECK(!retire()&&flags()==0xb19,"running action instance defers cleanup");p32(at(&power)+0x100,0);
    p32(at(&power)+0x4dc,flags()|0x40000000);
    CHECK(retire()&&flags()==0x40000b11&&!read_ptr(at(&power)+0xa0)&&!runtime_power_action_context.active,"idle cleanup preserves unrelated flag update");
    reset();p32(at(&power)+0x4dc,0xb19);CHECK(intent()&&runtime_power_action_context.rows[0].changed==1,"native existing intent remains unowned");
    CHECK(retire()&&flags()==0xb19,"native intent preserved at retirement");
    reset();CHECK(intent(),"native clear setup");p32(at(&power)+0x4dc,0xb11);CHECK(retire()&&flags()==0xb11,"native already-cleared intent is not recreated");
    reset();CHECK(intent(),"replacement setup");p64(at(&power)+0xa0,at(&foreign));CHECK(retire()&&flags()==0xb19&&read_ptr(at(&power)+0xa0)==at(&foreign),"foreign context replacement preserved");
    reset();CHECK(intent(),"disposed action setup");entry(at(&power))->generation++;
    CHECK(retire()&&flags()==0xb19,"recycled power receives no cleanup write");
    reset();CHECK(intent(),"disposed world setup");entry(at(&world))->live=0;
    CHECK(retire()&&flags()==0xb19,"disposed world receives no cleanup write");
    reset();CHECK(intent(),"AI restoration setup");runtime_power_action_context.rows[0].changed|=2;runtime_power_action_context.rows[0].old_ai=at(&old_ai);
    guest_camera_stamp_read(0,at(&old_ai),&runtime_power_action_context.rows[0].ai,1);
    CHECK(retire()&&flags()==0xb11&&read_ptr(at(&power)+0xa8)==at(&old_ai),"owned intent clears before original AI returns");
    for(int action=9;action<=54;action+=45){reset();p32(at(&pawn)+0xc20,(uint32_t)action);p32(at(&power)+0x100,0x10800000);p64(at(&power)+0xb0,88);
        CHECK(intent()&&flags()==0xb19,"held lease admits intent during reload or evade");CHECK(!retire()&&flags()==0xb19,"busy lease retirement waits");}
    reset();runtime_power_action_context_events=1000;CHECK(intent()&&flags()==0xb19&&!log_rows,"functional intent is independent of diagnostic quota");
    for(int bad=0;bad<30;++bad){reset();RuntimePowerActionContext *q=&runtime_power_action_context;
        switch(bad){
        case 0:g_runtime_config.role=ME3_ROLE_GUEST;break;
        case 1:session_game_thread++;break;case 2:runtime_control_enabled=0;break;case 3:runtime_scope_ready=0;break;
        case 4:configured_enabled=0;break;case 5:q->active=0;break;case 6:q->retiring=1;break;
        case 7:q->catalog.pc.pointer=at(&foreign);break;case 8:q->catalog.pawn.pointer=at(&foreign);break;
        case 9:scope_ready=0;break;case 10:fixture_catalog.registration_token++;break;
        case 11:p64(at(&pc)+0x3bc,at(&foreign));break;case 12:p64(at(&pawn)+0x374,at(&foreign));break;
        case 13:deny_power_type=1;break;case 14:p64(at(&power)+0x98,at(&foreign));break;
        case 15:p64(at(&power)+0xa0,at(&foreign));break;case 16:p64(at(&power)+0xa8,at(&old_ai));break;
        case 17:p64(at(&field)+0xd0,4);break;case 18:p64(at(&field)+0x78,0x2020);break;
        case 19:p32(at(&field)+0x94,0x4d8);break;case 20:p32(at(&field)+0x74,8);break;
        case 21:p32(at(&field)+0x70,2);break;case 22:entry(at(&field))->kind="IntProperty";break;
        case 23:q->count=0;break;case 24:entry(at(&power))->generation++;break;
        case 25:denied_at=at(&power)+0x4dc;break;case 26:unwritable_at=at(&power)+0x4dc;break;
        case 27:q->rows[1]=q->rows[0];q->count=2;break;
        case 28:q->catalog.count=2;q->catalog.powers[1]=q->catalog.powers[0];q->catalog.names[1][0]=24;fixture_catalog=q->catalog;break;
        case 29:entry(at(&klass))->path="Other.SFXPowerCustomActionBase";break;
        }
        Object original=power;unsigned changed=q->rows[0].changed;
        CHECK(!intent(),"refused invalid ownership or metadata");
        CHECK(!memcmp(&original,&power,sizeof power)&&q->rows[0].changed==changed,"refusal does not change power or claim intent");
    }
    reset();race_at=at(&power)+0x4dc;race_value=0xc11;
    CHECK(!intent()&&flags()==0xc11&&runtime_power_action_context.rows[0].changed==1,"CAS lost race preserves replacement flags");
    reset();CHECK(intent(),"cleanup write denial setup");unwritable_at=at(&power)+0x4dc;
    CHECK(!retire()&&flags()==0xb19&&runtime_power_action_context.rows[0].changed==5,"cleanup failure retains ownership and PC");
    unwritable_at=0;CHECK(retire()&&flags()==0xb11,"cleanup retries only owned intent");
    printf("PASS %u exact production host power-order intent/retirement checks; no game launched\n",cases);return 0;
}
