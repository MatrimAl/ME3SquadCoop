#ifndef ME3_RUNTIME_GUEST_MELEE_H
#define ME3_RUNTIME_GUEST_MELEE_H
/* Prepare the six normal player melee combo slots missing from Ashley.
 * Stock input, animation, targeting, authority damage and action replication
 * remain responsible for execution. Add the pawn's OWN melee passive through
 * PowerManager.AddPower; never borrow Shepard's power instance or stats. */
typedef struct RuntimeMeleeRow {GuestCameraStamp klass,cdo,instance;uint32_t anim[2];int changed;} RuntimeMeleeRow;
typedef struct RuntimeGuestMelee {
    RuntimeMovementScope scope;
    SessionHandle source,manager;
    GuestCameraStamp passive_class,passive_cdo,passive;
    GuestPlayerContextMethod add,remove,register_anim,unregister_anim;
    GuestCameraStamp anim_set;
    RuntimeMeleeRow rows[6];
    int valid,retiring,created,anim_registered,foreign;
    DWORD checked,monitor;
} RuntimeGuestMelee;
static RuntimeGuestMelee runtime_guest_melee;
static unsigned runtime_melee_events;
static int runtime_melee_power_id(uintptr_t action) {int32_t id=-1;return read_at(action+0x484,&id,4)&&id==43;}
static const char *runtime_melee_names[]={"SFXCustomAction_RifleMeleeOne","SFXCustomAction_RifleMeleeTwo",
    "SFXCustomAction_RifleMeleeThree","SFXCustomAction_PistolMeleeOne","SFXCustomAction_PistolMeleeTwo",
    "SFXCustomAction_PistolMeleeThree"};
#pragma pack(push,4)
typedef struct RuntimeMeleeAddParms {uintptr_t klass,result;} RuntimeMeleeAddParms;
typedef struct RuntimeMeleeRemoveParms {uintptr_t klass;uint32_t result;} RuntimeMeleeRemoveParms;
typedef struct RuntimeMeleeAnimParms {uintptr_t set;float time;uint32_t force;} RuntimeMeleeAnimParms;
#pragma pack(pop)
_Static_assert(sizeof(RuntimeMeleeAddParms)==16,"AddPower ABI16");
_Static_assert(sizeof(RuntimeMeleeRemoveParms)==12,"RemovePower ABI12");
static int runtime_guest_melee_enabled(void) {
    static int enabled=-1;
    if(enabled<0){wchar_t text[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_GUEST_MELEE",text,8);
        enabled=!n||(n==1&&text[0]==L'1');}return enabled;
}
static void runtime_melee_note(const char *event,unsigned slots,DWORD now) {
    if(runtime_melee_events++>=48)return;char text[360];
    snprintf(text,sizeof text,"{\"runtime_guest_melee\":\"%s\",\"role\":%d,\"normal_combo_slots\":%u,\"heavy_melee_ready\":false,\"damage_verified\":false,\"monotonic_ms\":%lu}\r\n",event,(int)g_runtime_config.role,slots,now);line(text);
}
static int runtime_melee_arrays(uintptr_t base,RuntimeGuestMelee *q,GameArray *pc,GameArray *pa,GameArray *powers) {
    uintptr_t pawn=q->scope.pawn.pointer,manager=q->manager.pointer;
    return runtime_movement_arrays_live(base,&q->scope)&&q->scope.classes.count>80&&
        session_handle_matches(q->manager)&&narrative_probe_object(base,manager,"SFXPowerManager")&&
        read_ptr(pawn+0xc18)==manager&&read_ptr(manager+0xd0)==pawn&&
        runtime_power_property(base,pawn,"BioPawn","PowerCustomActionClasses","ArrayProperty",0x6bc,16)&&
        runtime_power_property(base,pawn,"BioPawn","PowerCustomActions","ArrayProperty",0x6cc,16)&&
        runtime_power_property(base,manager,"SFXPowerManager","Powers","ArrayProperty",0x60,16)&&
        runtime_power_property(base,manager,"SFXPowerManager","MyPawn","ObjectProperty",0xd0,8)&&
        scene_readiness_array(pawn+0x6bc,pc,512)&&scene_readiness_array(pawn+0x6cc,pa,512)&&
        scene_readiness_array(manager+0x60,powers,RUNTIME_POWER_LIMIT);
}
static int runtime_melee_method(uintptr_t base,uintptr_t manager,const char *name,
        GuestPlayerContextMethod *out,int add) {
    uintptr_t fn=guest_player_context_find_method(base,read_ptr(manager+0x50),"SFXPowerManager",name);GameArray script={0};
    if(!guest_player_context_method_stamp(base,fn,out)||
       !guest_player_context_function_abi(base,fn,"SFXPowerManager",name,0x20002,add?16:12,8,0,0xf52b0,2)||
       !read_at(fn+0x7c,&script,sizeof script)||!script.data||script.count!=(add?667:232)||
       script.max<script.count||script.max>4096||guest_equipment_find_first(base,read_ptr(manager+0x50),name)!=fn)return 0;
    uintptr_t first=read_ptr(fn+0x70),second=read_ptr(first+0x68);char path[160];
    snprintf(path,sizeof path,"SFXGame.SFXPowerManager.%s.PowerClass",name);
    if(!runtime_delayed_fire_property(base,first,"ClassProperty",path,0,8,0x80)||read_ptr(first+0x40)!=fn)return 0;
    snprintf(path,sizeof path,"SFXGame.SFXPowerManager.%s.ReturnValue",name);
    return read_ptr(second+0x40)==fn&&runtime_delayed_fire_property(base,second,add?"ObjectProperty":"BoolProperty",path,8,add?8:4,0x580);
}
static uintptr_t runtime_melee_cell(const GameArray *a,unsigned index) {
    return a->data&&index<(unsigned)a->count?a->data+(SIZE_T)index*8:0;
}
static int runtime_melee_passive_valid(uintptr_t base,RuntimeGuestMelee *q,uintptr_t action) {
    return narrative_probe_object(base,action,"SFXPowerCustomAction_MeleePassivePower")&&
        read_ptr(action+0x50)==q->passive_class.pointer&&read_ptr(action+0x98)==q->scope.pawn.pointer&&
        runtime_power_action_context_layout(base,action)&&runtime_melee_power_id(action);
}
/* Only the existing live player in this world supplies loaded class refs.
 * A bounded native PawnList replaces a global object/class-name search. */
static int runtime_melee_source(uintptr_t base,RuntimeGuestMelee *q) {
    uintptr_t source=0,head=read_ptr(q->scope.info.pointer+0x6c8),pawn=head,seen[256];unsigned count=0;
    if(!runtime_power_property(base,q->scope.info.pointer,"WorldInfo","PawnList","ObjectProperty",0x6c8,8))return 0;
    while(pawn&&count<256) {
        for(unsigned i=0;i<count;++i)if(seen[i]==pawn)return 0;seen[count++]=pawn;
        if(!guest_camera_actor_live(base,pawn)||!is_kind_of(base,pawn,"Pawn")||read_ptr(pawn+0x198)!=q->scope.info.pointer)return 0;
        if(is_kind_of(base,pawn,"SFXPawn_Player")){if(source)return 0;source=pawn;}
        pawn=read_ptr(pawn+0x37c);
    }
    if(pawn||!source||source==q->scope.pawn.pointer||read_ptr(q->scope.info.pointer+0x6c8)!=head)return 0;
    q->source=session_capture_handle(source);GameArray classes={0},powers={0};
    if(!session_handle_matches(q->source)||!identity_array(source+0x69c,&classes,512)||classes.count<=80||
       !identity_array(source+0x6bc,&powers,512)||powers.count<=43)return 0;
    for(unsigned i=0;i<6;++i){RuntimeMeleeRow *row=&q->rows[i];uintptr_t klass=read_ptr(classes.data+(SIZE_T)(75+i)*8);
        if(!runtime_movement_named_class(base,klass,runtime_melee_names[i],&row->klass)||
           !guest_camera_stamp_read(base,read_ptr(klass+0x13c),&row->cdo,1)||read_ptr(row->cdo.pointer+0x50)!=klass||
           !is_kind_of(base,row->cdo.pointer,"SFXCustomAction_PlayerMeleeBase")||
           !runtime_power_property(base,row->cdo.pointer,"SFXCustomAction_PlayerMeleeBase","Power","ObjectProperty",0x1fc,8))return 0;
        GameArray anim={0};
        if(!runtime_power_property(base,row->cdo.pointer,"SFXCustomAction_SingleAnim","BS_Anim","StructProperty",0x110,16)||
           !identity_array(row->cdo.pointer+0x110,&anim,8)||anim.count<1)return 0;
        unsigned clips=0;uint32_t name[2];
        for(int n=0;n<anim.count;++n){if(!read_at(anim.data+(SIZE_T)n*8,name,8)||name[1])return 0;
            if(name[0]){if(clips++)return 0;memcpy(row->anim,name,8);}}
        if(clips!=1)return 0;
    }
    uintptr_t klass=read_ptr(powers.data+43*8);
    return runtime_movement_named_class(base,klass,"SFXPowerCustomAction_SoldierMeleePassive",&q->passive_class)&&
        guest_camera_stamp_read(base,read_ptr(klass+0x13c),&q->passive_cdo,1)&&
        read_ptr(q->passive_cdo.pointer+0x50)==klass&&is_kind_of(base,q->passive_cdo.pointer,"SFXPowerCustomAction_MeleePassivePower")&&
        runtime_power_property(base,q->passive_cdo.pointer,"SFXPowerCustomActionBase","PowerCustomActionID","IntProperty",0x484,4)&&
        runtime_melee_power_id(q->passive_cdo.pointer);
}
static int runtime_melee_clips_loaded(uintptr_t base,RuntimeGuestMelee *q) {
    uintptr_t mesh=read_ptr(q->scope.pawn.pointer+0x3f4),sequence_class=0;GameArray sets={0},again={0};unsigned found=0,total=0;
    if(!narrative_probe_object(base,mesh,"SkeletalMeshComponent")||
       !runtime_power_property(base,mesh,"SkeletalMeshComponent","AnimSets","ArrayProperty",0x320,16)||
       !identity_array(mesh+0x320,&sets,64))return 0;
    for(int i=0;i<sets.count;++i){uintptr_t set=read_ptr(sets.data+(SIZE_T)i*8);GameArray sequences={0},checked={0};
        if(!set)continue;
        if(!narrative_probe_object(base,set,"AnimSet")||!runtime_power_property(base,set,"AnimSet","Sequences","ArrayProperty",0x60,16)||
           !scene_readiness_array(set+0x60,&sequences,4096)||(total+=(unsigned)sequences.count)>8192)return 0;
        for(int n=0;n<sequences.count;++n){uintptr_t seq=read_ptr(sequences.data+(SIZE_T)n*8);uint32_t name[2];
            if(!seq)continue;if(!narrative_probe_object(base,seq,"AnimSequence"))return 0;
            uintptr_t klass=read_ptr(seq+0x50);
            if(klass!=sequence_class){if(!runtime_power_property(base,seq,"AnimSequence","SequenceName","NameProperty",0xf4,8))return 0;sequence_class=klass;}
            if(!read_at(seq+0xf4,name,8))return 0;
            for(unsigned j=0;j<6;++j)if(!memcmp(name,q->rows[j].anim,8))found|=1u<<j;
        }
        if(!read_at(set+0x60,&checked,sizeof checked)||memcmp(&sequences,&checked,sizeof checked))return 0;
    }
    return found==63&&read_ptr(q->scope.pawn.pointer+0x3f4)==mesh&&read_at(mesh+0x320,&again,sizeof again)&&!memcmp(&sets,&again,sizeof sets);
}
static int runtime_melee_anim_method(uintptr_t base,uintptr_t pawn,int add,GuestPlayerContextMethod *out) {
    const char *name=add?"RegisterTemporaryAnim":"UnregisterTemporaryAnim";
    uintptr_t fn=guest_player_context_find_method(base,read_ptr(pawn+0x50),"BioPawn",name);GameArray script={0};
    if(!guest_player_context_method_stamp(base,fn,out)||
       !guest_player_context_function_abi(base,fn,"BioPawn",name,add?0x20103:0x24103,add?8:16,65535,0,0xf52b0,add?1:3)||
       !read_at(fn+0x7c,&script,sizeof script)||!script.data||script.count!=(add?396:430)||
       script.max<script.count||script.max>4096||guest_equipment_find_first(base,read_ptr(pawn+0x50),name)!=fn)return 0;
    char path[160];uintptr_t field=read_ptr(fn+0x70);
    snprintf(path,sizeof path,"SFXGame.BioPawn.%s.TempAnim",name);
    if(!runtime_delayed_fire_property(base,field,"ObjectProperty",path,0,8,0x80))return 0;
    if(add)return 1;field=read_ptr(field+0x68);
    snprintf(path,sizeof path,"SFXGame.BioPawn.%s.fTimeLeft",name);
    if(!runtime_delayed_fire_property(base,field,"FloatProperty",path,8,4,0x90))return 0;field=read_ptr(field+0x68);
    snprintf(path,sizeof path,"SFXGame.BioPawn.%s.bForceRemove",name);
    return runtime_delayed_fire_property(base,field,"BoolProperty",path,12,4,0x90);
}
static int runtime_melee_temp_refcount(uintptr_t base,RuntimeGuestMelee *q) {
    GameArray array={0};int count=-1;unsigned found=0;uintptr_t pawn=q->scope.pawn.pointer;
    if(!runtime_power_property(base,pawn,"BioPawn","TemporaryAnims","ArrayProperty",0x76c,16)||
       !scene_readiness_array(pawn+0x76c,&array,128))return -1;
    for(int i=0;i<array.count;++i)if(read_ptr(array.data+(SIZE_T)i*16)==q->anim_set.pointer){
        if(found++||!read_at(array.data+(SIZE_T)i*16+8,&count,4)||count<0||count>4096)return -1;
    }
    return found==1?count:0;
}
static int runtime_melee_find_anim_set(uintptr_t base,RuntimeGuestMelee *q) {
    uintptr_t mesh=read_ptr(q->source.pointer+0x3f4);GameArray sets={0};
    if(!session_handle_matches(q->source)||!narrative_probe_object(base,mesh,"SkeletalMeshComponent")||
       !identity_array(mesh+0x320,&sets,64))return 0;
    for(int i=0;i<sets.count;++i){uintptr_t set=read_ptr(sets.data+(SIZE_T)i*8);GameArray sequences={0};unsigned mask=0;
        if(!set)continue;if(!narrative_probe_object(base,set,"AnimSet")||!scene_readiness_array(set+0x60,&sequences,4096))return 0;
        for(int n=0;n<sequences.count;++n){uintptr_t seq=read_ptr(sequences.data+(SIZE_T)n*8);uint32_t name[2];
            if(!seq)continue;if(!narrative_probe_object(base,seq,"AnimSequence")||!read_at(seq+0xf4,name,8))return 0;
            for(unsigned j=3;j<6;++j)if(!memcmp(name,q->rows[j].anim,8))mask|=1u<<j;
        }
        if(mask==56)return guest_camera_stamp_read(base,set,&q->anim_set,1);
    }
    return 0;
}
static int runtime_melee_instance(uintptr_t base,RuntimeGuestMelee *q,unsigned i,uintptr_t action) {
    return narrative_probe_object(base,action,"SFXCustomAction_PlayerMeleeBase")&&
        read_ptr(action+0x50)==q->rows[i].klass.pointer&&read_ptr(action+0x98)==q->scope.pawn.pointer&&
        runtime_power_action_context_layout(base,action)&&
        (!read_ptr(action+0x1fc)||read_ptr(action+0x1fc)==q->passive.pointer);
}
static int runtime_melee_instance_idle(uintptr_t action) {
    uint32_t flags=0;return read_at(action+0x100,&flags,4)&&!(flags&0x10800000)&&!read_ptr(action+0xb8);
}
static int runtime_melee_contains(const GameArray *powers,uintptr_t value) {
    unsigned count=0;for(int i=0;i<powers->count;++i)if(read_ptr(powers->data+(SIZE_T)i*8)==value)++count;
    return count==1;
}
static int runtime_melee_owns_passive(uintptr_t base,RuntimeGuestMelee *q,GameArray *pc,GameArray *pa,GameArray *powers) {
    return runtime_melee_arrays(base,q,pc,pa,powers)&&guest_camera_stamp_matches(base,&q->passive_class,1)&&
        guest_camera_stamp_matches(base,&q->passive,1)&&runtime_melee_passive_valid(base,q,q->passive.pointer)&&
        read_ptr(runtime_melee_cell(pc,43))==q->passive_class.pointer&&
        read_ptr(runtime_melee_cell(pa,43))==q->passive.pointer&&runtime_melee_contains(powers,q->passive.pointer);
}
static void runtime_guest_melee_retire(uintptr_t base,uintptr_t engine,DWORD now) {
    RuntimeGuestMelee *q=&runtime_guest_melee;if(!q->valid||GetCurrentThreadId()!=session_game_thread)return;q->retiring=1;GameArray pc={0},pa={0},powers={0};
    if(!runtime_melee_arrays(base,q,&pc,&pa,&powers)){runtime_melee_note("preserved-replaced-scope",0,now);memset(q,0,sizeof *q);return;}
    uintptr_t owner=read_ptr(q->scope.pawn.pointer+0x374);
    if(!runtime_movement_retire_owner_policy(owner,q->scope.pc.pointer,session_handle_matches(q->scope.pc),
            guest_camera_actor_live(base,owner)&&is_kind_of(base,owner,"SFXAI_Henchman"),
            read_ptr(owner+0x3bc)==q->scope.pawn.pointer,read_ptr(owner+0x198)==q->scope.info.pointer)){
        runtime_melee_note("preserved-new-owner",0,now);memset(q,0,sizeof *q);return;}
    if(!runtime_movement_idle(q->scope.pawn.pointer))return;
    unsigned restored=0;
    for(unsigned i=0;i<6;++i){RuntimeMeleeRow *row=&q->rows[i];if(!row->changed)continue;
        uintptr_t cell=runtime_melee_cell(&q->scope.actions,75+i),action=cell?read_ptr(cell):0;
        uintptr_t class_cell=q->scope.classes.data+(SIZE_T)(75+i)*8;
        if(read_ptr(class_cell)!=row->klass.pointer||!guest_camera_stamp_matches(base,&row->klass,1)||
           (action&&!runtime_melee_instance(base,q,i,action))){row->changed=0;q->foreign=1;continue;}
        if(action&&!runtime_melee_instance_idle(action))return;
        if(runtime_movement_restore(class_cell,cell,0,row->klass.pointer,action,0)){row->changed=0;++restored;}
    }
    for(unsigned i=0;i<6;++i)if(q->rows[i].changed)return;
    /* Foreign replacements can still refer to our passive. Preserve it rather
     * than deleting a native subsystem's live dependency. */
    if(q->created&&!q->foreign&&runtime_melee_owns_passive(base,q,&pc,&pa,&powers)&&
       runtime_melee_instance_idle(q->passive.pointer)&&guest_player_context_method_matches(base,&q->remove,0)&&
       guest_equipment_dispatch_pins(base)&&read_ptr(read_ptr(q->manager.pointer)+0x230)==base+0x40d450){
        RuntimeMeleeRemoveParms parms={q->passive_class.pointer,0};
        ((ProcessEvent)(base+0x40d450))((void*)q->manager.pointer,(void*)q->remove.function.pointer,&parms,NULL);
    } else if(q->created&&!q->foreign&&runtime_melee_arrays(base,q,&pc,&pa,&powers)&&
              guest_camera_stamp_matches(base,&q->passive_class,1)&&!q->passive.pointer&&
              read_ptr(runtime_melee_cell(&pc,43))==q->passive_class.pointer&&!read_ptr(runtime_melee_cell(&pa,43))){
        /* AddPower may fail before returning an instance. Undo only its exact
         * otherwise-empty class cell; never resize either native array. */
        int conflict=0;
        for(int i=0;i<powers.count;++i)if(read_ptr(read_ptr(powers.data+(SIZE_T)i*8)+0x50)==q->passive_class.pointer)conflict=1;
        if(!conflict)runtime_power_action_context_pointer(runtime_melee_cell(&pc,43),q->passive_class.pointer,0);
    }
    if(q->anim_registered&&!q->foreign&&session_handle_matches(q->scope.pawn)&&
       guest_camera_stamp_matches(base,&q->anim_set,1)&&runtime_melee_temp_refcount(base,q)>0&&
       guest_player_context_method_matches(base,&q->unregister_anim,0)&&guest_equipment_dispatch_pins(base)&&
       read_ptr(read_ptr(q->scope.pawn.pointer)+0x230)==base+0x40d450){
        RuntimeMeleeAnimParms parms={q->anim_set.pointer,0.0f,0};
        ((ProcessEvent)(base+0x40d450))((void*)q->scope.pawn.pointer,(void*)q->unregister_anim.function.pointer,&parms,NULL);
    }
    runtime_melee_note("normal-melee-slots-retired",restored,now);memset(q,0,sizeof *q);(void)engine;
}
static void runtime_guest_melee_tick(uintptr_t base,uintptr_t engine,DWORD now) {
    if(GetCurrentThreadId()!=session_game_thread)return;RuntimeGuestMelee *q=&runtime_guest_melee;RuntimeMovementScope scope={0};
    int admitted=runtime_guest_melee_enabled()&&runtime_movement_scope(base,engine,&scope);
    if(q->valid){
        int retire=q->retiring||!admitted||!runtime_movement_scope_same(&q->scope,&scope);
        if(!retire&&(!q->monitor||now-q->monitor>=500)){
            q->monitor=now;GameArray pc={0},pa={0},powers={0};
            if(!runtime_melee_owns_passive(base,q,&pc,&pa,&powers))retire=1;
            for(unsigned i=0;i<6&&!retire;++i)if(q->rows[i].changed&&
                (read_ptr(q->scope.classes.data+(SIZE_T)(75+i)*8)!=q->rows[i].klass.pointer||
                 !guest_camera_stamp_matches(base,&q->rows[i].klass,1)))retire=1;
        }
        if(retire)runtime_guest_melee_retire(base,engine,now);
        return;
    }
    if(!admitted||scope.classes.count<=80||(q->checked&&now-q->checked<5000)||!runtime_movement_idle(scope.pawn.pointer))return;
    q->checked=now;RuntimeGuestMelee candidate={0};candidate.scope=scope;
    candidate.manager=session_capture_handle(read_ptr(scope.pawn.pointer+0xc18));GameArray pc={0},pa={0},powers={0};
    if(!runtime_melee_arrays(base,&candidate,&pc,&pa,&powers)||!supported_image(base)||!guest_equipment_dispatch_pins(base))return;
    for(unsigned i=0;i<6;++i)if(read_ptr(scope.classes.data+(SIZE_T)(75+i)*8)||
        read_ptr(runtime_melee_cell(&scope.actions,75+i)))return;
    if(read_ptr(runtime_melee_cell(&pc,43))||read_ptr(runtime_melee_cell(&pa,43)))return;
    for(int i=0;i<powers.count;++i)if(narrative_probe_object(base,read_ptr(powers.data+(SIZE_T)i*8),"SFXPowerCustomAction_MeleePassivePower"))return;
    if(!runtime_melee_source(base,&candidate))return;
    int need_anims=!runtime_melee_clips_loaded(base,&candidate);
    if(need_anims&&(!runtime_melee_find_anim_set(base,&candidate)||
       !runtime_melee_anim_method(base,scope.pawn.pointer,1,&candidate.register_anim)||
       !runtime_melee_anim_method(base,scope.pawn.pointer,0,&candidate.unregister_anim))){runtime_melee_note("waiting-loaded-melee-clips",0,now);return;}
    if(
       !runtime_melee_method(base,candidate.manager.pointer,"AddPower",&candidate.add,1)||
       !runtime_melee_method(base,candidate.manager.pointer,"RemovePower",&candidate.remove,0)||
       read_ptr(read_ptr(candidate.manager.pointer)+0x230)!=base+0x40d450)return;
    RuntimeMovementScope again={0};
    if(!runtime_movement_scope(base,engine,&again)||!runtime_movement_scope_same(&scope,&again)||
       !runtime_movement_idle(scope.pawn.pointer)||!runtime_melee_arrays(base,&candidate,&pc,&pa,&powers))return;
    RuntimeMeleeAddParms parms={candidate.passive_class.pointer,0};
    ((ProcessEvent)(base+0x40d450))((void*)candidate.manager.pointer,(void*)candidate.add.function.pointer,&parms,NULL);
    candidate.valid=1;candidate.created=1;
    if(!guest_camera_stamp_read(base,parms.result,&candidate.passive,1)||!runtime_melee_owns_passive(base,&candidate,&pc,&pa,&powers)){
        *q=candidate;runtime_guest_melee_retire(base,engine,now);return;
    }
    *q=candidate;
    if(!runtime_movement_scope(base,engine,&again)||!runtime_movement_scope_same(&scope,&again)||!runtime_movement_idle(scope.pawn.pointer)){
        runtime_guest_melee_retire(base,engine,now);return;
    }
    if(need_anims){
        if(!guest_camera_stamp_matches(base,&q->anim_set,1)||read_ptr(read_ptr(scope.pawn.pointer)+0x230)!=base+0x40d450){runtime_guest_melee_retire(base,engine,now);return;}
        uintptr_t set=q->anim_set.pointer;
        ((ProcessEvent)(base+0x40d450))((void*)scope.pawn.pointer,(void*)q->register_anim.function.pointer,&set,NULL);
        q->anim_registered=1;
        if(runtime_melee_temp_refcount(base,q)<=0||!runtime_melee_clips_loaded(base,q)||
           !runtime_movement_scope(base,engine,&again)||!runtime_movement_scope_same(&scope,&again)||!runtime_movement_idle(scope.pawn.pointer)){
            runtime_guest_melee_retire(base,engine,now);return;
        }
    }
    unsigned applied=0;
    for(unsigned i=0;i<6;++i){RuntimeMeleeRow *row=&q->rows[i];
        if(!runtime_melee_arrays(base,q,&pc,&pa,&powers)||!guest_camera_stamp_matches(base,&row->klass,1)||
           !guest_camera_stamp_matches(base,&row->cdo,1)||
           !runtime_movement_borrow(q->scope.classes.data+(SIZE_T)(75+i)*8,runtime_melee_cell(&q->scope.actions,75+i),0,row->klass.pointer,0)){
            runtime_guest_melee_retire(base,engine,now);return;
        }
        row->changed=1;++applied;
    }
    runtime_melee_note("native-normal-melee-ready",applied,now);
}
#ifdef ME3_RUNTIME_FIXTURE
static int runtime_guest_melee_selftest(void) {
    _Alignas(8) uintptr_t cell[2]={0,0};
    if(!runtime_movement_borrow((uintptr_t)&cell[0],(uintptr_t)&cell[1],0,5,0)||cell[0]!=5||cell[1])return 0;
    cell[1]=7;
    if(!runtime_movement_restore((uintptr_t)&cell[0],(uintptr_t)&cell[1],0,5,7,0)||cell[0]||cell[1])return 0;
    cell[0]=9;cell[1]=11;
    if(runtime_movement_borrow((uintptr_t)&cell[0],(uintptr_t)&cell[1],0,5,0)||cell[0]!=9||cell[1]!=11||
       runtime_movement_restore((uintptr_t)&cell[0],(uintptr_t)&cell[1],0,5,7,0)||cell[0]!=9||cell[1]!=11)return 0;
    /* PowerCustomActionID is484. The old read-only capture accidentally read
     * Description4a8; accepting that field could make a valid grant impossible. */
    _Alignas(8) unsigned char object[0x500]={0};int32_t id=43;
    memcpy(object+0x4a8,&id,4);if(runtime_melee_power_id((uintptr_t)object))return 0;
    memcpy(object+0x484,&id,4);return runtime_melee_power_id((uintptr_t)object)&&
        sizeof(RuntimeMeleeAnimParms)==16&&offsetof(RuntimeMeleeRemoveParms,result)==8;
}
#endif
#endif
