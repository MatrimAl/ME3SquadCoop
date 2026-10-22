#ifndef ME3_SQUAD_COOP_NETWORK_ACTOR_IDENTITY_H
#define ME3_SQUAD_COOP_NETWORK_ACTOR_IDENTITY_H

/* Read-only actor-channel identity. Include after pair_control.h and existing
 * memory/object helpers. ChIndex is connection/lifetime scoped, never a stable
 * campaign actor ID. No matching by pawn class, object name or FName suffix.
 * Static provenance: private channel-layout-static-evidence.json, exact LE3
 * EXE b453871e...; pointers and both directions are revalidated at runtime.
 */
typedef struct NetworkActorIdentityEntry {
    uintptr_t base, pawn, channel, connection, driver;
    int index;
} NetworkActorIdentityEntry;

static NetworkActorIdentityEntry network_actor_identity_cache[4];
static unsigned int network_actor_identity_next = 0;
/* Exact opt-in observer installed by initial_actor_trace.h after validating
 * the original function. This admits only our forwarding trace, never an
 * arbitrary replacement or an altered engine/package validation routine. */
static uintptr_t network_actor_received_bunch_observer = 0;
static int network_actor_received_handler_matches(uintptr_t base) {
    uintptr_t actual=read_ptr(base+0x11a2150+0x2d0);
    return actual==base+0x488700 ||
        (network_actor_received_bunch_observer && actual==network_actor_received_bunch_observer);
}

static int network_actor_layout_matches(uintptr_t base) {
    static uintptr_t checked_base = 0;
    static int matches = 0;
    static const unsigned char init[] = {0x48,0x89,0x47,0x60,0x89,0x6f,0x6c,0x89,0x77,0x70};
    static const unsigned char type[] = {0xc7,0x41,0x78,0x02,0x00,0x00,0x00};
    static const unsigned char table[] = {0x48,0x63,0xcb,0x48,0x8d,0x9e,0x90,0x5f,0x00,0x00,0x48,0x89,0xbc,0xce,0x84,0x0f,0x00,0x00};
    static const unsigned char actor[] = {0x49,0x8b,0x85,0xa0,0x00,0x00,0x00,0xc6,0x80,0x6a,0x02,0x00,0x00,0x03};
    unsigned char scratch[32];
    if (checked_base == base) return matches;
    checked_base = base; matches = 0;
    if (!base || !read_at(base + 0x4840dd, scratch, sizeof init) || memcmp(scratch, init, sizeof init) ||
        !read_at(base + 0x484f32, scratch, sizeof type) || memcmp(scratch, type, sizeof type) ||
        !read_at(base + 0x480254, scratch, sizeof table) || memcmp(scratch, table, sizeof table) ||
        !read_at(base + 0x489ce5, scratch, sizeof actor) || memcmp(scratch, actor, sizeof actor)) return 0;
    matches = 1;
    return 1;
}

static int network_actor_connection_in_scope(uintptr_t base, uintptr_t driver, uintptr_t connection) {
    if (!is_live(base, driver) || !is_kind_of(base, driver, "NetDriver") ||
        !is_live(base, connection) || !is_kind_of(base, connection, "NetConnection") ||
        read_ptr(connection + 0x8c) != driver) return 0;
    if (native_session_role() == 5) return read_ptr(driver + 0x80) == connection;
    if (native_session_role() == 6) {
        GameArray clients = {0}; int memberships = 0;
        if (driver != identity_host_driver || read_ptr(driver + 0x80) ||
            !identity_array(driver + 0x70, &clients, 64)) return 0;
        for (int i = 0; i < clients.count; ++i)
            if (read_ptr(clients.data + (SIZE_T)i * 8) == connection) memberships++;
        return memberships == 1;
    }
    return 0;
}

static int network_actor_channel_valid(uintptr_t base, uintptr_t pawn, uintptr_t channel,
                                        uintptr_t driver, NetworkActorIdentityEntry *entry) {
    int32_t index = -1, type = -1;
    uintptr_t connection, vtable;
    if (!is_live(base, channel) || !is_kind_of(base, channel, "ActorChannel") ||
        read_ptr(channel + 0xa0) != pawn ||
        !read_at(channel + 0x6c, &index, 4) || index < 0 || index > 2047 ||
        !read_at(channel + 0x78, &type, 4) || type != 2) return 0;
    vtable = read_ptr(channel);
    if (vtable != base + 0x11a2150 || read_ptr(vtable + 0x2b0) != base + 0x4840a0 ||
        !network_actor_received_handler_matches(base)) return 0;
    connection = read_ptr(channel + 0x60);
    if (!network_actor_connection_in_scope(base, driver, connection) ||
        read_ptr(connection + 0xf84 + (SIZE_T)index * 8) != channel) return 0;
    entry->base = base; entry->pawn = pawn; entry->channel = channel;
    entry->connection = connection; entry->driver = driver; entry->index = index;
    return 1;
}

/* Enumerate every current peer's OpenChannels to reject multiple matches even
 * on the cached path. A malformed list makes the result unknown, not unique.
 */
static int network_actor_scan_driver(uintptr_t base, uintptr_t pawn, uintptr_t driver,
                                     NetworkActorIdentityEntry *result) {
    uintptr_t connections[64]; int connection_count = 0, matches = 0;
    if (!is_live(base, driver) || !is_kind_of(base, driver, "NetDriver")) return -1;
    if (native_session_role() == 6) {
        GameArray clients = {0};
        if (driver != identity_host_driver || read_ptr(driver + 0x80) ||
            !identity_array(driver + 0x70, &clients, 64) ||
            !read_at(clients.data, connections, (SIZE_T)clients.count * 8)) return -1;
        connection_count = clients.count;
    } else if (native_session_role() == 5) {
        connections[0] = read_ptr(driver + 0x80); connection_count = 1;
    } else return -1;
    for (int c = 0; c < connection_count; ++c) {
        GameArray open = {0}; uintptr_t channels[2048];
        uintptr_t connection = connections[c];
        if (!network_actor_connection_in_scope(base, driver, connection) ||
            !read_at(connection + 0x5f90, &open, sizeof open) || open.count < 0 ||
            open.count > 2048 || open.max < open.count || open.max > 4096 ||
            (open.count && (!open.data || !read_at(open.data, channels, (SIZE_T)open.count * 8)))) return -1;
        for (int i = 0; i < open.count; ++i) {
            NetworkActorIdentityEntry candidate = {0};
            uintptr_t channel = channels[i];
            /* Most open channels concern other actors; avoid class-name walks
             * for them while retaining safe reads and exact reciprocal checks.
             */
            if (!channel || read_ptr(channel + 0xa0) != pawn) continue;
            if (!network_actor_channel_valid(base, pawn, channel, driver, &candidate)) return -1;
            if (++matches > 1) return -1;
            *result = candidate;
        }
    }
    return matches == 1 ? result->index : -1;
}

typedef struct NetworkActorScope {
    uintptr_t world,level,info,driver;
    GameArray actors;
    unsigned char mode;
} NetworkActorScope;

/* Exact-build native references: GWorld load, its NetDriver selection and
 * UWorld's PersistentLevel/Actors[0]/NetMode getter. No native call is made.
 * Bind the actor to this current world, rather than choosing a pending or
 * similarly named driver from GObjects. Recheck the chain after enumeration.
 */
static int network_actor_world_pins(uintptr_t base) {
    static const unsigned char global[]={0x4c,0x8b,0x05,0x55,0xc6,0xfe,0x00};
    static const unsigned char driver[]={0x49,0x8b,0x98,0x20,0x01,0x00,0x00};
    static const unsigned char mode[]={0x48,0x8b,0x81,0x88,0x00,0x00,0x00,0x48,0x8b,0x48,0x60,0x48,0x8b,0x01,0x0f,0xb6,0x80,0xc8,0x07,0x00,0x00,0xc3};
    unsigned char a[sizeof global],b[sizeof driver],c[sizeof mode];
    return read_at(base+0x8c9fdc,a,sizeof a)&&!memcmp(a,global,sizeof a)&&
        read_at(base+0x8ca012,b,sizeof b)&&!memcmp(b,driver,sizeof b)&&
        read_at(base+0x8c1e90,c,sizeof c)&&!memcmp(c,mode,sizeof c);
}
static int network_actor_read_scope(uintptr_t base,uintptr_t actor,NetworkActorScope *s) {
    memset(s,0,sizeof *s);
    if(!network_actor_world_pins(base)||
       !read_at(base+0x18b6638,&s->world,8)||!is_live(base,s->world)||!is_kind_of(base,s->world,"World")||
       !read_at(s->world+0x88,&s->level,8)||!is_live(base,s->level)||!is_kind_of(base,s->level,"Level")||
       !identity_array(s->level+0x60,&s->actors,100000)||
       !read_at(s->actors.data,&s->info,8)||!is_live(base,s->info)||!is_kind_of(base,s->info,"WorldInfo")||
       read_ptr(actor+0x198)!=s->info||!read_at(s->info+0x7c8,&s->mode,1)||s->mode!=(native_session_role()==5?3:2)||
       !read_at(s->world+0x120,&s->driver,8)||!is_live(base,s->driver)||!is_kind_of(base,s->driver,"NetDriver"))return 0;
    return native_session_role()==5||(native_session_role()==6&&s->driver==identity_host_driver);
}

static int network_actor_channel_id_core(uintptr_t base, uintptr_t pawn) {
    NetworkActorIdentityEntry found = {0};NetworkActorScope before,after;
    if ((native_session_role() != 5 && native_session_role() != 6) || !network_actor_layout_matches(base) ||
        !is_live(base, pawn) || !is_kind_of(base, pawn, "Actor")) return -1;
    /* Consumers may use the published cache between calls, but resolution
     * always checks every current peer. A failed lookup invalidates its hint.
     */
    unsigned int slot=4;
    for(unsigned int i=0;i<4;++i)if(network_actor_identity_cache[i].base==base&&network_actor_identity_cache[i].pawn==pawn){
        slot=i;memset(&network_actor_identity_cache[i],0,sizeof network_actor_identity_cache[i]);
    }
    if(!network_actor_read_scope(base,pawn,&before)||
       network_actor_scan_driver(base,pawn,before.driver,&found)<0||
       !network_actor_read_scope(base,pawn,&after)||memcmp(&before,&after,sizeof before))return -1;
    if(slot==4)slot=network_actor_identity_next++%4;
    network_actor_identity_cache[slot]=found;
    return found.index;
}

/* Preserve the pawn-only contract for existing ownership/camera consumers. */
static int network_actor_channel_id_internal(uintptr_t base,uintptr_t pawn) {
    unsigned long long started=lookup_clock();int index=network_actor_channel_id_core(base,pawn);
    lookup_record("ActorChannel","resolve",started,0,0);return index;
}
static int network_actor_channel_id(uintptr_t base, uintptr_t pawn) {
    return is_live(base,pawn)&&is_kind_of(base,pawn,"Pawn") ? network_actor_channel_id_internal(base,pawn) : -1;
}
static int network_weapon_channel_id(uintptr_t base, uintptr_t weapon) {
    return is_live(base,weapon)&&is_kind_of(base,weapon,"SFXWeapon_NativeBase") ?
        network_actor_channel_id_internal(base,weapon) : -1;
}

#endif /* ME3_SQUAD_COOP_NETWORK_ACTOR_IDENTITY_H */
