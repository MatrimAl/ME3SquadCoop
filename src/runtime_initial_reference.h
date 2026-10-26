#ifndef ME3_RUNTIME_INITIAL_REFERENCE_H
#define ME3_RUNTIME_INITIAL_REFERENCE_H
/* Reuses protected inventory_reference_trace's exact scalar ABI/pins, scoped
 * solely to the shared initial-object observer. No manual archive reads. */
typedef void (*RuntimeInitialBits)(void*,void*,int32_t);
typedef void (*RuntimeInitialIndex)(void*,uint32_t*,uint32_t);
static RuntimeInitialBits runtime_initial_bits_original;
static RuntimeInitialIndex runtime_initial_index_original;
static void runtime_initial_bits_forward(void *archive,void *dest,int32_t count,uintptr_t caller) {
    runtime_initial_bits_original(archive,dest,count);DWORD error=GetLastError();
    initial_object_bits_observe(archive,dest,count,caller);SetLastError(error);
}
static void runtime_initial_index_forward(void *archive,uint32_t *dest,uint32_t maximum,uintptr_t caller) {
    runtime_initial_index_original(archive,dest,maximum);DWORD error=GetLastError();
    initial_object_index_observe(archive,dest,maximum,caller);SetLastError(error);
}
static void runtime_initial_bits(void *archive,void *dest,int32_t count) {
    runtime_initial_bits_forward(archive,dest,count,(uintptr_t)__builtin_return_address(0));
}
static void runtime_initial_index(void *archive,uint32_t *dest,uint32_t maximum) {
    runtime_initial_index_forward(archive,dest,maximum,(uintptr_t)__builtin_return_address(0));
}
static void runtime_initial_reference_install(uintptr_t base) {
    if(runtime_initial_reference_installed||!initial_object_installed||!initial_object_opted()||
       GetCurrentThreadId()!=runtime_rpc_receive_thread)return;
    static const unsigned char bits_pin[16]={0x48,0x89,0x5c,0x24,8,0x48,0x89,0x74,0x24,0x10,0x57,0x48,0x83,0xec,0x30,0x41};
    static const unsigned char index_pin[16]={0x48,0x89,0x5c,0x24,0x10,0x57,0x45,0x33,0xc9,0x41,0xbb,1,0,0,0,0x44};
    unsigned char bytes[16];uintptr_t slots=base+0x119ddb8;DWORD old=0,ignored;
    if(!supported_image(base)||read_ptr(slots)!=base+0xc2b00||read_ptr(slots+8)!=base+0xc3060||
       !read_at(base+0xc2b00,bytes,16)||memcmp(bytes,bits_pin,16)||
       !read_at(base+0xc3060,bytes,16)||memcmp(bytes,index_pin,16)||
       !VirtualProtect((void*)slots,16,PAGE_READWRITE,&old))return;
    runtime_initial_bits_original=(RuntimeInitialBits)(base+0xc2b00);
    runtime_initial_index_original=(RuntimeInitialIndex)(base+0xc3060);
    PVOID first=InterlockedCompareExchangePointer((PVOID volatile*)slots,(PVOID)runtime_initial_bits,(PVOID)(base+0xc2b00));
    if((uintptr_t)first==base+0xc2b00) {
        PVOID second=InterlockedCompareExchangePointer((PVOID volatile*)(slots+8),(PVOID)runtime_initial_index,(PVOID)(base+0xc3060));
        if((uintptr_t)second==base+0xc3060)runtime_initial_reference_installed=1;
        else InterlockedCompareExchangePointer((PVOID volatile*)slots,(PVOID)(base+0xc2b00),(PVOID)runtime_initial_bits);
    }
    VirtualProtect((void*)slots,16,old,&ignored);
    if(runtime_initial_reference_installed)line("{\"runtime_initial_reference\":\"installed\",\"output_only\":true,\"archive_replayed\":false}\r\n");
}
#ifdef ME3_RUNTIME_FIXTURE
static unsigned runtime_initial_fixture_calls;
static void runtime_initial_fixture_bits(void *archive,void *dest,int32_t count) {
    (void)archive;(void)count;++runtime_initial_fixture_calls;*(unsigned char*)dest=1;SetLastError(1234);
}
static void runtime_initial_fixture_index(void *archive,uint32_t *dest,uint32_t max) {
    (void)archive;(void)max;++runtime_initial_fixture_calls;*dest=37;SetLastError(2345);
}
static int runtime_initial_reference_selftest(void) {
    RuntimeInitialBits bits=runtime_initial_bits_original;RuntimeInitialIndex index=runtime_initial_index_original;
    InitialObjectFrame *frame=initial_object_frame,*decoding=initial_object_decoding;
    uintptr_t base=runtime_rpc_receive_base;DWORD thread=runtime_rpc_receive_thread;int installed=initial_object_installed;
    unsigned char archive[16]={0},tag=0;uintptr_t fake_base=0x100000,vt=fake_base+0x119dda8;memcpy(archive,&vt,8);
    InitialObjectFrame active={0};active.active=1;active.archive=(uintptr_t)archive;
    initial_object_frame=initial_object_decoding=&active;initial_object_installed=1;
    runtime_rpc_receive_base=fake_base;runtime_rpc_receive_thread=GetCurrentThreadId();
    runtime_initial_bits_original=runtime_initial_fixture_bits;runtime_initial_index_original=runtime_initial_fixture_index;
    runtime_initial_fixture_calls=0;runtime_initial_bits_forward(archive,&tag,1,fake_base+0x602c40);
    int ok=tag==1&&active.tag_known&&active.tag==1&&runtime_initial_fixture_calls==1&&GetLastError()==1234;
    uint32_t value=0;runtime_initial_index_forward(archive,&value,0x3ff,fake_base+0x602c5d);
    ok=ok&&value==37&&active.index_known&&active.index_value==37&&runtime_initial_fixture_calls==2&&GetLastError()==2345;
    active.index_known=0;runtime_initial_index_forward(archive,&value,0x3ff,fake_base+0x602c5e);
    ok=ok&&!active.index_known&&runtime_initial_fixture_calls==3&&GetLastError()==2345;
    runtime_initial_bits_original=bits;runtime_initial_index_original=index;initial_object_frame=frame;initial_object_decoding=decoding;
    runtime_rpc_receive_base=base;runtime_rpc_receive_thread=thread;initial_object_installed=installed;return ok;
}
#endif
#endif
