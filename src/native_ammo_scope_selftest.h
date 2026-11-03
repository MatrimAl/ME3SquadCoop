#ifndef ME3_NATIVE_AMMO_SCOPE_SELFTEST_H
#define ME3_NATIVE_AMMO_SCOPE_SELFTEST_H
/* Runs inside the existing synthetic native-channel fixture, never a game. */
static int native_ammo_scope_identity_fixture(uintptr_t base,const uintptr_t *o) {
    NativeAmmoScope s={0};s.base=base;s.generation=3;s.epoch=4;s.ready=1;s.map=o[21];
    uintptr_t pc=o[20],pawn=o[5],weapon=o[18],world=o[2],connection=o[4];
    network_fixture_pointer(pc+0x3bc,pawn);network_fixture_pointer(pc+0x198,world);
    network_fixture_pointer(pc+0x67c,connection);network_fixture_pointer(pawn+0x374,pc);
    network_fixture_pointer(pawn+0x41c,weapon);network_fixture_pointer(weapon+0x180,pawn);
    network_fixture_pointer(weapon+0x190,pawn);network_fixture_pointer(connection+0xf0,s.map);
    int ok=guest_camera_stamp_read(base,pc,&s.pc,0)&&guest_camera_stamp_read(base,pawn,&s.pawn,0)&&
        guest_camera_stamp_read(base,weapon,&s.weapon,0)&&guest_camera_stamp_read(base,world,&s.world,0)&&
        guest_camera_stamp_read(base,o[3],&s.driver,0)&&guest_camera_stamp_read(base,connection,&s.connection,0)&&
        guest_camera_stamp_read(base,o[19],&s.actor_channel,0)&&guest_camera_stamp_read(base,s.map,&s.package_map,0)&&
        network_actor_channel_valid(base,weapon,o[19],o[3],&s.channel)&&native_ammo_scope_valid(&s,0);
    /* Every perturbation is restored before the channel fixture continues. */
    uintptr_t addresses[]={pc+0x3bc,pawn+0x374,pawn+0x41c,weapon+0x180,weapon+0x190,
        weapon+0x198,pc+0x198,pawn+0x198,connection+0xf0,connection+0xf84+9*8,o[19]+0xa0};
    for(unsigned i=0;i<sizeof addresses/sizeof *addresses;++i){
        uintptr_t saved=read_ptr(addresses[i]);network_fixture_pointer(addresses[i],0);
        ok=ok&&!native_ammo_scope_valid(&s,0);network_fixture_pointer(addresses[i],saved);
    }
    /* Another live map, recycled object index and changed captured channel
     * must fail even though the selected weapon's address still matches. */
    network_fixture_pointer(connection+0xf0,o[20]);ok=ok&&!native_ammo_scope_valid(&s,0);
    network_fixture_pointer(connection+0xf0,s.map);
    const GuestCameraStamp *stamps[]={&s.weapon,&s.pc,&s.pawn,&s.world,&s.driver,&s.connection,&s.actor_channel,&s.package_map};
    for(unsigned i=0;i<sizeof stamps/sizeof *stamps;++i){
        const GuestCameraStamp *stamp=stamps[i];int32_t other_index=99;
        memcpy((void*)(stamp->pointer+8),&other_index,4);ok=ok&&!native_ammo_scope_valid(&s,0);
        memcpy((void*)(stamp->pointer+8),&stamp->index,4);
        /* Still-live slot/class with another valid object name: live checks
         * alone cannot replace a captured lifetime identity. */
        uint32_t other_name=stamp->name[0]?0:128;
        memcpy((void*)(stamp->pointer+0x48),&other_name,4);ok=ok&&!native_ammo_scope_valid(&s,0);
        memcpy((void*)(stamp->pointer+0x48),&stamp->name[0],4);
    }
    ++s.channel.index;ok=ok&&!native_ammo_scope_valid(&s,0);--s.channel.index;
    s.ready=0;ok=ok&&!native_ammo_scope_valid(&s,0);s.ready=1;
    s.generation=0;ok=ok&&!native_ammo_scope_valid(&s,0);s.generation=3;
    s.epoch=0;ok=ok&&!native_ammo_scope_valid(&s,0);s.epoch=4;
    ok=ok&&!native_ammo_scope_valid(&s,1)&&!native_ammo_scope_valid(&s,2);
    /* Host validation additionally binds the remote PC's owning connection. */
    test_action=6;identity_host_driver=o[3];*(unsigned char*)(world+0x7c8)=2;
    network_fixture_pointer(o[3]+0x80,0);
    ok=ok&&native_ammo_scope_valid(&s,1)&&!native_ammo_scope_valid(&s,0);
    network_fixture_pointer(pc+0x67c,o[7]);ok=ok&&!native_ammo_scope_valid(&s,1);
    network_fixture_pointer(pc+0x67c,connection);
    test_action=5;identity_host_driver=0;*(unsigned char*)(world+0x7c8)=3;
    network_fixture_pointer(o[3]+0x80,connection);
    network_fixture_pointer(pawn+0x374,0);network_fixture_pointer(pawn+0x41c,0);
    network_fixture_pointer(weapon+0x180,0);network_fixture_pointer(weapon+0x190,0);
    network_fixture_pointer(connection+0xf0,0);
    return ok;
}
#endif
