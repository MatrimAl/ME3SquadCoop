#ifndef ME3_RUNTIME_STORY_GATE_H
#define ME3_RUNTIME_STORY_GATE_H
#include "runtime_story_gate_policy.h"
#include "runtime_story_graph.h"
#include "../third_party/minhook/include/MinHook.h"
/* Authoritative event admission plus native movement rejection. No actor
 * position, plot flag, event counter, collision shape or time is rewritten. */
typedef uint32_t (*RuntimeStoryActivate)(void*,void*,void*,int,GameArray*,int,int);
typedef uint32_t (*RuntimeStoryMove)(void*,void*,const float*,const int32_t*,uint32_t,void*);
typedef struct RuntimeStoryEvent {SessionHandle event,volume,component,world;int kind;} RuntimeStoryEvent;
typedef struct RuntimeStoryPending {
    RuntimeStoryEvent source;SessionHandle driver,instigator;
    int active,push,force,has_indices,count,indices[8];
} RuntimeStoryPending;
typedef struct RuntimeStoryGate {
    uintptr_t base;RuntimeStoryActivate activate;RuntimeStoryMove move;
    RuntimeStoryEvent entries[128];unsigned count;SessionHandle cabin,world;
    RuntimeStoryPending pending;
    DWORD scanned,logged;int attempted,installed,busy,replaying;
} RuntimeStoryGate;
static RuntimeStoryGate runtime_story_gate;
static int runtime_story_gate_enabled(void) {
    static int enabled=-1;
    if(enabled<0){wchar_t text[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_STORY_GATES",text,8);enabled=!n||(n==1&&text[0]==L'1');}
    return enabled;
}
static int runtime_story_scope(uintptr_t base,SessionContext c) {
    return runtime_story_gate_enabled()&&runtime_control_enabled&&runtime_scope_ready&&
        InterlockedCompareExchange(&g_enable_requested,0,0)&&session_actor_live(base,c.pc)&&session_actor_live(base,c.world)&&
        read_ptr(c.pc+0x198)==c.world;
}
static int runtime_story_guest(uintptr_t base,SessionContext c,uintptr_t pawn) {
    if(g_runtime_config.role==ME3_ROLE_GUEST)
        return pawn==c.pawn&&guest_player_context_basic_owned(base,c.pc,pawn,c.world);
    return g_runtime_config.role==ME3_ROLE_HOST&&session_active&&!session_fault&&!session_releasing&&
        !session_cleanup_pending&&pawn==session_pawn&&session_handle_matches(session_pawn_handle)&&
        session_handle_matches(session_pc_handle)&&read_ptr(session_remote+0x3bc)==pawn&&read_ptr(pawn+0x374)==session_remote&&
        session_peer_connected(base,session_remote,c.world)&&session_story_valid(base);
}
static int runtime_story_event_live(uintptr_t base,const RuntimeStoryEvent *e,SessionContext c) {
    return e->world.pointer==c.world&&session_handle_matches(e->world)&&session_handle_matches(e->event)&&
        session_handle_matches(e->volume)&&session_handle_matches(e->component)&&
        narrative_probe_object(base,e->event.pointer,"SequenceEvent")&&
        read_ptr(e->event.pointer+0xfc)==e->volume.pointer&&read_ptr(e->volume.pointer+0x198)==c.world&&
        read_ptr(e->volume.pointer+0x1e0)==e->component.pointer;
}
static int runtime_story_event_armed(uintptr_t event) {
    uint32_t flags=0;int count=-1,limit=-1;
    return read_at(event+0x120,&flags,4)&&(flags&1)&&
        read_at(event+0x110,&count,4)&&read_at(event+0x114,&limit,4)&&
        count>=0&&limit>=0&&(!limit||count<limit);
}
static void runtime_story_log(const char *event,int kind,DWORD now) {
    RuntimeStoryGate *g=&runtime_story_gate;
    if(g->logged&&now-g->logged<1500)return;g->logged=now;
    char row[420];snprintf(row,sizeof row,"{\"runtime_story_gate\":\"%s\",\"kind\":%d,\"position_written\":false,\"plot_written\":false,\"monotonic_ms\":%lu}\r\n",event,kind,now);line(row);
}
/* Exact native brush query ABI is resolved once. A point query with the
 * capsule's bounding extent conservatively waits for its full body to clear. */
static int runtime_story_sweep(uintptr_t base,uintptr_t volume,const float start[3],const float end[3],const float extent[3]) {
    RuntimeStoryGate *g=&runtime_story_gate;
    uintptr_t component=read_ptr(volume+0x1e0);
    if(!g->installed||!narrative_probe_object(base,volume,"Volume")||!session_actor_live(base,volume)||
       !narrative_probe_object(base,component,"BrushComponent")||
       read_ptr(read_ptr(volume)+0x7f8)!=base+0x6bbb40)return -1;
    typedef unsigned char (*Sweep)(void*,const float*,const float*,const float*);
    return ((Sweep)(base+0x6bbb40))((void*)volume,end,start,extent)?1:0;
}
static int runtime_story_human_outside(uintptr_t base,SessionContext c,uintptr_t pawn) {
    RuntimeStoryGate *g=&runtime_story_gate;float location[3],extent[3];uintptr_t component=read_ptr(pawn+0x1e0);
    float height=0,radius=0;
    if(!session_actor_live(base,pawn)||!is_kind_of(base,pawn,"BioPawn")||read_ptr(pawn+0x198)!=c.world||
       !session_handle_matches(g->cabin)||!narrative_probe_object(base,component,"CylinderComponent")||
       !runtime_power_property(base,component,"CylinderComponent","CollisionHeight","FloatProperty",0x224,4)||
       !runtime_power_property(base,component,"CylinderComponent","CollisionRadius","FloatProperty",0x228,4)||
       !read_at(component+0x224,&height,4)||!read_at(component+0x228,&radius,4)||
       !read_at(pawn+0x108,location,12)||!isfinite(radius)||!isfinite(height)||radius<=0||height<radius||height>500)return -1;
    for(unsigned i=0;i<3;++i)if(!isfinite(location[i]))return -1;
    extent[0]=extent[1]=radius;extent[2]=height;
    int overlap=runtime_story_sweep(base,g->cabin.pointer,location,location,extent);
    return overlap<0?-1:!overlap;
}
static int runtime_story_group_ready(uintptr_t base,SessionContext c) {
    uintptr_t pawns[32]={0};unsigned count=1;uint32_t humans=1,known=0,inside=0;
    pawns[0]=c.pawn;
    if(g_runtime_config.role!=ME3_ROLE_HOST||!session_actor_live(base,c.pawn)||
       !is_kind_of(base,c.pawn,"SFXPawn_Player")||read_ptr(c.pc+0x3bc)!=c.pawn||
       read_ptr(c.pawn+0x374)!=c.pc||read_ptr(c.pawn+0x198)!=c.world)return 0;
    NetworkActorScope scope;GameArray peers={0},after={0};uintptr_t connections[31]={0};
    if(!network_actor_read_scope(base,c.pawn,&scope)||scope.info!=c.world||read_ptr(scope.driver+0x80)||
       !identity_array(scope.driver+0x70,&peers,31)||
       (peers.count&&!read_at(peers.data,connections,(SIZE_T)peers.count*8)))return 0;
    for(int i=0;i<peers.count;++i) {
        unsigned slot=count++;humans|=1u<<slot;uintptr_t connection=connections[i],pc=read_ptr(connection+0x68),pawn=read_ptr(pc+0x3bc);
        if(!network_actor_connection_in_scope(base,scope.driver,connection)||!session_actor_live(base,pc)||
           !is_kind_of(base,pc,"SFXPlayerController")||read_ptr(pc+0x198)!=c.world||!session_actor_live(base,pawn)||
           read_ptr(pawn+0x374)!=pc||read_ptr(pawn+0x198)!=c.world)return 0;
        for(unsigned n=0;n<slot;++n)if(pawns[n]==pawn)return 0;pawns[slot]=pawn;
    }
    for(unsigned i=0;i<count;++i) {
        int outside=runtime_story_human_outside(base,c,pawns[i]);
        if(outside>=0){known|=1u<<i;if(!outside)inside|=1u<<i;}
    }
    if(!read_at(scope.driver+0x70,&after,sizeof after)||memcmp(&after,&peers,sizeof peers))return 0;
    for(int i=0;i<peers.count;++i)if(read_ptr(peers.data+(size_t)i*8)!=connections[i])return 0;
    return runtime_story_all_outside(humans,known,inside);
}
static RuntimeStoryEvent *runtime_story_find(uintptr_t base,SessionContext c,uintptr_t event) {
    RuntimeStoryGate *g=&runtime_story_gate;
    for(unsigned i=0;i<g->count;++i)if(g->entries[i].event.pointer==event&&runtime_story_event_live(base,&g->entries[i],c))return g->entries+i;
    return NULL;
}
static uint32_t runtime_story_activate(void *event,void *originator,void *instigator,int test,GameArray *indices,int push,int force) {
    RuntimeStoryGate *g=&runtime_story_gate;DWORD error=GetLastError();
    if(g->installed&&!g->busy&&!g->replaying&&GetCurrentThreadId()==session_game_thread) {
        SessionContext c=session_context(session_engine);RuntimeStoryEvent *source=NULL;
        if(runtime_story_scope(g->base,c))source=runtime_story_find(g->base,c,(uintptr_t)event);
        if(source&&(uintptr_t)originator==source->volume.pointer&&runtime_story_event_armed((uintptr_t)event)) {
            int guest=runtime_story_guest(g->base,c,(uintptr_t)instigator);
            if(source->kind==RUNTIME_STORY_GATE_SHEPARD&&guest) {
                runtime_story_warning_show(g->base,c,RUNTIME_STORY_WARNING_SHEPARD,GetTickCount());
                runtime_story_log("guest-story-entry-blocked",source->kind,GetTickCount());SetLastError(error);return 0;
            }
            if(source->kind==RUNTIME_STORY_GATE_GROUP_EXIT) {
                if(g_runtime_config.role==ME3_ROLE_GUEST) {
                    runtime_story_warning_show(g->base,c,RUNTIME_STORY_WARNING_SHUTTLE,GetTickCount());
                    SetLastError(error);return 0; /* Host owns this world transition. */
                }
                g->busy=1;int ready=runtime_story_group_ready(g->base,c);g->busy=0;
                if(!ready) {
                    if(!test&&push>=0&&push<=1&&force>=0&&force<=1) {
                        RuntimeStoryPending next={0};next.source=*source;next.active=1;next.push=push;next.force=force;
                        next.instigator=session_capture_handle(c.pawn);NetworkActorScope scope;
                        int valid=network_actor_read_scope(g->base,c.pawn,&scope);
                        if(valid)next.driver=session_capture_handle(scope.driver);
                        if(indices){GameArray a={0};
                            valid=valid&&read_at((uintptr_t)indices,&a,sizeof a)&&a.count>=0&&a.count<=8&&a.max>=a.count&&a.max<=32&&
                                (!a.count||(a.data&&read_at(a.data,next.indices,(SIZE_T)a.count*4)));
                            next.count=a.count;next.has_indices=1;
                            for(int i=0;valid&&i<a.count;++i)if(next.indices[i]!=0)valid=0;
                        }
                        if(valid&&session_handle_matches(next.instigator)&&session_handle_matches(next.driver))g->pending=next;
                    }
                    runtime_story_warning_show(g->base,c,RUNTIME_STORY_WARNING_SHUTTLE,GetTickCount());
                    runtime_story_log("exit-waiting-for-all-players",source->kind,GetTickCount());SetLastError(error);return 0;
                }
                /* A fresh native touch can arrive before the next tick. It
                 * consumes the same deferred request instead of replaying it. */
                if(!test&&g->pending.active&&g->pending.source.event.pointer==(uintptr_t)event)
                    g->pending.active=0;
            }
        }
    }
    SetLastError(error);return g->activate(event,originator,instigator,test,indices,push,force);
}
static uint32_t runtime_story_forward_move(RuntimeStoryGate *g,void *world,void *actor,const float *delta,
        const int32_t *rotation,uint32_t flags,void *hit,int blocked,DWORD error) {
    float zero[3]={0,0,0};SetLastError(error);
    return g->move(world,actor,blocked?zero:delta,rotation,flags,hit);
}
static uint32_t runtime_story_move(void *world,void *actor,const float *delta,const int32_t *rotation,uint32_t flags,void *hit) {
    RuntimeStoryGate *g=&runtime_story_gate;DWORD error=GetLastError();float start[3],step[3],end[3],extent[3]={0,0,0};int blocked=0;
    if(g->installed&&!g->busy&&GetCurrentThreadId()==session_game_thread&&delta) {
        SessionContext c=session_context(session_engine);
        if(runtime_story_scope(g->base,c)&&!session_engine_blocked(c)&&runtime_story_guest(g->base,c,(uintptr_t)actor)&&
           (uintptr_t)world==read_ptr(g->base+0x18b6638)&&read_at((uintptr_t)actor+0x108,start,12)&&read_at((uintptr_t)delta,step,12)) {
            int finite=1;for(unsigned n=0;n<3;++n){end[n]=start[n]+step[n];finite&=isfinite(start[n])&&isfinite(step[n])&&isfinite(end[n]);}
            uintptr_t cylinder=read_ptr((uintptr_t)actor+0x1e0);float radius=0,height=0;
            finite=finite&&narrative_probe_object(g->base,cylinder,"CylinderComponent")&&
                read_at(cylinder+0x224,&height,4)&&read_at(cylinder+0x228,&radius,4)&&
                isfinite(radius)&&isfinite(height)&&radius>0&&height>=radius&&height<500;
            extent[0]=extent[1]=radius;extent[2]=height;
            if(finite&&(fabsf(step[0])+fabsf(step[1])+fabsf(step[2])>0)) {
                g->busy=1;
                for(unsigned i=0;i<g->count&&!blocked;++i) {
                    RuntimeStoryEvent *source=g->entries+i;float bounds[7];RuntimeStoryBox box;float t=0;
                    if(source->kind!=RUNTIME_STORY_GATE_SHEPARD||
                       !read_at(source->component.pointer+0x8c,bounds,sizeof bounds))continue;
                    for(unsigned n=0;n<3;++n){box.minimum[n]=bounds[n]-bounds[n+3];box.maximum[n]=bounds[n]+bounds[n+3];}
                    if(runtime_story_capsule_box(&box,start,radius,height)!=1&&
                       runtime_story_segment_box_entry(&box,start,end,radius,height,&t)!=1)continue;
                    if(!runtime_story_event_live(g->base,source,c)||!runtime_story_event_armed(source->event.pointer)||
                       !narrative_probe_object(g->base,source->volume.pointer,"Volume"))continue;
                    /* Allow an already-inside player to leave. Exact native
                     * brush sweep refines the broad-phase box, no box wall. */
                    int was_inside=runtime_story_sweep(g->base,source->volume.pointer,start,start,extent);
                    if(was_inside==0&&runtime_story_sweep(g->base,source->volume.pointer,start,end,extent)==1)blocked=1;
                }
                g->busy=0;
                if(blocked){runtime_story_warning_show(g->base,c,RUNTIME_STORY_WARNING_SHEPARD,GetTickCount());
                    runtime_story_log("guest-movement-entry-blocked",RUNTIME_STORY_GATE_SHEPARD,GetTickCount());}
            }
        }
    }
    return runtime_story_forward_move(g,world,actor,delta,rotation,flags,hit,blocked,error);
}
static void runtime_story_discover(uintptr_t base,SessionContext c,DWORD now) {
    RuntimeStoryGate *g=&runtime_story_gate;uintptr_t world=read_ptr(base+0x18b6638);GameArray levels={0};uintptr_t loaded[64]={0};
    if(g->scanned&&now-g->scanned<500)return;g->scanned=now;g->count=0;g->cabin=(SessionHandle){0};
    if(!narrative_probe_object(base,world,"World")||!scene_readiness_array(world+0x78,&levels,64)||
       (levels.count&&!read_at(levels.data,loaded,(SIZE_T)levels.count*8)))return;
    for(int n=0;n<levels.count;++n) {
        uintptr_t level=loaded[n];GameArray actors={0};
        if(!narrative_probe_object(base,level,"Level")||!scene_readiness_array(level+0x60,&actors,100000))continue;
        for(int i=0;i<actors.count;++i) {
            uintptr_t actor=read_ptr(actors.data+(size_t)i*8);GameArray events={0};char path[600];
            if(!actor||read_ptr(actor+0x198)!=c.world||!session_actor_live(base,actor))continue;
            if(is_kind_of(base,actor,"BioTriggerVolume")) {
                identity_path(base,actor,path,sizeof path);
                if(!strcmp(path,"BioD_ProMar_110Shuttle.TheWorld.PersistentLevel.BioTriggerVolume_4"))g->cabin=session_capture_handle(actor);
            }
            if(!scene_readiness_array(actor+0xe0,&events,32))continue;
            for(int j=0;j<events.count;++j) {
                uintptr_t event=read_ptr(events.data+(size_t)j*8);
                if(!narrative_probe_object(base,event,"SequenceEvent")||read_ptr(event+0xfc)!=actor)continue;
                int kind=runtime_story_graph_classify(base,event);if(!kind)continue;
                uintptr_t component=read_ptr(actor+0x1e0);
                if(!narrative_probe_object(base,component,"PrimitiveComponent"))continue;
                int duplicate=0;for(unsigned k=0;k<g->count;++k)if(g->entries[k].event.pointer==event)duplicate=1;
                if(duplicate)continue;if(g->count>=128){g->count=0;return;}
                RuntimeStoryEvent next={session_capture_handle(event),session_capture_handle(actor),session_capture_handle(component),session_capture_handle(c.world),kind};
                if(runtime_story_event_live(base,&next,c))g->entries[g->count++]=next;
            }
        }
    }
    int changed=g->world.pointer!=c.world||!session_handle_matches(g->world);
    g->world=session_capture_handle(c.world);
    static unsigned reported_count=UINT_MAX;static uintptr_t reported_cabin;
    if(changed||reported_count!=g->count||reported_cabin!=g->cabin.pointer) {
        unsigned story=0,group=0;
        for(unsigned i=0;i<g->count;++i){story+=g->entries[i].kind==RUNTIME_STORY_GATE_SHEPARD;group+=g->entries[i].kind==RUNTIME_STORY_GATE_GROUP_EXIT;}
        char row[300];snprintf(row,sizeof row,"{\"runtime_story_gate\":\"discovery\",\"story_entries\":%u,\"group_exits\":%u,\"cabin_found\":%s,\"monotonic_ms\":%lu}\r\n",story,group,g->cabin.pointer?"true":"false",now);line(row);
        reported_count=g->count;reported_cabin=g->cabin.pointer;
    }
}
static int runtime_story_query_prepare(uintptr_t base) {
    RuntimeStoryGate *g=&runtime_story_gate;
    if(g->installed)return g->base==base;
    if(g->attempted)return 0;g->attempted=1;
    static const unsigned char activate[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x48,0x89,0x7c,0x24,0x20,0x41,0x56,0x48,0x83};
    static const unsigned char move[]={0x48,0x8b,0xc4,0x4c,0x89,0x48,0x20,0x4c,0x89,0x40,0x18,0x48,0x89,0x48,0x08,0x55,0x53,0x56,0x57,0x41,0x54,0x41,0x55,0x41};
    static const unsigned char query[]={0x48,0x8b,0xc4,0x48,0x89,0x58,0x08,0x48,0x89,0x68,0x10,0x48,0x89,0x70,0x18,0x48,0x89,0x78,0x20,0x41,0x56,0x48,0x81,0xec};
    unsigned char bytes[24];
    if(!supported_image(base)||!read_at(base+0x724670,bytes,24)||memcmp(bytes,activate,24)||
       !read_at(base+0x584770,bytes,24)||memcmp(bytes,move,24)||
       !read_at(base+0x6bbb40,bytes,24)||memcmp(bytes,query,24))goto refused;
    MH_STATUS status=MH_Initialize();if(status!=MH_OK&&status!=MH_ERROR_ALREADY_INITIALIZED)goto refused;
    LPVOID a=NULL,m=NULL;
    if(MH_CreateHook((LPVOID)(base+0x724670),(LPVOID)runtime_story_activate,&a)!=MH_OK)goto refused;
    if(MH_CreateHook((LPVOID)(base+0x584770),(LPVOID)runtime_story_move,&m)!=MH_OK) {
        MH_RemoveHook((LPVOID)(base+0x724670));goto refused;
    }
    g->base=base;g->activate=(RuntimeStoryActivate)a;g->move=(RuntimeStoryMove)m;
    if(MH_EnableHook((LPVOID)(base+0x724670))!=MH_OK)goto cleanup;
    if(MH_EnableHook((LPVOID)(base+0x584770))!=MH_OK) {
        MH_DisableHook((LPVOID)(base+0x724670));goto cleanup;
    }
    g->installed=1;line("{\"runtime_story_gate\":\"native-admission-and-movement-installed\",\"gameplay_verified\":false}\r\n");return 1;
cleanup:
    MH_RemoveHook((LPVOID)(base+0x584770));MH_RemoveHook((LPVOID)(base+0x724670));
refused:
    line("{\"runtime_story_gate\":\"native-pins-refused\"}\r\n");return 0;
}
static uint32_t runtime_story_replay_pending(RuntimeStoryGate *g,uintptr_t shepard) {
    RuntimeStoryPending saved=g->pending;g->pending.active=0;
    GameArray indices={(uintptr_t)saved.indices,saved.count,saved.count};
    g->replaying=1;DWORD error=GetLastError();
    uint32_t accepted=g->activate((void*)saved.source.event.pointer,(void*)saved.source.volume.pointer,
        (void*)shepard,0,saved.has_indices?&indices:NULL,saved.push,saved.force);
    SetLastError(error);g->replaying=0;return accepted;
}
static void runtime_story_gate_tick(uintptr_t base,uintptr_t engine,DWORD now) {
    RuntimeStoryGate *g=&runtime_story_gate;SessionContext c=session_context(engine);
    if(!runtime_story_scope(base,c)) {g->pending.active=0;g->count=0;return;}
    if(!runtime_story_query_prepare(base))return;
    runtime_story_discover(base,c,now);
    RuntimeStoryPending *p=&g->pending;
    if(!p->active)return;
    NetworkActorScope scope;
    if(!runtime_story_event_live(base,&p->source,c)||!runtime_story_event_armed(p->source.event.pointer)||!session_handle_matches(p->driver)||
       !session_handle_matches(p->instigator)||p->instigator.pointer!=c.pawn||
       !network_actor_read_scope(base,c.pawn,&scope)||scope.driver!=p->driver.pointer) {p->active=0;return;}
    if(session_engine_blocked(c)||!runtime_story_group_ready(base,c))return;
    uint32_t accepted=runtime_story_replay_pending(g,c.pawn);
    runtime_story_log(accepted?"deferred-exit-native-accepted":"deferred-exit-native-refused",RUNTIME_STORY_GATE_GROUP_EXIT,now);
}
#ifdef ME3_RUNTIME_FIXTURE
static unsigned runtime_story_fixture_calls,runtime_story_fixture_ok;
static float runtime_story_fixture_delta[3]={7,8,9};
static int32_t runtime_story_fixture_rotation[3]={11,12,13};
static unsigned char runtime_story_fixture_hit;
static uint32_t runtime_story_fixture_move(void *world,void *actor,const float *delta,const int32_t *rotation,uint32_t flags,void *hit) {
    ++runtime_story_fixture_calls;
    if((uintptr_t)world==17&&(uintptr_t)actor==19&&rotation==runtime_story_fixture_rotation&&
       flags==0x12345678&&hit==&runtime_story_fixture_hit&&GetLastError()==701&&delta&&
       ((delta==runtime_story_fixture_delta&&delta[0]==7&&delta[1]==8&&delta[2]==9)||
        (delta!=runtime_story_fixture_delta&&delta[0]==0&&delta[1]==0&&delta[2]==0)))++runtime_story_fixture_ok;
    runtime_story_fixture_hit=23;SetLastError(702);return 0x81234567;
}
static uint32_t runtime_story_fixture_activate(void *event,void *originator,void *instigator,int test,GameArray *indices,int push,int force) {
    ++runtime_story_fixture_calls;
    if((uintptr_t)event==31&&(uintptr_t)originator==37&&(uintptr_t)instigator==41&&
       test==-3&&indices==(GameArray*)43&&push==5&&force==7&&GetLastError()==703)++runtime_story_fixture_ok;
    SetLastError(704);return 0x92345678;
}
static uint32_t runtime_story_fixture_replay(void *event,void *originator,void *instigator,int test,GameArray *indices,int push,int force) {
    RuntimeStoryGate *g=&runtime_story_gate;++runtime_story_fixture_calls;
    /* Reentrant discovery/another native touch may replace global pending
     * storage; the forwarded indices must belong to the saved stack copy. */
    g->pending.indices[0]=99;
    if((uintptr_t)event==31&&(uintptr_t)originator==37&&(uintptr_t)instigator==41&&test==0&&
       indices&&indices->count==2&&indices->max==2&&indices->data!=(uintptr_t)g->pending.indices&&
       ((int*)indices->data)[0]==0&&((int*)indices->data)[1]==0&&push==1&&force==0&&
       !g->pending.active&&g->replaying&&GetLastError()==705)++runtime_story_fixture_ok;
    SetLastError(706);return 1;
}
static int runtime_story_gate_selftest(void) {
    RuntimeStoryGate saved=runtime_story_gate;DWORD error=GetLastError();
    RuntimeStoryGate *g=&runtime_story_gate;memset(g,0,sizeof *g);
    g->activate=runtime_story_fixture_activate;g->move=runtime_story_fixture_move;
    runtime_story_fixture_calls=runtime_story_fixture_ok=0;
    SetLastError(703);
    int ok=runtime_story_activate((void*)31,(void*)37,(void*)41,-3,(GameArray*)43,5,7)==0x92345678&&GetLastError()==704;
    for(int blocked=0;blocked<2;++blocked) {
        runtime_story_fixture_hit=0;SetLastError(701);
        uint32_t result=blocked?runtime_story_forward_move(g,(void*)17,(void*)19,runtime_story_fixture_delta,
            runtime_story_fixture_rotation,0x12345678,&runtime_story_fixture_hit,1,701):
            runtime_story_move((void*)17,(void*)19,runtime_story_fixture_delta,runtime_story_fixture_rotation,0x12345678,&runtime_story_fixture_hit);
        ok&=result==0x81234567&&GetLastError()==702&&runtime_story_fixture_hit==23;
    }
    g->activate=runtime_story_fixture_replay;g->pending.active=1;g->pending.source.event.pointer=31;
    g->pending.source.volume.pointer=37;g->pending.push=1;g->pending.has_indices=1;g->pending.count=2;
    SetLastError(705);ok&=runtime_story_replay_pending(g,41)==1&&GetLastError()==705&&!g->replaying&&!g->pending.active;
    ok&=runtime_story_fixture_calls==4&&runtime_story_fixture_ok==4;
    unsigned char event[0x124]={0};uint32_t flags=1;int count=0,limit=1;
    memcpy(event+0x120,&flags,4);memcpy(event+0x110,&count,4);memcpy(event+0x114,&limit,4);
    ok&=runtime_story_event_armed((uintptr_t)event);count=1;memcpy(event+0x110,&count,4);
    ok&=!runtime_story_event_armed((uintptr_t)event);limit=0;memcpy(event+0x114,&limit,4);
    ok&=runtime_story_event_armed((uintptr_t)event);flags=0;memcpy(event+0x120,&flags,4);
    ok&=!runtime_story_event_armed((uintptr_t)event);
    runtime_story_gate=saved;SetLastError(error);return ok;
}
#endif
#endif
