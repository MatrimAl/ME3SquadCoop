#ifndef ME3_RUNTIME_TRANSPORT_FIXTURE_H
#define ME3_RUNTIME_TRANSPORT_FIXTURE_H
/* Allocated synthetic image and objects. No engine calls or sockets. */
static int runtime_transport_selftest(void) {
    static const wchar_t *bad[]={L"",L"-MULTIHOME=26.1.2.4",L"-MULTIHOME=26.1.2.30",
        L"-MULTIHOME=26.1.2.3junk",L"-MULTIHOME=026.1.2.3",L"MULTIHOME=26.1.2.3",
        L"foo-MULTIHOME=26.1.2.3",L"-XMULTIHOME=26.1.2.3",L"-MULTIHOME=\"26.1.2.3\"",
        L"\"-MULTIHOME=26.1.2.3\"",L"-MULTIHOME=26.1.2.3 -MULTIHOME=26.1.2.4",
        L"-MULTIHOME=26.1.2.3 -MULTIHOME=26.1.2.3",L"-MULTIHOME=26.1.2.4 -MULTIHOME=26.1.2.3"};
    for(size_t i=0;i<sizeof bad/sizeof bad[0];++i)
        if(runtime_transport_multihome_matches(bad[i],wcslen(bad[i]),L"26.1.2.3"))return 0;
    const wchar_t *good=L"-log\t-mUlTiHoMe=26.1.2.3 -NoSplash";
    if(!runtime_transport_multihome_matches(good,wcslen(good),L"26.1.2.3")||
       runtime_transport_multihome_matches(good,wcslen(good)-1,L"26.1.2.3")||
       runtime_transport_multihome_matches(good,wcslen(good),L"127.0.0.1")||
       runtime_transport_multihome_matches(NULL,0,L"26.1.2.3"))return 0;
    unsigned char *image=VirtualAlloc(NULL,0x1900000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    if(!image)return 0;
    uintptr_t base=(uintptr_t)image;
    const unsigned char getter[]={0x48,0x8d,0x05,0xd9,0x43,0x6e,0x01,0xc3};
    const unsigned char consumer[]={0xe8,0xd6,0x6f,0x81,0xff};
    const unsigned char parse[]={0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x6c,0x24,0x18,0x57,0x48,0x83,0xec,0x20};
    const unsigned char port[]={0x41,0x0f,0xb7,0x4f,0x50},bind[]={0xe8,0x72,0x9e,0x00,0x00};
    memcpy(image+0x1196e0,getter,sizeof getter);memcpy(image+0x902705,consumer,sizeof consumer);
    memcpy(image+0x113d80,parse,sizeof parse);memcpy(image+0x9033ae,port,sizeof port);
    memcpy(image+0x9033e9,bind,sizeof bind);
    wcscpy((wchar_t*)(image+0x17fdaa0),good);
    int ok=runtime_transport_bind_argument(base,L"26.1.2.3")&&!runtime_transport_bind_argument(1,L"26.1.2.3");
    image[0x1196e0]^=1;ok=ok&&!runtime_transport_bind_argument(base,L"26.1.2.3");image[0x1196e0]^=1;
    for(unsigned i=0;i<8192;++i)((wchar_t*)(image+0x17fdaa0))[i]=L'A';
    ok=ok&&!runtime_transport_bind_argument(base,L"26.1.2.3");
    uintptr_t objects[]={base+0x1000,base+0x2000,base+0x3000};
    unsigned char names[384]={0};const char *labels[]={"Driver","TcpNetDriver","Class"};
    for(int i=0;i<3;++i) {
        memcpy((void*)(objects[i]+8),&i,4);unsigned name=i*128;
        memcpy((void*)(objects[i]+0x48),&name,4);strcpy((char*)names+name+12,labels[i]);
        runtime_fixture_ptr(objects[i]+0x50,objects[i?2:1]);
    }
    GameArray array={(uintptr_t)objects,3,3};memcpy(image+OBJECTS_RVA,&array,sizeof array);
    runtime_fixture_ptr(base+NAME_POOLS_RVA,(uintptr_t)names);
    runtime_fixture_ptr(objects[0],base+0x12bc980);runtime_fixture_ptr(base+0x12bc980+0x2e0,base+0x903a80);
    unsigned char endpoint[16]={2,0,0x45,0x73,26,1,2,3};memcpy((void*)(objects[0]+0x1e4),endpoint,sizeof endpoint);
    Me3RuntimeConfig config={0};
    ok=ok&&runtime_config_parse(&config,L"1",L"host",L"26.1.2.3",NULL,L"17779")==ME3_CONFIG_READY&&
        runtime_transport_endpoint(base,objects[0],&config)&&!runtime_transport_endpoint(base,1,&config);
    /* Occupied-port fallback is not the requested host port. Client initialization
     * leaves zero in its cached address even after successful ephemeral bind. */
    ++endpoint[3];memcpy((void*)(objects[0]+0x1e4),endpoint,sizeof endpoint);
    ok=ok&&!runtime_transport_endpoint(base,objects[0],&config);
    config.role=ME3_ROLE_GUEST;ok=ok&&runtime_transport_endpoint(base,objects[0],&config);
    endpoint[2]=endpoint[3]=0;memcpy((void*)(objects[0]+0x1e4),endpoint,sizeof endpoint);
    ok=ok&&runtime_transport_endpoint(base,objects[0],&config);
    config.role=ME3_ROLE_HOST;ok=ok&&!runtime_transport_endpoint(base,objects[0],&config);
    config.role=ME3_ROLE_GUEST;
    endpoint[3]=1;endpoint[4]=27;ok=ok&&!runtime_transport_endpoint_bytes(endpoint,&config);
    endpoint[4]=26;endpoint[0]=23;ok=ok&&!runtime_transport_endpoint_bytes(endpoint,&config);
    endpoint[0]=2;memcpy((void*)(objects[0]+0x1e4),endpoint,sizeof endpoint);
    runtime_fixture_ptr(base+0x12bc980+0x2e0,base+0x903a81);
    ok=ok&&!runtime_transport_endpoint(base,objects[0],&config);
    runtime_fixture_ptr(base+0x12bc980+0x2e0,base+0x903a80);image[0x9033e9]^=1;
    ok=ok&&!runtime_transport_endpoint(base,objects[0],&config);
    VirtualFree(image,0,MEM_RELEASE);return ok;
}
#endif
