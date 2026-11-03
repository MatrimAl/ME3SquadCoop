#ifndef ME3_REVIVE_PEER_PROTOCOL_H
#define ME3_REVIVE_PEER_PROTOCOL_H
#include <stddef.h>
#include <stdint.h>
#include <wchar.h>
/* Scalar syntax only. Callers bind channels, package GUID and both nonces to
 * the actual native connection/world lifetimes; parsing grants no authority. */
enum {REVIVE_PEER_MESSAGE_CAPACITY=192};
typedef struct RevivePeerMessage {
    wchar_t kind;
    uint64_t guest_nonce,host_nonce;
    unsigned shepard_channel,ashley_channel;
    uint32_t guid[4];
} RevivePeerMessage;
static int revive_peer_message_valid(const RevivePeerMessage *m) {
    if(!m||!m->guest_nonce||!m->shepard_channel||m->shepard_channel>2047||
       !m->ashley_channel||m->ashley_channel>2047||m->shepard_channel==m->ashley_channel||
       !(m->guid[0]|m->guid[1]|m->guid[2]|m->guid[3]))return 0;
    if(m->kind==L'Q')return !m->host_nonce;
    return (m->kind==L'O'||m->kind==L'A'||m->kind==L'C'||m->kind==L'D'||m->kind==L'H')&&m->host_nonce;
}
static int revive_peer_number(const wchar_t **cursor,const wchar_t *end,uint64_t limit,uint64_t *out) {
    const wchar_t *p=*cursor;uint64_t value=0;unsigned digits=0;
    if(p>=end||*p<L'0'||*p>L'9'||(*p==L'0'&&p+1<end&&p[1]>=L'0'&&p[1]<=L'9'))return 0;
    while(p<end&&*p>=L'0'&&*p<=L'9') {
        uint64_t digit=(uint64_t)(*p-L'0');
        if(++digits>20||value>limit/10||(value==limit/10&&digit>limit%10))return 0;
        value=value*10+digit;++p;
    }
    *cursor=p;*out=value;return 1;
}
/* count includes exactly one final NUL; embedded NULs and hidden suffixes are
 * rejected before any token is read. Never scan past this supplied bound. */
static int revive_peer_parse(const wchar_t *text,size_t count,RevivePeerMessage *out) {
    if(!text||!out||count<25||count>REVIVE_PEER_MESSAGE_CAPACITY||text[count-1])return 0;
    for(size_t i=0;i+1<count;++i)if(text[i]<1||text[i]>127)return 0;
    if(wcsncmp(text,L"ME3RV1 ",7)||text[8]!=L' ')return 0;
    RevivePeerMessage m={0};m.kind=text[7];
    const wchar_t *p=text+9,*end=text+count-1;uint64_t values[8]={0};
    for(unsigned i=0;i<8;++i) {
        uint64_t limit=i<2?UINT64_MAX:i<4?2047:UINT32_MAX;
        if(!revive_peer_number(&p,end,limit,&values[i]))return 0;
        if(i<7){if(p>=end||*p++!=L' ')return 0;}
    }
    if(p!=end)return 0;
    m.guest_nonce=values[0];m.host_nonce=values[1];
    m.shepard_channel=(unsigned)values[2];m.ashley_channel=(unsigned)values[3];
    for(unsigned i=0;i<4;++i)m.guid[i]=(uint32_t)values[i+4];
    if(!revive_peer_message_valid(&m))return 0;
    *out=m;return 1;
}
/* Returns the count including NUL, or zero. The caller supplies 192 wchar_t.
 * Manual decimal emission is locale-independent and uses no engine/OS calls. */
static int revive_peer_format(const RevivePeerMessage *m,wchar_t text[REVIVE_PEER_MESSAGE_CAPACITY]) {
    if(!text)return 0;text[0]=0;
    if(!revive_peer_message_valid(m))return 0;
    wmemcpy(text,L"ME3RV1 ",7);text[7]=m->kind;text[8]=L' ';size_t at=9;
    uint64_t values[8]={m->guest_nonce,m->host_nonce,m->shepard_channel,m->ashley_channel,
        m->guid[0],m->guid[1],m->guid[2],m->guid[3]};
    for(unsigned i=0;i<8;++i) {
        wchar_t digits[20];unsigned n=0;uint64_t value=values[i];
        do{digits[n++]=(wchar_t)(L'0'+value%10);value/=10;}while(value);
        if(at+(i?1:0)+n>=REVIVE_PEER_MESSAGE_CAPACITY){text[0]=0;return 0;}
        if(i)text[at++]=L' ';
        while(n)text[at++]=digits[--n];
    }
    text[at]=0;return (int)at+1;
}
static int revive_peer_message_same(const RevivePeerMessage *a,const RevivePeerMessage *b) {
    if(a->kind!=b->kind||a->guest_nonce!=b->guest_nonce||a->host_nonce!=b->host_nonce||
       a->shepard_channel!=b->shepard_channel||a->ashley_channel!=b->ashley_channel)return 0;
    for(unsigned i=0;i<4;++i)if(a->guid[i]!=b->guid[i])return 0;
    return 1;
}
static int revive_peer_protocol_selftest(void) {
    const wchar_t kinds[]={L'Q',L'O',L'A',L'C',L'D',L'H'};
    RevivePeerMessage m={L'Q',1,0,1,2,{1,0,0,0}},parsed={0};
    wchar_t text[REVIVE_PEER_MESSAGE_CAPACITY];int n=revive_peer_format(&m,text);
    if(n!=25||wcscmp(text,L"ME3RV1 Q 1 0 1 2 1 0 0 0")||
       !revive_peer_parse(text,(size_t)n,&parsed)||!revive_peer_message_same(&m,&parsed))return 0;
    m.guest_nonce=UINT64_MAX;m.shepard_channel=2047;m.ashley_channel=1;
    for(unsigned i=0;i<4;++i)m.guid[i]=UINT32_MAX;
    for(unsigned i=0;i<sizeof kinds/sizeof *kinds;++i) {
        m.kind=kinds[i];m.host_nonce=m.kind==L'Q'?0:UINT64_MAX;n=revive_peer_format(&m,text);
        if(!n||n>REVIVE_PEER_MESSAGE_CAPACITY||!revive_peer_parse(text,(size_t)n,&parsed)||
           !revive_peer_message_same(&m,&parsed)||revive_peer_parse(text,(size_t)n-1,&parsed))return 0;
    }
    const wchar_t *bad[]={L"ME3RV1 Q 0 0 1 2 1 0 0 0",L"ME3RV1 Q 1 1 1 2 1 0 0 0",L"ME3RV1 O 1 0 1 2 1 0 0 0",
        L"ME3RV1 A 0 1 1 2 1 0 0 0",L"ME3RV1 X 1 1 1 2 1 0 0 0",L"ME3RV1 q 1 0 1 2 1 0 0 0",
        L"ME3RV2 Q 1 0 1 2 1 0 0 0",L"ME3RV1 Q -1 0 1 2 1 0 0 0",L"ME3RV1 Q +1 0 1 2 1 0 0 0",
        L"ME3RV1 Q 01 0 1 2 1 0 0 0",L"ME3RV1 Q 1 00 1 2 1 0 0 0",L"ME3RV1 Q 1 0 01 2 1 0 0 0",
        L"ME3RV1 Q 18446744073709551616 0 1 2 1 0 0 0",L"ME3RV1 H 1 18446744073709551616 1 2 1 0 0 0",
        L"ME3RV1 Q 1 0 0 2 1 0 0 0",L"ME3RV1 Q 1 0 1 0 1 0 0 0",L"ME3RV1 Q 1 0 2048 2 1 0 0 0",
        L"ME3RV1 Q 1 0 1 2048 1 0 0 0",L"ME3RV1 Q 1 0 1 1 1 0 0 0",L"ME3RV1 Q 1 0 1 2 0 0 0 0",
        L"ME3RV1 Q 1 0 1 2 4294967296 0 0 0",L"ME3RV1 Q 1 0 1 2 1 0 0 00",L"ME3RV1 Q 1 0 1 2 1 0 0 -1",
        L"ME3RV1 Q 1 0 1 2 1 0 0",L"ME3RV1 Q 1 0 1 2 1 0 0 0 1",L"ME3RV1 Q 1 0 1 2 1 0 0 0 ",
        L"ME3RV1 Q 1 0 1 2 1 0 0 0x",L"ME3RV1 Q  1 0 1 2 1 0 0 0",L"ME3RV1 Q 1\t0 1 2 1 0 0 0",
        L"ME3RV1 Q 1 0 1 2 1 0 0 \u00e9"};
    for(unsigned i=0;i<sizeof bad/sizeof *bad;++i) {
        parsed=m;
        if(revive_peer_parse(bad[i],wcslen(bad[i])+1,&parsed)||!revive_peer_message_same(&parsed,&m))return 0;
    }
    const wchar_t hidden[]=L"ME3RV1 Q 1 0 1 2 1 0 0 0\0extra";
    wchar_t long_text[REVIVE_PEER_MESSAGE_CAPACITY+1];
    for(unsigned i=0;i<REVIVE_PEER_MESSAGE_CAPACITY;++i)long_text[i]=L'1';long_text[REVIVE_PEER_MESSAGE_CAPACITY]=0;
    if(revive_peer_parse(hidden,sizeof hidden/sizeof *hidden,&parsed)||
       revive_peer_parse(long_text,sizeof long_text/sizeof *long_text,&parsed)||
       revive_peer_parse(NULL,25,&parsed)||revive_peer_parse(text,(size_t)n,NULL))return 0;
    m.kind=L'Q';m.host_nonce=1;if(revive_peer_format(&m,text)||text[0])return 0;
    m.host_nonce=0;m.guid[0]=m.guid[1]=m.guid[2]=m.guid[3]=0;
    return !revive_peer_format(&m,text)&&!text[0];
}
#endif
