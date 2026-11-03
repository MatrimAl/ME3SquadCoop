#ifndef ME3_RUNTIME_IDENTITY_H
#define ME3_RUNTIME_IDENTITY_H
#include <windows.h>
#include <stdint.h>
#include <string.h>

/* Shared read-only primitives from the protected probe. This in-process
 * timestamp/landmark check is unchanged; it supplements, and does not replace,
 * the complete executable SHA-256 verified by the protected launchers. */
static int read_at(uintptr_t address,void *out,SIZE_T size) {
    SIZE_T got=0;
    return ReadProcessMemory(GetCurrentProcess(),(const void*)address,out,size,&got)&&got==size;
}
static uintptr_t read_ptr(uintptr_t address) {
    uintptr_t result=0;read_at(address,&result,sizeof result);return result;
}
static int supported_image(uintptr_t base) {
    WORD mz;DWORD pe_at=0,timestamp=0;unsigned char code[7];
    const unsigned char expect[7]={0x48,0x8d,0x15,0xe3,0xf9,0xbb,0x00};
    if(!read_at(base,&mz,2)||mz!=0x5a4d||!read_at(base+0x3c,&pe_at,4))return 0;
    if(!read_at(base+pe_at+8,&timestamp,4)||timestamp!=1622077636)return 0;
    return read_at(base+0x65fde6,code,sizeof code)&&!memcmp(code,expect,sizeof code);
}
static int me3_runtime_identity_selftest(void) {
    uint32_t value=0x13579,copy=0;
    if(!read_at((uintptr_t)&value,&copy,sizeof copy)||copy!=value||read_at(1,&copy,sizeof copy))return 0;
    const SIZE_T size=0x65fde6+7;unsigned char *image=VirtualAlloc(NULL,size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!image)return 0;
    WORD mz=0x5a4d;DWORD pe=0x80,timestamp=1622077636;
    const unsigned char code[7]={0x48,0x8d,0x15,0xe3,0xf9,0xbb,0x00};
    memcpy(image,&mz,2);memcpy(image+0x3c,&pe,4);memcpy(image+pe+8,&timestamp,4);memcpy(image+0x65fde6,code,7);
    int ok=supported_image((uintptr_t)image)&&!supported_image(1);
    image[0]^=1;ok=ok&&!supported_image((uintptr_t)image);image[0]^=1;
    image[pe+8]^=1;ok=ok&&!supported_image((uintptr_t)image);image[pe+8]^=1;
    image[0x65fde6]^=1;ok=ok&&!supported_image((uintptr_t)image);
    VirtualFree(image,0,MEM_RELEASE);return ok;
}
#endif
