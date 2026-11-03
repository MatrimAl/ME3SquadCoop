#ifndef ME3_RUNTIME_STORY_PASS_H
#define ME3_RUNTIME_STORY_PASS_H
/* The movement fence is an approach rule, not a permanent wall. Remember
 * actual Shepard passage through an authored brush, never a guessed map side.
 * This does not admit a guest's native story event or modify the graph. */
typedef struct RuntimeStoryPass {
    RuntimeStoryEvent entries[128];unsigned count;
    SessionHandle shepard,world,root,driver;
    float previous[3];DWORD sampled;int previous_valid;
} RuntimeStoryPass;
static RuntimeStoryPass runtime_story_pass;
static int runtime_story_same_handle(SessionHandle a,SessionHandle b) {
    return a.pointer==b.pointer&&a.index==b.index&&a.name==b.name;
}
static int runtime_story_same_event(const RuntimeStoryEvent *a,const RuntimeStoryEvent *b) {
    return a->kind==b->kind&&runtime_story_same_handle(a->event,b->event)&&
        runtime_story_same_handle(a->volume,b->volume)&&runtime_story_same_handle(a->component,b->component)&&
        runtime_story_same_handle(a->world,b->world);
}
static int runtime_story_pass_has(const RuntimeStoryPass *p,const RuntimeStoryEvent *event) {
    for(unsigned i=0;i<p->count;++i)if(runtime_story_same_event(p->entries+i,event))return 1;
    return 0;
}
static int runtime_story_pass_segment(const float a[3],const float b[3],DWORD age) {
    if(age>250)return 0;
    double distance=0;
    for(unsigned i=0;i<3;++i){if(!isfinite(a[i])||!isfinite(b[i]))return 0;double d=(double)b[i]-a[i];distance+=d*d;}
    /* Missing samples and native teleports cannot establish a walked route. */
    return distance<=512.0*512.0;
}
static int runtime_story_pass_scope_live(uintptr_t base,SessionContext c) {
    RuntimeStoryPass *p=&runtime_story_pass;
    return p->world.pointer==c.world&&session_handle_matches(p->world)&&session_handle_matches(p->shepard)&&
        session_handle_matches(p->root)&&session_handle_matches(p->driver)&&
        read_ptr(base+0x18b6638)==p->root.pointer&&read_ptr(p->root.pointer+0x120)==p->driver.pointer&&
        session_actor_live(base,p->shepard.pointer)&&is_kind_of(base,p->shepard.pointer,"SFXPawn_Player")&&
        read_ptr(p->shepard.pointer+0x198)==c.world;
}
static int runtime_story_passed(uintptr_t base,SessionContext c,const RuntimeStoryEvent *event) {
    return runtime_story_pass_scope_live(base,c)&&runtime_story_pass_has(&runtime_story_pass,event);
}
static void runtime_story_pass_observe(uintptr_t base,SessionContext c,DWORD now) {
    RuntimeStoryPass *p=&runtime_story_pass;uintptr_t shepard=0;float position[3];
    if(g_runtime_config.role==ME3_ROLE_HOST&&session_actor_live(base,c.pawn)&&is_kind_of(base,c.pawn,"SFXPawn_Player")&&
       read_ptr(c.pc+0x3bc)==c.pawn&&read_ptr(c.pawn+0x374)==c.pc)shepard=c.pawn;
    else if(g_runtime_config.role==ME3_ROLE_GUEST&&presentation_guest_valid(base))shepard=presentation_guest.objects[2].pointer;
    if(!shepard||session_engine_blocked(c)){p->previous_valid=0;return;}
    NetworkActorScope scope;
    if(!session_actor_live(base,shepard)||!is_kind_of(base,shepard,"SFXPawn_Player")||
       read_ptr(shepard+0x198)!=c.world||!network_actor_read_scope(base,shepard,&scope)||scope.info!=c.world||
       !read_at(shepard+0x108,position,sizeof position)){p->previous_valid=0;return;}
    for(unsigned i=0;i<3;++i)if(!isfinite(position[i])){p->previous_valid=0;return;}
    SessionHandle actor=session_capture_handle(shepard),world=session_capture_handle(c.world);
    SessionHandle root=session_capture_handle(scope.world),driver=session_capture_handle(scope.driver);
    if(!session_handle_matches(actor)||!session_handle_matches(world)||!session_handle_matches(root)||!session_handle_matches(driver))return;
    if(!runtime_story_same_handle(p->shepard,actor)||!runtime_story_same_handle(p->world,world)||
       !runtime_story_same_handle(p->root,root)||!runtime_story_same_handle(p->driver,driver)) {
        memset(p,0,sizeof *p);p->shepard=actor;p->world=world;p->root=root;p->driver=driver;
    }
    int segment=p->previous_valid&&runtime_story_pass_segment(p->previous,position,now-p->sampled);
    /* The same lifetime may disappear briefly from discovery during a scene.
     * Retain its mark; discard replaced event/brush instances instead. */
    for(unsigned i=0;i<p->count;) {
        if(!runtime_story_event_live(base,p->entries+i,c))p->entries[i]=p->entries[--p->count];else ++i;
    }
    RuntimeStoryGate *g=&runtime_story_gate;float zero[3]={0,0,0};int busy=g->busy;g->busy=1;
    for(unsigned i=0;i<g->count&&p->count<128;++i) {
        RuntimeStoryEvent *event=g->entries+i;float bounds[7];RuntimeStoryBox box;float fraction=0;
        if(event->kind!=RUNTIME_STORY_GATE_SHEPARD||runtime_story_pass_has(p,event)||
           !read_at(event->component.pointer+0x8c,bounds,sizeof bounds))continue;
        for(unsigned n=0;n<3;++n){box.minimum[n]=bounds[n]-bounds[n+3];box.maximum[n]=bounds[n]+bounds[n+3];}
        if(runtime_story_capsule_box(&box,position,0,0)!=1&&
           (!segment||runtime_story_segment_box_entry(&box,p->previous,position,0,0,&fraction)!=1))continue;
        if(!runtime_story_event_live(base,event,c)||!narrative_probe_object(base,event->volume.pointer,"Volume"))continue;
        int crossed=runtime_story_sweep(base,event->volume.pointer,position,position,zero)==1;
        if(!crossed&&segment)crossed=runtime_story_sweep(base,event->volume.pointer,p->previous,position,zero)==1;
        if(!crossed||!runtime_story_event_live(base,event,c)||!runtime_story_pass_scope_live(base,c))continue;
        p->entries[p->count++]=*event;
        char row[270];snprintf(row,sizeof row,"{\"runtime_story_gate\":\"shepard-passage-movement-open\",\"kind\":1,\"guest_activation_allowed\":false,\"position_written\":false,\"monotonic_ms\":%lu}\r\n",now);line(row);
    }
    g->busy=busy;memcpy(p->previous,position,sizeof position);p->sampled=now;p->previous_valid=1;
}
#ifdef ME3_RUNTIME_FIXTURE
static int runtime_story_pass_selftest(void) {
    RuntimeStoryPass p={0};RuntimeStoryEvent e={0};e.kind=RUNTIME_STORY_GATE_SHEPARD;
    e.event=(SessionHandle){11,1,21};e.volume=(SessionHandle){12,2,22};
    e.component=(SessionHandle){13,3,23};e.world=(SessionHandle){14,4,24};
    if(runtime_story_pass_has(&p,&e))return 0;p.entries[p.count++]=e;
    if(!runtime_story_pass_has(&p,&e))return 0;
    for(unsigned i=0;i<4;++i)for(unsigned field=0;field<3;++field) {
        RuntimeStoryEvent changed=e;SessionHandle *identities[]={&changed.event,&changed.volume,&changed.component,&changed.world};
        if(field==0)++identities[i]->pointer;else if(field==1)++identities[i]->index;else ++identities[i]->name;
        if(runtime_story_pass_has(&p,&changed))return 0;
    }
    RuntimeStoryEvent group=e;group.kind=RUNTIME_STORY_GATE_GROUP_EXIT;
    if(runtime_story_pass_has(&p,&group))return 0;
    float a[3]={1,2,3},b[3]={20,30,40};
    if(!runtime_story_pass_segment(a,b,250)||runtime_story_pass_segment(a,b,251))return 0;
    b[0]=1000;if(runtime_story_pass_segment(a,b,1))return 0;
    b[0]=NAN;return !runtime_story_pass_segment(a,b,1);
}
#endif
#endif
