#ifndef ME3_FIRE_RPC_TRACE_H
#define ME3_FIRE_RPC_TRACE_H
/* Research-only script-entry observations for selected squad weapon calls.
 * Exact-build reflection includes class and state overrides; observing only
 * Weapon.ServerStartFire would miss Inactive.ServerStartFire. Shared native
 * execScriptFunction has the already pinned void(self,FFrame*,result) ABI.
 * No call is consumed, redirected or manufactured. Entry is not delivery:
 * only an exact native receive/frame/channel match labels an incoming RPC.
 */
typedef void (*FireRpcExec)(void*,void*,void*);
typedef struct FireRpcDefinition { const char *path;uint32_t flags;int length; } FireRpcDefinition;
static const FireRpcDefinition fire_rpc_definitions[]={
    {"Engine.Weapon.StartFire",0x20102,119},
    {"Engine.Weapon.ServerStartFire",0x2200c2,78},
    {"Engine.Weapon.BeginFire",0x20102,167},
    {"Engine.Weapon.ServerStopFire",0x2200c2,24},
    {"Engine.Weapon.EndFire",0x20102,24},
    {"Engine.Weapon.Inactive.ServerStartFire",0x2200c2,1208},
    {"Engine.Weapon.Inactive.ServerStopFire",0x2200c2,24},
    {"Engine.Weapon.Inactive.StartFire",0x20100,3},
    {"Engine.Weapon.Active.BeginFire",0x20102,127},
    {"SFXGame.SFXWeapon.Active.BeginFire",0x20102,295},
    {"SFXGame.SFXWeapon.Active.EndFire",0x20102,100},
    {"SFXGame.SFXWeapon.WeaponFiring_SemiAuto.EndFire",0x20102,60},
    {"SFXGame.SFXWeapon.EndFire",0x20102,34},
    {"SFXGame.SFXWeapon.StartFire",0x20102,155},
    {"SFXGame.SFXWeapon.ServerStopFire",0x2200c2,229}
};
#define FIRE_RPC_COUNT 15
static GuestPlayerContextMethod fire_rpc_methods[FIRE_RPC_COUNT];
static FireRpcExec fire_rpc_original;
static uintptr_t fire_rpc_base;
static int fire_rpc_enabled=-1,fire_rpc_installed,fire_rpc_attempted;
static unsigned fire_rpc_rows,fire_rpc_calls,fire_rpc_parent;
static int fire_rpc_opted(void) {
    if(fire_rpc_enabled<0){wchar_t value[8]={0};fire_rpc_enabled=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_FIRE_RPC_TRACE",value,8)==1&&value[0]==L'1';}
    return fire_rpc_enabled&&progress_enabled&&(test_action==5||test_action==6);
}
static int fire_rpc_take(void) { if(fire_rpc_rows>=256)return 0;++fire_rpc_rows;return 1; }
static int fire_rpc_receive_matches(uintptr_t channel,uintptr_t expected,uintptr_t node,uintptr_t fn,
                                    uintptr_t object,uintptr_t self,int net) {
    return net&&channel&&channel==expected&&node==fn&&object==self;
}
static void fire_rpc_snapshot(int k,const char *stage,unsigned id,unsigned parent,int incoming,
                              int frame_matches,int object_matches,unsigned long generation) {
    if(!fire_rpc_take())return;
    AmmoWireScope *s=&ammo_wire_scope;int valid=s->generation==generation&&ammo_wire_scope_valid();
    int used=-1,spare=-1;uint32_t nofire=0;unsigned char bfire=255,mode=255,role=255;char state[96]={0},safe[200];
    if(valid){
        valid=read_at(s->weapon.pointer+0x3b8,&used,4)&&read_at(s->weapon.pointer+0x3b4,&spare,4)&&
            read_at(s->pawn.pointer+0x578,&nofire,4)&&read_at(s->pc.pointer+0x480,&bfire,1)&&
            read_at(s->weapon.pointer+0x3b0,&mode,1)&&read_at(s->weapon.pointer+0x26a,&role,1);
    }
    int state_known=valid&&native_state_name(s->base,s->weapon.pointer,state,sizeof state);
    identity_quote(state,safe,sizeof safe);char row[1400];
    snprintf(row,sizeof row,"{\"fire_rpc_trace\":\"%s\",\"side\":\"%s\",\"call\":%u,\"parent_call\":%u,\"function\":\"%s\",\"generation\":%lu,\"channel_index\":%d,\"incoming_native_rpc\":%s,\"frame_node_matches\":%s,\"frame_object_matches\":%s,\"snapshot_valid\":%s,\"used\":%d,\"spare\":%d,\"bfire\":%u,\"pawn_no_weapon_firing\":%s,\"fire_mode\":%u,\"weapon_role\":%u,\"state_known\":%s,\"state\":\"%s\",\"original_forwarded\":%s,\"monotonic_ms\":%lu}\r\n",
        stage,test_action==6?"host":"guest",id,parent,fire_rpc_definitions[k].path,generation,s->channel.index,
        incoming?"true":"false",frame_matches?"true":"false",object_matches?"true":"false",valid?"true":"false",
        used,spare,(unsigned)bfire,valid?(nofire&0x10000?"true":"false"):"null",(unsigned)mode,(unsigned)role,
        state_known?"true":"false",safe,!strcmp(stage,"leave")?"true":"false",GetTickCount());line(row);
}
static void fire_rpc_exec(int k,void *self,void *frame,void *result) {
    DWORD before=GetLastError();AmmoWireScope *s=&ammo_wire_scope;
    int capture=fire_rpc_installed&&GetCurrentThreadId()==ammo_wire_thread&&fire_rpc_rows<256&&
        (uintptr_t)self==s->weapon.pointer&&ammo_wire_scope_valid()&&
        guest_player_context_method_matches(s->base,&fire_rpc_methods[k],0);
    unsigned id=0,parent=fire_rpc_parent;unsigned long generation=s->generation;int incoming=0,nm=0,om=0;
    if(capture){
        uintptr_t node=read_ptr((uintptr_t)frame+0x18),object=read_ptr((uintptr_t)frame+0x20);
        nm=node==fire_rpc_methods[k].function.pointer;om=object==(uintptr_t)self;
        incoming=test_action==6&&ammo_wire_receive&&fire_rpc_receive_matches(ammo_wire_receive->channel,
            s->channel.channel,node,fire_rpc_methods[k].function.pointer,object,(uintptr_t)self,
            !!(fire_rpc_definitions[k].flags&0x40));
        id=++fire_rpc_calls;fire_rpc_parent=id;fire_rpc_snapshot(k,"enter",id,parent,incoming,nm,om,generation);
    }
    SetLastError(before);
    fire_rpc_original(self,frame,result);
    DWORD after=GetLastError();
    if(capture){fire_rpc_snapshot(k,"leave",id,parent,incoming,nm,om,generation);fire_rpc_parent=parent;}
    SetLastError(after);
}
#define FIRE_RPC_WRAPPER(N) static void fire_rpc_exec_##N(void *a,void *b,void *c){fire_rpc_exec(N,a,b,c);}
FIRE_RPC_WRAPPER(0) FIRE_RPC_WRAPPER(1) FIRE_RPC_WRAPPER(2) FIRE_RPC_WRAPPER(3) FIRE_RPC_WRAPPER(4)
FIRE_RPC_WRAPPER(5) FIRE_RPC_WRAPPER(6) FIRE_RPC_WRAPPER(7) FIRE_RPC_WRAPPER(8) FIRE_RPC_WRAPPER(9)
FIRE_RPC_WRAPPER(10) FIRE_RPC_WRAPPER(11) FIRE_RPC_WRAPPER(12) FIRE_RPC_WRAPPER(13) FIRE_RPC_WRAPPER(14)
#undef FIRE_RPC_WRAPPER
static const FireRpcExec fire_rpc_wrappers[FIRE_RPC_COUNT]={
    fire_rpc_exec_0,fire_rpc_exec_1,fire_rpc_exec_2,fire_rpc_exec_3,fire_rpc_exec_4,
    fire_rpc_exec_5,fire_rpc_exec_6,fire_rpc_exec_7,fire_rpc_exec_8,fire_rpc_exec_9,
    fire_rpc_exec_10,fire_rpc_exec_11,fire_rpc_exec_12,fire_rpc_exec_13,fire_rpc_exec_14};
static int fire_rpc_validate(int k) {
    GuestPlayerContextMethod *m=&fire_rpc_methods[k];uintptr_t fn=m->function.pointer;
    uint32_t flags=0;uint16_t size=0,ret=0,index=1;unsigned char num=0;GameArray script={0};
    if(!guest_player_context_method_matches(fire_rpc_base,m,1)||
       !narrative_probe_object(fire_rpc_base,fn,"Function")||strcmp(m->function.full_name,fire_rpc_definitions[k].path)||
       !read_at(fn+0xe0,&flags,4)||flags!=fire_rpc_definitions[k].flags||
       !read_at(fn+0xe6,&size,2)||size!=1||!read_at(fn+0xe8,&ret,2)||ret!=65535||
       !read_at(fn+0xe4,&index,2)||index||!read_at(fn+0xea,&num,1)||num!=1||
       read_ptr(fn+0xd8)!=fire_rpc_base+0xf52b0||!read_at(fn+0x7c,&script,sizeof script)||
       script.count!=fire_rpc_definitions[k].length||script.max<script.count||script.max>4096||!script.data)return 0;
    uintptr_t prop=read_ptr(fn+0x70);char path[180];snprintf(path,sizeof path,"%s.FireModeNum",fire_rpc_definitions[k].path);
    return read_ptr(prop+0x40)==fn&&!read_ptr(prop+0x68)&&combat_fire_property(fire_rpc_base,prop,"ByteProperty",path,0,0x80);
}
static int fire_rpc_collect(uintptr_t parent,int states,unsigned *found) {
    uintptr_t child=read_ptr(parent+0x70),seen[1024];int count=0;
    while(child&&count<1024){
        if(!is_live(fire_rpc_base,child)||read_ptr(child+0x40)!=parent)return 0;
        for(int i=0;i<count;++i)if(seen[i]==child)return 0;seen[count++]=child;
        char name[96];object_name(fire_rpc_base,child,name,sizeof name);
        if(!strcmp(name,"StartFire")||!strcmp(name,"ServerStartFire")||!strcmp(name,"ServerStopFire")||!strcmp(name,"BeginFire")||!strcmp(name,"EndFire")){
            GuestPlayerContextMethod m;
            if(!guest_player_context_method_stamp(fire_rpc_base,child,&m))return 0;
            for(int k=0;k<FIRE_RPC_COUNT;++k)if(!strcmp(m.function.full_name,fire_rpc_definitions[k].path)){
                if(*found&(1u<<k))return 0;fire_rpc_methods[k]=m;*found|=1u<<k;
            }
        }
        if(states&&(!strcmp(name,"Inactive")||!strcmp(name,"Active")||!strcmp(name,"WeaponFiring_SemiAuto"))&&
           narrative_probe_object(fire_rpc_base,child,"State")&&!fire_rpc_collect(child,0,found))return 0;
        child=read_ptr(child+0x68);
    }
    return !child;
}
static void fire_rpc_trace_prepare(void) {
    if(!fire_rpc_opted()||fire_rpc_attempted||!ammo_wire_scope_valid())return;
    fire_rpc_attempted=1;AmmoWireScope *s=&ammo_wire_scope;fire_rpc_base=s->base;
    uintptr_t klass=s->weapon.klass;unsigned found=0;int installed=0;
    for(int depth=0;klass&&depth<32;++depth,klass=read_ptr(klass+0x60)){
        if(!narrative_probe_object(s->base,klass,"Class"))goto rejected;
        char path[600];identity_path(s->base,klass,path,sizeof path);
        if((!strcmp(path,"Engine.Weapon")||!strcmp(path,"SFXGame.SFXWeapon"))&&!fire_rpc_collect(klass,1,&found))goto rejected;
    }
    if(klass||found!=((1u<<FIRE_RPC_COUNT)-1)||!guest_equipment_dispatch_pins(s->base))goto rejected;
    for(int k=0;k<FIRE_RPC_COUNT;++k)if(!fire_rpc_validate(k))goto rejected;
    fire_rpc_original=(FireRpcExec)(s->base+0xf52b0);
    for(int k=0;k<FIRE_RPC_COUNT;++k){
        if(InterlockedCompareExchangePointer((PVOID volatile*)(fire_rpc_methods[k].function.pointer+0xd8),
            (PVOID)fire_rpc_wrappers[k],(PVOID)fire_rpc_original)!=(PVOID)fire_rpc_original)goto rejected;
        ++installed;
    }
    fire_rpc_installed=1;line("{\"fire_rpc_trace\":\"installed\",\"methods\":15,\"limit\":256,\"observational\":true}\r\n");return;
rejected:
    for(int k=0;k<installed;++k)InterlockedCompareExchangePointer((PVOID volatile*)(fire_rpc_methods[k].function.pointer+0xd8),
        (PVOID)fire_rpc_original,(PVOID)fire_rpc_wrappers[k]);
    line("{\"fire_rpc_trace\":\"not-installed\",\"reason\":\"exact-method-or-native-pins\"}\r\n");
}
static void *fire_rpc_fixture_args[3];static int fire_rpc_fixture_calls,fire_rpc_fixture_bad;
static void fire_rpc_fixture_forward(void *a,void *b,void *c) {
    ++fire_rpc_fixture_calls;
    if(a!=fire_rpc_fixture_args[0]||b!=fire_rpc_fixture_args[1]||c!=fire_rpc_fixture_args[2]||GetLastError()!=42)fire_rpc_fixture_bad=1;
    *(int*)c=987;SetLastError(123);
}
static int fire_rpc_selftest(void) {
    FireRpcExec saved=fire_rpc_original;int installed=fire_rpc_installed;unsigned rows=fire_rpc_rows;DWORD error=GetLastError();
    int a=1,b=2,c=3;fire_rpc_fixture_args[0]=&a;fire_rpc_fixture_args[1]=&b;fire_rpc_fixture_args[2]=&c;
    fire_rpc_installed=0;fire_rpc_original=fire_rpc_fixture_forward;fire_rpc_fixture_calls=fire_rpc_fixture_bad=0;
    SetLastError(42);fire_rpc_exec_0(&a,&b,&c);
    int ok=fire_rpc_fixture_calls==1&&!fire_rpc_fixture_bad&&c==987&&GetLastError()==123;
    fire_rpc_rows=255;ok=ok&&fire_rpc_take()&&!fire_rpc_take()&&fire_rpc_rows==256;
    ok=ok&&fire_rpc_receive_matches(1,1,2,2,3,3,1)&&!fire_rpc_receive_matches(4,1,2,2,3,3,1)&&
        !fire_rpc_receive_matches(1,1,4,2,3,3,1)&&!fire_rpc_receive_matches(1,1,2,2,4,3,1)&&
        !fire_rpc_receive_matches(1,1,2,2,3,3,0)&&!fire_rpc_receive_matches(0,0,2,2,3,3,1);
    fire_rpc_rows=rows;fire_rpc_original=saved;fire_rpc_installed=installed;SetLastError(error);return ok;
}
#endif
