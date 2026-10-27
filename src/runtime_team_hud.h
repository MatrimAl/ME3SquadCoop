#ifndef ME3_RUNTIME_TEAM_HUD_H
#define ME3_RUNTIME_TEAM_HUD_H
/* Read-only teammate awareness on the existing Canvas HUD hook. No new
 * network messages or gameplay writes: replica values may remain unknown. */
typedef struct RuntimeTeamHudScope {
    GuestCameraStamp pc,local,pawn,world,connection,driver,channel;
    unsigned long token;
    int index;
} RuntimeTeamHudScope;
typedef struct RuntimeTeamHud {
    RuntimeTeamHudScope scope;
    GuestVitalsValues values;
    DWORD sampled_at;
    int valid,life_valid;
    uint32_t life;
} RuntimeTeamHud;
static RuntimeTeamHud runtime_team_hud;
static int runtime_team_hud_enabled(void) {
    static int enabled=-1;
    if(enabled<0){wchar_t value[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_TEAM_HUD",value,8);
        enabled=!n||(n==1&&value[0]==L'1');}
    return enabled;
}
static int runtime_team_hud_owned(uintptr_t base,SessionContext c) {
    if(!runtime_control_enabled||!runtime_scope_ready||session_engine_blocked(c)||
       !InterlockedCompareExchange(&g_enable_requested,0,0))return 0;
    if(g_runtime_config.role==ME3_ROLE_GUEST)return guest_vitals_owned(base,c)&&presentation_guest_valid(base);
    return g_runtime_config.role==ME3_ROLE_HOST&&session_active&&!session_releasing&&!session_fault&&
        !session_cleanup_pending&&!session_pending_reason&&c.pc==session_host&&c.pawn==session_shepard&&
        c.world==session_world&&session_story_valid(base)&&session_handle_matches(session_pawn_handle)&&
        session_handle_matches(session_connection_handle)&&session_peer_connected(base,session_remote,c.world)&&
        read_ptr(session_remote+0x3bc)==session_pawn&&read_ptr(session_pawn+0x374)==session_remote;
}
static int runtime_team_hud_scope_same(const RuntimeTeamHudScope *a,const RuntimeTeamHudScope *b) {
    if(!a->token||a->token!=b->token||a->index!=b->index)return 0;
    const GuestCameraStamp *left[]={&a->pc,&a->local,&a->pawn,&a->world,&a->connection,&a->driver,&a->channel};
    const GuestCameraStamp *right[]={&b->pc,&b->local,&b->pawn,&b->world,&b->connection,&b->driver,&b->channel};
    for(unsigned i=0;i<7;++i)if(!guest_camera_stamp_core_equal(left[i],right[i])||
        strcmp(left[i]->full_name,right[i]->full_name))return 0;
    return 1;
}
static int runtime_team_hud_scope_read(uintptr_t base,SessionContext c,RuntimeTeamHudScope *out,uintptr_t host_channel) {
    memset(out,0,sizeof *out);
    if(!runtime_team_hud_enabled()||!runtime_team_hud_owned(base,c))return 0;
    int host=g_runtime_config.role==ME3_ROLE_HOST;NetworkActorIdentityEntry network={0};
    uintptr_t pawn=host?session_pawn:presentation_guest.objects[2].pointer;
    uintptr_t connection=host?session_connection:presentation_guest.objects[4].pointer;
    uintptr_t driver=host?identity_host_driver:presentation_guest.objects[5].pointer;
    uintptr_t channel=host?host_channel:presentation_guest.objects[6].pointer;
    if(host&&!channel) {
        if(network_actor_scan_driver(base,pawn,driver,&network)<0)return 0;
        channel=network.channel;
    }
    unsigned char role=255;
    if(pawn==c.pawn||!session_actor_live(base,pawn)||read_ptr(pawn+0x198)!=c.world||
       !is_kind_of(base,pawn,host?"SFXPawn_Ashley":"SFXPawn_Player")||
       !read_at(pawn+0x26a,&role,1)||role!=(host?3:1)||
       !network_actor_channel_valid(base,pawn,channel,driver,&network)||network.index<=0||
       network.connection!=connection||(!host&&network.index!=presentation_guest.index))return 0;
    uintptr_t values[]={c.pc,c.pawn,pawn,c.world,connection,driver,channel};
    GuestCameraStamp *stamps[]={&out->pc,&out->local,&out->pawn,&out->world,&out->connection,&out->driver,&out->channel};
    for(unsigned i=0;i<7;++i)if(!guest_camera_stamp_read(base,values[i],stamps[i],0))return 0;
    out->token=guest_vitals_scope_token();out->index=network.index;return out->token!=0;
}
static int runtime_team_hud_sample_fresh(const RuntimeTeamHud *sample,const RuntimeTeamHudScope *scope,DWORD now) {
    return sample->valid&&now-sample->sampled_at<=1000&&runtime_team_hud_scope_same(&sample->scope,scope);
}
static const wchar_t *runtime_team_hud_status(int valid,uint32_t life) {
    if(!valid)return L"";
    if(life&0x200)return L"  DEAD";
    return life&0x100?L"  DOWNED - REVIVE":L"";
}
static void runtime_team_hud_sample(uintptr_t base,SessionContext c,DWORD now) {
    RuntimeTeamHud next={0};RuntimeTeamHudScope checked;
    runtime_team_hud.valid=0;
    if(!runtime_team_hud_scope_read(base,c,&next.scope,0))return;
    guest_vitals_sample_actor(base,next.scope.pawn.pointer,&next.values);
    next.life_valid=read_at(next.scope.pawn.pointer+0xe20,&next.life,4);
    if(!runtime_team_hud_scope_read(base,c,&checked,next.scope.channel.pointer)||!runtime_team_hud_scope_same(&next.scope,&checked))return;
    next.sampled_at=now;next.valid=1;runtime_team_hud=next;
}
static void runtime_team_hud_draw(uintptr_t canvas,int32_t *size,float scale) {
    RuntimeTeamHud *q=&runtime_team_hud;RuntimeTeamHudScope current;uintptr_t base=guest_vitals.base;
    if(g_runtime_config.role==ME3_ROLE_GUEST&&runtime_native_hud_teammate_showing(base,session_context(session_engine)))return;
    if(!q->valid||!runtime_team_hud_scope_read(base,session_context(session_engine),&current,q->scope.channel.pointer)||
       !runtime_team_hud_sample_fresh(q,&current,GetTickCount()))return;
    float h=0,s=0,x=24*scale,y=118*scale,width=fminf(size[0]*0.24f,240*scale);
    int hv=q->values.hp_valid&&guest_vitals_fraction(q->values.hp,q->values.hp_max,&h);
    int sv=q->values.shield_valid&&guest_vitals_fraction(q->values.shield,q->values.shield_max,&s);
    guest_vitals_bar(canvas,x,y,width,h,hv,0,scale);guest_vitals_bar(canvas,x,y-9*scale,width,s,sv,1,scale);
    wchar_t label[192],hp[32],shield[32];
    if(hv)_snwprintf(hp,32,L"%.0f/%.0f",q->values.hp,q->values.hp_max);else wcscpy(hp,L"--");
    if(sv)_snwprintf(shield,32,L"%.0f/%.0f",q->values.shield,q->values.shield_max);else wcscpy(shield,L"--");
    int n=_snwprintf(label,192,L"%ls%ls\nHP %ls   SHIELD %ls",
        g_runtime_config.role==ME3_ROLE_HOST?L"ASHLEY":L"SHEPARD",runtime_team_hud_status(q->life_valid,q->life),hp,shield);
    if(n<=0||n>=191)return;
    float position[2]={x,y-43*scale};unsigned char color[4]={235,240,255,255};
    if(q->life_valid&&(q->life&0x300)){color[0]=75;color[1]=155;color[2]=255;}
    GuestVitalsText text={{(uintptr_t)label,n+1,n+1},0.7f*scale,0.7f*scale};
    guest_vitals_call(canvas,&guest_vitals.pos,position);guest_vitals_call(canvas,&guest_vitals.color,color);
    guest_vitals_call(canvas,&guest_vitals.text,&text);
}
static int runtime_team_or_story_enabled(void) {return runtime_team_hud_enabled()||runtime_story_warning_enabled();}
static int runtime_team_or_story_owned(uintptr_t base,SessionContext c) {
    return (runtime_team_hud_enabled()&&runtime_team_hud_owned(base,c))||runtime_story_warning_owned(base,c);
}
static void runtime_team_or_story_draw(uintptr_t canvas,int32_t *size,float scale) {
    if(runtime_team_hud_enabled())runtime_team_hud_draw(canvas,size,scale);
    runtime_story_warning_draw(canvas,size,scale);
}
static void runtime_team_hud_configure(void) {
    presentation_scope_enabled=runtime_team_hud_enabled;
    guest_vitals_extra_enabled=runtime_team_or_story_enabled;guest_vitals_extra_owned=runtime_team_or_story_owned;
    guest_vitals_extra_sample=runtime_team_hud_sample;guest_vitals_extra_draw=runtime_team_or_story_draw;
}
static int runtime_team_hud_selftest(void) {
    RuntimeTeamHud sample={0};RuntimeTeamHudScope scope={0};
    GuestVitalsValues unknown={1,1,100,100,100,100};
    guest_vitals_sample_actor(0,0,&unknown);
    if(unknown.hp_valid||unknown.shield_valid||unknown.hp||unknown.shield)return 0;
    scope.token=4;scope.index=2;
    GuestCameraStamp *stamps[]={&scope.pc,&scope.local,&scope.pawn,&scope.world,&scope.connection,&scope.driver,&scope.channel};
    for(unsigned i=0;i<7;++i){stamps[i]->pointer=100+i;stamps[i]->index=(int)i;}
    sample.scope=scope;sample.valid=1;sample.sampled_at=100;
    if(!runtime_team_hud_sample_fresh(&sample,&scope,1100)||runtime_team_hud_sample_fresh(&sample,&scope,1101))return 0;
    scope.token++;if(runtime_team_hud_sample_fresh(&sample,&scope,200))return 0;scope=sample.scope;
    for(unsigned i=0;i<7;++i){stamps[i]->index++;if(runtime_team_hud_sample_fresh(&sample,&scope,200))return 0;stamps[i]->index--;}
    scope.index++;if(runtime_team_hud_sample_fresh(&sample,&scope,200))return 0;
    return !*runtime_team_hud_status(1,0)&&!wcscmp(runtime_team_hud_status(1,0x100),L"  DOWNED - REVIVE")&&
        !wcscmp(runtime_team_hud_status(1,0x300),L"  DEAD")&&!*runtime_team_hud_status(0,0x300);
}
#endif
