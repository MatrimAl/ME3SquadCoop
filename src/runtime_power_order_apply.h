#ifndef ME3_RUNTIME_POWER_ORDER_APPLY_H
#define ME3_RUNTIME_POWER_ORDER_APPLY_H
/* SquadOrderUsePower writes an own-pawn PlayerOrder and returns. The stock
 * GameModeDefault.UsePower then calls ApplyTacticalOrders, which consumes it
 * through BioPlayerInput.ActivatePower. Complete that same path once; never
 * send a second power RPC or replay a consumed/replaced order. */
#pragma pack(push,4)
typedef struct RuntimePendingPowerOrder {
    float target_location[3],camera_location[3];
    int32_t camera_rotation[3];
    uint32_t power_name[2];
    uintptr_t target,switch_weapon;
} RuntimePendingPowerOrder;
#pragma pack(pop)
_Static_assert(sizeof(RuntimePendingPowerOrder)==0x3c,"native PlayerOrder size");
_Static_assert(offsetof(RuntimePendingPowerOrder,power_name)==0x24,"native PlayerOrder power name");
_Static_assert(offsetof(RuntimePendingPowerOrder,switch_weapon)==0x34,"native PlayerOrder weapon");
static int runtime_power_apply_contract(uintptr_t base,uintptr_t pc,GuestPlayerContextMethod *method) {
    RuntimeLayoutResolved layout;GameArray script={0};
    uintptr_t fn=guest_player_context_find_method(base,read_ptr(pc+0x50),"BioPlayerController","ApplyTacticalOrders");
    if(!guest_player_context_method_stamp(base,fn,method)||
       !guest_player_context_function_abi(base,fn,"BioPlayerController","ApplyTacticalOrders",0x20802,0,65535,0,0xf52b0,0)||
       !read_at(fn+0x7c,&script,sizeof script)||!script.data||script.count!=544||script.max<script.count||script.max>4096||
       !runtime_power_property_dim_uncached(base,pc,"BioPlayerController","m_currentOrder","StructProperty",0xa24,0x3c,1,&layout))return 0;
    uintptr_t order=read_ptr(layout.field+0xd0);GuestCameraStamp stamp={0};
    if(!narrative_probe_object(base,order,"ScriptStruct")||!guest_camera_stamp_read(base,order,&stamp,1)||
       strcmp(stamp.full_name,"SFXGame.BioPlayerController.PlayerOrder"))return 0;
    static const char *names[]={"nmPower","oTarget","oSwitchWeapon"};
    static const char *kinds[]={"NameProperty","ObjectProperty","ObjectProperty"};
    static const int offsets[]={0x24,0x2c,0x34};unsigned found=0;uintptr_t field=read_ptr(order+0x70);
    for(unsigned n=0;field&&n<16;++n){char name[96];int32_t offset=-1,size=0,dim=0;
        if(!is_live(base,field)||read_ptr(field+0x40)!=order)return 0;
        object_name(base,field,name,sizeof name);
        for(unsigned i=0;i<3;++i)if(!strcmp(name,names[i])){
            if((found&(1u<<i))||!narrative_probe_object(base,field,kinds[i])||
               !read_at(field+0x94,&offset,4)||offset!=offsets[i]||!read_at(field+0x74,&size,4)||size!=8||
               !read_at(field+0x70,&dim,4)||dim!=1)return 0;
            found|=1u<<i;
        }
        if(!read_at(field+0x68,&field,8))return 0;
    }
    return !field&&found==7&&guest_camera_stamp_matches(base,&stamp,1)&&
        guest_player_context_method_matches(base,method,1);
}
/* Only the freshly accepted name may be consumed. A native/external consumer,
 * a replacement target/order or a failed post-call scope leaves this path
 * untouched. Native code owns clearing the order and starting the action. */
static int runtime_power_apply_dispatch(ProcessEvent dispatch,uintptr_t pc,uintptr_t method,
        const RuntimePendingPowerOrder *expected,const uint32_t name[2],int current_scope) {
    DWORD error=GetLastError();RuntimePendingPowerOrder current={0};
    int allowed=current_scope&&dispatch&&pc&&method&&name[0]&&!expected->switch_weapon&&
        !memcmp(expected->power_name,name,8)&&read_at(pc+0xa24,&current,sizeof current)&&
        !memcmp(expected,&current,sizeof current);
    SetLastError(error);if(!allowed)return 0;
    unsigned char parms[16]={0};dispatch((void*)pc,(void*)method,parms,NULL);return 1;
}
static int runtime_power_apply_accepted(uintptr_t base,uintptr_t engine,const RuntimePowerCatalog *submitted,
        const uint32_t name[2]) {
    GuestPlayerContextMethod method={0};RuntimePendingPowerOrder order={0};
    RuntimePowerCatalog current;NetworkActorIdentityEntry channel={0};CombatContextChain chain;
    SessionContext c=session_context(engine);
    if(session_engine_blocked(c)||!runtime_control_enabled||!runtime_scope_ready||
       g_runtime_config.role!=ME3_ROLE_GUEST||!InterlockedCompareExchange(&g_enable_requested,0,0)||
       !runtime_power_read_catalog(base,c,1,&current)||!runtime_power_catalog_same(submitted,&current)||
       !guest_player_context_conditions(base,c.pc,c.pawn,c.world,1,1,1,&channel,&chain)||
       runtime_power_wheel_input_blocked(base,c)||!runtime_power_apply_contract(base,c.pc,&method)||
       !read_at(c.pc+0xa24,&order,sizeof order))return 0;
    uintptr_t pe=read_ptr(read_ptr(c.pc)+0x230);
    if(pe!=base+0x40d450)return 0;
    /* These validations/readers call no native gameplay. The final pending
     * snapshot is compared immediately before the single stock event. */
    return runtime_power_apply_dispatch((ProcessEvent)pe,c.pc,method.function.pointer,&order,name,
        guest_player_context_method_matches(base,&method,1)&&session_handle_matches(current.pc)&&
        session_handle_matches(current.pawn)&&read_ptr(c.pc+0x3bc)==c.pawn&&read_ptr(c.pawn+0x374)==c.pc);
}
#ifdef ME3_RUNTIME_FIXTURE
static unsigned runtime_power_apply_fixture_calls;static uintptr_t runtime_power_apply_fixture_pc;
static int runtime_power_apply_fixture_bad;
static void runtime_power_apply_fixture_stock(void *self,void *method,void *parms,void *result) {
    unsigned char zero[16]={0};
    if((uintptr_t)self!=runtime_power_apply_fixture_pc||method!=(void*)24||!parms||memcmp(parms,zero,16)||result||
       GetLastError()!=37)runtime_power_apply_fixture_bad=1;
    ++runtime_power_apply_fixture_calls;
    /* Model the stock ClearPlayerOrder. The same accepted request must not
     * invoke ApplyTacticalOrders a second time after natural consumption. */
    memset((void*)(runtime_power_apply_fixture_pc+0xa24),0,sizeof(RuntimePendingPowerOrder));SetLastError(53);
}
static int runtime_power_apply_selftest(void) {
    unsigned char memory[0xa24+sizeof(RuntimePendingPowerOrder)]={0};
    RuntimePendingPowerOrder accepted={0};uint32_t name[]={42,3};
    memcpy(accepted.power_name,name,8);accepted.target=88;accepted.target_location[0]=1;
    runtime_power_apply_fixture_pc=(uintptr_t)memory;runtime_power_apply_fixture_bad=0;runtime_power_apply_fixture_calls=0;
    for(int mode=0;mode<7;++mode){RuntimePendingPowerOrder pending=accepted;
        if(mode==1)pending.power_name[1]++;if(mode==2)pending.switch_weapon=16;
        if(mode==3)pending.target=96;if(mode==4)pending.camera_location[0]=2;
        memcpy(memory+0xa24,&pending,sizeof pending);SetLastError(37);
        int called=runtime_power_apply_dispatch(runtime_power_apply_fixture_stock,(uintptr_t)memory,24,&accepted,name,mode!=5);
        if(called!=(mode==0||mode==6)||GetLastError()!=(called?53:37))return 0;
        if(called){SetLastError(37);
            if(runtime_power_apply_dispatch(runtime_power_apply_fixture_stock,(uintptr_t)memory,24,&accepted,name,1)||GetLastError()!=37)return 0;}
    }
    SetLastError(37);
    if(runtime_power_apply_dispatch(runtime_power_apply_fixture_stock,1,24,&accepted,name,1)||GetLastError()!=37)return 0;
    return !runtime_power_apply_fixture_bad&&runtime_power_apply_fixture_calls==2;
}
#endif
#endif
