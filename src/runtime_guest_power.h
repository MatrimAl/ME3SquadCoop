#ifndef ME3_RUNTIME_GUEST_POWER_H
#define ME3_RUNTIME_GUEST_POWER_H
/* Ordinary guest command queue. No synthetic input, forced unlock/cooldown,
 * supplied caster/target or replacement RPC. The native order chooses aim. */
#define RUNTIME_POWER_LIMIT 64
typedef struct RuntimePowerCatalog {
    SessionHandle pc,pawn,info,input,manager,channel,connection,driver;
    unsigned long registration_token;
    GameArray array;
    int count;
    int source_indices[RUNTIME_POWER_LIMIT];
    SessionHandle powers[RUNTIME_POWER_LIMIT];
    uint32_t names[RUNTIME_POWER_LIMIT][2];
} RuntimePowerCatalog;
static RuntimePowerCatalog runtime_power_catalog;
static volatile LONG runtime_power_scope,runtime_power_count,runtime_power_status;
static volatile LONG64 runtime_power_request;
/* Serialize only queue metadata, never game reads or native calls. Both
 * producers and Tick try once; a preempted caller cannot stall the game. */
static SRWLOCK runtime_power_queue_lock=SRWLOCK_INIT;
static ULONGLONG runtime_power_submitted_at;
#define RUNTIME_POWER_REQUEST_MAX_AGE_MS 2000
static LONG runtime_power_generation;
static DWORD runtime_power_last_order;
enum {RUNTIME_POWER_IDLE=0,RUNTIME_POWER_QUEUED=1,RUNTIME_POWER_REJECTED=2,
    RUNTIME_POWER_NATIVE_ACCEPTED=3,RUNTIME_POWER_NATIVE_REFUSED=4,RUNTIME_POWER_NATIVE_DISPATCHED=5};
static int runtime_power_same_handle(SessionHandle a,SessionHandle b) {
    return a.pointer&&a.pointer==b.pointer&&a.index==b.index&&a.name==b.name;
}
static int runtime_power_catalog_same(const RuntimePowerCatalog *a,const RuntimePowerCatalog *b) {
    if(!runtime_power_same_handle(a->pc,b->pc)||!runtime_power_same_handle(a->pawn,b->pawn)||
       !runtime_power_same_handle(a->info,b->info)||!runtime_power_same_handle(a->input,b->input)||
       !runtime_power_same_handle(a->manager,b->manager)||
       !runtime_power_same_handle(a->channel,b->channel)||!runtime_power_same_handle(a->connection,b->connection)||
       !runtime_power_same_handle(a->driver,b->driver)||a->registration_token!=b->registration_token||
       memcmp(&a->array,&b->array,sizeof a->array)||a->count!=b->count)return 0;
    for(int i=0;i<a->count;++i)
        if(a->source_indices[i]!=b->source_indices[i]||
           !runtime_power_same_handle(a->powers[i],b->powers[i])||memcmp(a->names[i],b->names[i],8))return 0;
    return 1;
}
static void runtime_power_invalidate(void) {
    InterlockedExchange(&runtime_power_scope,0);InterlockedExchange(&runtime_power_count,0);
    /* Cancellation and status observers need only the atomic payload. Leave
     * the timestamp alone: it is read/written only under the queue lock.
     * An in-flight producer can publish an old scope after this clear; Tick
     * rejects that scope, whose generation is never reused. */
    if(InterlockedExchange64(&runtime_power_request,0))InterlockedExchange(&runtime_power_status,RUNTIME_POWER_REJECTED);
    memset(&runtime_power_catalog,0,sizeof runtime_power_catalog);
}
/* Only mod queue state is touched by exported callers. A UI must retain the scope
 * returned with its list; an index alone cannot identify a power after travel. */
#define RUNTIME_POWER_MAPPED_SELECTION UINT32_C(0x80000000)
static int runtime_power_mapped_action(unsigned action) {
    return action<=4||action==6;
}
static int runtime_power_enqueue(unsigned scope,unsigned selection) {
    unsigned index=(selection&~RUNTIME_POWER_MAPPED_SELECTION)-1;
    int mapped=(selection&RUNTIME_POWER_MAPPED_SELECTION)!=0;
    ULONGLONG submitted=GetTickCount64();
    if(!scope||scope>0x7fffffff||
       (mapped?!runtime_power_mapped_action(index):index>=RUNTIME_POWER_LIMIT)||
       !InterlockedCompareExchange(&g_enable_requested,0,0)||
       g_config_status!=ME3_CONFIG_READY||g_runtime_config.role!=ME3_ROLE_GUEST||
       InterlockedCompareExchange(&runtime_power_scope,0,0)!=(LONG)scope||
       (!mapped&&index>=(unsigned)InterlockedCompareExchange(&runtime_power_count,0,0)))return 0;
    LONG64 request=(LONG64)(((uint64_t)scope<<32)|selection);
    if(!TryAcquireSRWLockExclusive(&runtime_power_queue_lock))return 0;
    int accepted=0;
    if(InterlockedCompareExchange(&g_enable_requested,0,0)&&
       InterlockedCompareExchange(&runtime_power_scope,0,0)==(LONG)scope&&
       (mapped||index<(unsigned)InterlockedCompareExchange(&runtime_power_count,0,0))&&
       !InterlockedCompareExchange64(&runtime_power_request,0,0)) {
        runtime_power_submitted_at=submitted;
        accepted=InterlockedCompareExchange64(&runtime_power_request,request,0)==0;
    }
    ReleaseSRWLockExclusive(&runtime_power_queue_lock);
    return accepted;
}
static int runtime_request_power(unsigned scope,unsigned index) {
    if(index>=RUNTIME_POWER_LIMIT)return 0;
    return runtime_power_enqueue(scope,index+1);
}
static int runtime_request_mapped_power(unsigned scope,unsigned action) {
    if(!runtime_power_mapped_action(action))return 0;
    return runtime_power_enqueue(scope,RUNTIME_POWER_MAPPED_SELECTION|(action+1));
}
static LONG64 runtime_power_take_request(ULONGLONG *submitted) {
    if(!TryAcquireSRWLockExclusive(&runtime_power_queue_lock))return 0;
    LONG64 request=InterlockedExchange64(&runtime_power_request,0);
    *submitted=runtime_power_submitted_at;
    ReleaseSRWLockExclusive(&runtime_power_queue_lock);
    return request;
}
static int runtime_power_request_expired(ULONGLONG submitted) {
    ULONGLONG now=GetTickCount64();
    return now<submitted||now-submitted>RUNTIME_POWER_REQUEST_MAX_AGE_MS;
}
static int runtime_power_property(uintptr_t base,uintptr_t object,const char *owner,const char *name,
    const char *kind,int offset,int size);
static uintptr_t runtime_power_declaring_base(uintptr_t base,uintptr_t object) {
    uintptr_t klass=read_ptr(object+0x50),seen[48]={0};
    for(unsigned n=0;klass&&n<48;++n) {
        if(!narrative_probe_object(base,klass,"Class"))return 0;
        for(unsigned j=0;j<n;++j)if(seen[j]==klass)return 0;
        seen[n]=klass;char name[96];object_name(base,klass,name,sizeof name);
        if(!strcmp(name,"SFXPowerCustomActionBase"))return klass;
        if(!read_at(klass+0x60,&klass,8))return 0;
    }
    return 0;
}
/* Membership is read-only UI data. Native dispatch readiness is a separate
 * predicate, so an equip/power animation does not erase a valid UI list. */
static int runtime_power_read_membership(uintptr_t base,SessionContext c,RuntimePowerCatalog *out) {
    if(!guest_player_context_registered_for(c.pc,c.pawn)||
       !guest_player_context_basic_owned(base,c.pc,c.pawn,c.world))return 0;
    uint32_t life=0;
    /* Native downed/dead state invalidates the scope and queued input even
     * before action, input-ignore or ownership fields finish transitioning. */
    if(!read_at(c.pawn+0xe20,&life,4)||(life&0x300))return 0;
    uintptr_t input=read_ptr(c.pc+0x6dc),manager=0;GameArray array={0},after={0};NetworkActorIdentityEntry channel={0};
    if(!narrative_probe_object(base,input,"BioPlayerInput")||read_ptr(input+0x40)!=c.pc||
       !runtime_power_property(base,c.pc,"PlayerController","PlayerInput","ObjectProperty",0x6dc,8)||
       !read_at(c.pawn+0xc18,&manager,8)||!narrative_probe_object(base,manager,"SFXPowerManager")||
       read_ptr(manager+0xd0)!=c.pawn||!identity_array(manager+0x60,&array,RUNTIME_POWER_LIMIT)||!array.count||
       !guest_player_context_channel(base,c.pawn,&channel))return 0;
    memset(out,0,sizeof *out);out->pc=session_capture_handle(c.pc);out->pawn=session_capture_handle(c.pawn);
    out->info=session_capture_handle(c.world);out->input=session_capture_handle(input);
    out->manager=session_capture_handle(manager);out->array=array;
    out->channel=session_capture_handle(channel.channel);out->connection=session_capture_handle(channel.connection);
    out->driver=session_capture_handle(channel.driver);out->registration_token=guest_player_context_registration_token();
    if(!session_handle_matches(out->pc)||!session_handle_matches(out->pawn)||
       !session_handle_matches(out->info)||!session_handle_matches(out->input)||!session_handle_matches(out->manager)||
       !session_handle_matches(out->channel)||!session_handle_matches(out->connection)||!session_handle_matches(out->driver))return 0;
    uintptr_t sampled[RUNTIME_POWER_LIMIT]={0};SessionHandle layout={0};
    for(int i=0;i<array.count;++i) {
        uintptr_t item=0;
        if(!read_at(array.data+(size_t)i*8,&item,8))return 0;
        sampled[i]=item;
        /* Null is an absent native entry, not a reason to hide other powers.
         * Keep source positions so a rearranged sparse array changes scope.
         * Non-null unreadable/wrong-class entries still invalidate the list. */
        if(!item)continue;
        int entry=out->count;
        uint32_t flags=0;float rank=0;
        if(!narrative_probe_object(base,item,"SFXPowerCustomActionBase"))return 0;
        uintptr_t declaring=runtime_power_declaring_base(base,item);
        if(!declaring)return 0;
        if(!layout.pointer) {
            layout=session_capture_handle(declaring);
            if(!session_handle_matches(layout)||
               !runtime_power_property(base,item,"SFXPowerCustomActionBase","Rank","FloatProperty",0x488,4)||
               !runtime_power_property(base,item,"SFXPowerCustomActionBase","bEnabled","BoolProperty",0x4dc,4)||
               !runtime_power_property(base,item,"SFXPowerCustomActionBase","DisplayInHUD","BoolProperty",0x4dc,4))return 0;
        }
        /* All entries must inherit the same actual declaring Class, not
         * merely share its name. Reuse layout only within this read, with
         * no engine calls and no persistent metadata cache across worlds. */
        if(declaring!=layout.pointer||!session_handle_matches(layout)||
           !read_at(item+0x4dc,&flags,4)||!read_at(item+0x488,&rank,4)||!isfinite(rank))return 0;
        /* Match base power-wheel membership; cooldown/target/action checks
         * remain native dispatch policy, not reasons to reorder hotkeys. */
        if((flags&0x101)!=0x101||rank<=0)continue;
        if(!read_at(item+0x46c,out->names[entry],8))return 0;
        out->powers[entry]=session_capture_handle(item);
        if(!session_handle_matches(out->powers[entry]))return 0;
        out->source_indices[entry]=i;++out->count;
    }
    for(int i=0;i<array.count;++i) {
        uintptr_t item=0;
        if(!read_at(array.data+(size_t)i*8,&item,8)||item!=sampled[i])return 0;
    }
    return out->count&&session_handle_matches(layout)&&read_at(manager+0x60,&after,sizeof after)&&!memcmp(&array,&after,sizeof array)&&
        session_handle_matches(out->input)&&read_ptr(c.pc+0x6dc)==input&&read_ptr(input+0x40)==c.pc&&
        read_ptr(c.pawn+0xc18)==manager&&read_ptr(manager+0xd0)==c.pawn;
}
/* The supported native GetPower scans the full manager array and returns its
 * first exact PowerName match, including entries omitted from the UI catalog.
 * Prove that a name request will resolve to this particular eligible object. */
static int runtime_power_native_lookup_matches(uintptr_t base,const RuntimePowerCatalog *catalog,unsigned index) {
    static const unsigned char entry[]={0x48,0x63,0x41,0x68,0x48,0x89,0x54,0x24,0x10};
    unsigned char bytes[sizeof entry];GameArray array={0},after={0};
    if(index>=(unsigned)catalog->count||!session_handle_matches(catalog->manager)||
       !session_handle_matches(catalog->powers[index])||
       read_ptr(read_ptr(catalog->manager.pointer)+0x2b0)!=base+0xc0c4d0||
       !read_at(base+0xc0c4d0,bytes,sizeof bytes)||memcmp(bytes,entry,sizeof bytes)||
       !runtime_power_property(base,catalog->manager.pointer,"SFXPowerManager","Powers","ArrayProperty",0x60,16)||
       !runtime_power_property(base,catalog->powers[index].pointer,"SFXPowerCustomActionBase","PowerName","NameProperty",0x46c,8)||
       !identity_array(catalog->manager.pointer+0x60,&array,RUNTIME_POWER_LIMIT)||
       memcmp(&array,&catalog->array,sizeof array))return 0;
    uintptr_t objects[RUNTIME_POWER_LIMIT]={0};uint32_t names[RUNTIME_POWER_LIMIT][2]={{0}};int found=-1;
    for(int n=0;n<array.count;++n) {
        if(!read_at(array.data+(size_t)n*8,&objects[n],8))return 0;
        if(!objects[n])continue;
        if(!narrative_probe_object(base,objects[n],"SFXPowerCustomActionBase")||
           !read_at(objects[n]+0x46c,names[n],8))return 0;
        if(!memcmp(names[n],catalog->names[index],8)){found=n;break;}
    }
    if(found<0||objects[found]!=catalog->powers[index].pointer)return 0;
    for(int n=0;n<=found;++n) {
        uintptr_t object=0;uint32_t name[2]={0};
        if(!read_at(array.data+(size_t)n*8,&object,8)||object!=objects[n]||
           (object&&(!read_at(object+0x46c,name,8)||memcmp(name,names[n],8))))return 0;
    }
    return read_at(catalog->manager.pointer+0x60,&after,sizeof after)&&!memcmp(&array,&after,sizeof array)&&
        session_handle_matches(catalog->powers[index])&&session_handle_matches(catalog->manager)&&
        read_ptr(catalog->pawn.pointer+0xc18)==catalog->manager.pointer&&
        read_ptr(catalog->manager.pointer+0xd0)==catalog->pawn.pointer;
}
static int runtime_power_dispatch_idle(SessionContext c) {
    int32_t action=-1,power=-1;unsigned char ignored=255;
    return read_at(c.pawn+0xc20,&action,4)&&!action&&read_at(c.pawn+0xc28,&power,4)&&!power&&
        read_at(c.pc+0x782,&ignored,1)&&!ignored;
}
#include "runtime_power_reload_admission.h"
static int runtime_power_read_catalog(uintptr_t base,SessionContext c,int ordinary,RuntimePowerCatalog *out) {
    /* Typed native reload/evade input may reach stock UsePower, which owns queuing.
     * Other busy actions retain the restrictive command admission policy. */
    return ordinary&&runtime_power_input_ready(base,c)&&runtime_power_read_membership(base,c,out)&&
        runtime_power_input_ready(base,c);
}
/* Reflected layout corroborates the retained SDK before invoking an order.
 * No property writes or method substitution are performed. */
typedef struct RuntimeLayoutResolved {
    uintptr_t classes[48],field,owner_children;
    unsigned count,examined;
} RuntimeLayoutResolved;
static int runtime_power_property_dim_uncached(uintptr_t base,uintptr_t object,const char *owner,const char *name,
    const char *kind,int offset,int size,int dimensions,RuntimeLayoutResolved *resolved) {
    if(resolved)memset(resolved,0,sizeof *resolved);
    uintptr_t klass=read_ptr(object+0x50),seen[48]={0};
    for(unsigned n=0;klass&&n<48;++n) {
        if(!narrative_probe_object(base,klass,"Class"))return 0;
        for(unsigned j=0;j<n;++j)if(seen[j]==klass)return 0;seen[n]=klass;
        if(resolved)resolved->classes[resolved->count++]=klass;
        char text[96];object_name(base,klass,text,sizeof text);
        if(!strcmp(text,owner)) {
            uintptr_t field=read_ptr(klass+0x70);
            if(resolved)resolved->owner_children=field;
            for(unsigned k=0;field&&k<1024;++k) {
                if(resolved)++resolved->examined;
                if(!is_live(base,field))return 0;object_name(base,field,text,sizeof text);
                if(!strcmp(text,name)) {
                    int32_t got=-1,bytes=0,dim=0;
                    int valid=read_ptr(field+0x40)==klass&&narrative_probe_object(base,field,kind)&&
                        read_at(field+0x94,&got,4)&&got==offset&&read_at(field+0x74,&bytes,4)&&bytes==size&&
                        read_at(field+0x70,&dim,4)&&dim==dimensions;
                    if(valid&&resolved)resolved->field=field;
                    return valid;
                }
                if(!read_at(field+0x68,&field,8))return 0;
            }
            return 0;
        }
        if(!read_at(klass+0x60,&klass,8))return 0;
    }
    return 0;
}
#include "runtime_layout_cache.h"
static int runtime_power_property_dim(uintptr_t base,uintptr_t object,const char *owner,const char *name,
    const char *kind,int offset,int size,int dimensions) {
    return runtime_layout_cache_property(base,object,owner,name,kind,offset,size,dimensions);
}
static int runtime_power_property(uintptr_t base,uintptr_t object,const char *owner,const char *name,
    const char *kind,int offset,int size) {
    return runtime_power_property_dim(base,object,owner,name,kind,offset,size,1);
}
/* Campaign TryUsePower action numbers select named input properties, not
 * PowerManager array positions. Resolve only on Tick and never invent a slot. */
static int runtime_power_resolve_mapped(uintptr_t base,const RuntimePowerCatalog *catalog,unsigned action) {
    static const char *fields[]={"m_nmMappedPower","m_nmMappedPower2","m_nmMappedPower3",
        "m_nmMappedPower4","m_nmMappedPower5",NULL,"m_nmMappedPower7"};
    if(!runtime_power_mapped_action(action)||!session_handle_matches(catalog->input)||
       read_ptr(catalog->pc.pointer+0x6dc)!=catalog->input.pointer||
       read_ptr(catalog->input.pointer+0x40)!=catalog->pc.pointer)return -1;
    uint32_t name[2]={0};int offset=0x338+(int)action*8;
    if(!runtime_power_property(base,catalog->input.pointer,"BioPlayerInput",fields[action],"NameProperty",offset,8)||
       !read_at(catalog->input.pointer+offset,name,8)||!name[0])return -1;
    int found=-1;
    for(int i=0;i<catalog->count;++i)if(!memcmp(name,catalog->names[i],8)) {
        if(found>=0)return -1;
        found=i;
    }
    return found;
}
static void runtime_power_log(int status,const char *reason,unsigned index,DWORD now) {
    char row[420];snprintf(row,sizeof row,
        "{\"runtime_power\":\"order-result\",\"status\":%d,\"reason\":\"%s\",\"catalog_index\":%u,\"native_cast_verified\":false,\"target_effect_verified\":false,\"monotonic_ms\":%lu}\r\n",
        status,reason,index,now);line(row);InterlockedExchange(&runtime_power_status,status);
}
#include "runtime_power_input.h"
#include "runtime_native_power_input.h"
#include "runtime_mapped_power_input.h"
#include "runtime_power_order_apply.h"
#include "runtime_power_reload.h"
static void runtime_guest_power_tick(uintptr_t base,uintptr_t engine,SessionContext c,int ordinary,int after,DWORD now) {
    RuntimePowerCatalog current;
    if(!runtime_control_enabled||!runtime_scope_ready||!runtime_power_read_catalog(base,c,ordinary,&current)) {
        if(!after)runtime_power_input_poll(0,0);
        runtime_power_invalidate();return;
    }
    if(!runtime_power_catalog_same(&runtime_power_catalog,&current)) {
        runtime_power_invalidate();
        if(runtime_power_generation==0x7fffffff)return; /* Never reuse an external scope. */
        ++runtime_power_generation;runtime_power_catalog=current;
        InterlockedExchange(&runtime_power_count,current.count);
        InterlockedExchange(&runtime_power_scope,runtime_power_generation);
    }
    if(after)return;
    runtime_power_bind_slots(&current,(unsigned)runtime_power_scope);
    runtime_mapped_power_input_prepare(base,c,ordinary);
    runtime_native_power_input_install(base);
    int action=runtime_power_input_poll((unsigned)runtime_power_scope,(unsigned)current.count);
    int action_index=action>=0?runtime_power_slot_index(&current,(unsigned)action):-1;
    if(action_index>=0)runtime_request_power((unsigned)runtime_power_scope,(unsigned)action_index);
    ULONGLONG submitted=0;
    LONG64 request=runtime_power_take_request(&submitted);if(!request)return;
    unsigned scope=(unsigned)((uint64_t)request>>32),selection=(unsigned)request;
    unsigned requested=(selection&~RUNTIME_POWER_MAPPED_SELECTION)-1,index=requested;
    int mapped=(selection&RUNTIME_POWER_MAPPED_SELECTION)!=0;
    int status=RUNTIME_POWER_REJECTED;const char *reason="stale-scope-or-rate-limit";
    if(runtime_power_request_expired(submitted)){reason="power-request-expired";goto rejected;}
    if(runtime_power_wheel_input_blocked(base,c)){reason="native-wheel-drag-or-unreadable";goto rejected;}
    if(mapped) {
        int resolved=runtime_power_resolve_mapped(base,&current,requested);
        if(resolved<0){reason="mapped-power-unavailable-or-ambiguous";index=UINT32_MAX;goto rejected;}
        index=(unsigned)resolved;
    }
    if(scope!=(unsigned)runtime_power_scope||index>=(unsigned)current.count||
       (runtime_power_last_order&&now-runtime_power_last_order<250))goto rejected;
    for(int i=0;i<current.count;++i)if((unsigned)i!=index&&
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
    if(runtime_power_queued_action_instance(base,c)) {
        if(runtime_power_request_expired(submitted)){reason="power-request-expired";goto rejected;}
        runtime_power_last_order=now;
        int sent=runtime_power_reload_use(base,engine,&current,index,mapped,requested,submitted);
        status=sent?RUNTIME_POWER_NATIVE_DISPATCHED:RUNTIME_POWER_REJECTED;
        reason=sent?"native-action-usepower-dispatched":"action-usepower-context-or-method-not-ready";
        goto rejected;
    }
    uintptr_t method=guest_player_context_find_method(base,read_ptr(c.pc+0x50),"BioPlayerController","SquadOrderUsePower");
    NetworkActorIdentityEntry channel={0};CombatContextChain chain;
    RuntimePowerCatalog final;SessionContext checked=session_context(engine);uint32_t combat=2;
    if(!guest_power_order_contract(base,method)||
       !guest_player_context_conditions(base,c.pc,c.pawn,c.world,ordinary,1,1,&channel,&chain)||
       !runtime_power_read_catalog(base,checked,!session_engine_blocked(checked),&final)||
       !runtime_power_catalog_same(&current,&final)||
       (mapped&&runtime_power_resolve_mapped(base,&final,requested)!=(int)index)||
       !combat_context_get(base,read_ptr(c.pc+0x6dc),&combat)||combat!=1) {
        reason="native-order-or-current-control-not-ready";goto rejected;
    }
    uintptr_t pe=read_ptr(read_ptr(c.pc)+0x230);
    if(pe!=base+0x40d450){reason="native-dispatch-entry";goto rejected;}
    checked=session_context(engine);
    if(session_engine_blocked(checked)||!runtime_power_read_catalog(base,checked,1,&final)||
       runtime_power_wheel_input_blocked(base,checked)||
       !runtime_power_catalog_same(&current,&final)||
       (mapped&&runtime_power_resolve_mapped(base,&final,requested)!=(int)index)||
       !guest_player_context_conditions(base,checked.pc,checked.pawn,checked.world,1,1,1,&channel,&chain)||
       read_ptr(item+0x98)!=c.pawn||read_ptr(item+0xa0)!=c.pc||read_ptr(item+0xa8)||
       !guest_power_order_contract(base,method)||read_ptr(read_ptr(c.pc)+0x230)!=pe) {
        reason="context-changed-before-native-order";goto rejected;
    }
    if(!runtime_power_native_lookup_matches(base,&final,index)) {
        reason="native-name-resolves-to-different-or-unverified-power";goto rejected;
    }
    GuestPowerOrderParms parms={0};memcpy(parms.power_name,name,8);parms.pawn=c.pawn;parms.result=UINT32_MAX;
    if(runtime_power_request_expired(submitted)){reason="power-request-expired";goto rejected;}
    runtime_power_last_order=now;
    ((ProcessEvent)pe)((void*)c.pc,(void*)method,&parms,NULL);
    /* The native order only queues own-pawn powers. Reacquire all control
     * and catalog identities before the stock UsePower continuation. */
    status=parms.result==1?RUNTIME_POWER_NATIVE_ACCEPTED:parms.result==0?RUNTIME_POWER_NATIVE_REFUSED:RUNTIME_POWER_REJECTED;
    reason=parms.result==1?"native-tactical-order-accepted":parms.result==0?"native-tactical-order-refused":"native-return-unverified";
    if(parms.result==1&&runtime_power_apply_accepted(base,engine,&final,name))reason="native-tactical-order-applied";
rejected:
    runtime_power_log(status,reason,index,now);
}
#endif
