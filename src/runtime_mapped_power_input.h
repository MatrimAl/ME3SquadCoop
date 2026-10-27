#ifndef ME3_RUNTIME_MAPPED_POWER_INPUT_H
#define ME3_RUNTIME_MAPPED_POWER_INPUT_H
/* Opt-in candidate at the default mode's ordinary ProcessEvent virtual.
 * Only already-decoded TryUsePower parameters are read. No FFrame parsing,
 * script-function replacement, profile edits or engine calls in the callback.
 * Actual console/gamepad alias delivery through this virtual is unverified. */
typedef struct RuntimeMappedPowerInput {
    GuestCameraStamp mode,method,pc,pawn,world,input,manager;
    uintptr_t base,slot;
    ProcessEvent original;
    int installed;
} RuntimeMappedPowerInput;
static RuntimeMappedPowerInput runtime_mapped_input;
static unsigned runtime_mapped_input_events;
static int runtime_mapped_input_enabled(void) {
    static int configured,opted;
    if(!configured){wchar_t text[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_NATIVE_MAPPED_POWERS",text,8);
        opted=n==1&&text[0]==L'1';configured=1;}
    return opted;
}
static int runtime_mapped_input_policy(int opted,int thread,int guest,int enabled,int owned,int method,int ready,int action) {
    if(!opted||!thread||!guest||!enabled||!owned||!method)return 0; /* unchanged forwarding */
    return ready&&action?2:1; /* queue, or consume an unavailable owned command */
}
static int runtime_mapped_input_method(uintptr_t base,uintptr_t fn) {
    GuestCameraStamp stamp;GameArray code={0};uintptr_t field=read_ptr(fn+0x70);int found=0;
    if(!guest_camera_stamp_read(base,fn,&stamp,1)||strcmp(stamp.full_name,"SFXGame.SFXGameModeDefault.TryUsePower")||
       !guest_player_context_function_abi(base,fn,"SFXGameModeDefault","TryUsePower",0x20202,4,65535,0,0xf52b0,1)||
       !read_at(fn+0x7c,&code,sizeof code)||!code.data||code.count!=724||code.max<724||code.max>4096)return 0;
    for(unsigned n=0;field&&n<32;++n){char name[96];int32_t offset=-1,size=0,dim=0;uint64_t flags=0;
        if(!is_live(base,field)||read_ptr(field+0x40)!=fn)return 0;
        object_name(base,field,name,sizeof name);
        if(!strcmp(name,"Power")){
            if(found++||!narrative_probe_object(base,field,"IntProperty")||!read_at(field+0x94,&offset,4)||offset||
               !read_at(field+0x74,&size,4)||size!=4||!read_at(field+0x70,&dim,4)||dim!=1||
               !read_at(field+0x78,&flags,8)||flags!=0x80)return 0;
        }
        field=read_ptr(field+0x68);
    }
    return !field&&found==1&&guest_camera_stamp_matches(base,&stamp,1);
}
static int runtime_mapped_input_mode(uintptr_t base,SessionContext c,uintptr_t mode,uintptr_t *manager_out) {
    uintptr_t manager=read_ptr(c.pc+0xc38);GameArray modes={0},again={0};unsigned char current=255;
    uint32_t disabled=1,flags=0;char name[96];
    if(!narrative_probe_object(base,manager,"SFXGameModeManager")||read_ptr(manager+0x40)!=c.pc||
       !runtime_power_property(base,c.pc,"BioPlayerController","GameModeManager2","ObjectProperty",0xc38,8)||
       !runtime_power_property(base,manager,"SFXGameModeManager","GameModes","ArrayProperty",0x70,16)||
       !runtime_power_property(base,manager,"SFXGameModeManager","CurrentMode","ByteProperty",0xa8,1)||
       !runtime_power_property(base,manager,"SFXGameModeManager","bDisableInput","BoolProperty",0xa4,4)||
       !read_at(manager+0xa8,&current,1)||current||!read_at(manager+0xa4,&disabled,4)||(disabled&1)||
       !read_at(manager+0x70,&modes,sizeof modes)||!modes.data||modes.count<1||modes.count>64||
       modes.max<modes.count||modes.max>128||read_ptr(modes.data)!=mode||
       !narrative_probe_object(base,mode,"SFXGameModeDefault")||read_ptr(mode+0x40)!=c.pc||
       !read_at(mode+0xd0,&flags,4)||!(flags&2)||
       !read_at(manager+0x70,&again,sizeof again)||memcmp(&modes,&again,sizeof modes))return 0;
    object_name(base,read_ptr(mode+0x50),name,sizeof name);
    if(strcmp(name,"SFXGameModeDefault"))return 0;
    *manager_out=manager;return 1;
}
static void runtime_mapped_input_log(const char *reason,int action,int accepted) {
    if(runtime_mapped_input_events++>=64)return;
    char row[330];snprintf(row,sizeof row,"{\"runtime_mapped_power_input\":\"%s\",\"action\":%d,\"queued\":%s,\"native_alias_delivery_verified\":false,\"cast_verified\":false,\"monotonic_ms\":%lu}\r\n",
        reason,action,accepted?"true":"false",GetTickCount());line(row);
}
static int runtime_mapped_input_consume(void *self,void *function,void *parameters) {
    RuntimeMappedPowerInput *q=&runtime_mapped_input;uintptr_t base=q->base,mode=(uintptr_t)self;
    if(!q->installed||!runtime_mapped_input_enabled()||GetCurrentThreadId()!=session_game_thread||
       g_runtime_config.role!=ME3_ROLE_GUEST||!runtime_control_enabled||!runtime_scope_ready||
       !InterlockedCompareExchange(&g_enable_requested,0,0)||mode!=q->mode.pointer||
       (uintptr_t)function!=q->method.pointer)return 0;
    SessionContext c=session_context(session_engine);
    /* Released/foreign control and another mode preserve their stock behavior. */
    if(c.pc!=q->pc.pointer||c.pawn!=q->pawn.pointer||c.world!=q->world.pointer||
       !guest_camera_stamp_matches(base,&q->mode,1)||!guest_camera_stamp_matches(base,&q->method,1)||
       !guest_camera_stamp_matches(base,&q->pc,1)||!guest_camera_stamp_matches(base,&q->pawn,1)||
       !guest_camera_stamp_matches(base,&q->world,1)||
       !guest_player_context_registered_for(c.pc,c.pawn)||!guest_player_context_basic_owned(base,c.pc,c.pawn,c.world)||
       read_ptr(mode+0x40)!=c.pc||!runtime_mapped_input_method(base,(uintptr_t)function))return 0;
    uintptr_t manager=0;RuntimePowerCatalog current;int action=-1;
    unsigned scope=(unsigned)InterlockedCompareExchange(&runtime_power_scope,0,0);
    int ready=!session_engine_blocked(c)&&runtime_mapped_input_mode(base,c,mode,&manager)&&manager==q->manager.pointer&&
        guest_camera_stamp_matches(base,&q->manager,1)&&guest_camera_stamp_matches(base,&q->input,1)&&
        read_ptr(c.pc+0x6dc)==q->input.pointer&&read_ptr(q->input.pointer+0x40)==c.pc&&scope&&
        runtime_power_read_catalog(base,c,1,&current)&&runtime_power_catalog_same(&runtime_power_catalog,&current);
    int readable=parameters&&read_at((uintptr_t)parameters,&action,4);
    int policy=runtime_mapped_input_policy(1,1,1,1,1,1,ready,readable&&action>=0&&runtime_power_mapped_action((unsigned)action));
    int accepted=policy==2&&runtime_request_mapped_power(scope,(unsigned)action);
    runtime_mapped_input_log(accepted?"native-command-queued":!ready?"owned-command-not-ready":
        !readable?"owned-parameters-unreadable":policy!=2?"owned-action-unsupported":"owned-queue-busy",action,accepted);
    return 1;
}
static void runtime_mapped_process_event(void *self,void *function,void *parameters,void *result) {
    DWORD error=GetLastError();int consumed=runtime_mapped_input_consume(self,function,parameters);SetLastError(error);
    if(!consumed&&runtime_mapped_input.original)runtime_mapped_input.original(self,function,parameters,result);
}
static void runtime_mapped_power_input_prepare(uintptr_t base,SessionContext c,int ordinary) {
    RuntimeMappedPowerInput *q=&runtime_mapped_input;
    if(!runtime_mapped_input_enabled()||g_runtime_config.role!=ME3_ROLE_GUEST||!ordinary||
       !runtime_control_enabled||!runtime_scope_ready||!guest_player_context_registered_for(c.pc,c.pawn)||
       !guest_player_context_basic_owned(base,c.pc,c.pawn,c.world))return;
    uintptr_t manager=read_ptr(c.pc+0xc38);GameArray modes={0};
    if(!read_at(manager+0x70,&modes,sizeof modes)||!modes.data||modes.count<1||modes.count>64||
       modes.max<modes.count||modes.max>128)return;
    uintptr_t mode=read_ptr(modes.data),checked=0;
    if(!runtime_mapped_input_mode(base,c,mode,&checked)||checked!=manager)return;
    uintptr_t fn=guest_player_context_find_method(base,read_ptr(mode+0x50),"SFXGameModeDefault","TryUsePower");
    RuntimePowerCatalog catalog;
    if(!runtime_mapped_input_method(base,fn)||!runtime_power_read_catalog(base,c,1,&catalog)||
       !runtime_power_catalog_same(&runtime_power_catalog,&catalog))return;
    uintptr_t table=read_ptr(mode),slot=table+0x230;
    if(table<base||table>=base+0x1800000||(table&7))return;
    static const unsigned char entry[14]={0x40,0x55,0x41,0x56,0x41,0x57,0x48,0x81,0xec,0x90,0,0,0,0x48};
    unsigned char bytes[sizeof entry];
    if(!read_at(base+0xf4f80,bytes,sizeof bytes)||memcmp(bytes,entry,sizeof entry)||
       (q->installed?(q->base!=base||q->slot!=slot||read_ptr(slot)!=(uintptr_t)runtime_mapped_process_event):read_ptr(slot)!=base+0xf4f80))return;
    RuntimeMappedPowerInput next=*q;
    if(!guest_camera_stamp_read(base,mode,&next.mode,1)||!guest_camera_stamp_read(base,fn,&next.method,1)||
       !guest_camera_stamp_read(base,c.pc,&next.pc,1)||!guest_camera_stamp_read(base,c.pawn,&next.pawn,1)||
       !guest_camera_stamp_read(base,c.world,&next.world,1)||!guest_camera_stamp_read(base,catalog.input.pointer,&next.input,1)||
       !guest_camera_stamp_read(base,manager,&next.manager,1))return;
    if(!q->installed){
        DWORD old=0;if(!VirtualProtect((void*)slot,8,PAGE_READWRITE,&old))return;
        next.base=base;next.slot=slot;next.original=(ProcessEvent)(base+0xf4f80);*q=next;
        PVOID previous=InterlockedCompareExchangePointer((PVOID volatile*)slot,(PVOID)runtime_mapped_process_event,(PVOID)next.original);
        DWORD ignored;VirtualProtect((void*)slot,8,old,&ignored);
        if((uintptr_t)previous!=(uintptr_t)next.original)return;
        next.installed=1;runtime_mapped_input_log("process-event-candidate-installed",-1,0);
    }
    *q=next;
}
static int runtime_mapped_input_fixture_calls;
static void runtime_mapped_input_fixture_forward(void *self,void *function,void *parameters,void *result) {
    if(self==(void*)1&&function==(void*)2&&parameters==(void*)3&&result==(void*)4&&GetLastError()==73)++runtime_mapped_input_fixture_calls;
    SetLastError(123);
}
static int runtime_mapped_power_input_selftest(void) {
    if(runtime_mapped_input.installed)return 0;
    RuntimeMappedPowerInput saved=runtime_mapped_input;DWORD error=GetLastError();
    runtime_mapped_input.original=runtime_mapped_input_fixture_forward;runtime_mapped_input_fixture_calls=0;
    SetLastError(73);runtime_mapped_process_event((void*)1,(void*)2,(void*)3,(void*)4);
    int ok=runtime_mapped_input_fixture_calls==1&&GetLastError()==123;
    runtime_mapped_input=saved;SetLastError(error);
    if(!ok||runtime_mapped_input_policy(1,1,1,1,1,1,1,1)!=2||
       runtime_mapped_input_policy(1,1,1,1,1,1,0,1)!=1||runtime_mapped_input_policy(1,1,1,1,1,1,1,0)!=1)return 0;
    for(unsigned i=0;i<6;++i){int gate[6]={1,1,1,1,1,1};gate[i]=0;
        if(runtime_mapped_input_policy(gate[0],gate[1],gate[2],gate[3],gate[4],gate[5],1,1))return 0;}
    return 1;
}
#endif
