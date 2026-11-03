#ifndef ME3_RUNTIME_JOIN_OUTCOME_H
#define ME3_RUNTIME_JOIN_OUTCOME_H
/* Observe our dispatched join only. An elapsed window is not a native failure
 * verdict. Never disconnect, clear engine state or queue another request. */
typedef struct RuntimeJoinOutcome {DWORD started_at;int watching;} RuntimeJoinOutcome;
static RuntimeJoinOutcome runtime_join_outcome;
static int runtime_join_outcome_next(RuntimeJoinOutcome *state,int status,int enabled,
    int guest,int native_scope,DWORD now) {
    if(!enabled||!guest||(status!=RUNTIME_START_DISPATCHED&&status!=RUNTIME_START_JOIN_UNCONFIRMED)) {
        state->watching=0;return status;
    }
    if(!state->watching)return status;
    if(native_scope==1){state->watching=0;return RUNTIME_START_ALREADY_NETWORKED;}
    if(now-state->started_at>=60000)return RUNTIME_START_JOIN_UNCONFIRMED;
    return status;
}
static void runtime_join_outcome_tick(int native_scope,DWORD now) {
    LONG before=InterlockedCompareExchange(&runtime_network_start_status,0,0);
    int after=runtime_join_outcome_next(&runtime_join_outcome,(int)before,runtime_control_enabled,
        g_runtime_config.role==ME3_ROLE_GUEST,native_scope,now);
    /* Export callers may publish a new command concurrently. Do not overwrite
     * its status with this observation of the earlier dispatch. */
    if(after!=before&&InterlockedCompareExchange(&runtime_network_start_status,after,before)==before) {
        char row[300];snprintf(row,sizeof row,
            "{\"runtime_network\":\"join-outcome\",\"status\":%d,\"native_client_scope_observed\":%s,\"native_rejection_verified\":false,\"retry_queued\":false,\"monotonic_ms\":%lu}\r\n",
            after,after==RUNTIME_START_ALREADY_NETWORKED?"true":"false",now);line(row);
    }
}
static int runtime_join_outcome_selftest(void) {
    RuntimeJoinOutcome s={1000,1};
    if(runtime_join_outcome_next(&s,RUNTIME_START_DISPATCHED,1,1,0,60999)!=RUNTIME_START_DISPATCHED||
       runtime_join_outcome_next(&s,RUNTIME_START_DISPATCHED,1,1,0,61000)!=RUNTIME_START_JOIN_UNCONFIRMED||!s.watching||
       runtime_join_outcome_next(&s,RUNTIME_START_JOIN_UNCONFIRMED,1,1,1,62000)!=RUNTIME_START_ALREADY_NETWORKED||s.watching)return 0;
    s=(RuntimeJoinOutcome){UINT32_MAX-15,1};
    if(runtime_join_outcome_next(&s,RUNTIME_START_DISPATCHED,1,1,0,59983)!=RUNTIME_START_DISPATCHED||
       runtime_join_outcome_next(&s,RUNTIME_START_DISPATCHED,1,1,0,59984)!=RUNTIME_START_JOIN_UNCONFIRMED)return 0;
    int preserved[]={RUNTIME_START_CANCELLED,RUNTIME_START_RETRY_WAITING,RUNTIME_START_ABI_REFUSED,RUNTIME_START_LISTENING};
    for(unsigned i=0;i<sizeof preserved/sizeof *preserved;++i){
        s=(RuntimeJoinOutcome){1,1};
        if(runtime_join_outcome_next(&s,preserved[i],1,1,1,70000)!=preserved[i]||s.watching)return 0;
    }
    s=(RuntimeJoinOutcome){1,1};
    if(runtime_join_outcome_next(&s,RUNTIME_START_DISPATCHED,0,1,1,70000)!=RUNTIME_START_DISPATCHED||s.watching)return 0;
    s=(RuntimeJoinOutcome){1,1};
    if(runtime_join_outcome_next(&s,RUNTIME_START_DISPATCHED,1,0,1,70000)!=RUNTIME_START_DISPATCHED||s.watching)return 0;
    s=(RuntimeJoinOutcome){1,0};
    return runtime_join_outcome_next(&s,RUNTIME_START_DISPATCHED,1,1,1,70000)==RUNTIME_START_DISPATCHED;
}
#endif
