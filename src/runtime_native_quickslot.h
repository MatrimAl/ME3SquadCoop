#ifndef ME3_RUNTIME_NATIVE_QUICKSLOT_H
#define ME3_RUNTIME_NATIVE_QUICKSLOT_H
#include "../third_party/minhook/include/MinHook.h"
/* SetupQuickSlotPowers calls GetPawnFromName directly, then virtual+510.
 * PC Tick also resolves a pawn solely to detect saved-definition mismatch.
 * Redirect those exact callers. Saved controller definitions remain
 * untouched, and native setup retains its permission/state/display pass. */
typedef uintptr_t (*RuntimeQuickslotPawn)(void*,uint64_t);
typedef int (*RuntimeQuickslotSetup)(void*,uintptr_t,uint64_t,uintptr_t);
static RuntimeQuickslotPawn runtime_original_quickslot_pawn;
static RuntimeQuickslotSetup runtime_original_quickslot_setup;
static uintptr_t runtime_quickslot_base;
static int runtime_quickslot_installed;
static unsigned char runtime_quickslot_entry[16];
static uintptr_t runtime_quickslot_pawn(void *self,uint64_t tag);
static int runtime_quickslot_setup(void *self,uintptr_t pawn,uint64_t name,uintptr_t icon);
static int runtime_native_quickslot_active(uintptr_t base) {
    unsigned char bytes[sizeof runtime_quickslot_entry];
    return runtime_quickslot_installed&&base==runtime_quickslot_base&&
        read_ptr(base+0x14f6700+0x510)==(uintptr_t)runtime_quickslot_setup&&
        read_at(base+0xc4c8e0,bytes,sizeof bytes)&&!memcmp(bytes,runtime_quickslot_entry,sizeof bytes);
}
static int runtime_quickslot_owner(uintptr_t handler) {
    return GetCurrentThreadId()==session_game_thread&&runtime_power_icons_enabled()&&
        runtime_control_enabled&&runtime_scope_ready&&g_runtime_config.role==ME3_ROLE_GUEST&&
        InterlockedCompareExchange(&g_enable_requested,0,0)&&runtime_power_wheel_context.active&&
        handler==runtime_power_wheel_context.owner.handler.pointer;
}
static int runtime_quickslot_ready(uintptr_t base,SessionContext c,RuntimePowerCatalog *catalog) {
    return runtime_native_quickslot_active(base)&&runtime_power_wheel_context_valid(base,c)&&
        (!session_engine_blocked(c)||runtime_power_menu_context_held(base,c))&&
        runtime_power_read_membership(base,c,catalog)&&
        runtime_power_catalog_same(catalog,&runtime_power_wheel_context.catalog);
}
static uintptr_t runtime_quickslot_pawn(void *self,uint64_t tag) {
    uintptr_t base=runtime_quickslot_base,handler=(uintptr_t)self;
    uintptr_t caller=(uintptr_t)__builtin_return_address(0);
    if(!runtime_quickslot_installed||!runtime_quickslot_owner(handler)||
       (caller!=base+0xc544f5&&caller!=base+0xc58373))
        return runtime_original_quickslot_pawn(self,tag);
    SessionContext c=session_context(session_engine);RuntimePowerCatalog catalog;
    if(caller==base+0xc58373) {
        /* Saved definitions are not this wheel's assignment authority.
         * Suppress only that mismatch-triggered rebuild; periodic native
         * setup and the remaining per-icon UpdateDisplay loop still run. */
        return runtime_native_quickslot_active(base)&&runtime_power_wheel_context_valid(base,c)?
            0:runtime_original_quickslot_pawn(self,tag);
    }
    /* Keep a real pawn during a drag: returning None would make the native
     * caller clear icons before the per-icon adapter could preserve them. */
    return runtime_quickslot_ready(base,c,&catalog)?c.pawn:0;
}
static int runtime_quickslot_setup(void *self,uintptr_t pawn,uint64_t name,uintptr_t icon) {
    uintptr_t base=runtime_quickslot_base,handler=(uintptr_t)self;
    if(!runtime_quickslot_installed||(uintptr_t)__builtin_return_address(0)!=base+0xc5455b||!runtime_quickslot_owner(handler))
        return runtime_original_quickslot_setup(self,pawn,name,icon);
    SessionContext c=session_context(session_engine);RuntimePowerCatalog catalog;
    if(!runtime_quickslot_ready(base,c,&catalog)||runtime_power_wheel_drag_state(base,handler)!=0)return 0;
    RuntimeHotkeyWheel owner=runtime_power_wheel_context.owner;
    SessionHandle widget=session_capture_handle(icon);int slot=-1;
    for(unsigned n=0;n<8;++n)if(read_ptr(handler+0xbd8+n*8)==icon) {
        if(slot>=0)return 0;
        slot=(int)n;
    }
    if(slot<0||!runtime_power_icon_layout(base,&owner,(unsigned)slot,widget))return 0;
    RuntimePowerIconLease *paint=&runtime_power_icons;
    RuntimePowerIconSlot *captured=&paint->slots[slot];
    int tracked=paint->active&&paint->owner.handler.pointer==handler&&paint->owner.token==owner.token;
    if(tracked&&(paint->failed||paint->retiring||
       !runtime_power_same_handle(captured->icon,widget)||
       (captured->changed?!runtime_power_icon_matches(icon,captured->installed,
           captured->installed.pointer?c.pawn:0,captured->installed_name):
           !runtime_power_icon_values(icon,captured->original,captured->original_pawn.pointer)))) {
        paint->failed=1;return 0;
    }
    int entry=runtime_power_slot_index(&catalog,(unsigned)slot),result=0;
    SessionHandle desired=entry>=0?catalog.powers[entry]:(SessionHandle){0};
    uint32_t mapped[2]={0};
    if(entry>=0) {
        memcpy(mapped,catalog.names[entry],8);uint64_t packed=0;memcpy(&packed,mapped,8);
        if(!runtime_power_caster_power_owned(desired.pointer,c))return 0;
        result=runtime_original_quickslot_setup(self,c.pawn,packed,icon);
    } else {
        /* Returning success would make native setup assign an active state
         * to an empty capacity slot. Clear through its verified UI setter,
         * then return false so native UpdateDisplay preserves hidden state5. */
        if(read_ptr(icon+0x194)) {
            uintptr_t setter=runtime_power_icon_setter(base,icon);
            if(!setter)return 0;
            runtime_power_icon_set(base,icon,setter,0);
        }
    }
    /* Native UI work can change the registration, widget or map. Acknowledge
     * only this exact result; never adopt an arbitrary engine replacement. */
    SessionContext after=session_context(session_engine);RuntimePowerCatalog current;
    int valid=after.pc==c.pc&&after.pawn==c.pawn&&after.world==c.world&&
        runtime_quickslot_ready(base,after,&current)&&runtime_power_catalog_same(&catalog,&current)&&
        runtime_power_slot_index(&current,(unsigned)slot)==entry&&
        runtime_power_icon_layout(base,&owner,(unsigned)slot,widget);
    if(valid&&entry<0)valid=runtime_power_icon_none(base,icon,mapped);
    if(valid)valid=runtime_power_icon_matches(icon,desired,desired.pointer?c.pawn:0,mapped);
    if(tracked&&paint->active&&paint->owner.handler.pointer==handler&&paint->owner.token==owner.token&&
       runtime_power_same_handle(paint->slots[slot].icon,widget)) {
        if(!valid||(entry>=0&&!result))paint->failed=1;
        else {
            captured->installed=desired;memcpy(captured->installed_name,mapped,8);captured->changed=1;
        }
    }
    return valid?result:0;
}
static int runtime_native_quickslot_install(uintptr_t base) {
    static int attempted;
    if(runtime_quickslot_installed)return runtime_native_quickslot_active(base);
    if(attempted||GetCurrentThreadId()!=session_game_thread||!runtime_power_icons_enabled()||
       g_runtime_config.role!=ME3_ROLE_GUEST)return 0;
    attempted=1;
    static const unsigned char pawn_entry[]={0x48,0x8b,0x81,0x10,0x07,0,0,0x4c,0x8b,0xc9,0x48,0x85,0xc0,0x74,0x18,0x3b};
    static const unsigned char setup_entry[]={0x40,0x53,0x48,0x83,0xec,0x20,0x49,0x8b,0xd9,0x48,0x85,0xd2,0x74,0x5a,0x48,0x8b};
    static const unsigned char pawn_call[]={0xe8,0xeb,0x83,0xff,0xff};
    static const unsigned char setup_call[]={0xff,0x90,0x10,0x05,0,0};
    static const unsigned char compare_call[]={0xe8,0x6d,0x45,0xff,0xff};
    unsigned char got[16];uintptr_t slot=base+0x14f6700+0x510,target=base+0xc4c8e0;
    if(read_ptr(slot)!=base+0xc4c960||
       !read_at(target,got,sizeof pawn_entry)||memcmp(got,pawn_entry,sizeof pawn_entry)||
       !read_at(base+0xc4c960,got,sizeof setup_entry)||memcmp(got,setup_entry,sizeof setup_entry)||
       !read_at(base+0xc544f0,got,sizeof pawn_call)||memcmp(got,pawn_call,sizeof pawn_call)||
       !read_at(base+0xc54555,got,sizeof setup_call)||memcmp(got,setup_call,sizeof setup_call)||
       !read_at(base+0xc5836e,got,sizeof compare_call)||memcmp(got,compare_call,sizeof compare_call))return 0;
    MH_STATUS status=MH_Initialize();
    if(status!=MH_OK&&status!=MH_ERROR_ALREADY_INITIALIZED)return 0;
    runtime_quickslot_base=base;runtime_original_quickslot_setup=(RuntimeQuickslotSetup)(base+0xc4c960);
    LPVOID original=NULL;
    if(MH_CreateHook((LPVOID)target,(LPVOID)runtime_quickslot_pawn,&original)!=MH_OK)return 0;
    runtime_original_quickslot_pawn=(RuntimeQuickslotPawn)original;
    if(MH_EnableHook((LPVOID)target)!=MH_OK)return 0;
    /* The detour forwards until both hooks are published. Never unhook or
     * unload its trampoline in a scene/disable path; other callers retain it. */
    if(!read_at(target,runtime_quickslot_entry,sizeof runtime_quickslot_entry))return 0;
    DWORD old=0;
    if(!VirtualProtect((void*)slot,8,PAGE_READWRITE,&old))return 0;
    PVOID previous=InterlockedCompareExchangePointer((PVOID volatile*)slot,
        (PVOID)runtime_quickslot_setup,(PVOID)runtime_original_quickslot_setup);
    VirtualProtect((void*)slot,8,old,&old);
    runtime_quickslot_installed=(uintptr_t)previous==(uintptr_t)runtime_original_quickslot_setup;
    if(runtime_quickslot_installed)
        line("{\"runtime_power_icons\":\"native-refresh-bridge-installed\",\"gameplay_verified\":false}\r\n");
    return runtime_native_quickslot_active(base);
}
#endif
