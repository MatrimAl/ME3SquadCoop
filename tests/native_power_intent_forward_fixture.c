/* Actual production ingress/observer forwarding; native scope and metadata
 * capture are explicit stand-ins. No game or process attachment. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
typedef struct GuestCameraStamp {uintptr_t pointer;int index;char full_name[600];} GuestCameraStamp;
typedef struct GuestPlayerContextMethod {GuestCameraStamp function,owner;} GuestPlayerContextMethod;
typedef void (*RuntimeAmmoPickupExec)(void*,void*,void*);
#include "power_intent_forward_types.h"
static RuntimePowerStartObserver runtime_power_start_observer;
enum {ME3_ROLE_HOST=1,ME3_ROLE_GUEST=2};
static struct {int role;} g_runtime_config;
static DWORD session_game_thread;
static uintptr_t frame[8];static uint32_t output;
static RuntimePowerStartCapture captured;
static unsigned stock_calls,intent_calls,before_calls,after_calls,checks;
static int capture_ok,intent_result,failed;
static uintptr_t read_ptr(uintptr_t at){SetLastError(111);return *(uintptr_t*)at;}
static int runtime_power_start_observer_capture(void*self,void*f,RuntimePowerStartCapture*c){
    if(self!=(void*)16||f!=frame)failed=1;*c=captured;SetLastError(112);return capture_ok;
}
static int runtime_power_action_context_player_order(uintptr_t base,uintptr_t pc,uintptr_t pawn,uintptr_t power){
    if(base!=0x1000||pc!=0x2000||pawn!=0x3000||power!=0x4000)failed=1;
    ++intent_calls;SetLastError(113);return intent_result;
}
static int runtime_power_start_observer_before(void*self,void*f,RuntimePowerStartCapture*c){
    if(self!=(void*)16||f!=frame||GetLastError()!=37)failed=1;
    ++before_calls;c->valid=1;SetLastError(114);return 1;
}
static void runtime_power_start_observer_after(void*self,void*f,void*result,const RuntimePowerStartCapture*c){
    if(self!=(void*)16||f!=frame||result!=&output||!c->valid||GetLastError()!=53)failed=1;
    ++after_calls;SetLastError(115);
}
static void stock(void*self,void*f,void*result){
    if(self!=(void*)16||f!=frame||result!=&output||GetLastError()!=37)failed=1;
    ++stock_calls;*(uint32_t*)result=0x12345678;SetLastError(53);
}
#include "power_intent_forward_helpers.h"
static void setup(void){
    memset(&runtime_power_start_observer,0,sizeof runtime_power_start_observer);
    runtime_power_start_observer.base=0x1000;runtime_power_start_observer.prepared=1;
    runtime_power_start_observer.methods[1].function.pointer=0x5000;
    memset(&captured,0,sizeof captured);captured.kind=1;captured.valid=captured.frame_valid=captured.source_matches=1;
    captured.table_valid=captured.table_owner_matches=captured.table_pc_matches=1;
    captured.pc.pointer=0x2000;captured.pawn.pointer=0x3000;captured.power.pointer=0x4000;
    memset(frame,0,sizeof frame);frame[3]=0x5000;output=0;
    g_runtime_config.role=ME3_ROLE_HOST;session_game_thread=GetCurrentThreadId();capture_ok=intent_result=1;
    stock_calls=intent_calls=before_calls=after_calls=0;
}
static void run(const char*name,unsigned intents,unsigned observations){
    unsigned prior_depth=runtime_power_start_observer.depth;SetLastError(37);
    runtime_power_start_observer_forward((void*)16,frame,&output,stock);++checks;
    if(stock_calls!=1||intent_calls!=intents||before_calls!=observations||after_calls!=observations||
       runtime_power_start_observer.depth!=prior_depth||output!=0x12345678||GetLastError()!=53||failed){
        fprintf(stderr,"FAIL %s stock%u intent%u before%u after%u error%lu\n",name,stock_calls,intent_calls,before_calls,after_calls,GetLastError());exit(1);
    }
}
int main(void){
    setup();run("authenticated ingress",1,1);
    setup();runtime_power_start_observer.calls=64;run("intent independent of diagnostic quota",1,0);
    setup();runtime_power_start_observer.depth=4;run("intent independent of diagnostic depth",1,0);
    setup();intent_result=0;run("intent helper refusal still forwards stock",1,1);
    setup();capture_ok=0;run("unreadable capture",0,1);
    setup();g_runtime_config.role=ME3_ROLE_GUEST;run("guest never receives host adaptation",0,1);
    setup();frame[3]=0x6000;run("wrong script node",0,1);
    setup();runtime_power_start_observer.prepared=0;run("unprepared boundary",0,0);
    setup();session_game_thread=GetCurrentThreadId()+1;captured.valid=0;run("wrong thread capture refuses",0,0);
    for(unsigned bad=0;bad<8;++bad){setup();
        switch(bad){case 0:captured.kind=0;break;case 1:captured.valid=0;break;case 2:captured.frame_valid=0;break;
          case 3:captured.source_matches=0;break;case 4:captured.table_valid=0;break;
          case 5:captured.table_owner_matches=0;break;case 6:captured.table_pc_matches=0;break;case 7:captured.power.pointer=0;break;}
        run("scope/ownership refusal",0,1);
    }
    printf("PASS %u production host intent/observer forwarding cases; stock once, output/error preserved; no game\n",checks);return 0;
}
