#ifndef ME3_FUNCTION_LOOKUP_CACHE_H
#define ME3_FUNCTION_LOOKUP_CACHE_H
/* Game-thread-only discovery cache. Every hit revalidates GObjects membership
 * and the function, owner and Function-class identities. No UObject writes. */
typedef struct FunctionLookupStamp {
    uintptr_t object,klass,outer;
    uint64_t name;
    int32_t index;
} FunctionLookupStamp;
typedef struct FunctionLookupEntry {
    uintptr_t base;
    char owner[96],name[96];
    FunctionLookupStamp function,container,type;
} FunctionLookupEntry;
static FunctionLookupEntry function_lookup_entries[64];
static unsigned function_lookup_next;
static int function_lookup_stamp(uintptr_t base,uintptr_t object,FunctionLookupStamp *stamp) {
    unsigned char bytes[0x58];GameArray array={0};memset(stamp,0,sizeof *stamp);
    if(!object||!read_at(object,bytes,sizeof bytes)||!read_at(base+OBJECTS_RVA,&array,sizeof array)||
       !array.data||array.count<1||array.count>500000||array.max<array.count)return 0;
    memcpy(&stamp->index,bytes+8,4);
    if(stamp->index<0||stamp->index>=array.count||read_ptr(array.data+(SIZE_T)stamp->index*8)!=object)return 0;
    stamp->object=object;memcpy(&stamp->outer,bytes+0x40,8);memcpy(&stamp->name,bytes+0x48,8);
    memcpy(&stamp->klass,bytes+0x50,8);return 1;
}
static int function_lookup_valid(const FunctionLookupEntry *entry) {
    FunctionLookupStamp f,o,c;char text[96];
    if(!entry->base||!function_lookup_stamp(entry->base,entry->function.object,&f)||
       !function_lookup_stamp(entry->base,entry->container.object,&o)||
       !function_lookup_stamp(entry->base,entry->type.object,&c)||
       memcmp(&f,&entry->function,sizeof f)||memcmp(&o,&entry->container,sizeof o)||memcmp(&c,&entry->type,sizeof c)||
       f.outer!=o.object||f.klass!=c.object)return 0;
    object_name(entry->base,f.object,text,sizeof text);if(strcmp(text,entry->name))return 0;
    object_name(entry->base,o.object,text,sizeof text);if(strcmp(text,entry->owner))return 0;
    object_name(entry->base,c.object,text,sizeof text);return !strcmp(text,"Function");
}
static int function_lookup_capture(uintptr_t base,const char *owner,const char *name,uintptr_t function,FunctionLookupEntry *entry) {
    memset(entry,0,sizeof *entry);
    if(strlen(owner)>=sizeof entry->owner||strlen(name)>=sizeof entry->name||
       !function_lookup_stamp(base,function,&entry->function)||
       !function_lookup_stamp(base,entry->function.outer,&entry->container)||
       !function_lookup_stamp(base,entry->function.klass,&entry->type))return 0;
    entry->base=base;snprintf(entry->owner,sizeof entry->owner,"%s",owner);snprintf(entry->name,sizeof entry->name,"%s",name);
    return function_lookup_valid(entry);
}
static uintptr_t function_lookup_get(uintptr_t base,const char *owner,const char *name) {
    for(unsigned i=0;i<64;++i){FunctionLookupEntry *e=&function_lookup_entries[i];
        if(e->base!=base||strcmp(e->owner,owner)||strcmp(e->name,name))continue;
        if(function_lookup_valid(e))return e->function.object;
        memset(e,0,sizeof *e);return 0;
    }return 0;
}
static void function_lookup_put(uintptr_t base,const char *owner,const char *name,uintptr_t function) {
    FunctionLookupEntry entry;if(!function_lookup_capture(base,owner,name,function,&entry))return;
    function_lookup_entries[function_lookup_next++%64]=entry;
}
static int function_lookup_selftest(void) {
    unsigned char *base=VirtualAlloc(NULL,0x1900000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    if(!base)return 0;
    unsigned char objects[4][0x60]={{0}},pool[512]={0};uintptr_t pointers[4];
    const char *names[]={"FixtureFunction","FixtureOwner","Function","Class"};
    for(int i=0;i<4;++i){pointers[i]=(uintptr_t)objects[i];uint32_t name=(uint32_t)i*128;
        memcpy(objects[i]+8,&i,4);memcpy(objects[i]+0x48,&name,4);strcpy((char*)pool+i*128+12,names[i]);}
    for(int i=0;i<4;++i)memcpy(objects[i]+0x50,&pointers[i==0?2:3],8);
    memcpy(objects[0]+0x40,&pointers[1],8);
    GameArray array={(uintptr_t)pointers,4,4};uintptr_t pool_pointer=(uintptr_t)pool;
    memcpy(base+OBJECTS_RVA,&array,sizeof array);memcpy(base+NAME_POOLS_RVA,&pool_pointer,8);
    FunctionLookupEntry entry;int ok=function_lookup_capture((uintptr_t)base,names[1],names[0],pointers[0],&entry);
    /* Reject reuse even when the display-name string remains unchanged. */
    const int changes[][2]={{0,8},{0,0x40},{0,0x4c},{0,0x50},{1,8},{1,0x4c},{1,0x50},{2,8},{2,0x4c}};
    for(unsigned i=0;i<sizeof changes/sizeof *changes&&ok;++i){
        unsigned char *at=objects[changes[i][0]]+changes[i][1];*at^=1;
        ok=!function_lookup_valid(&entry);*at^=1;ok=ok&&function_lookup_valid(&entry);
    }
    uintptr_t saved=pointers[0];pointers[0]=0;ok=ok&&!function_lookup_valid(&entry);pointers[0]=saved;
    ok=ok&&function_lookup_valid(&entry);VirtualFree(base,0,MEM_RELEASE);return ok;
}
#endif
