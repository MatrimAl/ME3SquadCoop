#ifndef ME3_RUNTIME_SCENE_VOICE_PLAYBACK_H
#define ME3_RUNTIME_SCENE_VOICE_PLAYBACK_H
#include "runtime_scene_voice_pins.h"
#include "scene_voice_protocol.h"
/* A fresh, isolated native FaceOnlyVO node. No authored story node or full
 * conversation is activated. The owner authenticates every request. */
typedef struct RuntimeSceneVoiceRequest {
    RuntimeSceneScope scope;
    GuestCameraStamp action,matinee,matinee_channel,sequence,conversation,actor,host_shepard;
    uint64_t scene_nonce,line_generation;int32_t strref;unsigned flags;
} RuntimeSceneVoiceRequest;
typedef struct RuntimeSceneVoicePlayback {
    uintptr_t base,engine;RuntimeSceneInterpHook activated,player;
    GuestCameraStamp klass,node;RuntimeSceneVoiceRequest current,pending;
    uintptr_t activation_node,activation_player;
    DWORD checked;unsigned rows;int prepared,used,queued,busy,in_activation,rejected;
} RuntimeSceneVoicePlayback;
static RuntimeSceneVoicePlayback runtime_scene_voice_playback;
static int (*runtime_scene_voice_playback_request_live)(const RuntimeSceneVoiceRequest*,int);
static uintptr_t (*runtime_scene_voice_playback_authored_player)(uintptr_t);
static int runtime_scene_voice_playback_ready(void);
static void runtime_scene_voice_playback_activated(void*);
static uintptr_t runtime_scene_voice_playback_player(void*);
static void runtime_scene_voice_playback_note(const char *event) {
    RuntimeSceneVoicePlayback *s=&runtime_scene_voice_playback;if(s->rows++>=128)return;
    char row[400];snprintf(row,sizeof row,"{\"runtime_scene_voice_playback\":\"%s\",\"generation\":%llu,\"full_conversation_created\":false,\"story_outputs_linked\":false,\"gameplay_verified\":false,\"monotonic_ms\":%lu}\r\n",event,(unsigned long long)s->current.line_generation,GetTickCount());line(row);
}
static uintptr_t runtime_scene_voice_field(uintptr_t base,uintptr_t klass,const char *owner,const char *name,const char *kind,int off,int size) {
    char text[96];for(unsigned n=0;klass&&n<32;++n){
        if(!narrative_probe_object(base,klass,"Class"))return 0;object_name(base,klass,text,sizeof text);
        if(!strcmp(text,owner))return runtime_native_hud_field(base,klass,name,kind,off,size);
        klass=read_ptr(klass+0x60);
    }return 0;
}
static int runtime_scene_voice_playback_layout(uintptr_t base,uintptr_t klass) {
    static const struct {const char *name,*kind;int off,size;} f[]={
        {"m_pConversation","ObjectProperty",0x188,8},{"m_nmSpeakerTag","NameProperty",0x190,8},
        {"m_pActor","ObjectProperty",0x198,8},{"m_srLineStrRef","StringRefProperty",0x1b0,4},
        {"m_srActorNameOverride","StringRefProperty",0x1b4,4}};
    for(unsigned i=0;i<sizeof f/sizeof *f;++i)
        if(!runtime_scene_voice_field(base,klass,"BioSeqAct_FaceOnlyVO",f[i].name,f[i].kind,f[i].off,f[i].size))return 0;
    return runtime_native_hud_bool(base,klass,"m_bAllowInConversation",0x1c4,0x800)&&
        runtime_native_hud_bool(base,klass,"m_bErrorInActivation",0x1c4,0x80)&&
        runtime_native_hud_bool(base,klass,"m_bFemalePlayer",0x1c4,0x10000);
}
/* All four default outputs must exist: native Activated clears them by
 * index. Their edge arrays and direct targets must remain empty. */
static int runtime_scene_voice_isolated(uintptr_t base,uintptr_t node) {
    GameArray out={0},in={0},vars={0},events={0},same={0};uint32_t flags=0;
    if(!narrative_probe_object(base,node,"BioSeqAct_FaceOnlyVO")||read_ptr(node)!=base+RUNTIME_SCENE_VOICE_FOVO_VTABLE_RVA||
       !runtime_story_graph_output_layout(base,node)||!runtime_story_graph_variable_layout(base,node)||
       !runtime_story_graph_input_member(base,node,0)||!runtime_story_graph_input_member(base,node,1)||
       !scene_readiness_array(node+0x98,&out,4)||out.count!=4||!scene_readiness_array(node+0x88,&in,2)||in.count!=2||
       !scene_readiness_array(node+0xa8,&vars,8)||!scene_readiness_array(node+0xb8,&events,8)||events.count||
       !read_at(node+0xd4,&flags,4)||!(flags&4))return 0;
    for(int i=0;i<out.count;++i){GameArray edges={0};uintptr_t at=out.data+(SIZE_T)i*0x34;uint32_t name[2]={0};
        if(!scene_readiness_array(at,&edges,1)||edges.count||!read_at(at+0x20,name,8)||name[1]||read_ptr(at+0x28))return 0;
        char text[32];name_of(base,name[0],text,sizeof text);if(strcmp(text,"None"))return 0;
    }
    for(int i=0;i<in.count;++i){uint32_t name[2]={0},n=1;uintptr_t at=in.data+(SIZE_T)i*0x28;
        if(!read_at(at+0x10,name,8)||name[1]||read_ptr(at+0x18)||!read_at(at+0x20,&n,4)||n)return 0;
        char text[32];name_of(base,name[0],text,sizeof text);if(strcmp(text,"None"))return 0;
    }
    for(int i=0;i<vars.count;++i){GameArray linked={0};
        if(!scene_readiness_array(vars.data+(SIZE_T)i*0x4c,&linked,1)||linked.count)return 0;}
    return read_at(node+0x98,&same,sizeof same)&&!memcmp(&same,&out,sizeof out);
}
static int runtime_scene_voice_sequence_live(uintptr_t base,const GuestCameraStamp *stamp) {
    uint32_t flags=0;uintptr_t seq=stamp->pointer;
    return guest_camera_stamp_matches(base,stamp,1)&&narrative_probe_object(base,seq,"Sequence")&&
        read_ptr(read_ptr(seq)+0x470)==base+RUNTIME_SCENE_VOICE_QUEUE_OP_RVA&&
        runtime_power_property(base,seq,"Sequence","m_bSFXInUpdateOp","BoolProperty",0x1b0,4)&&
        runtime_native_hud_bool(base,runtime_story_graph_class(base,seq,"Sequence"),"m_bSFXInUpdateOp",0x1b0,8)&&
        read_at(seq+0x1b0,&flags,4);
}
static int runtime_scene_voice_sequence_idle(uintptr_t base,const GuestCameraStamp *stamp) {
    uint32_t flags=8;return runtime_scene_voice_sequence_live(base,stamp)&&
        read_at(stamp->pointer+0x1b0,&flags,4)&&!(flags&8);
}
static int runtime_scene_voice_member(uintptr_t at,uintptr_t item,int max) {
    GameArray a={0};int found=0;if(!scene_readiness_array(at,&a,max))return -1;
    for(int i=0;i<a.count;++i)if(read_ptr(a.data+(SIZE_T)i*8)==item)++found;return found;
}
/* Match the stock pointer-array append, including the native allocator.
 * This is only our freshly constructed node in its verified owner sequence. */
static int runtime_scene_voice_append(uintptr_t base,uintptr_t at,uintptr_t node) {
    GameArray a={0};
    if(!scene_readiness_array(at,&a,4096)||a.count>=4096||runtime_scene_voice_member(at,node,4096)!=0||
       !background_focus_writable(at,sizeof a))return 0;
    if(a.count==a.max){
        static const unsigned char entry[]={0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0xd9,0x41,0x8b,0xc0,0x48,0x8b,0x09,0x48,0x85,0xc9,0x75,0x04,0x85,0xc0,0x74,0x15,0x41,0x0f,0xaf,0xc1,0x45,0x33,0xc9,0x45,0x8d,0x41,0x10,0x8b,0xd0,0xe8,0x26,0xbd,0x04,0x00,0x48,0x89,0x03,0x48,0x83,0xc4,0x20,0x5b,0xc3};unsigned char bytes[sizeof entry];
        if(!read_at(base+0x73440,bytes,sizeof bytes)||memcmp(bytes,entry,sizeof bytes))return 0;
        int maximum=a.count+16;if(maximum>4096)maximum=4096;
        typedef void (*Resize)(void*,int,int,int);
        ((Resize)(base+0x73440))((void*)at,a.count,maximum,8);
        uintptr_t data=read_ptr(at);int count=-1,oldmax=-1;
        if(!data||!read_at(at+8,&count,4)||count!=a.count||!read_at(at+12,&oldmax,4)||oldmax!=a.max||
           !background_focus_writable(data,(SIZE_T)maximum*8))return 0;
        memcpy((void*)(at+12),&maximum,4);a.data=data;a.max=maximum;
    }
    if(!background_focus_writable(a.data+(SIZE_T)a.count*8,8))return 0;
    memcpy((void*)(a.data+(SIZE_T)a.count*8),&node,8);++a.count;memcpy((void*)(at+8),&a.count,4);return 1;
}
static int runtime_scene_voice_none(uintptr_t base,uint64_t *out) {
    uintptr_t ptr=read_ptr(base+RUNTIME_SCENE_VOICE_NAME_ENTRY_GLOBAL_RVA);if(!ptr)return 0;
    for(unsigned i=0;i<8;++i){uintptr_t pool=read_ptr(base+NAME_POOLS_RVA+i*8);int32_t size=0;
        if(!read_at(base+NAME_POOLS_RVA+0x40+i*4,&size,4)||size<0)return 0;
        if(pool&&ptr>=pool&&ptr-pool<(uintptr_t)size){
            uint32_t index=(uint32_t)(ptr-pool)|(i<<29);char text[32];name_of(base,index,text,sizeof text);
            if(strcmp(text,"None"))return 0;*out=index;return 1;}
    }return 0;
}
static int runtime_scene_voice_playback_node_live(void) {
    RuntimeSceneVoicePlayback *s=&runtime_scene_voice_playback;
    return s->used&&guest_camera_stamp_matches(s->base,&s->node,1)&&
        guest_camera_stamp_matches(s->base,&s->current.scope.root,0)&&
        guest_camera_stamp_matches(s->base,&s->current.scope.world,0)&&
        read_ptr(s->base+0x18b6638)==s->current.scope.root.pointer&&
        runtime_scene_voice_sequence_live(s->base,&s->current.sequence)&&
        read_ptr(s->node.pointer+0x40)==s->current.sequence.pointer&&
        read_ptr(s->node.pointer+0x70)==s->current.sequence.pointer&&runtime_scene_voice_isolated(s->base,s->node.pointer);
}
static int runtime_scene_voice_playback_ready(void) {
    RuntimeSceneVoicePlayback *s=&runtime_scene_voice_playback;
    return s->prepared&&guest_camera_stamp_matches(s->base,&s->klass,1)&&
        runtime_scene_interp_hook_live(&s->activated)&&runtime_scene_interp_hook_live(&s->player);
}
static void runtime_scene_voice_playback_activated(void *self) {
    RuntimeSceneVoicePlayback *s=&runtime_scene_voice_playback;DWORD error=GetLastError();
    if(GetCurrentThreadId()!=session_game_thread){SetLastError(error);((RuntimeSceneInterpStart)s->activated.original)(self);return;}
    int owned=s->used&&(uintptr_t)self==s->node.pointer&&guest_camera_stamp_matches(s->base,&s->node,1);
    if(owned&&(!runtime_scene_voice_playback_node_live()||!runtime_scene_voice_playback_request_live||!runtime_scene_voice_playback_request_live(&s->current,1))){
        uint32_t flags=0;if(read_at(s->node.pointer+0x1c4,&flags,4)&&background_focus_writable(s->node.pointer+0x1c4,4)){
            flags|=0x80;memcpy((void*)(s->node.pointer+0x1c4),&flags,4);s->rejected=1;}
        runtime_scene_voice_playback_note("stale-owned-activation-refused");SetLastError(error);return;
    }
    uintptr_t selected=owned?s->current.host_shepard.pointer:
        runtime_scene_voice_playback_authored_player?runtime_scene_voice_playback_authored_player((uintptr_t)self):0;
    int prior=s->in_activation;uintptr_t prior_node=s->activation_node,prior_player=s->activation_player;
    s->in_activation=owned?1:selected?2:0;s->activation_node=(uintptr_t)self;s->activation_player=selected;SetLastError(error);
    ((RuntimeSceneInterpStart)s->activated.original)(self);error=GetLastError();s->in_activation=prior;
    s->activation_node=prior_node;s->activation_player=prior_player;SetLastError(error);
}
static uintptr_t runtime_scene_voice_playback_player(void *world) {
    RuntimeSceneVoicePlayback *s=&runtime_scene_voice_playback;
    typedef uintptr_t (*Getter)(void*);uintptr_t result=((Getter)s->player.original)(world);DWORD error=GetLastError();
    uintptr_t caller=(uintptr_t)__builtin_return_address(0);
    if(caller==s->base+0xc2b36d&&s->activation_player&&(
       (s->in_activation==1&&runtime_scene_voice_playback_node_live()&&(uintptr_t)world==s->current.scope.world.pointer&&
        runtime_scene_voice_playback_request_live&&runtime_scene_voice_playback_request_live(&s->current,1))||
       (s->in_activation==2&&runtime_scene_voice_playback_authored_player&&
        runtime_scene_voice_playback_authored_player(s->activation_node)==s->activation_player)))result=s->activation_player;
    SetLastError(error);return result;
}
static int runtime_scene_voice_playback_retire(void) {
    RuntimeSceneVoicePlayback *s=&runtime_scene_voice_playback;if(!s->used)return 1;
    if(!guest_camera_stamp_matches(s->base,&s->node,0)||!guest_camera_stamp_matches(s->base,&s->current.scope.root,0)||
       !guest_camera_stamp_matches(s->base,&s->current.scope.world,0)||read_ptr(s->base+0x18b6638)!=s->current.scope.root.pointer){
        s->used=0;memset(&s->node,0,sizeof s->node);return 1;}
    if(!runtime_scene_voice_playback_node_live()||!runtime_scene_voice_sequence_idle(s->base,&s->current.sequence))return 0;
    GameArray all={0};uintptr_t info=s->current.scope.world.pointer,node=s->node.pointer,actor=read_ptr(node+0x198);
    if(!runtime_power_property(s->base,info,"BioWorldInfo","m_aCurrentFaceOnlyVOs","ArrayProperty",0x7dc,16)||
       !scene_readiness_array(info+0x7dc,&all,128))return 0;
    for(int i=0;i<all.count;++i){uintptr_t other=read_ptr(all.data+(SIZE_T)i*8);uint32_t flags=0;
        if(other==node)continue;
        if(!narrative_probe_object(s->base,other,"BioSeqAct_FaceOnlyVO")||!read_at(other+0x1c4,&flags,4))return 0;
        if(actor&&read_ptr(other+0x198)==actor&&!(flags&0x100))return 0; /* Another voice owns this actor. */
    }
    if(!runtime_scene_voice_pin_live(s->base,RUNTIME_SCENE_VOICE_PIN_END)||
       !runtime_scene_voice_pin_live(s->base,RUNTIME_SCENE_VOICE_PIN_KILL)||
       !runtime_scene_voice_pin_live(s->base,RUNTIME_SCENE_VOICE_PIN_REMOVE_ITEM))return 0;
    ((RuntimeSceneVoiceEndFOVO)(s->base+RUNTIME_SCENE_VOICE_END_RVA))((void*)info,(void*)node);
    if(!runtime_scene_voice_playback_node_live())return 0;
    RuntimeSceneVoiceRemoveItem remove=(RuntimeSceneVoiceRemoveItem)(s->base+RUNTIME_SCENE_VOICE_REMOVE_ITEM_RVA);
    const unsigned offsets[]={0xec,0x13c,0xdc};
    for(unsigned i=0;i<3;++i){uintptr_t at=s->current.sequence.pointer+offsets[i];
        if(runtime_scene_voice_member(at,node,4096)<0)return 0;remove((void*)at,&node);
        if(runtime_scene_voice_member(at,node,4096)!=0)return 0;}
    uintptr_t zero=0;if(background_focus_writable(node+0x70,8))memcpy((void*)(node+0x70),&zero,8);else return 0;
    s->used=0;s->rejected=0;memset(&s->node,0,sizeof s->node);runtime_scene_voice_playback_note("owned-voice-retired");return 1;
}
static int runtime_scene_voice_playback_enqueue(const RuntimeSceneVoiceRequest *r) {
    RuntimeSceneVoicePlayback *s=&runtime_scene_voice_playback;
    if(!r||!r->scene_nonce||!r->line_generation||r->strref<=0||r->flags>7)return 0;
    s->pending=*r;s->queued=1;return 1;
}
static void runtime_scene_voice_playback_end(uint64_t scene,uint64_t generation) {
    RuntimeSceneVoicePlayback *s=&runtime_scene_voice_playback;
    if(s->queued&&s->pending.scene_nonce==scene&&s->pending.line_generation<=generation)s->queued=0;
    if(s->used&&s->current.scene_nonce==scene&&s->current.line_generation<=generation)s->rejected=1;
}
static int runtime_scene_voice_playback_create(const RuntimeSceneVoiceRequest *r) {
    RuntimeSceneVoicePlayback *s=&runtime_scene_voice_playback;uint64_t name=0;uintptr_t base=s->base,seq=r->sequence.pointer;
    if(!runtime_scene_voice_playback_request_live||!runtime_scene_voice_playback_request_live(r,1)||
       !runtime_scene_voice_sequence_idle(base,&r->sequence)||!runtime_scene_voice_none(base,&name)||
       !runtime_scene_voice_pin_live(base,RUNTIME_SCENE_VOICE_PIN_FACTORY)||
       !runtime_scene_voice_pin_live(base,RUNTIME_SCENE_VOICE_PIN_QUEUE_OP))return 0;
    uintptr_t node=((RuntimeSceneVoiceFactory)(base+RUNTIME_SCENE_VOICE_FACTORY_RVA))(s->klass.pointer,seq,name,0,0,
        read_ptr(base+RUNTIME_SCENE_VOICE_FACTORY_ARG6_GLOBAL_RVA),0,0);
    if(!node||!guest_camera_stamp_read(base,node,&s->node,1)||read_ptr(node+0x40)!=seq||
       !runtime_scene_voice_isolated(base,node)||!runtime_scene_voice_sequence_live(base,&r->sequence))return 0;
    /* The fresh object has no references to authored actions. Attach exactly
     * as stock FOVO init does, then let the normal sequence scheduler run it. */
    uintptr_t at[]={node+0x70,node+0x188,node+0x198};
    for(unsigned i=0;i<3;++i)if(!background_focus_writable(at[i],8))return 0;
    if(!background_focus_writable(node+0x190,8)||!background_focus_writable(node+0x1b0,0x18))return 0;
    memcpy((void*)(node+0x70),&seq,8);memcpy((void*)(node+0x188),&r->conversation.pointer,8);
    memcpy((void*)(node+0x198),&r->actor.pointer,8);memcpy((void*)(node+0x190),&name,8);
    memcpy((void*)(node+0x1b0),&r->strref,4);int32_t zero=0;memcpy((void*)(node+0x1b4),&zero,4);
    uint32_t flags=0x800|0x40|((r->flags&SCENE_VOICE_HIDE_SUBTITLES)?1:0);memcpy((void*)(node+0x1c4),&flags,4);
    s->current=*r;s->used=1;s->rejected=0;
    if(!runtime_scene_voice_append(base,seq+0xdc,node))return 0;
    if(!runtime_scene_voice_playback_request_live(r,1))return 0;
    GameArray inputs={0};if(!scene_readiness_array(node+0x88,&inputs,2)||inputs.count!=2||
       !background_focus_writable(inputs.data+0x24,4))return 0;
    if(!((RuntimeSceneVoiceQueueOp)(base+RUNTIME_SCENE_VOICE_QUEUE_OP_RVA))((void*)seq,(void*)node,0))return 0;
    if(!runtime_scene_voice_playback_node_live())return 0;
    uint32_t impulse=0;if(!read_at(inputs.data+0x24,&impulse,4))return 0;impulse|=1;memcpy((void*)(inputs.data+0x24),&impulse,4);
    runtime_scene_voice_playback_note("isolated-native-voice-queued");return 1;
}
static void runtime_scene_voice_playback_tick(uintptr_t base,uintptr_t engine,DWORD now) {
    DWORD error=GetLastError();RuntimeSceneVoicePlayback *s=&runtime_scene_voice_playback;s->base=base;s->engine=engine;
    if(g_runtime_config.role!=ME3_ROLE_GUEST||GetCurrentThreadId()!=session_game_thread||s->busy)goto done;
    if(!s->prepared&&(!s->checked||now-s->checked>=1000)){
        s->checked=now;uintptr_t fn=function_named(base,"BioSeqAct_FaceOnlyVO","GetObjClassVersion"),klass=read_ptr(fn+0x40);
        GuestCameraStamp stamp={0};
        if(runtime_scene_voice_pins_ready(base)&&narrative_probe_object(base,klass,"Class")&&
           guest_camera_stamp_read(base,klass,&stamp,1)&&!strcmp(stamp.full_name,"SFXGame.BioSeqAct_FaceOnlyVO")&&
           runtime_scene_voice_playback_layout(base,klass)&&
           runtime_scene_interp_hook_install(base,&s->activated,RUNTIME_SCENE_VOICE_ACTIVATED_RVA,runtime_scene_voice_activated_entry,(void*)runtime_scene_voice_playback_activated)&&
           runtime_scene_interp_hook_install(base,&s->player,RUNTIME_SCENE_VOICE_WORLD_PLAYER_RVA,runtime_scene_voice_world_player_entry,(void*)runtime_scene_voice_playback_player)){
            s->klass=stamp;s->prepared=1;runtime_scene_voice_playback_note("native-isolated-voice-prepared");}
    }
    if(!runtime_scene_voice_playback_ready())goto done;
    s->busy=1;
    if(s->used&&(s->rejected||!runtime_scene_voice_playback_request_live||
       !runtime_scene_voice_playback_request_live(&s->current,0)||
       (s->queued&&(s->pending.scene_nonce!=s->current.scene_nonce||s->pending.line_generation!=s->current.line_generation)))){
        if(!runtime_scene_voice_playback_retire())goto busy_done;
    }
    if(!s->used&&s->queued){RuntimeSceneVoiceRequest request=s->pending;
        if(runtime_scene_voice_playback_request_live&&runtime_scene_voice_playback_request_live(&request,1)){
            s->queued=0;if(!runtime_scene_voice_playback_create(&request)){s->rejected=s->used;runtime_scene_voice_playback_note("isolated-voice-create-refused");}
        }
    }
 busy_done:s->busy=0;
 done:SetLastError(error);
}
#ifdef ME3_RUNTIME_FIXTURE
static int runtime_scene_voice_playback_selftest(void) {
    RuntimeSceneVoicePlayback saved=runtime_scene_voice_playback;memset(&runtime_scene_voice_playback,0,sizeof saved);
    RuntimeSceneVoiceRequest r={0};r.scene_nonce=1;r.line_generation=2;r.strref=3;
    int ok=runtime_scene_voice_playback_enqueue(&r)&&runtime_scene_voice_playback.queued;
    runtime_scene_voice_playback_end(2,9);ok=ok&&runtime_scene_voice_playback.queued;
    runtime_scene_voice_playback_end(1,1);ok=ok&&runtime_scene_voice_playback.queued;
    runtime_scene_voice_playback_end(1,2);ok=ok&&!runtime_scene_voice_playback.queued;
    r.strref=0;ok=ok&&!runtime_scene_voice_playback_enqueue(&r);runtime_scene_voice_playback=saved;
    return ok&&scene_voice_protocol_selftest();
}
#endif
#endif
