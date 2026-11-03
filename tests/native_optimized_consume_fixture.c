/* Authored memory only; no game/native body/process access. */
#define main observer_fixture_main
#define ME3_OPTIMIZED_OWNER_CONSUME_CANDIDATE_H
#include "native_optimized_consume_base.c"
#undef ME3_OPTIMIZED_OWNER_CONSUME_CANDIDATE_H
#undef main
#undef InterlockedCompareExchangePointer
static int qa=1,cas_conflict,cas_count,or_count,nested_rejected,original_owner=-1;
static DWORD fake_env(LPCWSTR name,LPWSTR value,DWORD size){(void)name;if(qa&&size>=2){value[0]=L'1';value[1]=0;return 1;}return 0;}
#define GetEnvironmentVariableW fake_env
static LONG fixture_exchange(volatile LONG *address,LONG replacement,LONG expected){
 CHECK(address==(volatile LONG*)(pawn+0x264));cas_count++;
 if(cas_conflict){*address^=0x100;return *address;}
 LONG previous=*address;if(previous==expected)*address=replacement;return previous;
}
static LONG fixture_or(volatile LONG *address,LONG mask){CHECK(address==(volatile LONG*)(pawn+0x264)&&mask==0x4000);or_count++;LONG previous=*address;*address|=mask;return previous;}
#undef InterlockedCompareExchange
#undef InterlockedOr
#define InterlockedCompareExchange fixture_exchange
#define InterlockedOr fixture_or
#include "optimized_owner_consume_qa.h"
static void trial_reset(void){reset();put32(pawn,0x264,0x4013);optimized_owner_consume_active=optimized_owner_consume_failed=0;cas_conflict=0;cas_count=or_count=0;mode=0;original_owner=-1;}
static void consume_original(void *p,void *a,void *s,void *m,void *c){
 CHECK(p==pawn&&a==archive&&s==shadow&&m==map&&c==channel);CHECK(GetLastError()==123);calls++;
 uint32_t flags=0;memcpy(&flags,pawn+0x264,4);original_owner=!!(flags&0x4000);
 if(mode==1)put32(pawn,0x264,(flags|0x800)&~2u);
 if(mode==2)put32(pawn,0x264,flags|0x4000);
 if(mode==3)put32(pawn,0x38,999); /* replacement identity */
 if(mode==4)put(channel,0xa0,0); /* same pawn, detached channel */
 if(mode==5){
  OptimizedOwnerConsumeLease nested=optimized_owner_consume_begin(&frame,(uintptr_t)pawn,(uintptr_t)archive,(uintptr_t)shadow,(uintptr_t)map,(uintptr_t)channel,initial_actor_trace_base+0x488aeb);
  CHECK(!nested.changed);nested_rejected++;optimized_owner_consume_end(&nested);
 }
 SetLastError(456);
}
static OptimizedOwnerConsumeLease trial(uintptr_t site){
 int old=calls;SetLastError(123);
 OptimizedOwnerConsumeLease lease=optimized_owner_consume_begin(inventory_decode_frame,(uintptr_t)pawn,(uintptr_t)archive,(uintptr_t)shadow,(uintptr_t)map,(uintptr_t)channel,site);
 CHECK(GetLastError()==123);SetLastError(123);consume_original(pawn,archive,shadow,map,channel);DWORD out=GetLastError();
 optimized_owner_consume_end(&lease);CHECK(GetLastError()==out&&out==456&&calls==old+1);
 return lease;
}
static uint32_t flags(void){uint32_t value=0;memcpy(&value,pawn+0x264,4);return value;}
int main(int argc,char **argv){
 if(argc>1&&!strcmp(argv[1],"canary"))CHECK(0);
 optimized_owner_installed=1;uintptr_t b=initial_actor_trace_base;OptimizedOwnerConsumeLease l;
 /* Cached opt-in is process startup policy: separately test off in another process. */
 if(argc>1&&!strcmp(argv[1],"off")){qa=0;trial_reset();l=trial(b+0x488aeb);CHECK(!l.changed&&flags()==0x4013&&original_owner==1&&!cas_count&&!or_count);puts("PASS consume candidate default-off");return 0;}
 for(int i=0;i<2;i++){trial_reset();l=trial(b+(i?0x489adf:0x488aeb));CHECK(l.changed&&l.restore_status==1&&original_owner==0&&flags()==0x4013&&cas_count==1&&or_count==1);}
 trial_reset();mode=1;l=trial(b+0x488aeb);CHECK(l.changed&&l.restore_status==1&&flags()==0x4811);
 trial_reset();mode=2;l=trial(b+0x488aeb);CHECK(l.changed&&l.restore_status==1&&flags()==0x4013);
 trial_reset();mode=3;l=trial(b+0x488aeb);CHECK(l.changed&&l.restore_status==-1&&!or_count&&optimized_owner_consume_failed&&flags()==0x13);
 /* Failure latch refuses further intervention, while wrapper still forwards. */
 put32(pawn,0x38,19);put32(pawn,0x264,0x4013);l=trial(b+0x488aeb);CHECK(!l.changed&&original_owner==1&&flags()==0x4013);
 trial_reset();mode=4;l=trial(b+0x488aeb);CHECK(l.changed&&l.restore_status==1&&flags()==0x4013&&or_count==1);
 trial_reset();mode=5;l=trial(b+0x488aeb);CHECK(l.changed&&l.restore_status==1&&nested_rejected==1&&cas_count==1&&or_count==1);
 trial_reset();cas_conflict=1;l=trial(b+0x488aeb);CHECK(!l.changed&&flags()==0x4113&&original_owner==1&&cas_count==1&&!or_count);
 trial_reset();put32(pawn,0x264,0x13);l=trial(b+0x488aeb);CHECK(!l.changed&&flags()==0x13&&!cas_count&&!or_count);
 trial_reset();put32(archive,0x38,1);l=trial(b+0x488aeb);CHECK(!l.changed&&flags()==0x4013&&!cas_count);
 trial_reset();put32(archive,0x9c,104);l=trial(b+0x488aeb);CHECK(!l.changed&&!cas_count);
 trial_reset();put32(archive,0x9c,99);l=trial(b+0x488aeb);CHECK(!l.changed&&!cas_count);
 trial_reset();put32(pawn,0x25c,0x4000000);l=trial(b+0x488aeb);CHECK(!l.changed&&!cas_count);
 trial_reset();put32(pawn,0x25c,8);l=trial(b+0x488aeb);CHECK(!l.changed&&!cas_count);
 trial_reset();l=trial(b+0x488aec);CHECK(!l.changed&&!cas_count);
 trial_reset();current_thread=8;l=trial(b+0x488aeb);CHECK(!l.changed&&!cas_count);current_thread=7;
 trial_reset();marker=0;l=trial(b+0x488aeb);CHECK(!l.changed&&!cas_count);marker=1;
 trial_reset();test_action=6;l=trial(b+0x488aeb);CHECK(!l.changed&&!cas_count);test_action=5;
 trial_reset();opted=0;l=trial(b+0x488aeb);CHECK(!l.changed&&!cas_count);opted=1;
 trial_reset();frame.before.valid=0;l=trial(b+0x488aeb);CHECK(!l.changed&&!cas_count);
 trial_reset();put(channel,0xc4,(uintptr_t)shadow+8);l=trial(b+0x488aeb);CHECK(!l.changed&&!cas_count);
 trial_reset();put(map,0x108,0xabcdf);l=trial(b+0x488aeb);CHECK(!l.changed&&!cas_count);
 trial_reset();put32(archive,0x14,0);l=trial(b+0x488aeb);CHECK(!l.changed&&!cas_count);
 trial_reset();optimized_owner_rows=128;l=trial(b+0x488aeb);CHECK(l.changed&&l.restore_status==1&&original_owner==0&&flags()==0x4013);
 CHECK(!optimized_owner_consume_active);puts("PASS consume candidate authored fixtures: both sites, scope/default-off, exact-one-forward, CAS conflict, unrelated flags, restore/identity failure, nested and observer-budget independence");return 0;
}
