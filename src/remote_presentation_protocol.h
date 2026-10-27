#ifndef ME3_REMOTE_PRESENTATION_PROTOCOL_H
#define ME3_REMOTE_PRESENTATION_PROTOCOL_H
/* Scalar presentation state only. Actor channels are connection-scoped;
 * callers must bind every message to the current native owner-PC RPC. */
typedef struct RemotePresentationMessage {
    wchar_t kind;
    uint64_t request;
    unsigned channel;
    unsigned char helmet[4];
    unsigned weapon_channel; /* 2048 means no admitted selected weapon */
} RemotePresentationMessage;
static int remote_presentation_number(const wchar_t **cursor,uint64_t limit,uint64_t *out) {
    const wchar_t *p=*cursor;uint64_t value=0;unsigned digits=0;
    if(*p<L'0'||*p>L'9'||(*p==L'0'&&p[1]>=L'0'&&p[1]<=L'9'))return 0;
    while(*p>=L'0'&&*p<=L'9') {
        uint64_t digit=(uint64_t)(*p-L'0');
        if(++digits>20||value>limit/10||(value==limit/10&&digit>limit%10))return 0;
        value=value*10+digit;++p;
    }
    *cursor=p;*out=value;return 1;
}
static int remote_presentation_parse(const wchar_t *text,size_t count,RemotePresentationMessage *m) {
    if(!text||!m||count<13||count>96||text[count-1])return 0;
    for(size_t i=0;i+1<count;++i)if(!text[i]||text[i]>127)return 0;
    if(wcsncmp(text,L"ME3PR2 ",7)||(text[7]!=L'Q'&&text[7]!=L'A')||text[8]!=L' ')return 0;
    RemotePresentationMessage value={0};value.kind=text[7];
    const wchar_t *p=text+9;uint64_t channel=0;
    if(!remote_presentation_number(&p,UINT64_MAX,&value.request)||!value.request||*p++!=L' '||
       !remote_presentation_number(&p,2047,&channel))return 0;
    value.channel=(unsigned)channel;
    if(value.kind==L'A')for(unsigned i=0;i<4;++i) {
        uint64_t state=0;
        if(*p++!=L' '||!remote_presentation_number(&p,3,&state))return 0;
        value.helmet[i]=(unsigned char)state;
    }
    if(value.kind==L'A') {
        uint64_t weapon=0;
        if(*p++!=L' '||!remote_presentation_number(&p,2048,&weapon))return 0;
        value.weapon_channel=(unsigned)weapon;
    }
    if(*p)return 0;
    *m=value;return 1;
}
static int remote_presentation_format(const RemotePresentationMessage *m,wchar_t text[96]) {
    int n=m->kind==L'Q'?_snwprintf(text,96,L"ME3PR2 Q %llu %u",(unsigned long long)m->request,m->channel):
        _snwprintf(text,96,L"ME3PR2 A %llu %u %u %u %u %u %u",(unsigned long long)m->request,m->channel,
            m->helmet[0],m->helmet[1],m->helmet[2],m->helmet[3],m->weapon_channel);
    RemotePresentationMessage check;
    return n>0&&n<96&&remote_presentation_parse(text,(size_t)n+1,&check)&&check.kind==m->kind?n+1:0;
}
static int remote_presentation_reply_matches(const RemotePresentationMessage *m,uint64_t request,
        unsigned channel,DWORD sent_at,DWORD now) {
    return m->kind==L'A'&&request&&m->request==request&&m->channel==channel&&now-sent_at<=3000;
}
static int remote_presentation_protocol_selftest(void) {
    RemotePresentationMessage a={L'A',UINT64_C(4294967297),2047,{2,0,0,1},4},b={0};wchar_t text[96];
    int n=remote_presentation_format(&a,text);
    if(!n||!remote_presentation_parse(text,n,&b)||memcmp(a.helmet,b.helmet,4)||b.request!=a.request||b.channel!=2047||b.weapon_channel!=4)return 0;
    const wchar_t *bad[]={L"ME3PR2 Q 0 1",L"ME3PR2 Q 01 1",L"ME3PR2 Q 18446744073709551616 1",
        L"ME3PR2 Q 1 2048",L"ME3PR2 A 1 2 4 0 0 1 4",L"ME3PR2 A 1 2 2 0 0",L"ME3PR2 A 1 2 -1 0 0 1 4",
        L"ME3PR2 A 1 2 2 0 0 1 4 extra",L"ME3PR2 Q 1 2 ",L"ME3PR2 X 1 2",L"ME3PR2 A 1 2 2 0 0 1 2049",
        L"ME3PR1 A 1 2 2 0 0 1"};
    for(unsigned i=0;i<sizeof bad/sizeof *bad;++i)if(remote_presentation_parse(bad[i],wcslen(bad[i])+1,&b))return 0;
    return remote_presentation_reply_matches(&a,a.request,2047,100,3100)&&
        !remote_presentation_reply_matches(&a,a.request+1,2047,100,200)&&
        !remote_presentation_reply_matches(&a,a.request,1,100,200)&&
        !remote_presentation_reply_matches(&a,a.request,2047,100,3101);
}
#endif
