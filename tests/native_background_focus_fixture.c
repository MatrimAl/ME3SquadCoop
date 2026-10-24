/* Private offline test. Real candidate wrapper + exact measured native exec
 * and getter execute only in allocated fixture memory. Viewport focus itself
 * is an explicit stand-in. No game process or game file is opened. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
typedef struct GameArray {uintptr_t data;int count,max;} GameArray;
typedef struct SessionHandle {uintptr_t pointer;int32_t index;uint64_t name;} SessionHandle;
typedef struct GuestCameraStamp {
    uintptr_t pointer,klass;int32_t index,class_index;uint32_t name[2],class_name[2];char full_name[600];
} GuestCameraStamp;
typedef struct SessionContext {uintptr_t pc,pawn,world,squad,manager;unsigned char mode;} SessionContext;
typedef struct Object {_Alignas(8) unsigned char bytes[0x1100];} Object;
typedef struct Registration {Object *object;const char *kind,*name,*path;} Registration;
static Object owner,fn,tick,field,world,level,info,engine,player,pc,remote,driver,connection,manager;
static Object class_object,world_vtable,viewport,focus_object,focus_vtable,frame,wrong_node,conversation;
static Registration registry[40];static int registered;
static uintptr_t base,peer_slots[2],actor_slots[1],player_slots[1];
static unsigned char script[396];static LONG native_calls;static int original_value,original_entry_error,logs,deny_pins,break_context;
static void put(uintptr_t at,const void *value,size_t count){memcpy((void*)at,value,count);}
static uintptr_t addr(Object *o){return(uintptr_t)o->bytes;}
static void p64(uintptr_t at,uintptr_t value){put(at,&value,8);}
static void p32(uintptr_t at,uint32_t value){put(at,&value,4);}
static void p16(uintptr_t at,uint16_t value){put(at,&value,2);}
static void p8(uintptr_t at,unsigned char value){put(at,&value,1);}
static int read_at(uintptr_t at,void *out,SIZE_T count){SIZE_T got=0;
    if(deny_pins&&(at==base+0xb4fa90||at==base+0xad40e0)){SetLastError(8001);return 0;}
    int ok=at&&ReadProcessMemory(GetCurrentProcess(),(void*)at,out,count,&got)&&got==count;
    SetLastError(8001);return ok;
}
static uintptr_t read_ptr(uintptr_t at){uintptr_t out=0;read_at(at,&out,8);return out;}
static Registration *reg(uintptr_t at){for(int i=0;i<registered;++i)if(addr(registry[i].object)==at)return&registry[i];return NULL;}
static int is_live(uintptr_t b,uintptr_t at){(void)b;return reg(at)!=NULL;}
static int is_kind_of(uintptr_t b,uintptr_t at,const char *kind){(void)b;Registration *r=reg(at);
    if(!r)return 0;if(!strcmp(r->kind,kind))return 1;
    return !strcmp(kind,"Actor")&&(!strcmp(r->kind,"BioWorldInfo")||!strcmp(r->kind,"PlayerController"));
}
static int narrative_probe_object(uintptr_t b,uintptr_t at,const char *kind){return is_kind_of(b,at,kind);}
static void object_name(uintptr_t b,uintptr_t at,char *out,size_t size){(void)b;Registration*r=reg(at);snprintf(out,size,"%s",r?r->name:"unknown");}
static int guest_camera_actor_live(uintptr_t b,uintptr_t at){return is_kind_of(b,at,"Actor")&&!(read_ptr(at+0x25c)&8)&&!(read_ptr(at+0x260)&0x800000);}
static SessionHandle session_capture_handle(uintptr_t at){SessionHandle h={0};h.pointer=at;
    if(!read_at(at+8,&h.index,4)||!read_at(at+0x48,&h.name,8))h.pointer=0;return h;
}
static int session_handle_matches(SessionHandle h){SessionHandle n=session_capture_handle(h.pointer);
    return n.pointer==h.pointer&&n.index==h.index&&n.name==h.name;
}
static int guest_camera_stamp_read(uintptr_t b,uintptr_t at,GuestCameraStamp*out,int full){(void)b;Registration*r=reg(at);memset(out,0,sizeof*out);
    if(!r)return 0;out->pointer=at;out->klass=read_ptr(at+0x50);read_at(at+8,&out->index,4);read_at(at+0x48,out->name,8);
    if(full)snprintf(out->full_name,sizeof out->full_name,"%s",r->path);return out->klass!=0;
}
static int guest_camera_stamp_matches(uintptr_t b,const GuestCameraStamp*s,int full){GuestCameraStamp n;
    return guest_camera_stamp_read(b,s->pointer,&n,full)&&n.klass==s->klass&&n.index==s->index&&!memcmp(n.name,s->name,8)&&(!full||!strcmp(n.full_name,s->full_name));
}
static uintptr_t local_controller(uintptr_t e){GameArray players={0};read_at(e+0x498,&players,sizeof players);
    return players.count==1?read_ptr(read_ptr(players.data)+0x68):0;
}
static SessionContext session_context(uintptr_t e){SessionContext c={0};c.pc=local_controller(e);c.world=read_ptr(c.pc+0x198);c.manager=read_ptr(c.pc+0xc38);
    c.mode=255;read_at(c.manager+0xa8,&c.mode,1);return c;
}
static uintptr_t guest_player_context_find_method(uintptr_t b,uintptr_t cls,const char*o,const char*n){(void)b;
    if(cls!=addr(&owner)||strcmp(o,"BioWorldInfo"))return 0;
    return !strcmp(n,"HasFocus")?addr(&fn):!strcmp(n,"Tick")?addr(&tick):0;
}
static int guest_player_context_function_abi(uintptr_t b,uintptr_t f,const char *o,const char*n,uint32_t flags,uint16_t size,uint16_t result,uint16_t index,uintptr_t native,unsigned char count){
    uint32_t af=0;uint16_t as=0,ar=0,ai=0;unsigned char ac=0;Registration*r=reg(f),*own=reg(read_ptr(f+0x40));
    return r&&own&&!strcmp(r->name,n)&&!strcmp(own->name,o)&&read_at(f+0xe0,&af,4)&&af==flags&&
        read_at(f+0xe4,&ai,2)&&ai==index&&read_at(f+0xe6,&as,2)&&as==size&&read_at(f+0xe8,&ar,2)&&ar==result&&
        read_at(f+0xea,&ac,1)&&ac==count&&read_ptr(f+0xd8)==b+native;
}
static void line(const char*text){(void)text;++logs;SetLastError(9999);}
#include "background_focus.h"

static void add(Object *object,const char*kind,const char*name,const char*path){
    int i=registered++;registry[i]=(Registration){object,kind,name,path};p32(addr(object)+8,(uint32_t)i+1);
    p64(addr(object)+0x48,(uintptr_t)(0x1000+i));p64(addr(object)+0x50,addr(&class_object));
}
static int viewport_focus(void*self){
    if((uintptr_t)self!=addr(&focus_object))abort();
    original_entry_error=(int)GetLastError();InterlockedIncrement(&native_calls);
    if(break_context)p64(addr(&world)+0x120,addr(&wrong_node));
    SetLastError(888);return original_value;
}
static int alternate_world_focus(void*self){(void)self;return viewport_focus((void*)addr(&focus_object));}
static void unknown_original(void*self,void*stack,void*output){(void)self;
    original_entry_error=(int)GetLastError();InterlockedIncrement(&native_calls);
    p64((uintptr_t)stack+0x28,read_ptr((uintptr_t)stack+0x28)+1);*(uint32_t*)output=2;SetLastError(888);
}
static void function(Object*f,uint32_t flags,uintptr_t native){
    p64(addr(f)+0x40,addr(&owner));p64(addr(f)+0xd8,native);p32(addr(f)+0xe0,flags);
    p16(addr(f)+0xe6,4);p16(addr(f)+0xe8,f==&fn?0:65535);p8(addr(f)+0xea,1);
}
static void setup(void){
    base=(uintptr_t)VirtualAlloc(NULL,0x18b8000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);if(!base)abort();
    add(&class_object,"Class","Class","Core.Class");add(&owner,"Class","BioWorldInfo","SFXGame.BioWorldInfo");
    add(&fn,"Function","HasFocus","SFXGame.BioWorldInfo.HasFocus");add(&tick,"Function","Tick","SFXGame.BioWorldInfo.Tick");
    add(&field,"BoolProperty","ReturnValue","SFXGame.BioWorldInfo.HasFocus.ReturnValue");
    add(&world,"World","TheWorld","Fixture.TheWorld");add(&level,"Level","PersistentLevel","Fixture.TheWorld.PersistentLevel");
    add(&info,"BioWorldInfo","BioWorldInfo_0","Fixture.BioWorldInfo_0");add(&engine,"Engine","SFXEngine_0","Fixture.Engine");
    add(&player,"LocalPlayer","LocalPlayer_0","Fixture.LocalPlayer");add(&pc,"PlayerController","PC_0","Fixture.PC_0");
    add(&remote,"PlayerController","PC_1","Fixture.PC_1");add(&driver,"NetDriver","NetDriver_0","Fixture.NetDriver");
    add(&connection,"NetConnection","NetConnection_0","Fixture.NetConnection");add(&manager,"SFXGameModeManager","ModeManager_0","Fixture.ModeManager");
    add(&wrong_node,"Function","Other","Fixture.Other");
    add(&conversation,"BioConversationManager","Conversation_0","Fixture.Conversation_0");
    p64(addr(&info)+0x50,addr(&owner));
    memcpy((void*)(base+0xb4fa90),background_focus_exec_pin,sizeof background_focus_exec_pin);
    memcpy((void*)(base+0xad40e0),background_focus_getter_pin,sizeof background_focus_getter_pin);
    p64(base+0x18b6638,addr(&world));p64(base+0x18b31d0,addr(&engine));p64(addr(&world)+0x88,addr(&level));
    actor_slots[0]=addr(&info);GameArray a={(uintptr_t)actor_slots,1,1};put(addr(&level)+0x60,&a,sizeof a);
    p64(addr(&world)+0x120,addr(&driver));p64(addr(&info),addr(&world_vtable));p64(addr(&world_vtable)+0x890,base+0xad40e0);
    player_slots[0]=addr(&player);a=(GameArray){(uintptr_t)player_slots,1,1};put(addr(&engine)+0x498,&a,sizeof a);
    p64(addr(&player)+0x68,addr(&pc));p64(addr(&pc)+0x67c,addr(&player));p64(addr(&pc)+0x198,addr(&info));
    p64(addr(&pc)+0xc38,addr(&manager));p8(addr(&manager)+0xa8,0);
    peer_slots[0]=addr(&connection);a=(GameArray){(uintptr_t)peer_slots,1,1};put(addr(&driver)+0x70,&a,sizeof a);
    p64(addr(&connection)+0x68,addr(&remote));p64(addr(&connection)+0x8c,addr(&driver));
    p64(addr(&remote)+0x67c,addr(&connection));p64(addr(&remote)+0x198,addr(&info));p8(addr(&info)+0x7c8,2);
    p64(addr(&engine)+0x7a0,addr(&viewport));p64(addr(&viewport)+0x148,addr(&focus_object));p64(addr(&focus_object),addr(&focus_vtable));
    p64(addr(&focus_vtable)+0x50,(uintptr_t)viewport_focus);
    p64(addr(&fn)+0x48,485441);function(&fn,0x20400,base+0xb4fa90);function(&tick,0x20102,base+0xf52b0);
    p64(addr(&fn)+0x70,addr(&field));p64(addr(&field)+0x40,addr(&fn));p64(addr(&field)+0x78,0x580);p64(addr(&field)+0xd0,1);
    script[22]=7;for(int i=25;i<=29;++i)script[i]=0x82;script[30]=0x81;script[265]=7;script[268]=0x82;
    for(int i=0;i<2;++i){int at=i?269:60;script[at]=0x65;p64((uintptr_t)script+at+1,485441);p16((uintptr_t)script+at+9,184);script[at+11]=0x16;}
    script[72]=0x16;script[281]=0x18;a=(GameArray){(uintptr_t)script,396,396};put(addr(&tick)+0x7c,&a,sizeof a);
    p64(addr(&frame)+0x18,addr(&tick));p64(addr(&frame)+0x20,addr(&info));
    FlushInstructionCache(GetCurrentProcess(),(void*)base,0x18b8000);
    SetLastError(777);background_focus_prepare(base,addr(&engine),1);
    if(!background_focus.installed||GetLastError()!=777)abort();
}
static int call_case(const char*name,int entry,int expected){
    int failures=0;uint32_t output=99;LONG before=native_calls;DWORD world_before=0,focus_before=0;
    read_at(addr(&info)+0x7c0,&world_before,4);read_at(addr(&info)+0x9c8,&focus_before,4);
    p64(addr(&frame)+0x28,(uintptr_t)script+entry);SetLastError(777);
    background_focus_exec((void*)addr(&info),(void*)addr(&frame),&output);
    DWORD error=GetLastError();DWORD world_after=0,focus_after=0;read_at(addr(&info)+0x7c0,&world_after,4);read_at(addr(&info)+0x9c8,&focus_after,4);
    if(output!=(uint32_t)expected||native_calls!=before+1||original_entry_error!=777||error!=888||world_before!=world_after||focus_before!=focus_after)failures=1;
    if(failures)fprintf(stderr,"FAIL %s result=%u expected=%d calls=%ld inerr=%d outerr=%lu reason=%s\n",name,output,expected,native_calls-before,original_entry_error,error,background_focus_reason);
    return failures;
}
static DWORD WINAPI wrong_thread(void*unused){(void)unused;return (DWORD)call_case("wrong thread",71,0);}
int main(void){setup();int fails=0,tests=0;
#define CASE(n,c,e) do{++tests;fails+=call_case(n,c,e);}while(0)
    CASE("ordinary false",71,1);CASE("second Tick site",280,1);
    p32(addr(&info)+0x7c0,0x400);p32(addr(&info)+0x9c8,0x8000);CASE("own focus pause restoration",280,1);
    p32(addr(&info)+0x9c8,0);CASE("intentional UI pause preserved",71,0);p32(addr(&info)+0x7c0,0);
    p64(addr(&info)+0x948,addr(&conversation));p32(addr(&conversation)+0xd0,1);
    p32(addr(&info)+0x7c0,0x400);p32(addr(&info)+0x9c8,0x8000);
    CASE("mode0 dialogue focus pause restoration",280,1);
    p32(addr(&info)+0x9c8,0);CASE("dialogue intentional pause preserved",280,0);
    p64(addr(&info)+0x948,0);p32(addr(&info)+0x7c0,0);
    original_value=1;CASE("already focused true",71,1);original_value=0;
    background_focus_prepare(base,addr(&engine),0);CASE("disabled",71,0);background_focus_prepare(base,addr(&engine),1);
    p64(addr(&frame)+0x18,addr(&wrong_node));CASE("wrong caller",71,0);p64(addr(&frame)+0x18,addr(&tick));
    CASE("wrong cursor",70,0);
    p64(addr(&connection)+0x8c,0);CASE("connection driver mismatch",71,0);p64(addr(&connection)+0x8c,addr(&driver));
    GameArray empty={0};put(addr(&driver)+0x70,&empty,sizeof empty);CASE("no native guest",71,0);
    GameArray peers={(uintptr_t)peer_slots,1,1};put(addr(&driver)+0x70,&peers,sizeof peers);
    deny_pins=1;CASE("unreadable method pin",71,0);deny_pins=0;
    p8(base+0xad4113,2);FlushInstructionCache(GetCurrentProcess(),(void*)(base+0xad40e0),67);
    CASE("changed getter bytes",71,0);p8(base+0xad4113,1);FlushInstructionCache(GetCurrentProcess(),(void*)(base+0xad40e0),67);
    p64(addr(&world_vtable)+0x890,(uintptr_t)alternate_world_focus);CASE("changed native getter target",71,0);p64(addr(&world_vtable)+0x890,base+0xad40e0);
    p32(addr(&fn)+0xe0,0x20401);CASE("changed native function ABI",71,0);p32(addr(&fn)+0xe0,0x20400);
    p64(addr(&field)+0xd0,2);CASE("changed bool return mask",71,0);p64(addr(&field)+0xd0,1);
    p8(addr(&manager)+0xa8,7);CASE("scene game mode",71,0);p8(addr(&manager)+0xa8,0);
    p32(addr(&pc)+0x77c,0x20000);CASE("cinematic PC",71,0);p32(addr(&pc)+0x77c,0);
    p32(addr(&info)+0x7c0,0x800);CASE("pending pause",71,0);p32(addr(&info)+0x7c0,0);
    script[61]^=1;CASE("changed HasFocus FName",71,0);script[61]^=1;
    break_context=1;CASE("world driver changed during original",71,0);break_context=0;p64(addr(&world)+0x120,addr(&driver));
    BackgroundFocusExec original=background_focus.original;background_focus.original=unknown_original;
    CASE("unknown result preserved",71,2);background_focus.original=original;
    p8(addr(&info)+0x7c8,3);p64(addr(&driver)+0x80,addr(&connection));p64(addr(&connection)+0x68,addr(&pc));CASE("guest server connection",71,1);
    p64(addr(&connection)+0x68,addr(&remote));CASE("guest wrong actor",71,0);p8(addr(&info)+0x7c8,2);
    HANDLE thread=CreateThread(NULL,0,wrong_thread,NULL,0,NULL);DWORD result=1;
    if(!thread||WaitForSingleObject(thread,10000)!=WAIT_OBJECT_0||!GetExitCodeThread(thread,&result)||result)++fails;
    if(thread)CloseHandle(thread);++tests;
    if(logs>4){fprintf(stderr,"unbounded logs: %d\n",logs);++fails;}
    VirtualFree((void*)base,0,MEM_RELEASE);
    printf("background focus actual-wrapper fixture: %d cases, %d failures; original forwarded once, LastError preserved, pause state unchanged\n",tests,fails);
    return fails?1:0;
}
