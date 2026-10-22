#ifndef ME3_GUEST_CINEMATIC_REGISTRATION_ADAPTER_H
#define ME3_GUEST_CINEMATIC_REGISTRATION_ADAPTER_H

/* Opt-in exact-build experiment, game thread only, before ordinary Tick.
 * Include after guest_camera_adapter.h and combat_context_probe.h. No pawn is
 * required: the intro plays before the guest takes possession. Caller passes
 * PROGRESS_TEST && private marker. No global object enumeration occurs here.
 *
 * LE3 2.0.0.48602 private camera-registration-19192-25716 evidence: host registers
 * implicit GroupActor cameras; client receive collects explicit variables only.
 * This supplements exactly two reflected reciprocal arrays. Ordinary Physics7
 * actor interpolation consumes them. Never replay Matinee initialization,
 * receive, callbacks, seek, plot tracks, position/clock/rotation/Physics writes.
 *
 * Allocation: inline native Add uses slack71330 + ResizeAllocation73440.
 * Fresh temporary buffers avoid Resize overwriting a live Data on failure.
 * bf0d0(ptr,0) is the SAME global engine allocator's ordinary Free wrapper.
 * Backend vt12fef00: Realloc9bbeb0(ptr,0) calls IAT1668ef8 and returnsNULL;
 * Free9b94d0 tailcalls that same IAT. Optional lock wrapper vt12fefa8 forwards
 * to its backend+8. Runtime pins reject other allocator configurations.
 * Stage both complete replacements, revalidate identities and old contents,
 * commit both headers without engine calls, then free displaced buffers.
 * Native Matinee teardown removes the supplemented entries normally. Fallback
 * compacts only entries this epoch inserted; capacities remain engine-owned.
 */

typedef struct GuestCinematicOps {
    void *context;
    int (*read)(void*,uintptr_t,void*,size_t);
    int (*write)(void*,uintptr_t,const void*,size_t);
    int (*reserve)(void*,GameArray*,int);
    void (*release)(void*,uintptr_t);
    int (*validate)(void*);
} GuestCinematicOps;
typedef struct GuestCinematicArrayPlan {
    uintptr_t address,value;
    GameArray before,after;
    uintptr_t values[64];
    int inserted,committed;
} GuestCinematicArrayPlan;

static int guest_cinematic_array_snapshot(GuestCinematicOps *ops,uintptr_t address,
                                         GuestCinematicArrayPlan *p,uintptr_t value) {
    memset(p,0,sizeof *p);p->address=address;p->value=value;
    if(!ops->read(ops->context,address,&p->before,sizeof p->before)||
       p->before.count<0||p->before.count>64||p->before.max<p->before.count||
       p->before.max>4096||(p->before.max&&!p->before.data)||
       (!p->before.max&&p->before.data)||
       (p->before.count&&!ops->read(ops->context,p->before.data,p->values,(size_t)p->before.count*8)))return 0;
    int memberships=0;for(int i=0;i<p->before.count;++i)memberships+=p->values[i]==value;
    if(memberships>1)return 0;
    p->inserted=!memberships;p->after=p->before;return 1;
}
static int guest_cinematic_array_unchanged(GuestCinematicOps *ops,const GuestCinematicArrayPlan *p) {
    GameArray current={0};uintptr_t values[64];
    return ops->read(ops->context,p->address,&current,sizeof current)&&!memcmp(&current,&p->before,sizeof current)&&
        (!current.count||(ops->read(ops->context,current.data,values,(size_t)current.count*8)&&
        !memcmp(values,p->values,(size_t)current.count*8)));
}
static void guest_cinematic_stage_free(GuestCinematicOps *ops,GuestCinematicArrayPlan *p) {
    if(p->inserted&&p->after.data&&p->after.data!=p->before.data)ops->release(ops->context,p->after.data);
    p->after=p->before;
}
static int guest_cinematic_stage(GuestCinematicOps *ops,GuestCinematicArrayPlan *p) {
    if(!p->inserted)return 1;
    if(p->before.count>=64)return 0;
    memset(&p->after,0,sizeof p->after);
    if(!ops->reserve(ops->context,&p->after,p->before.count+1)||!p->after.data||
       p->after.max<p->before.count+1||p->after.max>128)return 0;
    uintptr_t values[65];memcpy(values,p->values,(size_t)p->before.count*8);values[p->before.count]=p->value;
    p->after.count=p->before.count+1;
    return ops->write(ops->context,p->after.data,values,(size_t)p->after.count*8);
}
/* 1 committed (including already reciprocal),0 clean rejection/rollback,
 * -2 retained ambiguous buffers: caller MUST suppress every future mutation.
 * No old buffer is released before both headers are positively committed.
 * A failed commit may be partially written; rollback is attempted only if
 * each observed header equals our old OR intended header. External changes
 * are never overwritten. Ambiguous buffers are retained rather than freed.
 */
static int guest_cinematic_pair_append(GuestCinematicOps *ops,uintptr_t a,uintptr_t av,
                                      uintptr_t b,uintptr_t bv,GuestCinematicArrayPlan plans[2]) {
    memset(plans,0,sizeof(GuestCinematicArrayPlan)*2);
    if(!guest_cinematic_array_snapshot(ops,a,&plans[0],av)||
       !guest_cinematic_array_snapshot(ops,b,&plans[1],bv))return 0;
    if(!plans[0].inserted&&!plans[1].inserted)return 1;
    if(!guest_cinematic_stage(ops,&plans[0])||!guest_cinematic_stage(ops,&plans[1])) {
        guest_cinematic_stage_free(ops,&plans[0]);guest_cinematic_stage_free(ops,&plans[1]);return 0;
    }
    if(!ops->validate(ops->context)||!guest_cinematic_array_unchanged(ops,&plans[0])||
       !guest_cinematic_array_unchanged(ops,&plans[1])) {
        guest_cinematic_stage_free(ops,&plans[0]);guest_cinematic_stage_free(ops,&plans[1]);return 0;
    }
    int ok=1;
    for(int i=0;i<2;++i)if(plans[i].inserted) {
        GameArray checked={0};
        if(!ops->write(ops->context,plans[i].address,&plans[i].after,sizeof(GameArray))||
           !ops->read(ops->context,plans[i].address,&checked,sizeof checked)||
           memcmp(&checked,&plans[i].after,sizeof checked)){ok=0;break;}
        plans[i].committed=1;
    }
    if(!ok) {
        for(int i=0;i<2;++i)if(plans[i].inserted) {
            GameArray observed={0};
            if(!ops->read(ops->context,plans[i].address,&observed,sizeof observed)||
               (memcmp(&observed,&plans[i].before,sizeof observed)&&
                memcmp(&observed,&plans[i].after,sizeof observed)))return -2;
        }
        for(int i=0;i<2;++i)if(plans[i].inserted) {
            GameArray observed={0};
            if(!ops->write(ops->context,plans[i].address,&plans[i].before,sizeof(GameArray))||
               !ops->read(ops->context,plans[i].address,&observed,sizeof observed)||
               memcmp(&observed,&plans[i].before,sizeof observed))return -2;
        }
        guest_cinematic_stage_free(ops,&plans[0]);guest_cinematic_stage_free(ops,&plans[1]);return 0;
    }
    for(int i=0;i<2;++i)if(plans[i].inserted&&plans[i].before.data)
        ops->release(ops->context,plans[i].before.data);
    return 1;
}
/* No allocation/shrink. Existing order and all other memberships survive.
 * 1 removed/absent,0 readable rejection,-2 ambiguous partial write.
 */
static int guest_cinematic_remove_owned(GuestCinematicOps *ops,uintptr_t address,uintptr_t value,int *removed) {
    GuestCinematicArrayPlan p;*removed=0;
    if(!guest_cinematic_array_snapshot(ops,address,&p,value))return 0;
    if(p.inserted)return 1;
    int at=0;while(at<p.before.count&&p.values[at]!=value)++at;
    uintptr_t after[64];memcpy(after,p.values,sizeof after);
    memmove(after+at,after+at+1,(size_t)(p.before.count-at-1)*8);
    int count=p.before.count-1;
    if(!ops->validate(ops->context)||!guest_cinematic_array_unchanged(ops,&p))return 0;
    if((count&&!ops->write(ops->context,p.before.data,after,(size_t)count*8))||
       !ops->write(ops->context,address+8,&count,4))return -2;
    GameArray checked={0};uintptr_t check[64];
    if(!ops->read(ops->context,address,&checked,sizeof checked)||checked.data!=p.before.data||
       checked.max!=p.before.max||checked.count!=count||
       (count&&(!ops->read(ops->context,checked.data,check,(size_t)count*8)||memcmp(check,after,(size_t)count*8))))return -2;
    *removed=1;return 1;
}

typedef struct GuestCinematicLease {
    int used,inserted_actor,inserted_action;
    unsigned long token;
    GuestCameraStamp pc,info,world,level,player,matinee,action,instance,group,camera,track,move,director,channel,connection,driver;
    int channel_index,track_index,actor_count_before,action_count_before;
    DWORD logged_at;
} GuestCinematicLease;
typedef struct GuestCinematicContext {uintptr_t base,engine,pc,info;GuestCinematicLease *lease;} GuestCinematicContext;
static GuestCinematicLease guest_cinematic_leases[16];
static unsigned long guest_cinematic_token;
static int guest_cinematic_suppressed;
static DWORD guest_cinematic_last_scan,guest_cinematic_last_rejection;

static int guest_cinematic_hex_pin(uintptr_t at,const char *hex) {
    unsigned char observed[192];size_t n=strlen(hex)/2;
    if(n>sizeof observed||!read_at(at,observed,n))return 0;
    for(size_t i=0;i<n;++i) {
        unsigned char a=(unsigned char)hex[i*2],b=(unsigned char)hex[i*2+1];
        unsigned char x=(unsigned char)(((a<='9'?a-'0':a-'a'+10)<<4)|(b<='9'?b-'0':b-'a'+10));
        if(observed[i]!=x)return 0;
    }
    return 1;
}
static int guest_cinematic_allocator_pins(uintptr_t base) {
    if(!guest_cinematic_hex_pin(base+0x71330,"418bc08bca8bd0458bc1e921050200")||
       !guest_cinematic_hex_pin(base+0x91860,"448bc93bca7d30448bd2442bd1418bc2410fafc03d00400000730a8d0c498d04123bc87d0b4183fa4077084585c974038bc2c3418bc1c34585c97e214183f9087f088d044d02000000c3418bc19983e20703c2c1f80383c0204103c1c333c0c3")||
       !guest_cinematic_hex_pin(base+0x73440,"40534883ec20488bd9418bc0488b094885c9750485c07415410fafc14533c9458d41108bd0e826bd04004889034883c4205bc3")||
       !guest_cinematic_hex_pin(base+0xbf190,"48895c240848896c24104889742418574883ec30488be9418bd9488b0dd7e37001418bf88bf24885c9750ce8a0a38f00488b0dc1e37001488b01448bcf448bc6895c2420488bd5ff5010488b5c2440488b6c2448488b7424504883c4305fc3")||
       !guest_cinematic_hex_pin(base+0xbf0d0,"48895c2408574883ec20488bf98bda488b0da2e470014885c9750ce870a48f00488b0d91e47001488b01448bc3488bd7488b5c24304883c4205f48ff6018")||
       !guest_cinematic_hex_pin(base+0x9bbeb0,"4883ec28488bc2453bc876120f1f40004183f901760841d1e9453bc877f24885c07429488bc84585c07414418bd041b8100000004883c42848ff2529d06a00ff1503d06a0033c04883c428c3418bc8ba100000004883c42848ff25f1cf6a00")||
       !guest_cinematic_hex_pin(base+0x9b94d0,"488bca48ff251efa6a00"))return 0;
    uintptr_t heap=read_ptr(base+0x17cd588),vt=read_ptr(heap);
    if(vt==base+0x12fefa8) {
        if(read_ptr(vt+0x10)!=base+0x9bbf10||read_ptr(vt+0x18)!=base+0x9b94e0||
           !guest_cinematic_hex_pin(base+0x9bbf10,"40565741564883ec4048c7442430feffffff48895c246848896c2470418bf1418be84c8bf2488bd9488d41104889442460488d7918488bcfff155ac26a0085c0750a488bcfff1535c26a0090488b4b08488b014c8b50108b84248000000089442420448bce448bc5498bd641ffd2488bd8488bcfff150ec26a00488bc3488b5c2468488b6c24704883c440415e5f5ec3")||
           !guest_cinematic_hex_pin(base+0x9b94e0,"4885d27477574883ec3048c7442420feffffff48895c244048896c24504889742458418be8488bf2488bf9488d411048894424484883c118ff158aec6a0085c0750b488d4f18ff1564ec6a0090488b4f08488b01448bc5488bd6ff501890488d4f18ff1550ec6a00488b5c2440488b6c2450488b7424584883c4305fc3"))return 0;
        heap=read_ptr(heap+8);vt=read_ptr(heap);
    }
    return heap&&vt==base+0x12fef00&&read_ptr(vt+0x10)==base+0x9bbeb0&&read_ptr(vt+0x18)==base+0x9b94d0;
}
static int guest_cinematic_live_context(GuestCinematicContext *c) {
    unsigned char net=255;CombatContextChain chain;
    if(!c->base||local_controller(c->engine)!=c->pc||!guest_camera_actor_live(c->base,c->pc)||
       !is_kind_of(c->base,c->pc,"BioPlayerController")||!guest_camera_actor_live(c->base,c->info)||
       !is_kind_of(c->base,c->info,"BioWorldInfo")||read_ptr(c->pc+0x198)!=c->info||
       !read_at(c->info+0x7c8,&net,1)||net!=3)return 0;
    uintptr_t player=read_ptr(c->pc+0x67c);
    if(!narrative_probe_object(c->base,player,"LocalPlayer")||read_ptr(player+0x68)!=c->pc)return 0;
    combat_context_read_chain(c->base,&chain);
    return chain.world_valid&&chain.level_valid&&chain.info_valid&&chain.info==c->info;
}
static int guest_cinematic_member(uintptr_t at,uintptr_t value,int limit) {
    GameArray a={0};uintptr_t values[64];int found=0;
    if(limit>64||!scene_readiness_array(at,&a,limit)||
       (a.count&&!read_at(a.data,values,(size_t)a.count*8)))return -1;
    for(int i=0;i<a.count;++i)found+=values[i]==value;
    return found;
}
static int guest_cinematic_lease_scope(GuestCinematicContext *c,int binding) {
    GuestCinematicLease *l=c->lease;CombatContextChain chain;
    if(!l||!guest_cinematic_live_context(c)||l->pc.pointer!=c->pc||l->info.pointer!=c->info)return 0;
    GuestCameraStamp *stamps[]={&l->pc,&l->info,&l->world,&l->level,&l->player,&l->matinee,&l->action,
        &l->instance,&l->group,&l->camera,&l->track,&l->move,&l->director,&l->channel,&l->connection,&l->driver};
    for(size_t i=0;i<sizeof stamps/sizeof stamps[0];++i)if(!guest_camera_stamp_matches(c->base,stamps[i],1))return 0;
    combat_context_read_chain(c->base,&chain);
    if(chain.world!=l->world.pointer||chain.level!=l->level.pointer||chain.info!=l->info.pointer||
       read_ptr(c->pc+0x67c)!=l->player.pointer||
       !guest_camera_actor_live(c->base,l->camera.pointer)||!guest_camera_actor_live(c->base,l->matinee.pointer)||
       read_ptr(l->camera.pointer+0x198)!=c->info||read_ptr(l->matinee.pointer+0x198)!=c->info)return 0;
    if(!binding)return 1;
    NetworkActorIdentityEntry channel={0};unsigned char physics=255,role=255,remote=255,condition=255,frame=255,rotation=255;
    uint32_t action_flags=0,matinee_flags=0,common=0,move_flags=0;float a_position=0,m_position=0;
    GameArray instances={0},tracks={0},moves={0};
    uintptr_t a=l->action.pointer,i=l->instance.pointer,g=l->group.pointer,t=l->track.pointer;
    if(!network_actor_channel_valid(c->base,l->matinee.pointer,l->channel.pointer,l->driver.pointer,&channel)||
       channel.connection!=l->connection.pointer||channel.index!=l->channel_index||
       read_ptr(l->matinee.pointer+0x270)!=a||!read_at(a+0x180,&action_flags,4)||!(action_flags&2)||(action_flags&4)||
       !read_at(l->matinee.pointer+0x284,&matinee_flags,4)||!(matinee_flags&1)||(matinee_flags&4)||
       !read_at(a+0x170,&a_position,4)||!read_at(l->matinee.pointer+0x27c,&m_position,4)||
       !isfinite(a_position)||!isfinite(m_position)||
       read_ptr(c->pc+0x6ac)!=l->director.pointer||
       !scene_readiness_array(a+0x12c,&instances,32)||guest_cinematic_member(a+0x12c,i,32)!=1||
       read_ptr(i+0x70)!=g||read_ptr(i+0x78)!=l->camera.pointer||
       !scene_readiness_array(g+0x68,&tracks,64)||!scene_readiness_array(i+0x60,&moves,64)||
       l->track_index<0||l->track_index>=tracks.count||l->track_index>=moves.count||
       read_ptr(tracks.data+(size_t)l->track_index*8)!=t||
       read_ptr(moves.data+(size_t)l->track_index*8)!=l->move.pointer||
       !read_at(l->camera.pointer+0x268,&physics,1)||physics!=7||
       !read_at(l->camera.pointer+0x26a,&role,1)||role!=3||
       !read_at(l->camera.pointer+0x269,&remote,1)||remote!=0||
       !read_at(t+0x8c,&common,4)||(common&4)||!read_at(t+0xd8,&move_flags,4)||(move_flags&4)||
       !read_at(t+0x78,&condition,1)||condition!=0||!read_at(t+0xdc,&frame,1)||frame!=0||
       !read_at(t+0xdd,&rotation,1)||rotation!=0)return 0;
    int directors=0;
    for(int j=0;j<instances.count;++j) {
        uintptr_t gi=read_ptr(instances.data+(size_t)j*8);
        if(!narrative_probe_object(c->base,gi,"InterpGroupInst"))return 0;
        int n=guest_cinematic_member(gi+0x60,l->director.pointer,64);
        if(n<0)return 0;directors+=n;
    }
    return directors==1;
}
static int guest_cinematic_ops_read(void *context,uintptr_t at,void *out,size_t size) {
    (void)context;return read_at(at,out,size);
}
static int guest_cinematic_writable(uintptr_t at,size_t size) {
    if(!at||!size||at+size<at)return 0;
    MEMORY_BASIC_INFORMATION m;uintptr_t cursor=at,end=at+size;
    while(cursor<end) {
        if(!VirtualQuery((void*)cursor,&m,sizeof m)||m.State!=MEM_COMMIT||
           (m.Protect&(PAGE_GUARD|PAGE_NOACCESS))||
           !(m.Protect&(PAGE_READWRITE|PAGE_WRITECOPY|PAGE_EXECUTE_READWRITE|PAGE_EXECUTE_WRITECOPY)))return 0;
        uintptr_t next=(uintptr_t)m.BaseAddress+m.RegionSize;if(next<=cursor)return 0;cursor=next;
    }
    return 1;
}
static int guest_cinematic_ops_write(void *context,uintptr_t at,const void *value,size_t size) {
    (void)context;SIZE_T wrote=0;
    return guest_cinematic_writable(at,size)&&WriteProcessMemory(GetCurrentProcess(),(void*)at,value,size,&wrote)&&wrote==size;
}
static int guest_cinematic_ops_reserve(void *context,GameArray *a,int count) {
    GuestCinematicContext *c=context;
    typedef int32_t (*Slack)(GameArray*,int32_t,int32_t,int32_t);
    typedef void (*Resize)(GameArray*,int32_t,int32_t,int32_t);
    if(a->data||a->count||a->max||count<=0||count>64||!guest_cinematic_allocator_pins(c->base))return 0;
    int capacity=((Slack)(c->base+0x71330))(a,count,0,8);
    if(capacity<count||capacity>128)return 0;
    ((Resize)(c->base+0x73440))(a,0,capacity,8);a->max=capacity;
    return a->data&&guest_cinematic_writable(a->data,(size_t)capacity*8);
}
static void guest_cinematic_ops_release(void *context,uintptr_t data) {
    GuestCinematicContext *c=context;typedef void (*EngineFree)(void*,uint32_t);
    if(data)((EngineFree)(c->base+0xbf0d0))((void*)data,0);
}
static int guest_cinematic_ops_validate(void *context) {
    GuestCinematicContext *c=context;
    return guest_cinematic_allocator_pins(c->base)&&guest_cinematic_lease_scope(c,1)&&
        guest_cinematic_writable(c->lease->camera.pointer+0xf0,sizeof(GameArray))&&
        guest_cinematic_writable(c->lease->action.pointer+0xf8,sizeof(GameArray));
}
static int guest_cinematic_ops_cleanup_validate(void *context) {
    GuestCinematicContext *c=context;return guest_cinematic_lease_scope(c,0);
}
static GuestCinematicOps guest_cinematic_ops(GuestCinematicContext *c) {
    GuestCinematicOps ops={c,guest_cinematic_ops_read,guest_cinematic_ops_write,
        guest_cinematic_ops_reserve,guest_cinematic_ops_release,guest_cinematic_ops_validate};return ops;
}
static void guest_cinematic_log(GuestCinematicLease *l,const char *event,const char *reason,int verified,DWORD now,
                                int camera_memberships,int action_memberships,int removed_camera,int removed_action) {
    char row[5300],camera[1200],action[1200];
    identity_quote(l->camera.full_name,camera,sizeof camera);identity_quote(l->action.full_name,action,sizeof action);
    snprintf(row,sizeof row,"{\"guest_cinematic_registration_adapter\":\"%s\",\"reason\":\"%s\",\"lease_token\":%lu,\"verified\":%s,\"camera_pointer\":\"0x%llx\",\"camera_index\":%d,\"camera_path\":\"%s\",\"action_pointer\":\"0x%llx\",\"action_index\":%d,\"action_path\":\"%s\",\"world_info\":\"0x%llx\",\"matinee_pointer\":\"0x%llx\",\"group_instance\":\"0x%llx\",\"move_instance\":\"0x%llx\",\"actor_channel_index\":%d,\"actor_latent_count_before\":%d,\"action_latent_count_before\":%d,\"inserted_actor_action\":%s,\"inserted_action_actor\":%s,\"camera_current_action_memberships\":%d,\"action_current_camera_memberships\":%d,\"removed_actor_action\":%s,\"removed_action_actor\":%s,\"camera_motion_verified\":false,\"campaign_coop_verified\":false,\"monotonic_ms\":%lu}\r\n",
        event,reason,l->token,verified?"true":"false",(unsigned long long)l->camera.pointer,l->camera.index,camera,
        (unsigned long long)l->action.pointer,l->action.index,action,(unsigned long long)l->info.pointer,
        (unsigned long long)l->matinee.pointer,(unsigned long long)l->instance.pointer,(unsigned long long)l->move.pointer,
        l->channel_index,l->actor_count_before,l->action_count_before,l->inserted_actor?"true":"false",l->inserted_action?"true":"false",
        camera_memberships,action_memberships,removed_camera?"true":"false",removed_action?"true":"false",now);line(row);
}
static int guest_cinematic_capture(GuestCinematicContext *c,uintptr_t matinee,uintptr_t action,uintptr_t instance,
        uintptr_t group,uintptr_t camera,uintptr_t track,uintptr_t move,int track_index,
        const NetworkActorIdentityEntry *channel,GuestCinematicLease *out) {
    memset(out,0,sizeof *out);uintptr_t director=read_ptr(c->pc+0x6ac);CombatContextChain chain;
    combat_context_read_chain(c->base,&chain);
    if(!narrative_probe_object(c->base,director,"InterpTrackInstDirector"))return 0;
    uintptr_t objects[]={c->pc,c->info,chain.world,chain.level,read_ptr(c->pc+0x67c),matinee,action,instance,group,camera,track,move,
        director,channel->channel,channel->connection,channel->driver};
    GuestCameraStamp *stamps[]={&out->pc,&out->info,&out->world,&out->level,&out->player,&out->matinee,&out->action,
        &out->instance,&out->group,&out->camera,&out->track,&out->move,&out->director,&out->channel,&out->connection,&out->driver};
    for(size_t i=0;i<sizeof objects/sizeof objects[0];++i)if(!guest_camera_stamp_read(c->base,objects[i],stamps[i],1))return 0;
    out->channel_index=channel->index;out->track_index=track_index;c->lease=out;
    return guest_cinematic_lease_scope(c,1);
}
static void guest_cinematic_cleanup(GuestCinematicContext *c,GuestCinematicLease *l,DWORD now,const char *reason) {
    c->lease=l;
    if(!guest_cinematic_lease_scope(c,0)) {
        guest_cinematic_log(l,"cleanup-expired","captured-identity-or-current-world-invalid",0,now,-1,-1,0,0);
        memset(l,0,sizeof *l);return;
    }
    GuestCinematicOps ops=guest_cinematic_ops(c);ops.validate=guest_cinematic_ops_cleanup_validate;
    int ra=0,rb=0,a=1,b=1;
    if(l->inserted_actor)a=guest_cinematic_remove_owned(&ops,l->camera.pointer+0xf0,l->action.pointer,&ra);
    if(a>0&&l->inserted_action)b=guest_cinematic_remove_owned(&ops,l->action.pointer+0xf8,l->camera.pointer,&rb);
    int ca=guest_cinematic_member(l->camera.pointer+0xf0,l->action.pointer,64);
    int ac=guest_cinematic_member(l->action.pointer+0xf8,l->camera.pointer,64);
    int verified=a>0&&b>0&&(!l->inserted_actor||ca==0)&&(!l->inserted_action||ac==0);
    guest_cinematic_log(l,verified?"cleanup-complete":"cleanup-rejected",reason,verified,now,ca,ac,ra,rb);
    if(a<0||b<0)guest_cinematic_suppressed=1;
    if(verified)memset(l,0,sizeof *l);
}
static void guest_cinematic_registration_adapt(uintptr_t base,uintptr_t engine,uintptr_t pc,uintptr_t info,int enabled,DWORD now) {
    GuestCinematicContext c={base,engine,pc,info,NULL};
    int context=guest_cinematic_live_context(&c);
    for(size_t k=0;k<16;++k)if(guest_cinematic_leases[k].used) {
        GuestCinematicLease *l=&guest_cinematic_leases[k];c.lease=l;
        int held=enabled&&context&&!guest_cinematic_suppressed&&guest_cinematic_lease_scope(&c,1);
        int ca=held?guest_cinematic_member(l->camera.pointer+0xf0,l->action.pointer,64):-1;
        int ac=held?guest_cinematic_member(l->action.pointer+0xf8,l->camera.pointer,64):-1;
        if(!held||ca!=1||ac!=1) {
            if(!guest_cinematic_suppressed)guest_cinematic_cleanup(&c,l,now,enabled?"ordinary-binding-ended":"opt-in-disabled");
        } else if(!l->logged_at||now-l->logged_at>=1000) {
            l->logged_at=now;guest_cinematic_log(l,"held","ordinary-current-playing-binding",1,now,ca,ac,0,0);
        }
    }
    if(!enabled||!context||guest_cinematic_suppressed||test_action!=5||
       (guest_cinematic_last_scan&&now-guest_cinematic_last_scan<100))return;
    guest_cinematic_last_scan=now;
    if(!guest_cinematic_allocator_pins(base)||!network_actor_layout_matches(base)||
       read_ptr(base+0x11a2150+0x2b0)!=base+0x4840a0||!network_actor_received_handler_matches(base))return;
    uintptr_t driver=0,connection=0;
#ifdef ME3_SQUAD_COOP_ACTOR_CHANNEL_DIAGNOSTICS_H
    if(actor_channel_diagnostic_base==base&&actor_channel_diagnostic_world.object==info&&actor_channel_diagnostic_scope_matches(base)) {
        driver=actor_channel_diagnostic_driver.object;connection=actor_channel_diagnostic_connection.object;
    }
#endif
    if(!driver||!connection||!network_actor_connection_in_scope(base,driver,connection))return;
    GameArray open={0};uintptr_t channels[2048];
    if(!scene_readiness_array(connection+0x5f90,&open,2048)||
       (open.count&&!read_at(open.data,channels,(size_t)open.count*8)))return;
    int matinees=0;
    for(int n=0;n<open.count&&matinees<8;++n) {
        uintptr_t channel=channels[n],matinee=read_ptr(channel+0xa0);NetworkActorIdentityEntry verified={0};
        if(!guest_camera_actor_live(base,matinee)||!is_kind_of(base,matinee,"MatineeActor")||
           !network_actor_channel_valid(base,matinee,channel,driver,&verified)||verified.connection!=connection)continue;
        ++matinees;uintptr_t action=read_ptr(matinee+0x270);GameArray instances={0};
        if(!narrative_probe_object(base,action,"SeqAct_Interp")||!scene_readiness_array(action+0x12c,&instances,32))continue;
        int cameras=0;
        for(int gi=0;gi<instances.count&&cameras<8;++gi) {
            uintptr_t instance=read_ptr(instances.data+(size_t)gi*8),group=read_ptr(instance+0x70),camera=read_ptr(instance+0x78);
            if(!narrative_probe_object(base,instance,"InterpGroupInst")||!narrative_probe_object(base,group,"InterpGroup")||
               !guest_camera_actor_live(base,camera))continue;
            char klass[96];object_name(base,read_ptr(camera+0x50),klass,sizeof klass);if(strcmp(klass,"CameraActor"))continue;
            ++cameras;int existing=0,slot=-1;
            for(int k=0;k<16;++k) {
                if(!guest_cinematic_leases[k].used&&slot<0)slot=k;
                if(guest_cinematic_leases[k].used&&guest_cinematic_leases[k].action.pointer==action&&
                   guest_cinematic_leases[k].camera.pointer==camera)existing=1;
            }
            if(existing||slot<0)continue;
            GameArray tracks={0},moves={0};
            if(!scene_readiness_array(group+0x68,&tracks,64)||!scene_readiness_array(instance+0x60,&moves,64))continue;
            for(int ti=0;ti<tracks.count&&ti<moves.count;++ti) {
                uintptr_t track=read_ptr(tracks.data+(size_t)ti*8),move=read_ptr(moves.data+(size_t)ti*8);
                if(!narrative_probe_object(base,track,"InterpTrackMove")||!narrative_probe_object(base,move,"InterpTrackInstMove"))continue;
                GuestCinematicLease candidate;
                if(!guest_cinematic_capture(&c,matinee,action,instance,group,camera,track,move,ti,&verified,&candidate)) {
                    if(!guest_cinematic_last_rejection||now-guest_cinematic_last_rejection>=5000) {
                        guest_cinematic_last_rejection=now;
                        guest_cinematic_log(&candidate,"candidate-rejected","current-playing-camera-binding-guard",0,now,-1,-1,0,0);
                    }
                    break;
                }
                candidate.token=++guest_cinematic_token;GuestCinematicOps ops=guest_cinematic_ops(&c);GuestCinematicArrayPlan plans[2];
                memset(plans,0,sizeof plans);
                int result=guest_cinematic_pair_append(&ops,camera+0xf0,action,action+0xf8,camera,plans);
                candidate.actor_count_before=plans[0].before.count;candidate.action_count_before=plans[1].before.count;
                candidate.inserted_actor=plans[0].inserted;candidate.inserted_action=plans[1].inserted;
                int ca=guest_cinematic_member(camera+0xf0,action,64),ac=guest_cinematic_member(action+0xf8,camera,64);
                if(result==1&&(candidate.inserted_actor||candidate.inserted_action)) {
                    candidate.used=1;candidate.logged_at=now;guest_cinematic_leases[slot]=candidate;
                    guest_cinematic_log(&candidate,ca==1&&ac==1?"applied":"apply-readback-rejected",
                        "reciprocal-native-array-supplement",ca==1&&ac==1,now,ca,ac,0,0);
                } else if(result<0) {
                    guest_cinematic_suppressed=1;guest_cinematic_log(&candidate,"suppressed","ambiguous-header-commit-retained-buffers",0,now,ca,ac,0,0);return;
                } else if(!guest_cinematic_last_rejection||now-guest_cinematic_last_rejection>=5000) {
                    guest_cinematic_last_rejection=now;guest_cinematic_log(&candidate,"apply-rejected",result?"already-reciprocal":"staging-or-validation-failed",0,now,ca,ac,0,0);
                }
                break;
            }
        }
    }
}

/* Real transaction helpers with deterministic fake engine allocation. Never
 * used in-game. This proves reciprocal order preservation and failure paths;
 * it does not claim native allocator or cinematic runtime success. */
typedef struct GuestCinematicMock {
    GameArray a,b;uintptr_t initial_a[8],initial_b[8],buffers[8][128];
    int allocations,releases,fail_allocation,fail_commit,commit_writes,validation,external_change;
    unsigned int released;
} GuestCinematicMock;
static int guest_cinematic_mock_read(void *context,uintptr_t at,void *out,size_t size) {(void)context;memcpy(out,(void*)at,size);return 1;}
static int guest_cinematic_mock_write(void *context,uintptr_t at,const void *value,size_t size) {
    GuestCinematicMock *m=context;
    if(size==sizeof(GameArray)&&(at==(uintptr_t)&m->a||at==(uintptr_t)&m->b)) {
        ++m->commit_writes;if(m->fail_commit==m->commit_writes)return 0;
    }
    memcpy((void*)at,value,size);return 1;
}
static int guest_cinematic_mock_reserve(void *context,GameArray *a,int count) {
    GuestCinematicMock *m=context;int n=++m->allocations;
    if(n==m->fail_allocation||n>8)return 0;
    a->data=(uintptr_t)m->buffers[n-1];a->max=count;return 1;
}
static void guest_cinematic_mock_release(void *context,uintptr_t data) {
    GuestCinematicMock *m=context;++m->releases;
    for(int i=0;i<8;++i)if(data==(uintptr_t)m->buffers[i]) {
        if(m->released&(1u<<i))m->releases=-100;
        m->released|=1u<<i;return;
    }
    if(data!=(uintptr_t)m->initial_a&&data!=(uintptr_t)m->initial_b)m->releases=-100;
}
static int guest_cinematic_mock_validate(void *context) {
    GuestCinematicMock *m=context;
    if(m->external_change)m->initial_a[0]=999;
    return m->validation;
}
static void guest_cinematic_mock_init(GuestCinematicMock *m,GuestCinematicOps *ops) {
    memset(m,0,sizeof *m);m->validation=1;m->initial_a[0]=11;m->initial_a[1]=12;m->initial_b[0]=21;
    m->a=(GameArray){(uintptr_t)m->initial_a,2,8};m->b=(GameArray){(uintptr_t)m->initial_b,1,8};
    *ops=(GuestCinematicOps){m,guest_cinematic_mock_read,guest_cinematic_mock_write,
        guest_cinematic_mock_reserve,guest_cinematic_mock_release,guest_cinematic_mock_validate};
}
static int guest_cinematic_registration_selftest(void) {
    GuestCinematicMock m;GuestCinematicOps ops;GuestCinematicArrayPlan p[2];int removed=0;
    guest_cinematic_mock_init(&m,&ops);
    if(guest_cinematic_pair_append(&ops,(uintptr_t)&m.a,31,(uintptr_t)&m.b,41,p)!=1||
       m.a.count!=3||m.b.count!=2||m.releases!=2||
       ((uintptr_t*)m.a.data)[0]!=11||((uintptr_t*)m.a.data)[1]!=12||((uintptr_t*)m.a.data)[2]!=31||
       ((uintptr_t*)m.b.data)[0]!=21||((uintptr_t*)m.b.data)[1]!=41)return 0;
    int allocations=m.allocations,releases=m.releases;
    if(guest_cinematic_pair_append(&ops,(uintptr_t)&m.a,31,(uintptr_t)&m.b,41,p)!=1||p[0].inserted||p[1].inserted||
       m.allocations!=allocations||m.releases!=releases)return 0;
    if(guest_cinematic_remove_owned(&ops,(uintptr_t)&m.a,31,&removed)!=1||!removed||m.a.count!=2||
       ((uintptr_t*)m.a.data)[0]!=11||((uintptr_t*)m.a.data)[1]!=12)return 0;
    if(guest_cinematic_remove_owned(&ops,(uintptr_t)&m.b,41,&removed)!=1||!removed||m.b.count!=1||((uintptr_t*)m.b.data)[0]!=21)return 0;
    if(guest_cinematic_remove_owned(&ops,(uintptr_t)&m.b,41,&removed)!=1||removed)return 0;
    guest_cinematic_mock_init(&m,&ops);m.fail_allocation=2;
    if(guest_cinematic_pair_append(&ops,(uintptr_t)&m.a,31,(uintptr_t)&m.b,41,p)!=0||
       m.a.data!=(uintptr_t)m.initial_a||m.b.data!=(uintptr_t)m.initial_b||m.a.count!=2||m.b.count!=1||m.releases!=1)return 0;
    guest_cinematic_mock_init(&m,&ops);m.fail_commit=2;
    if(guest_cinematic_pair_append(&ops,(uintptr_t)&m.a,31,(uintptr_t)&m.b,41,p)!=0||
       m.a.data!=(uintptr_t)m.initial_a||m.b.data!=(uintptr_t)m.initial_b||m.a.count!=2||m.b.count!=1||m.releases!=2)return 0;
    guest_cinematic_mock_init(&m,&ops);m.initial_a[2]=31;m.a.count=3;
    if(guest_cinematic_pair_append(&ops,(uintptr_t)&m.a,31,(uintptr_t)&m.b,41,p)!=1||p[0].inserted||!p[1].inserted||
       m.a.data!=(uintptr_t)m.initial_a||m.a.count!=3||m.allocations!=1||m.releases!=1)return 0;
    guest_cinematic_mock_init(&m,&ops);m.external_change=1;
    if(guest_cinematic_pair_append(&ops,(uintptr_t)&m.a,31,(uintptr_t)&m.b,41,p)!=0||
       m.a.data!=(uintptr_t)m.initial_a||m.initial_a[0]!=999||m.a.count!=2||m.releases!=2)return 0;
    guest_cinematic_mock_init(&m,&ops);m.initial_a[2]=31;m.initial_a[3]=31;m.a.count=4;
    if(guest_cinematic_pair_append(&ops,(uintptr_t)&m.a,31,(uintptr_t)&m.b,41,p)!=0||m.allocations)return 0;
    guest_cinematic_mock_init(&m,&ops);m.initial_a[1]=31;m.initial_a[2]=12;m.a.count=3;
    if(guest_cinematic_remove_owned(&ops,(uintptr_t)&m.a,31,&removed)!=1||!removed||m.a.count!=2||
       m.initial_a[0]!=11||m.initial_a[1]!=12)return 0;
    guest_cinematic_mock_init(&m,&ops);m.validation=0;
    if(guest_cinematic_pair_append(&ops,(uintptr_t)&m.a,31,(uintptr_t)&m.b,41,p)!=0||
       m.a.data!=(uintptr_t)m.initial_a||m.b.data!=(uintptr_t)m.initial_b||m.releases!=2||m.commit_writes)return 0;
    guest_cinematic_mock_init(&m,&ops);
    for(int i=0;i<63;++i)m.buffers[7][i]=(uintptr_t)(100+i);
    m.a=(GameArray){(uintptr_t)m.buffers[7],63,128};
    if(guest_cinematic_pair_append(&ops,(uintptr_t)&m.a,31,(uintptr_t)&m.b,41,p)!=1||m.a.count!=64||
       guest_cinematic_remove_owned(&ops,(uintptr_t)&m.a,31,&removed)!=1||!removed||m.a.count!=63)return 0;
    return 1;
}
#endif
