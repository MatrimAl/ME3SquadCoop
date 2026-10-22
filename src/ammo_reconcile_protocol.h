#ifndef ME3_AMMO_RECONCILE_PROTOCOL_H
#define ME3_AMMO_RECONCILE_PROTOCOL_H
#include <stdint.h>
#include <stddef.h>
#include <wchar.h>
#include <stdio.h>
/* Fixed, bounded control payload. Native RPC ownership authenticates its
 * source separately; this parser is not an authentication mechanism. */
typedef struct AmmoReconcileMessage { wchar_t kind;uint64_t request;unsigned channel;int used,spare; } AmmoReconcileMessage;
typedef struct AmmoReconcilePending {
    uint64_t request;unsigned long generation;unsigned channel;
    int used,spare,active;uint32_t sent_at;
} AmmoReconcilePending;
static int ammo_reconcile_number(const wchar_t **cursor,uint64_t limit,uint64_t *out) {
    const wchar_t *p=*cursor;uint64_t value=0;unsigned digits=0;
    if(*p<L'0'||*p>L'9'||(*p==L'0'&&p[1]>=L'0'&&p[1]<=L'9'))return 0;
    while(*p>=L'0'&&*p<=L'9'){
        uint64_t digit=(uint64_t)(*p-L'0');if(value>limit/10||(value==limit/10&&digit>limit%10)||++digits>20)return 0;
        value=value*10+digit;++p;
    }
    *cursor=p;*out=value;return 1;
}
static int ammo_reconcile_parse(const wchar_t *text,size_t count,AmmoReconcileMessage *m) {
    if(!text||!m||count<12||count>96||text[count-1])return 0;
    for(size_t i=0;i+1<count;++i)if(!text[i]||text[i]>127)return 0;
    if(wcsncmp(text,L"ME3SC1 ",7)||(text[7]!=L'Q'&&text[7]!=L'A')||text[8]!=L' ')return 0;
    const wchar_t *p=text+9;uint64_t id=0,channel=0,used=0,spare=0;
    if(!ammo_reconcile_number(&p,UINT64_MAX,&id)||!id||*p++!=L' '||!ammo_reconcile_number(&p,2047,&channel))return 0;
    if(text[7]==L'A'&&(*p++!=L' '||!ammo_reconcile_number(&p,1000000,&used)||*p++!=L' '||!ammo_reconcile_number(&p,1000000,&spare)))return 0;
    if(*p)return 0;
    m->kind=text[7];m->request=id;m->channel=(unsigned)channel;m->used=(int)used;m->spare=(int)spare;return 1;
}
static int ammo_reconcile_format(const AmmoReconcileMessage *m,wchar_t text[96]) {
    int n=m->kind==L'Q'?_snwprintf(text,96,L"ME3SC1 Q %llu %u",(unsigned long long)m->request,m->channel):
        _snwprintf(text,96,L"ME3SC1 A %llu %u %d %d",(unsigned long long)m->request,m->channel,m->used,m->spare);
    AmmoReconcileMessage check;
    return n>0&&n<96&&ammo_reconcile_parse(text,(size_t)n+1,&check)&&check.kind==m->kind?n+1:0;
}
static int ammo_reconcile_response_allowed(const AmmoReconcilePending *p,const AmmoReconcileMessage *m,
        unsigned long generation,unsigned channel,uint32_t now,int idle,int used,int spare) {
    return p->active&&m->kind==L'A'&&m->request==p->request&&m->channel==p->channel&&channel==p->channel&&
        generation==p->generation&&now-p->sent_at<=5000&&idle==1&&
        ((used==p->used&&spare==p->spare)||(used==m->used&&spare==m->spare));
}
static int ammo_reconcile_protocol_selftest(void) {
    AmmoReconcileMessage m={L'A',UINT64_C(4294967297),26,2,210},parsed;wchar_t text[96];
    int count=ammo_reconcile_format(&m,text);if(!count||!ammo_reconcile_parse(text,count,&parsed)||parsed.used!=2||parsed.spare!=210||parsed.request!=m.request)return 0;
    const wchar_t *bad[]={L"ME3SC1 Q 0 26",L"ME3SC1 Q 01 26",L"ME3SC1 Q 18446744073709551616 26",L"ME3SC1 Q 1 2048",L"ME3SC1 A 1 26 -1 210",L"ME3SC1 A 1 26 1 1000001",L"ME3SC1 A 1 26 1 210 extra",L"ME3SC1 Q 1 26 ",L"ME3SC1 X 1 26"};
    for(unsigned i=0;i<sizeof bad/sizeof *bad;++i)if(ammo_reconcile_parse(bad[i],wcslen(bad[i])+1,&parsed))return 0;
    AmmoReconcilePending p={m.request,1,26,3,210,1,100};
    if(!ammo_reconcile_response_allowed(&p,&m,1,26,200,1,3,210)||
        ammo_reconcile_response_allowed(&p,&m,2,26,200,1,3,210)||
        ammo_reconcile_response_allowed(&p,&m,1,27,200,1,3,210)||
        ammo_reconcile_response_allowed(&p,&m,1,26,5101,1,3,210)||
        ammo_reconcile_response_allowed(&p,&m,1,26,200,0,3,210)||
        ammo_reconcile_response_allowed(&p,&m,1,26,200,1,4,210))return 0;
    m.request++;return !ammo_reconcile_response_allowed(&p,&m,1,26,200,1,3,210);
}
#endif
