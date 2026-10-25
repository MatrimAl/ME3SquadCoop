#ifndef ME3_GUEST_HIT_TRACE_CORE_H
#define ME3_GUEST_HIT_TRACE_CORE_H
#define GUEST_HIT_ROW_LIMIT 512u
/* Observational dispatch brackets. Callbacks observe; the original receives
 * every argument exactly once, including NULL params/results. Only the game
 * thread owns the stack/budget. No damage-success verdict is produced here. */
typedef struct GuestHitKey {
    uintptr_t pointer,klass;int32_t index,class_index;uint32_t name[2],class_name[2];
} GuestHitKey;
typedef struct GuestHitSample {
    GuestHitKey target,level,module,inventory,shield;
    int target_valid,health_valid,shield_valid,shield_present;float health,shields;
} GuestHitSample;
typedef struct GuestHitArgs {void *self,*function,*params,*result;} GuestHitArgs;
typedef struct GuestHitFrame {
    struct GuestHitFrame *previous;unsigned id,parent,nested_calls;
    unsigned long generation,epoch;uintptr_t target,weapon,pc;unsigned char mode;int32_t hits,channel_index,weapon_index;
    int captured,scope_after;GuestHitSample before,after;
} GuestHitFrame;
typedef struct GuestHitObserver {GuestHitFrame *current;unsigned rows,calls;} GuestHitObserver;
typedef struct GuestHitOps {
    int (*eligible)(void*,const GuestHitArgs*,uintptr_t,GuestHitFrame*);
    int (*scope_same)(void*,const GuestHitArgs*,const GuestHitFrame*);
    void (*sample)(void*,uintptr_t,GuestHitSample*);
    void (*emit)(void*,const GuestHitFrame*,int);
} GuestHitOps;
static int guest_hit_key_equal(const GuestHitKey *a,const GuestHitKey *b) {
    return a->pointer&&a->pointer==b->pointer&&a->klass==b->klass&&a->index==b->index&&a->class_index==b->class_index&&
        !memcmp(a->name,b->name,8)&&!memcmp(a->class_name,b->class_name,8);
}
static int guest_hit_optional_key_equal(const GuestHitKey *a,const GuestHitKey *b) {
    return (!a->pointer&&!b->pointer)||guest_hit_key_equal(a,b);
}
static int guest_hit_value(float value) {return isfinite(value)&&value>=0&&value<=1000000000.0f;}
static int guest_hit_target_pair(const GuestHitFrame *f) {
    return f->scope_after&&f->before.target_valid&&f->after.target_valid&&
        guest_hit_key_equal(&f->before.target,&f->after.target)&&guest_hit_key_equal(&f->before.level,&f->after.level);
}
static int guest_hit_health_pair(const GuestHitFrame *f) {
    return guest_hit_target_pair(f)&&f->before.health_valid&&f->after.health_valid&&
        guest_hit_key_equal(&f->before.module,&f->after.module)&&guest_hit_value(f->before.health)&&guest_hit_value(f->after.health);
}
static int guest_hit_shield_pair(const GuestHitFrame *f) {
    return guest_hit_target_pair(f)&&f->before.shield_valid&&f->after.shield_valid&&
        f->before.shield_present==f->after.shield_present&&
        guest_hit_optional_key_equal(&f->before.inventory,&f->after.inventory)&&
        (!f->before.shield_present||guest_hit_key_equal(&f->before.shield,&f->after.shield))&&
        guest_hit_value(f->before.shields)&&guest_hit_value(f->after.shields);
}
static int guest_hit_reserve(GuestHitObserver *observer) {
    if(observer->rows>GUEST_HIT_ROW_LIMIT-2)return 0;observer->rows+=2;return 1;
}
static int guest_hit_call_matches(uintptr_t caller,uintptr_t expected_caller,uintptr_t self,uintptr_t weapon,
                                  uintptr_t function,uintptr_t expected_function,int params,int result_null,int authoritative) {
    return authoritative==1&&caller&&caller==expected_caller&&self&&self==weapon&&function&&function==expected_function&&
        params==1&&result_null==1;
}
static void guest_hit_observe(GuestHitObserver *observer,ProcessEvent original,const GuestHitArgs *args,
                               uintptr_t caller,int game_thread,const GuestHitOps *ops,void *context) {
    DWORD before_error=GetLastError();GuestHitFrame frame={0};
    if(game_thread){
        frame.previous=observer->current;frame.parent=frame.previous?frame.previous->id:0;
        if(frame.previous)++frame.previous->nested_calls;
        observer->current=&frame;
        frame.captured=observer->rows<=GUEST_HIT_ROW_LIMIT-2&&ops->eligible(context,args,caller,&frame)&&guest_hit_reserve(observer);
        if(frame.captured){frame.id=++observer->calls;ops->sample(context,frame.target,&frame.before);ops->emit(context,&frame,0);}
    }
    SetLastError(before_error);original(args->self,args->function,args->params,args->result);DWORD after_error=GetLastError();
    if(game_thread){
        if(frame.captured){frame.scope_after=ops->scope_same(context,args,&frame);
            if(frame.scope_after)ops->sample(context,frame.target,&frame.after);
            ops->emit(context,&frame,1);}
        observer->current=frame.previous;
    }
    SetLastError(after_error);
}
#endif
