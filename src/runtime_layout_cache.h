#ifndef ME3_RUNTIME_LAYOUT_CACHE_H
#define ME3_RUNTIME_LAYOUT_CACHE_H
/* Positive reflected-layout cache for the pinned retail build. UClass
 * inheritance and interned FName text are immutable during each object's
 * lifetime; this is not an editor/hot-reload metadata cache. Every hit checks
 * GObjects membership, the full captured inheritance chain and its Super /
 * Children heads, field identity/type/owner/link/offset/size/dimension, and
 * the actual field-kind and Class metaclass identities. No game data changes.
 * Unrelated interior property-list rewiring is outside that retail contract;
 * ME3_SQUAD_COOP_LAYOUT_CACHE=0 retains the original full traversal.
 * Include after runtime_power_property_dim_uncached and RuntimeLayoutResolved. */
#define RUNTIME_LAYOUT_BUCKETS 64
#define RUNTIME_LAYOUT_WAYS 4
typedef struct RuntimeLayoutClassProof {
    FunctionLookupStamp identity;
    uintptr_t super,children;
} RuntimeLayoutClassProof;
typedef struct RuntimeLayoutFieldProof {
    FunctionLookupStamp identity;
    uintptr_t next;
    int32_t dimensions,element_size,offset;
} RuntimeLayoutFieldProof;
typedef struct RuntimeLayoutEntry {
    uintptr_t base;
    uint64_t hash;
    char owner[96],name[96],kind[96];
    int offset,size,dimensions;
    unsigned count;
    RuntimeLayoutClassProof classes[48],type,meta;
    RuntimeLayoutFieldProof field;
} RuntimeLayoutEntry;
typedef struct RuntimeLayoutProfile {
    uint64_t calls,total_ticks,max_ticks,hits,full_scans,full_ticks,full_max_ticks,failures;
    uint64_t captures,capture_ticks,invalidations,examined,emitted;
} RuntimeLayoutProfile;
static RuntimeLayoutEntry runtime_layout_entries[RUNTIME_LAYOUT_BUCKETS][RUNTIME_LAYOUT_WAYS];
static unsigned char runtime_layout_next[RUNTIME_LAYOUT_BUCKETS];
static RuntimeLayoutProfile runtime_layout_profiles[3];
static int runtime_layout_configured,runtime_layout_enabled;
static DWORD runtime_layout_started,runtime_layout_emitted;
static unsigned runtime_layout_windows;
static uint64_t runtime_layout_frequency;

static int runtime_layout_phase_policy(int enabled,DWORD elapsed) {
    return !enabled?2:elapsed<5000?0:1;
}
static void runtime_layout_configure(void) {
    if(runtime_layout_configured)return;
    wchar_t value[8]={0};LARGE_INTEGER frequency;
    DWORD length=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_LAYOUT_CACHE",value,8);
    runtime_layout_enabled=!(length==1&&value[0]==L'0');
    if(QueryPerformanceFrequency(&frequency)&&frequency.QuadPart>0)
        runtime_layout_frequency=(uint64_t)frequency.QuadPart;
    runtime_layout_started=runtime_layout_emitted=GetTickCount();runtime_layout_configured=1;
}
static uint64_t runtime_layout_clock(void) {
    LARGE_INTEGER value;
    return runtime_layout_frequency&&QueryPerformanceCounter(&value)?(uint64_t)value.QuadPart:0;
}
static uint64_t runtime_layout_elapsed(uint64_t start) {
    uint64_t end=runtime_layout_clock();return start&&end>=start?end-start:0;
}
static double runtime_layout_us(uint64_t ticks) {
    return runtime_layout_frequency?(double)ticks*1000000.0/(double)runtime_layout_frequency:0;
}
static int runtime_layout_objects(uintptr_t base,GameArray *objects) {
    return read_at(base+OBJECTS_RVA,objects,sizeof *objects)&&objects->data&&objects->count>0&&
        objects->count<=500000&&objects->max>=objects->count&&
        objects->data<=UINTPTR_MAX-(SIZE_T)(objects->count-1)*8;
}
static int runtime_layout_identity(uintptr_t object,const unsigned char *header,const GameArray *objects,FunctionLookupStamp *out) {
    memset(out,0,sizeof *out);memcpy(&out->index,header+8,4);
    if(out->index<0||out->index>=objects->count||read_ptr(objects->data+(SIZE_T)out->index*8)!=object)return 0;
    out->object=object;memcpy(&out->outer,header+0x40,8);memcpy(&out->name,header+0x48,8);
    memcpy(&out->klass,header+0x50,8);return out->klass!=0;
}
static int runtime_layout_class(uintptr_t object,const GameArray *objects,RuntimeLayoutClassProof *out) {
    unsigned char header[0x78];memset(out,0,sizeof *out);
    if(!object||!read_at(object,header,sizeof header)||!runtime_layout_identity(object,header,objects,&out->identity))return 0;
    memcpy(&out->super,header+0x60,8);memcpy(&out->children,header+0x70,8);return 1;
}
static int runtime_layout_field(uintptr_t object,const GameArray *objects,RuntimeLayoutFieldProof *out) {
    unsigned char header[0x98];memset(out,0,sizeof *out);
    if(!object||!read_at(object,header,sizeof header)||!runtime_layout_identity(object,header,objects,&out->identity))return 0;
    memcpy(&out->next,header+0x68,8);memcpy(&out->dimensions,header+0x70,4);
    memcpy(&out->element_size,header+0x74,4);memcpy(&out->offset,header+0x94,4);return 1;
}
static uint64_t runtime_layout_hash(uintptr_t klass,const char *owner,const char *name,const char *kind,int offset,int size,int dimensions) {
    uint64_t h=UINT64_C(14695981039346656037)^(uint64_t)klass;
    const char *strings[]={owner,name,kind};
    for(unsigned i=0;i<3;++i){for(const unsigned char *p=(const unsigned char*)strings[i];*p;++p)h=(h^*p)*UINT64_C(1099511628211);h=(h^255)*UINT64_C(1099511628211);}
    h=(h^(uint32_t)offset)*UINT64_C(1099511628211);h=(h^(uint32_t)size)*UINT64_C(1099511628211);
    return (h^(uint32_t)dimensions)*UINT64_C(1099511628211);
}
static int runtime_layout_key(const RuntimeLayoutEntry *e,uintptr_t base,uintptr_t klass,uint64_t hash,
        const char *owner,const char *name,const char *kind,int offset,int size,int dimensions) {
    return e->base==base&&e->count&&e->classes[0].identity.object==klass&&e->hash==hash&&
        e->offset==offset&&e->size==size&&e->dimensions==dimensions&&
        !strcmp(e->owner,owner)&&!strcmp(e->name,name)&&!strcmp(e->kind,kind);
}
static int runtime_layout_capture(uintptr_t base,uintptr_t object,const char *owner,const char *name,const char *kind,
        int offset,int size,int dimensions,const RuntimeLayoutResolved *resolved,RuntimeLayoutEntry *out) {
    GameArray objects={0};memset(out,0,sizeof *out);
    if(!resolved->field||!resolved->count||resolved->count>48||strlen(owner)>=96||strlen(name)>=96||strlen(kind)>=96||
       !runtime_layout_objects(base,&objects)||read_ptr(object+0x50)!=resolved->classes[0])return 0;
    for(unsigned i=0;i<resolved->count;++i) {
        if(!runtime_layout_class(resolved->classes[i],&objects,&out->classes[i])||
           (i&&out->classes[i-1].super!=resolved->classes[i]))return 0;
    }
    if(!runtime_layout_field(resolved->field,&objects,&out->field)||
       !runtime_layout_class(out->field.identity.klass,&objects,&out->type)||
       !runtime_layout_class(out->classes[0].identity.klass,&objects,&out->meta)||
       out->meta.identity.klass!=out->meta.identity.object||
       out->type.identity.klass!=out->meta.identity.object||
       out->field.identity.outer!=out->classes[resolved->count-1].identity.object||
       out->classes[resolved->count-1].children!=resolved->owner_children||
       out->field.offset!=offset||out->field.element_size!=size||out->field.dimensions!=dimensions)return 0;
    for(unsigned i=0;i<resolved->count;++i)if(out->classes[i].identity.klass!=out->meta.identity.object)return 0;
    char text[96];
    name_of(base,(uint32_t)out->type.identity.name,text,sizeof text);
    if(strcmp(text,kind))return 0; /* A derived property kind uses the original traversal instead. */
    name_of(base,(uint32_t)out->field.identity.name,text,sizeof text);if(strcmp(text,name))return 0;
    name_of(base,(uint32_t)out->classes[resolved->count-1].identity.name,text,sizeof text);if(strcmp(text,owner))return 0;
    out->base=base;out->count=resolved->count;out->offset=offset;out->size=size;out->dimensions=dimensions;
    snprintf(out->owner,sizeof out->owner,"%s",owner);snprintf(out->name,sizeof out->name,"%s",name);
    snprintf(out->kind,sizeof out->kind,"%s",kind);
    out->hash=runtime_layout_hash(resolved->classes[0],owner,name,kind,offset,size,dimensions);
    return 1;
}
static int runtime_layout_valid(const RuntimeLayoutEntry *e,uintptr_t object) {
    GameArray objects={0},after={0};RuntimeLayoutClassProof value;
    RuntimeLayoutFieldProof field;
    if(!e->base||!e->count||e->count>48||!runtime_layout_objects(e->base,&objects)||
       read_ptr(object+0x50)!=e->classes[0].identity.object)return 0;
    for(unsigned i=0;i<e->count;++i)if(!runtime_layout_class(e->classes[i].identity.object,&objects,&value)||
        memcmp(&value,&e->classes[i],sizeof value))return 0;
    if(!runtime_layout_field(e->field.identity.object,&objects,&field)||memcmp(&field,&e->field,sizeof field)||
       !runtime_layout_class(e->type.identity.object,&objects,&value)||memcmp(&value,&e->type,sizeof value)||
       !runtime_layout_class(e->meta.identity.object,&objects,&value)||memcmp(&value,&e->meta,sizeof value)||
       !runtime_layout_objects(e->base,&after)||after.data!=objects.data||after.count<objects.count||
       read_ptr(object+0x50)!=e->classes[0].identity.object)return 0;
    return 1;
}
static int runtime_layout_cache_property(uintptr_t base,uintptr_t object,const char *owner,const char *name,
        const char *kind,int offset,int size,int dimensions) {
    /* UI hooks and Tick share this game thread. Off-thread discovery retains
     * the original implementation without touching cache/profile state. */
    if(GetCurrentThreadId()!=session_game_thread)
        return runtime_power_property_dim_uncached(base,object,owner,name,kind,offset,size,dimensions,NULL);
    DWORD error=GetLastError();runtime_layout_configure();
    uint64_t started=runtime_layout_clock();
    unsigned phase=(unsigned)runtime_layout_phase_policy(runtime_layout_enabled,GetTickCount()-runtime_layout_started);
    RuntimeLayoutProfile *profile=&runtime_layout_profiles[phase];++profile->calls;
    uintptr_t klass=read_ptr(object+0x50);RuntimeLayoutEntry *entry=NULL,*destination=NULL;
    uint64_t hash=0;unsigned bucket=0;
    int cacheable=runtime_layout_enabled&&klass&&owner&&name&&kind&&strlen(owner)<96&&strlen(name)<96&&strlen(kind)<96;
    if(cacheable) {
        hash=runtime_layout_hash(klass,owner,name,kind,offset,size,dimensions);bucket=(unsigned)(hash&(RUNTIME_LAYOUT_BUCKETS-1));
        for(unsigned i=0;i<RUNTIME_LAYOUT_WAYS;++i) {
            RuntimeLayoutEntry *e=&runtime_layout_entries[bucket][i];
            if(runtime_layout_key(e,base,klass,hash,owner,name,kind,offset,size,dimensions)){entry=e;break;}
            if(!e->base)destination=e;
        }
    }
    if(phase==1&&entry) {
        if(runtime_layout_valid(entry,object)) {
            ++profile->hits;uint64_t ticks=runtime_layout_elapsed(started);profile->total_ticks+=ticks;
            if(ticks>profile->max_ticks)profile->max_ticks=ticks;
            SetLastError(error);return 1;
        }
        ++profile->invalidations;memset(entry,0,sizeof *entry);destination=entry;entry=NULL;
    }
    RuntimeLayoutResolved resolved;
    SetLastError(error);uint64_t full_started=runtime_layout_clock();
    int result=runtime_power_property_dim_uncached(base,object,owner,name,kind,offset,size,dimensions,&resolved);
    DWORD result_error=GetLastError();uint64_t full_ticks=runtime_layout_elapsed(full_started);
    ++profile->full_scans;profile->full_ticks+=full_ticks;profile->examined+=resolved.examined;
    if(full_ticks>profile->full_max_ticks)profile->full_max_ticks=full_ticks;
    if(!result)++profile->failures;
    if(result&&cacheable&&!entry) {
        RuntimeLayoutEntry captured;uint64_t capture_started=runtime_layout_clock();
        int valid=runtime_layout_capture(base,object,owner,name,kind,offset,size,dimensions,&resolved,&captured);
        ++profile->captures;profile->capture_ticks+=runtime_layout_elapsed(capture_started);
        if(valid) {
            if(!destination)destination=&runtime_layout_entries[bucket][runtime_layout_next[bucket]++%RUNTIME_LAYOUT_WAYS];
            *destination=captured;
        }
    }
    uint64_t ticks=runtime_layout_elapsed(started);profile->total_ticks+=ticks;
    if(ticks>profile->max_ticks)profile->max_ticks=ticks;
    SetLastError(result_error);return result;
}
static void runtime_layout_cache_flush(DWORD now) {
    if(!runtime_layout_configured||GetCurrentThreadId()!=session_game_thread||runtime_layout_windows>=128||
       now-runtime_layout_emitted<5000)return;
    DWORD error=GetLastError();runtime_layout_emitted=now;++runtime_layout_windows;
    static const char *names[]={"baseline-fullscan","cache","disabled-fullscan"};
    for(unsigned i=0;i<3;++i) {
        RuntimeLayoutProfile *p=&runtime_layout_profiles[i];if(!p->calls||p->calls==p->emitted)continue;
        char row[1400];
        snprintf(row,sizeof row,"{\"runtime_layout_profile\":\"%s\",\"calls\":%llu,\"total_us\":%.3f,\"max_us\":%.3f,\"cache_hits\":%llu,\"full_scans\":%llu,\"fullscan_avg_us\":%.3f,\"fullscan_max_us\":%.3f,\"fullscan_failed\":%llu,\"fields_examined\":%llu,\"captures\":%llu,\"capture_total_us\":%.3f,\"invalidations\":%llu,\"timing_valid\":%s,\"gpu_timing_measured\":false,\"gameplay_written\":false,\"monotonic_ms\":%lu}\r\n",
            names[i],(unsigned long long)p->calls,runtime_layout_us(p->total_ticks),runtime_layout_us(p->max_ticks),
            (unsigned long long)p->hits,(unsigned long long)p->full_scans,
            p->full_scans?runtime_layout_us(p->full_ticks)/(double)p->full_scans:0,runtime_layout_us(p->full_max_ticks),
            (unsigned long long)p->failures,(unsigned long long)p->examined,(unsigned long long)p->captures,
            runtime_layout_us(p->capture_ticks),(unsigned long long)p->invalidations,runtime_layout_frequency?"true":"false",now);
        line(row);p->emitted=p->calls;
    }
    SetLastError(error);
}
static int runtime_layout_cache_selftest(void) {
    if(runtime_layout_phase_policy(1,4999)!=0||runtime_layout_phase_policy(1,5000)!=1||
       runtime_layout_phase_policy(0,0)!=2||runtime_layout_phase_policy(0,5000)!=2)return 0;
    unsigned char *base=VirtualAlloc(NULL,0x1900000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    if(!base)return 0;
    unsigned char raw[8][0xa0]={{0}},pool[1024]={0};uintptr_t pointers[8];
    const char *names[]={"FixtureConcrete","FixtureOwner","FixtureField","FloatProperty","Class","FixtureObject","IntProperty","OtherClass"};
    for(int i=0;i<8;++i){pointers[i]=(uintptr_t)raw[i];uint32_t n=(uint32_t)i*128;
        memcpy(raw[i]+8,&i,4);memcpy(raw[i]+0x48,&n,4);strcpy((char*)pool+n+12,names[i]);}
    for(int i=0;i<8;++i)memcpy(raw[i]+0x50,&pointers[i==2?3:i==5?0:4],8);
    memcpy(raw[0]+0x60,&pointers[1],8);memcpy(raw[1]+0x70,&pointers[2],8);memcpy(raw[2]+0x40,&pointers[1],8);
    int32_t dim=1,size=4,offset=0x488;
    memcpy(raw[2]+0x70,&dim,4);memcpy(raw[2]+0x74,&size,4);memcpy(raw[2]+0x94,&offset,4);
    GameArray objects={(uintptr_t)pointers,8,8};uintptr_t name_pool=(uintptr_t)pool;
    memcpy(base+OBJECTS_RVA,&objects,sizeof objects);memcpy(base+NAME_POOLS_RVA,&name_pool,8);
    RuntimeLayoutResolved resolved;RuntimeLayoutEntry entry;
    int ok=runtime_power_property_dim_uncached((uintptr_t)base,pointers[5],names[1],names[2],names[3],offset,size,dim,&resolved)&&
        runtime_layout_capture((uintptr_t)base,pointers[5],names[1],names[2],names[3],offset,size,dim,&resolved,&entry)&&
        runtime_layout_valid(&entry,pointers[5]);
    const unsigned changes[][2]={{0,8},{0,0x48},{0,0x50},{0,0x60},{0,0x70},
        {1,8},{1,0x48},{1,0x50},{1,0x60},{1,0x70},{2,8},{2,0x40},{2,0x48},{2,0x50},
        {2,0x68},{2,0x70},{2,0x74},{2,0x94},{3,8},{3,0x48},{3,0x50},{3,0x60},{3,0x70},{4,8},{4,0x48},{4,0x50},{5,0x50}};
    for(unsigned i=0;i<sizeof changes/sizeof *changes&&ok;++i) {
        unsigned char *at=raw[changes[i][0]]+changes[i][1];*at^=1;
        ok=!runtime_layout_valid(&entry,pointers[5]);*at^=1;ok=ok&&runtime_layout_valid(&entry,pointers[5]);
    }
    uintptr_t saved=pointers[2];pointers[2]=0;ok=ok&&!runtime_layout_valid(&entry,pointers[5]);pointers[2]=saved;
    ok=ok&&runtime_layout_valid(&entry,pointers[5])&&
        runtime_layout_key(&entry,(uintptr_t)base,pointers[0],entry.hash,names[1],names[2],names[3],offset,size,dim)&&
        !runtime_layout_key(&entry,(uintptr_t)base,pointers[1],entry.hash,names[1],names[2],names[3],offset,size,dim)&&
        !runtime_layout_key(&entry,(uintptr_t)base,pointers[0],entry.hash,names[1],names[2],names[3],offset+1,size,dim)&&
        !runtime_layout_key(&entry,(uintptr_t)base,pointers[0],entry.hash,names[1],names[2],names[3],offset,size+1,dim)&&
        !runtime_layout_key(&entry,(uintptr_t)base,pointers[0],entry.hash,names[1],names[2],names[3],offset,size,dim+1);
    RuntimeLayoutEntry refused;RuntimeLayoutResolved absent={0};
    ok=ok&&!runtime_layout_capture((uintptr_t)base,pointers[5],names[1],names[2],names[3],offset,size,dim,&absent,&refused);
    /* Derived property kinds retain the original is_kind_of semantics via a miss. */
    memcpy(raw[2]+0x50,&pointers[6],8);memcpy(raw[6]+0x60,&pointers[3],8);
    ok=ok&&runtime_power_property_dim_uncached((uintptr_t)base,pointers[5],names[1],names[2],names[3],offset,size,dim,&resolved)&&
        !runtime_layout_capture((uintptr_t)base,pointers[5],names[1],names[2],names[3],offset,size,dim,&resolved,&refused);
    VirtualFree(base,0,MEM_RELEASE);return ok;
}
#endif
