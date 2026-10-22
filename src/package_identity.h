/* Read-only runtime package identity probe. Included after the observer's
 * memory/name/string helpers and GameArray layout. Private JSONL evidence;
 * do not distribute object maps as a gameplay mod. */
static int identity_host_done = 0, identity_client_done = 0;
static int refresh_host_map = 0, host_map_refreshed = 0, host_map_attempted = 0;
static uintptr_t identity_host_driver = 0;
static void identity_quote(const char *input, char *out, size_t cap) {
    size_t j = 0;
    for (size_t i = 0; input[i] && j + 2 < cap; ++i) {
        unsigned char c = (unsigned char)input[i];
        if (c < 32) c = ' ';
        if (c == '\\' || c == '"') out[j++] = '\\';
        out[j++] = (char)c;
    }
    out[j] = 0;
}
static uintptr_t identity_root(uintptr_t object) {
    uintptr_t previous = 0;
    for (int i = 0; object && i < 48; ++i) {
        uintptr_t next = read_ptr(object + 0x40);
        if (!next) return object;
        if (next == object || next == previous) return 0;
        previous = object; object = next;
    }
    return 0;
}
static void identity_path(uintptr_t base, uintptr_t object, char *out, size_t cap) {
    char parts[40][112]; int count = 0; out[0] = 0;
    while (object && count < 40) {
        char name[96]; int32_t number = 0;
        object_name(base, object, name, sizeof name);
        if (!name[0]) { snprintf(out, cap, "<unreadable>"); return; }
        read_at(object + 0x4c, &number, 4);
        if (number > 0) snprintf(parts[count], sizeof parts[count], "%s_%d", name, number - 1);
        else snprintf(parts[count], sizeof parts[count], "%s", name);
        count++;
        uintptr_t next = read_ptr(object + 0x40);
        if (next == object) break;
        object = next;
    }
    size_t used = 0;
    for (int i = count - 1; i >= 0 && used + 2 < cap; --i) {
        int n = snprintf(out + used, cap - used, "%s%s", used ? "." : "", parts[i]);
        if (n < 0 || (size_t)n >= cap - used) break;
        used += (size_t)n;
    }
}
static void identity_linker(uintptr_t base, uintptr_t object, char *out, size_t cap) {
    uintptr_t linker = read_ptr(object + 0x2c), linker_root = linker ? read_ptr(linker + 0x60) : 0;
    char kind[96], root_name[96], kind_json[200], root_json[200];
    object_name(base, linker ? read_ptr(linker + 0x50) : 0, kind, sizeof kind);
    object_name(base, linker_root, root_name, sizeof root_name);
    identity_quote(kind, kind_json, sizeof kind_json); identity_quote(root_name, root_json, sizeof root_json);
    wchar_t a[400] = {0}, b[400] = {0};
    if (linker) { game_string(linker + 0x194, a, 400); game_string(linker + 0x198, b, 400); }
    char a_json[1600], b_json[1600];
    json_text(a, a_json, sizeof a_json); json_text(b, b_json, sizeof b_json);
    int a_exists = a[0] && GetFileAttributesW(a) != INVALID_FILE_ATTRIBUTES;
    int b_exists = b[0] && GetFileAttributesW(b) != INVALID_FILE_ATTRIBUTES;
    snprintf(out, cap,
        "{\"present\":%s,\"class\":\"%s\",\"root\":\"%s\",\"candidate_194\":\"%s\",\"candidate_194_exists\":%s,\"candidate_198\":\"%s\",\"candidate_198_exists\":%s}",
        linker ? "true" : "false", kind_json, root_json, a_json, a_exists ? "true" : "false", b_json, b_exists ? "true" : "false");
}
static int identity_array(uintptr_t address, GameArray *array, int limit) {
    return read_at(address, array, sizeof *array) && array->data && array->count > 0 &&
           array->count <= limit && array->max >= array->count && array->max <= 500000;
}
static uintptr_t *identity_object_list(uintptr_t base, GameArray *array) {
    if (!identity_array(base + OBJECTS_RVA, array, 500000)) return NULL;
    SIZE_T bytes = (SIZE_T)array->count * 8;
    uintptr_t *objects = HeapAlloc(GetProcessHeap(), 0, bytes);
    if (!objects) return NULL;
    if (!read_at(array->data, objects, bytes)) { HeapFree(GetProcessHeap(), 0, objects); return NULL; }
    return objects;
}
static void identity_cached_guid(uintptr_t base, uintptr_t package, const char *phase, const char *name) {
    uintptr_t manager = read_ptr(base + 0x17d5630);
    uintptr_t vt = manager ? read_ptr(manager) : 0;
    const unsigned char want[8] = {0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c};
    unsigned char got[8];
    int valid = vt == base + 0x1361170 && read_ptr(vt + 0xb0) == base + 0xafa2f0 &&
                read_at(base + 0xafa2f0, got, sizeof got) && !memcmp(got, want, sizeof got);
    uint64_t fname = 0; uint32_t guid[4] = {0};
    if (valid && read_at(package + 0x48, &fname, sizeof fname)) {
        /* Native getter only reads existing UGuidCache hash tables and writes
           the caller-owned result. No cache/package identity is modified. */
        typedef void *(*GetCachedGuid)(void *, void *, uint64_t);
        ((GetCachedGuid)(base + 0xafa2f0))((void *)manager, guid, fname);
    }
    char text[500];
    snprintf(text, sizeof text,
        "{\"identity\":\"cached-guid\",\"phase\":\"%s\",\"package\":\"%s\",\"provider_verified\":%s,\"guid\":\"%08x-%08x-%08x-%08x\",\"found\":%s,\"metadata_modified\":false}\r\n",
        phase, name, valid ? "true" : "false", guid[0], guid[1], guid[2], guid[3],
        valid && (guid[0] || guid[1] || guid[2] || guid[3]) ? "true" : "false");
    line(text);
}
static void identity_package(uintptr_t base, uintptr_t package, const char *phase,
                              uintptr_t *objects, int object_count) {
    char name[96], path[768], path_json[1550], linker[3600], message[6400];
    object_name(base, package, name, sizeof name);
    identity_cached_guid(base, package, phase, name);
    identity_path(base, package, path, sizeof path); identity_quote(path, path_json, sizeof path_json);
    uint32_t guid[4] = {0}; read_at(package + 0x6c, guid, sizeof guid);
    int32_t index = -1; read_at(package + 8, &index, 4);
    GameArray generations = {0};
    char generation_text[500] = "[]";
    if (identity_array(package + 0x94, &generations, 32)) {
        int32_t counts[32] = {0};
        if (read_at(generations.data, counts, (SIZE_T)generations.count * 4)) {
            size_t at = 0; generation_text[at++] = '[';
            for (int i = 0; i < generations.count && at + 20 < sizeof generation_text; ++i)
                at += (size_t)snprintf(generation_text + at, sizeof generation_text - at, "%s%d", i ? "," : "", counts[i]);
            generation_text[at++] = ']'; generation_text[at] = 0;
        }
    }
    unsigned char raw[0xb0] = {0}; char raw_hex[0x161]; raw_hex[0] = 0;
    if (read_at(package + 0x60, raw, sizeof raw))
        for (size_t i = 0; i < sizeof raw; ++i) snprintf(raw_hex + i * 2, 3, "%02x", raw[i]);
    identity_linker(base, package, linker, sizeof linker);
    snprintf(message, sizeof message,
        "{\"identity\":\"package\",\"phase\":\"%s\",\"package\":\"%s\",\"path\":\"%s\",\"object_index\":%d,\"guid\":\"%08x-%08x-%08x-%08x\",\"generation_counts\":%s,\"linker\":%s,\"private_raw_60_110\":\"%s\",\"metadata_modified\":false}\r\n",
        phase, name, path_json, index, guid[0], guid[1], guid[2], guid[3], generation_text, linker, raw_hex);
    line(message);
    int owned = 0, class_count = 0, cdo_count = 0, linked_count = 0, unlinked_samples = 0, linked_samples = 0;
    for (int i = 0; i < object_count; ++i) {
        uintptr_t object = objects[i];
        if (!object || object == package || identity_root(object) != package) continue;
        owned++;
        char kind[96], object_name_text[96];
        object_name(base, read_ptr(object + 0x50), kind, sizeof kind);
        object_name(base, object, object_name_text, sizeof object_name_text);
        int is_class = strcmp(kind, "Class") == 0, is_cdo = strncmp(object_name_text, "Default__", 9) == 0;
        class_count += is_class; cdo_count += is_cdo;
        int linked = read_ptr(object + 0x2c) != 0; linked_count += linked;
        if (!((linked && linked_samples < 10) || (!linked && (is_class || is_cdo) && unlinked_samples < 6))) continue;
        if (linked) linked_samples++; else unlinked_samples++;
        identity_path(base, object, path, sizeof path); identity_quote(path, path_json, sizeof path_json);
        int32_t netindex = -1; int64_t linker_index = -1;
        read_at(object + 0x3c, &netindex, 4); read_at(object + 0x34, &linker_index, 8);
        identity_linker(base, object, linker, sizeof linker);
        snprintf(message, sizeof message,
            "{\"identity\":\"child\",\"phase\":\"%s\",\"package\":\"%s\",\"path\":\"%s\",\"class\":\"%s\",\"net_index\":%d,\"linker_index\":%lld,\"linker\":%s}\r\n",
            phase, name, path_json, kind, netindex, (long long)linker_index, linker);
        line(message);
    }
    int tables = 0;
    for (size_t offset = 0x60; offset + sizeof(GameArray) <= 0x110; offset += 4) {
        GameArray table = {0};
        if (!identity_array(package + offset, &table, 100000) || (table.data & 7)) continue;
        SIZE_T bytes = (SIZE_T)table.count * 8;
        uintptr_t *items = HeapAlloc(GetProcessHeap(), 0, bytes);
        if (!items) continue;
        if (!read_at(table.data, items, bytes)) { HeapFree(GetProcessHeap(), 0, items); continue; }
        int nonnull = 0, rooted = 0, zero_based = 0, one_based = 0;
        for (int i = 0; i < table.count; ++i) if (items[i]) {
            nonnull++;
            if (identity_root(items[i]) != package) continue;
            rooted++;
            int32_t net = -1; read_at(items[i] + 0x3c, &net, 4);
            zero_based += net == i; one_based += net == i + 1;
        }
        if (rooted < 3 || rooted * 10 < nonnull * 9) { HeapFree(GetProcessHeap(), 0, items); continue; }
        tables++;
        snprintf(message, sizeof message,
            "{\"identity\":\"net-table\",\"phase\":\"%s\",\"package\":\"%s\",\"offset\":%llu,\"count\":%d,\"nonnull\":%d,\"rooted\":%d,\"zero_based_matches\":%d,\"one_based_matches\":%d}\r\n",
            phase, name, (unsigned long long)offset, table.count, nonnull, rooted, zero_based, one_based);
        line(message);
        for (int i = 0; i < table.count; ++i) {
            uintptr_t object = items[i]; char kind[96] = ""; int32_t net = -1;
            path_json[0] = 0;
            if (object && identity_root(object) == package) {
                identity_path(base, object, path, sizeof path); identity_quote(path, path_json, sizeof path_json);
                object_name(base, read_ptr(object + 0x50), kind, sizeof kind); read_at(object + 0x3c, &net, 4);
            }
            snprintf(message, sizeof message,
                "{\"identity\":\"net-object\",\"phase\":\"%s\",\"package\":\"%s\",\"offset\":%llu,\"slot\":%d,\"path\":\"%s\",\"class\":\"%s\",\"net_index\":%d,\"nonnull\":%s}\r\n",
                phase, name, (unsigned long long)offset, i, path_json, kind, net, object ? "true" : "false");
            line(message);
        }
        HeapFree(GetProcessHeap(), 0, items);
    }
    snprintf(message, sizeof message,
        "{\"identity\":\"package-summary\",\"phase\":\"%s\",\"package\":\"%s\",\"owned_objects\":%d,\"classes\":%d,\"cdos\":%d,\"linked_objects\":%d,\"candidate_tables\":%d}\r\n",
        phase, name, owned, class_count, cdo_count, linked_count, tables);
    line(message);
}
static int identity_wanted(const char *name) {
    return strcmp(name, "Core") == 0 || strcmp(name, "SFXGame") == 0 || strcmp(name, "SFXGameContent") == 0 || strcmp(name, "BioP_Global") == 0;
}
static int identity_refresh_master(uintptr_t base) {
    GameArray array = {0}; uintptr_t *objects = identity_object_list(base, &array);
    if (!objects) return 0;
    uintptr_t driver = 0, packages[3] = {0};
    int drivers = 0, connections = 0, duplicate_packages = 0;
    for (int i = 0; i < array.count; ++i) {
        uintptr_t object = objects[i]; if (!object) continue;
        char kind[96], name[96];
        object_name(base, read_ptr(object + 0x50), kind, sizeof kind);
        object_name(base, object, name, sizeof name);
        if (!strncmp(name, "Default__", 9)) continue;
        if (strstr(kind, "NetDriver") && is_kind_of(base, object, "NetDriver")) { driver = object; drivers++; }
        if (strstr(kind, "Connection") && is_kind_of(base, object, "NetConnection")) connections++;
        if (strcmp(kind, "Package") || read_ptr(object + 0x40)) continue;
        int slot = !strcmp(name, "Core") ? 0 : !strcmp(name, "SFXGame") ? 1 : !strcmp(name, "SFXGameContent") ? 2 : -1;
        if (slot >= 0) { duplicate_packages += packages[slot] != 0; packages[slot] = object; }
    }
    HeapFree(GetProcessHeap(), 0, objects);
    int32_t client_count = -1, client_max = -1;
    uintptr_t map = drivers == 1 ? read_ptr(driver + 0x90) : 0;
    uintptr_t vt = map ? read_ptr(map) : 0;
    if (driver) { read_at(driver + 0x78, &client_count, 4); read_at(driver + 0x7c, &client_max, 4); }
    const unsigned char want[8] = {0x40,0x57,0x41,0x56,0x41,0x57,0x48,0x83}; unsigned char got[8];
    GameArray entries = {0}, network_packages = {0};
    int valid = drivers == 1 && !connections && client_count == 0 && client_max >= 0 && client_max <= 1024 &&
        !read_ptr(driver + 0x80) && !duplicate_packages &&
        packages[0] && packages[1] && packages[2] &&
        vt == base + 0x1086b10 && read_ptr(vt + 0x2e0) == base + 0xc8ea0 &&
        read_ptr(vt + 0x2d8) == base + 0xcac20 &&
        read_at(base + 0xc8ea0, got, sizeof got) && !memcmp(got, want, sizeof got) &&
        identity_array(map + 0x68, &entries, 20000) &&
        identity_array(base + 0x17f4b10, &network_packages, 20000);
    int matches[3] = {0}, registered[3] = {0}; uint32_t original_guids[3][4] = {{0}};
    if (valid) {
        for (int j = 0; j < 3; ++j) {
            read_at(packages[j] + 0x6c, original_guids[j], 16);
            for (int k = 0; k < entries.count; ++k) matches[j] += read_ptr(entries.data + (SIZE_T)k * 0x4c + 8) == packages[j];
            for (int k = 0; k < network_packages.count; ++k) registered[j] += read_ptr(network_packages.data + (SIZE_T)k * 8) == packages[j];
            valid = valid && matches[j] == 1 && registered[j] == 1;
        }
    }
    char text[600];
    snprintf(text, sizeof text,
        "{\"network\":\"master-map-refresh-preconditions\",\"valid\":%s,\"drivers\":%d,\"connections\":%d,\"client_count\":%d,\"entries_before\":%d,\"registered_packages\":%d}\r\n",
        valid ? "true" : "false", drivers, connections, client_count, entries.count, network_packages.count); line(text);
    if (!valid) return 0;
    /* Original UWorld::Listen uses this exact Init method. Rebuild only
       before any connection exists; no package/Guid or check is overwritten. */
    typedef void (*MapInit)(void *);
    ((MapInit)(base + 0xc8ea0))((void *)map);
    valid = identity_array(map + 0x68, &entries, 20000);
    for (int j = 0; j < 3 && valid; ++j) {
        uint32_t after[4]; valid = read_at(packages[j] + 0x6c, after, 16) && !memcmp(after, original_guids[j], 16);
        int matched = 0;
        for (int k = 0; k < entries.count; ++k) {
            uintptr_t info = entries.data + (SIZE_T)k * 0x4c; uint32_t mapped_guid[4];
            if (read_ptr(info + 8) == packages[j] && read_at(info + 0x10, mapped_guid, 16) && !memcmp(mapped_guid, after, 16)) matched++;
        }
        valid = valid && matched == 1;
    }
    snprintf(text, sizeof text,
        "{\"network\":\"master-map-refreshed\",\"verified\":%s,\"entries_after\":%d,\"resident_guids_modified\":false,\"validation_modified\":false}\r\n",
        valid ? "true" : "false", entries.count); line(text);
    if (valid) identity_host_driver = driver;
    return valid;
}
static void identity_snapshot(uintptr_t base, const char *phase) {
    GameArray array = {0}; uintptr_t *objects = identity_object_list(base, &array);
    if (!objects) { line("{\"identity\":\"objects-unreadable\"}\r\n"); return; }
    char message[800];
    snprintf(message, sizeof message, "{\"identity\":\"snapshot-begin\",\"phase\":\"%s\",\"objects\":%d}\r\n", phase, array.count); line(message);
    for (int i = 0; i < array.count; ++i) {
        uintptr_t object = objects[i]; if (!object) continue;
        char kind[96], name[96];
        object_name(base, read_ptr(object + 0x50), kind, sizeof kind);
        object_name(base, object, name, sizeof name);
        if (strcmp(kind, "Package") == 0 && identity_wanted(name) && !read_ptr(object + 0x40))
            identity_package(base, object, phase, objects, array.count);
        if (!strstr(kind, "PackageMap") || strncmp(name, "Default__", 9) == 0) continue;
        GameArray entries = {0};
        int readable = identity_array(object + 0x68, &entries, 20000);
        char map_path[400], map_json[810];
        identity_path(base, object, map_path, sizeof map_path); identity_quote(map_path, map_json, sizeof map_json);
        snprintf(message, sizeof message,
            "{\"identity\":\"package-map-header\",\"phase\":\"%s\",\"map\":\"%s\",\"path\":\"%s\",\"entries_readable\":%s,\"count\":%d}\r\n",
            phase, kind, map_json, readable ? "true" : "false", entries.count); line(message);
        if (!readable) continue;
        uint32_t first = 0; char first_name[96];
        if (!read_at(entries.data, &first, 4)) continue;
        name_of(base, first, first_name, sizeof first_name);
        /* A cooked map's first package need not be Core; examine every name
           instead of rejecting the complete map from its first entry. */
        for (int j = 0; j < entries.count; ++j) {
            uintptr_t info = entries.data + (SIZE_T)j * 0x4c; uint32_t fname = 0;
            /* Exact native constructor C5470 copies UPackage.Name(+48) to
               Info+0. +34 is a different FName (usually None), not Name. */
            read_at(info, &fname, 4); name_of(base, fname, first_name, sizeof first_name);
            if (!identity_wanted(first_name)) continue;
            uintptr_t parent = read_ptr(info + 8); char parent_name[96];
            object_name(base, parent, parent_name, sizeof parent_name);
            uint32_t guid[4] = {0}, parent_guid[4] = {0}; int32_t fields[4] = {0}, parent_index = -2;
            read_at(info + 0x10, guid, sizeof guid); read_at(info + 0x20, fields, sizeof fields);
            if (parent) { read_at(parent + 0x6c, parent_guid, sizeof parent_guid); read_at(parent + 8, &parent_index, 4); }
            snprintf(message, sizeof message,
                "{\"identity\":\"package-map-entry\",\"phase\":\"%s\",\"map\":\"%s\",\"package\":\"%s\",\"parent\":\"%s\",\"guid\":\"%08x-%08x-%08x-%08x\",\"parent_guid\":\"%08x-%08x-%08x-%08x\",\"parent_object_index\":%d,\"object_base\":%d,\"object_count\":%d,\"local_generation\":%d,\"remote_generation\":%d}\r\n",
                phase, kind, first_name, parent_name, guid[0], guid[1], guid[2], guid[3], parent_guid[0], parent_guid[1], parent_guid[2], parent_guid[3], parent_index, fields[0], fields[1], fields[2], fields[3]);
            line(message);
        }
    }
    HeapFree(GetProcessHeap(), 0, objects);
    snprintf(message, sizeof message, "{\"identity\":\"snapshot-end\",\"phase\":\"%s\",\"metadata_modified\":false}\r\n", phase); line(message);
}
