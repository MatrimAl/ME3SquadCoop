#ifndef ME3_NATIVE_REMOTE_CONTROLLER_H
#define ME3_NATIVE_REMOTE_CONTROLLER_H
/* Shared native remote-controller discovery; ambiguity is not an assignment. */
#ifdef ME3_PROTECTED_SESSION_ADMISSION_DIAGNOSTICS
typedef struct PairRemoteDiscoveryObservation {
    uintptr_t host,world;
    DWORD at;
    int snapshot_valid,class_prefilter,controller_kind,world_matches,reciprocal_matches;
} PairRemoteDiscoveryObservation;
static PairRemoteDiscoveryObservation pair_remote_discovery;
#endif
static uintptr_t pair_find_remote(uintptr_t base, uintptr_t host, uintptr_t world) {
#ifdef ME3_PROTECTED_SESSION_ADMISSION_DIAGNOSTICS
    memset(&pair_remote_discovery,0,sizeof pair_remote_discovery);
    pair_remote_discovery.host=host;pair_remote_discovery.world=world;pair_remote_discovery.at=GetTickCount();
#endif
    GameArray array = {0}; uintptr_t *objects = identity_object_list(base, &array);
    if (!objects) return 0;
#ifdef ME3_PROTECTED_SESSION_ADMISSION_DIAGNOSTICS
    pair_remote_discovery.snapshot_valid=1;
#endif
    uintptr_t found = 0; int matches = 0;
    for (int i = 0; i < array.count; ++i) {
        uintptr_t object = objects[i]; if (!object || object == host) continue;
        char kind[96]; object_name(base, read_ptr(object + 0x50), kind, sizeof kind);
        if (!strstr(kind, "PlayerController")) continue;
#ifdef ME3_PROTECTED_SESSION_ADMISSION_DIAGNOSTICS
        pair_remote_discovery.class_prefilter++;
#endif
        if (!is_kind_of(base, object, "BioPlayerController")) continue;
#ifdef ME3_PROTECTED_SESSION_ADMISSION_DIAGNOSTICS
        pair_remote_discovery.controller_kind++;
#endif
        if (read_ptr(object + 0x198) != world) continue;
#ifdef ME3_PROTECTED_SESSION_ADMISSION_DIAGNOSTICS
        pair_remote_discovery.world_matches++;
#endif
        uintptr_t player = read_ptr(object + 0x67c);
        if (is_live(base, player) && is_kind_of(base, player, "NetConnection") && read_ptr(player + 0x68) == object) { found = object; matches++; }
    }
#ifdef ME3_PROTECTED_SESSION_ADMISSION_DIAGNOSTICS
    pair_remote_discovery.reciprocal_matches=matches;
#endif
    HeapFree(GetProcessHeap(), 0, objects);
    return matches == 1 ? found : 0;
}
#endif
