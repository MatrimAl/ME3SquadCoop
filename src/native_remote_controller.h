#ifndef ME3_NATIVE_REMOTE_CONTROLLER_H
#define ME3_NATIVE_REMOTE_CONTROLLER_H
/* Shared native remote-controller discovery; ambiguity is not an assignment. */
static uintptr_t pair_find_remote(uintptr_t base, uintptr_t host, uintptr_t world) {
    GameArray array = {0}; uintptr_t *objects = identity_object_list(base, &array);
    if (!objects) return 0;
    uintptr_t found = 0; int matches = 0;
    for (int i = 0; i < array.count; ++i) {
        uintptr_t object = objects[i]; if (!object || object == host) continue;
        char kind[96]; object_name(base, read_ptr(object + 0x50), kind, sizeof kind);
        if (!strstr(kind, "PlayerController") || !is_kind_of(base, object, "BioPlayerController") || read_ptr(object + 0x198) != world) continue;
        uintptr_t player = read_ptr(object + 0x67c);
        if (is_live(base, player) && is_kind_of(base, player, "NetConnection") && read_ptr(player + 0x68) == object) { found = object; matches++; }
    }
    HeapFree(GetProcessHeap(), 0, objects);
    return matches == 1 ? found : 0;
}
#endif
