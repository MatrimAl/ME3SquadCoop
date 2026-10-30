#ifndef ME3_SCENE_VOICE_PROTOCOL_H
#define ME3_SCENE_VOICE_PROTOCOL_H
#include <stddef.h>
#include <stdint.h>
#include <wchar.h>

/* Scalar presentation data only. The caller must authenticate the owner-PC
 * source, scene nonce, current native Matinee/speaker channels and lifecycle.
 * Parsing this message grants no story, actor or conversation authority. */
enum {SCENE_VOICE_MESSAGE_CAPACITY=192,
      SCENE_VOICE_FEMALE=1,SCENE_VOICE_HIDE_SUBTITLES=2,SCENE_VOICE_ENTRY=4};
typedef struct SceneVoiceMessage {
    char kind;
    uint64_t scene_nonce,generation;
    uint32_t matinee_channel,speaker_channel,resource_id,strref,flags;
} SceneVoiceMessage;
static int scene_voice_message_valid(const SceneVoiceMessage *m) {
    if(!m||!m->scene_nonce||!m->generation)return 0;
    if(m->kind=='H')return !m->matinee_channel&&!m->speaker_channel&&!m->resource_id&&!m->strref&&m->flags<=1;
    if(m->kind=='S')return !m->matinee_channel&&!m->speaker_channel&&
        !m->resource_id&&!m->strref&&!m->flags;
    return m->kind=='P'&&m->matinee_channel>=1&&m->matinee_channel<=2047&&
        m->speaker_channel>=1&&m->speaker_channel<=2047&&m->resource_id>=1&&
        m->resource_id<=INT32_MAX&&m->strref>=1&&m->strref<=INT32_MAX&&m->flags<=7;
}
static int scene_voice_number(const wchar_t **cursor,const wchar_t *end,uint64_t limit,uint64_t *out) {
    const wchar_t *p=*cursor;uint64_t value=0;unsigned digits=0;
    if(p>=end||*p<L'0'||*p>L'9'||(*p==L'0'&&p+1<end&&p[1]>=L'0'&&p[1]<=L'9'))return 0;
    while(p<end&&*p>=L'0'&&*p<=L'9') {
        uint64_t digit=(uint64_t)(*p-L'0');
        if(++digits>20||value>limit/10||(value==limit/10&&digit>limit%10))return 0;
        value=value*10+digit;++p;
    }
    *cursor=p;*out=value;return 1;
}
/* count includes exactly the final NUL. No read extends past this bound;
 * malformed input leaves the caller's output unchanged. */
static int scene_voice_parse(const wchar_t *text,size_t count,SceneVoiceMessage *out) {
    if(!text||!out||count<13||count>SCENE_VOICE_MESSAGE_CAPACITY||text[count-1])return 0;
    for(size_t i=0;i+1<count;++i)if(text[i]<1||text[i]>127)return 0;
    if(wmemcmp(text,L"ME3VO1 ",7)||(text[7]!=L'P'&&text[7]!=L'S'&&text[7]!=L'H')||text[8]!=L' ')return 0;
    SceneVoiceMessage m={0};m.kind=(char)text[7];
    uint64_t words[7]={0};const wchar_t *p=text+9,*end=text+count-1;
    unsigned fields=m.kind=='P'?7:m.kind=='H'?3:2;
    for(unsigned i=0;i<fields;++i) {
        uint64_t limit=i<2?UINT64_MAX:m.kind=='H'?1:i<4?2047:i<6?INT32_MAX:7;
        if(i&&(p>=end||*p++!=L' '))return 0;
        if(!scene_voice_number(&p,end,limit,words+i))return 0;
    }
    if(p!=end)return 0;
    m.scene_nonce=words[0];m.generation=words[1];m.matinee_channel=(uint32_t)words[2];
    m.speaker_channel=(uint32_t)words[3];m.resource_id=(uint32_t)words[4];
    m.strref=(uint32_t)words[5];m.flags=(uint32_t)words[6];
    if(m.kind=='H'){m.flags=(uint32_t)words[2];m.matinee_channel=0;}
    if(!scene_voice_message_valid(&m))return 0;*out=m;return 1;
}
/* Locale-independent canonical decimal. The largest message fits well
 * within the fixed capacity; explicit checks preserve the format contract. */
static int scene_voice_format(const SceneVoiceMessage *m,wchar_t text[SCENE_VOICE_MESSAGE_CAPACITY]) {
    if(!text)return 0;text[0]=0;if(!scene_voice_message_valid(m))return 0;
    wmemcpy(text,L"ME3VO1 ",7);text[7]=(wchar_t)m->kind;size_t at=8;
    uint64_t words[7]={m->scene_nonce,m->generation,m->matinee_channel,m->speaker_channel,
        m->resource_id,m->strref,m->flags};unsigned fields=m->kind=='P'?7:m->kind=='H'?3:2;
    if(m->kind=='H')words[2]=m->flags;
    for(unsigned i=0;i<fields;++i) {
        wchar_t digits[20];unsigned n=0;uint64_t value=words[i];
        do{digits[n++]=(wchar_t)(L'0'+value%10);value/=10;}while(value);
        if(at+1+n>=SCENE_VOICE_MESSAGE_CAPACITY){text[0]=0;return 0;}
        text[at++]=L' ';while(n)text[at++]=digits[--n];
    }
    text[at]=0;return (int)at+1;
}
static int scene_voice_message_same(const SceneVoiceMessage *a,const SceneVoiceMessage *b) {
    return a&&b&&a->kind==b->kind&&a->scene_nonce==b->scene_nonce&&a->generation==b->generation&&
        a->matinee_channel==b->matinee_channel&&a->speaker_channel==b->speaker_channel&&
        a->resource_id==b->resource_id&&a->strref==b->strref&&a->flags==b->flags;
}
/* Sequence wrap and adoption of another scene are never accepted here. */
static int scene_voice_message_newer(const SceneVoiceMessage *m,uint64_t scene,uint64_t generation) {
    return scene_voice_message_valid(m)&&scene&&m->scene_nonce==scene&&m->generation>generation;
}
static int scene_voice_protocol_selftest(void) {
    SceneVoiceMessage m={'P',UINT64_MAX,UINT64_MAX,2047,2047,INT32_MAX,INT32_MAX,7},p={0};
    wchar_t text[SCENE_VOICE_MESSAGE_CAPACITY]={0};int n=scene_voice_format(&m,text);
    if(!n||n>SCENE_VOICE_MESSAGE_CAPACITY||!scene_voice_parse(text,(size_t)n,&p)||
       !scene_voice_message_same(&m,&p)||scene_voice_parse(text,(size_t)n-1,&p)||
       scene_voice_parse(text,(size_t)n+1,&p)||scene_voice_parse(NULL,(size_t)n,&p)||
       scene_voice_parse(text,(size_t)n,NULL)||scene_voice_format(&m,NULL)||
       !scene_voice_message_newer(&m,UINT64_MAX,UINT64_MAX-1)||
       scene_voice_message_newer(&m,UINT64_MAX,UINT64_MAX)||scene_voice_message_newer(&m,0,0)||
       scene_voice_message_newer(&m,UINT64_MAX-1,0))return 0;
    SceneVoiceMessage stop={'S',1,2,0,0,0,0,0};n=scene_voice_format(&stop,text);
    if(n!=13||!scene_voice_parse(text,(size_t)n,&p)||!scene_voice_message_same(&stop,&p)||
       wcscmp(text,L"ME3VO1 S 1 2"))return 0;
    SceneVoiceMessage host={'H',1,2,0,0,0,0,1};n=scene_voice_format(&host,text);
    if(!n||!scene_voice_parse(text,(size_t)n,&p)||!scene_voice_message_same(&host,&p)||wcscmp(text,L"ME3VO1 H 1 2 1"))return 0;
    const wchar_t *bad[]={
        L"ME3VO0 S 1 2",L"ME3VO2 S 1 2",L"ME3SC1 S 1 2",L"ME3CI1 S 1 2",L"ME3DC1 S 1 2",
        L"ME3VO1 X 1 2",L"ME3VO1 H 1 2 2",L"ME3VO1 H 1 2 1 0",L"ME3VO1 H 1 2 01",L"ME3VO1  S 1 2",L"ME3VO1 S 0 1",L"ME3VO1 S 1 0",
        L"ME3VO1 S 01 2",L"ME3VO1 S 1 02",L"ME3VO1 S +1 2",L"ME3VO1 S -1 2",
        L"ME3VO1 S 1 2 ",L"ME3VO1 S 1 2 0",L"ME3VO1 S 1\t2",L"ME3VO1 S 1  2",
        L"ME3VO1 S 18446744073709551616 1",L"ME3VO1 S 1 18446744073709551616",
        L"ME3VO1 P 1 1 0 1 1 1 0",L"ME3VO1 P 1 1 1 0 1 1 0",L"ME3VO1 P 1 1 2048 1 1 1 0",
        L"ME3VO1 P 1 1 1 2048 1 1 0",L"ME3VO1 P 1 1 1 1 0 1 0",L"ME3VO1 P 1 1 1 1 1 0 0",
        L"ME3VO1 P 1 1 1 1 2147483648 1 0",L"ME3VO1 P 1 1 1 1 1 2147483648 0",
        L"ME3VO1 P 1 1 1 1 1 1 8",L"ME3VO1 P 1 1 1 1 1 1 00",L"ME3VO1 P 1 1 1 1 1 1",
        L"ME3VO1 P 1 1 1 1 1 1 0 extra",L"ME3VO1 P 1 1 1 1 1 1 0\n"};
    for(unsigned i=0;i<sizeof bad/sizeof *bad;++i) {
        p=stop;
        if(scene_voice_parse(bad[i],wcslen(bad[i])+1,&p)||!scene_voice_message_same(&p,&stop))return 0;
    }
    n=scene_voice_format(&m,text);text[10]=0;
    if(scene_voice_parse(text,(size_t)n,&p))return 0;
    n=scene_voice_format(&m,text);text[10]=(wchar_t)128;
    if(scene_voice_parse(text,(size_t)n,&p)||scene_voice_parse(text,0,&p)||
       scene_voice_parse(text,SCENE_VOICE_MESSAGE_CAPACITY+1,&p))return 0;
    SceneVoiceMessage invalid=m;invalid.kind='S';
    if(scene_voice_message_valid(&invalid)||scene_voice_format(&invalid,text)||text[0])return 0;
    for(unsigned i=0;i<7;++i) {
        invalid=m;
        if(i==0)invalid.scene_nonce=0;else if(i==1)invalid.generation=0;
        else if(i==2)invalid.matinee_channel=0;else if(i==3)invalid.speaker_channel=0;
        else if(i==4)invalid.resource_id=0;else if(i==5)invalid.strref=0;else invalid.flags=8;
        if(scene_voice_message_valid(&invalid)||scene_voice_format(&invalid,text)||text[0])return 0;
    }
    return 1;
}
#endif
