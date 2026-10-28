#ifndef ME3_RUNTIME_GUEST_MOVEMENT_H
#define ME3_RUNTIME_GUEST_MOVEMENT_H
/* Human dodge preparation only. Ashley's authored slots54..57 select HenchRoll
 * SingleAnim actions. The SAME loaded package supplies PlayerEvade actions
 * using the same four CB_Evade clips, with native human prediction, direction,
 * velocity root motion and roll camera. Borrow slots, never replay an action,
 * replace an animation tree or write positions/input/movement flags. */
typedef struct RuntimeMovementScope {
    SessionHandle pc,pawn,info,world,driver,connection,channel,ai;
    GameArray classes,actions;
    unsigned long token;
} RuntimeMovementScope;
typedef struct RuntimeMovementRow {
    GuestCameraStamp old_class,new_class,old_cdo,new_cdo,old_instance,new_instance;
    SessionHandle old_ai;
    int changed,observed;
} RuntimeMovementRow;
typedef struct RuntimeGuestMovement {
    RuntimeMovementScope scope;
    RuntimeMovementRow rows[4];
    int valid,retiring;
    DWORD checked;
} RuntimeGuestMovement;
static RuntimeGuestMovement runtime_guest_movement;
static unsigned runtime_guest_movement_events;
static unsigned runtime_movement_wait_mask;
static unsigned long runtime_movement_wait_token;
static const char *runtime_movement_hench_names[]={"SFXCustomAction_HenchRollLeft","SFXCustomAction_HenchRollRight",
    "SFXCustomAction_HenchRollForward","SFXCustomAction_HenchRollBackward"};
static const char *runtime_movement_player_names[]={"SFXCustomAction_PlayerEvadeLeft","SFXCustomAction_PlayerEvadeRight",
    "SFXCustomAction_PlayerEvadeForward","SFXCustomAction_PlayerEvadeBackwards"};
static const char *runtime_movement_anim_names[]={"CB_EvadeLeft","CB_EvadeRight","CB_EvadeForward","CB_EvadeBack"};
static int runtime_guest_movement_enabled(void) {
    static int enabled=-1;
    if(enabled<0){wchar_t text[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_PLAYER_DODGE",text,8);
        enabled=!n||(n==1&&text[0]==L'1');}return enabled;
}
static void runtime_movement_note(const char *event,unsigned slots,DWORD now) {
    if(runtime_guest_movement_events++>=64)return;char row[300];
    snprintf(row,sizeof row,"{\"runtime_guest_movement\":\"%s\",\"role\":%d,\"dodge_slots\":%u,\"action_dispatched\":false,\"animation_tree_written\":false,\"full_movement_parity_verified\":false,\"monotonic_ms\":%lu}\r\n",
        event,(int)g_runtime_config.role,slots,now);line(row);
}
static int runtime_movement_same_handle(SessionHandle a,SessionHandle b) {
    return a.pointer==b.pointer&&a.index==b.index&&a.name==b.name;
}
static int runtime_movement_scope_same(const RuntimeMovementScope *a,const RuntimeMovementScope *b) {
    return a->token&&a->token==b->token&&runtime_movement_same_handle(a->pc,b->pc)&&
        runtime_movement_same_handle(a->pawn,b->pawn)&&runtime_movement_same_handle(a->info,b->info)&&
        runtime_movement_same_handle(a->world,b->world)&&runtime_movement_same_handle(a->driver,b->driver)&&
        runtime_movement_same_handle(a->connection,b->connection)&&runtime_movement_same_handle(a->channel,b->channel);
}
/* VerifyCA grows CustomActions lazily. Array storage is mutable, whereas the
 * captured pawn/world and each borrowed slot determine lease ownership. */
static int runtime_movement_arrays_live(uintptr_t base,RuntimeMovementScope *s) {
    GameArray classes={0},actions={0};
    int valid=session_handle_matches(s->pawn)&&session_handle_matches(s->info)&&session_handle_matches(s->world)&&
        guest_camera_actor_live(base,s->pawn.pointer)&&guest_camera_actor_live(base,s->info.pointer)&&
        read_ptr(s->pawn.pointer+0x198)==s->info.pointer&&read_ptr(base+0x18b6638)==s->world.pointer&&
        identity_array(s->pawn.pointer+0x69c,&classes,512)&&classes.count>57&&
        scene_readiness_array(s->pawn.pointer+0x6ac,&actions,512);
    if(valid){s->classes=classes;s->actions=actions;}return valid;
}
static uintptr_t runtime_movement_instance_cell(const GameArray *actions,unsigned index) {
    return actions->data&&index<(unsigned)actions->count?actions->data+(SIZE_T)index*8:0;
}
static int runtime_movement_idle(uintptr_t pawn) {
    int32_t action=-1,power=-1;
    return read_at(pawn+0xc20,&action,4)&&!action&&read_at(pawn+0xc28,&power,4)&&!power;
}
static int runtime_movement_action_idle_policy(uint32_t flags,uintptr_t timeline,int camera_in_use) {
    return !(flags&0x10800000)&&!timeline&&camera_in_use==0;
}
/* PlayerEvade keeps oPlayerCamera as a reusable Roll mode after it ends,
 * exactly as Shepard does. Its non-null cache is not an active action. Wait
 * only while that mode is actually driving the current camera/interpolation. */
static int runtime_movement_action_idle(uintptr_t base,uintptr_t action,uintptr_t pc) {
    uint32_t flags=0;uintptr_t roll=read_ptr(action+0xb0),timeline=read_ptr(action+0xb8);
    if(!read_at(action+0x100,&flags,4)||!runtime_movement_action_idle_policy(flags,timeline,0))return 0;
    if(!roll)return 1;
    if(!narrative_probe_object(base,roll,"SFXCameraMode_Roll")||
       !guest_camera_actor_live(base,pc)||!is_kind_of(base,pc,"SFXPlayerController"))return 0;
    uintptr_t camera=read_ptr(pc+0x684),mode=read_ptr(camera+0x558);
    if(!guest_camera_actor_live(base,camera)||!is_kind_of(base,camera,"SFXPlayerCamera")||
       read_ptr(pc+0xe28)!=camera||read_ptr(camera+0x4a8)!=pc||
       !narrative_probe_object(base,mode,"SFXCameraMode"))return 0;
    int used=mode==roll;
    if(!used&&is_kind_of(base,mode,"SFXCameraMode_Interpolate"))
        used=read_ptr(mode+0x158)==roll||read_ptr(mode+0x160)==roll;
    return runtime_movement_action_idle_policy(flags,timeline,used);
}
static int runtime_movement_scope(uintptr_t base,uintptr_t engine,RuntimeMovementScope *out) {
    SessionContext c=session_context(engine);NetworkActorScope world={0};NetworkActorIdentityEntry channel={0};
    memset(out,0,sizeof *out);uint32_t life=0;
    if(!runtime_guest_movement_enabled()||!runtime_control_enabled||!runtime_scope_ready||
       !InterlockedCompareExchange(&g_enable_requested,0,0)||session_engine_blocked(c))return 0;
    if(g_runtime_config.role==ME3_ROLE_HOST) {
        if(!session_active||session_fault||session_releasing||session_cleanup_pending||session_pending_reason||
           !session_story_valid(base)||c.world!=session_world||!session_handle_matches(session_pc_handle)||
           !session_handle_matches(session_pawn_handle)||!session_handle_matches(session_ai_handle)||
           !session_peer_connected(base,session_remote,c.world))return 0;
        c.pc=session_remote;c.pawn=session_pawn;out->token=session_epoch;out->ai=session_ai_handle;
    } else if(g_runtime_config.role==ME3_ROLE_GUEST) {
        if(!guest_player_context_registered_for(c.pc,c.pawn)||
           !guest_player_context_basic_owned(base,c.pc,c.pawn,c.world))return 0;
        out->token=guest_player_context_registration_token();
    } else return 0;
    if(!out->token||!guest_camera_actor_live(base,c.pc)||!is_kind_of(base,c.pc,"SFXPlayerController")||
       !guest_camera_actor_live(base,c.pawn)||!is_kind_of(base,c.pawn,"SFXPawn_Ashley")||
       read_ptr(c.pc+0x3bc)!=c.pawn||read_ptr(c.pawn+0x374)!=c.pc||read_ptr(c.pc+0x198)!=c.world||
       !read_at(c.pawn+0xe20,&life,4)||(life&0x300)||
       !network_actor_read_scope(base,c.pawn,&world)||world.info!=c.world||
       !guest_player_context_channel(base,c.pawn,&channel)||channel.index<=0||channel.driver!=world.driver||
       (g_runtime_config.role==ME3_ROLE_HOST&&channel.connection!=read_ptr(c.pc+0x67c))||
       !runtime_power_property(base,c.pawn,"BioPawn","CustomActionClasses","ArrayProperty",0x69c,16)||
       !runtime_power_property(base,c.pawn,"BioPawn","CustomActions","ArrayProperty",0x6ac,16)||
       !identity_array(c.pawn+0x69c,&out->classes,512)||out->classes.count<=57||
       !scene_readiness_array(c.pawn+0x6ac,&out->actions,512))return 0;
    out->pc=session_capture_handle(c.pc);out->pawn=session_capture_handle(c.pawn);out->info=session_capture_handle(c.world);
    out->world=session_capture_handle(world.world);out->driver=session_capture_handle(world.driver);
    out->connection=session_capture_handle(channel.connection);out->channel=session_capture_handle(channel.channel);
    return session_handle_matches(out->pc)&&session_handle_matches(out->pawn)&&session_handle_matches(out->info)&&
        session_handle_matches(out->world)&&session_handle_matches(out->driver)&&
        session_handle_matches(out->connection)&&session_handle_matches(out->channel);
}
static int runtime_movement_named_class(uintptr_t base,uintptr_t object,const char *name,GuestCameraStamp *out) {
    char path[160];snprintf(path,sizeof path,"SFXGameContent.%s",name);
    return narrative_probe_object(base,object,"Class")&&guest_camera_stamp_read(base,object,out,1)&&
        !strcmp(out->full_name,path);
}
static int runtime_movement_body_stance(uintptr_t base,uintptr_t klass,unsigned index,GuestCameraStamp *cdo) {
    uintptr_t object=read_ptr(klass+0x13c);GameArray names={0},again={0};uint32_t name[2]={0};char text[96];
    if(index>=4||!guest_camera_stamp_read(base,object,cdo,1)||read_ptr(object+0x50)!=klass||
       !is_kind_of(base,object,"SFXCustomAction_SingleAnim")||
       !runtime_power_property(base,object,"SFXCustomAction_SingleAnim","BS_Anim","StructProperty",0x110,16)||
       !identity_array(object+0x110,&names,8)||names.count!=1||!read_at(names.data,name,8)||!name[0]||name[1])return 0;
    name_of(base,name[0],text,sizeof text);
    return !strcmp(text,runtime_movement_anim_names[index])&&
        read_at(object+0x110,&again,sizeof again)&&!memcmp(&names,&again,sizeof names);
}
/* Both kinds use the SAME BodyStance clip, but require it to be present on
 * the actual current mesh. No clip, set or cached animation node is added. */
static int runtime_movement_loaded_anims(uintptr_t base,uintptr_t pawn) {
    uintptr_t mesh=read_ptr(pawn+0x3f4);GameArray sets={0},again={0};unsigned found=0,total=0;
    if(!narrative_probe_object(base,mesh,"SkeletalMeshComponent")||
       !runtime_power_property(base,pawn,"Pawn","Mesh","ObjectProperty",0x3f4,8)||
       !runtime_power_property(base,mesh,"SkeletalMeshComponent","AnimSets","ArrayProperty",0x320,16)||
       !identity_array(mesh+0x320,&sets,64))return 0;
    uintptr_t verified_sequence_class=0;
    for(int i=0;i<sets.count;++i) {
        uintptr_t set=read_ptr(sets.data+(SIZE_T)i*8);GameArray sequences={0},checked={0};
        if(!set)continue;
        if(!narrative_probe_object(base,set,"AnimSet")||
           !runtime_power_property(base,set,"AnimSet","Sequences","ArrayProperty",0x60,16)||
           !scene_readiness_array(set+0x60,&sequences,4096))return 0;
        if((total+=(unsigned)sequences.count)>8192)return 0;
        for(int n=0;n<sequences.count;++n) {
            uintptr_t sequence=read_ptr(sequences.data+(SIZE_T)n*8);uint32_t name[2]={0};char text[96];
            if(!sequence)continue;
            if(!narrative_probe_object(base,sequence,"AnimSequence"))return 0;
            uintptr_t klass=read_ptr(sequence+0x50);
            if(klass!=verified_sequence_class) {
                if(!runtime_power_property(base,sequence,"AnimSequence","SequenceName","NameProperty",0xf4,8))return 0;
                verified_sequence_class=klass;
            }
            if(!read_at(sequence+0xf4,name,8))return 0;
            if(!name[0]||name[1])continue;
            name_of(base,name[0],text,sizeof text);
            for(unsigned j=0;j<4;++j)if(!strcmp(text,runtime_movement_anim_names[j]))found|=1u<<j;
        }
        if(!read_at(set+0x60,&checked,sizeof checked)||memcmp(&sequences,&checked,sizeof checked))return 0;
    }
    return found==15&&read_ptr(pawn+0x3f4)==mesh&&read_at(mesh+0x320,&again,sizeof again)&&!memcmp(&sets,&again,sizeof sets);
}
static int runtime_movement_find_player_classes(uintptr_t base,RuntimeGuestMovement *q) {
    static GuestCameraStamp cached_outer,cached_classes[4];static SessionHandle cached_world;
    static int cached;
    uintptr_t outer=read_ptr(q->rows[0].old_class.pointer+0x40);unsigned found[4]={0};GameArray objects={0};
    for(unsigned i=1;i<4;++i)if(read_ptr(q->rows[i].old_class.pointer+0x40)!=outer)return 0;
    if(cached&&cached_outer.pointer==outer&&runtime_movement_same_handle(cached_world,q->scope.world)&&
       guest_camera_stamp_matches(base,&cached_outer,1)) {
        int valid=1;
        for(unsigned i=0;i<4;++i) {
            q->rows[i].new_class=cached_classes[i];
            if(!guest_camera_stamp_matches(base,&cached_classes[i],1)||
               !runtime_movement_body_stance(base,cached_classes[i].pointer,i,&q->rows[i].new_cdo)||
               !is_kind_of(base,q->rows[i].new_cdo.pointer,"SFXCustomAction_PlayerEvadeBase"))valid=0;
        }
        if(valid)return 1;
    }
    cached=0;
    uintptr_t *snapshot=identity_object_list(base,&objects);if(!snapshot)return 0;
    int valid=1;
    for(int i=0;i<objects.count&&valid;++i) {
        uintptr_t object=snapshot[i];if(read_ptr(object+0x40)!=outer)continue;
        char name[96];object_name(base,object,name,sizeof name);
        for(unsigned j=0;j<4;++j)if(!strcmp(name,runtime_movement_player_names[j])) {
            if(found[j]++||!runtime_movement_named_class(base,object,name,&q->rows[j].new_class)||
               !runtime_movement_body_stance(base,object,j,&q->rows[j].new_cdo)||
               !is_kind_of(base,q->rows[j].new_cdo.pointer,"SFXCustomAction_PlayerEvadeBase"))valid=0;
        }
    }
    HeapFree(GetProcessHeap(),0,snapshot);
    valid=valid&&found[0]==1&&found[1]==1&&found[2]==1&&found[3]==1;
    if(valid&&guest_camera_stamp_read(base,outer,&cached_outer,1)) {
        cached_world=q->scope.world;
        for(unsigned i=0;i<4;++i)cached_classes[i]=q->rows[i].new_class;
        cached=1;
    }
    return valid;
}
static int runtime_movement_instance(uintptr_t base,uintptr_t action,uintptr_t klass,uintptr_t pawn) {
    return narrative_probe_object(base,action,"BioCustomAction")&&read_ptr(action+0x50)==klass&&
        runtime_power_action_context_layout(base,action)&&read_ptr(action+0x98)==pawn;
}
/* No native call occurs between the two CAS steps. If a replacement wins,
 * rollback only the class pointer we installed, never its instance. */
static int runtime_movement_borrow(uintptr_t class_at,uintptr_t instance_at,uintptr_t old_class,
        uintptr_t new_class,uintptr_t old_instance) {
    if(!runtime_power_action_context_pointer(class_at,old_class,new_class))return 0;
    if(!instance_at&&!old_instance)return 1; /* Stock first input grows the lazy array. */
    if(runtime_power_action_context_pointer(instance_at,old_instance,0))return 1;
    runtime_power_action_context_pointer(class_at,new_class,old_class);return 0;
}
static int runtime_movement_restore(uintptr_t class_at,uintptr_t instance_at,uintptr_t old_class,
        uintptr_t new_class,uintptr_t owned_instance,uintptr_t old_instance) {
    if(read_ptr(class_at)!=new_class||(!instance_at?owned_instance!=0:read_ptr(instance_at)!=owned_instance)||
       !runtime_power_action_context_pointer(class_at,new_class,old_class))return 0;
    if(!instance_at)return 1;
    if(runtime_power_action_context_pointer(instance_at,owned_instance,old_instance))return 1;
    runtime_power_action_context_pointer(class_at,old_class,new_class);return 0;
}
static int runtime_movement_owned_instances(uintptr_t base,RuntimeGuestMovement *q,int idle) {
    unsigned remaining=0;int busy=0;
    for(unsigned i=0;i<4;++i) {RuntimeMovementRow *r=&q->rows[i];if(!r->changed)continue;
        uintptr_t at=runtime_movement_instance_cell(&q->scope.actions,54+i),action=at?read_ptr(at):0;
        if(!guest_camera_stamp_matches(base,&r->old_class,1)||!guest_camera_stamp_matches(base,&r->new_class,1)||
           read_ptr(q->scope.classes.data+(SIZE_T)(54+i)*8)!=r->new_class.pointer) {r->changed=0;continue;}
        if(!action) {r->observed=0;memset(&r->new_instance,0,sizeof r->new_instance);++remaining;continue;}
        if(r->observed) {
            if(action!=r->new_instance.pointer||!guest_camera_stamp_matches(base,&r->new_instance,1)) {r->changed=0;continue;}
        } else {
            if(!runtime_movement_instance(base,action,r->new_class.pointer,q->scope.pawn.pointer)||
               read_ptr(action+0xa0)!=q->scope.pc.pointer||read_ptr(action+0xa8)||
               !guest_camera_stamp_read(base,action,&r->new_instance,1)) {r->changed=0;continue;}
            r->observed=1;
        }
        uintptr_t view_pc=g_runtime_config.role==ME3_ROLE_GUEST?session_context(session_engine).pc:q->scope.pc.pointer;
        ++remaining;if(idle&&!runtime_movement_action_idle(base,action,view_pc))busy=1;
    }
    return busy?-1:(int)remaining;
}
static int runtime_movement_retire_owner_policy(uintptr_t owner,uintptr_t pc,int pc_live,int native_ai,int reciprocal,int same_world) {
    return !owner||(owner==pc&&pc_live)||(native_ai&&reciprocal&&same_world);
}
/* Before host UnPossess/AI handoff, and also every Tick while retirement is
 * pending. Busy native actions may finish; story release is never delayed. */
static void runtime_guest_movement_retire(uintptr_t base,uintptr_t engine,DWORD now) {
    RuntimeGuestMovement *q=&runtime_guest_movement;if(!q->valid||GetCurrentThreadId()!=session_game_thread)return;
    q->retiring=1;
    if(!runtime_movement_arrays_live(base,&q->scope)) {
        runtime_movement_note("retired-preserving-replaced-scope",0,now);memset(q,0,sizeof *q);return;
    }
    uintptr_t owner=read_ptr(q->scope.pawn.pointer+0x374);
    int allowed=runtime_movement_retire_owner_policy(owner,q->scope.pc.pointer,session_handle_matches(q->scope.pc),
        guest_camera_actor_live(base,owner)&&is_kind_of(base,owner,"SFXAI_Henchman"),
        read_ptr(owner+0x3bc)==q->scope.pawn.pointer,read_ptr(owner+0x198)==q->scope.info.pointer);
    if(!allowed) {runtime_movement_note("retired-preserving-new-owner",0,now);memset(q,0,sizeof *q);return;}
    if(!runtime_movement_idle(q->scope.pawn.pointer))return;
    int instances=runtime_movement_owned_instances(base,q,1);if(instances<0)return;
    if(!instances) {runtime_movement_note("retired-preserving-replaced-slot",0,now);memset(q,0,sizeof *q);return;}
    unsigned restored=0;
    for(unsigned i=0;i<4;++i) {RuntimeMovementRow *r=&q->rows[i];if(!r->changed)continue;
        uintptr_t old=0;
        if(r->old_instance.pointer&&guest_camera_stamp_matches(base,&r->old_instance,1)&&
           runtime_movement_instance(base,r->old_instance.pointer,r->old_class.pointer,q->scope.pawn.pointer)&&
           !read_ptr(r->old_instance.pointer+0xa0)&&
           read_ptr(r->old_instance.pointer+0xa8)==r->old_ai.pointer&&
           (!r->old_ai.pointer||(session_handle_matches(r->old_ai)&&(!owner||owner==r->old_ai.pointer))))old=r->old_instance.pointer;
        /* A disposed original instance becomes None. The next normal AI
         * action reconstructs it under its actual controller through VerifyCA. */
        uintptr_t current=r->observed?r->new_instance.pointer:0;
        if(runtime_movement_restore(q->scope.classes.data+(SIZE_T)(54+i)*8,runtime_movement_instance_cell(&q->scope.actions,54+i),
           r->old_class.pointer,r->new_class.pointer,current,old)){r->changed=0;++restored;}
    }
    for(unsigned i=0;i<4;++i)if(q->rows[i].changed)return;
    runtime_movement_note("native-hench-slots-restored",restored,now);memset(q,0,sizeof *q);
    (void)engine;
}
static void runtime_guest_movement_tick(uintptr_t base,uintptr_t engine,DWORD now) {
    if(GetCurrentThreadId()!=session_game_thread)return;
    RuntimeGuestMovement *q=&runtime_guest_movement;RuntimeMovementScope scope={0};
    int admitted=runtime_movement_scope(base,engine,&scope);
    if(q->valid) {
        if(q->retiring||!admitted||!runtime_movement_scope_same(&q->scope,&scope)) {
            runtime_guest_movement_retire(base,engine,now);return;
        }
        if(!runtime_movement_arrays_live(base,&q->scope)||runtime_movement_owned_instances(base,q,0)==0) {
            runtime_movement_note("preserved-native-slot-replacement",0,now);memset(q,0,sizeof *q);
        }
        return;
    }
    if(!admitted||(q->checked&&now-q->checked<5000)||!runtime_movement_idle(scope.pawn.pointer))return;
    q->checked=now;RuntimeGuestMovement candidate={0};candidate.scope=scope;
    for(unsigned i=0;i<4;++i) {RuntimeMovementRow *r=&candidate.rows[i];
        uintptr_t instance_at=runtime_movement_instance_cell(&scope.actions,54+i);
        uintptr_t klass=read_ptr(scope.classes.data+(SIZE_T)(54+i)*8),action=instance_at?read_ptr(instance_at):0;
        if(!runtime_movement_named_class(base,klass,runtime_movement_hench_names[i],&r->old_class)||
           !runtime_movement_body_stance(base,klass,i,&r->old_cdo))return;
        if(action) {
            uintptr_t ai=read_ptr(action+0xa8);
            uintptr_t cached_pc=read_ptr(action+0xa0);
            if(!runtime_movement_instance(base,action,klass,scope.pawn.pointer)||(cached_pc&&cached_pc!=scope.pc.pointer)||
               (cached_pc&&ai)||
               !runtime_movement_action_idle(base,action,scope.pc.pointer)||
               !runtime_power_action_context_ai(base,ai,scope.pawn.pointer,scope.info.pointer)||
               (ai&&read_ptr(ai+0x3bc))||
               (g_runtime_config.role==ME3_ROLE_HOST&&ai&&ai!=scope.ai.pointer)||
               !guest_camera_stamp_read(base,action,&r->old_instance,1))return;
            if(ai){r->old_ai=session_capture_handle(ai);if(!session_handle_matches(r->old_ai))return;}
        }
    }
    if(runtime_movement_wait_token!=scope.token){runtime_movement_wait_token=scope.token;runtime_movement_wait_mask=0;}
    if(!runtime_movement_find_player_classes(base,&candidate)) {
        if(!(runtime_movement_wait_mask&1)){runtime_movement_wait_mask|=1;runtime_movement_note("waiting-loaded-player-dodge-classes",0,now);}return;
    }
    if(!runtime_movement_loaded_anims(base,scope.pawn.pointer)) {
        if(!(runtime_movement_wait_mask&2)){runtime_movement_wait_mask|=2;runtime_movement_note("waiting-current-mesh-evade-clips",0,now);}return;
    }
    RuntimeMovementScope checked;
    if(!runtime_movement_scope(base,engine,&checked)||!runtime_movement_scope_same(&scope,&checked)||
       !runtime_movement_idle(scope.pawn.pointer))return;
    candidate.valid=1;*q=candidate;unsigned applied=0;
    for(unsigned i=0;i<4;++i) {RuntimeMovementRow *r=&q->rows[i];
        if(!runtime_movement_arrays_live(base,&q->scope)||!guest_camera_stamp_matches(base,&r->old_class,1)||
           !guest_camera_stamp_matches(base,&r->new_class,1)||
           !runtime_movement_borrow(q->scope.classes.data+(SIZE_T)(54+i)*8,runtime_movement_instance_cell(&q->scope.actions,54+i),
            r->old_class.pointer,r->new_class.pointer,r->old_instance.pointer)) {
            runtime_guest_movement_retire(base,engine,now);return;
        }
        r->changed=1;++applied;
    }
    runtime_movement_note("native-player-dodge-slots-borrowed",applied,now);
}
static int runtime_guest_movement_selftest(void) {
    _Alignas(8) uintptr_t fields[2]={0x10,0x30};
    if(!runtime_movement_borrow((uintptr_t)&fields[0],(uintptr_t)&fields[1],0x10,0x20,0x30)||fields[0]!=0x20||fields[1])return 0;
    fields[1]=0x40; /* A stock-created player instance. */
    if(!runtime_movement_restore((uintptr_t)&fields[0],(uintptr_t)&fields[1],0x10,0x20,0x40,0x30)||fields[0]!=0x10||fields[1]!=0x30)return 0;
    fields[1]=0x50; /* Lost second CAS preserves a foreign instance. */
    if(runtime_movement_borrow((uintptr_t)&fields[0],(uintptr_t)&fields[1],0x10,0x20,0x30)||fields[0]!=0x10||fields[1]!=0x50)return 0;
    fields[0]=0x60;
    if(runtime_movement_restore((uintptr_t)&fields[0],(uintptr_t)&fields[1],0x10,0x20,0x40,0x30)||fields[0]!=0x60||fields[1]!=0x50)return 0;
    fields[0]=0x20;fields[1]=0x40;
    if(!runtime_movement_restore((uintptr_t)&fields[0],(uintptr_t)&fields[1],0x10,0x20,0x40,0)||fields[0]!=0x10||fields[1])return 0;
    fields[0]=0x10;fields[1]=0x50;
    if(!runtime_movement_borrow((uintptr_t)&fields[0],0,0x10,0x20,0)||fields[0]!=0x20||fields[1]!=0x50||
       !runtime_movement_restore((uintptr_t)&fields[0],0,0x10,0x20,0,0)||fields[0]!=0x10||fields[1]!=0x50)return 0;
    GameArray empty={0},grown={0x1000,58,64};
    if(runtime_movement_instance_cell(&empty,54)||runtime_movement_instance_cell(&grown,58)||
       runtime_movement_instance_cell(&grown,57)!=0x1000+57*8)return 0;
    if(!runtime_movement_retire_owner_policy(0,16,0,0,0,0)||
       !runtime_movement_retire_owner_policy(16,16,1,0,0,0)||
       runtime_movement_retire_owner_policy(16,16,0,0,0,0)||
       runtime_movement_retire_owner_policy(24,16,1,0,1,1)||
       !runtime_movement_retire_owner_policy(24,16,0,1,1,1)||
       runtime_movement_retire_owner_policy(24,16,0,1,0,1)||
       runtime_movement_retire_owner_policy(24,16,0,1,1,0))return 0;
    unsigned char pawn[0xc30]={0};int32_t busy=54;
    if(!runtime_movement_idle((uintptr_t)pawn))return 0;
    memcpy(pawn+0xc20,&busy,4);if(runtime_movement_idle((uintptr_t)pawn))return 0;
    if(!runtime_movement_action_idle_policy(0xc1457206,0,0)||
       runtime_movement_action_idle_policy(0xd1457206,0,0)||
       runtime_movement_action_idle_policy(0xc1c57206,0,0)||
       runtime_movement_action_idle_policy(0xc1457206,16,0)||
       runtime_movement_action_idle_policy(0xc1457206,0,1)||
       runtime_movement_action_idle_policy(0xc1457206,0,-1))return 0;
    RuntimeMovementScope a={0},b={0};a.token=1;
    SessionHandle *handles[]={&a.pc,&a.pawn,&a.info,&a.world,&a.driver,&a.connection,&a.channel};
    for(unsigned i=0;i<7;++i)*handles[i]=(SessionHandle){0x100+i*8,(int32_t)i+1,i+2};
    a.classes=(GameArray){0x1000,64,64};a.actions=(GameArray){0x2000,64,64};b=a;
    if(!runtime_movement_scope_same(&a,&b))return 0;
    for(unsigned i=0;i<7;++i){++handles[i]->index;if(runtime_movement_scope_same(&a,&b))return 0;--handles[i]->index;}
    ++a.actions.data;++a.classes.data;if(!runtime_movement_scope_same(&a,&b))return 0;a=b;++a.token;
    return !runtime_movement_scope_same(&a,&b);
}
#endif
