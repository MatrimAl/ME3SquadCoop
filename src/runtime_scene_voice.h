#ifndef ME3_RUNTIME_SCENE_VOICE_H
#define ME3_RUNTIME_SCENE_VOICE_H
/* Actual host speech, independently authenticated from dialogue advice.
 * The natural native VO key supplies timing; it never runs guest story code. */
#include "runtime_scene_voice_playback.h"
typedef void (*RuntimeSceneVoiceKey)(void*,int,void*);
typedef struct RuntimeSceneVoiceKeyProof {
    RuntimeSceneScope scope;GuestCameraStamp action,track,instance,controller,matinee,channel,sequence;
    uint64_t serial;DWORD observed;int index,valid;
} RuntimeSceneVoiceKeyProof;
typedef struct RuntimeSceneVoice {
    uintptr_t base,engine;RuntimeSceneInterpHook key;
    RuntimeSceneVoiceKeyProof host,guest;
    RuntimeSceneScope received_scope;SceneVoiceMessage received,sent;
    RuntimeSceneScope sex_scope;SceneVoiceMessage sex_sent;
    RuntimeSceneVoiceRequest request;
    GuestCameraStamp conversations[3],sequences[3],roots[3];
    uintptr_t *scan;GameArray scan_objects;GuestCameraStamp scan_root,scan_sequence;
    uintptr_t scan_match;int scan_at,scan_matches;
    uint64_t serial,generation,consumed;DWORD received_at,sent_at,polled,scanned;
    uint64_t sex_generation,sex_nonce;DWORD sex_at,sex_sent_at;unsigned sex;int sex_valid;
    unsigned rows;int attempted,valid,have_request;
} RuntimeSceneVoice;
static RuntimeSceneVoice runtime_scene_voice;
static int runtime_scene_voice_ready(void);
static int runtime_scene_voice_host_sex(unsigned);
static void runtime_scene_voice_key(void*,int,void*);
static void runtime_scene_voice_note(const char *event) {
    RuntimeSceneVoice *s=&runtime_scene_voice;if(s->rows++>=128)return;
    char row[420];snprintf(row,sizeof row,"{\"runtime_scene_voice\":\"%s\",\"generation\":%llu,\"strref\":%u,\"story_replayed\":false,\"gameplay_verified\":false,\"monotonic_ms\":%lu}\r\n",event,(unsigned long long)s->received.generation,s->received.strref,GetTickCount());line(row);
}
static int runtime_scene_voice_channel(uintptr_t base,const RuntimeSceneScope *scope,unsigned index,
        const char *kind,GuestCameraStamp *actor,GuestCameraStamp *channel) {
    if(index<1||index>2047||!runtime_scene_scope_live(base,scope))return 0;
    uintptr_t ch=read_ptr(scope->connection.pointer+0xf84+(SIZE_T)index*8),a=read_ptr(ch+0xa0);NetworkActorIdentityEntry id={0};
    return guest_camera_actor_live(base,a)&&is_kind_of(base,a,kind)&&read_ptr(a+0x198)==scope->world.pointer&&
        network_actor_channel_valid(base,a,ch,scope->driver.pointer,&id)&&id.connection==scope->connection.pointer&&
        id.index==(int)index&&guest_camera_stamp_read(base,a,actor,1)&&guest_camera_stamp_read(base,ch,channel,1);
}
static int runtime_scene_voice_line_array(uintptr_t base,uintptr_t conversation,int entry,GameArray *array) {
    const char *name=entry?"m_EntryList":"m_ReplyList",*type=entry?"BioDialogEntryNode":"BioDialogReplyNode";
    uintptr_t klass=runtime_story_graph_class(base,conversation,"BioConversation"),structure=0;
    uintptr_t prop=runtime_native_hud_field(base,klass,name,"ArrayProperty",entry?0xb8:0xc8,16);
    if(!prop||!runtime_story_graph_struct(base,prop,type,entry?0x6c:0x5c,&structure))return 0;
    int found=0;for(unsigned n=0;structure&&n<8;++n){char text[64];
        if(!narrative_probe_object(base,structure,"ScriptStruct"))return 0;object_name(base,structure,text,sizeof text);
        if(!strcmp(text,"BioDialogNode")){
            found=runtime_native_hud_field(base,structure,"srText","StringRefProperty",0x18,4)!=0;break;}
        structure=read_ptr(structure+0x60);
    }
    return found&&scene_readiness_array(conversation+(entry?0xb8:0xc8),array,4096);
}
static int runtime_scene_voice_conversation_live(uintptr_t base,uintptr_t conv,uintptr_t sequence,int resource) {
    int id=0;GuestCameraStamp stamp={0};
    if(!narrative_probe_object(base,conv,"BioConversation")||!guest_camera_stamp_read(base,conv,&stamp,1)||
       !runtime_scene_interp_path(stamp.full_name)||read_ptr(conv+0x148)!=sequence||
       read_ptr(conv+0x40)!=read_ptr(sequence+0x40)||
       !runtime_power_property(base,conv,"BioConversation","MatineeSequence","ObjectProperty",0x148,8)||
       !runtime_power_property(base,conv,"BioConversation","m_nResRefID","IntProperty",0x168,4)||
       !read_at(conv+0x168,&id,4)||id<=0||(resource&&id!=resource))return 0;
    return 1;
}
static int runtime_scene_voice_line_exists(uintptr_t base,uintptr_t conv,int strref,int entry) {
    int matches=0,correct=0;
    for(int e=0;e<2;++e){GameArray a={0},same={0};int stride=e?0x6c:0x5c;
        if(!runtime_scene_voice_line_array(base,conv,e,&a))return 0;
        for(int i=0;i<a.count;++i){int value=0;if(!read_at(a.data+(SIZE_T)i*stride+0x18,&value,4))return 0;
            if(value==strref){++matches;if(e==entry)++correct;}}
        if(!read_at(conv+(e?0xb8:0xc8),&same,sizeof same)||memcmp(&same,&a,sizeof a))return 0;
    }return matches==1&&correct==1;
}
static int runtime_scene_voice_context(uintptr_t track,uintptr_t instance,RuntimeSceneVoiceKeyProof *out) {
    RuntimeSceneVoice *s=&runtime_scene_voice;uintptr_t base=s->base,gi=read_ptr(instance+0x40),action=read_ptr(gi+0x40);
    uintptr_t group=read_ptr(gi+0x70),data=read_ptr(action+0x154),sequence=read_ptr(action+0x40);
    GameArray groups={0},tracks={0},instances={0};GuestCameraStamp stamp={0};RuntimeSceneScope scope={0};
    if(g_runtime_config.role==ME3_ROLE_HOST){if(!runtime_scene_host_peer(base,session_context(s->engine),&scope))return 0;}
    else if(!runtime_scene_actor_observer_scope(base,s->engine,&scope))return 0;
    if(!narrative_probe_object(base,track,"BioEvtSysTrackVOElements")||
       !narrative_probe_object(base,instance,"BioEvtSysTrackVOElementsInst")||
       !runtime_power_property(base,instance,"BioEvtSysTrackVOElementsInst","m_pConvCont","ObjectProperty",0x64,8)||
       !narrative_probe_object(base,gi,"InterpGroupInst")||!narrative_probe_object(base,group,"InterpGroup")||
       !narrative_probe_object(base,data,"InterpData")||read_ptr(group+0x40)!=data||read_ptr(track+0x40)!=group||
       !runtime_scene_interp_action(base,action,scope.world.pointer,&stamp,NULL,NULL)||
       !narrative_probe_object(base,sequence,"Sequence")||
       !scene_readiness_array(action+0x12c,&groups,128)||!scene_readiness_array(group+0x68,&tracks,128)||
       !scene_readiness_array(gi+0x60,&instances,128)||tracks.count!=instances.count)return 0;
    int a=0,b=0;for(int i=0;i<groups.count;++i)if(read_ptr(groups.data+(SIZE_T)i*8)==gi)++a;
    for(int i=0;i<tracks.count;++i)if(read_ptr(tracks.data+(SIZE_T)i*8)==track&&read_ptr(instances.data+(SIZE_T)i*8)==instance)++b;
    if(a!=1||b!=1)return 0;
    uintptr_t matinee=read_ptr(action+0x15c);NetworkActorIdentityEntry id={0};
    if(g_runtime_config.role==ME3_ROLE_HOST){
        if(!guest_camera_actor_live(base,matinee)||!is_kind_of(base,matinee,"MatineeActor")||read_ptr(matinee+0x270)!=action||
           network_actor_scan_driver(base,matinee,scope.driver.pointer,&id)<1||id.connection!=scope.connection.pointer)return 0;
    }else{
        GameArray channels={0};int found=0;if(!scene_readiness_array(scope.connection.pointer+0x5f90,&channels,2048))return 0;
        for(int i=0;i<channels.count;++i){uintptr_t ch=read_ptr(channels.data+(SIZE_T)i*8),actor=read_ptr(ch+0xa0);
            if(!actor||read_ptr(actor+0x270)!=action)continue;
            if(++found!=1||!guest_camera_actor_live(base,actor)||!is_kind_of(base,actor,"MatineeActor")||
               !network_actor_channel_valid(base,actor,ch,scope.driver.pointer,&id)||id.connection!=scope.connection.pointer)return 0;
            matinee=actor;}
        if(found!=1)return 0;
    }
    RuntimeSceneVoiceKeyProof p={0};p.scope=scope;p.action=stamp;p.index=id.index;p.observed=GetTickCount();
    if(!guest_camera_stamp_read(base,track,&p.track,1)||!guest_camera_stamp_read(base,instance,&p.instance,1)||
       !guest_camera_stamp_read(base,matinee,&p.matinee,1)||!guest_camera_stamp_read(base,id.channel,&p.channel,1)||
       !guest_camera_stamp_read(base,sequence,&p.sequence,1))return 0;
    if(g_runtime_config.role==ME3_ROLE_HOST){uintptr_t controller=read_ptr(instance+0x64);
        if(!narrative_probe_object(base,controller,"BioConversationController")||read_ptr(controller+0x2dc)!=action||
           !guest_camera_stamp_read(base,controller,&p.controller,1))return 0;}
    p.valid=1;*out=p;return 1;
}
static int runtime_scene_voice_proof_live(const RuntimeSceneVoiceKeyProof *p) {
    RuntimeSceneVoice *s=&runtime_scene_voice;NetworkActorIdentityEntry id={0};uint32_t state=0;
    return p->valid&&runtime_scene_scope_live(s->base,&p->scope)&&guest_camera_stamp_matches(s->base,&p->action,1)&&
        guest_camera_stamp_matches(s->base,&p->track,1)&&guest_camera_stamp_matches(s->base,&p->instance,1)&&
        guest_camera_stamp_matches(s->base,&p->matinee,1)&&guest_camera_stamp_matches(s->base,&p->channel,1)&&
        guest_camera_stamp_matches(s->base,&p->sequence,1)&&read_ptr(p->action.pointer+0x40)==p->sequence.pointer&&
        read_ptr(p->matinee.pointer+0x270)==p->action.pointer&&read_at(p->matinee.pointer+0x284,&state,4)&&(state&1)&&
        network_actor_channel_valid(s->base,p->matinee.pointer,p->channel.pointer,p->scope.driver.pointer,&id)&&
        id.connection==p->scope.connection.pointer&&id.index==p->index;
}
static void runtime_scene_voice_key_observe(void *track,int key,void *instance) {
    RuntimeSceneVoice *s=&runtime_scene_voice;
    RuntimeSceneVoiceKeyProof p={0};
    if(key>=0&&GetCurrentThreadId()==session_game_thread&&runtime_scene_voice_context((uintptr_t)track,(uintptr_t)instance,&p)&&
       s->serial!=UINT64_MAX){p.serial=++s->serial;
        if(g_runtime_config.role==ME3_ROLE_HOST)s->host=p;else s->guest=p;
        runtime_scene_voice_note("natural-vo-key-observed");}
}
static void runtime_scene_voice_key_forward(void *track,int key,void *instance,RuntimeSceneVoiceKey original,RuntimeSceneVoiceKey observe) {
    original(track,key,instance);DWORD error=GetLastError();observe(track,key,instance);SetLastError(error);
}
static void runtime_scene_voice_key(void *track,int key,void *instance) {
    runtime_scene_voice_key_forward(track,key,instance,(RuntimeSceneVoiceKey)runtime_scene_voice.key.original,runtime_scene_voice_key_observe);
}
static int runtime_scene_voice_host_line(SceneVoiceMessage *out) {
    RuntimeSceneVoice *s=&runtime_scene_voice;RuntimeSceneVoiceKeyProof *p=&s->host;uintptr_t base=s->base,conv=0,speaker=0;
    uint32_t phase=0,flags=0,sex=0;int entry=0,index=-1,strref=0,resource=0;GameArray nodes={0};NetworkActorIdentityEntry actor={0};
    uintptr_t controller=p->controller.pointer;
    if(!runtime_scene_voice_proof_live(p)||!guest_camera_stamp_matches(base,&p->controller,1)||
       read_ptr(p->instance.pointer+0x64)!=controller||!runtime_ambient_conversation_layout(base,controller)||
       read_ptr(controller+0x2dc)!=p->action.pointer||!read_at(controller+0x358,&flags,4)||(flags&(2|0x80|0x1000|0x2000|0x8000))||
       !runtime_power_property(base,controller,"BioConversationController","m_nNodeStateFlags","IntProperty",0x31c,4)||
       !read_at(controller+0x31c,&phase,4)||((phase&3)!=1&&(phase&3)!=2))return 0;
    entry=(phase&3)==1;conv=read_ptr(controller+0x284);speaker=read_ptr(controller+0x2a4);
    if(!runtime_scene_voice_conversation_live(base,conv,p->sequence.pointer,0)||
       !read_at(controller+(entry?0x2ec:0x2f8),&index,4)||!runtime_scene_voice_line_array(base,conv,entry,&nodes)||index<0||index>=nodes.count||
       !read_at(nodes.data+(SIZE_T)index*(entry?0x6c:0x5c)+0x18,&strref,4)||strref<=0||
       !runtime_scene_voice_line_exists(base,conv,strref,entry)||!read_at(conv+0x168,&resource,4)||
       !guest_camera_actor_live(base,speaker)||!is_kind_of(base,speaker,"BioPawn")||read_ptr(speaker+0x198)!=p->scope.world.pointer||
       network_actor_scan_driver(base,speaker,p->scope.driver.pointer,&actor)<1||actor.connection!=p->scope.connection.pointer||
       !session_actor_live(base,session_shepard)||read_ptr(controller+0x29c)!=session_shepard||
       !read_at(session_shepard+0x578,&sex,4)||!runtime_scene_host_active)return 0;
    /* These four exact intro lines are authored in the main Matinee's
     * native FOVO tracks, including their preload interval before they enter
     * WorldInfo's playing list. Keep that existing route for the whole key. */
    if(resource==3807&&strref>=591649&&strref<=591652)return 0;
    /* The introductory main Matinee already owns several native FOVO keys.
     * Preserve that exact path instead of duplicating the same spoken line. */
    GameArray fovo={0};if(!scene_readiness_array(p->scope.world.pointer+0x7dc,&fovo,128))return 0;
    for(int i=0;i<fovo.count;++i){uintptr_t node=read_ptr(fovo.data+(SIZE_T)i*8);int ref=0;
        if(!narrative_probe_object(base,node,"BioSeqAct_FaceOnlyVO"))return 0;
        if(read_ptr(node+0x188)==conv&&read_at(node+0x1b0,&ref,4)&&ref==strref&&read_ptr(node+0x198)==speaker)return 0;}
    *out=(SceneVoiceMessage){'P',runtime_scene_host_nonce,0,(uint32_t)p->index,(uint32_t)actor.index,
        (uint32_t)resource,(uint32_t)strref,(sex&0x20?1u:0u)|(flags&0x10000?2u:0u)|(entry?4u:0u)};return 1;
}
static int runtime_scene_voice_receive(uintptr_t self,uintptr_t frame,const wchar_t *text,size_t count) {
    RuntimeSceneVoice *s=&runtime_scene_voice;NativeRpcReceiveFrame *r=native_rpc_receive_current(session_game_thread);
    RuntimeSceneScope scope={0};NetworkActorIdentityEntry source={0};SceneVoiceMessage message={0};
    if(g_runtime_config.role!=ME3_ROLE_GUEST||!scene_voice_parse(text,count,&message)||!r||!r->channel||
       !presentation_method_valid(1)||read_ptr(frame+0x18)!=presentation_methods[1].function.pointer||read_ptr(frame+0x20)!=self||
       !runtime_scene_actor_observer_scope(s->base,s->engine,&scope)||scope.pc.pointer!=self||
       r->channel!=scope.channel.pointer||!network_actor_channel_valid(s->base,self,r->channel,scope.driver.pointer,&source)||
       source.connection!=scope.connection.pointer||!runtime_scene_view.valid||
       !runtime_scene_scope_live(s->base,&runtime_scene_view.scope)||message.scene_nonce!=runtime_scene_view.message.nonce)return 1;
    if(message.kind=='H'){
        int same=s->sex_valid&&s->sex_nonce==message.scene_nonce&&
            guest_camera_stamp_core_equal(&s->sex_scope.connection,&scope.connection);
        if(same&&(message.generation<s->sex_generation||(message.generation==s->sex_generation&&message.flags!=s->sex)))return 1;
        int changed=!same||message.generation!=s->sex_generation;
        s->sex_scope=scope;s->sex=message.flags;s->sex_nonce=message.scene_nonce;s->sex_generation=message.generation;
        s->sex_at=GetTickCount();s->sex_valid=1;if(changed)runtime_scene_voice_note("authenticated-host-sex-captured");return 1;
    }
    if(s->valid&&(s->received.scene_nonce!=message.scene_nonce||
       !guest_camera_stamp_core_equal(&s->received_scope.connection,&scope.connection))){s->valid=0;s->consumed=0;}
    if(s->valid&&message.generation<s->received.generation)return 1;
    if(s->valid&&message.generation==s->received.generation&&!scene_voice_message_same(&message,&s->received))return 1;
    int changed=!s->valid||message.generation!=s->received.generation;
    s->received=message;s->received_scope=scope;s->received_at=GetTickCount();s->valid=1;
    if(changed){s->have_request=0;runtime_scene_voice_note("authenticated-host-line-received");}
    return 1;
}
static uintptr_t runtime_scene_voice_find_conversation(const RuntimeSceneVoiceKeyProof *proof,int resource) {
    RuntimeSceneVoice *s=&runtime_scene_voice;uintptr_t base=s->base,seq=proof->sequence.pointer;
    for(unsigned i=0;i<3;++i)if(s->sequences[i].pointer==seq&&guest_camera_stamp_matches(base,&s->sequences[i],1)&&
       guest_camera_stamp_matches(base,&s->roots[i],0)&&s->roots[i].pointer==proof->scope.root.pointer&&
       guest_camera_stamp_matches(base,&s->conversations[i],1)&&runtime_scene_voice_conversation_live(base,s->conversations[i].pointer,seq,resource))return s->conversations[i].pointer;
    DWORD now=GetTickCount();
    if(s->scan&&(s->scan_sequence.pointer!=seq||!guest_camera_stamp_matches(base,&s->scan_sequence,1)||
       !guest_camera_stamp_matches(base,&s->scan_root,0)||s->scan_root.pointer!=proof->scope.root.pointer)){
        HeapFree(GetProcessHeap(),0,s->scan);s->scan=NULL;}
    if(!s->scan){
        if(s->scanned&&now-s->scanned<250)return 0;s->scanned=now;
        s->scan=identity_object_list(base,&s->scan_objects);if(!s->scan)return 0;
        s->scan_sequence=proof->sequence;s->scan_root=proof->scope.root;s->scan_at=s->scan_matches=0;s->scan_match=0;
    }
    LARGE_INTEGER start,frequency,current;
    if(!QueryPerformanceFrequency(&frequency)||!QueryPerformanceCounter(&start))return 0;
    uintptr_t outer=read_ptr(seq+0x40);
    for(;s->scan_at<s->scan_objects.count;++s->scan_at){uintptr_t obj=s->scan[s->scan_at];
        if(obj&&read_ptr(obj+0x40)==outer&&read_ptr(obj+0x148)==seq&&runtime_scene_voice_conversation_live(base,obj,seq,resource)){
            if(++s->scan_matches>1)break;s->scan_match=obj;}
        if((s->scan_at&63)==63&&QueryPerformanceCounter(&current)&&current.QuadPart-start.QuadPart>=frequency.QuadPart/500){
            ++s->scan_at;return 0;}
    }
    uintptr_t found=s->scan_match;int count=s->scan_matches;GameArray objects={0};
    int stable=read_at(base+OBJECTS_RVA,&objects,sizeof objects)&&!memcmp(&objects,&s->scan_objects,sizeof objects);
    HeapFree(GetProcessHeap(),0,s->scan);s->scan=NULL;if(!stable||count!=1)return 0;
    for(unsigned i=0;i<3;++i)if(!s->sequences[i].pointer||!guest_camera_stamp_matches(base,&s->sequences[i],0)){
        if(!guest_camera_stamp_read(base,found,&s->conversations[i],1))return 0;
        s->sequences[i]=proof->sequence;s->roots[i]=proof->scope.root;break;}
    return found;
}
/* This nonreplicated cosmetic field is copied only onto the authenticated
 * host-Shepard replica. Local Ashley and every host pawn remain untouched. */
static int runtime_scene_voice_host_sex(unsigned female) {
    RuntimeScenePlayerBinding *p=&runtime_scene_player_binding;uintptr_t base=p->base,actor=p->actor.pointer;uint32_t before=0;
    uintptr_t type=runtime_story_graph_class(base,actor,"Pawn");
    if(female>1||!runtime_scene_player_live()||!runtime_native_hud_bool(base,type,"bIsFemale",0x578,0x20)||
       !read_at(actor+0x578,&before,4))return 0;
    uint32_t desired=(before&~UINT32_C(0x20))|(female?0x20:0);
    if(before!=desired){if(!background_focus_writable(actor+0x578,4)||
        (uint32_t)InterlockedCompareExchange((LONG volatile*)(actor+0x578),(LONG)desired,(LONG)before)!=before)return 0;}
    return runtime_scene_player_live()&&read_at(actor+0x578,&before,4)&&((before&0x20)!=0)==(female!=0);
}
static int runtime_scene_voice_sex_live(void) {
    RuntimeSceneVoice *s=&runtime_scene_voice;RuntimeScenePlayerBinding *p=&runtime_scene_player_binding;
    return s->sex_valid&&GetTickCount()-s->sex_at<=2000&&runtime_scene_scope_live(s->base,&s->sex_scope)&&
        runtime_scene_player_live()&&runtime_scene_view.valid&&s->sex_nonce==runtime_scene_view.message.nonce&&
        guest_camera_stamp_core_equal(&s->sex_scope.connection,&p->scope.connection)&&
        guest_camera_stamp_core_equal(&s->sex_scope.root,&p->scope.root);
}
/* The intro's native FOVO track already creates its own isolated nodes.
 * Give only a proved current track-instance member the same host-player
 * sex selection as our isolated speech, without replaying that native key. */
static uintptr_t runtime_scene_voice_authored_player(uintptr_t node) {
    RuntimeSceneVoice *s=&runtime_scene_voice;uintptr_t base=s->base,sequence=read_ptr(node+0x70);
    GuestCameraStamp stamp={0};
    if(!runtime_scene_voice_sex_live()||
       !runtime_scene_voice_isolated(base,node)||read_ptr(node+0x40)!=sequence||
       !guest_camera_stamp_read(base,sequence,&stamp,1)||
       strcmp(stamp.full_name,"BioD_ProMar_205ALiaraIntro.TheWorld.PersistentLevel.Main_Sequence.Liara_Introduction.ANIMCUTSCENE_promar_liara_intro"))return 0;
    uint32_t sex=0;if(!read_at(runtime_scene_player_binding.actor.pointer+0x578,&sex,4)||((sex&0x20)!=0)!=(s->sex!=0))return 0;
    GameArray channels={0};int owners=0;
    if(!scene_readiness_array(s->sex_scope.connection.pointer+0x5f90,&channels,2048))return 0;
    for(int i=0;i<channels.count;++i){uintptr_t channel=read_ptr(channels.data+(SIZE_T)i*8),matinee=read_ptr(channel+0xa0);
        uintptr_t action=read_ptr(matinee+0x270);NetworkActorIdentityEntry id={0};uint32_t state=0;
        if(!action||read_ptr(action+0x40)!=sequence)continue;
        if(!guest_camera_actor_live(base,matinee)||!is_kind_of(base,matinee,"MatineeActor")||
           !network_actor_channel_valid(base,matinee,channel,s->sex_scope.driver.pointer,&id)||id.connection!=s->sex_scope.connection.pointer||
           !read_at(matinee+0x284,&state,4)||!(state&1)||!guest_camera_stamp_read(base,action,&stamp,1)||
           strcmp(stamp.full_name,RUNTIME_SCENE_FX_PARENT "SeqAct_Interp_2"))return 0;
        GameArray groups={0};if(!scene_readiness_array(action+0x12c,&groups,128))return 0;
        for(int g=0;g<groups.count;++g){uintptr_t gi=read_ptr(groups.data+(SIZE_T)g*8),group=read_ptr(gi+0x70);
            GameArray tracks={0},instances={0};
            if(!narrative_probe_object(base,gi,"InterpGroupInst")||read_ptr(gi+0x40)!=action||
               !scene_readiness_array(group+0x68,&tracks,128)||!scene_readiness_array(gi+0x60,&instances,128)||tracks.count!=instances.count)return 0;
            for(int t=0;t<tracks.count;++t){uintptr_t track=read_ptr(tracks.data+(SIZE_T)t*8),inst=read_ptr(instances.data+(SIZE_T)t*8);
                if(!is_kind_of(base,track,"SFXInterpTrackPlayFaceOnlyVO"))continue;
                GameArray nodes={0};
                if(!narrative_probe_object(base,inst,"SFXInterpTrackPlayFaceOnlyVOInst")||read_ptr(inst+0x40)!=gi||
                   !runtime_power_property(base,inst,"SFXInterpTrackPlayFaceOnlyVOInst","m_aFOVOObjects","ArrayProperty",0x6c,16)||
                   !scene_readiness_array(inst+0x6c,&nodes,128))return 0;
                for(int n=0;n<nodes.count;++n)if(read_ptr(nodes.data+(SIZE_T)n*8)==node){
                    if(read_ptr(node+0x198)!=read_ptr(inst+0x64))return 0;++owners;}
            }
        }
    }
    return owners==1?runtime_scene_player_binding.actor.pointer:0;
}
static int runtime_scene_voice_request_live(const RuntimeSceneVoiceRequest *r,int starting) {
    RuntimeSceneVoice *s=&runtime_scene_voice;SceneVoiceMessage *m=&s->received;
    if(!s->valid||m->kind!='P'||m->scene_nonce!=r->scene_nonce||m->generation!=r->line_generation||
       GetTickCount()-s->received_at>500||!runtime_scene_scope_live(s->base,&s->received_scope)||
       !runtime_scene_view.valid||!runtime_scene_view.message.active||runtime_scene_view.message.nonce!=r->scene_nonce||
       !runtime_scene_voice_proof_live(&s->guest)||s->guest.index!=(int)m->matinee_channel||
       !runtime_scene_voice_sex_live()||s->sex!=(m->flags&1)||!guest_camera_stamp_matches(s->base,&r->host_shepard,1)||
       r->host_shepard.pointer!=runtime_scene_player_binding.actor.pointer||
       !guest_camera_stamp_matches(s->base,&r->actor,1)||!guest_camera_stamp_matches(s->base,&r->conversation,1)||
       !runtime_scene_voice_conversation_live(s->base,r->conversation.pointer,r->sequence.pointer,m->resource_id))return 0;
    GuestCameraStamp actor={0},channel={0},matinee={0},matinee_channel={0};
    if(!runtime_scene_voice_channel(s->base,&r->scope,m->speaker_channel,"BioPawn",&actor,&channel)||
       !guest_camera_stamp_core_equal(&actor,&r->actor)||
       !runtime_scene_voice_channel(s->base,&r->scope,m->matinee_channel,"MatineeActor",&matinee,&matinee_channel)||
       !guest_camera_stamp_core_equal(&matinee,&r->matinee)||read_ptr(matinee.pointer+0x270)!=r->action.pointer)return 0;
    uint32_t sex=0;if(!read_at(r->host_shepard.pointer+0x578,&sex,4)||((sex&0x20)!=0)!=((m->flags&1)!=0))return 0;
    return !starting||runtime_scene_voice_line_exists(s->base,r->conversation.pointer,r->strref,(m->flags&4)!=0);
}
static int runtime_scene_voice_ready(void) {
    return runtime_scene_interp_hook_live(&runtime_scene_voice.key)&&runtime_scene_voice_playback_ready()&&runtime_scene_voice_sex_live();
}
static int runtime_scene_voice_bootstrap_ready(void) {
    return runtime_scene_interp_hook_live(&runtime_scene_voice.key)&&runtime_scene_voice_playback_ready();
}
static void runtime_scene_voice_tick(uintptr_t base,uintptr_t engine,DWORD now) {
    DWORD error=GetLastError();RuntimeSceneVoice *s=&runtime_scene_voice;s->base=base;s->engine=engine;
    presentation_voice_receive=runtime_scene_voice_receive;runtime_scene_voice_playback_request_live=runtime_scene_voice_request_live;
    runtime_scene_voice_playback_authored_player=runtime_scene_voice_authored_player;
    runtime_scene_interp_voice_ready=runtime_scene_voice_ready;
    runtime_scene_interp_voice_bootstrap_ready=runtime_scene_voice_bootstrap_ready;
    if(GetCurrentThreadId()!=session_game_thread)goto done;
    if(!s->attempted&&runtime_control_enabled&&runtime_scope_ready&&runtime_scene_interp_enabled()){
        static const unsigned char key[]={0x48,0x83,0xec,0x28,0x49,0x8b,0xc8,0xe8,0x64,0x5d,0xff,0xff,0x48,0x85,0xc0,0x74,0x1e,0x48,0x8b,0x48,0x64,0x48,0x85,0xc9,0x74,0x15,0x8b,0x81,0x1c,0x03,0x00,0x00};
        s->attempted=1;
        if(supported_image(base)&&runtime_scene_interp_hook_install(base,&s->key,0xa4ac60,key,(void*)runtime_scene_voice_key))runtime_scene_voice_note("native-speech-key-prepared");
        else runtime_scene_voice_note("native-speech-key-refused");
    }
    if(g_runtime_config.role==ME3_ROLE_HOST){
        if(s->polled&&now-s->polled<50)goto done;s->polled=now;
        RuntimeSceneScope sex_scope={0};uint32_t sex=0;
        RuntimeSceneInterp *transport=&runtime_scene_interp;
        if(runtime_scene_host_nonce&&transport->voice_capability_valid&&now-transport->voice_received<=2000&&
           runtime_scene_scope_live(base,&transport->voice_capability)&&runtime_scene_host_peer(base,session_context(engine),&sex_scope)&&
           guest_camera_stamp_core_equal(&sex_scope.connection,&transport->voice_capability.connection)&&
           session_actor_live(base,session_shepard)&&is_kind_of(base,session_shepard,"SFXPawn_Player")&&
           read_ptr(session_shepard+0x198)==sex_scope.world.pointer&&read_at(session_shepard+0x578,&sex,4)){
            unsigned female=(sex&0x20)!=0;
            if(s->sex_sent.kind!='H'||s->sex_sent.scene_nonce!=runtime_scene_host_nonce||s->sex_sent.flags!=female){
                if(s->sex_sent.generation!=UINT64_MAX)s->sex_sent=(SceneVoiceMessage){'H',runtime_scene_host_nonce,s->sex_sent.generation+1,0,0,0,0,female};
                s->sex_sent_at=0;
            }
            if(!s->sex_sent_at||now-s->sex_sent_at>=500){wchar_t text[SCENE_VOICE_MESSAGE_CAPACITY];int n=scene_voice_format(&s->sex_sent,text);
                if(n&&presentation_send_text(sex_scope.peer.pointer,1,text,n))s->sex_sent_at=now;}
        }
        SceneVoiceMessage m={0};int live=runtime_scene_voice_host_line(&m);
        int changed=live?(s->sent.kind!='P'||s->sent.scene_nonce!=m.scene_nonce||s->sent.matinee_channel!=m.matinee_channel||
            s->sent.speaker_channel!=m.speaker_channel||s->sent.resource_id!=m.resource_id||s->sent.strref!=m.strref||s->sent.flags!=m.flags||
            s->host.serial!=s->consumed):s->sent.kind=='P';
        if(changed){if(s->generation==UINT64_MAX)goto done;++s->generation;s->consumed=s->host.serial;
            if(live){m.generation=s->generation;s->sent=m;}
            else{s->sent=(SceneVoiceMessage){'S',s->sent.scene_nonce,s->generation,0,0,0,0,0};}s->sent_at=0;}
        if(s->sent.kind&&(!s->sent_at||now-s->sent_at>=100)&&runtime_scene_scope_live(base,&s->host.scope)){
            wchar_t text[SCENE_VOICE_MESSAGE_CAPACITY];int n=scene_voice_format(&s->sent,text);
            if(n&&presentation_send_text(s->host.scope.peer.pointer,1,text,n))s->sent_at=now;}
        goto done;
    }
    if(g_runtime_config.role!=ME3_ROLE_GUEST)goto done;
    if(runtime_scene_voice_sex_live())runtime_scene_voice_host_sex(s->sex);
    if(!s->valid||s->received.kind=='S'||now-s->received_at>500||!runtime_scene_scope_live(base,&s->received_scope)){
        if(s->have_request)runtime_scene_voice_playback_end(s->request.scene_nonce,s->request.line_generation);
        s->have_request=0;goto playback;
    }
    if(s->received.generation==s->consumed)goto playback;
    if(!runtime_scene_voice_ready()||!runtime_scene_voice_proof_live(&s->guest)||s->guest.index!=(int)s->received.matinee_channel||
       now-s->guest.observed>1000||!runtime_scene_player_live())goto playback;
    RuntimeSceneVoiceRequest request={0};SceneVoiceMessage *m=&s->received;GuestCameraStamp actor_channel={0};
    request.scope=s->received_scope;request.action=s->guest.action;request.matinee=s->guest.matinee;
    request.matinee_channel=s->guest.channel;request.sequence=s->guest.sequence;request.host_shepard=runtime_scene_player_binding.actor;
    request.scene_nonce=m->scene_nonce;request.line_generation=m->generation;request.strref=(int)m->strref;request.flags=m->flags;
    uintptr_t conv=runtime_scene_voice_find_conversation(&s->guest,(int)m->resource_id);
    if(!conv||!guest_camera_stamp_read(base,conv,&request.conversation,1)||
       !runtime_scene_voice_channel(base,&request.scope,m->speaker_channel,"BioPawn",&request.actor,&actor_channel)||
       !runtime_scene_voice_host_sex(m->flags&1)||!runtime_scene_voice_request_live(&request,1))goto playback;
    if(runtime_scene_voice_playback_enqueue(&request)){s->request=request;s->have_request=1;s->consumed=m->generation;runtime_scene_voice_note("authenticated-line-enqueued");}
 playback:runtime_scene_voice_playback_tick(base,engine,now);
 done:SetLastError(error);
}
#ifdef ME3_RUNTIME_FIXTURE
static int runtime_scene_voice_fixture_calls,runtime_scene_voice_fixture_seen,runtime_scene_voice_fixture_bad;
static void runtime_scene_voice_fixture_key(void *track,int key,void *instance) {
    if(track!=(void*)16||key!=3||instance!=(void*)24||GetLastError()!=37)runtime_scene_voice_fixture_bad=1;
    ++runtime_scene_voice_fixture_calls;SetLastError(53);
}
static void runtime_scene_voice_fixture_observe(void *track,int key,void *instance) {
    if(track!=(void*)16||key!=3||instance!=(void*)24||runtime_scene_voice_fixture_calls!=1||GetLastError()!=53)
        runtime_scene_voice_fixture_bad=1;
    ++runtime_scene_voice_fixture_seen;SetLastError(11);
}
static int runtime_scene_voice_selftest(void) {
    DWORD error=GetLastError();runtime_scene_voice_fixture_calls=runtime_scene_voice_fixture_seen=runtime_scene_voice_fixture_bad=0;
    SetLastError(37);runtime_scene_voice_key_forward((void*)16,3,(void*)24,runtime_scene_voice_fixture_key,runtime_scene_voice_fixture_observe);
    int ok=runtime_scene_voice_fixture_calls==1&&runtime_scene_voice_fixture_seen==1&&!runtime_scene_voice_fixture_bad&&GetLastError()==53;
    SetLastError(error);return ok&&runtime_scene_voice_playback_selftest();
}
#endif
#endif
