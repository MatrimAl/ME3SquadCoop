#ifndef ME3_AMMO_CHANGE_TRACE_H
#define ME3_AMMO_CHANGE_TRACE_H

/* Opt-in, read-only post-Tick samples. Flags are observed OUTSIDE replication;
 * they are never a verdict about a particular connection's condition or packet.
 * No ProcessEvent, RPC, counter, flag or scheduling writes. Exact-build Actor:
 * flags25c dirty100000/skip1000000/onlyDirty8000000; flags260 forceUpdate100;
 * flags264 netOwner4000; update204/frequency208/lastUpdate210.
 */
typedef struct AmmoTraceFilter { unsigned rows; int have,used,spare,limited; } AmmoTraceFilter;
static int ammo_trace_take(AmmoTraceFilter *f,int new_context,int used,int spare) {
    int event=!f->have||new_context?1:(f->used!=used||f->spare!=spare?2:0);
    if(!event)return 0;
    if(f->rows>=128){if(f->limited)return 0;f->limited=1;return 3;}
    f->have=1;f->used=used;f->spare=spare;++f->rows;return event;
}
typedef struct AmmoChangeTrace {
    AmmoTraceFilter filter;
    unsigned long epoch;
    GuestCameraStamp pc,pawn,weapon,world;
    DWORD last_sample;
} AmmoChangeTrace;
static AmmoChangeTrace ammo_change_trace;

static void ammo_change_sample(uintptr_t base,uintptr_t pc,uintptr_t pawn,uintptr_t world,
                                unsigned long epoch,DWORD now) {
    if(!progress_enabled||!epoch||!has_marker(root)||(test_action!=5&&test_action!=6)||
       !guest_camera_actor_live(base,pc)||!guest_camera_actor_live(base,pawn)||
       !is_kind_of(base,pc,"PlayerController")||
       !is_kind_of(base,pawn,"SFXPawn_Ashley")||read_ptr(pc+0x3bc)!=pawn||read_ptr(pawn+0x374)!=pc||
       read_ptr(pawn+0x198)!=world||read_ptr(pc+0x198)!=world)return;
    uintptr_t weapon=read_ptr(pawn+0x41c);unsigned char net=255,role=255;
    now=GetTickCount(); /* Caller may have performed expensive bootstrap discovery. */
    int32_t used=0,spare=0;uint32_t flags[3]={0};float update=0,frequency=0,last=0,world_time=0;
    if(!guest_camera_actor_live(base,weapon)||!is_kind_of(base,weapon,"SFXWeapon_NativeBase")||
       read_ptr(weapon+0x180)!=pawn||read_ptr(weapon+0x190)!=pawn||read_ptr(weapon+0x198)!=world||
       !read_at(world+0x7c8,&net,1)||net!=(test_action==6?2:3)||
       !read_at(weapon+0x26a,&role,1)||(test_action==6?role!=3:(role<1||role>2))||
       !read_at(weapon+0x3b8,&used,4)||!read_at(weapon+0x3b4,&spare,4)||
       !read_at(weapon+0x25c,flags,sizeof flags))return;
    AmmoChangeTrace *t=&ammo_change_trace;
    int same=t->epoch==epoch&&t->pc.pointer==pc&&t->pawn.pointer==pawn&&t->weapon.pointer==weapon&&t->world.pointer==world&&
        guest_camera_stamp_matches(base,&t->pc,0)&&guest_camera_stamp_matches(base,&t->pawn,0)&&
        guest_camera_stamp_matches(base,&t->weapon,0)&&guest_camera_stamp_matches(base,&t->world,0);
    if(!same){
        if(!guest_camera_stamp_read(base,pc,&t->pc,1)||!guest_camera_stamp_read(base,pawn,&t->pawn,1)||
           !guest_camera_stamp_read(base,weapon,&t->weapon,1)||!guest_camera_stamp_read(base,world,&t->world,1))return;
        t->epoch=epoch;
    }
    DWORD interval=same&&t->last_sample?now-t->last_sample:0;t->last_sample=now;
    int event=ammo_trace_take(&t->filter,!same,used,spare);
    if(!event)return;
    if(event==3){line("{\"ammo_trace\":\"limit\",\"sample_budget\":128,\"read_only\":true}\r\n");return;}
    int schedule=read_at(weapon+0x204,&update,4)&&read_at(weapon+0x208,&frequency,4)&&
        read_at(weapon+0x210,&last,4)&&read_at(world+0x74c,&world_time,4)&&
        isfinite(update)&&isfinite(frequency)&&isfinite(last)&&isfinite(world_time);
    char u[48],f[48],l[48],w[48];narrative_probe_float(u,schedule,update);narrative_probe_float(f,schedule,frequency);
    narrative_probe_float(l,schedule,last);narrative_probe_float(w,schedule,world_time);
    int pawn_channel=network_actor_channel_id(base,pawn),weapon_channel=network_weapon_channel_id(base,weapon);
    char row[2400];snprintf(row,sizeof row,
        "{\"ammo_trace\":\"%s\",\"role\":\"%s\",\"epoch\":%lu,\"epoch_scope\":\"%s\",\"pawn_pointer\":\"0x%llx\",\"pawn_index\":%d,\"weapon_pointer\":\"0x%llx\",\"weapon_index\":%d,\"pawn_channel_index\":%d,\"weapon_channel_index\":%d,\"ammo_used_count\":%d,\"current_spare_ammo\":%d,\"weapon_role\":%u,\"actor_flags25c\":%u,\"actor_flags260\":%u,\"actor_flags264\":%u,\"net_dirty\":%s,\"skip_actor_properties\":%s,\"only_dirty_replication\":%s,\"force_net_update\":%s,\"net_owner_sample\":%s,\"net_update_time\":%s,\"net_update_frequency\":%s,\"last_net_update_time\":%s,\"world_time\":%s,\"sample_interval_ms\":%lu,\"monotonic_ms\":%lu,\"sampled_outside_replication\":true,\"packet_delivery_verified\":false,\"read_only\":true}\r\n",
        event==1?"baseline":"changed",test_action==6?"host-ashley":"guest-local",epoch,
        test_action==6?"host-ownership":"guest-context",(unsigned long long)pawn,t->pawn.index,
        (unsigned long long)weapon,t->weapon.index,pawn_channel,weapon_channel,used,spare,(unsigned)role,
        flags[0],flags[1],flags[2],flags[0]&0x100000?"true":"false",flags[0]&0x1000000?"true":"false",
        flags[0]&0x8000000?"true":"false",flags[1]&0x100?"true":"false",flags[2]&0x4000?"true":"false",
        u,f,l,w,interval,now);line(row);
}
static int ammo_trace_selftest(void) {
    AmmoTraceFilter f={0};
    if(ammo_trace_take(&f,0,0,210)!=1||ammo_trace_take(&f,0,0,210)!=0||
       ammo_trace_take(&f,0,1,210)!=2||ammo_trace_take(&f,0,1,209)!=2||
       ammo_trace_take(&f,1,0,210)!=1)return 0;
    while(f.rows<128)if(ammo_trace_take(&f,1,0,210)!=1)return 0;
    return ammo_trace_take(&f,1,0,210)==3&&ammo_trace_take(&f,1,99,0)==0&&f.rows==128;
}
#endif
