#ifndef ME3_COMBAT_ACTION_QA_H
#define ME3_COMBAT_ACTION_QA_H
/* Protected, default-off game-thread QA candidate. The real SpaceBar binding
 * is resolved through BioPlayerInput.DebugExecInputCommands, including its
 * Exclusive/tap/hold/release parser. No custom action, selected actor, mode,
 * transform, permission, physics or story state is assigned here. An ordinary
 * input call is evidence of dispatch only; traversal needs runtime evidence.
 * Include after combat_fire_bot.h and session_control.h. */
#pragma pack(push,4)
typedef struct CombatActionParms {GameString command;uint32_t pressed;} CombatActionParms;
#pragma pack(pop)
_Static_assert(sizeof(CombatActionParms)==20,"DebugExecInputCommands ABI");
_Static_assert(offsetof(CombatActionParms,pressed)==16,"DebugExecInputCommands bool ABI");

typedef struct CombatActionPulse {
    int attempted,pending,press_forwarded,release_forwarded,release_attempts;
    DWORD pressed_at;
} CombatActionPulse;
typedef int (*CombatActionForward)(void *context,const CombatActionParms *parms);
static int combat_action_call(CombatActionForward forward,void *context,int pressed) {
    wchar_t alias[]=L"PC_Action";
    CombatActionParms parms={{alias,10,10},pressed?1u:0u};
    return forward&&forward(context,&parms);
}
static int combat_action_press(CombatActionPulse *p,int opted,int safe,DWORD now,CombatActionForward forward,void *context) {
    if(!p||p->attempted||opted!=1||safe!=1)return 0;
    /* Record cleanup before forwarding, including a failed/uncertain attempt. */
    p->attempted=1;p->pending=1;p->pressed_at=now;
    p->press_forwarded=combat_action_call(forward,context,1);return p->press_forwarded;
}
/* 1=release forwarded, 0=not due or bounded retry, -1=unsafe/exhausted.
 * Never send a delayed release to a replacement input object. */
static int combat_action_release(CombatActionPulse *p,DWORD now,int force,int safe,CombatActionForward forward,void *context) {
    if(!p||!p->pending)return 0;
    if(!force&&now-p->pressed_at<100)return 0;
    if(safe!=1){p->pending=0;return -1;}
    ++p->release_attempts;
    if(combat_action_call(forward,context,0)){p->pending=0;p->release_forwarded=1;return 1;}
    if(p->release_attempts>=3){p->pending=0;return -1;}return 0;
}

#define COMBAT_ACTION_SCOPE_COUNT 6
enum {COMBAT_ACTION_PC,COMBAT_ACTION_PAWN,COMBAT_ACTION_WORLD,COMBAT_ACTION_INPUT,COMBAT_ACTION_MANAGER,COMBAT_ACTION_MODE};
typedef struct CombatActionQA {
    int loaded,opted,active;CombatActionPulse pulse;
    GuestCameraStamp scope[COMBAT_ACTION_SCOPE_COUNT];GuestPlayerContextMethod method;
    DWORD report_at;float origin[3];
} CombatActionQA;
static CombatActionQA combat_action_qa;
static const wchar_t combat_action_binding[]=L"Exclusive TryStandingJump | Exclusive Used | OnRelease StormOff | OnHold 0.2 StormOn | Exclusive PressAction | OnTap 0.3 TapAction | OnHold 0.3 HoldAction";
static int combat_action_opted(void) {
    CombatActionQA *q=&combat_action_qa;
    if(!q->loaded){wchar_t value[8]={0};q->loaded=1;DWORD length=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_COMBAT_ACTION_QA",value,8);q->opted=length==1&&!wcscmp(value,L"1");}
    return q->opted;
}
static int combat_action_scope_equal(const GuestCameraStamp *saved,const GuestCameraStamp *current,int reciprocal) {
    if(reciprocal!=1)return 0;
    for(unsigned i=0;i<COMBAT_ACTION_SCOPE_COUNT;++i)
        if(!guest_camera_stamp_core_equal(saved+i,current+i)||strcmp(saved[i].full_name,current[i].full_name))return 0;
    return 1;
}
static int combat_action_array(uintptr_t address,GameArray *out,int limit) {
    return read_at(address,out,sizeof *out)&&out->count>=0&&out->count<=limit&&out->max>=out->count&&out->max<=limit&&(!out->count||out->data);
}
static int combat_action_scope(uintptr_t base,int ordinary) {
    CombatActionQA *q=&combat_action_qa;GuestCameraStamp fresh[COMBAT_ACTION_SCOPE_COUNT];GameArray modes={0};unsigned char mode=255;
    for(unsigned i=0;i<COMBAT_ACTION_SCOPE_COUNT;++i)if(!guest_camera_stamp_read(base,q->scope[i].pointer,fresh+i,1))return 0;
    uintptr_t pc=fresh[COMBAT_ACTION_PC].pointer,pawn=fresh[COMBAT_ACTION_PAWN].pointer,world=fresh[COMBAT_ACTION_WORLD].pointer;
    uintptr_t input=fresh[COMBAT_ACTION_INPUT].pointer,manager=fresh[COMBAT_ACTION_MANAGER].pointer,normal=fresh[COMBAT_ACTION_MODE].pointer;
    int reciprocal=guest_camera_actor_live(base,pc)&&guest_camera_actor_live(base,pawn)&&guest_camera_actor_live(base,world)&&
        read_ptr(pc+0x3bc)==pawn&&read_ptr(pawn+0x374)==pc&&read_ptr(pawn+0x198)==world&&
        read_ptr(pc+0x6dc)==input&&read_ptr(input+0x40)==pc&&read_ptr(pc+0xc38)==manager&&read_ptr(manager+0x40)==pc&&
        read_ptr(normal+0x40)==pc&&combat_action_array(manager+0x70,&modes,32)&&modes.count>0&&read_ptr(modes.data)==normal&&
        read_at(manager+0xa8,&mode,1)&&mode<modes.count&&(!ordinary||mode==0);
    return combat_action_scope_equal(q->scope,fresh,reciprocal);
}
static int combat_action_pin(uintptr_t address,const unsigned char *expected,size_t size) {
    unsigned char code[48];return size<=sizeof code&&read_at(address,code,size)&&!memcmp(code,expected,size);
}
static int combat_action_pins(uintptr_t base,uintptr_t input,uintptr_t manager) {
    static const unsigned char wrapper[]={0x4c,0x8b,0xdc,0x57,0x48,0x83,0xec,0x40,0x49,0xc7,0x43,0xd8,0xfe,0xff,0xff,0xff,
        0x49,0x89,0x5b,0x08,0x49,0x89,0x6b,0x18,0x49,0x89,0x73,0x20,0x48,0x8b,0xda,0x48};
    static const unsigned char pe[]={0x40,0x55,0x41,0x56,0x41,0x57,0x48,0x81,0xec,0x90,0,0,0,0x48,0x8d,0x6c};
    static const unsigned char exec[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57};
    static const unsigned char alias[]={0x48,0x8b,0xc4,0x55,0x53,0x57,0x41,0x56,0x48,0x8d,0xa8,0x48,0xfe,0xff,0xff,0x48};
    static const unsigned char parser[]={0x48,0x8b,0xc4,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x8d};
    static const unsigned char manager_exec[]={0x48,0x8b,0x41,0x70,0x44,0x0f,0xb6,0x91,0xa8,0,0,0,0x4a,0x8b,0x0c,0xd0};
    uintptr_t vt=read_ptr(input),mt=read_ptr(manager);
    return supported_image(base)&&combat_context_layout(base)&&read_ptr(vt+0x230)==base+0xf4f80&&
        read_ptr(vt+0x268)==base+0xa63360&&read_ptr(vt+0x2e8)==base+0xa52b10&&read_ptr(vt+0x2f8)==base+0xa52fa0&&
        read_ptr(mt+0x268)==base+0xbb4780&&combat_action_pin(base+0xb752a0,wrapper,sizeof wrapper)&&
        combat_action_pin(base+0xf4f80,pe,sizeof pe)&&combat_action_pin(base+0xa63360,exec,sizeof exec)&&
        combat_action_pin(base+0xa52b10,alias,sizeof alias)&&combat_action_pin(base+0xa52fa0,parser,sizeof parser)&&
        combat_action_pin(base+0xbb4780,manager_exec,sizeof manager_exec);
}
static int combat_action_method(uintptr_t base,const GuestPlayerContextMethod *method) {
    uintptr_t fn=method->function.pointer;GameArray script={0};unsigned char code[20];uint64_t mask=0;int32_t size=0;
    if(!guest_player_context_method_matches(base,method,1)||strcmp(method->owner.full_name,"SFXGame.BioPlayerInput")||
       !guest_player_context_function_abi(base,fn,"BioPlayerInput","DebugExecInputCommands",0x20401,20,65535,0,0xb752a0,2)||
       !combat_action_array(fn+0x7c,&script,4096)||script.count!=20||!read_at(script.data,code,sizeof code)||
       code[0]!=0x29||code[9]!=0x29||code[18]!=0x0b||code[19]!=0x53)return 0;
    uintptr_t cmd=combat_fire_operand(code,1),button=combat_fire_operand(code,10);
    return read_ptr(cmd+0x40)==fn&&read_ptr(button+0x40)==fn&&
        combat_fire_property(base,cmd,"StrProperty","SFXGame.BioPlayerInput.DebugExecInputCommands.Cmd",0,0x400080)&&
        read_at(cmd+0x74,&size,4)&&size==16&&
        combat_fire_property(base,button,"BoolProperty","SFXGame.BioPlayerInput.DebugExecInputCommands.bButtonPressed",16,0x80)&&
        read_at(button+0x74,&size,4)&&size==4&&read_at(button+0xd0,&mask,8)&&mask==1;
}
static int combat_action_bindings(uintptr_t base,uintptr_t mode) {
    unsigned found[2]={0};
    /* Localized overrides must not shadow either key. Reject duplicates or
     * modifier-dependent alternatives rather than guessing lookup priority. */
    for(unsigned table=0;table<2;++table){GameArray binds={0},again={0};uintptr_t address=mode+0xa8+table*16;
        if(!combat_action_array(address,&binds,256))return 0;
        for(int i=0;i<binds.count;++i){uintptr_t row=binds.data+(uintptr_t)i*0x1c;uint32_t key[2],flags=0;char name[96];
            if(!read_at(row+16,key,8))return 0;name_of(base,key[0],name,sizeof name);
            int which=!strcmp(name,"SpaceBar")?0:!strcmp(name,"PC_Action")?1:-1;if(which<0)continue;
            GameString value={0};wchar_t command[256]={0};const wchar_t *expected=which?combat_action_binding:L"PC_Action";
            size_t length=wcslen(expected)+1;
            if(table||key[1]||!read_at(row+24,&flags,4)||flags||!read_at(row,&value,sizeof value)||!value.data||
               value.count!=(int)length||value.max<value.count||value.max>4096||length>256||
               !read_at((uintptr_t)value.data,command,length*sizeof(wchar_t))||memcmp(command,expected,length*sizeof(wchar_t)))return 0;
            ++found[which];
        }
        if(!read_at(address,&again,sizeof again)||memcmp(&binds,&again,sizeof binds))return 0;
    }
    return found[0]==1&&found[1]==1;
}
static int combat_action_turn_property(uintptr_t base,uintptr_t input) {
    uintptr_t klass=read_ptr(input+0x50);int depth=0;
    while(is_live(base,klass)&&depth++<16){char name[96];object_name(base,klass,name,sizeof name);
        if(!strcmp(name,"PlayerInput")){
            uintptr_t property=read_ptr(klass+0x70);int count=0;
            while(property&&count++<1024){int32_t size=0;
                if(!is_live(base,property))return 0;object_name(base,property,name,sizeof name);
                if(!strcmp(name,"aTurn"))return combat_fire_property(base,property,"FloatProperty","Engine.PlayerInput.aTurn",0x1a8,4)&&
                    read_ptr(property+0x40)==klass&&read_at(property+0x74,&size,4)&&size==4;
                property=read_ptr(property+0x68);
            }return 0;
        }klass=read_ptr(klass+0x60);
    }return 0;
}
static int combat_action_touching_target(const uintptr_t *actors,int count,uintptr_t target) {
    if(!actors||!target||count<0||count>256)return 0;int found=0;
    for(int i=0;i<count;++i)if(actors[i]==target)++found;
    return found==1;
}
/* Native selector uses controller forward projected horizontally. The QA
 * candidate uses a narrower facing cone and approaches behind the real top. */
static int combat_action_heading(const float top[3],const float bottom[3],int32_t yaw,float *turn,float target[3]) {
    *turn=0;if(!scene_readiness_finite3(top)||!scene_readiness_finite3(bottom))return 0;
    float dx=bottom[0]-top[0],dy=bottom[1]-top[1],length=sqrtf(dx*dx+dy*dy);
    if(!isfinite(length)||length<50||length>500||bottom[2]-top[2]>-250||bottom[2]-top[2]<-600)return 0;
    target[0]=top[0]-60*dx/length;target[1]=top[1]-60*dy/length;target[2]=top[2];
    float desired=atan2f(dy,dx)/0.00009587379924285f,error=desired-(float)(yaw%65536);
    while(error>32768)error-=65536;while(error < -32768)error+=65536;
    if(fabsf(error)>512)*turn=fmaxf(-128,fminf(128,error*0.03f));
    return isfinite(*turn);
}
/* These are eligibility observations only. The engine's own bound-command
 * parser and native selector still decide whether an ordinary tap traverses. */
static int combat_action_approach_safe(float top_distance,float height,DWORD elapsed) {
    return isfinite(top_distance)&&isfinite(height)&&top_distance>=45&&top_distance<=230&&
        fabsf(height)<=120&&elapsed<10000;
}
static int combat_action_ready(int touching,float top_distance,float turn,float speed,DWORD still) {
    return touching==1&&isfinite(top_distance)&&top_distance>=45&&top_distance<=80&&
        isfinite(turn)&&turn==0&&isfinite(speed)&&speed<15&&still>=250;
}
/* Recorded contact fluctuated79.8..80.8 units. Once braking starts at the
 * original80-unit boundary, keep axes neutral through a2-unit contact margin.
 * This never widens combat_action_ready or substitutes geometry for Touching. */
static int combat_action_braking(int previous,int touching,float top_distance,float turn) {
    return (previous==0||previous==1)&&touching==1&&isfinite(top_distance)&&
        top_distance>=45&&top_distance<=(previous?82:80)&&isfinite(turn)&&turn==0;
}
static int combat_action_jump_context(uintptr_t base,uintptr_t pawn,uintptr_t world,
                                      const GuestCameraStamp *saved_top,float top_xyz[3],float bottom_xyz[3],int *touching) {
    *touching=0;GameArray paths={0},contacts={0},again={0};uintptr_t actors[256];uint32_t top_flags=0;
    if(!guest_camera_stamp_matches(base,saved_top,1)||!guest_camera_actor_live(base,saved_top->pointer)||
       strcmp(saved_top->full_name,"BioD_ProMar_120Gatehouse.TheWorld.PersistentLevel.SFXNav_JumpDownNode_6")||
       !is_kind_of(base,saved_top->pointer,"SFXNav_JumpDownNode")||read_ptr(saved_top->pointer+0x198)!=world||
       !read_at(saved_top->pointer+0x108,top_xyz,12)||!scene_readiness_finite3(top_xyz)||
       !read_at(saved_top->pointer+0x398,&top_flags,4)||(top_flags&1)!=1||
       !combat_action_array(saved_top->pointer+0x2ac,&paths,32)||paths.count<1||
       !combat_action_array(pawn+0xb0,&contacts,256)||
       (contacts.count&&!read_at(contacts.data,actors,(size_t)contacts.count*8)))return 0;
    *touching=combat_action_touching_target(actors,contacts.count,saved_top->pointer);
    if(!read_at(pawn+0xb0,&again,sizeof again)||memcmp(&contacts,&again,sizeof contacts))return 0;
    uintptr_t bottom=read_ptr(saved_top->pointer+0x380),cylinder=read_ptr(saved_top->pointer+0x388);
    GuestCameraStamp stamp;float bounds[7];
    if(!guest_camera_actor_live(base,bottom)||read_ptr(bottom+0x198)!=world||
       !guest_camera_stamp_read(base,bottom,&stamp,1)||
       strcmp(stamp.full_name,"BioD_ProMar_120Gatehouse.TheWorld.PersistentLevel.SFXNav_JumpDownNode_7")||
       !is_kind_of(base,bottom,"SFXNav_JumpDownNode")||!read_at(bottom+0x108,bottom_xyz,12)||
       !scene_readiness_finite3(bottom_xyz)||!narrative_probe_object(base,cylinder,"CylinderComponent")||
       read_ptr(cylinder+0x40)!=saved_top->pointer||read_ptr(cylinder+0x78)!=saved_top->pointer||
       !read_at(cylinder+0x8c,bounds,sizeof bounds)||!scene_readiness_finite3(bounds)||
       !isfinite(bounds[3])||!isfinite(bounds[4])||!isfinite(bounds[5])||!isfinite(bounds[6])||
       distance(bounds,top_xyz)>1||fabsf(bounds[3]-50)>0.1f||fabsf(bounds[4]-50)>0.1f||fabsf(bounds[5]-90)>0.1f)return 0;
    const float expected_top[3]={-22184.104f,43566.086f,0},expected_bottom[3]={-22054.25f,43431.02f,-380};
    if(distance(top_xyz,expected_top)>1||distance(bottom_xyz,expected_bottom)>1)return 0;
    unsigned found=0;
    for(int i=0;i<paths.count;++i){uintptr_t spec=read_ptr(paths.data+(uintptr_t)i*8);uint32_t flags=0;unsigned char pruned=255;int32_t radius=0,height=0,blocked=-1;
        if(!narrative_probe_object(base,spec,"SFXJumpDownReachSpec"))continue;
        if(!guest_camera_stamp_read(base,spec,&stamp,1)||
           strcmp(stamp.full_name,"BioD_ProMar_120Gatehouse.TheWorld.PersistentLevel.SFXJumpDownReachSpec_5")||
           read_ptr(spec+0x8c)!=saved_top->pointer||read_ptr(spec+0x78)!=bottom||read_ptr(spec+0x94)||read_ptr(spec+0xbc)||
           !read_at(spec+0xa0,&radius,4)||radius!=34||!read_at(spec+0xa4,&height,4)||height!=90||
           !read_at(spec+0xb0,&blocked,4)||blocked||!read_at(spec+0xb4,&flags,4)||flags!=0xd||
           !read_at(spec+0xb8,&pruned,1)||pruned)return 0;
        ++found;
    }
    return read_at(saved_top->pointer+0x2ac,&again,sizeof again)&&!memcmp(&paths,&again,sizeof paths)&&found==1;
}
typedef struct CombatActionDispatch {uintptr_t base;} CombatActionDispatch;
static int combat_action_dispatch(void *context,const CombatActionParms *parms) {
    uintptr_t base=((CombatActionDispatch*)context)->base;CombatActionQA *q=&combat_action_qa;
    uintptr_t input=q->scope[COMBAT_ACTION_INPUT].pointer,manager=q->scope[COMBAT_ACTION_MANAGER].pointer;
    uint64_t flags=0;
    if(!combat_action_scope(base,1)||!narrative_probe_object(base,input,"BioPlayerInput")||
       !read_at(input+0xc,&flags,8)||(flags&(UINT64_C(1)<<61))||!combat_action_method(base,&q->method)||
       !combat_action_pins(base,input,manager)||!combat_action_bindings(base,q->scope[COMBAT_ACTION_MODE].pointer))return 0;
    ((ProcessEvent)(base+0xf4f80))((void*)input,(void*)q->method.function.pointer,(void*)parms,NULL);return 1;
}
typedef struct CombatActionSample {
    float xyz[3],velocity[3],floor[3];int32_t action,power;unsigned char physics,mode;int grounded;
} CombatActionSample;
static int combat_action_sample(uintptr_t base,CombatActionSample *s) {
    CombatActionQA *q=&combat_action_qa;uintptr_t pawn=q->scope[COMBAT_ACTION_PAWN].pointer;uint32_t life=0;
    memset(s,0,sizeof *s);s->action=s->power=-1;s->mode=s->physics=255;
    if(!combat_action_scope(base,0)||!read_at(pawn+0xe20,&life,4)||(life&0x200)||
       !read_at(pawn+0x108,s->xyz,12)||!scene_readiness_finite3(s->xyz)||
       !read_at(pawn+0x138,s->velocity,12)||!scene_readiness_finite3(s->velocity)||
       !read_at(pawn+0x2fc,s->floor,12)||!scene_readiness_finite3(s->floor)||
       !read_at(pawn+0xc20,&s->action,4)||!read_at(pawn+0xc28,&s->power,4)||!read_at(pawn+0x268,&s->physics,1)||
       !read_at(q->scope[COMBAT_ACTION_MANAGER].pointer+0xa8,&s->mode,1))return 0;
    s->grounded=s->physics==1&&s->floor[2]>=0.6f&&s->floor[2]<=1.01f&&fabsf(s->velocity[2])<15;return 1;
}
static void combat_action_log(uintptr_t base,const char *event,const char *reason,DWORD now) {
    CombatActionQA *q=&combat_action_qa;CombatActionSample s;int valid=combat_action_sample(base,&s);char row[1500];
    snprintf(row,sizeof row,"{\"combat_action_qa\":\"%s\",\"reason\":\"%s\",\"command\":\"PC_Action\",\"press_forwarded\":%s,\"release_forwarded\":%s,\"release_pending\":%s,\"release_attempts\":%d,\"sample_valid\":%s,\"mode\":%u,\"custom_action\":%d,\"power_action\":%d,\"physics\":%u,\"grounded\":%s,\"position\":[%.3f,%.3f,%.3f],\"velocity\":[%.3f,%.3f,%.3f],\"floor_z\":%.4f,\"height_change\":%.3f,\"elapsed_ms\":%lu,\"monotonic_ms\":%lu,\"position_writes\":false,\"traversal_verified\":false,\"combat_verified\":false}\r\n",
        event,reason,q->pulse.press_forwarded?"true":"false",q->pulse.release_forwarded?"true":"false",q->pulse.pending?"true":"false",q->pulse.release_attempts,
        valid?"true":"false",s.mode,s.action,s.power,s.physics,valid&&s.grounded?"true":"false",
        valid?s.xyz[0]:0,valid?s.xyz[1]:0,valid?s.xyz[2]:0,valid?s.velocity[0]:0,valid?s.velocity[1]:0,valid?s.velocity[2]:0,
        valid?s.floor[2]:0,valid?s.xyz[2]-q->origin[2]:0,q->pulse.attempted?now-q->pulse.pressed_at:0,now);line(row);
}
static int combat_action_cleanup(uintptr_t base,DWORD now) {
    CombatActionQA *q=&combat_action_qa;if(!q->pulse.pending)return !q->pulse.attempted||q->pulse.release_forwarded;
    CombatActionDispatch dispatch={base};int result=combat_action_release(&q->pulse,now,1,combat_action_scope(base,1),combat_action_dispatch,&dispatch);
    combat_action_log(base,result==1?"released":"cleanup-unverified",result==1?"matching-alias-release":"original-input-method-or-binding-unavailable",now);
    return result==1;
}
static int combat_action_begin(uintptr_t base,uintptr_t pc,uintptr_t pawn,uintptr_t world,uintptr_t input,const float xyz[3],DWORD now) {
    CombatActionQA *q=&combat_action_qa;if(!combat_action_opted()||q->pulse.attempted)return 0;
    uintptr_t manager=read_ptr(pc+0xc38);GameArray modes={0};unsigned char current=255;
    if(!narrative_probe_object(base,manager,"SFXGameModeManager")||!combat_action_array(manager+0x70,&modes,32)||modes.count<1||
       !read_at(manager+0xa8,&current,1)||current!=0)return 0;
    uintptr_t mode=read_ptr(modes.data),objects[]={pc,pawn,world,input,manager,mode};
    if(!narrative_probe_object(base,mode,"SFXGameModeDefault"))return 0;
    for(unsigned i=0;i<COMBAT_ACTION_SCOPE_COUNT;++i)if(!guest_camera_stamp_read(base,objects[i],q->scope+i,1))return 0;
    uintptr_t fn=guest_player_context_find_method(base,read_ptr(input+0x50),"BioPlayerInput","DebugExecInputCommands");
    if(!guest_player_context_method_stamp(base,fn,&q->method)||!combat_action_method(base,&q->method)||
       !combat_action_scope(base,1)||!combat_action_pins(base,input,manager)||!combat_action_bindings(base,mode))return 0;
    memcpy(q->origin,xyz,12);q->report_at=now;CombatActionDispatch dispatch={base};
    q->active=1;int called=combat_action_press(&q->pulse,1,1,now,combat_action_dispatch,&dispatch);
    combat_action_log(base,called?"pressed":"press-unverified","guarded-stationary-bound-command-candidate",now);return called;
}
/* Returns 1 while observing, 0 after its five-second window, -1 on failure.
 * Ordinary action/airborne state is observed, not replaced with route axes. */
static int combat_action_poll(uintptr_t base,DWORD now) {
    CombatActionQA *q=&combat_action_qa;CombatActionDispatch dispatch={base};CombatActionSample s;
    if(!q->active)return 0;
    int release=combat_action_release(&q->pulse,now,0,combat_action_scope(base,1),combat_action_dispatch,&dispatch);
    if(release)combat_action_log(base,release==1?"released":"cleanup-unverified",release==1?"matching-alias-release":"identity-or-release-retry-limit",now);
    if(release<0||(!q->pulse.pending&&!q->pulse.release_forwarded)||!combat_action_sample(base,&s))return -1;
    float dx=s.xyz[0]-q->origin[0],dy=s.xyz[1]-q->origin[1],dz=s.xyz[2]-q->origin[2];
    if(dx*dx+dy*dy>600*600||dz < -700||dz>250)return -1;
    if(!q->report_at||now-q->report_at>=100){q->report_at=now;combat_action_log(base,"observed","natural-action-state",now);}
    if(now-q->pulse.pressed_at>=5000){q->active=0;combat_action_log(base,"observation-complete","runtime-verdict-required",now);return 0;}return 1;
}

typedef struct CombatActionFixture {int calls,presses,releases,fail,invalid;CombatActionParms last;} CombatActionFixture;
static int combat_action_fixture_forward(void *context,const CombatActionParms *parms) {
    CombatActionFixture *f=context;++f->calls;f->last=*parms;
    if(parms->command.count!=10||parms->command.max!=10||wcscmp(parms->command.data,L"PC_Action")||parms->pressed>1)f->invalid=1;
    if(parms->pressed)++f->presses;else ++f->releases;return !f->fail;
}
static int combat_action_qa_selftest(void) {
    CombatActionPulse p={0};CombatActionFixture f={0};
    if(combat_action_press(&p,0,1,100,combat_action_fixture_forward,&f)||combat_action_press(&p,1,0,100,combat_action_fixture_forward,&f)||f.calls)return 0;
    if(!combat_action_press(&p,1,1,100,combat_action_fixture_forward,&f)||f.calls!=1||f.presses!=1||!p.pending)return 0;
    if(combat_action_press(&p,1,1,110,combat_action_fixture_forward,&f)||f.calls!=1)return 0;
    if(combat_action_release(&p,199,0,1,combat_action_fixture_forward,&f)||f.calls!=1)return 0;
    if(combat_action_release(&p,200,0,1,combat_action_fixture_forward,&f)!=1||p.pending||!p.release_forwarded||f.calls!=2||f.releases!=1||f.invalid)return 0;
    if(combat_action_release(&p,300,1,1,combat_action_fixture_forward,&f)||f.calls!=2)return 0;
    memset(&p,0,sizeof p);memset(&f,0,sizeof f);
    if(!combat_action_press(&p,1,1,400,combat_action_fixture_forward,&f)||combat_action_release(&p,401,1,1,combat_action_fixture_forward,&f)!=1||f.releases!=1)return 0;
    memset(&p,0,sizeof p);memset(&f,0,sizeof f);
    if(!combat_action_press(&p,1,1,500,combat_action_fixture_forward,&f)||combat_action_release(&p,600,0,0,combat_action_fixture_forward,&f)!=-1||f.calls!=1||p.pending)return 0;
    memset(&p,0,sizeof p);memset(&f,0,sizeof f);f.fail=1;
    if(combat_action_press(&p,1,1,700,combat_action_fixture_forward,&f)||!p.pending)return 0;
    for(int i=0;i<3;++i)if(combat_action_release(&p,800+i,1,1,combat_action_fixture_forward,&f)!=(i==2?-1:0))return 0;
    if(p.pending||p.release_forwarded||f.calls!=4||f.releases!=3||f.invalid)return 0;
    memset(&p,0,sizeof p);memset(&f,0,sizeof f);
    if(!combat_action_press(&p,1,1,0xfffffff0u,combat_action_fixture_forward,&f)||
       combat_action_release(&p,0x53u,0,1,combat_action_fixture_forward,&f)||
       combat_action_release(&p,0x54u,0,1,combat_action_fixture_forward,&f)!=1||f.calls!=2)return 0;
    GuestCameraStamp a[COMBAT_ACTION_SCOPE_COUNT]={0},b[COMBAT_ACTION_SCOPE_COUNT];
    for(unsigned i=0;i<COMBAT_ACTION_SCOPE_COUNT;++i){a[i].pointer=100+i;a[i].klass=200+i;a[i].index=300+i;a[i].class_index=400+i;a[i].name[0]=500+i;a[i].class_name[0]=600+i;strcpy(a[i].full_name,"fixture");}
    memcpy(b,a,sizeof a);if(!combat_action_scope_equal(a,b,1)||combat_action_scope_equal(a,b,0))return 0;
    for(unsigned i=0;i<COMBAT_ACTION_SCOPE_COUNT;++i){++b[i].index;if(combat_action_scope_equal(a,b,1))return 0;--b[i].index;
        ++b[i].klass;if(combat_action_scope_equal(a,b,1))return 0;--b[i].klass;
        b[i].full_name[0]='x';if(combat_action_scope_equal(a,b,1))return 0;b[i].full_name[0]='f';}
    uintptr_t touching[]={5,9,15};if(!combat_action_touching_target(touching,3,9)||combat_action_touching_target(touching,3,10))return 0;
    touching[2]=9;if(combat_action_touching_target(touching,3,9)||combat_action_touching_target(touching,257,9))return 0;
    float top[3]={0,0,0},bottom[3]={200,0,-380},target[3],turn=0;
    if(!combat_action_heading(top,bottom,0,&turn,target)||turn||fabsf(target[0]+60)>0.01f||target[1])return 0;
    if(!combat_action_heading(top,bottom,-16384,&turn,target)||turn!=128)return 0;
    if(!combat_action_heading(top,bottom,16384,&turn,target)||turn!=-128)return 0;
    if(!combat_action_heading(top,bottom,65536,&turn,target)||turn)return 0;
    bottom[2]=0;if(combat_action_heading(top,bottom,0,&turn,target))return 0;
    if(!combat_action_approach_safe(45,120,9999)||combat_action_approach_safe(44.9f,0,0)||
       combat_action_approach_safe(231,0,0)||combat_action_approach_safe(60,121,0)||
       combat_action_approach_safe(60,0,10000)||combat_action_approach_safe(NAN,0,0))return 0;
    if(!combat_action_ready(1,60,0,0,250)||combat_action_ready(0,60,0,0,250)||
       combat_action_ready(1,44.9f,0,0,250)||combat_action_ready(1,81,0,0,250)||
       combat_action_ready(1,60,1,0,250)||combat_action_ready(1,60,0,15,250)||
       combat_action_ready(1,60,0,0,249))return 0;
    const float contact_trace[]={79.843f,80.697f,80.243f,80.751f,80.384f,79.874f};
    int braking=0;
    for(unsigned i=0;i<sizeof contact_trace/sizeof *contact_trace;++i){
        braking=combat_action_braking(braking,1,contact_trace[i],0);if(!braking)return 0;
    }
    if(combat_action_braking(0,1,80.1f,0)||combat_action_braking(1,1,82.01f,0)||
       combat_action_braking(1,0,79,0)||combat_action_braking(1,1,79,1)||
       combat_action_braking(1,1,44.9f,0)||combat_action_braking(2,1,79,0)||
       combat_action_braking(1,1,NAN,0))return 0;
    return 1;
}
#endif
