#ifndef ME3_GUEST_ACTIVATION_TRACE_H
#define ME3_GUEST_ACTIVATION_TRACE_H
/* Default-off protected observation of the exact empty SFXWeapon override.
 * A hit proves this UFunction.Func entry, not the entire ClientGivenTo chain.
 * Calls before installation, other weapons and VM paths bypassing Func are
 * outside coverage. Never change locals, script, counter or dispatch target. */
typedef void (*GuestActivationExec)(void*,void*,void*);
static GuestActivationExec guest_activation_original;
static GuestPlayerContextMethod guest_activation_method;
static int guest_activation_enabled=-1,guest_activation_installed,guest_activation_attempted;
static unsigned guest_activation_rows,guest_activation_calls;
static int guest_activation_opted(void) {
    if(guest_activation_enabled<0){wchar_t value[8]={0};guest_activation_enabled=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_ACTIVATION_TRACE",value,8)==1&&value[0]==L'1';}
    return guest_activation_enabled&&progress_enabled&&test_action==5;
}
static int guest_activation_matches(uintptr_t self,uintptr_t selected,DWORD thread,DWORD owner,
    unsigned long generation,unsigned long current,int live) {
    return self&&self==selected&&owner&&thread==owner&&generation&&generation==current&&live==1;
}
static int guest_activation_reserve(void) {
    if(guest_activation_rows>126)return 0;guest_activation_rows+=2;return 1;
}
static void guest_activation_snapshot(const char *stage,unsigned call,unsigned long generation,uintptr_t frame) {
    AmmoWireScope *s=&ammo_wire_scope;char state[96]={0},safe[200],row[1700];
    int valid=generation==s->generation&&ammo_wire_scope_valid();
    uintptr_t inventory=valid?read_ptr(s->pawn.pointer+0x414):0;
    uintptr_t weapon_inventory=valid?read_ptr(s->weapon.pointer+0x2b0):0;
    int known=valid&&native_state_name(s->base,s->weapon.pointer,state,sizeof state);
    identity_quote(state,safe,sizeof safe);
    /* Parameters may still be bytecode expressions on entry. Do not interpret
     * frame.Locals as initialized values or advance frame.Code to decode them. */
    snprintf(row,sizeof row,
        "{\"guest_activation_trace\":\"%s\",\"call\":%u,\"generation\":%lu,\"channel_index\":%d,\"function\":\"SFXGame.SFXWeapon.ClientWeaponSet\",\"scope_valid\":%s,\"frame_node_matches\":%s,\"frame_object_matches\":%s,\"inventory\":\"0x%llx\",\"weapon_inventory\":\"0x%llx\",\"state_known\":%s,\"state\":\"%s\",\"original_forwarded\":%s,\"activation_verified\":false,\"monotonic_ms\":%lu}\r\n",
        stage,call,generation,s->channel.index,valid?"true":"false",
        read_ptr(frame+0x18)==guest_activation_method.function.pointer?"true":"false",
        read_ptr(frame+0x20)==s->weapon.pointer?"true":"false",
        (unsigned long long)inventory,(unsigned long long)weapon_inventory,known?"true":"false",safe,
        !strcmp(stage,"leave")?"true":"false",GetTickCount());line(row);
}
static void guest_activation_exec(void *self,void *frame,void *result) {
    DWORD before=GetLastError();AmmoWireScope *s=&ammo_wire_scope;
    unsigned long generation=s->generation;unsigned call=0;
    int capture=guest_activation_installed&&guest_activation_opted()&&
        guest_activation_matches((uintptr_t)self,s->weapon.pointer,GetCurrentThreadId(),ammo_wire_thread,
            generation,s->generation,ammo_wire_scope_valid())&&
        guest_player_context_method_matches(s->base,&guest_activation_method,1)&&
        read_ptr(guest_activation_method.function.pointer+0xd8)==(uintptr_t)guest_activation_exec&&
        guest_equipment_sfx_weapon_set_body(s->base,guest_activation_method.function.pointer)&&guest_activation_reserve();
    if(capture){call=++guest_activation_calls;guest_activation_snapshot("enter",call,generation,(uintptr_t)frame);}
    SetLastError(before);guest_activation_original(self,frame,result);DWORD after=GetLastError();
    if(capture)guest_activation_snapshot("leave",call,generation,(uintptr_t)frame);
    SetLastError(after);
}
static void guest_activation_prepare(void) {
    if(!guest_activation_opted()||guest_activation_attempted||GetCurrentThreadId()!=ammo_wire_thread||!ammo_wire_scope_valid())return;
    AmmoWireScope *s=&ammo_wire_scope;
    uintptr_t fn=guest_player_context_find_method(s->base,s->weapon.klass,"SFXWeapon","ClientWeaponSet");
    if(!fn)return;
    guest_activation_attempted=1;
    if(!supported_image(s->base)||!guest_equipment_dispatch_pins(s->base)||
       !guest_player_context_method_stamp(s->base,fn,&guest_activation_method)||
       strcmp(guest_activation_method.function.full_name,"SFXGame.SFXWeapon.ClientWeaponSet")||
       !guest_player_context_function_abi(s->base,fn,"SFXWeapon","ClientWeaponSet",0x10241c0,8,65535,0,0xf52b0,2)||
       !guest_equipment_sfx_weapon_set_body(s->base,fn)||!native_script_slot_matches(s->base,s->weapon.klass,223,fn))goto rejected;
    uintptr_t slot=fn+0xd8;DWORD old;
    if(!VirtualProtect((void*)slot,8,PAGE_READWRITE,&old))goto rejected;
    guest_activation_original=(GuestActivationExec)(s->base+0xf52b0);
    PVOID previous=InterlockedCompareExchangePointer((PVOID volatile*)slot,(PVOID)guest_activation_exec,(PVOID)guest_activation_original);
    DWORD ignored;VirtualProtect((void*)slot,8,old,&ignored);
    if(previous!=(PVOID)guest_activation_original)goto rejected;
    guest_activation_installed=1;line("{\"guest_activation_trace\":\"installed\",\"limit\":128,\"function\":\"SFXGame.SFXWeapon.ClientWeaponSet\",\"forwarding_only\":true}\r\n");return;
rejected:line("{\"guest_activation_trace\":\"not-installed\",\"reason\":\"exact-function-body-or-slot\"}\r\n");
}
static unsigned guest_activation_fixture_calls;static int guest_activation_fixture_bad;
static void *guest_activation_fixture_args[3];
static void guest_activation_fixture_forward(void *a,void *b,void *c) {
    ++guest_activation_fixture_calls;
    if(a!=guest_activation_fixture_args[0]||b!=guest_activation_fixture_args[1]||c!=guest_activation_fixture_args[2]||GetLastError()!=42)guest_activation_fixture_bad=1;
    *(int*)c=987;SetLastError(123);
}
static int guest_activation_selftest(void) {
    GuestActivationExec saved=guest_activation_original;int installed=guest_activation_installed;
    unsigned rows=guest_activation_rows;DWORD error=GetLastError();
    guest_activation_original=guest_activation_fixture_forward;guest_activation_installed=0;
    int a=1,b=2,c=3;guest_activation_fixture_args[0]=&a;guest_activation_fixture_args[1]=&b;guest_activation_fixture_args[2]=&c;
    guest_activation_fixture_calls=0;guest_activation_fixture_bad=0;SetLastError(42);guest_activation_exec(&a,&b,&c);
    int ok=c==987&&guest_activation_fixture_calls==1&&!guest_activation_fixture_bad&&GetLastError()==123;
    ok=ok&&guest_activation_matches(1,1,2,2,3,3,1)&&!guest_activation_matches(4,1,2,2,3,3,1)&&
        !guest_activation_matches(1,1,4,2,3,3,1)&&!guest_activation_matches(1,1,2,2,4,3,1)&&
        !guest_activation_matches(1,1,2,2,3,3,0)&&!guest_activation_matches(0,0,2,2,3,3,1);
    guest_activation_rows=126;ok=ok&&guest_activation_reserve()&&!guest_activation_reserve()&&guest_activation_rows==128;
    guest_activation_rows=rows;guest_activation_original=saved;guest_activation_installed=installed;SetLastError(error);return ok;
}
#endif
