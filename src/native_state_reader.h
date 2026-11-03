#ifndef ME3_NATIVE_STATE_READER_H
#define ME3_NATIVE_STATE_READER_H
/* Read-only equivalent of the live-state path in exact-build execGetStateName
 * at RVA e4681. Intrinsic 284 cannot be queried with ordinary ProcessEvent.
 * Missing, stale or special native nodes remain unknown, never "None".
 */
static int native_state_fields(uintptr_t object, uintptr_t *node, uint32_t name[2]) {
    uintptr_t frame=0,again=0,check=0;int32_t index=-1;
    *node=0;name[0]=name[1]=UINT32_MAX;
    if(!object||!read_at(object+0x24,&frame,8)||!frame||
       !read_at(frame+0x48,node,8)||!*node||!read_at(*node+8,&index,4)||index<0||
       !read_at(*node+0x48,name,8)||name[0]==UINT32_MAX||name[1]>4096||
       !read_at(object+0x24,&again,8)||again!=frame||
       !read_at(frame+0x48,&check,8)||check!=*node)return 0;
    return 1;
}
static int native_state_name(uintptr_t base,uintptr_t object,char *out,size_t size) {
    static const unsigned char pin[]={
        0x48,0x8b,0x47,0x24,0x48,0x85,0xc0,0x74,0x4f,0x48,0x8b,0x40,0x48,0x48,0x85,0xc0,
        0x74,0x46,0x83,0x78,0x08,0xff,0x74,0x15,0x48,0x83,0xc0,0x48,0x48,0x8b,0x00,0x48,
        0x89,0x03,0x48,0x8b,0x5c,0x24,0x38,0x48,0x83,0xc4,0x20,0x5f,0xc3};
    unsigned char actual[sizeof pin];uintptr_t node=0;uint32_t name[2],again[2];
    if(!out||!size)return 0;out[0]=0;
    if(!read_at(base+0xe4681,actual,sizeof actual)||memcmp(actual,pin,sizeof pin)||
       !is_live(base,object)||!native_state_fields(object,&node,name)||
       !is_live(base,node)||!is_kind_of(base,node,"State")||
       !read_at(node+0x48,again,8)||memcmp(name,again,8))return 0;
    name_of(base,name[0],out,size);
    if(!out[0]||!strcmp(out,"<unreadable>")){out[0]=0;return 0;}
    return 1;
}
static int native_state_reader_selftest(void) {
    unsigned char object[0x30]={0},frame[0x50]={0},state[0x50]={0};
    uintptr_t fp=(uintptr_t)frame,sp=(uintptr_t)state,node=0;
    uint32_t name[2]={123,0},found[2];int32_t index=7;
    memcpy(object+0x24,&fp,8);memcpy(frame+0x48,&sp,8);
    memcpy(state+8,&index,4);memcpy(state+0x48,name,8);
    if(!native_state_fields((uintptr_t)object,&node,found)||node!=sp||memcmp(name,found,8))return 0;
    index=-1;memcpy(state+8,&index,4);
    if(native_state_fields((uintptr_t)object,&node,found))return 0;
    index=7;memcpy(state+8,&index,4);fp=1;memcpy(object+0x24,&fp,8);
    if(native_state_fields((uintptr_t)object,&node,found))return 0;
    fp=0;memcpy(object+0x24,&fp,8);
    return !native_state_fields((uintptr_t)object,&node,found);
}
#endif
