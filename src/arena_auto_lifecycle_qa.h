#ifndef ME3_ARENA_AUTO_LIFECYCLE_QA_H
#define ME3_ARENA_AUTO_LIFECYCLE_QA_H
/* Fixed, protected SPCer experiment. No computer input, transform, health,
 * package GUID or story-guard edits. All travel/spawn actions are native
 * ConsoleCommand; gameplay uses the existing ordinary movement/fire paths. */
#include "arena_auto_startup_qa.h"
static int arena_auto_joined,arena_auto_spawn_sent;
static DWORD arena_auto_step_at,arena_auto_state_at;
static GuestCameraStamp arena_auto_step_pc,arena_auto_step_input,arena_auto_step_pawn;
static float arena_auto_step_start[3];
static int arena_auto_step_injected;
static void arena_auto_log(const char *event,DWORD now) {
    char row[220];snprintf(row,sizeof row,"{\"arena_auto_qa\":\"%s\",\"role\":\"%s\",\"monotonic_ms\":%lu}\r\n",event,test_role,now);line(row);
}
static int arena_auto_step_same(uintptr_t base) {
    return guest_camera_stamp_matches(base,&arena_auto_step_pc,1)&&guest_camera_stamp_matches(base,&arena_auto_step_input,1)&&
        read_ptr(arena_auto_step_pc.pointer+0x6dc)==arena_auto_step_input.pointer&&
        read_ptr(arena_auto_step_input.pointer+0x40)==arena_auto_step_pc.pointer;
}
static int arena_auto_input_field(uintptr_t base,uintptr_t input,const char *name,int offset) {
    uintptr_t cls=read_ptr(input+0x50),field=guest_hit_damage_field(base,cls,"PlayerInput",name);char path[160];int32_t size=0;
    snprintf(path,sizeof path,"Engine.PlayerInput.%s",name);
    return combat_fire_property(base,field,"FloatProperty",path,offset,4)&&read_at(field+0x74,&size,4)&&size==4;
}
static int arena_auto_step_write(uintptr_t base,float value) {
    SIZE_T wrote=0;
    if(!arena_auto_step_same(base))return 0;
    int a=WriteProcessMemory(GetCurrentProcess(),(void*)(arena_auto_step_input.pointer+0x194),&value,4,&wrote)&&wrote==4;
    int b=WriteProcessMemory(GetCurrentProcess(),(void*)(arena_auto_step_input.pointer+0x1a4),&value,4,&wrote)&&wrote==4;
    return a&&b;
}
static int arena_auto_spawn_list(uintptr_t base) {
    GameArray all={0};uintptr_t *objects=identity_object_list(base,&all),list=0;
    if(!objects)return 0;
    for(int i=0;i<all.count;++i) {
        uintptr_t object=objects[i];char name[96],path[600];
        if(!object)continue;object_name(base,object,name,sizeof name);
        if(strcmp(name,"SeqVar_ObjectList")||!is_kind_of(base,object,"SeqVar_ObjectList"))continue;
        identity_path(base,object,path,sizeof path);
        if(!strcmp(path,"BioD_MPCer_100SP.TheWorld.PersistentLevel.Main_Sequence.SeqVar_ObjectList_0")){if(list){list=0;break;}list=object;}
    }
    HeapFree(GetProcessHeap(),0,objects);if(!list)return 0;
    uintptr_t field=guest_hit_damage_field(base,read_ptr(list+0x50),"SeqVar_ObjectList","ObjList");int32_t offset=-1,size=0;GameArray entries={0};
    if(!is_live(base,field)||!is_kind_of(base,field,"ArrayProperty")||!read_at(field+0x94,&offset,4)||offset!=184||
       !read_at(field+0x74,&size,4)||size!=16||!identity_array(list+offset,&entries,128)||entries.count<1)return 0;
    for(int i=0;i<entries.count;++i)if(!is_live(base,read_ptr(entries.data+i*8))||!is_kind_of(base,read_ptr(entries.data+i*8),"BioPathPoint"))return 0;
    GameArray after={0};return read_at(list+offset,&after,sizeof after)&&!memcmp(&after,&entries,sizeof after);
}
static int arena_auto_roster_ready(uintptr_t base,SessionContext c) {
    uintptr_t squad=read_ptr(c.world+0x8e8);GameArray members={0};int ashley=0,leader=0,james=0;
    if(!is_live(base,squad)||!identity_array(squad+0x270,&members,4))return 0;
    for(int i=0;i<members.count;++i){uintptr_t p=read_ptr(members.data+i*8);if(!session_actor_live(base,p))return 0;
        ashley+=is_kind_of(base,p,"SFXPawn_Ashley");leader+=p==c.pawn;james+=is_kind_of(base,p,"SFXPawn_Marine");}
    return ashley==1&&leader==1&&james==1&&members.count==3;
}
static void arena_auto_lifecycle_tick(uintptr_t base,uintptr_t engine,int after,DWORD now) {
    if(!arena_pair_qa||!has_marker(root))return;
    SessionContext c=session_context(engine);int current=arena_pair_world(base),clear=current&&!session_engine_blocked(c)&&
        session_actor_live(base,c.pawn)&&read_ptr(c.pawn+0x374)==c.pc;
    if(!after) {
        if(clear){if(!arena_auto_ready_at)arena_auto_ready_at=now;}else arena_auto_ready_at=0;
    }
    if(!arena_pair_auto_qa)return;
    if(!after&&(!arena_auto_state_at||now-arena_auto_state_at>=1000)) {
        arena_auto_state_at=now;char row[600];int roster=current&&arena_auto_roster_ready(base,c);
        snprintf(row,sizeof row,"{\"arena_auto_qa\":\"prewarm-state\",\"role\":\"%s\",\"current_spcer\":%s,\"scene_clear\":%s,\"ready_stable_ms\":%lu,\"roster_ready\":%s,\"host_map_refreshed\":%s,\"parallel_qa\":%s,\"monotonic_ms\":%lu}\r\n",
            test_role,current?"true":"false",clear?"true":"false",arena_auto_ready_at?now-arena_auto_ready_at:0,
            roster?"true":"false",host_map_refreshed?"true":"false",arena_parallel_qa?"true":"false",now);line(row);
    }
    if(after){
        if(arena_auto_step_injected){if(arena_auto_step_write(base,0))arena_auto_step_injected=0;else if(!arena_auto_input_fault){arena_auto_input_fault=1;arena_auto_movement_done=1;arena_auto_log("input-cleanup-unverified",now);}}
        arena_auto_startup_tick(base,engine,now);return;
    }
    if(test_action==5&&!arena_auto_joined&&current&&clear&&arena_auto_ready_at&&now-arena_auto_ready_at>=5000&&arena_auto_roster_ready(base,c)&&arena_join_gate_released()) {
        unsigned char net=255;read_at(c.world+0x7c8,&net,1);
        if(net==0){arena_auto_joined=1;identity_snapshot(base,"arena-client-prewarmed");wchar_t command[80];
            _snwprintf(command,80,L"open %ls:%u",direct_ip_config.peer_ipv4,(unsigned)direct_ip_config.port);
            arena_auto_log("join-after-complete-SPCer-prewarm",now);console(base,c.pc,command);return;}
    }
    int held=test_action==6?session_active:(arena_auto_joined&&session_actor_live(base,c.pawn)&&is_kind_of(base,c.pawn,"SFXPawn_Ashley"));
    if(held&&clear&&!arena_auto_movement_done&&!arena_auto_input_fault) {
        uintptr_t input=read_ptr(c.pc+0x6dc);float pos[3],velocity[3],floor[3];unsigned char physics=255,ignored=255;
        int ready=progress_pawn_ready(base,c.pawn,c.pc)&&read_at(c.pawn+0x108,pos,12)&&scene_readiness_finite3(pos)&&
            read_at(c.pawn+0x138,velocity,12)&&scene_readiness_finite3(velocity)&&fabsf(velocity[2])<15&&
            read_at(c.pawn+0x2fc,floor,12)&&scene_readiness_finite3(floor)&&floor[2]>=0.6f&&floor[2]<=1.01f&&
            read_at(c.pawn+0x268,&physics,1)&&physics==1&&read_at(c.pc+0x781,&ignored,1)&&!ignored&&
            is_live(base,input)&&is_kind_of(base,input,"PlayerInput")&&read_ptr(input+0x40)==c.pc;
        if(ready&&!arena_auto_step_at&&arena_auto_input_field(base,input,"aBaseY",0x194)&&arena_auto_input_field(base,input,"aForward",0x1a4)&&
           guest_camera_stamp_read(base,c.pc,&arena_auto_step_pc,1)&&guest_camera_stamp_read(base,c.pawn,&arena_auto_step_pawn,1)&&guest_camera_stamp_read(base,input,&arena_auto_step_input,1)){
            arena_auto_step_at=now;memcpy(arena_auto_step_start,pos,sizeof pos);arena_auto_log("bounded-walk-start",now);
        }
        if(arena_auto_step_at){
            int same=guest_camera_stamp_matches(base,&arena_auto_step_pawn,1)&&arena_auto_step_pawn.pointer==c.pawn;
            if(!ready||!same||now-arena_auto_step_at>=1500||distance(pos,arena_auto_step_start)>160){
                arena_auto_step_write(base,0);arena_auto_movement_done=1;arena_auto_log(ready&&same?"bounded-walk-end":"bounded-walk-aborted",now);
            }else {arena_auto_step_injected=1;if(!arena_auto_step_write(base,0.12f)){arena_auto_input_fault=1;arena_auto_movement_done=1;arena_auto_log("input-write-failed",now);}}
        }
    }else if(arena_auto_step_at&&!arena_auto_movement_done&&!clear){arena_auto_step_write(base,0);arena_auto_movement_done=1;arena_auto_log("bounded-walk-scene-release",now);}
    if(test_action==6&&session_active&&clear&&arena_auto_movement_done&&!arena_auto_input_fault&&!arena_auto_spawn_sent&&now-session_acquired_at>=5000) {
        if(arena_auto_spawn_list(base)){arena_auto_spawn_sent=1;wchar_t command[]=L"ce ME3SC_ArenaStart";
            arena_auto_log("normal-arena-console-event",now);console(base,c.pc,command);}
    }
}
#endif
