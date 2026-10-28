#ifndef ME3_RUNTIME_GUEST_TRAVERSAL_H
#define ME3_RUNTIME_GUEST_TRAVERSAL_H
/* Borrow the loaded player's traversal actions only for a current human
 * Ashley. Native input, geometry queries, root motion and replication perform
 * the action. No position, movement flags, animation tree or action is replayed. */
typedef struct RuntimeTraversalRow {RuntimeMovementRow slot;int prepared;} RuntimeTraversalRow;
typedef struct RuntimeGuestTraversal {
    RuntimeMovementScope scope;
    SessionHandle source;
    RuntimeTraversalRow rows[GUEST_TRAVERSAL_ROW_COUNT];
    int valid,retiring;
    DWORD checked;
} RuntimeGuestTraversal;
static RuntimeGuestTraversal runtime_guest_traversal;
static int runtime_traversal_handoff_ready(uintptr_t base,uintptr_t pawn);
static const char *runtime_traversal_old[]={"SFXCustomAction_AIMantleOverCover","SFXCustomAction_AIMantleUp",
    "SFXCustomAction_CoverSlipLeft","SFXCustomAction_CoverSlipLeftStanding","SFXCustomAction_CoverSlipRight",
    "SFXCustomAction_CoverSlipRightStanding","SFXCustomAction_AILadderClimbUp","SFXCustomAction_AILadderClimbDown",
    "SFXCustomAction_StandingGapJump",NULL,NULL,NULL,NULL,NULL};
static unsigned runtime_traversal_events;
static int runtime_traversal_enabled(void) {
    static int enabled=-1;if(enabled<0){wchar_t text[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_PLAYER_TRAVERSAL",text,8);
        enabled=!n||(n==1&&text[0]==L'1');}return enabled;
}
static void runtime_traversal_note(const char *event,unsigned slots,DWORD now) {
    if(runtime_traversal_events++>=64)return;char text[340];
    snprintf(text,sizeof text,"{\"runtime_guest_traversal\":\"%s\",\"role\":%d,\"changed_slots\":%u,\"action_dispatched\":false,\"position_written\":false,\"gameplay_verified\":false,\"monotonic_ms\":%lu}\r\n",event,(int)g_runtime_config.role,slots,now);line(text);
}
static int runtime_traversal_action_idle(uintptr_t base,uintptr_t action,uintptr_t pc) {
    uint32_t flags=0;uintptr_t cached=read_ptr(action+0xb0);
    if(!read_at(action+0x100,&flags,4)||!runtime_movement_action_idle_policy(flags,read_ptr(action+0xb8),0))return 0;
    if(!cached)return 1;
    if(!narrative_probe_object(base,cached,"SFXCameraMode"))return 0;
    uintptr_t camera=read_ptr(pc+0x684),mode=read_ptr(camera+0x558);
    if(!guest_camera_actor_live(base,camera)||!is_kind_of(base,camera,"SFXPlayerCamera")||
       read_ptr(pc+0xe28)!=camera||read_ptr(camera+0x4a8)!=pc||!narrative_probe_object(base,mode,"SFXCameraMode"))return 0;
    int active=mode==cached;
    if(!active&&is_kind_of(base,mode,"SFXCameraMode_Interpolate"))active=read_ptr(mode+0x158)==cached||read_ptr(mode+0x160)==cached;
    return !active;
}
static int runtime_movement_player_source(uintptr_t base,const RuntimeMovementScope *scope,SessionHandle *handle,GameArray *classes,int minimum) {
    uintptr_t pawn=read_ptr(scope->info.pointer+0x6c8),source=0,seen[256];unsigned count=0;
    if(!runtime_power_property(base,scope->info.pointer,"WorldInfo","PawnList","ObjectProperty",0x6c8,8))return 0;
    while(pawn&&count<256){
        for(unsigned i=0;i<count;++i)if(seen[i]==pawn)return 0;seen[count++]=pawn;
        if(!guest_camera_actor_live(base,pawn)||!is_kind_of(base,pawn,"Pawn")||read_ptr(pawn+0x198)!=scope->info.pointer)return 0;
        if(is_kind_of(base,pawn,"SFXPawn_Player")){if(source)return 0;source=pawn;}
        pawn=read_ptr(pawn+0x37c);
    }
    if(pawn||!source||source==scope->pawn.pointer)return 0;*handle=session_capture_handle(source);
    return session_handle_matches(*handle)&&identity_array(source+0x69c,classes,512)&&classes->count>minimum;
}
static int runtime_traversal_source(uintptr_t base,RuntimeGuestTraversal *q,GameArray *classes) {
    return runtime_movement_player_source(base,&q->scope,&q->source,classes,47);
}
/* BodyStance is the native FName array used by these actions. Inspect only
 * that exact reflected struct, not unrelated same-sized arrays. */
static int runtime_traversal_mesh_clips(uintptr_t base,uintptr_t pawn,const uint32_t wanted[64][2],unsigned count) {
    uintptr_t mesh=read_ptr(pawn+0x3f4);GameArray sets={0},again={0};uint64_t found=0;unsigned total=0;
    if(!count||count>64||!narrative_probe_object(base,mesh,"SkeletalMeshComponent")||
       !runtime_power_property(base,mesh,"SkeletalMeshComponent","AnimSets","ArrayProperty",0x320,16)||
       !identity_array(mesh+0x320,&sets,64))return 0;
    for(int i=0;i<sets.count;++i){uintptr_t set=read_ptr(sets.data+(SIZE_T)i*8);GameArray sequences={0},checked={0};
        if(!set)continue;
        if(!narrative_probe_object(base,set,"AnimSet")||!runtime_power_property(base,set,"AnimSet","Sequences","ArrayProperty",0x60,16)||
           !scene_readiness_array(set+0x60,&sequences,4096)||(total+=(unsigned)sequences.count)>8192)return 0;
        for(int n=0;n<sequences.count;++n){uintptr_t sequence=read_ptr(sequences.data+(SIZE_T)n*8);uint32_t name[2];
            if(!sequence)continue;
            if(!narrative_probe_object(base,sequence,"AnimSequence")||!read_at(sequence+0xf4,name,8))return 0;
            for(unsigned w=0;w<count;++w)if(!memcmp(name,wanted[w],8))found|=UINT64_C(1)<<w;
        }
        if(!read_at(set+0x60,&checked,sizeof checked)||memcmp(&sequences,&checked,sizeof checked))return 0;
    }
    uint64_t all=count==64?UINT64_MAX:(UINT64_C(1)<<count)-1;
    return found==all&&read_ptr(pawn+0x3f4)==mesh&&read_at(mesh+0x320,&again,sizeof again)&&!memcmp(&sets,&again,sizeof sets);
}
static int runtime_traversal_clip_name_valid(const uint32_t name[2]) {
    /* Number3 encodes the native suffix _2. Preserve the complete FName
     * pair: never strip that suffix or substitute an unnumbered clip. */
    return name[0]&&name[1]<=INT_MAX;
}
static int runtime_traversal_clips(uintptr_t base,uintptr_t pawn,uintptr_t cdo,int native_ladder_loader) {
    uintptr_t klass=read_ptr(cdo+0x50),seen[32];unsigned count=0,stances=0,clips=0;uint32_t wanted[64][2];
    while(klass&&count<32){
        for(unsigned i=0;i<count;++i)if(seen[i]==klass)return 0;seen[count++]=klass;
        if(!narrative_probe_object(base,klass,"Class"))return 0;
        uintptr_t field=read_ptr(klass+0x70);unsigned n=0;
        while(field&&n++<256){
            if(!is_live(base,field)||read_ptr(field+0x40)!=klass)return 0;
            if(narrative_probe_object(base,field,"StructProperty")){
                uintptr_t structure=read_ptr(field+0xd0);char name[96];object_name(base,structure,name,sizeof name);
                if(!strcmp(name,"BodyStance")){int32_t offset=-1,size=0,dim=0;GameArray array={0},again={0};++stances;
                    if(!narrative_probe_object(base,structure,"ScriptStruct")||!read_at(field+0x94,&offset,4)||offset<0x110||offset>0x1000||
                       !read_at(field+0x74,&size,4)||size!=16||!read_at(field+0x70,&dim,4)||dim!=1||
                       !scene_readiness_array(cdo+(uintptr_t)offset,&array,16))return 0;
                    for(int j=0;j<array.count;++j){uint32_t clip[2];if(!read_at(array.data+(SIZE_T)j*8,clip,8))return 0;
                        if(!clip[0])continue;if(!runtime_traversal_clip_name_valid(clip)||clips>=64)return 0;memcpy(wanted[clips++],clip,8);
                    }
                    if(!read_at(cdo+(uintptr_t)offset,&again,sizeof again)||memcmp(&array,&again,sizeof array))return 0;
                }
            }
            field=read_ptr(field+0x68);
        }
        if(field)return 0;klass=read_ptr(klass+0x60);
    }
    /* Native ladder Start/Stop own the selected node's AnimInfo registration.
     * Other actions require their clips now; scan the mesh once per row rather
     * than repeating the full animation traversal for each required clip. */
    return !klass&&stances&&clips&&(native_ladder_loader||runtime_traversal_mesh_clips(base,pawn,wanted,clips));
}
static int runtime_traversal_instance(uintptr_t base,uintptr_t action,uintptr_t klass,uintptr_t pawn) {
    return runtime_movement_instance(base,action,klass,pawn);
}
static int runtime_traversal_authored_policy(unsigned index,uintptr_t klass,uintptr_t action) {
    return guest_traversal_row((int)index)>=0&&(guest_traversal_empty_authored((int)index)?!(klass||action):klass!=0);
}
static int runtime_traversal_player_policy(unsigned index,uint32_t flags) {
    return (index!=28&&index!=29)||(flags&0x10)!=0;
}
static int runtime_traversal_prepare_row(uintptr_t base,RuntimeGuestTraversal *q,unsigned i,const GameArray *source) {
    unsigned index=guest_traversal_indices[i];RuntimeMovementRow *r=&q->rows[i].slot;
    uintptr_t old=read_ptr(q->scope.classes.data+(SIZE_T)index*8),next=read_ptr(source->data+(SIZE_T)index*8);
    uintptr_t cell=runtime_movement_instance_cell(&q->scope.actions,index),action=cell?read_ptr(cell):0;
    if(!runtime_traversal_authored_policy(index,old,action))return 0;
    if(index==28||index==29){uint32_t flags=0;
        /* GapJump selects player versus AI alignment from bIsAPlayer, not
         * merely the current controller. Never manufacture that native bit. */
        if(!runtime_power_property(base,q->scope.pawn.pointer,"BioPawn","bIsAPlayer","BoolProperty",0xe20,4)||
           !read_at(q->scope.pawn.pointer+0xe20,&flags,4)||!runtime_traversal_player_policy(index,flags)||
           !runtime_traversal_handoff_ready(base,q->scope.pawn.pointer))return 0;
    }
    if(!guest_traversal_empty_authored((int)index)&&(!runtime_movement_named_class(base,old,runtime_traversal_old[i],&r->old_class)||
       !guest_camera_stamp_read(base,read_ptr(old+0x13c),&r->old_cdo,1)||read_ptr(r->old_cdo.pointer+0x50)!=old))return 0;
    if(!narrative_probe_object(base,next,"Class")||!guest_camera_stamp_read(base,next,&r->new_class,1)||
       strcmp(r->new_class.full_name,guest_traversal_player_class((int)index))||
       !guest_camera_stamp_read(base,read_ptr(next+0x13c),&r->new_cdo,1)||read_ptr(r->new_cdo.pointer+0x50)!=next||
       !is_kind_of(base,r->new_cdo.pointer,"BioCustomAction")||
       !runtime_traversal_clips(base,q->scope.pawn.pointer,r->new_cdo.pointer,index==46||index==47))return 0;
    if(action){uintptr_t ai=read_ptr(action+0xa8),pc=read_ptr(action+0xa0);
        if(!runtime_traversal_instance(base,action,old,q->scope.pawn.pointer)||(pc&&pc!=q->scope.pc.pointer)||(pc&&ai)||
           !runtime_traversal_action_idle(base,action,q->scope.pc.pointer)||
           !runtime_power_action_context_ai(base,ai,q->scope.pawn.pointer,q->scope.info.pointer)||(ai&&read_ptr(ai+0x3bc))||
           (g_runtime_config.role==ME3_ROLE_HOST&&ai&&ai!=q->scope.ai.pointer)||!guest_camera_stamp_read(base,action,&r->old_instance,1))return 0;
        if(ai){r->old_ai=session_capture_handle(ai);if(!session_handle_matches(r->old_ai))return 0;}
    }
    q->rows[i].prepared=1;return 1;
}
static int runtime_traversal_observe_row(uintptr_t base,RuntimeGuestTraversal *q,unsigned i,int idle) {
    RuntimeMovementRow *r=&q->rows[i].slot;unsigned index=guest_traversal_indices[i];
    uintptr_t cell=runtime_movement_instance_cell(&q->scope.actions,index),action=cell?read_ptr(cell):0;
    if((r->old_class.pointer&&!guest_camera_stamp_matches(base,&r->old_class,1))||!guest_camera_stamp_matches(base,&r->new_class,1)||
       read_ptr(q->scope.classes.data+(SIZE_T)index*8)!=r->new_class.pointer)return 0;
    if(!action){r->observed=0;memset(&r->new_instance,0,sizeof r->new_instance);return 1;}
    if(r->observed){if(action!=r->new_instance.pointer||!guest_camera_stamp_matches(base,&r->new_instance,1))return 0;}
    else{
        if(!runtime_traversal_instance(base,action,r->new_class.pointer,q->scope.pawn.pointer)||
           read_ptr(action+0xa0)!=q->scope.pc.pointer||read_ptr(action+0xa8)||!guest_camera_stamp_read(base,action,&r->new_instance,1))return 0;
        r->observed=1;
    }
    uintptr_t pc=g_runtime_config.role==ME3_ROLE_GUEST?session_context(session_engine).pc:q->scope.pc.pointer;
    return idle&&!runtime_traversal_action_idle(base,action,pc)?-1:1;
}
#include "runtime_guest_traversal_handoff.h"
static void runtime_guest_traversal_retire(uintptr_t base,uintptr_t engine,DWORD now) {
    RuntimeGuestTraversal *q=&runtime_guest_traversal;if(!q->valid||GetCurrentThreadId()!=session_game_thread)return;q->retiring=1;
    if(!runtime_movement_arrays_live(base,&q->scope)){runtime_traversal_note("preserved-replaced-scope",0,now);memset(q,0,sizeof *q);return;}
    uintptr_t owner=read_ptr(q->scope.pawn.pointer+0x374);
    if(!runtime_movement_retire_owner_policy(owner,q->scope.pc.pointer,session_handle_matches(q->scope.pc),
       guest_camera_actor_live(base,owner)&&is_kind_of(base,owner,"SFXAI_Henchman"),
       read_ptr(owner+0x3bc)==q->scope.pawn.pointer,read_ptr(owner+0x198)==q->scope.info.pointer)){
        runtime_traversal_note("preserved-new-owner",0,now);memset(q,0,sizeof *q);return;}
    if(!runtime_movement_idle(q->scope.pawn.pointer))return;unsigned restored=0;
    for(unsigned i=0;i<GUEST_TRAVERSAL_ROW_COUNT;++i){RuntimeMovementRow *r=&q->rows[i].slot;if(!r->changed)continue;
        int owned=runtime_traversal_observe_row(base,q,i,1);if(owned<0)continue;if(!owned){r->changed=0;continue;}
        uintptr_t old=0;
        if(r->old_instance.pointer&&guest_camera_stamp_matches(base,&r->old_instance,1)&&
           runtime_traversal_instance(base,r->old_instance.pointer,r->old_class.pointer,q->scope.pawn.pointer)&&
           !read_ptr(r->old_instance.pointer+0xa0)&&read_ptr(r->old_instance.pointer+0xa8)==r->old_ai.pointer&&
           (!r->old_ai.pointer||(session_handle_matches(r->old_ai)&&(!owner||owner==r->old_ai.pointer))))old=r->old_instance.pointer;
        unsigned index=guest_traversal_indices[i];
        if(runtime_movement_restore(q->scope.classes.data+(SIZE_T)index*8,runtime_movement_instance_cell(&q->scope.actions,index),
           r->old_class.pointer,r->new_class.pointer,r->observed?r->new_instance.pointer:0,old)){r->changed=0;++restored;}
    }
    for(unsigned i=0;i<GUEST_TRAVERSAL_ROW_COUNT;++i)if(q->rows[i].slot.changed)return;
    runtime_traversal_note("native-ai-traversal-slots-restored",restored,now);memset(q,0,sizeof *q);(void)engine;
}
static void runtime_guest_traversal_tick(uintptr_t base,uintptr_t engine,DWORD now) {
    if(GetCurrentThreadId()!=session_game_thread)return;RuntimeGuestTraversal *q=&runtime_guest_traversal;RuntimeMovementScope scope={0};
    int admitted=runtime_traversal_enabled()&&runtime_movement_scope(base,engine,&scope);
    if(q->valid){
        if(q->retiring||!admitted||!runtime_movement_scope_same(&q->scope,&scope)){runtime_guest_traversal_retire(base,engine,now);return;}
        if(!runtime_movement_arrays_live(base,&q->scope)){runtime_guest_traversal_retire(base,engine,now);return;}
        for(unsigned i=0;i<GUEST_TRAVERSAL_ROW_COUNT;++i)if(q->rows[i].slot.changed&&!runtime_traversal_observe_row(base,q,i,0))q->rows[i].slot.changed=0;
    }
    if(!admitted||(q->checked&&now-q->checked<5000)||!runtime_movement_idle(scope.pawn.pointer))return;
    q->checked=now;if(!q->valid){q->scope=scope;q->valid=1;}else q->scope=scope;
    GameArray source={0};if(!runtime_traversal_source(base,q,&source))return;unsigned applied=0;
    for(unsigned i=0;i<GUEST_TRAVERSAL_ROW_COUNT;++i){RuntimeMovementRow *r=&q->rows[i].slot;if(r->changed)continue;
        /* A native replacement wins. Preparing only accepts the known authored
         * AI class or an authored empty slot, never another subsystem's slot. */
        memset(&q->rows[i],0,sizeof q->rows[i]);
        if(!runtime_movement_arrays_live(base,&q->scope)||!runtime_traversal_prepare_row(base,q,i,&source))continue;
        unsigned index=guest_traversal_indices[i];
        if(runtime_movement_borrow(q->scope.classes.data+(SIZE_T)index*8,runtime_movement_instance_cell(&q->scope.actions,index),
           r->old_class.pointer,r->new_class.pointer,r->old_instance.pointer)){r->changed=1;++applied;}
    }
    if(applied)runtime_traversal_note("native-player-traversal-slots-borrowed",applied,now);
}
#ifdef ME3_RUNTIME_FIXTURE
static int runtime_guest_traversal_selftest(void) {
    const uint32_t plain[2]={12,0},numbered[2]={12,3},invalid[2]={12,UINT32_MAX},none[2]={0,0};
    if(!runtime_traversal_clip_name_valid(plain)||!runtime_traversal_clip_name_valid(numbered)||
       runtime_traversal_clip_name_valid(invalid)||runtime_traversal_clip_name_valid(none))return 0;
    _Alignas(8) uintptr_t cells[16]={0};for(unsigned i=0;i<8;++i){cells[i*2]=10+i;cells[i*2+1]=30+i;}
    for(unsigned i=0;i<8;++i)if(!runtime_movement_borrow((uintptr_t)&cells[i*2],(uintptr_t)&cells[i*2+1],10+i,50+i,30+i))return 0;
    cells[6]=999; /* One foreign traversal class cannot orphan the other rows. */
    for(unsigned i=0;i<8;++i){int restored=runtime_movement_restore((uintptr_t)&cells[i*2],(uintptr_t)&cells[i*2+1],10+i,50+i,0,30+i);
        if(i==3){if(restored||cells[i*2]!=999)return 0;}else if(!restored||cells[i*2]!=10+i||cells[i*2+1]!=30+i)return 0;
    }
    /* Empty authored slots must return to None after stock lazy instancing. */
    _Alignas(8) uintptr_t empty[2]={0,0};
    if(!runtime_movement_borrow((uintptr_t)&empty[0],0,0,80,0)||empty[0]!=80)return 0;
    empty[1]=90;
    if(!runtime_movement_restore((uintptr_t)&empty[0],(uintptr_t)&empty[1],0,80,90,0)||empty[0]||empty[1])return 0;
    if(!runtime_movement_borrow((uintptr_t)&empty[0],(uintptr_t)&empty[1],0,80,0))return 0;
    empty[1]=999; /* A foreign cached instance prevents class rollback. */
    if(runtime_movement_restore((uintptr_t)&empty[0],(uintptr_t)&empty[1],0,80,90,0)||empty[0]!=80||empty[1]!=999)return 0;
    return guest_traversal_indices[0]==31&&guest_traversal_indices[7]==47&&
        guest_traversal_row(28)==8&&guest_traversal_row(29)==9&&guest_traversal_row(40)==10&&guest_traversal_row(43)==13&&
        guest_traversal_row(30)<0&&guest_traversal_row(44)<0&&guest_traversal_row(54)<0&&
        guest_traversal_empty_authored(29)&&guest_traversal_empty_authored(40)&&guest_traversal_empty_authored(43)&&
        !guest_traversal_empty_authored(28)&&!guest_traversal_empty_authored(46)&&
        runtime_traversal_authored_policy(29,0,0)&&runtime_traversal_authored_policy(43,0,0)&&
        !runtime_traversal_authored_policy(29,10,0)&&!runtime_traversal_authored_policy(40,0,20)&&
        runtime_traversal_authored_policy(28,10,20)&&!runtime_traversal_authored_policy(28,0,0)&&
        !runtime_traversal_authored_policy(30,10,0)&&
        runtime_traversal_player_policy(28,0x10)&&runtime_traversal_player_policy(29,0x30)&&
        !runtime_traversal_player_policy(28,0)&&!runtime_traversal_player_policy(29,8)&&runtime_traversal_player_policy(40,0)&&
        !strcmp(guest_traversal_player_class(28),"SFXGameContent.SFXCustomAction_GapJump")&&
        !strcmp(guest_traversal_player_class(29),"SFXGameContent.SFXCustomAction_StandingGapJump")&&
        !strcmp(guest_traversal_player_class(40),"SFXGame.SFXCustomAction_Cover90TurnRight")&&
        !strcmp(guest_traversal_player_class(41),"SFXGame.SFXCustomAction_Cover90TurnRightStanding")&&
        !strcmp(guest_traversal_player_class(42),"SFXGame.SFXCustomAction_Cover90TurnLeft")&&
        !strcmp(guest_traversal_player_class(43),"SFXGame.SFXCustomAction_Cover90TurnLeftStanding")&&
        !runtime_movement_action_idle_policy(0x10000000,0,0)&&!runtime_movement_action_idle_policy(0,1,0)&&runtime_traversal_handoff_selftest();
}
#endif
#endif
