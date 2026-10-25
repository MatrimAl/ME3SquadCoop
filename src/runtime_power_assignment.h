#ifndef ME3_RUNTIME_POWER_ASSIGNMENT_H
#define ME3_RUNTIME_POWER_ASSIGNMENT_H
/* NewSetQuickSlotPower clears its destination before virtual+0x548. Handle
 * the already-decoded native drop here without parsing a VM frame, touching
 * saved definitions or recursively invoking native UI/gameplay methods. */
typedef void (*RuntimePowerAssignment)(void*,int,int,uint32_t,uint32_t);
static RuntimePowerAssignment runtime_original_assignment;
static uintptr_t runtime_assignment_base;
static int runtime_assignment_installed;
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
    if(destination<0||destination>=8) {lease->failed=1;return;}
    RuntimePowerIconSlot *target=&lease->slots[destination];
    if(!runtime_power_icon_layout(base,&lease->owner,(unsigned)destination,target->icon)) {lease->failed=1;return;}
    uint32_t empty[2]={0};
    if(runtime_power_icon_values(target->icon.pointer,(SessionHandle){0},0)&&
       runtime_power_icon_none(base,target->icon.pointer,empty)) {
        /* Retain the pre-lease baseline. A rejected drop must still own the
         * wrapper's clear, allowing idle retirement to restore this slot. */
        target->installed=(SessionHandle){0};memcpy(target->installed_name,empty,8);target->changed=1;
    } else if(target->changed?!runtime_power_icon_matches(target->icon.pointer,target->installed,
              target->installed.pointer?lease->owner.pawn.pointer:0,target->installed_name):
              !runtime_power_icon_values(target->icon.pointer,target->original,target->original_pawn.pointer)) {
        lease->failed=1;return;
    }
    /* The verified NewSet exec calls the wrapper, which tail-jumps here.
     * MoveByName instead derives a source from saved Shepard definitions. */
    RuntimePowerCatalog catalog;
    if(caller!=base+0xb68c55||from_power>1||silent>1||lease->failed||session_engine_blocked(c)||
       runtime_power_wheel_drag_state(base,handler)<0||
       !runtime_power_read_membership(base,c,&catalog)||!runtime_power_catalog_same(&catalog,&lease->catalog)) {
        lease->failed=1;return;
    }
    int index=-1;
    if(!from_power) {
        if(source<0||source>=8) {lease->failed=1;return;}
        index=runtime_power_slot_index(&catalog,(unsigned)source);
        RuntimePowerIconSlot *origin=&lease->slots[source];
        if(index<0||!runtime_power_icon_layout(base,&lease->owner,(unsigned)source,origin->icon)||
           (source!=destination&&!runtime_power_icon_matches(origin->icon.pointer,catalog.powers[index],
               c.pawn,catalog.names[index]))) {lease->failed=1;return;}
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
           !read_at(object+0x194,&power,8)||!read_at(object+0x1a4,name,8)) {lease->failed=1;return;}
        index=runtime_power_name_index(&catalog,name);
        if(index<0||catalog.powers[index].pointer!=power||
           !runtime_power_icon_matches(object,catalog.powers[index],c.pawn,name)||
           !read_at(handler+0x45c,&after,sizeof after)||memcmp(&icons,&after,sizeof icons)||
           read_ptr(icons.data+(size_t)source*8)!=object) {lease->failed=1;return;}
    }
    if(read_ptr(catalog.powers[index].pointer+0x98)!=c.pawn||
       !runtime_power_assign_slot((unsigned)destination,catalog.names[index])) {lease->failed=1;return;}
    /* Keep the icon baseline, repaint on the first idle Tick, and invalidate
     * commands/list selections accepted against the previous assignment. */
    lease->drop_accepted=1;lease->drag_observed=0;runtime_power_invalidate();
    line("{\"runtime_power_input\":\"guest-slot-assigned\",\"profile_write\":false,\"gameplay_verified\":false}\r\n");
}
static int runtime_power_assignment_active(uintptr_t base) {
    return runtime_assignment_installed&&read_ptr(base+0x14f6700+0x548)==(uintptr_t)runtime_power_assignment;
}
static int runtime_power_assignment_install(uintptr_t base) {
    uintptr_t slot=base+0x14f6700+0x548,expected=base+0xc52eb0;
    if(runtime_assignment_installed)return runtime_power_assignment_active(base);
    static const unsigned char want[]={0x48,0x8b,0xc4,0x55,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x8b,0xec};
    unsigned char got[sizeof want];DWORD old=0;
    if(read_ptr(slot)!=expected||!read_at(expected,got,sizeof got)||memcmp(got,want,sizeof want)||
       !VirtualProtect((void*)slot,8,PAGE_READWRITE,&old))return 0;
    runtime_assignment_base=base;runtime_original_assignment=(RuntimePowerAssignment)expected;
    PVOID previous=InterlockedCompareExchangePointer((PVOID volatile*)slot,(PVOID)runtime_power_assignment,(PVOID)expected);
    VirtualProtect((void*)slot,8,old,&old);
    runtime_assignment_installed=(uintptr_t)previous==expected;
    return runtime_assignment_installed;
}
#endif
