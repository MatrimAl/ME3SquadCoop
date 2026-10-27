#ifndef ME3_RUNTIME_AUTO_REJOIN_FIXTURE_H
#define ME3_RUNTIME_AUTO_REJOIN_FIXTURE_H
/* Synthetic handles exercise the recovery policy after its owned-client
 * validation boundary; this is not a live ownership or travel fixture. */
static int runtime_attach_recovery_selftest(void) {
    unsigned char *image=VirtualAlloc(NULL,0x1900000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    if(!image)return 0;
    unsigned char memory[3][0x60]={0};uintptr_t slots[3];
    for(int32_t i=0;i<3;++i) {
        uint64_t name=(uint64_t)i+1;slots[i]=(uintptr_t)memory[i];
        memcpy(memory[i]+8,&i,4);memcpy(memory[i]+0x48,&name,8);
    }
    GameArray objects={(uintptr_t)slots,3,3};
    memcpy(image+OBJECTS_RVA,&objects,sizeof objects);
    SessionHandle world=session_capture_handle(slots[0]),driver=session_capture_handle(slots[1]),
        target=session_capture_handle(slots[2]);
    Me3RuntimeConfig config=g_runtime_config;Me3RuntimeConfigStatus status=g_config_status;
    LONG enabled=g_enable_requested,requested=runtime_join_requested,attempted=runtime_join_attempted,
        retry=runtime_rejoin_requested,origin=runtime_rejoin_verified_origin;
    SessionHandle saved_world=runtime_auto_rejoin_origin_world,saved_driver=runtime_auto_rejoin_origin_driver,
        saved_target=runtime_auto_rejoin_target;
    unsigned epoch=runtime_auto_rejoin_epoch,consumed=runtime_auto_rejoin_attempted_epoch;
    g_enable_requested=1;g_config_status=ME3_CONFIG_READY;g_runtime_config.role=ME3_ROLE_GUEST;
    runtime_join_requested=runtime_rejoin_requested=runtime_rejoin_verified_origin=0;
    runtime_auto_rejoin_epoch=runtime_auto_rejoin_attempted_epoch=0;
    memset(&runtime_auto_rejoin_origin_world,0,sizeof runtime_auto_rejoin_origin_world);
    memset(&runtime_auto_rejoin_origin_driver,0,sizeof runtime_auto_rejoin_origin_driver);
    memset(&runtime_auto_rejoin_target,0,sizeof runtime_auto_rejoin_target);
    /* A failed initial join has no observed origin and cannot auto-retry. */
    runtime_join_attempted=1;
    int ok=!runtime_auto_rejoin_queue_origin((uintptr_t)image,target)&&!runtime_rejoin_requested;
    /* Attach preserves the fact that startup never attempted a join. */
    runtime_join_attempted=0;
    ok=ok&&runtime_auto_rejoin_observe_origin(world,driver)&&runtime_rejoin_history_valid()&&
        runtime_rejoin_verified_origin==1&&!runtime_join_attempted&&runtime_auto_rejoin_epoch==1;
    ok=ok&&!runtime_auto_rejoin_old_gone((uintptr_t)image)&&
        !runtime_auto_rejoin_queue_origin((uintptr_t)image,target)&&!runtime_rejoin_requested;
    slots[0]=0;
    ok=ok&&!runtime_auto_rejoin_old_gone((uintptr_t)image)&&
        !runtime_auto_rejoin_queue_origin((uintptr_t)image,target);
    slots[1]=0;
    ok=ok&&runtime_auto_rejoin_old_gone((uintptr_t)image)&&
        runtime_auto_rejoin_queue_origin((uintptr_t)image,target)&&runtime_rejoin_requested==1&&
        !runtime_join_attempted&&runtime_auto_rejoin_attempted_epoch==1&&
        runtime_auto_rejoin_target.pointer==target.pointer&&session_handle_matches(runtime_auto_rejoin_target);
    /* Clearing the request after any outcome cannot spend this origin twice. */
    runtime_rejoin_requested=0;
    ok=ok&&!runtime_auto_rejoin_observe_origin(world,driver)&&
        !runtime_auto_rejoin_queue_origin((uintptr_t)image,target)&&!runtime_rejoin_requested&&
        runtime_auto_rejoin_epoch==1&&runtime_auto_rejoin_attempted_epoch==1&&!runtime_join_attempted;
    g_runtime_config=config;g_config_status=status;g_enable_requested=enabled;
    runtime_join_requested=requested;runtime_join_attempted=attempted;runtime_rejoin_requested=retry;
    runtime_rejoin_verified_origin=origin;runtime_auto_rejoin_origin_world=saved_world;
    runtime_auto_rejoin_origin_driver=saved_driver;runtime_auto_rejoin_target=saved_target;
    runtime_auto_rejoin_epoch=epoch;runtime_auto_rejoin_attempted_epoch=consumed;
    VirtualFree(image,0,MEM_RELEASE);return ok;
}
#endif
