#ifndef ME3_COMBAT_EQUIPMENT_PROBE_H
#define ME3_COMBAT_EQUIPMENT_PROBE_H

/* Read-only equipment snapshot, called at the existing 1Hz pawn sampling rate.
 * SDK Engine_classes: Pawn.InvManager414/Weapon41c, Actor.Owner180,
 * Instigator190; Pawn.ShotCount564/FlashCount582/FiringMode583;
 * SFXGame_classes:19597 SFXWeapon_NativeBase SpareAmmo3b4,
 * AmmoUsedCount3b8/flags3bc. Raw counts alone are not a firing/damage verdict.
 * This does not initialize, equip, unfold, reload or fire a weapon.
 */
static void combat_equipment_sample(uintptr_t base,uintptr_t pawn,const char *role,DWORD now) {
    if(!scene_readiness_actor(base,pawn)||!is_kind_of(base,pawn,"BioPawn"))return;
    uintptr_t inventory=0,weapon=0,owner=0,instigator=0;
    int inventory_read=read_at(pawn+0x414,&inventory,8),weapon_read=read_at(pawn+0x41c,&weapon,8);
    int weapon_valid=weapon_read&&scene_readiness_actor(base,weapon)&&is_kind_of(base,weapon,"SFXWeapon_NativeBase");
    int owner_read=weapon_valid&&read_at(weapon+0x180,&owner,8);
    int instigator_read=weapon_valid&&read_at(weapon+0x190,&instigator,8);
    int32_t spare=0,used=0;uint32_t flags=0,pawn_flags=0;
    int counts_read=weapon_valid&&read_at(weapon+0x3b4,&spare,4)&&read_at(weapon+0x3b8,&used,4);
    int flags_read=weapon_valid&&read_at(weapon+0x3bc,&flags,4);
    int pawn_flags_read=read_at(pawn+0x578,&pawn_flags,4);
    char pawn_id[1600],weapon_id[1600],inventory_id[1600],row[10000];
    narrative_probe_identity(base,pawn,1,pawn_id,sizeof pawn_id);
    narrative_probe_identity(base,weapon,weapon_read,weapon_id,sizeof weapon_id);
    narrative_probe_identity(base,inventory,inventory_read,inventory_id,sizeof inventory_id);
    snprintf(row,sizeof row,"{\"combat_equipment_probe\":\"weapon\",\"role\":\"%s\",\"pawn\":%s,\"weapon\":%s,\"inventory_manager\":%s,\"weapon_class_valid\":%s,\"owner_is_pawn\":%s,\"instigator_is_pawn\":%s,\"ammo_counts_read_valid\":%s,\"current_spare_ammo\":%d,\"ammo_used_count\":%d,\"weapon_flags_read_valid\":%s,\"weapon_flags_raw\":%u,\"weapon_expanded\":%s,\"weapon_initialized\":%s,\"weapon_infinite_ammo\":%s,\"pawn_flags_read_valid\":%s,\"pawn_no_weapon_firing\":%s,\"firing_tested\":false,\"read_only\":true,\"monotonic_ms\":%lu}\r\n",
        role,pawn_id,weapon_id,inventory_id,narrative_probe_bool(1,weapon_valid),
        narrative_probe_bool(owner_read,owner==pawn),narrative_probe_bool(instigator_read,instigator==pawn),
        narrative_probe_bool(1,counts_read),spare,used,narrative_probe_bool(1,flags_read),flags,
        narrative_probe_bool(flags_read,flags&1),narrative_probe_bool(flags_read,flags&0x1000),narrative_probe_bool(flags_read,flags&0x200),
        narrative_probe_bool(1,pawn_flags_read),narrative_probe_bool(pawn_flags_read,pawn_flags&0x10000),now);
    line(row);
    uintptr_t inventory_owner=0,pawn_owner=0;unsigned char weapon_role=255,remote_role=255;
    int inventory_valid=inventory_read&&scene_readiness_actor(base,inventory)&&is_kind_of(base,inventory,"InventoryManager");
    int inventory_owner_read=inventory_valid&&read_at(inventory+0x180,&inventory_owner,8);
    int pawn_owner_read=read_at(pawn+0x180,&pawn_owner,8);
    int roles_read=weapon_valid&&read_at(weapon+0x26a,&weapon_role,1)&&read_at(weapon+0x269,&remote_role,1);
    char owner_id[1600],inventory_owner_id[1600],pawn_owner_id[1600];
    narrative_probe_identity(base,owner,owner_read,owner_id,sizeof owner_id);
    narrative_probe_identity(base,inventory_owner,inventory_owner_read,inventory_owner_id,sizeof inventory_owner_id);
    narrative_probe_identity(base,pawn_owner,pawn_owner_read,pawn_owner_id,sizeof pawn_owner_id);
    snprintf(row,sizeof row,"{\"combat_equipment_probe\":\"owner-chain\",\"role\":\"%s\",\"pawn\":%s,\"weapon\":%s,\"weapon_owner\":%s,\"inventory_owner\":%s,\"pawn_owner\":%s,\"roles_read_valid\":%s,\"weapon_role\":%u,\"weapon_remote_role\":%u,\"read_only\":true,\"monotonic_ms\":%lu}\r\n",
        role,pawn_id,weapon_id,owner_id,inventory_owner_id,pawn_owner_id,narrative_probe_bool(1,roles_read),weapon_role,remote_role,now);
    line(row);
    uintptr_t weapon_inventory=0,inventory_instigator=0;
    int weapon_inventory_read=weapon_valid&&read_at(weapon+0x2b0,&weapon_inventory,8);
    int inventory_instigator_read=inventory_valid&&read_at(inventory+0x190,&inventory_instigator,8);
    char weapon_inventory_id[1600],inventory_instigator_id[1600];
    narrative_probe_identity(base,weapon_inventory,weapon_inventory_read,weapon_inventory_id,sizeof weapon_inventory_id);
    narrative_probe_identity(base,inventory_instigator,inventory_instigator_read,inventory_instigator_id,sizeof inventory_instigator_id);
    snprintf(row,sizeof row,"{\"combat_equipment_probe\":\"inventory-links\",\"role\":\"%s\",\"pawn\":%s,\"weapon\":%s,\"pawn_inventory_manager\":%s,\"weapon_inventory_manager\":%s,\"inventory_instigator\":%s,\"read_only\":true,\"monotonic_ms\":%lu}\r\n",
        role,pawn_id,weapon_id,inventory_id,weapon_inventory_id,inventory_instigator_id,now);
    line(row);
    int32_t shots=0;unsigned char flash=0,firing_mode=255;float last_fire=0;
    int shot_fields=read_at(pawn+0x564,&shots,4)&&read_at(pawn+0x582,&flash,1)&&read_at(pawn+0x583,&firing_mode,1);
    int last_fire_read=weapon_valid&&is_kind_of(base,weapon,"SFXWeapon")&&
        read_at(weapon+0x109c,&last_fire,4)&&isfinite(last_fire);
    char last_fire_json[48];narrative_probe_float(last_fire_json,last_fire_read,last_fire);
    snprintf(row,sizeof row,"{\"combat_equipment_probe\":\"shot-state\",\"role\":\"%s\",\"pawn\":%s,\"weapon\":%s,\"shot_fields_read_valid\":%s,\"pawn_shot_count\":%d,\"pawn_flash_count\":%u,\"pawn_firing_mode\":%u,\"last_fire_time_read_valid\":%s,\"weapon_last_fire_time\":%s,\"snapshot_is_firing_verdict\":false,\"read_only\":true,\"monotonic_ms\":%lu}\r\n",
        role,pawn_id,weapon_id,narrative_probe_bool(1,shot_fields),shots,flash,firing_mode,
        narrative_probe_bool(1,last_fire_read),last_fire_json,now);
    line(row);
}
#endif
