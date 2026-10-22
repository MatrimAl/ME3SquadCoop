#ifndef ME3_NATIVE_FIRE_TRACE_H
#define ME3_NATIVE_FIRE_TRACE_H
/* Default-off research observer for the actual native virtual dispatch.
 * Supported LE3 execFireAmmunition (4b5ec0) tail-calls self.vtable+7d8 with
 * RCX=self only; the observed SFXWeapon vtable14fa430 points to c62120.
 * That native fallback dispatches the base script through ProcessEvent.
 * VM-internal script calls may bypass UFunction.Func observers. Conversely,
 * callers that bypass this particular virtual slot are not observed here.
 * An entry proves dispatch, never ammunition consumption or target damage.
 */
typedef void (*NativeFireCall)(void*);
static NativeFireCall native_fire_original;
static GuestPlayerContextMethod native_fire_method;
static int native_fire_enabled=-1,native_fire_attempted,native_fire_installed;
static unsigned native_fire_rows,native_fire_calls;
static const unsigned char native_fire_bridge_pin[]={
    0x40,0x53,0x48,0x83,0xec,0x20,0x48,0xff,0x42,0x28,0x48,0x8b,0xd9,0x48,0x8b,0x42,0x28,0x80,0x38,0x41,0x75,0x14,0x48,0x8b,0x4a,0x20,0x48,0xff,0xc0,0x45,0x33,0xc0,0x48,0x89,0x42,0x28,0xff,0x15,0xce,0xee,0x33,0x01,0x48,0x8b,0x03,0x48,0x8b,0xcb,0x48,0x83,0xc4,0x20,0x5b,0x48,0xff,0xa0,0xd8,0x07,0x00,0x00};
static const unsigned char native_fire_body_pin[]={
    0x48,0x89,0x5c,0x24,0x10,0x57,0x48,0x83,0xec,0x20,0x48,0x8b,0xf9,0x48,0x8b,0x0d,0xcc,0xf1,0xc4,0x00,0x48,0x85,0xc9,0x75,0x1f,0x48,0x8d,0x0d,0xc8,0xa3,0x41,0x00,0xe8,0xbb,0xb5,0x79,0xff,0x48,0x89,0x05,0xb4,0xf1,0xc4,0x00,0xe8,0x8f,0x1c,0x7a,0xff,0x48,0x8b,0x0d,0xa8,0xf1,0xc4,0x00,0x33,0xd2,0xe8,0x81,0xad,0x46,0xff,0x48,0x8b,0x0d,0xaa,0x89,0xb4,0x00,0x48,0x8b,0xd8,0xe8,0x72,0x42,0x43,0xff,0x89,0x44,0x24,0x30,0x41,0xb9,0xff,0xff,0xff,0xff,0xc7,0x44,0x24,0x34,0x00,0x00,0x00,0x00,0x45,0x33,0xc0,0x48,0x8b,0x54,0x24,0x30,0x48,0x8b,0xcb,0xe8,0xd0,0x22,0x4c,0xff,0x4c,0x8b,0x17,0x45,0x33,0xc9,0x45,0x33,0xc0,0x48,0x8b,0xd0,0x48,0x8b,0xcf,0x48,0x8b,0x5c,0x24,0x38,0x48,0x83,0xc4,0x20,0x5f,0x49,0xff,0xa2,0x30,0x02,0x00,0x00};
static int native_fire_opted(void) {
    if(native_fire_enabled<0){wchar_t value[8]={0};native_fire_enabled=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_NATIVE_FIRE_TRACE",value,8)==1&&value[0]==L'1';}
    return native_fire_enabled&&ammo_wire_opted_in();
}
static int native_fire_pins(uintptr_t base) {
    unsigned char bridge[sizeof native_fire_bridge_pin],body[sizeof native_fire_body_pin];
    return read_at(base+0x4b5ec0,bridge,sizeof bridge)&&!memcmp(bridge,native_fire_bridge_pin,sizeof bridge)&&
        read_at(base+0xc62120,body,sizeof body)&&!memcmp(body,native_fire_body_pin,sizeof body);
}
static void native_fire_exec(void *self);
static int native_fire_dispatch_valid(uintptr_t base,uintptr_t weapon,int installed) {
    return read_ptr(weapon)==base+0x14fa430&&
        read_ptr(base+0x14fa430+0x7d8)==(installed?(uintptr_t)native_fire_exec:base+0xc62120)&&native_fire_pins(base);
}
static int native_fire_context_matches(uintptr_t self,uintptr_t weapon,DWORD thread,DWORD game_thread,
                                       unsigned long generation,unsigned long expected_generation,int valid) {
    return valid&&self&&self==weapon&&thread==game_thread&&generation==expected_generation;
}
static int native_fire_reserve(void) {
    if(native_fire_rows>126)return 0;native_fire_rows+=2;return 1;
}
static void native_fire_snapshot(const char *stage,unsigned id,unsigned long generation,int incoming) {
    AmmoWireScope *s=&ammo_wire_scope;int used=-1,spare=-1,shot_count=-1;float chance=0,per_shot=0;
    unsigned char role=255,pawn_role=255;char state[96]={0},safe[200];
    int valid=native_fire_context_matches(s->weapon.pointer,s->weapon.pointer,GetCurrentThreadId(),ammo_wire_thread,
        s->generation,generation,ammo_wire_scope_valid())&&
        read_at(s->weapon.pointer+0x3b8,&used,4)&&read_at(s->weapon.pointer+0x3b4,&spare,4)&&
        read_at(s->weapon.pointer+0x59c,&chance,4)&&read_at(s->weapon.pointer+0xfa4,&per_shot,4)&&
        read_at(s->weapon.pointer+0x26a,&role,1)&&read_at(s->pawn.pointer+0x26a,&pawn_role,1)&&
        read_at(s->pawn.pointer+0x564,&shot_count,4)&&chance>=0&&chance<=10000&&per_shot>=0&&per_shot<=10000;
    int known=valid&&native_state_name(s->base,s->weapon.pointer,state,sizeof state);identity_quote(state,safe,sizeof safe);
    char row[1200];snprintf(row,sizeof row,"{\"native_fire_trace\":\"%s\",\"side\":\"%s\",\"call\":%u,\"generation\":%lu,\"channel_index\":%d,\"snapshot_valid\":%s,\"used\":%d,\"spare\":%d,\"pawn_shot_count\":%d,\"weapon_role\":%u,\"pawn_role\":%u,\"no_ammo_use_chance_value\":%.9g,\"ammo_per_shot\":%.9g,\"state_known\":%s,\"state\":\"%s\",\"selected_weapon_receive_context\":%s,\"original_forwarded\":%s,\"damage_verified\":false,\"read_only\":true,\"monotonic_ms\":%lu}\r\n",
        stage,test_action==6?"host":"guest",id,generation,s->channel.index,valid?"true":"false",used,spare,shot_count,
        (unsigned)role,(unsigned)pawn_role,valid?(double)chance:0,valid?(double)per_shot:0,known?"true":"false",safe,
        incoming?"true":"false",!strcmp(stage,"leave")?"true":"false",GetTickCount());line(row);
}
static void native_fire_exec(void *self) {
    DWORD before=GetLastError();AmmoWireScope *s=&ammo_wire_scope;unsigned id=0;unsigned long generation=s->generation;
    int capture=native_fire_installed&&GetCurrentThreadId()==ammo_wire_thread&&(uintptr_t)self==s->weapon.pointer&&
        native_fire_context_matches((uintptr_t)self,s->weapon.pointer,
        GetCurrentThreadId(),ammo_wire_thread,s->generation,generation,ammo_wire_scope_valid())&&
        guest_player_context_method_matches(s->base,&native_fire_method,0)&&
        native_fire_dispatch_valid(s->base,(uintptr_t)self,1)&&native_fire_reserve();
    int incoming=capture&&ammo_wire_receive&&ammo_wire_receive->channel==s->channel.channel;
    if(capture){id=++native_fire_calls;native_fire_snapshot("enter",id,generation,incoming);}
    SetLastError(before);native_fire_original(self);DWORD after=GetLastError();
    if(capture)native_fire_snapshot("leave",id,generation,incoming);
    SetLastError(after);
}
static void native_fire_trace_prepare(void) {
    if(!native_fire_opted()||native_fire_attempted||GetCurrentThreadId()!=ammo_wire_thread||!ammo_wire_scope_valid())return;
    native_fire_attempted=1;AmmoWireScope *s=&ammo_wire_scope;DWORD protection;
    uintptr_t fn=guest_player_context_find_method(s->base,s->weapon.klass,"SFXWeapon_NativeBase","FireAmmunition");
    if(!supported_image(s->base)||!native_fire_dispatch_valid(s->base,s->weapon.pointer,0)||
       !guest_player_context_method_stamp(s->base,fn,&native_fire_method)||
       strcmp(native_fire_method.function.full_name,"SFXGame.SFXWeapon_NativeBase.FireAmmunition")||
       !guest_player_context_function_abi(s->base,fn,"SFXWeapon_NativeBase","FireAmmunition",0x20500,0,65535,0,0x4b5ec0,0))goto rejected;
    uintptr_t slot=s->base+0x14fa430+0x7d8;native_fire_original=(NativeFireCall)(s->base+0xc62120);
    if(!VirtualProtect((void*)slot,8,PAGE_READWRITE,&protection))goto rejected;
    PVOID previous=InterlockedCompareExchangePointer((PVOID volatile*)slot,(PVOID)native_fire_exec,(PVOID)native_fire_original);
    DWORD ignored;VirtualProtect((void*)slot,8,protection,&ignored);
    if(previous!=(PVOID)native_fire_original)goto rejected;
    native_fire_installed=1;line("{\"native_fire_trace\":\"installed\",\"limit\":128,\"self_only_abi\":true,\"virtual_dispatch_only\":true,\"read_only\":true}\r\n");return;
rejected:
    line("{\"native_fire_trace\":\"not-installed\",\"reason\":\"exact-native-slot-or-method\"}\r\n");
}
static void *native_fire_fixture_self;static unsigned native_fire_fixture_calls;static int native_fire_fixture_bad;
static void native_fire_fixture_forward(void *self) {
    ++native_fire_fixture_calls;if(self!=native_fire_fixture_self||GetLastError()!=42)native_fire_fixture_bad=1;
    *(int*)self=987;SetLastError(123);
}
static int native_fire_selftest(void) {
    NativeFireCall saved=native_fire_original;int installed=native_fire_installed;unsigned rows=native_fire_rows;DWORD error=GetLastError();
    native_fire_installed=0;native_fire_original=native_fire_fixture_forward;native_fire_fixture_calls=0;native_fire_fixture_bad=0;
    int data=3;native_fire_fixture_self=&data;SetLastError(42);native_fire_exec(&data);
    int ok=data==987&&native_fire_fixture_calls==1&&!native_fire_fixture_bad&&GetLastError()==123;
    ok=ok&&native_fire_context_matches(1,1,2,2,3,3,1)&&!native_fire_context_matches(4,1,2,2,3,3,1)&&
        !native_fire_context_matches(1,1,4,2,3,3,1)&&!native_fire_context_matches(1,1,2,2,4,3,1)&&
        !native_fire_context_matches(1,1,2,2,3,3,0)&&!native_fire_context_matches(0,0,2,2,3,3,1);
    native_fire_rows=126;ok=ok&&native_fire_reserve()&&!native_fire_reserve()&&native_fire_rows==128;
    unsigned char *image=VirtualAlloc(NULL,0x1500000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!image)ok=0;
    else {
        uintptr_t base=(uintptr_t)image,weapon=base+0x1000;*(uintptr_t*)weapon=base+0x14fa430;
        *(uintptr_t*)(base+0x14fa430+0x7d8)=base+0xc62120;
        memcpy(image+0x4b5ec0,native_fire_bridge_pin,sizeof native_fire_bridge_pin);memcpy(image+0xc62120,native_fire_body_pin,sizeof native_fire_body_pin);
        ok=ok&&native_fire_dispatch_valid(base,weapon,0)&&!native_fire_dispatch_valid(base,weapon,1);
        *(uintptr_t*)(base+0x14fa430+0x7d8)+=1;ok=ok&&!native_fire_dispatch_valid(base,weapon,0);
        *(uintptr_t*)(base+0x14fa430+0x7d8)=base+0xc62120;
        image[0x4b5ec0+0x35]^=1;ok=ok&&!native_fire_dispatch_valid(base,weapon,0);image[0x4b5ec0+0x35]^=1;
        image[0xc62120+0x10]^=1;ok=ok&&!native_fire_dispatch_valid(base,weapon,0);image[0xc62120+0x10]^=1;
        *(uintptr_t*)weapon+=8;ok=ok&&!native_fire_dispatch_valid(base,weapon,0);*(uintptr_t*)weapon-=8;
        *(uintptr_t*)(base+0x14fa430+0x7d8)=(uintptr_t)native_fire_exec;
        ok=ok&&native_fire_dispatch_valid(base,weapon,1);VirtualFree(image,0,MEM_RELEASE);
    }
    native_fire_original=saved;native_fire_installed=installed;native_fire_rows=rows;SetLastError(error);return ok;
}
#endif
