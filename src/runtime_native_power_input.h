#ifndef ME3_RUNTIME_NATIVE_POWER_INPUT_H
#define ME3_RUNTIME_NATIVE_POWER_INPUT_H
/* Optional bridge at the native PC wheel virtual, after the engine has
 * decoded UseAbility's integer argument. No FFrame parsing or key polling.
 * Like the Tick bridge, this forwarding slot stays resident until exit. */
typedef void (*RuntimeNativeHotkey)(void *handler,int slot);
static RuntimeNativeHotkey runtime_original_hotkey;
static uintptr_t runtime_hotkey_base;
static int runtime_hotkey_installed;
typedef struct RuntimeHotkeyWheel {
    SessionHandle handler,pc,pawn,world;
    unsigned long token;
    ULONGLONG observed;
} RuntimeHotkeyWheel;
static RuntimeHotkeyWheel runtime_hotkey_wheel;
static void runtime_native_hotkey(void *self,int slot) {
    RuntimeNativeHotkey original=runtime_original_hotkey;
    uintptr_t base=runtime_hotkey_base,handler=(uintptr_t)self;
    if(GetCurrentThreadId()!=session_game_thread||!runtime_native_power_input_enabled()||
       !InterlockedCompareExchange(&g_enable_requested,0,0)||!runtime_control_enabled||
       !runtime_scope_ready||g_runtime_config.role!=ME3_ROLE_GUEST) {
        original(self,slot);return;
    }
    SessionContext c=session_context(session_engine);
    /* A host or another controller's wheel must retain stock semantics. Do
     * not infer local ownership from the wheel's cached story-squad pawn. */
    if(!narrative_probe_object(base,handler,"SFXSFHandler_PCPowerWheel")||
       read_ptr(handler)!=base+0x14f6700||read_ptr(handler+0x6f8)!=c.pc||
       !runtime_power_property(base,handler,"SFXSFHandler_PowerWheel","m_pPlayerController","ObjectProperty",0x6f8,8)||
       !guest_player_context_registered_for(c.pc,c.pawn)||
       !guest_player_context_basic_owned(base,c.pc,c.pawn,c.world)) {
        original(self,slot);return;
    }
    runtime_hotkey_wheel=(RuntimeHotkeyWheel){session_capture_handle(handler),
        session_capture_handle(c.pc),session_capture_handle(c.pawn),session_capture_handle(c.world),
        guest_player_context_registration_token(),GetTickCount64()};
    /* Once identified as our wheel, consume the command even if unavailable:
     * falling through could issue a stock order through stale story icons. */
    RuntimePowerCatalog current;unsigned scope=(unsigned)runtime_power_scope;
    int index=-1;
    if(slot>=0&&slot<8&&scope&&!session_engine_blocked(c)&&
       runtime_power_read_catalog(base,c,1,&current)&&
       runtime_power_catalog_same(&runtime_power_catalog,&current))
        index=runtime_power_slot_index(&current,(unsigned)slot);
    int accepted=index>=0&&runtime_request_power(scope,(unsigned)index);
    runtime_power_log(accepted?RUNTIME_POWER_QUEUED:RUNTIME_POWER_REJECTED,
        accepted?"native-hotkey-queued":"native-hotkey-unavailable",
        index>=0?(unsigned)index:UINT32_MAX,GetTickCount());
}
static int runtime_native_power_input_active(void) {
    return runtime_hotkey_installed&&
        read_ptr(runtime_hotkey_base+0x14f6700+0x528)==(uintptr_t)runtime_native_hotkey;
}
static void runtime_native_power_input_install(uintptr_t base) {
    static int attempted;
    if(attempted||!runtime_native_power_input_enabled()||
       g_runtime_config.role!=ME3_ROLE_GUEST||GetCurrentThreadId()!=session_game_thread)return;
    attempted=1;
    uintptr_t slot=base+0x14f6700+0x528,expected=base+0xc5ccc0;
    static const unsigned char want[]={0x40,0x55,0x57,0x48,0x8b,0xec,0x48,0x83,0xec,0x58,0x48,0x8b,0xf9};
    unsigned char got[sizeof want];
    if(read_ptr(slot)!=expected||!read_at(expected,got,sizeof got)||memcmp(got,want,sizeof want)) {
        line("{\"runtime_power_input\":\"native-hotkey-bridge-refused\",\"reason\":\"entry-or-slot\"}\r\n");return;
    }
    DWORD old=0;
    if(!VirtualProtect((void*)slot,8,PAGE_READWRITE,&old)) {
        line("{\"runtime_power_input\":\"native-hotkey-bridge-refused\",\"reason\":\"slot-protection\"}\r\n");return;
    }
    runtime_hotkey_base=base;runtime_original_hotkey=(RuntimeNativeHotkey)expected;
    PVOID previous=InterlockedCompareExchangePointer((PVOID volatile*)slot,
        (PVOID)runtime_native_hotkey,(PVOID)expected);
    VirtualProtect((void*)slot,8,old,&old);
    runtime_hotkey_installed=(uintptr_t)previous==expected;
    line(runtime_hotkey_installed?
        "{\"runtime_power_input\":\"native-hotkey-bridge-installed\",\"gameplay_verified\":false}\r\n":
        "{\"runtime_power_input\":\"native-hotkey-bridge-refused\",\"reason\":\"slot-changed\"}\r\n");
}
#endif
