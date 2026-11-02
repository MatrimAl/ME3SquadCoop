/* Execute actual guest precheck/order helpers. Slot and scalar installations
 * are explicit counters; no Windows page, property or archive is hooked. */
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef struct RuntimeSceneReferenceScope RuntimeSceneReferenceScope;
typedef int (*RuntimeSceneReferenceNative)(void*,void*,void*,void*);
#include "scene_prepare_types.h"
static RuntimeSceneReference runtime_scene_reference;
static const uintptr_t base=UINT64_C(0x100000000);
static const unsigned char expected[23]={0x49,0x8b,0,0x4d,0x8b,0xd0,0x4c,0x8b,0x81,0xd0,0,0,0,0x49,0x8b,0xca,0x48,0xff,0xa0,0xc8,2,0,0};
enum {ME3_ROLE_HOST=1,ME3_ROLE_GUEST=2};static struct {int role;}g_runtime_config;
static DWORD runtime_rpc_receive_thread;static int runtime_rpc_receive_installed,runtime_initial_reference_installed;
static int image_ok,slot_ok,body_read,body_ok,opted,scalar_ok;
static unsigned scalar_installs,slot_installs,rows,checks;
static int (*runtime_reference_extra_requested)(void);
static void (*runtime_reference_extra_bits)(void*,void*,int32_t,uintptr_t);
static void (*runtime_reference_extra_index)(void*,uint32_t*,uint32_t,uintptr_t);
static int supported_image(uintptr_t image){return image==base&&image_ok;}
static uintptr_t read_ptr(uintptr_t p){return p==base+0x1077d88+0x2f8&&slot_ok?base+0x146f60:0;}
static int read_at(uintptr_t p,void *out,size_t n){if(p!=base+0x146f60||n!=23||!body_read)return 0;memcpy(out,expected,23);if(!body_ok)((unsigned char*)out)[7]^=1;return 1;}
static int runtime_scene_reference_opted(void){return opted;}
static int runtime_scene_reference_requested(void){return 1;}
static void runtime_initial_reference_install(uintptr_t image){if(image==base){++scalar_installs;runtime_initial_reference_installed=scalar_ok;}}
static int runtime_scene_reference_slot_install(uintptr_t image){if(image!=base)return 0;++slot_installs;runtime_scene_reference.installed=1;return 1;}
static void runtime_scene_reference_bits(void*a,void*d,int32_t c,uintptr_t p){(void)a;(void)d;(void)c;(void)p;}
static void runtime_scene_reference_index(void*a,uint32_t*d,uint32_t c,uintptr_t p){(void)a;(void)d;(void)c;(void)p;}
static void line(const char *text){(void)text;++rows;}
#include "scene_prepare_helpers.h"
static void reset(void){memset(&runtime_scene_reference,0,sizeof runtime_scene_reference);
    runtime_rpc_receive_thread=GetCurrentThreadId();runtime_rpc_receive_installed=1;runtime_initial_reference_installed=0;
    g_runtime_config.role=ME3_ROLE_GUEST;image_ok=slot_ok=body_read=body_ok=opted=scalar_ok=1;
    scalar_installs=slot_installs=rows=0;runtime_reference_extra_requested=NULL;runtime_reference_extra_bits=NULL;runtime_reference_extra_index=NULL;}
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void){
    reset();runtime_scene_reference_prepare(base);CHECK(scalar_installs==1&&slot_installs==1&&rows==1);
    CHECK(runtime_scene_reference.installed&&runtime_reference_extra_bits&&runtime_reference_extra_index);
    for(unsigned bad=0;bad<8;++bad){reset();switch(bad){
        case 0:image_ok=0;break;case 1:slot_ok=0;break;case 2:body_read=0;break;case 3:body_ok=0;break;
        case 4:opted=0;break;case 5:g_runtime_config.role=ME3_ROLE_HOST;break;case 6:++runtime_rpc_receive_thread;break;
        case 7:runtime_rpc_receive_installed=0;break;}
        runtime_scene_reference_prepare(base);CHECK(!scalar_installs&&!slot_installs&&!rows&&!runtime_reference_extra_requested);}
    reset();scalar_ok=0;runtime_scene_reference_prepare(base);CHECK(scalar_installs==1&&!slot_installs&&!rows);
    printf("PASS %u production guest property precheck/installation-order checks; no hooks\n",checks);return 0;
}
