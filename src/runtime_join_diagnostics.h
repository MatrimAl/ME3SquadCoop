#ifndef ME3_RUNTIME_JOIN_DIAGNOSTICS_H
#define ME3_RUNTIME_JOIN_DIAGNOSTICS_H
/* Observe the returned engine-owned ConsoleCommand FString synchronously,
 * before the caller's existing EngineFree. This never retains engine pointers,
 * dereferences the pre-travel controller/world, or changes native ownership.
 * Text belongs only in the private runtime log, not public validation evidence.
 */
#define RUNTIME_JOIN_TEXT_UNITS 512
#define RUNTIME_JOIN_JSON_BYTES (RUNTIME_JOIN_TEXT_UNITS*6+1)
typedef enum RuntimeJoinTextStatus {
    RUNTIME_JOIN_TEXT_VALID=0,RUNTIME_JOIN_TEXT_EMPTY=1,
    RUNTIME_JOIN_TEXT_TRUNCATED=2,RUNTIME_JOIN_TEXT_UNREADABLE=3,
    RUNTIME_JOIN_TEXT_MALFORMED=4
} RuntimeJoinTextStatus;
typedef struct RuntimeJoinText {
    RuntimeJoinTextStatus status;
    int32_t declared_units,copied_units;
    char escaped[RUNTIME_JOIN_JSON_BYTES];
} RuntimeJoinText;
static const char *runtime_join_text_status_name(RuntimeJoinTextStatus status) {
    switch(status) {
    case RUNTIME_JOIN_TEXT_VALID:return "valid";
    case RUNTIME_JOIN_TEXT_EMPTY:return "empty";
    case RUNTIME_JOIN_TEXT_TRUNCATED:return "truncated";
    case RUNTIME_JOIN_TEXT_MALFORMED:return "malformed";
    default:return "unreadable";
    }
}
static int runtime_join_escape_utf8(const char *input,size_t count,char *out,size_t capacity) {
    static const char hex[]="0123456789abcdef";
    size_t used=0;
    if(!out||!capacity||(!input&&count))return 0;
    out[0]=0;
    for(size_t i=0;i<count;++i) {
        unsigned char c=(unsigned char)input[i];size_t need=c<32?6:(c=='"'||c=='\\'?2:1);
        if(need>=capacity-used){out[0]=0;return 0;}
        if(c<32) {
            out[used++]='\\';out[used++]='u';out[used++]='0';out[used++]='0';
            out[used++]=hex[c>>4];out[used++]=hex[c&15];
        } else {
            if(c=='"'||c=='\\')out[used++]='\\';
            out[used++]=(char)c;
        }
    }
    out[used]=0;return 1;
}
static RuntimeJoinText runtime_join_capture_console_result(const GameString *result) {
    RuntimeJoinText out={0};GameString value={0},again={0};
    wchar_t wide[RUNTIME_JOIN_TEXT_UNITS]={0},last=0;
    out.status=RUNTIME_JOIN_TEXT_UNREADABLE;
    if(!result||!read_at((uintptr_t)result,&value,sizeof value))return out;
    out.declared_units=value.count;
    /* Diagnostic bounds are deliberately small. Refusal is not a conclusion
     * about why native travel failed. FString Count includes its final NUL. */
    if(value.count<0||value.max<value.count||value.max>65536||
       (!value.data&&(value.count||value.max))||
       (value.count&&((uintptr_t)value.data>UINTPTR_MAX-(size_t)value.count*sizeof(wchar_t)))) {
        out.status=RUNTIME_JOIN_TEXT_MALFORMED;return out;
    }
    if(!value.count){out.status=RUNTIME_JOIN_TEXT_EMPTY;return out;}
    size_t chars=(size_t)value.count-1;
    size_t copied=chars<RUNTIME_JOIN_TEXT_UNITS-1?chars:RUNTIME_JOIN_TEXT_UNITS-1;
    if((copied&&!read_at((uintptr_t)value.data,wide,copied*sizeof(wchar_t)))||
       !read_at((uintptr_t)value.data+chars*sizeof(wchar_t),&last,sizeof last)||
       !read_at((uintptr_t)result,&again,sizeof again))return out;
    if(value.data!=again.data||value.count!=again.count||value.max!=again.max)return out;
    if(last){out.status=RUNTIME_JOIN_TEXT_MALFORMED;return out;}
    for(size_t i=0;i<copied;++i)if(!wide[i]) {
        out.status=RUNTIME_JOIN_TEXT_MALFORMED;return out;
    }
    int truncated=chars>copied;
    /* Do not split a UTF-16 surrogate pair at the bounded prefix boundary. */
    if(truncated&&copied&&wide[copied-1]>=0xd800&&wide[copied-1]<=0xdbff)--copied;
    if(copied) {
        char utf8[RUNTIME_JOIN_TEXT_UNITS*3];
        int bytes=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,wide,(int)copied,
            utf8,sizeof utf8,NULL,NULL);
        if(bytes<=0||!runtime_join_escape_utf8(utf8,(size_t)bytes,out.escaped,sizeof out.escaped)) {
            out.status=RUNTIME_JOIN_TEXT_MALFORMED;return out;
        }
    }
    out.copied_units=(int32_t)copied;
    out.status=truncated?RUNTIME_JOIN_TEXT_TRUNCATED:(copied?RUNTIME_JOIN_TEXT_VALID:RUNTIME_JOIN_TEXT_EMPTY);
    return out;
}
static void runtime_join_log_console_result(const GameString *result) {
    RuntimeJoinText text=runtime_join_capture_console_result(result);
    char row[RUNTIME_JOIN_JSON_BYTES+280];
    snprintf(row,sizeof row,
        "{\"runtime_network\":\"join-console-result\",\"text_status\":\"%s\",\"declared_utf16_units\":%d,\"copied_utf16_units\":%d,\"text\":\"%s\",\"connection_verified\":false}\r\n",
        runtime_join_text_status_name(text.status),text.declared_units,text.copied_units,text.escaped);
    line(row);
}
static int runtime_join_diagnostics_selftest(void) {
    GameString value={0};RuntimeJoinText out=runtime_join_capture_console_result(&value);
    if(out.status!=RUNTIME_JOIN_TEXT_EMPTY||out.escaped[0]||out.copied_units)return 0;
    out=runtime_join_capture_console_result((const GameString*)1);
    if(out.status!=RUNTIME_JOIN_TEXT_UNREADABLE)return 0;
    value.count=2;value.max=2;out=runtime_join_capture_console_result(&value);
    if(out.status!=RUNTIME_JOIN_TEXT_MALFORMED)return 0;
    value.data=(wchar_t*)1;out=runtime_join_capture_console_result(&value);
    if(out.status!=RUNTIME_JOIN_TEXT_UNREADABLE)return 0;
    wchar_t escaped[]={L'"',L'\\',L'\n',L'\r',L'\t',1,0};
    value.data=escaped;value.count=value.max=7;out=runtime_join_capture_console_result(&value);
    if(out.status!=RUNTIME_JOIN_TEXT_VALID||out.copied_units!=6||
       strcmp(out.escaped,"\\\"\\\\\\u000a\\u000d\\u0009\\u0001"))return 0;
    value.max=6;out=runtime_join_capture_console_result(&value);
    if(out.status!=RUNTIME_JOIN_TEXT_MALFORMED)return 0;
    value.count=value.max=-1;out=runtime_join_capture_console_result(&value);
    if(out.status!=RUNTIME_JOIN_TEXT_MALFORMED)return 0;
    wchar_t no_nul[]={L'a',L'b'};value.data=no_nul;value.count=value.max=2;
    out=runtime_join_capture_console_result(&value);
    if(out.status!=RUNTIME_JOIN_TEXT_MALFORMED)return 0;
    wchar_t invalid[]={0xd800,0};value.data=invalid;value.count=value.max=2;
    out=runtime_join_capture_console_result(&value);
    if(out.status!=RUNTIME_JOIN_TEXT_MALFORMED)return 0;
    wchar_t long_text[RUNTIME_JOIN_TEXT_UNITS+4];
    for(size_t i=0;i<sizeof long_text/sizeof *long_text;++i)long_text[i]=L'x';
    long_text[RUNTIME_JOIN_TEXT_UNITS+3]=0;value.data=long_text;
    value.count=value.max=RUNTIME_JOIN_TEXT_UNITS+4;out=runtime_join_capture_console_result(&value);
    if(out.status!=RUNTIME_JOIN_TEXT_TRUNCATED||out.copied_units!=RUNTIME_JOIN_TEXT_UNITS-1||
       strlen(out.escaped)!=RUNTIME_JOIN_TEXT_UNITS-1)return 0;
    long_text[RUNTIME_JOIN_TEXT_UNITS-2]=0xd83d;long_text[RUNTIME_JOIN_TEXT_UNITS-1]=0xde00;
    out=runtime_join_capture_console_result(&value);
    if(out.status!=RUNTIME_JOIN_TEXT_TRUNCATED||out.copied_units!=RUNTIME_JOIN_TEXT_UNITS-2)return 0;
    wchar_t unicode[]={0x03a9,0xd83d,0xde00,0};value.data=unicode;value.count=value.max=4;
    out=runtime_join_capture_console_result(&value);
    if(out.status!=RUNTIME_JOIN_TEXT_VALID||strcmp(out.escaped,"\xce\xa9\xf0\x9f\x98\x80"))return 0;
    char tiny[2];return !runtime_join_escape_utf8("\n",1,tiny,sizeof tiny)&&!tiny[0];
}
#endif
