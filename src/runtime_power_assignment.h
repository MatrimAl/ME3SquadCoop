#ifndef ME3_RUNTIME_POWER_ASSIGNMENT_H
#define ME3_RUNTIME_POWER_ASSIGNMENT_H
#include "../third_party/minhook/include/MinHook.h"
/* NewSetQuickSlotPower clears its destination before virtual+0x548. Handle
 * the already-decoded native drop here without parsing a VM frame, touching
 * saved definitions or recursively invoking native UI/gameplay methods. */
typedef void (*RuntimePowerAssignment)(void*,int,int,uint32_t,uint32_t);
static RuntimePowerAssignment runtime_original_assignment;
static uintptr_t runtime_assignment_base;
static int runtime_assignment_installed;
typedef void (*RuntimePowerNamedAssignment)(void*,int,uint64_t);
static RuntimePowerNamedAssignment runtime_original_named_assignment;
static int runtime_named_assignment_installed;
static unsigned char runtime_named_assignment_entry[16];
static RuntimePowerAssignment runtime_original_new_assignment;
static int runtime_new_assignment_installed,runtime_assignment_decoded;
static unsigned char runtime_new_assignment_entry[16];
static int runtime_power_assignment_reject_policy(int destination,int structural,int expected) {
    return destination>=0&&destination<8&&structural&&expected;
}
/* The native EndDrag virtual calls the assignment slot directly, bypassing
 * NewSetQuickSlotPower and its pre-clear bridge. Match its actual decoded
 * gesture, not a source index from saved Shepard quickslot definitions. */
static int runtime_power_assignment_drag_policy(int destination,int source,uint32_t from_power,uint32_t silent,
        int target,int dragging,uint32_t flags) {
    return destination>=0&&destination<8&&source>=0&&destination==target&&source==dragging&&
        !silent&&from_power<=1&&(flags&8)&&
        (from_power?((flags&2)&&!(flags&4)):((flags&4)&&!(flags&2)));
}
static int runtime_power_assignment_native_drag(uintptr_t base,uintptr_t handler,uintptr_t caller,
        int destination,int source,uint32_t from_power,uint32_t silent) {
    static const unsigned char call[]={0xff,0x90,0x48,0x05,0x00,0x00};
    unsigned char bytes[sizeof call];int32_t target=-1,dragging=-1;uint32_t flags=0;
    if(caller!=base+0xc56b06||!read_at(base+0xc56b00,bytes,sizeof bytes)||memcmp(bytes,call,sizeof call)||
       !runtime_power_property(base,handler,"SFXSFHandler_PCPowerWheel","m_nCurrentDropTarget","IntProperty",0xc3c,4)||
       !runtime_power_property(base,handler,"SFXSFHandler_PCPowerWheel","m_nDraggingIcon","IntProperty",0xc38,4)||
       !read_at(handler+0xc3c,&target,4)||!read_at(handler+0xc38,&dragging,4)||
       !read_at(handler+0xc54,&flags,4))return 0;
    return runtime_power_assignment_drag_policy(destination,source,from_power,silent,target,dragging,flags);
}
/* A valid native clear belongs to the existing lease even if the requested
 * drop cannot be resolved. Keep the last name map and repaint next idle Tick;
 * an unsupported drop must not disable every otherwise valid hotkey. */
static void runtime_power_assignment_reject(RuntimePowerIconLease *lease,const char *reason) {
    lease->drop_accepted=1;lease->drag_observed=0;runtime_power_invalidate();
    static unsigned rows;
    if(rows++<32){char row[230];snprintf(row,sizeof row,
        "{\"runtime_power_input\":\"guest-slot-drop-refused\",\"reason\":\"%s\",\"map_preserved\":true,\"profile_write\":false}\r\n",reason);line(row);}
}
static void runtime_power_assignment(void *self,int destination,int source,uint32_t from_power,uint32_t silent) {
    uintptr_t caller=(uintptr_t)__builtin_return_address(0);
    uintptr_t base=runtime_assignment_base,handler=(uintptr_t)self;
    RuntimePowerAssignment original=runtime_original_assignment;
    RuntimePowerIconLease *lease=&runtime_power_icons;
    if(GetCurrentThreadId()!=session_game_thread||!runtime_power_icons_enabled()||
       !runtime_control_enabled||!runtime_scope_ready||g_runtime_config.role!=ME3_ROLE_GUEST||
       !InterlockedCompareExchange(&g_enable_requested,0,0)) {
        original(self,destination,source,from_power,silent);return;
    }
    SessionContext c=session_context(session_engine);
    if(!lease->active||handler!=lease->owner.handler.pointer||
       c.pc!=lease->owner.pc.pointer||c.pawn!=lease->owner.pawn.pointer||
       !session_handle_matches(lease->owner.pawn)||lease->owner.token!=guest_player_context_registration_token()||
       !guest_player_context_registered_for(c.pc,c.pawn)||
       !guest_player_context_basic_owned(base,c.pc,c.pawn,c.world)||
       !runtime_power_icon_owner(base,&lease->owner,c)) {
        original(self,destination,source,from_power,silent);return;
    }
    /* A leased guest wheel must not fall through to saved-definition writes
     * when a source, caller or layout is unavailable. */
    if(lease->retiring)return;
    if(destination<0||destination>=8) {runtime_power_assignment_reject(lease,"destination");return;}
    RuntimePowerIconSlot *target=&lease->slots[destination];
    if(!runtime_power_icon_layout(base,&lease->owner,(unsigned)destination,target->icon)) {lease->failed=1;return;}
    uint32_t empty[2]={0};
    if(runtime_power_icon_values(target->icon.pointer,(SessionHandle){0},0)&&
       runtime_power_icon_none(base,target->icon.pointer,empty)) {
        /* Retain the pre-lease baseline. A rejected drop must still own the
         * wrapper's clear, allowing idle retirement to restore this slot. */
        target->installed=(SessionHandle){0};memcpy(target->installed_name,empty,8);target->changed=1;
    } else if(!runtime_power_icon_expected(target,lease->owner.pawn.pointer)) {
        lease->failed=1;return;
    }
    /* NewSet and its pre-clear bridge are decoded indexed requests. Native
     * EndDrag is a separate direct virtual caller with the captured gesture.
     * MoveByName's saved source still cannot enter through this path. */
    RuntimePowerCatalog catalog;
    const char *rejection=NULL;
    int native_drag=runtime_power_assignment_native_drag(base,handler,caller,destination,source,from_power,silent);
    if(caller!=base+0xb68c55&&!runtime_assignment_decoded&&!native_drag)rejection="indexed-caller";
    else if(from_power>1||silent>1)rejection="indexed-booleans";
    else if(lease->failed)rejection="prior-widget-failure";
    else if(!runtime_power_ui_allowed(base,c))rejection="ui-mode";
    else if(runtime_power_wheel_drag_state(base,handler)<0)rejection="drag-layout";
    else if(!runtime_power_read_membership(base,c,&catalog))rejection="membership";
    else if(!runtime_power_catalog_same(&catalog,&lease->catalog))rejection="catalog-changed";
    if(rejection){runtime_power_assignment_reject(lease,rejection);return;}
    int index=-1;
    if(!from_power) {
        if(source<0||source>=8) {runtime_power_assignment_reject(lease,"source-slot");return;}
        index=runtime_power_slot_index(&catalog,(unsigned)source);
        RuntimePowerIconSlot *origin=&lease->slots[source];
        if(index<0||!runtime_power_icon_layout(base,&lease->owner,(unsigned)source,origin->icon)||
           (source!=destination&&!runtime_power_icon_matches(origin->icon.pointer,catalog.powers[index],
               c.pawn,catalog.names[index]))) {runtime_power_assignment_reject(lease,"source-icon");return;}
    } else {
        GameArray icons={0},after={0};uintptr_t object=0,power=0;uint32_t name[2]={0};
        if(source<0||!runtime_power_property(base,handler,"SFXSFHandler_PowerWheel","m_aPowerIcons","ArrayProperty",0x45c,16)||
           !identity_array(handler+0x45c,&icons,RUNTIME_POWER_LIMIT*3)||source>=icons.count||
           !read_at(icons.data+(size_t)source*8,&object,8)||
           !narrative_probe_object(base,object,"SFXGUIValue_PowerIcon")||read_ptr(object)!=base+0x13b20f8||
           read_ptr(object+0x40)!=handler||
           !runtime_power_property(base,object,"SFXGUIValue_PowerIcon","pPower","ObjectProperty",0x194,8)||
           !runtime_power_property(base,object,"SFXGUIValue_PowerIcon","pPawn","ObjectProperty",0x19c,8)||
           !runtime_power_property(base,object,"SFXGUIValue_PowerIcon","nmPowerName","NameProperty",0x1a4,8)||
           !read_at(object+0x194,&power,8)||!read_at(object+0x1a4,name,8)) {runtime_power_assignment_reject(lease,"power-list-source");return;}
        index=runtime_power_name_index(&catalog,name);
        if(index<0||catalog.powers[index].pointer!=power||
           !runtime_power_icon_matches(object,catalog.powers[index],c.pawn,name)||
           !read_at(handler+0x45c,&after,sizeof after)||memcmp(&icons,&after,sizeof icons)||
           read_ptr(icons.data+(size_t)source*8)!=object) {runtime_power_assignment_reject(lease,"power-list-changed");return;}
    }
    if(read_ptr(catalog.powers[index].pointer+0x98)!=c.pawn||
       !runtime_power_assign_slot((unsigned)destination,catalog.names[index])) {runtime_power_assignment_reject(lease,"power-identity-or-map");return;}
    /* Keep the icon baseline, repaint on the first idle Tick, and invalidate
     * commands/list selections accepted against the previous assignment. */
    lease->drop_accepted=1;lease->drag_observed=0;runtime_power_invalidate();
    if(native_drag)line("{\"runtime_power_input\":\"guest-native-drag-slot-assigned\",\"profile_write\":false,\"gameplay_verified\":false}\r\n");
    else line("{\"runtime_power_input\":\"guest-slot-assigned\",\"profile_write\":false,\"gameplay_verified\":false}\r\n");
}
/* NewSet's native body clears the destination before calling the assignment
 * virtual. Observe its decoded request BEFORE that clear for the owned guest
 * wheel. The map supplies both slot identities; the idle painter performs the
 * final UI setters, without an intermediate empty icon invalidating the lease. */
static void runtime_power_new_assignment(void *self,int destination,int source,uint32_t from_power,uint32_t silent) {
    DWORD error=GetLastError();RuntimePowerIconLease *lease=&runtime_power_icons;
    if(GetCurrentThreadId()!=session_game_thread||!runtime_power_icons_enabled()||
       !runtime_control_enabled||!runtime_scope_ready||g_runtime_config.role!=ME3_ROLE_GUEST||
       !InterlockedCompareExchange(&g_enable_requested,0,0)||!lease->active||
       lease->owner.handler.pointer!=(uintptr_t)self||runtime_assignment_decoded) {
        SetLastError(error);runtime_original_new_assignment(self,destination,source,from_power,silent);return;
    }
    SessionContext c=session_context(session_engine);
    if(c.pc!=lease->owner.pc.pointer||c.pawn!=lease->owner.pawn.pointer||
       !guest_player_context_registered_for(c.pc,c.pawn)||
       !guest_player_context_basic_owned(runtime_assignment_base,c.pc,c.pawn,c.world)||
       !runtime_power_icon_owner(runtime_assignment_base,&lease->owner,c)) {
        SetLastError(error);runtime_original_new_assignment(self,destination,source,from_power,silent);return;
    }
    runtime_assignment_decoded=1;
    runtime_power_assignment(self,destination,source,from_power,silent);
    runtime_assignment_decoded=0;SetLastError(error);
}
static int runtime_power_new_assignment_active(uintptr_t base) {
    unsigned char got[16];return runtime_new_assignment_installed&&base==runtime_assignment_base&&
        read_at(base+0xc505e0,got,sizeof got)&&!memcmp(got,runtime_new_assignment_entry,sizeof got);
}
static int runtime_power_new_assignment_install(uintptr_t base) {
    if(runtime_new_assignment_installed)return runtime_power_new_assignment_active(base);
    static const unsigned char entry[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x10,0x48,0x89,0x7c,0x24,0x18,0x41};
    unsigned char got[16];if(!read_at(base+0xc505e0,got,sizeof got)||memcmp(got,entry,sizeof got))return 0;
    MH_STATUS status=MH_Initialize();if(status!=MH_OK&&status!=MH_ERROR_ALREADY_INITIALIZED)return 0;
    LPVOID original=NULL;
    if(MH_CreateHook((LPVOID)(base+0xc505e0),(LPVOID)runtime_power_new_assignment,&original)!=MH_OK)return 0;
    runtime_original_new_assignment=(RuntimePowerAssignment)original;
    if(MH_EnableHook((LPVOID)(base+0xc505e0))!=MH_OK)return 0;
    if(!read_at(base+0xc505e0,runtime_new_assignment_entry,sizeof runtime_new_assignment_entry))return 0;
    runtime_new_assignment_installed=1;
    line("{\"runtime_power_input\":\"guest-preclear-slot-bridge-installed\",\"profile_write\":false}\r\n");return 1;
}
static void runtime_power_named_assignment(void *self,int destination,uint64_t packed) {
    DWORD error=GetLastError();uintptr_t handler=(uintptr_t)self,base=runtime_assignment_base;
    uintptr_t caller=(uintptr_t)__builtin_return_address(0);RuntimePowerIconLease *lease=&runtime_power_icons;
    if(GetCurrentThreadId()!=session_game_thread||!runtime_named_assignment_installed||
       !runtime_power_icons_enabled()||!runtime_control_enabled||!runtime_scope_ready||
       g_runtime_config.role!=ME3_ROLE_GUEST||!InterlockedCompareExchange(&g_enable_requested,0,0)||
       !lease->active||handler!=lease->owner.handler.pointer) {
        SetLastError(error);runtime_original_named_assignment(self,destination,packed);return;
    }
    SessionContext c=session_context(session_engine);RuntimePowerCatalog catalog;uint32_t name[2];memcpy(name,&packed,8);
    int owned=c.pc==lease->owner.pc.pointer&&c.pawn==lease->owner.pawn.pointer&&
        session_handle_matches(lease->owner.pawn)&&lease->owner.token==guest_player_context_registration_token()&&
        guest_player_context_registered_for(c.pc,c.pawn)&&guest_player_context_basic_owned(base,c.pc,c.pawn,c.world)&&
        runtime_power_icon_owner(base,&lease->owner,c);
    if(!owned){SetLastError(error);runtime_original_named_assignment(self,destination,packed);return;}
    /* This native body normally searches saved Shepard definitions. Resolve
     * its decoded FName from this guest's existing catalog instead. */
    if(lease->retiring||lease->failed)goto done;
    if(caller!=base+0xb68da3||destination<0||destination>=8||
       !runtime_power_ui_allowed(base,c)||runtime_power_wheel_drag_state(base,handler)<0||
       !runtime_power_read_membership(base,c,&catalog)||!runtime_power_catalog_same(&catalog,&lease->catalog)) {
        runtime_power_assignment_reject(lease,"named-caller-or-catalog");goto done;
    }
    int index=runtime_power_name_index(&catalog,name);
    if(index<0||read_ptr(catalog.powers[index].pointer+0x98)!=c.pawn) {
        runtime_power_assignment_reject(lease,"named-power");goto done;
    }
    for(unsigned n=0;n<8;++n) {
        RuntimePowerIconSlot *slot=&lease->slots[n];
        if(!runtime_power_icon_layout(base,&lease->owner,n,slot->icon)||!runtime_power_icon_expected(slot,c.pawn)) {
            lease->failed=1;goto done;
        }
    }
    if(!runtime_power_assign_slot((unsigned)destination,name)) {
        runtime_power_assignment_reject(lease,"ambiguous-name-map");goto done;
    }
    lease->drop_accepted=1;lease->drag_observed=0;runtime_power_invalidate();
    line("{\"runtime_power_input\":\"guest-named-slot-assigned\",\"profile_write\":false,\"gameplay_verified\":false}\r\n");
done:SetLastError(error);
}
static int runtime_power_named_assignment_active(uintptr_t base) {
    unsigned char got[16];return runtime_named_assignment_installed&&base==runtime_assignment_base&&
        read_at(base+0xc50570,got,sizeof got)&&!memcmp(got,runtime_named_assignment_entry,sizeof got);
}
static int runtime_power_named_assignment_install(uintptr_t base) {
    if(runtime_named_assignment_installed)return runtime_power_named_assignment_active(base);
    static const unsigned char entry[]={0x4c,0x89,0x44,0x24,0x18,0x53,0x48,0x83,0xec,0x30,0x4c,0x8b,0x99,0xf8,0x06,0x00};
    static const unsigned char call[]={0xe8,0xcd,0x77,0x0e,0x00}; /* b68d9e -> c50570 */
    unsigned char got[16];
    if(!read_at(base+0xc50570,got,sizeof entry)||memcmp(got,entry,sizeof entry)||
       !read_at(base+0xb68d9e,got,sizeof call)||memcmp(got,call,sizeof call))return 0;
    MH_STATUS status=MH_Initialize();if(status!=MH_OK&&status!=MH_ERROR_ALREADY_INITIALIZED)return 0;
    LPVOID original=NULL;
    if(MH_CreateHook((LPVOID)(base+0xc50570),(LPVOID)runtime_power_named_assignment,&original)!=MH_OK)return 0;
    runtime_original_named_assignment=(RuntimePowerNamedAssignment)original;
    if(MH_EnableHook((LPVOID)(base+0xc50570))!=MH_OK)return 0;
    if(!read_at(base+0xc50570,runtime_named_assignment_entry,sizeof runtime_named_assignment_entry))return 0;
    runtime_named_assignment_installed=1;
    line("{\"runtime_power_input\":\"guest-named-slot-bridge-installed\",\"profile_write\":false}\r\n");return 1;
}
static int runtime_power_assignment_active(uintptr_t base) {
    return runtime_assignment_installed&&read_ptr(base+0x14f6700+0x548)==(uintptr_t)runtime_power_assignment&&
        runtime_power_named_assignment_active(base)&&runtime_power_new_assignment_active(base);
}
static int runtime_power_assignment_install(uintptr_t base) {
    uintptr_t slot=base+0x14f6700+0x548,expected=base+0xc52eb0;
    if(runtime_assignment_installed)return runtime_power_named_assignment_install(base)&&
        runtime_power_new_assignment_install(base)&&runtime_power_assignment_active(base);
    static const unsigned char want[]={0x48,0x8b,0xc4,0x55,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x8b,0xec};
    unsigned char got[sizeof want];DWORD old=0;
    if(read_ptr(slot)!=expected||!read_at(expected,got,sizeof got)||memcmp(got,want,sizeof want)||
       !VirtualProtect((void*)slot,8,PAGE_READWRITE,&old))return 0;
    runtime_assignment_base=base;runtime_original_assignment=(RuntimePowerAssignment)expected;
    PVOID previous=InterlockedCompareExchangePointer((PVOID volatile*)slot,(PVOID)runtime_power_assignment,(PVOID)expected);
    VirtualProtect((void*)slot,8,old,&old);
    runtime_assignment_installed=(uintptr_t)previous==expected;
    return runtime_assignment_installed&&runtime_power_named_assignment_install(base)&&runtime_power_new_assignment_install(base);
}
#ifdef ME3_RUNTIME_FIXTURE
static unsigned runtime_assignment_fixture_calls;
static int runtime_assignment_fixture_bad;
static void runtime_assignment_fixture_stock(void *self,int destination,int source,uint32_t from_power,uint32_t silent) {
    ++runtime_assignment_fixture_calls;
    if(self!=(void*)16||destination!=3||source!=7||from_power!=1||silent||GetLastError()!=111)
        runtime_assignment_fixture_bad=1;
    SetLastError(222);
}
static int runtime_power_assignment_selftest(void) {
    DWORD error=GetLastError();int enabled=runtime_control_enabled;
    RuntimePowerAssignment original=runtime_original_new_assignment;
    runtime_control_enabled=0;runtime_original_new_assignment=runtime_assignment_fixture_stock;
    runtime_assignment_fixture_calls=0;runtime_assignment_fixture_bad=0;
    SetLastError(111);runtime_power_new_assignment((void*)16,3,7,1,0);
    int forwarded=runtime_assignment_fixture_calls==1&&!runtime_assignment_fixture_bad&&GetLastError()==222;
    runtime_control_enabled=enabled;runtime_original_new_assignment=original;SetLastError(error);
    RuntimePowerSlot saved[9];memcpy(saved,runtime_power_slots,sizeof saved);memset(runtime_power_slots,0,sizeof runtime_power_slots);
    uint32_t first[2]={41,0},second[2]={42,0};
    int ok=forwarded&&
        runtime_power_assignment_drag_policy(1,0,0,0,1,0,12)&&
        runtime_power_assignment_drag_policy(7,13,1,0,7,13,10)&&
        !runtime_power_assignment_drag_policy(1,0,0,0,2,0,12)&&
        !runtime_power_assignment_drag_policy(1,0,0,0,1,2,12)&&
        !runtime_power_assignment_drag_policy(1,0,0,0,1,0,4)&&
        !runtime_power_assignment_drag_policy(1,0,0,0,1,0,10)&&
        !runtime_power_assignment_drag_policy(1,0,1,0,1,0,12)&&
        !runtime_power_assignment_drag_policy(1,0,1,0,1,0,14)&&
        !runtime_power_assignment_drag_policy(1,0,0,1,1,0,12)&&
        !runtime_power_assignment_drag_policy(-1,0,0,0,-1,0,12)&&
        !runtime_power_assignment_drag_policy(8,0,0,0,8,0,12)&&
        !runtime_power_assignment_drag_policy(1,-1,0,0,1,-1,12)&&
        !runtime_power_assignment_drag_policy(1,0,2,0,1,0,12)&&
        runtime_power_drag_blocks_input(-1)&&!runtime_power_drag_blocks_input(0)&&
        runtime_power_drag_blocks_input(1)&&!runtime_power_drag_blocks_input(2)&&runtime_power_drag_blocks_input(3)&&
        runtime_power_assign_slot(0,first)&&runtime_power_assign_slot(1,second)&&runtime_power_assign_slot(1,first)&&
        !memcmp(runtime_power_slots[1].name,first,8)&&!memcmp(runtime_power_slots[0].name,second,8)&&
        runtime_power_assign_slot(7,first)&&!memcmp(runtime_power_slots[7].name,first,8)&&
        !runtime_power_slots[1].bound&&!runtime_power_assign_slot(8,first)&&
        runtime_power_assignment_reject_policy(7,1,1)&&!runtime_power_assignment_reject_policy(8,1,1)&&
        !runtime_power_assignment_reject_policy(0,0,1)&&!runtime_power_assignment_reject_policy(0,1,0);
    memcpy(runtime_power_slots,saved,sizeof saved);return ok;
}
#endif
#endif
