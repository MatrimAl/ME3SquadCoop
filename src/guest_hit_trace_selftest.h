#ifndef ME3_GUEST_HIT_TRACE_SELFTEST_H
#define ME3_GUEST_HIT_TRACE_SELFTEST_H
typedef struct GuestHitFixture {
    GuestHitObserver observer;GuestHitArgs args;GuestHitFrame records[8];
    unsigned calls,samples,emits;int bad,nested,depth,mutate,read_fail,authority;unsigned long generation;
    float health;
} GuestHitFixture;
static GuestHitFixture *guest_hit_fixture;
static GuestHitKey guest_hit_fixture_key(uintptr_t pointer) {
    GuestHitKey key={pointer,pointer+10,(int32_t)pointer,(int32_t)pointer+10,{1,0},{2,0}};return key;
}
static int guest_hit_fixture_eligible(void *context,const GuestHitArgs *args,uintptr_t caller,GuestHitFrame *frame) {
    GuestHitFixture *f=context;
    if(!guest_hit_call_matches(caller,100,(uintptr_t)args->self,(uintptr_t)f->args.self,
        (uintptr_t)args->function,(uintptr_t)f->args.function,args->params!=NULL,args->result==NULL,f->authority))return 0;
    frame->generation=f->generation;frame->target=30;return 1;
}
static int guest_hit_fixture_same(void *context,const GuestHitArgs *args,const GuestHitFrame *frame) {
    (void)args;return frame->generation==((GuestHitFixture*)context)->generation;
}
static void guest_hit_fixture_sample(void *context,uintptr_t target,GuestHitSample *out) {
    GuestHitFixture *f=context;memset(out,0,sizeof *out);++f->samples;
    out->target=guest_hit_fixture_key(target);out->level=guest_hit_fixture_key(40);out->module=guest_hit_fixture_key(50);
    out->target_valid=out->health_valid=out->shield_valid=1;out->health=f->health;
    if(f->mutate==1&&f->calls)++out->target.index;
    if(f->mutate==2&&f->calls)++out->module.index;
    if(f->read_fail&&f->calls)out->health_valid=out->shield_valid=0;
    SetLastError(700);
}
static void guest_hit_fixture_emit(void *context,const GuestHitFrame *frame,int leaving) {
    GuestHitFixture *f=context;if(f->emits>=8){f->bad=1;return;}
    if(frame!=f->observer.current||(!leaving&&frame->scope_after))f->bad=1;
    f->records[f->emits++]=*frame;SetLastError(701);
}
static const GuestHitOps guest_hit_fixture_ops={guest_hit_fixture_eligible,guest_hit_fixture_same,guest_hit_fixture_sample,guest_hit_fixture_emit};
static void guest_hit_fixture_forward(void *self,void *function,void *params,void *result) {
    GuestHitFixture *f=guest_hit_fixture;++f->calls;
    if(self!=f->args.self||function!=f->args.function||params!=f->args.params||result!=f->args.result||GetLastError()!=42)f->bad=1;
    if(result)*(int*)result=987;
    if(f->nested&&!f->depth){
        ++f->depth;GuestHitFrame *parent=f->observer.current;
        SetLastError(42);guest_hit_observe(&f->observer,guest_hit_fixture_forward,&f->args,99,1,&guest_hit_fixture_ops,f);
        if(f->observer.current!=parent||GetLastError()!=123)f->bad=1;
        --f->depth;
    }
    if(f->mutate==3)++f->generation;f->health-=10;SetLastError(123);
}
static void guest_hit_fixture_reset(GuestHitFixture *f) {
    memset(f,0,sizeof *f);f->args.self=(void*)10;f->args.function=(void*)20;f->args.params=(void*)30;
    f->generation=1;f->authority=1;f->health=100;guest_hit_fixture=f;
}
static int guest_hit_selftest(void) {
    DWORD error=GetLastError();GuestHitFixture fixture,*saved_fixture=guest_hit_fixture;int ok=1;
    ProcessEvent saved_original=guest_hit_original;int saved_installed=guest_hit_installed;
    uintptr_t saved_base=guest_hit_base,saved_registered=guest_hit_registered;
    guest_hit_fixture_reset(&fixture);guest_hit_installed=0;guest_hit_original=guest_hit_fixture_forward;
    int result=3;fixture.args.result=&result;SetLastError(42);
    guest_hit_exec(fixture.args.self,fixture.args.function,fixture.args.params,fixture.args.result);
    ok=ok&&fixture.calls==1&&!fixture.bad&&result==987&&GetLastError()==123&&!fixture.emits;
    guest_hit_fixture_reset(&fixture);fixture.args.params=NULL;fixture.args.result=NULL;SetLastError(42);
    guest_hit_exec(fixture.args.self,fixture.args.function,NULL,NULL);
    ok=ok&&fixture.calls==1&&!fixture.bad&&GetLastError()==123;
    guest_hit_fixture_reset(&fixture);SetLastError(42);
    guest_hit_observe(&fixture.observer,guest_hit_fixture_forward,&fixture.args,100,0,&guest_hit_fixture_ops,&fixture);
    ok=ok&&fixture.calls==1&&!fixture.samples&&!fixture.emits&&!fixture.bad&&GetLastError()==123;
    guest_hit_fixture_reset(&fixture);SetLastError(42);
    guest_hit_observe(&fixture.observer,guest_hit_fixture_forward,&fixture.args,100,1,&guest_hit_fixture_ops,&fixture);
    ok=ok&&fixture.calls==1&&fixture.samples==2&&fixture.emits==2&&!fixture.bad&&GetLastError()==123&&
        fixture.observer.current==NULL&&fixture.observer.rows==2&&guest_hit_health_pair(&fixture.records[1])&&
        fixture.records[1].before.health==100&&fixture.records[1].after.health==90&&guest_hit_shield_pair(&fixture.records[1]);
    guest_hit_fixture_reset(&fixture);fixture.nested=1;SetLastError(42);
    guest_hit_observe(&fixture.observer,guest_hit_fixture_forward,&fixture.args,100,1,&guest_hit_fixture_ops,&fixture);
    ok=ok&&fixture.calls==2&&fixture.emits==2&&!fixture.bad&&fixture.observer.current==NULL&&
        fixture.records[1].nested_calls==1&&fixture.records[1].parent==0&&GetLastError()==123;
    for(int mutation=1;mutation<=3;++mutation){
        guest_hit_fixture_reset(&fixture);fixture.mutate=mutation;SetLastError(42);
        guest_hit_observe(&fixture.observer,guest_hit_fixture_forward,&fixture.args,100,1,&guest_hit_fixture_ops,&fixture);
        ok=ok&&fixture.calls==1&&!fixture.bad&&fixture.emits==2&&!guest_hit_health_pair(&fixture.records[1]);
        if(mutation!=2)ok=ok&&!guest_hit_shield_pair(&fixture.records[1]);
    }
    guest_hit_fixture_reset(&fixture);fixture.read_fail=1;SetLastError(42);
    guest_hit_observe(&fixture.observer,guest_hit_fixture_forward,&fixture.args,100,1,&guest_hit_fixture_ops,&fixture);
    ok=ok&&fixture.calls==1&&fixture.emits==2&&!guest_hit_health_pair(&fixture.records[1])&&!guest_hit_shield_pair(&fixture.records[1]);
    guest_hit_fixture_reset(&fixture);fixture.observer.rows=126;SetLastError(42);
    guest_hit_observe(&fixture.observer,guest_hit_fixture_forward,&fixture.args,100,1,&guest_hit_fixture_ops,&fixture);
    SetLastError(42);guest_hit_observe(&fixture.observer,guest_hit_fixture_forward,&fixture.args,100,1,&guest_hit_fixture_ops,&fixture);
    ok=ok&&fixture.calls==2&&fixture.emits==2&&fixture.samples==2&&fixture.observer.rows==128&&!fixture.bad;
    ok=ok&&guest_hit_call_matches(1,1,2,2,3,3,1,1,1)&&
        !guest_hit_call_matches(4,1,2,2,3,3,1,1,1)&&!guest_hit_call_matches(1,1,4,2,3,3,1,1,1)&&
        !guest_hit_call_matches(1,1,2,2,4,3,1,1,1)&&!guest_hit_call_matches(1,1,2,2,3,3,0,1,1)&&
        !guest_hit_call_matches(1,1,2,2,3,3,1,0,1)&&!guest_hit_call_matches(1,1,2,2,3,3,1,1,0);
    GuestHitFrame pair=fixture.records[1];pair.before.shield_present=pair.after.shield_present=1;
    pair.before.inventory=pair.after.inventory=guest_hit_fixture_key(60);pair.before.shield=pair.after.shield=guest_hit_fixture_key(70);
    pair.before.shields=100;pair.after.shields=90;ok=ok&&guest_hit_shield_pair(&pair);
    ++pair.after.shield.index;ok=ok&&!guest_hit_shield_pair(&pair);--pair.after.shield.index;
    ++pair.after.inventory.index;ok=ok&&!guest_hit_shield_pair(&pair);--pair.after.inventory.index;
    pair.after.shields=NAN;ok=ok&&!guest_hit_shield_pair(&pair);
    pair.after.shields=90;pair.scope_after=0;ok=ok&&!guest_hit_health_pair(&pair)&&!guest_hit_shield_pair(&pair);
    GuestHitObserver budget={0};budget.rows=126;ok=ok&&guest_hit_reserve(&budget)&&!guest_hit_reserve(&budget)&&budget.rows==128;
    unsigned char *memory=VirtualAlloc(NULL,0x1500000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    if(!memory)ok=0;
    else {
        uintptr_t base=(uintptr_t)memory,weapon=base+0x1000,slot=base+0x14fa430+0x230;
        memcpy(memory+0x40d450,guest_hit_actor_pe_pin,sizeof guest_hit_actor_pe_pin);
        memcpy(memory+0xb1f654,guest_hit_slot_load_pin,sizeof guest_hit_slot_load_pin);
        memcpy(memory+0xb1f6cb,guest_hit_call_pin,sizeof guest_hit_call_pin);
        *(uintptr_t*)weapon=base+0x14fa430;*(uintptr_t*)slot=base+0x40d450;
        guest_hit_installed=0;guest_hit_base=base;guest_hit_registered=0;guest_hit_original=(ProcessEvent)(base+0x40d450);
        ok=ok&&guest_hit_dispatch_valid(base,weapon,0)&&guest_equipment_weapon_pe_valid(base,weapon,base+0x40d450)&&
            !guest_equipment_weapon_pe_valid(base,weapon,(uintptr_t)guest_hit_exec)&&!guest_equipment_weapon_pe_valid(base,weapon,999);
        *(uintptr_t*)slot=(uintptr_t)guest_hit_exec;guest_hit_installed=1;guest_hit_registered=(uintptr_t)guest_hit_exec;
        for(int epoch=0;epoch<2;++epoch)ok=ok&&guest_equipment_weapon_pe_valid(base,weapon,(uintptr_t)guest_hit_exec);
        ++guest_hit_registered;++*(uintptr_t*)slot;
        ok=ok&&!guest_equipment_weapon_pe_valid(base,weapon,(uintptr_t)guest_hit_exec);--guest_hit_registered;--*(uintptr_t*)slot;
        guest_hit_original=(ProcessEvent)(base+0x40d451);ok=ok&&!guest_equipment_weapon_pe_valid(base,weapon,(uintptr_t)guest_hit_exec);
        guest_hit_original=(ProcessEvent)(base+0x40d450);*(uintptr_t*)weapon+=8;
        ok=ok&&!guest_equipment_weapon_pe_valid(base,weapon,(uintptr_t)guest_hit_exec);*(uintptr_t*)weapon-=8;
        ++*(uintptr_t*)slot;ok=ok&&!guest_equipment_weapon_pe_valid(base,weapon,(uintptr_t)guest_hit_exec);--*(uintptr_t*)slot;
        memory[0xb1f6cb]^=1;ok=ok&&!guest_equipment_weapon_pe_valid(base,weapon,(uintptr_t)guest_hit_exec);memory[0xb1f6cb]^=1;
        memory[0x40d450]^=1;ok=ok&&!guest_equipment_weapon_pe_valid(base,weapon,(uintptr_t)guest_hit_exec);memory[0x40d450]^=1;
        float value=0;*(float*)(memory+0x2000)=1;ok=ok&&guest_hit_read_value(base+0x2000,&value)&&value==1;
        *(float*)(memory+0x2000)=NAN;ok=ok&&!guest_hit_read_value(base+0x2000,&value);
        DWORD old=0;*(float*)(memory+0x2000)=1;
        ok=ok&&VirtualProtect(memory+0x2000,4096,PAGE_NOACCESS,&old)&&!guest_hit_read_value(base+0x2000,&value);
        VirtualFree(memory,0,MEM_RELEASE);
    }
    guest_hit_original=saved_original;guest_hit_installed=saved_installed;guest_hit_base=saved_base;guest_hit_registered=saved_registered;
    guest_hit_fixture=saved_fixture;SetLastError(error);return ok;
}
#endif
