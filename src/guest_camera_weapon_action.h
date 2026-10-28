#ifndef ME3_GUEST_CAMERA_WEAPON_ACTION_H
#define ME3_GUEST_CAMERA_WEAPON_ACTION_H
#include "guest_cover_melee_layout.h"
/* Presentation-only retention during real native weapon, power and evade actions.
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
static int guest_camera_evade_policy(int index,int player_evade,int pc_matches,int ai_empty,
        int requested_roll,int active_camera_matches) {
    return index>=54&&index<=57&&player_evade&&pc_matches&&ai_empty&&requested_roll&&active_camera_matches;
}
static int guest_camera_melee_policy(int index,int player_melee,int canonical_class,int pc_matches,
        int ai_empty,uintptr_t requested,uintptr_t active) {
    return ((index>=75&&index<=80)||index==58||index==73)&&player_melee&&canonical_class&&pc_matches&&ai_empty&&!requested&&!active;
}
static int guest_camera_cover_policy(int index,int canonical,int player_melee,int pc_matches,int ai_empty,
        int requested_supported,int active_matches) {
    return index>=62&&index<=71&&canonical&&player_melee&&pc_matches&&ai_empty&&requested_supported&&active_matches;
}
static int guest_camera_cover_mode_name(const char *name) {
    return !strcmp(name,"SFXGame.SFXCameraMode_CoverMeleeLeft")||!strcmp(name,"SFXGame.SFXCameraMode_CoverMeleeRight")||
        !strcmp(name,"SFXGame.SFXCameraMode_CoverSyncMelee")||!strcmp(name,"SFXGame.SFXCameraMode_CoverSyncMeleeLeft");
}
static int guest_camera_ignore_move_policy(unsigned ignore,int power,int evade,uint32_t action_flags) {
    return !ignore||(ignore==1&&(power||evade)&&(action_flags&4));
}
/* kind0=draw/holster/reload,1=power,2=player evades,3=player melee. */
static int guest_camera_native_action(uintptr_t base,uintptr_t pc,uintptr_t pawn,int32_t index,int kind,uint32_t *flags_out) {
    GameArray classes={0},actions={0},again={0};
    int power=kind==1,evade=kind==2,melee=kind==3,cover=melee&&index>=62&&index<=71;
    if(kind<0||kind>3)return 0;
    uintptr_t class_field=pawn+(power?0x6bc:0x69c),action_field=pawn+(power?0x6cc:0x6ac);
    if(index<=0||index>=512||!read_at(class_field,&classes,sizeof classes)||
       !read_at(action_field,&actions,sizeof actions)||!classes.data||!actions.data||
       classes.count<=index||classes.count>512||classes.max<classes.count||classes.max>1024||
       actions.count<=index||actions.count>512||actions.max<actions.count||actions.max>1024)return 0;
    uintptr_t klass=0,action=0,owner=0,requested_camera=0,active_camera=0;
    GuestCameraStamp type={0},object={0},requested={0},active={0};
    if(!read_at(classes.data+(size_t)index*8,&klass,8)||!read_at(actions.data+(size_t)index*8,&action,8)||
       !is_live(base,klass)||!is_kind_of(base,klass,"Class")||!is_live(base,action)||
       !is_kind_of(base,action,"BioCustomAction")||read_ptr(action+0x50)!=klass||
       !guest_camera_stamp_read(base,klass,&type,1)||!guest_camera_stamp_read(base,action,&object,1)||
       !read_at(action+0x98,&owner,8)||owner!=pawn||read_ptr(pawn+0x374)!=pc||read_ptr(pc+0x3bc)!=pawn||
       !read_at(action+0x100,flags_out,4))return 0;
    if(power) {
        if(!is_kind_of(base,action,"SFXPowerCustomActionBase"))return 0;
    } else if(evade) {
        if(!read_at(action+0x78,&requested_camera,8)||!read_at(action+0xb0,&active_camera,8)||
           !is_kind_of(base,requested_camera,"Class")||!guest_camera_stamp_read(base,requested_camera,&requested,1)||
           strcmp(requested.full_name,"SFXGame.SFXCameraMode_Roll"))return 0;
        int camera_matches=!active_camera;
        if(active_camera) {
            uint32_t camera_flags=0;
            camera_matches=guest_camera_stamp_read(base,active_camera,&active,0)&&
                active.klass==requested_camera&&is_kind_of(base,active_camera,"SFXCameraMode_Roll")&&
                read_ptr(active_camera+0x7c)==pawn&&read_at(active_camera+0x11c,&camera_flags,4)&&!(camera_flags&0x20);
        }
        if(!guest_camera_evade_policy(index,is_kind_of(base,action,"SFXCustomAction_PlayerEvadeBase"),
           read_ptr(action+0xa0)==pc,!read_ptr(action+0xa8),1,camera_matches))return 0;
    } else if(cover) {
        if(!read_at(action+0x78,&requested_camera,8)||!read_at(action+0xb0,&active_camera,8))return 0;
        int requested_supported=!requested_camera,active_matches=!active_camera;
        if(requested_camera){
            requested_supported=is_live(base,requested_camera)&&is_kind_of(base,requested_camera,"Class")&&
                guest_camera_stamp_read(base,requested_camera,&requested,1)&&guest_camera_cover_mode_name(requested.full_name);
            if(active_camera){uint32_t camera_flags=0;
                active_matches=guest_camera_stamp_read(base,active_camera,&active,0)&&active.klass==requested_camera&&
                    read_ptr(active_camera+0x7c)==pawn&&read_at(active_camera+0x11c,&camera_flags,4)&&!(camera_flags&0x20)&&(camera_flags&2);
            }
        }
        if(!guest_camera_cover_policy(index,!strcmp(type.full_name,guest_cover_melee_class(index)),
            is_kind_of(base,action,guest_cover_melee_synced(index)?"SFXCustomAction_ClassMelee":"SFXCustomAction_PlayerMeleeBase"),
            read_ptr(action+0xa0)==pc,!read_ptr(action+0xa8),requested_supported,active_matches))return 0;
    } else if(melee) {
        static const char *names[]={"SFXGameContent.SFXCustomAction_RifleMeleeOne","SFXGameContent.SFXCustomAction_RifleMeleeTwo",
            "SFXGameContent.SFXCustomAction_RifleMeleeThree","SFXGameContent.SFXCustomAction_PistolMeleeOne",
            "SFXGameContent.SFXCustomAction_PistolMeleeTwo","SFXGameContent.SFXCustomAction_PistolMeleeThree"};
        int heavy=index==58||index==73;
        if((!heavy&&(index<75||index>80))||!read_at(action+0x78,&requested_camera,8)||!read_at(action+0xb0,&active_camera,8)||
           !guest_camera_melee_policy(index,is_kind_of(base,action,heavy?"SFXCustomAction_PlayerHeavyMeleeBase":"SFXCustomAction_PlayerMeleeBase"),
               !strcmp(type.full_name,heavy?"SFXGameContent.SFXCustomAction_SoldierPunch":names[index-75]),
               read_ptr(action+0xa0)==pc,!read_ptr(action+0xa8),requested_camera,active_camera))return 0;
    } else {
        if((strcmp(type.full_name,"SFXGame.SFXCustomAction_DrawWeapon")&&strcmp(type.full_name,"SFXGame.SFXCustomAction_HolsterWeapon")&&
            !is_kind_of(base,action,"SFXCustomAction_ReloadBase"))||
           !read_at(action+0x78,&requested_camera,8)||requested_camera||
           !read_at(action+0xb0,&active_camera,8)||active_camera)return 0;
    }
    int32_t current=-1;
    return read_at(pawn+(power?0xc28:0xc20),&current,4)&&current==index&&
        read_at(class_field,&again,sizeof again)&&!memcmp(&again,&classes,sizeof again)&&
        read_at(action_field,&again,sizeof again)&&!memcmp(&again,&actions,sizeof again)&&
        read_ptr(classes.data+(size_t)index*8)==klass&&read_ptr(actions.data+(size_t)index*8)==action&&
        guest_camera_stamp_matches(base,&type,0)&&guest_camera_stamp_matches(base,&object,0)&&
        read_ptr(action+0x98)==pawn&&read_ptr(pawn+0x374)==pc&&read_ptr(pc+0x3bc)==pawn&&
        (!(evade||melee)||(read_ptr(action+0xa0)==pc&&!read_ptr(action+0xa8)&&
          read_ptr(action+0x78)==requested_camera&&read_ptr(action+0xb0)==active_camera&&
          ((melee&&!cover)||(!requested_camera?(!active_camera):
           guest_camera_stamp_matches(base,&requested,0)&&(!active_camera||guest_camera_stamp_matches(base,&active,0))))));
}
/* This only retains an existing camera scalar. The action's historical AI/PC
 * caches are not invoked or changed. Evade additionally requires stock's real
 * local-player cache and canonical Roll camera. Melee requires the real
 * local-player cache and no requested action camera; reload/power retention
 * remains independent of their historical controller caches. */
#endif
