#ifndef ME3_RUNTIME_POWER_INPUT_H
#define ME3_RUNTIME_POWER_INPUT_H
/* Optional physical-input adapter. It produces a scoped slot action only;
 * the ordinary power queue owns all native gameplay validation/dispatch. */
typedef struct RuntimePowerBinding {unsigned slot;int key;} RuntimePowerBinding;
static const RuntimePowerBinding runtime_power_bindings[]={
    {0,'1'},{1,'2'},{2,'3'},{3,'4'},{4,'5'},{5,'6'},{6,'7'},{7,'8'},{8,'9'}
};
static int runtime_power_input_poll(unsigned scope,unsigned count) {
    static int configured,opted;
    static unsigned previous_down,previous_scope;
    static ULONGLONG previous_at;
    if(!configured) {
        wchar_t value[8]={0};
        DWORD length=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_POWER_HOTKEYS",value,8);
        opted=length==1&&value[0]==L'1';configured=1;
    }
    if(!opted)return -1;
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
        if((pressed&(1u<<i))&&runtime_power_bindings[i].slot<count)
            return (int)runtime_power_bindings[i].slot;
    return -1;
}
#endif
