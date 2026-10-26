#ifndef ME3_RUNTIME_LOCAL_SQUAD_SET_H
#define ME3_RUNTIME_LOCAL_SQUAD_SET_H
#include "../third_party/minhook/include/MinHook.h"
/* Experimental local-only bridge for the existing squad's exact, validated
 * two-assignment script. The native callspace still runs; only its skip
 * result for this actor/function on the dispatching thread is adapted. */
typedef int (*LocalSquadCallspace)(void*,void*,void*,void*);
typedef struct LocalSquadCall {uintptr_t actor,function;int adapted;} LocalSquadCall;
static LocalSquadCallspace local_squad_original;
static DWORD local_squad_tls=TLS_OUT_OF_INDEXES;
static uintptr_t local_squad_base;
static int local_squad_installed,local_squad_attempted;
static unsigned char local_squad_hook_bytes[16];
static int local_squad_callspace(void *actor,void *function,void *parameters,void *frame) {
    int result=local_squad_original(actor,function,parameters,frame);
    LocalSquadCall *call=local_squad_tls!=TLS_OUT_OF_INDEXES?(LocalSquadCall*)TlsGetValue(local_squad_tls):NULL;
    if(call&&!call->adapted&&result==1&&!frame&&(uintptr_t)actor==call->actor&&
       (uintptr_t)function==call->function){call->adapted=1;return 0;}
    return result;
}
static int local_squad_hook_ready(uintptr_t base) {
    unsigned char got[16];
    if(local_squad_installed)return base==local_squad_base&&read_at(base+0x70a250,got,sizeof got)&&
        !memcmp(got,local_squad_hook_bytes,sizeof got);
    if(local_squad_attempted)return 0;local_squad_attempted=1;
    static const unsigned char entry[16]={0x4c,0x89,0x44,0x24,0x18,0x53,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x57,0x48,0x83};
    if(!read_at(base+0x70a250,got,sizeof got)||memcmp(got,entry,sizeof got))return 0;
    local_squad_tls=TlsAlloc();if(local_squad_tls==TLS_OUT_OF_INDEXES)return 0;
    MH_STATUS status=MH_Initialize();if(status!=MH_OK&&status!=MH_ERROR_ALREADY_INITIALIZED)return 0;
    LPVOID original=NULL;LPVOID target=(LPVOID)(base+0x70a250);
    if(MH_CreateHook(target,(LPVOID)local_squad_callspace,&original)!=MH_OK)return 0;
    local_squad_original=(LocalSquadCallspace)original;
    if(MH_EnableHook(target)!=MH_OK){MH_RemoveHook(target);local_squad_original=NULL;return 0;}
    local_squad_base=base;local_squad_installed=1;
    if(!read_at(base+0x70a250,local_squad_hook_bytes,sizeof local_squad_hook_bytes))return 0;
    return 1;
}
static int runtime_local_squad_set(uintptr_t base,uintptr_t actor,void *parms) {
    static int loaded,enabled;
    if(!loaded){
        wchar_t value[4]={0};loaded=1;
        DWORD length=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_LOCAL_SQUAD_SET",value,4);
        enabled=!length||(length==1&&value[0]==L'1');
    }
    GuestPlayerContextMethod *method=&guest_player_context_methods.set_pawn;
    unsigned char role=255,net=255;uintptr_t info=read_ptr(actor+0x198);
    if(!enabled||!guest_camera_actor_live(base,actor)||!guest_player_context_methods_valid(base,1)||
       !read_at(actor+0x26a,&role,1)||role!=1||!read_at(info+0x7c8,&net,1)||net!=3||
       read_ptr(read_ptr(actor)+0x248)!=base+0x70a250||!local_squad_hook_ready(base)||
       TlsGetValue(local_squad_tls))
        return guest_player_context_dispatch(base,actor,method,parms);
    LocalSquadCall call={actor,method->function.pointer,0};
    if(!TlsSetValue(local_squad_tls,&call))return guest_player_context_dispatch(base,actor,method,parms);
    int dispatched=guest_player_context_dispatch(base,actor,method,parms);
    TlsSetValue(local_squad_tls,NULL);
    char row[240];snprintf(row,sizeof row,"{\"guest_existing_context\":\"local-set-dispatch\",\"dispatched\":%s,\"skip_adapted\":%s,\"exact_actor_and_function\":true}\r\n",dispatched?"true":"false",call.adapted?"true":"false");line(row);
    return dispatched;
}
#endif
