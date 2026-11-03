#ifndef ME3_GUEST_HIT_DAMAGE_INPUTS_H
#define ME3_GUEST_HIT_DAMAGE_INPUTS_H
/* Read-only inputs at the existing selected-weapon drain observation.
 * No getter replay, damage dispatch, property write or verdict. */
static GuestCameraStamp guest_hit_damage_fields[6];
static int guest_hit_damage_layout_ready;
static uintptr_t guest_hit_damage_field(uintptr_t base,uintptr_t klass,const char *owner,const char *name) {
    for(int depth=0;klass&&depth<16;++depth){
        if(!is_live(base,klass))return 0;char text[96];object_name(base,klass,text,sizeof text);
        if(!strcmp(text,owner)){
            uintptr_t child=read_ptr(klass+0x70);
            for(int n=0;child&&n<1024;++n){
                if(!is_live(base,child))return 0;object_name(base,child,text,sizeof text);
                if(!strcmp(text,name))return child;
                child=read_ptr(child+0x68);
            }return 0;
        }klass=read_ptr(klass+0x60);
    }return 0;
}
static int guest_hit_damage_layout(uintptr_t base,uintptr_t klass) {
    const char *names[]={"Damage","DamageHench","bSuppressDamage","DefaultFireMode","InstantHitDamageTypes"};
    const char *types[]={"StructProperty","FloatProperty","BoolProperty","ByteProperty","ArrayProperty"};
    const int offsets[]={0x43c,0xf80,0x10b4,0x10be,0x354},sizes[]={40,4,4,1,16};
    const uint64_t flags[]={0x404001,0x4001,1,0,0x400000};
    if(guest_hit_damage_layout_ready){
        for(int i=0;i<6;++i)if(!guest_camera_stamp_matches(base,&guest_hit_damage_fields[i],1))return 0;
        return 1;
    }
    for(int i=0;i<5;++i){
        const char *owner=i==4?"Weapon":"SFXWeapon";char path[180];int32_t size=0;
        uintptr_t prop=guest_hit_damage_field(base,klass,owner,names[i]);
        snprintf(path,sizeof path,"%s.%s.%s",i==4?"Engine":"SFXGame",owner,names[i]);
        if(!combat_fire_property(base,prop,types[i],path,offsets[i],flags[i])||
           !read_at(prop+0x74,&size,4)||size!=sizes[i]||
           !guest_camera_stamp_read(base,prop,&guest_hit_damage_fields[i],1))return 0;
    }
    uint64_t mask=0;uintptr_t structure=read_ptr(guest_hit_damage_fields[0].pointer+0xd0);
    if(!read_at(guest_hit_damage_fields[2].pointer+0xd0,&mask,8)||mask!=0x20||
       !narrative_probe_object(base,structure,"ScriptStruct"))return 0;
    uintptr_t value=read_ptr(structure+0x70);int found=0;
    for(int n=0;value&&n<128;++n){
        if(!is_live(base,value))return 0;char name[96];object_name(base,value,name,sizeof name);
        if(!strcmp(name,"Value")){
            int32_t offset=-1,size=0;
            if(!narrative_probe_object(base,value,"FloatProperty")||read_ptr(value+0x40)!=structure||
               !read_at(value+0x94,&offset,4)||offset!=0x20||!read_at(value+0x74,&size,4)||size!=4||
               !guest_camera_stamp_read(base,value,&guest_hit_damage_fields[5],1))return 0;
            ++found;
        }value=read_ptr(value+0x68);
    }
    guest_hit_damage_layout_ready=found==1;return guest_hit_damage_layout_ready;
}
static void guest_hit_damage_inputs(const GuestHitFrame *frame) {
    uintptr_t base=ammo_wire_scope.base,weapon=frame->weapon;
    if(!guest_hit_damage_layout(base,read_ptr(weapon+0x50))){
        line("{\"guest_hit_damage_inputs\":\"unavailable\",\"reason\":\"reflected-layout\",\"read_only\":true}\r\n");return;
    }
    float damage=0,hench=0;uint32_t flags=0;unsigned char default_mode=255;GameArray types={0};
    int damage_valid=read_at(weapon+0x45c,&damage,4)&&isfinite(damage);
    int hench_valid=read_at(weapon+0xf80,&hench,4)&&isfinite(hench);
    int flags_valid=read_at(weapon+0x10b4,&flags,4),default_valid=read_at(weapon+0x10be,&default_mode,1);
    int array_valid=guest_hit_array(weapon+0x354,&types,32);
    int mode=frame->mode?frame->mode:(default_valid?default_mode:-1);
    uintptr_t type=0,instigator=read_ptr(weapon+0x190);
    int type_valid=array_valid&&mode>=0&&mode<types.count&&read_at(types.data+(uintptr_t)mode*8,&type,8);
    char damage_text[48],hench_text[48],type_text[1800],row[2800];
    narrative_probe_float(damage_text,damage_valid,damage);narrative_probe_float(hench_text,hench_valid,hench);
    narrative_probe_identity(base,type,type_valid,type_text,sizeof type_text);
    snprintf(row,sizeof row,"{\"guest_hit_damage_inputs\":\"sample\",\"call\":%u,\"weapon_index\":%d,\"layout_verified\":true,\"damage_value\":%s,\"damage_hench\":%s,\"suppress_damage\":%s,\"instigator_is_henchman\":%s,\"firing_mode\":%u,\"damage_type_array_valid\":%s,\"damage_type_count\":%d,\"selected_type_index\":%d,\"damage_type\":%s,\"read_only\":true,\"damage_verified\":false,\"monotonic_ms\":%lu}\r\n",
        frame->id,frame->weapon_index,damage_text,hench_text,narrative_probe_bool(flags_valid,(flags&0x20)!=0),
        narrative_probe_bool(is_live(base,instigator),is_kind_of(base,instigator,"SFXPawn_Henchman")),
        (unsigned)frame->mode,array_valid?"true":"false",array_valid?types.count:0,mode,type_text,GetTickCount());line(row);
}
#endif
