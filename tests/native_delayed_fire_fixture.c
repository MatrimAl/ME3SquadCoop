/* Actual private helper test. Engine readers are replaced, not ME3 validation. */
#include <windows.h>
#include <stdint.h>
#include <stddef.h>
#define CHECK(condition) do { if(!(condition)) { \
 const char message[]="actual C fixture CHECK failed\r\n";DWORD wrote; \
 WriteFile(GetStdHandle(STD_ERROR_HANDLE),message,sizeof(message)-1,&wrote,NULL);ExitProcess(2); } } while(0)
typedef void (*ProcessEvent)(void*,void*,void*,void*);
typedef void (*FireRpcExec)(void*,void*,void*);
typedef struct DelayedFireContext {
 int valid,same,active,cursor;unsigned mode,default_mode,pending,current;
} DelayedFireContext;
static DelayedFireContext delayed_fire_capture(int,void*,void*);
static void delayed_fire_after(DelayedFireContext*);
#define ME3_DELAYED_FIRE_FIXTURE 1
#include "delayed_fire_cancel_qa.h"
static DelayedFireContext configured;
static unsigned original_calls,dispatch_calls,sequence;static DWORD entry_error;
static uintptr_t original_args[3];static DelayedFireParms observed;
static void fixture_pe(void *weapon,void *fn,void *parms,void *result) {
 CHECK(sequence==1);CHECK((uintptr_t)weapon==0x1000);CHECK((uintptr_t)fn==0x2000);CHECK(!result);
 observed=*(DelayedFireParms*)parms;CHECK(observed.name[0]==43&&!observed.name[1]&&!observed.target);
 ++dispatch_calls;sequence=2;SetLastError(999);
}
static DelayedFireContext delayed_fire_capture(int k,void *self,void *frame) {
 CHECK(k==14&&(uintptr_t)self==0x1000&&(uintptr_t)frame==0x3000);SetLastError(701);return configured;
}
static void delayed_fire_after(DelayedFireContext *c) {
 const uint32_t name[2]={43,0};
 int eligible=delayed_fire_decision(c->valid,c->same,c->active,c->cursor,c->mode,c->default_mode,c->pending,c->current);
 CHECK(sequence==1);CHECK(GetLastError()==321);delayed_fire_dispatch(eligible,fixture_pe,0x1000,0x2000,name);
 /* This injected reader intentionally disturbs error; real stock wrapper must restore it. */
 SetLastError(702);
}
static void fixture_stock(void *self,void *frame,void *result) {
 CHECK(!sequence);entry_error=GetLastError();++original_calls;sequence=1;
 original_args[0]=(uintptr_t)self;original_args[1]=(uintptr_t)frame;original_args[2]=(uintptr_t)result;SetLastError(321);
}
static void run(unsigned expected) {
 original_calls=dispatch_calls=sequence=0;observed=(DelayedFireParms){{0,0},0};SetLastError(123);
 delayed_fire_exec_stock(14,(void*)0x1000,(void*)0x3000,(void*)0x4000,fixture_stock);
 CHECK(original_calls==1&&dispatch_calls==expected&&entry_error==123&&GetLastError()==321);
 CHECK(original_args[0]==0x1000&&original_args[1]==0x3000&&original_args[2]==0x4000);
}
void mainCRTStartup(void) {
 const char *command=GetCommandLineA();
 for(const char *p=command;*p;++p)if(p[0]=='-'&&p[1]=='-'&&p[2]=='c'&&p[3]=='a'&&p[4]=='n'&&p[5]=='a'&&p[6]=='r'&&p[7]=='y')CHECK(0);
 const DelayedFireContext valid={1,1,1,228,2,2,2,0};configured=valid;run(1);
 configured=valid;configured.valid=0;run(0); /* includes default-off capture */
 configured=valid;configured.same=0;run(0); /* stale generation/lease/channel */
 configured=valid;configured.active=0;run(0);
 configured=valid;configured.cursor=203;run(0); /* native client/null-world early return */
 configured=valid;configured.mode=4;run(0);
 configured=valid;configured.default_mode=1;run(0);
 configured=valid;configured.pending=0;run(0);
 configured=valid;configured.current=2;run(0);
 const uint32_t bad[2]={43,1};SetLastError(876);CHECK(!delayed_fire_dispatch(1,fixture_pe,0x1000,0x2000,bad));CHECK(GetLastError()==876);
 const char message[]="10 actual C helper checks passed; engine bindings/gameplay unverified\r\n";DWORD wrote;
 WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),message,sizeof(message)-1,&wrote,NULL);ExitProcess(0);
}
