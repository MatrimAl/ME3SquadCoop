#ifndef ME3_RUNTIME_AMMO_SETTLE_H
#define ME3_RUNTIME_AMMO_SETTLE_H
#include "runtime_ammo_settle_protocol.h"
/* Settled host snapshots only. Native fire, prediction and receive storage are
 * unchanged. A received reply queues mod state; the normal game-thread Tick
 * alone may apply it to the same existing idle guest weapon. */
typedef struct RuntimeAmmoSettle {
    RuntimeGuestFireScope scope;
    GuestCameraStamp fields[2];
    AmmoReconcileState state;
    AmmoReconcileMessage reply;
    uintptr_t base,engine;
    uint32_t generation;
    unsigned sequences,rows;
    DWORD checked;
    int ready,reply_queued,prepared_reported;
} RuntimeAmmoSettle;
static RuntimeAmmoSettle runtime_ammo_settle;
static int runtime_ammo_settle_enabled(void) {
    static int enabled=-1;
    if(enabled<0){wchar_t text[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_SETTLED_AMMO",text,8);
        enabled=!n||(n==1&&text[0]==L'1');}
    return enabled;
}
static void runtime_ammo_settle_note(const char *event,uint64_t request,int used,int spare) {
    RuntimeAmmoSettle *s=&runtime_ammo_settle;
    if(s->rows++>=128)return;
    char row[520];snprintf(row,sizeof row,
        "{\"runtime_ammo_settle\":\"%s\",\"role\":%d,\"generation\":%u,\"request\":%llu,\"channel_index\":%d,\"used\":%d,\"spare\":%d,\"during_fire_correction\":false,\"per_shot_acknowledgement_verified\":false,\"gameplay_verified\":false,\"monotonic_ms\":%lu}\r\n",
        event,(int)g_runtime_config.role,s->generation,(unsigned long long)request,s->scope.weapon_channel.index,
        used,spare,GetTickCount());line(row);
}
static void runtime_ammo_settle_reset(RuntimeAmmoSettle *s) {
    s->ready=0;s->reply_queued=0;ammo_reconcile_state_bind(&s->state,0);
}
static int runtime_ammo_settle_live(RuntimeAmmoSettle *s) {
    RuntimeGuestFireScope current;
    return s->ready&&runtime_ammo_settle_enabled()&&GetCurrentThreadId()==session_game_thread&&
        runtime_guest_fire_installed&&runtime_guest_fire_base==s->base&&runtime_guest_fire_engine==s->engine&&
        runtime_guest_fire_dispatch_valid(s->base,s->scope.lease.weapon.pointer,1)&&
        runtime_guest_fire_scope(s->base,s->engine,&current)&&runtime_guest_fire_scope_same(&s->scope,&current)&&
        guest_camera_stamp_matches(s->base,&s->fields[0],0)&&guest_camera_stamp_matches(s->base,&s->fields[1],0);
}
static int runtime_ammo_settle_idle(RuntimeAmmoSettle *s,int *used,int *spare) {
    uintptr_t weapon=s->scope.lease.weapon.pointer,inventory=s->scope.lease.inventory.pointer,pending=1;
    char state[96];
    return runtime_ammo_settle_live(s)&&
        read_at(weapon+0x3b8,used,4)&&*used>=0&&*used<=1000000&&
        read_at(weapon+0x3b4,spare,4)&&*spare>=0&&*spare<=1000000&&
        read_at(inventory+0x288,&pending,8)&&!pending&&
        runtime_scene_weapon_item_idle(s->base,s->scope.lease.pc.pointer,s->scope.lease.pawn.pointer,inventory,weapon)&&
        native_state_name(s->base,weapon,state,sizeof state)&&!strcmp(state,"Active");
}
/* Always called before the exact selected native fire entry, independently
 * of diagnostic collection limits. Counter equality after an ABA is no proof
 * of inactivity. This writes mod request state only. */
static void runtime_ammo_settle_fire_activity(uintptr_t weapon) {
    RuntimeAmmoSettle *s=&runtime_ammo_settle;
    if(!s->ready||!weapon||GetCurrentThreadId()!=session_game_thread)return;
    /* The shared native wrapper already selected the current owned weapon.
     * Even a replacement cancels old mod state; cancellation never grants a
     * counter write and need not pay for network discovery on every bullet. */
    if(weapon!=s->scope.lease.weapon.pointer){runtime_ammo_settle_reset(s);return;}
    s->reply_queued=0;
    if(g_runtime_config.role==ME3_ROLE_GUEST)ammo_reconcile_guest_activity(&s->state.guest);
    else s->state.host.sampled=0;
}
static void runtime_ammo_settle_sample_activity(RuntimeAmmoSettle *s) {
    uintptr_t weapon=s->scope.lease.weapon.pointer,pawn=s->scope.lease.pawn.pointer;
    uintptr_t inventory=s->scope.lease.inventory.pointer,pending_weapon=1;
    unsigned char fire=255,current=255,pending=255;uint32_t busy=0;
    int used=-1,spare=-1,requests[32]={0};GameArray pending_fire={0};
    int idle=read_at(s->scope.lease.pc.pointer+0x480,&fire,1)&&!fire&&
        runtime_movement_idle(pawn)&&read_at(weapon+0x3b0,&current,1)&&!current&&
        read_at(weapon+0x10c2,&pending,1)&&!pending&&
        read_at(weapon+0x10b4,&busy,4)&&!(busy&0x1c000)&&
        read_at(inventory+0x288,&pending_weapon,8)&&!pending_weapon&&
        scene_readiness_array(inventory+0x270,&pending_fire,32)&&
        (!pending_fire.count||read_at(pending_fire.data,requests,(SIZE_T)pending_fire.count*4))&&
        read_at(weapon+0x3b8,&used,4)&&read_at(weapon+0x3b4,&spare,4);
    for(int i=0;idle&&i<pending_fire.count;++i)if(requests[i])idle=0;
    int changed=g_runtime_config.role==ME3_ROLE_GUEST?
        s->state.guest.have&&(used!=s->state.guest.used||spare!=s->state.guest.spare):
        s->state.host.sampled&&(used!=s->state.host.used||spare!=s->state.host.spare);
    if(!idle||changed){
        s->reply_queued=0;
        if(g_runtime_config.role==ME3_ROLE_GUEST)ammo_reconcile_guest_activity(&s->state.guest);
        else s->state.host.sampled=0;
    }
}
static int runtime_ammo_settle_source(RuntimeAmmoSettle *s,uintptr_t self,uintptr_t frame,int k) {
    NativeRpcReceiveFrame *r=native_rpc_receive_current(session_game_thread);
    NetworkActorIdentityEntry channel={0};NetworkActorScope world;
    if(!r||!r->channel||!presentation_method_valid(k)||!runtime_ammo_settle_live(s)||
       self!=s->scope.lease.pc.pointer||read_ptr(frame+0x18)!=presentation_methods[k].function.pointer||
       read_ptr(frame+0x20)!=self||!network_actor_read_scope(s->base,self,&world)||
       world.info!=s->scope.world.info||world.driver!=s->scope.world.driver||
       !network_actor_channel_valid(s->base,self,r->channel,world.driver,&channel)||
       channel.connection!=s->scope.lease.connection.pointer||read_ptr(channel.connection+0x68)!=self)return 0;
    if(g_runtime_config.role==ME3_ROLE_HOST)return k==0&&self==session_remote&&
        channel.connection==session_connection&&read_ptr(self+0x67c)==channel.connection;
    return g_runtime_config.role==ME3_ROLE_GUEST&&k==1&&local_controller(s->engine)==self&&
        read_ptr(world.driver+0x80)==channel.connection;
}
static int runtime_ammo_settle_receive(uintptr_t self,uintptr_t frame,const wchar_t *text,size_t count) {
    RuntimeAmmoSettle *s=&runtime_ammo_settle;AmmoReconcileMessage m={0};
    int k=g_runtime_config.role==ME3_ROLE_HOST?0:1;
    if(!runtime_ammo_settle_parse(text,count,&m)||!runtime_ammo_settle_source(s,self,frame,k)||
       m.channel!=(unsigned)s->scope.weapon_channel.index)return 1;
    if(!ammo_reconcile_state_bind(&s->state,s->generation))return 1;
    if(k==0){
        if(ammo_reconcile_host_request(&s->state.host,&m,GetTickCount())==1)
            runtime_ammo_settle_note("request-received",m.request,0,0);
    } else if(m.kind==L'A'&&s->state.guest.pending.active&&
              m.request==s->state.guest.pending.request&&m.channel==s->state.guest.pending.channel&&!s->reply_queued){
        s->reply=m;s->reply_queued=1;
        runtime_ammo_settle_note("host-reply-received",m.request,m.used,m.spare);
    }
    return 1;
}
static int runtime_ammo_settle_send(RuntimeAmmoSettle *s,int k,const AmmoReconcileMessage *message) {
    wchar_t text[96];int count=runtime_ammo_settle_format(message,text);
    return count&&runtime_ammo_settle_live(s)&&presentation_send_text(s->scope.lease.pc.pointer,k,text,count);
}
/* Two aligned native int cells. Refuse changed storage; if the second compare
 * loses, undo only our still-owned first cell. No partial foreign overwrite. */
static int runtime_ammo_settle_cells(uintptr_t at,const int expected[2],const int desired[2]) {
    if((at&3)||!background_focus_writable(at,8))return 0;
    int check[2];if(!read_at(at,check,8)||memcmp(check,expected,8))return 0;
    if(InterlockedCompareExchange((LONG volatile*)at,desired[0],expected[0])!=expected[0])return 0;
    if(InterlockedCompareExchange((LONG volatile*)(at+4),desired[1],expected[1])!=expected[1]){
        InterlockedCompareExchange((LONG volatile*)at,expected[0],desired[0]);return 0;
    }
    return read_at(at,check,8)&&!memcmp(check,desired,8);
}
static int runtime_ammo_settle_apply(RuntimeAmmoSettle *s,const AmmoReconcileMessage *reply,DWORD now) {
    int used=-1,spare=-1;GuestCameraStamp fields[2];
    if(g_runtime_config.role!=ME3_ROLE_GUEST||!runtime_ammo_settle_idle(s,&used,&spare)||
       !ammo_reconcile_response_allowed(&s->state.guest.pending,reply,s->generation,
           (unsigned)s->scope.weapon_channel.index,now,1,used,spare)||
       !native_ammo_wire_property(s->base,s->fields[0].pointer,0,&fields[0])||
       !native_ammo_wire_property(s->base,s->fields[1].pointer,1,&fields[1]))return 0;
    int expected[2]={spare,used},desired[2]={reply->spare,reply->used},check_used=-1,check_spare=-1;
    if(!runtime_ammo_settle_idle(s,&check_used,&check_spare)||check_used!=used||check_spare!=spare)return 0;
    int applied=runtime_ammo_settle_cells(s->scope.lease.weapon.pointer+0x3b4,expected,desired);
    int stable=applied&&runtime_ammo_settle_live(s)&&
        read_at(s->scope.lease.weapon.pointer+0x3b8,&check_used,4)&&check_used==reply->used&&
        read_at(s->scope.lease.weapon.pointer+0x3b4,&check_spare,4)&&check_spare==reply->spare;
    runtime_ammo_settle_note(stable?(used==reply->used&&spare==reply->spare?"already-matched":"settled-host-count-applied"):
        "settled-apply-unverified",reply->request,reply->used,reply->spare);
    return stable;
}
static void runtime_ammo_settle_tick(uintptr_t base,uintptr_t engine,DWORD now) {
    DWORD error=GetLastError();RuntimeAmmoSettle *s=&runtime_ammo_settle;
    presentation_ammo_receive=runtime_ammo_settle_receive;runtime_ammo_fire_activity=runtime_ammo_settle_fire_activity;
    if(GetCurrentThreadId()!=session_game_thread)goto done;
    if(!runtime_ammo_settle_enabled()||!runtime_control_enabled||!runtime_scope_ready||
       !InterlockedCompareExchange(&g_enable_requested,0,0)||session_engine_blocked(session_context(engine))){
        runtime_ammo_settle_reset(s);goto done;}
    /* Cheap activity sampling is per Tick. Complete network/metadata scope
     * validation, scheduling and writes run at most20Hz. */
    if(s->ready)runtime_ammo_settle_sample_activity(s);
    if(s->checked&&now-s->checked<50)goto done;s->checked=now;
    RuntimeGuestFireScope current;
    if(!runtime_guest_fire_scope(base,engine,&current)||!runtime_guest_fire_installed||
       !runtime_guest_fire_dispatch_valid(base,current.lease.weapon.pointer,1)){
        runtime_ammo_settle_reset(s);goto done;
    }
    if(!s->ready||s->base!=base||s->engine!=engine||!runtime_guest_fire_scope_same(&s->scope,&current)){
        runtime_ammo_settle_reset(s);
        if(s->generation==UINT32_MAX||!native_ammo_wire_properties(base,current.lease.weapon.pointer,s->fields))goto done;
        s->base=base;s->engine=engine;s->scope=current;++s->generation;s->ready=1;
        ammo_reconcile_state_bind(&s->state,s->generation);
    }
    if(!s->prepared_reported){s->prepared_reported=1;runtime_ammo_settle_note("owned-weapon-snapshot-ready",0,0,0);}
    /* No counter write or engine call while busy. */
    int used=-1,spare=-1,idle=runtime_ammo_settle_idle(s,&used,&spare);
    if(g_runtime_config.role==ME3_ROLE_HOST){
        AmmoReconcileMessage reply={0};
        if(ammo_reconcile_host_snapshot(&s->state.host,idle,used,spare,(unsigned)s->scope.weapon_channel.index,now,&reply)){
            int sent=runtime_ammo_settle_send(s,1,&reply);
            if(sent&&runtime_ammo_settle_live(s)){
                ammo_reconcile_host_sent(&s->state.host,&reply);runtime_ammo_settle_note("settled-host-reply",reply.request,reply.used,reply.spare);
            }
        }
        goto done;
    }
    AmmoReconcileMessage request={0};
    int send=ammo_reconcile_guest_poll_limit(&s->state.guest,s->generation,(unsigned)s->scope.weapon_channel.index,
        idle,used,spare,now,&s->sequences,UINT32_MAX,&request);
    if(!idle)s->reply_queued=0;
    if(s->reply_queued){
        AmmoReconcileMessage reply=s->reply;s->reply_queued=0;
        int allowed=ammo_reconcile_response_allowed(&s->state.guest.pending,&reply,s->generation,
            (unsigned)s->scope.weapon_channel.index,now,idle,used,spare);
        int applied=allowed&&runtime_ammo_settle_apply(s,&reply,now);
        if(allowed)ammo_reconcile_guest_complete(&s->state.guest,&reply,applied);
    }
    if(send){
        int sent=runtime_ammo_settle_send(s,0,&request);
        if(sent)runtime_ammo_settle_note(send==1?"settled-request-sent":"settled-request-retried",request.request,used,spare);
        else if(send==1)ammo_reconcile_cancel_pending(&s->state.guest.pending);
    }
 done:SetLastError(error);
}
#ifdef ME3_RUNTIME_FIXTURE
static int runtime_ammo_settle_fixture_calls,runtime_ammo_settle_fixture_bad;
static void *runtime_ammo_settle_fixture_weapon;
static void runtime_ammo_settle_fixture_stock(void *self) {
    ++runtime_ammo_settle_fixture_calls;
    if(self!=runtime_ammo_settle_fixture_weapon||runtime_ammo_settle.state.guest.pending.active||
       runtime_ammo_settle.state.guest.quiet_started||runtime_ammo_settle.reply_queued||
       !runtime_ammo_settle.state.guest.dirty||GetLastError()!=41)runtime_ammo_settle_fixture_bad=1;
    SetLastError(67);
}
static int runtime_ammo_settle_selftest(void) {
    DWORD error=GetLastError();_Alignas(4) int values[2]={210,5},expected[2]={210,5},desired[2]={210,4};
    int ok=runtime_ammo_settle_cells((uintptr_t)values,expected,desired)&&values[0]==210&&values[1]==4;
    ok=ok&&!runtime_ammo_settle_cells((uintptr_t)values,expected,(int[2]){99,0})&&values[0]==210&&values[1]==4&&
        !runtime_ammo_settle_cells((uintptr_t)values+1,desired,expected)&&
        runtime_ammo_settle_cells((uintptr_t)values,desired,desired);
    RuntimeAmmoSettle state={0};state.ready=state.reply_queued=1;state.generation=3;state.sequences=75;
    ammo_reconcile_state_bind(&state.state,3);state.state.guest.pending.active=1;state.state.host.pending=1;
    runtime_ammo_settle_reset(&state);
    ok=ok&&!state.ready&&!state.reply_queued&&!state.state.guest.pending.active&&!state.state.host.pending&&
        state.generation==3&&state.sequences==75&&runtime_ammo_settle_protocol_selftest();
    /* Exercise the real native wrapper after its diagnostic budget is used.
     * The operational pending reply must be cancelled BEFORE stock, even
     * though no native capture/metadata walk is admitted on this path. */
    RuntimeAmmoSettle saved=runtime_ammo_settle;RuntimeGuestWeapon weapon=runtime_guest_weapon;
    RuntimeGuestFireCall original=runtime_guest_fire_original;void (*activity)(uintptr_t)=runtime_ammo_fire_activity;
    int installed=runtime_guest_fire_installed,observing=runtime_ammo_observation_enabled;
    unsigned calls=runtime_guest_fire_calls;DWORD thread=session_game_thread;int role=g_runtime_config.role;
    runtime_ammo_settle=(RuntimeAmmoSettle){0};runtime_ammo_settle.ready=runtime_ammo_settle.reply_queued=1;
    runtime_ammo_settle.scope.lease.weapon.pointer=(uintptr_t)values;
    runtime_ammo_settle.state.guest.pending.active=runtime_ammo_settle.state.guest.quiet_started=1;
    runtime_guest_weapon.valid=1;runtime_guest_weapon.weapon.pointer=(uintptr_t)values;
    runtime_guest_fire_original=runtime_ammo_settle_fixture_stock;runtime_ammo_fire_activity=runtime_ammo_settle_fire_activity;
    runtime_guest_fire_installed=1;runtime_guest_fire_calls=64;runtime_ammo_observation_enabled=0;
    session_game_thread=GetCurrentThreadId();g_runtime_config.role=ME3_ROLE_GUEST;
    runtime_ammo_settle_fixture_calls=runtime_ammo_settle_fixture_bad=0;runtime_ammo_settle_fixture_weapon=values;
    SetLastError(41);runtime_guest_fire_audit_exec(values);
    ok=ok&&runtime_ammo_settle_fixture_calls==1&&!runtime_ammo_settle_fixture_bad&&GetLastError()==67;
    runtime_ammo_settle=saved;runtime_guest_weapon=weapon;runtime_guest_fire_original=original;runtime_ammo_fire_activity=activity;
    runtime_guest_fire_installed=installed;runtime_guest_fire_calls=calls;runtime_ammo_observation_enabled=observing;
    session_game_thread=thread;g_runtime_config.role=role;
    SetLastError(error);return ok;
}
#endif
#endif
