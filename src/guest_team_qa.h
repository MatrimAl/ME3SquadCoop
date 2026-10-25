#ifndef ME3_GUEST_TEAM_QA_H
#define ME3_GUEST_TEAM_QA_H
/* Protected probe only: stock ChangeTeam, never a raw Team write. */
#pragma pack(push,4)
typedef struct GuestTeamParms {uintptr_t other;int32_t team;uint32_t new_team,result;} GuestTeamParms;
#pragma pack(pop)
_Static_assert(sizeof(GuestTeamParms)==20,"ChangeTeam ABI");
_Static_assert(offsetof(GuestTeamParms,result)==16,"ChangeTeam return ABI");
typedef struct GuestTeamQA {GuestCameraStamp pc,pri,game,team,world,function;int owned,attempts,attempted;DWORD scan_at;} GuestTeamQA;
static GuestTeamQA guest_team_qa;
static int guest_team_qa_opted(void) {
    wchar_t value[8]={0};return test_action==6&&progress_enabled&&has_marker(root)&&
        GetEnvironmentVariableW(L"ME3_SQUAD_COOP_GUEST_TEAM_QA",value,8)==1&&value[0]==L'1';
}
static int guest_team_property(uintptr_t base,uintptr_t object,const char *owner,const char *name,const char *kind,int offset,int size,uint64_t flags) {
    uintptr_t p=guest_hit_damage_field(base,read_ptr(object+0x50),owner,name);int32_t got=0;char path[220];
    snprintf(path,sizeof path,"%s.%s.%s",(!strncmp(owner,"SFX",3)||!strncmp(owner,"Bio",3))?"SFXGame":"Engine",owner,name);
    return combat_fire_property(base,p,kind,path,offset,flags)&&read_at(p+0x74,&got,4)&&got==size;
}
static int guest_team_abi(uintptr_t base,uintptr_t game,uintptr_t *method) {
    uintptr_t fn=guest_player_context_find_method(base,read_ptr(game+0x50),"SFXGame","ChangeTeam");
    if(!guest_player_context_function_abi(base,fn,"SFXGame","ChangeTeam",0x20002,20,16,0,0xf52b0,4)||
       read_ptr(read_ptr(game)+0x230)!=base+0x40d450)return 0;
    const char *names[]={"Other","N","bNewTeam","ReturnValue"},*types[]={"ObjectProperty","IntProperty","BoolProperty","BoolProperty"};
    const int offsets[]={0,8,12,16},sizes[]={8,4,4,4};uintptr_t prop=read_ptr(fn+0x70);
    for(int i=0;i<4;++i){char path[180];int32_t size=0;uint64_t mask=0;
        snprintf(path,sizeof path,"SFXGame.SFXGame.ChangeTeam.%s",names[i]);
        if(read_ptr(prop+0x40)!=fn||!combat_fire_property(base,prop,types[i],path,offsets[i],i==3?0x580:0x80)||
           !read_at(prop+0x74,&size,4)||size!=sizes[i]||(i>=2&&(!read_at(prop+0xd0,&mask,8)||mask!=1)))return 0;
        prop=read_ptr(prop+0x68);
    }
    if(prop)return 0;*method=fn;return 1;
}
static int guest_team_dispatch(ProcessEvent dispatch,uintptr_t game,uintptr_t fn,uintptr_t pc,int index) {
    if(!dispatch||!game||!fn||!pc||!((index>=0&&index<4)||index==255))return 0;
    GuestTeamParms parms={pc,index,0,0};dispatch((void*)game,(void*)fn,&parms,NULL);return (parms.result&1)!=0;
}
static void guest_team_log(const char *event,int index,int verified,DWORD now) {
    char row[480];snprintf(row,sizeof row,"{\"guest_team_qa\":\"%s\",\"team_index\":%d,\"verified\":%s,\"attempts\":%d,\"stock_change_team\":true,\"combat_verified\":false,\"monotonic_ms\":%lu}\r\n",event,index,verified?"true":"false",guest_team_qa.attempts,now);line(row);
}
static int guest_team_restore_eligible(int owned,int identities,int world_same,int pri_same,int assignment_same,int dispatch_valid) {
    return owned&&identities&&world_same&&pri_same&&assignment_same&&dispatch_valid;
}
static void guest_team_qa_release(uintptr_t base,DWORD now) {
    GuestTeamQA *q=&guest_team_qa;if(!q->owned)return;
    int identities=guest_camera_stamp_matches(base,&q->pc,1)&&guest_camera_stamp_matches(base,&q->pri,1)&&
        guest_camera_stamp_matches(base,&q->game,1)&&guest_camera_stamp_matches(base,&q->world,1)&&guest_camera_stamp_matches(base,&q->team,1);
    GameArray teams={0};uintptr_t fn=0;int valid=identities&&
        read_ptr(q->world.pointer+0x6b0)==q->game.pointer&&read_ptr(q->game.pointer+0x198)==q->world.pointer&&
        guest_hit_array(q->game.pointer+0x4e4,&teams,64)&&teams.count>=4&&
        guest_team_abi(base,q->game.pointer,&fn)&&fn==q->function.pointer&&guest_camera_stamp_matches(base,&q->function,1);
    int current_world=session_context(session_engine).world==q->world.pointer&&read_ptr(q->pc.pointer+0x198)==q->world.pointer;
    if(!guest_team_restore_eligible(q->owned,identities,current_world,
        read_ptr(q->pc.pointer+0x3c4)==q->pri.pointer,read_ptr(q->pri.pointer+0x2d8)==q->team.pointer,valid)){
        guest_team_log("restore-skipped-changed-context",255,0,now);q->owned=0;return;
    }
    int returned=guest_team_dispatch((ProcessEvent)(base+0x40d450),q->game.pointer,fn,q->pc.pointer,255);
    int restored=read_ptr(q->pri.pointer+0x2d8)==0;guest_team_log(returned&&restored?"restored":"restore-unverified",255,returned&&restored,now);
    /* One bounded stock restore attempt; failure is reported, never raw-cleared. */
    q->owned=0;q->attempted=0;
}
static int guest_team_modes_clear(uintptr_t base,uintptr_t pc) {
    uintptr_t manager=read_ptr(pc+0xc38);GameArray modes={0};
    if(!narrative_probe_object(base,manager,"SFXGameModeManager")||
       !guest_team_property(base,pc,"BioPlayerController","GameModeManager2","ObjectProperty",0xc38,8,0x2000)||
       !guest_team_property(base,manager,"SFXGameModeManager","GameModes","ArrayProperty",0x70,16,0x402000)||
       !guest_hit_array(manager+0x70,&modes,64)||modes.count<=8)return 0;
    for(int i=7;i<=8;++i){uintptr_t mode=read_ptr(modes.data+i*8);uint32_t flags=0;
        uint64_t mask=0;uintptr_t prop=guest_hit_damage_field(base,read_ptr(mode+0x50),"SFXGameModeBase","bIsActive");
        if(!narrative_probe_object(base,mode,"SFXGameModeBase")||read_ptr(mode+0x40)!=pc||
           !guest_team_property(base,mode,"SFXGameModeBase","bIsActive","BoolProperty",0xd0,4,1)||
           !read_at(prop+0xd0,&mask,8)||mask!=2||!read_at(mode+0xd0,&flags,4)||(flags&2))return 0;
    }return 1;
}
/* Output only, on reason changes, at most 48 records in this protected process. */
static void guest_team_wait(const char *reason,DWORD now) {
    static const char *last;static unsigned count;
    if(count>=48||(last&&!strcmp(last,reason)))return;
    last=reason;++count;char row[400];
    snprintf(row,sizeof row,"{\"guest_team_qa\":\"eligibility\",\"reason\":\"%s\",\"record\":%u,\"gameplay_writes\":false,\"monotonic_ms\":%lu}\r\n",reason,count,now);line(row);
}
static void guest_team_qa_tick(uintptr_t base,DWORD now) {
    GuestTeamQA *q=&guest_team_qa;
    if(!guest_team_qa_opted()){guest_team_qa_release(base,now);return;}
    if(!session_active||session_releasing){guest_team_wait("session-inactive-or-releasing",now);return;}
    if(now-q->scan_at<250)return;q->scan_at=now;
    if(!session_story_valid(base)||!session_peer_connected(base,session_remote,session_world)||
       read_ptr(session_remote+0x3bc)!=session_pawn||read_ptr(session_pawn+0x374)!=session_remote||
       session_engine_blocked(session_context(session_engine))){guest_team_wait("session-context-or-scene",now);return;}
    if(!guest_team_modes_clear(base,session_host)){guest_team_wait("host-mode-or-reflected-layout",now);return;}
    if(!guest_team_modes_clear(base,session_remote)){guest_team_wait("remote-mode-or-reflected-layout",now);return;}
    uintptr_t pc=session_remote,world=session_world,pri=read_ptr(pc+0x3c4),host_pri=read_ptr(session_host+0x3c4),game=read_ptr(world+0x6b0),team=read_ptr(host_pri+0x2d8),fn=0;
    if(!narrative_probe_object(base,pri,"PlayerReplicationInfo")||!narrative_probe_object(base,host_pri,"PlayerReplicationInfo")||
       !narrative_probe_object(base,game,"SFXGame")||!narrative_probe_object(base,team,"SFXTeamInfo")||
       read_ptr(game+0x198)!=world||read_ptr(team+0x198)!=world||read_ptr(pri+0x198)!=world){guest_team_wait("pri-game-host-team-or-world",now);return;}
    if(q->pri.pointer==pri&&guest_camera_stamp_matches(base,&q->pri,1)&&q->attempted){guest_team_wait("current-pri-attempted",now);return;}
    if(q->owned)guest_team_qa_release(base,now);
    if(q->attempts>=8){guest_team_wait("attempt-budget",now);return;}
    if(read_ptr(pri+0x2d8)){guest_team_wait("preserve-existing-team",now);return;}
    unsigned char net=255;GameArray teams={0};int32_t index=-1;
    if(!read_at(world+0x7c8,&net,1)||net!=2){guest_team_wait("not-authoritative-listen-world",now);return;}
    if(!guest_team_property(base,world,"WorldInfo","Game","ObjectProperty",0x6b0,8,0)||
       !guest_team_property(base,game,"SFXGame","Teams","ArrayProperty",0x4e4,16,0x400000)||
       !guest_team_property(base,pc,"Controller","PlayerReplicationInfo","ObjectProperty",0x3c4,8,UINT64_C(0x100000020))||
       !guest_team_property(base,pri,"PlayerReplicationInfo","Team","ObjectProperty",0x2d8,8,UINT64_C(0x100000020))||
       !guest_team_property(base,team,"TeamInfo","TeamIndex","IntProperty",0x288,4,UINT64_C(0x140000020))){guest_team_wait("team-reflected-layout",now);return;}
    if(!read_at(team+0x288,&index,4)||index<0||index>=4||!guest_hit_array(game+0x4e4,&teams,64)||teams.count<4||
       read_ptr(teams.data+index*8)!=team){guest_team_wait("existing-team-table-or-index",now);return;}
    if(!guest_team_abi(base,game,&fn)){guest_team_wait("change-team-function-abi",now);return;}
    if(!guest_camera_stamp_read(base,pc,&q->pc,1)||!guest_camera_stamp_read(base,pri,&q->pri,1)||
       !guest_camera_stamp_read(base,game,&q->game,1)||!guest_camera_stamp_read(base,team,&q->team,1)||
       !guest_camera_stamp_read(base,world,&q->world,1)||!guest_camera_stamp_read(base,fn,&q->function,1)){guest_team_wait("identity-stamp",now);return;}
    q->attempted=1;++q->attempts;
    int returned=guest_team_dispatch((ProcessEvent)(base+0x40d450),game,fn,pc,index);
    q->owned=guest_camera_stamp_matches(base,&q->pri,1)&&read_ptr(pc+0x3c4)==pri&&read_ptr(pri+0x2d8)==team;
    guest_team_log(returned&&q->owned?"initialized":"initialization-unverified",index,returned&&q->owned,now);
}
#endif
