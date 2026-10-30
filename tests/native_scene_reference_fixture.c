/* Production forwarding/scalar helpers; scope/identity/native calls are
 * explicit stand-ins. All output storage belongs to this fixture. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
typedef struct GuestCameraStamp {uintptr_t pointer;int index;} GuestCameraStamp;
typedef struct NativeRpcReceiveFrame {uintptr_t channel;} NativeRpcReceiveFrame;
typedef int (*RuntimeSceneReferenceNative)(void*,void*,void*,void*);
typedef void (*RuntimeInitialBits)(void*,void*,int32_t);
typedef void (*RuntimeInitialIndex)(void*,uint32_t*,uint32_t);
#include "scene_reference_types.h"
static RuntimeSceneReference runtime_scene_reference;
static DWORD runtime_rpc_receive_thread;
static RuntimeInitialBits runtime_initial_bits_original;
static RuntimeInitialIndex runtime_initial_index_original;
static void (*runtime_reference_extra_bits)(void*,void*,int32_t,uintptr_t);
static void (*runtime_reference_extra_index)(void*,uint32_t*,uint32_t,uintptr_t);
static unsigned captures,stock_calls,emits,scalar_calls,initial_calls,extra_calls,checks;
static int enabled=1,mode,native_result=-7,failed;
static unsigned char tag;static uint32_t index_value;
static unsigned char property[16],nested_property[16],archive[16],map[16];static uintptr_t output;
static int read_at(uintptr_t p,void*out,SIZE_T size){SIZE_T got=0;return p&&ReadProcessMemory(GetCurrentProcess(),(void*)p,out,size,&got)&&got==size;}
static int runtime_scene_reference_current(RuntimeSceneReferenceScope*f,void*a){
    return f&&f==runtime_scene_reference.scope&&f->active&&f->archive==(uintptr_t)a;
}
static int runtime_scene_reference_capture(RuntimeSceneReferenceScope*f,uintptr_t p,uintptr_t a,uintptr_t m,uintptr_t caller){
    (void)m;(void)caller;++captures;SetLastError(888);
    if(!enabled||p!=(uintptr_t)property)return 0;f->active=1;f->archive=a;return 1;
}
static void runtime_scene_reference_emit(const RuntimeSceneReferenceScope*f,void*d,int result){
    if(f!=runtime_scene_reference.scope||!f->active||d!=&output||result!=native_result)failed=1;
    ++emits;SetLastError(999);
}
static void initial_object_bits_observe(void*a,void*d,int32_t count,uintptr_t caller){
    (void)a;(void)d;(void)count;(void)caller;++initial_calls;SetLastError(4321);
}
static void initial_object_index_observe(void*a,uint32_t*d,uint32_t maximum,uintptr_t caller){
    (void)a;(void)d;(void)maximum;(void)caller;++initial_calls;SetLastError(4321);
}
static int runtime_scene_reference_forward(void*,void*,void*,void*,uintptr_t);
static void runtime_scene_reference_bits(void*,void*,int32_t,uintptr_t);
static void runtime_scene_reference_index(void*,uint32_t*,uint32_t,uintptr_t);
static int stock(void*p,void*a,void*m,void*d){
    if(a!=archive||m!=map||d!=&output||GetLastError()!=37)failed=1;++stock_calls;
    if(p==property&&mode==1){
        unsigned before=stock_calls;SetLastError(37);
        int nested=runtime_scene_reference_forward(nested_property,a,m,d,0);
        if(nested!=native_result||stock_calls!=before+1||GetLastError()!=53)failed=1;
    }
    if(p==nested_property){tag=1;index_value=91;
        runtime_scene_reference_bits(a,&tag,1,runtime_scene_reference.base+0x602c40);
        runtime_scene_reference_index(a,&index_value,0x3ff,runtime_scene_reference.base+0x602c5d);
        if(runtime_scene_reference.scope->tag_known||runtime_scene_reference.scope->index_known)failed=1;
    }
    *(uintptr_t*)d=0x1234;SetLastError(53);return native_result;
}
static void scalar_bits(void*a,void*d,int32_t count){(void)a;(void)count;++scalar_calls;*(unsigned char*)d=1;SetLastError(1234);}
static void scalar_index(void*a,uint32_t*d,uint32_t maximum){(void)a;(void)maximum;++scalar_calls;*d=37;SetLastError(2345);}
static void extra_bits(void*a,void*d,int32_t count,uintptr_t caller){(void)a;(void)d;(void)count;(void)caller;++extra_calls;SetLastError(9876);}
static void extra_index(void*a,uint32_t*d,uint32_t maximum,uintptr_t caller){(void)a;(void)d;(void)maximum;(void)caller;++extra_calls;SetLastError(9876);}
#include "scene_reference_helpers.h"
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"FAIL line%d %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){
    runtime_rpc_receive_thread=GetCurrentThreadId();runtime_scene_reference.base=0x100000;runtime_scene_reference.original=stock;
    for(mode=0;mode<3;++mode){stock_calls=emits=captures=0;output=0;enabled=mode!=2;SetLastError(37);
        CHECK(runtime_scene_reference_forward(property,archive,map,&output,0)==native_result);
        CHECK(stock_calls==(mode==1?2u:1u)&&emits==(mode==2?0u:1u)&&!failed&&GetLastError()==53&&output==0x1234&&!runtime_scene_reference.scope);
    }
    RuntimeSceneReferenceScope f={0};f.active=1;f.archive=(uintptr_t)archive;runtime_scene_reference.scope=&f;
    tag=1;runtime_scene_reference_bits(archive,&tag,1,0x100000+0x602c40);CHECK(f.tag_known&&f.tag==1);
    index_value=37;runtime_scene_reference_index(archive,&index_value,0x3ff,0x100000+0x602c5d);CHECK(f.index_known&&f.index==37);
    f.tag_known=f.index_known=0;tag=0;runtime_scene_reference_bits(archive,&tag,1,0x100000+0x602c40);
    runtime_scene_reference_index(archive,&index_value,0x80000000,0x100000+0x602cc3);CHECK(f.tag_known&&f.tag==0&&f.index_known&&f.index==37);
    for(unsigned bad=0;bad<6;++bad){f.tag_known=f.index_known=0;f.active=bad!=0;tag=bad==5?2:1;
        runtime_scene_reference_bits(bad==1?map:archive,&tag,bad==2?2:1,0x100000+(bad==3?0x602c41:0x602c40));
        if(bad==4)f.tag_known=0;
        runtime_scene_reference_index(archive,&index_value,0x3ff,0x100000+0x602c5d);
        CHECK(!f.index_known);
    }
    f.active=1;f.tag_known=1;f.tag=1;f.index_known=0;
    runtime_scene_reference_index(archive,&index_value,0x80000000,0x100000+0x602cc3);CHECK(!f.index_known);
    runtime_scene_reference_index(archive,&index_value,0x3ff,0x100000+0x602c5e);CHECK(!f.index_known);
    runtime_scene_reference.scope=NULL;runtime_initial_bits_original=scalar_bits;runtime_initial_index_original=scalar_index;
    runtime_reference_extra_bits=extra_bits;runtime_reference_extra_index=extra_index;
    runtime_initial_bits_forward(archive,&tag,1,0);CHECK(scalar_calls==1&&initial_calls==1&&extra_calls==1&&tag==1&&GetLastError()==1234);
    runtime_initial_index_forward(archive,&index_value,0x3ff,0);CHECK(scalar_calls==2&&initial_calls==2&&extra_calls==2&&index_value==37&&GetLastError()==2345);
    runtime_reference_extra_bits=NULL;runtime_initial_bits_forward(archive,&tag,1,0);CHECK(scalar_calls==3&&extra_calls==2&&GetLastError()==1234);
    printf("PASS %u production scene-reference scope/scalar/forwarding checks; no game\n",checks);return 0;
}
