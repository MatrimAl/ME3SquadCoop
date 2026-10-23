#ifndef ME3_RUNTIME_TRANSPORT_H
#define ME3_RUNTIME_TRANSPORT_H
/* Exact native transport prerequisites, read-only. The supported socket
 * provider obtains MULTIHOME from appCmdLine, not FURL.Host or a mod variable.
 * Native host init may advance an occupied port, so inspect its actual endpoint.
 * Client init retains zero in its cached sockaddr when the OS selects an
 * ephemeral port; that cache is not a getsockname result.
 * These predicates establish neither reachability nor successful co-op. */
static int runtime_transport_pin(uintptr_t address,const unsigned char *wanted,size_t count) {
    unsigned char actual[128];
    return count<=sizeof actual&&read_at(address,actual,count)&&!memcmp(actual,wanted,count);
}
static int runtime_transport_commandline_pins(uintptr_t base) {
    static const unsigned char getter[]={0x48,0x8d,0x05,0xd9,0x43,0x6e,0x01,0xc3};
    static const unsigned char consumer[]={0xe8,0xd6,0x6f,0x81,0xff};
    static const unsigned char parse[]={0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x6c,0x24,0x18,0x57,0x48,0x83,0xec,0x20};
    return runtime_transport_pin(base+0x1196e0,getter,sizeof getter)&&
        runtime_transport_pin(base+0x902705,consumer,sizeof consumer)&&
        runtime_transport_pin(base+0x113d80,parse,sizeof parse);
}
static wchar_t runtime_transport_upper(wchar_t value) {
    return value>=L'a'&&value<=L'z'?value-(L'a'-L'A'):value;
}
static int runtime_transport_space(wchar_t value) {return value==L' '||value==L'\t';}
static int runtime_transport_multihome_matches(const wchar_t *command,size_t length,const wchar_t *bind) {
    static const wchar_t key[]=L"MULTIHOME=";
    if(!command||length>=8192||command[length]||
       !direct_ip_native_parse_ipv4(bind,0,NULL))return 0;
    size_t bind_length=wcslen(bind);unsigned matches=0;
    for(size_t i=0;i<length;++i) {
        if(!command[i])return 0;
        size_t j=0;
        while(j<10&&i+j<length&&runtime_transport_upper(command[i+j])==key[j])++j;
        if(j!=10)continue;
        /* Reject duplicates, embedded/quoted spelling and conflicting values.
         * The native parser accepts the first eligible case-insensitive match;
         * a single plain argument is the deliberately narrower supported form. */
        if(++matches!=1||i<1||command[i-1]!=L'-'||
           (i>1&&!runtime_transport_space(command[i-2]))||
           i+10+bind_length>length||wcsncmp(command+i+10,bind,bind_length)||
           (i+10+bind_length<length&&!runtime_transport_space(command[i+10+bind_length])))return 0;
    }
    return matches==1;
}
static int runtime_transport_bind_argument(uintptr_t base,const wchar_t *bind) {
    if(!runtime_transport_commandline_pins(base))return 0;
    /* The pinned getter is LEA RAX,[RIP+disp32], not an indirect pointer.
     * Derive its buffer from the instruction: 0x1196e0+7+0x16e43d9=
     * 0x17fdac0. The preceding 0x20 bytes are not command-line storage. */
    int32_t displacement=0;
    if(!read_at(base+0x1196e0+3,&displacement,sizeof displacement))return 0;
    uintptr_t command_address=base+0x1196e0+7+(intptr_t)displacement;
    wchar_t command[8192];size_t used=0;
    /* Engine buffer from the pinned getter. Bounded complete read, no
     * native function invocation and no substitution of GetCommandLineW. */
    while(used<8192) {
        size_t chunk=128;
        if(!read_at(command_address+used*2,command+used,chunk*2))return 0;
        for(size_t i=0;i<chunk;++i)if(!command[used+i])
            return runtime_transport_multihome_matches(command,used+i,bind);
        used+=chunk;
    }
    return 0;
}
static int runtime_transport_endpoint_bytes(const unsigned char address[16],
    const Me3RuntimeConfig *config) {
    unsigned char ip[4];
    if(!address||!config||!config->endpoint.port||!direct_ip_native_parse_ipv4(config->endpoint.bind_ipv4,0,ip)||
       (config->role!=ME3_ROLE_HOST&&config->role!=ME3_ROLE_GUEST)||
       address[0]!=2||address[1]!=0||memcmp(address+4,ip,4))return 0;
    unsigned port=((unsigned)address[2]<<8)|address[3];
    /* InitConnect passes bClient=1 to 0x903120. At 0x90339a..0x9033ac that
     * branch keeps driver.LocalAddr.Port zero; BindNextPort returns the real
     * ephemeral port without copying it back on the successful zero-port path
     * (0x90d2eb..0x90d311). Requiring nonzero here blocks valid guest control.
     * The caller still validates native client net mode and server connection.
     * A host's requested nonzero port remains exact, including fallback refusal. */
    return config->role==ME3_ROLE_GUEST||port==config->endpoint.port;
}
static int runtime_transport_endpoint(uintptr_t base,uintptr_t driver,const Me3RuntimeConfig *config) {
    static const unsigned char port_source[]={0x41,0x0f,0xb7,0x4f,0x50};
    static const unsigned char bind_call[]={0xe8,0x72,0x9e,0x00,0x00};
    unsigned char first[16],last[16];
    uintptr_t vt=read_ptr(driver);
    return is_live(base,driver)&&is_kind_of(base,driver,"TcpNetDriver")&&vt==base+0x12bc980&&
        read_ptr(vt+0x2e0)==base+0x903a80&&
        runtime_transport_pin(base+0x9033ae,port_source,sizeof port_source)&&
        runtime_transport_pin(base+0x9033e9,bind_call,sizeof bind_call)&&
        read_at(driver+0x1e4,first,sizeof first)&&runtime_transport_endpoint_bytes(first,config)&&
        read_at(driver+0x1e4,last,sizeof last)&&!memcmp(first,last,sizeof first)&&read_ptr(driver)==vt;
}
#endif
