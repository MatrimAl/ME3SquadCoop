#ifndef ME3_RUNTIME_POWER_ICONS_H
#define ME3_RUNTIME_POWER_ICONS_H
/* Optional native UI lease. Observe the controller's existing wheel through
 * its hotkey call; never load a movie or copy native FString allocations. */
typedef struct RuntimePowerIconSlot {
    SessionHandle icon,original,original_pawn,installed;
    uint32_t installed_name[2];
    int changed;
} RuntimePowerIconSlot;
typedef struct RuntimePowerIconLease {
    RuntimeHotkeyWheel owner;
    RuntimePowerIconSlot slots[8];
    RuntimePowerCatalog catalog;
    int active,failed,drag_observed,drop_accepted,retiring;
} RuntimePowerIconLease;
static RuntimePowerIconLease runtime_power_icons;
static int runtime_power_assignment_install(uintptr_t base);
static int runtime_power_assignment_active(uintptr_t base);
static int runtime_power_ui_allowed(uintptr_t base,SessionContext c);
static int runtime_power_caster_install(uintptr_t base);
static int runtime_power_caster_active(uintptr_t base);
static int runtime_power_icons_enabled(void) {
    static int configured,opted;
    if(!configured) {
        wchar_t value[8]={0};
        DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_NATIVE_POWER_ICONS",value,8);
        opted=n==1&&value[0]==L'1';configured=1;
    }
    return opted&&runtime_native_power_input_enabled();
}
static int runtime_power_icon_owner(uintptr_t base,const RuntimeHotkeyWheel *owner,SessionContext c) {
    uintptr_t handler=owner->handler.pointer;
    return c.world==owner->world.pointer&&session_handle_matches(owner->world)&&
        session_handle_matches(owner->pc)&&session_handle_matches(owner->handler)&&
        narrative_probe_object(base,handler,"SFXSFHandler_PCPowerWheel")&&
        read_ptr(handler)==base+0x14f6700&&read_ptr(handler+0x6f8)==owner->pc.pointer&&
        runtime_power_property(base,handler,"SFXSFHandler_PowerWheel","m_pPlayerController","ObjectProperty",0x6f8,8)&&
        runtime_power_property(base,handler,"SFXGUIMovie","oWorldInfo","ObjectProperty",0x2bc,8)&&
        read_ptr(handler+0x2bc)==owner->world.pointer&&
        runtime_power_property_dim(base,handler,"SFXSFHandler_PCPowerWheel","m_aQuickSlotIcons","ObjectProperty",0xbd8,8,8);
}
static int runtime_power_icon_layout(uintptr_t base,const RuntimeHotkeyWheel *owner,unsigned slot,SessionHandle icon) {
    uintptr_t object=icon.pointer;
    return slot<8&&session_handle_matches(icon)&&
        read_ptr(owner->handler.pointer+0xbd8+slot*8)==object&&read_ptr(object+0x40)==owner->handler.pointer&&
        narrative_probe_object(base,object,"SFXGUIValue_QuickSlotPowerIcon")&&read_ptr(object)==base+0x13b29b8&&
        runtime_power_property(base,object,"SFXGUIValue_PowerIcon","pPower","ObjectProperty",0x194,8)&&
        runtime_power_property(base,object,"SFXGUIValue_PowerIcon","pPawn","ObjectProperty",0x19c,8)&&
        runtime_power_property(base,object,"SFXGUIValue_PowerIcon","nmPowerName","NameProperty",0x1a4,8);
}
static uintptr_t runtime_power_icon_setter(uintptr_t base,uintptr_t object) {
    uintptr_t fn=guest_player_context_find_method(base,read_ptr(object+0x50),"SFXGUIValue_PowerIcon","SetPower");
    if(read_ptr(read_ptr(object)+0x230)!=base+0xf4f80||
       !guest_player_context_function_abi(base,fn,"SFXGUIValue_PowerIcon","SetPower",0x20401,8,65535,0,0xb58aa0,1))return 0;
    uintptr_t field=read_ptr(fn+0x70);int32_t dim=0,bytes=0,offset=-1;uint64_t flags=0;char name[96];
    if(!narrative_probe_object(base,field,"ObjectProperty")||read_ptr(field+0x40)!=fn)return 0;
    object_name(base,field,name,sizeof name);
    return !strcmp(name,"pNewPower")&&!read_ptr(field+0x68)&&
        read_at(field+0x70,&dim,4)&&dim==1&&read_at(field+0x74,&bytes,4)&&bytes==8&&
        read_at(field+0x94,&offset,4)&&offset==0&&read_at(field+0x78,&flags,8)&&flags==0x80?fn:0;
}
static int runtime_power_icon_values(uintptr_t object,SessionHandle power,uintptr_t pawn) {
    uintptr_t got_power=0,got_pawn=0;
    return (!power.pointer||session_handle_matches(power))&&read_at(object+0x194,&got_power,8)&&
        got_power==power.pointer&&read_at(object+0x19c,&got_pawn,8)&&got_pawn==pawn;
}
static int runtime_power_icon_matches(uintptr_t object,SessionHandle power,uintptr_t pawn,const uint32_t name[2]) {
    uint32_t got[2]={0};
    return runtime_power_icon_values(object,power,pawn)&&read_at(object+0x1a4,got,8)&&!memcmp(got,name,8);
}
static int runtime_power_icon_none(uintptr_t base,uintptr_t object,uint32_t name[2]) {
    char text[80];
    if(!read_at(object+0x1a4,name,8)||name[1])return 0;
    name_of(base,name[0],text,sizeof text);return !strcmp(text,"None");
}
static void runtime_power_icon_set(uintptr_t base,uintptr_t object,uintptr_t method,uintptr_t power) {
    ((ProcessEvent)(base+0xf4f80))((void*)object,(void*)method,&power,NULL);
}
static void runtime_power_icons_release(uintptr_t base,uintptr_t engine) {
    RuntimePowerIconLease *lease=&runtime_power_icons;
    if(!lease->active)return;
    lease->retiring=1;
    /* Revalidate after every native setter. Never invoke an old-world widget
     * or undo an engine/UI replacement that no longer holds our value. */
    for(unsigned n=0;n<8;++n) {
        RuntimePowerIconSlot *s=&lease->slots[n];
        if(!s->changed)continue;
        s->changed=0;SessionContext c=session_context(engine);
        if(!runtime_power_icon_owner(base,&lease->owner,c)||
           runtime_power_wheel_drag_state(base,lease->owner.handler.pointer)!=0||
           !runtime_power_icon_layout(base,&lease->owner,n,s->icon)||
           !runtime_power_icon_values(s->icon.pointer,s->installed,
               s->installed.pointer?lease->owner.pawn.pointer:0))continue;
        uintptr_t original=0;
        if(s->original.pointer&&session_handle_matches(s->original)&&session_handle_matches(s->original_pawn)&&
           narrative_probe_object(base,s->original.pointer,"SFXPowerCustomActionBase")&&
           read_ptr(s->original.pointer+0x98)==s->original_pawn.pointer)original=s->original.pointer;
        uintptr_t method=runtime_power_icon_setter(base,s->icon.pointer);
        if(method)runtime_power_icon_set(base,s->icon.pointer,method,original);
    }
    if(runtime_power_same_handle(runtime_hotkey_wheel.handler,lease->owner.handler)&&
       runtime_power_same_handle(runtime_hotkey_wheel.pc,lease->owner.pc)&&
       runtime_power_same_handle(runtime_hotkey_wheel.pawn,lease->owner.pawn)&&
       runtime_power_same_handle(runtime_hotkey_wheel.world,lease->owner.world)&&
       runtime_hotkey_wheel.token==lease->owner.token)runtime_hotkey_wheel.observed=0;
    memset(lease,0,sizeof *lease);
    line("{\"runtime_power_icons\":\"lease-retired\",\"restoration_verified\":false}\r\n");
}
static void runtime_power_icons_tick(uintptr_t base,uintptr_t engine,int ordinary) {
    RuntimePowerIconLease *lease=&runtime_power_icons;
    if(!lease->active&&!runtime_power_icons_enabled())return;
    if(GetCurrentThreadId()!=session_game_thread)return;
    SessionContext c=session_context(engine);RuntimePowerCatalog catalog;
    int ready=runtime_power_icons_enabled()&&runtime_native_power_input_active()&&runtime_control_enabled&&
        runtime_scope_ready&&runtime_power_ui_allowed(base,c)&&
        runtime_power_read_membership(base,c,&catalog);
    if(lease->active&&(lease->failed||!runtime_power_caster_active(base)||!runtime_power_assignment_active(base)||!runtime_power_icons_enabled()||!runtime_native_power_input_active()||
       !runtime_control_enabled||!runtime_scope_ready||!runtime_power_ui_allowed(base,c)||
       c.pc!=lease->owner.pc.pointer||c.pawn!=lease->owner.pawn.pointer||
       !session_handle_matches(lease->owner.pawn)||lease->owner.token!=guest_player_context_registration_token()||
       !guest_player_context_registered_for(c.pc,c.pawn)||!guest_player_context_basic_owned(base,c.pc,c.pawn,c.world)||
       (ready&&!runtime_power_catalog_same(&catalog,&lease->catalog))||
       !runtime_power_icon_owner(base,&lease->owner,c))) {
        runtime_power_icons_release(base,engine);return;
    }
    if(!ready)return;
    if(!lease->active) {
        RuntimeHotkeyWheel owner=runtime_hotkey_wheel;ULONGLONG now=GetTickCount64();
        if(!owner.observed||now<owner.observed||now-owner.observed>1000||
           c.pc!=owner.pc.pointer||c.pawn!=owner.pawn.pointer||!session_handle_matches(owner.pawn)||
           owner.token!=guest_player_context_registration_token()||!runtime_power_icon_owner(base,&owner,c)||
           !runtime_power_catalog_same(&catalog,&runtime_power_catalog))return;
        RuntimePowerIconLease captured={0};captured.owner=owner;captured.catalog=catalog;
        if(runtime_power_wheel_drag_state(base,owner.handler.pointer)!=0)return;
        for(unsigned n=0;n<8;++n) {
            RuntimePowerIconSlot *s=&captured.slots[n];
            s->icon=session_capture_handle(read_ptr(owner.handler.pointer+0xbd8+n*8));
            if(!runtime_power_icon_layout(base,&owner,n,s->icon)||!runtime_power_icon_setter(base,s->icon.pointer))return;
            uintptr_t power=0,pawn=0;
            if(!read_at(s->icon.pointer+0x194,&power,8)||!read_at(s->icon.pointer+0x19c,&pawn,8))return;
            if(power) {
                s->original=session_capture_handle(power);s->original_pawn=session_capture_handle(pawn);
                if(!session_handle_matches(s->original)||!session_handle_matches(s->original_pawn)||
                   !narrative_probe_object(base,power,"SFXPowerCustomActionBase")||read_ptr(power+0x98)!=pawn)return;
            } else if(pawn)return;
        }
        if(!runtime_power_assignment_install(base)||!runtime_power_caster_install(base))return;
        captured.active=1;*lease=captured;
    }
    int drag=runtime_power_wheel_drag_state(base,lease->owner.handler.pointer);
    if(drag<0) {lease->failed=1;runtime_power_icons_release(base,engine);return;}
    if(drag) {
        if(lease->drop_accepted)return; /* Native release cleanup may finish next Tick. */
        if(!lease->drag_observed) {
            /* Cancel pre-drag requests. They may identify the old slot map,
             * even if the native drop later picks a different power. */
            runtime_power_invalidate();lease->drag_observed=1;
        }
        return;
    }
    if(lease->drop_accepted) {lease->drop_accepted=0;lease->drag_observed=0;}
    if(lease->drag_observed) {
        /* Native drop can update saved quickslot definitions independently of
         * our name map. Retire instead of repainting over that native edit. */
        runtime_power_icons_release(base,engine);return;
    }
    /* At most one new assignment per Tick; retirement separately restores at
     * most eight. Native UI work can invalidate every handle: return after a
     * setter and its revalidated readback, including a failed readback. */
    for(unsigned n=0;n<8;++n) {
        RuntimePowerIconSlot *s=&lease->slots[n];
        if(!runtime_power_icon_layout(base,&lease->owner,n,s->icon)) {lease->failed=1;break;}
        int index=runtime_power_slot_index(&catalog,n);
        SessionHandle desired=index>=0?catalog.powers[index]:(SessionHandle){0};
        uintptr_t pawn=desired.pointer?c.pawn:0;
        uint32_t name[2]={0};
        if(index>=0)memcpy(name,catalog.names[index],8);
        if(s->changed&&!runtime_power_icon_matches(s->icon.pointer,s->installed,
               s->installed.pointer?lease->owner.pawn.pointer:0,s->installed_name)) {lease->failed=1;break;}
        if(!s->changed&&!runtime_power_icon_values(s->icon.pointer,s->original,s->original_pawn.pointer)) {lease->failed=1;break;}
        if(!desired.pointer&&!read_ptr(s->icon.pointer+0x194)) {
            if(read_ptr(s->icon.pointer+0x19c)||!runtime_power_icon_none(base,s->icon.pointer,name)) {lease->failed=1;break;}
            continue;
        }
        if(runtime_power_icon_matches(s->icon.pointer,desired,pawn,name))continue;
        /* SetPower returns immediately for an equal pointer. Do not mistake
         * that no-op for repair of an inconsistent pawn/name cache. */
        if(read_ptr(s->icon.pointer+0x194)==desired.pointer) {lease->failed=1;break;}
        if(desired.pointer&&read_ptr(desired.pointer+0x98)!=c.pawn) {lease->failed=1;break;}
        uintptr_t method=runtime_power_icon_setter(base,s->icon.pointer);
        if(!method) {lease->failed=1;break;}
        s->installed=desired;memcpy(s->installed_name,name,8);s->changed=1;
        runtime_power_icon_set(base,s->icon.pointer,method,desired.pointer);
        SessionContext post=session_context(engine);
        if(post.pc!=lease->owner.pc.pointer||post.pawn!=lease->owner.pawn.pointer||
           !runtime_power_icon_owner(base,&lease->owner,post)||
           !runtime_power_icon_layout(base,&lease->owner,n,s->icon)) {lease->failed=1;return;}
        if(!desired.pointer) {
            /* Native SetPower(None) supplies the process's real None name. */
            if(!runtime_power_icon_none(base,s->icon.pointer,s->installed_name))lease->failed=1;
        }
        if(!runtime_power_icon_matches(s->icon.pointer,s->installed,pawn,s->installed_name))lease->failed=1;
        return;
    }
    if(lease->failed)runtime_power_icons_release(base,engine);
}
#endif
