#ifndef ME3_GUEST_EXISTING_CONTEXT_H
#define ME3_GUEST_EXISTING_CONTEXT_H
/* Use the current native squad, either already ready or with empty local
 * player caches. Never own its actor lifetime, registration or membership. */
enum {GEC_PC,GEC_PAWN,GEC_INFO,GEC_WORLD,GEC_LEVEL,GEC_PLAYER,GEC_SQUAD,GEC_CHANNEL,GEC_CONNECTION,GEC_DRIVER,GEC_COUNT};
typedef struct GuestExistingContextLease {
    GuestCameraStamp objects[GEC_COUNT];
    unsigned long token;
    uintptr_t current_before;
    int channel_index,tracked,active,registered,cache_writes_owned,cleanup_calls;
    DWORD reported,cleanup_at;
} GuestExistingContextLease;
static GuestExistingContextLease guest_existing_context;
static void guest_existing_context_log(const char *event,const char *reason,DWORD now) {
    char row[500];snprintf(row,sizeof row,
        "{\"guest_existing_context\":\"%s\",\"reason\":\"%s\",\"lease_token\":%lu,\"registered\":%s,\"cache_writes_owned\":%s,\"actor_lifetime_owned\":false,\"world_slot_written\":false,\"membership_written\":false,\"monotonic_ms\":%lu}\r\n",
        event,reason,guest_existing_context.token,guest_existing_context.registered?"true":"false",guest_existing_context.cache_writes_owned?"true":"false",now);line(row);
}
/* A native replacement is preserved. The expected pawn is compared as a
 * pointer value only; cleanup never dereferences a disposed controlled pawn. */
static int guest_existing_context_restore_field(uintptr_t at,uintptr_t pawn,uintptr_t baseline) {
    uintptr_t actual=0;
    if(!read_at(at,&actual,8))return 0;
    if(actual!=pawn||actual==baseline)return 1;
    if(!guest_player_context_field_writable(at))return 0;
    uintptr_t previous=(uintptr_t)InterlockedCompareExchangePointer((PVOID volatile*)at,(PVOID)baseline,(PVOID)pawn);
    if(previous!=pawn)return 1; /* Engine changed it; preserve its value. */
    return read_at(at,&actual,8)&&actual==baseline;
}
static int guest_existing_context_cleanup(uintptr_t base,const char *reason,DWORD now) {
    GuestExistingContextLease *l=&guest_existing_context;l->registered=0;
    if(!l->active)return 1;
    if(!l->cache_writes_owned) {
        l->active=0;guest_existing_context_log("released","native-ready-observation-ended-no-write",now);return 1;
    }
    CombatContextChain chain;combat_context_read_chain(base,&chain);
    int identity=1,replaced=0;const unsigned keep[]={GEC_INFO,GEC_WORLD,GEC_LEVEL,GEC_SQUAD};
    for(unsigned i=0;i<4;++i) {
        GuestCameraStamp actual={0},*saved=&l->objects[keep[i]];
        identity&=guest_camera_stamp_matches(base,saved,1);
        if(guest_camera_stamp_read(base,saved->pointer,&actual,1)&&
           (!guest_camera_stamp_core_equal(saved,&actual)||strcmp(saved->full_name,actual.full_name)))replaced=1;
    }
    int world_known=chain.world_valid&&chain.info_valid&&chain.level_valid;
    if(replaced||(world_known&&(chain.world!=l->objects[GEC_WORLD].pointer||
       chain.info!=l->objects[GEC_INFO].pointer||chain.level!=l->objects[GEC_LEVEL].pointer))) {
        l->active=0;guest_existing_context_log("retired","captured-object-or-world-replaced-no-write",now);return 1;
    }
    if(!identity||!world_known) {
        if(!l->reported||now-l->reported>=5000){l->reported=now;guest_existing_context_log("cleanup-pending","captured-context-unreadable-no-write",now);}
        return 0;
    }
    uintptr_t squad=l->objects[GEC_SQUAD].pointer,pawn=l->objects[GEC_PAWN].pointer,cached=1,current=1;
    if(read_at(squad+0x2a8,&cached,8)&&read_at(squad+0x2cc,&current,8)) {
        /* m_playerPawn+2cc is unaligned. Clear it only through the verified
         * normal two-assignment script, on the game thread, with both caches
         * still ours/null. Never apply an Interlocked API to that field. */
        if(l->current_before==0&&current==pawn) {
            if(cached&&cached!=pawn) {
                l->active=0;guest_existing_context_log("preserved","native-context-adopted-no-write",now);return 1;
            }
            if(l->cleanup_calls<8&&(!l->cleanup_at||now-l->cleanup_at>=1000)) {
                l->cleanup_at=now;++l->cleanup_calls;uintptr_t none=0;
                guest_player_context_dispatch(base,squad,&guest_player_context_methods.set_pawn,&none);
            }
            if(read_at(squad+0x2a8,&cached,8)&&!cached&&read_at(squad+0x2cc,&current,8)&&!current) {
                l->active=0;guest_existing_context_log("released",reason,now);return 1;
            }
        }else if(guest_existing_context_restore_field(squad+0x2a8,pawn,0)) {
            /* The current cache either equals its captured baseline already
             * or has a native replacement; neither authorizes a write. */
            l->active=0;guest_existing_context_log("released",reason,now);return 1;
        }
    }
    if(!l->reported||now-l->reported>=5000){l->reported=now;guest_existing_context_log("cleanup-pending",reason,now);}
    return 0;
}
static int guest_existing_context_same(uintptr_t base,uintptr_t pc,uintptr_t pawn,uintptr_t info,
    const NetworkActorIdentityEntry *channel,const CombatContextChain *chain) {
    GuestExistingContextLease *l=&guest_existing_context;
    uintptr_t expected[GEC_COUNT]={pc,pawn,info,chain->world,chain->level,read_ptr(pc+0x67c),chain->squad,
        channel->channel,channel->connection,channel->driver};
    if(l->token!=guest_player_context_token||channel->index<=0||channel->index!=l->channel_index)return 0;
    for(unsigned i=0;i<GEC_COUNT;++i)
        if(expected[i]!=l->objects[i].pointer||!guest_camera_stamp_matches(base,&l->objects[i],1))return 0;
    return guest_camera_actor_live(base,chain->squad)&&read_ptr(chain->squad+0x198)==info&&
        read_ptr(chain->squad+0x40)==chain->level&&read_ptr(info+0x8e8)==chain->squad&&read_ptr(pawn+0xba8)==chain->squad;
}
static int guest_existing_context_gate(uintptr_t base,uintptr_t pc) {
    uint32_t value=2;uintptr_t input=read_ptr(pc+0x6dc);
    return is_live(base,input)&&combat_context_get(base,input,&value)&&value==1;
}
static void guest_existing_context_adapt(uintptr_t base,uintptr_t pc,uintptr_t pawn,uintptr_t info,
    int eligible,const NetworkActorIdentityEntry *channel,const CombatContextChain *chain,DWORD now) {
    GuestExistingContextLease *l=&guest_existing_context;
    if(l->active) {
        if(!eligible||!guest_existing_context_same(base,pc,pawn,info,channel,chain)||
           read_ptr(chain->squad+0x2a8)!=pawn||read_ptr(chain->squad+0x2cc)!=pawn) {
            guest_existing_context_cleanup(base,"scope-or-native-cache-change",now);return;
        }
        if(!l->registered){guest_existing_context_cleanup(base,"incomplete-application",now);return;}
        return;
    }
    if(!eligible||channel->index<=0||!chain->squad||guest_player_context_lease.tracked||guest_player_context_lease.pending_cleanup||
       (l->tracked&&l->token==guest_player_context_token))return;
    uintptr_t squad=chain->squad,cached=1,current=1;uint32_t flags=0;
    if(!guest_camera_actor_live(base,squad)||!is_kind_of(base,squad,"BioPlayerSquad")||
       read_ptr(squad+0x198)!=info||read_ptr(squad+0x40)!=chain->level||read_ptr(pawn+0xba8)!=squad||
       !read_at(squad+0x2a8,&cached,8)||!read_at(squad+0x2cc,&current,8)||(current&&current!=pawn)||
       (cached&&(cached!=pawn||current!=pawn))||
       !read_at(squad+0x2b0,&flags,4)||(flags&0xe)!=0xe||
       !guest_player_context_pins(base)||!guest_player_context_resolve(base,pc,now)||
       read_ptr(squad+0x50)!=guest_player_context_methods.klass.pointer)return;
    int native_ready=cached==pawn;
    GuestExistingContextLease next={0};next.token=guest_player_context_token;next.current_before=current;
    next.channel_index=channel->index;next.tracked=1;
    uintptr_t objects[GEC_COUNT]={pc,pawn,info,chain->world,chain->level,read_ptr(pc+0x67c),squad,
        channel->channel,channel->connection,channel->driver};
    for(unsigned i=0;i<GEC_COUNT;++i)if(!guest_camera_stamp_read(base,objects[i],&next.objects[i],1))return;
    *l=next;
    NetworkActorIdentityEntry checked_channel={0};CombatContextChain checked_chain;
    if(!guest_player_context_conditions(base,pc,pawn,info,1,1,1,&checked_channel,&checked_chain)||
       !guest_existing_context_same(base,pc,pawn,info,&checked_channel,&checked_chain)||
       !read_at(squad+0x2a8,&cached,8)||cached!=(native_ready?pawn:0)||
       !read_at(squad+0x2cc,&current,8)||current!=l->current_before)return;
    /* Track the observation before native calls; one attempt per control
     * scope. Only the initializing branch owns the two script cache writes. */
    l->active=1;uintptr_t target=pawn;int dispatched=1;
    if(!native_ready) {
        l->cache_writes_owned=1;
        dispatched=guest_player_context_dispatch(base,squad,&guest_player_context_methods.set_pawn,&target);
    }
    /* Preserve the original short-circuit order, but record the first failed
     * condition. -1 means the check was not reached, not a false result. */
    int conditions=-1,scope=-1,cached_ok=-1,current_ok=-1,getter=-1;
    if(dispatched)conditions=guest_player_context_conditions(base,pc,pawn,info,1,1,0,&checked_channel,&checked_chain);
    if(conditions==1)scope=guest_existing_context_same(base,pc,pawn,info,&checked_channel,&checked_chain);
    if(scope==1)cached_ok=read_ptr(squad+0x2a8)==pawn;
    if(cached_ok==1)current_ok=read_ptr(squad+0x2cc)==pawn;
    if(current_ok==1)getter=guest_existing_context_gate(base,pc);
    int verified=getter==1;
    combat_context_read_chain(base,&checked_chain);
    if(!verified||!guest_existing_context_same(base,pc,pawn,info,&checked_channel,&checked_chain)||
       read_ptr(squad+0x2a8)!=pawn||read_ptr(squad+0x2cc)!=pawn) {
        char details[320];snprintf(details,sizeof details,
            "{\"guest_existing_context\":\"verification-detail\",\"native_ready\":%s,\"dispatch\":%d,\"conditions\":%d,\"scope\":%d,\"cached_player\":%d,\"current_player\":%d,\"combat_getter\":%d,\"not_checked\":-1,\"monotonic_ms\":%lu}\r\n",
            native_ready?"true":"false",dispatched,conditions,scope,cached_ok,current_ok,getter,now);line(details);
        guest_existing_context_log("rejected","native-set-player-or-getter-not-verified",now);
        guest_existing_context_cleanup(base,"application-not-verified",now);return;
    }
    l->registered=1;
    guest_existing_context_log(native_ready?"adopted":"applied",native_ready?
        "existing-native-player-context-verified":"normal-set-player-on-existing-squad",now);
}
static int guest_player_context_registered_for(uintptr_t pc,uintptr_t pawn) {
    return (guest_player_context_lease.registered&&guest_player_context_lease.pc.pointer==pc&&guest_player_context_lease.pawn.pointer==pawn)||
        (guest_existing_context.registered&&guest_existing_context.objects[GEC_PC].pointer==pc&&guest_existing_context.objects[GEC_PAWN].pointer==pawn);
}
static unsigned long guest_player_context_registration_token(void) {
    return guest_existing_context.registered?guest_existing_context.token:guest_player_context_lease.token;
}
#endif
