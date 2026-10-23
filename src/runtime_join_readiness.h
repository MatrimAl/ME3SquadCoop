#ifndef ME3_RUNTIME_JOIN_READINESS_H
#define ME3_RUNTIME_JOIN_READINESS_H
/* Admission policy, not an inferred native minimum: observe a stable ordinary
 * player context before consuming the irreversible one-shot console request. */
typedef struct RuntimeJoinSample {
    SessionHandle objects[6]; /* World, WorldInfo, PC, pawn, manager, squad */
    float world_time;
} RuntimeJoinSample;
typedef struct RuntimeJoinReadiness {
    RuntimeJoinSample first;
    DWORD first_at,last_at;
    float last_world_time;
    unsigned samples;
} RuntimeJoinReadiness;
static RuntimeJoinReadiness runtime_join_readiness;
static int runtime_join_sample_same(const RuntimeJoinSample *a,const RuntimeJoinSample *b) {
    for(unsigned i=0;i<6;++i) {
        SessionHandle x=a->objects[i],y=b->objects[i];
        if(!x.pointer||x.pointer!=y.pointer||x.index!=y.index||x.name!=y.name)return 0;
    }
    return 1;
}
static int runtime_join_readiness_observe(RuntimeJoinReadiness *state,
    const RuntimeJoinSample *sample,DWORD now) {
    if(!sample||!isfinite(sample->world_time)||sample->world_time<0||
       !runtime_join_sample_same(sample,sample)) {
        memset(state,0,sizeof *state);return 0;
    }
    if(!state->samples||!runtime_join_sample_same(&state->first,sample)||
       now-state->last_at>375||sample->world_time<state->last_world_time) {
        memset(state,0,sizeof *state);state->first=*sample;state->first_at=now;
    }
    state->last_at=now;state->last_world_time=sample->world_time;
    if(state->samples<5)++state->samples;
    return state->samples>=5&&now-state->first_at>=1000&&
        sample->world_time-state->first.world_time>=0.5f;
}
static int runtime_join_context_sample(uintptr_t base,SessionContext c,
    const CombatContextChain *chain,RuntimeJoinSample *sample) {
    if(!chain->info_valid||c.world!=chain->info||!runtime_host_story_idle(base,c)||
       read_ptr(c.pc+0x3bc)!=c.pawn||read_ptr(c.pawn+0x198)!=c.world||
       !is_live(base,c.manager)||!is_kind_of(base,c.manager,"SFXGameModeManager")||
       !is_live(base,c.squad)||!is_kind_of(base,c.squad,"BioBaseSquad")||
       read_ptr(c.squad+0x2cc)!=c.pawn)return 0;
    uintptr_t pointers[]={chain->world,c.world,c.pc,c.pawn,c.manager,c.squad};
    memset(sample,0,sizeof *sample);
    for(unsigned i=0;i<6;++i) {
        sample->objects[i]=session_capture_handle(pointers[i]);
        if(!session_handle_matches(sample->objects[i]))return 0;
    }
    return read_at(c.world+0x74c,&sample->world_time,4)&&isfinite(sample->world_time)&&sample->world_time>=0;
}
static int runtime_join_readiness_selftest(void) {
    RuntimeJoinReadiness state={0};RuntimeJoinSample sample={0};
    for(unsigned i=0;i<6;++i)sample.objects[i]=(SessionHandle){0x1000+i*0x100,(int32_t)i,10+i};
    /* A transient initial-world clear frame must never consume the request. */
    sample.world_time=0.43f;if(runtime_join_readiness_observe(&state,&sample,1000))return 0;
    if(runtime_join_readiness_observe(&state,NULL,1250)||state.samples)return 0;
    for(unsigned i=0;i<5;++i) {
        sample.world_time=10+i*0.25f;
        if(runtime_join_readiness_observe(&state,&sample,2000+i*250)!=(i==4))return 0;
    }
    /* Same pointer with reused identity, a skipped observation, or clock
     * regression must restart the observation window. */
    ++sample.objects[3].name;sample.world_time=11.25f;
    if(runtime_join_readiness_observe(&state,&sample,3250)||state.samples!=1)return 0;
    sample.world_time=12;
    if(runtime_join_readiness_observe(&state,&sample,4000)||state.samples!=1)return 0;
    sample.world_time=1;
    if(runtime_join_readiness_observe(&state,&sample,4250)||state.samples!=1)return 0;
    /* Wall time alone, including a paused world, is insufficient. */
    memset(&state,0,sizeof state);
    for(unsigned i=0;i<8;++i)if(runtime_join_readiness_observe(&state,&sample,5000+i*250))return 0;
    sample.world_time=NAN;if(runtime_join_readiness_observe(&state,&sample,7000)||state.samples)return 0;
    sample.world_time=1;sample.objects[0].pointer=0;
    if(runtime_join_readiness_observe(&state,&sample,7250)||state.samples)return 0;
    return 1;
}
#endif
