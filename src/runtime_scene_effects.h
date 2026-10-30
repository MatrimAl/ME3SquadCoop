#ifndef ME3_RUNTIME_SCENE_EFFECTS_H
#define ME3_RUNTIME_SCENE_EFFECTS_H
/* Guest-only cosmetics for the three native Liara EventTrack callbacks.
 * Include after runtime_scene_interp.h and runtime_scene_owner_binding.h.
 * The callback only queues evidence. Tick calls the existing effect manager;
 * no sequence output/input/Activated, scene clock or gameplay state is run. */
enum { RUNTIME_SCENE_FX_CHARGE=1,RUNTIME_SCENE_FX_START=2,RUNTIME_SCENE_FX_STOP=3,
       RUNTIME_SCENE_FX_QUEUE=8 };
#define RUNTIME_SCENE_FX_PARENT "BioD_ProMar_205ALiaraIntro.TheWorld.PersistentLevel.Main_Sequence.Liara_Introduction.ANIMCUTSCENE_promar_liara_intro."
#pragma pack(push,1)
typedef struct RuntimeSceneFxTarget {
    uint32_t id[4];float hit[3],refined_hit[3],refined_ray[3],normal[3],ray[3],params[3];
    uintptr_t instigator,hit_actor;uint32_t bone[2];uintptr_t material;uint32_t refined;
} RuntimeSceneFxTarget;
typedef struct RuntimeSceneFxPlay {uintptr_t effect;RuntimeSceneFxTarget target;uintptr_t owner;} RuntimeSceneFxPlay;
typedef struct RuntimeSceneFxStart {uintptr_t effect;RuntimeSceneFxTarget target;uintptr_t owner;uint32_t result[4];} RuntimeSceneFxStart;
typedef struct RuntimeSceneFxStop {uintptr_t effect;uint32_t id[4],cooldown;uintptr_t owner;} RuntimeSceneFxStop;
#pragma pack(pop)
_Static_assert(sizeof(RuntimeSceneFxTarget)==124,"RvrClientEffectTarget124");
_Static_assert(offsetof(RuntimeSceneFxTarget,instigator)==88,"target Instigator88");
_Static_assert(sizeof(RuntimeSceneFxPlay)==140,"PlayOnTarget140");
_Static_assert(sizeof(RuntimeSceneFxStart)==156,"StartOnTarget156");
_Static_assert(sizeof(RuntimeSceneFxStop)==36,"Stop36");

typedef struct RuntimeSceneFxEvent {
    RuntimeSceneScope scope;
    GuestCameraStamp action,track,actor,channel;
    uint64_t nonce;uint32_t name[2];float time;int key,kind;DWORD queued;
} RuntimeSceneFxEvent;
typedef struct RuntimeSceneFxLease {
    RuntimeSceneFxEvent event;
    GuestCameraStamp manager,gri,effect,target;
    uint32_t id[4];int used;
} RuntimeSceneFxLease;
typedef struct RuntimeSceneEffects {
    uintptr_t base,engine;GuestVitalsMethod play,start,stop;
    GuestCameraStamp target_type;
    RuntimeSceneFxEvent queue[RUNTIME_SCENE_FX_QUEUE],ledger;
    RuntimeSceneFxLease lease;
    unsigned count,seen,rows;DWORD checked;int prepared,busy,ledger_valid;
} RuntimeSceneEffects;
static RuntimeSceneEffects runtime_scene_effects;

static int runtime_scene_effects_kind(const char *name) {
    return !strcmp(name,"Liara_Biotic_Cast")?RUNTIME_SCENE_FX_CHARGE:
        !strcmp(name,"Singularity")?RUNTIME_SCENE_FX_START:
        !strcmp(name,"stop_singularity")?RUNTIME_SCENE_FX_STOP:0;
}
static const char *runtime_scene_effects_name(int kind) {
    return kind==1?"Liara_Biotic_Cast":kind==2?"Singularity":kind==3?"stop_singularity":"";
}
static int runtime_scene_effects_guid(const uint32_t id[4]) {return (id[0]|id[1]|id[2]|id[3])!=0;}
static int runtime_scene_effects_same(const GuestCameraStamp *a,const GuestCameraStamp *b) {
    return a->pointer==b->pointer&&a->klass==b->klass&&a->index==b->index&&a->class_index==b->class_index&&
        !memcmp(a->name,b->name,8)&&!memcmp(a->class_name,b->class_name,8);
}
static void runtime_scene_effects_note(const char *event,int kind) {
    RuntimeSceneEffects *s=&runtime_scene_effects;if(s->rows++>=128)return;
    char row[500];snprintf(row,sizeof row,
        "{\"runtime_scene_effects\":\"%s\",\"kind\":%d,\"queue\":%u,\"kismet_replayed\":false,\"gameplay_written\":false,\"native_remote_role_default_verified\":false,\"gameplay_verified\":false,\"monotonic_ms\":%lu}\r\n",
        event,kind,s->count,GetTickCount());line(row);
}
static int runtime_scene_effects_string(uintptr_t at,const char *expected) {
    GameString text={0},again={0};wchar_t value[96]={0};size_t n=strlen(expected);
    if(n>=96||!read_at(at,&text,sizeof text)||text.count!=(int)n+1||text.max<text.count||text.max>512||
       !text.data||!read_at((uintptr_t)text.data,value,(n+1)*2)||value[n]||
       !read_at(at,&again,sizeof again)||memcmp(&text,&again,sizeof text))return 0;
    for(size_t i=0;i<n;++i)if(value[i]!=(wchar_t)(unsigned char)expected[i])return 0;return 1;
}
/* Return-aware equivalent of guest_vitals_method. No native entry is patched. */
static int runtime_scene_effects_method(uintptr_t base,const char *name,uint32_t flags,unsigned size,
        unsigned ret,const GuestVitalsField *params,unsigned count,GuestVitalsMethod *out) {
    uintptr_t fn=function_named(base,"RvrClientEffectManager",name),field=read_ptr(fn+0x70),entry=read_ptr(fn+0xd8);
    uint32_t actual=0;uint16_t bytes=0,result=0,native=1;unsigned char n=0;GuestCameraStamp stamp={0};char owner[96];
    object_name(base,read_ptr(fn+0x40),owner,sizeof owner);
    if(strcmp(owner,"RvrClientEffectManager")||!narrative_probe_object(base,fn,"Function")||
       !guest_camera_stamp_read(base,fn,&stamp,1)||!read_at(fn+0xe0,&actual,4)||actual!=flags||
       !read_at(fn+0xe4,&native,2)||native||!read_at(fn+0xe6,&bytes,2)||bytes!=size||
       !read_at(fn+0xe8,&result,2)||result!=ret||!read_at(fn+0xea,&n,1)||n!=count||
       entry<base+0x1000||entry>=base+0x1300000||entry==base+0xf52b0)return 0;
    unsigned found=0,seen=0;uintptr_t prior[64]={0};
    for(;field&&seen<64;field=read_ptr(field+0x68)){uint64_t pf=0;char text[96];
        for(unsigned i=0;i<seen;++i)if(prior[i]==field)return 0;prior[seen++]=field;
        if(!is_live(base,field)||read_ptr(field+0x40)!=fn||!read_at(field+0x78,&pf,8))return 0;
        if(pf&0x80){int match=-1;object_name(base,field,text,sizeof text);
            for(unsigned i=0;i<count;++i)if(!strcmp(text,params[i].name))match=(int)i;
            if(match<0||(found&(1u<<match))||pf!=params[match].flags||
               runtime_native_hud_field(base,fn,text,params[match].kind,params[match].offset,params[match].size)!=field)return 0;
            found|=1u<<match;
        }
    }
    if(field||found!=((1u<<count)-1)||!guest_camera_stamp_matches(base,&stamp,1))return 0;
    *out=(GuestVitalsMethod){stamp,entry,flags};return 1;
}
static int runtime_scene_effects_target_layout(uintptr_t base,uintptr_t type) {
    static const GuestVitalsField fields[]={
        {"Id","StructProperty",0,16,0},{"HitLocation","StructProperty",16,12,1},
        {"RefinedHitLocation","StructProperty",28,12,0},{"RefinedRayDir","StructProperty",40,12,0},
        {"HitNormal","StructProperty",52,12,1},{"RayDir","StructProperty",64,12,1},
        {"SpawnValue","StructProperty",76,12,1},{"Instigator","ObjectProperty",88,8,0},
        {"HitActor","ObjectProperty",96,8,0},{"HitBone","NameProperty",104,8,1},
        {"HitMaterial","ObjectProperty",112,8,0},{"bHasRefinedHitLocation","BoolProperty",120,4,0}};
    GuestCameraStamp stamp={0};int size=0;unsigned count=0;uintptr_t field=read_ptr(type+0x70);
    if(!narrative_probe_object(base,type,"ScriptStruct")||!guest_camera_stamp_read(base,type,&stamp,1)||
       strcmp(stamp.full_name,"SFXGame.RvrClientEffectInterface.RvrClientEffectTarget")||
       !read_at(type+0x78,&size,4)||size!=124)return 0;
    for(;field&&count<13;field=read_ptr(field+0x68)){if(!is_live(base,field)||read_ptr(field+0x40)!=type)return 0;++count;}
    if(field||count!=12)return 0;
    for(unsigned i=0;i<12;++i){uint64_t flags=UINT64_MAX;
        uintptr_t p=runtime_native_hud_field(base,type,fields[i].name,fields[i].kind,fields[i].offset,fields[i].size);
        if(!p||!read_at(p+0x78,&flags,8)||flags!=fields[i].flags)return 0;
    }
    return runtime_native_hud_bool(base,type,"bHasRefinedHitLocation",120,1)&&guest_camera_stamp_matches(base,&stamp,1);
}
static int runtime_scene_effects_resolve(uintptr_t base) {
    RuntimeSceneEffects *s=&runtime_scene_effects;
    static const GuestVitalsField play[]={{"pEffect","ObjectProperty",0,8,0x80},
        {"oTarget","StructProperty",8,124,0x182},{"pOwner","ObjectProperty",132,8,0x90}};
    static const GuestVitalsField start[]={{"pEffect","ObjectProperty",0,8,0x80},
        {"oTarget","StructProperty",8,124,0x182},{"pOwner","ObjectProperty",132,8,0x90},
        {"ReturnValue","StructProperty",140,16,0x580}};
    static const GuestVitalsField stop[]={{"pEffect","ObjectProperty",0,8,0x80},{"Id","StructProperty",8,16,0x80},
        {"bAllowCooldown","BoolProperty",24,4,0x80},{"pOwner","ObjectProperty",28,8,0x90}};
    if(!supported_image(base)||!runtime_scene_effects_method(base,"PlayOnTarget",0x424401,140,65535,play,3,&s->play)||
       !runtime_scene_effects_method(base,"StartOnTarget",0x424401,156,140,start,4,&s->start)||
       !runtime_scene_effects_method(base,"Stop",0x24401,36,65535,stop,4,&s->stop))return 0;
    uintptr_t p=runtime_native_hud_field(base,s->play.object.pointer,"oTarget","StructProperty",8,124);
    uintptr_t q=runtime_native_hud_field(base,s->start.object.pointer,"oTarget","StructProperty",8,124),type=read_ptr(p+0xd0);
    if(!p||!q||read_ptr(q+0xd0)!=type||!runtime_scene_effects_target_layout(base,type)||
       !guest_camera_stamp_read(base,type,&s->target_type,1))return 0;
    return 1;
}
static int runtime_scene_effects_ready(void) {
    RuntimeSceneEffects *s=&runtime_scene_effects;
    /* Infrastructure only: the mainA assets may still be unloaded here. */
    return s->prepared&&guest_vitals_method_live(s->base,&s->play)&&guest_vitals_method_live(s->base,&s->start)&&
        guest_vitals_method_live(s->base,&s->stop)&&guest_camera_stamp_matches(s->base,&s->target_type,1);
}
static int runtime_scene_effects_host_scope(uintptr_t base,const RuntimeSceneScope *scope,DWORD now,uint64_t nonce) {
    return g_runtime_config.role==ME3_ROLE_GUEST&&runtime_scene_scope_live(base,scope)&&runtime_scene_view.valid&&
        runtime_scene_view.message.nonce==nonce&&runtime_scene_fresh(1,runtime_scene_view.message.active,
            runtime_scene_view.received_at,now)&&runtime_scene_scope_live(base,&runtime_scene_view.scope)&&
        runtime_scene_effects_same(&scope->pc,&runtime_scene_view.scope.pc)&&
        runtime_scene_effects_same(&scope->root,&runtime_scene_view.scope.root)&&
        runtime_scene_effects_same(&scope->connection,&runtime_scene_view.scope.connection)&&
        runtime_scene_effects_same(&scope->channel,&runtime_scene_view.scope.channel);
}
static int runtime_scene_effects_channel(uintptr_t base,const RuntimeSceneScope *scope,uintptr_t action,
        GuestCameraStamp *actor_out,GuestCameraStamp *channel_out,int playing) {
    GameArray channels={0},again={0};int found=0;GuestCameraStamp actor={0},channel={0};
    if(!scene_readiness_array(scope->connection.pointer+0x5f90,&channels,2048))return 0;
    for(int i=0;i<channels.count;++i){uintptr_t ch=read_ptr(channels.data+(SIZE_T)i*8),a=read_ptr(ch+0xa0);uint32_t state=0;
        if(!a||read_ptr(a+0x270)!=action)continue;NetworkActorIdentityEntry entry={0};
        if(++found!=1||!guest_camera_actor_live(base,a)||!is_kind_of(base,a,"MatineeActor")||
           read_ptr(a+0x198)!=scope->world.pointer||!read_at(a+0x284,&state,4)||(playing&&!(state&1))||
           !network_actor_channel_valid(base,a,ch,scope->driver.pointer,&entry)||entry.connection!=scope->connection.pointer||
           !guest_camera_stamp_read(base,a,&actor,1)||!guest_camera_stamp_read(base,ch,&channel,1))return 0;
    }
    if(found!=1||!read_at(scope->connection.pointer+0x5f90,&again,sizeof again)||memcmp(&channels,&again,sizeof channels))return 0;
    *actor_out=actor;*channel_out=channel;return 1;
}
static int runtime_scene_effects_key(uintptr_t base,uintptr_t action,uintptr_t track,int key,
        GuestCameraStamp *track_stamp,uint32_t name[2],float *time,int *kind) {
    GameArray keys={0},again={0},tracks={0},same={0};uintptr_t type=0,group=read_ptr(track+0x40),data=read_ptr(group+0x40);
    uintptr_t owner=runtime_story_graph_class(base,track,"InterpTrackEvent");
    uintptr_t array=runtime_native_hud_field(base,owner,"EventTrack","ArrayProperty",0x90,16);
    if(!track||!narrative_probe_object(base,track,"InterpTrackEvent")||!guest_camera_stamp_read(base,track,track_stamp,1)||
       (strcmp(track_stamp->full_name,RUNTIME_SCENE_FX_PARENT "InterpData_2.InterpGroup_13.InterpTrackEvent_0")&&
        strcmp(track_stamp->full_name,RUNTIME_SCENE_FX_PARENT "InterpData_2.InterpGroup_22.InterpTrackEvent_0"))||
       !array||!runtime_story_graph_struct(base,array,"EventTrackKey",12,&type)||
       !runtime_native_hud_field(base,type,"EventName","NameProperty",0,8)||
       !runtime_native_hud_field(base,type,"Time","FloatProperty",8,4)||
       !runtime_power_property(base,group,"InterpGroup","InterpTracks","ArrayProperty",0x68,16)||
       !narrative_probe_object(base,data,"InterpData")||read_ptr(action+0x154)!=data||
       !scene_readiness_array(group+0x68,&tracks,128)||!scene_readiness_array(track+0x90,&keys,128)||
       key<0||key>=keys.count)return 0;
    int found=0;for(int i=0;i<tracks.count;++i)if(read_ptr(tracks.data+(SIZE_T)i*8)==track)++found;
    uintptr_t at=keys.data+(SIZE_T)key*12;char text[96];
    if(found!=1||!read_at(at,name,8)||!name[0]||name[1]||!read_at(at+8,time,4)||!isfinite(*time)||*time<0||*time>3600)return 0;
    name_of(base,name[0],text,sizeof text);*kind=runtime_scene_effects_kind(text);
    return *kind&&read_at(track+0x90,&again,sizeof again)&&!memcmp(&keys,&again,sizeof keys)&&
        read_at(group+0x68,&same,sizeof same)&&!memcmp(&tracks,&same,sizeof tracks)&&
        guest_camera_stamp_matches(base,track_stamp,1)&&read_ptr(action+0x154)==data;
}
static int runtime_scene_effects_event_live(const RuntimeSceneFxEvent *e,DWORD now,int fresh) {
    RuntimeSceneEffects *s=&runtime_scene_effects;GuestCameraStamp a={0},channel={0},track={0},action={0};
    uint32_t name[2]={0};float time=0;int kind=0;uintptr_t level=0,root=0;
    return (!fresh||now-e->queued<=500)&&runtime_scene_effects_host_scope(s->base,&e->scope,now,e->nonce)&&
        guest_camera_stamp_matches(s->base,&e->action,1)&&guest_camera_stamp_matches(s->base,&e->track,1)&&
        !strcmp(e->action.full_name,RUNTIME_SCENE_FX_PARENT "SeqAct_Interp_2")&&
        runtime_scene_interp_action(s->base,e->action.pointer,e->scope.world.pointer,&action,&level,&root)&&
        root==e->scope.root.pointer&&runtime_scene_effects_channel(s->base,&e->scope,e->action.pointer,&a,&channel,1)&&
        runtime_scene_effects_same(&a,&e->actor)&&runtime_scene_effects_same(&channel,&e->channel)&&
        runtime_scene_effects_key(s->base,e->action.pointer,e->track.pointer,e->key,&track,name,&time,&kind)&&
        !memcmp(name,e->name,8)&&time==e->time&&kind==e->kind&&runtime_scene_effects_same(&track,&e->track);
}
static int runtime_scene_effects_instance(const RuntimeSceneFxEvent *a,const RuntimeSceneFxEvent *b) {
    return a->nonce==b->nonce&&runtime_scene_effects_same(&a->action,&b->action)&&
        runtime_scene_effects_same(&a->actor,&b->actor)&&runtime_scene_effects_same(&a->channel,&b->channel)&&
        runtime_scene_effects_same(&a->scope.root,&b->scope.root)&&
        runtime_scene_effects_same(&a->scope.connection,&b->scope.connection);
}
static int runtime_scene_effects_order(unsigned seen,int kind) {
    return kind>=1&&kind<=3&&!(seen&(1u<<kind))&&
        !(kind==1&&(seen&((1u<<2)|(1u<<3))))&&!(kind==2&&(seen&(1u<<3)));
}
static int runtime_scene_effects_enqueue(RuntimeSceneEffects *s,const RuntimeSceneFxEvent *event) {
    if(s->ledger_valid&&!runtime_scene_effects_instance(&s->ledger,event)){
        s->count=0;s->seen=0;s->ledger_valid=0;
    }
    if(!runtime_scene_effects_order(s->seen,event->kind)||s->count>=RUNTIME_SCENE_FX_QUEUE)return 0;
    if(!s->ledger_valid){s->ledger=*event;s->ledger_valid=1;}
    s->queue[s->count++]=*event;s->seen|=1u<<event->kind;return 1;
}
static void runtime_scene_effects_event(uintptr_t action,uintptr_t track,int key) {
    DWORD error=GetLastError();RuntimeSceneEffects *s=&runtime_scene_effects;RuntimeSceneFxEvent e={0};
    if(!track||s->busy||GetCurrentThreadId()!=session_game_thread||g_runtime_config.role!=ME3_ROLE_GUEST||
       !runtime_scene_effects_ready()||!runtime_scene_actor_observer_scope(s->base,s->engine,&e.scope))goto done;
    e.nonce=runtime_scene_view.message.nonce;e.queued=GetTickCount();e.key=key;
    if(!runtime_scene_effects_host_scope(s->base,&e.scope,e.queued,e.nonce)||
       !guest_camera_stamp_read(s->base,action,&e.action,1)||
       strcmp(e.action.full_name,RUNTIME_SCENE_FX_PARENT "SeqAct_Interp_2")||
       !runtime_scene_effects_channel(s->base,&e.scope,action,&e.actor,&e.channel,1)||
       !runtime_scene_effects_key(s->base,action,track,key,&e.track,e.name,&e.time,&e.kind)||
       !runtime_scene_effects_event_live(&e,e.queued,1))goto done;
    runtime_scene_effects_note(runtime_scene_effects_enqueue(s,&e)?"natural-event-queued":"duplicate-or-order-refused",e.kind);
 done:SetLastError(error);
}
static int runtime_scene_effects_module(const char *name) {
    static const char *classes[]={"RvrCEffectModuleEmitter","RvrCEffectModuleLensFlare","RvrCEffectModuleSound",
        "RvrCEffectModuleLocation","RvrCEffectModuleLifetime","RvrCEffectModuleParameters","RvrCEffectModuleEffectsMaterial"};
    for(unsigned i=0;i<7;++i)if(!strcmp(name,classes[i]))return 1;return 0;
}
static int runtime_scene_effects_cosmetic(uintptr_t base,uintptr_t effect,int kind) {
    GameArray modules={0},again={0};uintptr_t values[16]={0};GuestCameraStamp stamp={0};
    const char *path=kind==1?"BioVFX_Env_ProMar.VCFX.Cine_Liara_Charge_VCFX":"BioVFX_B_Singularity.VCFX.Singularity_Imp_Cine_VCFX";
    if(!narrative_probe_object(base,effect,"RvrClientEffect")||!guest_camera_stamp_read(base,effect,&stamp,1)||
       strcmp(stamp.full_name,path)||!runtime_power_property(base,effect,"RvrClientEffect","m_lstModules","ArrayProperty",0x68,16)||
       !scene_readiness_array(effect+0x68,&modules,16)||modules.count<1||
       !read_at(modules.data,values,(SIZE_T)modules.count*8))return 0;
    for(int i=0;i<modules.count;++i){char klass[96];
        if(!narrative_probe_object(base,values[i],"RvrClientEffectModule")||read_ptr(values[i]+0x40)!=effect)return 0;
        object_name(base,read_ptr(values[i]+0x50),klass,sizeof klass);if(!runtime_scene_effects_module(klass))return 0;
        for(int j=0;j<i;++j)if(values[j]==values[i])return 0;
    }
    if(!read_at(effect+0x68,&again,sizeof again)||memcmp(&modules,&again,sizeof modules))return 0;
    for(int i=0;i<modules.count;++i)if(read_ptr(modules.data+(SIZE_T)i*8)!=values[i])return 0;
    return guest_camera_stamp_matches(base,&stamp,1);
}
static uintptr_t runtime_scene_effects_node(uintptr_t base,const RuntimeSceneFxEvent *e) {
    GameArray outputs={0},again={0};uintptr_t result=0;int found=0;
    if(!runtime_story_graph_output_layout(base,e->action.pointer)||
       !scene_readiness_array(e->action.pointer+0x98,&outputs,64))return 0;
    for(int i=0;i<outputs.count;++i){uintptr_t at=outputs.data+(SIZE_T)i*0x34;
        if(!runtime_scene_effects_string(at+0x10,runtime_scene_effects_name(e->kind)))continue;
        GameArray links={0},same={0};int input=-1;GuestCameraStamp stamp={0};
        if(++found!=1||!scene_readiness_array(at,&links,4)||links.count!=1||!read_at(links.data+8,&input,4)||
           input!=(e->kind==3?1:0)||!read_at(links.data,&result,8)||
           !guest_camera_stamp_read(base,result,&stamp,1)||read_ptr(result+0x40)!=read_ptr(e->action.pointer+0x40)||
           strcmp(stamp.full_name,e->kind==1?RUNTIME_SCENE_FX_PARENT "RvrSeqAct_SpawnClientEffect_0":
                RUNTIME_SCENE_FX_PARENT "RvrSeqAct_StoppableClientEffect_0")||
           !is_kind_of(base,result,e->kind==1?"RvrSeqAct_SpawnClientEffect":"RvrSeqAct_StoppableClientEffect")||
           !read_at(at,&same,sizeof same)||memcmp(&links,&same,sizeof links))return 0;
    }
    return found==1&&read_at(e->action.pointer+0x98,&again,sizeof again)&&!memcmp(&outputs,&again,sizeof outputs)?result:0;
}
static uintptr_t runtime_scene_effects_target(uintptr_t base,const RuntimeSceneFxEvent *e,uintptr_t node) {
    GameArray links={0},again={0},variables={0},same={0};uintptr_t variable=0,type=0;
    uintptr_t owner=runtime_story_graph_class(base,node,"SequenceOp");
    uintptr_t prop=runtime_native_hud_field(base,owner,"VariableLinks","ArrayProperty",0xa8,16);
    if(!runtime_story_graph_variable_layout(base,node)||!runtime_story_graph_struct(base,prop,"SeqVarLink",0x4c,&type)||
       !runtime_native_hud_field(base,type,"PropertyName","NameProperty",0x30,8)||
       !scene_readiness_array(node+0xa8,&links,4)||links.count!=1||
       !runtime_scene_effects_string(links.data+0x10,"Target"))return 0;
    uint32_t name[2]={0};char text[96];
    if(!read_at(links.data+0x30,name,8)||!name[0]||name[1])return 0;name_of(base,name[0],text,sizeof text);
    if(strcmp(text,"Targets")||!scene_readiness_array(links.data,&variables,4)||variables.count!=1||
       !read_at(variables.data,&variable,8)||read_ptr(variable+0x40)!=read_ptr(node+0x40)||
       !read_at(node+0xa8,&again,sizeof again)||memcmp(&links,&again,sizeof links)||
       !read_at(links.data,&same,sizeof same)||memcmp(&variables,&same,sizeof variables))return 0;
    GuestCameraStamp stamp={0};if(!guest_camera_stamp_read(base,variable,&stamp,1))return 0;
    if(e->kind==1){NetworkActorScope world={0};uint32_t tag[2]={0};
        if(strcmp(stamp.full_name,RUNTIME_SCENE_FX_PARENT "BioSeqVar_ObjectFindByTag_5")||
           !narrative_probe_object(base,variable,"BioSeqVar_ObjectFindByTag")||
           !runtime_power_property(base,variable,"BioSeqVar_ObjectFindByTag","m_sObjectTagToFind","NameProperty",0xb8,8)||
           !read_at(variable+0xb8,tag,8)||!tag[0]||tag[1])return 0;
        name_of(base,tag[0],text,sizeof text);if(strcmp(text,"hench_liara")||
           !network_actor_read_scope(base,e->scope.pc.pointer,&world)||world.world!=e->scope.root.pointer||
           world.info!=e->scope.world.pointer)return 0;
        return runtime_scene_owner_resolve(base,e->scope.world.pointer,&world);
    }
    uintptr_t target=0;GuestCameraStamp actor={0};
    if(strcmp(stamp.full_name,RUNTIME_SCENE_FX_PARENT "SeqVar_Object_8")||!narrative_probe_object(base,variable,"SeqVar_Object")||
       !runtime_power_property(base,variable,"SeqVar_Object","ObjValue","ObjectProperty",0xac,8)||
       !read_at(variable+0xac,&target,8)||!guest_camera_actor_live(base,target)||!is_kind_of(base,target,"Note")||
       read_ptr(target+0x198)!=e->scope.world.pointer||!guest_camera_stamp_read(base,target,&actor,1)||
       strcmp(actor.full_name,"BioD_ProMar_205ALiaraIntro.TheWorld.PersistentLevel.Note_0"))return 0;
    return target;
}
static void runtime_scene_effects_target_init(RuntimeSceneFxTarget *out,uintptr_t actor,const float location[3],const float params[3]) {
    memset(out,0,sizeof *out);out->refined_ray[0]=-1;out->normal[0]=1;out->ray[0]=-1;
    memcpy(out->hit,location,12);memcpy(out->params,params,12);out->instigator=actor;
}
static int runtime_scene_effects_payload(const RuntimeSceneFxEvent *e,GuestCameraStamp *manager,
        GuestCameraStamp *gri,GuestCameraStamp *effect,GuestCameraStamp *target,RuntimeSceneFxTarget *out) {
    RuntimeSceneEffects *s=&runtime_scene_effects;uintptr_t base=s->base,node=runtime_scene_effects_node(base,e),asset=0;
    float params[3]={0},location[3]={0};if(!node)return 0;
    if(e->kind==1){GameArray effects={0},again={0};
        if(!runtime_power_property(base,node,"RvrSeqAct_SpawnClientEffect","m_lstEffects","ArrayProperty",0xf8,16)||
           !scene_readiness_array(node+0xf8,&effects,4)||effects.count!=1||!read_at(effects.data,&asset,8)||
           !read_at(node+0xf8,&again,sizeof again)||memcmp(&effects,&again,sizeof effects))return 0;
    }else {uint32_t cooldown=0;
        if(!runtime_power_property(base,node,"RvrSeqAct_StoppableClientEffect","m_pEffect","ObjectProperty",0x114,8)||
           !runtime_power_property(base,node,"RvrSeqAct_StoppableClientEffect","m_bAllowCooldown","BoolProperty",0x11c,4)||
           !runtime_native_hud_bool(base,read_ptr(node+0x50),"m_bAllowCooldown",0x11c,1)||
           !read_at(node+0x114,&asset,8)||!read_at(node+0x11c,&cooldown,4)||!(cooldown&1))return 0;
    }
    uintptr_t actor=runtime_scene_effects_target(base,e,node),replication=read_ptr(e->scope.world.pointer+0x6a8),effects=read_ptr(replication+0x350);
    if(!actor||!runtime_scene_effects_cosmetic(base,asset,e->kind)||
       !runtime_power_property(base,node,e->kind==1?"RvrSeqAct_SpawnClientEffect":"RvrSeqAct_StoppableClientEffect",
            "m_vSpawnParameters","StructProperty",0x108,12)||!read_at(node+0x108,params,12)||
       !runtime_power_property(base,actor,"Actor","Location","StructProperty",0x108,12)||!read_at(actor+0x108,location,12)||
       !guest_camera_actor_live(base,replication)||!is_kind_of(base,replication,"SFXGRI")||
       read_ptr(replication+0x198)!=e->scope.world.pointer||!narrative_probe_object(base,effects,"RvrClientEffectManager")||
       !runtime_power_property(base,e->scope.world.pointer,"WorldInfo","GRI","ObjectProperty",0x6a8,8)||
       !runtime_power_property(base,replication,"SFXGRI","m_pClientEffectManager","ObjectProperty",0x350,8))return 0;
    for(unsigned i=0;i<3;++i)if(!isfinite(params[i])||!isfinite(location[i])||fabsf(location[i])>1e8f)return 0;
    if(!guest_camera_stamp_read(base,effects,manager,1)||!guest_camera_stamp_read(base,replication,gri,1)||
       !guest_camera_stamp_read(base,asset,effect,1)||!guest_camera_stamp_read(base,actor,target,1))return 0;
    runtime_scene_effects_target_init(out,actor,location,params);return 1;
}
static int runtime_scene_effects_lease_live(const RuntimeSceneFxLease *l) {
    RuntimeSceneEffects *s=&runtime_scene_effects;uintptr_t base=s->base;
    return l->used&&runtime_scene_effects_guid(l->id)&&guest_camera_stamp_matches(base,&l->event.scope.root,0)&&
        guest_camera_stamp_matches(base,&l->event.scope.world,0)&&read_ptr(base+0x18b6638)==l->event.scope.root.pointer&&
        guest_camera_stamp_matches(base,&l->manager,1)&&guest_camera_stamp_matches(base,&l->gri,1)&&
        guest_camera_stamp_matches(base,&l->effect,1)&&read_ptr(l->event.scope.world.pointer+0x6a8)==l->gri.pointer&&
        read_ptr(l->gri.pointer+0x350)==l->manager.pointer&&runtime_scene_effects_cosmetic(base,l->effect.pointer,2);
}
static void runtime_scene_effects_invoke(ProcessEvent invoke,uintptr_t manager,uintptr_t method,void *parms) {
    DWORD error=GetLastError();invoke((void*)manager,(void*)method,parms,NULL);SetLastError(error);
}
static void runtime_scene_effects_stop_lease(void) {
    RuntimeSceneEffects *s=&runtime_scene_effects;RuntimeSceneFxStop parms={0};
    parms.effect=s->lease.effect.pointer;memcpy(parms.id,s->lease.id,16);parms.cooldown=1;
    s->busy=1;runtime_scene_effects_invoke((ProcessEvent)(s->base+0xf4f80),s->lease.manager.pointer,s->stop.object.pointer,&parms);s->busy=0;
    memset(&s->lease,0,sizeof s->lease);runtime_scene_effects_note("native-local-guid-stopped",3);
}
static int runtime_scene_effects_ended(const RuntimeSceneFxEvent *e) {
    RuntimeSceneEffects *s=&runtime_scene_effects;uint32_t state=1;
    /* The channel UObject may remain allocated and report playing after a
     * disconnect. End this previously authenticated scope independently of
     * that object's destruction. The caller validates the same local
     * world/manager/effect lease before stopping only our returned GUID. */
    if(!runtime_scene_scope_live(s->base,&e->scope))return 1;
    if(runtime_scene_view.valid&&runtime_scene_scope_live(s->base,&runtime_scene_view.scope)&&
       runtime_scene_effects_same(&runtime_scene_view.scope.root,&e->scope.root)&&
       (runtime_scene_view.message.nonce!=e->nonce||!runtime_scene_view.message.active))return 1;
    if(!guest_camera_stamp_matches(s->base,&e->actor,1)||!guest_camera_stamp_matches(s->base,&e->channel,1))return 1;
    return read_at(e->actor.pointer+0x284,&state,4)&&!(state&1);
}
static void runtime_scene_effects_pop(RuntimeSceneEffects *s) {
    if(s->count){--s->count;if(s->count)memmove(s->queue,s->queue+1,s->count*sizeof s->queue[0]);}
}
static void runtime_scene_effects_tick(uintptr_t base,uintptr_t engine,DWORD now) {
    DWORD error=GetLastError();RuntimeSceneEffects *s=&runtime_scene_effects;s->base=base;s->engine=engine;
    runtime_scene_interp_effects_ready=runtime_scene_effects_ready;
    runtime_scene_interp_effect_event=runtime_scene_effects_event;
    if(GetCurrentThreadId()!=session_game_thread||g_runtime_config.role!=ME3_ROLE_GUEST||s->busy)goto done;
    if(!runtime_scene_effects_ready()&&(!s->checked||now-s->checked>=1000)){
        s->checked=now;s->prepared=runtime_scene_effects_resolve(base);
        runtime_scene_effects_note(s->prepared?"native-manager-abi-prepared":"native-manager-abi-refused",0);
    }
    if(!runtime_scene_effects_ready())goto done;
    if(s->lease.used&&!runtime_scene_effects_lease_live(&s->lease)){
        memset(&s->lease,0,sizeof s->lease);runtime_scene_effects_note("expired-guid-identity-dropped",3);
    }
    if(s->lease.used&&(runtime_scene_effects_ended(&s->lease.event)||
       (s->ledger_valid&&!runtime_scene_effects_instance(&s->lease.event,&s->ledger)))){
        runtime_scene_effects_stop_lease();goto done; /* One native call this Tick. */
    }
    if(s->ledger_valid&&runtime_scene_effects_ended(&s->ledger)){
        s->ledger_valid=0;s->seen=0;s->count=0;runtime_scene_effects_note("native-scene-ledger-retired",0);
    }
    if(!s->count)goto done;
    RuntimeSceneFxEvent e=s->queue[0];
    if(!runtime_scene_effects_event_live(&e,now,1)){
        runtime_scene_effects_pop(s);runtime_scene_effects_note("stale-or-identity-event-refused",e.kind);goto done;
    }
    GuestCameraStamp manager={0},gri={0},effect={0},target={0};RuntimeSceneFxTarget value={0};
    if(!runtime_scene_effects_payload(&e,&manager,&gri,&effect,&target,&value)){
        /* FindByTag's bounded resolver may need another ordinary Tick. */
        runtime_scene_effects_note("authored-cosmetic-payload-not-ready",e.kind);goto done;
    }
    if(!runtime_scene_effects_event_live(&e,GetTickCount(),1)||!runtime_scene_effects_ready())goto done;
    if(e.kind==RUNTIME_SCENE_FX_STOP){
        runtime_scene_effects_pop(s);
        if(s->lease.used&&runtime_scene_effects_instance(&s->lease.event,&e)&&
           runtime_scene_effects_same(&s->lease.manager,&manager)&&runtime_scene_effects_same(&s->lease.effect,&effect))
            runtime_scene_effects_stop_lease();
        else runtime_scene_effects_note("stop-without-own-guid-refused",3);
        goto done;
    }
    if(e.kind==RUNTIME_SCENE_FX_START&&s->lease.used){runtime_scene_effects_stop_lease();goto done;}
    s->busy=1;
    if(e.kind==RUNTIME_SCENE_FX_CHARGE){RuntimeSceneFxPlay parms={effect.pointer,value,target.pointer};
        runtime_scene_effects_invoke((ProcessEvent)(base+0xf4f80),manager.pointer,s->play.object.pointer,&parms);
    }else {RuntimeSceneFxStart parms={0};parms.effect=effect.pointer;parms.target=value;parms.owner=target.pointer;
        runtime_scene_effects_invoke((ProcessEvent)(base+0xf4f80),manager.pointer,s->start.object.pointer,&parms);
        if(runtime_scene_effects_guid(parms.result)){
            s->lease=(RuntimeSceneFxLease){0};s->lease.event=e;s->lease.manager=manager;s->lease.gri=gri;
            s->lease.effect=effect;s->lease.target=target;memcpy(s->lease.id,parms.result,16);s->lease.used=1;
        }else runtime_scene_effects_note("native-start-returned-zero-guid",2);
    }
    s->busy=0;runtime_scene_effects_pop(s);runtime_scene_effects_note("native-cosmetic-dispatched",e.kind);
 done:SetLastError(error);
}
#ifdef ME3_RUNTIME_FIXTURE
static int runtime_scene_fx_fixture_calls,runtime_scene_fx_fixture_bad;
static void runtime_scene_fx_fixture_call(void *self,void *method,void *parms,void *result) {
    RuntimeSceneFxStart *p=(RuntimeSceneFxStart*)parms;
    if(self!=(void*)16||method!=(void*)24||result||GetLastError()!=37||p->owner!=48||p->target.instigator!=48)
        runtime_scene_fx_fixture_bad=1;
    p->result[0]=9;++runtime_scene_fx_fixture_calls;SetLastError(53);
}
static int runtime_scene_effects_selftest(void) {
    DWORD error=GetLastError();int ok=1;RuntimeSceneEffects state={0};RuntimeSceneFxEvent event={0};
    float point[3]={11,12,13},params[3]={21,22,23};RuntimeSceneFxStart call={0};
    runtime_scene_effects_target_init(&call.target,48,point,params);call.owner=48;
    ok=ok&&sizeof call==156&&offsetof(RuntimeSceneFxStart,result)==140&&
        offsetof(RuntimeSceneFxStop,owner)==28&&call.target.refined_ray[0]==-1&&
        call.target.normal[0]==1&&call.target.ray[0]==-1&&!call.target.refined&&
        !call.target.hit_actor&&!call.target.material&&!runtime_scene_effects_guid(call.target.id)&&
        !memcmp(call.target.hit,point,12)&&!memcmp(call.target.params,params,12)&&
        runtime_scene_effects_kind("Liara_Biotic_Cast")==1&&runtime_scene_effects_kind("Singularity")==2&&
        runtime_scene_effects_kind("stop_singularity")==3&&!runtime_scene_effects_kind("Completed")&&
        runtime_scene_effects_module("RvrCEffectModuleSound")&&!runtime_scene_effects_module("RvrCEffectModuleSpawnActor");
    event.kind=1;event.key=0;event.nonce=1;event.action.pointer=16;event.actor.pointer=32;event.channel.pointer=48;
    ok=ok&&runtime_scene_effects_enqueue(&state,&event)&&state.count==1&&!runtime_scene_effects_enqueue(&state,&event);
    event.track.pointer=999;ok=ok&&!runtime_scene_effects_enqueue(&state,&event); /* Second native track cannot double it. */
    event.kind=2;event.key=1;ok=ok&&runtime_scene_effects_enqueue(&state,&event)&&state.count==2;
    event.kind=3;event.key=2;ok=ok&&runtime_scene_effects_enqueue(&state,&event)&&state.count==3;
    runtime_scene_effects_pop(&state);ok=ok&&state.count==2&&state.queue[0].kind==2&&state.queue[1].kind==3;
    event.kind=1;ok=ok&&!runtime_scene_effects_enqueue(&state,&event);
    event.actor.pointer=33;ok=ok&&runtime_scene_effects_enqueue(&state,&event)&&state.count==1&&state.seen==(1u<<1);
    state.count=8;event.kind=2;ok=ok&&!runtime_scene_effects_enqueue(&state,&event)&&state.count==8;
    runtime_scene_fx_fixture_calls=runtime_scene_fx_fixture_bad=0;SetLastError(37);
    runtime_scene_effects_invoke(runtime_scene_fx_fixture_call,16,24,&call);
    ok=ok&&runtime_scene_fx_fixture_calls==1&&!runtime_scene_fx_fixture_bad&&call.result[0]==9&&GetLastError()==37;
    ok=ok&&runtime_scene_effects_order(0,2)&&!runtime_scene_effects_order(1u<<3,2)&&
        !runtime_scene_effects_order(0,0)&&!runtime_scene_effects_order(0,4);
    SetLastError(error);return ok;
}
#endif
#endif
