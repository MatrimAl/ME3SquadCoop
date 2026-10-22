#ifndef ME3_RUNTIME_FINGERPRINT_H
#define ME3_RUNTIME_FINGERPRINT_H
#include <bcrypt.h>
/* Full on-disk SHA-256 is checked outside DllMain before installing hooks.
 * Open without FILE_SHARE_WRITE so another writer cannot change the bytes
 * during verification. The existing in-memory landmarks remain mandatory. */
static int runtime_hash_file_matches(const wchar_t *path,const unsigned char expected[32]) {
    HANDLE file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,NULL,
        OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);
    if(file==INVALID_HANDLE_VALUE)return 0;
    BCRYPT_ALG_HANDLE algorithm=NULL;BCRYPT_HASH_HANDLE hash=NULL;
    unsigned char actual[32],buffer[65536];DWORD got=0;int ok=0;
    if(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_SHA256_ALGORITHM,NULL,0)<0||
       BCryptCreateHash(algorithm,&hash,NULL,0,NULL,0,0)<0)goto done;
    for(;;) {
        if(!ReadFile(file,buffer,sizeof buffer,&got,NULL))goto done;
        if(!got)break;
        if(BCryptHashData(hash,buffer,got,0)<0)goto done;
    }
    ok=BCryptFinishHash(hash,actual,sizeof actual,0)>=0&&!memcmp(actual,expected,32);
done:
    if(hash)BCryptDestroyHash(hash);
    if(algorithm)BCryptCloseAlgorithmProvider(algorithm,0);
    CloseHandle(file);return ok;
}
static int runtime_executable_fingerprint(const wchar_t *path,uintptr_t base) {
    static const unsigned char expected[32]={
        0xb4,0x53,0x87,0x1e,0xb6,0xea,0xcf,0xf0,0xc6,0x46,0x49,0x0f,0xfb,0xc5,0x88,0xf7,
        0x43,0x39,0x9a,0x82,0x9b,0x1f,0x9e,0x31,0xcd,0x77,0x75,0xdc,0x95,0xda,0x80,0x01};
    return supported_image(base)&&runtime_hash_file_matches(path,expected);
}
#endif
