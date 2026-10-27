#ifndef ME3_RUNTIME_POWER_MENU_H
#define ME3_RUNTIME_POWER_MENU_H
#include "runtime_command_menu_mode.h"
/* Retain only a pre-existing registered guest context in an unpaused native
 * power-wheel/Shift command mode. This never acquires context, dispatches powers or clears
 * pause/scene flags. Other modes continue through ordinary scene cleanup. */
static int runtime_power_menu_context_held(uintptr_t base,SessionContext c) {
    const char *mode_class=runtime_command_menu_class(c.mode);
    if(!mode_class||!runtime_control_enabled||!runtime_scope_ready||
       g_runtime_config.role!=ME3_ROLE_GUEST||!InterlockedCompareExchange(&g_enable_requested,0,0)||
       session_environment_block_reason(c)||!guest_player_context_registered_for(c.pc,c.pawn)||
       !guest_player_context_basic_owned(base,c.pc,c.pawn,c.world))return 0;
    unsigned long token=guest_player_context_registration_token();
    uint32_t life=0,combat=0;
    if(!token||!read_at(c.pawn+0xe20,&life,4)||(life&(0x300|0x40000))||
       !read_at(c.pawn+0xe18,&combat,4)||!(combat&8))return 0;
    uintptr_t manager=read_ptr(c.pc+0xc38),mode=0;GameArray modes={0},after={0};unsigned char active=255;
    if(manager!=c.manager||!narrative_probe_object(base,manager,"SFXGameModeManager")||
       !runtime_power_property(base,c.pc,"BioPlayerController","GameModeManager2","ObjectProperty",0xc38,8)||
       !runtime_power_property(base,manager,"SFXGameModeManager","GameModes","ArrayProperty",0x70,16)||
       !runtime_power_property(base,manager,"SFXGameModeManager","CurrentMode","ByteProperty",0xa8,1)||
       !identity_array(manager+0x70,&modes,64)||modes.count<=c.mode||
       !read_at(modes.data+(size_t)c.mode*8,&mode,8)||!narrative_probe_object(base,mode,mode_class)||
       read_ptr(mode+0x40)!=c.pc||!read_at(manager+0xa8,&active,1)||active!=c.mode)return 0;
    NetworkActorIdentityEntry channel={0};CombatContextChain chain;
    if(!guest_player_context_channel(base,c.pawn,&channel))return 0;
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
        read_ptr(modes.data+(size_t)c.mode*8)==mode&&read_ptr(c.pc+0xc38)==manager&&
        read_at(manager+0xa8,&active,1)&&active==c.mode&&!session_environment_block_reason(c)&&
        token==guest_player_context_registration_token()&&guest_player_context_registered_for(c.pc,c.pawn)&&
        guest_player_context_basic_owned(base,c.pc,c.pawn,c.world)&&
        read_at(c.pawn+0xe20,&life,4)&&!(life&(0x300|0x40000));
}
static int runtime_power_menu_held(uintptr_t base,SessionContext c) {
    RuntimePowerIconLease *lease=&runtime_power_icons;
    RuntimePowerCatalog current;
    /* UI mutation still requires the fully captured optional icon lease.
     * Mere context retention never grants icon acquisition or casting. */
    return runtime_power_icons_enabled()&&lease->active&&!lease->failed&&!lease->retiring&&
        runtime_power_assignment_active(base)&&runtime_native_power_input_active()&&
        runtime_power_caster_active(base)&&runtime_power_menu_context_held(base,c)&&
        c.pc==lease->owner.pc.pointer&&c.pawn==lease->owner.pawn.pointer&&
        session_handle_matches(lease->owner.pawn)&&lease->owner.token==guest_player_context_registration_token()&&
        runtime_power_icon_owner(base,&lease->owner,c)&&
        runtime_power_read_membership(base,c,&current)&&runtime_power_catalog_same(&current,&lease->catalog);
}
static int runtime_power_ui_allowed(uintptr_t base,SessionContext c) {
    return !session_engine_blocked(c)||runtime_power_menu_held(base,c);
}
#endif
