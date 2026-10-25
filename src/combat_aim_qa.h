#ifndef ME3_COMBAT_AIM_QA_H
#define ME3_COMBAT_AIM_QA_H
#include <stdlib.h>
/* Protected, default-off encounter test. Normal input axes and the existing
 * FireWeapon/StopFiringWeapon APIs only. No rotation/position/health/hit writes.
 * Targets are observed Trooper replicas, not a hostility or damage oracle. */
typedef struct CombatAimView {float xyz[3];int32_t rotation[3];} CombatAimView;
_Static_assert(sizeof(CombatAimView)==24,"GetPlayerViewPoint output ABI");
typedef struct CombatAimQA {
    int done,active,phase,firing,yaw_sign,pitch_sign;DWORD scan_at,started,phase_at,stable_at,report_at,ready_at;
    int32_t calibration_angle;GuestCameraStamp pc,pawn,world,input,mode,target;
} CombatAimQA;
static CombatAimQA combat_aim_qa;
static GuestCameraStamp combat_aim_view_method;
static int32_t combat_aim_delta(int32_t a,int32_t b) {
    int64_t value=((int64_t)a-b)%65536;if(value>=32768)value-=65536;if(value< -32768)value+=65536;return (int32_t)value;
}
static float combat_aim_axis(int32_t error,int sign) {
    if(sign!=1&&sign!=-1)return 0;
    return fmaxf(-0.15f,fminf(0.15f,(float)error*0.15f/2048))*sign;
}
static int combat_aim_selftest(void) {
    return combat_aim_delta(0,65535)==1&&combat_aim_delta(65535,0)==-1&&
        combat_aim_delta(32768,0)==-32768&&combat_aim_delta(INT32_MAX,INT32_MIN)==-1&&
        combat_aim_axis(32767,1)==0.15f&&combat_aim_axis(-32768,-1)==0.15f&&combat_aim_axis(100,0)==0;
}
static void combat_aim_log(uintptr_t base,const char *event,const char *reason,DWORD now) {
    CombatAimQA *q=&combat_aim_qa;char row[2400],target[1200];identity_quote(q->target.full_name,target,sizeof target);
    snprintf(row,sizeof row,"{\"combat_aim_qa\":\"%s\",\"reason\":\"%s\",\"phase\":%d,\"target_index\":%d,\"target_path\":\"%s\",\"target_channel\":%d,\"yaw_sign\":%d,\"pitch_sign\":%d,\"elapsed_ms\":%lu,\"monotonic_ms\":%lu,\"damage_verified\":false}\r\n",
        event,reason,q->phase,q->target.index,target,network_actor_channel_id(base,q->target.pointer),q->yaw_sign,q->pitch_sign,now-q->started,now);line(row);
}
static int combat_aim_input_same(uintptr_t base) {
    CombatAimQA *q=&combat_aim_qa;
    return guest_camera_stamp_matches(base,&q->pc,1)&&guest_camera_stamp_matches(base,&q->input,1)&&
        read_ptr(q->pc.pointer+0x6dc)==q->input.pointer&&read_ptr(q->input.pointer+0x40)==q->pc.pointer;
}
static int combat_aim_write(uintptr_t base,float yaw,float pitch) {
    CombatAimQA *q=&combat_aim_qa;SIZE_T wrote=0;
    return combat_aim_input_same(base)&&isfinite(yaw)&&isfinite(pitch)&&fabsf(yaw)<=0.15001f&&fabsf(pitch)<=0.15001f&&
        WriteProcessMemory(GetCurrentProcess(),(void*)(q->input.pointer+0x1a8),&yaw,4,&wrote)&&wrote==4&&
        WriteProcessMemory(GetCurrentProcess(),(void*)(q->input.pointer+0x1b4),&pitch,4,&wrote)&&wrote==4;
}
static void combat_aim_finish(uintptr_t base,const char *reason,DWORD now) {
    CombatAimQA *q=&combat_aim_qa;int stopped=!q->firing;
    if(q->firing&&guest_camera_stamp_matches(base,&q->mode,1)&&guest_camera_stamp_matches(base,&q->pc,1)&&
       read_ptr(q->mode.pointer+0x40)==q->pc.pointer&&read_ptr(q->pc.pointer+0x198)==q->world.pointer&&
       read_ptr(q->pc.pointer+0x3bc)==q->pawn.pointer)stopped=combat_fire_dispatch(base,q->mode.pointer,0);
    combat_aim_write(base,0,0);q->firing=0;q->active=0;q->done=1;
    combat_aim_log(base,stopped?"finished":"stop-unverified",reason,now);
}
static int combat_aim_pitch_property(uintptr_t base,uintptr_t input) {
    uintptr_t klass=read_ptr(input+0x50);
    for(int depth=0;klass&&depth<16;++depth){char name[96];object_name(base,klass,name,sizeof name);
        if(!strcmp(name,"PlayerInput")){
            uintptr_t prop=read_ptr(klass+0x70);
            for(int i=0;prop&&i<1024;++i){int32_t size=0;if(!is_live(base,prop))return 0;object_name(base,prop,name,sizeof name);
                if(!strcmp(name,"aLookUp"))return combat_fire_property(base,prop,"FloatProperty","Engine.PlayerInput.aLookUp",0x1b4,4)&&read_at(prop+0x74,&size,4)&&size==4;
                prop=read_ptr(prop+0x68);
            }return 0;
        }klass=read_ptr(klass+0x60);
    }return 0;
}
static int combat_aim_view(uintptr_t base,uintptr_t pc,CombatAimView *view) {
    uintptr_t fn=combat_aim_view_method.pointer;
    if(!guest_camera_stamp_matches(base,&combat_aim_view_method,1)){
        fn=guest_player_context_find_method(base,read_ptr(pc+0x50),"BioPlayerController","GetPlayerViewPoint");
        if(!guest_camera_stamp_read(base,fn,&combat_aim_view_method,1))return 0;
    }
    if(!guest_player_context_function_abi(base,fn,"BioPlayerController","GetPlayerViewPoint",0x420902,24,65535,0,0xf52b0,2)||
       read_ptr(read_ptr(pc)+0x230)!=base+0x40d450)return 0;
    const char *names[]={"out_Location","out_rotation"};uintptr_t prop=read_ptr(fn+0x70);
    for(int i=0;i<2;++i){char path[220];int32_t size=0;snprintf(path,sizeof path,"SFXGame.BioPlayerController.GetPlayerViewPoint.%s",names[i]);
        if(!combat_fire_property(base,prop,"StructProperty",path,i*12,0x180)||!read_at(prop+0x74,&size,4)||size!=12)return 0;prop=read_ptr(prop+0x68);
    }
    for(int i=0;i<3;++i){view->xyz[i]=NAN;view->rotation[i]=INT32_MIN;}
    ((ProcessEvent)(base+0x40d450))((void*)pc,(void*)fn,view,NULL);
    return scene_readiness_finite3(view->xyz)&&view->rotation[0]!=INT32_MIN&&view->rotation[1]!=INT32_MIN;
}
static int combat_aim_target(uintptr_t base,uintptr_t target,uintptr_t world,float xyz[3]) {
    uint32_t flags=0,dead=0;unsigned char role=0;
    return guest_camera_actor_live(base,target)&&is_kind_of(base,target,"SFXPawn_Trooper")&&read_ptr(target+0x198)==world&&
        network_actor_channel_id(base,target)>=0&&read_at(target+0x26a,&role,1)&&role==1&&
        read_at(target+0x25c,&flags,4)&&!(flags&10)&&read_at(target+0xe20,&dead,4)&&!(dead&0x200)&&
        read_at(target+0x108,xyz,12)&&scene_readiness_finite3(xyz);
}
static void combat_aim_qa_tick(uintptr_t base,uintptr_t engine,int after,DWORD now) {
    CombatAimQA *q=&combat_aim_qa;static int loaded,opted;
    if(!loaded){wchar_t text[8]={0};loaded=1;opted=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_ENEMY_AIM_QA",text,8)==1&&text[0]==L'1';}
    if(after){if(q->active&&!combat_aim_write(base,0,0))combat_aim_finish(base,"post-tick-input-cleanup",now);return;}
    if(q->done||!opted)return;
    SessionContext c=session_context(engine);uintptr_t mode=0,weapon=0;const char *reason="disabled";float pos[3],target_xyz[3],velocity[3],floor[3];unsigned char ignored=255,physics=255;
    int eligible=!session_manual_guest_input()&&test_action==5&&progress_enabled&&has_marker(root)&&
        !combat_fire_pulse.active&&!session_engine_blocked(c)&&
        combat_fire_conditions(base,c.pc,c.pawn,c.world,1,1,&mode,&weapon,&reason)&&
        read_at(c.pc+0x782,&ignored,1)&&!ignored&&read_at(c.pawn+0x108,pos,12)&&scene_readiness_finite3(pos)&&pos[2]<-250&&
        read_at(c.pawn+0x268,&physics,1)&&physics==1&&read_at(c.pawn+0x138,velocity,12)&&scene_readiness_finite3(velocity)&&fabsf(velocity[2])<15&&
        read_at(c.pawn+0x2fc,floor,12)&&scene_readiness_finite3(floor)&&floor[2]>=0.6f&&floor[2]<=1.01f;
    if(!eligible){q->ready_at=0;if(q->active)combat_aim_finish(base,"ordinary-owned-fire-context-ended",now);return;}
    if(!q->active){
        if(!q->ready_at){q->ready_at=now;return;}if(now-q->ready_at<500)return;
        if(now-q->scan_at<1000)return;q->scan_at=now;
        GameArray list={0};uintptr_t *objects=identity_object_list(base,&list),target=0;float nearest=7000;
        if(!objects)return;
        for(int i=0;i<list.count;++i)if(objects[i]&&read_ptr(objects[i]+0x198)==c.world&&combat_aim_target(base,objects[i],c.world,target_xyz)){
            float d=distance(pos,target_xyz);if(d>150&&d<nearest){nearest=d;target=objects[i];}}
        HeapFree(GetProcessHeap(),0,objects);if(!target)return;
        uintptr_t input=read_ptr(c.pc+0x6dc);CombatAimView view;
        if(!combat_action_turn_property(base,input)||!combat_aim_pitch_property(base,input)){
            combat_aim_log(base,"rejected","normal-look-input-property",now);return;}
        if(!combat_aim_view(base,c.pc,&view)){combat_aim_log(base,"rejected","viewpoint-ABI-or-output",now);return;}
        if(!guest_camera_stamp_read(base,c.pc,&q->pc,1)||!guest_camera_stamp_read(base,c.pawn,&q->pawn,1)||
           !guest_camera_stamp_read(base,c.world,&q->world,1)||!guest_camera_stamp_read(base,input,&q->input,1)||
           !guest_camera_stamp_read(base,mode,&q->mode,1)||!guest_camera_stamp_read(base,target,&q->target,1))return;
        q->active=1;q->started=q->phase_at=now;q->phase=0;q->calibration_angle=view.rotation[1];
        combat_aim_log(base,"started","observed-loaded-Trooper-replica",now);
    }
    if(now-q->started>=25000||!guest_camera_stamp_matches(base,&q->pawn,1)||!guest_camera_stamp_matches(base,&q->world,1)||
       !guest_camera_stamp_matches(base,&q->mode,1)||q->pc.pointer!=c.pc||q->pawn.pointer!=c.pawn||q->world.pointer!=c.world||
       !guest_camera_stamp_matches(base,&q->target,1)||!combat_aim_target(base,q->target.pointer,c.world,target_xyz)){
        combat_aim_finish(base,"identity-target-or-25-second-bound",now);return;
    }
    CombatAimView view;if(!combat_aim_view(base,c.pc,&view)){combat_aim_finish(base,"viewpoint-ABI-or-output",now);return;}
    /* Pause only the test movement axes while measuring aim. */
    if(!combat_aim_input_same(base)||!combat_route_write_axes(q->input.pointer,0,0)){combat_aim_finish(base,"owned-movement-input",now);return;}
    float yaw=0,pitch=0;
    if(q->phase<2){
        int axis=q->phase==0?1:0;int32_t delta=combat_aim_delta(view.rotation[axis],q->calibration_angle);
        if(now-q->phase_at>=300&&abs(delta)>=16){
            if(q->phase==0)q->yaw_sign=delta>0?1:-1;else q->pitch_sign=delta>0?1:-1;
            ++q->phase;q->phase_at=now;q->calibration_angle=view.rotation[0];combat_aim_log(base,"calibrated","measured-normal-input-direction",now);
        }else if(now-q->phase_at>=1000){combat_aim_finish(base,"normal-look-input-no-response",now);return;}
        if(q->phase==0)yaw=0.1f;else if(q->phase==1)pitch=0.1f;
    }else{
        target_xyz[2]+=65;float dx=target_xyz[0]-view.xyz[0],dy=target_xyz[1]-view.xyz[1],dz=target_xyz[2]-view.xyz[2];
        float flat=sqrtf(dx*dx+dy*dy);if(!isfinite(flat)||flat<1){combat_aim_finish(base,"target-view-geometry",now);return;}
        int32_t desired_yaw=(int32_t)(atan2f(dy,dx)*10430.37835f),desired_pitch=(int32_t)(atan2f(dz,flat)*10430.37835f);
        int32_t ey=combat_aim_delta(desired_yaw,view.rotation[1]),ep=combat_aim_delta(desired_pitch,view.rotation[0]);
        yaw=combat_aim_axis(ey,q->yaw_sign);pitch=combat_aim_axis(ep,q->pitch_sign);
        if(q->phase==2){
            if(abs(ey)<=32&&abs(ep)<=32){if(!q->stable_at)q->stable_at=now;}else q->stable_at=0;
            if(q->stable_at&&now-q->stable_at>=250){
                if(!combat_fire_resolve(base,mode,now)||!combat_fire_dispatch(base,mode,1)){combat_aim_finish(base,"normal-fire-dispatch-rejected",now);return;}
                q->firing=1;q->phase=3;q->phase_at=now;combat_aim_log(base,"pressed","normal-FireWeapon-after-aim-lock",now);
                render_capture(base,engine,L"guest-progress");
            }
        }else if(now-q->phase_at>=2000){combat_aim_finish(base,"normal-two-second-fire-release",now);return;}
        if(!q->report_at||now-q->report_at>=1000){char row[500];q->report_at=now;
            snprintf(row,sizeof row,"{\"combat_aim_errors\":true,\"yaw\":%d,\"pitch\":%d,\"target_index\":%d,\"monotonic_ms\":%lu}\r\n",ey,ep,q->target.index,now);line(row);}
    }
    if(!combat_aim_write(base,yaw,pitch))combat_aim_finish(base,"normal-look-input-write",now);
}
#endif
