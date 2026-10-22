#ifndef ME3_GUEST_CAMERA_ADAPTER_H
#define ME3_GUEST_CAMERA_ADAPTER_H

/* Opt-in, guest-local camera candidate. ordinary_mode is a boolean supplied
 * by the caller's existing authoritative scene gate, not an enum value.
 * Include after package_identity.h/network_actor_identity.h. Call on the game
 * thread before/after ordinary Tick, including disabled/empty contexts so a
 * captured baseline can be restored. This never reasserts an applied scale.
 *
 * SDK: BioPawn.CameraArmScale+e14 is CPF_Transient, not marked CPF_Net;
 * Engine.Player.Actor68, PC.Player67c/Pawn3bc/AcknowledgedPawn694;
 * SFXPC.MyCamerae28; Camera.PCOwner4a8/ViewTarget.Target3f4;
 * SFXCameraNativeBase.CurrentCameraMode558; SFXCameraMode.Target7c/flags11c.
 * Private SDK Engine_classes:8260 AcknowledgedPawn694,10461 Player.Actor68;
 * SFXGame_classes:8023+ SFXGameModeBase flags+d0: active2,
 * allowMovement400,stopMovement800,allowCamera1000; manager GameModes70,
 * disableInput+a4 mask1 and CurrentMode+a8 (8199+). These are reflected
 * permission fields, not guessed native flags or calls to change modes.
 * Exact-build owned pair27632/30532 showed identical authored Combat camera
 * modes with host arm scale1 and NPC scale0. Rotated(Hook-Scale*Offset)
 * predicts both actual POVs within0.002units. This adapts presentation only.
 * No coordinate, actor flag, host, class-default, plot or singleton writes.
 */
typedef struct GuestCameraStamp {
    uintptr_t pointer, klass;
    int32_t index, class_index;
    uint32_t name[2], class_name[2];
    char full_name[600];
} GuestCameraStamp;
typedef struct GuestCameraLease {
    int tracked, applied, suppressed;
    GuestCameraStamp pc,pawn,world,camera,mode;
    uint32_t baseline_bits;
    DWORD retry_at;
} GuestCameraLease;
static GuestCameraLease guest_camera_lease;
static DWORD guest_camera_last_failure;
static DWORD guest_camera_last_eligibility;
static const char *guest_camera_eligibility_reason="not-evaluated";
static int guest_camera_reject(const char *reason) {guest_camera_eligibility_reason=reason;return 0;}

static uint32_t guest_camera_float_bits(float value) {
    uint32_t bits;memcpy(&bits,&value,4);return bits;
}
static int guest_camera_stamp_core_equal(const GuestCameraStamp *a,const GuestCameraStamp *b) {
    return a->pointer&&a->pointer==b->pointer&&a->klass==b->klass&&a->index==b->index&&a->class_index==b->class_index&&
        !memcmp(a->name,b->name,8)&&!memcmp(a->class_name,b->class_name,8);
}
static int guest_camera_stamp_read(uintptr_t base,uintptr_t object,GuestCameraStamp *out,int full) {
    memset(out,0,sizeof *out);
    if(!is_live(base,object)||!read_at(object+8,&out->index,4)||!read_at(object+0x48,out->name,8)||
       !read_at(object+0x50,&out->klass,8)||!is_live(base,out->klass)||
       !read_at(out->klass+8,&out->class_index,4)||!read_at(out->klass+0x48,out->class_name,8)) return 0;
    out->pointer=object;
    if(full) {
        /* Validate the whole outer chain before recording its full path. */
        uintptr_t cursor=object,seen[40];int depth=0;
        while(cursor&&depth<40) {
            if(!is_live(base,cursor)) return 0;
            for(int i=0;i<depth;++i) if(seen[i]==cursor) return 0;
            seen[depth++]=cursor;
            if(!read_at(cursor+0x40,&cursor,8)) return 0;
        }
        if(cursor) return 0;
        identity_path(base,object,out->full_name,sizeof out->full_name);
        if(!out->full_name[0]||!strcmp(out->full_name,"<unreadable>")||strlen(out->full_name)>=sizeof out->full_name-1) return 0;
    }
    return 1;
}
static int guest_camera_stamp_matches(uintptr_t base,const GuestCameraStamp *saved,int full) {
    GuestCameraStamp current;
    return guest_camera_stamp_read(base,saved->pointer,&current,full)&&guest_camera_stamp_core_equal(saved,&current)&&
        (!full||!strcmp(saved->full_name,current.full_name));
}
static int guest_camera_actor_live(uintptr_t base,uintptr_t actor) {
    uint32_t a=0,b=0;
    return is_live(base,actor)&&is_kind_of(base,actor,"Actor")&&read_at(actor+0x25c,&a,4)&&read_at(actor+0x260,&b,4)&&!(a&8)&&!(b&0x800000);
}
static int guest_camera_zero_baseline(uint32_t bits) {
    float value;memcpy(&value,&bits,4);return isfinite(value)&&value==0.0f;
}
static int guest_camera_owned_value(uint32_t bits) { return bits==guest_camera_float_bits(1.0f); }
static void guest_camera_log(const char *event,const char *reason,DWORD now,int verified,uint32_t current_bits,int current_valid) {
    char row[1700],name[1200];float baseline=0,current=0;
    memcpy(&baseline,&guest_camera_lease.baseline_bits,4);memcpy(&current,&current_bits,4);
    identity_quote(guest_camera_lease.pawn.full_name,name,sizeof name);
    snprintf(row,sizeof row,"{\"guest_camera_adapter\":\"%s\",\"reason\":\"%s\",\"verified\":%s,\"pawn_path\":\"%s\",\"baseline_bits\":%u,\"baseline\":%.6f,\"current_read_valid\":%s,\"current_bits\":%u,\"current_finite\":%s,\"current\":%.6f,\"monotonic_ms\":%lu}\r\n",
        event,reason,verified?"true":"false",name,guest_camera_lease.baseline_bits,isfinite(baseline)?baseline:0,current_valid?"true":"false",current_bits,current_valid&&isfinite(current)?"true":"false",current_valid&&isfinite(current)?current:0,now);line(row);
}
static void guest_camera_read_failure(const char *reason,DWORD now) {
    if(!guest_camera_last_failure||now-guest_camera_last_failure>=5000) {
        guest_camera_last_failure=now;guest_camera_log("read-failure",reason,now,0,0,0);
    }
}
static int guest_camera_captured_pawn_valid(uintptr_t base,int full) {
    return guest_camera_stamp_matches(base,&guest_camera_lease.pawn,full)&&
        guest_camera_stamp_matches(base,&guest_camera_lease.world,full)&&
        guest_camera_actor_live(base,guest_camera_lease.pawn.pointer)&&
        guest_camera_actor_live(base,guest_camera_lease.world.pointer)&&
        read_ptr(guest_camera_lease.pawn.pointer+0x198)==guest_camera_lease.world.pointer;
}
/* Compare before restoring. If another engine action changed this transient
 * value, its value wins. Failed reads/writes retain a bounded retry record;
 * invalid captured identities are dropped without touching their memory.
 */
static int guest_camera_restore(uintptr_t base,const char *reason,DWORD now) {
    if(!guest_camera_lease.tracked) return 1;
    if(!guest_camera_lease.applied) {memset(&guest_camera_lease,0,sizeof guest_camera_lease);return 1;}
    if(guest_camera_lease.retry_at&&now-guest_camera_lease.retry_at<1000) return 0;
    guest_camera_lease.retry_at=now;
    if(!guest_camera_captured_pawn_valid(base,1)) {
        guest_camera_log("restore-skipped","captured-identity-invalid",now,0,0,0);
        memset(&guest_camera_lease,0,sizeof guest_camera_lease);return 1;
    }
    uint32_t current=0;
    if(!read_at(guest_camera_lease.pawn.pointer+0xe14,&current,4)) {guest_camera_read_failure("restore-scale",now);return 0;}
    if(!guest_camera_owned_value(current)) {
        guest_camera_log("restore-preserved","external-scale-change",now,current==guest_camera_lease.baseline_bits,current,1);
        memset(&guest_camera_lease,0,sizeof guest_camera_lease);return 1;
    }
    SIZE_T wrote=0;uint32_t after=0;
    int ok=WriteProcessMemory(GetCurrentProcess(),(void*)(guest_camera_lease.pawn.pointer+0xe14),&guest_camera_lease.baseline_bits,4,&wrote)&&wrote==4;
    int checked=read_at(guest_camera_lease.pawn.pointer+0xe14,&after,4);
    int verified=checked&&after==guest_camera_lease.baseline_bits;
    guest_camera_log(verified?"restored":"restore-failure",reason,now,verified,checked?after:current,checked);
    if(verified) {memset(&guest_camera_lease,0,sizeof guest_camera_lease);return 1;}
    if(!ok||!checked) return 0;
    /* A readable externally replaced value no longer belongs to us. */
    if(!guest_camera_owned_value(after)) {memset(&guest_camera_lease,0,sizeof guest_camera_lease);return 1;}
    return 0;
}
/* Use only already discovered positive channel records. Discovery belongs to
 * the bounded network sampler; this pre/post Tick guard never scans GObjects
 * or calls network_actor_channel_id's potentially expensive cold path. */
static int guest_camera_cached_channel(uintptr_t base,uintptr_t pawn) {
    if(!network_actor_layout_matches(base)) return 0;
    uintptr_t channel=0;int index=0;
    for(unsigned int i=0;i<4;++i) {
        NetworkActorIdentityEntry *cached=&network_actor_identity_cache[i],current={0};
        if(cached->base!=base||cached->pawn!=pawn||cached->index<=0) continue;
        if(!network_actor_channel_valid(base,pawn,cached->channel,cached->driver,&current)||
           current.index!=cached->index||current.connection!=cached->connection) continue;
        if(channel&&channel!=current.channel) return 0;
        channel=current.channel;index=current.index;
    }
    return channel&&index>0;
}
static int guest_camera_conditions(uintptr_t base,uintptr_t pc,uintptr_t pawn,uintptr_t world,
                                    int ordinary_mode,int enabled,uintptr_t *camera_out,uintptr_t *mode_out) {
    unsigned char net=255,current_mode=255,ignore=255;uint32_t disable=0,pc_flags=0,pawn_flags=0,busy=0;
    int32_t action=-1,power=-1;
    guest_camera_eligibility_reason="eligible";
    if(!enabled) return guest_camera_reject("disabled");
    if(!ordinary_mode) return guest_camera_reject("not-ordinary-scene-gate");
    if(!guest_camera_actor_live(base,pc)||!is_kind_of(base,pc,"SFXPlayerController")||
       !guest_camera_actor_live(base,pawn)||!is_kind_of(base,pawn,"SFXPawn_Ashley")||!is_kind_of(base,pawn,"BioPawn")||
       !guest_camera_actor_live(base,world)||!is_kind_of(base,world,"WorldInfo")) return guest_camera_reject("actor-identity-or-class");
    if(read_ptr(pc+0x198)!=world||read_ptr(pawn+0x198)!=world||!read_at(world+0x7c8,&net,1)||net!=3) return guest_camera_reject("world-or-netmode");
    if(read_ptr(pc+0x3bc)!=pawn||read_ptr(pawn+0x374)!=pc||read_ptr(pc+0x694)!=pawn) return guest_camera_reject("possession-or-acknowledged-pawn");
    if(!read_at(pc+0x77c,&pc_flags,4)||(pc_flags&(0x800|0x20000))||!read_at(pc+0x781,&ignore,1)||ignore) return guest_camera_reject("controller-cinematic-or-move-input");
    if(!read_at(pawn+0xc20,&action,4)||action||!read_at(pawn+0xc28,&power,4)||power) return guest_camera_reject("custom-action-or-power");
    if(
       !read_at(pawn+0xe18,&pawn_flags,4)||(pawn_flags&(0x200000|0x20000000))||
       !read_at(pawn+0xe20,&busy,4)||(busy&0x40000)) return guest_camera_reject("pawn-frozen-root-motion-or-conversation");
    uintptr_t player=read_ptr(pc+0x67c),manager=read_ptr(pc+0xc38);
    if(!is_live(base,player)||!is_kind_of(base,player,"LocalPlayer")||read_ptr(player+0x68)!=pc||
       !is_live(base,manager)||!is_kind_of(base,manager,"SFXGameModeManager")||
       !read_at(manager+0xa8,&current_mode,1)||current_mode!=0||!read_at(manager+0xa4,&disable,4)||(disable&1)) return guest_camera_reject("local-player-or-manager-state");
    GameArray modes={0};uint32_t permissions=0;
    if(!read_at(manager+0x70,&modes,sizeof modes)||!modes.data||modes.count<1||modes.count>64||modes.max<modes.count||modes.max>128) return guest_camera_reject("manager-mode-array");
    uintptr_t default_mode=read_ptr(modes.data);
    if(!is_live(base,default_mode)||!is_kind_of(base,default_mode,"SFXGameModeDefault")||
       !read_at(default_mode+0xd0,&permissions,4)||(permissions&0x800)||
       (permissions&(2|0x400|0x1000))!=(2|0x400|0x1000)) return guest_camera_reject("default-mode-class-or-permissions");
    uintptr_t camera=read_ptr(pc+0x684),mode=0;uint32_t camera_flags=0,mode_flags=0;
    if(!guest_camera_actor_live(base,camera)||!is_kind_of(base,camera,"SFXPlayerCamera")||read_ptr(pc+0xe28)!=camera||
       read_ptr(camera+0x4a8)!=pc||read_ptr(camera+0x198)!=world||read_ptr(camera+0x3f4)!=pawn||
       !read_at(camera+0x57c,&camera_flags,4)||(camera_flags&1)) return guest_camera_reject("camera-reciprocity-target-or-disabled");
    mode=read_ptr(camera+0x558);
    if(!is_live(base,mode)||!is_kind_of(base,mode,"SFXCameraMode_Combat")||read_ptr(mode+0x7c)!=pawn||
       !read_at(mode+0x11c,&mode_flags,4)||(mode_flags&0x20)||!(mode_flags&2)) return guest_camera_reject("combat-mode-target-or-flags");
    if(!guest_camera_cached_channel(base,pawn)) return guest_camera_reject("positive-channel-cache-not-ready");
    *camera_out=camera;*mode_out=mode;return 1;
}
static void guest_camera_log_eligibility(uintptr_t base,uintptr_t pc,uintptr_t pawn,uintptr_t world,
                                         int ordinary_mode,int enabled,DWORD now) {
    if(guest_camera_last_eligibility&&now-guest_camera_last_eligibility<5000) return;
    if(!guest_camera_actor_live(base,pc)||!guest_camera_actor_live(base,pawn)||!is_kind_of(base,pawn,"SFXPawn_Ashley")||
       read_ptr(pc+0x3bc)!=pawn||read_ptr(pawn+0x374)!=pc) return;
    guest_camera_last_eligibility=now;
    uintptr_t manager=read_ptr(pc+0xc38),camera=read_ptr(pc+0x684),mode=0,default_mode=0;
    unsigned char net=255,current=255,ignore=255;uint32_t disable=0,permissions=0,camera_flags=0,mode_flags=0;int32_t action=-1,power=-1;
    int net_valid=guest_camera_actor_live(base,world)&&read_at(world+0x7c8,&net,1);
    int manager_valid=is_live(base,manager)&&is_kind_of(base,manager,"SFXGameModeManager");
    int manager_fields=manager_valid&&read_at(manager+0xa8,&current,1)&&read_at(manager+0xa4,&disable,4);
    GameArray modes={0};int permissions_valid=0;
    if(manager_valid&&read_at(manager+0x70,&modes,sizeof modes)&&modes.data&&modes.count>0&&modes.count<=64&&modes.max>=modes.count&&modes.max<=128) {
        default_mode=read_ptr(modes.data);
        permissions_valid=is_live(base,default_mode)&&is_kind_of(base,default_mode,"SFXGameModeDefault")&&read_at(default_mode+0xd0,&permissions,4);
    }
    int camera_valid=guest_camera_actor_live(base,camera)&&is_kind_of(base,camera,"SFXPlayerCamera");
    int camera_flags_valid=camera_valid&&read_at(camera+0x57c,&camera_flags,4);
    if(camera_valid) mode=read_ptr(camera+0x558);
    int mode_valid=is_live(base,mode)&&is_kind_of(base,mode,"SFXCameraMode_Combat");
    int mode_flags_valid=mode_valid&&read_at(mode+0x11c,&mode_flags,4);
    int actions_valid=read_at(pawn+0xc20,&action,4)&&read_at(pawn+0xc28,&power,4);
    int ignore_valid=read_at(pc+0x781,&ignore,1);char row[2200];
    snprintf(row,sizeof row,"{\"guest_camera_adapter\":\"eligibility\",\"reason\":\"%s\",\"enabled\":%s,\"ordinary_gate\":%s,\"netmode_valid\":%s,\"netmode\":%u,\"acknowledged_pawn_matches\":%s,\"manager_fields_valid\":%s,\"manager_mode\":%u,\"manager_disable_flags\":%u,\"permissions_valid\":%s,\"default_permissions\":%u,\"ignore_move_valid\":%s,\"ignore_move\":%u,\"actions_valid\":%s,\"action\":%d,\"power_action\":%d,\"camera_valid\":%s,\"my_camera_matches\":%s,\"camera_owner_matches\":%s,\"camera_target_matches\":%s,\"camera_flags_valid\":%s,\"camera_flags\":%u,\"combat_mode_valid\":%s,\"mode_target_matches\":%s,\"mode_flags_valid\":%s,\"mode_flags\":%u,\"positive_channel_cached\":%s,\"monotonic_ms\":%lu}\r\n",
        guest_camera_eligibility_reason,enabled?"true":"false",ordinary_mode?"true":"false",net_valid?"true":"false",net,
        read_ptr(pc+0x694)==pawn?"true":"false",manager_fields?"true":"false",current,disable,permissions_valid?"true":"false",permissions,
        ignore_valid?"true":"false",ignore,actions_valid?"true":"false",action,power,camera_valid?"true":"false",camera_valid&&read_ptr(pc+0xe28)==camera?"true":"false",
        camera_valid&&read_ptr(camera+0x4a8)==pc?"true":"false",camera_valid&&read_ptr(camera+0x3f4)==pawn?"true":"false",camera_flags_valid?"true":"false",camera_flags,
        mode_valid?"true":"false",mode_valid&&read_ptr(mode+0x7c)==pawn?"true":"false",mode_flags_valid?"true":"false",mode_flags,
        guest_camera_cached_channel(base,pawn)?"true":"false",now);line(row);
}
static void guest_camera_adapt(uintptr_t base,uintptr_t pc,uintptr_t pawn,uintptr_t world,
                               int ordinary_mode,int enabled,DWORD now) {
    uintptr_t camera=0,mode=0;
    int eligible=guest_camera_conditions(base,pc,pawn,world,ordinary_mode,enabled,&camera,&mode);
    if(!eligible) guest_camera_log_eligibility(base,pc,pawn,world,ordinary_mode,enabled,now);
    if(guest_camera_lease.tracked) {
        int same=eligible&&pc==guest_camera_lease.pc.pointer&&pawn==guest_camera_lease.pawn.pointer&&world==guest_camera_lease.world.pointer&&
            camera==guest_camera_lease.camera.pointer&&mode==guest_camera_lease.mode.pointer&&
            guest_camera_stamp_matches(base,&guest_camera_lease.pc,0)&&guest_camera_stamp_matches(base,&guest_camera_lease.pawn,0)&&
            guest_camera_stamp_matches(base,&guest_camera_lease.world,0)&&guest_camera_stamp_matches(base,&guest_camera_lease.camera,0)&&guest_camera_stamp_matches(base,&guest_camera_lease.mode,0);
        if(!same) {
            guest_camera_restore(base,!enabled?"disabled":!ordinary_mode?"scene":"context-or-ownership-change",now);
            return;
        }
        if(guest_camera_lease.applied) {
            uint32_t current=0;
            if(!read_at(pawn+0xe14,&current,4)) {guest_camera_read_failure("monitor-scale",now);return;}
            if(!guest_camera_owned_value(current)) {
                guest_camera_log("preserved","external-scale-change",now,1,current,1);
                guest_camera_lease.applied=0;guest_camera_lease.suppressed=1;
            }
        }
        return;
    }
    if(!eligible) return;
    GuestCameraLease next={0};uint32_t baseline=0;
    if(!guest_camera_stamp_read(base,pc,&next.pc,1)||!guest_camera_stamp_read(base,pawn,&next.pawn,1)||
       !guest_camera_stamp_read(base,world,&next.world,1)||!guest_camera_stamp_read(base,camera,&next.camera,1)||
       !guest_camera_stamp_read(base,mode,&next.mode,1)||!read_at(pawn+0xe14,&baseline,4)) {
        guest_camera_read_failure("capture-handles-or-baseline",now);return;
    }
    next.tracked=1;next.baseline_bits=baseline;guest_camera_lease=next;
    if(!guest_camera_zero_baseline(baseline)) {
        guest_camera_lease.suppressed=1;guest_camera_log("skipped","baseline-is-not-finite-zero",now,1,baseline,1);return;
    }
    /* Revalidate all captured identities and conditions immediately before
     * the only apply write. No field is written on a failed guard. */
    uintptr_t checked_camera=0,checked_mode=0;uint32_t current=0;
    if(!guest_camera_conditions(base,pc,pawn,world,ordinary_mode,enabled,&checked_camera,&checked_mode)||checked_camera!=camera||checked_mode!=mode||
       !guest_camera_captured_pawn_valid(base,1)||!guest_camera_stamp_matches(base,&next.pc,1)||
       !guest_camera_stamp_matches(base,&next.camera,1)||!guest_camera_stamp_matches(base,&next.mode,1)||
       !read_at(pawn+0xe14,&current,4)||current!=baseline) {
        guest_camera_log("skipped","capture-context-changed",now,0,current,0);guest_camera_lease.suppressed=1;return;
    }
    uint32_t adapted=guest_camera_float_bits(1.0f),after=0;SIZE_T wrote=0;
    int ok=WriteProcessMemory(GetCurrentProcess(),(void*)(pawn+0xe14),&adapted,4,&wrote)&&wrote==4;
    int checked=read_at(pawn+0xe14,&after,4);
    int verified=checked&&after==adapted;
    guest_camera_lease.applied=verified||wrote>0;
    guest_camera_lease.suppressed=!verified;
    guest_camera_log(verified?"applied":"apply-failure",ok?"normal-combat-arm":"write-failed",now,verified,checked?after:baseline,checked);
}
/* Pure tests: stale/reused identities, signed-zero baseline preservation and
 * compare-before-restore policy. Main may expose this via its native runner.
 */
static int guest_camera_adapter_selftest(void) {
    GuestCameraStamp a={0},b={0};a.pointer=b.pointer=16;a.klass=b.klass=32;a.index=b.index=1;a.class_index=b.class_index=2;
    a.name[0]=b.name[0]=3;a.name[1]=b.name[1]=4;a.class_name[0]=b.class_name[0]=5;
    if(!guest_camera_stamp_core_equal(&a,&b)) return 0;
    b.name[1]++;if(guest_camera_stamp_core_equal(&a,&b)) return 0;b=a;b.index++;if(guest_camera_stamp_core_equal(&a,&b)) return 0;
    b=a;b.klass++;if(guest_camera_stamp_core_equal(&a,&b)) return 0;b=a;b.class_name[0]++;if(guest_camera_stamp_core_equal(&a,&b)) return 0;
    return guest_camera_zero_baseline(guest_camera_float_bits(0.0f))&&guest_camera_zero_baseline(guest_camera_float_bits(-0.0f))&&
        !guest_camera_zero_baseline(guest_camera_float_bits(1.0f))&&!guest_camera_zero_baseline(guest_camera_float_bits(NAN))&&
        guest_camera_owned_value(guest_camera_float_bits(1.0f))&&!guest_camera_owned_value(guest_camera_float_bits(0.5f))&&!guest_camera_owned_value(guest_camera_float_bits(0.0f));
}
#endif
