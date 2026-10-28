#ifndef ME3_RUNTIME_GUEST_REACTIONS_H
#define ME3_RUNTIME_GUEST_REACTIONS_H
/* Human-owned damage reactions from the same-world player's native table.
 * Keep the standard family consistent: its alternate/knee NPC actions are
 * absent for the player. Native authority/selection/randomness/hit context,
 * animation, replication and Stop lifecycle remain the only action drivers. */
#include "guest_reaction_layout.h"
typedef struct RuntimeGuestReactions {
    RuntimeMovementScope scope;
    SessionHandle source;
    RuntimeMovementRow rows[GUEST_REACTION_COUNT];
    DWORD checked;
    int valid,retiring;
} RuntimeGuestReactions;
static RuntimeGuestReactions runtime_guest_reactions;
static const char *runtime_reaction_old[GUEST_REACTION_COUNT]={"SFXCustomAction_StandardImpact","SFXCustomAction_StandardImpactII",
    "SFXCustomAction_StandardImpactForward","SFXCustomAction_StandardImpactLeft","SFXCustomAction_StandardImpactRight",
    "SFXCustomAction_StandardImpactKnee",NULL,"SFXCustomAction_StaggerImpact","SFXCustomAction_StaggerImpactII",
    "SFXCustomAction_StaggerImpactForward","SFXCustomAction_StaggerImpactLeft","SFXCustomAction_StaggerImpactRight","SFXCustomAction_Meleed"};
static unsigned runtime_reaction_events;
static int runtime_reactions_enabled(void) {
    static int enabled=-1;if(enabled<0){wchar_t text[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_PLAYER_REACTIONS",text,8);
        enabled=!n||(n==1&&text[0]==L'1');}return enabled;
}
static void runtime_reaction_note(const char *event,unsigned slots,DWORD now) {
    if(runtime_reaction_events++>=48)return;char text[320];
    snprintf(text,sizeof text,"{\"runtime_guest_reactions\":\"%s\",\"role\":%d,\"changed_slots\":%u,\"action_dispatched\":false,\"damage_written\":false,\"gameplay_verified\":false,\"monotonic_ms\":%lu}\r\n",event,(int)g_runtime_config.role,slots,now);line(text);
}
static int runtime_reaction_arrays_live(uintptr_t base,RuntimeGuestReactions *q) {
    return runtime_movement_arrays_live(base,&q->scope)&&q->scope.classes.count>100;
}
static int runtime_reaction_prepare(uintptr_t base,RuntimeGuestReactions *q,unsigned i,const GameArray *source) {
    RuntimeMovementRow *r=&q->rows[i];unsigned index=guest_reaction_indices[i];
    uintptr_t old=read_ptr(q->scope.classes.data+(SIZE_T)index*8),next=read_ptr(source->data+(SIZE_T)index*8);
    uintptr_t cell=runtime_movement_instance_cell(&q->scope.actions,index),action=cell?read_ptr(cell):0;
    if(i==6){if(old||action)return 0;}
    else if(!runtime_movement_named_class(base,old,runtime_reaction_old[i],&r->old_class)||
        !guest_camera_stamp_read(base,read_ptr(old+0x13c),&r->old_cdo,1)||read_ptr(r->old_cdo.pointer+0x50)!=old)return 0;
    if(guest_reaction_action((int)index)){
        if(!narrative_probe_object(base,next,"Class")||!guest_camera_stamp_read(base,next,&r->new_class,1)||
           strcmp(r->new_class.full_name,guest_reaction_player_class((int)index))||
           !guest_camera_stamp_read(base,read_ptr(next+0x13c),&r->new_cdo,1)||read_ptr(r->new_cdo.pointer+0x50)!=next||
           !is_kind_of(base,r->new_cdo.pointer,i==6?"SFXCustomAction_SingleAnim":"SFXCustomAction_DamageReaction")||
           !runtime_traversal_clips(base,q->scope.pawn.pointer,r->new_cdo.pointer,0))return 0;
    }else if(next)return 0; /* Match the actual player's absent82/86 entries. */
    if(action){uintptr_t ai=read_ptr(action+0xa8),pc=read_ptr(action+0xa0);
        if(!runtime_movement_instance(base,action,old,q->scope.pawn.pointer)||(pc&&pc!=q->scope.pc.pointer)||(pc&&ai)||
           !runtime_traversal_action_idle(base,action,q->scope.pc.pointer)||
           !runtime_power_action_context_ai(base,ai,q->scope.pawn.pointer,q->scope.info.pointer)||(ai&&read_ptr(ai+0x3bc))||
           (g_runtime_config.role==ME3_ROLE_HOST&&ai&&ai!=q->scope.ai.pointer)||!guest_camera_stamp_read(base,action,&r->old_instance,1))return 0;
        if(ai){r->old_ai=session_capture_handle(ai);if(!session_handle_matches(r->old_ai))return 0;}
    }
    return 1;
}
static int runtime_reaction_observe(uintptr_t base,RuntimeGuestReactions *q,unsigned i,int idle) {
    RuntimeMovementRow *r=&q->rows[i];unsigned index=guest_reaction_indices[i];
    uintptr_t cell=runtime_movement_instance_cell(&q->scope.actions,index),action=cell?read_ptr(cell):0;
    if((r->old_class.pointer&&!guest_camera_stamp_matches(base,&r->old_class,1))||
       (r->new_class.pointer&&!guest_camera_stamp_matches(base,&r->new_class,1))||
       read_ptr(q->scope.classes.data+(SIZE_T)index*8)!=r->new_class.pointer)return 0;
    if(!action){r->observed=0;memset(&r->new_instance,0,sizeof r->new_instance);return 1;}
    if(!r->new_class.pointer)return 0;
    /* The same action object can have its controller caches rebound without
     * changing its UObject stamp. That cache becomes a different owner's. */
    if(!runtime_movement_instance(base,action,r->new_class.pointer,q->scope.pawn.pointer)||
       read_ptr(action+0xa0)!=q->scope.pc.pointer||read_ptr(action+0xa8))return 0;
    if(r->observed){if(action!=r->new_instance.pointer||!guest_camera_stamp_matches(base,&r->new_instance,1))return 0;}
    else{
        if(!guest_camera_stamp_read(base,action,&r->new_instance,1))return 0;
        r->observed=1;
    }
    return idle&&!runtime_traversal_action_idle(base,action,q->scope.pc.pointer)?-1:1;
}
static int runtime_reaction_restore(uintptr_t base,RuntimeGuestReactions *q,unsigned i) {
    RuntimeMovementRow *r=&q->rows[i];if(!r->changed)return 1;
    int own=runtime_reaction_observe(base,q,i,1);if(own<0)return 0;if(!own){r->changed=0;return 1;}
    uintptr_t old=0,owner=read_ptr(q->scope.pawn.pointer+0x374);
    if(r->old_instance.pointer&&guest_camera_stamp_matches(base,&r->old_instance,1)&&
       runtime_movement_instance(base,r->old_instance.pointer,r->old_class.pointer,q->scope.pawn.pointer)&&
       !read_ptr(r->old_instance.pointer+0xa0)&&read_ptr(r->old_instance.pointer+0xa8)==r->old_ai.pointer&&
       (!r->old_ai.pointer||(session_handle_matches(r->old_ai)&&(!owner||owner==r->old_ai.pointer))))old=r->old_instance.pointer;
    unsigned index=guest_reaction_indices[i];
    if(!runtime_movement_restore(q->scope.classes.data+(SIZE_T)index*8,runtime_movement_instance_cell(&q->scope.actions,index),
       r->old_class.pointer,r->new_class.pointer,r->observed?r->new_instance.pointer:0,old))return 0;
    r->changed=0;return 1;
}
static void runtime_guest_reactions_retire(uintptr_t base,uintptr_t engine,DWORD now) {
    RuntimeGuestReactions *q=&runtime_guest_reactions;if(!q->valid||GetCurrentThreadId()!=session_game_thread)return;q->retiring=1;
    if(!runtime_reaction_arrays_live(base,q)){runtime_reaction_note("preserved-replaced-scope",0,now);memset(q,0,sizeof *q);return;}
    uintptr_t owner=read_ptr(q->scope.pawn.pointer+0x374);
    if(!runtime_movement_retire_owner_policy(owner,q->scope.pc.pointer,session_handle_matches(q->scope.pc),
       guest_camera_actor_live(base,owner)&&is_kind_of(base,owner,"SFXAI_Henchman"),
       read_ptr(owner+0x3bc)==q->scope.pawn.pointer,read_ptr(owner+0x198)==q->scope.info.pointer)){
        runtime_reaction_note("preserved-new-owner",0,now);memset(q,0,sizeof *q);return;}
    if(!runtime_movement_idle(q->scope.pawn.pointer))return;unsigned restored=0;
    for(unsigned i=0;i<GUEST_REACTION_COUNT;++i)if(q->rows[i].changed){if(runtime_reaction_restore(base,q,i))++restored;}
    for(unsigned i=0;i<GUEST_REACTION_COUNT;++i)if(q->rows[i].changed)return;
    runtime_reaction_note("native-ai-reaction-slots-restored",restored,now);memset(q,0,sizeof *q);(void)engine;
}
static int runtime_reaction_apply(RuntimeGuestReactions *q,unsigned i) {
    RuntimeMovementRow *r=&q->rows[i];unsigned index=guest_reaction_indices[i];
    if(!runtime_movement_borrow(q->scope.classes.data+(SIZE_T)index*8,runtime_movement_instance_cell(&q->scope.actions,index),
        r->old_class.pointer,r->new_class.pointer,r->old_instance.pointer))return 0;r->changed=1;return 1;
}
static int runtime_reaction_apply_group(RuntimeGuestReactions *q,unsigned first,unsigned count,unsigned prepared) {
    if(!count||count>GUEST_REACTION_STANDARD_COUNT||first>=GUEST_REACTION_COUNT||count>GUEST_REACTION_COUNT-first||prepared!=(1u<<count)-1)return 0;
    for(unsigned i=first;i<first+count;++i)if(!runtime_reaction_apply(q,i))return -1;
    return (int)count;
}
static int runtime_reaction_apply_family(RuntimeGuestReactions *q,unsigned prepared) {
    return runtime_reaction_apply_group(q,0,GUEST_REACTION_STANDARD_COUNT,prepared);
}
#include "runtime_guest_cover_knockback.h"
static void runtime_guest_reactions_tick(uintptr_t base,uintptr_t engine,DWORD now) {
    if(GetCurrentThreadId()!=session_game_thread)return;RuntimeGuestReactions *q=&runtime_guest_reactions;RuntimeMovementScope scope={0};uint32_t player=0;
    int admitted=runtime_reactions_enabled()&&runtime_movement_scope(base,engine,&scope)&&
        runtime_power_property(base,scope.pawn.pointer,"BioPawn","bIsAPlayer","BoolProperty",0xe20,4)&&
        read_at(scope.pawn.pointer+0xe20,&player,4)&&(player&0x10)&&scope.classes.count>100;
    if(admitted)runtime_guest_cover_knockback_prepare(base,&scope,now);
    if(q->valid){
        if(q->retiring||!admitted||!runtime_movement_scope_same(&q->scope,&scope)){runtime_guest_reactions_retire(base,engine,now);return;}
        if(!runtime_reaction_arrays_live(base,q)){runtime_guest_reactions_retire(base,engine,now);return;}
        for(unsigned i=0;i<GUEST_REACTION_COUNT;++i)if(q->rows[i].changed&&!runtime_reaction_observe(base,q,i,0)){
            /* A native replacement retires the family; do not partly reapply
             * player variants over another subsystem's alternate reaction. */
            q->rows[i].changed=0;runtime_guest_reactions_retire(base,engine,now);return;
        }
    }
    if(!admitted||(q->checked&&now-q->checked<5000)||!runtime_movement_idle(scope.pawn.pointer))return;
    q->checked=now;q->scope=scope;q->valid=1;GameArray source={0};
    if(!runtime_movement_player_source(base,&q->scope,&q->source,&source,100))return;
    if(!q->rows[0].changed){unsigned prepared=0;
        for(unsigned i=0;i<GUEST_REACTION_STANDARD_COUNT;++i){memset(&q->rows[i],0,sizeof q->rows[i]);if(runtime_reaction_prepare(base,q,i,&source))prepared|=1u<<i;}
        int applied=runtime_reaction_apply_family(q,prepared);
        if(applied<0){q->retiring=1;runtime_guest_reactions_retire(base,engine,now);return;}
        if(applied)runtime_reaction_note("native-player-standard-family-borrowed",(unsigned)applied,now);
    }
    if(!q->rows[6].changed){memset(&q->rows[6],0,sizeof q->rows[6]);
        if(runtime_reaction_prepare(base,q,6,&source)&&runtime_reaction_apply(q,6))runtime_reaction_note("native-player-shield-reaction-borrowed",1,now);
    }
    if(!q->rows[GUEST_REACTION_STAGGER_FIRST].changed){unsigned prepared=0;
        for(unsigned i=0;i<GUEST_REACTION_STAGGER_COUNT;++i){unsigned row=GUEST_REACTION_STAGGER_FIRST+i;
            memset(&q->rows[row],0,sizeof q->rows[row]);if(runtime_reaction_prepare(base,q,row,&source))prepared|=1u<<i;
        }
        int applied=runtime_reaction_apply_group(q,GUEST_REACTION_STAGGER_FIRST,GUEST_REACTION_STAGGER_COUNT,prepared);
        if(applied<0){q->retiring=1;runtime_guest_reactions_retire(base,engine,now);return;}
        if(applied)runtime_reaction_note("native-player-stagger-family-borrowed",(unsigned)applied,now);
    }
    if(!q->rows[12].changed){memset(&q->rows[12],0,sizeof q->rows[12]);
        if(runtime_reaction_prepare(base,q,12,&source)&&runtime_reaction_apply(q,12))runtime_reaction_note("native-player-meleed-reaction-borrowed",1,now);
    }
}
#ifdef ME3_RUNTIME_FIXTURE
static int runtime_guest_reactions_selftest(void) {
    _Alignas(8) uintptr_t cells[GUEST_REACTION_COUNT*2]={0};
    /* Real CAS policy includes NPC->None rows and a separate None->player row. */
    for(unsigned i=0;i<GUEST_REACTION_COUNT;++i){uintptr_t old=i==6?0:10+i,next=guest_reaction_action((int)guest_reaction_indices[i])?40+i:0;
        cells[i*2]=old;cells[i*2+1]=old?80+i:0;
        if(!runtime_movement_borrow((uintptr_t)&cells[i*2],(uintptr_t)&cells[i*2+1],old,next,old?80+i:0)||cells[i*2]!=next||cells[i*2+1])return 0;
        if(next)cells[i*2+1]=100+i;
        if(!runtime_movement_restore((uintptr_t)&cells[i*2],(uintptr_t)&cells[i*2+1],old,next,next?100+i:0,0)||cells[i*2]!=old||cells[i*2+1])return 0;
    }
    cells[0]=0;cells[1]=999;
    if(runtime_movement_restore((uintptr_t)&cells[0],(uintptr_t)&cells[1],10,0,0,0)||cells[0]||cells[1]!=999)return 0;
    _Alignas(8) uintptr_t classes[101]={0},actions[101]={0};RuntimeGuestReactions q={0};
    q.scope.classes=(GameArray){(uintptr_t)classes,101,101};q.scope.actions=(GameArray){(uintptr_t)actions,101,101};
    for(unsigned i=0;i<GUEST_REACTION_STANDARD_COUNT;++i){unsigned at=guest_reaction_indices[i];
        q.rows[i].old_class.pointer=classes[at]=10+i;q.rows[i].new_class.pointer=(i==1||i==5)?0:40+i;
    }
    if(runtime_reaction_apply_family(&q,0x1f))return 0; /* Missing one clip: no partial player family. */
    for(unsigned i=0;i<GUEST_REACTION_STANDARD_COUNT;++i)if(classes[guest_reaction_indices[i]]!=10+i||q.rows[i].changed)return 0;
    classes[84]=999;
    if(runtime_reaction_apply_family(&q,0x3f)!=-1||classes[84]!=999||q.rows[3].changed)return 0;
    for(unsigned i=0;i<3;++i){unsigned at=guest_reaction_indices[i];
        if(!q.rows[i].changed||!runtime_movement_restore((uintptr_t)&classes[at],(uintptr_t)&actions[at],10+i,q.rows[i].new_class.pointer,0,0))return 0;
        q.rows[i].changed=0;
    }
    classes[84]=13;
    if(runtime_reaction_apply_family(&q,0x3f)!=GUEST_REACTION_STANDARD_COUNT)return 0;
    for(unsigned i=7;i<12;++i){unsigned at=guest_reaction_indices[i];
        q.rows[i].old_class.pointer=classes[at]=10+i;q.rows[i].new_class.pointer=i==8?0:40+i;
    }
    if(runtime_reaction_apply_group(&q,7,5,0xf))return 0;
    for(unsigned i=7;i<12;++i)if(classes[guest_reaction_indices[i]]!=10+i||q.rows[i].changed)return 0;
    classes[89]=999;
    if(runtime_reaction_apply_group(&q,7,5,0x1f)!=-1||classes[89]!=999||q.rows[9].changed)return 0;
    for(unsigned i=7;i<9;++i){unsigned at=guest_reaction_indices[i];
        if(!q.rows[i].changed||!runtime_movement_restore((uintptr_t)&classes[at],(uintptr_t)&actions[at],10+i,q.rows[i].new_class.pointer,0,0))return 0;
        q.rows[i].changed=0;
    }
    classes[89]=19;
    if(runtime_reaction_apply_group(&q,7,5,0x1f)!=5||classes[88])return 0;
    if(runtime_reaction_apply_group(&q,13,1,1)||runtime_reaction_apply_group(&q,7,0,0)||runtime_reaction_apply_group(&q,7,7,0x7f))return 0;
    return guest_reaction_row(81)==0&&guest_reaction_row(86)==5&&guest_reaction_row(100)==6&&guest_reaction_row(87)==7&&
        guest_reaction_row(88)==8&&guest_reaction_row(96)==12&&guest_reaction_row(92)<0&&
        guest_reaction_action(81)&&guest_reaction_action(83)&&guest_reaction_action(100)&&!guest_reaction_action(82)&&!guest_reaction_action(86)&&
        guest_reaction_action(87)&&!guest_reaction_action(88)&&guest_reaction_action(89)&&guest_reaction_action(96)&&
        !strcmp(guest_reaction_player_class(100),"SFXGame.SFXCustomAction_PlayerShieldStandardImpact");
}
#endif
#endif
