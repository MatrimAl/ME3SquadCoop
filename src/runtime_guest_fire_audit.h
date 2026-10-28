#ifndef ME3_RUNTIME_GUEST_FIRE_AUDIT_H
#define ME3_RUNTIME_GUEST_FIRE_AUDIT_H
#include "native_fire_pins.h"
/* Bounded ordinary-entry observation, never an ammunition repair. The exact
 * self-only native virtual slot is forwarded once. Script/native callers that
 * bypass this slot remain outside coverage. Missing rows establish neither
 * refusal nor absence of firing.
 * Include after the ordinary owned-weapon and native receive helpers. */
typedef void (*RuntimeGuestFireCall)(void*);
typedef struct RuntimeGuestFireScope {
    RuntimeGuestWeapon lease;
    NetworkActorScope world;
    NetworkActorIdentityEntry pawn_channel,weapon_channel;
    unsigned long token;
    int side;
} RuntimeGuestFireScope;
typedef struct RuntimeGuestFireSample {
    int valid,state_known,used,spare,shots;
    unsigned char fire,current,pending,mode,weapon_role,pawn_role;
    float chance,per_shot;
    char state[96];
    DWORD tick;
} RuntimeGuestFireSample;
typedef struct RuntimeGuestFireCapture {
    RuntimeGuestFireScope scope;
    RuntimeGuestFireSample before;
    uintptr_t receive_frame,receive_channel;
    int valid,receive_index,receive_selected;
    unsigned id,generation;
} RuntimeGuestFireCapture;
static RuntimeGuestFireCall runtime_guest_fire_original;
static GuestPlayerContextMethod runtime_guest_fire_method;
static RuntimeGuestFireScope runtime_guest_fire_last_scope;
static uintptr_t runtime_guest_fire_base,runtime_guest_fire_engine;
static int runtime_guest_fire_installed,runtime_guest_fire_attempted;
static unsigned runtime_guest_fire_calls,runtime_guest_fire_generation;
static int runtime_guest_fire_enabled(void) {
    static int enabled=-1;
    if(enabled<0){wchar_t value[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_FIRE_AUDIT",value,8);
        enabled=!n||(n==1&&value[0]==L'1');}
    return enabled;
}
static int runtime_guest_fire_selected(uintptr_t self,uintptr_t weapon,int valid,DWORD thread,DWORD owner,
        unsigned calls,int installed) {
    return installed&&valid&&self&&self==weapon&&owner&&thread==owner&&calls<64;
}
static int runtime_guest_fire_handle_same(SessionHandle a,SessionHandle b) {
    return a.pointer&&a.pointer==b.pointer&&a.index==b.index&&a.name==b.name;
}
static int runtime_guest_fire_scope_same(const RuntimeGuestFireScope *a,const RuntimeGuestFireScope *b) {
    const RuntimeGuestWeapon *x=&a->lease,*y=&b->lease;
    return a->token&&a->token==b->token&&a->side==b->side&&
        runtime_guest_fire_handle_same(x->pc,y->pc)&&runtime_guest_fire_handle_same(x->pawn,y->pawn)&&
        runtime_guest_fire_handle_same(x->weapon,y->weapon)&&runtime_guest_fire_handle_same(x->inventory,y->inventory)&&
        runtime_guest_fire_handle_same(x->world,y->world)&&runtime_guest_fire_handle_same(x->root,y->root)&&
        runtime_guest_fire_handle_same(x->klass,y->klass)&&runtime_guest_fire_handle_same(x->cdo,y->cdo)&&
        runtime_guest_fire_handle_same(x->damage_field,y->damage_field)&&runtime_guest_fire_handle_same(x->mode_field,y->mode_field)&&
        runtime_guest_fire_handle_same(x->pawn_channel,y->pawn_channel)&&runtime_guest_fire_handle_same(x->weapon_channel,y->weapon_channel)&&
        runtime_guest_fire_handle_same(x->connection,y->connection)&&runtime_guest_fire_handle_same(x->driver,y->driver)&&
        a->world.world==b->world.world&&a->world.level==b->world.level&&
        a->world.info==b->world.info&&a->world.driver==b->world.driver&&a->world.mode==b->world.mode&&
        !memcmp(&a->pawn_channel,&b->pawn_channel,sizeof a->pawn_channel)&&
        !memcmp(&a->weapon_channel,&b->weapon_channel,sizeof a->weapon_channel);
}
static int runtime_guest_fire_scope(uintptr_t base,uintptr_t engine,RuntimeGuestFireScope *out) {
    memset(out,0,sizeof *out);RuntimeGuestWeapon *q=&runtime_guest_weapon;SessionContext c;
    if(GetCurrentThreadId()!=session_game_thread||!runtime_guest_fire_enabled()||!q->valid||
       g_config_status!=ME3_CONFIG_READY||!runtime_control_enabled||!runtime_scope_ready||
       !InterlockedCompareExchange(&g_enable_requested,0,0)||
       !runtime_guest_weapon_context(base,engine,&c,&out->world,&out->pawn_channel)||
       !runtime_guest_weapon_handles(q)||q->pc.pointer!=c.pc||q->pawn.pointer!=c.pawn||q->world.pointer!=c.world||
       q->root.pointer!=out->world.world||q->driver.pointer!=out->world.driver||
       q->connection.pointer!=out->pawn_channel.connection||q->pawn_channel.pointer!=out->pawn_channel.channel||
       !runtime_guest_weapon_chain(base,c.pc,c.pawn,q->weapon.pointer,q->inventory.pointer,c.world,1)||
       read_ptr(q->weapon.pointer+0x50)!=q->klass.pointer||read_ptr(q->klass.pointer+0x13c)!=q->cdo.pointer||
       !network_actor_channel_valid(base,q->weapon.pointer,q->weapon_channel.pointer,out->world.driver,&out->weapon_channel)||
       out->weapon_channel.connection!=q->connection.pointer)return 0;
    if(g_runtime_config.role==ME3_ROLE_HOST) {
        if(session_pending_reason||session_travel_waiting||!session_epoch||
           read_ptr(q->pc.pointer+0x67c)!=q->connection.pointer||read_ptr(q->connection.pointer+0x68)!=q->pc.pointer)return 0;
        out->token=session_epoch;
    } else if(g_runtime_config.role==ME3_ROLE_GUEST)out->token=guest_player_context_registration_token();
    else return 0;
    if(!out->token)return 0;
    out->lease=*q;out->side=(int)g_runtime_config.role;return 1;
}
static void runtime_guest_fire_audit_exec(void *self);
static int runtime_guest_fire_dispatch_valid(uintptr_t base,uintptr_t weapon,int installed) {
    return read_ptr(weapon)==base+0x14fa430&&
        read_ptr(base+0x14fa430+0x7d8)==(installed?(uintptr_t)runtime_guest_fire_audit_exec:base+0xc62120);
}
static RuntimeGuestFireSample runtime_guest_fire_sample(const RuntimeGuestFireScope *s) {
    RuntimeGuestFireSample a={0};a.used=a.spare=a.shots=-1;
    uintptr_t weapon=s->lease.weapon.pointer,pawn=s->lease.pawn.pointer,pc=s->lease.pc.pointer;
    a.tick=GetTickCount();
    a.valid=read_at(weapon+0x3b8,&a.used,4)&&read_at(weapon+0x3b4,&a.spare,4)&&
        read_at(pawn+0x564,&a.shots,4)&&read_at(pc+0x480,&a.fire,1)&&
        read_at(weapon+0x3b0,&a.current,1)&&read_at(weapon+0x10c2,&a.pending,1)&&read_at(weapon+0x10be,&a.mode,1)&&
        read_at(weapon+0x26a,&a.weapon_role,1)&&read_at(pawn+0x26a,&a.pawn_role,1)&&
        read_at(weapon+0x59c,&a.chance,4)&&read_at(weapon+0xfa4,&a.per_shot,4)&&
        isfinite(a.chance)&&a.chance>=0&&a.chance<=10000&&isfinite(a.per_shot)&&a.per_shot>=0&&a.per_shot<=10000;
    a.state_known=a.valid&&native_state_name(runtime_guest_fire_base,weapon,a.state,sizeof a.state);
    return a;
}
static RuntimeGuestFireCapture runtime_guest_fire_capture(void *self) {
    RuntimeGuestFireCapture c={0};RuntimeGuestWeapon *q=&runtime_guest_weapon;
    /* Every unrelated NPC/weapon and every exhausted capture takes this cheap
     * pointer/thread/budget exit before object, class or network discovery. */
    if(!runtime_guest_fire_selected((uintptr_t)self,q->weapon.pointer,q->valid,GetCurrentThreadId(),
            session_game_thread,runtime_guest_fire_calls,runtime_guest_fire_installed)||
       !runtime_guest_fire_scope(runtime_guest_fire_base,runtime_guest_fire_engine,&c.scope)||
       !guest_player_context_method_matches(runtime_guest_fire_base,&runtime_guest_fire_method,0)||
       !runtime_guest_fire_dispatch_valid(runtime_guest_fire_base,(uintptr_t)self,1))return c;
    if(!runtime_guest_fire_scope_same(&c.scope,&runtime_guest_fire_last_scope)) {
        runtime_guest_fire_last_scope=c.scope;
        if(++runtime_guest_fire_generation==0)++runtime_guest_fire_generation;
    }
    c.id=++runtime_guest_fire_calls;c.generation=runtime_guest_fire_generation;c.receive_index=-1;
    NativeRpcReceiveFrame *r=native_rpc_receive_current(session_game_thread);
    if(r) {
        c.receive_frame=(uintptr_t)r;c.receive_channel=r->channel;
        NetworkActorIdentityEntry channel={0};
        if(r->channel&&network_actor_channel_valid(runtime_guest_fire_base,read_ptr(r->channel+0xa0),r->channel,
                c.scope.world.driver,&channel)&&channel.connection==c.scope.lease.connection.pointer) {
            c.receive_index=channel.index;
            c.receive_selected=r->channel==c.scope.weapon_channel.channel;
        }
    }
    c.before=runtime_guest_fire_sample(&c.scope);c.valid=1;return c;
}
static void runtime_guest_fire_after(RuntimeGuestFireCapture *c) {
    if(!c->valid)return;
    RuntimeGuestFireScope current;RuntimeGuestFireSample after={0};after.used=after.spare=after.shots=-1;
    int stable=runtime_guest_fire_scope(runtime_guest_fire_base,runtime_guest_fire_engine,&current)&&
        runtime_guest_fire_scope_same(&c->scope,&current)&&
        guest_player_context_method_matches(runtime_guest_fire_base,&runtime_guest_fire_method,0)&&
        runtime_guest_fire_dispatch_valid(runtime_guest_fire_base,c->scope.lease.weapon.pointer,1);
    if(stable)after=runtime_guest_fire_sample(&current);
    NativeRpcReceiveFrame *r=native_rpc_receive_current(session_game_thread);
    int same_receive=c->receive_frame&&r&&(uintptr_t)r==c->receive_frame&&r->channel==c->receive_channel;
    RuntimeGuestFireSample *b=&c->before;char before_state[200],after_state[200],row[2600];
    identity_quote(b->state,before_state,sizeof before_state);identity_quote(after.state,after_state,sizeof after_state);
    snprintf(row,sizeof row,"{\"runtime_guest_fire_audit\":\"native-call\",\"side\":\"%s\",\"call\":%u,\"scope_generation\":%u,\"registration_token\":%lu,\"weapon_index\":%d,\"pawn_index\":%d,\"channel_index\":%d,\"scope_stable\":%s,\"before_valid\":%s,\"after_valid\":%s,\"used_before\":%d,\"used_after\":%d,\"spare_before\":%d,\"spare_after\":%d,\"shot_count_before\":%d,\"shot_count_after\":%d,\"fire_input_before\":%u,\"fire_input_after\":%u,\"current_mode_before\":%u,\"current_mode_after\":%u,\"pending_mode_before\":%u,\"pending_mode_after\":%u,\"default_mode_before\":%u,\"default_mode_after\":%u,\"weapon_role\":%u,\"pawn_role\":%u,\"no_ammo_use_chance_value_before\":%.9g,\"no_ammo_use_chance_value_after\":%.9g,\"ammo_per_shot_before\":%.9g,\"ammo_per_shot_after\":%.9g,\"state_before_known\":%s,\"state_after_known\":%s,\"state_before\":\"%s\",\"state_after\":\"%s\",\"native_receive_frame\":\"0x%llx\",\"native_receive_channel_index\":%d,\"selected_weapon_receive_context\":%s,\"same_receive_after\":%s,\"receive_context_only\":true,\"original_forwarded_once\":true,\"ammo_written\":false,\"damage_verified\":false,\"before_monotonic_ms\":%lu,\"after_monotonic_ms\":%lu}\r\n",
        c->scope.side==ME3_ROLE_HOST?"host":"guest",c->id,c->generation,c->scope.token,
        c->scope.lease.weapon.index,c->scope.lease.pawn.index,c->scope.weapon_channel.index,
        stable?"true":"false",b->valid?"true":"false",after.valid?"true":"false",
        b->used,after.used,b->spare,after.spare,b->shots,after.shots,(unsigned)b->fire,(unsigned)after.fire,
        (unsigned)b->current,(unsigned)after.current,(unsigned)b->pending,(unsigned)after.pending,
        (unsigned)b->mode,(unsigned)after.mode,(unsigned)b->weapon_role,(unsigned)b->pawn_role,
        b->valid?(double)b->chance:0,after.valid?(double)after.chance:0,b->valid?(double)b->per_shot:0,after.valid?(double)after.per_shot:0,
        b->state_known?"true":"false",after.state_known?"true":"false",before_state,after_state,
        (unsigned long long)c->receive_frame,c->receive_index,c->receive_selected?"true":"false",same_receive?"true":"false",b->tick,after.tick);
    line(row);
}
static void runtime_guest_fire_forward(void *self,RuntimeGuestFireCall original,
        RuntimeGuestFireCapture (*capture)(void*),void (*after)(RuntimeGuestFireCapture*)) {
    DWORD incoming=GetLastError();RuntimeGuestFireCapture c=capture(self);
    SetLastError(incoming);original(self);DWORD outgoing=GetLastError();
    after(&c);SetLastError(outgoing);
}
static void runtime_guest_fire_audit_exec(void *self) {
    DWORD incoming=GetLastError();RuntimeGuestWeapon *q=&runtime_guest_weapon;
    if(!runtime_guest_fire_selected((uintptr_t)self,q->weapon.pointer,q->valid,GetCurrentThreadId(),
            session_game_thread,runtime_guest_fire_calls,runtime_guest_fire_installed)) {
        SetLastError(incoming);runtime_guest_fire_original(self);return;
    }
    SetLastError(incoming);
    runtime_guest_fire_forward(self,runtime_guest_fire_original,runtime_guest_fire_capture,runtime_guest_fire_after);
}
static void runtime_guest_fire_audit_prepare(uintptr_t base,uintptr_t engine,DWORD now) {
    (void)now;
    if(runtime_guest_fire_installed||runtime_guest_fire_attempted||!runtime_guest_fire_enabled()||
       GetCurrentThreadId()!=session_game_thread||!runtime_guest_weapon.valid)return;
    RuntimeGuestFireScope scope;
    if(!runtime_guest_fire_scope(base,engine,&scope))return;
    runtime_guest_fire_attempted=1;uintptr_t weapon=scope.lease.weapon.pointer;
    uintptr_t fn=guest_player_context_find_method(base,scope.lease.klass.pointer,"SFXWeapon_NativeBase","FireAmmunition");
    if(!supported_image(base)||!native_fire_pins(base)||!runtime_guest_fire_dispatch_valid(base,weapon,0)||
       !guest_player_context_method_stamp(base,fn,&runtime_guest_fire_method)||
       strcmp(runtime_guest_fire_method.function.full_name,"SFXGame.SFXWeapon_NativeBase.FireAmmunition")||
       !guest_player_context_function_abi(base,fn,"SFXWeapon_NativeBase","FireAmmunition",0x20500,0,65535,0,0x4b5ec0,0))goto rejected;
    uintptr_t slot=base+0x14fa430+0x7d8;DWORD protection=0;
    runtime_guest_fire_base=base;runtime_guest_fire_engine=engine;
    runtime_guest_fire_original=(RuntimeGuestFireCall)(base+0xc62120);
    if(!VirtualProtect((void*)slot,8,PAGE_READWRITE,&protection))goto rejected;
    PVOID previous=InterlockedCompareExchangePointer((PVOID volatile*)slot,(PVOID)runtime_guest_fire_audit_exec,
        (PVOID)runtime_guest_fire_original);
    DWORD ignored;VirtualProtect((void*)slot,8,protection,&ignored);
    if(previous!=(PVOID)runtime_guest_fire_original)goto rejected;
    runtime_guest_fire_installed=1;
    line("{\"runtime_guest_fire_audit\":\"installed\",\"call_limit\":64,\"combined_rows\":true,\"virtual_dispatch_only\":true,\"ammo_written\":false,\"damage_verified\":false}\r\n");return;
rejected:
    line("{\"runtime_guest_fire_audit\":\"not-installed\",\"reason\":\"exact-native-slot-or-method\"}\r\n");
}
#ifdef ME3_RUNTIME_FIXTURE
static void *runtime_guest_fire_fixture_self;
static unsigned runtime_guest_fire_fixture_calls,runtime_guest_fire_fixture_order;
static int runtime_guest_fire_fixture_bad,runtime_guest_fire_fixture_valid;
static RuntimeGuestFireCapture runtime_guest_fire_fixture_capture(void *self) {
    RuntimeGuestFireCapture c={0};c.valid=runtime_guest_fire_fixture_valid;
    if(self!=runtime_guest_fire_fixture_self||runtime_guest_fire_fixture_order++)runtime_guest_fire_fixture_bad=1;
    SetLastError(17);return c;
}
static void runtime_guest_fire_fixture_stock(void *self) {
    if(self!=runtime_guest_fire_fixture_self||GetLastError()!=41||runtime_guest_fire_fixture_order++!=1)
        runtime_guest_fire_fixture_bad=1;
    ++runtime_guest_fire_fixture_calls;SetLastError(67);
}
static void runtime_guest_fire_fixture_after(RuntimeGuestFireCapture *c) {
    if(c->valid!=runtime_guest_fire_fixture_valid||GetLastError()!=67||runtime_guest_fire_fixture_order++!=2)
        runtime_guest_fire_fixture_bad=1;
    SetLastError(89);
}
static void runtime_guest_fire_fixture_bypass(void *self) {
    if(self!=runtime_guest_fire_fixture_self||GetLastError()!=41)runtime_guest_fire_fixture_bad=1;
    ++runtime_guest_fire_fixture_calls;SetLastError(67);
}
static int runtime_guest_fire_audit_selftest(void) {
    DWORD error=GetLastError();int object,ok=1;runtime_guest_fire_fixture_self=&object;runtime_guest_fire_fixture_bad=0;
    for(int valid=0;valid<2;++valid) {
        runtime_guest_fire_fixture_valid=valid;runtime_guest_fire_fixture_calls=runtime_guest_fire_fixture_order=0;SetLastError(41);
        runtime_guest_fire_forward(&object,runtime_guest_fire_fixture_stock,runtime_guest_fire_fixture_capture,runtime_guest_fire_fixture_after);
        ok=ok&&runtime_guest_fire_fixture_calls==1&&runtime_guest_fire_fixture_order==3&&
            !runtime_guest_fire_fixture_bad&&GetLastError()==67;
    }
    RuntimeGuestFireCall original=runtime_guest_fire_original;int installed=runtime_guest_fire_installed;
    runtime_guest_fire_original=runtime_guest_fire_fixture_bypass;runtime_guest_fire_installed=0;
    runtime_guest_fire_fixture_calls=0;SetLastError(41);runtime_guest_fire_audit_exec(&object);
    ok=ok&&runtime_guest_fire_fixture_calls==1&&!runtime_guest_fire_fixture_bad&&GetLastError()==67;
    runtime_guest_fire_original=original;runtime_guest_fire_installed=installed;
    ok=ok&&runtime_guest_fire_selected(1,1,1,2,2,63,1)&&
        !runtime_guest_fire_selected(0,0,1,2,2,0,1)&&!runtime_guest_fire_selected(3,1,1,2,2,0,1)&&
        !runtime_guest_fire_selected(1,1,0,2,2,0,1)&&!runtime_guest_fire_selected(1,1,1,3,2,0,1)&&
        !runtime_guest_fire_selected(1,1,1,2,0,0,1)&&!runtime_guest_fire_selected(1,1,1,2,2,64,1)&&
        !runtime_guest_fire_selected(1,1,1,2,2,0,0);
    RuntimeGuestFireScope a={0},b={0};SessionHandle handle={1,2,3};
    a.token=5;a.side=ME3_ROLE_HOST;
    a.lease.pc=a.lease.pawn=a.lease.weapon=a.lease.inventory=a.lease.world=a.lease.root=a.lease.klass=a.lease.cdo=
        a.lease.damage_field=a.lease.mode_field=a.lease.pawn_channel=a.lease.weapon_channel=a.lease.connection=a.lease.driver=handle;
    b=a;ok=ok&&runtime_guest_fire_scope_same(&a,&b);b.token++;ok=ok&&!runtime_guest_fire_scope_same(&a,&b);
    b=a;b.lease.weapon.index++;ok=ok&&!runtime_guest_fire_scope_same(&a,&b);
    b=a;b.lease.pc.name++;ok=ok&&!runtime_guest_fire_scope_same(&a,&b);
    b=a;b.lease.connection.pointer++;ok=ok&&!runtime_guest_fire_scope_same(&a,&b);
    b=a;b.weapon_channel.index++;ok=ok&&!runtime_guest_fire_scope_same(&a,&b);
    b=a;b.world.info++;ok=ok&&!runtime_guest_fire_scope_same(&a,&b);
    b=a;b.side=ME3_ROLE_GUEST;ok=ok&&!runtime_guest_fire_scope_same(&a,&b);
    unsigned char *image=VirtualAlloc(NULL,0x1500000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!image)ok=0;
    else {
        uintptr_t base=(uintptr_t)image,weapon=base+0x1000;*(uintptr_t*)weapon=base+0x14fa430;
        memcpy(image+0x4b5ec0,native_fire_bridge_pin,sizeof native_fire_bridge_pin);
        memcpy(image+0xc62120,native_fire_body_pin,sizeof native_fire_body_pin);
        *(uintptr_t*)(base+0x14fa430+0x7d8)=base+0xc62120;
        ok=ok&&native_fire_pins(base)&&runtime_guest_fire_dispatch_valid(base,weapon,0)&&!runtime_guest_fire_dispatch_valid(base,weapon,1);
        image[0x4b5ec0]^=1;ok=ok&&!native_fire_pins(base);image[0x4b5ec0]^=1;
        image[0xc62120]^=1;ok=ok&&!native_fire_pins(base);image[0xc62120]^=1;
        *(uintptr_t*)(base+0x14fa430+0x7d8)+=1;ok=ok&&!runtime_guest_fire_dispatch_valid(base,weapon,0);
        *(uintptr_t*)(base+0x14fa430+0x7d8)=(uintptr_t)runtime_guest_fire_audit_exec;
        ok=ok&&runtime_guest_fire_dispatch_valid(base,weapon,1);
        *(uintptr_t*)weapon+=8;ok=ok&&!runtime_guest_fire_dispatch_valid(base,weapon,1);
        VirtualFree(image,0,MEM_RELEASE);
    }
    SetLastError(error);return ok;
}
#endif
#endif
