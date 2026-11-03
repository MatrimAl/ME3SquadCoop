#ifndef ME3_SQUAD_CONTEXT_SUPPORT_H
#define ME3_SQUAD_CONTEXT_SUPPORT_H
/* Shared read-only adapter predicates; unchanged diagnostic implementations. */
static int scene_readiness_array(uintptr_t at, GameArray *a, int limit) {
    return read_at(at,a,sizeof *a) && a->count>=0 && a->count<=limit &&
        a->max>=a->count && a->max<=4096 && (!a->count || a->data);
}
static int scene_readiness_actor(uintptr_t base, uintptr_t actor) {
    uint32_t a=0,b=0;
    return is_live(base,actor) && is_kind_of(base,actor,"Actor") &&
        read_at(actor+0x25c,&a,4) && read_at(actor+0x260,&b,4) &&
        !(a&8) && !(b&0x800000);
}
static const char *narrative_probe_bool(int known, int value) {
    return known ? (value ? "true" : "false") : "null";
}
static int narrative_probe_object(uintptr_t base, uintptr_t object, const char *kind) {
    return is_live(base,object) && is_kind_of(base,object,kind);
}
#endif
