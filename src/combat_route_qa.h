#ifndef ME3_COMBAT_ROUTE_QA_H
#define ME3_COMBAT_ROUTE_QA_H
/* Default-off protected QA driver: ordinary host PlayerInput axes only.
 * Mars nav locations were measured privately; the approach is a candidate
 * route, not a claim of traversed terrain or combat. Stop before the real
 * jump node. Never invoke traversal or edit actor/story/physics state.
 * Include after session_control.h; call before and after the ordinary Tick. */
typedef struct CombatRoutePoint {const char *name;float xyz[3];} CombatRoutePoint;
static const CombatRoutePoint combat_route_points[]={
    {"PathNode_103",{-22525.412f,49222.133f,225.0f}},
    {"PathNode_104",{-23081.881f,48666.277f,225.0f}},
    {"PathNode_112",{-23078.938f,48084.230f,225.0f}},
    {"PathNode_111",{-23002.736f,47478.316f,234.481f}},
    {"PathNode_110",{-22777.227f,46675.863f,215.735f}},
    {"PathNode_109",{-22577.695f,45918.262f,200.0f}},
    {"PathNode_108",{-22462.197f,45221.164f,74.406f}},
    {"PathNode_107",{-22447.604f,44560.285f,-12.404f}},
    {"PathNode_106",{-22732.002f,43981.504f,-56.813f}},
    {"SFXNav_JumpDownNode_6",{-22184.104f,43566.086f,0.0f}}
};
#define COMBAT_ROUTE_COUNT (sizeof combat_route_points/sizeof *combat_route_points)
static const float combat_route_start[3]={-21960.033f,49931.332f,279.492f};
/* 150 units behind the top node; arrival radius 25 keeps the capsule away
 * from its radius-50 traversal cylinder. No drop or action-button input. */
static const float combat_route_stop[3]={-22288.13f,43674.24f,0.0f};
typedef struct CombatRoutePolicy {int opted,authority,identity,ordinary,camera,ground,input;DWORD elapsed;} CombatRoutePolicy;
static int combat_route_policy(const CombatRoutePolicy *p) {
    return p&&p->opted==1&&p->authority==1&&p->identity==1&&p->ordinary==1&&p->camera==1&&
        p->ground==1&&p->input==1&&p->elapsed<75000;
}
static int combat_route_axes(const float position[3],const float target[3],int32_t yaw,float gain,float out[2]) {
    out[0]=out[1]=0;
    if(!scene_readiness_finite3(position)||!scene_readiness_finite3(target)||!isfinite(gain)||gain<=0||gain>0.45f)return 0;
    float dx=target[0]-position[0],dy=target[1]-position[1],length=sqrtf(dx*dx+dy*dy);
    if(!isfinite(length)||length<1||length>1500)return 0;
    float angle=(float)(yaw%65536)*0.00009587379924285f;
    out[0]=gain*(dx*cosf(angle)+dy*sinf(angle))/length;
    out[1]=gain*(-dx*sinf(angle)+dy*cosf(angle))/length;
    return isfinite(out[0])&&isfinite(out[1])&&fabsf(out[0])<=0.45001f&&fabsf(out[1])<=0.45001f;
}
static float combat_route_horizontal(const float a[3],const float b[3]) {
    float x=a[0]-b[0],y=a[1]-b[1];return sqrtf(x*x+y*y);
}
static int combat_route_corridor(const float position[3],const float from[3],const float to[3]) {
    if(!scene_readiness_finite3(position)||!scene_readiness_finite3(from)||!scene_readiness_finite3(to))return 0;
    float x=to[0]-from[0],y=to[1]-from[1],den=x*x+y*y;
    if(!isfinite(den)||den<1)return 0;
    float t=((position[0]-from[0])*x+(position[1]-from[1])*y)/den;t=fmaxf(0,fminf(1,t));
    float nearest[3]={from[0]+t*x,from[1]+t*y,0};
    return combat_route_horizontal(position,nearest)<=180&&fabsf(position[2]-(from[2]+t*(to[2]-from[2])))<=180;
}
static int combat_route_start_allowed(const float position[3]) {
    /* A joined peer can displace the otherwise stationary landing pawn.
     * Capture its actual guarded/grounded start inside this landing envelope;
     * only the first leg uses that origin. Later nav corridors stay fixed. */
    return scene_readiness_finite3(position)&&combat_route_horizontal(position,combat_route_start)<=350&&
        fabsf(position[2]-combat_route_start[2])<=100;
}
typedef struct CombatRouteQA {
    int loaded,opted,active,done,stopping,injected;unsigned waypoint,cleanup_tries;
    DWORD stable_at,started,advanced_at,report_at,discovery_at,stopped_at,still_at,paused_at;
    GuestCameraStamp pc,pawn,world,squad,input,camera,nodes[COMBAT_ROUTE_COUNT];
    uintptr_t story_player,story_initial,node_world;float progress[3],axes[2],origin[3];int nodes_ready;
} CombatRouteQA;
static CombatRouteQA combat_route_qa;
static int combat_route_qa_host_mode(void) {
    CombatRouteQA *q=&combat_route_qa;
    if(!q->loaded){wchar_t value[8]={0};q->loaded=1;DWORD length=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_COMBAT_ROUTE_QA",value,8);q->opted=length==1&&!wcscmp(value,L"1");}
    return q->opted&&((test_action==6&&!strcmp(test_role,"host"))||
        ((test_action==1||test_action==4)&&!strcmp(test_role,"single")));
}
static void combat_route_log(const char *event,const char *reason,const float *position,DWORD now) {
    CombatRouteQA *q=&combat_route_qa;char row[750];
    snprintf(row,sizeof row,"{\"combat_route_qa\":\"%s\",\"reason\":\"%s\",\"waypoint\":%u,\"position\":[%.3f,%.3f,%.3f],\"forward\":%.5f,\"strafe\":%.5f,\"elapsed_ms\":%lu,\"monotonic_ms\":%lu,\"position_writes\":false,\"traversal_invoked\":false,\"combat_verified\":false}\r\n",
        event,reason,q->waypoint,position?position[0]:0,position?position[1]:0,position?position[2]:0,
        q->axes[0],q->axes[1],q->started?now-q->started:0,now);line(row);
}
static int combat_route_write_axes(uintptr_t input,float forward,float strafe) {
    SIZE_T wrote=0;int ok=1;const uintptr_t offsets[]={0x194,0x1a4,0x1ac};const float values[]={forward,forward,strafe};
    for(unsigned i=0;i<3;++i)ok=WriteProcessMemory(GetCurrentProcess(),(void*)(input+offsets[i]),values+i,4,&wrote)&&wrote==4&&ok;
    return ok;
}
static int combat_route_cleanup(uintptr_t base) {
    CombatRouteQA *q=&combat_route_qa;if(!q->injected)return 1;
    int safe=guest_camera_stamp_matches(base,&q->pc,1)&&guest_camera_actor_live(base,q->pc.pointer)&&
        guest_camera_stamp_matches(base,&q->input,1)&&read_ptr(q->pc.pointer+0x6dc)==q->input.pointer&&
        read_ptr(q->input.pointer+0x40)==q->pc.pointer;
    ++q->cleanup_tries;int cleaned=safe&&combat_route_write_axes(q->input.pointer,0,0);
    if(cleaned||!safe||q->cleanup_tries>=3)q->injected=0;return cleaned;
}
static void combat_route_end(uintptr_t base,const char *event,const char *reason,const float *position,DWORD now) {
    CombatRouteQA *q=&combat_route_qa;int clean=combat_route_cleanup(base);q->done=1;q->active=0;
    combat_route_log(clean?event:"cleanup-unverified",clean?reason:"old-input-identity-or-write",position,now);
}
static int combat_route_discover(uintptr_t base,uintptr_t world,DWORD now) {
    CombatRouteQA *q=&combat_route_qa;if(q->nodes_ready&&q->node_world==world)return 1;
    if(q->discovery_at&&now-q->discovery_at<1000)return 0;q->discovery_at=now;
    GameArray list={0};uintptr_t *objects=identity_object_list(base,&list);if(!objects)return 0;
    unsigned found[COMBAT_ROUTE_COUNT]={0};GuestCameraStamp nodes[COMBAT_ROUTE_COUNT];memset(nodes,0,sizeof nodes);
    for(int i=0;i<list.count;++i){uintptr_t at=objects[i];if(!at)continue;char name[96];object_name(base,at,name,sizeof name);
        for(unsigned j=0;j<COMBAT_ROUTE_COUNT;++j)if(!strcmp(name,combat_route_points[j].name)){
            GuestCameraStamp stamp;char expected[240];float xyz[3];
            snprintf(expected,sizeof expected,"BioD_ProMar_120Gatehouse.TheWorld.PersistentLevel.%s",name);
            if(!guest_camera_actor_live(base,at)||read_ptr(at+0x198)!=world||
               !is_kind_of(base,at,j+1==COMBAT_ROUTE_COUNT?"SFXNav_JumpDownNode":"PathNode")||
               !guest_camera_stamp_read(base,at,&stamp,1)||strcmp(stamp.full_name,expected)||
               !read_at(at+0x108,xyz,12)||!scene_readiness_finite3(xyz)||distance(xyz,combat_route_points[j].xyz)>1.0f)continue;
            nodes[j]=stamp;++found[j];
        }
    }HeapFree(GetProcessHeap(),0,objects);
    for(unsigned j=0;j<COMBAT_ROUTE_COUNT;++j)if(found[j]!=1)return 0;
    memcpy(q->nodes,nodes,sizeof nodes);q->node_world=world;q->nodes_ready=1;return 1;
}
static int combat_route_context(uintptr_t base,uintptr_t engine,SessionContext c,float xyz[3],float velocity[3],int32_t rotation[3],uintptr_t *input,uintptr_t *camera) {
    uintptr_t local=0;unsigned char net=255,ignored=255;uint32_t flags=0,life=0;float floor[3];int32_t disabled=-1;GameArray overrides={0};
    if(!combat_context_local_pc(base,engine,&local)||local!=c.pc||!guest_camera_actor_live(base,c.pc)||
       !is_kind_of(base,c.pc,"SFXPlayerController")||!guest_camera_actor_live(base,c.pawn)||!is_kind_of(base,c.pawn,"SFXPawn_Player")||
       read_ptr(c.pawn+0x374)!=c.pc||read_ptr(c.pawn+0x198)!=c.world||!guest_camera_actor_live(base,c.world)||
       !is_kind_of(base,c.world,"BioWorldInfo")||!narrative_probe_object(base,c.squad,"BioPlayerSquad")||read_ptr(c.squad+0x2cc)!=c.pawn||
       session_engine_blocked(c)||!progress_pawn_ready(base,c.pawn,c.pc)||!read_at(c.pawn+0xe20,&life,4)||(life&0x200)||
       !read_at(c.world+0x7c8,&net,1)||(net!=0&&net!=2)||!read_at(c.pc+0x77c,&flags,4)||(flags&(0x800|0x20000))||
       !read_at(c.pc+0x781,&ignored,1)||ignored||read_ptr(c.world+0x670)||
       !read_at(c.pawn+0x108,xyz,12)||!scene_readiness_finite3(xyz)||!read_at(c.pawn+0x138,velocity,12)||!scene_readiness_finite3(velocity)||
       !read_at(c.pawn+0x2fc,floor,12)||!scene_readiness_finite3(floor)||floor[2]<0.6f||floor[2]>1.01f||fabsf(velocity[2])>250||
       !read_at(c.pc+0x114,rotation,12))return 0;
    /* Retain the ordinary paired bot's joined-peer gate. The guest first
     * prewarms Mars; moving several kilometres before it joins strands it. */
    if(test_action==6){
        CombatContextChain chain;GameArray peers={0};combat_context_read_chain(base,&chain);
        if(net!=2||!chain.info_valid||chain.info!=c.world||read_ptr(chain.world+0x120)!=identity_host_driver||
           !identity_array(identity_host_driver+0x70,&peers,2)||peers.count!=1)return 0;
        uintptr_t connection=read_ptr(peers.data),remote=read_ptr(connection+0x68);
        if(remote==c.pc||!session_peer_connected(base,remote,c.world))return 0;
    }
    uintptr_t player=read_ptr(c.pc+0x67c);*input=read_ptr(c.pc+0x6dc);*camera=read_ptr(c.pc+0x684);
    GuestCameraStamp stamp;
    return narrative_probe_object(base,player,"LocalPlayer")&&read_ptr(player+0x68)==c.pc&&read_ptr(player+0x40)==engine&&
        narrative_probe_object(base,*input,"BioPlayerInput")&&read_ptr(*input+0x40)==c.pc&&
        read_at(*input+0x398,&disabled,4)&&disabled==0&&read_at(*input+0x2c4,&overrides,sizeof overrides)&&overrides.count==0&&
        overrides.max>=0&&overrides.max<=1024&&guest_camera_actor_live(base,*camera)&&read_ptr(*camera+0x3f4)==c.pawn&&
        guest_camera_stamp_read(base,c.world,&stamp,1)&&!_strnicmp(stamp.full_name,"BioP_ProMar.TheWorld.PersistentLevel.",37);
}
static int combat_route_same(uintptr_t base,SessionContext c) {
    CombatRouteQA *q=&combat_route_qa;
    return q->pc.pointer==c.pc&&q->pawn.pointer==c.pawn&&q->world.pointer==c.world&&q->squad.pointer==c.squad&&
        q->input.pointer==read_ptr(c.pc+0x6dc)&&q->camera.pointer==read_ptr(c.pc+0x684)&&
        read_ptr(c.pc+0x3bc)==c.pawn&&read_ptr(c.pawn+0x374)==c.pc&&read_ptr(c.pawn+0x198)==c.world&&
        read_ptr(c.squad+0x2cc)==q->story_player&&read_ptr(c.squad+0x2d4)==q->story_initial&&
        guest_camera_stamp_matches(base,&q->pc,1)&&guest_camera_stamp_matches(base,&q->pawn,1)&&
        guest_camera_stamp_matches(base,&q->world,1)&&guest_camera_stamp_matches(base,&q->squad,1)&&
        guest_camera_stamp_matches(base,&q->input,1)&&guest_camera_stamp_matches(base,&q->camera,1);
}
static void combat_route_qa_tick(uintptr_t base,uintptr_t engine,int after,DWORD now) {
    CombatRouteQA *q=&combat_route_qa;
    if(after){if(!combat_route_cleanup(base)&&q->active)combat_route_end(base,"aborted","input-cleanup",NULL,now);return;}
    if(!combat_route_qa_host_mode()||!has_marker(root)){if(q->active)combat_route_end(base,"aborted","opt-in-or-protection-ended",NULL,now);return;}
    if(q->done)return;
    if(q->active&&now-q->started>=75000){combat_route_end(base,"aborted","75-second-bound",NULL,now);return;}
    SessionContext c=session_context(engine);float xyz[3],velocity[3];int32_t rotation[3];uintptr_t input=0,camera=0;
    if(!combat_route_context(base,engine,c,xyz,velocity,rotation,&input,&camera)){
        if(q->active){
            if(!combat_route_same(base,c)){combat_route_end(base,"aborted","host-story-or-input-changed",NULL,now);return;}
            if(!combat_route_cleanup(base)){combat_route_end(base,"aborted","pause-input-cleanup",NULL,now);return;}
            /* Observed normal landing includes a brief 54-unit step down.
             * Keep axes neutral while airborne; resume only through every
             * original guard. Never carry movement across a changed owner. */
            if(!q->paused_at){q->paused_at=now;combat_route_log("paused","ordinary-ground-context-unavailable",NULL,now);}
            if(now-q->paused_at>=2000)combat_route_end(base,"aborted","two-second-context-pause",NULL,now);
        }
        else {q->stable_at=0;if(!q->report_at||now-q->report_at>=5000){q->report_at=now;combat_route_log("waiting","ordinary-host-context",NULL,now);}}
        return;
    }
    if(q->active&&q->paused_at){
        if(!combat_route_same(base,c)||now-q->paused_at>=2000){combat_route_end(base,"aborted","pause-ended-without-safe-context",xyz,now);return;}
        q->paused_at=0;q->advanced_at=now;memcpy(q->progress,xyz,12);combat_route_log("resumed","all-original-host-guards-restored",xyz,now);
    }
    if(!q->active){
        if(!combat_route_start_allowed(xyz)){
            combat_route_end(base,"rejected","unexpected-mars-start",xyz,now);return;}
        if(!combat_route_discover(base,c.world,now))return;
        if(!q->stable_at){q->stable_at=now;return;}if(now-q->stable_at<1500)return;
        if(!guest_camera_stamp_read(base,c.pc,&q->pc,1)||!guest_camera_stamp_read(base,c.pawn,&q->pawn,1)||
           !guest_camera_stamp_read(base,c.world,&q->world,1)||!guest_camera_stamp_read(base,c.squad,&q->squad,1)||
           !guest_camera_stamp_read(base,input,&q->input,1)||!guest_camera_stamp_read(base,camera,&q->camera,1))return;
        q->story_player=read_ptr(c.squad+0x2cc);q->story_initial=read_ptr(c.squad+0x2d4);
        q->active=1;q->started=q->advanced_at=now;memcpy(q->progress,xyz,12);memcpy(q->origin,xyz,12);
        /* Preserve the existing acquisition warmup against actual ordinary
         * walking, replacing only the opt-in host's old straight-line bot. */
        if(progress_enabled&&!progress_walk_at){progress_walk_at=now;memcpy(progress_host_start,xyz,12);}
        combat_route_log("started","normal-player-input-measured-nav-candidate",xyz,now);
    }
    int same=combat_route_same(base,c);
    CombatRoutePolicy policy={1,1,same,1,1,1,1,now-q->started};
    if(!combat_route_policy(&policy)){combat_route_end(base,"aborted",same?"75-second-bound":"host-story-or-input-changed",xyz,now);return;}
    if(combat_route_horizontal(xyz,combat_route_points[COMBAT_ROUTE_COUNT-1].xyz)<110){combat_route_end(base,"aborted","drop-clearance",xyz,now);return;}
    if(q->stopping){
        float speed=sqrtf(velocity[0]*velocity[0]+velocity[1]*velocity[1]);
        if(combat_route_horizontal(xyz,combat_route_stop)>65){combat_route_end(base,"aborted","stop-drift",xyz,now);return;}
        if(speed<15){if(!q->still_at)q->still_at=now;if(now-q->still_at>=500){combat_route_end(base,"approach-complete","stopped-before-drop-no-traversal",xyz,now);return;}}
        else q->still_at=0;
        if(now-q->stopped_at>=4000)combat_route_end(base,"aborted","stop-not-stable",xyz,now);return;
    }
    const float *target=q->waypoint+1==COMBAT_ROUTE_COUNT?combat_route_stop:combat_route_points[q->waypoint].xyz;
    const float *from=q->waypoint?combat_route_points[q->waypoint-1].xyz:q->origin;
    if(!combat_route_corridor(xyz,from,target)||!guest_camera_stamp_matches(base,&q->nodes[q->waypoint],1)||
       read_ptr(q->nodes[q->waypoint].pointer+0x198)!=c.world){
        combat_route_end(base,"aborted","nav-corridor-or-node-changed",xyz,now);return;}
    float remaining=combat_route_horizontal(xyz,target);float radius=q->waypoint+1==COMBAT_ROUTE_COUNT?25:70;
    if(remaining<=radius){
        if(q->waypoint+1==COMBAT_ROUTE_COUNT){q->stopping=1;q->stopped_at=now;q->axes[0]=q->axes[1]=0;combat_route_log("stopping","before-real-jump-node",xyz,now);return;}
        ++q->waypoint;q->advanced_at=now;memcpy(q->progress,xyz,12);combat_route_log("waypoint","measured-nav-candidate-reached",xyz,now);return;
    }
    if(combat_route_horizontal(xyz,q->progress)>20){q->advanced_at=now;memcpy(q->progress,xyz,12);}
    if(now-q->advanced_at>=4000){combat_route_end(base,"aborted","four-second-stuck-bound",xyz,now);return;}
    float gain=q->waypoint+1==COMBAT_ROUTE_COUNT?0.18f:0.45f;
    if(!combat_route_axes(xyz,target,rotation[1],gain,q->axes)){combat_route_end(base,"aborted","input-vector-invalid",xyz,now);return;}
    q->injected=1;q->cleanup_tries=0;if(!combat_route_write_axes(input,q->axes[0],q->axes[1])){combat_route_end(base,"aborted","input-write-failed",xyz,now);return;}
    if(!q->report_at||now-q->report_at>=1000){q->report_at=now;combat_route_log("input","ordinary-host-axes",xyz,now);}
}
static int combat_route_qa_selftest(void) {
    CombatRoutePolicy p={1,1,1,1,1,1,1,0};if(!combat_route_policy(&p)||combat_route_policy(NULL))return 0;
    int *fields[]={&p.opted,&p.authority,&p.identity,&p.ordinary,&p.camera,&p.ground,&p.input};
    for(unsigned i=0;i<sizeof fields/sizeof *fields;++i){*fields[i]=0;if(combat_route_policy(&p))return 0;*fields[i]=1;}
    p.elapsed=75000;if(combat_route_policy(&p))return 0;
    float position[3]={0,0,0},target[3]={100,0,0},out[2];
    if(!combat_route_axes(position,target,0,0.45f,out)||fabsf(out[0]-0.45f)>0.0001f||fabsf(out[1])>0.0001f)return 0;
    target[0]=0;target[1]=100;if(!combat_route_axes(position,target,16384,0.45f,out)||fabsf(out[0]-0.45f)>0.0001f||fabsf(out[1])>0.0001f)return 0;
    if(combat_route_axes(position,target,0,1,out))return 0;
    target[0]=target[1]=100;if(combat_route_axes(position,target,0,0.6f,out))return 0;
    target[1]=NAN;if(combat_route_axes(position,target,0,0.45f,out))return 0;
    float from[3]={0,0,0},to[3]={100,0,0};position[0]=50;position[1]=180;
    if(!combat_route_corridor(position,from,to))return 0;position[1]=181;if(combat_route_corridor(position,from,to))return 0;
    position[1]=0;position[2]=181;if(combat_route_corridor(position,from,to))return 0;
    float joined[3]={-21897.965f,49762.996f,279.492f};
    if(!combat_route_start_allowed(joined))return 0;
    float settled[3]={-21873.033f,49660.332f,226.492f};if(!combat_route_start_allowed(settled))return 0;
    if(!combat_route_corridor(settled,settled,combat_route_points[0].xyz))return 0;
    memcpy(joined,combat_route_start,12);joined[0]+=349;if(!combat_route_start_allowed(joined))return 0;
    memcpy(joined,combat_route_start,12);joined[2]+=101;if(combat_route_start_allowed(joined))return 0;
    memcpy(joined,combat_route_start,12);joined[1]+=351;if(combat_route_start_allowed(joined))return 0;
    return combat_route_horizontal(combat_route_stop,combat_route_points[COMBAT_ROUTE_COUNT-1].xyz)>149;
}
#endif
