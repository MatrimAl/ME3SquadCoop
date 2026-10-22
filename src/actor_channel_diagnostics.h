#ifndef ME3_SQUAD_COOP_ACTOR_CHANNEL_DIAGNOSTICS_H
#define ME3_SQUAD_COOP_ACTOR_CHANNEL_DIAGNOSTICS_H

/* Read-only guest channel inventory. Include after network_actor_identity.h;
 * also needs package_identity.h helpers, is_live/is_kind_of/read_at/read_ptr,
 * object_name, line, isfinite and the existing GameArray definition.
 * Call actor_channel_diagnostics_sample(base, world, now) from progress_sample.
 * No engine functions, setters, input, network traffic or gameplay writes.
 */
static DWORD actor_channel_diagnostics_last = 0;
static int actor_channel_diagnostics_sampled = 0;

typedef struct ActorChannelDiagnosticHandle {
    uintptr_t object, klass;
    int32_t index;
    uint64_t name;
} ActorChannelDiagnosticHandle;
static ActorChannelDiagnosticHandle actor_channel_diagnostic_world = {0};
static ActorChannelDiagnosticHandle actor_channel_diagnostic_driver = {0};
static ActorChannelDiagnosticHandle actor_channel_diagnostic_connection = {0};
static uintptr_t actor_channel_diagnostic_base = 0;
static int actor_channel_diagnostic_cold_attempted = 0;

static ActorChannelDiagnosticHandle actor_channel_diagnostic_capture(uintptr_t base, uintptr_t object) {
    ActorChannelDiagnosticHandle handle = {0};
    if (!is_live(base, object) || !read_at(object + 8, &handle.index, 4) ||
        !read_at(object + 0x48, &handle.name, 8)) return handle;
    handle.object = object; handle.klass = read_ptr(object + 0x50);
    return handle;
}
static int actor_channel_diagnostic_handle_matches(uintptr_t base, ActorChannelDiagnosticHandle handle) {
    int32_t index = -1; uint64_t name = 0;
    return handle.object && is_live(base, handle.object) &&
        read_at(handle.object + 8, &index, 4) && index == handle.index &&
        read_at(handle.object + 0x48, &name, 8) && name == handle.name &&
        read_ptr(handle.object + 0x50) == handle.klass;
}
static void actor_channel_diagnostic_clear_scope(void) {
    memset(&actor_channel_diagnostic_driver, 0, sizeof actor_channel_diagnostic_driver);
    memset(&actor_channel_diagnostic_connection, 0, sizeof actor_channel_diagnostic_connection);
}
static int actor_channel_diagnostic_scope_matches(uintptr_t base) {
    return actor_channel_diagnostic_handle_matches(base, actor_channel_diagnostic_world) &&
        actor_channel_diagnostic_handle_matches(base, actor_channel_diagnostic_driver) &&
        actor_channel_diagnostic_handle_matches(base, actor_channel_diagnostic_connection) &&
        network_actor_connection_in_scope(base, actor_channel_diagnostic_driver.object,
                                          actor_channel_diagnostic_connection.object);
}
static void actor_channel_diagnostic_unavailable(const char *reason, const char *mode, DWORD started, DWORD now) {
    char text[420]; snprintf(text, sizeof text,
        "{\"channel_diagnostic\":\"unavailable\",\"reason\":\"%s\",\"discovery_mode\":\"%s\",\"scope_invariants\":false,\"sampling_duration_ms\":%lu,\"monotonic_ms\":%lu}\r\n",
        reason, mode, GetTickCount() - started, now);
    line(text);
}

static void actor_channel_diagnostics_superchain(uintptr_t base, uintptr_t klass,
                                                char *output, size_t capacity) {
    uintptr_t visited[12]; int count = 0; size_t used = 0;
    output[0] = 0;
    while (is_live(base, klass) && count < 12 && used + 2 < capacity) {
        char name[96];
        for (int i = 0; i < count; ++i) if (visited[i] == klass) return;
        visited[count++] = klass;
        object_name(base, klass, name, sizeof name);
        int wrote = snprintf(output + used, capacity - used, "%s%s", used ? ">" : "", name);
        if (wrote < 0 || (size_t)wrote >= capacity - used) return;
        used += (size_t)wrote;
        klass = read_ptr(klass + 0x60);
    }
}

static void actor_channel_diagnostics_sample(uintptr_t base, uintptr_t world, DWORD now) {
    if (test_action != 5 || !is_live(base, world)) return;
    unsigned char net_mode = 255;
    if (!read_at(world + 0x7c8, &net_mode, 1) || net_mode != 3) return;
    if (actor_channel_diagnostic_base != base || actor_channel_diagnostic_world.object != world ||
        !actor_channel_diagnostic_handle_matches(base, actor_channel_diagnostic_world)) {
        actor_channel_diagnostic_base = base;
        actor_channel_diagnostic_world = actor_channel_diagnostic_capture(base, world);
        actor_channel_diagnostic_clear_scope(); actor_channel_diagnostic_cold_attempted = 0;
        actor_channel_diagnostics_sampled = 0;
    }
    if (actor_channel_diagnostics_sampled && now - actor_channel_diagnostics_last < 5000) return;
    actor_channel_diagnostics_sampled = 1; actor_channel_diagnostics_last = now;
    DWORD started = GetTickCount(); const char *discovery_mode = "verified-cache";
    /* Recheck live vtable pins each sample in addition to the shared image-byte
     * checks; cached handles never authorize a stale/different native layout.
     */
    if (!network_actor_layout_matches(base) || read_ptr(base + 0x11a2150 + 0x2b0) != base + 0x4840a0 ||
        !network_actor_received_handler_matches(base)) {
        actor_channel_diagnostic_clear_scope();
        actor_channel_diagnostic_unavailable("native-layout-pins", "none", started, now); return;
    }
    uintptr_t driver = 0, connection = 0; int drivers = 0;
    if (actor_channel_diagnostic_scope_matches(base)) {
        driver = actor_channel_diagnostic_driver.object;
        connection = actor_channel_diagnostic_connection.object;
    } else {
        actor_channel_diagnostic_clear_scope();
        discovery_mode = "actor-identity-cache";
        for (unsigned int i = 0; i < sizeof network_actor_identity_cache / sizeof network_actor_identity_cache[0]; ++i) {
            NetworkActorIdentityEntry *cached = &network_actor_identity_cache[i];
            NetworkActorIdentityEntry verified = {0};
            if (cached->base != base || !is_live(base, cached->pawn) || read_ptr(cached->pawn + 0x198) != world ||
                !network_actor_channel_valid(base, cached->pawn, cached->channel, cached->driver, &verified) ||
                verified.connection != cached->connection || verified.index != cached->index) continue;
            if (drivers && (driver != verified.driver || connection != verified.connection)) {
                actor_channel_diagnostic_unavailable("identity-cache-ambiguous", discovery_mode, started, now); return;
            }
            driver = verified.driver; connection = verified.connection; drivers = 1;
        }
        if (!drivers) {
            /* During a playable lease, prefer unknown diagnostics over a long
             * discovery stall. A single cold attempt per world is sufficient;
             * the ordinary actor-identity cache can populate the scope later.
             */
            uintptr_t pc = local_controller(read_ptr(base + 0x18b31d0));
            uintptr_t owned = is_live(base, pc) ? read_ptr(pc + 0x3bc) : 0;
            if (actor_channel_diagnostic_cold_attempted ||
                (is_live(base, owned) && read_ptr(owned + 0x374) == pc)) {
                actor_channel_diagnostic_unavailable("cold-scan-deferred", "none", started, now); return;
            }
            actor_channel_diagnostic_cold_attempted = 1; discovery_mode = "gobjects-cold";
            GameArray array = {0}; uintptr_t *objects = identity_object_list(base, &array);
            if (!objects) {
                actor_channel_diagnostic_unavailable("object-snapshot", discovery_mode, started, now); return;
            }
            for (int i = 0; i < array.count; ++i) {
                uintptr_t object = objects[i], peer; char kind[96];
                if (!object) continue;
                object_name(base, read_ptr(object + 0x50), kind, sizeof kind);
                /* Name is only a cheap prefilter; native inheritance, liveness
                 * and all reciprocal ownership checks still authorize scope.
                 */
                if (!strstr(kind, "NetDriver") || !is_live(base, object) || !is_kind_of(base, object, "NetDriver")) continue;
                peer = read_ptr(object + 0x80);
                if (network_actor_connection_in_scope(base, object, peer)) {
                    driver = object; connection = peer; drivers++;
                }
            }
            HeapFree(GetProcessHeap(), 0, objects);
            if (drivers != 1) {
                actor_channel_diagnostic_unavailable("server-driver-not-unique", discovery_mode, started, now); return;
            }
        }
        actor_channel_diagnostic_driver = actor_channel_diagnostic_capture(base, driver);
        actor_channel_diagnostic_connection = actor_channel_diagnostic_capture(base, connection);
    }
    if (!actor_channel_diagnostic_scope_matches(base)) {
        actor_channel_diagnostic_clear_scope();
        actor_channel_diagnostic_unavailable("scope-identities-changed", discovery_mode, started, now); return;
    }
    uintptr_t channels[2048];
    if (!read_at(connection + 0xf84, channels, sizeof channels)) {
        actor_channel_diagnostic_unavailable("channel-table-read", discovery_mode, started, now); return;
    }
    int matched = 0, logged = 0, invalid = 0;
    for (int slot = 0; slot < 2048; ++slot) {
        uintptr_t channel = channels[slot], actor, actor_class;
        NetworkActorIdentityEntry entry = {0};
        if (!channel || read_ptr(channel) != base + 0x11a2150 ||
            !is_live(base, channel) || !is_kind_of(base, channel, "ActorChannel")) continue;
        actor = read_ptr(channel + 0xa0);
        /* The generic channel validator allows a NULL Actor for diagnostics;
         * this call does not claim that an unresolved channel has pawn identity.
         */
        if (!network_actor_channel_valid(base, actor, channel, driver, &entry) ||
            entry.connection != connection || entry.index != slot) { invalid++; continue; }
        matched++;
        if (logged >= 128) continue;
        logged++;
        actor_class = read_ptr(channel + 0xa8);
        int live = is_live(base, actor), is_actor = live && is_kind_of(base, actor, "Actor");
        int pawn = is_actor && is_kind_of(base, actor, "Pawn");
        int bio_pawn = pawn && is_kind_of(base, actor, "BioPawn");
        int player = pawn && is_kind_of(base, actor, "SFXPawn_Player");
        int soldier = pawn && is_kind_of(base, actor, "SFXPawn_PlayerSoldier");
        uintptr_t klass = live ? read_ptr(actor + 0x50) : 0;
        uintptr_t actor_world = is_actor ? read_ptr(actor + 0x198) : 0;
        uintptr_t controller = pawn ? read_ptr(actor + 0x374) : 0;
        uintptr_t mesh = pawn ? read_ptr(actor + 0x3f4) : 0;
        uintptr_t asset = is_live(base, mesh) ? read_ptr(mesh + 0x258) : 0;
        uint32_t flags = 0, mesh_flags = 0; float position[3] = {0};
        unsigned char role = 255, remote_role = 255;
        int flags_known = is_actor && read_at(actor + 0x25c, &flags, 4);
        int position_known = is_actor && read_at(actor + 0x108, position, sizeof position) &&
            isfinite(position[0]) && isfinite(position[1]) && isfinite(position[2]);
        int mesh_flags_known = is_live(base, mesh) && is_kind_of(base, mesh, "PrimitiveComponent") &&
            read_at(mesh + 0x168, &mesh_flags, 4);
        if (is_actor) { read_at(actor + 0x26a, &role, 1); read_at(actor + 0x269, &remote_role, 1); }
        char class_name[96] = {0}, channel_class[96] = {0}, controller_class[96] = {0};
        char chain[640] = {0}, path[640] = {0}, world_path[640] = {0};
        char chain_json[1280], path_json[1280], world_json[1280];
        char class_json[200], channel_json[200], controller_json[200], position_json[120];
        if (is_live(base, klass)) object_name(base, klass, class_name, sizeof class_name);
        if (is_live(base, actor_class)) object_name(base, actor_class, channel_class, sizeof channel_class);
        if (is_live(base, controller)) object_name(base, read_ptr(controller + 0x50), controller_class, sizeof controller_class);
        if (live) identity_path(base, actor, path, sizeof path);
        if (is_live(base, actor_world)) identity_path(base, actor_world, world_path, sizeof world_path);
        actor_channel_diagnostics_superchain(base, klass, chain, sizeof chain);
        identity_quote(class_name, class_json, sizeof class_json);
        identity_quote(channel_class, channel_json, sizeof channel_json);
        identity_quote(controller_class, controller_json, sizeof controller_json);
        identity_quote(chain, chain_json, sizeof chain_json);
        identity_quote(path, path_json, sizeof path_json);
        identity_quote(world_path, world_json, sizeof world_json);
        if (position_known) snprintf(position_json, sizeof position_json, "[%.3f,%.3f,%.3f]", position[0], position[1], position[2]);
        else strcpy(position_json, "null");
        char row[6144];
        snprintf(row, sizeof row,
            "{\"channel_diagnostic\":\"actor\",\"channel_index\":%d,\"channel_type\":2,\"reciprocal_channel\":true,"
            "\"actor_present\":%s,\"actor_live\":%s,\"actor_class\":\"%s\",\"channel_actor_class\":\"%s\",\"super_chain\":\"%s\","
            "\"actor_path\":\"%s\",\"actor_world\":\"%s\",\"actor_world_matches\":%s,\"is_actor\":%s,\"is_pawn\":%s,\"is_bio_pawn\":%s,"
            "\"is_sfx_player\":%s,\"is_player_soldier\":%s,\"flags_known\":%s,\"actor_flags\":%u,\"hidden\":%s,\"deleted\":%s,\"only_owner_see\":%s,"
            "\"position\":%s,\"position_known\":%s,\"role\":%u,\"remote_role\":%u,\"controller_class\":\"%s\",\"controller_reciprocal\":%s,"
            "\"mesh_present\":%s,\"mesh_asset_present\":%s,\"mesh_flags_known\":%s,\"mesh_hidden_game\":%s,\"monotonic_ms\":%lu}\r\n",
            slot, actor ? "true" : "false", live ? "true" : "false", class_json, channel_json, chain_json,
            path_json, world_json, actor_world == world ? "true" : "false", is_actor ? "true" : "false", pawn ? "true" : "false", bio_pawn ? "true" : "false",
            player ? "true" : "false", soldier ? "true" : "false", flags_known ? "true" : "false", (unsigned)flags,
            flags_known && (flags & 2) ? "true" : "false", flags_known && (flags & 8) ? "true" : "false", flags_known && (flags & 32) ? "true" : "false",
            position_json, position_known ? "true" : "false", role, remote_role, controller_json,
            is_live(base, controller) && read_ptr(controller + 0x3bc) == actor ? "true" : "false",
            is_live(base, mesh) ? "true" : "false", is_live(base, asset) ? "true" : "false", mesh_flags_known ? "true" : "false",
            mesh_flags_known && (mesh_flags & 4) ? "true" : "false", now);
        line(row);
    }
    int scope_ok = actor_channel_diagnostic_scope_matches(base);
    char summary[600]; snprintf(summary, sizeof summary,
        "{\"channel_diagnostic\":\"summary\",\"scanned_slots\":2048,\"valid_actor_channels\":%d,\"logged_channels\":%d,\"truncated\":%s,\"channel_invariant_failures\":%d,"
        "\"discovery_mode\":\"%s\",\"scope_invariants\":%s,\"world_identity_matches\":%s,\"driver_identity_matches\":%s,\"connection_identity_matches\":%s,\"sampling_duration_ms\":%lu,\"monotonic_ms\":%lu}\r\n",
        matched, logged, matched > logged ? "true" : "false", invalid, discovery_mode, scope_ok ? "true" : "false",
        actor_channel_diagnostic_handle_matches(base, actor_channel_diagnostic_world) ? "true" : "false",
        actor_channel_diagnostic_handle_matches(base, actor_channel_diagnostic_driver) ? "true" : "false",
        actor_channel_diagnostic_handle_matches(base, actor_channel_diagnostic_connection) ? "true" : "false", GetTickCount() - started, now);
    line(summary);
    if (!scope_ok) actor_channel_diagnostic_clear_scope();
}

#endif /* ME3_SQUAD_COOP_ACTOR_CHANNEL_DIAGNOSTICS_H */
