#ifndef ME3_RUNTIME_STUNT_ACTOR_RETENTION_H
#define ME3_RUNTIME_STUNT_ACTOR_RETENTION_H
/* Exact client-level initialization prunes authored nonstatic stunt actors
 * before Matinee can bind them. Keep only those existing map actors at this
 * one callsite; normal initialization continues after the skipped destruction.
 * All other actors/callers retain original DestroyActor behavior. */
#include "../third_party/minhook/include/MinHook.h"
typedef uint64_t (*RuntimeStuntDestroy)(void*,void*,int,int);
static RuntimeStuntDestroy runtime_stunt_original;
static uintptr_t runtime_stunt_base;
static DWORD runtime_stunt_thread;
static int runtime_stunt_attempted,runtime_stunt_installed;
static unsigned runtime_stunt_rows;
static int runtime_stunt_case(int enabled,int guest,int thread,int caller,int arguments,int world,
        int authored,int stunt,uint32_t actor_flags,uint32_t pending,uint32_t script) {
    return enabled==1&&guest==1&&thread==1&&caller==1&&arguments==1&&world==1&&authored==1&&stunt==1&&
        !(actor_flags&13)&&!(pending&0x800000)&&!(script&0x400000);
}
static int runtime_stunt_level_member(uintptr_t base,uintptr_t world,uintptr_t info,uintptr_t level) {
    GameArray levels={0},streaming={0};int memberships=read_ptr(world+0x88)==level;
    if(!narrative_probe_object(base,level,"Level"))return 0;
    if(!scene_readiness_array(world+0x78,&levels,4096))return 0;
    for(int i=0;i<levels.count;++i)if(read_ptr(levels.data+(uintptr_t)i*8)==level)memberships=1;
    /* A level can be initialized before its visible flag is published. Its
     * loaded streaming object must still belong to this exact current world. */
    if(!scene_readiness_array(info+0x3e4,&streaming,4096))return 0;
    for(int i=0;i<streaming.count;++i) {
        uintptr_t entry=read_ptr(streaming.data+(uintptr_t)i*8);
        if(is_live(base,entry)&&read_ptr(entry+0xa0)==level)memberships=1;
    }
    return memberships;
}
static int runtime_stunt_keep(uintptr_t world,uintptr_t actor,int net_force,int modify,uintptr_t caller) {
    uintptr_t base=runtime_stunt_base;
    if(!runtime_control_enabled||g_runtime_config.role!=ME3_ROLE_GUEST||GetCurrentThreadId()!=runtime_stunt_thread||
       caller!=base+0x592ef3||net_force||modify!=1||read_ptr(base+0x18b6638)!=world||
       !narrative_probe_object(base,world,"World")||!is_live(base,actor)||!is_kind_of(base,actor,"SFXStuntActor"))return 0;
    uint32_t flags=0,pending=0,script=0;
    if(!read_at(actor+0x25c,&flags,4)||!read_at(actor+0x260,&pending,4)||!read_at(actor+0x264,&script,4)||
       !runtime_stunt_case(1,1,1,1,1,1,1,1,flags,pending,script))return 0;
    uintptr_t driver=read_ptr(world+0x120),connection=read_ptr(driver+0x80),persistent=read_ptr(world+0x88);
    GameArray actors={0};
    if(!narrative_probe_object(base,driver,"NetDriver")||!narrative_probe_object(base,connection,"NetConnection")||
       read_ptr(connection+0x8c)!=driver||!scene_readiness_array(persistent+0x60,&actors,100000)||!actors.count)return 0;
    uintptr_t info=read_ptr(actors.data),level=read_ptr(actor+0x40),klass=read_ptr(actor+0x50),archetype=read_ptr(actor+0x58);
    if(!narrative_probe_object(base,info,"WorldInfo")||(read_ptr(actor+0x198)&&read_ptr(actor+0x198)!=info)||
       !runtime_stunt_level_member(base,world,info,level)||!is_live(base,archetype)||
       !is_kind_of(base,archetype,"SFXStuntActor")||read_ptr(archetype+0x50)!=klass||
       !scene_readiness_array(level+0x60,&actors,100000))return 0;
    int found=0;
    for(int i=0;i<actors.count;++i)found+=read_ptr(actors.data+(uintptr_t)i*8)==actor;
    if(found!=1)return 0;
    GuestCameraStamp w,a,l,c,d,r;
    if(!guest_camera_stamp_read(base,world,&w,1)||!guest_camera_stamp_read(base,actor,&a,1)||
       !guest_camera_stamp_read(base,level,&l,1)||!guest_camera_stamp_read(base,connection,&c,1)||
       !guest_camera_stamp_read(base,driver,&d,1)||!guest_camera_stamp_read(base,archetype,&r,1)||
       !guest_camera_stamp_matches(base,&w,1)||!guest_camera_stamp_matches(base,&a,1)||
       !guest_camera_stamp_matches(base,&l,1)||!guest_camera_stamp_matches(base,&c,1)||
       !guest_camera_stamp_matches(base,&d,1)||!guest_camera_stamp_matches(base,&r,1)||
       read_ptr(base+0x18b6638)!=world||read_ptr(world+0x120)!=driver||read_ptr(driver+0x80)!=connection)return 0;
    return 1;
}
static uint64_t runtime_stunt_forward(void *world,void *actor,int net_force,int modify,uintptr_t caller) {
    DWORD error=GetLastError();int keep=runtime_stunt_keep((uintptr_t)world,(uintptr_t)actor,net_force,modify,caller);
    if(keep) {
        if(runtime_stunt_rows++<48) {
            char path[400],safe[800],row[1100];identity_path(runtime_stunt_base,(uintptr_t)actor,path,sizeof path);
            identity_quote(path,safe,sizeof safe);snprintf(row,sizeof row,
                "{\"runtime_stunt_retention\":\"client-level-prune-preserved\",\"actor\":\"%s\",\"normal_initialization_continues\":true,\"actor_flags_written\":false,\"monotonic_ms\":%lu}\r\n",safe,GetTickCount());line(row);
        }
        SetLastError(error);return 0;
    }
    SetLastError(error);return runtime_stunt_original(world,actor,net_force,modify);
}
static uint64_t runtime_stunt_destroy(void *world,void *actor,int net_force,int modify) {
    return runtime_stunt_forward(world,actor,net_force,modify,(uintptr_t)__builtin_return_address(0));
}
static void runtime_stunt_actor_prepare(uintptr_t base,uintptr_t engine) {
    (void)engine;
    if(runtime_stunt_attempted||!runtime_control_enabled||g_runtime_config.role!=ME3_ROLE_GUEST)return;
    runtime_stunt_attempted=1;
    static const unsigned char entry[]={0x48,0x8b,0xc4,0x44,0x89,0x48,0x20,0x48,0x89,0x48,0x08,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x8d};
    static const unsigned char call[]={0x45,0x85,0xff,0x75,0x65,0xf7,0x83,0x64,0x02,0,0,0,0,0x40,0,0x75,0x59,0xf6,0x83,0x5c,0x02,0,0,0x05,0x75,0x18,0x48,0x8b,0x0d,0x54,0x37,0x32,0x01,0x45,0x8d,0x4f,0x01,0x45,0x33,0xc0,0x48,0x8b,0xd3,0xe8,0x1d,0xcf,0xfe,0xff,0xeb,0x2e};
    unsigned char bytes[sizeof call];
    if(!supported_image(base)||!read_at(base+0x57fe10,bytes,sizeof entry)||memcmp(bytes,entry,sizeof entry)||
       !read_at(base+0x592ec3,bytes,sizeof call)||memcmp(bytes,call,sizeof call))goto refused;
    MH_STATUS status=MH_Initialize();if(status!=MH_OK&&status!=MH_ERROR_ALREADY_INITIALIZED)goto refused;
    LPVOID original=NULL;
    if(MH_CreateHook((LPVOID)(base+0x57fe10),(LPVOID)runtime_stunt_destroy,&original)!=MH_OK)goto refused;
    runtime_stunt_original=(RuntimeStuntDestroy)original;runtime_stunt_base=base;runtime_stunt_thread=GetCurrentThreadId();
    if(MH_EnableHook((LPVOID)(base+0x57fe10))!=MH_OK) {
        MH_RemoveHook((LPVOID)(base+0x57fe10));runtime_stunt_original=NULL;goto refused;
    }
    runtime_stunt_installed=1;line("{\"runtime_stunt_retention\":\"installed\",\"single_client_level_callsite\":true}\r\n");return;
refused:line("{\"runtime_stunt_retention\":\"native-pins-refused\"}\r\n");
}
#ifdef ME3_RUNTIME_FIXTURE
static int runtime_stunt_fixture_calls;
static uint64_t runtime_stunt_fixture_original(void *w,void *a,int n,int m) {
    if(w!=(void*)1||a!=(void*)2||n!=3||m!=4)return 0;
    ++runtime_stunt_fixture_calls;SetLastError(123);return UINT64_C(4294967297);
}
static int runtime_stunt_selftest(void) {
    if(!runtime_stunt_case(1,1,1,1,1,1,1,1,0,0,0))return 0;
    for(int i=0;i<8;++i){int g[8]={1,1,1,1,1,1,1,1};g[i]=0;
        if(runtime_stunt_case(g[0],g[1],g[2],g[3],g[4],g[5],g[6],g[7],0,0,0))return 0;}
    if(runtime_stunt_case(1,1,1,1,1,1,1,1,8,0,0)||runtime_stunt_case(1,1,1,1,1,1,1,1,4,0,0)||
       runtime_stunt_case(1,1,1,1,1,1,1,1,0,0x800000,0)||runtime_stunt_case(1,1,1,1,1,1,1,1,0,0,0x400000))return 0;
    RuntimeStuntDestroy original=runtime_stunt_original;runtime_stunt_original=runtime_stunt_fixture_original;
    runtime_stunt_fixture_calls=0;uint64_t result=runtime_stunt_forward((void*)1,(void*)2,3,4,0);
    int ok=result==UINT64_C(4294967297)&&runtime_stunt_fixture_calls==1&&GetLastError()==123;
    runtime_stunt_original=original;return ok;
}
#endif
#endif
