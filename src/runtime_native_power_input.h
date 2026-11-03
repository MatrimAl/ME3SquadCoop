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
    int registry_observation;
} RuntimeHotkeyWheel;
static RuntimeHotkeyWheel runtime_hotkey_wheel;
static int runtime_power_drag_blocks_input(int state) {
    /* Native MouseUp permits doHotKey while its source index is still set.
     * DoingDrag is the actual activity bit; a remembered index alone is not
     * a drag and must not indefinitely disable normal numbered inputs. */
    return state<0||(state&1)!=0;
}
static void runtime_native_hotkey_rejection(uintptr_t handler,SessionContext c,int slot,unsigned scope,int drag,int index) {
    static unsigned rows;if(rows++>=48)return;
    int32_t action=-1,power=-1;uint32_t life=0;unsigned char ignored=255;char row[650];
    read_at(c.pawn+0xc20,&action,4);read_at(c.pawn+0xc28,&power,4);read_at(c.pawn+0xe20,&life,4);
    read_at(c.pc+0x782,&ignored,1);
    snprintf(row,sizeof row,"{\"runtime_power_input\":\"native-hotkey-refusal-context\",\"slot\":%d,\"catalog_scope\":%u,\"drag_state\":%d,\"catalog_index\":%d,\"action\":%d,\"power_action\":%d,\"ignore_look\":%u,\"life_flags\":%u,\"game_mode\":%u,\"weapon\":\"0x%llx\",\"handler\":\"0x%llx\",\"slot_name_bound\":%s,\"monotonic_ms\":%lu}\r\n",
        slot,scope,drag,index,action,power,(unsigned)ignored,life,(unsigned)c.mode,
        (unsigned long long)read_ptr(c.pawn+0x41c),(unsigned long long)handler,
        slot>=0&&slot<8&&runtime_power_slots[slot].bound?"true":"false",GetTickCount());line(row);
}
/* Native drag owns the icon's transitional state. Validate the actual boolean
 * masks as well as their shared storage offset; unreadable is not idle. */
static int runtime_power_wheel_drag_state(uintptr_t base,uintptr_t handler) {
    static const char *names[]={"m_bDoingDrag"};
    static const uint64_t masks[]={8};
    if(!narrative_probe_object(base,handler,"SFXSFHandler_PCPowerWheel"))return -1;
    uintptr_t klass=read_ptr(handler+0x50);
    char name[96];object_name(base,klass,name,sizeof name);
    /* Only the exact native PC wheel is installed by this candidate. */
    if(strcmp(name,"SFXSFHandler_PCPowerWheel"))return -1;
    for(unsigned n=0;n<1;++n) {
        if(!runtime_power_property(base,handler,"SFXSFHandler_PCPowerWheel",names[n],"BoolProperty",0xc54,4))return -1;
        uintptr_t field=read_ptr(klass+0x70);int found=0;
        for(unsigned k=0;field&&k<1024;++k) {
            if(!is_live(base,field))return -1;
            object_name(base,field,name,sizeof name);
            if(!strcmp(name,names[n])) {
                uint64_t mask=0;
                if(!read_at(field+0xd0,&mask,8)||mask!=masks[n])return -1;
                found=1;break;
            }
            if(!read_at(field+0x68,&field,8))return -1;
        }
        if(!found)return -1;
    }
    uint32_t flags=0;int32_t dragging=-2;
    if(!runtime_power_property(base,handler,"SFXSFHandler_PCPowerWheel","m_nDraggingIcon","IntProperty",0xc38,4)||
       !read_at(handler+0xc54,&flags,4)||!read_at(handler+0xc38,&dragging,4)||dragging< -1)return -1;
    /* Power/key flags describe the kind of drag and may remain set after a
     * release. The pending source index and DoingDrag flag describe activity. */
    return (dragging>=0?2:0)|((flags&8)?1:0);
}
static int runtime_power_wheel_input_blocked(uintptr_t base,SessionContext c) {
    RuntimeHotkeyWheel *owner=&runtime_hotkey_wheel;
    if(!runtime_native_power_input_enabled()||c.pc!=owner->pc.pointer||c.pawn!=owner->pawn.pointer||
       c.world!=owner->world.pointer||owner->token!=guest_player_context_registration_token()||
       !session_handle_matches(owner->pc)||!session_handle_matches(owner->pawn)||
       !session_handle_matches(owner->world)||!session_handle_matches(owner->handler))return 0;
    if(read_ptr(owner->handler.pointer+0x6f8)!=c.pc)return 0;
    return runtime_power_drag_blocks_input(runtime_power_wheel_drag_state(base,owner->handler.pointer));
}
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
        guest_player_context_registration_token(),GetTickCount64(),0};
    /* Once identified as our wheel, consume the command even if unavailable:
     * falling through could issue a stock order through stale story icons. */
    RuntimePowerCatalog current;unsigned scope=(unsigned)runtime_power_scope;
    int index=-1,drag=runtime_power_wheel_drag_state(base,handler);
    /* Mouse-up calls doHotKey before clearing the pending source index.
     * It may enqueue then; Tick still waits for the released/idle state. */
    if(slot>=0&&slot<8&&scope&&!runtime_power_drag_blocks_input(drag)&&!session_engine_blocked(c)&&
       runtime_power_read_catalog(base,c,1,&current)&&
       runtime_power_catalog_same(&runtime_power_catalog,&current))
        index=runtime_power_slot_index(&current,(unsigned)slot);
    int accepted=index>=0&&runtime_request_power(scope,(unsigned)index);
    if(!accepted)runtime_native_hotkey_rejection(handler,c,slot,scope,drag,index);
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
