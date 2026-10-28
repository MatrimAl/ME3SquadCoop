#ifndef ME3_RUNTIME_GUEST_HIT_FEEDBACK_H
#define ME3_RUNTIME_GUEST_HIT_FEEDBACK_H
/* Supply the missing player damage-direction queue on a real owned weapon
 * impact RepNotify. Never replay PlayHit, damage, reactions or shield effects.
 * A receive identifies delivery time, not the server's original impact time. */
typedef void (*RuntimeHitExec)(void*,void*,void*);
#pragma pack(push,4)
typedef struct RuntimeHitImpact {
    uintptr_t weapon,projectile;
    int32_t reaction;
    unsigned char trigger,delay,padding[2];
} RuntimeHitImpact;
typedef struct RuntimeHitHostileParms {uintptr_t other;uint32_t result;} RuntimeHitHostileParms;
#pragma pack(pop)
_Static_assert(sizeof(RuntimeHitImpact)==24,"native weapon impact ABI24");
_Static_assert(offsetof(RuntimeHitImpact,trigger)==20,"native impact trigger offset");
_Static_assert(sizeof(RuntimeHitHostileParms)==12,"Pawn.IsHostile ABI12");
typedef struct RuntimeHitScope {
    SessionHandle objects[7]; /* PC, pawn, info, world, driver, connection, channel */
    unsigned long token;
    int channel_index;
} RuntimeHitScope;
typedef struct RuntimeHitPending {
    RuntimeHitScope scope;
    GuestCameraStamp weapon,attacker;
    DWORD received;
    unsigned char trigger;
} RuntimeHitPending;
typedef struct RuntimeHitCapture {
    RuntimeHitScope scope;
    RuntimeHitImpact impact;
    RuntimeHitPending source;
    NativeRpcReceiveFrame *receive;
    uintptr_t frame;
    int valid;
} RuntimeHitCapture;
typedef struct RuntimeHitFeedback {
    uintptr_t base,hostile_entry;
    GuestVitalsMethod notify,queue;
    GuestPlayerContextMethod hostile;
    RuntimeHitExec original;
    RuntimeHitScope baseline;
    RuntimeHitPending pending[8];
    DWORD checked;
    unsigned count,rows;
    unsigned char trigger;
    int installed,busy,baselined;
} RuntimeHitFeedback;
static RuntimeHitFeedback runtime_hit_feedback;
static void runtime_hit_exec(void*,void*,void*);
static int runtime_hit_enabled(void) {
    static int enabled=-1;
    if(enabled<0){wchar_t text[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_GUEST_HIT_FEEDBACK",text,8);
        enabled=!n||(n==1&&text[0]==L'1');}return enabled;
}
static int runtime_hit_scope_same(const RuntimeHitScope *a,const RuntimeHitScope *b) {
    if(!a->token||a->token!=b->token||a->channel_index!=b->channel_index)return 0;
    for(unsigned i=0;i<7;++i)if(!runtime_movement_same_handle(a->objects[i],b->objects[i]))return 0;
    return 1;
}
static int runtime_hit_scope(uintptr_t base,RuntimeHitScope *out) {
    SessionContext c=session_context(session_engine);NetworkActorScope world={0};NetworkActorIdentityEntry channel={0};uint32_t flags=0;
    memset(out,0,sizeof *out);
    if(!runtime_hit_enabled()||GetCurrentThreadId()!=session_game_thread||!guest_vitals_owned(base,c)||
       !guest_camera_actor_live(base,c.pc)||!is_kind_of(base,c.pc,"SFXPlayerController")||
       !guest_camera_actor_live(base,c.pawn)||read_ptr(c.pc+0x3bc)!=c.pawn||read_ptr(c.pawn+0x374)!=c.pc||
       read_ptr(c.pc+0x198)!=c.world||!read_at(c.pawn+0xe20,&flags,4)||!(flags&0x10)||(flags&0x300)||
       !network_actor_read_scope(base,c.pawn,&world)||world.info!=c.world||
       !guest_player_context_channel(base,c.pawn,&channel)||channel.index<=0||channel.driver!=world.driver)return 0;
    uintptr_t objects[]={c.pc,c.pawn,c.world,world.world,world.driver,channel.connection,channel.channel};
    for(unsigned i=0;i<7;++i){out->objects[i]=session_capture_handle(objects[i]);if(!session_handle_matches(out->objects[i]))return 0;}
    out->token=guest_player_context_registration_token();out->channel_index=channel.index;return out->token!=0;
}
static int runtime_hit_layout(uintptr_t base,uintptr_t pawn) {
    uintptr_t field=runtime_guest_team_field(base,pawn,"BioPawn","ReplicatedWeaponImpactInfo","StructProperty",0x9cc,24,0x100002020ULL);
    uintptr_t type=read_ptr(field+0xd0);GuestCameraStamp stamp={0};int32_t size=0;
    if(!field||!guest_camera_stamp_read(base,type,&stamp,1)||strcmp(stamp.full_name,"SFXGame.BioPawn.ReplicatedWeaponImpact")||
       !narrative_probe_object(base,type,"ScriptStruct")||!read_at(type+0x78,&size,4)||size!=24)return 0;
    const char *names[]={"oWeapon","oProjectile","CustomActionReactionType","TriggerCounter","Delay"};
    const char *kinds[]={"ObjectProperty","ObjectProperty","IntProperty","ByteProperty","ByteProperty"};
    const int offsets[]={0,8,16,20,21},sizes[]={8,8,4,1,1};
    for(unsigned i=0;i<5;++i){uint64_t flags=1;uintptr_t prop=runtime_native_hud_field(base,type,names[i],kinds[i],offsets[i],sizes[i]);
        if(!prop||!read_at(prop+0x78,&flags,8)||flags)return 0;
        if(i<2){char name[96];uintptr_t klass=read_ptr(prop+0xd0);object_name(base,klass,name,sizeof name);
            if(!narrative_probe_object(base,klass,"Class")||strcmp(name,i?"SFXProjectile":"SFXWeapon"))return 0;}}
    return guest_camera_stamp_matches(base,&stamp,1);
}
static int runtime_hit_read(uintptr_t pawn,RuntimeHitImpact *out) {return read_at(pawn+0x9cc,out,sizeof *out);}
static int runtime_hit_impact_same(const RuntimeHitImpact *a,const RuntimeHitImpact *b) {
    return a->weapon==b->weapon&&a->projectile==b->projectile&&a->reaction==b->reaction&&a->trigger==b->trigger&&a->delay==b->delay;
}
static int runtime_hit_fresh(RuntimeHitFeedback *s,const RuntimeHitScope *scope,unsigned char trigger) {
    /* Initial/current replicated state is not a new hit. Counter wrapping is
     * valid; equality is a duplicate. No hit count or age is inferred. */
    if(!s->baselined||!runtime_hit_scope_same(&s->baseline,scope)) {
        s->baseline=*scope;s->trigger=trigger;s->baselined=1;s->count=0;return 0;
    }
    if(trigger==s->trigger)return 0;s->trigger=trigger;return 1;
}
static int runtime_hit_notify_live(uintptr_t base,uintptr_t expected) {
    RuntimeHitFeedback *s=&runtime_hit_feedback;GameArray script={0};uint16_t native=1,size=0,ret=0;uint32_t flags=0;unsigned char count=0;
    return guest_camera_stamp_matches(base,&s->notify.object,1)&&!strcmp(s->notify.object.full_name,"SFXGame.BioPawn.ReplicatedEvent")&&
        read_ptr(s->notify.object.pointer+0xd8)==expected&&read_at(s->notify.object.pointer+0xe0,&flags,4)&&flags==0x20902&&
        read_at(s->notify.object.pointer+0xe4,&native,2)&&!native&&read_at(s->notify.object.pointer+0xe6,&size,2)&&size==8&&
        read_at(s->notify.object.pointer+0xe8,&ret,2)&&ret==65535&&read_at(s->notify.object.pointer+0xea,&count,1)&&count==1&&
        read_at(s->notify.object.pointer+0x7c,&script,sizeof script)&&script.data&&script.count==891&&script.max>=891&&script.max<=4096;
}
static int runtime_hit_methods(uintptr_t base,uintptr_t pc) {
    RuntimeHitFeedback *s=&runtime_hit_feedback;
    return guest_vitals_method_live(base,&s->queue)&&!strcmp(s->queue.object.full_name,"SFXGame.BioPlayerController.QueueDamageIndicator")&&
        guest_equipment_find_first(base,read_ptr(pc+0x50),"QueueDamageIndicator")==s->queue.object.pointer&&
        guest_player_context_method_matches(base,&s->hostile,1)&&
        !strcmp(s->hostile.function.full_name,"Engine.Pawn.IsHostile")&&
        s->hostile_entry>=base+0x1000&&s->hostile_entry<base+0x1300000&&s->hostile_entry!=base+0xf52b0&&
        guest_player_context_function_abi(base,s->hostile.function.pointer,"Pawn","IsHostile",0x20400,12,8,0,s->hostile_entry-base,2)&&
        read_ptr(read_ptr(pc)+0x230)==base+0x40d450;
}
static int runtime_hit_source(uintptr_t base,const RuntimeHitScope *scope,uintptr_t weapon,RuntimeHitPending *out) {
    uintptr_t info=scope->objects[2].pointer,attacker=read_ptr(weapon+0x190);
    if(!guest_camera_actor_live(base,weapon)||!is_kind_of(base,weapon,"SFXWeapon")||read_ptr(weapon+0x198)!=info||
       !runtime_power_property(base,weapon,"Actor","Instigator","ObjectProperty",0x190,8)||
       !guest_camera_actor_live(base,attacker)||!is_kind_of(base,attacker,"Pawn")||attacker==scope->objects[1].pointer||
       read_ptr(attacker+0x198)!=info||
       guest_equipment_find_first(base,read_ptr(attacker+0x50),"IsHostile")!=runtime_hit_feedback.hostile.function.pointer||
       read_ptr(read_ptr(attacker)+0x230)!=base+0x40d450||
       !guest_camera_stamp_read(base,weapon,&out->weapon,1)||!guest_camera_stamp_read(base,attacker,&out->attacker,1))return 0;
    return read_ptr(weapon+0x190)==attacker;
}
static RuntimeHitCapture runtime_hit_capture(void *self,void *frame) {
    RuntimeHitFeedback *s=&runtime_hit_feedback;RuntimeHitCapture c={0};uint32_t name[2]={0};char text[96];GameArray script={0};
    NativeRpcReceiveFrame *receive=native_rpc_receive_current(session_game_thread);
    if(!s->installed||s->busy||!receive||!receive->channel||
       !runtime_hit_notify_live(s->base,(uintptr_t)runtime_hit_exec)||!runtime_hit_scope(s->base,&c.scope)||
       (uintptr_t)self!=c.scope.objects[1].pointer||receive->channel!=c.scope.objects[6].pointer||
       !runtime_hit_methods(s->base,c.scope.objects[0].pointer)||!runtime_hit_layout(s->base,(uintptr_t)self)||
       read_ptr((uintptr_t)frame+0x18)!=s->notify.object.pointer||read_ptr((uintptr_t)frame+0x20)!=(uintptr_t)self||
       !read_at(s->notify.object.pointer+0x7c,&script,sizeof script)||read_ptr((uintptr_t)frame+0x28)!=script.data||
       !read_at(read_ptr((uintptr_t)frame+0x30),name,8)||!name[0]||name[1])return c;
    name_of(s->base,name[0],text,sizeof text);if(strcmp(text,"ReplicatedWeaponImpactInfo")||!runtime_hit_read((uintptr_t)self,&c.impact))return c;
    if(!runtime_hit_fresh(s,&c.scope,c.impact.trigger)||!runtime_hit_source(s->base,&c.scope,c.impact.weapon,&c.source))return c;
    c.frame=(uintptr_t)frame;c.receive=receive;c.valid=1;return c;
}
static void runtime_hit_enqueue(RuntimeHitFeedback *s,const RuntimeHitPending *p) {
    for(unsigned i=0;i<s->count;++i)if(runtime_hit_scope_same(&s->pending[i].scope,&p->scope)&&
       s->pending[i].attacker.pointer==p->attacker.pointer&&s->pending[i].attacker.index==p->attacker.index) {s->pending[i]=*p;return;}
    if(s->count<8)s->pending[s->count++]=*p;
}
static void runtime_hit_after(RuntimeHitCapture *c) {
    RuntimeHitFeedback *s=&runtime_hit_feedback;RuntimeHitScope current={0};RuntimeHitImpact impact={0};RuntimeHitPending p={0};
    if(!c->valid||s->busy||native_rpc_receive_current(session_game_thread)!=c->receive||
       !c->receive||c->receive->channel!=c->scope.objects[6].pointer||
       !runtime_hit_notify_live(s->base,(uintptr_t)runtime_hit_exec)||!runtime_hit_scope(s->base,&current)||
       !runtime_hit_scope_same(&current,&c->scope)||!runtime_hit_methods(s->base,current.objects[0].pointer)||
       !runtime_hit_read(current.objects[1].pointer,&impact)||!runtime_hit_impact_same(&impact,&c->impact)||
       !guest_camera_stamp_matches(s->base,&c->source.weapon,1)||!guest_camera_stamp_matches(s->base,&c->source.attacker,1)||
       !runtime_hit_source(s->base,&current,impact.weapon,&p)||p.attacker.pointer!=c->source.attacker.pointer||p.attacker.index!=c->source.attacker.index)return;
    p.scope=current;p.trigger=impact.trigger;p.received=GetTickCount();runtime_hit_enqueue(s,&p);
}
static void runtime_hit_forward(void *self,void *frame,void *result,RuntimeHitExec stock,
        RuntimeHitCapture (*capture)(void*,void*),void (*after)(RuntimeHitCapture*)) {
    DWORD entry=GetLastError();RuntimeHitCapture c=capture(self,frame);SetLastError(entry);stock(self,frame,result);
    DWORD error=GetLastError();after(&c);SetLastError(error);
}
static void runtime_hit_exec(void *self,void *frame,void *result) {
    runtime_hit_forward(self,frame,result,runtime_hit_feedback.original,runtime_hit_capture,runtime_hit_after);
}
static int runtime_hit_resolve(uintptr_t base,const RuntimeHitScope *scope) {
    RuntimeHitFeedback *s=&runtime_hit_feedback;
    static const GuestVitalsField notify[]={{"VarName","NameProperty",0,8,0x80}},queue[]={{"oPawn","ObjectProperty",0,8,0x80}};
    uintptr_t hostile=function_named(base,"Pawn","IsHostile");
    s->hostile_entry=read_ptr(hostile+0xd8);
    if(!guest_vitals_method(base,"BioPawn","ReplicatedEvent",0x20902,8,notify,1,&s->notify)||
       !guest_vitals_method(base,"BioPlayerController","QueueDamageIndicator",0x20400,8,queue,1,&s->queue)||
       !guest_player_context_method_stamp(base,hostile,&s->hostile)||!runtime_hit_layout(base,scope->objects[1].pointer)||
       !runtime_hit_methods(base,scope->objects[0].pointer)||!runtime_hit_notify_live(base,base+0xf52b0))return 0;
    uintptr_t prop=read_ptr(hostile+0x70);const char *names[]={"Other","ReturnValue"},*kinds[]={"ObjectProperty","BoolProperty"};
    for(unsigned i=0;i<2;++i){char name[96];int32_t offset=-1,size=0,dim=0;uint64_t flags=0,mask=0;object_name(base,prop,name,sizeof name);
        if(!is_live(base,prop)||read_ptr(prop+0x40)!=hostile||strcmp(name,names[i])||!narrative_probe_object(base,prop,kinds[i])||
           !read_at(prop+0x94,&offset,4)||offset!=(i?8:0)||!read_at(prop+0x74,&size,4)||size!=(i?4:8)||
           !read_at(prop+0x70,&dim,4)||dim!=1||!read_at(prop+0x78,&flags,8)||flags!=(i?0x580:0x80)||
           (i&&(!read_at(prop+0xd0,&mask,8)||mask!=1)))return 0;prop=read_ptr(prop+0x68);}
    return !prop;
}
static int runtime_hit_pending_current(const RuntimeHitPending *p,const RuntimeHitScope *scope,DWORD now) {
    return now-p->received<=500&&runtime_hit_scope_same(&p->scope,scope);
}
static void runtime_guest_hit_feedback_tick(uintptr_t base,DWORD now) {
    DWORD error=GetLastError();RuntimeHitFeedback *s=&runtime_hit_feedback;RuntimeHitScope scope={0};RuntimeHitImpact impact={0};
    if(GetCurrentThreadId()!=session_game_thread||g_runtime_config.role!=ME3_ROLE_GUEST||s->busy)goto done;
    if(!runtime_hit_scope(base,&scope)||!runtime_hit_read(scope.objects[1].pointer,&impact)) {s->baselined=0;s->count=0;goto done;}
    if(!s->baselined||!runtime_hit_scope_same(&s->baseline,&scope))runtime_hit_fresh(s,&scope,impact.trigger);
    if(!s->installed) {
        if(s->checked&&now-s->checked<1000)goto done;s->checked=now;s->base=base;
        if(!runtime_rpc_receive_installed||!supported_image(base)||!guest_equipment_dispatch_pins(base)||!runtime_hit_resolve(base,&scope)||
           !background_focus_writable(s->notify.object.pointer+0xd8,8))goto done;
        s->original=(RuntimeHitExec)(base+0xf52b0);
        if(InterlockedCompareExchangePointer((PVOID volatile*)(s->notify.object.pointer+0xd8),(PVOID)runtime_hit_exec,(PVOID)s->original)!=(PVOID)s->original)goto done;
        s->installed=1;line("{\"guest_hit_feedback\":\"stock-direction-queue-bridge-installed\",\"cosmetic_only\":true,\"gameplay_verified\":false}\r\n");
    }
    /* Consume before native callbacks. Independent attackers in the same frame
     * retain separate entries; repeated impacts from one attacker coalesce. */
    RuntimeHitPending pending[8];unsigned count=s->count;memcpy(pending,s->pending,sizeof pending);s->count=0;
    for(unsigned i=0;i<count;++i){RuntimeHitPending *p=&pending[i],source={0};RuntimeHitScope current={0};
        if(!runtime_hit_notify_live(base,(uintptr_t)runtime_hit_exec)||!runtime_hit_scope(base,&current)||
           !runtime_hit_pending_current(p,&current,now)||!runtime_hit_methods(base,current.objects[0].pointer)||
           !guest_camera_stamp_matches(base,&p->weapon,1)||!guest_camera_stamp_matches(base,&p->attacker,1)||
           !runtime_hit_source(base,&current,p->weapon.pointer,&source)||
           source.attacker.pointer!=p->attacker.pointer||source.attacker.index!=p->attacker.index)continue;
        RuntimeHitHostileParms hostile={current.objects[1].pointer,0};s->busy=1;SetLastError(error);
        ((ProcessEvent)(base+0x40d450))((void*)p->attacker.pointer,(void*)s->hostile.function.pointer,&hostile,NULL);s->busy=0;
        if(!(hostile.result&1)||!runtime_hit_scope(base,&current)||!runtime_hit_pending_current(p,&current,GetTickCount())||
           !runtime_hit_methods(base,current.objects[0].pointer)||!guest_camera_stamp_matches(base,&p->weapon,1)||
           !guest_camera_stamp_matches(base,&p->attacker,1)||!runtime_hit_source(base,&current,p->weapon.pointer,&source)||
           source.attacker.pointer!=p->attacker.pointer||source.attacker.index!=p->attacker.index)continue;
        uintptr_t attacker=p->attacker.pointer;s->busy=1;SetLastError(error);
        ((ProcessEvent)(base+0x40d450))((void*)current.objects[0].pointer,(void*)s->queue.object.pointer,&attacker,NULL);s->busy=0;
        if(s->rows++<32){char row[360];snprintf(row,sizeof row,"{\"guest_hit_feedback\":\"stock-direction-queued\",\"attacker_index\":%d,\"trigger\":%u,\"channel_index\":%d,\"scope_token\":%lu,\"damage_replayed\":false,\"gameplay_verified\":false,\"monotonic_ms\":%lu}\r\n",p->attacker.index,(unsigned)p->trigger,current.channel_index,current.token,GetTickCount());line(row);}
    }
done:SetLastError(error);
}
#ifdef ME3_RUNTIME_FIXTURE
static int runtime_hit_fixture_order,runtime_hit_fixture_bad,runtime_hit_fixture_valid;
static void *runtime_hit_fixture_args[3];
static RuntimeHitCapture runtime_hit_fixture_capture(void *self,void *frame) {
    RuntimeHitCapture c={0};c.valid=runtime_hit_fixture_valid;if(runtime_hit_fixture_order++||self!=runtime_hit_fixture_args[0]||frame!=runtime_hit_fixture_args[1])runtime_hit_fixture_bad=1;SetLastError(11);return c;
}
static void runtime_hit_fixture_stock(void *self,void *frame,void *result) {
    if(runtime_hit_fixture_order++!=1||GetLastError()!=37||self!=runtime_hit_fixture_args[0]||frame!=runtime_hit_fixture_args[1]||result!=runtime_hit_fixture_args[2])runtime_hit_fixture_bad=1;SetLastError(53);
}
static void runtime_hit_fixture_after(RuntimeHitCapture *c) {
    if(runtime_hit_fixture_order++!=2||GetLastError()!=53||c->valid!=runtime_hit_fixture_valid)runtime_hit_fixture_bad=1;SetLastError(79);
}
static int runtime_guest_hit_feedback_selftest(void) {
    DWORD error=GetLastError();RuntimeHitFeedback s={0};RuntimeHitScope scope={0};RuntimeHitPending p={0};int a,b,c,ok=1;
    scope.token=7;scope.channel_index=2;scope.objects[0].pointer=1;
    ok=ok&&!runtime_hit_fresh(&s,&scope,255)&&!runtime_hit_fresh(&s,&scope,255)&&runtime_hit_fresh(&s,&scope,0)&&!runtime_hit_fresh(&s,&scope,0);
    p.scope=scope;p.received=0xfffffff0;p.attacker.pointer=12;p.attacker.index=3;
    ok=ok&&runtime_hit_pending_current(&p,&scope,20)&&!runtime_hit_pending_current(&p,&scope,501);
    runtime_hit_enqueue(&s,&p);p.trigger=9;runtime_hit_enqueue(&s,&p);ok=ok&&s.count==1&&s.pending[0].trigger==9;
    for(unsigned i=1;i<10;++i){p.attacker.pointer=12+i;runtime_hit_enqueue(&s,&p);}ok=ok&&s.count==8;
    scope.token=8;ok=ok&&!runtime_hit_pending_current(&p,&scope,20)&&!runtime_hit_fresh(&s,&scope,7)&&!s.count;
    scope.objects[6].pointer=99;ok=ok&&!runtime_hit_fresh(&s,&scope,8);
    scope.objects[2].index=123;ok=ok&&!runtime_hit_fresh(&s,&scope,9);scope.channel_index=3;ok=ok&&!runtime_hit_fresh(&s,&scope,10);
    RuntimeHitImpact x={1,2,3,4,5,{0}},y=x;y.padding[0]=255;ok=ok&&runtime_hit_impact_same(&x,&y);y.trigger=6;ok=ok&&!runtime_hit_impact_same(&x,&y);
    runtime_hit_fixture_args[0]=&a;runtime_hit_fixture_args[1]=&b;runtime_hit_fixture_args[2]=&c;
    for(int valid=0;valid<2;++valid){runtime_hit_fixture_order=runtime_hit_fixture_bad=0;runtime_hit_fixture_valid=valid;
        SetLastError(37);runtime_hit_forward(&a,&b,&c,runtime_hit_fixture_stock,runtime_hit_fixture_capture,runtime_hit_fixture_after);
        ok=ok&&runtime_hit_fixture_order==3&&!runtime_hit_fixture_bad&&GetLastError()==53;}
    SetLastError(error);return ok;
}
#endif
#endif
