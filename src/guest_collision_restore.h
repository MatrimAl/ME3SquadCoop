#ifndef ME3_GUEST_COLLISION_RESTORE_H
#define ME3_GUEST_COLLISION_RESTORE_H
/* Default-off repair of an observed release/reacquire collision loss. Never
 * derive permission from a pawn name alone, reassert during a scene, move an
 * actor, or restore the broken collision state when this repair is disabled.
 * Include after guest_player_context_adapter.h. Call on the game thread.
 * enabled means the session lifetime is active, including scenes/PlayersOnly;
 * ordinary is the separate per-frame permission to acquire/repair. */
typedef struct GuestCollisionShape {uint32_t actor,block,component;unsigned char type,replicated;} GuestCollisionShape;
typedef struct GuestCollisionRestore {
    int configured,opted,tracked,released,pending,was_owned,suppressed;unsigned long token,attempted;
    GuestCameraStamp pc,pawn,world,component,cdo,template,archetype,pawn_archetype,channel,connection,driver;
    NetworkActorIdentityEntry network;
    GuestCollisionShape healthy,release;
    GuestPlayerContextMethod method;
    DWORD reported;
} GuestCollisionRestore;
static GuestCollisionRestore guest_collision_restore;
static int guest_collision_equal(const GuestCollisionShape *a,const GuestCollisionShape *b) {
    return a->actor==b->actor&&a->block==b->block&&a->component==b->component&&
        a->type==b->type&&a->replicated==b->replicated;
}
static int guest_collision_healthy(const GuestCollisionShape *s) {
    /* Actual supported Ashley defaults: COLLIDE_BlockAllButWeapons. */
    /* COLLIDE_MAX (10) is the observed initial replicated-byte sentinel,
     * including the supported class default. It is never a native argument. */
    return s&&s->actor==0x60000000&&s->block==1&&s->component==0xaa&&s->type==6&&s->replicated<=10;
}
static int guest_collision_physical_equal(const GuestCollisionShape *a,const GuestCollisionShape *b) {
    /* The CDO's transient replicated-type byte is not a physical default. */
    return a->actor==b->actor&&a->block==b->block&&a->component==b->component&&a->type==b->type;
}
static int guest_collision_disabled(const GuestCollisionShape *s) {
    /* Measured owned return: world collision survives, actor collision does not;
     * the blocking bit can be absent or retained. Both exact states were
     * captured with the healthy cylinder response. Native type6 preserves
     * existing world/blocking bits. Other partial shapes remain refused. */
    if(s&&s->actor==0x40000000)return (s->block==0||s->block==1)&&s->component==0xaa&&s->type==0&&s->replicated==0;
    /* Actual landing release: actor collision stays enabled, but world and
     * blocking collision are absent. Admit only the observed type/response,
     * not arbitrary partial collision states. */
    if(s&&s->actor==0x20000000)return s->block==0&&s->component==0x12&&s->type==8&&s->replicated==8;
    /* The engine can disable Actor collision while its cylinder retains the
     * captured class-default response0xaa. Missing Actor world collision is
     * required for this fully disabled Actor case. */
    return s&&s->actor==0&&s->block==0&&(!(s->component&2)||s->component==0xaa)&&s->type<10&&s->replicated<=10;
}
static int guest_collision_shape(uintptr_t pawn,uintptr_t component,GuestCollisionShape *s) {
    uint32_t a=0,b=0,c=0;unsigned char type[2];
    if(!read_at(pawn+0x260,&a,4)||!read_at(pawn+0x264,&b,4)||
       !read_at(component+0x16c,&c,4)||!read_at(pawn+0x26b,type,2))return 0;
    /* Reflection identifies only mask0x1 as bBlockActors. Adjacent mask0x2 is
     * outside this collision contract and must remain untouched. */
    *s=(GuestCollisionShape){a&0x60000000,b&1,c&0xba,type[0],type[1]};return 1;
}
static int guest_collision_world_compare(uintptr_t pawn,uint32_t expected,int enable) {
    uintptr_t at=pawn+0x260;MEMORY_BASIC_INFORMATION m={0};
    if((at&3)||!VirtualQuery((void*)at,&m,sizeof m)||m.State!=MEM_COMMIT||
       (m.Protect&(PAGE_GUARD|PAGE_NOACCESS))||at<(uintptr_t)m.BaseAddress||
       at+4<at||at+4>(uintptr_t)m.BaseAddress+m.RegionSize)return 0;
    DWORD p=m.Protect&0xff;
    if(p!=PAGE_READWRITE&&p!=PAGE_WRITECOPY&&p!=PAGE_EXECUTE_READWRITE&&p!=PAGE_EXECUTE_WRITECOPY)return 0;
    uint32_t next=enable?expected|0x40000000:expected&~UINT32_C(0x40000000);
    return (uint32_t)InterlockedCompareExchange((LONG volatile*)at,(LONG)next,(LONG)expected)==expected;
}
static void guest_collision_log(const char *event,const char *reason,DWORD now,int healthy) {
    GuestCollisionRestore *l=&guest_collision_restore;char row[500];
    snprintf(row,sizeof row,"{\"guest_collision_restore\":\"%s\",\"reason\":\"%s\",\"lease_token\":%lu,\"channel_index\":%d,\"healthy_collision_observed\":%s,\"position_written\":false,\"combat_verified\":false,\"monotonic_ms\":%lu}\r\n",
        event,reason,l->token,l->network.index,healthy?"true":"false",now);line(row);
}
static int guest_collision_identity(uintptr_t base) {
    GuestCollisionRestore *l=&guest_collision_restore;
    return l->tracked&&guest_camera_stamp_matches(base,&l->pc,1)&&guest_camera_actor_live(base,l->pc.pointer)&&
        guest_camera_stamp_matches(base,&l->pawn,1)&&guest_camera_actor_live(base,l->pawn.pointer)&&
        guest_camera_stamp_matches(base,&l->world,1)&&guest_camera_actor_live(base,l->world.pointer)&&
        guest_camera_stamp_matches(base,&l->component,1)&&guest_camera_stamp_matches(base,&l->cdo,1)&&
        guest_camera_stamp_matches(base,&l->template,1)&&guest_camera_stamp_matches(base,&l->channel,1)&&
        guest_camera_stamp_matches(base,&l->archetype,1)&&guest_camera_stamp_matches(base,&l->pawn_archetype,1)&&
        guest_camera_stamp_matches(base,&l->connection,1)&&guest_camera_stamp_matches(base,&l->driver,1)&&
        read_ptr(l->pc.pointer+0x198)==l->world.pointer&&read_ptr(l->pawn.pointer+0x198)==l->world.pointer&&
        read_ptr(l->pawn.pointer+0x1e0)==l->component.pointer&&read_ptr(l->pawn.klass+0x13c)==l->cdo.pointer&&
        read_ptr(l->cdo.pointer+0x1e0)==l->template.pointer&&read_ptr(l->component.pointer+0x58)==l->archetype.pointer&&
        read_ptr(l->pawn.pointer+0x58)==l->pawn_archetype.pointer&&
        read_ptr(l->archetype.pointer+0x40)==l->pawn_archetype.pointer&&
        l->archetype.klass==l->component.klass&&l->pawn_archetype.klass==l->pawn.klass;
}
static int guest_collision_method(uintptr_t base,GuestPlayerContextMethod *method) {
    uintptr_t fn=function_named(base,"Actor","SetCollisionType"),field=read_ptr(fn+0x70);
    static const unsigned char entry[]={0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20,0x48,0x8b,0x42,0x28};
    static const unsigned char call[]={0x0f,0xb6,0x54,0x24,0x38,0x48,0x8b,0xcf,0xe8,0xe4,0x55,0xf5,0xff};
    unsigned char actual[16];uint64_t flags=0;int offset=-1,size=0,dim=0;char name[96];
    if(!guest_player_context_function_abi(base,fn,"Actor","SetCollisionType",0x20401,1,65535,0,0x4ba0c0,1)||
       !read_at(base+0x4ba0c0,actual,sizeof entry)||memcmp(actual,entry,sizeof entry)||
       !read_at(base+0x4ba11f,actual,sizeof call)||memcmp(actual,call,sizeof call)||
       !narrative_probe_object(base,field,"ByteProperty")||read_ptr(field+0x40)!=fn)return 0;
    object_name(base,field,name,sizeof name);
    return !strcmp(name,"NewCollisionType")&&read_at(field+0x94,&offset,4)&&offset==0&&
        read_at(field+0x74,&size,4)&&size==1&&read_at(field+0x70,&dim,4)&&dim==1&&
        read_at(field+0x78,&flags,8)&&flags==0x80&&guest_player_context_method_stamp(base,fn,method);
}
static int guest_collision_components(uintptr_t base,uintptr_t pawn,GameArray *head,GuestCameraStamp out[64]) {
    memset(out,0,sizeof(GuestCameraStamp)*64);
    if(!read_at(pawn+0x60,head,sizeof *head)||!head->data||head->count<1||head->count>64||head->max<head->count||head->max>128)return 0;
    for(int i=0;i<head->count;++i) {
        uintptr_t at=0;if(!read_at(head->data+(size_t)i*8,&at,8))return 0;
        if(at&&(!narrative_probe_object(base,at,"ActorComponent")||!guest_camera_stamp_read(base,at,out+i,1)))return 0;
    }
    return 1;
}
static int guest_collision_components_same(uintptr_t base,uintptr_t pawn,const GameArray *head,const GuestCameraStamp saved[64]) {
    GameArray current={0};if(!read_at(pawn+0x60,&current,sizeof current)||memcmp(&current,head,sizeof current))return 0;
    for(int i=0;i<head->count;++i){uintptr_t at=1;
        if(!read_at(head->data+(size_t)i*8,&at,8)||at!=saved[i].pointer||(at&&!guest_camera_stamp_matches(base,saved+i,1)))return 0;}
    return 1;
}
static int guest_collision_ordinary(uintptr_t base,uintptr_t pc,uintptr_t pawn,uintptr_t world) {
    uint32_t flags=0,dead=0,life=0;uintptr_t pauser=1,conversation=0;unsigned char roles[2],physics=0;
    /* The supported guest stays locally autonomous (Role2) across control
     * return, but its RemoteRole can be0/2 rather than the initial3. Role0 is
     * admitted only after this adapter captured a healthy owned baseline.
     * Authority
     * comes from the current client connection/channel and reciprocal owned
     * pawn checked by the caller, not this client's RemoteRole byte. Do not
     * rewrite either role or admit a local simulated/authority pawn. */
    if(!read_at(world+0x7c0,&flags,4)||(flags&0xc00)||!read_at(world+0x670,&pauser,8)||pauser||
       !read_at(pawn+0xe20,&dead,4)||(dead&0x200)||!read_at(pawn+0xe18,&life,4)||!(life&0x40)||
       !read_at(pawn+0x25c,&flags,4)||(flags&0x4000000)||!read_at(pawn+0x269,roles,2)||
       (roles[0]!=2&&roles[0]!=3&&!(roles[0]==0&&guest_collision_restore.tracked))||roles[1]!=2||
       !read_at(pawn+0x268,&physics,1)||(physics!=1&&physics!=2)||!read_at(pc+0x77c,&flags,4)||(flags&0x20000)||
       !read_at(world+0x948,&conversation,8))return 0;
    if(conversation) {
        GameArray active={0};
        if(!narrative_probe_object(base,conversation,"BioConversationManager")||
           !read_at(conversation+0xc8,&active,sizeof active)||active.count||active.max<0||active.max>4096||
           !read_at(conversation+0xe8,&flags,4)||(flags&1))return 0;
    }
    return 1;
}
static void guest_collision_adapt(uintptr_t base,uintptr_t pc,uintptr_t pawn,uintptr_t world,int ordinary,int enabled,DWORD now) {
    GuestCollisionRestore *l=&guest_collision_restore;
    if(!l->configured){wchar_t value[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_COLLISION_RESTORE",value,8);
        l->configured=1;l->opted=n==1&&value[0]==L'1';}
    if(!enabled||!l->opted){
        /* Repairs retain a valid engine collision state. Reintroducing the
         * observed broken state on disable would make an owned pawn fall. */
        l->tracked=l->released=l->pending=l->was_owned=0;return;
    }
    if(l->tracked&&!guest_collision_identity(base)){guest_collision_log("retired","captured-identity-changed",now,0);l->tracked=0;l->suppressed=1;}
    if(l->suppressed)return;
    uintptr_t camera=0,mode=0;NetworkActorIdentityEntry channel={0};
    int owned=guest_player_context_basic_owned(base,pc,pawn,world);
    int eligible=owned&&ordinary&&guest_collision_ordinary(base,pc,pawn,world)&&guest_player_context_lease.registered&&
        guest_player_context_lease.pc.pointer==pc&&guest_player_context_lease.pawn.pointer==pawn&&
        guest_camera_conditions(base,pc,pawn,world,ordinary,enabled,&camera,&mode)&&
        guest_player_context_channel(base,pawn,&channel);
    if(!l->tracked) {
        if(!eligible)return;
        uintptr_t component=read_ptr(pawn+0x1e0),cdo=read_ptr(read_ptr(pawn+0x50)+0x13c),templ=read_ptr(cdo+0x1e0);
        uintptr_t archetype=read_ptr(component+0x58),pawn_archetype=read_ptr(pawn+0x58);
        GuestCollisionShape shape={0},defaults={0};
        if(!narrative_probe_object(base,component,"CylinderComponent")||!narrative_probe_object(base,templ,"CylinderComponent")||
           !is_kind_of(base,cdo,"SFXPawn_Ashley")||read_ptr(cdo+0x50)!=read_ptr(pawn+0x50)||
           !narrative_probe_object(base,archetype,"CylinderComponent")||!is_kind_of(base,pawn_archetype,"SFXPawn_Ashley")||
           read_ptr(archetype+0x40)!=pawn_archetype||read_ptr(archetype+0x50)!=read_ptr(component+0x50)||
           read_ptr(pawn_archetype+0x50)!=read_ptr(pawn+0x50)||!guest_collision_shape(pawn,component,&shape)||
           !guest_collision_shape(cdo,templ,&defaults)||!guest_collision_healthy(&shape)||!guest_collision_physical_equal(&shape,&defaults)) {
            if(!l->reported||now-l->reported>=5000){l->reported=now;guest_collision_log("baseline-refused","healthy-class-template-shape-required",now,0);}return;
        }
        uintptr_t targets[]={pc,pawn,world,component,cdo,templ,archetype,pawn_archetype,channel.channel,channel.connection,channel.driver};
        GuestCameraStamp *stamps[]={&l->pc,&l->pawn,&l->world,&l->component,&l->cdo,&l->template,&l->archetype,&l->pawn_archetype,&l->channel,&l->connection,&l->driver};
        for(unsigned i=0;i<11;++i)if(!guest_camera_stamp_read(base,targets[i],stamps[i],1))return;
        l->healthy=shape;l->network=channel;l->token=guest_player_context_lease.token;l->tracked=1;l->was_owned=1;
        guest_collision_log("baseline","observed-healthy-owned-collision",now,1);return;
    }
    if(!owned) {
        l->was_owned=0;GuestCollisionShape shape={0};
        if(!guest_collision_shape(l->pawn.pointer,l->component.pointer,&shape)){l->released=l->pending=0;return;}
        int disabled=guest_collision_disabled(&shape);
        /* Observe the already-read released shape. Do not widen a physical
         * predicate merely because ownership/camera handoff passed. */
        if(!l->reported||now-l->reported>=5000) {
            char row[600];l->reported=now;
            snprintf(row,sizeof row,"{\"guest_collision_restore\":\"released-shape\",\"lease_token\":%lu,\"actor_mask\":%u,\"block_mask\":%u,\"component_mask\":%u,\"collision_type\":%u,\"replicated_type\":%u,\"disabled_shape_admitted\":%s,\"read_only\":true,\"monotonic_ms\":%lu}\r\n",
                l->token,shape.actor,shape.block,shape.component,(unsigned)shape.type,(unsigned)shape.replicated,
                disabled?"true":"false",now);line(row);
        }
        if(!disabled){l->released=l->pending=0;return;}
        if(l->pending&&guest_collision_equal(&shape,&l->release))l->released=1;
        else {l->release=shape;l->pending=1;l->released=0;}
        return;
    }
    if(!eligible||!l->released||l->was_owned||guest_player_context_lease.token==l->token)return;
    l->was_owned=1;l->token=guest_player_context_lease.token;
    if(channel.channel!=l->network.channel||channel.index!=l->network.index||channel.connection!=l->network.connection||
       channel.driver!=l->network.driver||l->attempted==l->token)return;
    l->attempted=l->token;GuestCollisionShape before={0};GameArray components={0};GuestCameraStamp saved[64];
    const char *refusal=NULL;
    if(!guest_collision_identity(base))refusal="captured-identity-changed";
    else if(!guest_collision_shape(pawn,l->component.pointer,&before))refusal="reacquired-shape-unreadable";
    else if(!guest_collision_equal(&before,&l->release))refusal="released-shape-changed";
    else if(!guest_collision_components(base,pawn,&components,saved))refusal="component-array-identity-unavailable";
    else if(!guest_collision_method(base,&l->method))refusal="native-method-abi-mismatch";
    if(refusal) {
        guest_collision_log("refused",refusal,now,0);
        if(!strcmp(refusal,"released-shape-changed")) {
            char row[700];
            snprintf(row,sizeof row,"{\"guest_collision_restore\":\"reacquired-shape\",\"lease_token\":%lu,\"actor_mask\":%u,\"block_mask\":%u,\"component_mask\":%u,\"collision_type\":%u,\"replicated_type\":%u,\"released_actor_mask\":%u,\"released_block_mask\":%u,\"released_component_mask\":%u,\"released_collision_type\":%u,\"released_replicated_type\":%u,\"read_only\":true,\"monotonic_ms\":%lu}\r\n",
                l->token,before.actor,before.block,before.component,(unsigned)before.type,(unsigned)before.replicated,
                l->release.actor,l->release.block,l->release.component,(unsigned)l->release.type,(unsigned)l->release.replicated,now);
            line(row);
        }
        return;
    }
    unsigned char type=l->healthy.type;
    int dispatched=guest_player_context_dispatch(base,pawn,&l->method,&type);uint32_t current=0;
    int stable=dispatched&&guest_collision_identity(base)&&guest_collision_components_same(base,pawn,&components,saved)&&
        guest_player_context_basic_owned(base,pc,pawn,world)&&guest_collision_ordinary(base,pc,pawn,world)&&read_at(pawn+0x260,&current,4);
    int world_ready=stable&&((current&0x40000000)||guest_collision_world_compare(pawn,current,1));
    GuestCollisionShape after={0};int repaired=world_ready&&guest_collision_shape(pawn,l->component.pointer,&after)&&
        guest_collision_physical_equal(&after,&l->healthy)&&after.replicated==l->healthy.type;
    guest_collision_log("repair-attempt",repaired?"native-type-and-captured-world-collision-restored":"post-call-state-unverified",now,repaired);
    l->released=l->pending=0;
}
static int guest_collision_selftest(void) {
    GuestCollisionShape healthy={0x60000000,1,0xaa,6,6},released={0,0,0x20,1,1},wrong=healthy;
    if(!guest_collision_healthy(&healthy)||!guest_collision_disabled(&released)||guest_collision_healthy(&released)||guest_collision_disabled(&healthy))return 0;
    wrong=healthy;wrong.replicated=10;if(!guest_collision_healthy(&wrong))return 0;
    wrong.replicated=11;if(guest_collision_healthy(&wrong))return 0;wrong=healthy;
    wrong.component^=0x10;if(guest_collision_healthy(&wrong)||guest_collision_equal(&healthy,&wrong))return 0;
    _Alignas(8) unsigned char memory[0x270]={0},component[0x170]={0};
    uint32_t value=0x60000000;memcpy(memory+0x260,&value,4);
    value=3;memcpy(memory+0x264,&value,4);value=0xaa;memcpy(component+0x16c,&value,4);
    memory[0x26b]=memory[0x26c]=6;GuestCollisionShape observed={0};
    if(!guest_collision_shape((uintptr_t)memory,(uintptr_t)component,&observed)||!guest_collision_healthy(&observed)||observed.block!=1)return 0;
    uint32_t before=0x29690100,after=0;memcpy(memory+0x260,&before,4);
    if(guest_collision_world_compare((uintptr_t)memory,before^0x100,1)||!guest_collision_world_compare((uintptr_t)memory,before,1))return 0;
    memcpy(&after,memory+0x260,4);if(after!=(before|0x40000000))return 0;
    if(!guest_collision_world_compare((uintptr_t)memory,after,0))return 0;
    memcpy(&after,memory+0x260,4);return after==before;
}
#endif
