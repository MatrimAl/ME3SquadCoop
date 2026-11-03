#ifndef ME3_ARENA_PAIR_QA_H
#define ME3_ARENA_PAIR_QA_H
/* Protected manual SPCer pair only. Native session/scene/equipment guards
 * remain authoritative; no route, fire, save or synthetic-scene automation. */
#include "arena_join_gate_qa.h"
static int arena_pair_qa,arena_pair_auto_qa;
static int arena_auto_movement_done,arena_auto_input_fault;
static DWORD arena_auto_ready_at;
static int arena_pair_policy(int opted,int guards,int action,int progress,int session,
                             int manual,int refresh,int reload,int prewarm) {
    return opted==1&&!guards&&(action==5||action==6)&&progress==1&&session==1&&manual==1&&
        !reload&&!prewarm&&(action!=6||refresh==1);
}
static int arena_pair_world(uintptr_t base) {
    uintptr_t world=read_ptr(base+0x18b6638),level=read_ptr(world+0x88);
    char path[600];
    if(!is_live(base,world)||!is_kind_of(base,world,"World")||!is_live(base,level)||
       read_ptr(level+0x40)!=world)return 0;
    identity_path(base,world,path,sizeof path);
    return !_stricmp(path,"Biop_SPCer.TheWorld");
}
static int arena_pair_selftest(void) {
    return arena_pair_policy(1,0,6,1,1,1,1,0,0)&&arena_pair_policy(1,0,5,1,1,1,0,0,0)&&
        !arena_pair_policy(0,0,6,1,1,1,1,0,0)&&!arena_pair_policy(1,1,6,1,1,1,1,0,0)&&
        !arena_pair_policy(1,0,4,1,1,1,1,0,0)&&!arena_pair_policy(1,0,6,0,1,1,1,0,0)&&
        !arena_pair_policy(1,0,6,1,0,1,1,0,0)&&!arena_pair_policy(1,0,6,1,1,0,1,0,0)&&
        !arena_pair_policy(1,0,6,1,1,1,0,0,0)&&!arena_pair_policy(1,0,6,1,1,1,1,1,0)&&
        !arena_pair_policy(1,0,5,1,1,1,0,0,1);
}
#endif
