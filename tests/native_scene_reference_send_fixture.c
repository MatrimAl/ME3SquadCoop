/* Native identity/pin/discovery helpers below are controlled stand-ins. The
 * metadata/lease/capture/forward/preparation bodies are production source. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uintptr_t SIZE_T;
typedef struct GameArray {uintptr_t data;int32_t count,max;} GameArray;
typedef struct GuestCameraStamp {uintptr_t pointer,klass;int32_t index,class_index;uint32_t name[2],class_name[2];char full_name[600];} GuestCameraStamp;
typedef struct RuntimeSceneScope {GuestCameraStamp world,driver,connection;int valid;} RuntimeSceneScope;
typedef int (*RuntimeSceneReferenceNative)(void*,void*,void*,void*);
typedef struct NetworkActorIdentityEntry {uintptr_t base,pawn,channel,connection,driver;int index;} NetworkActorIdentityEntry;
typedef struct RuntimeSceneInterpLease {GuestCameraStamp action,world;int used;} RuntimeSceneInterpLease;
typedef struct RuntimeSceneInterp {RuntimeSceneInterpLease leases[64];RuntimeSceneScope capability;} RuntimeSceneInterp;
typedef struct SessionContext {int ignored;} SessionContext;
#include "send_types.h"
static RuntimeSceneReferenceSend runtime_scene_reference_send;
static RuntimeSceneInterp runtime_scene_interp;
enum {ME3_ROLE_HOST=1,ME3_ROLE_GUEST=2};
static struct {int role;}g_runtime_config;
static int runtime_control_enabled=1,runtime_scope_ready=1;
static DWORD session_game_thread;static uintptr_t session_engine;
static int (*runtime_scene_reference_host_forward)(void*,void*,void*,void*,uintptr_t,RuntimeSceneReferenceNative);
static unsigned char actor[0x290],action[0x200],sequence[0x80],package[0x120],map[0x180],archive[0x80],channel[0xc0],writer[0x20];
static const uintptr_t base=UINT64_C(0x100000000),world=0x90,driver=0x98,connection=0xa0;
static int property_ok=1,capability_ok=1,peer_ok=1,channel_ok=1,actor_ok=1,lease_ok=1,scope_ok=1,slot_ok=1,package_ok=1;
static unsigned installs,emits,stock_calls,checks,outer_reads,property_checks;static int mode,failed,native_result=-7,fail_input,fail_outer_post;
static char last_row[6000];
static int read_at(uintptr_t p,void *out,SIZE_T n) {
    if(p==(uintptr_t)actor+0x270&&fail_input)return 0;
    if(p==(uintptr_t)package+0x40&&fail_outer_post&&++outer_reads>1)return 0;
    if(p==0x1234&&n==8){uintptr_t v=UINT64_C(0x100000000)+0x1077d88;memcpy(out,&v,8);return 1;}
    if(p==0x1234+0x94&&n==4){int32_t v=0x270;memcpy(out,&v,4);return 1;}
    SIZE_T got=0;return p&&ReadProcessMemory(GetCurrentProcess(),(void*)p,out,n,&got)&&got==n;}
static uintptr_t read_ptr(uintptr_t p){uintptr_t value=0;read_at(p,&value,8);return value;}
static void putptr(unsigned char *p,uintptr_t value){memcpy(p,&value,8);}
static void stamp(uintptr_t p,GuestCameraStamp *s){memset(s,0,sizeof *s);s->pointer=p;s->klass=p+1;s->index=(int)(p&0xffff);s->class_index=3;s->name[0]=4;s->name[1]=9;strcpy(s->full_name,p==(uintptr_t)package?"localized_package":"localized_package.Line.Action");}
static int guest_camera_stamp_read(uintptr_t image,uintptr_t p,GuestCameraStamp *s,int full){(void)full;if(image!=base||!p)return 0;stamp(p,s);return 1;}
static int guest_camera_stamp_core_equal(const GuestCameraStamp *a,const GuestCameraStamp *b){return a->pointer&&a->pointer==b->pointer&&a->klass==b->klass&&a->index==b->index&&!memcmp(a->name,b->name,8);}
static int guest_camera_stamp_matches(uintptr_t image,const GuestCameraStamp *s,int full){(void)image;(void)full;return s->pointer!=0;}
static int narrative_probe_object(uintptr_t image,uintptr_t p,const char *kind){return image==base&&p==(uintptr_t)package&&!strcmp(kind,"Package")&&package_ok;}
static int is_kind_of(uintptr_t image,uintptr_t p,const char *kind){return image==base&&p==(uintptr_t)actor&&!strcmp(kind,"MatineeActor");}
static int guest_camera_actor_live(uintptr_t image,uintptr_t p){return image==base&&p==(uintptr_t)actor&&actor_ok;}
static int runtime_scene_reference_property(uintptr_t image,uintptr_t p){++property_checks;return image==base&&p==0x1234&&property_ok;}
static SessionContext session_context(uintptr_t engine){(void)engine;return(SessionContext){0};}
static int runtime_scene_host_peer(uintptr_t image,SessionContext context,RuntimeSceneScope *s){(void)context;if(image!=base||!peer_ok)return 0;memset(s,0,sizeof *s);stamp(world,&s->world);stamp(driver,&s->driver);stamp(connection,&s->connection);s->valid=1;return 1;}
static int runtime_scene_scope_live(uintptr_t image,const RuntimeSceneScope *s){return image==base&&s->valid&&scope_ok;}
static int runtime_scene_interp_capability_live(DWORD now){(void)now;return capability_ok;}
static int runtime_scene_interp_lease_live(const RuntimeSceneInterpLease *l){return l->used&&lease_ok;}
static int runtime_scene_interp_action(uintptr_t image,uintptr_t p,uintptr_t w,GuestCameraStamp *s,uintptr_t *a,uintptr_t *b){(void)a;(void)b;if(image!=base||p!=(uintptr_t)action||w!=world)return 0;stamp(p,s);return 1;}
static int network_actor_scan_driver(uintptr_t image,uintptr_t a,uintptr_t d,NetworkActorIdentityEntry *entry){if(image!=base||a!=(uintptr_t)actor||d!=driver||!channel_ok)return -1;*entry=(NetworkActorIdentityEntry){base,a,(uintptr_t)channel,connection,driver,7};return 7;}
static int network_actor_channel_valid(uintptr_t image,uintptr_t a,uintptr_t ch,uintptr_t d,NetworkActorIdentityEntry *entry){return ch==(uintptr_t)channel&&network_actor_scan_driver(image,a,d,entry)>=0;}
static int runtime_scene_reference_slot_install(uintptr_t image){++installs;return image==base&&slot_ok;}
static void line(const char *text){++emits;snprintf(last_row,sizeof last_row,"%s",text);SetLastError(999);}
#include "send_helpers.h"
static void reset(void){
    memset(actor,0,sizeof actor);memset(action,0,sizeof action);memset(sequence,0,sizeof sequence);memset(package,0,sizeof package);
    memset(map,0,sizeof map);memset(archive,0,sizeof archive);memset(channel,0,sizeof channel);
    memset(&runtime_scene_reference_send,0,sizeof runtime_scene_reference_send);memset(&runtime_scene_interp,0,sizeof runtime_scene_interp);
    runtime_scene_reference_send.base=base;runtime_scene_reference_send.installed=1;runtime_scene_reference_send.depth=1;
    session_game_thread=GetCurrentThreadId();g_runtime_config.role=ME3_ROLE_HOST;runtime_control_enabled=runtime_scope_ready=1;
    property_ok=capability_ok=peer_ok=channel_ok=actor_ok=lease_ok=scope_ok=slot_ok=package_ok=1;
    putptr(actor+0x270,(uintptr_t)action);putptr(actor+0x198,world);putptr(map,base+0x11a0d20);putptr(map+0x108,connection);
    putptr(archive,(uintptr_t)writer);putptr(writer+0x10,0x55);putptr(writer+0x18,0x66);
    putptr(action+0x40,(uintptr_t)sequence);putptr(sequence+0x40,(uintptr_t)package);putptr(action+0x15c,(uintptr_t)actor);
    int32_t netindex=123;memcpy(action+0x3c,&netindex,4);
    stamp((uintptr_t)action,&runtime_scene_interp.leases[0].action);stamp(world,&runtime_scene_interp.leases[0].world);
    runtime_scene_interp.leases[0].used=1;stamp(connection,&runtime_scene_interp.capability.connection);
    installs=emits=stock_calls=outer_reads=property_checks=0;failed=mode=fail_input=fail_outer_post=0;runtime_scene_reference_host_forward=NULL;
}
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
static int capture(RuntimeSceneReferenceSendFrame *f){return runtime_scene_reference_send_capture(f,0x1234,(uintptr_t)archive,(uintptr_t)map,(uintptr_t)actor+0x270,base+0x48d433);}
static int stock(void *p,void *a,void *m,void *d){
    if(p!=(void*)0x1234||a!=archive||m!=map||d!=actor+0x270||GetLastError()!=37)failed=1;++stock_calls;
    if(mode==1&&stock_calls==1){SetLastError(37);int r=runtime_scene_reference_send_forward(p,a,m,d,base+0x48d433,stock);if(r!=native_result||GetLastError()!=53)failed=1;}
    if(mode==2)scope_ok=0;
    SetLastError(53);return native_result;
}
int main(int argc,char **argv){
    reset();runtime_scene_reference_send.installed=0;
    if(argc!=2)return 1;
    runtime_scene_reference_send_prepare(base);
    if(!strcmp(argv[1],"disabled")){CHECK(!runtime_scene_reference_send.installed&&!installs&&!runtime_scene_reference_host_forward);printf("PASS default-off/explicit-off/invalid opt-in preparation\n");return 0;}
    CHECK(runtime_scene_reference_send.installed&&installs==1&&runtime_scene_reference_host_forward==runtime_scene_reference_send_forward);
    RuntimeSceneReferenceSendFrame f={0};reset();CHECK(capture(&f));CHECK(f.before.input_read&&f.before.netindex_read&&f.before.netindex==123);
    CHECK(f.before.package_complete&&f.before.depth==3&&f.before.package.pointer==(uintptr_t)package&&f.before.package.name[1]==9);
    RuntimeSceneReferenceSendMetadata current={0};runtime_scene_reference_send_metadata(base,(uintptr_t)map,(uintptr_t)actor+0x270,&current);
    CHECK(runtime_scene_reference_send_metadata_equal(&f.before,&current));
    int32_t generations[2]={100,4000};GameArray generation_header={(uintptr_t)generations,2,2};
    memcpy(package+0x94,&generation_header,sizeof generation_header);
    runtime_scene_reference_send_metadata(base,(uintptr_t)map,(uintptr_t)actor+0x270,&current);
    CHECK(current.generation_last_read&&current.generation_last==4000);
    generation_header.max=1;memcpy(package+0x94,&generation_header,sizeof generation_header);
    runtime_scene_reference_send_metadata(base,(uintptr_t)map,(uintptr_t)actor+0x270,&current);CHECK(!current.generation_last_read);
    generation_header=(GameArray){UINTPTR_MAX-3,2,2};memcpy(package+0x94,&generation_header,sizeof generation_header);
    runtime_scene_reference_send_metadata(base,(uintptr_t)map,(uintptr_t)actor+0x270,&current);CHECK(!current.generation_last_read);
    memset(package+0x94,0,sizeof generation_header);
    int32_t netindex=-1;memcpy(action+0x3c,&netindex,4);runtime_scene_reference_send_metadata(base,(uintptr_t)map,(uintptr_t)actor+0x270,&current);
    CHECK(current.netindex_read&&current.netindex==-1&&!runtime_scene_reference_send_metadata_equal(&f.before,&current));
    reset();putptr(actor+0x270,0);memset(&f,0,sizeof f);CHECK(capture(&f));CHECK(f.before.input_read&&!f.before.input&&!f.before.netindex_read&&f.before.stable);
    reset();runtime_scene_reference_send_metadata(base,(uintptr_t)map,1,&current);CHECK(!current.input_read&&!current.stable&&!current.netindex_read);
    reset();fail_input=1;memset(&f,0,sizeof f);CHECK(capture(&f));CHECK(!f.before.input_read&&!f.before.stable);
    reset();fail_outer_post=1;runtime_scene_reference_send_metadata(base,(uintptr_t)map,(uintptr_t)actor+0x270,&current);
    CHECK(current.package_complete&&!current.stable);
    reset();
    putptr(sequence+0x40,(uintptr_t)action);runtime_scene_reference_send_metadata(base,(uintptr_t)map,(uintptr_t)actor+0x270,&current);CHECK(!current.package_complete&&!current.stable);
    reset();package_ok=0;runtime_scene_reference_send_metadata(base,(uintptr_t)map,(uintptr_t)actor+0x270,&current);CHECK(!current.package_complete&&!current.stable);
    reset();putptr(actor+0x270,(uintptr_t)sequence);memset(&f,0,sizeof f);CHECK(!capture(&f));
    for(unsigned bad=0;bad<16;++bad){reset();memset(&f,0,sizeof f);
        switch(bad){case 0:runtime_scene_reference_send.installed=0;break;case 1:runtime_scene_reference_send.calls=64;break;
        case 2:runtime_scene_reference_send.depth=2;break;case 3:g_runtime_config.role=ME3_ROLE_GUEST;break;
        case 4:runtime_control_enabled=0;break;case 5:runtime_scope_ready=0;break;case 6:++session_game_thread;break;
        case 7:property_ok=0;break;case 8:putptr(map,0);break;case 9:archive[0x14]=1;break;
        case 10:capability_ok=0;break;case 11:peer_ok=0;break;case 12:putptr(map+0x108,0);break;
        case 13:actor_ok=0;break;case 14:channel_ok=0;break;case 15:lease_ok=0;break;}CHECK(!capture(&f));}
    reset();runtime_scene_interp.leases[1]=runtime_scene_interp.leases[0];memset(&f,0,sizeof f);CHECK(!capture(&f));
    reset();putptr(action+0x15c,0);memset(&f,0,sizeof f);CHECK(!capture(&f));
    reset();memset(&f,0,sizeof f);CHECK(!runtime_scene_reference_send_capture(&f,0x1234,(uintptr_t)archive,(uintptr_t)map,(uintptr_t)actor+0x270,base+0x48d434));
    const int returns[]={-7,0,1};
    for(unsigned returned=0;returned<3;++returned)for(int test=0;test<5;++test){reset();native_result=returns[returned];runtime_scene_reference_send.depth=0;mode=test==1?1:test==2?2:0;
        if(test==3)runtime_scene_reference_send.calls=64;if(test==4)++session_game_thread;
        SetLastError(37);int result=runtime_scene_reference_send_forward((void*)0x1234,archive,map,actor+0x270,base+0x48d433,stock);
        CHECK(result==native_result&&GetLastError()==53&&!failed&&stock_calls==(test==1?2u:1u));
        CHECK(runtime_scene_reference_send.depth==0&&emits==(test>=3?0u:1u));
        CHECK(property_checks==(test>=3?0u:1u));
        if(test==2)CHECK(strstr(last_row,"\"same_scope\":false")&&strstr(last_row,"\"metadata_stable\":false"));
        if(test==0)CHECK(strstr(last_row,"\"metadata_stable\":true")&&strstr(last_row,"\"package_lookup_performed\":false"));
    }
    for(unsigned bad=0;bad<5;++bad){reset();runtime_scene_reference_send.installed=0;
        switch(bad){case 0:g_runtime_config.role=ME3_ROLE_GUEST;break;case 1:runtime_control_enabled=0;break;case 2:runtime_scope_ready=0;break;case 3:++session_game_thread;break;case 4:slot_ok=0;break;}
        runtime_scene_reference_send_prepare(base);CHECK(!runtime_scene_reference_send.installed&&!runtime_scene_reference_host_forward);}
    reset();memset(&f,0,sizeof f);CHECK(capture(&f));
    for(unsigned i=0;i<599;++i)f.action.full_name[i]=f.before.package.full_name[i]=(i&1)?'"':'\\';
    f.action.full_name[599]=f.before.package.full_name[599]=0;
    runtime_scene_reference_send_emit(&f,7);CHECK(strlen(last_row)<4600&&strstr(last_row,"\"monotonic_ms\":"));
    printf("MAX_ROW %s",last_row);
    printf("PASS %u production host-send metadata/lease/capture/forward/preparation checks; no game\n",checks);return 0;
}
