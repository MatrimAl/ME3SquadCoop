#ifndef ME3_RUNTIME_POWER_START_OBSERVER_H
#define ME3_RUNTIME_POWER_START_OBSERVER_H
/* Evidence at the existing local, server and pawn power-start scripts.
 * Authenticated server ingress carries player-order intent through the existing
 * scoped context. No request is sent, retried, retargeted or replaced. */
typedef struct RuntimePowerStartCapture {
    GuestCameraStamp pc,pawn,power;
    uintptr_t frame,node,locals,channel;
    unsigned serial;int kind,valid,frame_valid,power_id,action,power_action,pending,pending_power,source_matches;
    int table_valid,table_count,table_power_index,table_class_index,table_owner_matches,table_pc_matches;
    unsigned char role,pawn_role;uint32_t power_flags;
    uint32_t order_flags;
    int intent_read,permission_read,henchman,grenade,stock_power_type,actual_class_index;
    float last_can_do,last_finished,min_interval,game_time,rank,cooldown;
    char actual_class_path[260];
} RuntimePowerStartCapture;
typedef struct RuntimePowerStartObserver {
    uintptr_t base;GuestPlayerContextMethod methods[3];DWORD checked;
    unsigned calls,depth;int prepared;
} RuntimePowerStartObserver;
static RuntimePowerStartObserver runtime_power_start_observer;
static const char *runtime_power_start_names[]={"StartPowerCustomAction","ServerStartPowerCustomAction","Pawn.StartPowerCustomAction"};
static int runtime_power_start_observer_node_match(uintptr_t node) {
    RuntimePowerStartObserver *s=&runtime_power_start_observer;
    return s->prepared&&node&&(node==s->methods[0].function.pointer||node==s->methods[1].function.pointer||node==s->methods[2].function.pointer);
}
static int runtime_power_start_observer_method(uintptr_t base,int kind,const GuestPlayerContextMethod *method) {
    GameArray script={0};uintptr_t fn=method->function.pointer;
    static const uint32_t flags[]={0x20002,0x2200c2,0x24102};
    static const unsigned sizes[]={12,48,32},returns[]={8,65535,28},counts[]={2,5,5},vm[]={712,160,209};
    return kind>=0&&kind<3&&guest_player_context_method_matches(base,method,1)&&
        guest_player_context_function_abi(base,fn,kind==2?"BioPawn":"BioPlayerController",kind==2?"StartPowerCustomAction":runtime_power_start_names[kind],
            flags[kind],sizes[kind],returns[kind],0,0xf52b0,counts[kind])&&
        read_at(fn+0x7c,&script,sizeof script)&&script.data&&script.count==(int)vm[kind]&&
        script.max>=script.count&&script.max<=4096;
}
static int runtime_power_start_observer_scope(uintptr_t self,RuntimePowerStartCapture *c) {
    RuntimePowerStartObserver *s=&runtime_power_start_observer;uintptr_t base=s->base;
    memset(&c->pc,0,sizeof c->pc);memset(&c->pawn,0,sizeof c->pawn);c->valid=0;c->source_matches=0;
    if(GetCurrentThreadId()!=session_game_thread||!runtime_control_enabled||!runtime_scope_ready)return 0;
    SessionContext local=session_context(session_engine);
    int selected=g_runtime_config.role==ME3_ROLE_GUEST?self==local.pc:
        g_runtime_config.role==ME3_ROLE_HOST&&session_active&&self==session_remote;
    uintptr_t pawn=read_ptr(self+0x3bc);
    if(!selected||!guest_camera_actor_live(base,self)||!is_kind_of(base,self,"SFXPlayerController")||
       read_ptr(self+0x198)!=local.world||!guest_camera_stamp_read(base,self,&c->pc,0))return 0;
    if(guest_camera_actor_live(base,pawn)&&is_kind_of(base,pawn,"BioPawn")&&read_ptr(pawn+0x198)==local.world)
        guest_camera_stamp_read(base,pawn,&c->pawn,0);
    c->role=255;c->pawn_role=255;read_at(self+0x26a,&c->role,1);
    c->action=c->power_action=c->pending=c->pending_power=-1;
    read_at(self+0xd70,&c->pending,4);read_at(self+0xd74,&c->pending_power,4);
    if(c->pawn.pointer){read_at(pawn+0x26a,&c->pawn_role,1);read_at(pawn+0xc20,&c->action,4);read_at(pawn+0xc28,&c->power_action,4);}
    NativeRpcReceiveFrame *rpc=native_rpc_receive_current(session_game_thread);c->channel=rpc?rpc->channel:0;
    if(c->channel){NetworkActorScope world={0};NetworkActorIdentityEntry source={0};
        c->source_matches=network_actor_read_scope(base,self,&world)&&
            network_actor_channel_valid(base,self,c->channel,world.driver,&source)&&
            source.connection==read_ptr(self+0x67c);}
    c->valid=1;return 1;
}
static void runtime_power_start_observer_table(RuntimePowerStartCapture *c) {
    uintptr_t base=runtime_power_start_observer.base,pawn=c->pawn.pointer;GameArray powers={0},classes={0};
    GuestCameraStamp power={0},klass={0};c->table_valid=0;c->table_count=0;c->table_power_index=c->table_class_index=-1;
    c->table_owner_matches=c->table_pc_matches=0;
    c->intent_read=c->permission_read=c->henchman=c->grenade=c->stock_power_type=0;c->actual_class_index=-1;
    c->actual_class_path[0]=0;
    if(!c->valid||!c->frame_valid||c->power_id<0||!guest_camera_stamp_matches(base,&c->pawn,0)||
       !runtime_power_property(base,pawn,"BioPawn","PowerCustomActions","ArrayProperty",0x6cc,16)||
       !runtime_power_property(base,pawn,"BioPawn","PowerCustomActionClasses","ArrayProperty",0x6bc,16)||
       !scene_readiness_array(pawn+0x6cc,&powers,512)||!scene_readiness_array(pawn+0x6bc,&classes,512))return;
    c->table_valid=1;c->table_count=powers.count;
    if(c->power_id<classes.count){uintptr_t p=read_ptr(classes.data+(SIZE_T)c->power_id*8);
        if(narrative_probe_object(base,p,"Class")&&guest_camera_stamp_read(base,p,&klass,0))c->table_class_index=klass.index;}
    if(c->power_id<powers.count){uintptr_t p=read_ptr(powers.data+(SIZE_T)c->power_id*8);
        if(narrative_probe_object(base,p,"SFXPowerCustomActionBase")&&guest_camera_stamp_read(base,p,&power,0)){
            c->power=power;
            c->table_power_index=power.index;c->table_owner_matches=read_ptr(p+0x98)==pawn;
            c->table_pc_matches=read_ptr(p+0xa0)==c->pc.pointer;read_at(p+0x100,&c->power_flags,4);
            c->henchman=is_kind_of(base,pawn,"SFXPawn_Henchman");
            c->grenade=is_kind_of(base,p,"SFXPowerCustomAction_GrenadeBase");
            c->stock_power_type=is_kind_of(base,p,"SFXPowerCustomAction");
            GuestCameraStamp actual={0};
            if(guest_camera_stamp_read(base,read_ptr(p+0x50),&actual,1)){
                c->actual_class_index=actual.index;snprintf(c->actual_class_path,sizeof c->actual_class_path,"%s",actual.full_name);}
            c->intent_read=runtime_power_property(base,p,"SFXPowerCustomActionBase","m_bPlayerOrderedPowerUse","BoolProperty",0x4dc,4)&&
                read_at(p+0x4dc,&c->order_flags,4);
            uintptr_t info=read_ptr(pawn+0x198);
            c->permission_read=runtime_power_property(base,p,"BioCustomAction","LastCanDoCustomActionTime","FloatProperty",0xdc,4)&&
                runtime_power_property(base,p,"BioCustomAction","LastFinishedTime","FloatProperty",0xe0,4)&&
                runtime_power_property(base,p,"BioCustomAction","MinTimeBetweenActions","FloatProperty",0xe4,4)&&
                runtime_power_property(base,info,"WorldInfo","GameTimeSeconds","FloatProperty",0x750,4)&&
                read_at(p+0xdc,&c->last_can_do,4)&&read_at(p+0xe0,&c->last_finished,4)&&read_at(p+0xe4,&c->min_interval,4)&&
                read_at(info+0x750,&c->game_time,4)&&read_at(p+0x488,&c->rank,4)&&read_at(p+0x498,&c->cooldown,4)&&
                isfinite(c->last_can_do)&&isfinite(c->last_finished)&&isfinite(c->min_interval)&&isfinite(c->game_time)&&
                isfinite(c->rank)&&isfinite(c->cooldown);
        }}
}
static void runtime_power_start_observer_emit(const char *phase,const RuntimePowerStartCapture *c,int result_valid,uint32_t result,int same) {
    char row[2500];snprintf(row,sizeof row,
        "{\"runtime_power_start_observer\":\"boundary\",\"serial\":%u,\"phase\":\"%s\",\"kind\":\"%s\",\"runtime_role\":%d,\"pc_index\":%d,\"pawn_index\":%d,\"pc_role\":%u,\"pawn_role\":%u,\"scope_valid\":%s,\"frame_valid\":%s,\"same_identity\":%s,\"power_id\":%d,\"power_flags\":%u,\"table_valid\":%s,\"table_count\":%d,\"table_power_index\":%d,\"table_class_index\":%d,\"table_owner_matches\":%s,\"table_pc_matches\":%s,\"action\":%d,\"power_action\":%d,\"pending\":%d,\"pending_power\":%d,\"received_bunch_channel\":\"%p\",\"received_bunch_actor_matches\":%s,\"return_readable\":%s,\"return_value\":%u,\"read_only\":true,\"power_dispatched\":false,\"monotonic_ms\":%lu}\r\n",
        c->serial,phase,runtime_power_start_names[c->kind],(int)g_runtime_config.role,c->pc.index,c->pawn.index,
        (unsigned)c->role,(unsigned)c->pawn_role,c->valid?"true":"false",c->frame_valid?"true":"false",same?"true":"false",c->power_id,c->power_flags,
        c->table_valid?"true":"false",c->table_count,c->table_power_index,c->table_class_index,
        c->table_owner_matches?"true":"false",c->table_pc_matches?"true":"false",
        c->action,c->power_action,c->pending,c->pending_power,(void*)c->channel,c->source_matches?"true":"false",result_valid?"true":"false",result,GetTickCount());line(row);
    snprintf(row,sizeof row,
        "{\"runtime_power_start_observer\":\"native-permission-context\",\"serial\":%u,\"phase\":\"%s\",\"kind\":\"%s\",\"actual_class_index\":%d,\"actual_class_path\":\"%s\",\"henchman\":%s,\"grenade\":%s,\"stock_power_type\":%s,\"intent_read\":%s,\"order_flags\":%u,\"player_ordered\":%s,\"permission_read\":%s,\"cached_permission\":%s,\"last_can_do\":%.9g,\"last_finished\":%.9g,\"min_interval\":%.9g,\"game_time\":%.9g,\"rank\":%.9g,\"cooldown\":%.9g,\"read_only\":true,\"monotonic_ms\":%lu}\r\n",
        c->serial,phase,runtime_power_start_names[c->kind],c->actual_class_index,c->actual_class_path,
        c->henchman?"true":"false",c->grenade?"true":"false",c->stock_power_type?"true":"false",c->intent_read?"true":"false",c->order_flags,
        c->intent_read&&(c->order_flags&8)?"true":"false",c->permission_read?"true":"false",(c->power_flags&0x400000)?"true":"false",
        c->permission_read?(double)c->last_can_do:0,c->permission_read?(double)c->last_finished:0,c->permission_read?(double)c->min_interval:0,
        c->permission_read?(double)c->game_time:0,c->permission_read?(double)c->rank:0,c->permission_read?(double)c->cooldown:0,GetTickCount());line(row);
}
static int runtime_power_start_observer_capture(void *self,void *frame,RuntimePowerStartCapture *c) {
    RuntimePowerStartObserver *s=&runtime_power_start_observer;uintptr_t node=read_ptr((uintptr_t)frame+0x18);GameArray script={0};
    if(!runtime_power_start_observer_node_match(node))return 0;
    c->kind=node==s->methods[2].function.pointer?2:node==s->methods[1].function.pointer?1:0;
    uintptr_t pc=c->kind==2?read_ptr((uintptr_t)self+0x374):(uintptr_t)self;
    if(!runtime_power_start_observer_scope(pc,c)||(c->kind==2&&c->pawn.pointer!=(uintptr_t)self))return 0;
    c->frame=(uintptr_t)frame;c->node=node;c->power_id=-1;
    if(runtime_power_start_observer_method(s->base,c->kind,&s->methods[c->kind])&&
       read_ptr(c->frame+0x20)==(uintptr_t)self&&read_at(node+0x7c,&script,sizeof script)&&
       read_ptr(c->frame+0x28)==script.data&&(c->locals=read_ptr(c->frame+0x30))){
        c->frame_valid=1;
        if(c->kind)read_at(c->locals,&c->power_id,4);
        else{uintptr_t power=read_ptr(c->locals);
            if(narrative_probe_object(s->base,power,"SFXPowerCustomActionBase")&&
               guest_camera_stamp_read(s->base,power,&c->power,0)){
                read_at(power+0x484,&c->power_id,4);read_at(power+0x100,&c->power_flags,4);}}
    }
    runtime_power_start_observer_table(c);return 1;
}
static int runtime_power_start_observer_before(void *self,void *frame,RuntimePowerStartCapture *c) {
    if(!runtime_power_start_observer_capture(self,frame,c))return 0;
    runtime_power_start_observer_emit("before",c,0,0,1);return 1;
}
static void runtime_power_start_observer_after(void *self,void *frame,void *result,const RuntimePowerStartCapture *before) {
    (void)frame;RuntimePowerStartCapture after=*before;
    uintptr_t pc=before->kind==2?read_ptr((uintptr_t)self+0x374):(uintptr_t)self;
    int valid=runtime_power_start_observer_scope(pc,&after);
    int same=valid&&guest_camera_stamp_matches(runtime_power_start_observer.base,&before->pc,0)&&
        (!before->pawn.pointer||guest_camera_stamp_matches(runtime_power_start_observer.base,&before->pawn,0))&&
        after.pc.pointer==before->pc.pointer&&after.pawn.pointer==before->pawn.pointer;
    uint32_t returned=0;int readable=before->kind!=1&&read_at((uintptr_t)result,&returned,4);
    runtime_power_start_observer_table(&after);runtime_power_start_observer_emit("after",&after,readable,returned,same);
}
/* The stock executor always runs once, including refusal and quota exhaustion.
 * Preserve its exact arguments and resulting LastError across both observers. */
static void runtime_power_start_observer_observe(void *self,void *frame,void *result,RuntimeAmmoPickupExec stock,
        RuntimePowerStartObserver *s,int enabled,int (*before)(void*,void*,RuntimePowerStartCapture*),
        void (*after)(void*,void*,void*,const RuntimePowerStartCapture*)) {
    DWORD error=GetLastError();unsigned depth=s->depth;
    RuntimePowerStartCapture c={0};int observe=enabled&&depth<4&&s->calls<64;
    if(observe){s->depth=depth+1;c.serial=s->calls+1;observe=before(self,frame,&c);
        if(observe)++s->calls;else s->depth=depth;}
    SetLastError(error);stock(self,frame,result);error=GetLastError();
    if(observe){after(self,frame,result,&c);s->depth=depth;}SetLastError(error);
}
static void runtime_power_start_observer_forward(void *self,void *frame,void *result,RuntimeAmmoPickupExec stock) {
    RuntimePowerStartObserver *s=&runtime_power_start_observer;DWORD error=GetLastError();
    /* Carry only the authenticated remote human's order intent into the
     * existing host power context. Stock receives the original request once;
     * this is independent of the bounded diagnostic quota. */
    if(g_runtime_config.role==ME3_ROLE_HOST&&s->prepared&&
       read_ptr((uintptr_t)frame+0x18)==s->methods[1].function.pointer){
        RuntimePowerStartCapture c={0};
        if(runtime_power_start_observer_capture(self,frame,&c)&&c.kind==1&&c.valid&&c.frame_valid&&c.source_matches&&
           c.table_valid&&c.table_owner_matches&&c.table_pc_matches&&c.power.pointer)
            runtime_power_action_context_player_order(s->base,c.pc.pointer,c.pawn.pointer,c.power.pointer);
    }
    SetLastError(error);
    runtime_power_start_observer_observe(self,frame,result,stock,s,s->prepared&&GetCurrentThreadId()==session_game_thread,
        runtime_power_start_observer_before,runtime_power_start_observer_after);
}
static void runtime_power_start_observer_prepare(uintptr_t base,DWORD now) {
    RuntimePowerStartObserver *s=&runtime_power_start_observer;DWORD error=GetLastError();
    if(s->prepared||!runtime_control_enabled||!runtime_scope_ready||GetCurrentThreadId()!=session_game_thread)goto done;
    if(s->checked&&now-s->checked<1000)goto done;s->checked=now;s->base=base;
    for(int i=0;i<3;++i){uintptr_t fn=function_named(base,i==2?"BioPawn":"BioPlayerController",i==2?"StartPowerCustomAction":runtime_power_start_names[i]);
        if(!guest_player_context_method_stamp(base,fn,&s->methods[i])||!runtime_power_start_observer_method(base,i,&s->methods[i]))goto done;}
    s->prepared=1;line("{\"runtime_power_start_observer\":\"native-script-boundaries-prepared\",\"pawn_start_result_boundary\":true,\"authenticated_player_order_context\":true,\"power_dispatched\":false}\r\n");
 done:SetLastError(error);
}
#ifdef ME3_RUNTIME_FIXTURE
static unsigned runtime_power_start_fixture_calls,runtime_power_start_fixture_before,runtime_power_start_fixture_after;
static int runtime_power_start_fixture_bad,runtime_power_start_fixture_accept;
static void runtime_power_start_fixture_stock(void *a,void *b,void *c){
    if(a!=(void*)16||b!=(void*)24||c!=(void*)32||GetLastError()!=37)runtime_power_start_fixture_bad=1;
    ++runtime_power_start_fixture_calls;SetLastError(53);
}
static int runtime_power_start_fixture_before_call(void *a,void *b,RuntimePowerStartCapture *c){
    if(a!=(void*)16||b!=(void*)24||GetLastError()!=37)runtime_power_start_fixture_bad=1;
    if(c->serial!=1)runtime_power_start_fixture_bad=1;
    ++runtime_power_start_fixture_before;SetLastError(11);return runtime_power_start_fixture_accept;
}
static void runtime_power_start_fixture_after_call(void *a,void *b,void *c,const RuntimePowerStartCapture *capture){
    if(a!=(void*)16||b!=(void*)24||c!=(void*)32||GetLastError()!=53)runtime_power_start_fixture_bad=1;
    if(capture->serial!=1)runtime_power_start_fixture_bad=1;
    ++runtime_power_start_fixture_after;SetLastError(71);
}
static int runtime_power_start_observer_selftest(void) {
    DWORD error=GetLastError();int ok=1;
    for(int mode=0;mode<5;++mode){RuntimePowerStartObserver s={0};int enabled=mode!=0;
        if(mode==3)s.calls=64;if(mode==4)s.depth=4;
        runtime_power_start_fixture_accept=mode!=2;
        runtime_power_start_fixture_calls=runtime_power_start_fixture_before=runtime_power_start_fixture_after=0;runtime_power_start_fixture_bad=0;
        SetLastError(37);runtime_power_start_observer_observe((void*)16,(void*)24,(void*)32,runtime_power_start_fixture_stock,&s,enabled,
            runtime_power_start_fixture_before_call,runtime_power_start_fixture_after_call);
        unsigned attempted=mode==1||mode==2,accepted=mode==1;
        ok=ok&&!runtime_power_start_fixture_bad&&runtime_power_start_fixture_calls==1&&runtime_power_start_fixture_before==attempted&&
            runtime_power_start_fixture_after==accepted&&GetLastError()==53&&s.depth==(mode==4?4u:0u)&&
            s.calls==(mode==3?64u:accepted);
    }
    SetLastError(error);return ok;
}
#endif
#endif
