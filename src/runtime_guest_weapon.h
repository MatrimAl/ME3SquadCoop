#ifndef ME3_RUNTIME_GUEST_WEAPON_H
#define ME3_RUNTIME_GUEST_WEAPON_H
/* An owned campaign companion still takes SFXWeapon.GetFireModeBaseDamage's
 * unconditional Henchman multiplier. InitializeWeapon also selects FullAuto2
 * for non-human instigators, but humans may legitimately use FullAuto too.
 * Borrow only these two verified fields. Never replay initialization or
 * change live fire/ammo. */
typedef struct RuntimeGuestWeapon {
    SessionHandle pc,pawn,weapon,inventory,world,root,klass,cdo,damage_field,mode_field;
    SessionHandle pawn_channel,weapon_channel,connection,driver;
    uint32_t damage_bits;unsigned char old_mode,human_mode;
    int valid,damage_attempted,damage_changed,mode_attempted,mode_changed,wait_reported;
    DWORD scanned;
} RuntimeGuestWeapon;
static RuntimeGuestWeapon runtime_guest_weapon;
static unsigned runtime_guest_weapon_events;
static int runtime_guest_weapon_enabled(void) {
    static int enabled=-1;
    if(enabled<0){wchar_t value[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_PLAYER_WEAPON",value,8);
        enabled=!n||(n==1&&value[0]==L'1');}
    return enabled;
}
static void runtime_guest_weapon_log(const char *event,const RuntimeGuestWeapon *q,DWORD now) {
    if(runtime_guest_weapon_events++>=96)return;char row[360];
    float before=0;memcpy(&before,&q->damage_bits,4);
    snprintf(row,sizeof row,"{\"runtime_guest_weapon\":\"%s\",\"role\":%d,\"baseline_hench_scale\":%.9g,\"authored_fire_mode\":%u,\"damage_scale_owned\":%s,\"default_mode_owned\":%s,\"mechanics_parity_verified\":false,\"monotonic_ms\":%lu}\r\n",
        event,(int)g_runtime_config.role,before,(unsigned)q->human_mode,q->damage_changed?"true":"false",q->mode_changed?"true":"false",now);line(row);
}
static int runtime_guest_weapon_cas(uintptr_t at,uint32_t expected,uint32_t desired) {
    if((at&3)||!background_focus_writable(at,4))return 0;
    return (uint32_t)InterlockedCompareExchange((LONG volatile*)at,(LONG)desired,(LONG)expected)==expected;
}
static int runtime_guest_weapon_damage_valid(uint32_t bits) {
    float value=0;memcpy(&value,&bits,4);
    return isfinite(value)&&value>=0.0f&&value<=16.0f;
}
static uint32_t runtime_guest_weapon_mode_word(uint32_t word,unsigned char mode) {
    return (word&UINT32_C(0xff00ffff))|((uint32_t)mode<<16);
}
static int runtime_guest_weapon_mode_policy(unsigned current,unsigned authored) {
    /* SFXWeapon.FireModes: None0, SemiAuto1, FullAuto2, Burst3, Reload4.
     * FullAuto is a valid human default, not a marker of AI ownership. */
    return current==2&&(authored==1||authored==3);
}
static int runtime_guest_weapon_firing_entry(unsigned mode,int state_count,int type_count,
        uint32_t name_index,uint32_t name_number,unsigned type) {
    return runtime_guest_weapon_mode_policy(2,mode)&&state_count>0&&state_count<=32&&
        type_count>0&&type_count<=32&&mode<(unsigned)state_count&&mode<(unsigned)type_count&&
        name_index&&name_number<=4096&&type<3; /* Stock SendToFiringState rejects EWFT_None3. */
}
static int runtime_guest_weapon_handles(const RuntimeGuestWeapon *q) {
    return session_handle_matches(q->pc)&&session_handle_matches(q->pawn)&&session_handle_matches(q->weapon)&&
        session_handle_matches(q->inventory)&&session_handle_matches(q->world)&&session_handle_matches(q->root)&&
        session_handle_matches(q->klass)&&session_handle_matches(q->cdo)&&
        session_handle_matches(q->damage_field)&&session_handle_matches(q->mode_field)&&
        session_handle_matches(q->pawn_channel)&&session_handle_matches(q->weapon_channel)&&
        session_handle_matches(q->connection)&&session_handle_matches(q->driver);
}
static int runtime_guest_weapon_layout(uintptr_t base,uintptr_t weapon,uintptr_t *damage,uintptr_t *mode) {
    *damage=runtime_guest_team_field(base,weapon,"SFXWeapon","DamageHench","FloatProperty",0xf80,4,0x4001);
    *mode=runtime_guest_team_field(base,weapon,"SFXWeapon","DefaultFireMode","ByteProperty",0x10be,1,0);
    return *damage&&*mode&&
        runtime_guest_team_field(base,weapon,"SFXWeapon","PendingFireMode","ByteProperty",0x10c2,1,0x2000)&&
        runtime_guest_team_field(base,weapon,"Weapon","CurrentFireMode","ByteProperty",0x3b0,1,0)&&
        runtime_guest_team_field(base,weapon,"Weapon","FiringStatesArray","ArrayProperty",0x2e4,16,0x400000)&&
        runtime_guest_team_field(base,weapon,"Weapon","WeaponFireTypes","ArrayProperty",0x2f4,16,0x400000);
}
static int runtime_guest_weapon_chain(uintptr_t base,uintptr_t pc,uintptr_t pawn,uintptr_t weapon,uintptr_t inventory,uintptr_t world,int equipped) {
    return guest_camera_actor_live(base,pc)&&is_kind_of(base,pc,"SFXPlayerController")&&
        guest_camera_actor_live(base,pawn)&&is_kind_of(base,pawn,"SFXPawn_Ashley")&&
        guest_camera_actor_live(base,weapon)&&is_kind_of(base,weapon,"SFXWeapon")&&
        guest_camera_actor_live(base,inventory)&&is_kind_of(base,inventory,"SFXInventoryManager")&&
        read_ptr(pc+0x198)==world&&read_ptr(pawn+0x198)==world&&read_ptr(weapon+0x198)==world&&read_ptr(inventory+0x198)==world&&
        read_ptr(weapon+0x180)==pawn&&read_ptr(weapon+0x190)==pawn&&read_ptr(weapon+0x2b0)==inventory&&
        read_ptr(inventory+0x180)==pawn&&read_ptr(inventory+0x190)==pawn&&read_ptr(pawn+0x414)==inventory&&
        (!equipped||(read_ptr(pawn+0x41c)==weapon&&read_ptr(pc+0x3bc)==pawn&&read_ptr(pawn+0x374)==pc));
}
/* Zero pending mode is proven by Active.CheckBeginFire's native script. An
 * outstanding FullAuto2 request completes/cancels through stock code. */
static int runtime_guest_weapon_idle(uintptr_t base,const RuntimeGuestWeapon *q) {
    unsigned char fire=255,pending=255,current=255;int32_t action=-1,power=-1;uint32_t busy=0;GameArray requests={0};char state[96];
    if(!read_at(q->pc.pointer+0x480,&fire,1)||fire||
       !read_at(q->pawn.pointer+0xc20,&action,4)||action||!read_at(q->pawn.pointer+0xc28,&power,4)||power||
       !read_at(q->weapon.pointer+0x10c2,&pending,1)||pending||!read_at(q->weapon.pointer+0x3b0,&current,1)||current||
       !read_at(q->weapon.pointer+0x10b4,&busy,4)||(busy&0x1c000)||read_ptr(q->inventory.pointer+0x288)||
       !runtime_guest_team_field(base,q->inventory.pointer,"InventoryManager","PendingFire","ArrayProperty",0x270,16,0x400000)||
       !scene_readiness_array(q->inventory.pointer+0x270,&requests,32)||
       !native_state_name(base,q->weapon.pointer,state,sizeof state)||strcmp(state,"Active"))return 0;
    for(int i=0;i<requests.count;++i){int32_t value=-1;if(!read_at(requests.data+(SIZE_T)i*4,&value,4)||value)return 0;}
    return 1;
}
static int runtime_guest_weapon_modes(uintptr_t weapon,unsigned char mode) {
    /* SendToFiringState indexes both arrays with this same SFX selector;
     * CurrentFireMode0 is None/idle, not a distinct primary input channel.
     * SFXWeapon.GetFireInterval uses 60/GetRateOfFire(), not Engine.FireInterval. */
    GameArray states={0},types={0};uint32_t name[2];unsigned char type=255;
    return runtime_guest_weapon_mode_policy(2,mode)&&
        identity_array(weapon+0x2e4,&states,32)&&identity_array(weapon+0x2f4,&types,32)&&
        mode<states.count&&mode<types.count&&
        read_at(states.data+(SIZE_T)mode*8,name,8)&&read_at(types.data+mode,&type,1)&&
        runtime_guest_weapon_firing_entry(mode,states.count,types.count,name[0],name[1],type);
}
/* Cleanup is independent of enable/scene admission, so the core calls this
 * BEFORE native AI handoff. Ownership or class replacement always wins. */
static void runtime_guest_weapon_retire(uintptr_t base,uintptr_t engine,DWORD now) {
    RuntimeGuestWeapon *q=&runtime_guest_weapon;if(!q->valid)return;
    SessionContext c=session_context(engine);NetworkActorScope scope={0};uintptr_t damage=0,mode=0;
    int same=runtime_guest_weapon_handles(q)&&c.world==q->world.pointer&&
        network_actor_read_scope(base,q->pawn.pointer,&scope)&&scope.world==q->root.pointer&&scope.driver==q->driver.pointer&&
        runtime_guest_weapon_chain(base,q->pc.pointer,q->pawn.pointer,q->weapon.pointer,q->inventory.pointer,q->world.pointer,0)&&
        (!read_ptr(q->pawn.pointer+0x374)||read_ptr(q->pawn.pointer+0x374)==q->pc.pointer)&&
        (!read_ptr(q->pc.pointer+0x3bc)||read_ptr(q->pc.pointer+0x3bc)==q->pawn.pointer)&&
        read_ptr(q->weapon.pointer+0x50)==q->klass.pointer&&read_ptr(q->klass.pointer+0x13c)==q->cdo.pointer&&
        runtime_guest_weapon_layout(base,q->weapon.pointer,&damage,&mode)&&damage==q->damage_field.pointer&&mode==q->mode_field.pointer;
    if(same){uint32_t word=0;
        if(q->mode_changed&&read_at(q->weapon.pointer+0x10bc,&word,4)&&((word>>16)&255)==q->human_mode)
            runtime_guest_weapon_cas(q->weapon.pointer+0x10bc,word,runtime_guest_weapon_mode_word(word,q->old_mode));
        if(q->damage_changed)runtime_guest_weapon_cas(q->weapon.pointer+0xf80,guest_camera_float_bits(1.0f),q->damage_bits);
    }
    runtime_guest_weapon_log(same?"retired-conditionally":"retired-preserving-replacement",q,now);memset(q,0,sizeof *q);
}
static int runtime_guest_weapon_context(uintptr_t base,uintptr_t engine,SessionContext *out,NetworkActorScope *scope,NetworkActorIdentityEntry *pawn_channel) {
    SessionContext c=session_context(engine);uint32_t life=0;
    if(!runtime_guest_weapon_enabled()||!runtime_control_enabled||!runtime_scope_ready||
       !InterlockedCompareExchange(&g_enable_requested,0,0)||session_engine_blocked(c))return 0;
    if(g_runtime_config.role==ME3_ROLE_HOST){
        if(!session_active||session_fault||session_releasing||session_cleanup_pending||!session_story_valid(base)||
           !session_handle_matches(session_pc_handle)||!session_handle_matches(session_pawn_handle)||
           !session_peer_connected(base,session_remote,c.world))return 0;
        c.pc=session_remote;c.pawn=session_pawn;
    } else if(g_runtime_config.role!=ME3_ROLE_GUEST||!guest_player_context_basic_owned(base,c.pc,c.pawn,c.world))return 0;
    if(!read_at(c.pawn+0xe20,&life,4)||(life&0x300)||!network_actor_read_scope(base,c.pawn,scope)||scope->info!=c.world||
       !guest_player_context_channel(base,c.pawn,pawn_channel)||pawn_channel->driver!=scope->driver||pawn_channel->index<=0)return 0;
    if(g_runtime_config.role==ME3_ROLE_HOST&&pawn_channel->connection!=read_ptr(c.pc+0x67c))return 0;
    *out=c;return 1;
}
static void runtime_guest_weapon_tick(uintptr_t base,uintptr_t engine,DWORD now) {
    RuntimeGuestWeapon *q=&runtime_guest_weapon;SessionContext c;NetworkActorScope scope={0};NetworkActorIdentityEntry pawn_channel={0},weapon_channel={0};
    if(GetCurrentThreadId()!=session_game_thread)return;
    if(!runtime_guest_weapon_context(base,engine,&c,&scope,&pawn_channel)) {runtime_guest_weapon_retire(base,engine,now);return;}
    uintptr_t weapon=read_ptr(c.pawn+0x41c),inventory=read_ptr(c.pawn+0x414);
    if(q->valid&&(q->pc.pointer!=c.pc||q->pawn.pointer!=c.pawn||q->weapon.pointer!=weapon||q->inventory.pointer!=inventory||
       q->world.pointer!=c.world||q->root.pointer!=scope.world||q->connection.pointer!=pawn_channel.connection||
       q->pawn_channel.pointer!=pawn_channel.channel||!runtime_guest_weapon_handles(q)))runtime_guest_weapon_retire(base,engine,now);
    if(q->scanned&&now-q->scanned<250)return;q->scanned=now;
    if(!runtime_guest_weapon_chain(base,c.pc,c.pawn,weapon,inventory,c.world,1))return;
    if(!q->valid){uintptr_t damage=0,mode=0,klass=read_ptr(weapon+0x50),cdo=read_ptr(klass+0x13c);uint32_t bits=0;unsigned char old=255,human=255;
        if(!runtime_guest_weapon_layout(base,weapon,&damage,&mode)||!narrative_probe_object(base,klass,"Class")||
           !is_live(base,cdo)||read_ptr(cdo+0x50)!=klass||
           !read_at(weapon+0xf80,&bits,4)||!runtime_guest_weapon_damage_valid(bits)||
           !read_at(weapon+0x10be,&old,1)||!read_at(cdo+0x10be,&human,1)||
           network_actor_scan_driver(base,weapon,scope.driver,&weapon_channel)<0||weapon_channel.index<=0||weapon_channel.connection!=pawn_channel.connection)return;
        RuntimeGuestWeapon next={0};next.pc=session_capture_handle(c.pc);next.pawn=session_capture_handle(c.pawn);
        next.weapon=session_capture_handle(weapon);next.inventory=session_capture_handle(inventory);next.world=session_capture_handle(c.world);
        next.root=session_capture_handle(scope.world);next.klass=session_capture_handle(klass);next.cdo=session_capture_handle(cdo);
        next.damage_field=session_capture_handle(damage);next.mode_field=session_capture_handle(mode);
        next.pawn_channel=session_capture_handle(pawn_channel.channel);next.weapon_channel=session_capture_handle(weapon_channel.channel);
        next.connection=session_capture_handle(pawn_channel.connection);next.driver=session_capture_handle(scope.driver);
        next.damage_bits=bits;next.old_mode=old;next.human_mode=human;next.scanned=now;
        if(!runtime_guest_weapon_handles(&next))return;next.valid=1;*q=next;
    }
    if(!network_actor_channel_valid(base,weapon,q->weapon_channel.pointer,scope.driver,&weapon_channel)||
       weapon_channel.connection!=q->connection.pointer||read_ptr(weapon+0x50)!=q->klass.pointer||
       read_ptr(q->klass.pointer+0x13c)!=q->cdo.pointer)return;
    uint32_t bits=0,word=0;
    if(q->damage_changed&&(!read_at(weapon+0xf80,&bits,4)||bits!=guest_camera_float_bits(1.0f))){
        q->damage_changed=0;runtime_guest_weapon_log("preserved-native-damage-scale",q,now);}
    if(q->mode_changed&&(!read_at(weapon+0x10bc,&word,4)||((word>>16)&255)!=q->human_mode)){
        q->mode_changed=0;runtime_guest_weapon_log("preserved-native-default-mode",q,now);}
    if(!q->damage_attempted&&!runtime_guest_weapon_idle(base,q)) {
        if(!q->wait_reported){q->wait_reported=1;runtime_guest_weapon_log("waiting-stock-fire-idle",q,now);}return;
    }
    if(!q->damage_attempted){q->damage_attempted=1;
        if(q->damage_bits!=guest_camera_float_bits(1.0f))q->damage_changed=runtime_guest_weapon_cas(weapon+0xf80,q->damage_bits,guest_camera_float_bits(1.0f));
        runtime_guest_weapon_log("player-damage-scale",q,now);}
    if(q->mode_attempted)return;
    if(!read_at(weapon+0x10bc,&word,4)||!runtime_guest_weapon_mode_policy((word>>16)&255,q->human_mode)){
        q->mode_attempted=1;runtime_guest_weapon_log("preserved-authored-or-native-mode",q,now);return;}
    if(!runtime_guest_weapon_idle(base,q)){
        if(!q->wait_reported){q->wait_reported=1;runtime_guest_weapon_log("waiting-stock-fire-idle",q,now);}return;}
    q->mode_attempted=1;
    if(q->old_mode==2&&runtime_guest_weapon_modes(weapon,q->human_mode)&&
       read_ptr(q->cdo.pointer+0x50)==q->klass.pointer){unsigned char authored=255;
        if(read_at(q->cdo.pointer+0x10be,&authored,1)&&authored==q->human_mode)
            q->mode_changed=runtime_guest_weapon_cas(weapon+0x10bc,word,runtime_guest_weapon_mode_word(word,q->human_mode));}
    runtime_guest_weapon_log(q->mode_changed?"player-authored-default-mode":"authored-mode-layout-refused",q,now);
}
static int runtime_guest_weapon_selftest(void) {
    _Alignas(4) volatile LONG word=(LONG)UINT32_C(0xa502beef);uintptr_t at=(uintptr_t)&word;
    uint32_t before=(uint32_t)word,human=runtime_guest_weapon_mode_word(before,1);
    if(human!=UINT32_C(0xa501beef)||!runtime_guest_weapon_cas(at,before,human)||
       runtime_guest_weapon_cas(at,before,0)||((uint32_t)word)!=human||
       !runtime_guest_weapon_cas(at,human,before)||runtime_guest_weapon_cas(at+1,before,0))return 0;
    /* A foreign value survives both a stale apply and conditional restore. */
    word=(LONG)guest_camera_float_bits(0.7f);
    if(runtime_guest_weapon_cas(at,guest_camera_float_bits(0.3f),guest_camera_float_bits(1.0f))||
       runtime_guest_weapon_cas(at,guest_camera_float_bits(1.0f),guest_camera_float_bits(0.3f))||
       (uint32_t)word!=guest_camera_float_bits(0.7f))return 0;
    return runtime_guest_weapon_damage_valid(guest_camera_float_bits(0.3f))&&
        runtime_guest_weapon_damage_valid(guest_camera_float_bits(1.0f))&&
        !runtime_guest_weapon_damage_valid(guest_camera_float_bits(-0.3f))&&
        !runtime_guest_weapon_damage_valid(guest_camera_float_bits(NAN))&&
        runtime_guest_weapon_mode_policy(2,1)&&runtime_guest_weapon_mode_policy(2,3)&&
        !runtime_guest_weapon_mode_policy(2,0)&&!runtime_guest_weapon_mode_policy(2,2)&&
        !runtime_guest_weapon_mode_policy(2,4)&&!runtime_guest_weapon_mode_policy(2,5)&&
        !runtime_guest_weapon_mode_policy(2,255)&&!runtime_guest_weapon_mode_policy(0,1)&&
        !runtime_guest_weapon_mode_policy(1,3)&&!runtime_guest_weapon_mode_policy(3,1)&&
        runtime_guest_weapon_firing_entry(1,5,5,42,0,0)&&
        runtime_guest_weapon_firing_entry(3,5,5,42,0,1)&&
        runtime_guest_weapon_firing_entry(1,5,5,42,0,2)&&
        !runtime_guest_weapon_firing_entry(0,5,5,42,0,0)&&
        !runtime_guest_weapon_firing_entry(4,5,5,42,0,0)&&
        !runtime_guest_weapon_firing_entry(1,5,5,42,0,3)&&
        !runtime_guest_weapon_firing_entry(1,5,5,0,0,0)&&
        !runtime_guest_weapon_firing_entry(1,5,5,42,4097,0)&&
        !runtime_guest_weapon_firing_entry(3,3,5,42,0,0)&&
        !runtime_guest_weapon_firing_entry(3,5,3,42,0,0)&&
        !runtime_guest_weapon_firing_entry(1,0,5,42,0,0)&&
        !runtime_guest_weapon_firing_entry(1,5,0,42,0,0)&&
        !runtime_guest_weapon_firing_entry(1,33,5,42,0,0);
}
#endif
