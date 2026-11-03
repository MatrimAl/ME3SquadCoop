#ifndef ME3_PROBE_EXCEPTION_CONTEXT_H
#define ME3_PROBE_EXCEPTION_CONTEXT_H
/* Exception observation only: cached module bases, no loader/symbol lookup,
 * recovery, engine calls or exception suppression inside the handler. */
static int probe_exception_format(char *row,size_t capacity,uintptr_t fault,
    int query_valid,uintptr_t allocation,DWORD type,uintptr_t probe,uintptr_t game,
    int parameters_valid,ULONG_PTR access,ULONG_PTR address,DWORD now) {
    int image=query_valid&&type==MEM_IMAGE&&allocation&&fault>=allocation;
    int in_probe=image&&probe&&allocation==probe;
    int in_game=image&&game&&allocation==game;
    int n=snprintf(row,capacity,
        "{\"exception\":\"access_violation\",\"fault\":\"0x%llx\",\"module_rva\":\"0x%llx\",\"allocation_query_valid\":%s,\"allocation_base\":\"0x%llx\",\"allocation_type\":%lu,\"fault_in_probe\":%s,\"fault_in_game\":%s,\"access_parameters_valid\":%s,\"access_kind\":%llu,\"access_address\":\"0x%llx\",\"monotonic_ms\":%lu}\r\n",
        (unsigned long long)fault,(unsigned long long)(image?fault-allocation:0),
        query_valid?"true":"false",(unsigned long long)(query_valid?allocation:0),query_valid?type:0,
        in_probe?"true":"false",in_game?"true":"false",parameters_valid?"true":"false",
        (unsigned long long)(parameters_valid?access:0),(unsigned long long)(parameters_valid?address:0),now);
    return n>0&&(size_t)n<capacity;
}
static int probe_exception_context_selftest(void) {
    char row[768],small[32];
    if(!probe_exception_format(row,sizeof row,0x10c8fe,1,0x100000,MEM_IMAGE,0x100000,0x200000,1,0,0x298,123)||
       !strstr(row,"\"fault_in_probe\":true")||!strstr(row,"\"fault_in_game\":false")||
       !strstr(row,"\"module_rva\":\"0xc8fe\"")||!strstr(row,"\"monotonic_ms\":123"))return 0;
    if(!probe_exception_format(row,sizeof row,0x20c8fe,1,0x200000,MEM_IMAGE,0x100000,0x200000,1,0,0x298,123)||
       !strstr(row,"\"fault_in_game\":true")||!strstr(row,"\"fault_in_probe\":false"))return 0;
    for(int mode=0;mode<4;++mode) {
        int query=mode!=0;DWORD type=mode==1?MEM_PRIVATE:MEM_IMAGE;
        uintptr_t allocation=mode==2?0x300000:0x100000,probe=mode==3?0:0x100000;
        if(!probe_exception_format(row,sizeof row,0x10c8fe,query,allocation,type,probe,0x200000,0,1,0x298,123)||
           !strstr(row,"\"fault_in_probe\":false")||!strstr(row,"\"fault_in_game\":false")||
           !strstr(row,"\"access_parameters_valid\":false")||!strstr(row,"\"access_address\":\"0x0\""))return 0;
    }
    return !probe_exception_format(small,sizeof small,UINTPTR_MAX,1,1,MEM_IMAGE,1,2,1,UINTPTR_MAX,UINTPTR_MAX,UINT32_MAX);
}
#endif
