#ifndef ME3_RUNTIME_GUEST_TRAVERSAL_HANDOFF_H
#define ME3_RUNTIME_GUEST_TRAVERSAL_HANDOFF_H
/* BioPawn.UnPossessed clears bIsAPlayer before a later GapJump.Stop could
 * restore its collision exception. Finish only our actual owned jump through
 * stock EndCustomAction before forwarding that real callback on either peer.
 * Never delay story handoff, clear action fields or replay Stop ourselves. */
typedef void (*RuntimeTraversalExec)(void*,void*,void*);
typedef struct RuntimeTraversalHandoff {
    uintptr_t base;
    GuestPlayerContextMethod unpossessed,end;
    RuntimeTraversalExec original;
    int installed,busy;
    unsigned rows;
} RuntimeTraversalHandoff;
static RuntimeTraversalHandoff runtime_traversal_handoff;
static void runtime_traversal_handoff_exec(void*,void*,void*);
/* Exact supported-package VM bodies and operand identities. */
#include "runtime_guest_traversal_handoff_pins.h"
static int runtime_traversal_handoff_method(uintptr_t base,const GuestPlayerContextMethod *method,uintptr_t expected,int end) {
    uintptr_t fn=method->function.pointer;uint32_t flags=0;uint16_t index=1,size=1,ret=0;unsigned char count=1;GameArray script={0};
    return guest_player_context_method_matches(base,method,1)&&read_ptr(fn+0xd8)==expected&&
        !strcmp(method->function.full_name,end?"SFXGame.BioPawn.EndCustomAction":"SFXGame.BioPawn.UnPossessed")&&
        read_at(fn+0xe0,&flags,4)&&flags==(end?0x20103:RUNTIME_TRAVERSAL_UNPOSSESSED_FLAGS)&&
        read_at(fn+0xe4,&index,2)&&!index&&read_at(fn+0xe6,&size,2)&&!size&&
        read_at(fn+0xe8,&ret,2)&&ret==65535&&read_at(fn+0xea,&count,1)&&!count&&
        !read_ptr(fn+0x70)&&read_at(fn+0x7c,&script,sizeof script)&&script.data&&
        script.count==(end?16:RUNTIME_TRAVERSAL_UNPOSSESSED_SIZE)&&script.max>=script.count&&script.max<=4096;
}
/* Cleanup deliberately does not require a still-live network channel: the
 * real disconnect callback is precisely when the channel may have retired. */
static int runtime_traversal_handoff_owned(uintptr_t base,uintptr_t pawn) {
    RuntimeGuestTraversal *q=&runtime_guest_traversal;int32_t action=0,power=-1;uint32_t flags=0;
    if(!q->valid||q->scope.pawn.pointer!=pawn||!runtime_movement_arrays_live(base,&q->scope)||
       !session_handle_matches(q->scope.pc)||!narrative_probe_object(base,q->scope.pc.pointer,"SFXPlayerController")||
       read_ptr(pawn+0x374)!=q->scope.pc.pointer||read_ptr(q->scope.pc.pointer+0x3bc)!=pawn||
       read_ptr(q->scope.pc.pointer+0x198)!=q->scope.info.pointer||
       !read_at(pawn+0xe20,&flags,4)||!(flags&0x10)||!read_at(pawn+0xc20,&action,4)||
       (action!=28&&action!=29)||!read_at(pawn+0xc28,&power,4)||power)return 0;
    unsigned row=(unsigned)guest_traversal_row(action);RuntimeMovementRow *r=&q->rows[row].slot;
    if(!r->changed||!runtime_traversal_observe_row(base,q,row,0)||!r->observed)return 0;
    uintptr_t instance=r->new_instance.pointer;
    return runtime_traversal_instance(base,instance,r->new_class.pointer,pawn)&&
        read_ptr(instance+0xa0)==q->scope.pc.pointer&&!read_ptr(instance+0xa8);
}
typedef struct RuntimeTraversalEndContext {RuntimeMovementScope scope;uintptr_t pawn;} RuntimeTraversalEndContext;
static int runtime_traversal_handoff_can_end(void *context) {
    RuntimeTraversalEndContext *c=context;RuntimeTraversalHandoff *s=&runtime_traversal_handoff;
    return runtime_movement_scope_same(&c->scope,&runtime_guest_traversal.scope)&&
        runtime_traversal_handoff_owned(s->base,c->pawn)&&
        runtime_traversal_handoff_method(s->base,&s->end,s->base+0xf52b0,1)&&
        guest_equipment_find_first(s->base,read_ptr(c->pawn+0x50),"EndCustomAction")==s->end.function.pointer&&
        read_ptr(read_ptr(c->pawn)+0x230)==s->base+0x40d450;
}
static void runtime_traversal_handoff_end(void *context) {
    RuntimeTraversalEndContext *c=context;RuntimeTraversalHandoff *s=&runtime_traversal_handoff;unsigned char parms[8]={0};
    ((ProcessEvent)(s->base+0x40d450))((void*)c->pawn,(void*)s->end.function.pointer,parms,NULL);
    if(s->rows++<32)line("{\"runtime_guest_traversal\":\"owned-jump-end-before-unpossessed\",\"native_end_dispatched\":true,\"collision_written\":false,\"gameplay_verified\":false}\r\n");
}
static unsigned runtime_traversal_handoff_finish(void *context,int (*owned)(void*),void (*end)(void*)) {
    unsigned calls=0;for(unsigned pass=0;pass<2;++pass){if(!owned(context))break;end(context);++calls;}return calls;
}
static void runtime_traversal_handoff_before(void *self,void *frame) {
    RuntimeTraversalHandoff *s=&runtime_traversal_handoff;GameArray script={0};uintptr_t pawn=(uintptr_t)self;
    if(!s->installed||s->busy||GetCurrentThreadId()!=session_game_thread||
       !runtime_traversal_handoff_method(s->base,&s->unpossessed,(uintptr_t)runtime_traversal_handoff_exec,0)||
       read_ptr((uintptr_t)frame+0x18)!=s->unpossessed.function.pointer||read_ptr((uintptr_t)frame+0x20)!=pawn||
       !read_at(s->unpossessed.function.pointer+0x7c,&script,sizeof script)||read_ptr((uintptr_t)frame+0x28)!=script.data)return;
    s->busy=1;
    /* DoCustomAction(0) can consume the single replicated buffered action on
     * a client. A second call is permitted only if that is again our jump. */
    RuntimeTraversalEndContext c={runtime_guest_traversal.scope,pawn};
    runtime_traversal_handoff_finish(&c,runtime_traversal_handoff_can_end,runtime_traversal_handoff_end);
    s->busy=0;
}
static void runtime_traversal_handoff_forward(void *self,void *frame,void *result,RuntimeTraversalExec stock,void (*before)(void*,void*)) {
    DWORD error=GetLastError();before(self,frame);SetLastError(error);stock(self,frame,result);
}
static void runtime_traversal_handoff_exec(void *self,void *frame,void *result) {
    runtime_traversal_handoff_forward(self,frame,result,runtime_traversal_handoff.original,runtime_traversal_handoff_before);
}
static int runtime_traversal_handoff_ready(uintptr_t base,uintptr_t pawn) {
    RuntimeTraversalHandoff *s=&runtime_traversal_handoff;
    if(s->installed)return s->base==base&&runtime_traversal_handoff_method(base,&s->unpossessed,(uintptr_t)runtime_traversal_handoff_exec,0)&&
        runtime_traversal_handoff_method(base,&s->end,base+0xf52b0,1)&&
        guest_equipment_find_first(base,read_ptr(pawn+0x50),"UnPossessed")==s->unpossessed.function.pointer&&
        guest_equipment_find_first(base,read_ptr(pawn+0x50),"EndCustomAction")==s->end.function.pointer;
    uintptr_t klass=read_ptr(pawn+0x50),unpossessed=guest_player_context_find_method(base,klass,"BioPawn","UnPossessed"),
        end=guest_player_context_find_method(base,klass,"BioPawn","EndCustomAction");
    if(!supported_image(base)||!guest_equipment_dispatch_pins(base)||
       !guest_player_context_method_stamp(base,unpossessed,&s->unpossessed)||!guest_player_context_method_stamp(base,end,&s->end)||
       !runtime_traversal_handoff_method(base,&s->unpossessed,base+0xf52b0,0)||!runtime_traversal_handoff_method(base,&s->end,base+0xf52b0,1)||
       !runtime_traversal_handoff_bodies(base,unpossessed,end)||guest_equipment_find_first(base,klass,"UnPossessed")!=unpossessed||
       guest_equipment_find_first(base,klass,"EndCustomAction")!=end||read_ptr(read_ptr(pawn)+0x230)!=base+0x40d450||
       !background_focus_writable(unpossessed+0xd8,8))return 0;
    s->base=base;s->original=(RuntimeTraversalExec)(base+0xf52b0);
    if(InterlockedCompareExchangePointer((PVOID volatile*)(unpossessed+0xd8),(PVOID)runtime_traversal_handoff_exec,(PVOID)s->original)!=(PVOID)s->original)return 0;
    s->installed=1;
    line("{\"runtime_guest_traversal\":\"owned-jump-unpossessed-guard-installed\",\"original_callback_preserved\":true,\"gameplay_verified\":false}\r\n");return 1;
}
#ifdef ME3_RUNTIME_FIXTURE
static void *runtime_traversal_fixture_args[3];static unsigned runtime_traversal_fixture_order,runtime_traversal_fixture_calls;static int runtime_traversal_fixture_bad;
static void runtime_traversal_fixture_before(void *self,void *frame) {
    if(self!=runtime_traversal_fixture_args[0]||frame!=runtime_traversal_fixture_args[1]||runtime_traversal_fixture_order++)runtime_traversal_fixture_bad=1;
    SetLastError(17);
}
static void runtime_traversal_fixture_original(void *self,void *frame,void *result) {
    if(self!=runtime_traversal_fixture_args[0]||frame!=runtime_traversal_fixture_args[1]||result!=runtime_traversal_fixture_args[2]||
       runtime_traversal_fixture_order++!=1||GetLastError()!=37)runtime_traversal_fixture_bad=1;
    ++runtime_traversal_fixture_calls;SetLastError(53);
}
typedef struct RuntimeTraversalFinishFixture {unsigned remaining,calls;int foreign;} RuntimeTraversalFinishFixture;
static int runtime_traversal_finish_fixture_owned(void *context) {
    RuntimeTraversalFinishFixture *c=context;return c->remaining&&c->foreign<=0;
}
static void runtime_traversal_finish_fixture_end(void *context) {
    RuntimeTraversalFinishFixture *c=context;++c->calls;--c->remaining;if(c->foreign<0)c->foreign=1;
}
static int runtime_traversal_handoff_selftest(void) {
    DWORD saved=GetLastError();int a,b,c;
    runtime_traversal_fixture_args[0]=&a;runtime_traversal_fixture_args[1]=&b;runtime_traversal_fixture_args[2]=&c;
    runtime_traversal_fixture_order=runtime_traversal_fixture_calls=0;runtime_traversal_fixture_bad=0;SetLastError(37);
    runtime_traversal_handoff_forward(&a,&b,&c,runtime_traversal_fixture_original,runtime_traversal_fixture_before);
    int ok=runtime_traversal_fixture_calls==1&&runtime_traversal_fixture_order==2&&!runtime_traversal_fixture_bad&&GetLastError()==53;
    RuntimeTraversalFinishFixture f={2,0,0};
    ok=ok&&runtime_traversal_handoff_finish(&f,runtime_traversal_finish_fixture_owned,runtime_traversal_finish_fixture_end)==2&&f.calls==2&&!f.remaining;
    f=(RuntimeTraversalFinishFixture){3,0,0};
    ok=ok&&runtime_traversal_handoff_finish(&f,runtime_traversal_finish_fixture_owned,runtime_traversal_finish_fixture_end)==2&&f.calls==2&&f.remaining==1;
    f=(RuntimeTraversalFinishFixture){2,0,1};
    ok=ok&&!runtime_traversal_handoff_finish(&f,runtime_traversal_finish_fixture_owned,runtime_traversal_finish_fixture_end)&&!f.calls;
    f=(RuntimeTraversalFinishFixture){2,0,-1};
    ok=ok&&runtime_traversal_handoff_finish(&f,runtime_traversal_finish_fixture_owned,runtime_traversal_finish_fixture_end)==1&&f.calls==1&&f.remaining==1;
    SetLastError(saved);return ok;
}
#endif
#endif
