#ifndef ME3_RUNTIME_POWER_RELOAD_ADMISSION_H
#define ME3_RUNTIME_POWER_RELOAD_ADMISSION_H
/* Stock UsePower owns reload cancellation and blocking-action queuing. Only
 * a real reload or one of the four owned PlayerEvade actions can enter here;
 * no action, movement, camera, permission or input-ignore state is changed. */
static int runtime_power_action_input_state(int index,int power,unsigned move,unsigned look,uint32_t pawn_flags,uint32_t life,
        int evade,uint32_t action_flags) {
    return index>0&&index<512&&!power&&(!move||(move==1&&evade&&(action_flags&4)))&&
        !look&&!(pawn_flags&0x20200000)&&!(life&0x40300);
}
static int runtime_power_reload_state(int index,int power,unsigned move,unsigned look,uint32_t pawn_flags,uint32_t life) {
    return runtime_power_action_input_state(index,power,move,look,pawn_flags,life,0,0);
}
static int runtime_power_evade_input_class(int index,const char *path,int player_evade,int pc_matches,int ai_empty) {
    static const char *names[]={"SFXGameContent.SFXCustomAction_PlayerEvadeLeft","SFXGameContent.SFXCustomAction_PlayerEvadeRight",
        "SFXGameContent.SFXCustomAction_PlayerEvadeForward","SFXGameContent.SFXCustomAction_PlayerEvadeBackwards"};
    return index>=54&&index<=57&&path&&player_evade&&pc_matches&&ai_empty&&!strcmp(path,names[index-54]);
}
static uintptr_t runtime_power_queued_action_instance(uintptr_t base,SessionContext c) {
    int32_t index=-1,power=-1;unsigned char move=255,look=255;uint32_t flags=0,life=0;
    GameArray classes={0},actions={0},again={0};GuestCameraStamp instance={0},type={0};
    if(g_runtime_config.role!=ME3_ROLE_GUEST||
       !read_at(c.pawn+0xc20,&index,4)||!read_at(c.pawn+0xc28,&power,4)||
       !read_at(c.pc+0x781,&move,1)||!read_at(c.pc+0x782,&look,1)||
       !read_at(c.pawn+0xe18,&flags,4)||!read_at(c.pawn+0xe20,&life,4)||
       !runtime_power_action_input_state(index,power,move,look,flags,life,1,4)||
       !runtime_power_property(base,c.pawn,"BioPawn","CustomActionClasses","ArrayProperty",0x69c,16)||
       !runtime_power_property(base,c.pawn,"BioPawn","CustomActions","ArrayProperty",0x6ac,16)||
       !read_at(c.pawn+0x69c,&classes,sizeof classes)||!read_at(c.pawn+0x6ac,&actions,sizeof actions)||
       !classes.data||!actions.data||classes.count<=index||actions.count<=index||classes.count>512||actions.count>512||
       classes.max<classes.count||actions.max<actions.count||classes.max>1024||actions.max>1024)return 0;
    uintptr_t klass=read_ptr(classes.data+(SIZE_T)index*8),action=read_ptr(actions.data+(SIZE_T)index*8);
    if(!narrative_probe_object(base,klass,"Class")||!narrative_probe_object(base,action,"BioCustomAction")||
       read_ptr(action+0x50)!=klass||read_ptr(action+0x98)!=c.pawn||
       !guest_camera_stamp_read(base,action,&instance,0)||!guest_camera_stamp_read(base,klass,&type,0)||
       read_ptr(c.pawn+0x374)!=c.pc||read_ptr(c.pc+0x3bc)!=c.pawn)return 0;
    int evade=runtime_power_evade_input_class(index,type.full_name,is_kind_of(base,action,"SFXCustomAction_PlayerEvadeBase"),
        read_ptr(action+0xa0)==c.pc,!read_ptr(action+0xa8));uint32_t action_flags=0;
    if((!evade&&!is_kind_of(base,action,"SFXCustomAction_ReloadBase"))||!read_at(action+0x100,&action_flags,4)||
       !runtime_power_action_input_state(index,power,move,look,flags,life,evade,action_flags))return 0;
    int32_t current=-1,current_power=-1;
    return read_at(c.pawn+0xc20,&current,4)&&current==index&&read_at(c.pawn+0xc28,&current_power,4)&&!current_power&&
        read_at(c.pawn+0x69c,&again,sizeof again)&&!memcmp(&again,&classes,sizeof again)&&
        read_at(c.pawn+0x6ac,&again,sizeof again)&&!memcmp(&again,&actions,sizeof again)&&
        read_ptr(classes.data+(SIZE_T)index*8)==klass&&read_ptr(actions.data+(SIZE_T)index*8)==action&&
        guest_camera_stamp_matches(base,&instance,0)&&guest_camera_stamp_matches(base,&type,0)?action:0;
}
static int runtime_power_input_ready(uintptr_t base,SessionContext c) {
    return runtime_power_dispatch_idle(c)||runtime_power_queued_action_instance(base,c)!=0;
}
#endif
