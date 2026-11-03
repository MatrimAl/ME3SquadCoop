#ifndef ME3_RUNTIME_STORY_GATE_POLICY_H
#define ME3_RUNTIME_STORY_GATE_POLICY_H
#include <stdint.h>
#include <math.h>
#include <wchar.h>

/* Pure decisions only. Runtime callers prove native event, world, participants
 * and collision shape identities. No map coordinates or game objects live here.
 * A grouped exit may be completed by the last human leaving, irrespective of
 * character. An ordinary story entry belongs to Shepard. */
enum RuntimeStoryGateKind {
    RUNTIME_STORY_GATE_NONE=0,
    RUNTIME_STORY_GATE_SHEPARD=1,
    RUNTIME_STORY_GATE_GROUP_EXIT=2
};
enum RuntimeStoryGateRole {
    RUNTIME_STORY_ROLE_OTHER=0,
    RUNTIME_STORY_ROLE_SHEPARD=1,
    RUNTIME_STORY_ROLE_GUEST=2
};
enum RuntimeStoryGateDecision {
    RUNTIME_STORY_ALLOW=0,
    RUNTIME_STORY_HOST_ONLY=1,
    RUNTIME_STORY_WAIT_PLAYERS=2
};
enum {RUNTIME_STORY_FLOW_GATE=1,RUNTIME_STORY_FLOW_INTERP=2};
static int runtime_story_flow_input(int kind,const wchar_t *label) {
    if(!label)return 0;
    if(kind==RUNTIME_STORY_FLOW_GATE)return !wcscmp(label,L"In");
    if(kind==RUNTIME_STORY_FLOW_INTERP)return !wcscmp(label,L"Play")||!wcscmp(label,L"Reverse");
    return 0;
}
static int runtime_story_all_outside(uint32_t humans,uint32_t known,uint32_t inside) {
    /* Missing/disconnected observations cannot be interpreted as an empty
     * cabin. The native participant snapshot supplies the current humans mask. */
    return humans&&(known&humans)==humans&&!(inside&humans);
}
static int runtime_story_gate_decide(int coop,int kind,int role,
        uint32_t humans,uint32_t known,uint32_t inside) {
    if(!coop||kind==RUNTIME_STORY_GATE_NONE)return RUNTIME_STORY_ALLOW;
    if(kind==RUNTIME_STORY_GATE_GROUP_EXIT)
        return runtime_story_all_outside(humans,known,inside)?RUNTIME_STORY_ALLOW:RUNTIME_STORY_WAIT_PLAYERS;
    if(kind==RUNTIME_STORY_GATE_SHEPARD&&role==RUNTIME_STORY_ROLE_GUEST)return RUNTIME_STORY_HOST_ONLY;
    /* Unrelated NPC/system events retain their original behavior. Runtime
     * classification must never relabel every event as a player story entry. */
    return RUNTIME_STORY_ALLOW;
}

typedef struct RuntimeStoryExitQueue {
    uint64_t generation;
    int pending;
} RuntimeStoryExitQueue;
static int runtime_story_exit_queue_step(RuntimeStoryExitQueue *q,uint64_t generation,
        int request,int scope_valid,int all_outside) {
    if(!q)return 0;
    if(scope_valid!=1||!generation){q->generation=0;q->pending=0;return 0;}
    if(q->generation!=generation){q->generation=generation;q->pending=0;}
    if(request)q->pending=1;
    if(!q->pending||all_outside!=1)return 0;
    q->pending=0;return 1;
}

typedef struct RuntimeStoryBox {float minimum[3],maximum[3];} RuntimeStoryBox;
static int runtime_story_box_valid(const RuntimeStoryBox *box) {
    if(!box)return 0;
    for(unsigned i=0;i<3;++i)
        if(!isfinite(box->minimum[i])||!isfinite(box->maximum[i])||box->minimum[i]>=box->maximum[i])return 0;
    return 1;
}
/* Conservative broad-phase overlap. A convex/native volume query is still
 * required to distinguish a non-box trigger from its AABB. -1 is unknown. */
static int runtime_story_capsule_box(const RuntimeStoryBox *box,const float center[3],float radius,float half_height) {
    if(!runtime_story_box_valid(box)||!center||!isfinite(radius)||!isfinite(half_height)||
       radius<0||half_height<radius)return -1;
    for(unsigned i=0;i<3;++i) {
        if(!isfinite(center[i]))return -1;
        float extent=i==2?half_height:radius;
        if(center[i]+extent<box->minimum[i]||center[i]-extent>box->maximum[i])return 0;
    }
    return 1;
}
/* A proposed move from outside crossing a box, including a pass completely
 * through it, can be rejected before native movement triggers Touch. Players
 * already strictly inside may leave. This function never moves a player.
 * Radius/height expansion is deliberately conservative for broad-phase use.
 * Returns -1 for untrusted geometry, 0 for no new entry, 1 with entry fraction. */
static int runtime_story_segment_box_entry(const RuntimeStoryBox *box,const float start[3],const float end[3],
        float radius,float half_height,float *fraction) {
    if(!fraction||!runtime_story_box_valid(box)||!start||!end||!isfinite(radius)||!isfinite(half_height)||
       radius<0||half_height<radius)return -1;
    double lower[3],upper[3],a[3],b[3];int inside=1,moving=0;
    for(unsigned i=0;i<3;++i) {
        if(!isfinite(start[i])||!isfinite(end[i]))return -1;
        double extent=i==2?half_height:radius;
        lower[i]=(double)box->minimum[i]-extent;upper[i]=(double)box->maximum[i]+extent;
        a[i]=start[i];b[i]=end[i];
        if(a[i]<=lower[i]||a[i]>=upper[i])inside=0;
        if(a[i]!=b[i])moving=1;
    }
    if(inside||!moving)return 0;
    double enter=0,leave=1;
    for(unsigned i=0;i<3;++i) {
        double delta=b[i]-a[i];
        if(delta==0) {if(a[i]<=lower[i]||a[i]>=upper[i])return 0;continue;}
        double first=(lower[i]-a[i])/delta,last=(upper[i]-a[i])/delta;
        if(first>last){double swap=first;first=last;last=swap;}
        if(first>enter)enter=first;
        if(last<leave)leave=last;
        if(enter>leave)return 0;
    }
    /* A boundary start moving away merely touches at t=0; it does not enter. */
    if(leave<=0||enter>1)return 0;
    *fraction=(float)enter;return 1;
}
static int runtime_story_gate_policy_selftest(void) {
    if(!runtime_story_flow_input(RUNTIME_STORY_FLOW_GATE,L"In")||
       runtime_story_flow_input(RUNTIME_STORY_FLOW_GATE,L"Open")||
       runtime_story_flow_input(RUNTIME_STORY_FLOW_GATE,L"Close")||
       runtime_story_flow_input(RUNTIME_STORY_FLOW_GATE,L"Toggle")||
       !runtime_story_flow_input(RUNTIME_STORY_FLOW_INTERP,L"Play")||
       !runtime_story_flow_input(RUNTIME_STORY_FLOW_INTERP,L"Reverse")||
       runtime_story_flow_input(RUNTIME_STORY_FLOW_INTERP,L"Stop")||
       runtime_story_flow_input(RUNTIME_STORY_FLOW_INTERP,L"Pause")||
       runtime_story_flow_input(RUNTIME_STORY_FLOW_INTERP,L"Change Dir")||
       runtime_story_flow_input(RUNTIME_STORY_FLOW_INTERP,L"play")||
       runtime_story_flow_input(RUNTIME_STORY_FLOW_GATE,NULL))return 0;
    /* A guest cannot start a story entry, while Shepard and unrelated native
     * NPC events retain their behavior. Disabling co-op restores native rules. */
    if(runtime_story_gate_decide(1,RUNTIME_STORY_GATE_SHEPARD,RUNTIME_STORY_ROLE_GUEST,3,3,0)!=RUNTIME_STORY_HOST_ONLY||
       runtime_story_gate_decide(1,RUNTIME_STORY_GATE_SHEPARD,RUNTIME_STORY_ROLE_SHEPARD,3,3,0)!=RUNTIME_STORY_ALLOW||
       runtime_story_gate_decide(1,RUNTIME_STORY_GATE_SHEPARD,RUNTIME_STORY_ROLE_OTHER,3,3,0)!=RUNTIME_STORY_ALLOW||
       runtime_story_gate_decide(0,RUNTIME_STORY_GATE_SHEPARD,RUNTIME_STORY_ROLE_GUEST,3,3,0)!=RUNTIME_STORY_ALLOW||
       runtime_story_gate_decide(1,RUNTIME_STORY_GATE_NONE,RUNTIME_STORY_ROLE_GUEST,3,3,0)!=RUNTIME_STORY_ALLOW)return 0;
    /* Guest leaves first: host occupancy postpones the original close. Host
     * leaves last: one pending native request becomes eligible exactly once. */
    RuntimeStoryExitQueue q={0};
    if(runtime_story_gate_decide(1,RUNTIME_STORY_GATE_GROUP_EXIT,RUNTIME_STORY_ROLE_GUEST,3,3,1)!=RUNTIME_STORY_WAIT_PLAYERS||
       runtime_story_exit_queue_step(&q,7,1,1,0)||!q.pending||
       runtime_story_exit_queue_step(&q,7,0,1,0)||
       !runtime_story_exit_queue_step(&q,7,0,1,runtime_story_all_outside(3,3,0))||q.pending||
       runtime_story_exit_queue_step(&q,7,0,1,1))return 0;
    /* Reverse order, three humans, missing observations, re-entry and world
     * replacement do not confuse "no sample" with "everyone outside". */
    if(runtime_story_gate_decide(1,RUNTIME_STORY_GATE_GROUP_EXIT,RUNTIME_STORY_ROLE_SHEPARD,3,3,2)!=RUNTIME_STORY_WAIT_PLAYERS||
       runtime_story_all_outside(7,7,4)||runtime_story_all_outside(3,1,0)||runtime_story_all_outside(0,0,0)||
       !runtime_story_all_outside(3,3,4))return 0;
    if(runtime_story_exit_queue_step(&q,7,1,1,0)||runtime_story_exit_queue_step(&q,8,0,1,1)||q.pending||
       runtime_story_exit_queue_step(&q,8,1,0,1)||q.pending||runtime_story_exit_queue_step(&q,0,1,1,1)||
       runtime_story_exit_queue_step(&q,8,1,1,-1)||!q.pending)return 0;
    RuntimeStoryBox box={{-2,-3,-4},{2,3,4}};float t=-1;
    float start[3]={-5,0,0},end[3]={5,0,0};
    if(runtime_story_segment_box_entry(&box,start,end,0,0,&t)!=1||fabsf(t-0.3f)>0.00001f)return 0;
    start[1]=end[1]=5;
    if(runtime_story_segment_box_entry(&box,start,end,0,0,&t)!=0)return 0;
    start[0]=0;start[1]=0;end[1]=0;
    if(runtime_story_segment_box_entry(&box,start,end,0,0,&t)!=0)return 0;
    start[0]=-2;end[0]=-5;
    if(runtime_story_segment_box_entry(&box,start,end,0,0,&t)!=0)return 0;
    end[0]=0;
    if(runtime_story_segment_box_entry(&box,start,end,0,0,&t)!=1||t!=0)return 0;
    end[0]=-2;end[1]=2;
    if(runtime_story_segment_box_entry(&box,start,end,0,0,&t)!=0)return 0;
    float center[3]={3,0,0};
    if(runtime_story_capsule_box(&box,center,1,2)!=1)return 0;
    center[0]=4;
    if(runtime_story_capsule_box(&box,center,1,2)!=0||runtime_story_capsule_box(&box,center,2,1)!=-1)return 0;
    center[0]=NAN;
    if(runtime_story_capsule_box(&box,center,1,2)!=-1)return 0;
    box.maximum[0]=box.minimum[0];
    return runtime_story_segment_box_entry(&box,start,end,0,0,&t)==-1;
}
#endif
