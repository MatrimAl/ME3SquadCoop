#ifndef ME3_SQUAD_PAWN_READINESS_H
#define ME3_SQUAD_PAWN_READINESS_H
/* Shared read-only readiness; native_state_reader.h must be included first. */
static void progress_state_name(uintptr_t base, uintptr_t object, char *out, size_t size) {
    if(!native_state_name(base,object,out,size))snprintf(out,size,"unknown");
}
static int progress_pawn_ready(uintptr_t base, uintptr_t pawn, uintptr_t ai) {
    int32_t action = -1, power = -1; uint32_t a = 0, b = 0, c = 0, attach = 0;
    unsigned char physics = 0; uint32_t actor_flags = 0;
    if (!is_live(base, pawn) || !read_at(pawn + 0xc20, &action, 4) || !read_at(pawn + 0xc28, &power, 4) ||
        !read_at(pawn + 0xe18, &a, 4) || !read_at(pawn + 0xe1c, &b, 4) || !read_at(pawn + 0xe20, &c, 4) ||
        !read_at(pawn + 0x25c, &actor_flags, 4) || !read_at(pawn + 0x260, &attach, 4) || !read_at(pawn + 0x268, &physics, 1)) return 0;
    char state[96]; progress_state_name(base, ai, state, sizeof state);
    if (is_kind_of(base,ai,"AIController") && (read_ptr(ai+0x484) || read_ptr(ai+0x48c))) return 0;
    return action == 0 && power == 0 && (a & 0x40) && !(a & (0x200000 | 0x20000000)) &&
           !(b & 0x10000000) && !(c & 0x40000) && !(attach & 0x400) && !(actor_flags & 2) && physics == 1 &&
           !strstr(state, "Script") && !strstr(state, "Cinematic");
}
static int progress_companion_near(uintptr_t host, uintptr_t companion) {
    float a[3],b[3];
    return read_at(host+0x108,a,sizeof a) && read_at(companion+0x108,b,sizeof b) &&
           isfinite(a[0]) && isfinite(a[1]) && isfinite(a[2]) && isfinite(b[0]) && isfinite(b[1]) && isfinite(b[2]) && distance(a,b) <= 2000.0f;
}
#endif
