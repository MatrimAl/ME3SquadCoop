#ifndef ME3_RUNTIME_AMMO_SETTLE_PROTOCOL_H
#define ME3_RUNTIME_AMMO_SETTLE_PROTOCOL_H
#include "ammo_reconcile_state.h"
/* Ordinary settled snapshots have their own wire namespace. Keep the tested
 * protected parser and scheduler behavior; this wrapper changes only its prefix.
 * Native owner-PC provenance and current weapon scope are separate admission
 * requirements supplied by the runtime, never established by parsing text. */
static int runtime_ammo_settle_parse(const wchar_t *text,size_t count,AmmoReconcileMessage *out) {
    wchar_t translated[96];AmmoReconcileMessage parsed={0};
    if(!text||!out||count<12||count>96||wcsncmp(text,L"ME3AM1 ",7))return 0;
    wmemcpy(translated,text,count);wmemcpy(translated,L"ME3SC1 ",7);
    if(!ammo_reconcile_parse(translated,count,&parsed))return 0;
    *out=parsed;return 1;
}
static int runtime_ammo_settle_format(const AmmoReconcileMessage *message,wchar_t text[96]) {
    wchar_t translated[96];AmmoReconcileMessage parsed={0};
    if(!message||!text||(message->kind!=L'Q'&&message->kind!=L'A')||
       (message->kind==L'Q'&&(message->used||message->spare)))return 0;
    int count=ammo_reconcile_format(message,translated);
    if(!count)return 0;
    wmemcpy(translated,L"ME3AM1 ",7);
    if(!runtime_ammo_settle_parse(translated,(size_t)count,&parsed)||
       parsed.kind!=message->kind||parsed.request!=message->request||parsed.channel!=message->channel||
       parsed.used!=message->used||parsed.spare!=message->spare)return 0;
    wmemcpy(text,translated,(size_t)count);return count;
}
static int runtime_ammo_settle_protocol_selftest(void) {
    AmmoReconcileMessage messages[]={
        {L'Q',1,0,0,0},
        {L'Q',UINT64_MAX,2047,0,0},
        {L'A',UINT64_C(4294967297),26,4,210},
        {L'A',UINT64_MAX,2047,1000000,1000000}};
    wchar_t text[96];AmmoReconcileMessage parsed={0};
    for(unsigned i=0;i<sizeof messages/sizeof *messages;++i){
        int count=runtime_ammo_settle_format(messages+i,text);
        if(!count||wcsncmp(text,L"ME3AM1 ",7)||
           !runtime_ammo_settle_parse(text,(size_t)count,&parsed)||
           parsed.kind!=messages[i].kind||parsed.request!=messages[i].request||
           parsed.channel!=messages[i].channel||parsed.used!=messages[i].used||parsed.spare!=messages[i].spare)return 0;
    }
    const wchar_t *bad[]={
        L"ME3SC1 Q 1 26",L"ME3RV1 Q 1 26",L"ME3DC1 Q 1 26",L"ME3PR2 Q 1 26",
        L"ME3AM1 Q 0 26",L"ME3AM1 Q 01 26",L"ME3AM1 Q 18446744073709551616 26",
        L"ME3AM1 Q 1 2048",L"ME3AM1 A 1 26 -1 210",L"ME3AM1 A 1 26 1 1000001",
        L"ME3AM1 A 1 26 1 210 extra",L"ME3AM1 Q 1 26 ",L"ME3AM1 X 1 26",
        L"ME3AM1 Q 1 26 0 0",L"ME3AM1 Q 1\t26"};
    for(unsigned i=0;i<sizeof bad/sizeof *bad;++i)
        if(runtime_ammo_settle_parse(bad[i],wcslen(bad[i])+1,&parsed))return 0;
    AmmoReconcileMessage q={L'Q',1,26,0,0};int count=runtime_ammo_settle_format(&q,text);
    if(!count||runtime_ammo_settle_parse(NULL,(size_t)count,&parsed)||
       runtime_ammo_settle_parse(text,(size_t)count,NULL)||
       runtime_ammo_settle_parse(text,11,&parsed)||runtime_ammo_settle_parse(text,97,&parsed))return 0;
    text[10]=0;if(runtime_ammo_settle_parse(text,(size_t)count,&parsed))return 0;
    count=runtime_ammo_settle_format(&q,text);text[9]=128;
    if(runtime_ammo_settle_parse(text,(size_t)count,&parsed))return 0;
    count=runtime_ammo_settle_format(&q,text);text[count-1]=L'x';
    if(runtime_ammo_settle_parse(text,(size_t)count,&parsed))return 0;
    q.used=1;if(runtime_ammo_settle_format(&q,text))return 0;
    q.used=0;q.kind=L'X';if(runtime_ammo_settle_format(&q,text)||
       runtime_ammo_settle_format(NULL,text)||runtime_ammo_settle_format(&q,NULL))return 0;
    return ammo_reconcile_protocol_selftest()&&ammo_reconcile_host_selftest()&&
        ammo_reconcile_guest_selftest()&&ammo_reconcile_state_selftest();
}
#endif
