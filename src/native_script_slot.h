#ifndef ME3_NATIVE_SCRIPT_SLOT_H
#define ME3_NATIVE_SCRIPT_SLOT_H
/* Read-only indexed UnrealScript lookup, not a C++ vtable. Exact supported
 * LE3 class helper at RVA cbb30 reads array144/count14c. The caller validates
 * its live class and expected function identity; no state/name fallback is
 * emulated here and no engine function is invoked. */
static int native_script_slot_matches(uintptr_t base,uintptr_t klass,unsigned slot,uintptr_t expected) {
    static const unsigned char pin[]={0x41,0x3b,0x9f,0x4c,0x01,0x00,0x00,0x7d,0x10,
        0x49,0x8b,0x87,0x44,0x01,0x00,0x00,0x48,0x8b,0x04,0xd8};
    unsigned char got[sizeof pin];GameArray first={0},last={0};uintptr_t function=0;
    return base&&klass&&expected&&slot<4096&&
        read_at(base+0xcbb77,got,sizeof got)&&!memcmp(got,pin,sizeof pin)&&
        read_at(klass+0x144,&first,sizeof first)&&first.data&&first.count>0&&
        first.max>=first.count&&first.max<=4096&&slot<(unsigned)first.count&&
        read_at(first.data+(SIZE_T)slot*8,&function,8)&&function==expected&&
        read_at(klass+0x144,&last,sizeof last)&&!memcmp(&first,&last,sizeof first);
}
static int native_script_slot_selftest(void) {
    unsigned char *image=VirtualAlloc(NULL,0xd0000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    if(!image)return 0;
    const unsigned char pin[]={0x41,0x3b,0x9f,0x4c,0x01,0x00,0x00,0x7d,0x10,
        0x49,0x8b,0x87,0x44,0x01,0x00,0x00,0x48,0x8b,0x04,0xd8};
    memcpy(image+0xcbb77,pin,sizeof pin);
    uintptr_t base=(uintptr_t)image,klass=base+0x1000,table[224]={0};
    uintptr_t expected=base+0x2000;table[223]=expected;
    GameArray a={(uintptr_t)table,224,224};memcpy((void*)(klass+0x144),&a,sizeof a);
    int ok=native_script_slot_matches(base,klass,223,expected)&&
        !native_script_slot_matches(base,klass,222,expected)&&
        !native_script_slot_matches(base,klass,224,expected)&&
        !native_script_slot_matches(base,klass,4096,expected)&&
        !native_script_slot_matches(base,klass,223,expected+8)&&
        !native_script_slot_matches(base,klass,223,0);
    a.count=223;memcpy((void*)(klass+0x144),&a,sizeof a);
    ok=ok&&!native_script_slot_matches(base,klass,223,expected);
    a.count=224;a.max=223;memcpy((void*)(klass+0x144),&a,sizeof a);
    ok=ok&&!native_script_slot_matches(base,klass,223,expected);
    a.max=4097;memcpy((void*)(klass+0x144),&a,sizeof a);
    ok=ok&&!native_script_slot_matches(base,klass,223,expected);
    a.max=224;memcpy((void*)(klass+0x144),&a,sizeof a);image[0xcbb77]^=1;
    ok=ok&&!native_script_slot_matches(base,klass,223,expected);
    VirtualFree(image,0,MEM_RELEASE);return ok;
}
#endif
