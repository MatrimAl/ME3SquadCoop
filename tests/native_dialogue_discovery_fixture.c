/* The actual registry validators/ownership/discovery bodies are inserted by
 * the runner. Readable fixture regions and native/type pin results stand in
 * for engine memory only. This program cannot open a process or invoke UI. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uintptr_t SIZE_T;
typedef struct GameArray {uintptr_t data;int32_t count,max;} GameArray;
static const uintptr_t base=UINT64_C(0x100000000),pc=24,world=32;
static unsigned char registry[0xd0],engine[0x4b0],player[0x70];
static unsigned char panel_a[0x330],panel_b[0x330],record_a[0x68],record_b[0x68];
static uintptr_t players[1],primary[2],secondary[2];
static int pins=1,method_pins=1,type_pins=1;
typedef struct Region {uintptr_t address;size_t size;} Region;
static Region regions[12];static unsigned region_count,checks;
static void region(void *address,size_t size) {regions[region_count++]=(Region){(uintptr_t)address,size};}
static int read_at(uintptr_t address,void *out,size_t size) {
    if(size==8&&address==base+0x18b6be0){uintptr_t p=(uintptr_t)registry;memcpy(out,&p,8);return 1;}
    if(size==8&&address==base+0x18b31d0){uintptr_t p=(uintptr_t)engine;memcpy(out,&p,8);return 1;}
    for(unsigned i=0;i<region_count;++i)if(address>=regions[i].address&&
        address-regions[i].address<=regions[i].size&&size<=regions[i].size-(address-regions[i].address)){
        memcpy(out,(void*)address,size);return 1;}
    return 0;
}
static uintptr_t read_ptr(uintptr_t address) {uintptr_t p=0;read_at(address,&p,8);return p;}
static int narrative_probe_object(uintptr_t image,uintptr_t object,const char *kind) {
    return type_pins&&image==base&&!strcmp(kind,"BioSFHandler_PCConversation")&&
        (object==(uintptr_t)panel_a||object==(uintptr_t)panel_b);
}
static int runtime_dialogue_object_pe(uintptr_t image,uintptr_t object) {
    return method_pins&&image==base&&(object==(uintptr_t)panel_a||object==(uintptr_t)panel_b);
}
static int runtime_power_wheel_registry_pins(uintptr_t image) {return pins&&image==base;}
/* PRODUCTION_HELPERS */
static void put_ptr(unsigned char *where,uintptr_t p) {memcpy(where,&p,8);}
static void put_array(unsigned offset,uintptr_t data,int count,int max) {
    GameArray a={data,count,max};memcpy(registry+offset,&a,sizeof a);
}
static void reset(void) {
    memset(registry,0,sizeof registry);memset(engine,0,sizeof engine);memset(player,0,sizeof player);
    memset(panel_a,0,sizeof panel_a);memset(panel_b,0,sizeof panel_b);
    memset(record_a,0,sizeof record_a);memset(record_b,0,sizeof record_b);
    pins=method_pins=type_pins=1;region_count=0;
    region(registry,sizeof registry);region(engine,sizeof engine);region(player,sizeof player);
    region(panel_a,sizeof panel_a);region(panel_b,sizeof panel_b);
    region(record_a,sizeof record_a);region(record_b,sizeof record_b);
    region(players,sizeof players);region(primary,sizeof primary);region(secondary,sizeof secondary);
    players[0]=(uintptr_t)player;put_ptr(player+0x68,pc);
    GameArray a={(uintptr_t)players,1,1};memcpy(engine+0x498,&a,sizeof a);
    unsigned char *panels[]={panel_a,panel_b};
    for(unsigned i=0;i<2;++i){put_ptr(panels[i],base+0x13ac170);
        put_ptr(panels[i]+0x2bc,world);put_ptr(panels[i]+0x318,(uintptr_t)panels[i]);}
    put_ptr(record_a+0x5c,(uintptr_t)panel_a);put_ptr(record_b+0x5c,(uintptr_t)panel_b);
    primary[0]=(uintptr_t)record_a;primary[1]=(uintptr_t)record_b;
    secondary[0]=(uintptr_t)record_a;secondary[1]=(uintptr_t)record_b;
    put_array(0x4c,(uintptr_t)primary,1,2);
}
#define CHECK(test) do{++checks;if(!(test)){fprintf(stderr,"line %d: %s\n",__LINE__,#test);return 1;}}while(0)
int main(void) {
    RuntimeDialogueRecord found={0};reset();
    /* Actual paired-run layout: primary contains our panel; secondary is the
     * ordinary canonical empty TArray. It must not discard the primary hit. */
    CHECK(runtime_dialogue_find_panel(base,pc,world,&found)==(uintptr_t)panel_a);
    CHECK(found.registry==(uintptr_t)registry&&found.list==(uintptr_t)registry+0x4c);
    CHECK(found.record==(uintptr_t)record_a&&found.index==0);
    put_array(0xbc,(uintptr_t)secondary,0,2);
    CHECK(runtime_dialogue_find_panel(base,pc,world,&found)==(uintptr_t)panel_a);
    /* Either list can be empty; an empty registry is valid but has no panel. */
    reset();put_array(0x4c,0,0,0);put_array(0xbc,(uintptr_t)secondary,1,2);
    CHECK(runtime_dialogue_find_panel(base,pc,world,&found)==(uintptr_t)panel_a);
    CHECK(found.list==(uintptr_t)registry+0xbc);
    put_array(0xbc,0,0,0);CHECK(!runtime_dialogue_find_panel(base,pc,world,&found));
    /* Header corruption must still refuse even after finding a valid panel. */
    const GameArray malformed[]={{0,1,1},{(uintptr_t)secondary,-1,2},
        {(uintptr_t)secondary,129,129},{(uintptr_t)secondary,2,1},
        {(uintptr_t)secondary,0,4097}};
    for(unsigned i=0;i<sizeof malformed/sizeof malformed[0];++i){reset();
        memcpy(registry+0xbc,malformed+i,sizeof malformed[i]);
        CHECK(!runtime_dialogue_find_panel(base,pc,world,&found));}
    /* Duplicate records for the same panel are acceptable; two distinct
     * matching panels are ambiguous and cannot be adopted. */
    reset();put_array(0xbc,(uintptr_t)secondary,1,2);
    CHECK(runtime_dialogue_find_panel(base,pc,world,&found)==(uintptr_t)panel_a);
    put_array(0xbc,(uintptr_t)(secondary+1),1,1);
    CHECK(!runtime_dialogue_find_panel(base,pc,world,&found));
    reset();put_array(0x4c,(uintptr_t)primary,2,2);
    CHECK(!runtime_dialogue_find_panel(base,pc,world,&found));
    /* Preserve exact image/type/native-entry and current PC/world/self gates. */
    reset();pins=0;CHECK(!runtime_dialogue_find_panel(base,pc,world,&found));
    reset();method_pins=0;CHECK(!runtime_dialogue_find_panel(base,pc,world,&found));
    reset();type_pins=0;CHECK(!runtime_dialogue_find_panel(base,pc,world,&found));
    reset();put_ptr(panel_a,base+0x13ac178);CHECK(!runtime_dialogue_find_panel(base,pc,world,&found));
    reset();put_ptr(panel_a+0x2bc,world+8);CHECK(!runtime_dialogue_find_panel(base,pc,world,&found));
    reset();put_ptr(panel_a+0x318,(uintptr_t)panel_b);CHECK(!runtime_dialogue_find_panel(base,pc,world,&found));
    reset();put_ptr(player+0x68,pc+8);CHECK(!runtime_dialogue_find_panel(base,pc,world,&found));
    reset();int32_t invalid_index=1;memcpy(panel_a+0x1b8,&invalid_index,4);
    CHECK(!runtime_dialogue_find_panel(base,pc,world,&found));
    reset();GameArray empty_players={0};memcpy(engine+0x498,&empty_players,sizeof empty_players);
    CHECK(!runtime_dialogue_find_panel(base,pc,world,&found));
    printf("PASS %u production registry-array/panel-discovery checks; no game launched\n",checks);
    return 0;
}
