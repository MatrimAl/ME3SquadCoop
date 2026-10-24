#ifndef ME3_COMBAT_FIRE_BOT_H
#define ME3_COMBAT_FIRE_BOT_H

/* Opt-in paired-test input pulse, NOT a firing-success oracle. Include after
 * guest_player_context_adapter.h; game thread only. Call pre/post ordinary
 * Tick, BEFORE guest context cleanup, also with enabled/ordinary false.
 * Capture provenance: private live-script-19192-combat-fire.json. Mode's Outer
 * is the local PC: FireWeapon checks native combat gate then sets bFire=1;
 * StopFiringWeapon only sets bFire=0. No direct input/weapon/RPC/flag writes.
 * UObject ctor120e90 pins vtable1090898; its ProcessEvent+230=f4f80.
 */
typedef struct CombatFireMethods {
    uintptr_t base,mode_class;
    DWORD discovery_at;
    int ready;
    GuestPlayerContextMethod fire,stop;
} CombatFireMethods;
typedef struct CombatFirePulse {
    unsigned long token,attempted_token;
    DWORD stable_at,gate_at,pressed_at,report_at,release_at;
    int active,positive_getter,ticks,release_attempts;
    GuestCameraStamp pc,pawn,world,mode,weapon;
    uintptr_t channel;
    int channel_index;
} CombatFirePulse;
static CombatFireMethods combat_fire_methods;
static CombatFirePulse combat_fire_pulse;
static const char *combat_fire_resolution_reason="not-resolved";

static int combat_fire_property(uintptr_t base,uintptr_t property,const char *kind,
                                const char *path,int offset,uint64_t flags) {
    GuestCameraStamp stamp;int32_t found=-1;uint64_t got=0;
    return narrative_probe_object(base,property,kind)&&guest_camera_stamp_read(base,property,&stamp,1)&&
        !strcmp(stamp.full_name,path)&&read_at(property+0x94,&found,4)&&found==offset&&
        read_at(property+0x78,&got,8)&&got==flags;
}
static uintptr_t combat_fire_operand(const unsigned char *code,size_t at) {
    uintptr_t result=0;memcpy(&result,code+at,8);return result;
}
static int combat_fire_body(uintptr_t base,uintptr_t fn,int firing) {
    /* Mask only process-local UObject operands and the resolved FName. The
     * virtual-call cache bytes remain pinned; a changed body fails closed.
     */
    static const unsigned char fire[180]={
        0x07,0x45,0x00,0xf2,0x19,0x2e,0,0,0,0,0,0,0,0,0x19,0x62,0,0,0,0,0,0,0,0,
        0x09,0x00,0,0,0,0,0,0,0,0,0x00,0x62,0,0,0,0,0,0,0,0,0x0a,0x00,0,0,0,0,0,0,0,0,
        0x00,0x1c,0,0,0,0,0,0,0,0,0x16,0x28,0x16,0x04,0x0b,0x07,0x90,0x00,0x9a,0x38,
        0x3a,0x19,0x62,0,0,0,0,0,0,0,0,0x09,0x00,0,0,0,0,0,0,0,0,0x00,0x61,0,0,0,0,0,0,0,0,
        0x38,0x3a,0x24,0x00,0x16,0x19,0x62,0,0,0,0,0,0,0,0,0x09,0x00,0,0,0,0,0,0,0,0,0x00,0x65,
        0,0,0,0,0,0,0,0,0xcf,0x02,0x28,0x16,0x0f,0x19,0x62,0,0,0,0,0,0,0,0,0x09,0x00,
        0,0,0,0,0,0,0,0,0x00,0x61,0,0,0,0,0,0,0,0,0x24,0x01,0x04,0x0b,0x53};
    static const unsigned char stop[36]={0x0f,0x19,0x62,0,0,0,0,0,0,0,0,0x09,0x00,
        0,0,0,0,0,0,0,0,0x00,0x61,0,0,0,0,0,0,0,0,0x24,0x00,0x04,0x0b,0x53};
    static const unsigned int fire_offsets[]={6,16,26,36,46,56,77,87,97,112,147,157,167};
    static const unsigned int stop_offsets[]={3,13,23};
    GameArray script={0};unsigned char code[180],masked[180];size_t length=firing?180:36;
    if(!read_at(fn+0x7c,&script,sizeof script)||script.count!=(int)length||script.max<script.count||
       script.max>4096||!script.data||!read_at(script.data,code,length))return 0;
    memcpy(masked,code,length);
    const unsigned int *offsets=firing?fire_offsets:stop_offsets;
    size_t count=firing?sizeof fire_offsets/sizeof *fire_offsets:sizeof stop_offsets/sizeof *stop_offsets;
    for(size_t i=0;i<count;++i)memset(masked+offsets[i],0,8);
    if(firing)memset(masked+132,0,8);
    if(memcmp(masked,firing?fire:stop,length))return 0;
    uintptr_t outer=combat_fire_operand(code,firing?16:3),fire_prop=combat_fire_operand(code,firing?87:13);
    if(!combat_fire_property(base,outer,"ObjectProperty","Core.Object.Outer",0x40,0x21002)||
       !combat_fire_property(base,fire_prop,"ByteProperty","Engine.Controller.bFire",0x480,4))return 0;
    if(!firing)return combat_fire_operand(code,23)==fire_prop;
    if(combat_fire_operand(code,77)!=outer||combat_fire_operand(code,112)!=outer||combat_fire_operand(code,147)!=outer||
       combat_fire_operand(code,97)!=fire_prop||combat_fire_operand(code,157)!=fire_prop||combat_fire_operand(code,167)!=fire_prop)return 0;
    uintptr_t input_prop=combat_fire_operand(code,26),gate=combat_fire_operand(code,56);
    uintptr_t result=combat_fire_operand(code,46),klass=combat_fire_operand(code,6);GuestCameraStamp type;
    uint64_t mask=0;uint32_t fname[2];char name[96];memcpy(fname,code+132,8);name_of(base,fname[0],name,sizeof name);
    return guest_camera_stamp_read(base,klass,&type,1)&&!strcmp(type.full_name,"SFXGame.BioPlayerInput")&&
        narrative_probe_object(base,klass,"Class")&&combat_fire_operand(code,36)==input_prop&&
        combat_fire_property(base,input_prop,"ObjectProperty","Engine.PlayerController.PlayerInput",0x6dc,0x4002001)&&
        guest_player_context_function_abi(base,gate,"BioPlayerInput","IsCombatEnabled",0x20401,4,0,0,0xb75250,1)&&
        combat_fire_property(base,result,"BoolProperty","SFXGame.BioPlayerInput.IsCombatEnabled.ReturnValue",0,0x580)&&
        read_ptr(result+0x40)==gate&&read_at(result+0xd0,&mask,8)&&mask==1&&fname[1]==0&&!strcmp(name,"SetAFKLastActionTime");
}
static int combat_fire_method_valid(uintptr_t base,const GuestPlayerContextMethod *method,int fire) {
    return guest_player_context_method_matches(base,method,1)&&
        guest_player_context_function_abi(base,method->function.pointer,"SFXGameModeBase",fire?"FireWeapon":"StopFiringWeapon",
            0x20202,0,65535,0,0xf52b0,0)&&combat_fire_body(base,method->function.pointer,fire);
}
static int combat_fire_resolve(uintptr_t base,uintptr_t mode,DWORD now) {
    CombatFireMethods *m=&combat_fire_methods;uintptr_t klass=read_ptr(mode+0x50);
    if(m->ready&&m->base==base&&m->mode_class==klass&&
       guest_player_context_method_matches(base,&m->fire,0)&&guest_player_context_method_matches(base,&m->stop,0))return 1;
    if(m->discovery_at&&now-m->discovery_at<1000)return 0;
    m->discovery_at=now;m->ready=0;
    uintptr_t f=guest_player_context_find_method(base,klass,"SFXGameModeBase","FireWeapon");
    uintptr_t s=guest_player_context_find_method(base,klass,"SFXGameModeBase","StopFiringWeapon");
    if(!guest_player_context_method_stamp(base,f,&m->fire)){combat_fire_resolution_reason="fire-method-identity";return 0;}
    if(!guest_player_context_method_stamp(base,s,&m->stop)){combat_fire_resolution_reason="stop-method-identity";return 0;}
    if(!combat_fire_method_valid(base,&m->fire,1)){combat_fire_resolution_reason="fire-method-abi-or-body";return 0;}
    if(!combat_fire_method_valid(base,&m->stop,0)){combat_fire_resolution_reason="stop-method-abi-or-body";return 0;}
    combat_fire_resolution_reason="resolved";
    m->base=base;m->mode_class=klass;m->ready=1;return 1;
}
static int combat_fire_dispatch(uintptr_t base,uintptr_t mode,int firing) {
    static const unsigned char pe_pin[16]={0x40,0x55,0x41,0x56,0x41,0x57,0x48,0x81,0xec,0x90,0,0,0,0x48,0x8d,0x6c};
    unsigned char code[16];uint64_t flags=0;CombatFireMethods *m=&combat_fire_methods;
    if(!m->ready||m->base!=base||!narrative_probe_object(base,mode,"SFXGameModeDefault")||
       !read_at(mode+0xc,&flags,8)||(flags&(UINT64_C(1)<<61))||
       !combat_fire_method_valid(base,firing?&m->fire:&m->stop,firing)||!combat_context_layout(base)||
       read_ptr(read_ptr(mode)+0x230)!=base+0xf4f80||!read_at(base+0xf4f80,code,16)||memcmp(code,pe_pin,16))return 0;
    ((ProcessEvent)(base+0xf4f80))((void*)mode,(void*)(firing?m->fire.function.pointer:m->stop.function.pointer),NULL,NULL);return 1;
}
static void combat_fire_log(uintptr_t base,const char *event,const char *reason,DWORD now,
                            int before_valid,unsigned char before,int after_valid,unsigned char after,int called) {
    CombatFirePulse *p=&combat_fire_pulse;char row[5800],pc[1200],pawn[1200],weapon[1200];
    identity_quote(p->pc.full_name,pc,sizeof pc);identity_quote(p->pawn.full_name,pawn,sizeof pawn);identity_quote(p->weapon.full_name,weapon,sizeof weapon);
    uintptr_t owner=0,instigator=0;int relations=guest_camera_stamp_matches(base,&p->weapon,0)&&
        read_at(p->weapon.pointer+0x180,&owner,8)&&read_at(p->weapon.pointer+0x190,&instigator,8);
    unsigned char pawn_roles[2]={255,255},weapon_roles[2]={255,255};
    int roles=guest_camera_stamp_matches(base,&p->pawn,0)&&guest_camera_stamp_matches(base,&p->weapon,0)&&
        read_at(p->pawn.pointer+0x269,pawn_roles,2)&&read_at(p->weapon.pointer+0x269,weapon_roles,2);
    char audit[700];snprintf(audit,sizeof audit,
        "{\"combat_fire_bot\":\"pulse-scope\",\"lease_token\":%lu,\"pc_pointer\":\"0x%llx\",\"pawn_pointer\":\"0x%llx\",\"pawn_object_index\":%d,\"weapon_pointer\":\"0x%llx\",\"weapon_object_index\":%d,\"baseline_ms\":%lu,\"held_ms\":%lu,\"release_attempts\":%d,\"monotonic_ms\":%lu}\r\n",
        p->token,(unsigned long long)p->pc.pointer,(unsigned long long)p->pawn.pointer,p->pawn.index,
        (unsigned long long)p->weapon.pointer,p->weapon.index,p->stable_at?now-p->stable_at:0,
        p->pressed_at?now-p->pressed_at:0,p->release_attempts,now);line(audit);
    snprintf(row,sizeof row,"{\"combat_fire_bot\":\"%s\",\"reason\":\"%s\",\"lease_token\":%lu,\"pc_path\":\"%s\",\"pawn_path\":\"%s\",\"weapon_path\":\"%s\",\"pawn_channel_index\":%d,\"relations_read_valid\":%s,\"weapon_owner\":\"0x%llx\",\"weapon_instigator\":\"0x%llx\",\"owner_is_pawn\":%s,\"instigator_is_pawn\":%s,\"roles_read_valid\":%s,\"pawn_role\":%u,\"pawn_remote_role\":%u,\"weapon_role\":%u,\"weapon_remote_role\":%u,\"input_field_offset\":1152,\"bfire_before_valid\":%s,\"bfire_before\":%u,\"bfire_after_valid\":%s,\"bfire_after\":%u,\"ordinary_script_dispatched\":%s,\"ordinary_ticks_held\":%d,\"firing_verified\":false,\"authoritative_ammo_verdict_required\":true,\"monotonic_ms\":%lu}\r\n",
        event,reason,p->token,pc,pawn,weapon,p->channel_index,relations?"true":"false",(unsigned long long)owner,(unsigned long long)instigator,
        narrative_probe_bool(relations,owner==p->pawn.pointer),narrative_probe_bool(relations,instigator==p->pawn.pointer),roles?"true":"false",
        (unsigned)pawn_roles[1],(unsigned)pawn_roles[0],(unsigned)weapon_roles[1],(unsigned)weapon_roles[0],before_valid?"true":"false",(unsigned)before,
        after_valid?"true":"false",(unsigned)after,called?"true":"false",p->ticks,now);line(row);
}
static void combat_fire_stop(uintptr_t base,const char *reason,DWORD now) {
    CombatFirePulse *p=&combat_fire_pulse;if(!p->active)return;
    if(p->release_attempts&&now-p->release_at<100)return;
    p->release_at=now;++p->release_attempts;
    unsigned char before=255,after=255;int br=0,ar=0,called=0;
    /* Narrow Stop body only touches this exact old PC's input byte. It does
     * not require an active mode/current pawn, nor the combat getter/context.
     * Ownership may already have changed; never redirect it to a new PC.
     */
    int safe=guest_camera_stamp_matches(base,&p->pc,1)&&guest_camera_actor_live(base,p->pc.pointer)&&
        guest_camera_stamp_matches(base,&p->mode,1)&&read_ptr(p->mode.pointer+0x40)==p->pc.pointer;
    if(safe){br=read_at(p->pc.pointer+0x480,&before,1);called=combat_fire_dispatch(base,p->mode.pointer,0);
        ar=read_at(p->pc.pointer+0x480,&after,1);}
    combat_fire_log(base,called&&ar&&after==0?"released":"release-unverified",reason,now,br,before,ar,after,called);
    /* No repeat pulse. Failed cleanup retains ownership for bounded retries
     * while the exact old identity remains safe; stale objects are untouched.
     */
    if((called&&ar&&after==0)||!safe||p->release_attempts>=3)p->active=0;
}
/* Pure QA permission gate. Native local Role determines autonomous control;
 * RemoteRole describes the counterpart and is not a substitute for the
 * separately validated owned/acknowledged pawn, native channel and context.
 * Retained scene-return remote0/local2 is admitted to the ordinary FireWeapon
 * input path. Exact owned FireWeapon/Pawn.StartFire/Weapon.StartFire scripts
 * do not require pawn RemoteRole2/3. Engine guards still decide actual fire. */
static int combat_fire_roles_valid(const unsigned char pawn[2],const unsigned char weapon[2],uint32_t nofire) {
    return (pawn[0]==0||pawn[0]==2||pawn[0]==3)&&pawn[1]==2&&weapon[0]==3&&weapon[1]>=1&&weapon[1]<=2&&!(nofire&0x10000);
}
static int combat_fire_roles_selftest(void) {
    unsigned char pawn[2]={3,2},weapon[2]={3,1};
    if(!combat_fire_roles_valid(pawn,weapon,0))return 0;
    /* Actual retained scene-return tuple: autonomous pawn, simulated weapon. */
    pawn[0]=2;if(!combat_fire_roles_valid(pawn,weapon,0))return 0;
    pawn[0]=0;if(!combat_fire_roles_valid(pawn,weapon,0))return 0;
    if(combat_fire_roles_valid(pawn,weapon,0x10000))return 0;
    pawn[0]=3;
    if(combat_fire_roles_valid(pawn,weapon,0x10000))return 0;
    pawn[1]=1;if(combat_fire_roles_valid(pawn,weapon,0))return 0;
    pawn[1]=0;if(combat_fire_roles_valid(pawn,weapon,0))return 0;
    pawn[1]=3;if(combat_fire_roles_valid(pawn,weapon,0))return 0;
    pawn[1]=2;weapon[1]=3;if(combat_fire_roles_valid(pawn,weapon,0))return 0;
    weapon[1]=0;if(combat_fire_roles_valid(pawn,weapon,0))return 0;
    weapon[1]=1;pawn[0]=255;if(combat_fire_roles_valid(pawn,weapon,0))return 0;
    pawn[0]=3;weapon[0]=255;
    return !combat_fire_roles_valid(pawn,weapon,0);
}
static int combat_fire_conditions(uintptr_t base,uintptr_t pc,uintptr_t pawn,uintptr_t world,int ordinary,
                                  int enabled,uintptr_t *mode_out,uintptr_t *weapon_out,const char **reason) {
    *reason="eligible";GuestPlayerContextLease *l=&guest_player_context_lease;
    if(!enabled||!progress_enabled||!has_marker(root)||test_action!=5||strcmp(test_role,"client")){*reason="disabled";return 0;}
    NetworkActorIdentityEntry channel={0};CombatContextChain chain;
    if(!ordinary||!guest_player_context_conditions(base,pc,pawn,world,ordinary,enabled,1,&channel,&chain)){
        *reason=!ordinary?"scene":guest_player_context_reason;return 0;}
    if(!l->registered||l->pending_cleanup||l->suppressed||l->getter_after!=1||
       !guest_player_context_same(base,pc,pawn,world,&channel)||!guest_player_context_squad_shape(base,pawn)||
       chain.squad!=l->squad.pointer||chain.cached!=pawn||combat_context_derived(&chain,combat_context_layout(base),1)!=1){*reason="registered-native-context";return 0;}
    uintptr_t manager=read_ptr(pc+0xc38);GameArray modes={0};
    if(!read_at(manager+0x70,&modes,sizeof modes)||!modes.data||modes.count<1||modes.count>64||modes.max<modes.count||modes.max>128){*reason="mode-array";return 0;}
    uintptr_t mode=read_ptr(modes.data);
    if(!narrative_probe_object(base,mode,"SFXGameModeDefault")||read_ptr(mode+0x40)!=pc){*reason="mode-outer-not-local-pc";return 0;}
    uintptr_t weapon=0,owner=0,instigator=0,inventory=0;unsigned char pawn_roles[2],weapon_roles[2];uint32_t nofire=0;
    if(!read_at(pawn+0x41c,&weapon,8)||!guest_camera_actor_live(base,weapon)||!is_kind_of(base,weapon,"SFXWeapon_NativeBase")){
        *reason="missing-live-weapon";return 0;}
    *mode_out=mode;*weapon_out=weapon; /* Retain actual diagnostic identities on rejection. */
    if(!read_at(weapon+0x180,&owner,8)||!read_at(weapon+0x190,&instigator,8)||instigator!=pawn||owner!=pawn||
       read_ptr(weapon+0x198)!=world||!read_at(pawn+0x414,&inventory,8)||
       !guest_camera_actor_live(base,inventory)||!is_kind_of(base,inventory,"InventoryManager")||
       read_ptr(weapon+0x2b0)!=inventory){*reason="weapon-owner-instigator-inventory-chain";return 0;}
    if(!read_at(pawn+0x269,pawn_roles,2)||!read_at(weapon+0x269,weapon_roles,2)||
       !read_at(pawn+0x578,&nofire,4)||!combat_fire_roles_valid(pawn_roles,weapon_roles,nofire)){*reason="weapon-or-pawn-role-fire-permission";return 0;}
    return 1;
}
static void combat_fire_bot_tick(uintptr_t base,uintptr_t pc,uintptr_t pawn,uintptr_t world,
                                int ordinary,int enabled,int after,DWORD now) {
    CombatFirePulse *p=&combat_fire_pulse;GuestPlayerContextLease *l=&guest_player_context_lease;
    uintptr_t mode=0,weapon=0;const char *reason=NULL;
    int eligible=combat_fire_conditions(base,pc,pawn,world,ordinary,enabled,&mode,&weapon,&reason);
    int same=p->token&&p->token==l->token&&p->pc.pointer==pc&&p->pawn.pointer==pawn&&p->world.pointer==world&&
        p->mode.pointer==mode&&p->weapon.pointer==weapon&&guest_camera_stamp_matches(base,&p->pc,0)&&
        guest_camera_stamp_matches(base,&p->pawn,0)&&guest_camera_stamp_matches(base,&p->mode,0)&&
        guest_camera_stamp_matches(base,&p->weapon,0)&&l->channel.pointer==p->channel&&l->channel_index==p->channel_index;
    if(p->active){
        if(!eligible||!same||now-p->pressed_at>=250){combat_fire_stop(base,!eligible?reason:!same?"ownership-or-weapon-change":"250ms-timeout",now);return;}
        if(after)++p->ticks;return;
    }
    if(!eligible){
        p->stable_at=0;p->positive_getter=0;
        if(enabled&&progress_enabled&&test_action==5&&l->registered&&(!p->report_at||now-p->report_at>=5000)){
            p->report_at=now;guest_camera_stamp_read(base,pc,&p->pc,1);guest_camera_stamp_read(base,pawn,&p->pawn,1);
            memset(&p->weapon,0,sizeof p->weapon);if(weapon)guest_camera_stamp_read(base,weapon,&p->weapon,1);p->token=l->token;
            combat_fire_log(base,"eligibility",reason,now,0,0,0,0,0);}
        return;
    }
    if(p->attempted_token==l->token)return;
    if(!same){
        unsigned long attempted=p->attempted_token;DWORD report=p->report_at;memset(p,0,sizeof *p);
        p->attempted_token=attempted;p->report_at=report;p->token=l->token;p->channel=l->channel.pointer;p->channel_index=l->channel_index;
        if(!guest_camera_stamp_read(base,pc,&p->pc,1)||!guest_camera_stamp_read(base,pawn,&p->pawn,1)||
           !guest_camera_stamp_read(base,world,&p->world,1)||!guest_camera_stamp_read(base,mode,&p->mode,1)||
           !guest_camera_stamp_read(base,weapon,&p->weapon,1))return;
        p->stable_at=now;
    }
    unsigned char before=255;
    if(!read_at(pc+0x480,&before,1)||before){p->stable_at=0;p->positive_getter=0;return;}
    if(!p->stable_at)p->stable_at=now;
    if(!p->gate_at||now-p->gate_at>=1000){p->gate_at=now;p->positive_getter=guest_player_context_gate(base,"fire-bot-baseline",now)==1;
        if(!p->positive_getter)p->stable_at=0;}
    if(!after||!p->positive_getter||!p->stable_at||now-p->stable_at<8000)return;
    if(!combat_fire_resolve(base,mode,now)){
        if(!p->report_at||now-p->report_at>=5000){p->report_at=now;
            combat_fire_log(base,"method-unavailable",combat_fire_resolution_reason,now,1,before,0,0,0);}
        return;
    }
    p->attempted_token=p->token; /* One attempt per real context/ownership lease. */
    if(guest_player_context_gate(base,"fire-bot-pre-dispatch",now)!=1||
       !combat_fire_conditions(base,pc,pawn,world,ordinary,enabled,&mode,&weapon,&reason)||
       mode!=p->mode.pointer||weapon!=p->weapon.pointer||!guest_camera_stamp_matches(base,&p->pc,1)||
       !guest_camera_stamp_matches(base,&p->pawn,1)||!guest_camera_stamp_matches(base,&p->mode,1)||
       !guest_camera_stamp_matches(base,&p->weapon,1)||!read_at(pc+0x480,&before,1)||before)return;
    p->active=1;p->pressed_at=now;p->ticks=0;
    int called=combat_fire_dispatch(base,mode,1);unsigned char result=255;int read=read_at(pc+0x480,&result,1);
    combat_fire_log(base,called&&read&&result==1?"pressed":"press-unverified","ordinary-FireWeapon",now,1,before,read,result,called);
    if(!called||!read||result!=1)combat_fire_stop(base,"press-not-verified",now);
}

#endif
