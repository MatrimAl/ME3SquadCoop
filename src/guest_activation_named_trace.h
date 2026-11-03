#ifndef ME3_GUEST_ACTIVATION_NAMED_TRACE_H
#define ME3_GUEST_ACTIVATION_NAMED_TRACE_H
/* Earlier observation than weapon readiness/UFunction.Func. Only opcode65's
 * native forwarding table entry is leased. Never decode arguments, resolve a
 * function by invoking engine lookup, or modify frame.Code/locals ourselves.
 * A hit proves an admitted named call, not its selected state override. */
static GuestActivationExec activation_named_original;
static uintptr_t activation_named_base;
static DWORD activation_named_thread;
static int activation_named_installed,activation_named_attempted;
static unsigned activation_named_rows,activation_named_calls;
static int activation_named_selector(unsigned slot,const char *name,unsigned number) {
    return slot==223&&!number&&!strcmp(name,"ClientWeaponSet");
}
static int activation_named_owner(uintptr_t base,uintptr_t self,uintptr_t *pc,uintptr_t *pawn) {
    NetworkActorScope world;
    *pc=local_controller(read_ptr(base+0x18b31d0));*pawn=read_ptr(*pc+0x3bc);
    return guest_camera_actor_live(base,self)&&is_kind_of(base,self,"SFXWeapon")&&
        guest_camera_actor_live(base,*pc)&&is_kind_of(base,*pc,"PlayerController")&&
        guest_camera_actor_live(base,*pawn)&&is_kind_of(base,*pawn,"SFXPawn_Ashley")&&
        read_ptr(*pawn+0x374)==*pc&&read_ptr(*pawn+0x198)==read_ptr(*pc+0x198)&&
        (read_ptr(*pawn+0x41c)==self||read_ptr(self+0x190)==*pawn)&&
        network_actor_read_scope(base,self,&world)&&world.info==read_ptr(*pawn+0x198);
}
static void activation_named_exec(void *self,void *frame,void *result) {
    DWORD before=GetLastError();uintptr_t pc=0,pawn=0,base=activation_named_base;
    unsigned id=0;uintptr_t code=0;unsigned char selector[10];char name[96]={0};
    int capture=activation_named_installed&&guest_activation_opted()&&
        GetCurrentThreadId()==activation_named_thread&&activation_named_rows<=126&&
        read_at((uintptr_t)frame+0x28,&code,8)&&read_at(code,selector,sizeof selector);
    if(capture){uint32_t names[2];uint16_t slot;memcpy(names,selector,8);memcpy(&slot,selector+8,2);
        if(slot!=223||names[1])capture=0;
        else {name_of(base,names[0],name,sizeof name);capture=activation_named_selector(slot,name,names[1]);}
    }
    /* File/object discovery is performed only after the narrow selector,
     * never on every unrelated NamedFunction call. */
    capture=capture&&has_marker(root)&&activation_named_owner(base,(uintptr_t)self,&pc,&pawn);
    GuestCameraStamp weapon={0},controller={0},owner={0},info={0};char parent[600]={0},safe[1200],state[96]={0},state_safe[200];
    if(capture)capture=guest_camera_stamp_read(base,(uintptr_t)self,&weapon,0)&&
        guest_camera_stamp_read(base,pc,&controller,0)&&guest_camera_stamp_read(base,pawn,&owner,0)&&
        guest_camera_stamp_read(base,read_ptr(pawn+0x198),&info,0);
    if(capture){
        id=++activation_named_calls;activation_named_rows+=2;
        uintptr_t node=read_ptr((uintptr_t)frame+0x18),target=0;GameArray table={0};char path[600]={0},target_safe[1200],row[2400];
        if(is_live(base,node))identity_path(base,node,parent,sizeof parent);identity_quote(parent,safe,sizeof safe);
        if(read_at(weapon.klass+0x144,&table,sizeof table)&&table.data&&table.count>223&&table.max>=table.count&&table.max<=4096){
            target=read_ptr(table.data+223*8);
            if(!native_script_slot_matches(base,weapon.klass,223,target))target=0;
        }
        if(is_live(base,target))identity_path(base,target,path,sizeof path);identity_quote(path,target_safe,sizeof target_safe);
        int known=native_state_name(base,(uintptr_t)self,state,sizeof state);identity_quote(state,state_safe,sizeof state_safe);
        NativeRpcReceiveFrame *incoming=native_rpc_receive_current(activation_named_thread);
        snprintf(row,sizeof row,"{\"guest_activation_named\":\"enter\",\"call\":%u,\"slot\":223,\"parent\":\"%s\",\"class_target\":\"%s\",\"frame_object_matches\":%s,\"weapon_index\":%d,\"pawn_index\":%d,\"inventory\":\"0x%llx\",\"state_known\":%s,\"state\":\"%s\",\"native_receive_present\":%s,\"incoming_channel_actor_matches\":%s,\"state_override_observed\":false,\"original_forwarded\":false,\"monotonic_ms\":%lu}\r\n",
            id,safe,target_safe,read_ptr((uintptr_t)frame+0x20)==(uintptr_t)self?"true":"false",weapon.index,owner.index,
            (unsigned long long)read_ptr(pawn+0x414),known?"true":"false",state_safe,
            incoming?"true":"false",incoming&&incoming->channel&&read_ptr(incoming->channel+0xa0)==(uintptr_t)self?"true":"false",GetTickCount());line(row);
    }
    SetLastError(before);activation_named_original(self,frame,result);DWORD after=GetLastError();
    if(capture){
        int stable=guest_camera_stamp_matches(base,&weapon,0)&&guest_camera_stamp_matches(base,&controller,0)&&
            guest_camera_stamp_matches(base,&owner,0)&&guest_camera_stamp_matches(base,&info,0)&&
            read_ptr(pc+0x3bc)==pawn&&read_ptr(pawn+0x374)==pc;
        char row[900],state_after[96]={0},quoted[200];int known=stable&&native_state_name(base,(uintptr_t)self,state_after,sizeof state_after);
        identity_quote(state_after,quoted,sizeof quoted);
        snprintf(row,sizeof row,"{\"guest_activation_named\":\"leave\",\"call\":%u,\"scope_stable\":%s,\"state_known\":%s,\"state\":\"%s\",\"code_advance_bytes\":%lld,\"original_forwarded\":true,\"activation_verified\":false,\"monotonic_ms\":%lu}\r\n",
            id,stable?"true":"false",known?"true":"false",quoted,(long long)(read_ptr((uintptr_t)frame+0x28)-code),GetTickCount());line(row);
    }
    SetLastError(after);
}
static void activation_named_prepare(uintptr_t base) {
    if(!guest_activation_opted()||activation_named_attempted||!has_marker(root))return;
    activation_named_attempted=1;
    static const unsigned char body[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x10,0x57,0x48,0x83,0xec,0x20,
        0x48,0x8b,0x42,0x28,0x48,0x8b,0xfa,0x49,0x8b,0xd8,0x48,0x8b,0xf1,0x45,0x33,0xc0,0x48,0x8b,0x10,
        0x48,0x83,0xc0,0x08,0x48,0x89,0x47,0x28,0x44,0x0f,0xb7,0x08};
    unsigned char got[sizeof body];uintptr_t slot=base+0x17f4ed8,expected=base+0xe9e20;
    if(!supported_image(base)||!read_at(expected,got,sizeof got)||memcmp(got,body,sizeof body)||read_ptr(slot)!=expected)goto rejected;
    DWORD old;if(!VirtualProtect((void*)slot,8,PAGE_READWRITE,&old))goto rejected;
    activation_named_base=base;activation_named_thread=GetCurrentThreadId();activation_named_original=(GuestActivationExec)expected;
    PVOID previous=InterlockedCompareExchangePointer((PVOID volatile*)slot,(PVOID)activation_named_exec,(PVOID)expected);
    DWORD ignored;VirtualProtect((void*)slot,8,old,&ignored);
    if((uintptr_t)previous!=expected)goto rejected;
    activation_named_installed=1;char row[400];snprintf(row,sizeof row,"{\"guest_activation_named\":\"installed\",\"before_equipment_readiness\":true,\"limit\":128,\"monotonic_ms\":%lu}\r\n",GetTickCount());line(row);return;
rejected:line("{\"guest_activation_named\":\"not-installed\",\"reason\":\"exact-opcode-handler-or-slot\"}\r\n");
}
static int activation_named_selftest(void) {
    GuestActivationExec saved=activation_named_original;int installed=activation_named_installed;DWORD error=GetLastError();
    activation_named_installed=0;activation_named_original=guest_activation_fixture_forward;
    int a=1,b=2,c=3;guest_activation_fixture_args[0]=&a;guest_activation_fixture_args[1]=&b;guest_activation_fixture_args[2]=&c;
    guest_activation_fixture_calls=0;guest_activation_fixture_bad=0;SetLastError(42);activation_named_exec(&a,&b,&c);
    int ok=c==987&&guest_activation_fixture_calls==1&&!guest_activation_fixture_bad&&GetLastError()==123&&
        activation_named_selector(223,"ClientWeaponSet",0)&&!activation_named_selector(222,"ClientWeaponSet",0)&&
        !activation_named_selector(223,"Other",0)&&!activation_named_selector(223,"ClientWeaponSet",1);
    activation_named_original=saved;activation_named_installed=installed;SetLastError(error);return ok;
}
#endif
