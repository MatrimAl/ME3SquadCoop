#ifndef ME3_RUNTIME_POWER_WHEEL_CONTEXT_H
#define ME3_RUNTIME_POWER_WHEEL_CONTEXT_H
/* The native base wheel caches Pawn.Squad, while the guest input context may
 * use a separate registered squad. Lease UI references only; let native Tick
 * select the pawn and populate its groups. Never call population manually. */
typedef struct RuntimePowerWheelContext {
    RuntimeHotkeyWheel owner;
    RuntimePowerCatalog catalog;
    SessionHandle squad,baseline[4]; /* squad, player, hench1, hench2 */
    unsigned changed;
    int active,retiring;
} RuntimePowerWheelContext;
static RuntimePowerWheelContext runtime_power_wheel_context;
static int runtime_power_wheel_context_layout(uintptr_t base,const RuntimeHotkeyWheel *owner,SessionContext c) {
    static const char *fields[]={"m_pPlayerSquad","m_pShepardPawn","m_pHench1Pawn","m_pHench2Pawn"};
    static const int offsets[]={0x700,0x710,0x718,0x720};
    if(!runtime_power_icon_owner(base,owner,c))return 0;
    for(unsigned n=0;n<4;++n)
        if(!runtime_power_property(base,owner->handler.pointer,"SFXSFHandler_PowerWheel",fields[n],
            "ObjectProperty",offsets[n],8))return 0;
    return 1;
}
static uintptr_t runtime_power_wheel_registered_squad(uintptr_t base,SessionContext c) {
    NetworkActorIdentityEntry channel={0};CombatContextChain chain;
    if(!guest_player_context_registered_for(c.pc,c.pawn)||
       !guest_player_context_basic_owned(base,c.pc,c.pawn,c.world)||
       !guest_player_context_channel(base,c.pawn,&channel))return 0;
    combat_context_read_chain(base,&chain);
    if(!chain.info_valid||chain.info!=c.world||!chain.squad)return 0;
    if(guest_existing_context.registered) {
        if(!guest_existing_context_same(base,c.pc,c.pawn,c.world,&channel,&chain))return 0;
    } else {
        GuestPlayerContextLease *context=&guest_player_context_lease;
        if(context->pending_cleanup||!guest_player_context_same(base,c.pc,c.pawn,c.world,&channel)||
           context->squad.pointer!=chain.squad||!guest_player_context_squad_shape(base,c.pawn))return 0;
    }
    return narrative_probe_object(base,chain.squad,"BioPlayerSquad")&&
        read_ptr(chain.squad+0x198)==c.world&&read_ptr(chain.squad+0x2a8)==c.pawn&&
        read_ptr(chain.squad+0x2cc)==c.pawn?chain.squad:0;
}
static int runtime_power_wheel_pointer(uintptr_t at,uintptr_t expected,uintptr_t desired) {
    if(expected==desired)return read_ptr(at)==expected;
    if((at&7)||!background_focus_writable(at,8))return 0;
    return (uintptr_t)InterlockedCompareExchangePointer((PVOID volatile*)at,(PVOID)desired,(PVOID)expected)==expected;
}
static uintptr_t runtime_power_wheel_baseline(uintptr_t base,SessionHandle value,uintptr_t world,unsigned index) {
    return value.pointer&&session_handle_matches(value)&&guest_camera_actor_live(base,value.pointer)&&
        narrative_probe_object(base,value.pointer,index?"BioPawn":"BioBaseSquad")&&
        read_ptr(value.pointer+0x198)==world?value.pointer:0;
}
static void runtime_power_wheel_context_release(uintptr_t base,uintptr_t engine) {
    RuntimePowerWheelContext *lease=&runtime_power_wheel_context;
    if(!lease->active)return;
    lease->retiring=1;SessionContext c=session_context(engine);
    uintptr_t handler=lease->owner.handler.pointer;
    if(runtime_power_wheel_context_layout(base,&lease->owner,c)&&
       read_ptr(handler+0x700)==lease->squad.pointer) {
        static const int offsets[]={0x700,0x710,0x718,0x720};
        /* Restoring is conditional on our exact installed value. A disposed
         * baseline becomes null; a native replacement is never overwritten. */
        for(unsigned n=2;n<4;++n)if(lease->changed&(1u<<n)) {
            uintptr_t installed=0;
            uintptr_t baseline=runtime_power_wheel_baseline(base,lease->baseline[n],c.world,n);
            runtime_power_wheel_pointer(handler+offsets[n],installed,baseline);
        }
        if(lease->changed&1)
            runtime_power_wheel_pointer(handler+0x700,lease->squad.pointer,
                runtime_power_wheel_baseline(base,lease->baseline[0],c.world,0));
    }
    memset(lease,0,sizeof *lease);
    line("{\"runtime_power_wheel_context\":\"retired\",\"restoration_verified\":false}\r\n");
}
static int runtime_power_wheel_context_valid(uintptr_t base,SessionContext c) {
    RuntimePowerWheelContext *lease=&runtime_power_wheel_context;
    uint32_t flags=0;uintptr_t hench1=1,hench2=1;
    return lease->active&&!lease->retiring&&c.pc==lease->owner.pc.pointer&&c.pawn==lease->owner.pawn.pointer&&
        session_handle_matches(lease->owner.pawn)&&session_handle_matches(lease->squad)&&
        lease->owner.token==guest_player_context_registration_token()&&
        runtime_power_wheel_context_layout(base,&lease->owner,c)&&
        runtime_power_wheel_registered_squad(base,c)==lease->squad.pointer&&
        read_ptr(lease->owner.handler.pointer+0x700)==lease->squad.pointer&&
        read_at(lease->owner.handler.pointer+0x7f8,&flags,4)&&!(flags&0x10)&&
        read_at(lease->owner.handler.pointer+0x718,&hench1,8)&&!hench1&&
        read_at(lease->owner.handler.pointer+0x720,&hench2,8)&&!hench2;
}
static void runtime_power_wheel_context_tick(uintptr_t base,uintptr_t engine) {
    RuntimePowerWheelContext *lease=&runtime_power_wheel_context;
    if(GetCurrentThreadId()!=session_game_thread||(!lease->active&&!runtime_power_icons_enabled()))return;
    SessionContext c=session_context(engine);RuntimePowerCatalog catalog;
    int allowed=runtime_power_icons_enabled()&&runtime_native_power_input_active()&&runtime_control_enabled&&
        runtime_scope_ready&&g_runtime_config.role==ME3_ROLE_GUEST&&InterlockedCompareExchange(&g_enable_requested,0,0)&&
        (!session_engine_blocked(c)||runtime_power_menu_context_held(base,c))&&
        runtime_power_read_membership(base,c,&catalog);
    if(lease->active) {
        if(!allowed||!runtime_power_caster_active(base)||!runtime_power_wheel_context_valid(base,c))
            runtime_power_wheel_context_release(base,engine);
        else lease->catalog=catalog;
        return;
    }
    if(!allowed||session_engine_blocked(c))return; /* No acquisition inside a menu. */
    if(runtime_hotkey_wheel.registry_observation)runtime_hotkey_wheel.observed=0;
    runtime_power_wheel_discover(base,c,1);
    RuntimeHotkeyWheel owner=runtime_hotkey_wheel;ULONGLONG now=GetTickCount64();
    if(!owner.observed||now<owner.observed||now-owner.observed>1000||
       owner.pc.pointer!=c.pc||owner.pawn.pointer!=c.pawn||!session_handle_matches(owner.pawn)||
       owner.token!=guest_player_context_registration_token()||
       !runtime_power_wheel_context_layout(base,&owner,c))return;
    uintptr_t squad=runtime_power_wheel_registered_squad(base,c);uint32_t flags=0;
    if(!squad||!read_at(owner.handler.pointer+0x7f8,&flags,4)||(flags&0x10)||
       runtime_power_drag_blocks_input(runtime_power_wheel_drag_state(base,owner.handler.pointer)))return;
    RuntimePowerWheelContext next={0};next.owner=owner;next.catalog=catalog;
    next.squad=session_capture_handle(squad);
    static const int offsets[]={0x700,0x710,0x718,0x720};
    for(unsigned n=0;n<4;++n) {
        uintptr_t value=0;
        if(!read_at(owner.handler.pointer+offsets[n],&value,8))return;
        if(value) {
            next.baseline[n]=session_capture_handle(value);
            if(runtime_power_wheel_baseline(base,next.baseline[n],c.world,n)!=value)return;
        }
    }
    if(!session_handle_matches(next.squad)||!runtime_power_caster_install(base))return;
    next.active=1;*lease=next;
    if(next.baseline[0].pointer!=squad) {
        /* Native Tick owns player+710 and the corresponding group refresh.
         * Do not restore that cache manually: comparing its old value with
         * the restored squad is what triggers native list repopulation. */
        lease->changed|=1;
        if(!runtime_power_wheel_pointer(owner.handler.pointer+0x700,next.baseline[0].pointer,squad)) {
            lease->changed=0;runtime_power_wheel_context_release(base,engine);return;
        }
    }
    for(unsigned n=2;n<4;++n)if(next.baseline[n].pointer) {
        if(!runtime_power_wheel_pointer(owner.handler.pointer+offsets[n],next.baseline[n].pointer,0)) {
            runtime_power_wheel_context_release(base,engine);return;
        }
        lease->changed|=1u<<n;
    }
    if(!runtime_power_wheel_context_valid(base,session_context(engine))) {
        runtime_power_wheel_context_release(base,engine);return;
    }
    line("{\"runtime_power_wheel_context\":\"leased\",\"population_dispatched\":false,\"gameplay_verified\":false}\r\n");
}
#endif
