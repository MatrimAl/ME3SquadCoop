#ifndef ME3_GUEST_HIT_TRACE_H
#define ME3_GUEST_HIT_TRACE_H
/* Default-off protected host observer for the drained native instant-hit
 * dispatch, NOT the enqueue routine. Exact SFXWeapon vtable14fa430+230 is
 * Actor.ProcessEvent40d450; drain callb1f6e1 returns tob1f6e3 with R9=NULL.
 * Only current remote-owned Ashley's selected weapon/channel is observed.
 * Every call still reaches the original Actor PE, including its engine gates.
 * Bracketed health changes are scoped observations, never a combat verdict. */
#include "guest_hit_trace_core.h"
static ProcessEvent guest_hit_original;
static GuestPlayerContextMethod guest_hit_method;
static GuestHitObserver guest_hit_observer;
static uintptr_t guest_hit_base,guest_hit_registered;
static int guest_hit_enabled=-1,guest_hit_attempted,guest_hit_installed;
static DWORD guest_hit_discovery_at;
static int guest_hit_waiting_name;
static const unsigned char guest_hit_actor_pe_pin[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57,0x48,0x83,0xec,0x20,0x48,0x8b,0xd9,0x49,0x8b};
static const unsigned char guest_hit_slot_load_pin[]={0x48,0x8b,0x98,0x30,0x02,0x00,0x00};
static const unsigned char guest_hit_call_pin[]={0xe8,0x90,0x4d,0x60,0xff,0x45,0x33,0xc9,0x4c,0x8d,0x84,0x24,0x80,0x00,0x00,0x00,0x48,0x8b,0xd0,0x49,0x8b,0xce,0xff,0xd3};
static int guest_hit_bytes(uintptr_t address,const unsigned char *want,size_t size) {
    unsigned char code[48];return size<=sizeof code&&read_at(address,code,size)&&!memcmp(code,want,size);
}
static int guest_hit_native_pins(uintptr_t base) {
    return guest_hit_bytes(base+0x40d450,guest_hit_actor_pe_pin,sizeof guest_hit_actor_pe_pin)&&
        guest_hit_bytes(base+0xb1f654,guest_hit_slot_load_pin,sizeof guest_hit_slot_load_pin)&&
        guest_hit_bytes(base+0xb1f6cb,guest_hit_call_pin,sizeof guest_hit_call_pin);
}
static int guest_hit_opted(void) {
    if(guest_hit_enabled<0){wchar_t value[8]={0};guest_hit_enabled=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_GUEST_HIT_TRACE",value,8)==1&&value[0]==L'1';}
    return guest_hit_enabled&&test_action==6&&ammo_wire_opted_in();
}
static void guest_hit_exec(void *self,void *function,void *params,void *result);
static int guest_hit_dispatch_valid(uintptr_t base,uintptr_t weapon,int installed) {
    uintptr_t actual=read_ptr(base+0x14fa430+0x230);
    return read_ptr(weapon)==base+0x14fa430&&guest_hit_native_pins(base)&&
        (installed?(guest_hit_installed&&guest_hit_base==base&&guest_hit_registered==(uintptr_t)guest_hit_exec&&
            guest_hit_original==(ProcessEvent)(base+0x40d450)&&actual==guest_hit_registered):actual==base+0x40d450);
}
/* The equipment guard admits only this exact validated forwarding observer.
 * A registered arbitrary callback, another vtable, or modified original fails. */
static int guest_hit_weapon_pe_matches(uintptr_t base,uintptr_t weapon,uintptr_t pe) {
    return pe==(uintptr_t)guest_hit_exec&&guest_hit_dispatch_valid(base,weapon,1);
}
static int guest_hit_array(uintptr_t address,GameArray *out,int limit) {
    return read_at(address,out,sizeof *out)&&out->count>=0&&out->count<=limit&&out->max>=out->count&&out->max<=limit&&(!out->count||out->data);
}
static int guest_hit_key(uintptr_t base,uintptr_t object,GuestHitKey *out) {
    GuestCameraStamp stamp;uint64_t flags=0;
    _Static_assert(offsetof(GuestCameraStamp,full_name)==sizeof(GuestHitKey),"compact object stamp");
    memset(out,0,sizeof *out);
    if(!guest_camera_stamp_read(base,object,&stamp,0)||read_ptr(stamp.klass+0x13c)==object||
       !read_at(object+0xc,&flags,8)||(flags&(UINT64_C(1)<<61)))return 0;
    memcpy(out,&stamp,sizeof *out);return 1;
}
static int guest_hit_read_value(uintptr_t address,float *value) {
    return read_at(address,value,4)&&guest_hit_value(*value);
}
static int guest_hit_membership(uintptr_t address,uintptr_t target,int limit) {
    GameArray array={0},again={0};uintptr_t entries[256];int found=0;
    if(!guest_hit_array(address,&array,limit))return 0;
    for(int i=0;i<array.count;i+=256){int n=array.count-i;if(n>256)n=256;
        if(!read_at(array.data+(uintptr_t)i*8,entries,(size_t)n*8))return 0;
        for(int j=0;j<n;++j)if(entries[j]==target)++found;
    }
    return read_at(address,&again,sizeof again)&&!memcmp(&array,&again,sizeof array)&&found==1;
}
static int guest_hit_loaded_level(uintptr_t base,uintptr_t level,const NetworkActorScope *scope) {
    if(level==scope->level)return 1;
    GameArray streams={0},again={0};int found=0;
    if(!guest_hit_array(scope->info+0x3e4,&streams,512))return 0;
    for(int i=0;i<streams.count;++i){uintptr_t entry=0,loaded=0;uint32_t flags=0;
        if(!read_at(streams.data+(uintptr_t)i*8,&entry,8))return 0;
        if(!entry)continue;
        if(!narrative_probe_object(base,entry,"LevelStreaming")||!read_at(entry+0xa0,&loaded,8)||!read_at(entry+0xcc,&flags,4))return 0;
        if(loaded==level){if(!(flags&1))return 0;++found;}
    }
    return read_at(scope->info+0x3e4,&again,sizeof again)&&!memcmp(&streams,&again,sizeof streams)&&found==1;
}
static void guest_hit_live_sample(void *context,uintptr_t target,GuestHitSample *out) {
    (void)context;AmmoWireScope *s=&ammo_wire_scope;uintptr_t base=s->base,level=0;unsigned char role=255;
    NetworkActorScope current,again;memset(out,0,sizeof *out);
    if(!guest_camera_actor_live(base,target)||!is_kind_of(base,target,"BioPawn")||
       !guest_hit_key(base,target,&out->target)||!read_at(target+0x26a,&role,1)||role!=3||
       !network_actor_read_scope(base,target,&current)||current.mode!=2||current.info!=s->world.pointer||
       !read_at(target+0x40,&level,8)||!narrative_probe_object(base,level,"Level")||!guest_hit_key(base,level,&out->level)||
       !guest_hit_loaded_level(base,level,&current)||!guest_hit_membership(level+0x60,target,100000))return;
    out->target_valid=1;
    GameArray modules={0},mod_again={0};uintptr_t module=0;int found=0,valid=guest_hit_array(target+0x70,&modules,128);
    if(valid)for(int i=0;i<modules.count;++i){uintptr_t at=0;
        if(!read_at(modules.data+(uintptr_t)i*8,&at,8)){valid=0;break;}
        if(!at)continue;
        if(!narrative_probe_object(base,at,"SFXModule")){valid=0;break;}
        if(is_kind_of(base,at,"SFXModule_DamageNativeBase")){module=at;++found;}
    }
    if(valid&&found==1&&read_ptr(module+0x60)==target&&guest_hit_key(base,module,&out->module)&&
       guest_hit_read_value(module+0x98,&out->health)&&
       read_at(target+0x70,&mod_again,sizeof mod_again)&&!memcmp(&modules,&mod_again,sizeof modules))out->health_valid=1;
    uintptr_t inventory=0;
    if(!read_at(target+0x414,&inventory,8))goto finish;
    if(!inventory){out->shield_valid=1;goto finish;}
    if(!guest_camera_actor_live(base,inventory)||!is_kind_of(base,inventory,"InventoryManager")||
       read_ptr(inventory+0x198)!=s->world.pointer||!guest_hit_key(base,inventory,&out->inventory))goto finish;
    uintptr_t head=0,item=0,shield=0,seen[128];int count=0,shields=0;
    if(!read_at(inventory+0x280,&head,8))goto finish;item=head;
    while(item){
        if(count>=128||!guest_camera_actor_live(base,item)||!is_kind_of(base,item,"Inventory")||
           read_ptr(item+0x2b0)!=inventory)goto finish;
        for(int i=0;i<count;++i)if(seen[i]==item)goto finish;seen[count++]=item;
        if(is_kind_of(base,item,"SFXShield_Base")){shield=item;++shields;}
        if(!read_at(item+0x2a8,&item,8))goto finish;
    }
    uintptr_t again_inventory=0,again_head=0;
    if(!read_at(target+0x414,&again_inventory,8)||again_inventory!=inventory||
       !read_at(inventory+0x280,&again_head,8)||again_head!=head||shields>1)goto finish;
    if(!shields){out->shield_valid=1;goto finish;}
    if(read_ptr(shield+0x180)==target&&read_ptr(shield+0x190)==target&&read_ptr(shield+0x198)==s->world.pointer&&
       read_ptr(shield+0x2b0)==inventory&&read_at(shield+0x26a,&role,1)&&role==3&&
       guest_hit_key(base,shield,&out->shield)&&guest_hit_read_value(shield+0x3c4,&out->shields))
        out->shield_present=out->shield_valid=1;
finish:
    {GuestHitKey key;
        uintptr_t current_inventory=0;
        if(!read_at(target+0x414,&current_inventory,8)||current_inventory!=inventory)out->shield_valid=0;
        if(!guest_hit_key(base,target,&key)||!guest_hit_key_equal(&out->target,&key)||
           !guest_hit_key(base,level,&key)||!guest_hit_key_equal(&out->level,&key))
            out->target_valid=out->health_valid=out->shield_valid=0;
        if(out->health_valid&&(!guest_hit_key(base,module,&key)||!guest_hit_key_equal(&out->module,&key)||
           read_ptr(module+0x60)!=target||!guest_hit_membership(target+0x70,module,128)))out->health_valid=0;
        if(out->shield_valid&&inventory&&(!guest_hit_key(base,inventory,&key)||!guest_hit_key_equal(&out->inventory,&key)||
           read_ptr(target+0x414)!=inventory))out->shield_valid=0;
        if(out->shield_valid&&out->shield_present&&(!guest_hit_key(base,out->shield.pointer,&key)||
           !guest_hit_key_equal(&out->shield,&key)||read_ptr(out->shield.pointer+0x180)!=target||
           read_ptr(out->shield.pointer+0x190)!=target||read_ptr(out->shield.pointer+0x2b0)!=inventory))out->shield_valid=0;
    }
    if(!network_actor_read_scope(base,target,&again)||memcmp(&current,&again,sizeof current))
        out->target_valid=out->health_valid=out->shield_valid=0;
}
static int guest_hit_method_valid(uintptr_t base) {
    GuestPlayerContextMethod *m=&guest_hit_method;uintptr_t fn=m->function.pointer;GameArray script={0};
    if(!guest_player_context_method_matches(base,m,1)||strcmp(m->function.full_name,"SFXGame.SFXWeapon.ProcessInstantHit_Internal")||
       !guest_player_context_function_abi(base,fn,"SFXWeapon","ProcessInstantHit_Internal",0x24102,108,65535,0,0xf52b0,3)||
       !guest_hit_array(fn+0x7c,&script,4096)||script.count!=1102)return 0;
    const char *names[]={"FiringMode","Impact","NumHits"},*types[]={"ByteProperty","StructProperty","IntProperty"};
    const int offsets[]={0,4,104},sizes[]={1,100,4};const uint64_t flags[]={0x80,0x80,0x90};
    uintptr_t prop=read_ptr(fn+0x70),impact=0;
    for(int i=0;i<3;++i){char path[180];int32_t size=0;
        snprintf(path,sizeof path,"SFXGame.SFXWeapon.ProcessInstantHit_Internal.%s",names[i]);
        if(read_ptr(prop+0x40)!=fn||!combat_fire_property(base,prop,types[i],path,offsets[i],flags[i])||
           !read_at(prop+0x74,&size,4)||size!=sizes[i])return 0;
        if(i==1)impact=read_ptr(prop+0xd0);prop=read_ptr(prop+0x68);
    }
    GuestCameraStamp structure;int32_t size=0;
    if(!guest_camera_stamp_read(base,impact,&structure,1)||strcmp(structure.full_name,"Engine.Actor.ImpactInfo")||
       !narrative_probe_object(base,impact,"ScriptStruct")||!read_at(impact+0x78,&size,4)||size!=100)return 0;
    unsigned hit_fields=0;prop=read_ptr(impact+0x70);
    for(unsigned i=0;prop&&i<32;++i){char name[96];
        if(!is_live(base,prop)||read_ptr(prop+0x40)!=impact)return 0;object_name(base,prop,name,sizeof name);
        if(!strcmp(name,"HitActor")){
            if(!combat_fire_property(base,prop,"ObjectProperty","Engine.Actor.ImpactInfo.HitActor",0x58,0x100000)||
               !read_at(prop+0x74,&size,4)||size!=8)return 0;
            ++hit_fields;
        }prop=read_ptr(prop+0x68);
    }
    if(prop||hit_fields!=1)return 0;
    /* Bind the native cached FNameEntry to this reflected function's name. */
    uintptr_t cached=read_ptr(base+0x17a9458),pool=read_ptr(base+NAME_POOLS_RVA+(m->function.name[0]>>29)*8);
    const char expected[]="ProcessInstantHit_Internal";char actual[sizeof expected];
    return cached&&pool&&cached==pool+(m->function.name[0]&0x1fffffff)&&!m->function.name[1]&&
        read_at(cached+12,actual,sizeof actual)&&!memcmp(actual,expected,sizeof expected);
}
static int guest_hit_live_scope(void) {
    AmmoWireScope *s=&ammo_wire_scope;unsigned char net=255,weapon_role=255,pawn_role=255;
    return test_action==6&&ammo_wire_scope_valid()&&is_kind_of(s->base,s->pawn.pointer,"SFXPawn_Ashley")&&
        read_at(s->world.pointer+0x7c8,&net,1)&&net==2&&read_at(s->weapon.pointer+0x26a,&weapon_role,1)&&weapon_role==3&&
        read_at(s->pawn.pointer+0x26a,&pawn_role,1)&&pawn_role==3&&read_ptr(s->pc.pointer+0x67c)==s->channel.connection;
}
static int guest_hit_live_eligible(void *context,const GuestHitArgs *args,uintptr_t caller,GuestHitFrame *frame) {
    (void)context;AmmoWireScope *s=&ammo_wire_scope;
    if(!guest_hit_call_matches(caller,s->base+0xb1f6e3,(uintptr_t)args->self,s->weapon.pointer,
        (uintptr_t)args->function,guest_hit_method.function.pointer,args->params!=NULL,args->result==NULL,1)||
       !guest_hit_installed||!guest_hit_live_scope()||!guest_hit_method_valid(s->base)||
       !guest_hit_dispatch_valid(s->base,(uintptr_t)args->self,1)||
       !read_at((uintptr_t)args->params,&frame->mode,1)||
       !read_at((uintptr_t)args->params+0x5c,&frame->target,8)||
       !read_at((uintptr_t)args->params+0x68,&frame->hits,4))return 0;
    frame->generation=s->generation;frame->epoch=s->epoch;frame->weapon=s->weapon.pointer;frame->pc=s->pc.pointer;
    frame->channel_index=s->channel.index;frame->weapon_index=s->weapon.index;return 1;
}
static int guest_hit_live_same(void *context,const GuestHitArgs *args,const GuestHitFrame *frame) {
    (void)context;AmmoWireScope *s=&ammo_wire_scope;
    return s->generation==frame->generation&&s->epoch==frame->epoch&&(uintptr_t)args->self==s->weapon.pointer&&
        s->weapon.pointer==frame->weapon&&s->pc.pointer==frame->pc&&s->channel.index==frame->channel_index&&s->weapon.index==frame->weapon_index&&
        guest_hit_live_scope()&&guest_hit_method_valid(s->base)&&guest_hit_dispatch_valid(s->base,(uintptr_t)args->self,1);
}
static void guest_hit_live_emit(void *context,const GuestHitFrame *frame,int leaving) {
    (void)context;const GuestHitSample *sample=leaving?&frame->after:&frame->before;
    char health[48],shields[48],row[2000];int hp=leaving&&guest_hit_health_pair(frame),sp=leaving&&guest_hit_shield_pair(frame);
    narrative_probe_float(health,sample->health_valid,sample->health);narrative_probe_float(shields,sample->shield_valid,sample->shields);
    snprintf(row,sizeof row,"{\"guest_hit_trace\":\"%s\",\"call\":%u,\"parent_call\":%u,\"nested_dispatch_calls\":%u,\"generation\":%lu,\"epoch\":%lu,\"channel_index\":%d,\"weapon_index\":%d,\"target_pointer\":\"0x%llx\",\"target_index\":%d,\"target_valid\":%s,\"health\":%s,\"shields\":%s,\"shield_present\":%s,\"health_pair_valid\":%s,\"shield_pair_valid\":%s,\"health_decrease_observed\":%s,\"shield_decrease_observed\":%s,\"firing_mode\":%u,\"num_hits\":%d,\"scope_after_valid\":%s,\"original_forwarded\":%s,\"damage_verified\":false,\"hostility_verified\":false,\"probe_changed_payload\":false,\"monotonic_ms\":%lu}\r\n",
        leaving?"leave":"enter",frame->id,frame->parent,frame->nested_calls,frame->generation,frame->epoch,frame->channel_index,frame->weapon_index,
        (unsigned long long)frame->target,sample->target.index,sample->target_valid?"true":"false",health,shields,
        sample->shield_present?"true":"false",hp?"true":"false",sp?"true":"false",
        hp&&frame->after.health<frame->before.health?"true":"false",sp&&frame->after.shields<frame->before.shields?"true":"false",
        (unsigned)frame->mode,frame->hits,leaving?(frame->scope_after?"true":"false"):"null",leaving?"true":"false",GetTickCount());line(row);
}
static const GuestHitOps guest_hit_live_ops={guest_hit_live_eligible,guest_hit_live_same,guest_hit_live_sample,guest_hit_live_emit};
static void guest_hit_exec(void *self,void *function,void *params,void *result) {
    DWORD error=GetLastError();uintptr_t caller=(uintptr_t)__builtin_return_address(0);GuestHitArgs args={self,function,params,result};
    int on_thread=guest_hit_installed&&GetCurrentThreadId()==ammo_wire_thread;SetLastError(error);
    guest_hit_observe(&guest_hit_observer,guest_hit_original,&args,caller,on_thread,&guest_hit_live_ops,NULL);
}
static void guest_hit_trace_prepare(void) {
    if(!guest_hit_opted()||guest_hit_attempted||GetCurrentThreadId()!=ammo_wire_thread)return;
    DWORD now=GetTickCount();if(guest_hit_discovery_at&&now-guest_hit_discovery_at<1000)return;guest_hit_discovery_at=now;
    if(!guest_hit_live_scope())return;
    AmmoWireScope *s=&ammo_wire_scope;DWORD protection=0,ignored=0;const char *reason="exact-method-slot-native-pins-or-protection";
    if(!read_ptr(s->base+0x17a9458)){
        if(!guest_hit_waiting_name){guest_hit_waiting_name=1;line("{\"guest_hit_trace\":\"waiting\",\"reason\":\"native-cached-event-name-not-ready\"}\r\n");}return;
    }
    guest_hit_attempted=1;
    uintptr_t fn=guest_player_context_find_method(s->base,s->weapon.klass,"SFXWeapon","ProcessInstantHit_Internal");
    if(!supported_image(s->base)){reason="unsupported-image";goto rejected;}
    if(!guest_hit_dispatch_valid(s->base,s->weapon.pointer,0)){reason="native-slot-or-drain-pins";goto rejected;}
    if(!guest_player_context_method_stamp(s->base,fn,&guest_hit_method)){reason="reflected-method-discovery";goto rejected;}
    if(!guest_hit_method_valid(s->base)){reason="reflected-event-abi-or-cached-name";goto rejected;}
    uintptr_t slot=s->base+0x14fa430+0x230;guest_hit_original=(ProcessEvent)(s->base+0x40d450);guest_hit_base=s->base;
    if(!VirtualProtect((void*)slot,8,PAGE_READWRITE,&protection))goto rejected;
    PVOID previous=InterlockedCompareExchangePointer((PVOID volatile*)slot,(PVOID)guest_hit_exec,(PVOID)guest_hit_original);
    int restored=VirtualProtect((void*)slot,8,protection,&ignored);
    if(previous!=(PVOID)guest_hit_original||read_ptr(slot)!=(uintptr_t)guest_hit_exec||!restored){
        if(previous==(PVOID)guest_hit_original){DWORD temporary=0;
            if(VirtualProtect((void*)slot,8,PAGE_READWRITE,&temporary)){
                InterlockedCompareExchangePointer((PVOID volatile*)slot,(PVOID)guest_hit_original,(PVOID)guest_hit_exec);
                VirtualProtect((void*)slot,8,protection,&ignored);
            }
        }goto rejected;
    }
    guest_hit_installed=1;guest_hit_registered=(uintptr_t)guest_hit_exec;
    line("{\"guest_hit_trace\":\"installed\",\"limit\":128,\"exact_drain_callsite\":true,\"gameplay_writes\":false,\"damage_verified\":false}\r\n");return;
rejected:
    {char row[220];snprintf(row,sizeof row,"{\"guest_hit_trace\":\"not-installed\",\"reason\":\"%s\"}\r\n",reason);line(row);}
}
#endif
