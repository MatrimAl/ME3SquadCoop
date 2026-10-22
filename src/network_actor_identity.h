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
    if (test_action == 5) return read_ptr(driver + 0x80) == connection;
    if (test_action == 6) {
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
    if (test_action == 6) {
        GameArray clients = {0};
        if (driver != identity_host_driver || read_ptr(driver + 0x80) ||
            !identity_array(driver + 0x70, &clients, 64) ||
            !read_at(clients.data, connections, (SIZE_T)clients.count * 8)) return -1;
        connection_count = clients.count;
    } else if (test_action == 5) {
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

static int network_actor_channel_id_core(uintptr_t base, uintptr_t pawn) {
    NetworkActorIdentityEntry candidate = {0}, found = {0};
    GameArray array = {0}; uintptr_t *objects; int matches = 0;
    if ((test_action != 5 && test_action != 6) || !network_actor_layout_matches(base) ||
        !is_live(base, pawn) || !is_kind_of(base, pawn, "Actor")) return -1;
    unsigned char mode=255;uintptr_t world=read_ptr(pawn+0x198);
    if(!world||!read_at(world+0x7c8,&mode,1)||mode!=(test_action==5?3:2))return -1;
    if(test_action==6) { GameArray peers={0};if(!identity_array(identity_host_driver+0x70,&peers,64))return -1; }
    for (unsigned int i = 0; i < 4; ++i) {
        NetworkActorIdentityEntry *cached = &network_actor_identity_cache[i];
        if (cached->base != base || cached->pawn != pawn) continue;
        if (network_actor_channel_valid(base, pawn, cached->channel, cached->driver, &candidate) &&
            candidate.index == cached->index && candidate.connection == cached->connection) {
            int index = network_actor_scan_driver(base, pawn, cached->driver, &found);
            if (index >= 0) { *cached = found; return index; }
            memset(cached, 0, sizeof *cached);
            return -1;
        }
        memset(cached, 0, sizeof *cached);
    }
    /* Cold path discovers actual channels/drivers from the bounded GObjects
     * snapshot. It never infers a counterpart from Ashley's class or path.
     */
    objects = identity_object_list(base, &array);
    if (!objects) return -1;
    for (int i = 0; i < array.count; ++i) {
        uintptr_t channel = objects[i], connection, driver = 0;
        if (!channel || read_ptr(channel) != base + 0x11a2150 || read_ptr(channel + 0xa0) != pawn) continue;
        connection = read_ptr(channel + 0x60);
        if (test_action == 6) driver = identity_host_driver;
        else {
            int driver_matches = 0;
            /* Only a channel's owning connection can identify the client-side
             * driver; a similarly named live/pending driver is insufficient.
             */
            for (int j = 0; j < array.count; ++j) {
                uintptr_t object = objects[j];
                if (object && read_ptr(object + 0x80) == connection &&
                    is_live(base, object) && is_kind_of(base, object, "NetDriver")) {
                    driver = object; driver_matches++;
                }
            }
            if (driver_matches != 1) continue;
        }
        if (!network_actor_channel_valid(base, pawn, channel, driver, &candidate)) continue;
        found = candidate;
        if (++matches > 1) break;
    }
    HeapFree(GetProcessHeap(), 0, objects);
    if (matches != 1) return -1;
    /* Runtime validation includes both connection collections, not just one
     * coincidental pawn pointer somewhere in an object of the right class.
     */
    if (network_actor_scan_driver(base, pawn, found.driver, &candidate) < 0 ||
        candidate.channel != found.channel || candidate.connection != found.connection) return -1;
    network_actor_identity_cache[network_actor_identity_next++ % 4] = candidate;
    return candidate.index;
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
