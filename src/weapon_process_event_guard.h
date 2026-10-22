#ifndef ME3_WEAPON_PROCESS_EVENT_GUARD_H
#define ME3_WEAPON_PROCESS_EVENT_GUARD_H
/* The ordinary entry has no observer dependency and accepts only Actor PE.
 * Only the protected probe compiles the exact observer's additional check. */
#ifdef ME3_SQUAD_COOP_PROTECTED_HIT_TRACE
static int guest_hit_weapon_pe_matches(uintptr_t base,uintptr_t weapon,uintptr_t pe);
#endif
static int guest_equipment_weapon_pe_valid(uintptr_t base,uintptr_t weapon,uintptr_t pe) {
    if(pe==base+0x40d450)return 1;
#ifdef ME3_SQUAD_COOP_PROTECTED_HIT_TRACE
    return guest_hit_weapon_pe_matches(base,weapon,pe);
#else
    (void)weapon;return 0;
#endif
}
#endif
