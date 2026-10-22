#ifndef ME3_RUNTIME_CONFIG_H
#define ME3_RUNTIME_CONFIG_H
#include "direct_ip_native.h"

/* Explicit future runtime configuration; no QA role/default/loopback fallback.
 * Parsing opens no sockets, changes no files and calls no game methods. */
typedef enum Me3RuntimeRole {ME3_ROLE_NONE=0,ME3_ROLE_HOST=1,ME3_ROLE_GUEST=2} Me3RuntimeRole;
typedef struct Me3RuntimeConfig {Me3RuntimeRole role;DirectIPNativeConfig endpoint;} Me3RuntimeConfig;
typedef enum Me3RuntimeConfigStatus {
    ME3_CONFIG_DISABLED=0,ME3_CONFIG_READY=1,ME3_CONFIG_BAD_ARGUMENT=2,
    ME3_CONFIG_BAD_ENABLED=3,ME3_CONFIG_BAD_ROLE=4,ME3_CONFIG_BAD_ENDPOINT=5,ME3_CONFIG_ENVIRONMENT_ERROR=6
} Me3RuntimeConfigStatus;
static Me3RuntimeConfigStatus runtime_config_parse(Me3RuntimeConfig *out,const wchar_t *enabled,
    const wchar_t *role,const wchar_t *bind_ipv4,const wchar_t *peer_ipv4,const wchar_t *port) {
    if(!out)return ME3_CONFIG_BAD_ARGUMENT;memset(out,0,sizeof *out);
    if(!enabled||!wcscmp(enabled,L"0"))return ME3_CONFIG_DISABLED;
    if(wcscmp(enabled,L"1"))return ME3_CONFIG_BAD_ENABLED;
    Me3RuntimeConfig config={0};
    if(role&&!wcscmp(role,L"host"))config.role=ME3_ROLE_HOST;
    else if(role&&!wcscmp(role,L"guest"))config.role=ME3_ROLE_GUEST;
    else return ME3_CONFIG_BAD_ROLE;
    /* Fixed non-QA policy makes an absent endpoint incomplete and rejects
     * loopback literals even if the probe's QA environment is inherited. */
    if(direct_ip_native_parse_config(&config.endpoint,config.role==ME3_ROLE_GUEST,
        bind_ipv4,peer_ipv4,port,L"0")!=DIRECT_IP_NATIVE_OK||config.endpoint.defaults_used||config.endpoint.loopback_qa)
        return ME3_CONFIG_BAD_ENDPOINT;
    *out=config;return ME3_CONFIG_READY;
}
static Me3RuntimeConfigStatus runtime_config_load_environment(Me3RuntimeConfig *out) {
    if(!out)return ME3_CONFIG_BAD_ARGUMENT;memset(out,0,sizeof *out);
    wchar_t enabled[2],role[8],bind[16],peer[16],port[6];int en=0,ro=0,bi=0,pe=0,po=0;
    if(!direct_ip_native_read_environment(L"ME3_SQUAD_COOP_ENABLED",enabled,2,&en)||
       !direct_ip_native_read_environment(L"ME3_SQUAD_COOP_ROLE",role,8,&ro)||
       !direct_ip_native_read_environment(L"ME3_SQUAD_COOP_BIND_IPV4",bind,16,&bi)||
       !direct_ip_native_read_environment(L"ME3_SQUAD_COOP_PEER_IPV4",peer,16,&pe)||
       !direct_ip_native_read_environment(L"ME3_SQUAD_COOP_PORT",port,6,&po))return ME3_CONFIG_ENVIRONMENT_ERROR;
    return runtime_config_parse(out,en?enabled:NULL,ro?role:NULL,bi?bind:NULL,pe?peer:NULL,po?port:NULL);
}
static int runtime_config_selftest(void) {
    Me3RuntimeConfig c;
    if(runtime_config_parse(NULL,NULL,NULL,NULL,NULL,NULL)!=ME3_CONFIG_BAD_ARGUMENT)return 0;
    if(runtime_config_parse(&c,NULL,NULL,NULL,NULL,NULL)!=ME3_CONFIG_DISABLED||c.role||c.endpoint.port)return 0;
    if(runtime_config_parse(&c,L"1",L"host",L"26.1.2.3",NULL,L"17779")!=ME3_CONFIG_READY||
       c.role!=ME3_ROLE_HOST||c.endpoint.port!=17779||c.endpoint.defaults_used||c.endpoint.loopback_qa)return 0;
    if(runtime_config_parse(&c,L"1",L"guest",L"26.1.2.3",L"26.2.3.4",L"17779")!=ME3_CONFIG_READY||c.role!=ME3_ROLE_GUEST)return 0;
    if(runtime_config_parse(&c,L"1",L"host",NULL,NULL,NULL)!=ME3_CONFIG_BAD_ENDPOINT||c.role||c.endpoint.port)return 0;
    if(runtime_config_parse(&c,L"1",L"client",L"26.1.2.3",L"26.2.3.4",L"17779")!=ME3_CONFIG_BAD_ROLE)return 0;
    if(runtime_config_parse(&c,L"true",L"host",L"26.1.2.3",NULL,L"17779")!=ME3_CONFIG_BAD_ENABLED)return 0;
    if(runtime_config_parse(&c,L"1",L"host",L"127.0.0.1",NULL,L"17779")!=ME3_CONFIG_BAD_ENDPOINT)return 0;
    if(runtime_config_parse(&c,L"1",L"host",L"26.1.2.3",L"26.2.3.4",L"17779")!=ME3_CONFIG_BAD_ENDPOINT)return 0;
    if(runtime_config_parse(&c,L"1",L"guest",L"26.1.2.3",NULL,L"17779")!=ME3_CONFIG_BAD_ENDPOINT)return 0;
    return runtime_config_parse(&c,L"0",NULL,NULL,NULL,NULL)==ME3_CONFIG_DISABLED&&!c.role&&!c.endpoint.port;
}
#endif
