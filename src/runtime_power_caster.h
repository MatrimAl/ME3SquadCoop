#ifndef ME3_RUNTIME_POWER_CASTER_H
#define ME3_RUNTIME_POWER_CASTER_H
/* Full-wheel SelectCurrentPower obtains a caster and native index group
 * through virtual+0x508. Validate the entire owned group before returning
 * Ashley, preserving native selection/targeting/queue behavior afterwards. */
typedef int (*RuntimePowerCaster)(void*,uintptr_t*,uintptr_t*);
static RuntimePowerCaster runtime_original_caster;
static uintptr_t runtime_caster_base;
static int runtime_caster_installed;
static int runtime_power_caster_group(uintptr_t at,GameArray *array) {
    /* A player-only wheel normally has empty henchman groups. Empty is valid
     * here, unlike identity_array's nonempty UObject inventory contract. */
    return read_at(at,array,sizeof *array)&&array->count>=0&&array->count<=RUNTIME_POWER_LIMIT&&
        array->max>=array->count&&array->max<=RUNTIME_POWER_LIMIT*3&&(!array->count||array->data);
}
static int runtime_power_caster_power_owned(uintptr_t power,SessionContext c) {
    uintptr_t pawn=0,pc=0,ai=1;
    return read_at(power+0x98,&pawn,8)&&pawn==c.pawn&&read_at(power+0xa0,&pc,8)&&pc==c.pc&&
        read_at(power+0xa8,&ai,8)&&!ai;
}
static int runtime_power_caster_empty(uintptr_t base,uintptr_t icon) {
    uint32_t name[2]={0};unsigned char states[2]={0};
    /* Native SetPower(None) clears pawn/name and sets current/desired state5.
     * Such capacity slots cannot enter the state3 cancellation pass. */
    return runtime_power_icon_values(icon,(SessionHandle){0},0)&&
        runtime_power_icon_none(base,icon,name)&&
        runtime_power_property(base,icon,"SFXGUIValue_PowerIcon","eState","ByteProperty",0x1bc,1)&&
        runtime_power_property(base,icon,"SFXGUIValue_PowerIcon","eDesiredState","ByteProperty",0x1bd,1)&&
        read_at(icon+0x1bc,states,2)&&states[0]==5&&states[1]==5;
}
static int runtime_power_caster(void *self,uintptr_t *pawn_out,uintptr_t *group_out) {
    uintptr_t caller=(uintptr_t)__builtin_return_address(0),base=runtime_caster_base,handler=(uintptr_t)self;
    RuntimePowerWheelContext *lease=&runtime_power_wheel_context;
    if(GetCurrentThreadId()!=session_game_thread||caller!=base+0xc5295e||
       !runtime_power_icons_enabled()||!runtime_control_enabled||!runtime_scope_ready||
       !InterlockedCompareExchange(&g_enable_requested,0,0)||g_runtime_config.role!=ME3_ROLE_GUEST||
       !lease->active||handler!=lease->owner.handler.pointer)
        return runtime_original_caster(self,pawn_out,group_out);
    /* Output locations belong to this exact native caller's stack. Refuse
     * unreadable/unwritable outputs rather than dereference foreign memory. */
    if(!pawn_out||!group_out||pawn_out==group_out||
       !background_focus_writable((uintptr_t)pawn_out,8)||!background_focus_writable((uintptr_t)group_out,8))return 0;
    *pawn_out=0;*group_out=0;
    SessionContext c=session_context(session_engine);RuntimePowerCatalog catalog;
    if(lease->retiring||!runtime_power_wheel_context_valid(base,c)||c.pc!=lease->owner.pc.pointer||c.pawn!=lease->owner.pawn.pointer||
       !session_handle_matches(lease->owner.pawn)||lease->owner.token!=guest_player_context_registration_token()||
       !runtime_power_icon_owner(base,&lease->owner,c)||
       (session_engine_blocked(c)&&!runtime_power_menu_context_held(base,c))||
       runtime_power_wheel_drag_state(base,handler)!=0||
       !runtime_power_read_membership(base,c,&catalog)||!runtime_power_catalog_same(&catalog,&lease->catalog))return 0;
    if(!runtime_power_property(base,handler,"SFXSFHandler_PowerWheel","m_nCurrentPowerIconIndex","IntProperty",0x780,4)||
       !runtime_power_property(base,handler,"SFXSFHandler_PowerWheel","m_aPowerIcons","ArrayProperty",0x45c,16)||
       !runtime_power_property(base,handler,"SFXSFHandler_PowerWheel","m_oPowerIndices","StructProperty",0x3c4,0x30)||
       !runtime_power_property(base,handler,"SFXSFHandler_PowerWheel","m_bProcessSquadHenchmen","BoolProperty",0x7f8,4))return 0;
    int32_t selected=-1;uint32_t flags=0;GameArray icons={0},groups[3]={{0}},after={0};
    if(!read_at(handler+0x780,&selected,4)||selected<0||
       !identity_array(handler+0x45c,&icons,RUNTIME_POWER_LIMIT*3)||selected>=icons.count||
       !read_at(handler+0x7f8,&flags,4))return 0;
    int chosen=-1;int32_t indices[3][RUNTIME_POWER_LIMIT]={{0}};
    for(unsigned n=0;n<3;++n) {
        if(!runtime_power_caster_group(handler+0x3c4+n*16,&groups[n]))return 0;
        for(int j=0;j<groups[n].count;++j) {
            if(!read_at(groups[n].data+(size_t)j*4,&indices[n][j],4)||
               indices[n][j]<0||indices[n][j]>=icons.count)return 0;
            if(indices[n][j]==selected) {
                if(chosen>=0)return 0;
                chosen=(int)n;
            }
        }
    }
    if(chosen<0||(chosen&&!(flags&0x10)))return 0;
    GameArray group=groups[chosen];SessionHandle icon_handles[RUNTIME_POWER_LIMIT]={{0}};
    int membership[RUNTIME_POWER_LIMIT]={0};
    for(int j=0;j<group.count;++j) {
        membership[j]=-1;
        int index=indices[chosen][j];uintptr_t icon=0,power=0;uint32_t name[2]={0};
        for(int k=0;k<j;++k)if(indices[chosen][k]==index)return 0;
        if(!read_at(icons.data+(size_t)index*8,&icon,8)||
           !narrative_probe_object(base,icon,"SFXGUIValue_PowerIcon")||read_ptr(icon)!=base+0x13b20f8||
           read_ptr(icon+0x40)!=handler||
           !runtime_power_property(base,icon,"SFXGUIValue_PowerIcon","pPower","ObjectProperty",0x194,8)||
           !runtime_power_property(base,icon,"SFXGUIValue_PowerIcon","pPawn","ObjectProperty",0x19c,8)||
           !runtime_power_property(base,icon,"SFXGUIValue_PowerIcon","nmPowerName","NameProperty",0x1a4,8)||
           !read_at(icon+0x194,&power,8)||!read_at(icon+0x1a4,name,8))return 0;
        icon_handles[j]=session_capture_handle(icon);
        if(!session_handle_matches(icon_handles[j]))return 0;
        if(!power) {
            if(index==selected||!runtime_power_caster_empty(base,icon))return 0;
            continue;
        }
        int entry=runtime_power_name_index(&catalog,name);
        if(entry<0||catalog.powers[entry].pointer!=power||
           !runtime_power_native_lookup_matches(base,&catalog,(unsigned)entry)||
           !runtime_power_icon_matches(icon,catalog.powers[entry],c.pawn,name)||
           !runtime_power_property(base,power,"BioCustomAction","m_oPawn","ObjectProperty",0x98,8)||
           !runtime_power_property(base,power,"BioCustomAction","m_oPC","ObjectProperty",0xa0,8)||
           !runtime_power_property(base,power,"BioCustomAction","m_oAI","ObjectProperty",0xa8,8)||
           !runtime_power_caster_power_owned(power,c))return 0;
        for(int k=0;k<j;++k)if(membership[k]==entry)return 0;
        membership[j]=entry;
    }
    for(unsigned n=0;n<3;++n) {
        if(!read_at(handler+0x3c4+n*16,&after,sizeof after)||memcmp(&after,&groups[n],sizeof after))return 0;
        for(int j=0;j<groups[n].count;++j) {
            int32_t index=-1;
            if(!read_at(groups[n].data+(size_t)j*4,&index,4)||index!=indices[n][j])return 0;
        }
    }
    for(int j=0;j<group.count;++j) {
        int entry=membership[j];
        if(!session_handle_matches(icon_handles[j])||
           read_ptr(icons.data+(size_t)indices[chosen][j]*8)!=icon_handles[j].pointer)return 0;
        if(entry<0) {
            if(!runtime_power_caster_empty(base,icon_handles[j].pointer))return 0;
            continue;
        }
        if(!runtime_power_icon_matches(icon_handles[j].pointer,catalog.powers[entry],c.pawn,catalog.names[entry])||
           !runtime_power_caster_power_owned(catalog.powers[entry].pointer,c))return 0;
    }
    int32_t last=-1;uint32_t last_flags=0;SessionContext current=session_context(session_engine);
    if(current.pc!=c.pc||current.pawn!=c.pawn||current.world!=c.world||
       (session_engine_blocked(current)&&!runtime_power_menu_context_held(base,current))||
       !runtime_power_wheel_context_valid(base,current)||!runtime_power_icon_owner(base,&lease->owner,current)||
       !read_at(handler+0x780,&last,4)||last!=selected||!read_at(handler+0x7f8,&last_flags,4)||last_flags!=flags||
       !read_at(handler+0x45c,&after,sizeof after)||memcmp(&after,&icons,sizeof after))return 0;
    *pawn_out=c.pawn;*group_out=handler+0x3c4+(unsigned)chosen*16;
    return 1;
}
static int runtime_power_caster_active(uintptr_t base) {
    return runtime_caster_installed&&read_ptr(base+0x14f6700+0x508)==(uintptr_t)runtime_power_caster;
}
static int runtime_power_caster_install(uintptr_t base) {
    uintptr_t slot=base+0x14f6700+0x508,expected=base+0xc4d740;
    if(runtime_caster_installed)return runtime_power_caster_active(base);
    static const unsigned char want[]={0x48,0x89,0x5c,0x24,0x08,0x48,0xc7,0x02,0,0,0,0};
    unsigned char got[sizeof want];DWORD old=0;
    if(read_ptr(slot)!=expected||!read_at(expected,got,sizeof got)||memcmp(got,want,sizeof want)||
       !VirtualProtect((void*)slot,8,PAGE_READWRITE,&old))return 0;
    runtime_caster_base=base;runtime_original_caster=(RuntimePowerCaster)expected;
    PVOID previous=InterlockedCompareExchangePointer((PVOID volatile*)slot,(PVOID)runtime_power_caster,(PVOID)expected);
    VirtualProtect((void*)slot,8,old,&old);
    runtime_caster_installed=(uintptr_t)previous==expected;return runtime_caster_installed;
}
#endif
