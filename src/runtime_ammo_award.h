#ifndef ME3_RUNTIME_AMMO_AWARD_H
#define ME3_RUNTIME_AMMO_AWARD_H
/* Select the retained MP percentage in this one real GiveAmmo call's locals.
 * Native foreach/AddAmmo still owns all quantities, caps and weapon exceptions.
 * No actor, handler, ammunition, profile or asset field is changed here. */
#include "runtime_ammo_award_pins.h"
typedef struct RuntimeAmmoAward {
    uintptr_t base;
    GuestPlayerContextMethod give;
    DWORD checked;
    unsigned rows;
} RuntimeAmmoAward;
static RuntimeAmmoAward runtime_ammo_award;
typedef struct RuntimeAmmoAwardCapture {
    SessionContext host;
    GuestCameraStamp pickup,pawn,pc,inventory,gri,handler;
    GameArray script;
    uintptr_t locals,frame;
    unsigned char tier;
    float authored;
    int weapon_offset,valid;
} RuntimeAmmoAwardCapture;
static int runtime_ammo_award_enabled(void) {
    static int enabled=-1;if(enabled<0){wchar_t text[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_MP_AMMO_AWARD",text,8);
        enabled=!n||(n==1&&text[0]==L'1');}return enabled;
}
static int runtime_ammo_award_policy(unsigned tier,float authored) {
    static const float campaign[]={0.75f,0.6f,0.4f};
    return tier<3&&authored==campaign[tier];
}
static int runtime_ammo_award_contract(uintptr_t base,int *weapon_offset) {
    RuntimeAmmoAward *s=&runtime_ammo_award;int size=0;uintptr_t prop=read_ptr(s->give.function.pointer+0x70);
    static const char *names[]={"P","GRI","AmmoAwardPct","Weapon"};
    static const char *types[]={"ObjectProperty","ObjectProperty","FloatProperty","ObjectProperty"};
    static const int offsets[]={0,8,16,0},sizes[]={8,8,4,8};
    static const char *classes[]={"Engine.Pawn","SFXGame.SFXGRI",NULL,"SFXGame.SFXWeapon"};
    if(!guest_player_context_method_matches(base,&s->give,1)||
       !guest_player_context_function_abi(base,s->give.function.pointer,"SFXDroppedAmmo","GiveAmmo",0x20002,8,65535,0,0xf52b0,1)||
       !read_at(s->give.function.pointer+0x78,&size,4)||size<28||size>64||!runtime_ammo_award_body(base,s->give.function.pointer))return 0;
    int iterator_offset=0;
    for(unsigned i=0;i<4;++i){char path[128];snprintf(path,sizeof path,"SFXGame.SFXDroppedAmmo.GiveAmmo.%s",names[i]);
        int at=offsets[i];if(i==3){if(!read_at(prop+0x94,&at,4)||at<20||(at&3)||at>size-8)return 0;iterator_offset=at;}
        if(!runtime_delayed_fire_property(base,prop,types[i],path,at,sizes[i],i?0:0x80)||
           read_ptr(prop+0x40)!=s->give.function.pointer)return 0;
        if(classes[i]){GuestCameraStamp target={0};
            if(!guest_camera_stamp_read(base,read_ptr(prop+0xd0),&target,1)||strcmp(target.full_name,classes[i]))return 0;}
        prop=read_ptr(prop+0x68);
    }
    if(prop)return 0;if(weapon_offset)*weapon_offset=iterator_offset;return 1;
}
static RuntimeAmmoAwardCapture runtime_ammo_award_capture(void *self,void *frame) {
    RuntimeAmmoAward *s=&runtime_ammo_award;RuntimeAmmoAwardCapture c={0};uintptr_t pickup=(uintptr_t)self,f=(uintptr_t)frame;
    uintptr_t state=0;uint32_t name[2],disabled=0;unsigned char role=0;
    if(!runtime_ammo_award_enabled()||!runtime_ammo_pickup.installed||!runtime_ammo_award_node||
       !runtime_shared_supply_coop(s->base,&c.host)||!runtime_ammo_pickup_executor_pins(s->base)||
       !runtime_ammo_award_contract(s->base,&c.weapon_offset)||!f||read_ptr(f+0x18)!=runtime_ammo_award_node||read_ptr(f+0x20)!=pickup||
       !read_at(runtime_ammo_award_node+0x7c,&c.script,sizeof c.script)||!c.script.data||
       c.script.count!=RUNTIME_AMMO_AWARD_VM_SIZE||c.script.max<c.script.count||c.script.max>4096||
       c.script.data>UINTPTR_MAX-RUNTIME_AMMO_AWARD_VM_SIZE||read_ptr(f+0x28)!=c.script.data||
       !guest_camera_actor_live(s->base,pickup)||!is_kind_of(s->base,pickup,"SFXDroppedAmmo")||
       read_ptr(pickup+0x198)!=c.host.world||!read_at(pickup+0x26a,&role,1)||role!=3||
       !native_state_fields(pickup,&state,name)||state!=runtime_ammo_pickup.valid.owner.pointer||
       !guest_camera_stamp_matches(s->base,&runtime_ammo_pickup.valid.owner,1)||
       !runtime_ammo_pickup_bit(s->base,pickup,"Actor","bTickIsDisabled",0x25c,2)||
       !read_at(pickup+0x25c,&disabled,4)||(disabled&0x40))return c;
    c.frame=f;c.locals=read_ptr(f+0x30);uintptr_t pawn=read_ptr(c.locals),pc=read_ptr(pawn+0x374);
    uintptr_t inventory=read_ptr(pawn+0x414),gri=read_ptr(c.host.world+0x6a8),handler=read_ptr(gri+0x340);
    if(!c.locals||(c.locals&7)||read_ptr(c.locals+8)||read_ptr(c.locals+c.weapon_offset)||
       !background_focus_writable(c.locals,c.weapon_offset+8)||!background_focus_writable(f+0x28,8)||
       !guest_camera_actor_live(s->base,pawn)||!guest_camera_actor_live(s->base,pc)||
       read_ptr(pc+0x3bc)!=pawn||read_ptr(pc+0x198)!=c.host.world||read_ptr(pawn+0x198)!=c.host.world||
       !narrative_probe_object(s->base,inventory,"InventoryManager")||read_ptr(inventory+0x180)!=pawn||
       !guest_camera_actor_live(s->base,gri)||!is_kind_of(s->base,gri,"SFXGRI")||read_ptr(gri+0x198)!=c.host.world||
       !narrative_probe_object(s->base,handler,"SFXDifficultyHandler")||read_ptr(handler+0x40)!=gri||
       !runtime_guest_team_field(s->base,handler,"SFXDifficultyHandler","CurrentDifficulty","ByteProperty",0x100,1,0)||
       !runtime_guest_team_field(s->base,handler,"SFXDifficultyHandler","AmmoPct","FloatProperty",0xc8,4,0)||
       !read_at(handler+0x100,&c.tier,1)||!read_at(handler+0xc8,&c.authored,4)||!runtime_ammo_award_policy(c.tier,c.authored))return c;
    uint32_t local_pct=1;if(!read_at(c.locals+16,&local_pct,4)||local_pct)return c;
    if(pawn!=c.host.pawn){RuntimeMovementScope guest={0};
        if(!runtime_ammo_pickup_owner(s->base,&guest)||guest.pawn.pointer!=pawn||guest.pc.pointer!=pc||
           guest.info.pointer!=c.host.world)return c;}
    else if(pc!=c.host.pc||!is_kind_of(s->base,pawn,"SFXPawn_Player"))return c;
    if(!guest_camera_stamp_read(s->base,pickup,&c.pickup,1)||!guest_camera_stamp_read(s->base,pawn,&c.pawn,1)||
       !guest_camera_stamp_read(s->base,pc,&c.pc,1)||!guest_camera_stamp_read(s->base,inventory,&c.inventory,1)||
       !guest_camera_stamp_read(s->base,gri,&c.gri,1)||!guest_camera_stamp_read(s->base,handler,&c.handler,1))return c;
    c.valid=1;return c;
}
static int runtime_ammo_award_local_select_using(int eligible,uintptr_t frame,uintptr_t locals,int weapon_offset,const GameArray *script,uintptr_t gri,
        int (*pointer)(uintptr_t,uintptr_t,uintptr_t)) {
    uint32_t half=UINT32_C(0x3f000000);unsigned char opcode=0;
    if(!eligible||!frame||!locals||!gri||(locals&7)||weapon_offset<20||(weapon_offset&3)||weapon_offset>56||!script->data||script->count!=RUNTIME_AMMO_AWARD_VM_SIZE||
       script->data>UINTPTR_MAX-RUNTIME_AMMO_AWARD_VM_SIZE||read_ptr(frame+0x30)!=locals||
       read_ptr(frame+0x28)!=script->data||read_ptr(locals+8)||read_ptr(locals+weapon_offset)||
       !read_at(script->data+RUNTIME_AMMO_AWARD_ITERATOR_OFFSET,&opcode,1)||opcode!=0x2f||
       !background_focus_writable(locals,weapon_offset+8)||!background_focus_writable(frame+0x28,8))return 0;
    if(!pointer(locals+8,0,gri))return 0;
    if(InterlockedCompareExchange((LONG volatile*)(locals+16),(LONG)half,0)!=0){
        pointer(locals+8,gri,0);return 0;}
    if(!pointer(frame+0x28,script->data,script->data+RUNTIME_AMMO_AWARD_ITERATOR_OFFSET)){
        InterlockedCompareExchange((LONG volatile*)(locals+16),0,(LONG)half);
        pointer(locals+8,gri,0);return 0;}
    return 1;
}
static int runtime_ammo_award_local_select(int eligible,uintptr_t frame,uintptr_t locals,int weapon_offset,const GameArray *script,uintptr_t gri) {
    return runtime_ammo_award_local_select_using(eligible,frame,locals,weapon_offset,script,gri,runtime_power_action_context_pointer);
}
static void runtime_ammo_award_before(void *self,void *frame) {
    RuntimeAmmoAward *s=&runtime_ammo_award;
    RuntimeAmmoAwardCapture a=runtime_ammo_award_capture(self,frame);if(!a.valid)return;
    RuntimeAmmoAwardCapture b=runtime_ammo_award_capture(self,frame);
    if(!b.valid||a.locals!=b.locals||a.weapon_offset!=b.weapon_offset||a.tier!=b.tier||a.authored!=b.authored||memcmp(&a.script,&b.script,sizeof a.script)||
       !guest_camera_stamp_core_equal(&a.pickup,&b.pickup)||!guest_camera_stamp_core_equal(&a.pawn,&b.pawn)||
       !guest_camera_stamp_core_equal(&a.pc,&b.pc)||!guest_camera_stamp_core_equal(&a.inventory,&b.inventory)||
       !guest_camera_stamp_core_equal(&a.gri,&b.gri)||!guest_camera_stamp_core_equal(&a.handler,&b.handler))return;
    if(runtime_ammo_award_local_select(1,a.frame,a.locals,a.weapon_offset,&a.script,a.gri.pointer)&&s->rows++<32){char row[512];
        snprintf(row,sizeof row,"{\"runtime_mp_ammo_award\":\"native-inventory-award-selected\",\"pickup_index\":%d,\"pawn_index\":%d,\"difficulty_ordinal\":%u,\"authored_percent\":%.9g,\"local_award_percent\":0.5,\"handler_written\":false,\"ammo_written\":false,\"gameplay_verified\":false,\"monotonic_ms\":%lu}\r\n",a.pickup.index,a.pawn.index,(unsigned)a.tier,(double)a.authored,GetTickCount());line(row);}
}
static void runtime_ammo_award_prepare(uintptr_t base,DWORD now) {
    RuntimeAmmoAward *s=&runtime_ammo_award;
    if(runtime_ammo_award_node||!runtime_ammo_award_enabled()||g_runtime_config.role!=ME3_ROLE_HOST||
       GetCurrentThreadId()!=session_game_thread||!runtime_control_enabled||!runtime_scope_ready||
       (s->checked&&now-s->checked<2000))return;s->checked=now;
    uintptr_t fn=function_named(base,"SFXDroppedAmmo","GiveAmmo");
    if(!supported_image(base)||!guest_player_context_method_stamp(base,fn,&s->give)||!runtime_ammo_award_contract(base,NULL))return;
    s->base=base;runtime_ammo_award_node=fn;
    line("{\"runtime_mp_ammo_award\":\"native-local-award-guard-prepared\",\"handler_written\":false,\"ammo_written\":false,\"gameplay_verified\":false}\r\n");
}
#ifdef ME3_RUNTIME_FIXTURE
static uintptr_t runtime_award_fixture_refuse;
static int runtime_award_fixture_pointer(uintptr_t at,uintptr_t expected,uintptr_t desired) {
    return at!=runtime_award_fixture_refuse&&runtime_power_action_context_pointer(at,expected,desired);
}
static unsigned runtime_award_fixture_calls;
static void *runtime_award_fixture_self,*runtime_award_fixture_frame,*runtime_award_fixture_result;
static void runtime_award_fixture_stock(void *self,void *frame,void *result) {
    if(self!=runtime_award_fixture_self||frame!=runtime_award_fixture_frame||result!=runtime_award_fixture_result||GetLastError()!=37)
        runtime_award_fixture_calls+=100;
    ++runtime_award_fixture_calls;SetLastError(53);
}
static int runtime_ammo_award_selftest(void) {
    DWORD saved_error=GetLastError();
    int ok=runtime_ammo_award_policy(0,0.75f)&&runtime_ammo_award_policy(1,0.6f)&&runtime_ammo_award_policy(2,0.4f)&&
        !runtime_ammo_award_policy(3,0.25f)&&!runtime_ammo_award_policy(4,0.25f)&&!runtime_ammo_award_policy(255,0.75f)&&
        !runtime_ammo_award_policy(0,0.5f)&&!runtime_ammo_award_policy(2,NAN);
    union {uintptr_t align;unsigned char bytes[64];} frame={0},locals={0};unsigned char code[RUNTIME_AMMO_AWARD_VM_SIZE]={0};
    GameArray script={(uintptr_t)code,RUNTIME_AMMO_AWARD_VM_SIZE,RUNTIME_AMMO_AWARD_VM_SIZE};
    uintptr_t f=(uintptr_t)frame.bytes,l=(uintptr_t)locals.bytes,start=script.data,gri=123,pawn=71;uint32_t pct=0;
    memcpy(locals.bytes,&pawn,8);memset(locals.bytes+28,0xcc,4);
    memcpy(frame.bytes+0x30,&l,8);memcpy(frame.bytes+0x28,&start,8);code[RUNTIME_AMMO_AWARD_ITERATOR_OFFSET]=0x2f;
    ok=ok&&!runtime_ammo_award_local_select(0,f,l,20,&script,gri)&&read_ptr(f+0x28)==start&&!read_ptr(l+8);
    ok=ok&&runtime_ammo_award_local_select(1,f,l,20,&script,gri)&&read_ptr(f+0x28)==start+177&&read_ptr(l+8)==gri;
    read_at(l+16,&pct,4);ok=ok&&pct==UINT32_C(0x3f000000)&&!read_ptr(l+20)&&!runtime_ammo_award_local_select(1,f,l,20,&script,gri);
    uint32_t canary=0;read_at(l+28,&canary,4);ok=ok&&canary==UINT32_C(0xcccccccc)&&read_ptr(l)==pawn;
    memset(locals.bytes,0,sizeof locals.bytes);memcpy(frame.bytes+0x28,&start,8);code[177]=0;
    ok=ok&&!runtime_ammo_award_local_select(1,f,l,20,&script,gri)&&!read_ptr(l+8)&&read_ptr(f+0x28)==start;code[177]=0x2f;
    pct=7;memcpy(locals.bytes+16,&pct,4);
    ok=ok&&!runtime_ammo_award_local_select(1,f,l,20,&script,gri)&&!read_ptr(l+8)&&read_ptr(f+0x28)==start;
    read_at(l+16,&pct,4);ok=ok&&pct==7;
    memset(locals.bytes,0,sizeof locals.bytes);
    ok=ok&&runtime_ammo_award_local_select(1,f,l,24,&script,gri)&&read_ptr(f+0x28)==start+177;
    memset(locals.bytes,0,sizeof locals.bytes);memcpy(frame.bytes+0x28,&start,8);
    ok=ok&&!runtime_ammo_award_local_select(1,f,l,16,&script,gri)&&!runtime_ammo_award_local_select(1,f,l,21,&script,gri)&&
        !runtime_ammo_award_local_select(1,f,l,60,&script,gri)&&!read_ptr(l+8);
    runtime_award_fixture_refuse=f+0x28;
    ok=ok&&!runtime_ammo_award_local_select_using(1,f,l,20,&script,gri,runtime_award_fixture_pointer)&&
        !read_ptr(l+8)&&read_ptr(f+0x28)==start;
    read_at(l+16,&pct,4);ok=ok&&!pct;runtime_award_fixture_refuse=0;
    RuntimeAmmoPickup old_pickup=runtime_ammo_pickup;uintptr_t old_node=runtime_ammo_award_node;Me3RuntimeRole old_role=g_runtime_config.role;
    runtime_ammo_pickup.installed=1;runtime_ammo_pickup.valid.function.pointer=779;runtime_ammo_award_node=777;
    runtime_ammo_pickup.original=runtime_award_fixture_stock;g_runtime_config.role=ME3_ROLE_GUEST;
    runtime_award_fixture_self=locals.bytes;runtime_award_fixture_frame=frame.bytes;runtime_award_fixture_result=code;
    for(uintptr_t node=777;node<=778;++node){memcpy(frame.bytes+0x18,&node,8);runtime_award_fixture_calls=0;SetLastError(37);
        runtime_ammo_pickup_exec(locals.bytes,frame.bytes,code);ok=ok&&runtime_award_fixture_calls==1&&GetLastError()==53;}
    runtime_ammo_pickup=old_pickup;runtime_ammo_award_node=old_node;g_runtime_config.role=old_role;SetLastError(saved_error);
    return ok;
}
#endif
#endif
