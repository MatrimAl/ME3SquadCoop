#ifndef ME3_AMMO_WIRE_TRACE_H
#define ME3_AMMO_WIRE_TRACE_H
/* Explicit research opt-in. Observe the engine's IntProperty NetSerializeItem
 * without changing arguments, data, archive, result or LastError. Exact LE3:
 * 108dfb0+2f8 ->146f40 serializes four bytes from R9 using archive RDX.
 * Property send return48d433: data=Actor+Offset; receive return488ed4:
 * data is scratch, later copied by property+320. ReceivedBunch brackets bind
 * scratch decoding to the selected weapon channel and observe final storage.
 * Encoding is not delivery. Decode alone is not proof of final application.
 */
typedef int (*AmmoWireSerialize)(void*,void*,void*,void*);
typedef struct AmmoWireScope {
    uintptr_t base,map;unsigned long generation,epoch;
    GuestCameraStamp pc,pawn,weapon,world,properties[2];
    NetworkActorIdentityEntry channel;
    int ready;
} AmmoWireScope;
typedef struct AmmoWireReceive {
    struct AmmoWireReceive *previous;
    unsigned long generation,id;unsigned mask;
    int valid,before[2],decoded[2];
} AmmoWireReceive;
static AmmoWireScope ammo_wire_scope;
static AmmoWireReceive *ammo_wire_receive;
static AmmoWireSerialize ammo_wire_original;
static uintptr_t ammo_wire_base;
static DWORD ammo_wire_thread,ammo_wire_discovery_at;
static int ammo_wire_enabled=-1,ammo_wire_attempted,ammo_wire_installed;
static unsigned ammo_wire_rows,ammo_wire_limited,ammo_wire_receive_id;
static int ammo_wire_opted_in(void) {
    if(ammo_wire_enabled<0){wchar_t value[8]={0};ammo_wire_enabled=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_AMMO_WIRE_TRACE",value,8)==1&&value[0]==L'1';}
    return ammo_wire_enabled&&progress_enabled&&(test_action==5||test_action==6);
}
static int ammo_wire_take(void) {
    if(ammo_wire_rows<256){++ammo_wire_rows;return 1;}
    if(!ammo_wire_limited){ammo_wire_limited=1;line("{\"ammo_wire\":\"limit\",\"sample_budget\":256,\"observational\":true}\r\n");}
    return 0;
}
static int ammo_wire_scope_valid(void) {
    AmmoWireScope *s=&ammo_wire_scope;NetworkActorIdentityEntry ch={0};NetworkActorScope world;
    return s->ready&&has_marker(root)&&guest_camera_stamp_matches(s->base,&s->pc,0)&&
        guest_camera_stamp_matches(s->base,&s->pawn,0)&&guest_camera_stamp_matches(s->base,&s->weapon,0)&&
        guest_camera_stamp_matches(s->base,&s->world,0)&&
        read_ptr(s->pc.pointer+0x3bc)==s->pawn.pointer&&read_ptr(s->pawn.pointer+0x374)==s->pc.pointer&&
        read_ptr(s->pawn.pointer+0x41c)==s->weapon.pointer&&read_ptr(s->weapon.pointer+0x180)==s->pawn.pointer&&
        read_ptr(s->weapon.pointer+0x190)==s->pawn.pointer&&read_ptr(s->weapon.pointer+0x198)==s->world.pointer&&
        read_ptr(s->pc.pointer+0x198)==s->world.pointer&&read_ptr(s->pawn.pointer+0x198)==s->world.pointer&&
        network_actor_read_scope(s->base,s->weapon.pointer,&world)&&world.info==s->world.pointer&&world.driver==s->channel.driver&&
        network_actor_channel_valid(s->base,s->weapon.pointer,s->channel.channel,s->channel.driver,&ch)&&
        ch.connection==s->channel.connection&&ch.index==s->channel.index&&
        read_ptr(ch.connection+0xf0)==s->map&&is_live(s->base,s->map)&&
        (test_action!=6||read_ptr(s->pc.pointer+0x67c)==ch.connection);
}
static int ammo_wire_property(uintptr_t base,uintptr_t prop,int field,GuestCameraStamp *stamp) {
    int32_t offset=-1,dim=0,size=0;uint64_t flags=0;uint16_t rep[2]={0};
    const char *path=field?"SFXGame.SFXWeapon_NativeBase.CurrentSpareAmmo":"SFXGame.SFXWeapon_NativeBase.AmmoUsedCount";
    return guest_camera_stamp_read(base,prop,stamp,1)&&is_kind_of(base,prop,"IntProperty")&&
        !strcmp(stamp->full_name,path)&&read_ptr(prop)==base+0x108dfb0&&
        read_at(prop+0x70,&dim,4)&&dim==1&&read_at(prop+0x74,&size,4)&&size==4&&
        read_at(prop+0x78,&flags,8)&&flags==0x20&&read_at(prop+0x80,rep,4)&&rep[0]==66&&rep[1]==(field?22:23)&&
        read_at(prop+0x94,&offset,4)&&offset==(field?0x3b4:0x3b8);
}
static int ammo_wire_properties(uintptr_t base,uintptr_t weapon,GuestCameraStamp out[2]) {
    uintptr_t klass=read_ptr(weapon+0x50);int found=0;
    for(int depth=0;klass&&depth<32;++depth){
        if(!is_live(base,klass))return 0;
        char name[96];object_name(base,klass,name,sizeof name);
        if(!strcmp(name,"SFXWeapon_NativeBase")){
            uintptr_t child=read_ptr(klass+0x70),seen[512];int n=0;
            while(child&&n<512){
                if(!is_live(base,child))return 0;
                for(int i=0;i<n;++i)if(seen[i]==child)return 0;
                seen[n++]=child;object_name(base,child,name,sizeof name);
                int field=!strcmp(name,"AmmoUsedCount")?0:!strcmp(name,"CurrentSpareAmmo")?1:-1;
                if(field>=0){if((found&(1<<field))||read_ptr(child+0x40)!=klass||!ammo_wire_property(base,child,field,&out[field]))return 0;found|=1<<field;}
                child=read_ptr(child+0x68);
            }
            return !child&&found==3;
        }
        klass=read_ptr(klass+0x60);
    }
    return 0;
}
static void ammo_wire_prepare(uintptr_t base) {
    if(!ammo_wire_installed||GetCurrentThreadId()!=ammo_wire_thread)return;
    AmmoChangeTrace *t=&ammo_change_trace;AmmoWireScope *s=&ammo_wire_scope;
    if(s->ready&&s->epoch==t->epoch&&guest_camera_stamp_core_equal(&s->weapon,&t->weapon)&&ammo_wire_scope_valid())return;
    s->ready=0;if(!t->epoch||!guest_camera_stamp_matches(base,&t->weapon,0)||
        !guest_camera_stamp_matches(base,&t->pc,0)||!guest_camera_stamp_matches(base,&t->pawn,0)||
        read_ptr(t->pc.pointer+0x3bc)!=t->pawn.pointer||read_ptr(t->pawn.pointer+0x374)!=t->pc.pointer)return;
    DWORD now=GetTickCount();if(ammo_wire_discovery_at&&now-ammo_wire_discovery_at<1000)return;ammo_wire_discovery_at=now;
    int index=network_weapon_channel_id(base,t->weapon.pointer);if(index<0)return;
    NetworkActorIdentityEntry ch={0};
    for(unsigned i=0;i<4;++i)if(network_actor_identity_cache[i].base==base&&network_actor_identity_cache[i].pawn==t->weapon.pointer)ch=network_actor_identity_cache[i];
    if(!ch.channel||!ammo_wire_properties(base,t->weapon.pointer,s->properties))return;
    uintptr_t map=read_ptr(ch.connection+0xf0);if(!is_live(base,map)||!is_kind_of(base,map,"PackageMap"))return;
    s->base=base;s->pc=t->pc;s->pawn=t->pawn;s->weapon=t->weapon;s->world=t->world;s->epoch=t->epoch;
    s->map=map;s->channel=ch;++s->generation;s->ready=1;
    if(!ammo_wire_scope_valid()){s->ready=0;return;}
    if(ammo_wire_take()){char row[500];snprintf(row,sizeof row,"{\"ammo_wire\":\"scope\",\"side\":\"%s\",\"generation\":%lu,\"epoch\":%lu,\"weapon_index\":%d,\"channel_index\":%d,\"monotonic_ms\":%lu}\r\n",test_action==6?"host":"guest",s->generation,s->epoch,s->weapon.index,ch.index,now);line(row);}
}
static void ammo_wire_receive_begin(AmmoWireReceive *f,uintptr_t channel) {
    memset(f,0,sizeof *f);f->previous=ammo_wire_receive;
    if(!ammo_wire_installed||GetCurrentThreadId()!=ammo_wire_thread)return;
    /* Push invalid frames too: a nested receive for another actor must never
     * inherit the outer weapon's identity. */
    ammo_wire_receive=f;AmmoWireScope *s=&ammo_wire_scope;f->generation=s->generation;
    if(test_action==5&&channel==s->channel.channel&&ammo_wire_scope_valid()&&
       read_at(s->weapon.pointer+0x3b8,&f->before[0],4)&&read_at(s->weapon.pointer+0x3b4,&f->before[1],4)){
        f->valid=1;f->id=++ammo_wire_receive_id;
    }
}
static void ammo_wire_receive_end(AmmoWireReceive *f) {
    if(ammo_wire_receive!=f)return;
    AmmoWireScope *s=&ammo_wire_scope;int after[2]={0};
    if(f->valid&&f->mask&&f->generation==s->generation&&ammo_wire_scope_valid()&&
       read_at(s->weapon.pointer+0x3b8,&after[0],4)&&read_at(s->weapon.pointer+0x3b4,&after[1],4)&&ammo_wire_take()){
        char row[1100];snprintf(row,sizeof row,"{\"ammo_wire\":\"receive-complete\",\"generation\":%lu,\"receive_id\":%lu,\"channel_index\":%d,\"field_mask\":%u,\"used_before\":%d,\"used_after\":%d,\"spare_before\":%d,\"spare_after\":%d,\"decoded_used\":%d,\"decoded_spare\":%d,\"original_receive_forwarded\":true,\"probe_changed_payload\":false,\"monotonic_ms\":%lu}\r\n",s->generation,f->id,s->channel.index,f->mask,f->before[0],after[0],f->before[1],after[1],f->decoded[0],f->decoded[1],GetTickCount());line(row);
    }
    ammo_wire_receive=f->previous;
}
static int ammo_wire_serialize(void *property,void *archive,void *map,void *data) {
    uintptr_t caller=(uintptr_t)__builtin_return_address(0);DWORD before_error=GetLastError();
    AmmoWireScope *s=&ammo_wire_scope;AmmoWireReceive *f=ammo_wire_receive;
    int field=(uintptr_t)property==s->properties[0].pointer?0:(uintptr_t)property==s->properties[1].pointer?1:-1;
    int eligible=ammo_wire_installed&&field>=0&&GetCurrentThreadId()==ammo_wire_thread;
    if(eligible&&ammo_wire_rows>=256){ammo_wire_take();eligible=0;}
    int capture=eligible&&
        (uintptr_t)map==s->map&&ammo_wire_scope_valid()&&guest_camera_stamp_matches(s->base,&s->properties[field],0)&&
        ((test_action==6&&caller==s->base+0x48d433&&(uintptr_t)data==s->weapon.pointer+(field?0x3b4:0x3b8))||
         (test_action==5&&caller==s->base+0x488ed4&&f&&f->valid&&f->generation==s->generation));
    int value_before=0,value_after=0;uint32_t error_before=1,error_after=1;
    if(capture)capture=read_at((uintptr_t)data,&value_before,4)&&read_at((uintptr_t)archive+0x38,&error_before,4);
    SetLastError(before_error);int result=ammo_wire_original(property,archive,map,data);DWORD after_error=GetLastError();
    if(capture&&ammo_wire_scope_valid()&&read_at((uintptr_t)data,&value_after,4)&&read_at((uintptr_t)archive+0x38,&error_after,4)&&ammo_wire_take()){
        int accepted=result==1&&!error_before&&!error_after;
        if(accepted&&test_action==5&&f&&f->valid&&f->generation==s->generation){f->mask|=1u<<field;f->decoded[field]=value_after;}
        char row[1000];snprintf(row,sizeof row,"{\"ammo_wire\":\"field\",\"direction\":\"%s\",\"field\":\"%s\",\"generation\":%lu,\"receive_id\":%lu,\"channel_index\":%d,\"value_before\":%d,\"value_after\":%d,\"native_result\":%d,\"archive_error_before\":%u,\"archive_error_after\":%u,\"accepted\":%s,\"original_forwarded\":true,\"probe_changed_payload\":false,\"packet_delivery_verified\":false,\"monotonic_ms\":%lu}\r\n",test_action==6?"encode":"decode",field?"CurrentSpareAmmo":"AmmoUsedCount",s->generation,test_action==5&&f?f->id:0,s->channel.index,value_before,value_after,result,error_before,error_after,accepted?"true":"false",GetTickCount());line(row);
    }
    SetLastError(after_error);return result;
}
static int ammo_wire_pins(uintptr_t base) {
    static const unsigned char fn[]={0x48,0x83,0xec,0x28,0x48,0x8b,0xca,0x41,0xb8,0x04,0x00,0x00,0x00,0x49,0x8b,0xd1,0xe8,0xdb,0xd9,0xf3,0xff,0xb8,0x01,0x00,0x00,0x00,0x48,0x83,0xc4,0x28,0xc3};
    static const unsigned char send[]={0x48,0x8b,0x03,0x4c,0x8b,0x90,0xf8,0x02,0x00,0x00,0x48,0x63,0xf6,0x4d,0x8b,0x8f,0xa0,0x00,0x00,0x00,0x4c,0x03,0xce,0x4d,0x8b,0x47,0x60,0x4d,0x8b,0x80,0xf0,0x00,0x00,0x00,0x48,0x8d,0x55,0x10,0x48,0x8b,0xcb,0x41,0xff,0xd2,0x83,0x7d,0x48,0x00};
    static const unsigned char receive[]={0x48,0x8b,0x07,0x4c,0x8b,0xad,0xf0,0x00,0x00,0x00,0x4d,0x8b,0x45,0x60,0x4c,0x8b,0xcb,0x4d,0x8b,0x80,0xf0,0x00,0x00,0x00,0x48,0x8b,0x95,0xf8,0x00,0x00,0x00,0x48,0x8b,0xcf,0xff,0x90,0xf8,0x02,0x00,0x00};
    unsigned char a[sizeof fn],b[sizeof receive],c[sizeof send];
    return read_at(base+0x146f40,a,sizeof a)&&!memcmp(a,fn,sizeof a)&&
        read_at(base+0x488eac,b,sizeof b)&&!memcmp(b,receive,sizeof b)&&
        read_at(base+0x48d407,c,sizeof c)&&!memcmp(c,send,sizeof c);
}
static void ammo_wire_install(uintptr_t base) {
    if(!ammo_wire_opted_in()||ammo_wire_attempted||!has_marker(root))return;
    ammo_wire_attempted=1;uintptr_t slot=base+0x108dfb0+0x2f8,expected=base+0x146f40;DWORD protection;
    if(!supported_image(base)||read_ptr(slot)!=expected||!ammo_wire_pins(base)||
       !VirtualProtect((void*)slot,8,PAGE_READWRITE,&protection)){
        line("{\"ammo_wire\":\"not-installed\",\"reason\":\"native-pins-or-slot\"}\r\n");return;
    }
    ammo_wire_original=(AmmoWireSerialize)expected;ammo_wire_base=base;ammo_wire_thread=GetCurrentThreadId();
    PVOID previous=InterlockedCompareExchangePointer((PVOID volatile*)slot,(PVOID)ammo_wire_serialize,(PVOID)expected);
    DWORD ignored;VirtualProtect((void*)slot,8,protection,&ignored);
    if((uintptr_t)previous!=expected){ammo_wire_original=NULL;return;}
    ammo_wire_installed=1;line("{\"ammo_wire\":\"installed\",\"sample_budget\":256,\"probe_changed_payload\":false}\r\n");
}
static uintptr_t ammo_wire_fixture_args[4];static unsigned ammo_wire_fixture_calls;
static int ammo_wire_fixture_forward(void *a,void *b,void *c,void *d) {
    ammo_wire_fixture_args[0]=(uintptr_t)a;ammo_wire_fixture_args[1]=(uintptr_t)b;
    ammo_wire_fixture_args[2]=(uintptr_t)c;ammo_wire_fixture_args[3]=(uintptr_t)d;
    ++ammo_wire_fixture_calls;*(int*)d=47;SetLastError(321);return 17;
}
static int ammo_wire_selftest(void) {
    AmmoWireSerialize saved=ammo_wire_original;int installed=ammo_wire_installed;DWORD thread=ammo_wire_thread,error=GetLastError();
    AmmoWireReceive *previous=ammo_wire_receive;unsigned rows=ammo_wire_rows,limited=ammo_wire_limited;
    int ready=ammo_wire_scope.ready;ammo_wire_scope.ready=0;ammo_wire_original=ammo_wire_fixture_forward;
    int a=1,b=2,c=3,data=0;ammo_wire_fixture_calls=0;ammo_wire_installed=0;
    int result=ammo_wire_serialize(&a,&b,&c,&data);
    int ok=result==17&&data==47&&GetLastError()==321&&ammo_wire_fixture_calls==1&&
        ammo_wire_fixture_args[0]==(uintptr_t)&a&&ammo_wire_fixture_args[1]==(uintptr_t)&b&&
        ammo_wire_fixture_args[2]==(uintptr_t)&c&&ammo_wire_fixture_args[3]==(uintptr_t)&data;
    ammo_wire_installed=1;ammo_wire_thread=GetCurrentThreadId();
    AmmoWireReceive outer={0},inner;outer.valid=1;ammo_wire_receive=&outer;
    ammo_wire_receive_begin(&inner,1);ok=ok&&ammo_wire_receive==&inner&&!inner.valid&&inner.previous==&outer;
    ammo_wire_receive_end(&inner);ok=ok&&ammo_wire_receive==&outer;
    ammo_wire_rows=0;ammo_wire_limited=0;
    for(int i=0;i<256;++i)ok=ammo_wire_take()&&ok;
    ok=ok&&!ammo_wire_take()&&!ammo_wire_take()&&ammo_wire_rows==256&&ammo_wire_limited;
    ammo_wire_original=saved;ammo_wire_installed=installed;ammo_wire_thread=thread;ammo_wire_receive=previous;
    ammo_wire_rows=rows;ammo_wire_limited=limited;ammo_wire_scope.ready=ready;SetLastError(error);return ok;
}
#endif
