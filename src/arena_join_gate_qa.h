#ifndef ME3_ARENA_JOIN_GATE_QA_H
#define ME3_ARENA_JOIN_GATE_QA_H
/* Protected parallel arena QA only. The coordinator releases this per-run
 * event after both retained children report full prewarm readiness. Polling
 * never blocks Tick; the native scene/roster/ownership gates still apply. */
static int arena_parallel_qa;
static wchar_t arena_join_event_name[96];
static int arena_join_nonce_valid(const wchar_t *value,DWORD length) {
    if(!value||length!=32)return 0;
    for(DWORD i=0;i<length;++i)
        if(!((value[i]>=L'0'&&value[i]<=L'9')||(value[i]>=L'a'&&value[i]<=L'f')))return 0;
    return value[length]==0;
}
static int arena_join_gate_configure(int arena,int automatic) {
    wchar_t flag[8]={0},nonce[40]={0};
    DWORD length=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_ARENA_PARALLEL_QA",flag,8);
    if(!length)return 1;
    if(length==1&&flag[0]==L'0')return 1;
    if(length!=1||flag[0]!=L'1'||!arena||!automatic)return 0;
    length=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_ARENA_JOIN_GATE",nonce,40);
    if(!arena_join_nonce_valid(nonce,length))return 0;
    int n=_snwprintf(arena_join_event_name,96,L"Local\\ME3SquadCoopArenaReady_%ls",nonce);
    if(n<=0||n>=96)return 0;
    arena_parallel_qa=1;return 1;
}
static int arena_join_gate_released(void) {
    if(!arena_parallel_qa)return 1;
    /* Open with wait permission only. This process never creates or signals
     * the coordinator event, and retains no handle between frames. */
    HANDLE event=OpenEventW(SYNCHRONIZE,FALSE,arena_join_event_name);
    if(!event)return 0;
    DWORD result=WaitForSingleObject(event,0);CloseHandle(event);
    return result==WAIT_OBJECT_0;
}
#endif
