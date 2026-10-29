#ifndef ME3_NATIVE_POWER_MANAGER_EVENT_PINS_H
#define ME3_NATIVE_POWER_MANAGER_EVENT_PINS_H
/* Exact UObject ProcessEvent used by the supported SFXPowerManager vtable.
 * The caller must separately validate the manager and its owned pawn scope. */
static const unsigned char native_power_manager_event_entry[32]={
        0x40,0x55,0x41,0x56,0x41,0x57,0x48,0x81,0xec,0x90,0x00,0x00,0x00,0x48,0x8d,0x6c,
        0x24,0x20,0x48,0xc7,0x45,0x50,0xfe,0xff,0xff,0xff,0x48,0x89,0x9d,0x90,0x00,0x00,
    };
static uintptr_t runtime_melee_manager_event(uintptr_t base,uintptr_t manager) {
    unsigned char actual[32];
    return read_ptr(manager)==base+0x14d7ee8&&read_ptr(base+0x14d7ee8+0x2b0)==base+0xc0c4d0&&
        read_ptr(base+0x14d7ee8+0x230)==base+0xf4f80&&read_at(base+0xf4f80,actual,sizeof actual)&&
        !memcmp(actual,native_power_manager_event_entry,sizeof actual)?base+0xf4f80:0;
}
#ifdef ME3_RUNTIME_FIXTURE
static int runtime_melee_manager_event_selftest(void) {
    unsigned char *image=VirtualAlloc(NULL,0x1500000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!image)return 0;uintptr_t base=(uintptr_t)image,manager=base+0x1000;
    *(uintptr_t*)manager=base+0x14d7ee8;
    *(uintptr_t*)(base+0x14d7ee8+0x230)=base+0xf4f80;
    *(uintptr_t*)(base+0x14d7ee8+0x2b0)=base+0xc0c4d0;
    memcpy(image+0xf4f80,native_power_manager_event_entry,sizeof native_power_manager_event_entry);
    int ok=runtime_melee_manager_event(base,manager)==base+0xf4f80;
    *(uintptr_t*)(base+0x14d7ee8+0x230)=base+0x40d450;
    ok=ok&&!runtime_melee_manager_event(base,manager);
    *(uintptr_t*)(base+0x14d7ee8+0x230)=base+0xf4f80;
    image[0xf4f80]^=1;ok=ok&&!runtime_melee_manager_event(base,manager);image[0xf4f80]^=1;
    *(uintptr_t*)(base+0x14d7ee8+0x2b0)+=1;ok=ok&&!runtime_melee_manager_event(base,manager);
    *(uintptr_t*)(base+0x14d7ee8+0x2b0)=base+0xc0c4d0;
    *(uintptr_t*)manager+=8;ok=ok&&!runtime_melee_manager_event(base,manager);
    VirtualFree(image,0,MEM_RELEASE);return ok;
}
#endif
#endif
