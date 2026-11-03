#ifndef ME3_RUNTIME_GUEST_COVER_KNOCKBACK_H
#define ME3_RUNTIME_GUEST_COVER_KNOCKBACK_H
/* SFXPawn_Player refuses knockback candidates while in cover. Human Ashley
 * inherits BioPawn's selector instead. For that exact owned case, execute the
 * selector's own existing ReturnFalse branch through its original executor.
 * No candidate array/result/damage/cover state is written or replayed. */
typedef void (*RuntimeCoverKnockbackExec)(void*,void*,void*);
typedef struct RuntimeCoverKnockback {
    uintptr_t base;
    GuestPlayerContextMethod select,cover;
    RuntimeCoverKnockbackExec original;
    DWORD checked;
    unsigned rows;
    int installed;
} RuntimeCoverKnockback;
static RuntimeCoverKnockback runtime_cover_knockback;
static void runtime_cover_knockback_exec(void*,void*,void*);
#include "runtime_guest_cover_knockback_pins.h"
static int runtime_cover_knockback_scope(uintptr_t base,uintptr_t pawn,RuntimeMovementScope *out) {
    uint32_t flags=0;
    return runtime_reactions_enabled()&&runtime_movement_scope(base,session_engine,out)&&out->pawn.pointer==pawn&&
        runtime_power_property(base,pawn,"BioPawn","bIsAPlayer","BoolProperty",0xe20,4)&&
        read_at(pawn+0xe20,&flags,4)&&(flags&0x10);
}
static int runtime_cover_knockback_methods(uintptr_t base,uintptr_t pawn,uintptr_t expected) {
    RuntimeCoverKnockback *s=&runtime_cover_knockback;
    return guest_player_context_method_matches(base,&s->select,1)&&guest_player_context_method_matches(base,&s->cover,1)&&
        !strcmp(s->select.function.full_name,"SFXGame.BioPawn.GetKnockbackReactions")&&
        !strcmp(s->cover.function.full_name,"SFXGame.BioPawn.IsInCover")&&
        guest_player_context_function_abi(base,s->select.function.pointer,"BioPawn","GetKnockbackReactions",0x480002,28,24,0,expected-base,4)&&
        guest_player_context_function_abi(base,s->cover.function.pointer,"BioPawn","IsInCover",0x20401,4,0,0,RUNTIME_COVER_GETTER_RVA,1)&&
        runtime_cover_knockback_bodies(base,s->select.function.pointer,s->cover.function.pointer)&&
        guest_equipment_find_first(base,read_ptr(pawn+0x50),"GetKnockbackReactions")==s->select.function.pointer&&
        guest_equipment_find_first(base,read_ptr(pawn+0x50),"IsInCover")==s->cover.function.pointer&&
        read_ptr(read_ptr(pawn)+0x230)==base+0x40d450;
}
static int runtime_cover_knockback_entry(uintptr_t frame,uintptr_t pawn,uintptr_t fn,GameArray *script) {
    return read_ptr(frame+0x18)==fn&&read_ptr(frame+0x20)==pawn&&read_at(fn+0x7c,script,sizeof *script)&&
        script->data&&script->count==RUNTIME_COVER_SELECTOR_SIZE&&script->max>=script->count&&script->max<=4096&&
        script->data<=UINTPTR_MAX-(uintptr_t)script->count&&read_ptr(frame+0x28)==script->data;
}
static int runtime_cover_knockback_cover_policy(unsigned type,uintptr_t link,int32_t slot,unsigned action) {
    return type&&link&&slot>=0&&action!=11;
}
static int runtime_cover_knockback_in_cover(uintptr_t base,uintptr_t pawn) {
    unsigned char type=0,action=11;uintptr_t link=0;int32_t slot=-1;
    /* Exact IsInCover native leaf, guarded by its supported-image pins. The
     * stock predicate does not dereference CurrentLink or mutate cover. */
    return runtime_power_property(base,pawn,"BioPawn","CoverType","ByteProperty",0xe26,1)&&
        runtime_power_property(base,pawn,"BioPawn","CurrentLink","ObjectProperty",0xb68,8)&&
        runtime_power_property(base,pawn,"BioPawn","CurrentSlotIdx","IntProperty",0xcc4,4)&&
        runtime_power_property(base,pawn,"BioPawn","CoverAction","ByteProperty",0xe27,1)&&
        read_at(pawn+0xe26,&type,1)&&read_at(pawn+0xb68,&link,8)&&read_at(pawn+0xcc4,&slot,4)&&
        read_at(pawn+0xe27,&action,1)&&runtime_cover_knockback_cover_policy(type,link,slot,action);
}
static int runtime_cover_knockback_redirect(int eligible,uintptr_t frame,const GameArray *script) {
    static const unsigned char returns_false[]={0x04,0x28};unsigned char code[2];
    if(!eligible||!script->data||script->count!=RUNTIME_COVER_SELECTOR_SIZE||script->data>UINTPTR_MAX-(uintptr_t)script->count||
       RUNTIME_COVER_FALSE_OFFSET+2>script->count||!read_at(script->data+RUNTIME_COVER_FALSE_OFFSET,code,2)||memcmp(code,returns_false,2))return 0;
    return runtime_power_action_context_pointer(frame+0x28,script->data,script->data+RUNTIME_COVER_FALSE_OFFSET);
}
static void runtime_cover_knockback_before(void *self,void *frame) {
    RuntimeCoverKnockback *s=&runtime_cover_knockback;uintptr_t pawn=(uintptr_t)self;RuntimeMovementScope scope={0},again={0};GameArray script={0},checked={0};
    if(!s->installed||GetCurrentThreadId()!=session_game_thread||
       !runtime_cover_knockback_scope(s->base,pawn,&scope)||
       !runtime_cover_knockback_native_pins(s->base)||
       !runtime_cover_knockback_methods(s->base,pawn,(uintptr_t)runtime_cover_knockback_exec)||
       !runtime_cover_knockback_entry((uintptr_t)frame,pawn,s->select.function.pointer,&script))return;
    if(!runtime_cover_knockback_in_cover(s->base,pawn)||
       !runtime_cover_knockback_scope(s->base,pawn,&again)||!runtime_movement_scope_same(&scope,&again)||
       !runtime_cover_knockback_methods(s->base,pawn,(uintptr_t)runtime_cover_knockback_exec)||
       !runtime_cover_knockback_entry((uintptr_t)frame,pawn,s->select.function.pointer,&checked)||memcmp(&script,&checked,sizeof script)||
       !runtime_cover_knockback_in_cover(s->base,pawn))return;
    if(runtime_cover_knockback_redirect(1,(uintptr_t)frame,&script)&&s->rows++<32){char log[340];
        snprintf(log,sizeof log,"{\"runtime_guest_cover_knockback\":\"stock-false-branch-selected\",\"role\":%d,\"scope_token\":%lu,\"candidate_array_written\":false,\"damage_written\":false,\"gameplay_verified\":false,\"monotonic_ms\":%lu}\r\n",(int)g_runtime_config.role,scope.token,GetTickCount());line(log);
    }
}
static void runtime_cover_knockback_forward(void *self,void *frame,void *result,RuntimeCoverKnockbackExec stock,void (*before)(void*,void*)) {
    DWORD error=GetLastError();before(self,frame);SetLastError(error);stock(self,frame,result);
}
static void runtime_cover_knockback_exec(void *self,void *frame,void *result) {
    runtime_cover_knockback_forward(self,frame,result,runtime_cover_knockback.original,runtime_cover_knockback_before);
}
static void runtime_guest_cover_knockback_prepare(uintptr_t base,const RuntimeMovementScope *scope,DWORD now) {
    RuntimeCoverKnockback *s=&runtime_cover_knockback;
    if(s->installed||GetCurrentThreadId()!=session_game_thread||(s->checked&&now-s->checked<2000))return;s->checked=now;
    uintptr_t pawn=scope->pawn.pointer,klass=read_ptr(pawn+0x50),fn=guest_player_context_find_method(base,klass,"BioPawn","GetKnockbackReactions"),
        cover=guest_player_context_find_method(base,klass,"BioPawn","IsInCover");
    if(!supported_image(base)||!guest_equipment_dispatch_pins(base)||!runtime_cover_knockback_native_pins(base)||
       !guest_player_context_method_stamp(base,fn,&s->select)||!guest_player_context_method_stamp(base,cover,&s->cover)||
       !runtime_cover_knockback_methods(base,pawn,base+0xf52b0)||!background_focus_writable(fn+0xd8,8))return;
    s->base=base;s->original=(RuntimeCoverKnockbackExec)(base+0xf52b0);
    if(InterlockedCompareExchangePointer((PVOID volatile*)(fn+0xd8),(PVOID)runtime_cover_knockback_exec,(PVOID)s->original)!=(PVOID)s->original)return;
    s->installed=1;
    line("{\"runtime_guest_cover_knockback\":\"native-selector-guard-installed\",\"gameplay_verified\":false}\r\n");
}
#ifdef ME3_RUNTIME_FIXTURE
static void *runtime_cover_fixture_args[3];static unsigned runtime_cover_fixture_calls,runtime_cover_fixture_order;static int runtime_cover_fixture_bad,runtime_cover_fixture_redirect;
static void runtime_cover_fixture_before(void *self,void *frame) {
    if(self!=runtime_cover_fixture_args[0]||frame!=runtime_cover_fixture_args[1]||runtime_cover_fixture_order++)runtime_cover_fixture_bad=1;
    GameArray script={read_ptr((uintptr_t)frame+0x28),RUNTIME_COVER_SELECTOR_SIZE,RUNTIME_COVER_SELECTOR_SIZE};
    if(runtime_cover_fixture_redirect&&!runtime_cover_knockback_redirect(1,(uintptr_t)frame,&script))runtime_cover_fixture_bad=1;
    SetLastError(17);
}
static void runtime_cover_fixture_original(void *self,void *frame,void *result) {
    if(self!=runtime_cover_fixture_args[0]||frame!=runtime_cover_fixture_args[1]||result!=runtime_cover_fixture_args[2]||
       runtime_cover_fixture_order++!=1||GetLastError()!=37)runtime_cover_fixture_bad=1;
    unsigned char opcode[2];if(!read_at(read_ptr((uintptr_t)frame+0x28),opcode,2))runtime_cover_fixture_bad=1;
    *(int*)result=opcode[0]==0x04&&opcode[1]==0x28?0:1;
    ++runtime_cover_fixture_calls;SetLastError(53);
}
static int runtime_guest_cover_knockback_selftest(void) {
    DWORD saved=GetLastError();unsigned char code[RUNTIME_COVER_SELECTOR_SIZE]={0};code[RUNTIME_COVER_FALSE_OFFSET]=0x04;code[RUNTIME_COVER_FALSE_OFFSET+1]=0x28;
    _Alignas(8) uintptr_t frame[12]={0},before[12];GameArray script={(uintptr_t)code,sizeof code,sizeof code};int result=-1,owner;
    frame[3]=123;frame[4]=(uintptr_t)&owner;frame[5]=(uintptr_t)code;frame[6]=456;memcpy(before,frame,sizeof frame);
    if(runtime_cover_knockback_redirect(0,(uintptr_t)frame,&script)||memcmp(before,frame,sizeof frame))return 0;
    for(int redirect=0;redirect<2;++redirect){memcpy(frame,before,sizeof frame);result=-1;
        runtime_cover_fixture_args[0]=&owner;runtime_cover_fixture_args[1]=frame;runtime_cover_fixture_args[2]=&result;
        runtime_cover_fixture_calls=runtime_cover_fixture_order=0;runtime_cover_fixture_bad=0;runtime_cover_fixture_redirect=redirect;SetLastError(37);
        runtime_cover_knockback_forward(&owner,frame,&result,runtime_cover_fixture_original,runtime_cover_fixture_before);
        if(runtime_cover_fixture_bad||runtime_cover_fixture_calls!=1||runtime_cover_fixture_order!=2||GetLastError()!=53||result!=(redirect?0:1))return 0;
        frame[5]=before[5];if(memcmp(before,frame,sizeof frame))return 0; /* Node/Object/locals unchanged. */
    }
    frame[5]=(uintptr_t)code+1;
    if(runtime_cover_knockback_redirect(1,(uintptr_t)frame,&script)||frame[5]!=(uintptr_t)code+1)return 0;
    frame[5]=(uintptr_t)code;code[RUNTIME_COVER_FALSE_OFFSET+1]=0x27;
    if(runtime_cover_knockback_redirect(1,(uintptr_t)frame,&script)||frame[5]!=(uintptr_t)code)return 0;
    int ok=runtime_cover_knockback_cover_policy(1,123,0,0)&&!runtime_cover_knockback_cover_policy(0,123,0,0)&&
        !runtime_cover_knockback_cover_policy(1,0,0,0)&&!runtime_cover_knockback_cover_policy(1,123,-1,0)&&
        !runtime_cover_knockback_cover_policy(1,123,0,11);
    SetLastError(saved);return ok;
}
#endif
#endif
