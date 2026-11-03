#ifndef ME3_RUNTIME_AMMO_RECEIVE_OBSERVER_H
#define ME3_RUNTIME_AMMO_RECEIVE_OBSERVER_H
#include "native_ammo_scope.h"
#include "native_ammo_wire_pins.h"
/* Observe the actual stock integer decode and subsequent native storage.
 * No packet edits, acknowledgement inference, counter repair or new transport. */
typedef int (*RuntimeAmmoSerialize)(void*,void*,void*,void*);
typedef struct RuntimeAmmoReceiveObserver {
    NativeAmmoScope scope;
    RuntimeAmmoSerialize original;
    RuntimeAmmoReceiveFrame *frame;
    uintptr_t base,engine;
    DWORD checked,thread;
    unsigned rows,open,id,generation;
    int installed,attempted;
} RuntimeAmmoReceiveObserver;
static RuntimeAmmoReceiveObserver runtime_ammo_receive_observer;
static int runtime_ammo_receive_serialize(void*,void*,void*,void*);
static int runtime_ammo_receive_enabled(void) {
    static int enabled=-1;if(enabled<0){wchar_t text[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_AMMO_RECEIVE_AUDIT",text,8);
        enabled=!n||(n==1&&text[0]==L'1');}return enabled;
}
static int runtime_ammo_receive_valid(void) {
    RuntimeAmmoReceiveObserver *s=&runtime_ammo_receive_observer;
    return s->installed&&runtime_ammo_receive_enabled()&&GetCurrentThreadId()==s->thread&&
        g_runtime_config.role==ME3_ROLE_GUEST&&runtime_control_enabled&&runtime_scope_ready&&
        InterlockedCompareExchange(&g_enable_requested,0,0)&&runtime_guest_weapon.valid&&
        s->scope.epoch==guest_player_context_registration_token()&&
        s->scope.weapon.pointer==runtime_guest_weapon.weapon.pointer&&
        read_ptr(s->base+0x108dfb0+0x2f8)==(uintptr_t)runtime_ammo_receive_serialize&&
        native_ammo_scope_valid(&s->scope,0);
}
static int runtime_ammo_receive_fire_coverage(void) {
    RuntimeAmmoReceiveObserver *s=&runtime_ammo_receive_observer;
    return runtime_ammo_observation_enabled&&runtime_guest_fire_installed&&runtime_guest_fire_base==s->base&&
        session_game_thread==s->thread&&runtime_guest_weapon.valid&&
        runtime_guest_weapon.weapon.pointer==s->scope.weapon.pointer&&
        runtime_guest_fire_dispatch_valid(s->base,s->scope.weapon.pointer,1);
}
static void runtime_ammo_receive_push(RuntimeAmmoReceiveFrame *f,uintptr_t channel,int owner) {
    RuntimeAmmoReceiveObserver *s=&runtime_ammo_receive_observer;memset(f,0,sizeof *f);
    if(!owner)return;f->previous=s->frame;f->channel=channel;f->active=1;
    if(s->frame&&s->frame->valid&&s->frame->nested_receives!=UINT_MAX)++s->frame->nested_receives;s->frame=f;
}
static void runtime_ammo_receive_begin(RuntimeAmmoReceiveFrame *f,uintptr_t channel) {
    RuntimeAmmoReceiveObserver *s=&runtime_ammo_receive_observer;
    runtime_ammo_receive_push(f,channel,s->thread&&GetCurrentThreadId()==s->thread);
    if(!f->active||s->rows+s->open>=128||channel!=s->scope.channel.channel||!runtime_ammo_receive_valid())return;
    NativeAmmoScope *q=&s->scope;
    if(!read_at(q->weapon.pointer+0x3b8,&f->before[0],4)||!read_at(q->weapon.pointer+0x3b4,&f->before[1],4))return;
    unsigned char fire=0;if(!read_at(q->pc.pointer+0x480,&fire,1))return;
    f->valid=1;f->generation=q->generation;f->id=++s->id;++s->open;
    f->weapon_index=q->weapon.index;f->channel_index=q->channel.index;f->fire_before=fire;
    f->fire_before_count=runtime_ammo_observation_fire_calls;f->begin_order=runtime_ammo_observation_next();
    f->fire_coverage=runtime_ammo_receive_fire_coverage();
}
static int runtime_ammo_receive_correction(const RuntimeAmmoReceiveFrame *f,int stable,int coverage,int after_used,uint64_t fire_after) {
    return stable&&coverage&&f->fire_coverage&&!f->nested_receives&&f->counts[0]==1&&f->accepted[0]&&f->before[0]!=after_used&&f->decoded[0]==after_used&&
        f->fire_before_count==fire_after;
}
static void runtime_ammo_receive_end(RuntimeAmmoReceiveFrame *f) {
    RuntimeAmmoReceiveObserver *s=&runtime_ammo_receive_observer;
    if(!f->active||GetCurrentThreadId()!=s->thread||s->frame!=f)return;
    int after[2]={-1,-1};unsigned char fire=0;
    int stable=f->valid&&f->generation==s->scope.generation&&runtime_ammo_receive_valid()&&
        read_at(s->scope.weapon.pointer+0x3b8,&after[0],4)&&read_at(s->scope.weapon.pointer+0x3b4,&after[1],4)&&
        read_at(s->scope.pc.pointer+0x480,&fire,1);
    uint64_t order=f->valid?runtime_ammo_observation_next():0,fire_after=runtime_ammo_observation_fire_calls;
    int coverage=stable&&runtime_ammo_receive_fire_coverage();
    /* One reserved completion row covers both fields. Duplicates remain
     * explicit ambiguity; they are never treated as a single applied value. */
    int fields=f->counts[0]||f->counts[1];
    int useful=s->rows<4||!stable||f->counts[0]>1||f->counts[1]>1||f->nested_receives||
        (f->counts[0]&&!f->accepted[0])||(f->counts[1]&&!f->accepted[1])||
        (stable&&(after[0]<f->before[0]||after[1]!=f->before[1]));
    if(f->valid&&fields&&useful&&s->rows<128){++s->rows;char row[2200];
        snprintf(row,sizeof row,"{\"runtime_ammo_receive\":\"native-receive-complete\",\"receive_id\":%u,\"scope_generation\":%u,\"weapon_index\":%d,\"channel_index\":%d,\"scope_stable\":%s,\"used_before\":%d,\"used_after\":%d,\"spare_before\":%d,\"spare_after\":%d,\"decoded_used\":%d,\"decoded_spare\":%d,\"used_decode_count\":%u,\"spare_decode_count\":%u,\"used_decode_accepted\":%s,\"spare_decode_accepted\":%s,\"used_archive_errors\":[%u,%u],\"spare_archive_errors\":[%u,%u],\"nested_receives\":%u,\"fire_counter_coverage\":%s,\"fire_input_before\":%d,\"fire_input_after\":%u,\"selected_fire_calls_before\":%llu,\"selected_fire_calls_after\":%llu,\"fire_calls_at_decode\":[%llu,%llu],\"native_event_begin\":%llu,\"native_event_decode\":[%llu,%llu],\"native_event_end\":%llu,\"native_decode_matches_counter_change\":%s,\"per_shot_acknowledgement_verified\":false,\"ammo_written\":false,\"payload_changed\":false,\"monotonic_ms\":%lu}\r\n",
            f->id,f->generation,f->weapon_index,f->channel_index,stable?"true":"false",f->before[0],after[0],f->before[1],after[1],
            f->decoded[0],f->decoded[1],f->counts[0],f->counts[1],f->accepted[0]?"true":"false",f->accepted[1]?"true":"false",
            f->errors_before[0],f->errors_after[0],f->errors_before[1],f->errors_after[1],f->nested_receives,(coverage&&f->fire_coverage)?"true":"false",f->fire_before,(unsigned)fire,
            (unsigned long long)f->fire_before_count,(unsigned long long)fire_after,(unsigned long long)f->fire_at_decode[0],
            (unsigned long long)f->fire_at_decode[1],(unsigned long long)f->begin_order,(unsigned long long)f->decode_order[0],
            (unsigned long long)f->decode_order[1],(unsigned long long)order,runtime_ammo_receive_correction(f,stable,coverage,after[0],fire_after)?"true":"false",GetTickCount());line(row);
    }
    if(f->valid&&s->open)--s->open;s->frame=f->previous;f->active=0;
}
static void runtime_ammo_receive_decode_begin(RuntimeAmmoReceiveFrame *f,int field,uint64_t order,uint64_t fire) {
    if(field<0||field>1)return;
    if(f->counts[field]!=UINT_MAX)++f->counts[field];f->accepted[field]=0;f->decoded[field]=0;
    f->errors_before[field]=f->errors_after[field]=UINT_MAX;f->decode_order[field]=order;f->fire_at_decode[field]=fire;
}
static void runtime_ammo_receive_decode_end(RuntimeAmmoReceiveFrame *f,int field,int value,int result,uint32_t before,uint32_t after,
        uint64_t order,uint64_t fire) {
    if(field<0||field>1)return;f->decoded[field]=value;
    f->accepted[field]=f->counts[field]==1&&result==1&&!before&&!after;
    f->errors_before[field]=before;f->errors_after[field]=after;f->decode_order[field]=order;f->fire_at_decode[field]=fire;
}
static int runtime_ammo_receive_serialize(void *property,void *archive,void *map,void *data) {
    RuntimeAmmoReceiveObserver *s=&runtime_ammo_receive_observer;DWORD entry=GetLastError();
    uintptr_t caller=(uintptr_t)__builtin_return_address(0);
    int field=(uintptr_t)property==s->scope.properties[0].pointer?0:(uintptr_t)property==s->scope.properties[1].pointer?1:-1;
    RuntimeAmmoReceiveFrame *f=s->frame;uint32_t before=1;
    int candidate=field>=0&&s->thread&&GetCurrentThreadId()==s->thread&&caller==s->base+0x488ed4&&
        f&&f->valid&&f->active&&f->channel==s->scope.channel.channel&&f->generation==s->scope.generation&&
        (uintptr_t)map==s->scope.map;
    if(candidate)runtime_ammo_receive_decode_begin(f,field,runtime_ammo_observation_next(),runtime_ammo_observation_fire_calls);
    int selected=candidate&&runtime_ammo_receive_valid()&&
        guest_camera_stamp_matches(s->base,&s->scope.properties[field],0)&&
        read_at((uintptr_t)archive+0x38,&before,4);
    SetLastError(entry);int result=s->original(property,archive,map,data);DWORD error=GetLastError();
    int value=0;uint32_t after=1;
    if(selected&&s->frame==f&&f->generation==s->scope.generation&&runtime_ammo_receive_valid()&&
       read_at((uintptr_t)data,&value,4)&&read_at((uintptr_t)archive+0x38,&after,4))
        runtime_ammo_receive_decode_end(f,field,value,result,before,after,runtime_ammo_observation_next(),runtime_ammo_observation_fire_calls);
    SetLastError(error);return result;
}
static void runtime_ammo_receive_prepare(uintptr_t base,uintptr_t engine,DWORD now) {
    RuntimeAmmoReceiveObserver *s=&runtime_ammo_receive_observer;
    if(!runtime_ammo_receive_enabled()||g_runtime_config.role!=ME3_ROLE_GUEST||GetCurrentThreadId()!=session_game_thread||
       !runtime_rpc_receive_installed||!runtime_control_enabled||!runtime_scope_ready||s->rows>=128)return;
    if(s->scope.ready&&runtime_ammo_receive_valid())return;
    if(s->checked&&now-s->checked<1000)return;s->checked=now;s->scope.ready=0;
    RuntimeGuestFireScope fire={0};if(!runtime_guest_fire_scope(base,engine,&fire))return;
    NativeAmmoScope next={0};next.base=base;next.epoch=fire.token;next.channel=fire.weapon_channel;
    next.map=read_ptr(next.channel.connection+0xf0);
    uintptr_t objects[]={fire.lease.pc.pointer,fire.lease.pawn.pointer,fire.lease.weapon.pointer,fire.lease.world.pointer,
        next.channel.driver,next.channel.connection,next.channel.channel,next.map};
    GuestCameraStamp *stamps[]={&next.pc,&next.pawn,&next.weapon,&next.world,&next.driver,&next.connection,&next.actor_channel,&next.package_map};
    for(unsigned i=0;i<8;++i)if(!guest_camera_stamp_read(base,objects[i],stamps[i],0))return;
    if(!native_ammo_wire_properties(base,next.weapon.pointer,next.properties))return;
    if(++s->generation==0)++s->generation;next.generation=s->generation;next.ready=1;
    if(!native_ammo_scope_valid(&next,0))return;
    s->scope=next;s->base=base;s->engine=engine;s->thread=session_game_thread;
    if(s->installed)return;if(s->attempted)return;s->attempted=1;
    uintptr_t slot=base+0x108dfb0+0x2f8,expected=base+0x146f40;DWORD protection=0;
    if(!supported_image(base)||!native_ammo_wire_pins(base)||read_ptr(slot)!=expected||
       !VirtualProtect((void*)slot,8,PAGE_READWRITE,&protection))return;
    s->original=(RuntimeAmmoSerialize)expected;
    PVOID previous=InterlockedCompareExchangePointer((PVOID volatile*)slot,(PVOID)runtime_ammo_receive_serialize,(PVOID)expected);
    DWORD ignored;VirtualProtect((void*)slot,8,protection,&ignored);
    if((uintptr_t)previous!=expected){s->original=NULL;return;}
    s->installed=1;runtime_ammo_observation_enabled=1;
    runtime_ammo_receive_begin_observer=runtime_ammo_receive_begin;runtime_ammo_receive_end_observer=runtime_ammo_receive_end;
    line("{\"runtime_ammo_receive\":\"installed\",\"completion_limit\":128,\"guest_decode_only\":true,\"ammo_written\":false,\"payload_changed\":false}\r\n");
}
#ifdef ME3_RUNTIME_FIXTURE
static void *runtime_ammo_receive_fixture_args[4];static unsigned runtime_ammo_receive_fixture_calls;static int runtime_ammo_receive_fixture_bad;
static int runtime_ammo_receive_fixture_stock(void *a,void *b,void *c,void *d) {
    if(a!=runtime_ammo_receive_fixture_args[0]||b!=runtime_ammo_receive_fixture_args[1]||
       c!=runtime_ammo_receive_fixture_args[2]||d!=runtime_ammo_receive_fixture_args[3]||GetLastError()!=37)runtime_ammo_receive_fixture_bad=1;
    ++runtime_ammo_receive_fixture_calls;*(int*)d=47;SetLastError(53);return 17;
}
static int runtime_ammo_receive_selftest(void) {
    DWORD saved_error=GetLastError();
    RuntimeAmmoReceiveObserver saved=runtime_ammo_receive_observer;memset(&runtime_ammo_receive_observer,0,sizeof runtime_ammo_receive_observer);
    RuntimeAmmoReceiveFrame outer={0},inner={0},foreign={0};int ok=1;
    runtime_ammo_receive_observer.thread=GetCurrentThreadId();
    runtime_ammo_receive_push(&outer,11,1);outer.valid=1;
    runtime_ammo_receive_push(&foreign,0,0);ok=ok&&runtime_ammo_receive_observer.frame==&outer&&!foreign.active;
    runtime_ammo_receive_push(&inner,0,1);ok=ok&&runtime_ammo_receive_observer.frame==&inner&&!inner.valid&&inner.previous==&outer&&outer.nested_receives==1;
    runtime_ammo_receive_end(&inner);ok=ok&&runtime_ammo_receive_observer.frame==&outer&&!inner.active;
    outer.valid=0;runtime_ammo_receive_end(&outer);ok=ok&&!runtime_ammo_receive_observer.frame;
    RuntimeAmmoReceiveFrame f={0};f.before[0]=9;f.fire_before_count=12;f.fire_coverage=1;
    runtime_ammo_receive_decode_begin(&f,0,20,12);runtime_ammo_receive_decode_end(&f,0,7,1,0,0,21,12);
    ok=ok&&runtime_ammo_receive_correction(&f,1,1,7,12)&&!runtime_ammo_receive_correction(&f,0,1,7,12)&&
        !runtime_ammo_receive_correction(&f,1,1,8,12)&&!runtime_ammo_receive_correction(&f,1,1,7,13)&&
        !runtime_ammo_receive_correction(&f,1,0,7,12);
    f.fire_coverage=0;ok=ok&&!runtime_ammo_receive_correction(&f,1,1,7,12);f.fire_coverage=1;
    f.nested_receives=1;ok=ok&&!runtime_ammo_receive_correction(&f,1,1,7,12);f.nested_receives=0;
    runtime_ammo_receive_decode_begin(&f,0,22,12);runtime_ammo_receive_decode_end(&f,0,7,1,0,0,23,12);
    ok=ok&&f.counts[0]==2&&!f.accepted[0]&&!runtime_ammo_receive_correction(&f,1,1,7,12);
    memset(&f,0,sizeof f);runtime_ammo_receive_decode_begin(&f,0,24,12);
    ok=ok&&!f.accepted[0]&&f.errors_after[0]==UINT_MAX;
    runtime_ammo_receive_decode_begin(&f,0,25,12);runtime_ammo_receive_decode_end(&f,0,7,1,0,0,26,12);
    ok=ok&&f.counts[0]==2&&!f.accepted[0];
    memset(&f,0,sizeof f);runtime_ammo_receive_decode_begin(&f,1,27,12);runtime_ammo_receive_decode_end(&f,1,30,1,0,1,28,12);ok=ok&&!f.accepted[1];
    runtime_ammo_receive_decode_begin(&f,-1,0,0);ok=ok&&!f.counts[0]&&f.counts[1]==1;
    int a=0,b=0,c=0,data=0;
    runtime_ammo_receive_fixture_args[0]=&a;runtime_ammo_receive_fixture_args[1]=&b;
    runtime_ammo_receive_fixture_args[2]=&c;runtime_ammo_receive_fixture_args[3]=&data;
    runtime_ammo_receive_observer.original=runtime_ammo_receive_fixture_stock;
    runtime_ammo_receive_fixture_calls=0;runtime_ammo_receive_fixture_bad=0;SetLastError(37);
    int result=runtime_ammo_receive_serialize(&a,&b,&c,&data);
    ok=ok&&result==17&&data==47&&GetLastError()==53&&runtime_ammo_receive_fixture_calls==1&&!runtime_ammo_receive_fixture_bad;
    runtime_ammo_receive_observer.scope.properties[0].pointer=(uintptr_t)&a;f.valid=f.active=1;
    runtime_ammo_receive_observer.frame=&f;runtime_ammo_receive_fixture_calls=0;SetLastError(37);
    result=runtime_ammo_receive_serialize(&a,&b,&c,&data);
    ok=ok&&result==17&&GetLastError()==53&&runtime_ammo_receive_fixture_calls==1&&!runtime_ammo_receive_fixture_bad&&!f.counts[0];
    runtime_ammo_receive_observer=saved;SetLastError(saved_error);return ok;
}
#endif
#endif
