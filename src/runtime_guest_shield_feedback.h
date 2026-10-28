#ifndef ME3_RUNTIME_GUEST_SHIELD_FEEDBACK_H
#define ME3_RUNTIME_GUEST_SHIELD_FEEDBACK_H
/* Cosmetic player-shield feedback for the existing owned companion shield.
 * Capture only a real native RepNotify, forward it once unchanged, then defer
 * stock cosmetic calls to Tick. Never replay damage, shield breach or a hit. */
typedef void (*RuntimeShieldExec)(void*,void*,void*);
typedef struct RuntimeShieldScope {
    GuestCameraStamp objects[14]; /* shield, pawn, PC, info, inventory, camera,
        camera mode, world, driver, connection, channel, GRI, pool, CE manager */
    NetworkActorScope world;
    NetworkActorIdentityEntry channel;
    GuestCameraStamp critical;
    float critical_fraction;
    int player;
    unsigned long token;
} RuntimeShieldScope;
typedef struct RuntimeShieldCapture {
    RuntimeShieldScope scope;
    NativeRpcReceiveFrame *receive;
    uintptr_t frame;
    float before,maximum;
    int valid,notification;
} RuntimeShieldCapture;
typedef struct RuntimeShieldPending {
    RuntimeShieldScope scope;
    float current,maximum;
    DWORD received;
    int kind;
} RuntimeShieldPending;
typedef struct RuntimeShieldFeedback {
    uintptr_t base;
    GuestVitalsMethod notify,particle,play;
    GuestCameraStamp player_class,cdo,critical,breach;
    float critical_fraction;
    RuntimeShieldExec original;
    RuntimeShieldPending pending;
    DWORD prepared;
    unsigned rows;
    int installed,busy,reported_unknown;
} RuntimeShieldFeedback;
static RuntimeShieldFeedback runtime_shield_feedback;
enum { RUNTIME_SHIELD_VALUE=1,RUNTIME_SHIELD_MAXIMUM=2,RUNTIME_SHIELD_BREACH=3 };
#pragma pack(push,1)
typedef struct RuntimeShieldParticleParms {uintptr_t template;float scale,offset[3];} RuntimeShieldParticleParms;
typedef struct RuntimeShieldEffectParms {uintptr_t effect,instigator;float params[3];} RuntimeShieldEffectParms;
#pragma pack(pop)
_Static_assert(sizeof(RuntimeShieldParticleParms)==24,"shield ActivatePSC ABI24");
_Static_assert(sizeof(RuntimeShieldEffectParms)==28,"client effect Play ABI28");
static int runtime_guest_shield_feedback_enabled(void) {
    static int enabled=-1;
    if(enabled<0){wchar_t text[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_GUEST_SHIELD_FEEDBACK",text,8);
        enabled=!n||(n==1&&text[0]==L'1');}
    return enabled;
}
static int runtime_shield_transition(float before,float after,float maximum,float fraction) {
    if(!isfinite(before)||!isfinite(after)||!isfinite(maximum)||!isfinite(fraction)||
       maximum<=0||maximum>1e9f||before<=0||before>maximum||after<0||after>=before||fraction<=0||fraction>=1)return 0;
    if(after==0)return 2;
    float critical=maximum*fraction;
    return before>=critical&&after<critical?1:0;
}
static int runtime_shield_values(uintptr_t shield,float *current,float *maximum) {
    return read_at(shield+0x3c4,current,4)&&read_at(shield+0x304,maximum,4)&&
        isfinite(*current)&&isfinite(*maximum)&&*maximum>0&&*maximum<=1e9f&&*current>=0&&*current<=*maximum;
}
static int runtime_shield_notification(const char *name) {
    return !strcmp(name,"ReplicatedShield")?RUNTIME_SHIELD_VALUE:
        !strcmp(name,"MaxShields")?RUNTIME_SHIELD_MAXIMUM:
        !strcmp(name,"ShieldBreachReplicationInfo")?RUNTIME_SHIELD_BREACH:0;
}
static void runtime_shield_pending_update(RuntimeShieldPending *p,const RuntimeShieldScope *scope,
        int notification,float before,float value,float old_maximum,float maximum,DWORD now) {
    /* Player BreachShields already supplies its native PSC/CE/sound/shake.
     * Scalar RepNotify does not execute player ApplyDamageToShields's LOW
     * extra. Supplement that positive crossing only, never native breach.
     * A capacity modifier is not a hit, even when it lowers CurrentShields. */
    if(notification==RUNTIME_SHIELD_MAXIMUM||old_maximum!=maximum||
       (scope->player&&(notification==RUNTIME_SHIELD_BREACH||value<=0))) {
        memset(p,0,sizeof *p);return;
    }
    int kind=runtime_shield_transition(before,value,maximum,scope->critical_fraction);
    if(notification!=RUNTIME_SHIELD_VALUE&&!(notification==RUNTIME_SHIELD_BREACH&&kind==2)) {
        memset(p,0,sizeof *p);return;
    }
    if(p->kind&&!memcmp(&p->scope,scope,sizeof *scope)&&p->maximum==maximum) {
        if(value>p->current){memset(p,0,sizeof *p);}
        else if(value<before){p->current=value;if(kind>p->kind)p->kind=kind;return;}
        else return; /* A second RepNotify for the same state must not replay. */
    }
    if(kind){*p=(RuntimeShieldPending){*scope,value,maximum,now,kind};}
}
static int runtime_shield_pending_current(const RuntimeShieldPending *p,const RuntimeShieldScope *scope,
        float value,float maximum,DWORD now) {
    return p->kind&&(!p->scope.player||(p->kind==1&&value>0))&&now-p->received<=500&&
        !memcmp(&p->scope,scope,sizeof *scope)&&value==p->current&&maximum==p->maximum;
}
static int runtime_shield_assets_live(uintptr_t base);
static int runtime_shield_scope(uintptr_t base,uintptr_t shield,uintptr_t channel,RuntimeShieldScope *out) {
    memset(out,0,sizeof *out);SessionContext c=session_context(session_engine);uintptr_t camera=0,mode=0;
    uint32_t life=0,camera_flags=0,mode_flags=0;
    if(!runtime_guest_shield_feedback_enabled()||GetCurrentThreadId()!=session_game_thread||
       !guest_vitals_owned(base,c)||!read_at(c.pawn+0xe20,&life,4)||(life&0x300)||
       !guest_camera_actor_live(base,shield)||!is_kind_of(base,shield,"SFXShield_Base")||
       !network_actor_layout_matches(base)||
       !network_actor_read_scope(base,shield,&out->world)||out->world.info!=c.world||
       !network_actor_channel_valid(base,shield,channel,out->world.driver,&out->channel)||out->channel.index<=0)return 0;
    /* Hit cosmetics must not depend on the shoulder-scale adapter's idle
     * action admission. Reload, cover and powers can legitimately take hits. */
    camera=read_ptr(c.pc+0x684);mode=read_ptr(camera+0x558);
    if(!guest_camera_actor_live(base,camera)||!is_kind_of(base,camera,"SFXPlayerCamera")||
       read_ptr(c.pc+0xe28)!=camera||read_ptr(camera+0x4a8)!=c.pc||read_ptr(camera+0x198)!=c.world||
       read_ptr(camera+0x3f4)!=c.pawn||!read_at(camera+0x57c,&camera_flags,4)||(camera_flags&1)||
       !narrative_probe_object(base,mode,"SFXCameraMode")||read_ptr(mode+0x7c)!=c.pawn||
       !read_at(mode+0x11c,&mode_flags,4)||(mode_flags&0x20))return 0;
    uintptr_t inventory=read_ptr(c.pawn+0x414),gri=read_ptr(c.world+0x6a8),pool=read_ptr(gri+0x310),effects=read_ptr(gri+0x350);
    if(!guest_camera_actor_live(base,inventory)||!is_kind_of(base,inventory,"SFXInventoryManager")||
       read_ptr(inventory+0x180)!=c.pawn||read_ptr(inventory+0x190)!=c.pawn||
       read_ptr(shield+0x180)!=c.pawn||read_ptr(shield+0x190)!=c.pawn||read_ptr(shield+0x2b0)!=inventory||
       !guest_camera_actor_live(base,gri)||!is_kind_of(base,gri,"SFXGRI")||read_ptr(gri+0x198)!=c.world||
       !guest_camera_actor_live(base,pool)||!is_kind_of(base,pool,"SFXObjectPool")||read_ptr(pool+0x198)!=c.world||
       !narrative_probe_object(base,effects,"RvrClientEffectManager")||
       !runtime_power_property(base,c.world,"WorldInfo","GRI","ObjectProperty",0x6a8,8)||
       !runtime_power_property(base,gri,"SFXGRI","ObjectPool","ObjectProperty",0x310,8)||
       !runtime_power_property(base,gri,"SFXGRI","m_pClientEffectManager","ObjectProperty",0x350,8)||
       !runtime_power_property(base,shield,"SFXShield_Base","CurrentShields","FloatProperty",0x3c4,4)||
       !runtime_power_property(base,shield,"SFXShield_Base","MaxShields","StructProperty",0x2e4,40))return 0;
    uintptr_t seen[128]={0},item=read_ptr(inventory+0x280),head=item;unsigned count=0,shields=0;
    while(item&&count<128) {
        if(!guest_camera_actor_live(base,item)||!is_kind_of(base,item,"Inventory"))return 0;
        for(unsigned i=0;i<count;++i)if(seen[i]==item)return 0;seen[count++]=item;
        if(is_kind_of(base,item,"SFXShield_Base")){if(item!=shield||++shields!=1)return 0;}
        item=read_ptr(item+0x2a8);
    }
    if(item||shields!=1||read_ptr(inventory+0x280)!=head)return 0;
    uintptr_t objects[]={shield,c.pawn,c.pc,c.world,inventory,camera,mode,out->world.world,
        out->world.driver,out->channel.connection,channel,gri,pool,effects};
    for(unsigned i=0;i<14;++i)if(!guest_camera_stamp_read(base,objects[i],&out->objects[i],1))return 0;
    out->player=is_kind_of(base,shield,"SFXShield_Player");
    if(out->player) {
        uintptr_t critical=read_ptr(shield+0x488);float fraction=0,again=0;
        if(!runtime_power_property(base,shield,"SFXShield_Player","PS_CriticalHit","ObjectProperty",0x488,8)||
           !runtime_power_property(base,shield,"SFXShield_Player","PartialBreakPct","FloatProperty",0x4b0,4)||
           !narrative_probe_object(base,critical,"ParticleSystem")||
           !guest_camera_stamp_read(base,critical,&out->critical,1)||
           !read_at(shield+0x4b0,&fraction,4)||!isfinite(fraction)||fraction<=0||fraction>=1||
           read_ptr(shield+0x488)!=critical||!read_at(shield+0x4b0,&again,4)||again!=fraction||
           !guest_camera_stamp_matches(base,&out->critical,1))return 0;
        out->critical_fraction=fraction;
    } else {
        if(!runtime_shield_assets_live(base))return 0;
        out->critical=runtime_shield_feedback.critical;
        out->critical_fraction=runtime_shield_feedback.critical_fraction;
    }
    out->token=guest_player_context_registration_token();return 1;
}
static int runtime_shield_notify_live(uintptr_t base,uintptr_t expected) {
    RuntimeShieldFeedback *s=&runtime_shield_feedback;GameArray script={0};uint16_t native=1;
    uint32_t flags=0;uint16_t size=0,ret=0;unsigned char count=0;
    return s->base==base&&guest_camera_stamp_matches(base,&s->notify.object,1)&&
        !strcmp(s->notify.object.full_name,"SFXGame.SFXShield_Base.ReplicatedEvent")&&
        read_ptr(s->notify.object.pointer+0xd8)==expected&&
        read_at(s->notify.object.pointer+0xe0,&flags,4)&&flags==0x20902&&
        read_at(s->notify.object.pointer+0xe4,&native,2)&&!native&&
        read_at(s->notify.object.pointer+0xe6,&size,2)&&size==8&&
        read_at(s->notify.object.pointer+0xe8,&ret,2)&&ret==65535&&
        read_at(s->notify.object.pointer+0xea,&count,1)&&count==1&&
        read_at(s->notify.object.pointer+0x7c,&script,sizeof script)&&script.data&&script.count==199&&
        script.max>=script.count&&script.max<=4096;
}
static int runtime_shield_contract_live(uintptr_t base) {
    RuntimeShieldFeedback *s=&runtime_shield_feedback;
    return guest_camera_stamp_matches(base,&s->player_class,1)&&guest_camera_stamp_matches(base,&s->cdo,1)&&
        read_ptr(s->player_class.pointer+0x13c)==s->cdo.pointer&&read_ptr(s->cdo.pointer+0x50)==s->player_class.pointer&&
        guest_vitals_method_live(base,&s->particle)&&guest_vitals_method_live(base,&s->play);
}
static int runtime_shield_assets_live(uintptr_t base) {
    RuntimeShieldFeedback *s=&runtime_shield_feedback;float fraction=0;
    return runtime_shield_contract_live(base)&&
        guest_camera_stamp_matches(base,&s->critical,1)&&guest_camera_stamp_matches(base,&s->breach,1)&&
        read_ptr(s->cdo.pointer+0x488)==s->critical.pointer&&read_ptr(s->cdo.pointer+0x490)==s->breach.pointer&&
        read_at(s->cdo.pointer+0x4b0,&fraction,4)&&fraction==s->critical_fraction;
}
static void runtime_shield_exec(void *self,void *frame,void *result);
static RuntimeShieldCapture runtime_shield_capture(void *self,void *frame) {
    RuntimeShieldFeedback *s=&runtime_shield_feedback;RuntimeShieldCapture c={0};GameArray script={0};
    NativeRpcReceiveFrame *receive=native_rpc_receive_current(session_game_thread);uint32_t name[2]={0};char text[96];
    if(!s->installed||s->busy||!receive||!receive->channel||
       !runtime_shield_notify_live(s->base,(uintptr_t)runtime_shield_exec)||!runtime_shield_contract_live(s->base)||
       read_ptr((uintptr_t)frame+0x18)!=s->notify.object.pointer||read_ptr((uintptr_t)frame+0x20)!=(uintptr_t)self||
       !read_at(s->notify.object.pointer+0x7c,&script,sizeof script)||read_ptr((uintptr_t)frame+0x28)!=script.data||
       !read_at(read_ptr((uintptr_t)frame+0x30),name,8)||!name[0]||name[1])return c;
    name_of(s->base,name[0],text,sizeof text);
    c.notification=runtime_shield_notification(text);if(!c.notification)return c;
    if(!runtime_shield_scope(s->base,(uintptr_t)self,receive->channel,&c.scope)||
       !runtime_shield_values((uintptr_t)self,&c.before,&c.maximum))return c;
    c.receive=receive;c.frame=(uintptr_t)frame;c.valid=1;return c;
}
static void runtime_shield_after(RuntimeShieldCapture *c) {
    if(!c->valid)return;
    RuntimeShieldFeedback *s=&runtime_shield_feedback;RuntimeShieldScope current;float value=0,maximum=0;
    if(c->notification==RUNTIME_SHIELD_MAXIMUM||
       (c->scope.player&&c->notification==RUNTIME_SHIELD_BREACH))
        memset(&s->pending,0,sizeof s->pending);
    NativeRpcReceiveFrame *receive=native_rpc_receive_current(session_game_thread);uintptr_t shield=c->scope.objects[0].pointer;
    if(receive!=c->receive||!receive||receive->channel!=c->scope.channel.channel||
       !runtime_shield_notify_live(s->base,(uintptr_t)runtime_shield_exec)||
       read_ptr(c->frame+0x18)!=s->notify.object.pointer||read_ptr(c->frame+0x20)!=shield||
       !runtime_shield_scope(s->base,shield,receive->channel,&current)||memcmp(&current,&c->scope,sizeof current)||
       !runtime_shield_values(shield,&value,&maximum))return;
    runtime_shield_pending_update(&s->pending,&current,c->notification,c->before,value,c->maximum,maximum,GetTickCount());
}
static void runtime_shield_forward(void *self,void *frame,void *result,RuntimeShieldExec stock,
        RuntimeShieldCapture (*capture)(void*,void*),void (*after)(RuntimeShieldCapture*)) {
    DWORD entry=GetLastError();RuntimeShieldCapture c=capture(self,frame);
    SetLastError(entry);stock(self,frame,result);DWORD error=GetLastError();after(&c);SetLastError(error);
}
static void runtime_shield_exec(void *self,void *frame,void *result) {
    runtime_shield_forward(self,frame,result,runtime_shield_feedback.original,runtime_shield_capture,runtime_shield_after);
}
static int runtime_shield_resolve(uintptr_t base) {
    RuntimeShieldFeedback *s=&runtime_shield_feedback;
    static const GuestVitalsField notify[]={{"VarName","NameProperty",0,8,0x80}};
    static const GuestVitalsField particle[]={{"Template","ObjectProperty",0,8,0x80},
        {"ImpactScale","FloatProperty",8,4,0x90},{"Offset","StructProperty",12,12,0x90}};
    static const GuestVitalsField play[]={{"pEffect","ObjectProperty",0,8,0x80},
        {"pInstigator","ObjectProperty",8,8,0x80},{"vParams","StructProperty",16,12,0x90}};
    uintptr_t player=read_ptr(function_named(base,"SFXShield_Player","BreachShields")+0x40),cdo=read_ptr(player+0x13c);
    char name[96];object_name(base,player,name,sizeof name);
    if(strcmp(name,"SFXShield_Player")||!narrative_probe_object(base,player,"Class")||
       !narrative_probe_object(base,cdo,"SFXShield_Player")||read_ptr(cdo+0x50)!=player||
       !runtime_power_property(base,cdo,"SFXShield_Player","PS_CriticalHit","ObjectProperty",0x488,8)||
       !runtime_power_property(base,cdo,"SFXShield_Player","CE_ShieldBreak","ObjectProperty",0x490,8)||
       !runtime_power_property(base,cdo,"SFXShield_Player","PartialBreakPct","FloatProperty",0x4b0,4)||
       !guest_camera_stamp_read(base,player,&s->player_class,1)||!guest_camera_stamp_read(base,cdo,&s->cdo,1)||
       !guest_vitals_method(base,"SFXShield_Base","ReplicatedEvent",0x20902,8,notify,1,&s->notify)||
       !guest_vitals_method(base,"SFXShield_Base","ActivatePSC",0x84103,24,particle,3,&s->particle)||
       !guest_vitals_method(base,"RvrClientEffectManager","Play",0x24401,28,play,3,&s->play))return 0;
    /* Canonical assets are only a nonplayer fallback. A real player shield
     * supplies its own reflected critical asset and threshold in its scope. */
    GuestCameraStamp critical={0},breach={0};float fraction=0;
    memset(&s->critical,0,sizeof s->critical);memset(&s->breach,0,sizeof s->breach);s->critical_fraction=0;
    if(narrative_probe_object(base,read_ptr(cdo+0x488),"ParticleSystem")&&
       narrative_probe_object(base,read_ptr(cdo+0x490),"RvrClientEffectInterface")&&
       guest_camera_stamp_read(base,read_ptr(cdo+0x488),&critical,1)&&
       guest_camera_stamp_read(base,read_ptr(cdo+0x490),&breach,1)&&
       read_at(cdo+0x4b0,&fraction,4)&&isfinite(fraction)&&fraction>0&&fraction<1) {
        s->critical=critical;s->breach=breach;s->critical_fraction=fraction;
    }
    s->base=base;return runtime_shield_notify_live(base,base+0xf52b0)&&runtime_shield_contract_live(base);
}
static void runtime_guest_shield_feedback_tick(uintptr_t base,uintptr_t engine,DWORD now) {
    DWORD error=GetLastError();RuntimeShieldFeedback *s=&runtime_shield_feedback;
    if(GetCurrentThreadId()!=session_game_thread||g_runtime_config.role!=ME3_ROLE_GUEST||s->busy)goto done;
    if(!runtime_guest_shield_feedback_enabled()||!guest_vitals_owned(base,session_context(engine))) {
        memset(&s->pending,0,sizeof s->pending);goto done;
    }
    if(!s->installed) {
        if(s->prepared&&now-s->prepared<1000)goto done;s->prepared=now;
        /* The existing receive forwarder supplies channel provenance. */
        if(!runtime_rpc_receive_installed||!supported_image(base)||!guest_equipment_dispatch_pins(base)||
           !runtime_shield_resolve(base)) {
            if(!s->reported_unknown){s->reported_unknown=1;line("{\"guest_shield_feedback\":\"waiting-for-stock-contract-or-assets\",\"gameplay_verified\":false}\r\n");}
            goto done;
        }
        uintptr_t slot=s->notify.object.pointer+0xd8;
        if(!background_focus_writable(slot,8))goto done;
        s->original=(RuntimeShieldExec)(base+0xf52b0);
        if(InterlockedCompareExchangePointer((PVOID volatile*)slot,(PVOID)runtime_shield_exec,
           (PVOID)s->original)!=(PVOID)s->original)goto done;
        s->installed=1;line("{\"guest_shield_feedback\":\"stock-repnotify-wrapper-installed\",\"cosmetic_only\":true,\"gameplay_verified\":false}\r\n");
    }
    if(s->pending.kind) {
        RuntimeShieldPending p=s->pending;RuntimeShieldScope current;float value=0,maximum=0;
        memset(&s->pending,0,sizeof s->pending); /* Consume before any native callback. */
        if(!runtime_shield_notify_live(base,(uintptr_t)runtime_shield_exec)||
           !runtime_shield_contract_live(base)||!runtime_shield_scope(base,p.scope.objects[0].pointer,p.scope.channel.channel,&current)||
           !runtime_shield_values(p.scope.objects[0].pointer,&value,&maximum)||
           !runtime_shield_pending_current(&p,&current,value,maximum,now))goto done;
        uintptr_t shield=p.scope.objects[0].pointer,manager=p.scope.objects[13].pointer;
        uintptr_t actor_pe=read_ptr(read_ptr(shield)+0x230),object_pe=read_ptr(read_ptr(manager)+0x230);
        if(actor_pe!=base+0x40d450||object_pe!=base+0xf4f80)goto done;
        RuntimeShieldParticleParms particle={p.scope.critical.pointer,1.0f,{0,0,65}};int breach_called=0;
        s->busy=1;SetLastError(error);
        ((ProcessEvent)actor_pe)((void*)shield,(void*)s->particle.object.pointer,&particle,NULL);
        s->busy=0;
        if(p.kind==2&&!p.scope.player&&runtime_shield_assets_live(base)&&
           runtime_shield_scope(base,shield,p.scope.channel.channel,&current)&&!memcmp(&current,&p.scope,sizeof current)&&
           runtime_shield_values(shield,&value,&maximum)&&value==p.current&&maximum==p.maximum&&
           read_ptr(read_ptr(manager)+0x230)==object_pe) {
            RuntimeShieldEffectParms effect={s->breach.pointer,p.scope.objects[1].pointer,{0,0,0}};
            s->busy=1;SetLastError(error);
            ((ProcessEvent)object_pe)((void*)manager,(void*)s->play.object.pointer,&effect,NULL);s->busy=0;
            breach_called=1;
        }
        if(s->rows++<16){char row[280];snprintf(row,sizeof row,
            "{\"guest_shield_feedback\":\"stock-cosmetic-dispatched\",\"kind\":%d,\"player_shield\":%s,\"breach_effect_dispatched\":%s,\"current\":%.3f,\"maximum\":%.3f,\"gameplay_verified\":false}\r\n",p.kind,p.scope.player?"true":"false",breach_called?"true":"false",p.current,p.maximum);line(row);}
    }
done:SetLastError(error);
}
#ifdef ME3_RUNTIME_FIXTURE
static void *runtime_shield_fixture_args[3];
static unsigned runtime_shield_fixture_calls,runtime_shield_fixture_order;
static int runtime_shield_fixture_valid,runtime_shield_fixture_bad;
static RuntimeShieldCapture runtime_shield_fixture_capture(void *self,void *frame) {
    RuntimeShieldCapture c={0};c.valid=runtime_shield_fixture_valid;
    if(self!=runtime_shield_fixture_args[0]||frame!=runtime_shield_fixture_args[1]||runtime_shield_fixture_order++)runtime_shield_fixture_bad=1;
    SetLastError(11);return c;
}
static void runtime_shield_fixture_stock(void *self,void *frame,void *result) {
    if(self!=runtime_shield_fixture_args[0]||frame!=runtime_shield_fixture_args[1]||result!=runtime_shield_fixture_args[2]||
       GetLastError()!=37||runtime_shield_fixture_order++!=1)runtime_shield_fixture_bad=1;
    ++runtime_shield_fixture_calls;SetLastError(53);
}
static void runtime_shield_fixture_after(RuntimeShieldCapture *c) {
    if(c->valid!=runtime_shield_fixture_valid||GetLastError()!=53||runtime_shield_fixture_order++!=2)runtime_shield_fixture_bad=1;
    SetLastError(79);
}
static int runtime_guest_shield_feedback_selftest(void) {
    DWORD error=GetLastError();int a,b,c,ok=1;
    runtime_shield_fixture_args[0]=&a;runtime_shield_fixture_args[1]=&b;runtime_shield_fixture_args[2]=&c;
    runtime_shield_fixture_bad=0;
    for(int valid=0;valid<2;++valid) {
        runtime_shield_fixture_valid=valid;runtime_shield_fixture_calls=runtime_shield_fixture_order=0;SetLastError(37);
        runtime_shield_forward(&a,&b,&c,runtime_shield_fixture_stock,runtime_shield_fixture_capture,runtime_shield_fixture_after);
        ok=ok&&GetLastError()==53&&runtime_shield_fixture_calls==1&&runtime_shield_fixture_order==3&&!runtime_shield_fixture_bad;
    }
    ok=ok&&runtime_shield_transition(100,54,100,.55f)==1&&runtime_shield_transition(55,54,100,.55f)==1&&
        !runtime_shield_transition(54,53,100,.55f)&&runtime_shield_transition(1,0,100,.55f)==2&&
        !runtime_shield_transition(0,0,100,.55f)&&!runtime_shield_transition(50,100,100,.55f)&&
        !runtime_shield_transition(100,100,100,.55f)&&!runtime_shield_transition(101,50,100,.55f)&&
        !runtime_shield_transition(100,-1,100,.55f)&&!runtime_shield_transition(100,50,0,.55f)&&
        !runtime_shield_transition(NAN,0,100,.55f)&&!runtime_shield_transition(100,NAN,100,.55f)&&
        !runtime_shield_transition(100,0,INFINITY,.55f)&&!runtime_shield_transition(100,0,100,NAN)&&
        !runtime_shield_transition(100,0,100,0)&&!runtime_shield_transition(100,0,100,1);
    ok=ok&&runtime_shield_notification("ReplicatedShield")==RUNTIME_SHIELD_VALUE&&
        runtime_shield_notification("MaxShields")==RUNTIME_SHIELD_MAXIMUM&&
        runtime_shield_notification("ShieldBreachReplicationInfo")==RUNTIME_SHIELD_BREACH&&
        !runtime_shield_notification("CurrentShields");
    RuntimeShieldPending pending={0};RuntimeShieldScope scope={0};scope.token=42;scope.critical_fraction=.55f;
    runtime_shield_pending_update(&pending,&scope,RUNTIME_SHIELD_VALUE,100,54,100,100,1000);
    ok=ok&&pending.kind==1&&pending.current==54&&pending.received==1000;
    runtime_shield_pending_update(&pending,&scope,RUNTIME_SHIELD_VALUE,54,50,100,100,1001);
    ok=ok&&pending.kind==1&&pending.current==50&&pending.received==1000;
    runtime_shield_pending_update(&pending,&scope,RUNTIME_SHIELD_VALUE,50,0,100,100,1002);
    ok=ok&&pending.kind==2&&pending.current==0&&pending.received==1000;
    runtime_shield_pending_update(&pending,&scope,RUNTIME_SHIELD_VALUE,0,0,100,100,1003);
    ok=ok&&pending.kind==2&&pending.current==0&&pending.received==1000;
    memset(&pending,0,sizeof pending); /* Consumed event cannot be recreated by another zero RepNotify. */
    runtime_shield_pending_update(&pending,&scope,RUNTIME_SHIELD_VALUE,0,0,100,100,1004);ok=ok&&!pending.kind;
    runtime_shield_pending_update(&pending,&scope,RUNTIME_SHIELD_VALUE,100,54,100,100,1005);
    runtime_shield_pending_update(&pending,&scope,RUNTIME_SHIELD_VALUE,54,75,100,100,1006);ok=ok&&!pending.kind;
    runtime_shield_pending_update(&pending,&scope,RUNTIME_SHIELD_VALUE,100,54,100,100,1007);
    ++scope.token;runtime_shield_pending_update(&pending,&scope,RUNTIME_SHIELD_BREACH,100,0,100,100,1008);
    ok=ok&&pending.kind==2&&pending.scope.token==43&&pending.received==1008;
    memset(&pending,0,sizeof pending);scope.player=1;scope.critical_fraction=.4f;scope.critical.pointer=64;scope.critical.index=17;
    /* The instance threshold, not the canonical fallback's .55, controls LOW. */
    runtime_shield_pending_update(&pending,&scope,RUNTIME_SHIELD_VALUE,100,54,100,100,2000);ok=ok&&!pending.kind;
    runtime_shield_pending_update(&pending,&scope,RUNTIME_SHIELD_VALUE,100,39,100,100,2001);
    ok=ok&&pending.kind==1&&pending.current==39&&pending.scope.critical.pointer==64&&
        runtime_shield_pending_current(&pending,&scope,39,100,2501)&&
        !runtime_shield_pending_current(&pending,&scope,39,100,2502)&&
        !runtime_shield_pending_current(&pending,&scope,38,100,2002)&&
        !runtime_shield_pending_current(&pending,&scope,39,101,2002);
    RuntimeShieldScope changed=scope;
    ++changed.critical.pointer;ok=ok&&!runtime_shield_pending_current(&pending,&changed,39,100,2002);
    changed=scope;++changed.critical.index;ok=ok&&!runtime_shield_pending_current(&pending,&changed,39,100,2002);
    changed=scope;changed.critical.pointer=0;ok=ok&&!runtime_shield_pending_current(&pending,&changed,39,100,2002);
    changed=scope;changed.critical_fraction=.55f;ok=ok&&!runtime_shield_pending_current(&pending,&changed,39,100,2002);
    changed=scope;++changed.token;ok=ok&&!runtime_shield_pending_current(&pending,&changed,39,100,2002);
    runtime_shield_pending_update(&pending,&scope,RUNTIME_SHIELD_VALUE,39,39,100,100,2002);
    ok=ok&&pending.kind==1&&pending.received==2001; /* Duplicate keeps one pending cosmetic. */
    runtime_shield_pending_update(&pending,&scope,RUNTIME_SHIELD_VALUE,39,0,100,100,2003);ok=ok&&!pending.kind;
    runtime_shield_pending_update(&pending,&scope,RUNTIME_SHIELD_BREACH,100,0,100,100,2004);ok=ok&&!pending.kind;
    runtime_shield_pending_update(&pending,&scope,RUNTIME_SHIELD_VALUE,100,39,100,100,2005);
    runtime_shield_pending_update(&pending,&scope,RUNTIME_SHIELD_BREACH,39,39,100,100,2006);ok=ok&&!pending.kind;
    runtime_shield_pending_update(&pending,&scope,RUNTIME_SHIELD_MAXIMUM,100,39,100,100,2007);ok=ok&&!pending.kind;
    runtime_shield_pending_update(&pending,&scope,RUNTIME_SHIELD_VALUE,100,39,100,100,2008);
    runtime_shield_pending_update(&pending,&scope,RUNTIME_SHIELD_MAXIMUM,39,39,100,100,2009);ok=ok&&!pending.kind;
    runtime_shield_pending_update(&pending,&scope,RUNTIME_SHIELD_VALUE,100,39,101,100,2010);ok=ok&&!pending.kind;
    runtime_shield_pending_update(&pending,&scope,RUNTIME_SHIELD_VALUE,100,39,100,100,2011);
    runtime_shield_pending_update(&pending,&scope,RUNTIME_SHIELD_VALUE,39,75,100,100,2012);ok=ok&&!pending.kind;
    scope.player=0;
    runtime_shield_pending_update(&pending,&scope,RUNTIME_SHIELD_BREACH,100,39,100,100,2013);ok=ok&&!pending.kind;
    runtime_shield_pending_update(&pending,&scope,RUNTIME_SHIELD_MAXIMUM,100,0,100,100,2014);ok=ok&&!pending.kind;
    scope.player=1;pending=(RuntimeShieldPending){scope,0,100,2015,2};
    ok=ok&&!runtime_shield_pending_current(&pending,&scope,0,100,2015); /* No player breach replay. */
    SetLastError(error);return ok;
}
#endif
#endif
