#ifndef ME3_RUNTIME_POWER_MENU_H
#define ME3_RUNTIME_POWER_MENU_H
/* Retain only a pre-existing registered guest UI lease in an unpaused native
 * power-wheel mode. This never acquires context, dispatches powers or clears
 * pause/scene flags. Other modes continue through ordinary scene cleanup. */
static int runtime_power_menu_held(uintptr_t base,SessionContext c) {
    RuntimePowerIconLease *lease=&runtime_power_icons;
    if(c.mode!=3||!runtime_power_icons_enabled()||!runtime_control_enabled||!runtime_scope_ready||
       g_runtime_config.role!=ME3_ROLE_GUEST||!InterlockedCompareExchange(&g_enable_requested,0,0)||
       !lease->active||lease->failed||lease->retiring||!runtime_power_assignment_active(base)||
       !runtime_native_power_input_active()||!runtime_power_caster_active(base)||session_environment_block_reason(c)||
       c.pc!=lease->owner.pc.pointer||c.pawn!=lease->owner.pawn.pointer||
       !session_handle_matches(lease->owner.pawn)||lease->owner.token!=guest_player_context_registration_token()||
       !runtime_power_icon_owner(base,&lease->owner,c))return 0;
    uintptr_t manager=read_ptr(c.pc+0xc38),mode=0;GameArray modes={0},after={0};unsigned char active=255;
    if(manager!=c.manager||!narrative_probe_object(base,manager,"SFXGameModeManager")||
       !runtime_power_property(base,c.pc,"BioPlayerController","GameModeManager2","ObjectProperty",0xc38,8)||
       !runtime_power_property(base,manager,"SFXGameModeManager","GameModes","ArrayProperty",0x70,16)||
       !runtime_power_property(base,manager,"SFXGameModeManager","CurrentMode","ByteProperty",0xa8,1)||
       !identity_array(manager+0x70,&modes,64)||modes.count<=3||
       !read_at(modes.data+3*8,&mode,8)||!narrative_probe_object(base,mode,"SFXGameModePowerWheel")||
       read_ptr(mode+0x40)!=c.pc||!read_at(manager+0xa8,&active,1)||active!=3)return 0;
    RuntimePowerCatalog current;NetworkActorIdentityEntry channel={0};CombatContextChain chain;
    if(!runtime_power_read_membership(base,c,&current)||!runtime_power_catalog_same(&current,&lease->catalog)||
       !guest_player_context_channel(base,c.pawn,&channel))return 0;
    combat_context_read_chain(base,&chain);
    if(!chain.info_valid||chain.info!=c.world)return 0;
    if(guest_existing_context.registered) {
        if(!guest_existing_context_same(base,c.pc,c.pawn,c.world,&channel,&chain)||
           read_ptr(chain.squad+0x2a8)!=c.pawn||read_ptr(chain.squad+0x2cc)!=c.pawn)return 0;
    } else {
        GuestPlayerContextLease *context=&guest_player_context_lease;
        if(!context->registered||context->pending_cleanup||
           !guest_player_context_same(base,c.pc,c.pawn,c.world,&channel)||
           !guest_player_context_squad_shape(base,c.pawn)||read_ptr(c.world+0x8e8)!=context->squad.pointer)return 0;
    }
    return read_at(manager+0x70,&after,sizeof after)&&!memcmp(&modes,&after,sizeof modes)&&
        read_ptr(modes.data+3*8)==mode&&read_ptr(c.pc+0xc38)==manager&&
        read_at(manager+0xa8,&active,1)&&active==3&&!session_environment_block_reason(c);
}
static int runtime_power_ui_allowed(uintptr_t base,SessionContext c) {
    return !session_engine_blocked(c)||runtime_power_menu_held(base,c);
}
#endif
