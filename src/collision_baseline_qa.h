#ifndef ME3_COLLISION_BASELINE_QA_H
#define ME3_COLLISION_BASELINE_QA_H
/* Protected probe only. Park after the second acquisition; the guest's existing
 * two-second input pulse runs only after reacquisition, with firing/follow off.
 * The existing scene/reacquisition policy remains active.
 * Never a player runtime policy or a movement/combat verdict. */
static int collision_baseline_qa_value(const wchar_t *value,DWORD length) {
    return length==1&&value[0]==L'1';
}
static int collision_baseline_qa_opted(void) {
    static int configured,opted;
    if(!configured) {
        wchar_t value[8]={0};
        DWORD length=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_COLLISION_BASELINE_QA",value,8);
        opted=collision_baseline_qa_value(value,length);configured=1;
    }
    return opted;
}
static int collision_baseline_qa_stop_route(int opted,DWORD epoch) {
    return opted==1&&epoch>=2;
}
static int collision_baseline_qa_selftest(void) {
    if(!collision_baseline_qa_value(L"1",1)||collision_baseline_qa_value(L"",0)||
       collision_baseline_qa_value(L"0",1)||collision_baseline_qa_value(L"11",2)||
       collision_baseline_qa_value(L"true",4)||collision_baseline_qa_value(L"1",9))return 0;
    return !collision_baseline_qa_stop_route(0,2)&&!collision_baseline_qa_stop_route(1,0)&&
        !collision_baseline_qa_stop_route(1,1)&&collision_baseline_qa_stop_route(1,2)&&
        collision_baseline_qa_stop_route(1,UINT32_MAX);
}
#endif
