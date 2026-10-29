#ifndef ME3_SCENE_SPECTATOR_PROTOCOL_H
#define ME3_SCENE_SPECTATOR_PROTOCOL_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>
#include <math.h>
/* Scalar camera presentation only. Parsing grants no scene or connection
 * authority. Callers bind the nonce to the verified native peer/world lifetime
 * and reset sequence state only when that scope is explicitly replaced. */
enum {SCENE_SPECTATOR_MESSAGE_CAPACITY=192};
typedef struct SceneSpectatorMessage {
    uint64_t nonce,sequence;
    int active;
    float position[3];
    int32_t rotation[3];
    float fov;
} SceneSpectatorMessage;
_Static_assert(sizeof(float)==4&&sizeof(int32_t)==4,"spectator scalar widths");
static void scene_spectator_words(const SceneSpectatorMessage *m,uint32_t words[7]) {
    for(unsigned i=0;i<3;++i){memcpy(words+i,m->position+i,4);memcpy(words+i+3,m->rotation+i,4);}
    memcpy(words+6,&m->fov,4);
}
static int scene_spectator_message_valid(const SceneSpectatorMessage *m) {
    if(!m||!m->nonce||!m->sequence||(m->active!=0&&m->active!=1))return 0;
    if(!m->active){uint32_t words[7];scene_spectator_words(m,words);
        for(unsigned i=0;i<7;++i)if(words[i])return 0;return 1;}
    for(unsigned i=0;i<3;++i)if(!isfinite(m->position[i])||fabsf(m->position[i])>100000000.0f)return 0;
    return isfinite(m->fov)&&m->fov>=1.0f&&m->fov<=179.0f;
}
static int scene_spectator_number(const wchar_t **cursor,const wchar_t *end,uint64_t *out) {
    const wchar_t *p=*cursor;uint64_t value=0;unsigned digits=0;
    if(p>=end||*p<L'0'||*p>L'9'||(*p==L'0'&&p+1<end&&p[1]>=L'0'&&p[1]<=L'9'))return 0;
    while(p<end&&*p>=L'0'&&*p<=L'9'){
        uint64_t digit=(uint64_t)(*p-L'0');
        if(++digits>20||value>UINT64_MAX/10||(value==UINT64_MAX/10&&digit>UINT64_MAX%10))return 0;
        value=value*10+digit;++p;}
    if(!value)return 0;*cursor=p;*out=value;return 1;
}
/* count includes exactly one final NUL. All reads are bounded by that count;
 * embedded NULs, non-ASCII data and any suffix are rejected before publication. */
static int scene_spectator_parse(const wchar_t *text,size_t count,SceneSpectatorMessage *out) {
    if(!text||!out||count<76||count>SCENE_SPECTATOR_MESSAGE_CAPACITY||text[count-1])return 0;
    for(size_t i=0;i+1<count;++i)if(text[i]<1||text[i]>127)return 0;
    if(wmemcmp(text,L"ME3SC1 ",7))return 0;
    SceneSpectatorMessage m={0};const wchar_t *p=text+7,*end=text+count-1;
    if(!scene_spectator_number(&p,end,&m.nonce)||p>=end||*p++!=L' '||
       !scene_spectator_number(&p,end,&m.sequence)||p>=end||*p++!=L' '||
       p>=end||(*p!=L'0'&&*p!=L'1'))return 0;
    m.active=(int)(*p++-L'0');uint32_t words[7]={0};
    for(unsigned i=0;i<7;++i){
        if(p>=end||*p++!=L' '||end-p<8)return 0;
        uint32_t word=0;
        for(unsigned j=0;j<8;++j){wchar_t c=*p++;unsigned digit;
            if(c>=L'0'&&c<=L'9')digit=(unsigned)(c-L'0');
            else if(c>=L'a'&&c<=L'f')digit=(unsigned)(c-L'a'+10);else return 0;
            word=(word<<4)|digit;}
        words[i]=word;
    }
    if(p!=end)return 0;
    for(unsigned i=0;i<3;++i){memcpy(m.position+i,words+i,4);memcpy(m.rotation+i,words+i+3,4);}
    memcpy(&m.fov,words+6,4);
    if(!scene_spectator_message_valid(&m))return 0;*out=m;return 1;
}
/* Locale-independent fixed-width lowercase hex preserves native float/rotator
 * bits. Inactive callers must supply a fully cleared view, not latent data. */
static int scene_spectator_format(const SceneSpectatorMessage *m,wchar_t text[SCENE_SPECTATOR_MESSAGE_CAPACITY]) {
    if(!text)return 0;text[0]=0;if(!scene_spectator_message_valid(m))return 0;
    wmemcpy(text,L"ME3SC1 ",7);size_t at=7;uint64_t values[2]={m->nonce,m->sequence};
    for(unsigned i=0;i<2;++i){wchar_t digits[20];unsigned n=0;uint64_t value=values[i];
        do{digits[n++]=(wchar_t)(L'0'+value%10);value/=10;}while(value);
        if(at+n+1>=SCENE_SPECTATOR_MESSAGE_CAPACITY){text[0]=0;return 0;}
        while(n)text[at++]=digits[--n];text[at++]=L' ';}
    text[at++]=(wchar_t)(L'0'+m->active);uint32_t words[7];scene_spectator_words(m,words);
    const wchar_t *hex=L"0123456789abcdef";
    for(unsigned i=0;i<7;++i){
        if(at+9>=SCENE_SPECTATOR_MESSAGE_CAPACITY){text[0]=0;return 0;}
        text[at++]=L' ';for(unsigned j=0;j<8;++j)text[at++]=hex[(words[i]>>(28-j*4))&15];}
    text[at]=0;return (int)at+1;
}
/* Never adopt a new nonce or accept sequence wrap here. A fresh native scope
 * may start with last_sequence0 after its owner separately binds the nonce. */
static int scene_spectator_message_newer(const SceneSpectatorMessage *m,uint64_t expected_nonce,uint64_t last_sequence) {
    return scene_spectator_message_valid(m)&&expected_nonce&&m->nonce==expected_nonce&&m->sequence>last_sequence;
}
static int scene_spectator_message_same(const SceneSpectatorMessage *a,const SceneSpectatorMessage *b) {
    uint32_t x[7],y[7];scene_spectator_words(a,x);scene_spectator_words(b,y);
    return a->nonce==b->nonce&&a->sequence==b->sequence&&a->active==b->active&&!memcmp(x,y,sizeof x);
}
static int scene_spectator_protocol_selftest(void) {
    SceneSpectatorMessage m={UINT64_C(4294967297),UINT64_MAX,1,{1.0f,-2.0f,100000000.0f},{INT32_MIN,0,INT32_MAX},90.0f},parsed={0};
    wchar_t text[SCENE_SPECTATOR_MESSAGE_CAPACITY];int n=scene_spectator_format(&m,text);
    if(!n||n>SCENE_SPECTATOR_MESSAGE_CAPACITY||!scene_spectator_parse(text,(size_t)n,&parsed)||
       !scene_spectator_message_same(&m,&parsed)||scene_spectator_parse(text,(size_t)n-1,&parsed)||
       !scene_spectator_message_newer(&m,m.nonce,0)||!scene_spectator_message_newer(&m,m.nonce,UINT64_MAX-1)||
       scene_spectator_message_newer(&m,m.nonce,UINT64_MAX)||scene_spectator_message_newer(&m,m.nonce+1,0)||
       scene_spectator_message_newer(&m,0,0))return 0;
    const wchar_t *bad[]={
        L"ME3SC1 0 1 0 00000000 00000000 00000000 00000000 00000000 00000000 00000000",
        L"ME3SC1 1 0 0 00000000 00000000 00000000 00000000 00000000 00000000 00000000",
        L"ME3SC1 01 1 0 00000000 00000000 00000000 00000000 00000000 00000000 00000000",
        L"ME3SC1 1 01 0 00000000 00000000 00000000 00000000 00000000 00000000 00000000",
        L"ME3SC1 18446744073709551616 1 0 00000000 00000000 00000000 00000000 00000000 00000000 00000000",
        L"ME3SC1 1 18446744073709551616 0 00000000 00000000 00000000 00000000 00000000 00000000 00000000",
        L"ME3SC1 1 1 2 00000000 00000000 00000000 00000000 00000000 00000000 00000000",
        L"ME3SC1 1 1 00 00000000 00000000 00000000 00000000 00000000 00000000 00000000",
        L"ME3SC1 1 1 0 00000000 00000000 00000000 00000000 00000000 00000000 80000000",
        L"ME3SC1 1 1 1 7fc00000 00000000 00000000 00000000 00000000 00000000 42b40000",
        L"ME3SC1 1 1 1 7f800000 00000000 00000000 00000000 00000000 00000000 42b40000",
        L"ME3SC1 1 1 1 4d000000 00000000 00000000 00000000 00000000 00000000 42b40000",
        L"ME3SC1 1 1 1 00000000 00000000 00000000 00000000 00000000 00000000 3f000000",
        L"ME3SC1 1 1 1 00000000 00000000 00000000 00000000 00000000 00000000 43340000",
        L"ME3SC1 1 1 1 00000000 00000000 00000000 00000000 00000000 00000000 7f800000",
        L"ME3SC1 1 1 1 00000000 00000000 00000000 00000000 00000000 00000000 42B40000",
        L"ME3SC1 1 1 0 0000000 00000000 00000000 00000000 00000000 00000000 00000000",
        L"ME3SC1 1 1 0 000000000 00000000 00000000 00000000 00000000 00000000 00000000",
        L"ME3SC1 1 1 0 00000000 00000000 00000000 00000000 00000000 00000000 00000000 extra",
        L"ME3SC1 1 1 0 00000000 00000000 00000000 00000000 00000000 00000000 00000000 ",
        L"ME3SC1 1\t1 0 00000000 00000000 00000000 00000000 00000000 00000000 00000000",
        L"ME3SC2 1 1 0 00000000 00000000 00000000 00000000 00000000 00000000 00000000"};
    for(unsigned i=0;i<sizeof bad/sizeof *bad;++i){parsed=m;
        if(scene_spectator_parse(bad[i],wcslen(bad[i])+1,&parsed)||!scene_spectator_message_same(&m,&parsed))return 0;}
    const wchar_t hidden[]=L"ME3SC1 1 1 0 00000000 00000000 00000000 00000000 00000000 00000000 00000000\0extra";
    wchar_t oversized[SCENE_SPECTATOR_MESSAGE_CAPACITY+1];
    for(unsigned i=0;i<SCENE_SPECTATOR_MESSAGE_CAPACITY;++i)oversized[i]=L'1';oversized[SCENE_SPECTATOR_MESSAGE_CAPACITY]=0;
    if(scene_spectator_parse(hidden,sizeof hidden/sizeof *hidden,&parsed)||
       scene_spectator_parse(oversized,sizeof oversized/sizeof *oversized,&parsed)||scene_spectator_parse(NULL,76,&parsed)||
       scene_spectator_parse(text,(size_t)n,NULL))return 0;
    memset(&m,0,sizeof m);m.nonce=m.sequence=1;n=scene_spectator_format(&m,text);
    if(n!=76||wcscmp(text,L"ME3SC1 1 1 0 00000000 00000000 00000000 00000000 00000000 00000000 00000000")||
       !scene_spectator_parse(text,(size_t)n,&parsed)||!scene_spectator_message_same(&m,&parsed))return 0;
    m.position[0]=-0.0f;if(scene_spectator_format(&m,text)||text[0])return 0;
    memset(m.position,0,sizeof m.position);m.active=1;m.fov=1.0f;
    if(!scene_spectator_format(&m,text))return 0;m.fov=179.0f;if(!scene_spectator_format(&m,text))return 0;
    m.position[0]=NAN;if(scene_spectator_format(&m,text)||text[0])return 0;
    m.position[0]=0;m.nonce=0;return !scene_spectator_format(&m,text)&&!text[0];
}
#endif
