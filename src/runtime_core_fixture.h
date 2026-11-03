#ifndef ME3_RUNTIME_CORE_FIXTURE_H
#define ME3_RUNTIME_CORE_FIXTURE_H
/* Synthetic heap objects only. The actual shared release implementation calls
 * the normal validated invoke path into a fixture-owned ProcessEvent stub.
 * This tests cleanup ordering/identity policy, not real engine behavior. */
static uintptr_t runtime_fixture_objects[19];
static unsigned runtime_fixture_unpossess,runtime_fixture_possess,runtime_fixture_order;
static void runtime_fixture_ptr(uintptr_t address,uintptr_t value) {memcpy((void*)address,&value,8);}
static void runtime_fixture_pe(void *self,void *function,void *parameters,void *result) {
    (void)result;uintptr_t *o=runtime_fixture_objects;
    if((uintptr_t)self==o[0]&&(uintptr_t)function==o[16]) {
        ++runtime_fixture_unpossess;runtime_fixture_order=1;
        runtime_fixture_ptr(o[0]+0x3bc,0);
        if(read_ptr(o[1]+0x374)==o[0])runtime_fixture_ptr(o[1]+0x374,0);
    } else if((uintptr_t)self==o[2]&&(uintptr_t)function==o[17]&&
              parameters&&read_ptr((uintptr_t)parameters)==o[1]&&runtime_fixture_order==1) {
        ++runtime_fixture_possess;runtime_fixture_order=2;
        runtime_fixture_ptr(o[2]+0x3bc,o[1]);runtime_fixture_ptr(o[1]+0x374,o[2]);
    }
}
static void runtime_fixture_lease(void) {
    uintptr_t *o=runtime_fixture_objects;
    runtime_fixture_ptr(o[0]+0x3bc,o[1]);runtime_fixture_ptr(o[1]+0x374,o[0]);
    runtime_fixture_ptr(o[2]+0x3bc,0);runtime_fixture_ptr(o[3]+0x3bc,o[4]);
    runtime_fixture_ptr(o[4]+0x374,o[3]);runtime_fixture_ptr(o[6]+0x2cc,o[4]);runtime_fixture_ptr(o[6]+0x2d4,o[4]);
    session_remote=o[0];session_pawn=o[1];session_ai=o[2];session_host=o[3];session_shepard=o[4];
    session_world=o[5];session_squad=o[6];identity_host_driver=o[7];
    session_story_player=o[4];session_story_initial=o[4];
    session_pc_handle=session_capture_handle(o[0]);session_pawn_handle=session_capture_handle(o[1]);
    session_ai_handle=session_capture_handle(o[2]);session_host_handle=session_capture_handle(o[3]);
    session_shepard_handle=session_capture_handle(o[4]);session_world_handle=session_capture_handle(o[5]);
    session_squad_handle=session_capture_handle(o[6]);session_driver_handle=session_capture_handle(o[7]);
    session_enabled=1;session_active=1;session_fault=0;session_releasing=0;session_release_failures=0;
    memset(&session_root_world_handle,0,sizeof session_root_world_handle);session_travel_waiting=0;session_travel_wait_at=0;
    session_cleanup_pending=0;session_disconnect_observed=0;session_pending_reason=NULL;
    runtime_fixture_unpossess=runtime_fixture_possess=runtime_fixture_order=0;
}
static int runtime_core_cleanup_selftest(void) {
    unsigned char *image=VirtualAlloc(NULL,0x1900000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    unsigned char *memory=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,19*0x1000);
    if(!image||!memory) {if(image)VirtualFree(image,0,MEM_RELEASE);if(memory)HeapFree(GetProcessHeap(),0,memory);return 0;}
    uintptr_t base=(uintptr_t)image,*o=runtime_fixture_objects;unsigned char names[19*128]={0};
    const char *labels[]={"Remote","Ashley","SavedAI","Host","Shepard","Info","Squad","Driver",
        "PlayerController","BioAiController","Pawn","WorldInfo","BioBaseSquad","NetDriver","Function","Class",
        "UnPossess","Possess","OtherOwner"};
    const int classes[]={8,10,9,8,10,11,12,13,15,15,15,15,15,15,15,15,14,14,8};
    for(int i=0;i<19;++i) {
        o[i]=(uintptr_t)(memory+i*0x1000);uint32_t name=i*128;
        memcpy((void*)(o[i]+8),&i,4);memcpy((void*)(o[i]+0x48),&name,4);
        strcpy((char*)names+i*128+12,labels[i]);
    }
    for(int i=0;i<19;++i)runtime_fixture_ptr(o[i]+0x50,o[classes[i]]);
    GameArray objects={(uintptr_t)o,19,19};memcpy(image+OBJECTS_RVA,&objects,sizeof objects);
    runtime_fixture_ptr(base+NAME_POOLS_RVA,(uintptr_t)names);
    runtime_fixture_ptr(o[16]+0x40,o[8]);runtime_fixture_ptr(o[17]+0x40,o[9]);
    runtime_fixture_ptr(o[0],base+0x1000);runtime_fixture_ptr(o[2],base+0x1000);
    runtime_fixture_ptr(base+0x1000+0x230,base+0x2000);
    /* A fixture-owned executable trampoline keeps the real invoke address
     * range guard active instead of substituting or bypassing that guard. */
    unsigned char stub[]={0x48,0xb8,0,0,0,0,0,0,0,0,0xff,0xe0};uintptr_t callback=(uintptr_t)runtime_fixture_pe;
    memcpy(stub+2,&callback,8);memcpy(image+0x2000,stub,sizeof stub);DWORD old=0;
    int ok=VirtualProtect(image+0x2000,0x1000,PAGE_EXECUTE_READ,&old)!=0;
    if(ok)FlushInstructionCache(GetCurrentProcess(),image+0x2000,sizeof stub);
    SessionContext context={0};context.world=o[5];
    if(ok) {
        /* Prepared travel-disposal classifier cases; execution is deferred.
         * A rename/read failure must not count as native object disposal. */
        SessionHandle saved=session_capture_handle(o[1]);
        uint64_t original_name=read_ptr(o[1]+0x48),renamed_name=original_name+1;
        ok=!session_registry_retired(base,saved,&objects);
        memcpy((void*)(o[1]+0x48),&renamed_name,8);
        ok=ok&&!session_registry_retired(base,saved,&objects);
        memcpy((void*)(o[1]+0x48),&original_name,8);
        uintptr_t slots[19];memcpy(slots,o,sizeof slots);slots[1]=0;
        GameArray disposed={(uintptr_t)slots,19,19};
        ok=ok&&session_registry_retired(base,saved,&disposed);
        slots[1]=o[18];ok=ok&&session_registry_retired(base,saved,&disposed);
        GameArray unknown={1,19,19};
        ok=ok&&!session_registry_retired(base,saved,&unknown);
        SessionHandle outside=saved;outside.index=19;
        ok=ok&&!session_registry_retired(base,outside,&objects);
        /* Prepare admission regression cases for the next offline harness run.
         * These are allocated fixture objects, never a live campaign world. */
        runtime_fixture_lease();
        SessionContext admission={o[3],o[4],o[5],o[6],o[18],0};
        session_admission_wait_reset();
        ok=ok&&!session_admission_wait_ready(base,admission,1000)&&
            !session_admission_wait_ready(base,admission,1499)&&session_admission_wait_ready(base,admission,1500);
        /* Every tracked address must restart the interval; no cross-context
         * timer credit even when the replacement object is live. */
        for(unsigned i=0;ok&&i<6;++i) {
            session_admission_wait_reset();identity_host_driver=o[7];
            ok=!session_admission_wait_ready(base,admission,2000);
            SessionContext changed_context=admission;
            switch(i){case 0:changed_context.pc=o[0];break;case 1:changed_context.pawn=o[1];break;
                case 2:changed_context.world=o[18];break;case 3:changed_context.squad=o[18];break;
                case 4:changed_context.manager=o[0];break;case 5:identity_host_driver=o[18];break;}
            ok=ok&&!session_admission_wait_ready(base,changed_context,2600)&&
                !session_admission_wait_ready(base,changed_context,3099)&&session_admission_wait_ready(base,changed_context,3100);
        }
        identity_host_driver=o[7];session_admission_wait_reset();
        ok=ok&&!session_admission_wait_ready(base,admission,4000);
        uint64_t admission_name=read_ptr(o[4]+0x48),renamed=admission_name+1;
        memcpy((void*)(o[4]+0x48),&renamed,8);
        ok=ok&&!session_admission_wait_ready(base,admission,4600)&&!session_admission_wait_ready(base,admission,5099);
        memcpy((void*)(o[4]+0x48),&admission_name,8);
        identity_host_driver=0;
        ok=ok&&!session_admission_wait_ready(base,admission,5200)&&!session_admission_window.tracked;
        identity_host_driver=o[7];
        ok=ok&&!session_admission_wait_ready(base,admission,5300)&&!session_admission_wait_ready(base,admission,5799)&&
            session_admission_wait_ready(base,admission,5800);
        /* Reset also revokes elapsed credit after scope/enable loss. */
        session_admission_wait_reset();
        ok=ok&&!session_admission_wait_ready(base,admission,9000);
        session_admission_wait_reset();
        ok=ok&&!session_admission_wait_ready(base,admission,UINT32_MAX-100)&&
            !session_admission_wait_ready(base,admission,398)&&session_admission_wait_ready(base,admission,399);
        session_admission_wait_reset();
        runtime_fixture_lease();session_release(base,context,"scene",100);
        ok=ok&&!session_active&&!session_fault&&runtime_fixture_unpossess==1&&runtime_fixture_possess==1&&
            runtime_fixture_order==2&&read_ptr(o[1]+0x374)==o[2]&&session_story_valid(base);
        /* Object reuse invalidates writes even when its pointer is unchanged. */
        runtime_fixture_lease();uint64_t name=read_ptr(o[1]+0x48),changed=name+1;
        memcpy((void*)(o[1]+0x48),&changed,8);session_release(base,context,"disabled",101);
        ok=ok&&!session_active&&session_fault&&!runtime_fixture_unpossess&&!runtime_fixture_possess;
        memcpy((void*)(o[1]+0x48),&name,8);
        runtime_fixture_lease();context.world=o[18];session_release(base,context,"world-change",102);
        ok=ok&&!session_active&&session_fault&&!runtime_fixture_unpossess&&!runtime_fixture_possess;
        /* Detachment/AI handoff is distinct from observed driver removal. */
        runtime_fixture_lease();context.world=o[5];session_cleanup_pending=1;
        uintptr_t peers[]={o[18]};GameArray array={(uintptr_t)peers,1,1};
        memcpy((void*)(o[7]+0x70),&array,sizeof array);session_connection=o[18];
        session_release(base,context,"disconnect",103);
        ok=ok&&session_active&&session_pending_reason&&runtime_fixture_unpossess==1&&runtime_fixture_possess==1;
        session_disconnect_observed=1;array.count=0;memcpy((void*)(o[7]+0x70),&array,sizeof array);
        session_release(base,context,"disconnect",104);
        ok=ok&&!session_active&&!session_fault&&runtime_fixture_unpossess==1&&runtime_fixture_possess==1;
    }
    session_enabled=session_active=session_fault=session_cleanup_pending=session_disconnect_observed=0;
    session_admission_wait_reset();
    session_pending_reason=NULL;session_remote=session_connection=session_pawn=session_ai=0;identity_host_driver=0;
    memset(function_lookup_entries,0,sizeof function_lookup_entries);function_lookup_next=0;
    VirtualFree(image,0,MEM_RELEASE);HeapFree(GetProcessHeap(),0,memory);memset(o,0,sizeof runtime_fixture_objects);
    return ok;
}
#endif
