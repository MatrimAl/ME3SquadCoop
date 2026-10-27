#ifndef ME3_RUNTIME_GUEST_TEAM_H
#define ME3_RUNTIME_GUEST_TEAM_H
/* Register the existing remote PC in Shepard's existing native team. Native
 * GetTeamNum returns 255 for a null PRI.Team; native IsHostile rejects that
 * unassigned team in either direction. Never bypass damage or hostility. */
#pragma pack(push,4)
typedef struct RuntimeGuestTeamParms {uintptr_t other;int32_t team;uint32_t new_team,result;} RuntimeGuestTeamParms;
#pragma pack(pop)
_Static_assert(sizeof(RuntimeGuestTeamParms)==20,"stock ChangeTeam ABI");
_Static_assert(offsetof(RuntimeGuestTeamParms,result)==16,"stock ChangeTeam result");
typedef struct RuntimeGuestTeam {
    SessionHandle pc,pri,game,team,world,function,connection,driver;
    int owned,attempted;unsigned attempts;DWORD scanned;
} RuntimeGuestTeam;
static RuntimeGuestTeam runtime_guest_team;
static int runtime_guest_team_enabled(void) {
    static int enabled=-1;
    if(enabled<0){wchar_t value[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_GUEST_TEAM",value,8);
        enabled=!n||(n==1&&value[0]==L'1');}
    return enabled;
}
static uintptr_t runtime_guest_team_field(uintptr_t base,uintptr_t object,const char *owner,const char *name,
        const char *kind,int offset,int size,uint64_t flags) {
    uintptr_t klass=read_ptr(object+0x50),seen[48]={0};char text[96];
    for(unsigned n=0;klass&&n<48;++n) {
        if(!narrative_probe_object(base,klass,"Class"))return 0;
        for(unsigned j=0;j<n;++j)if(seen[j]==klass)return 0;seen[n]=klass;
        object_name(base,klass,text,sizeof text);
        if(!strcmp(text,owner)) {
            uintptr_t field=runtime_native_hud_field(base,klass,name,kind,offset,size);uint64_t actual=0;
            return field&&read_at(field+0x78,&actual,8)&&actual==flags?field:0;
        }
        klass=read_ptr(klass+0x60);
    }
    return 0;
}
static int runtime_guest_team_abi(uintptr_t base,uintptr_t game,uintptr_t *method) {
    uintptr_t fn=guest_player_context_find_method(base,read_ptr(game+0x50),"SFXGame","ChangeTeam");
    if(!guest_player_context_function_abi(base,fn,"SFXGame","ChangeTeam",0x20002,20,16,0,0xf52b0,4)||
       read_ptr(read_ptr(game)+0x230)!=base+0x40d450)return 0;
    const char *names[]={"Other","N","bNewTeam","ReturnValue"};
    const char *types[]={"ObjectProperty","IntProperty","BoolProperty","BoolProperty"};
    const int offsets[]={0,8,12,16},sizes[]={8,4,4,4};uintptr_t field=read_ptr(fn+0x70);
    for(unsigned i=0;i<4;++i) {
        int offset=-1,size=0,dim=0;uint64_t flags=0,mask=0;char name[96];
        object_name(base,field,name,sizeof name);
        if(read_ptr(field+0x40)!=fn||strcmp(name,names[i])||!narrative_probe_object(base,field,types[i])||
           !read_at(field+0x94,&offset,4)||offset!=offsets[i]||!read_at(field+0x74,&size,4)||size!=sizes[i]||
           !read_at(field+0x70,&dim,4)||dim!=1||!read_at(field+0x78,&flags,8)||flags!=(i==3?0x580:0x80)||
           (i>=2&&(!read_at(field+0xd0,&mask,8)||mask!=1)))return 0;
        field=read_ptr(field+0x68);
    }
    if(field)return 0;*method=fn;return 1;
}
static int runtime_guest_team_dispatch(ProcessEvent dispatch,uintptr_t game,uintptr_t function,uintptr_t pc,int team) {
    if(!dispatch||!game||!function||!pc||!((team>=0&&team<4)||team==255))return 0;
    RuntimeGuestTeamParms params={pc,team,0,0};
    dispatch((void*)game,(void*)function,&params,NULL);return (params.result&1)!=0;
}
static void runtime_guest_team_log(const char *event,int index,int verified,DWORD now) {
    char row[360];snprintf(row,sizeof row,"{\"runtime_guest_team\":\"%s\",\"team_index\":%d,\"verified\":%s,\"stock_change_team\":true,\"damage_bypassed\":false,\"combat_verified\":false,\"monotonic_ms\":%lu}\r\n",event,index,verified?"true":"false",now);line(row);
}
static void runtime_guest_team_wait(const char *reason,DWORD now) {
    static const char *last;static unsigned count;
    if(count>=32||(last&&!strcmp(last,reason)))return;last=reason;++count;
    runtime_guest_team_log(reason,-1,0,now);
}
static int runtime_guest_team_restore_policy(int owned,int identities,int world,int pri,int assignment,int abi) {
    return owned&&identities&&world&&pri&&assignment&&abi;
}
static int runtime_guest_team_lifetimes(const RuntimeGuestTeam *q) {
    return session_handle_matches(q->pc)&&session_handle_matches(q->pri)&&session_handle_matches(q->game)&&
        session_handle_matches(q->team)&&session_handle_matches(q->world)&&session_handle_matches(q->function);
}
/* A scene only borrows pawn control; the connected player's team registration
 * persists. Retire on disable, disconnect or actual identity replacement. */
static int runtime_guest_team_retire(uintptr_t base,SessionContext c,DWORD now) {
    RuntimeGuestTeam *q=&runtime_guest_team;if(!q->owned)return 0;
    int live=runtime_guest_team_lifetimes(q);uintptr_t function=0;GameArray teams={0};
    int world=c.world==q->world.pointer&&read_ptr(q->world.pointer+0x6b0)==q->game.pointer&&
        read_ptr(q->game.pointer+0x198)==q->world.pointer&&read_ptr(q->pc.pointer+0x198)==q->world.pointer;
    int same_pri=read_ptr(q->pc.pointer+0x3c4)==q->pri.pointer;
    int same_team=read_ptr(q->pri.pointer+0x2d8)==q->team.pointer;
    if(!live||!world||!same_pri||!same_team) {
        runtime_guest_team_log("retire-preserved-native-replacement",255,0,now);q->owned=0;return 0;
    }
    if(session_environment_block_reason(c))return 0;
    int team_index=-1;
    int abi=identity_array(q->game.pointer+0x4e4,&teams,64)&&teams.count>=4&&
        read_at(q->team.pointer+0x288,&team_index,4)&&team_index>=0&&team_index<4&&
        read_ptr(teams.data+(SIZE_T)team_index*8)==q->team.pointer&&
        runtime_guest_team_abi(base,q->game.pointer,&function)&&function==q->function.pointer;
    if(!runtime_guest_team_restore_policy(q->owned,live,world,same_pri,same_team,abi))return 0;
    int returned=runtime_guest_team_dispatch((ProcessEvent)(base+0x40d450),q->game.pointer,function,q->pc.pointer,255);
    int restored=runtime_guest_team_lifetimes(q)&&read_ptr(q->pc.pointer+0x3c4)==q->pri.pointer&&
        read_ptr(q->pri.pointer+0x2d8)==0;
    runtime_guest_team_log(returned&&restored?"restored-original-null-team":"restore-unverified",255,returned&&restored,now);
    q->owned=0;q->attempted=0;return 1;
}
static int runtime_guest_team_modes_clear(uintptr_t base,uintptr_t pc) {
    uintptr_t manager=read_ptr(pc+0xc38);GameArray modes={0};
    if(!narrative_probe_object(base,manager,"SFXGameModeManager")||
       !runtime_guest_team_field(base,pc,"BioPlayerController","GameModeManager2","ObjectProperty",0xc38,8,0x2000)||
       !runtime_guest_team_field(base,manager,"SFXGameModeManager","GameModes","ArrayProperty",0x70,16,0x402000)||
       !identity_array(manager+0x70,&modes,64)||modes.count<=8)return 0;
    for(unsigned i=7;i<=8;++i) {
        uintptr_t mode=read_ptr(modes.data+(SIZE_T)i*8);uint64_t mask=0;uint32_t flags=0;
        uintptr_t field=runtime_guest_team_field(base,mode,"SFXGameModeBase","bIsActive","BoolProperty",0xd0,4,1);
        if(!narrative_probe_object(base,mode,"SFXGameModeBase")||read_ptr(mode+0x40)!=pc||!field||
           !read_at(field+0xd0,&mask,8)||mask!=2||!read_at(mode+0xd0,&flags,4)||(flags&2))return 0;
    }
    return 1;
}
static int runtime_guest_team_registration_policy(int enabled,int current_session,int reciprocal,int story,int scene,
        int existing_team,int index,int count,int same_host_team) {
    return enabled&&current_session&&reciprocal&&story&&!scene&&!existing_team&&index>=0&&index<4&&count>=4&&same_host_team;
}
/* Returns 1 only when a native call occurred; caller must refresh context. */
static int runtime_guest_team_tick(uintptr_t base,uintptr_t engine,DWORD now) {
    RuntimeGuestTeam *q=&runtime_guest_team;
    if(GetCurrentThreadId()!=session_game_thread||g_runtime_config.role!=ME3_ROLE_HOST)return 0;
    SessionContext c=session_context(engine);
    int enabled=runtime_guest_team_enabled()&&runtime_control_enabled&&runtime_scope_ready&&
        InterlockedCompareExchange(&g_enable_requested,0,0);
    int same_connection=q->owned&&session_handle_matches(q->connection)&&session_handle_matches(q->driver)&&
        read_ptr(q->connection.pointer+0x68)==q->pc.pointer&&
        network_actor_connection_in_scope(base,q->driver.pointer,q->connection.pointer);
    if(q->owned&&(!enabled||!same_connection||!runtime_guest_team_lifetimes(q)||
       c.world!=q->world.pointer||read_ptr(q->pc.pointer+0x198)!=q->world.pointer||
       read_ptr(c.world+0x6b0)!=q->game.pointer||
       read_ptr(q->pc.pointer+0x3c4)!=q->pri.pointer||read_ptr(q->pri.pointer+0x2d8)!=q->team.pointer))
        return runtime_guest_team_retire(base,c,now);
    if(!enabled||q->owned||!session_active||session_fault||session_releasing||session_cleanup_pending)return 0;
    if(q->scanned&&now-q->scanned<500)return 0;q->scanned=now;
    uintptr_t pc=session_remote,pawn=session_pawn,world=session_world;
    if(!session_story_valid(base)||!session_peer_connected(base,pc,world)||
       !session_handle_matches(session_pc_handle)||!session_handle_matches(session_pawn_handle)||
       read_ptr(pc+0x3bc)!=pawn||read_ptr(pawn+0x374)!=pc||c.world!=world||
       session_environment_block_reason(c)||!runtime_guest_team_modes_clear(base,c.pc)||!runtime_guest_team_modes_clear(base,pc)) {
        runtime_guest_team_wait("waiting-native-peer-or-scene",now);return 0;
    }
    uintptr_t pri=read_ptr(pc+0x3c4),host_pri=read_ptr(c.pc+0x3c4),game=read_ptr(world+0x6b0),team=read_ptr(host_pri+0x2d8);
    if(q->attempted&&q->pri.pointer==pri&&q->world.pointer==world&&
       session_handle_matches(q->pri)&&session_handle_matches(q->world))return 0;
    if(!narrative_probe_object(base,pri,"PlayerReplicationInfo")||!narrative_probe_object(base,host_pri,"PlayerReplicationInfo")||
       !narrative_probe_object(base,game,"SFXGame")||!narrative_probe_object(base,team,"SFXTeamInfo")||
       read_ptr(pri+0x198)!=world||read_ptr(game+0x198)!=world||read_ptr(team+0x198)!=world) {
        runtime_guest_team_wait("waiting-native-pri-or-host-team",now);return 0;
    }
    if(!runtime_guest_team_field(base,world,"WorldInfo","Game","ObjectProperty",0x6b0,8,0)||
       !runtime_guest_team_field(base,game,"SFXGame","Teams","ArrayProperty",0x4e4,16,0x400000)||
       !runtime_guest_team_field(base,pc,"Controller","PlayerReplicationInfo","ObjectProperty",0x3c4,8,UINT64_C(0x100000020))||
       !runtime_guest_team_field(base,pri,"PlayerReplicationInfo","Team","ObjectProperty",0x2d8,8,UINT64_C(0x100000020))||
       !runtime_guest_team_field(base,team,"TeamInfo","TeamIndex","IntProperty",0x288,4,UINT64_C(0x140000020))) {
        runtime_guest_team_wait("native-team-layout-refused",now);return 0;
    }
    uintptr_t current=0,function=0;int index=-1;GameArray teams={0};
    if(!read_at(pri+0x2d8,&current,8)||!read_at(team+0x288,&index,4)||!identity_array(game+0x4e4,&teams,64)||
       !runtime_guest_team_registration_policy(enabled,session_active,1,1,0,current!=0,index,teams.count,
           index>=0&&index<teams.count&&read_ptr(teams.data+(SIZE_T)index*8)==team)||
       !runtime_guest_team_abi(base,game,&function)) {
        runtime_guest_team_wait(current?"preserved-existing-native-team":"stock-team-context-refused",now);return 0;
    }
    uintptr_t connection=read_ptr(pc+0x67c),driver=read_ptr(connection+0x8c);
    RuntimeGuestTeam next={session_capture_handle(pc),session_capture_handle(pri),session_capture_handle(game),
        session_capture_handle(team),session_capture_handle(world),session_capture_handle(function),
        session_capture_handle(connection),session_capture_handle(driver),0,1,q->attempts+1,now};
    if(!runtime_guest_team_lifetimes(&next)||!session_handle_matches(next.connection)||!session_handle_matches(next.driver)||
       !network_actor_connection_in_scope(base,driver,connection)||read_ptr(connection+0x68)!=pc||
       read_ptr(pc+0x3c4)!=pri||read_ptr(pri+0x2d8)||read_ptr(host_pri+0x2d8)!=team||
       !session_story_valid(base)||session_environment_block_reason(session_context(engine)))return 0;
    *q=next;
    int returned=runtime_guest_team_dispatch((ProcessEvent)(base+0x40d450),game,function,pc,index);
    q->owned=runtime_guest_team_lifetimes(q)&&read_ptr(pc+0x3c4)==pri&&read_ptr(pri+0x2d8)==team;
    runtime_guest_team_log(returned&&q->owned?"registered-existing-host-team":"registration-unverified",index,returned&&q->owned,now);
    return 1;
}
#ifdef ME3_RUNTIME_FIXTURE
static unsigned runtime_guest_team_fixture_calls;static int runtime_guest_team_fixture_index;
static void runtime_guest_team_fixture_forward(void *self,void *fn,void *params,void *result) {
    RuntimeGuestTeamParms *p=params;
    if((uintptr_t)self==10&&(uintptr_t)fn==20&&!result&&p->other==30&&p->team==runtime_guest_team_fixture_index&&
       !p->new_team&&!p->result&&GetLastError()==801)++runtime_guest_team_fixture_calls;
    p->result=1;SetLastError(802);
}
static int runtime_guest_team_selftest(void) {
    if(!runtime_guest_team_registration_policy(1,1,1,1,0,0,0,4,1))return 0;
    for(unsigned i=0;i<9;++i){int v[]={1,1,1,1,0,0,0,4,1};
        if(i<4||i==8)v[i]=0;else if(i==4||i==5)v[i]=1;else if(i==6)v[i]=4;else v[i]=3;
        if(runtime_guest_team_registration_policy(v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7],v[8]))return 0;}
    if(!runtime_guest_team_restore_policy(1,1,1,1,1,1))return 0;
    for(unsigned i=0;i<6;++i){int v[]={1,1,1,1,1,1};v[i]=0;
        if(runtime_guest_team_restore_policy(v[0],v[1],v[2],v[3],v[4],v[5]))return 0;}
    DWORD error=GetLastError();runtime_guest_team_fixture_calls=0;
    int ok=1;
    for(unsigned i=0;i<2;++i){runtime_guest_team_fixture_index=i?255:0;SetLastError(801);
        ok&=runtime_guest_team_dispatch(runtime_guest_team_fixture_forward,10,20,30,runtime_guest_team_fixture_index)&&GetLastError()==802;}
    ok&=runtime_guest_team_fixture_calls==2;
    ok&=!runtime_guest_team_dispatch(runtime_guest_team_fixture_forward,10,20,30,4)&&
        !runtime_guest_team_dispatch(runtime_guest_team_fixture_forward,10,20,30,-1)&&
        !runtime_guest_team_dispatch(NULL,10,20,30,0)&&runtime_guest_team_fixture_calls==2;
    SetLastError(error);return ok;
}
#endif
#endif
