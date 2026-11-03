#ifndef ME3_RUNTIME_POWER_INPUT_H
#define ME3_RUNTIME_POWER_INPUT_H
/* Optional physical-input adapter. It produces a scoped slot action only;
 * the ordinary power queue owns all native gameplay validation/dispatch. */
typedef struct RuntimePowerBinding {unsigned slot;int key;} RuntimePowerBinding;
static const RuntimePowerBinding runtime_power_bindings[]={
    {0,'1'},{1,'2'},{2,'3'},{3,'4'},{4,'5'},{5,'6'},{6,'7'},{7,'8'},{8,'9'}
};
typedef struct RuntimePowerSlot {int bound;uint32_t name[2];} RuntimePowerSlot;
static RuntimePowerSlot runtime_power_slots[9];
static int runtime_native_power_input_enabled(void) {
    static int configured,opted;
    if(!configured) {
        wchar_t value[8]={0};
        DWORD length=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_NATIVE_POWER_HOTKEYS",value,8);
        opted=!length||(length==1&&value[0]==L'1');configured=1;
    }
    return opted;
}
static int runtime_power_input_enabled(void) {
    static int configured,opted;
    if(!configured) {
        wchar_t value[8]={0};
        DWORD length=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_POWER_HOTKEYS",value,8);
        opted=length==1&&value[0]==L'1';configured=1;
    }
    return opted;
}
/* FName values belong to this process's name table, not any world object.
 * Keep bindings over catalog/mission changes; never silently replace one. */
static int runtime_power_name_index(const RuntimePowerCatalog *catalog,const uint32_t name[2]) {
    int found=-1;
    for(int i=0;i<catalog->count;++i)if(!memcmp(catalog->names[i],name,8)) {
        if(found>=0)return -1; /* Ambiguous names cannot identify a power. */
        found=i;
    }
    return found;
}
static int runtime_power_slot_index(const RuntimePowerCatalog *catalog,unsigned slot) {
    return slot<9&&runtime_power_slots[slot].bound?
        runtime_power_name_index(catalog,runtime_power_slots[slot].name):-1;
}
/* Game-thread only. A native drop changes our name map, never saved Shepard
 * definitions. Moving an existing name swaps the previous destination back. */
static int runtime_power_assign_slot(unsigned destination,const uint32_t name[2]) {
    if(destination>=8||!name[0])return 0;
    int source=-1;
    for(int n=0;n<9;++n)if(runtime_power_slots[n].bound&&!memcmp(runtime_power_slots[n].name,name,8)) {
        if(source>=0)return 0;
        source=n;
    }
    RuntimePowerSlot previous=runtime_power_slots[destination];
    if(source>=0)runtime_power_slots[source]=previous;
    runtime_power_slots[destination].bound=1;memcpy(runtime_power_slots[destination].name,name,8);
    return 1;
}
static void runtime_power_bind_slots(const RuntimePowerCatalog *catalog,unsigned scope) {
    static unsigned sampled_scope;
    if((!runtime_power_input_enabled()&&!runtime_native_power_input_enabled())||!scope||scope==sampled_scope)return;
    sampled_scope=scope;
    for(int i=0;i<catalog->count;++i) {
        const uint32_t *name=catalog->names[i];int existing=0,empty=-1;
        if(!name[0]||runtime_power_name_index(catalog,name)!=i)continue;
        for(int j=0;j<9;++j) {
            if(runtime_power_slots[j].bound&&!memcmp(runtime_power_slots[j].name,name,8))existing=1;
            if(!runtime_power_slots[j].bound&&empty<0)empty=j;
        }
        if(!existing&&empty>=0) {
            memcpy(runtime_power_slots[empty].name,name,8);runtime_power_slots[empty].bound=1;
        }
    }
}
static int runtime_power_input_poll(unsigned scope,unsigned count) {
    static unsigned previous_down,previous_scope;
    static ULONGLONG previous_at;
    if(!runtime_power_input_enabled())return -1;
    ULONGLONG now=GetTickCount64();unsigned down=0;
    int modifiers=(GetAsyncKeyState(VK_CONTROL)&0x8000)&&(GetAsyncKeyState(VK_MENU)&0x8000)&&
        !(GetAsyncKeyState(VK_LWIN)&0x8000)&&!(GetAsyncKeyState(VK_RWIN)&0x8000);
    for(unsigned i=0;i<sizeof runtime_power_bindings/sizeof *runtime_power_bindings;++i)
        if(modifiers&&(GetAsyncKeyState(runtime_power_bindings[i].key)&0x8000))down|=1u<<i;
    HWND foreground=GetForegroundWindow();DWORD pid=0;
    int focused=foreground&&GetWindowThreadProcessId(foreground,&pid)&&pid==GetCurrentProcessId();
    unsigned active=focused&&scope&&count?scope:0;
    unsigned pressed=down&~previous_down;
    int armed=active&&active==previous_scope&&previous_at&&now>=previous_at&&now-previous_at<=500;
    previous_down=down;previous_scope=active;previous_at=now;
    /* Baseline on startup/focus/scene/catalog changes and long Tick gaps.
     * Holding a key across any such boundary must not become a new order. */
    if(!armed||!pressed||(down&(down-1)))return -1;
    for(unsigned i=0;i<sizeof runtime_power_bindings/sizeof *runtime_power_bindings;++i)
        if(pressed&(1u<<i))
            return (int)runtime_power_bindings[i].slot;
    return -1;
}
#endif
