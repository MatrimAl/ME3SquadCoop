#ifndef ME3_GUEST_CAMERA_WEAPON_ACTION_H
#define ME3_GUEST_CAMERA_WEAPON_ACTION_H
/* Presentation-only retention during a real native draw/holster action.
 * Read the current pawn's actual action tables, not a global numeric ID.
 * Supported-build BioPawn arrays69c/6ac and BioCustomAction links98/a0/a8,
 * camera fields78/b0 are from the retained SDK and shipped package defaults. */
static int guest_camera_weapon_action(uintptr_t base,uintptr_t pc,uintptr_t pawn,int32_t index) {
    GameArray classes={0},actions={0},again={0};
    if(index<=0||index>=512||!read_at(pawn+0x69c,&classes,sizeof classes)||
       !read_at(pawn+0x6ac,&actions,sizeof actions)||!classes.data||!actions.data||
       classes.count<=index||classes.count>512||classes.max<classes.count||classes.max>1024||
       actions.count<=index||actions.count>512||actions.max<actions.count||actions.max>1024)return 0;
    uintptr_t klass=0,action=0,owner=0,controller=0,ai=1,requested_camera=1,active_camera=1;
    GuestCameraStamp type={0},object={0};
    if(!read_at(classes.data+(size_t)index*8,&klass,8)||!read_at(actions.data+(size_t)index*8,&action,8)||
       !is_live(base,klass)||!is_kind_of(base,klass,"Class")||!is_live(base,action)||
       !is_kind_of(base,action,"BioCustomAction")||read_ptr(action+0x50)!=klass||
       !guest_camera_stamp_read(base,klass,&type,1)||!guest_camera_stamp_read(base,action,&object,1)||
       (strcmp(type.full_name,"SFXGame.SFXCustomAction_DrawWeapon")&&strcmp(type.full_name,"SFXGame.SFXCustomAction_HolsterWeapon"))||
       !read_at(action+0x98,&owner,8)||owner!=pawn||!read_at(action+0xa0,&controller,8)||controller!=pc||
       !read_at(action+0xa8,&ai,8)||ai||
       !read_at(action+0x78,&requested_camera,8)||requested_camera||!read_at(action+0xb0,&active_camera,8)||active_camera)return 0;
    int32_t current=-1;
    return read_at(pawn+0xc20,&current,4)&&current==index&&
        read_at(pawn+0x69c,&again,sizeof again)&&!memcmp(&again,&classes,sizeof again)&&
        read_at(pawn+0x6ac,&again,sizeof again)&&!memcmp(&again,&actions,sizeof again)&&
        read_ptr(classes.data+(size_t)index*8)==klass&&read_ptr(actions.data+(size_t)index*8)==action&&
        guest_camera_stamp_matches(base,&type,0)&&guest_camera_stamp_matches(base,&object,0)&&
        read_ptr(action+0x98)==pawn&&read_ptr(action+0xa0)==pc&&
        read_at(action+0xa8,&ai,8)&&!ai;
}
#endif
