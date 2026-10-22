#ifndef ME3_AMMO_RECONCILE_H
#define ME3_AMMO_RECONCILE_H
#include "ammo_reconcile_protocol.h"
/* Research-only control messages over existing owner-PC native RPCs. Both
 * participants must opt in. Reserved messages are consumed before chat/UI.
 * Ordinary messages and outbound dispatch retain the original script entry.
 * Never author host ammunition; apply only an idle, fresh matching reply.
 */
typedef void (*AmmoRpcExec)(void*,void*,void*);
static AmmoRpcExec ammo_rpc_original;
static GuestPlayerContextMethod ammo_rpc_methods[2];
static uintptr_t ammo_rpc_base;
static int ammo_rpc_enabled=-1,ammo_rpc_attempted,ammo_rpc_installed;
static unsigned ammo_rpc_events,ammo_rpc_sequences;
typedef struct AmmoRpcState {
    unsigned long generation;int have,used,spare,dirty;DWORD quiet,wait_log,retry;
    unsigned retries;AmmoReconcilePending pending;
    uint64_t host_id,last_host_id;int host_pending,host_used,host_spare;DWORD host_quiet,host_received;
} AmmoRpcState;
static AmmoRpcState ammo_rpc_state;
static void ammo_rpc_exec(void*,void*,void*);
static int ammo_rpc_opted(void) {
    if(ammo_rpc_enabled<0){wchar_t value[8]={0};ammo_rpc_enabled=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_AMMO_RECONCILE",value,8)==1&&value[0]==L'1';}
    return ammo_rpc_enabled&&progress_enabled&&(test_action==5||test_action==6);
}
static void ammo_rpc_log(const char *event,const char *reason,uint64_t id,int used,int spare) {
    if(ammo_rpc_events>=128)return;++ammo_rpc_events;char row[1000],safe[200];identity_quote(reason,safe,sizeof safe);
    snprintf(row,sizeof row,"{\"ammo_reconcile\":\"%s\",\"reason\":\"%s\",\"side\":\"%s\",\"generation\":%lu,\"request\":%llu,\"channel_index\":%d,\"used\":%d,\"spare\":%d,\"monotonic_ms\":%lu}\r\n",event,safe,test_action==6?"host":"guest",ammo_wire_scope.generation,(unsigned long long)id,ammo_wire_scope.channel.index,used,spare,GetTickCount());line(row);
}
static int ammo_rpc_method_valid(int k) {
    uintptr_t fn=ammo_rpc_methods[k].function.pointer;uint32_t flags=0;uint16_t size=0,ret=0,index=1;unsigned char params=0;
    return guest_player_context_method_matches(ammo_rpc_base,&ammo_rpc_methods[k],1)&&
        read_ptr(fn+0xd8)==(uintptr_t)ammo_rpc_exec&&read_at(fn+0xe0,&flags,4)&&flags==(k?0x10201c2:0x220042)&&
        read_at(fn+0xe6,&size,2)&&size==16&&read_at(fn+0xe8,&ret,2)&&ret==65535&&
        read_at(fn+0xe4,&index,2)&&!index&&read_at(fn+0xea,&params,1)&&params==1;
}
static int ammo_rpc_source(uintptr_t self,uintptr_t frame,int k) {
    AmmoWireReceive *r=ammo_wire_receive;NetworkActorScope world;NetworkActorIdentityEntry channel={0};
    if(!r||!r->channel||GetCurrentThreadId()!=ammo_wire_thread||
       (test_action==6?k!=0:k!=1)||!is_kind_of(ammo_rpc_base,self,"PlayerController")||
       read_ptr(frame+0x18)!=ammo_rpc_methods[k].function.pointer||read_ptr(frame+0x20)!=self||
       !network_actor_read_scope(ammo_rpc_base,self,&world)||
       !network_actor_channel_valid(ammo_rpc_base,self,r->channel,world.driver,&channel)||
       read_ptr(channel.connection+0x68)!=self)return 0;
    if(test_action==5&&local_controller(read_ptr(ammo_rpc_base+0x18b31d0))!=self)return 0;
    return network_actor_channel_id_internal(ammo_rpc_base,self)==channel.index;
}
static int ammo_rpc_idle(int *used,int *spare,char state[96]) {
    AmmoWireScope *s=&ammo_wire_scope;unsigned char fire=255;
    return ammo_wire_scope_valid()&&read_at(s->weapon.pointer+0x3b8,used,4)&&*used>=0&&*used<=1000000&&
        read_at(s->weapon.pointer+0x3b4,spare,4)&&*spare>=0&&*spare<=1000000&&
        read_at(s->pc.pointer+0x480,&fire,1)&&!fire&&
        native_state_name(s->base,s->weapon.pointer,state,96)&&!strcmp(state,"Active");
}
static int ammo_rpc_send(int k,const AmmoReconcileMessage *message) {
    AmmoWireScope *s=&ammo_wire_scope;wchar_t text[96];int count=ammo_reconcile_format(message,text);
    if(!count||!ammo_rpc_method_valid(k)||!ammo_wire_scope_valid()||
       read_ptr(read_ptr(s->pc.pointer)+0x230)!=s->base+0x40d450||!guest_equipment_dispatch_pins(s->base))return 0;
    GameArray parameter={(uintptr_t)text,count,count};
    ((ProcessEvent)(s->base+0x40d450))((void*)s->pc.pointer,(void*)ammo_rpc_methods[k].function.pointer,&parameter,NULL);return 1;
}
static int ammo_rpc_receive(uintptr_t self,uintptr_t frame) {
    int k=test_action==6?0:1;GameArray string={0};wchar_t text[96]={0};
    if(!ammo_rpc_installed||!ammo_rpc_method_valid(k))return 0;
    if(read_ptr(frame+0x18)!=ammo_rpc_methods[k].function.pointer)return 0;
    uintptr_t locals=read_ptr(frame+0x30);
    if(!read_at(locals,&string,sizeof string)||!string.data||string.count<8||string.count>96||
       string.max<string.count||string.max>256||!read_at(string.data,text,(SIZE_T)string.count*sizeof(wchar_t))||
       wcsncmp(text,L"ME3SC1 ",7))return 0;
    if(!ammo_rpc_source(self,frame,k)){ammo_rpc_log("rejected","native-rpc-source-context",0,0,0);return 0;}
    AmmoReconcileMessage m={0};AmmoWireScope *s=&ammo_wire_scope;AmmoRpcState *t=&ammo_rpc_state;
    if(!ammo_reconcile_parse(text,string.count,&m)||!ammo_wire_scope_valid()||self!=s->pc.pointer||
       m.channel!=(unsigned)s->channel.index){ammo_rpc_log("rejected","schema-or-current-scope",0,0,0);return 1;}
    DWORD now=GetTickCount();
    if(test_action==6){
        if(m.kind!=L'Q'||m.request<=t->last_host_id||(t->host_pending&&m.request<t->host_id)){
            ammo_rpc_log("rejected","request-order",m.request,0,0);return 1;
        }
        if(!t->host_pending||m.request!=t->host_id){t->host_id=m.request;t->host_pending=1;t->host_quiet=0;t->host_received=now;ammo_rpc_log("received-request","native-owner-pc-rpc",m.request,0,0);}
        return 1;
    }
    int used=0,spare=0;char state[96]={0};int idle=ammo_rpc_idle(&used,&spare,state);
    if(!ammo_reconcile_response_allowed(&t->pending,&m,s->generation,(unsigned)s->channel.index,now,idle,used,spare)){
        if(t->pending.active&&m.request==t->pending.request){t->pending.active=0;t->dirty=1;t->quiet=0;}
        ammo_rpc_log("rejected","stale-or-not-idle-response",m.request,used,spare);return 1;
    }
    int values[2]={m.spare,m.used},after[2]={0};SIZE_T written=0;
    int changed=used!=m.used||spare!=m.spare;
    int applied=!changed||(WriteProcessMemory(GetCurrentProcess(),(void*)(s->weapon.pointer+0x3b4),values,sizeof values,&written)&&written==sizeof values);
    applied=applied&&read_at(s->weapon.pointer+0x3b4,after,sizeof after)&&after[0]==m.spare&&after[1]==m.used;
    t->pending.active=0;
    if(applied){t->used=m.used;t->spare=m.spare;t->dirty=0;t->quiet=0;}
    ammo_rpc_log(applied?(changed?"applied":"already-matched"):"apply-unverified","native-owner-pc-response",m.request,m.used,m.spare);
    return 1;
}
static void ammo_rpc_exec(void *self,void *frame,void *result) {
    DWORD error=GetLastError();int consumed=ammo_rpc_receive((uintptr_t)self,(uintptr_t)frame);SetLastError(error);
    if(!consumed)ammo_rpc_original(self,frame,result);
}
static void ammo_rpc_install(void) {
    if(ammo_rpc_attempted||!ammo_wire_scope_valid())return;
    ammo_rpc_attempted=1;AmmoWireScope *s=&ammo_wire_scope;ammo_rpc_base=s->base;
    const char *names[]={"ServerSay","ClientAdminMessage"};
    for(int k=0;k<2;++k){
        uintptr_t fn=guest_player_context_find_method(s->base,s->pc.klass,"PlayerController",names[k]);
        if(!guest_player_context_method_stamp(s->base,fn,&ammo_rpc_methods[k])||
           guest_equipment_find_first(s->base,s->pc.klass,names[k])!=fn||
           !guest_player_context_function_abi(s->base,fn,"PlayerController",names[k],k?0x10201c2:0x220042,16,65535,0,0xf52b0,1))goto rejected;
        uintptr_t child=read_ptr(fn+0x70);int found=0;
        for(int i=0;child&&i<32;++i){
            if(!is_live(s->base,child))goto rejected;
            char name[96];object_name(s->base,child,name,sizeof name);
            if(!strcmp(name,"Msg")){
                char path[180];snprintf(path,sizeof path,"Engine.PlayerController.%s.Msg",names[k]);
                if(found++||read_ptr(child+0x40)!=fn||!combat_fire_property(s->base,child,"StrProperty",path,0,0x400080))goto rejected;
            }
            child=read_ptr(child+0x68);
        }
        if(child||found!=1)goto rejected;
    }
    if(!guest_equipment_dispatch_pins(s->base))goto rejected;
    ammo_rpc_original=(AmmoRpcExec)(s->base+0xf52b0);
    for(int k=0;k<2;++k){
        uintptr_t slot=ammo_rpc_methods[k].function.pointer+0xd8;
        if(InterlockedCompareExchangePointer((PVOID volatile*)slot,(PVOID)ammo_rpc_exec,(PVOID)ammo_rpc_original)!=(PVOID)ammo_rpc_original){
            if(k)InterlockedCompareExchangePointer((PVOID volatile*)(ammo_rpc_methods[0].function.pointer+0xd8),(PVOID)ammo_rpc_original,(PVOID)ammo_rpc_exec);
            goto rejected;
        }
    }
    ammo_rpc_installed=1;ammo_rpc_log("installed","native-owner-pc-rpc",0,0,0);return;
rejected:ammo_rpc_log("not-installed","exact-method-or-native-pins",0,0,0);
}
static void ammo_reconcile_tick(void) {
    if(!ammo_rpc_opted())return;
    if(!ammo_wire_scope_valid()){memset(&ammo_rpc_state,0,sizeof ammo_rpc_state);return;}
    ammo_rpc_install();if(!ammo_rpc_installed)return;
    AmmoWireScope *s=&ammo_wire_scope;AmmoRpcState *t=&ammo_rpc_state;DWORD now=GetTickCount();
    int used=0,spare=0;char state[96]={0};int idle=ammo_rpc_idle(&used,&spare,state);
    if(t->generation!=s->generation){memset(t,0,sizeof *t);t->generation=s->generation;t->have=1;t->used=used;t->spare=spare;}
    if(test_action==6){
        if(!t->host_pending)return;
        if(now-t->host_received>5000){t->host_pending=0;return;}
        if(!idle){t->host_quiet=0;return;}
        if(!t->host_quiet||t->host_used!=used||t->host_spare!=spare){t->host_used=used;t->host_spare=spare;t->host_quiet=now;return;}
        if(now-t->host_quiet<250)return;
        AmmoReconcileMessage reply={L'A',t->host_id,(unsigned)s->channel.index,used,spare};
        int sent=ammo_rpc_send(1,&reply);ammo_rpc_log(sent?"replied":"send-unverified","settled-host-snapshot",reply.request,used,spare);
        if(sent){t->last_host_id=t->host_id;t->host_pending=0;}return;
    }
    if(t->used!=used||t->spare!=spare){t->used=used;t->spare=spare;t->dirty=1;t->quiet=0;}
    if(!idle){t->quiet=0;t->pending.active=0;return;}
    if(t->pending.active){
        if(now-t->pending.sent_at>5000){t->pending.active=0;t->quiet=0;return;}
        if(now-t->retry>=1000&&t->retries<3){AmmoReconcileMessage retry={L'Q',t->pending.request,t->pending.channel,0,0};ammo_rpc_send(0,&retry);t->retry=now;++t->retries;}
        return;
    }
    if(!t->dirty)return;
    if(!t->quiet){t->quiet=now;return;}
    if(now-t->quiet<250)return;
    if(ammo_rpc_sequences>=64)return;
    uint64_t id=((uint64_t)s->generation<<32)|++ammo_rpc_sequences;
    AmmoReconcileMessage request={L'Q',id,(unsigned)s->channel.index,0,0};
    t->pending=(AmmoReconcilePending){id,s->generation,(unsigned)s->channel.index,used,spare,1,now};t->retry=now;t->retries=1;
    int sent=ammo_rpc_send(0,&request);ammo_rpc_log(sent?"requested":"send-unverified","settled-guest-weapon",id,used,spare);
    if(!sent)t->pending.active=0;
}
static int ammo_rpc_fixture_calls;static void ammo_rpc_fixture_forward(void *a,void *b,void *c){(void)a;(void)b;(void)c;++ammo_rpc_fixture_calls;SetLastError(123);}
static int ammo_rpc_selftest(void) {
    AmmoRpcExec saved=ammo_rpc_original;int installed=ammo_rpc_installed;DWORD error=GetLastError();
    ammo_rpc_installed=0;ammo_rpc_original=ammo_rpc_fixture_forward;ammo_rpc_fixture_calls=0;
    ammo_rpc_exec(NULL,NULL,NULL);int ok=ammo_rpc_fixture_calls==1&&GetLastError()==123;
    ammo_rpc_original=saved;ammo_rpc_installed=installed;SetLastError(error);return ok&&ammo_reconcile_protocol_selftest();
}
#endif
