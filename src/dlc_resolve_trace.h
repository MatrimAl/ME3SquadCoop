#ifndef ME3_DLC_RESOLVE_TRACE_H
#define ME3_DLC_RESOLVE_TRACE_H
/* Exact supported file-manager virtual+e8 ABI, checked against callsite
 * afd37a..afd38a and wrapper a48b80..a48b9a. Resident forwarding observer. */
typedef int (*DlcResolveOriginal)(void*,const wchar_t*,GameString*);
static DlcResolveOriginal dlc_resolve_original;
static volatile LONG dlc_resolve_rows;
static const char *dlc_resolve_install_reason="not-attempted";
static unsigned dlc_resolve_install_attempts;
static int dlc_resolve_installed;
static int dlc_resolve_name(const wchar_t *path) {
    return path&&!_wcsicmp(path,L"Default_DLC_MOD_ME3SquadCoop.bin");
}
static void dlc_resolve_text(const wchar_t *path,char *out,size_t capacity) {
    size_t used=0;out[0]=0;
    for(unsigned i=0;path&&i<MAX_PATH&&used+7<capacity;++i) {
        wchar_t c=0;if(!read_at((uintptr_t)(path+i),&c,2)||!c)break;
        if(c==L'\\'||c==L'"'){out[used++]='\\';out[used++]=(char)c;}
        else if(c>=32&&c<127)out[used++]=(char)c;
        else used+=(size_t)snprintf(out+used,capacity-used,"\\u%04x",(unsigned)c);
    }
    out[used]=0;
}
static int dlc_resolve_forward(void *self,const wchar_t *name,GameString *output) {
    DWORD before=GetLastError();
    wchar_t bounded[64]={0};
    int selected=0;
    if(name&&read_at((uintptr_t)name,bounded,sizeof bounded-2))selected=dlc_resolve_name(bounded);
    SetLastError(before);
    int result=dlc_resolve_original(self,name,output);
    DWORD after=GetLastError();
    if(selected&&InterlockedIncrement(&dlc_resolve_rows)<=16) {
        GameString value={0};char resolved[6*MAX_PATH]={0},row[7*MAX_PATH];
        int readable=output&&read_at((uintptr_t)output,&value,sizeof value)&&value.count>0&&
            value.count<=MAX_PATH&&value.max>=value.count&&value.max<=4096;
        if(readable)dlc_resolve_text(value.data,resolved,sizeof resolved);
        snprintf(row,sizeof row,"{\"dlc_config_resolve\":true,\"name\":\"Default_DLC_MOD_ME3SquadCoop.bin\",\"native_result\":%d,\"output_readable\":%s,\"resolved_path\":\"%s\",\"forwarded_unchanged\":true,\"monotonic_ms\":%lu}\r\n",result,readable?"true":"false",resolved,GetTickCount());
        line(row);
    }
    SetLastError(after);return result;
}
static int dlc_resolve_install(uintptr_t base) {
    dlc_resolve_install_reason="not-opted";
    if(!probe_dlc_io_trace||!probe_guards_only)return 0;
    uintptr_t manager=read_ptr(base+0x17cd5d0),vt=read_ptr(manager),slot=vt+0xe8;
    const unsigned char expected[]={0x48,0x83,0xec,0x28,0x45,0x33,0xc9,0xe8,0xc4,0xd7,0xff,0xff,0x33,0xc9,0x48,0x85,0xc0,0x0f,0x95,0xc1,0x8b,0xc1,0x48,0x83,0xc4,0x28,0xc3};
    unsigned char actual[sizeof expected];
    dlc_resolve_install_reason="manager-not-ready";if(!manager)return 0;
    dlc_resolve_install_reason="vtable-identity";if(vt!=base+0x12ff310)return 0;
    dlc_resolve_install_reason="slot-identity";if(read_ptr(slot)!=base+0xa48b80)return 0;
    dlc_resolve_install_reason="wrapper-pins";
    if(!read_at(base+0xa48b80,actual,sizeof actual)||memcmp(actual,expected,sizeof expected))return 0;
    DWORD old=0;dlc_resolve_install_reason="slot-protection";
    if(!VirtualProtect((void*)slot,8,PAGE_READWRITE,&old))return 0;
    dlc_resolve_original=(DlcResolveOriginal)(base+0xa48b80);
    PVOID previous=InterlockedCompareExchangePointer((PVOID volatile*)slot,(PVOID)dlc_resolve_forward,(PVOID)dlc_resolve_original);
    VirtualProtect((void*)slot,8,old,&old);
    if(previous!=(PVOID)dlc_resolve_original){dlc_resolve_original=NULL;dlc_resolve_install_reason="slot-changed";return 0;}
    dlc_resolve_install_reason="installed";
    line("{\"dlc_resolve_observer\":true,\"slot\":\"file-manager+e8\",\"max_rows\":16,\"read_only_arguments\":true}\r\n");return 1;
}
static void dlc_resolve_install_observe(uintptr_t base) {
    if(!probe_dlc_io_trace||!probe_guards_only||dlc_resolve_installed||dlc_resolve_install_attempts>=140)return;
    static const char *last_reason;
    ++dlc_resolve_install_attempts;
    dlc_resolve_installed=dlc_resolve_install(base);
    if(!dlc_resolve_installed&&(!last_reason||strcmp(last_reason,dlc_resolve_install_reason))) {
        char row[240];snprintf(row,sizeof row,"{\"dlc_resolve_observer\":false,\"reason\":\"%s\",\"attempt\":%u,\"retry_remaining\":%u}\r\n",dlc_resolve_install_reason,dlc_resolve_install_attempts,140-dlc_resolve_install_attempts);line(row);
    }
    last_reason=dlc_resolve_install_reason;
}
#endif
