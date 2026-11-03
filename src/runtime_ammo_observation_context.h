#ifndef ME3_RUNTIME_AMMO_OBSERVATION_CONTEXT_H
#define ME3_RUNTIME_AMMO_OBSERVATION_CONTEXT_H
/* Ordinary observational context only; no ownership or packet authentication. */
typedef struct RuntimeAmmoReceiveFrame {
    struct RuntimeAmmoReceiveFrame *previous;
    uintptr_t channel;
    unsigned generation,id,counts[2],nested_receives;
    uint32_t errors_before[2],errors_after[2];
    int active,valid,accepted[2],decoded[2],before[2],fire_before,weapon_index,channel_index,fire_coverage;
    uint64_t begin_order,decode_order[2],fire_at_decode[2],fire_before_count;
} RuntimeAmmoReceiveFrame;
static void (*runtime_ammo_receive_begin_observer)(RuntimeAmmoReceiveFrame*,uintptr_t);
static void (*runtime_ammo_receive_end_observer)(RuntimeAmmoReceiveFrame*);
static int runtime_ammo_observation_enabled;
static uint64_t runtime_ammo_observation_order,runtime_ammo_observation_fire_calls;
static uint64_t runtime_ammo_observation_next(void) {
    if(!runtime_ammo_observation_enabled)return 0;
    if(++runtime_ammo_observation_order==0)++runtime_ammo_observation_order;
    return runtime_ammo_observation_order;
}
#endif
