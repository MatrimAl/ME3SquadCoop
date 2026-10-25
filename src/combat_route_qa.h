#ifndef ME3_COMBAT_ROUTE_QA_H
#define ME3_COMBAT_ROUTE_QA_H
/* Default-off protected QA driver: ordinary host PlayerInput axes only.
 * Mars nav locations were measured privately; the approach is a candidate
 * route, not a claim of traversed terrain or combat. Stop before the real
 * jump node. A separate explicit action opt-in may dispatch the captured
 * normal binding there. Never edit actor/story/physics state.
 * Include after session_control.h; call before and after the ordinary Tick. */
typedef struct CombatRoutePoint {const char *name;float xyz[3];} CombatRoutePoint;
static const CombatRoutePoint combat_route_points[]={
    /* Measured shuttle navigation anchors. Turning directly toward road103
     * stopped near the left doorway at unchanged shuttle height.
     * Leave through the center before changing direction onto the road. */
    {"BioPathPoint_1",{-21930.008f,49803.781f,278.492f}},
    {"BioPathPoint_3",{-21924.662f,49481.430f,225.0f}},
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
#define COMBAT_ROUTE_ROAD_FIRST 2u
/* Measured route plus the ordinary landing scene exceeds the old75s QA cap. */
#define COMBAT_ROUTE_LIMIT_MS 120000u
static const char *combat_route_node_package(unsigned point) {
    return point<COMBAT_ROUTE_ROAD_FIRST?"BioD_ProMar_110Shuttle":"BioD_ProMar_120Gatehouse";
}
static const char *combat_route_node_kind(unsigned point) {
    return point<COMBAT_ROUTE_ROAD_FIRST?"BioPathPoint":point+1==COMBAT_ROUTE_COUNT?"SFXNav_JumpDownNode":"PathNode";
}
static const float combat_route_start[3]={-21960.033f,49931.332f,279.492f};
/* 150 units behind the top node; arrival radius 25 keeps the capsule away
 * from its radius-50 traversal cylinder. No drop or action-button input. */
static const float combat_route_stop[3]={-22288.13f,43674.24f,0.0f};
/* Separate lower phase: the last upper node retains its drop semantics. */
static const CombatRoutePoint combat_lower_points[]={
    {"PathNode_102",{-21876.9f,43172.7f,-380.0f}},
    {"PathNode_101",{-22437.7f,42669.7f,-380.0f}},
    {"PathNode_100",{-22970.6f,42022.4f,-380.0f}},
    {"PathNode_21",{-23363.9f,41188.6f,-380.0f}},
    {"PathNode_99",{-23991.1f,40632.9f,-381.6f}}
};
#define COMBAT_LOWER_COUNT (sizeof combat_lower_points/sizeof *combat_lower_points)
#include "combat_action_qa.h"
typedef struct CombatRoutePolicy {int opted,authority,identity,ordinary,camera,ground,input;DWORD elapsed;} CombatRoutePolicy;
static int combat_route_policy(const CombatRoutePolicy *p) {
    return p&&p->opted==1&&p->authority==1&&p->identity==1&&p->ordinary==1&&p->camera==1&&
        p->ground==1&&p->input==1&&p->elapsed<COMBAT_ROUTE_LIMIT_MS;
}
/* A real landing conversation held the ordinary gate for 13.39 seconds.
 * Positive scene evidence may wait neutrally; unknown/airborne context keeps
 * the original two-second allowance. The total route budget never restarts. */
static DWORD combat_route_pause_budget(int observed_scene) {
    return observed_scene==1?20000:2000;
}
static int combat_route_pause_within(DWORD elapsed,DWORD budget) {
    return (budget==2000||budget==20000)&&elapsed<budget;
}
static int combat_route_observed_scene(uintptr_t base,SessionContext c) {
    uint32_t pc_flags=0,world_flags=0;uintptr_t manager=0;int32_t active=0;uint32_t activating=0;
    if(!read_at(c.pc+0x77c,&pc_flags,4)||!read_at(c.world+0x7c0,&world_flags,4))return 0;
    if(pc_flags&0x20000)return 1;
    if(world_flags&0x400)return 1;
    if(c.mode==7||c.mode==8)return 1;
    return read_at(c.world+0x948,&manager,8)&&narrative_probe_object(base,manager,"BioConversationManager")&&
        read_at(manager+0xd0,&active,4)&&read_at(manager+0xe8,&activating,4)&&
        ((active>0&&active<=128)||(activating&1));
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
    /* The joined landing position varies. Bound the first input leg by the
     * same 1500-unit range as ordinary steering, relative to the validated
     * first road nav point. Capture the actual grounded start as its corridor
     * origin; later nav corridors and all context checks remain fixed. */
    return scene_readiness_finite3(position)&&combat_route_horizontal(position,combat_route_points[COMBAT_ROUTE_ROAD_FIRST].xyz)<=1500&&
        fabsf(position[2]-combat_route_points[COMBAT_ROUTE_ROAD_FIRST].xyz[2])<=180;
}
static int combat_route_first_point(const float position[3],unsigned *point) {
    if(!point||!combat_route_start_allowed(position))return 0;
    /* Already outside starts must not retrace the doorway. Keep the original
     * start domain; skip an anchor only after its measured exit plane. The
     * road additionally requires the actual lower ground height. */
    if(position[1]<=combat_route_points[1].xyz[1]+35&&fabsf(position[2]-combat_route_points[1].xyz[2])<=20)
        *point=COMBAT_ROUTE_ROAD_FIRST;
    else if(position[1]<combat_route_points[0].xyz[1]-35)*point=1;
    else *point=0;
    return 1;
}
static float combat_route_arrival_radius(unsigned point) {
    /* A wide road radius would cut the corner before clearing the doorway. */
    return point==0?20.0f:point==1?35.0f:point+1==COMBAT_ROUTE_COUNT?25.0f:70.0f;
}
static int combat_route_arrived(const float position[3],unsigned point,const float target[3]) {
    if(point>=COMBAT_ROUTE_COUNT||!scene_readiness_finite3(position)||!scene_readiness_finite3(target))return 0;
    float vertical=point==0?15.0f:point==1?20.0f:180.0f;
    return combat_route_horizontal(position,target)<=combat_route_arrival_radius(point)&&fabsf(position[2]-target[2])<=vertical;
}
/* object_name intentionally returns the FName base only. Actor instance
 * suffixes use the separate one-based Number field, not another name string. */
static int combat_route_numbered_point(const char *name,uint32_t number,unsigned *point) {
    if(!name||!number||!point)return 0;
    char numbered[96];int length=snprintf(numbered,sizeof numbered,"%s_%lu",name,(unsigned long)(number-1));
    if(length<0||length>=(int)sizeof numbered)return 0;
    for(unsigned j=0;j<COMBAT_ROUTE_COUNT;++j)if(!strcmp(numbered,combat_route_points[j].name)){*point=j;return 1;}
    return 0;
}
typedef struct CombatRouteQA {
    int loaded,opted,active,done,stopping,injected,action_approach,action_braking,turn_injected;unsigned waypoint,first_waypoint,cleanup_tries;
    DWORD stable_at,started,advanced_at,report_at,discovery_at,stopped_at,still_at,paused_at;
    GuestCameraStamp pc,pawn,world,squad,input,camera,nodes[COMBAT_ROUTE_COUNT];
    uintptr_t story_player,story_initial,node_world;float progress[3],axes[2],origin[3],turn;int nodes_ready;
    DWORD action_approach_at,pause_budget;
    int forward_trial,forward_observing;DWORD forward_at,forward_observe_at;
    float forward_origin[3],forward_bottom[3];
    int lower_trial;unsigned lower_waypoint;DWORD lower_at;
    GuestCameraStamp lower_nodes[COMBAT_LOWER_COUNT];float lower_origin[3];
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
    snprintf(row,sizeof row,"{\"combat_route_qa\":\"%s\",\"reason\":\"%s\",\"waypoint\":%u,\"position\":[%.3f,%.3f,%.3f],\"forward\":%.5f,\"strafe\":%.5f,\"turn\":%.5f,\"elapsed_ms\":%lu,\"monotonic_ms\":%lu,\"position_writes\":false,\"traversal_invoked\":false,\"combat_verified\":false}\r\n",
        event,reason,q->waypoint,position?position[0]:0,position?position[1]:0,position?position[2]:0,
        q->axes[0],q->axes[1],q->turn,q->started?now-q->started:0,now);line(row);
}
static int combat_route_write_axes(uintptr_t input,float forward,float strafe) {
    SIZE_T wrote=0;int ok=1;const uintptr_t offsets[]={0x194,0x1a4,0x1ac};const float values[]={forward,forward,strafe};
    for(unsigned i=0;i<3;++i)ok=WriteProcessMemory(GetCurrentProcess(),(void*)(input+offsets[i]),values+i,4,&wrote)&&wrote==4&&ok;
    return ok;
}
static int combat_route_cleanup(uintptr_t base) {
    CombatRouteQA *q=&combat_route_qa;if(!q->injected&&!q->turn_injected)return 1;
    int safe=guest_camera_stamp_matches(base,&q->pc,1)&&guest_camera_actor_live(base,q->pc.pointer)&&
        guest_camera_stamp_matches(base,&q->input,1)&&read_ptr(q->pc.pointer+0x6dc)==q->input.pointer&&
        read_ptr(q->input.pointer+0x40)==q->pc.pointer;
    ++q->cleanup_tries;int cleaned=safe&&combat_route_write_axes(q->input.pointer,0,0);
    if(safe&&q->turn_injected){SIZE_T wrote=0;float neutral=0;
        int turn_clean=WriteProcessMemory(GetCurrentProcess(),(void*)(q->input.pointer+0x1a8),&neutral,4,&wrote)&&wrote==4;
        cleaned=cleaned&&turn_clean;
    }
    if(cleaned||!safe||q->cleanup_tries>=3)q->injected=q->turn_injected=0;return cleaned;
}
static void combat_route_end(uintptr_t base,const char *event,const char *reason,const float *position,DWORD now) {
    CombatRouteQA *q=&combat_route_qa;int clean=combat_route_cleanup(base);
    int action_clean=combat_action_cleanup(base,now);clean=clean&&action_clean;
    combat_action_qa.active=0;q->done=1;q->active=0;
    combat_route_log(clean?event:"cleanup-unverified",clean?reason:"old-input-identity-or-write",position,now);
}
static int combat_route_discover(uintptr_t base,uintptr_t world,DWORD now) {
    CombatRouteQA *q=&combat_route_qa;if(q->nodes_ready&&q->node_world==world)return 1;
    if(q->discovery_at&&now-q->discovery_at<1000)return 0;q->discovery_at=now;
    GameArray list={0};uintptr_t *objects=identity_object_list(base,&list);if(!objects)return 0;
    unsigned found[COMBAT_ROUTE_COUNT]={0};GuestCameraStamp nodes[COMBAT_ROUTE_COUNT];memset(nodes,0,sizeof nodes);
    for(int i=0;i<list.count;++i){uintptr_t at=objects[i];if(!at)continue;char name[96];object_name(base,at,name,sizeof name);
        if(strcmp(name,"BioPathPoint")&&strcmp(name,"PathNode")&&strcmp(name,"SFXNav_JumpDownNode"))continue;
        uint32_t number=0;unsigned j=0;
        if(!read_at(at+0x4c,&number,4)||!combat_route_numbered_point(name,number,&j))continue;
        {
            GuestCameraStamp stamp;char expected[240];float xyz[3];
            snprintf(expected,sizeof expected,"%s.TheWorld.PersistentLevel.%s",combat_route_node_package(j),combat_route_points[j].name);
            if(!guest_camera_actor_live(base,at)||read_ptr(at+0x198)!=world||
               !is_kind_of(base,at,combat_route_node_kind(j))||
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
static int combat_lower_opted(void) {
    wchar_t text[8]={0};return GetEnvironmentVariableW(L"ME3_SQUAD_COOP_LOWER_ROUTE_QA",text,8)==1&&text[0]==L'1';
}
static int combat_lower_landing(const float xyz[3],const float bottom[3]) {
    return scene_readiness_finite3(xyz)&&scene_readiness_finite3(bottom)&&
        combat_route_horizontal(xyz,bottom)<=250&&fabsf(xyz[2]-bottom[2])<=15;
}
static int combat_lower_discover(uintptr_t base,uintptr_t world) {
    CombatRouteQA *q=&combat_route_qa;GameArray list={0};unsigned found[COMBAT_LOWER_COUNT]={0};
    uintptr_t *objects=identity_object_list(base,&list);if(!objects)return 0;
    GuestCameraStamp nodes[COMBAT_LOWER_COUNT];memset(nodes,0,sizeof nodes);
    for(int i=0;i<list.count;++i){uintptr_t at=objects[i];char name[96];
        if(!at||!guest_camera_actor_live(base,at)||read_ptr(at+0x198)!=world||!is_kind_of(base,at,"PathNode"))continue;
        object_name(base,at,name,sizeof name);if(strcmp(name,"PathNode"))continue;
        uint32_t number=0;if(!read_at(at+0x4c,&number,4)||!number)continue;
        char numbered[96];snprintf(numbered,sizeof numbered,"PathNode_%lu",(unsigned long)(number-1));
        for(unsigned j=0;j<COMBAT_LOWER_COUNT;++j)if(!strcmp(numbered,combat_lower_points[j].name)){
            GuestCameraStamp stamp;float xyz[3];char expected[240];
            snprintf(expected,sizeof expected,"BioD_ProMar_120Gatehouse.TheWorld.PersistentLevel.%s",numbered);
            if(guest_camera_stamp_read(base,at,&stamp,1)&&!strcmp(stamp.full_name,expected)&&
               read_at(at+0x108,xyz,12)&&scene_readiness_finite3(xyz)&&distance(xyz,combat_lower_points[j].xyz)<=2){nodes[j]=stamp;++found[j];}
        }
    }
    HeapFree(GetProcessHeap(),0,objects);
    for(unsigned j=0;j<COMBAT_LOWER_COUNT;++j)if(found[j]!=1)return 0;
    memcpy(q->lower_nodes,nodes,sizeof nodes);return 1;
}
static void combat_lower_tick(uintptr_t base,uintptr_t engine,SessionContext c,DWORD now) {
    CombatRouteQA *q=&combat_route_qa;float xyz[3],velocity[3],target[3];int32_t rotation[3];uintptr_t input=0,camera=0;
    if(!combat_lower_opted()||now-q->lower_at>=45000||!combat_route_same(base,c)){
        combat_route_end(base,"aborted","lower-identity-opt-in-or-45-second-bound",NULL,now);return;}
    if(!combat_route_context(base,engine,c,xyz,velocity,rotation,&input,&camera)){
        if(!combat_route_cleanup(base))combat_route_end(base,"aborted","lower-context-input-cleanup",NULL,now);
        return;
    }
    if(!q->lower_nodes[0].pointer&&!combat_lower_discover(base,c.world)){
        combat_route_end(base,"rejected","lower-nav-chain-not-unique-or-measured",xyz,now);return;}
    unsigned j=q->lower_waypoint;
    if(j>=COMBAT_LOWER_COUNT||!guest_camera_stamp_matches(base,&q->lower_nodes[j],1)||
       read_ptr(q->lower_nodes[j].pointer+0x198)!=c.world||!read_at(q->lower_nodes[j].pointer+0x108,target,12)||
       !scene_readiness_finite3(target)||distance(target,combat_lower_points[j].xyz)>2||
       !combat_route_corridor(xyz,q->lower_origin,target)){
        combat_route_end(base,"aborted","lower-node-or-corridor-changed",xyz,now);return;}
    if(combat_route_horizontal(xyz,target)<=70&&fabsf(xyz[2]-target[2])<=30){
        combat_route_log("lower-waypoint",combat_lower_points[j].name,xyz,now);
        if(++q->lower_waypoint>=COMBAT_LOWER_COUNT){combat_route_end(base,"lower-route-complete","measured-lower-nav-chain",xyz,now);return;}
        memcpy(q->lower_origin,xyz,12);memcpy(q->progress,xyz,12);q->advanced_at=now;return;
    }
    if(combat_route_horizontal(xyz,q->progress)>20){memcpy(q->progress,xyz,12);q->advanced_at=now;}
    if(now-q->advanced_at>=4000){combat_route_end(base,"aborted","lower-four-second-stuck-bound",xyz,now);return;}
    if(!combat_route_axes(xyz,target,rotation[1],0.45f,q->axes)){
        combat_route_end(base,"aborted","lower-normal-input-vector",xyz,now);return;}
    q->injected=1;q->cleanup_tries=0;
    if(!combat_route_write_axes(input,q->axes[0],q->axes[1])){combat_route_end(base,"aborted","lower-input-write",xyz,now);return;}
    if(!q->report_at||now-q->report_at>=1000){q->report_at=now;combat_route_log("lower-input",combat_lower_points[j].name,xyz,now);}
}
/* Explicit protected alternative to the stationary normal-key trial. */
static int combat_route_forward_opted(void) {
    wchar_t value[8]={0};DWORD length=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_FORWARD_EDGE_QA",value,8);
    return length==1&&!wcscmp(value,L"1")&&combat_action_opted();
}
/* Private bounded QA policy: input axes only; native gameplay owns traversal. */
enum {COMBAT_FORWARD_ABORT=-1,COMBAT_FORWARD_OBSERVE=0,COMBAT_FORWARD_INPUT=1,COMBAT_FORWARD_DONE=2};
static int combat_forward_step(int observing,int same,int ordinary,int walking,int action,int grounded,
        unsigned elapsed,unsigned observing_elapsed,float horizontal,float height) {
    if((observing!=0&&observing!=1)||same!=1||!isfinite(horizontal)||!isfinite(height)||
       horizontal<0||horizontal>600||fabsf(height)>600)return COMBAT_FORWARD_ABORT;
    if(elapsed>=8000)return COMBAT_FORWARD_DONE;
    if(observing)return observing_elapsed>=5000?COMBAT_FORWARD_DONE:COMBAT_FORWARD_OBSERVE;
    if(ordinary!=1||walking!=1||action!=0||grounded!=1||elapsed>=3000||horizontal>=280)
        return COMBAT_FORWARD_OBSERVE;
    return COMBAT_FORWARD_INPUT;
}
static void combat_route_forward_tick(uintptr_t base,uintptr_t engine,SessionContext c,DWORD now) {
    CombatRouteQA *q=&combat_route_qa;float xyz[3],velocity[3],floor[3];int32_t action=-1,power=-1,rotation[3];
    unsigned char physics=255;uint32_t life=0;
    int same=combat_route_same(base,c);
    if(!same||!combat_route_forward_opted()||!has_marker(root)||
       !read_at(c.pawn+0x108,xyz,12)||!scene_readiness_finite3(xyz)||
       !read_at(c.pawn+0x138,velocity,12)||!scene_readiness_finite3(velocity)||
       !read_at(c.pawn+0x2fc,floor,12)||!scene_readiness_finite3(floor)||
       !read_at(c.pawn+0x268,&physics,1)||!read_at(c.pawn+0xc20,&action,4)||
       !read_at(c.pawn+0xc28,&power,4)||!read_at(c.pawn+0xe20,&life,4)||(life&0x200)){
        combat_route_end(base,"aborted","forward-edge-identity-opt-in-or-sample",NULL,now);return;
    }
    int grounded=physics==1&&floor[2]>=0.6f&&floor[2]<=1.01f&&fabsf(velocity[2])<15;
    int ordinary=c.mode==0&&!session_engine_blocked(c)&&power==0;
    int step=combat_forward_step(q->forward_observing,1,ordinary,physics==1,action,grounded,
        now-q->forward_at,q->forward_observing?now-q->forward_observe_at:0,
        combat_route_horizontal(xyz,q->forward_origin),xyz[2]-q->forward_origin[2]);
    if(step==COMBAT_FORWARD_ABORT){combat_route_end(base,"aborted","forward-edge-transform-bound",xyz,now);return;}
    if(step==COMBAT_FORWARD_DONE){
        if(combat_lower_opted()&&ordinary&&grounded&&combat_lower_landing(xyz,q->forward_bottom)){
            if(!combat_route_cleanup(base)){combat_route_end(base,"aborted","lower-start-input-cleanup",xyz,now);return;}
            q->forward_trial=0;q->lower_trial=1;q->lower_at=q->advanced_at=now;q->lower_waypoint=0;
            memcpy(q->lower_origin,xyz,12);memcpy(q->progress,xyz,12);
            combat_route_log("lower-started","actual-grounded-drop-completed",xyz,now);return;
        }
        combat_route_end(base,"forward-edge-observation-complete","actual-action-and-landing-verdict-required",xyz,now);return;
    }
    if(step==COMBAT_FORWARD_OBSERVE){
        if(!combat_route_cleanup(base)){combat_route_end(base,"aborted","forward-edge-input-cleanup",xyz,now);return;}
        if(!q->forward_observing){q->forward_observing=1;q->forward_observe_at=now;
            combat_route_log("forward-edge-neutral","native-state-or-input-bound",xyz,now);}
        q->axes[0]=q->axes[1]=0;
        if(!q->report_at||now-q->report_at>=100){q->report_at=now;
            char row[550];snprintf(row,sizeof row,"{\"combat_forward_edge\":\"observation\",\"custom_action\":%d,\"power_action\":%d,\"physics\":%u,\"grounded\":%s,\"position\":[%.3f,%.3f,%.3f],\"elapsed_ms\":%lu,\"monotonic_ms\":%lu,\"position_written\":false}\r\n",
                action,power,physics,grounded?"true":"false",xyz[0],xyz[1],xyz[2],now-q->forward_at,now);line(row);}
        return;
    }
    uintptr_t input=0,camera=0;float checked_xyz[3],checked_velocity[3],top[3],bottom[3],target[3],turn=0;int touching=0;
    if(!combat_route_context(base,engine,c,checked_xyz,checked_velocity,rotation,&input,&camera)||
       !combat_action_jump_context(base,c.pawn,c.world,q->nodes+COMBAT_ROUTE_COUNT-1,top,bottom,&touching)||
       memcmp(bottom,q->forward_bottom,sizeof bottom)||
       !combat_action_heading(top,bottom,rotation[1],&turn,target)||turn!=0){
        combat_route_end(base,"aborted","forward-edge-normal-context-or-node",xyz,now);return;
    }
    float flat_target[3]={bottom[0],bottom[1],q->forward_origin[2]};
    if(!combat_route_corridor(checked_xyz,q->forward_origin,flat_target)||
       /* The slow approach gain moved only 29 units in the bounded trial.
        * Two seconds at road gain stopped after 167 units, still on top.
        * Bound the last approach to 3 seconds/280 units; stop immediately
        * when native falling begins. No traversal action is forced. */
       !combat_route_axes(checked_xyz,flat_target,rotation[1],0.45f,q->axes)){
        combat_route_end(base,"aborted","forward-edge-normal-axis-or-corridor",xyz,now);return;
    }
    q->injected=1;q->cleanup_tries=0;
    if(!combat_route_write_axes(input,q->axes[0],q->axes[1])){
        combat_route_end(base,"aborted","forward-edge-input-write",xyz,now);return;
    }
    if(!q->report_at||now-q->report_at>=250){q->report_at=now;combat_route_log("forward-edge-input","ordinary-bottom-direction-only",xyz,now);}
}
static void combat_route_action_approach(uintptr_t base,SessionContext c,uintptr_t input,
                                          const float xyz[3],const float velocity[3],int32_t yaw,DWORD now) {
    CombatRouteQA *q=&combat_route_qa;float top[3],bottom[3],target[3];int touching=0;
    const char *rejected=NULL;
    if(!combat_action_opted())rejected="normal-action-opt-in-ended";
    else if(!combat_action_turn_property(base,input))rejected="normal-action-turn-property";
    else if(!combat_action_jump_context(base,c.pawn,c.world,q->nodes+COMBAT_ROUTE_COUNT-1,top,bottom,&touching))
        rejected="normal-action-top-spec-or-touch-context";
    else if(!combat_action_heading(top,bottom,yaw,&q->turn,target))rejected="normal-action-heading-geometry";
    else if(!combat_action_approach_safe(combat_route_horizontal(xyz,top),xyz[2]-top[2],now-q->action_approach_at))
        rejected="normal-action-distance-height-or-time-bound";
    else if(!combat_route_corridor(xyz,combat_route_stop,target))rejected="normal-action-approach-corridor";
    if(rejected){combat_route_end(base,"aborted",rejected,xyz,now);return;}
    float speed=sqrtf(velocity[0]*velocity[0]+velocity[1]*velocity[1]),top_distance=combat_route_horizontal(xyz,top);
    /* Stop inside the actual overlap, before approaching closer than 45 units.
     * A geometric overlap alone never substitutes for Actor.Touching evidence. */
    q->action_braking=combat_action_braking(q->action_braking,touching,top_distance,q->turn);
    int in_place=q->action_braking;
    if(in_place&&speed<15){if(!q->still_at)q->still_at=now;}
    else q->still_at=0;
    if(combat_action_ready(touching,top_distance,q->turn,speed,q->still_at?now-q->still_at:0)){
        if(combat_route_forward_opted()){
            if(!combat_route_cleanup(base)){combat_route_end(base,"rejected","forward-edge-start-cleanup",xyz,now);return;}
            q->forward_trial=1;q->forward_observing=0;q->forward_at=now;q->action_approach=0;
            memcpy(q->forward_origin,xyz,12);memcpy(q->forward_bottom,bottom,12);
            combat_route_log("forward-edge-started","actual-touch-facing-and-still",xyz,now);return;
        }
        if(!combat_route_cleanup(base))combat_route_end(base,"rejected","normal-action-input-cleanup",xyz,now);
        else if(!combat_action_begin(base,c.pc,c.pawn,c.world,input,xyz,now))
            combat_route_end(base,"rejected","normal-action-setup-or-dispatch",xyz,now);
        return;
    }
    q->axes[0]=q->axes[1]=0;
    if(q->turn==0&&!in_place&&combat_route_horizontal(xyz,target)>5&&
       !combat_route_axes(xyz,target,yaw,0.08f,q->axes)){
        combat_route_end(base,"aborted","normal-action-input-vector",xyz,now);return;
    }
    q->injected=1;q->cleanup_tries=0;
    if(!combat_route_write_axes(input,q->axes[0],q->axes[1])){
        combat_route_end(base,"aborted","normal-action-approach-input-write",xyz,now);return;
    }
    if(q->turn!=0){SIZE_T wrote=0;q->turn_injected=1;
        if(!WriteProcessMemory(GetCurrentProcess(),(void*)(input+0x1a8),&q->turn,4,&wrote)||wrote!=4){
            combat_route_end(base,"aborted","normal-action-turn-input-write",xyz,now);return;
        }
    }
    if(!q->report_at||now-q->report_at>=250){q->report_at=now;
        combat_route_log("action-approach",touching?"actual-top-touch-awaiting-facing-and-rest":"ordinary-input-seeking-top-touch",xyz,now);}
}
static void combat_route_qa_tick(uintptr_t base,uintptr_t engine,int after,DWORD now) {
    CombatRouteQA *q=&combat_route_qa;
    if(after){if(!combat_route_cleanup(base)&&q->active)combat_route_end(base,"aborted","input-cleanup",NULL,now);return;}
    /* A stopped/disabled run still owns its bounded pending release. */
    if(q->done){if(combat_action_qa.pulse.pending)combat_action_cleanup(base,now);return;}
    if(collision_baseline_qa_stop_route(collision_baseline_qa_opted(),session_epoch)) {
        q->axes[0]=q->axes[1]=q->turn=0;
        combat_route_end(base,"baseline-observation-stop","second-acquisition-no-further-route",NULL,now);return;
    }
    if(!combat_route_qa_host_mode()||!has_marker(root)){if(q->active)combat_route_end(base,"aborted","opt-in-or-protection-ended",NULL,now);return;}
    if(q->active&&!q->forward_trial&&!q->lower_trial&&now-q->started>=COMBAT_ROUTE_LIMIT_MS){combat_route_end(base,"aborted","120-second-bound",NULL,now);return;}
    SessionContext c=session_context(engine);float xyz[3],velocity[3];int32_t rotation[3];uintptr_t input=0,camera=0;
    if(q->lower_trial){combat_lower_tick(base,engine,c,now);return;}
    if(q->forward_trial){combat_route_forward_tick(base,engine,c,now);return;}
    if(combat_action_qa.active){
        if(!combat_route_same(base,c)){combat_route_end(base,"aborted","host-story-or-input-changed",NULL,now);return;}
        int observing=combat_action_poll(base,now);
        if(observing<=0)combat_route_end(base,observing<0?"aborted":"action-observation-complete",
            observing<0?"action-identity-state-or-bound":"normal-binding-runtime-verdict-required",NULL,now);
        return;
    }
    if(!combat_route_context(base,engine,c,xyz,velocity,rotation,&input,&camera)){
        if(q->active){
            if(!combat_route_same(base,c)){combat_route_end(base,"aborted","host-story-or-input-changed",NULL,now);return;}
            if(!combat_route_cleanup(base)){combat_route_end(base,"aborted","pause-input-cleanup",NULL,now);return;}
            q->action_braking=0;q->still_at=0;
            /* Observed normal landing includes a brief 54-unit step down.
             * Keep axes neutral while airborne; resume only through every
             * original guard. Never carry movement across a changed owner. */
            if(!q->paused_at){
                q->paused_at=now;q->pause_budget=combat_route_pause_budget(combat_route_observed_scene(base,c));
                combat_route_log("paused",q->pause_budget==20000?"observed-engine-scene-awaiting-normal-return":"ordinary-ground-context-unavailable",NULL,now);
            }
            if(!combat_route_pause_within(now-q->paused_at,q->pause_budget))
                combat_route_end(base,"aborted",q->pause_budget==20000?"20-second-scene-pause-bound":"two-second-context-pause",NULL,now);
        }
        else {q->stable_at=0;if(!q->report_at||now-q->report_at>=5000){q->report_at=now;combat_route_log("waiting","ordinary-host-context",NULL,now);}}
        return;
    }
    if(q->active&&q->paused_at){
        if(!combat_route_same(base,c)||!combat_route_pause_within(now-q->paused_at,q->pause_budget)){combat_route_end(base,"aborted","pause-ended-without-safe-context",xyz,now);return;}
        q->paused_at=0;q->pause_budget=0;q->advanced_at=now;memcpy(q->progress,xyz,12);combat_route_log("resumed","all-original-host-guards-restored",xyz,now);
    }
    if(!q->active){
        if(!combat_route_first_point(xyz,&q->waypoint)){
            combat_route_end(base,"rejected","unexpected-mars-start",xyz,now);return;}
        if(!combat_route_discover(base,c.world,now))return;
        if(!q->stable_at){q->stable_at=now;return;}if(now-q->stable_at<1500)return;
        if(!guest_camera_stamp_read(base,c.pc,&q->pc,1)||!guest_camera_stamp_read(base,c.pawn,&q->pawn,1)||
           !guest_camera_stamp_read(base,c.world,&q->world,1)||!guest_camera_stamp_read(base,c.squad,&q->squad,1)||
           !guest_camera_stamp_read(base,input,&q->input,1)||!guest_camera_stamp_read(base,camera,&q->camera,1))return;
        q->story_player=read_ptr(c.squad+0x2cc);q->story_initial=read_ptr(c.squad+0x2d4);
        q->first_waypoint=q->waypoint;
        q->active=1;q->started=q->advanced_at=now;memcpy(q->progress,xyz,12);memcpy(q->origin,xyz,12);
        /* Preserve the existing acquisition warmup against actual ordinary
         * walking, replacing only the opt-in host's old straight-line bot. */
        if(progress_enabled&&!progress_walk_at){progress_walk_at=now;memcpy(progress_host_start,xyz,12);}
        combat_route_log("started","normal-player-input-measured-nav-candidate",xyz,now);
    }
    int same=combat_route_same(base,c);
    CombatRoutePolicy policy={1,1,same,1,1,1,1,now-q->started};
    if(!combat_route_policy(&policy)){combat_route_end(base,"aborted",same?"120-second-bound":"host-story-or-input-changed",xyz,now);return;}
    if(q->action_approach){combat_route_action_approach(base,c,input,xyz,velocity,rotation[1],now);return;}
    if(combat_route_horizontal(xyz,combat_route_points[COMBAT_ROUTE_COUNT-1].xyz)<110){combat_route_end(base,"aborted","drop-clearance",xyz,now);return;}
    if(q->stopping){
        float speed=sqrtf(velocity[0]*velocity[0]+velocity[1]*velocity[1]);
        if(combat_route_horizontal(xyz,combat_route_stop)>65){combat_route_end(base,"aborted","stop-drift",xyz,now);return;}
        if(speed<15){if(!q->still_at)q->still_at=now;if(now-q->still_at>=500){
            if(!combat_action_opted())combat_route_end(base,"approach-complete","stopped-before-drop-no-traversal",xyz,now);
            else {q->action_approach=1;q->action_braking=0;q->action_approach_at=now;q->still_at=0;
                combat_route_log("action-approach-started","explicit-normal-action-opt-in",xyz,now);}
            return;
        }}
        else q->still_at=0;
        if(now-q->stopped_at>=4000)combat_route_end(base,"aborted","stop-not-stable",xyz,now);return;
    }
    const float *target=q->waypoint+1==COMBAT_ROUTE_COUNT?combat_route_stop:combat_route_points[q->waypoint].xyz;
    const float *from=q->waypoint&&q->waypoint!=q->first_waypoint?combat_route_points[q->waypoint-1].xyz:q->origin;
    if(!combat_route_corridor(xyz,from,target)||!guest_camera_stamp_matches(base,&q->nodes[q->waypoint],1)||
       read_ptr(q->nodes[q->waypoint].pointer+0x198)!=c.world){
        combat_route_end(base,"aborted","nav-corridor-or-node-changed",xyz,now);return;}
    float remaining=combat_route_horizontal(xyz,target);
    if(combat_route_arrived(xyz,q->waypoint,target)){
        if(q->waypoint+1==COMBAT_ROUTE_COUNT){q->stopping=1;q->stopped_at=now;q->axes[0]=q->axes[1]=0;combat_route_log("stopping","before-real-jump-node",xyz,now);return;}
        ++q->waypoint;q->advanced_at=now;memcpy(q->progress,xyz,12);combat_route_log("waypoint","measured-nav-candidate-reached",xyz,now);return;
    }
    if(combat_route_horizontal(xyz,q->progress)>20){q->advanced_at=now;memcpy(q->progress,xyz,12);}
    if(now-q->advanced_at>=4000){combat_route_end(base,"aborted","four-second-stuck-bound",xyz,now);return;}
    float gain=q->waypoint+1==COMBAT_ROUTE_COUNT?0.18f:0.45f;
    if(q->waypoint<COMBAT_ROUTE_ROAD_FIRST)gain=fminf(0.30f,fmaxf(0.08f,remaining*0.003f));
    if(!combat_route_axes(xyz,target,rotation[1],gain,q->axes)){combat_route_end(base,"aborted","input-vector-invalid",xyz,now);return;}
    q->injected=1;q->cleanup_tries=0;if(!combat_route_write_axes(input,q->axes[0],q->axes[1])){combat_route_end(base,"aborted","input-write-failed",xyz,now);return;}
    if(!q->report_at||now-q->report_at>=1000){q->report_at=now;combat_route_log("input","ordinary-host-axes",xyz,now);}
}
static int combat_route_qa_selftest(void) {
    float bottom[3]={-22054.248f,43431.023f,-380},landing[3]={-21949.033f,43319.332f,-378.508f};
    if(!combat_lower_landing(landing,bottom))return 0;
    float invalid[3]={bottom[0]+251,bottom[1],bottom[2]};if(combat_lower_landing(invalid,bottom))return 0;
    invalid[0]=bottom[0];invalid[2]=bottom[2]+16;if(combat_lower_landing(invalid,bottom))return 0;
    invalid[2]=NAN;if(combat_lower_landing(invalid,bottom))return 0;
    int appearance_case[]={1,1,1,1,1,1,1,1,0,0,0};
    if(!remote_appearance_case(1,1,1,1,1,1,1,1,0,0,0))return 0;
    for(unsigned i=0;i<11;++i){int changed[11];memcpy(changed,appearance_case,sizeof changed);changed[i]=i<8?0:1;
        if(remote_appearance_case(changed[0],changed[1],changed[2],changed[3],changed[4],changed[5],changed[6],changed[7],changed[8],changed[9],changed[10]))return 0;}
    if(!(combat_forward_step(0,1,1,1,0,1,0,0,0,0)==COMBAT_FORWARD_INPUT))return 0;
    if(!(combat_forward_step(0,1,1,1,0,1,2999,0,279.99f,0)==COMBAT_FORWARD_INPUT))return 0;
    if(!(combat_forward_step(0,1,1,1,0,1,3000,0,50,0)==COMBAT_FORWARD_OBSERVE))return 0;
    if(!(combat_forward_step(0,1,1,1,0,1,0,0,280,0)==COMBAT_FORWARD_OBSERVE))return 0;
    if(!(combat_forward_step(0,1,1,0,0,1,10,0,1,0)==COMBAT_FORWARD_OBSERVE))return 0;
    if(!(combat_forward_step(0,1,1,1,30,1,10,0,1,0)==COMBAT_FORWARD_OBSERVE))return 0;
    if(!(combat_forward_step(0,1,1,1,0,0,10,0,1,0)==COMBAT_FORWARD_OBSERVE))return 0;
    if(!(combat_forward_step(0,1,0,1,0,1,10,0,1,0)==COMBAT_FORWARD_OBSERVE))return 0;
    if(!(combat_forward_step(0,0,1,1,0,1,10,0,1,0)==COMBAT_FORWARD_ABORT))return 0;
    if(!(combat_forward_step(1,0,1,1,0,1,10,0,1,0)==COMBAT_FORWARD_ABORT))return 0;
    if(!(combat_forward_step(1,1,0,0,30,0,3100,4999,267,-380)==COMBAT_FORWARD_OBSERVE))return 0;
    if(!(combat_forward_step(1,1,1,1,0,1,8000,5000,267,-380)==COMBAT_FORWARD_DONE))return 0;
    if(!(combat_forward_step(1,1,1,1,0,1,0,0,601,0)==COMBAT_FORWARD_ABORT))return 0;
    if(!(combat_forward_step(1,1,1,1,0,1,0,0,0,-601)==COMBAT_FORWARD_ABORT))return 0;
    if(!(combat_forward_step(0,1,1,1,0,1,0,0,NAN,0)==COMBAT_FORWARD_ABORT))return 0;
    if(!(combat_forward_step(0,1,1,1,0,1,0,0,0,INFINITY)==COMBAT_FORWARD_ABORT))return 0;
 uint32_t start=0xfffffff0u,now=0xba8u;
    if(!(now-start==3000u))return 0;
    if(!(combat_forward_step(0,1,1,1,0,1,now-start,0,0,0)==COMBAT_FORWARD_OBSERVE))return 0;

    int action_loaded=combat_action_qa.loaded,action_opted=combat_action_qa.opted;
    combat_action_qa.loaded=1;combat_action_qa.opted=1;
    int forced_release=session_policy_synthetic(1,30001)||session_policy_synthetic(1,18001);
    combat_action_qa.loaded=action_loaded;combat_action_qa.opted=action_opted;
    if(forced_release)return 0;
    if(session_guest_disconnect_delay(0,1)!=40000||session_guest_disconnect_delay(1,0)!=60000||
       session_guest_disconnect_delay(1,-1)!=60000||session_guest_disconnect_delay(1,1)!=150000)return 0;
    if(progress_guest_follow_limit(0)!=36000||progress_guest_follow_limit(-1)!=36000||
       progress_guest_follow_limit(1)!=145000||progress_guest_follow_limit(1)>=session_guest_disconnect_delay(1,1))return 0;
    if(combat_route_pause_budget(1)!=20000||combat_route_pause_budget(0)!=2000||combat_route_pause_budget(-1)!=2000||
       !combat_route_pause_within(13390,combat_route_pause_budget(1))||combat_route_pause_within(13390,combat_route_pause_budget(0))||
       combat_route_pause_within(20000,20000)||combat_route_pause_within(2000,2000)||combat_route_pause_within(1,75000))return 0;
    const uint32_t captured_numbers[]={2,4,104,105,113,112,111,110,109,108,107,7};unsigned point=99;
    for(unsigned i=0;i<COMBAT_ROUTE_COUNT;++i)
        if(!combat_route_numbered_point(combat_route_node_kind(i),captured_numbers[i],&point)||point!=i)return 0;
    if(strcmp(combat_route_node_package(0),"BioD_ProMar_110Shuttle")||strcmp(combat_route_node_package(1),"BioD_ProMar_110Shuttle")||
       strcmp(combat_route_node_package(COMBAT_ROUTE_ROAD_FIRST),"BioD_ProMar_120Gatehouse"))return 0;
    if(combat_route_numbered_point("PathNode",0,&point)||combat_route_numbered_point("PathNode",106,&point)||
       combat_route_numbered_point("OtherNode",104,&point)||combat_route_numbered_point("PathNode_103",104,&point)||
       combat_route_numbered_point(NULL,104,&point)||combat_route_numbered_point("PathNode",104,NULL))return 0;
    CombatRoutePolicy p={1,1,1,1,1,1,1,0};if(!combat_route_policy(&p)||combat_route_policy(NULL))return 0;
    int *fields[]={&p.opted,&p.authority,&p.identity,&p.ordinary,&p.camera,&p.ground,&p.input};
    for(unsigned i=0;i<sizeof fields/sizeof *fields;++i){*fields[i]=0;if(combat_route_policy(&p))return 0;*fields[i]=1;}
    p.elapsed=COMBAT_ROUTE_LIMIT_MS-1;if(!combat_route_policy(&p))return 0;
    p.elapsed=COMBAT_ROUTE_LIMIT_MS;if(combat_route_policy(&p))return 0;
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
    float displaced[3]={-21924.662f,49481.430f,225.0f};
    if(!combat_route_start_allowed(displaced)||!combat_route_corridor(displaced,displaced,combat_route_points[0].xyz))return 0;
    if(!combat_route_start_allowed(combat_route_start))return 0;
    if(!combat_route_first_point(combat_route_start,&point)||point!=0||
       !combat_route_first_point(settled,&point)||point!=1||
       !combat_route_first_point(displaced,&point)||point!=COMBAT_ROUTE_ROAD_FIRST||
       combat_route_first_point(combat_route_start,NULL))return 0;
    /* The stuck direct-to-road run moved left into shuttle geometry. Its
     * corrected first ordinary steering leg must instead move right/forward
     * toward the captured doorway center, independent of controller yaw. */
    if(!combat_route_axes(combat_route_start,combat_route_points[0].xyz,0,0.30f,out)||out[0]<=0||out[1]>=0)return 0;
    float threshold[3];memcpy(threshold,combat_route_points[0].xyz,12);
    threshold[0]+=40;if(combat_route_arrived(threshold,0,combat_route_points[0].xyz))return 0;
    threshold[0]-=40;if(!combat_route_arrived(threshold,0,combat_route_points[0].xyz))return 0;
    memcpy(threshold,combat_route_points[1].xyz,12);threshold[2]=279.492f;
    if(combat_route_arrived(threshold,1,combat_route_points[1].xyz)||
       !combat_route_first_point(threshold,&point)||point!=1)return 0;
    threshold[2]=225;if(!combat_route_arrived(threshold,1,combat_route_points[1].xyz))return 0;
    threshold[0]=NAN;if(combat_route_arrived(threshold,1,combat_route_points[1].xyz)||combat_route_first_point(threshold,&point))return 0;
    memcpy(joined,combat_route_points[COMBAT_ROUTE_ROAD_FIRST].xyz,12);joined[0]+=1499;if(!combat_route_start_allowed(joined))return 0;
    memcpy(joined,combat_route_points[COMBAT_ROUTE_ROAD_FIRST].xyz,12);joined[0]+=1501;if(combat_route_start_allowed(joined))return 0;
    memcpy(joined,combat_route_points[COMBAT_ROUTE_ROAD_FIRST].xyz,12);joined[2]+=180;if(!combat_route_start_allowed(joined))return 0;
    joined[2]+=1;if(combat_route_start_allowed(joined))return 0;
    memcpy(joined,combat_route_points[COMBAT_ROUTE_ROAD_FIRST].xyz,12);joined[2]-=181;if(combat_route_start_allowed(joined))return 0;
    memcpy(joined,combat_route_points[COMBAT_ROUTE_ROAD_FIRST].xyz,12);joined[0]=NAN;if(combat_route_start_allowed(joined))return 0;
    memcpy(joined,combat_route_points[COMBAT_ROUTE_ROAD_FIRST].xyz,12);joined[2]=INFINITY;if(combat_route_start_allowed(joined))return 0;
    return combat_route_horizontal(combat_route_stop,combat_route_points[COMBAT_ROUTE_COUNT-1].xyz)>149;
}
#endif
