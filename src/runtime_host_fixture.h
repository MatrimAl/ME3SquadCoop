#ifndef ME3_RUNTIME_HOST_FIXTURE_H
#define ME3_RUNTIME_HOST_FIXTURE_H
/* The checked production startup pipeline uses fixture-owned allocation/listen
 * callbacks on allocated synthetic objects. No engine code or socket is run. */
static uintptr_t runtime_host_fixture_o[43];
static unsigned runtime_host_fixture_copies,runtime_host_fixture_destroys,runtime_host_fixture_inits;
static int runtime_host_fixture_failure;
static RuntimeHostURL *runtime_host_fixture_copy(RuntimeHostURL *to,const RuntimeHostURL *from) {
    *to=*from;GameString *target[]={&to->protocol,&to->host,&to->map,&to->portal};
    const GameString *source[]={&from->protocol,&from->host,&from->map,&from->portal};
    for(int i=0;i<4;++i) {
        target[i]->data=NULL;
        if(source[i]->count) {
            target[i]->data=HeapAlloc(GetProcessHeap(),0,(size_t)source[i]->count*2);
            memcpy(target[i]->data,source[i]->data,(size_t)source[i]->count*2);
        }
    }
    to->options.data=0;
    if(from->options.count) {
        GameString *options=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,(size_t)from->options.count*16);
        to->options.data=(uintptr_t)options;
        for(int i=0;i<from->options.count;++i) {
            GameString source_option=((GameString*)from->options.data)[i];options[i]=source_option;
            options[i].data=HeapAlloc(GetProcessHeap(),0,(size_t)source_option.count*2);
            memcpy(options[i].data,source_option.data,(size_t)source_option.count*2);
        }
    }
    ++runtime_host_fixture_copies;return to;
}
static void runtime_host_fixture_destroy(RuntimeHostURL *url) {
    GameString *strings[]={&url->protocol,&url->host,&url->map,&url->portal};
    for(int i=0;i<4;++i)if(strings[i]->data)HeapFree(GetProcessHeap(),0,strings[i]->data);
    for(int i=0;i<url->options.count;++i)HeapFree(GetProcessHeap(),0,((GameString*)url->options.data)[i].data);
    if(url->options.data)HeapFree(GetProcessHeap(),0,(void*)url->options.data);
    memset(url,0,sizeof *url);++runtime_host_fixture_destroys;
}
static void runtime_host_fixture_error_free(void *memory,uint32_t flags) {(void)flags;HeapFree(GetProcessHeap(),0,memory);}
static int runtime_host_fixture_listen(void *world,RuntimeHostURL *url,GameString *error) {
    (void)error;uintptr_t *o=runtime_host_fixture_o;
    int port=url->port;runtime_host_fixture_destroy(url);
    if(runtime_host_fixture_failure==1)return 0;
    runtime_fixture_ptr((uintptr_t)world+0x120,o[7]);*(unsigned char*)(o[2]+0x7c8)=2;
    unsigned char address[16]={2,0,0,0,26,1,2,3};
    port+=runtime_host_fixture_failure==2;address[2]=(unsigned char)(port>>8);address[3]=(unsigned char)port;
    memcpy((void*)(o[7]+0x1e4),address,sizeof address);
    return 1;
}
static void runtime_host_fixture_initialize(void *map) {
    (void)map;++runtime_host_fixture_inits;
    if(runtime_host_fixture_failure==3)*(uint32_t*)(runtime_host_fixture_o[9]+0x6c)^=1;
}
static int runtime_host_startup_selftest(void) {
    unsigned char *image=VirtualAlloc(NULL,0x1900000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    unsigned char *memory=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,43*0x1000);
    if(!image||!memory) {if(image)VirtualFree(image,0,MEM_RELEASE);if(memory)HeapFree(GetProcessHeap(),0,memory);return 0;}
    uintptr_t base=(uintptr_t)image,*o=runtime_host_fixture_o;
    const char *labels[]={"TheWorld","PersistentLevel","Info","PC","Shepard","Squad","Game","Driver","Master",
        "Core","SFXGame","SFXGameContent","Biop_ProMar","Mode","Player","EngineInstance",
        "MaxPlayers","MaxPlayersAllowed","NumPlayers","Class","World","Level","BioWorldInfo","Actor",
        "SFXPlayerController","SFXPawn_Player","BioBaseSquad","GameInfo","TcpNetDriver","PackageMap",
        "Package","SFXGameModeManager","LocalPlayer","Engine","IntProperty","Object","BioPawn","NetDriver",
        "SFXPawn_PlayerSoldier","SFXPawn_PlayerParty","SFXPawn","Pawn","SFXPawn_Ashley"};
    const int classes[]={20,21,22,24,38,26,27,28,29,30,30,30,30,31,32,33,34,34,34,
        19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19,19};
    unsigned char names[43*128]={0};
    for(int i=0;i<43;++i) {
        o[i]=(uintptr_t)(memory+i*0x1000);memcpy((void*)(o[i]+8),&i,4);unsigned name=i*128;
        memcpy((void*)(o[i]+0x48),&name,4);strcpy((char*)names+name+12,labels[i]);
    }
    for(int i=0;i<43;++i)runtime_fixture_ptr(o[i]+0x50,o[classes[i]]);
    const int actors[]={22,24,26,27};
    for(unsigned i=0;i<sizeof actors/sizeof actors[0];++i)runtime_fixture_ptr(o[actors[i]]+0x60,o[23]);
    /* Captured native Shepard hierarchy, with no invented BioPlayerPawn:
     * Soldier -> Player -> PlayerParty -> SFXPawn -> BioPawn -> Pawn -> Actor. */
    const int pawn_chain[]={38,25,39,40,36,41,23,35};
    for(unsigned i=1;i<sizeof pawn_chain/sizeof pawn_chain[0];++i)
        runtime_fixture_ptr(o[pawn_chain[i-1]]+0x60,o[pawn_chain[i]]);
    runtime_fixture_ptr(o[42]+0x60,o[40]);runtime_fixture_ptr(o[28]+0x60,o[37]);
    GameArray objects={(uintptr_t)o,43,43};memcpy(image+OBJECTS_RVA,&objects,sizeof objects);
    runtime_fixture_ptr(base+NAME_POOLS_RVA,(uintptr_t)names);runtime_fixture_ptr(base+0x18b6638,o[0]);
    runtime_fixture_ptr(o[0]+0x40,o[12]);runtime_fixture_ptr(o[0]+0x88,o[1]);
    uintptr_t level_actors[]={o[2]};GameArray actors_array={(uintptr_t)level_actors,1,1};
    memcpy((void*)(o[1]+0x60),&actors_array,sizeof actors_array);
    runtime_fixture_ptr(o[2]+0x8e8,o[5]);runtime_fixture_ptr(o[2]+0x6b0,o[6]);
    runtime_fixture_ptr(o[5]+0x2a8,o[4]);*(uint32_t*)(o[5]+0x2b0)=4;*(uint32_t*)(o[4]+0xe18)=8;
    runtime_fixture_ptr(o[5]+0x2cc,o[4]);runtime_fixture_ptr(o[5]+0x2d4,o[4]);
    runtime_fixture_ptr(o[3]+0x3bc,o[4]);runtime_fixture_ptr(o[4]+0x374,o[3]);
    runtime_fixture_ptr(o[3]+0x198,o[2]);runtime_fixture_ptr(o[3]+0xc38,o[13]);runtime_fixture_ptr(o[3]+0x67c,o[14]);
    runtime_fixture_ptr(o[14]+0x68,o[3]);uintptr_t players[]={o[14]};GameArray local={(uintptr_t)players,1,1};
    memcpy((void*)(o[15]+0x498),&local,sizeof local);
    runtime_fixture_ptr(o[27]+0x70,o[16]);
    for(int i=16;i<=18;++i) {
        runtime_fixture_ptr(o[i]+0x40,o[27]);runtime_fixture_ptr(o[i]+0x68,i<18?o[i+1]:0);
        *(int32_t*)(o[i]+0x70)=1;*(int32_t*)(o[i]+0x74)=4;*(int32_t*)(o[i]+0x94)=0x42c+(i-16)*4;
    }
    *(uint64_t*)(o[16]+0x78)=0x44000;
    runtime_fixture_ptr(o[7],base+0x12bc980);runtime_fixture_ptr(base+0x12bc980+0x2e0,base+0x903a80);
    const unsigned char port_pin[]={0x41,0x0f,0xb7,0x4f,0x50},bind_pin[]={0xe8,0x72,0x9e,0,0};
    memcpy(image+0x9033ae,port_pin,sizeof port_pin);memcpy(image+0x9033e9,bind_pin,sizeof bind_pin);
    runtime_fixture_ptr(o[7]+0x90,o[8]);runtime_fixture_ptr(o[8],base+0x1086b10);
    runtime_fixture_ptr(base+0x1086b10+0x2e0,base+0xc8ea0);
    uintptr_t registered[]={o[9],o[10],o[11]};GameArray registered_array={(uintptr_t)registered,3,3};
    memcpy(image+0x17f4b10,&registered_array,sizeof registered_array);
    unsigned char entries[3*0x4c]={0};GameArray entry_array={(uintptr_t)entries,3,3};memcpy((void*)(o[8]+0x68),&entry_array,sizeof entry_array);
    for(int i=0;i<3;++i) {
        runtime_fixture_ptr((uintptr_t)entries+(size_t)i*0x4c+8,o[9+i]);uint32_t guid[4]={1u+(unsigned)i,2,3,4};
        memcpy((void*)(o[9+i]+0x6c),guid,16);memcpy(entries+(size_t)i*0x4c+0x10,guid,16);
    }
    wchar_t protocol[]=L"unreal",map[]=L"BioP_ProMar",option[]=L"name=Player";
    GameString options={option,12,12};
    RuntimeHostURL url={{protocol,7,7},{0},{map,12,12},{(uintptr_t)&options,1,1},{0},7777,1};
    memcpy((void*)(o[15]+0x8ac),&url,sizeof url);
    Me3RuntimeConfig saved=g_runtime_config;
    int ok=runtime_config_parse(&g_runtime_config,L"1",L"host",L"26.1.2.3",NULL,L"17779")==ME3_CONFIG_READY;
    const RuntimeHostNativeCalls calls={runtime_host_fixture_copy,runtime_host_fixture_destroy,
        runtime_host_fixture_listen,runtime_host_fixture_initialize,runtime_host_fixture_error_free};
    SessionContext context=session_context(o[15]);CombatContextChain chain;
    ok=ok&&runtime_host_story_idle(base,context);
    runtime_fixture_ptr(o[4]+0x50,o[42]);ok=ok&&!runtime_host_story_idle(base,context);
    runtime_fixture_ptr(o[4]+0x50,o[36]);ok=ok&&!runtime_host_story_idle(base,context);
    runtime_fixture_ptr(o[4]+0x50,o[38]);ok=ok&&runtime_host_story_idle(base,context);
    runtime_fixture_ptr(o[4]+0x198,o[2]);combat_context_read_chain(base,&chain);
    RuntimeJoinSample join_sample={0};
    ok=ok&&runtime_join_context_sample(base,context,&chain,&join_sample);
    SessionContext wrong_manager=context;wrong_manager.manager=o[5];
    ok=ok&&!runtime_join_context_sample(base,wrong_manager,&chain,&join_sample);
    runtime_fixture_ptr(o[4]+0x374,0);ok=ok&&!runtime_join_context_sample(base,context,&chain,&join_sample);
    runtime_fixture_ptr(o[4]+0x374,o[3]);runtime_fixture_ptr(o[5]+0x2cc,0);
    ok=ok&&!runtime_join_context_sample(base,context,&chain,&join_sample);
    runtime_fixture_ptr(o[5]+0x2cc,o[4]);runtime_fixture_ptr(o[4]+0x198,o[1]);
    ok=ok&&!runtime_join_context_sample(base,context,&chain,&join_sample);
    runtime_fixture_ptr(o[4]+0x198,o[2]);
    ok=ok&&runtime_join_context_sample(base,context,&chain,&join_sample);
    for(int failure=0;failure<=3&&ok;++failure) {
        runtime_host_fixture_failure=failure;runtime_host_fixture_copies=runtime_host_fixture_destroys=runtime_host_fixture_inits=0;
        runtime_fixture_ptr(o[0]+0x120,0);*(unsigned char*)(o[2]+0x7c8)=0;
        *(int32_t*)(o[6]+0x42c)=1;*(int32_t*)(o[6]+0x430)=1;*(int32_t*)(o[6]+0x434)=1;
        combat_context_read_chain(base,&chain);
        int status=runtime_host_start_checked(base,o[15],context,&chain,&calls);
        int expected=failure==0?RUNTIME_START_LISTENING:failure==1?RUNTIME_START_HOST_NATIVE_FAILED:RUNTIME_START_HOST_VERIFY_FAILED;
        ok=status==expected&&runtime_host_fixture_copies==1&&runtime_host_fixture_destroys==1&&
            runtime_host_fixture_inits==(failure==0||failure==3?1u:0u)&&
            !memcmp((void*)(o[15]+0x8ac),&url,sizeof url)&&read_ptr(o[3]+0x3bc)==o[4]&&
            read_ptr(o[4]+0x374)==o[3]&&read_ptr(o[5]+0x2cc)==o[4]&&read_ptr(o[5]+0x2d4)==o[4];
        if(failure==0)ok=ok&&runtime_host_capacity.active&&*(int32_t*)(o[6]+0x42c)==2&&
            runtime_host_capacity_restore(base,&runtime_host_capacity)&&*(int32_t*)(o[6]+0x42c)==1;
        else ok=ok&&!runtime_host_capacity.active&&*(int32_t*)(o[6]+0x42c)==1&&*(int32_t*)(o[6]+0x430)==1;
        if(failure!=1)ok=ok&&read_ptr(o[0]+0x120)==o[7]; /* engine owns the listener */
    }
    *(uint32_t*)(o[9]+0x6c)=1;
    RuntimeHostCapacityLease lease;*(int32_t*)(o[6]+0x42c)=1;*(int32_t*)(o[6]+0x430)=1;
    ok=ok&&runtime_host_capacity_apply(base,o[2],&lease);*(int32_t*)(o[6]+0x42c)=3;
    ok=ok&&!runtime_host_capacity_restore(base,&lease)&&*(int32_t*)(o[6]+0x42c)==3&&*(int32_t*)(o[6]+0x430)==1;
    *(int32_t*)(o[6]+0x42c)=1;ok=ok&&runtime_host_capacity_apply(base,o[2],&lease);
    *(uint64_t*)(o[6]+0x48)^=1;ok=ok&&!runtime_host_capacity_restore(base,&lease)&&
        *(int32_t*)(o[6]+0x42c)==2&&*(int32_t*)(o[6]+0x430)==2;
    g_runtime_config=saved;memset(&runtime_host_capacity,0,sizeof runtime_host_capacity);
    VirtualFree(image,0,MEM_RELEASE);HeapFree(GetProcessHeap(),0,memory);memset(o,0,sizeof runtime_host_fixture_o);return ok;
}
#endif
