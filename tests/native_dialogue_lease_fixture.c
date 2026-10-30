/* The runner inserts the actual production lease structs and functions.
 * Only engine identity/read/call boundaries are replaced with deterministic
 * local controls; this program never loads a game DLL or opens a process. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef struct GuestCameraStamp {uintptr_t pointer;} GuestCameraStamp;
typedef struct DialogueChoiceCatalog {uint64_t nonce,generation;} DialogueChoiceCatalog;
typedef struct GuestPlayerContextMethod {GuestCameraStamp function;} GuestPlayerContextMethod;
/* PRODUCTION_STRUCTS */
typedef struct RuntimeDialogueChoices {
    uintptr_t base;RuntimeDialoguePanel lease;GuestPlayerContextMethod script[12];int busy;
} RuntimeDialogueChoices;
static RuntimeDialogueChoices runtime_dialogue;
static struct {
    uintptr_t panel;int unknown,full,fail_set,fail_options,options_make_unknown,values[5];
    unsigned set_calls[5],option_calls,option_applied,hide_calls,shown,refusals;
} engine;
typedef void (*ProcessEvent)(void*,void*,void*,void*);
static void fixture_process_event(void *self,void *fn,void *parameters,void *result) {
    (void)self;(void)fn;(void)parameters;(void)result;++engine.hide_calls;
}
static int guest_camera_stamp_read(uintptr_t base,uintptr_t object,GuestCameraStamp *out,int full) {
    (void)base;(void)full;out->pointer=object;return object!=0;
}
static int guest_camera_stamp_matches(uintptr_t base,const GuestCameraStamp *stamp,int full) {
    (void)base;(void)full;return stamp->pointer!=0;
}
static int runtime_scene_camera_stamp_state(uintptr_t base,const GuestCameraStamp *stamp) {
    (void)base;return engine.unknown?0:stamp->pointer?1:-1;
}
static int runtime_dialogue_query_full(uintptr_t base,uintptr_t world,uintptr_t *full) {
    (void)base;(void)world;*full=engine.full?99:0;return 1;
}
static int runtime_dialogue_guest_local_free(void) {return !engine.full;}
static uintptr_t runtime_dialogue_find_panel(uintptr_t base,uintptr_t pc,uintptr_t world,RuntimeDialogueRecord *record) {
    (void)base;(void)pc;(void)world;*record=(RuntimeDialogueRecord){1,2,3,0};return engine.panel;
}
static int runtime_dialogue_panel_current(uintptr_t base,const RuntimeDialoguePanel *p) {
    (void)base;return !engine.unknown&&p->active&&p->panel.pointer==engine.panel;
}
static int runtime_dialogue_method_live(uintptr_t base,const GuestPlayerContextMethod *method,uintptr_t entry) {
    (void)base;(void)method;(void)entry;return 1;
}
static int runtime_dialogue_body(uintptr_t base,uintptr_t function,unsigned kind) {
    (void)base;(void)function;(void)kind;return 1;
}
static int runtime_dialogue_ui_value(uintptr_t panel,unsigned bit,int set,uint32_t *value) {
    (void)panel;if(bit>=5)return 0;
    if(set){++engine.set_calls[bit];
        if((int)bit==engine.fail_set){engine.unknown=1;return 0;}
        engine.values[bit]=!!*value;
    }else *value=(uint32_t)engine.values[bit];
    return 1;
}
static int runtime_dialogue_options(uintptr_t panel,const DialogueChoiceCatalog *catalog,int marker) {
    (void)panel;(void)catalog;(void)marker;++engine.option_calls;
    if(engine.fail_options)return 0; /* The native ProcessEvent was not called. */
    ++engine.option_applied;
    if(engine.options_make_unknown)engine.unknown=1;
    return 1;
}
static void runtime_dialogue_note(const char *event) {
    if(!strcmp(event,"guest-native-wheel-presented"))++engine.shown;
}
static void runtime_dialogue_panel_refused(unsigned gate,const char *reason,const DialogueChoiceCatalog *catalog) {
    (void)gate;(void)reason;(void)catalog;++engine.refusals;
}
/* PRODUCTION_FUNCTIONS */
static void reset(void) {
    memset(&runtime_dialogue,0,sizeof runtime_dialogue);memset(&engine,0,sizeof engine);
    runtime_dialogue.base=(uintptr_t)fixture_process_event-0xf4f80;
    engine.panel=16;engine.fail_set=-1;
}
#define CHECK(test) do{if(!(test)){fprintf(stderr,"line %d: %s\n",__LINE__,#test);return 1;}}while(0)
int main(void) {
    DialogueChoiceCatalog c={1,1};reset();engine.fail_set=2;
    /* A real setter failure starts retirement, whose identity read is then
     * temporarily unknown. It must retain the original ownership ledger. */
    CHECK(!runtime_dialogue_lease_begin(24,32,&c));
    CHECK(runtime_dialogue.lease.active&&!runtime_dialogue.lease.ready&&runtime_dialogue.lease.retiring);
    CHECK(runtime_dialogue.lease.owned==7&&engine.option_calls==0&&engine.shown==0);
    c.generation=2;
    CHECK(!runtime_dialogue_lease_begin(24,32,&c));
    CHECK(runtime_dialogue.lease.generation==0&&engine.option_calls==0&&engine.shown==0);
    /* A native/foreign replacement of one written flag wins during cleanup.
     * The retry may acquire a new complete lease only after cleanup succeeds. */
    engine.values[0]=0;engine.unknown=0;engine.fail_set=-1;
    CHECK(runtime_dialogue_lease_begin(24,32,&c));
    CHECK(runtime_dialogue.lease.active&&runtime_dialogue.lease.ready&&!runtime_dialogue.lease.retiring);
    CHECK(runtime_dialogue.lease.generation==2&&engine.option_calls==1&&engine.shown==1);
    CHECK(engine.set_calls[0]==2); /* Initial and new admission; no foreign-state restore. */
    for(unsigned i=0;i<5;++i)CHECK(engine.values[i]==1);
    unsigned sets[5];memcpy(sets,engine.set_calls,sizeof sets);
    c.generation=3;CHECK(runtime_dialogue_lease_begin(24,32,&c));
    CHECK(runtime_dialogue.lease.ready&&runtime_dialogue.lease.generation==3&&engine.option_calls==2);
    CHECK(!memcmp(sets,engine.set_calls,sizeof sets)&&engine.shown==1);
    /* Starting cleanup revokes readiness immediately, even if the first
     * identity read is unknown. A later catalog cannot revive that lease. */
    engine.unknown=1;runtime_dialogue_lease_retire();c.generation=4;
    CHECK(runtime_dialogue.lease.active&&!runtime_dialogue.lease.ready&&runtime_dialogue.lease.retiring);
    CHECK(!runtime_dialogue_lease_begin(24,32,&c)&&engine.option_calls==2);
    engine.unknown=0;engine.full=1;
    CHECK(!runtime_dialogue_lease_begin(24,32,&c));
    CHECK(!runtime_dialogue.lease.active&&engine.option_calls==2&&engine.hide_calls==0);
    for(unsigned i=0;i<5;++i)CHECK(engine.values[i]==1); /* Native conversation takes ownership. */
    reset();c.generation=1;engine.fail_set=2;
    CHECK(!runtime_dialogue_lease_begin(24,32,&c));
    engine.unknown=0;engine.panel=48;engine.fail_set=-1;c.generation=2;
    CHECK(!runtime_dialogue_lease_begin(24,32,&c));
    CHECK(runtime_dialogue.lease.active&&!runtime_dialogue.lease.ready&&engine.option_calls==0);
    CHECK(runtime_dialogue.lease.panel.pointer==16); /* Foreign panel not adopted. */
    reset();c.generation=1;CHECK(runtime_dialogue_lease_begin(24,32,&c));
    engine.fail_options=1;c.generation=2;
    CHECK(!runtime_dialogue_lease_begin(24,32,&c));
    CHECK(!runtime_dialogue.lease.active&&!runtime_dialogue.lease.ready&&engine.hide_calls==1);
    for(unsigned i=0;i<5;++i)CHECK(engine.values[i]==0);
    /* Native options are written before the post-call identity read. With
     * all five baseline flags already true, no flag cleanup would hide a
     * missed options write: the explicit widget obligation is essential. */
    reset();c.generation=1;engine.options_make_unknown=1;
    for(unsigned i=0;i<5;++i)engine.values[i]=1;
    CHECK(!runtime_dialogue_lease_begin(24,32,&c));
    CHECK(engine.option_applied==1&&runtime_dialogue.lease.active&&runtime_dialogue.lease.options);
    CHECK(!runtime_dialogue.lease.ready&&runtime_dialogue.lease.retiring&&!runtime_dialogue.lease.owned);
    c.generation=2;CHECK(!runtime_dialogue_lease_begin(24,32,&c)&&engine.option_applied==1);
    engine.unknown=0;runtime_dialogue_lease_retire();
    CHECK(!runtime_dialogue.lease.active&&engine.hide_calls==1);
    for(unsigned i=0;i<5;++i)CHECK(engine.values[i]==1);
    /* The already-admitted catalog path must record an options write too.
     * Native replacement may have relinquished the previous strings. */
    reset();c.generation=1;CHECK(runtime_dialogue_lease_begin(24,32,&c));
    runtime_dialogue.lease.options=0;engine.options_make_unknown=1;c.generation=2;
    CHECK(!runtime_dialogue_lease_begin(24,32,&c));
    CHECK(engine.option_applied==2&&runtime_dialogue.lease.options&&runtime_dialogue.lease.retiring);
    CHECK(runtime_dialogue.lease.generation==1&&!runtime_dialogue.lease.ready);
    engine.unknown=0;runtime_dialogue_lease_retire();
    CHECK(!runtime_dialogue.lease.active&&engine.hide_calls==1);
    /* A real conversation or a foreign panel must never be hidden merely
     * because the retained ledger says our earlier options were written. */
    reset();c.generation=1;engine.options_make_unknown=1;
    for(unsigned i=0;i<5;++i)engine.values[i]=1;
    CHECK(!runtime_dialogue_lease_begin(24,32,&c)&&runtime_dialogue.lease.options);
    engine.unknown=0;engine.full=1;runtime_dialogue_lease_retire();
    CHECK(!runtime_dialogue.lease.active&&engine.hide_calls==0);
    for(unsigned i=0;i<5;++i)CHECK(engine.values[i]==1);
    reset();c.generation=1;engine.options_make_unknown=1;
    CHECK(!runtime_dialogue_lease_begin(24,32,&c)&&runtime_dialogue.lease.options);
    engine.unknown=0;engine.panel=48;runtime_dialogue_lease_retire();
    CHECK(runtime_dialogue.lease.active&&runtime_dialogue.lease.options&&engine.hide_calls==0);
    c.generation=2;CHECK(!runtime_dialogue_lease_begin(24,32,&c)&&engine.option_applied==1);
    /* A failed pre-native-call validation creates no widget obligation. */
    reset();c.generation=1;engine.fail_options=1;
    for(unsigned i=0;i<5;++i)engine.values[i]=1;
    CHECK(!runtime_dialogue_lease_begin(24,32,&c));
    CHECK(engine.option_calls==1&&engine.option_applied==0&&engine.hide_calls==0&&!runtime_dialogue.lease.active);
    return 0;
}
