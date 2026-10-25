#ifndef ME3_RUNTIME_GUEST_POWER_H
#define ME3_RUNTIME_GUEST_POWER_H
/* Ordinary guest command queue. No synthetic input, forced unlock/cooldown,
 * supplied caster/target or replacement RPC. The native order chooses aim. */
#define RUNTIME_POWER_LIMIT 64
typedef struct RuntimePowerCatalog {
    SessionHandle pc,pawn,info,manager,channel,connection,driver;
    unsigned long registration_token;
    GameArray array;
    SessionHandle powers[RUNTIME_POWER_LIMIT];
    uint32_t names[RUNTIME_POWER_LIMIT][2];
} RuntimePowerCatalog;
static RuntimePowerCatalog runtime_power_catalog;
static volatile LONG runtime_power_scope,runtime_power_count,runtime_power_status;
static volatile LONG64 runtime_power_request;
static LONG runtime_power_generation;
static DWORD runtime_power_last_order;
enum {RUNTIME_POWER_IDLE=0,RUNTIME_POWER_QUEUED=1,RUNTIME_POWER_REJECTED=2,
    RUNTIME_POWER_NATIVE_ACCEPTED=3,RUNTIME_POWER_NATIVE_REFUSED=4};
static int runtime_power_same_handle(SessionHandle a,SessionHandle b) {
    return a.pointer&&a.pointer==b.pointer&&a.index==b.index&&a.name==b.name;
}
static int runtime_power_catalog_same(const RuntimePowerCatalog *a,const RuntimePowerCatalog *b) {
    if(!runtime_power_same_handle(a->pc,b->pc)||!runtime_power_same_handle(a->pawn,b->pawn)||
       !runtime_power_same_handle(a->info,b->info)||!runtime_power_same_handle(a->manager,b->manager)||
       !runtime_power_same_handle(a->channel,b->channel)||!runtime_power_same_handle(a->connection,b->connection)||
       !runtime_power_same_handle(a->driver,b->driver)||a->registration_token!=b->registration_token||
       memcmp(&a->array,&b->array,sizeof a->array))return 0;
    for(int i=0;i<a->array.count;++i)
        if(!runtime_power_same_handle(a->powers[i],b->powers[i])||memcmp(a->names[i],b->names[i],8))return 0;
    return 1;
}
static void runtime_power_invalidate(void) {
    InterlockedExchange(&runtime_power_scope,0);InterlockedExchange(&runtime_power_count,0);
    if(InterlockedExchange64(&runtime_power_request,0))InterlockedExchange(&runtime_power_status,RUNTIME_POWER_REJECTED);
    memset(&runtime_power_catalog,0,sizeof runtime_power_catalog);
}
/* Only atomics are touched by exported callers. A UI must retain the scope
 * returned with its list; an index alone cannot identify a power after travel. */
static int runtime_request_power(unsigned scope,unsigned index) {
    if(!scope||scope>0x7fffffff||index>=RUNTIME_POWER_LIMIT||
       !InterlockedCompareExchange(&g_enable_requested,0,0)||
       g_config_status!=ME3_CONFIG_READY||g_runtime_config.role!=ME3_ROLE_GUEST||
       InterlockedCompareExchange(&runtime_power_scope,0,0)!=(LONG)scope||
       index>=(unsigned)InterlockedCompareExchange(&runtime_power_count,0,0))return 0;
    LONG64 request=(LONG64)(((uint64_t)scope<<32)|(index+1));
    return InterlockedCompareExchange64(&runtime_power_request,request,0)==0;
}
static int runtime_power_read_catalog(uintptr_t base,SessionContext c,int ordinary,RuntimePowerCatalog *out) {
    if(!ordinary||!guest_player_context_registered_for(c.pc,c.pawn)||
       !guest_player_context_basic_owned(base,c.pc,c.pawn,c.world))return 0;
    int32_t action=-1,power=-1;unsigned char ignored=255;
    if(!read_at(c.pawn+0xc20,&action,4)||action||!read_at(c.pawn+0xc28,&power,4)||power||
       !read_at(c.pc+0x782,&ignored,1)||ignored)return 0;
    uintptr_t manager=0;GameArray array={0},after={0};NetworkActorIdentityEntry channel={0};
    if(!read_at(c.pawn+0xc18,&manager,8)||!narrative_probe_object(base,manager,"SFXPowerManager")||
       read_ptr(manager+0xd0)!=c.pawn||!identity_array(manager+0x60,&array,RUNTIME_POWER_LIMIT)||!array.count||
       !guest_player_context_channel(base,c.pawn,&channel))return 0;
    memset(out,0,sizeof *out);out->pc=session_capture_handle(c.pc);out->pawn=session_capture_handle(c.pawn);
    out->info=session_capture_handle(c.world);out->manager=session_capture_handle(manager);out->array=array;
    out->channel=session_capture_handle(channel.channel);out->connection=session_capture_handle(channel.connection);
    out->driver=session_capture_handle(channel.driver);out->registration_token=guest_player_context_registration_token();
    if(!session_handle_matches(out->pc)||!session_handle_matches(out->pawn)||
       !session_handle_matches(out->info)||!session_handle_matches(out->manager)||
       !session_handle_matches(out->channel)||!session_handle_matches(out->connection)||!session_handle_matches(out->driver))return 0;
    for(int i=0;i<array.count;++i) {
        uintptr_t item=0;
        if(!read_at(array.data+(size_t)i*8,&item,8)||!narrative_probe_object(base,item,"SFXPowerCustomActionBase")||
           !read_at(item+0x46c,out->names[i],8))return 0;
        out->powers[i]=session_capture_handle(item);if(!session_handle_matches(out->powers[i]))return 0;
    }
    return read_at(manager+0x60,&after,sizeof after)&&!memcmp(&array,&after,sizeof array)&&
        read_ptr(c.pawn+0xc18)==manager&&read_ptr(manager+0xd0)==c.pawn;
}
/* Reflected layout corroborates the retained SDK before invoking an order.
 * No property writes or method substitution are performed. */
static int runtime_power_property(uintptr_t base,uintptr_t object,const char *owner,const char *name,
    const char *kind,int offset,int size) {
    uintptr_t klass=read_ptr(object+0x50),seen[48]={0};
    for(unsigned n=0;klass&&n<48;++n) {
        if(!narrative_probe_object(base,klass,"Class"))return 0;
        for(unsigned j=0;j<n;++j)if(seen[j]==klass)return 0;seen[n]=klass;
        char text[96];object_name(base,klass,text,sizeof text);
        if(!strcmp(text,owner)) {
            uintptr_t field=read_ptr(klass+0x70);
            for(unsigned k=0;field&&k<1024;++k) {
                if(!is_live(base,field))return 0;object_name(base,field,text,sizeof text);
                if(!strcmp(text,name)) {
                    int32_t got=-1,bytes=0,dim=0;
                    return read_ptr(field+0x40)==klass&&narrative_probe_object(base,field,kind)&&
                        read_at(field+0x94,&got,4)&&got==offset&&read_at(field+0x74,&bytes,4)&&bytes==size&&
                        read_at(field+0x70,&dim,4)&&dim==1;
                }
                if(!read_at(field+0x68,&field,8))return 0;
            }
            return 0;
        }
        if(!read_at(klass+0x60,&klass,8))return 0;
    }
    return 0;
}
static void runtime_power_log(int status,const char *reason,unsigned index,DWORD now) {
    char row[420];snprintf(row,sizeof row,
        "{\"runtime_power\":\"order-result\",\"status\":%d,\"reason\":\"%s\",\"catalog_index\":%u,\"native_cast_verified\":false,\"target_effect_verified\":false,\"monotonic_ms\":%lu}\r\n",
        status,reason,index,now);line(row);InterlockedExchange(&runtime_power_status,status);
}
static void runtime_guest_power_tick(uintptr_t base,uintptr_t engine,SessionContext c,int ordinary,int after,DWORD now) {
    RuntimePowerCatalog current;
    if(!runtime_control_enabled||!runtime_scope_ready||!runtime_power_read_catalog(base,c,ordinary,&current)) {
        runtime_power_invalidate();return;
    }
    if(!runtime_power_catalog_same(&runtime_power_catalog,&current)) {
        runtime_power_invalidate();
        if(runtime_power_generation==0x7fffffff)return; /* Never reuse an external scope. */
        ++runtime_power_generation;runtime_power_catalog=current;
        InterlockedExchange(&runtime_power_count,current.array.count);
        InterlockedExchange(&runtime_power_scope,runtime_power_generation);
    }
    if(after)return;
    LONG64 request=InterlockedExchange64(&runtime_power_request,0);if(!request)return;
    unsigned scope=(unsigned)((uint64_t)request>>32),index=(unsigned)request-1;
    int status=RUNTIME_POWER_REJECTED;const char *reason="stale-scope-or-rate-limit";
    if(scope!=(unsigned)runtime_power_scope||index>=(unsigned)current.array.count||
       (runtime_power_last_order&&now-runtime_power_last_order<250))goto rejected;
    for(int i=0;i<current.array.count;++i)if((unsigned)i!=index&&
        !memcmp(current.names[index],current.names[i],8)) {
        reason="ambiguous-native-power-name";goto rejected;
    }
    uintptr_t item=current.powers[index].pointer,manager=current.manager.pointer;
    float rank=0;uint32_t name[2]={0};uintptr_t owner=0,pc=0,ai=1;
    if(!runtime_power_property(base,c.pawn,"BioPawn","PowerManager","ObjectProperty",0xc18,8)||
       !runtime_power_property(base,manager,"SFXPowerManager","Powers","ArrayProperty",0x60,16)||
       !runtime_power_property(base,manager,"SFXPowerManager","MyPawn","ObjectProperty",0xd0,8)||
       !runtime_power_property(base,item,"SFXPowerCustomActionBase","PowerName","NameProperty",0x46c,8)||
       !runtime_power_property(base,item,"SFXPowerCustomActionBase","Rank","FloatProperty",0x488,4)||
       !runtime_power_property(base,item,"BioCustomAction","m_oPawn","ObjectProperty",0x98,8)||
       !runtime_power_property(base,item,"BioCustomAction","m_oPC","ObjectProperty",0xa0,8)||
       !runtime_power_property(base,item,"BioCustomAction","m_oAI","ObjectProperty",0xa8,8)) {
        reason="power-layout-unverified";goto rejected;
    }
    if(!read_at(item+0x98,&owner,8)||owner!=c.pawn||!read_at(item+0xa0,&pc,8)||pc!=c.pc||
       !read_at(item+0xa8,&ai,8)||ai||!read_at(item+0x488,&rank,4)||!isfinite(rank)||rank<=0||
       !read_at(item+0x46c,name,8)||!name[0]||memcmp(name,current.names[index],8)) {
        reason="power-rank-or-cached-controller-not-ready";goto rejected;
    }
    uintptr_t method=guest_player_context_find_method(base,read_ptr(c.pc+0x50),"BioPlayerController","SquadOrderUsePower");
    NetworkActorIdentityEntry channel={0};CombatContextChain chain;
    RuntimePowerCatalog final;SessionContext checked=session_context(engine);uint32_t combat=2;
    if(!guest_power_order_contract(base,method)||
       !guest_player_context_conditions(base,c.pc,c.pawn,c.world,ordinary,1,1,&channel,&chain)||
       !runtime_power_read_catalog(base,checked,!session_engine_blocked(checked),&final)||
       !runtime_power_catalog_same(&current,&final)||
       !combat_context_get(base,read_ptr(c.pc+0x6dc),&combat)||combat!=1) {
        reason="native-order-or-current-control-not-ready";goto rejected;
    }
    uintptr_t pe=read_ptr(read_ptr(c.pc)+0x230);
    if(pe!=base+0x40d450){reason="native-dispatch-entry";goto rejected;}
    checked=session_context(engine);
    if(session_engine_blocked(checked)||!runtime_power_read_catalog(base,checked,1,&final)||
       !runtime_power_catalog_same(&current,&final)||
       !guest_player_context_conditions(base,checked.pc,checked.pawn,checked.world,1,1,1,&channel,&chain)||
       read_ptr(item+0x98)!=c.pawn||read_ptr(item+0xa0)!=c.pc||read_ptr(item+0xa8)||
       !guest_power_order_contract(base,method)||read_ptr(read_ptr(c.pc)+0x230)!=pe) {
        reason="context-changed-before-native-order";goto rejected;
    }
    GuestPowerOrderParms parms={0};memcpy(parms.power_name,name,8);parms.pawn=c.pawn;parms.result=UINT32_MAX;
    runtime_power_last_order=now;
    ((ProcessEvent)pe)((void*)c.pc,(void*)method,&parms,NULL);
    /* Do not dereference captured game objects after native execution. */
    status=parms.result==1?RUNTIME_POWER_NATIVE_ACCEPTED:parms.result==0?RUNTIME_POWER_NATIVE_REFUSED:RUNTIME_POWER_REJECTED;
    reason=parms.result==1?"native-tactical-order-accepted":parms.result==0?"native-tactical-order-refused":"native-return-unverified";
rejected:
    runtime_power_log(status,reason,index,now);
}
#endif
