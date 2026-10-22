#ifndef ME3_RUNTIME_HOST_BOOTSTRAP_H
#define ME3_RUNTIME_HOST_BOOTSTRAP_H
/* Explicit one-shot current-world listen. Uses genuine engine allocation,
 * Listen, package-map Init and validation; never reloads a mission. Failure
 * after native listener creation is fatal for this mod instance: no companion
 * lease, no invented driver destruction, and restart is required. */
typedef struct RuntimeHostURL {
    GameString protocol,host,map;
    GameArray options;
    GameString portal;
    int32_t port,valid;
} RuntimeHostURL;
_Static_assert(sizeof(RuntimeHostURL)==0x58,"native FURL size");
typedef RuntimeHostURL *(*RuntimeHostURLCopy)(RuntimeHostURL*,const RuntimeHostURL*);
typedef void (*RuntimeHostURLDestroy)(RuntimeHostURL*);
typedef int (*RuntimeHostListen)(void*,RuntimeHostURL*,GameString*);
typedef uintptr_t (*RuntimeHostDriverClass)(void);
typedef struct RuntimeHostCapacityLease {
    SessionHandle game,info;
    int32_t players,allowed;
    int active;
} RuntimeHostCapacityLease;
static RuntimeHostCapacityLease runtime_host_capacity;

static int runtime_host_hash_pin(uintptr_t address,size_t count,uint64_t expected) {
    unsigned char bytes[128];uint64_t hash=UINT64_C(0xcbf29ce484222325);
    while(count) {
        size_t take=count<sizeof bytes?count:sizeof bytes;
        if(!read_at(address,bytes,take))return 0;
        for(size_t i=0;i<take;++i)hash=(hash^bytes[i])*UINT64_C(0x100000001b3);
        address+=take;count-=take;
    }
    return hash==expected;
}
static int runtime_host_native_pins(uintptr_t base) {
    /* Code integrity pins complement the mandatory full executable SHA-256.
     * Relative instruction bytes are stable under the supported image layout. */
    return runtime_host_hash_pin(base+0x8c38f0,0x35a,UINT64_C(0xbe0caafca665bcc7))&&
        runtime_host_hash_pin(base+0x53c4d0,0x157,UINT64_C(0xdfb8b26871512e32))&&
        runtime_host_hash_pin(base+0x1fb090,0x90,UINT64_C(0xaf0d741c69e32021))&&
        runtime_host_hash_pin(base+0x98c550,0x17c,UINT64_C(0x9d45eed07a920c90))&&
        runtime_host_hash_pin(base+0xc8ea0,0x80,UINT64_C(0x2b75e47bc15e663a))&&
        runtime_host_hash_pin(base+0x903a80,0xe0,UINT64_C(0xcd82f9c1796d2c53))&&
        runtime_host_hash_pin(base+0x903120,0x3b7,UINT64_C(0x27a9dd30ade2fb22))&&
        read_ptr(base+0x12bc980+0x2e0)==base+0x903a80&&
        read_ptr(base+0x1086b10+0x2e0)==base+0xc8ea0&&
        guest_cinematic_allocator_pins(base);
}
static int runtime_host_string(GameString value,wchar_t out[512]) {
    if(value.count<0||value.count>512||value.max<value.count||value.max>4096||
       (value.count&&!value.data))return 0;
    out[0]=0;
    if(!value.count)return 1;
    return read_at((uintptr_t)value.data,out,(size_t)value.count*2)&&!out[value.count-1]&&
        wcslen(out)==(size_t)value.count-1;
}
static uintptr_t runtime_host_world_package(uintptr_t base,uintptr_t world) {
    uintptr_t seen[48];int count=0;
    while(world&&count<48) {
        if(!is_live(base,world))return 0;
        for(int i=0;i<count;++i)if(seen[i]==world)return 0;
        seen[count++]=world;uintptr_t outer=0;
        if(!read_at(world+0x40,&outer,8))return 0;
        if(!outer)return is_kind_of(base,world,"Package")?world:0;
        world=outer;
    }
    return 0;
}
static int runtime_host_url_valid(uintptr_t base,uintptr_t world,const RuntimeHostURL *url) {
    wchar_t protocol[512],host[512],map[512],portal[512],option[512];
    if(!url||!runtime_host_string(url->protocol,protocol)||_wcsicmp(protocol,L"unreal")||
       !runtime_host_string(url->host,host)||host[0]||!runtime_host_string(url->map,map)||!map[0]||
       !runtime_host_string(url->portal,portal)||url->valid!=1||url->port<0||url->port>65535||
       url->options.count<0||url->options.count>64||url->options.max<url->options.count||
       url->options.max>256||(url->options.count&&!url->options.data))return 0;
    for(int i=0;i<url->options.count;++i) {
        GameString item;
        if(!read_at(url->options.data+(size_t)i*sizeof item,&item,sizeof item)||
           !runtime_host_string(item,option))return 0;
    }
    uintptr_t root=runtime_host_world_package(base,world);char name[96];uint32_t number=1;
    if(!root||!read_at(root+0x4c,&number,4)||number)return 0;
    object_name(base,root,name,sizeof name);size_t count=strlen(name);
    if(!count||count!=wcslen(map))return 0;
    for(size_t i=0;i<count;++i)
        if((unsigned char)name[i]>127||runtime_transport_upper(map[i])!=
           runtime_transport_upper((wchar_t)(unsigned char)name[i]))return 0;
    return 1;
}
static int runtime_host_capacity_property(uintptr_t base,uintptr_t owner,const char *name,int offset,uint64_t flags) {
    uintptr_t field=read_ptr(owner+0x70),seen[512];unsigned count=0,matches=0;
    while(field&&count<512) {
        if(!is_live(base,field))return 0;
        for(unsigned i=0;i<count;++i)if(seen[i]==field)return 0;
        seen[count++]=field;char text[96];object_name(base,field,text,sizeof text);
        if(!strcmp(text,name)) {
            int dim=0,size=0,got=0;uint64_t actual=0;
            if(++matches!=1||!is_kind_of(base,field,"IntProperty")||read_ptr(field+0x40)!=owner||
               !read_at(field+0x70,&dim,4)||dim!=1||!read_at(field+0x74,&size,4)||size!=4||
               !read_at(field+0x94,&got,4)||got!=offset||!read_at(field+0x78,&actual,8)||actual!=flags)return 0;
        }
        if(!read_at(field+0x68,&field,8))return 0;
    }
    return !field&&matches==1;
}
static int runtime_host_capacity_layout(uintptr_t base,uintptr_t game) {
    uintptr_t klass=read_ptr(game+0x50),seen[48];unsigned count=0;
    while(klass&&count<48) {
        if(!is_live(base,klass)||!is_kind_of(base,klass,"Class"))return 0;
        for(unsigned i=0;i<count;++i)if(seen[i]==klass)return 0;
        seen[count++]=klass;char text[96];object_name(base,klass,text,sizeof text);
        if(!strcmp(text,"GameInfo"))return
            runtime_host_capacity_property(base,klass,"MaxPlayers",0x42c,UINT64_C(0x44000))&&
            runtime_host_capacity_property(base,klass,"MaxPlayersAllowed",0x430,0)&&
            runtime_host_capacity_property(base,klass,"NumPlayers",0x434,0);
        if(!read_at(klass+0x60,&klass,8))return 0;
    }
    return 0;
}
static int runtime_host_capacity_identity(uintptr_t base,const RuntimeHostCapacityLease *lease) {
    return lease->active&&session_handle_matches(lease->game)&&session_handle_matches(lease->info)&&
        session_actor_live(base,lease->game.pointer)&&is_kind_of(base,lease->game.pointer,"GameInfo")&&
        session_actor_live(base,lease->info.pointer)&&read_ptr(lease->info.pointer+0x6b0)==lease->game.pointer;
}
static int runtime_host_capacity_values(uintptr_t game,int32_t *players,int32_t *allowed,int32_t *num) {
    return read_at(game+0x42c,players,4)&&read_at(game+0x430,allowed,4)&&read_at(game+0x434,num,4)&&
        *players>=0&&*players<=2&&*allowed>=0&&*allowed<=2&&*num>=0&&*num<=1;
}
static int runtime_host_capacity_installed(uintptr_t base,const RuntimeHostCapacityLease *lease) {
    int32_t players=0,allowed=0,num=0;
    return runtime_host_capacity_identity(base,lease)&&
        runtime_host_capacity_values(lease->game.pointer,&players,&allowed,&num)&&players==2&&allowed==2;
}
static int runtime_host_capacity_apply(uintptr_t base,uintptr_t info,RuntimeHostCapacityLease *lease) {
    memset(lease,0,sizeof *lease);uintptr_t game=0;int32_t num=0;
    if(!read_at(info+0x6b0,&game,8)||!session_actor_live(base,game)||!is_kind_of(base,game,"GameInfo")||
       !runtime_host_capacity_layout(base,game)||
       !runtime_host_capacity_values(game,&lease->players,&lease->allowed,&num))return 0;
    lease->game=session_capture_handle(game);lease->info=session_capture_handle(info);lease->active=1;
    if(!runtime_host_capacity_identity(base,lease)) {lease->active=0;return 0;}
    if(InterlockedCompareExchange((LONG volatile*)(game+0x430),2,lease->allowed)!=lease->allowed) {
        lease->active=0;return 0;
    }
    if(InterlockedCompareExchange((LONG volatile*)(game+0x42c),2,lease->players)!=lease->players) {
        InterlockedCompareExchange((LONG volatile*)(game+0x430),lease->allowed,2);lease->active=0;return 0;
    }
    return 1;
}
static int runtime_host_capacity_restore(uintptr_t base,RuntimeHostCapacityLease *lease) {
    if(!runtime_host_capacity_identity(base,lease)) {lease->active=0;return 0;}
    uintptr_t game=lease->game.pointer;
    /* Each exact installed value is restored only if still ours. Never rewrite
     * new game-owned values, config files, profiles or a reused object. */
    int restored=InterlockedCompareExchange((LONG volatile*)(game+0x42c),lease->players,2)==2;
    restored&=InterlockedCompareExchange((LONG volatile*)(game+0x430),lease->allowed,2)==2;
    lease->active=0;return restored;
}
static int runtime_host_client_free(uintptr_t base,uintptr_t world,uintptr_t driver) {
    uintptr_t server=1;GameArray clients={0};
    return is_live(base,world)&&read_ptr(world+0x120)==driver&&is_live(base,driver)&&
        is_kind_of(base,driver,"TcpNetDriver")&&read_ptr(driver)==base+0x12bc980&&
        read_at(driver+0x80,&server,8)&&!server&&read_at(driver+0x70,&clients,sizeof clients)&&
        clients.count==0&&clients.max>=0&&clients.max<=128;
}
typedef void (*RuntimeHostMapInit)(void*);
static int runtime_host_master_prepare(uintptr_t base,uintptr_t world,uintptr_t driver,RuntimeHostMapInit initialize) {
    if(!runtime_host_client_free(base,world,driver))return 0;
    uintptr_t map=read_ptr(driver+0x90),packages[3]={0};GameArray objects={0},registered={0},entries={0};
    if(!is_live(base,map)||!is_kind_of(base,map,"PackageMap")||read_ptr(map)!=base+0x1086b10||
       read_ptr(base+0x1086b10+0x2e0)!=base+0xc8ea0||
       !identity_array(base+0x17f4b10,&registered,20000))return 0;
    uintptr_t *list=identity_object_list(base,&objects);if(!list)return 0;int valid=1;
    for(int i=0;i<objects.count;++i) {
        uintptr_t object=list[i];if(!object||read_ptr(object+0x40)||!is_live(base,object)||
            !is_kind_of(base,object,"Package"))continue;
        char name[96];object_name(base,object,name,sizeof name);
        int slot=!strcmp(name,"Core")?0:!strcmp(name,"SFXGame")?1:!strcmp(name,"SFXGameContent")?2:-1;
        if(slot>=0) {if(packages[slot])valid=0;packages[slot]=object;}
    }
    HeapFree(GetProcessHeap(),0,list);uint32_t guids[3][4]={{0}};SessionHandle handles[3];
    for(int j=0;j<3&&valid;++j) {
        int matches=0;valid=packages[j]&&read_at(packages[j]+0x6c,guids[j],16)&&
            (guids[j][0]||guids[j][1]||guids[j][2]||guids[j][3]);
        for(int i=0;i<registered.count;++i) {
            uintptr_t value=0;
            if(!read_at(registered.data+(size_t)i*8,&value,8)) {valid=0;break;}
            matches+=value==packages[j];
        }
        valid=valid&&matches==1;handles[j]=session_capture_handle(packages[j]);
    }
    if(!valid||!runtime_host_client_free(base,world,driver))return 0;
    /* The same original Init is called inside UWorld::Listen. Repeat only with
     * zero connections, before engine network Tick can admit a guest. */
    initialize((void*)map);
    GameArray registered_after={0},entries_after={0};
    if(!runtime_host_client_free(base,world,driver)||read_ptr(driver+0x90)!=map||
       !identity_array(map+0x68,&entries,20000)||
       !read_at(base+0x17f4b10,&registered_after,sizeof registered_after)||
       memcmp(&registered,&registered_after,sizeof registered))return 0;
    for(int j=0;j<3;++j) {
        uint32_t current[4];int matches=0;
        if(!session_handle_matches(handles[j])||!is_live(base,packages[j])||
           !read_at(packages[j]+0x6c,current,16)||memcmp(current,guids[j],16))return 0;
        for(int i=0;i<entries.count;++i) {
            uintptr_t parent=0;uint32_t mapped[4];uintptr_t entry=entries.data+(size_t)i*0x4c;
            if(!read_at(entry+8,&parent,8))return 0;
            if(parent==packages[j]) {
                if(!read_at(entry+0x10,mapped,16)||memcmp(mapped,current,16))return 0;
                ++matches;
            }
        }
        if(matches!=1)return 0;
        matches=0;
        for(int i=0;i<registered.count;++i) {
            uintptr_t value=0;
            if(!read_at(registered.data+(size_t)i*8,&value,8))return 0;
            matches+=value==packages[j];
        }
        if(matches!=1)return 0;
    }
    return runtime_host_client_free(base,world,driver)&&read_ptr(driver+0x90)==map&&
        read_at(map+0x68,&entries_after,sizeof entries_after)&&!memcmp(&entries,&entries_after,sizeof entries)&&
        read_at(base+0x17f4b10,&registered_after,sizeof registered_after)&&!memcmp(&registered,&registered_after,sizeof registered);
}
static int runtime_host_story_idle(uintptr_t base,SessionContext c) {
    uint32_t flags=0,world_flags=0,activating=0;uintptr_t pauser=1,conversation=0;
    if(session_engine_blocked(c)||!session_actor_live(base,c.pawn)||!is_kind_of(base,c.pawn,"BioPlayerPawn")||
       read_ptr(c.pawn+0x374)!=c.pc||!read_at(c.pc+0x77c,&flags,4)||(flags&0x20000)||
       !read_at(c.world+0x7c0,&world_flags,4)||(world_flags&0xc00)||
       !read_at(c.world+0x670,&pauser,8)||pauser||!read_at(c.world+0x948,&conversation,8))return 0;
    if(conversation) {
        GameArray active={0};
        if(!is_live(base,conversation)||!is_kind_of(base,conversation,"BioConversationManager")||
           !read_at(conversation+0xc8,&active,sizeof active)||active.count||active.max<0||active.max>4096||
           !read_at(conversation+0xe8,&activating,4)||(activating&1))return 0;
    }
    return 1;
}
static int runtime_host_driver_class(uintptr_t base) {
    uintptr_t klass=((RuntimeHostDriverClass)(base+0x98c550))();char name[96];
    if(!is_live(base,klass)||!is_kind_of(base,klass,"Class"))return 0;
    object_name(base,klass,name,sizeof name);uintptr_t cdo=read_ptr(klass+0x13c);
    return !strcmp(name,"TcpNetDriver")&&is_live(base,cdo)&&read_ptr(cdo+0x50)==klass&&
        read_ptr(cdo)==base+0x12bc980;
}
typedef struct RuntimeHostNativeCalls {
    RuntimeHostURLCopy copy;
    RuntimeHostURLDestroy destroy;
    RuntimeHostListen listen;
    RuntimeHostMapInit initialize;
    void (*free_error)(void*,uint32_t);
} RuntimeHostNativeCalls;
static int runtime_host_start_checked(uintptr_t base,uintptr_t engine,SessionContext c,
    const CombatContextChain *chain,const RuntimeHostNativeCalls *calls) {
    RuntimeHostURL before={0},copied={0},after={0};
    if(!read_at(engine+0x8ac,&before,sizeof before)||
       !runtime_host_url_valid(base,chain->world,&before)) {
        line("{\"runtime_network\":\"listen-refused\",\"reason\":\"current-map-url\"}\r\n");return RUNTIME_START_ABI_REFUSED;
    }
    /* Capture all ordinary story/control identities before native allocation. */
    SessionHandle pc=session_capture_handle(c.pc),pawn=session_capture_handle(c.pawn),
        world=session_capture_handle(chain->world),info=session_capture_handle(c.world);
    uintptr_t squad_player=read_ptr(c.squad+0x2cc),squad_initial=read_ptr(c.squad+0x2d4);
    if(!runtime_host_capacity_apply(base,c.world,&runtime_host_capacity)) {
        line("{\"runtime_network\":\"listen-refused\",\"reason\":\"capacity-contract\"}\r\n");return RUNTIME_START_CAPACITY_REFUSED;
    }
    calls->copy(&copied,(const RuntimeHostURL*)(engine+0x8ac));
    CombatContextChain current;combat_context_read_chain(base,&current);
    int stable=read_at(engine+0x8ac,&after,sizeof after)&&!memcmp(&before,&after,sizeof before)&&
        combat_context_chain_equal(chain,&current)&&session_handle_matches(pc)&&session_handle_matches(pawn)&&
        session_handle_matches(world)&&session_handle_matches(info)&&local_controller(engine)==c.pc&&
        read_ptr(c.pc+0x3bc)==c.pawn&&read_ptr(c.pawn+0x374)==c.pc&&
        read_ptr(c.squad+0x2cc)==squad_player&&read_ptr(c.squad+0x2d4)==squad_initial&&
        runtime_host_capacity_installed(base,&runtime_host_capacity)&&
        runtime_host_story_idle(base,c)&&runtime_join_travel_idle(chain->world,engine)&&
        runtime_host_url_valid(base,chain->world,&copied);
    if(!stable) {
        calls->destroy(&copied);
        runtime_host_capacity_restore(base,&runtime_host_capacity);
        line("{\"runtime_network\":\"listen-refused\",\"reason\":\"context-changed-before-listen\"}\r\n");return RUNTIME_START_ABI_REFUSED;
    }
    copied.port=g_runtime_config.endpoint.port;GameString error={0};
    /* Listen consumes the deep FURL copy on every ordinary return. Do not free
     * it again, and never pass borrowed LastURL/stack-backed FString data. */
    int listened=calls->listen((void*)chain->world,&copied,&error);
    if(error.data)calls->free_error(error.data,0);
    uintptr_t driver=read_ptr(chain->world+0x120);unsigned char net=255;
    combat_context_read_chain(base,&current);
    int verified=listened&&combat_context_chain_equal(chain,&current)&&
        session_handle_matches(pc)&&session_handle_matches(pawn)&&session_handle_matches(world)&&session_handle_matches(info)&&
        local_controller(engine)==c.pc&&read_ptr(c.pc+0x3bc)==c.pawn&&read_ptr(c.pawn+0x374)==c.pc&&
        read_ptr(c.squad+0x2cc)==squad_player&&read_ptr(c.squad+0x2d4)==squad_initial&&
        runtime_host_capacity_installed(base,&runtime_host_capacity)&&
        read_at(c.world+0x7c8,&net,1)&&net==2&&runtime_host_client_free(base,chain->world,driver)&&
        runtime_transport_endpoint(base,driver,&g_runtime_config)&&
        runtime_host_master_prepare(base,chain->world,driver,calls->initialize)&&
        read_at(engine+0x8ac,&after,sizeof after)&&!memcmp(&before,&after,sizeof before)&&
        runtime_host_url_valid(base,chain->world,&after);
    if(!verified) {
        char row[280];int endpoint=runtime_transport_endpoint(base,driver,&g_runtime_config);
        snprintf(row,sizeof row,"{\"runtime_network\":\"listen-verification-failed\",\"native_returned_success\":%s,\"net_mode\":%u,\"endpoint_matches\":%s,\"capacity_matches\":%s,\"restart_required\":true}\r\n",listened?"true":"false",(unsigned)net,endpoint?"true":"false",runtime_host_capacity_installed(base,&runtime_host_capacity)?"true":"false");line(row);
        runtime_host_capacity_restore(base,&runtime_host_capacity);
        return listened?RUNTIME_START_HOST_VERIFY_FAILED:RUNTIME_START_HOST_NATIVE_FAILED;
    }
    return RUNTIME_START_LISTENING;
}
static int runtime_host_start(uintptr_t base,uintptr_t engine,SessionContext c,const CombatContextChain *chain) {
    if(!runtime_host_native_pins(base)) {
        line("{\"runtime_network\":\"listen-refused\",\"reason\":\"native-code-contract\"}\r\n");return RUNTIME_START_ABI_REFUSED;
    }
    if(!runtime_host_driver_class(base)) {
        line("{\"runtime_network\":\"listen-refused\",\"reason\":\"selected-driver-class\"}\r\n");return RUNTIME_START_ABI_REFUSED;
    }
    const RuntimeHostNativeCalls calls={
        (RuntimeHostURLCopy)(base+0x53c4d0),(RuntimeHostURLDestroy)(base+0x1fb090),
        (RuntimeHostListen)(base+0x8c38f0),(RuntimeHostMapInit)(base+0xc8ea0),
        (void(*)(void*,uint32_t))(base+0xbf0d0)};
    return runtime_host_start_checked(base,engine,c,chain,&calls);
}
static void runtime_host_capacity_poll(uintptr_t base,uintptr_t engine) {
    if(!runtime_host_capacity.active||session_active||
       (runtime_control_enabled&&InterlockedCompareExchange(&runtime_host_start_gate,0,0)))return;
    CombatContextChain chain;combat_context_read_chain(base,&chain);
    if(!chain.info_valid||chain.info!=runtime_host_capacity.info.pointer) {
        /* A disposed/replaced world retires this transient lease; it does not
         * authorize writing into an older or reused GameInfo. */
        runtime_host_capacity.active=0;return;
    }
    uintptr_t driver=read_ptr(chain.world+0x120);GameArray peers={0};
    if(driver&&(!is_live(base,driver)||!read_at(driver+0x70,&peers,sizeof peers)||peers.count||
       peers.max<0||peers.max>128))return;
    int restored=runtime_host_capacity_restore(base,&runtime_host_capacity);
    char row[180];snprintf(row,sizeof row,"{\"runtime_network\":\"capacity-cleanup\",\"compare_before_restore\":true,\"restored\":%s}\r\n",restored?"true":"false");line(row);
    (void)engine;
}
#endif
