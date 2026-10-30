#ifndef ME3_RUNTIME_POWER_RELOAD_H
#define ME3_RUNTIME_POWER_RELOAD_H
/* Only reload uses the complete stock mode event. Idle orders keep their
 * established explicit native acceptance/ApplyTacticalOrders path. */
#pragma pack(push,4)
typedef struct RuntimeReloadPowerParms {uint32_t name[2];uintptr_t pawn;} RuntimeReloadPowerParms;
#pragma pack(pop)
_Static_assert(sizeof(RuntimeReloadPowerParms)==16,"UsePower parameters");
_Static_assert(offsetof(RuntimeReloadPowerParms,pawn)==8,"UsePower user offset");
static int runtime_power_reload_method(uintptr_t base,uintptr_t mode,GuestPlayerContextMethod *out) {
    uintptr_t fn=guest_player_context_find_method(base,read_ptr(mode+0x50),"SFXGameModeDefault","UsePower");
    GameArray script={0};GuestCameraStamp target={0};unsigned found=0;
    if(!guest_player_context_method_stamp(base,fn,out)||
       !guest_player_context_function_abi(base,fn,"SFXGameModeDefault","UsePower",0x20202,16,65535,0,0xf52b0,2)||
       !read_at(fn+0x7c,&script,sizeof script)||!script.data||script.count!=515||script.max<script.count||script.max>4096)return 0;
    uintptr_t field=read_ptr(fn+0x70);
    for(unsigned n=0;field&&n<16;++n){char name[96];int32_t offset=-1,size=0,dim=0;uint64_t flags=0;
        if(!is_live(base,field)||read_ptr(field+0x40)!=fn)return 0;
        object_name(base,field,name,sizeof name);
        int index=!strcmp(name,"PowerName")?0:!strcmp(name,"User")?1:-1;
        if(index>=0){
            if((found&(1u<<index))||!narrative_probe_object(base,field,index?"ObjectProperty":"NameProperty")||
               !read_at(field+0x94,&offset,4)||offset!=(index?8:0)||!read_at(field+0x74,&size,4)||size!=8||
               !read_at(field+0x70,&dim,4)||dim!=1||!read_at(field+0x78,&flags,8)||flags!=0x80)return 0;
            if(index&&(!guest_camera_stamp_read(base,read_ptr(field+0xd0),&target,1)||
                strcmp(target.full_name,"SFXGame.SFXPawn")))return 0;
            found|=1u<<index;
        }
        if(!read_at(field+0x68,&field,8))return 0;
    }
    return !field&&found==3&&guest_player_context_method_matches(base,out,1);
}
static int runtime_power_reload_dispatch(ProcessEvent stock,uintptr_t mode,uintptr_t method,
        const uint32_t name[2],uintptr_t pawn,int current,ULONGLONG submitted) {
    DWORD error=GetLastError();
    int allowed=current&&stock&&mode&&method&&pawn&&name[0]&&!runtime_power_request_expired(submitted);
    SetLastError(error);if(!allowed)return 0;
    RuntimeReloadPowerParms parms={{name[0],name[1]},pawn};
    stock((void*)mode,(void*)method,&parms,NULL);return 1;
}
static int runtime_power_reload_use(uintptr_t base,uintptr_t engine,const RuntimePowerCatalog *submitted,unsigned index,
        int mapped,unsigned requested,ULONGLONG at) {
    SessionContext c=session_context(engine);RuntimePowerCatalog current;
    NetworkActorIdentityEntry channel={0};CombatContextChain chain;GuestPlayerContextMethod method={0};
    if(!runtime_control_enabled||!runtime_scope_ready||g_runtime_config.role!=ME3_ROLE_GUEST||
       !InterlockedCompareExchange(&g_enable_requested,0,0)||session_engine_blocked(c)||
       !runtime_power_read_catalog(base,c,1,&current)||!runtime_power_catalog_same(submitted,&current)||
       index>=(unsigned)current.count||runtime_power_wheel_input_blocked(base,c)||
       !guest_player_context_conditions(base,c.pc,c.pawn,c.world,1,1,0,&channel,&chain))return 0;
    uintptr_t reload=runtime_power_reload_instance(base,c),manager=read_ptr(c.pc+0xc38);GameArray modes={0};
    if(!reload||!read_at(manager+0x70,&modes,sizeof modes)||!modes.data||modes.count<1||modes.count>64||
       modes.max<modes.count||modes.max>128)return 0;
    uintptr_t mode=read_ptr(modes.data),checked_manager=0;uint32_t permissions=0,combat=2;
    GuestCameraStamp mode_stamp={0},manager_stamp={0},reload_stamp={0};
    if(!runtime_mapped_input_mode(base,c,mode,&checked_manager)||checked_manager!=manager||
       !read_at(mode+0xd0,&permissions,4)||(permissions&0xc00)!=0x400||
       !combat_context_get(base,current.input.pointer,&combat)||combat!=1||
       !runtime_power_reload_method(base,mode,&method)||!guest_camera_stamp_read(base,mode,&mode_stamp,0)||
       !guest_camera_stamp_read(base,manager,&manager_stamp,0)||!guest_camera_stamp_read(base,reload,&reload_stamp,0)||
       !runtime_power_native_lookup_matches(base,&current,index)||
       (mapped&&runtime_power_resolve_mapped(base,&current,requested)!=(int)index))return 0;
    uintptr_t pe=read_ptr(read_ptr(mode)+0x230);
    int dispatch_supported=pe==base+0xf4f80||
        (runtime_mapped_input.installed&&runtime_mapped_input.base==base&&runtime_mapped_input.slot==read_ptr(mode)+0x230&&
         pe==(uintptr_t)runtime_mapped_process_event&&(uintptr_t)runtime_mapped_input.original==base+0xf4f80);
    SessionContext checked=session_context(engine);RuntimePowerCatalog final;
    if(!dispatch_supported||session_engine_blocked(checked)||!runtime_power_read_catalog(base,checked,1,&final)||
       !runtime_power_catalog_same(&current,&final)||runtime_power_wheel_input_blocked(base,checked)||
       !guest_player_context_conditions(base,checked.pc,checked.pawn,checked.world,1,1,0,&channel,&chain)||
       runtime_power_reload_instance(base,checked)!=reload||!guest_camera_stamp_matches(base,&reload_stamp,0)||
       !guest_camera_stamp_matches(base,&manager_stamp,0)||!guest_camera_stamp_matches(base,&mode_stamp,0)||
       !runtime_mapped_input_mode(base,checked,mode,&checked_manager)||checked_manager!=manager||
       !guest_player_context_method_matches(base,&method,1)||read_ptr(read_ptr(mode)+0x230)!=pe||
       (mapped&&runtime_power_resolve_mapped(base,&final,requested)!=(int)index))return 0;
    /* The event may cancel reload/start a power/change ownership. Only scalar
     * logging is permitted after it returns; the caller reuses no game data. */
    return runtime_power_reload_dispatch((ProcessEvent)pe,mode,method.function.pointer,final.names[index],checked.pawn,1,at);
}
#ifdef ME3_RUNTIME_FIXTURE
static unsigned runtime_power_reload_fixture_calls;static int runtime_power_reload_fixture_bad;
static void runtime_power_reload_fixture_stock(void *self,void *method,void *parameters,void *result) {
    RuntimeReloadPowerParms expected={{42,3},32};
    if(self!=(void*)16||method!=(void*)24||!parameters||memcmp(parameters,&expected,sizeof expected)||result||GetLastError()!=37)
        runtime_power_reload_fixture_bad=1;
    ++runtime_power_reload_fixture_calls;SetLastError(53);
}
static int runtime_power_reload_selftest(void) {
    DWORD error=GetLastError();uint32_t name[]={42,3},none[]={0,0};ULONGLONG now=GetTickCount64();
    runtime_power_reload_fixture_calls=0;runtime_power_reload_fixture_bad=0;
    SetLastError(37);int ok=runtime_power_reload_dispatch(runtime_power_reload_fixture_stock,16,24,name,32,1,now)&&GetLastError()==53;
    for(int mode=0;mode<6;++mode){SetLastError(37);
        if(runtime_power_reload_dispatch(mode==0?NULL:runtime_power_reload_fixture_stock,mode==1?0:16,mode==2?0:24,
            mode==3?none:name,mode==4?0:32,mode!=5,now)||GetLastError()!=37)ok=0;}
    if(runtime_power_reload_dispatch(runtime_power_reload_fixture_stock,16,24,name,32,1,now+60000))ok=0;
    if(now>RUNTIME_POWER_REQUEST_MAX_AGE_MS&&runtime_power_reload_dispatch(runtime_power_reload_fixture_stock,16,24,name,32,1,
        now-RUNTIME_POWER_REQUEST_MAX_AGE_MS-1))ok=0;
    if(!runtime_power_reload_state(9,0,0,0,8,0)||runtime_power_reload_state(0,0,0,0,8,0)||
       runtime_power_reload_state(512,0,0,0,8,0)||runtime_power_reload_state(9,23,0,0,8,0)||
       runtime_power_reload_state(9,0,1,0,8,0)||runtime_power_reload_state(9,0,0,1,8,0)||
       runtime_power_reload_state(9,0,0,0,0x200008,0)||runtime_power_reload_state(9,0,0,0,0x20000008,0))ok=0;
    for(unsigned bit=0;bit<32;++bit)if((0x40300u&(1u<<bit))&&runtime_power_reload_state(9,0,0,0,8,1u<<bit))ok=0;
    ok=ok&&!runtime_power_reload_fixture_bad&&runtime_power_reload_fixture_calls==1;SetLastError(error);return ok;
}
#endif
#endif
