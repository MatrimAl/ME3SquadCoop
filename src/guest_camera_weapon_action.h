#ifndef ME3_GUEST_CAMERA_WEAPON_ACTION_H
#define ME3_GUEST_CAMERA_WEAPON_ACTION_H
/* Presentation-only retention during real native weapon and power actions.
 * Read the current pawn's actual action tables, not a global numeric ID.
 * Supported-build BioPawn arrays69c/6ac and BioCustomAction links98/a0/a8,
 * camera fields78/b0 are from the retained SDK and shipped package defaults. */
static int guest_camera_action_policy(int holding,int custom,int power,int weapon,int power_instance) {
    if(custom<0||power<0)return 0;
    if(!custom&&!power)return 1;
    if(!holding)return 0;
    if(custom==132)return power>0&&power_instance; /* Supported SDK CA_Power. */
    return !power&&weapon;
}
static int guest_camera_native_action(uintptr_t base,uintptr_t pc,uintptr_t pawn,int32_t index,int power,uint32_t *flags_out) {
    GameArray classes={0},actions={0},again={0};
    uintptr_t class_field=pawn+(power?0x6bc:0x69c),action_field=pawn+(power?0x6cc:0x6ac);
    if(index<=0||index>=512||!read_at(class_field,&classes,sizeof classes)||
       !read_at(action_field,&actions,sizeof actions)||!classes.data||!actions.data||
       classes.count<=index||classes.count>512||classes.max<classes.count||classes.max>1024||
       actions.count<=index||actions.count>512||actions.max<actions.count||actions.max>1024)return 0;
    uintptr_t klass=0,action=0,owner=0,requested_camera=1,active_camera=1;
    GuestCameraStamp type={0},object={0};
    if(!read_at(classes.data+(size_t)index*8,&klass,8)||!read_at(actions.data+(size_t)index*8,&action,8)||
       !is_live(base,klass)||!is_kind_of(base,klass,"Class")||!is_live(base,action)||
       !is_kind_of(base,action,"BioCustomAction")||read_ptr(action+0x50)!=klass||
       !guest_camera_stamp_read(base,klass,&type,1)||!guest_camera_stamp_read(base,action,&object,1)||
       (power?!is_kind_of(base,action,"SFXPowerCustomActionBase"):
        (strcmp(type.full_name,"SFXGame.SFXCustomAction_DrawWeapon")&&strcmp(type.full_name,"SFXGame.SFXCustomAction_HolsterWeapon")&&
         !is_kind_of(base,action,"SFXCustomAction_ReloadBase")))||
       !read_at(action+0x98,&owner,8)||owner!=pawn||read_ptr(pawn+0x374)!=pc||read_ptr(pc+0x3bc)!=pawn||
       !read_at(action+0x100,flags_out,4)||
       (!power&&(!read_at(action+0x78,&requested_camera,8)||requested_camera||!read_at(action+0xb0,&active_camera,8)||active_camera)))return 0;
    int32_t current=-1;
    return read_at(pawn+(power?0xc28:0xc20),&current,4)&&current==index&&
        read_at(class_field,&again,sizeof again)&&!memcmp(&again,&classes,sizeof again)&&
        read_at(action_field,&again,sizeof again)&&!memcmp(&again,&actions,sizeof again)&&
        read_ptr(classes.data+(size_t)index*8)==klass&&read_ptr(actions.data+(size_t)index*8)==action&&
        guest_camera_stamp_matches(base,&type,0)&&guest_camera_stamp_matches(base,&object,0)&&
        read_ptr(action+0x98)==pawn&&read_ptr(pawn+0x374)==pc&&read_ptr(pc+0x3bc)==pawn;
}
/* This only retains an existing camera scalar. The action's historical AI/PC
 * caches do not select its current human owner and are neither invoked nor
 * modified here; current native action tables and reciprocal possession do. */
#endif
