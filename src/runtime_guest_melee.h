#ifndef ME3_RUNTIME_GUEST_MELEE_H
#define ME3_RUNTIME_GUEST_MELEE_H
#include "native_power_manager_event_pins.h"
/* Prepare normal player melee combos and the guarded Soldier heavy/storm slots.
 * Stock input, animation, targeting, authority damage and action replication
 * remain responsible for execution. Add the pawn's OWN melee passive through
 * PowerManager.AddPower; never borrow Shepard's power instance or stats. */
typedef struct RuntimeMeleeRow {GuestCameraStamp klass,cdo,instance,anim_set;uint32_t anim[2],anim_alt[2];int changed,anim_registered;} RuntimeMeleeRow;
typedef struct RuntimeGuestMelee {
    RuntimeMovementScope scope;
    SessionHandle source,manager;
    GuestCameraStamp passive_class,passive_cdo,passive;
    GuestPlayerContextMethod add,remove,register_anim,unregister_anim;
    GuestCameraStamp anim_set;
    RuntimeMeleeRow rows[19];
    uint32_t prepared;
    int valid,retiring,created,anim_registered,foreign,dispatching;
    DWORD checked,monitor;
} RuntimeGuestMelee;
static RuntimeGuestMelee runtime_guest_melee;
static unsigned runtime_melee_events;
static const unsigned runtime_melee_indices[]={75,76,77,78,79,80,58,73,62,63,64,65,66,67,68,69,70,71,72};
#define RUNTIME_MELEE_ROWS 19
#define RUNTIME_MELEE_NORMAL_MASK UINT32_C(0x3f)
#define RUNTIME_MELEE_HEAVY_MASK UINT32_C(0xc0)
#define RUNTIME_MELEE_COVER_MASK UINT32_C(0x3ff00)
static int runtime_melee_row_prepared(const RuntimeGuestMelee *q,unsigned i) {
    return i<RUNTIME_MELEE_ROWS&&(q->prepared&(UINT32_C(1)<<i));
}
static int runtime_melee_row_is_sync(unsigned i) {
    return i<RUNTIME_MELEE_ROWS&&guest_cover_melee_synced((int)runtime_melee_indices[i]);
}
static const char *runtime_melee_row_base(unsigned i) {
    return i==6||i==7?"SFXCustomAction_PlayerHeavyMeleeBase":runtime_melee_row_is_sync(i)?
        "SFXCustomAction_ClassMelee":"SFXCustomAction_PlayerMeleeBase";
}
static uintptr_t runtime_melee_row_power_offset(unsigned i) {
    return i==6||i==7?0x19c:runtime_melee_row_is_sync(i)?0x17c:0x1fc;
}
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
    int heavy=runtime_guest_melee.valid&&!runtime_guest_melee.retiring&&
        runtime_guest_melee.rows[6].changed&&runtime_guest_melee.rows[7].changed;
    unsigned cover=0;for(unsigned i=8;i<RUNTIME_MELEE_ROWS;++i)if(runtime_guest_melee.rows[i].changed)++cover;
    snprintf(text,sizeof text,"{\"runtime_guest_melee\":\"%s\",\"role\":%d,\"changed_slots\":%u,\"heavy_melee_ready\":%s,\"cover_slots\":%u,\"damage_verified\":false,\"monotonic_ms\":%lu}\r\n",event,(int)g_runtime_config.role,slots,heavy?"true":"false",cover,now);line(text);
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
static int runtime_melee_stance_at(uintptr_t cdo,uintptr_t offset,uint32_t out[2]) {
    GameArray anim={0};unsigned clips=0;uint32_t name[2];
    if(!identity_array(cdo+offset,&anim,8)||anim.count<1)return 0;
    for(int n=0;n<anim.count;++n){if(!read_at(anim.data+(SIZE_T)n*8,name,8)||name[1])return 0;
        if(name[0]){if(clips++)return 0;memcpy(out,name,8);}}
    return clips==1;
}
static int runtime_melee_stance_clip(uintptr_t base,uintptr_t cdo,uint32_t out[2]) {
    return runtime_power_property(base,cdo,"SFXCustomAction_SingleAnim","BS_Anim","StructProperty",0x110,16)&&
        runtime_melee_stance_at(cdo,0x110,out);
}
static int runtime_melee_cover_pistol_stance(uintptr_t base,uintptr_t cdo,uint32_t out[2]) {
    uintptr_t klass=read_ptr(cdo+0x50),field=read_ptr(klass+0x70);char text[96];
    for(unsigned n=0;field&&n<128;++n){
        if(!is_live(base,field)||read_ptr(field+0x40)!=klass)return 0;
        object_name(base,field,text,sizeof text);
        if(!strcmp(text,"BS_AnimPistol")){int32_t offset=-1;
            return read_at(field+0x94,&offset,4)&&offset>=0x250&&offset<=0x400&&
                runtime_power_property(base,cdo,"SFXCustomAction_SyncCoverMelee","BS_AnimPistol","StructProperty",offset,16)&&
                runtime_melee_stance_at(cdo,(uintptr_t)offset,out);
        }
        field=read_ptr(field+0x68);
    }
    return 0;
}
static void runtime_melee_cover_source(uintptr_t base,RuntimeGuestMelee *q,const GameArray *classes) {
    for(unsigned i=8;i<RUNTIME_MELEE_ROWS;++i){unsigned index=runtime_melee_indices[i];
        /* Slot72 has no authored animation info on the supported player.
         * It is observed, but not borrowed without that native prerequisite. */
        if(index==72)continue;
        if(runtime_melee_row_prepared(q,i))continue;RuntimeMeleeRow candidate={0};
        uintptr_t klass=read_ptr(classes->data+(SIZE_T)index*8);
        if(read_ptr(q->scope.classes.data+(SIZE_T)index*8)||read_ptr(runtime_melee_cell(&q->scope.actions,index))||
           !narrative_probe_object(base,klass,"Class")||!guest_camera_stamp_read(base,klass,&candidate.klass,1)||
           strcmp(candidate.klass.full_name,guest_cover_melee_class((int)index))||
           !guest_camera_stamp_read(base,read_ptr(klass+0x13c),&candidate.cdo,1)||read_ptr(candidate.cdo.pointer+0x50)!=klass||
           !is_kind_of(base,candidate.cdo.pointer,runtime_melee_row_base(i))||
           !runtime_power_property(base,candidate.cdo.pointer,runtime_melee_row_base(i),"Power","ObjectProperty",runtime_melee_row_power_offset(i),8))continue;
        if(runtime_melee_row_is_sync(i)){
            uintptr_t cdo=candidate.cdo.pointer,anim=read_ptr(cdo+0x174);GameArray sequences={0};
            if(!runtime_power_property(base,cdo,"SFXCustomAction_ClassMelee","PowerClass","ClassProperty",0x16c,8)||
               (read_ptr(cdo+0x16c)&&read_ptr(cdo+0x16c)!=q->passive_class.pointer)||
               !runtime_power_property(base,cdo,"SFXCustomAction_ClassMelee","AnimInfo","ObjectProperty",0x174,8)||
               !narrative_probe_object(base,anim,"SFXAnimSetCookSpec")||
               !runtime_power_property(base,anim,"SFXAnimSetCookSpec","AnimSet","ObjectProperty",0x60,8)||
               !narrative_probe_object(base,read_ptr(anim+0x60),"AnimSet")||
               !identity_array(read_ptr(anim+0x60)+0x60,&sequences,4096)||!sequences.count)continue;
        } else if(!runtime_melee_stance_clip(base,candidate.cdo.pointer,candidate.anim)||
                  (index==63&&!runtime_melee_cover_pistol_stance(base,candidate.cdo.pointer,candidate.anim_alt)))continue;
        q->rows[i]=candidate;q->prepared|=UINT32_C(1)<<i;
    }
}
static void runtime_melee_heavy_source(uintptr_t base,RuntimeGuestMelee *q,const GameArray *classes) {
    for(unsigned i=6;i<8;++i){unsigned index=runtime_melee_indices[i];RuntimeMeleeRow *row=&q->rows[i];
        uintptr_t klass=read_ptr(classes->data+(SIZE_T)index*8);
        if(read_ptr(q->scope.classes.data+(SIZE_T)index*8)||read_ptr(runtime_melee_cell(&q->scope.actions,index))||
           !runtime_movement_named_class(base,klass,"SFXCustomAction_SoldierPunch",&row->klass)||
           !guest_camera_stamp_read(base,read_ptr(klass+0x13c),&row->cdo,1)||read_ptr(row->cdo.pointer+0x50)!=klass||
           !is_kind_of(base,row->cdo.pointer,"SFXCustomAction_PlayerHeavyMeleeBase")||
           !runtime_power_property(base,row->cdo.pointer,"SFXCustomAction_PlayerHeavyMeleeBase","Power","ObjectProperty",0x19c,8)||
           !runtime_power_property(base,row->cdo.pointer,"SFXCustomAction_PlayerHeavyMeleeBase","AnimInfo","ObjectProperty",0x194,8))return;
        uintptr_t anim=read_ptr(row->cdo.pointer+0x194);GameArray sequences={0};
        if(!narrative_probe_object(base,anim,"SFXAnimSetCookSpec")||
           !runtime_power_property(base,anim,"SFXAnimSetCookSpec","AnimSet","ObjectProperty",0x60,8)||
           !narrative_probe_object(base,read_ptr(anim+0x60),"AnimSet")||
           !identity_array(read_ptr(anim+0x60)+0x60,&sequences,4096)||!sequences.count)return;
    }
    if(runtime_heavy_plot_prepare(base,q->scope.info.pointer,q->rows[6].klass.pointer)&&runtime_heavy_plot_authorize(base,&q->scope,q->rows[6].klass,q->rows[6].cdo)&&
       runtime_heavy_plot_authorize(base,&q->scope,q->rows[7].klass,q->rows[7].cdo))q->prepared|=RUNTIME_MELEE_HEAVY_MASK;
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
        if(!runtime_melee_stance_clip(base,row->cdo.pointer,row->anim))return 0;
    }
    uintptr_t klass=read_ptr(powers.data+43*8);
    int valid=runtime_movement_named_class(base,klass,"SFXPowerCustomAction_SoldierMeleePassive",&q->passive_class)&&
        guest_camera_stamp_read(base,read_ptr(klass+0x13c),&q->passive_cdo,1)&&
        read_ptr(q->passive_cdo.pointer+0x50)==klass&&is_kind_of(base,q->passive_cdo.pointer,"SFXPowerCustomAction_MeleePassivePower")&&
        runtime_power_property(base,q->passive_cdo.pointer,"SFXPowerCustomActionBase","PowerCustomActionID","IntProperty",0x484,4)&&
        runtime_melee_power_id(q->passive_cdo.pointer);
    if(valid){runtime_melee_heavy_source(base,q,&classes);runtime_melee_cover_source(base,q,&classes);}
    return valid;
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
static int runtime_melee_temp_refcount_at(uintptr_t base,RuntimeGuestMelee *q,uintptr_t set) {
    GameArray array={0};int count=-1;unsigned found=0;uintptr_t pawn=q->scope.pawn.pointer;
    if(!runtime_power_property(base,pawn,"BioPawn","TemporaryAnims","ArrayProperty",0x76c,16)||
       !scene_readiness_array(pawn+0x76c,&array,128))return -1;
    for(int i=0;i<array.count;++i)if(read_ptr(array.data+(SIZE_T)i*16)==set){
        if(found++||!read_at(array.data+(SIZE_T)i*16+8,&count,4)||count<0||count>4096)return -1;
    }
    return found==1?count:0;
}
static int runtime_melee_temp_refcount(uintptr_t base,RuntimeGuestMelee *q) {
    return runtime_melee_temp_refcount_at(base,q,q->anim_set.pointer);
}
static int runtime_melee_set_has_clip(uintptr_t base,uintptr_t set,const uint32_t name[2]) {
    GameArray sequences={0},again={0};int found=0;
    if(!name[0]||name[1]||!narrative_probe_object(base,set,"AnimSet")||
       !runtime_power_property(base,set,"AnimSet","Sequences","ArrayProperty",0x60,16)||
       !scene_readiness_array(set+0x60,&sequences,4096))return 0;
    for(int n=0;n<sequences.count;++n){uintptr_t sequence=read_ptr(sequences.data+(SIZE_T)n*8);uint32_t actual[2];
        if(!sequence)continue;
        if(!narrative_probe_object(base,sequence,"AnimSequence")||!read_at(sequence+0xf4,actual,8))return 0;
        if(!memcmp(name,actual,8))found=1;
    }
    return found&&read_at(set+0x60,&again,sizeof again)&&!memcmp(&sequences,&again,sizeof sequences);
}
static int runtime_melee_pawn_clip(uintptr_t base,uintptr_t pawn,const uint32_t name[2],GuestCameraStamp *out) {
    uintptr_t mesh=read_ptr(pawn+0x3f4);GameArray sets={0},again={0};GuestCameraStamp matched={0};
    if(!guest_camera_actor_live(base,pawn)||!narrative_probe_object(base,mesh,"SkeletalMeshComponent")||
       !runtime_power_property(base,mesh,"SkeletalMeshComponent","AnimSets","ArrayProperty",0x320,16)||
       !identity_array(mesh+0x320,&sets,64))return 0;
    for(int i=0;i<sets.count&&!matched.pointer;++i){uintptr_t set=read_ptr(sets.data+(SIZE_T)i*8);
        if(set&&runtime_melee_set_has_clip(base,set,name)&&!guest_camera_stamp_read(base,set,&matched,1))return 0;
    }
    if(!matched.pointer||read_ptr(pawn+0x3f4)!=mesh||!read_at(mesh+0x320,&again,sizeof again)||memcmp(&sets,&again,sizeof sets))return 0;
    if(out)*out=matched;return 1;
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
static unsigned runtime_melee_family_mask(unsigned available) {
    return ((available&7)==7?7u:0u)|((available&56)==56?56u:0u);
}
/* Rifle and pistol combos are independent native families. A missing pistol
 * set must not disable rifle, heavy and cover melee too. Resolve each clip
 * through the actual loaded player mesh; split source sets are valid. */
static void runtime_melee_normal_prepare(uintptr_t base,RuntimeGuestMelee *q) {
    unsigned available=0;
    for(unsigned i=0;i<6;++i){RuntimeMeleeRow *row=&q->rows[i];
        if(row->changed||runtime_melee_pawn_clip(base,q->scope.pawn.pointer,row->anim,NULL)){available|=1u<<i;continue;}
        GuestCameraStamp set={0};
        if(runtime_melee_pawn_clip(base,q->source.pointer,row->anim,&set)&&
           runtime_melee_anim_method(base,q->scope.pawn.pointer,1,&q->register_anim)&&
           runtime_melee_anim_method(base,q->scope.pawn.pointer,0,&q->unregister_anim)){
            row->anim_set=set;available|=1u<<i;
        }
    }
    q->prepared=(q->prepared&~RUNTIME_MELEE_NORMAL_MASK)|runtime_melee_family_mask(available);
}
static int runtime_melee_normal_publish(uintptr_t base,uintptr_t engine,RuntimeGuestMelee *q,DWORD now) {
    unsigned applied=0;GameArray pc={0},pa={0},powers={0};RuntimeMovementScope scope={0};
    for(unsigned i=0;i<6;++i){RuntimeMeleeRow *row=&q->rows[i];unsigned index=runtime_melee_indices[i];
        if(row->changed||!runtime_melee_row_prepared(q,i))continue;
        if(!runtime_melee_pawn_clip(base,q->scope.pawn.pointer,row->anim,NULL)){
            if(!guest_camera_stamp_matches(base,&row->anim_set,1)||!runtime_melee_set_has_clip(base,row->anim_set.pointer,row->anim)||
               !runtime_melee_anim_method(base,q->scope.pawn.pointer,1,&q->register_anim)||
               !runtime_melee_anim_method(base,q->scope.pawn.pointer,0,&q->unregister_anim)||
               !guest_equipment_dispatch_pins(base)||read_ptr(read_ptr(q->scope.pawn.pointer)+0x230)!=base+0x40d450)return -1;
            int before=runtime_melee_temp_refcount_at(base,q,row->anim_set.pointer);if(before<0)return -1;
            uintptr_t set=row->anim_set.pointer;q->dispatching=1;
            ((ProcessEvent)(base+0x40d450))((void*)q->scope.pawn.pointer,(void*)q->register_anim.function.pointer,&set,NULL);
            q->dispatching=0;row->anim_registered=1;
            if(q->retiring||!runtime_movement_scope(base,engine,&scope)||!runtime_movement_scope_same(&q->scope,&scope)||
               !runtime_movement_idle(q->scope.pawn.pointer)||runtime_melee_temp_refcount_at(base,q,set)!=before+1||
               !runtime_melee_pawn_clip(base,q->scope.pawn.pointer,row->anim,NULL))return -1;
        }
        if(!runtime_melee_arrays(base,q,&pc,&pa,&powers)||!guest_camera_stamp_matches(base,&row->klass,1)||
           !guest_camera_stamp_matches(base,&row->cdo,1)||
           !runtime_movement_borrow(q->scope.classes.data+(SIZE_T)index*8,runtime_melee_cell(&q->scope.actions,index),0,row->klass.pointer,0))return -1;
        row->changed=1;++applied;
    }
    if(applied)runtime_melee_note("native-normal-melee-family-ready",applied,now);return (int)applied;
}
static int runtime_melee_instance(uintptr_t base,RuntimeGuestMelee *q,unsigned i,uintptr_t action) {
    uintptr_t power_field=runtime_melee_row_power_offset(i);
    return runtime_melee_row_prepared(q,i)&&narrative_probe_object(base,action,runtime_melee_row_base(i))&&
        read_ptr(action+0x50)==q->rows[i].klass.pointer&&read_ptr(action+0x98)==q->scope.pawn.pointer&&
        runtime_power_action_context_layout(base,action)&&
        (!read_ptr(action+power_field)||read_ptr(action+power_field)==q->passive.pointer);
}
static int runtime_melee_instance_idle(uintptr_t action) {
    uint32_t flags=0;return read_at(action+0x100,&flags,4)&&!(flags&0x10800000)&&!read_ptr(action+0xb8);
}
static int runtime_melee_owns_passive(uintptr_t base,RuntimeGuestMelee *q,GameArray *pc,GameArray *pa,GameArray *powers);
static int runtime_melee_cover_publish(uintptr_t base,uintptr_t engine,RuntimeGuestMelee *q,DWORD now) {
    unsigned applied=0;RuntimeMovementScope scope={0};GameArray pc={0},pa={0},powers={0};
    for(unsigned i=8;i<RUNTIME_MELEE_ROWS;++i){RuntimeMeleeRow *row=&q->rows[i];unsigned index=runtime_melee_indices[i];
        if(!runtime_melee_row_prepared(q,i)||row->changed)continue;
        if(q->retiring||!runtime_movement_scope(base,engine,&scope)||!runtime_movement_scope_same(&q->scope,&scope)||
           !runtime_movement_idle(q->scope.pawn.pointer)||!runtime_melee_owns_passive(base,q,&pc,&pa,&powers))return -1;
        int main_loaded=runtime_melee_row_is_sync(i)||runtime_melee_pawn_clip(base,q->scope.pawn.pointer,row->anim,NULL);
        int alternate_loaded=!row->anim_alt[0]||runtime_melee_pawn_clip(base,q->scope.pawn.pointer,row->anim_alt,NULL);
        if(!main_loaded||!alternate_loaded){const uint32_t *missing=main_loaded?row->anim_alt:row->anim;
            if(!session_handle_matches(q->source)||!runtime_melee_pawn_clip(base,q->source.pointer,missing,&row->anim_set)||
               !runtime_melee_anim_method(base,q->scope.pawn.pointer,1,&q->register_anim)||
               !runtime_melee_anim_method(base,q->scope.pawn.pointer,0,&q->unregister_anim)){
                q->prepared&=~(UINT32_C(1)<<i);continue;
            }
            /* A slot with two absent stances needs one loaded set containing
             * both. Defer this optional slot before native mutation when the
             * source splits them; never retire the core for that asset gap. */
            if((!main_loaded&&!runtime_melee_set_has_clip(base,row->anim_set.pointer,row->anim))||
               (!alternate_loaded&&!runtime_melee_set_has_clip(base,row->anim_set.pointer,row->anim_alt))){
                q->prepared&=~(UINT32_C(1)<<i);continue;
            }
            if(!guest_camera_stamp_matches(base,&row->anim_set,1)||!guest_equipment_dispatch_pins(base)||
               read_ptr(read_ptr(q->scope.pawn.pointer)+0x230)!=base+0x40d450)return -1;
            uintptr_t set=row->anim_set.pointer;q->dispatching=1;
            ((ProcessEvent)(base+0x40d450))((void*)q->scope.pawn.pointer,(void*)q->register_anim.function.pointer,&set,NULL);
            q->dispatching=0;row->anim_registered=1;
            if(q->retiring||!runtime_movement_scope(base,engine,&scope)||!runtime_movement_scope_same(&q->scope,&scope)||
               !runtime_movement_idle(q->scope.pawn.pointer)||runtime_melee_temp_refcount_at(base,q,set)<=0||
               !runtime_melee_pawn_clip(base,q->scope.pawn.pointer,row->anim,NULL)||
               (row->anim_alt[0]&&!runtime_melee_pawn_clip(base,q->scope.pawn.pointer,row->anim_alt,NULL)))return -1;
        }
        if(!runtime_melee_arrays(base,q,&pc,&pa,&powers)||!guest_camera_stamp_matches(base,&row->klass,1)||
           !guest_camera_stamp_matches(base,&row->cdo,1)||
           !runtime_movement_borrow(q->scope.classes.data+(SIZE_T)index*8,runtime_melee_cell(&q->scope.actions,index),0,row->klass.pointer,0))return -1;
        row->changed=1;++applied;
    }
    if(applied)runtime_melee_note("native-cover-melee-slots-ready",applied,now);
    return (int)applied;
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
static int runtime_melee_passive_absent(const RuntimeGuestMelee *q,const GameArray *pc,const GameArray *pa,const GameArray *powers) {
    if(read_ptr(runtime_melee_cell(pc,43))==q->passive_class.pointer||
       (q->passive.pointer&&read_ptr(runtime_melee_cell(pa,43))==q->passive.pointer))return 0;
    for(int i=0;i<powers->count;++i){uintptr_t power=read_ptr(powers->data+(SIZE_T)i*8);
        if((q->passive.pointer&&power==q->passive.pointer)||
           (!q->passive.pointer&&read_ptr(power+0x50)==q->passive_class.pointer))return 0;}
    return 1;
}
static int runtime_melee_finish_remove(RuntimeGuestMelee *q,const GameArray *pc,const GameArray *pa,const GameArray *powers) {
    if(!runtime_melee_passive_absent(q,pc,pa,powers))return 0;
    q->created=0;return 1;
}
static int runtime_melee_finish_unregister(RuntimeGuestMelee *q,int before,int after) {
    if(before<=0||after!=before-1)return 0;
    q->anim_registered=0;return 1;
}
static int runtime_melee_restored_slots_empty(const RuntimeGuestMelee *q) {
    for(unsigned i=0;i<RUNTIME_MELEE_ROWS;++i)if(read_ptr(q->scope.classes.data+(SIZE_T)runtime_melee_indices[i]*8)||
        read_ptr(runtime_melee_cell(&q->scope.actions,runtime_melee_indices[i])))return 0;
    return 1;
}
static int runtime_melee_cleanup_scope(uintptr_t base,RuntimeGuestMelee *q,GameArray *pc,GameArray *pa,GameArray *powers) {
    if(!runtime_melee_arrays(base,q,pc,pa,powers))return 0;
    uintptr_t owner=read_ptr(q->scope.pawn.pointer+0x374);
    return runtime_movement_retire_owner_policy(owner,q->scope.pc.pointer,session_handle_matches(q->scope.pc),
        guest_camera_actor_live(base,owner)&&is_kind_of(base,owner,"SFXAI_Henchman"),
        read_ptr(owner+0x3bc)==q->scope.pawn.pointer,read_ptr(owner+0x198)==q->scope.info.pointer);
}
/* Native callbacks may release ownership while a dispatch is still on the
 * stack. Keep this ledger until the call returns, then recheck its scope. */
static int runtime_melee_cleanup_resume(RuntimeGuestMelee *q,int same_scope) {
    q->dispatching=0;
    if(!same_scope){memset(q,0,sizeof *q);return 0;}
    return 1;
}
static void runtime_guest_melee_retire(uintptr_t base,uintptr_t engine,DWORD now) {
    RuntimeGuestMelee *q=&runtime_guest_melee;if(!q->valid||GetCurrentThreadId()!=session_game_thread)return;q->retiring=1;
    if(q->dispatching)return;GameArray pc={0},pa={0},powers={0};
    if(!runtime_melee_arrays(base,q,&pc,&pa,&powers)){runtime_melee_note("preserved-replaced-scope",0,now);memset(q,0,sizeof *q);return;}
    uintptr_t owner=read_ptr(q->scope.pawn.pointer+0x374);
    if(!runtime_movement_retire_owner_policy(owner,q->scope.pc.pointer,session_handle_matches(q->scope.pc),
            guest_camera_actor_live(base,owner)&&is_kind_of(base,owner,"SFXAI_Henchman"),
            read_ptr(owner+0x3bc)==q->scope.pawn.pointer,read_ptr(owner+0x198)==q->scope.info.pointer)){
        runtime_melee_note("preserved-new-owner",0,now);memset(q,0,sizeof *q);return;}
    if(!runtime_movement_idle(q->scope.pawn.pointer))return;
    unsigned restored=0;
    for(unsigned i=0;i<RUNTIME_MELEE_ROWS;++i){RuntimeMeleeRow *row=&q->rows[i];if(!row->changed)continue;
        unsigned index=runtime_melee_indices[i];
        uintptr_t cell=runtime_melee_cell(&q->scope.actions,index),action=cell?read_ptr(cell):0;
        uintptr_t class_cell=q->scope.classes.data+(SIZE_T)index*8;
        if(read_ptr(class_cell)!=row->klass.pointer||!guest_camera_stamp_matches(base,&row->klass,1)||
           (action&&!runtime_melee_instance(base,q,i,action))){row->changed=0;q->foreign=1;continue;}
        if(action&&!runtime_melee_instance_idle(action))return;
        if(runtime_movement_restore(class_cell,cell,0,row->klass.pointer,action,0)){row->changed=0;++restored;}
    }
    for(unsigned i=0;i<RUNTIME_MELEE_ROWS;++i)if(q->rows[i].changed)return;
    /* Cleanup can span ticks. A native subsystem may have filled a normal or
     * heavy slot while our passive/animation retirement was waiting. */
    if(!runtime_melee_restored_slots_empty(q))q->foreign=1;
    /* Foreign replacements can still refer to our passive. Preserve it rather
     * than deleting a native subsystem's live dependency. */
    if(q->created&&!q->foreign){
        if(runtime_melee_finish_remove(q,&pc,&pa,&powers)){}
        else if(runtime_melee_owns_passive(base,q,&pc,&pa,&powers)){
            uintptr_t manager_event=runtime_melee_manager_event(base,q->manager.pointer);
            if(!runtime_melee_instance_idle(q->passive.pointer)||!guest_player_context_method_matches(base,&q->remove,0)||
               !guest_equipment_dispatch_pins(base)||!manager_event)return;
            RuntimeMeleeRemoveParms parms={q->passive_class.pointer,0};q->dispatching=1;
            ((ProcessEvent)manager_event)((void*)q->manager.pointer,(void*)q->remove.function.pointer,&parms,NULL);
            if(!runtime_melee_cleanup_resume(q,runtime_melee_cleanup_scope(base,q,&pc,&pa,&powers)))return;
            if(!runtime_melee_restored_slots_empty(q))q->foreign=1;
            /* A refused call remains pending. A successful native removal is
             * recorded before animation cleanup, so a later retry cannot remove twice. */
            if(!runtime_melee_finish_remove(q,&pc,&pa,&powers))return;
        } else if(guest_camera_stamp_matches(base,&q->passive_class,1)&&!q->passive.pointer&&
                  read_ptr(runtime_melee_cell(&pc,43))==q->passive_class.pointer&&!read_ptr(runtime_melee_cell(&pa,43))){
            /* AddPower may fail before returning an instance. Undo only its exact
             * otherwise-empty class cell; never resize either native array. */
            int conflict=0;
            for(int i=0;i<powers.count;++i)if(read_ptr(read_ptr(powers.data+(SIZE_T)i*8)+0x50)==q->passive_class.pointer)conflict=1;
            if(conflict)q->foreign=1;
            else if(!runtime_power_action_context_pointer(runtime_melee_cell(&pc,43),q->passive_class.pointer,0))return;
            else q->created=0;
        } else q->foreign=1;
    }
    if(q->anim_registered&&!q->foreign){
        if(!runtime_movement_idle(q->scope.pawn.pointer))return;
        if(!guest_camera_stamp_matches(base,&q->anim_set,1))q->anim_registered=0;
        else {
            int before=runtime_melee_temp_refcount(base,q);if(before<0)return;
            if(!before)q->anim_registered=0;
            else {
                if(!guest_player_context_method_matches(base,&q->unregister_anim,0)||!guest_equipment_dispatch_pins(base)||
                   read_ptr(read_ptr(q->scope.pawn.pointer)+0x230)!=base+0x40d450)return;
                RuntimeMeleeAnimParms parms={q->anim_set.pointer,0.0f,0};q->dispatching=1;
                ((ProcessEvent)(base+0x40d450))((void*)q->scope.pawn.pointer,(void*)q->unregister_anim.function.pointer,&parms,NULL);
                if(!runtime_melee_cleanup_resume(q,runtime_melee_cleanup_scope(base,q,&pc,&pa,&powers)))return;
                if(!runtime_melee_finish_unregister(q,before,runtime_melee_temp_refcount(base,q)))return;
            }
        }
    }
    if(!q->foreign)for(unsigned i=0;i<RUNTIME_MELEE_ROWS;++i){RuntimeMeleeRow *row=&q->rows[i];
        if(!row->anim_registered)continue;
        if(!guest_camera_stamp_matches(base,&row->anim_set,1)){row->anim_registered=0;continue;}
        if(!runtime_movement_idle(q->scope.pawn.pointer))return;
        int before=runtime_melee_temp_refcount_at(base,q,row->anim_set.pointer);if(before<0)return;
        if(!before){row->anim_registered=0;continue;}
        if(!guest_player_context_method_matches(base,&q->unregister_anim,0)||!guest_equipment_dispatch_pins(base)||
           read_ptr(read_ptr(q->scope.pawn.pointer)+0x230)!=base+0x40d450)return;
        RuntimeMeleeAnimParms parms={row->anim_set.pointer,0.0f,0};q->dispatching=1;
        ((ProcessEvent)(base+0x40d450))((void*)q->scope.pawn.pointer,(void*)q->unregister_anim.function.pointer,&parms,NULL);
        if(!runtime_melee_cleanup_resume(q,runtime_melee_cleanup_scope(base,q,&pc,&pa,&powers)))return;
        if(!runtime_melee_restored_slots_empty(q)){q->foreign=1;break;}
        if(runtime_melee_temp_refcount_at(base,q,row->anim_set.pointer)!=before-1)return;
        row->anim_registered=0;
    }
    runtime_melee_note("normal-melee-slots-retired",restored,now);memset(q,0,sizeof *q);(void)engine;
}
static void runtime_guest_melee_tick(uintptr_t base,uintptr_t engine,DWORD now) {
    if(GetCurrentThreadId()!=session_game_thread)return;RuntimeGuestMelee *q=&runtime_guest_melee;RuntimeMovementScope scope={0};
    if(q->dispatching)return;
    int admitted=runtime_guest_melee_enabled()&&runtime_movement_scope(base,engine,&scope);
    if(q->valid){
        int retire=q->retiring||!admitted||!runtime_movement_scope_same(&q->scope,&scope)||
            ((q->prepared&RUNTIME_MELEE_HEAVY_MASK)&&!runtime_heavy_plot_ready(base));
        if(!retire&&(!q->monitor||now-q->monitor>=500)){
            q->monitor=now;GameArray pc={0},pa={0},powers={0};
            if(!runtime_melee_owns_passive(base,q,&pc,&pa,&powers))retire=1;
            for(unsigned i=0;i<RUNTIME_MELEE_ROWS&&!retire;++i)if(q->rows[i].changed&&
                (read_ptr(q->scope.classes.data+(SIZE_T)runtime_melee_indices[i]*8)!=q->rows[i].klass.pointer||
                 !guest_camera_stamp_matches(base,&q->rows[i].klass,1)))retire=1;
        }
        if(!retire&&((q->prepared&(RUNTIME_MELEE_NORMAL_MASK|RUNTIME_MELEE_HEAVY_MASK|RUNTIME_MELEE_COVER_MASK))!=
                   (RUNTIME_MELEE_NORMAL_MASK|RUNTIME_MELEE_HEAVY_MASK|RUNTIME_MELEE_COVER_MASK))&&
           now-q->checked>=5000&&runtime_movement_idle(q->scope.pawn.pointer)&&
           session_handle_matches(q->source)&&read_ptr(q->source.pointer+0x198)==q->scope.info.pointer){
            q->checked=now;GameArray source_classes={0};
            if(identity_array(q->source.pointer+0x69c,&source_classes,512)&&source_classes.count>80){
                runtime_melee_normal_prepare(base,q);
                if(runtime_melee_normal_publish(base,engine,q,now)<0){runtime_guest_melee_retire(base,engine,now);return;}
                if(!(q->prepared&RUNTIME_MELEE_HEAVY_MASK))runtime_melee_heavy_source(base,q,&source_classes);
                runtime_melee_cover_source(base,q,&source_classes);
                if(q->prepared&RUNTIME_MELEE_HEAVY_MASK){
                    unsigned heavy_added=0;
                    for(unsigned i=6;i<8;++i){RuntimeMeleeRow *row=&q->rows[i];unsigned index=runtime_melee_indices[i];if(row->changed)continue;
                        if(!runtime_movement_arrays_live(base,&q->scope)||!runtime_heavy_plot_ready(base)||
                           !guest_camera_stamp_matches(base,&row->klass,1)||!guest_camera_stamp_matches(base,&row->cdo,1)||
                           !runtime_movement_borrow(q->scope.classes.data+(SIZE_T)index*8,
                               runtime_melee_cell(&q->scope.actions,index),0,row->klass.pointer,0)){retire=1;break;}
                        row->changed=1;++heavy_added;
                    }
                    if(!retire&&heavy_added)runtime_melee_note("native-heavy-melee-upgraded",heavy_added,now);
                }
                if(!retire&&runtime_melee_cover_publish(base,engine,q,now)<0)retire=1;
            }
        }
        if(retire)runtime_guest_melee_retire(base,engine,now);
        return;
    }
    if(!admitted||scope.classes.count<=80||(q->checked&&now-q->checked<5000)||!runtime_movement_idle(scope.pawn.pointer))return;
    q->checked=now;RuntimeGuestMelee candidate={0};candidate.scope=scope;candidate.prepared=RUNTIME_MELEE_NORMAL_MASK;
    candidate.manager=session_capture_handle(read_ptr(scope.pawn.pointer+0xc18));GameArray pc={0},pa={0},powers={0};
    if(!runtime_melee_arrays(base,&candidate,&pc,&pa,&powers)||!supported_image(base)||!guest_equipment_dispatch_pins(base))return;
    for(unsigned i=0;i<6;++i)if(read_ptr(scope.classes.data+(SIZE_T)(75+i)*8)||
        read_ptr(runtime_melee_cell(&scope.actions,75+i)))return;
    if(read_ptr(runtime_melee_cell(&pc,43))||read_ptr(runtime_melee_cell(&pa,43)))return;
    for(int i=0;i<powers.count;++i)if(narrative_probe_object(base,read_ptr(powers.data+(SIZE_T)i*8),"SFXPowerCustomAction_MeleePassivePower"))return;
    if(!runtime_melee_source(base,&candidate))return;
    runtime_melee_normal_prepare(base,&candidate);
    if((candidate.prepared&RUNTIME_MELEE_NORMAL_MASK)!=RUNTIME_MELEE_NORMAL_MASK)
        runtime_melee_note("waiting-optional-normal-melee-family",candidate.prepared&RUNTIME_MELEE_NORMAL_MASK,now);
    if(!(candidate.prepared&(RUNTIME_MELEE_NORMAL_MASK|RUNTIME_MELEE_HEAVY_MASK|RUNTIME_MELEE_COVER_MASK)))return;
    uintptr_t manager_event=runtime_melee_manager_event(base,candidate.manager.pointer);
    if(
       !runtime_melee_method(base,candidate.manager.pointer,"AddPower",&candidate.add,1)||
       !runtime_melee_method(base,candidate.manager.pointer,"RemovePower",&candidate.remove,0)||
       !manager_event)return;
    RuntimeMovementScope again={0};
    if(!runtime_movement_scope(base,engine,&again)||!runtime_movement_scope_same(&scope,&again)||
       !runtime_movement_idle(scope.pawn.pointer)||!runtime_melee_arrays(base,&candidate,&pc,&pa,&powers)||
       runtime_melee_manager_event(base,candidate.manager.pointer)!=manager_event)return;
    candidate.valid=1;candidate.created=1;*q=candidate;
    RuntimeMeleeAddParms parms={q->passive_class.pointer,0};q->dispatching=1;
    ((ProcessEvent)manager_event)((void*)q->manager.pointer,(void*)q->add.function.pointer,&parms,NULL);
    q->dispatching=0;
    if(!guest_camera_stamp_read(base,parms.result,&q->passive,1)||!runtime_melee_owns_passive(base,q,&pc,&pa,&powers)){
        runtime_guest_melee_retire(base,engine,now);return;
    }
    if(q->retiring||!runtime_movement_scope(base,engine,&again)||!runtime_movement_scope_same(&scope,&again)||!runtime_movement_idle(scope.pawn.pointer)){
        runtime_guest_melee_retire(base,engine,now);return;
    }
    if(runtime_melee_normal_publish(base,engine,q,now)<0){runtime_guest_melee_retire(base,engine,now);return;}
    unsigned applied=0;
    for(unsigned i=0;i<RUNTIME_MELEE_ROWS;++i){RuntimeMeleeRow *row=&q->rows[i];unsigned index=runtime_melee_indices[i];
        if(i<6||i>=8)continue; /* Normal/cover rows own their individual anim references. */
        if(!runtime_melee_row_prepared(q,i))continue;
        if(!runtime_melee_arrays(base,q,&pc,&pa,&powers)||!guest_camera_stamp_matches(base,&row->klass,1)||
           !guest_camera_stamp_matches(base,&row->cdo,1)||
           ((i==6||i==7)&&!runtime_heavy_plot_ready(base))||
           !runtime_movement_borrow(q->scope.classes.data+(SIZE_T)index*8,runtime_melee_cell(&q->scope.actions,index),0,row->klass.pointer,0)){
            runtime_guest_melee_retire(base,engine,now);return;
        }
        row->changed=1;++applied;
    }
    runtime_melee_note(q->prepared&RUNTIME_MELEE_HEAVY_MASK?"native-normal-heavy-melee-ready":"native-normal-melee-ready",applied,now);
    if(runtime_melee_cover_publish(base,engine,q,now)<0)runtime_guest_melee_retire(base,engine,now);
}
#ifdef ME3_RUNTIME_FIXTURE
static int runtime_melee_fixture_remove(RuntimeGuestMelee *q,GameArray *pc,GameArray *pa,GameArray *powers,int succeeds,unsigned *calls) {
    if(!q->created)return 1;
    ++*calls;
    if(succeeds){
        memset((void*)runtime_melee_cell(pc,43),0,8);memset((void*)runtime_melee_cell(pa,43),0,8);
        memset((void*)powers->data,0,8);
    }
    return runtime_melee_finish_remove(q,pc,pa,powers);
}
static int runtime_melee_cleanup_selftest(void) {
    _Alignas(8) uintptr_t pc_cells[44]={0},pa_cells[44]={0},power_cells[1]={0};
    GameArray pc={(uintptr_t)pc_cells,44,44},pa={(uintptr_t)pa_cells,44,44},powers={(uintptr_t)power_cells,1,1};
    RuntimeGuestMelee lease={0};lease.valid=lease.retiring=lease.created=lease.anim_registered=1;
    lease.passive_class.pointer=0x1230;lease.passive.pointer=0x4560;
    pc_cells[43]=lease.passive_class.pointer;pa_cells[43]=power_cells[0]=lease.passive.pointer;unsigned calls=0;
    /* RemovePower returning false without changing its native containers must
     * leave the passive and temporary-animation cleanup pending for retry. */
    if(runtime_melee_fixture_remove(&lease,&pc,&pa,&powers,0,&calls)||calls!=1||
       !lease.created||!lease.anim_registered||!lease.valid)return 0;
    /* Clearing only the two indexed cells does not remove Powers membership. */
    pc_cells[43]=pa_cells[43]=0;
    if(runtime_melee_finish_remove(&lease,&pc,&pa,&powers)||!lease.created)return 0;
    pc_cells[43]=lease.passive_class.pointer;pa_cells[43]=lease.passive.pointer;
    if(!runtime_melee_fixture_remove(&lease,&pc,&pa,&powers,1,&calls)||calls!=2||lease.created||!lease.anim_registered)return 0;
    /* Animation retirement may defer after successful RemovePower. Its next
     * retry must not issue the destructive class-based removal again. */
    if(runtime_melee_finish_unregister(&lease,2,2)||!lease.anim_registered||
       !runtime_melee_fixture_remove(&lease,&pc,&pa,&powers,1,&calls)||calls!=2||
       !runtime_melee_finish_unregister(&lease,2,1)||lease.anim_registered)return 0;
    /* A synchronous release during dispatch marks retirement but cannot clear
     * the live ledger. Tick also leaves the in-flight call untouched. */
    RuntimeGuestMelee saved=runtime_guest_melee;DWORD saved_thread=session_game_thread;
    runtime_guest_melee=lease;runtime_guest_melee.valid=runtime_guest_melee.created=runtime_guest_melee.anim_registered=1;
    runtime_guest_melee.retiring=0;runtime_guest_melee.dispatching=1;session_game_thread=GetCurrentThreadId();
    runtime_guest_melee_retire(0,0,0);runtime_guest_melee_tick(0,0,0);
    int in_flight_kept=runtime_guest_melee.valid&&runtime_guest_melee.retiring&&runtime_guest_melee.dispatching&&
        runtime_guest_melee.created&&runtime_guest_melee.anim_registered;
    runtime_guest_melee=saved;session_game_thread=saved_thread;if(!in_flight_kept)return 0;
    _Alignas(8) uintptr_t restored_classes[81]={0},restored_actions[81]={0};
    lease.scope.classes=(GameArray){(uintptr_t)restored_classes,81,81};
    lease.scope.actions=(GameArray){(uintptr_t)restored_actions,81,81};
    if(!runtime_melee_restored_slots_empty(&lease))return 0;
    restored_actions[80]=0x7890;if(runtime_melee_restored_slots_empty(&lease))return 0;
    restored_actions[80]=0;restored_classes[75]=0x7890;if(runtime_melee_restored_slots_empty(&lease))return 0;
    restored_classes[75]=0;restored_classes[62]=0x7890;if(runtime_melee_restored_slots_empty(&lease))return 0;
    restored_classes[62]=0;restored_actions[71]=0x7890;if(runtime_melee_restored_slots_empty(&lease))return 0;
    /* A native world/owner replacement drops only bookkeeping; do not inspect
     * or write the previous world's containers after the refreshed gate fails. */
    lease.valid=lease.created=lease.anim_registered=lease.dispatching=1;
    pc_cells[43]=0x7890;pa_cells[43]=power_cells[0]=0xabc0;
    if(runtime_melee_cleanup_resume(&lease,0)||lease.valid||lease.created||lease.anim_registered||lease.dispatching||
       pc_cells[43]!=0x7890||pa_cells[43]!=0xabc0||power_cells[0]!=0xabc0)return 0;
    return 1;
}
static int runtime_guest_melee_selftest(void) {
    if(runtime_melee_family_mask(7)!=7||runtime_melee_family_mask(56)!=56||runtime_melee_family_mask(63)!=63||
       runtime_melee_family_mask(3)||runtime_melee_family_mask(24)||runtime_melee_family_mask(31)!=7)return 0;
    RuntimeGuestMelee rows={0};rows.prepared=RUNTIME_MELEE_NORMAL_MASK|RUNTIME_MELEE_COVER_MASK;
    /* Optional covers must work without acquiring the plot-guarded heavy rows;
     * Hvy cover actions use normal PlayerMeleeBase.Power, unlike SoldierPunch. */
    if(runtime_melee_row_prepared(&rows,6)||!runtime_melee_row_prepared(&rows,8)||!runtime_melee_row_prepared(&rows,17)||
       runtime_melee_row_prepared(&rows,18)||runtime_melee_row_prepared(&rows,19)||
       runtime_melee_row_power_offset(8)!=0x17c||runtime_melee_row_power_offset(9)!=0x1fc||
       runtime_melee_row_power_offset(14)!=0x1fc||runtime_melee_row_power_offset(6)!=0x19c||
       runtime_melee_indices[8]!=62||runtime_melee_indices[17]!=71)return 0;
    rows.prepared&=~(UINT32_C(1)<<12);
    if(runtime_melee_row_prepared(&rows,12)||!runtime_melee_row_prepared(&rows,0)||!runtime_melee_row_prepared(&rows,17))return 0;
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
        sizeof(RuntimeMeleeAnimParms)==16&&offsetof(RuntimeMeleeRemoveParms,result)==8&&runtime_melee_cleanup_selftest()&&runtime_melee_manager_event_selftest();
}
#endif
#endif
