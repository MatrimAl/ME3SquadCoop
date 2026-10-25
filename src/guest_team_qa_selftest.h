#ifndef ME3_GUEST_TEAM_QA_SELFTEST_H
#define ME3_GUEST_TEAM_QA_SELFTEST_H
static int guest_team_fixture_calls,guest_team_fixture_bad,guest_team_fixture_return,guest_team_fixture_index;
static void guest_team_fixture_forward(void *self,void *fn,void *params,void *result) {
    GuestTeamParms *p=params;++guest_team_fixture_calls;
    if((uintptr_t)self!=10||(uintptr_t)fn!=20||result||p->other!=30||p->team!=guest_team_fixture_index||p->new_team||p->result)guest_team_fixture_bad=1;
    p->result=(uint32_t)guest_team_fixture_return;
}
static int guest_team_qa_selftest(void) {
    guest_team_fixture_calls=guest_team_fixture_bad=0;guest_team_fixture_return=1;guest_team_fixture_index=0;
    if(!guest_team_dispatch(guest_team_fixture_forward,10,20,30,0)||guest_team_fixture_calls!=1||guest_team_fixture_bad)return 0;
    guest_team_fixture_index=255;if(!guest_team_dispatch(guest_team_fixture_forward,10,20,30,255)||guest_team_fixture_calls!=2)return 0;
    guest_team_fixture_return=0;if(guest_team_dispatch(guest_team_fixture_forward,10,20,30,255)||guest_team_fixture_calls!=3)return 0;
    guest_team_fixture_return=2;if(guest_team_dispatch(guest_team_fixture_forward,10,20,30,255))return 0;
    int calls=guest_team_fixture_calls;
    if(guest_team_dispatch(guest_team_fixture_forward,10,20,30,4)||guest_team_dispatch(guest_team_fixture_forward,10,20,30,-1)||
       guest_team_dispatch(guest_team_fixture_forward,10,20,0,0)||guest_team_dispatch(NULL,10,20,30,0)||guest_team_fixture_calls!=calls)return 0;
    if(!guest_team_restore_eligible(1,1,1,1,1,1))return 0;
    for(int i=0;i<6;++i){int v[]={1,1,1,1,1,1};v[i]=0;if(guest_team_restore_eligible(v[0],v[1],v[2],v[3],v[4],v[5]))return 0;}
    return !guest_team_fixture_bad;
}
#endif
