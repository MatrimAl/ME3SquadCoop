#ifndef ME3_NATIVE_AMMO_SCOPE_H
#define ME3_NATIVE_AMMO_SCOPE_H
/* Read-only local weapon scope. Requires existing stamp/channel helpers.
 * Caller owns game-thread admission, session policy and scope discovery.
 * Channel indices are connection/lifetime scoped, not peer-stable actor IDs. */
typedef struct NativeAmmoScope {
    uintptr_t base,map;unsigned long generation,epoch;
    GuestCameraStamp pc,pawn,weapon,world,properties[2];
    GuestCameraStamp driver,connection,actor_channel,package_map;
    NetworkActorIdentityEntry channel;
    int ready;
} NativeAmmoScope;
static int native_ammo_scope_valid(const NativeAmmoScope *s,int host) {
    NetworkActorIdentityEntry ch={0};NetworkActorScope world;
    if((host!=0&&host!=1)||native_session_role()!=(host?6:5))return 0;
    return s->ready&&s->base&&s->generation&&s->epoch&&
        guest_camera_stamp_matches(s->base,&s->pc,0)&&
        guest_camera_stamp_matches(s->base,&s->pawn,0)&&guest_camera_stamp_matches(s->base,&s->weapon,0)&&
        guest_camera_stamp_matches(s->base,&s->world,0)&&
        s->driver.pointer==s->channel.driver&&s->connection.pointer==s->channel.connection&&
        s->actor_channel.pointer==s->channel.channel&&s->package_map.pointer==s->map&&
        guest_camera_stamp_matches(s->base,&s->driver,0)&&guest_camera_stamp_matches(s->base,&s->connection,0)&&
        guest_camera_stamp_matches(s->base,&s->actor_channel,0)&&guest_camera_stamp_matches(s->base,&s->package_map,0)&&
        is_kind_of(s->base,s->pc.pointer,"PlayerController")&&
        is_kind_of(s->base,s->pawn.pointer,"Pawn")&&is_kind_of(s->base,s->weapon.pointer,"SFXWeapon_NativeBase")&&
        read_ptr(s->pc.pointer+0x3bc)==s->pawn.pointer&&read_ptr(s->pawn.pointer+0x374)==s->pc.pointer&&
        read_ptr(s->pawn.pointer+0x41c)==s->weapon.pointer&&read_ptr(s->weapon.pointer+0x180)==s->pawn.pointer&&
        read_ptr(s->weapon.pointer+0x190)==s->pawn.pointer&&read_ptr(s->weapon.pointer+0x198)==s->world.pointer&&
        read_ptr(s->pc.pointer+0x198)==s->world.pointer&&read_ptr(s->pawn.pointer+0x198)==s->world.pointer&&
        network_actor_read_scope(s->base,s->weapon.pointer,&world)&&world.info==s->world.pointer&&world.driver==s->channel.driver&&
        network_actor_channel_valid(s->base,s->weapon.pointer,s->channel.channel,s->channel.driver,&ch)&&
        ch.connection==s->channel.connection&&ch.index==s->channel.index&&
        read_ptr(ch.connection+0xf0)==s->map&&is_live(s->base,s->map)&&is_kind_of(s->base,s->map,"PackageMap")&&
        (!host||read_ptr(s->pc.pointer+0x67c)==ch.connection);
}
#endif
