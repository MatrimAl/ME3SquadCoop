#ifndef ME3_DIALOGUE_CHOICE_PROTOCOL_H
#define ME3_DIALOGUE_CHOICE_PROTOCOL_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <wchar.h>
/* UI presentation/advice only. A parsed packet is never connection authority.
 * Fixed lowercase hexadecimal fields also carry localized UTF-16 unchanged. */
enum {DIALOGUE_CHOICE_ROWS=6,DIALOGUE_CHOICE_TEXT=512,DIALOGUE_CHOICE_CHUNK=28,DIALOGUE_CHOICE_WIRE=192};
typedef struct DialogueChoiceRow {int32_t reply;uint32_t mode;unsigned length;uint16_t text[DIALOGUE_CHOICE_TEXT];} DialogueChoiceRow;
typedef struct DialogueChoiceCatalog {uint64_t nonce,generation;uint32_t digest;int32_t entry;DialogueChoiceRow rows[6];} DialogueChoiceCatalog;
typedef struct DialogueChoicePacket {
    wchar_t kind;uint64_t nonce,generation;uint32_t digest;int32_t entry,reply;
    unsigned slot,mode,length,offset,count;uint16_t text[DIALOGUE_CHOICE_CHUNK];
} DialogueChoicePacket;
typedef struct DialogueChoiceStage {
    DialogueChoiceCatalog catalog;unsigned char seen[6][DIALOGUE_CHOICE_TEXT];
    unsigned row_mask,commit,failed;
} DialogueChoiceStage;
static int dialogue_choice_text_valid(const uint16_t *text,unsigned count) {
    for(unsigned i=0;i<count;++i){uint16_t c=text[i];if(!c)return 0;
        if(c>=0xd800&&c<=0xdbff){if(++i>=count||text[i]<0xdc00||text[i]>0xdfff)return 0;}
        else if(c>=0xdc00&&c<=0xdfff)return 0;}
    return 1;
}
static uint32_t dialogue_choice_hash_word(uint32_t h,uint32_t v,unsigned bytes) {
    for(unsigned i=0;i<bytes;++i){h^=(v>>(i*8))&255u;h*=UINT32_C(16777619);}return h;
}
static uint32_t dialogue_choice_digest(const DialogueChoiceCatalog *c) {
    uint32_t h=dialogue_choice_hash_word(UINT32_C(2166136261),(uint32_t)c->entry,4);
    for(unsigned i=0;i<6;++i){const DialogueChoiceRow *r=c->rows+i;
        h=dialogue_choice_hash_word(h,(uint32_t)r->reply,4);h=dialogue_choice_hash_word(h,r->mode,4);
        h=dialogue_choice_hash_word(h,r->length,4);
        for(unsigned j=0;j<r->length&&j<DIALOGUE_CHOICE_TEXT;++j)h=dialogue_choice_hash_word(h,r->text[j],2);}
    return h;
}
static int dialogue_choice_catalog_valid(const DialogueChoiceCatalog *c) {
    if(!c||!c->nonce||!c->generation||c->entry<0)return 0;
    for(unsigned i=0;i<6;++i){const DialogueChoiceRow *r=c->rows+i;
        if(r->reply< -3||r->mode>4||r->length>DIALOGUE_CHOICE_TEXT||
           !dialogue_choice_text_valid(r->text,r->length)||(r->reply==-1&&r->length))return 0;}
    return c->digest==dialogue_choice_digest(c);
}
static int dialogue_choice_packet_valid(const DialogueChoicePacket *p) {
    if(!p||!p->nonce||!p->generation||p->entry<0)return 0;
    if(p->kind==L'C'||p->kind==L'Z')return !p->slot&&!p->reply&&!p->mode&&!p->length&&!p->offset&&!p->count;
    if(p->slot>=6||p->reply< -3||p->mode>4)return 0;
    if(p->kind==L'V'||p->kind==L'A')return p->reply>=0&&!p->mode&&!p->length&&!p->offset&&!p->count;
    return p->kind==L'F'&&p->length<=DIALOGUE_CHOICE_TEXT&&p->offset<=p->length&&
        p->count<=DIALOGUE_CHOICE_CHUNK&&p->count<=p->length-p->offset&&
        (p->length?(p->count>0):(!p->offset&&!p->count))&&!(p->reply==-1&&p->length);
}
static void dialogue_choice_put(wchar_t **at,uint64_t value,unsigned digits) {
    static const wchar_t hex[]=L"0123456789abcdef";
    for(unsigned i=digits;i;--i)*(*at)++=hex[(value>>((i-1)*4))&15];
}
static int dialogue_choice_get(const wchar_t **at,const wchar_t *end,unsigned digits,uint64_t *value) {
    if((size_t)(end-*at)<digits)return 0;uint64_t n=0;
    for(unsigned i=0;i<digits;++i){wchar_t c=*(*at)++;unsigned d;
        if(c>=L'0'&&c<=L'9')d=(unsigned)(c-L'0');else if(c>=L'a'&&c<=L'f')d=(unsigned)(c-L'a'+10);else return 0;n=(n<<4)|d;}
    *value=n;return 1;
}
static int dialogue_choice_format(const DialogueChoicePacket *p,wchar_t text[DIALOGUE_CHOICE_WIRE]) {
    if(!text)return 0;text[0]=0;if(!dialogue_choice_packet_valid(p))return 0;
    wmemcpy(text,L"ME3DC1 ",7);wchar_t *at=text+7;*at++=p->kind;
    dialogue_choice_put(&at,p->nonce,16);dialogue_choice_put(&at,p->generation,16);
    dialogue_choice_put(&at,p->digest,8);dialogue_choice_put(&at,(uint32_t)p->entry,8);
    if(p->kind==L'F'||p->kind==L'V'||p->kind==L'A'){
        dialogue_choice_put(&at,p->slot,1);dialogue_choice_put(&at,(uint32_t)p->reply,8);}
    if(p->kind==L'F'){
        dialogue_choice_put(&at,p->mode,1);dialogue_choice_put(&at,p->length,3);
        dialogue_choice_put(&at,p->offset,3);dialogue_choice_put(&at,p->count,2);
        for(unsigned i=0;i<p->count;++i)dialogue_choice_put(&at,p->text[i],4);}
    if(at-text>=DIALOGUE_CHOICE_WIRE){text[0]=0;return 0;}*at=0;return (int)(at-text)+1;
}
static int dialogue_choice_parse(const wchar_t *text,size_t count,DialogueChoicePacket *out) {
    if(!text||!out||count<57||count>DIALOGUE_CHOICE_WIRE||text[count-1]||wmemcmp(text,L"ME3DC1 ",7))return 0;
    for(size_t i=0;i+1<count;++i)if(text[i]<1||text[i]>127)return 0;
    DialogueChoicePacket p={0};const wchar_t *at=text+7,*end=text+count-1;uint64_t v;
    p.kind=*at++;
    if(!dialogue_choice_get(&at,end,16,&p.nonce)||!dialogue_choice_get(&at,end,16,&p.generation)||
       !dialogue_choice_get(&at,end,8,&v))return 0;p.digest=(uint32_t)v;
    if(!dialogue_choice_get(&at,end,8,&v))return 0;p.entry=(int32_t)(uint32_t)v;
    if(p.kind==L'F'||p.kind==L'V'||p.kind==L'A'){
        if(!dialogue_choice_get(&at,end,1,&v))return 0;p.slot=(unsigned)v;
        if(!dialogue_choice_get(&at,end,8,&v))return 0;p.reply=(int32_t)(uint32_t)v;}
    if(p.kind==L'F'){
        if(!dialogue_choice_get(&at,end,1,&v))return 0;p.mode=(unsigned)v;
        if(!dialogue_choice_get(&at,end,3,&v))return 0;p.length=(unsigned)v;
        if(!dialogue_choice_get(&at,end,3,&v))return 0;p.offset=(unsigned)v;
        if(!dialogue_choice_get(&at,end,2,&v)||v>DIALOGUE_CHOICE_CHUNK)return 0;p.count=(unsigned)v;
        for(unsigned i=0;i<p.count;++i){if(!dialogue_choice_get(&at,end,4,&v))return 0;p.text[i]=(uint16_t)v;}}
    if(at!=end||!dialogue_choice_packet_valid(&p))return 0;*out=p;return 1;
}
static DialogueChoicePacket dialogue_choice_packet(const DialogueChoiceCatalog *c,wchar_t kind,unsigned slot,unsigned offset) {
    DialogueChoicePacket p={0};p.kind=kind;p.nonce=c->nonce;p.generation=c->generation;p.digest=c->digest;p.entry=c->entry;
    if((kind==L'F'||kind==L'V'||kind==L'A')&&slot>=6){p.nonce=0;return p;}
    if((kind==L'F'||kind==L'V'||kind==L'A')&&slot<6){const DialogueChoiceRow *r=c->rows+slot;p.slot=slot;p.reply=r->reply;
        if(kind==L'F'){p.mode=r->mode;p.length=r->length;p.offset=offset;
            if(offset<=r->length){p.count=r->length-offset;if(p.count>DIALOGUE_CHOICE_CHUNK)p.count=DIALOGUE_CHOICE_CHUNK;
                memcpy(p.text,r->text+offset,p.count*2);}}}
    return p;
}
/* Return2 only when an entire matching catalog is ready. A commit arriving
 * before fragments is harmless; no partial row ever becomes visible. */
static int dialogue_choice_stage(DialogueChoiceStage *s,const DialogueChoicePacket *p,DialogueChoiceCatalog *out) {
    if(!s||!out||!dialogue_choice_packet_valid(p)||(p->kind!=L'F'&&p->kind!=L'C'&&p->kind!=L'Z'))return 0;
    if(p->nonce<s->catalog.nonce||(p->nonce==s->catalog.nonce&&p->generation<s->catalog.generation))return 0;
    if(p->nonce!=s->catalog.nonce||p->generation!=s->catalog.generation){memset(s,0,sizeof *s);
        s->catalog.nonce=p->nonce;s->catalog.generation=p->generation;s->catalog.digest=p->digest;s->catalog.entry=p->entry;}
    if(s->failed||s->catalog.digest!=p->digest||s->catalog.entry!=p->entry){s->failed=1;return 0;}
    if(p->kind==L'Z'){s->failed=1;return 3;}
    if(p->kind==L'C')s->commit=1;
    else {DialogueChoiceRow *r=s->catalog.rows+p->slot;unsigned bit=1u<<p->slot;
        if((s->row_mask&bit)&&(r->reply!=p->reply||r->mode!=p->mode||r->length!=p->length)){s->failed=1;return 0;}
        if(!(s->row_mask&bit)){r->reply=p->reply;r->mode=p->mode;r->length=p->length;s->row_mask|=bit;}
        for(unsigned i=0;i<p->count;++i){unsigned at=p->offset+i;
            if(s->seen[p->slot][at]&&r->text[at]!=p->text[i]){s->failed=1;return 0;}
            r->text[at]=p->text[i];s->seen[p->slot][at]=1;}}
    if(!s->commit||s->row_mask!=63)return 1;
    for(unsigned i=0;i<6;++i)for(unsigned j=0;j<s->catalog.rows[i].length;++j)if(!s->seen[i][j])return 1;
    if(!dialogue_choice_catalog_valid(&s->catalog)){s->failed=1;return 0;}*out=s->catalog;return 2;
}
static int dialogue_choice_vote_matches(const DialogueChoiceCatalog *c,const DialogueChoicePacket *p) {
    return dialogue_choice_catalog_valid(c)&&dialogue_choice_packet_valid(p)&&(p->kind==L'V'||p->kind==L'A')&&
        p->nonce==c->nonce&&p->generation==c->generation&&p->digest==c->digest&&p->entry==c->entry&&p->slot<6&&
        c->rows[p->slot].reply==p->reply&&p->reply>=0&&c->rows[p->slot].mode!=4&&c->rows[p->slot].length;
}
static int dialogue_choice_protocol_selftest(void) {
    DialogueChoiceCatalog c={0},out={0};DialogueChoiceStage stage={0};c.nonce=UINT64_MAX;c.generation=1;c.entry=9;
    for(unsigned i=0;i<6;++i){c.rows[i].reply=(int32_t)i;c.rows[i].mode=i%5;c.rows[i].length=i?1:512;
        for(unsigned j=0;j<c.rows[i].length;++j)c.rows[i].text[j]=(uint16_t)(0x100+j);}
    c.rows[0].text[27]=0xd83d;c.rows[0].text[28]=0xde80;c.digest=dialogue_choice_digest(&c);
    if(!dialogue_choice_catalog_valid(&c))return 0;
    DialogueChoicePacket p=dialogue_choice_packet(&c,L'C',0,0),parsed={0};wchar_t wire[192];int n=dialogue_choice_format(&p,wire);
    if(!n||!dialogue_choice_parse(wire,(size_t)n,&parsed)||dialogue_choice_stage(&stage,&parsed,&out)!=1)return 0;
    for(int row=5;row>=0;--row)for(unsigned offset=0;offset<c.rows[row].length;offset+=28){
        p=dialogue_choice_packet(&c,L'F',(unsigned)row,offset);n=dialogue_choice_format(&p,wire);
        if(!n||n>192||!dialogue_choice_parse(wire,(size_t)n,&parsed)||!dialogue_choice_stage(&stage,&parsed,&out))return 0;}
    if(!dialogue_choice_catalog_valid(&out)||memcmp(out.rows,c.rows,sizeof c.rows))return 0;
    p=dialogue_choice_packet(&c,L'V',0,0);if(!dialogue_choice_vote_matches(&c,&p))return 0;
    p.generation++;if(dialogue_choice_vote_matches(&c,&p))return 0;
    p=dialogue_choice_packet(&c,L'V',4,0);if(dialogue_choice_vote_matches(&c,&p))return 0;
    const wchar_t kinds[]={L'F',L'V',L'A'};
    for(unsigned i=0;i<3;++i){p=dialogue_choice_packet(&c,kinds[i],6,0);
        if(dialogue_choice_packet_valid(&p)||dialogue_choice_format(&p,wire)||dialogue_choice_vote_matches(&c,&p))return 0;
        p=dialogue_choice_packet(&c,kinds[i],UINT32_MAX,0);if(dialogue_choice_packet_valid(&p))return 0;}
    p=dialogue_choice_packet(&c,L'F',0,0);p.text[0]++;if(dialogue_choice_stage(&stage,&p,&out)||!stage.failed)return 0;
    p=dialogue_choice_packet(&c,L'C',0,0);n=dialogue_choice_format(&p,wire);wire[8]=L'G';
    if(dialogue_choice_parse(wire,(size_t)n,&parsed)||dialogue_choice_parse(wire,(size_t)n-1,&parsed))return 0;
    c.rows[0].text[27]=0xd800;c.rows[0].text[28]=L'x';c.digest=dialogue_choice_digest(&c);
    return !dialogue_choice_catalog_valid(&c);
}
#endif
