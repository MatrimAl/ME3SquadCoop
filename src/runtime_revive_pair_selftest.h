#ifndef ME3_RUNTIME_REVIVE_PAIR_SELFTEST_H
#define ME3_RUNTIME_REVIVE_PAIR_SELFTEST_H
/* Pure admission-identity fixture: no engine objects, calls or global leases. */
static int runtime_revive_pair_selftest(void) {
    unsigned char pawn[0xe24]={0};int action=1;uint32_t life=0x100;
    if(!runtime_revive_creation_idle((uintptr_t)pawn))return 0;
    memcpy(pawn+0xc20,&action,4);if(runtime_revive_creation_idle((uintptr_t)pawn))return 0;
    memset(pawn+0xc20,0,4);memcpy(pawn+0xc28,&action,4);if(runtime_revive_creation_idle((uintptr_t)pawn))return 0;
    memset(pawn+0xc28,0,4);memcpy(pawn+0xe20,&life,4);if(runtime_revive_creation_idle((uintptr_t)pawn))return 0;
    RuntimeRevivePair a={0},b;
    a.generation=9;a.sampled_at=100;a.package.pointer=10;a.package.index=3;
    a.package_guid[0]=0x1234;
    for(unsigned i=0;i<2;++i) {
        a.token[i]=(int)i+1;a.status[i]=1;a.service[i].pointer=20+i;
        a.scope[i].registration=7;a.scope[i].channel_index=(int)i+4;
        GuestCameraStamp *stamps[]={&a.scope[i].pc,&a.scope[i].pawn,&a.scope[i].world,
            &a.scope[i].channel,&a.scope[i].connection,&a.scope[i].driver};
        for(unsigned j=0;j<6;++j){stamps[j]->pointer=100+i*10+j;stamps[j]->index=(int)j;}
    }
    b=a;
    if(!runtime_revive_pair_matches(&a,&b,3100))return 0;
    b.status[0]=2;b.status[1]=2;
    if(!runtime_revive_pair_matches(&a,&b,101))return 0;
    for(int status=0;status<=4;++status)
        if(runtime_revive_status_ready(status)!=(status==1||status==2))return 0;
    int status[2]={1,1},ready[2]={1,0};
    if(runtime_revive_pair_preflight_ready(status,ready))return 0;
    ready[1]=1;if(!runtime_revive_pair_preflight_ready(status,ready))return 0;
    status[0]=2;ready[0]=0;if(!runtime_revive_pair_preflight_ready(status,ready))return 0;
    status[1]=3;if(runtime_revive_pair_preflight_ready(status,ready))return 0;
    b=a;b.status[1]=3;if(runtime_revive_pair_matches(&a,&b,101))return 0;
    b=a;b.scope[1].registration++;if(runtime_revive_pair_matches(&a,&b,101))return 0;
    b=a;b.scope[0].channel.index++;if(runtime_revive_pair_matches(&a,&b,101))return 0;
    b=a;b.token[0]++;if(runtime_revive_pair_matches(&a,&b,101))return 0;
    b=a;b.generation++;if(runtime_revive_pair_matches(&a,&b,101))return 0;
    b=a;b.package_guid[3]++;if(runtime_revive_pair_matches(&a,&b,101))return 0;
    b=a;return !runtime_revive_pair_matches(&a,&b,3101);
}
#endif
