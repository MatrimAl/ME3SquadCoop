#ifndef ME3_RUNTIME_DIALOGUE_CHOICES_H
#define ME3_RUNTIME_DIALOGUE_CHOICES_H
#include "dialogue_choice_protocol.h"
static int runtime_dialogue_native_bytes(uintptr_t,uintptr_t,const unsigned char*,size_t);
#include "runtime_dialogue_choice_pins.h"
/* Mirror the real host wheel and carry advisory selections only. No local
 * conversation/controller, input event, plot choice or game mode is created. */
#pragma pack(push,4)
typedef struct RuntimeDialogueOptions {struct {GameArray text;int32_t mode;} row[6];} RuntimeDialogueOptions;
#pragma pack(pop)
_Static_assert(sizeof(RuntimeDialogueOptions)==120,"native six-option ABI");
typedef void (*RuntimeDialogueNativeUpdate)(void*);
typedef void (*RuntimeDialogueArrowUpdate)(void*,int);
typedef struct RuntimeDialogueRecord {uintptr_t registry,list,record;int index;} RuntimeDialogueRecord;
typedef struct RuntimeDialoguePanel {
    GuestCameraStamp panel,pc,world;
    RuntimeDialogueRecord record;
    unsigned owned;
    uint32_t baseline[5],written[5];
    uint64_t generation,arrow_generation;
    int active,options;
} RuntimeDialoguePanel;
typedef struct RuntimeDialogueChoices {
    uintptr_t base;
    GuestPlayerContextMethod script[12],ui[8],keys[2],full,need;
    RuntimeSceneScope host_scope,guest_scope;
    GuestCameraStamp controller,data,host_panel;
    DialogueChoiceCatalog host,guest;
    DialogueChoiceStage stage;
    DialogueChoicePacket pending_host,vote;
    RuntimeDialoguePanel lease;
    RuntimeDialogueNativeUpdate original_update;
    unsigned char original_entry[16],hook_entry[16],trampoline_entry[64];
    DWORD prepared_at,guest_received,sent_at,vote_sent,panel_scanned;
    uint64_t generation,suppressed_generation;
    unsigned row,offset,events,vote_attempts;
    int prepared,installed,busy,active_host,active_guest,send_catalog,send_close,scene_suspended;
    int pending_host_valid,advice_slot,vote_pending,vote_acked,host_marked;
    const char *prepare_step;
    char last_refused[180];
} RuntimeDialogueChoices;
static RuntimeDialogueChoices runtime_dialogue;
static int runtime_dialogue_native_bytes(uintptr_t base,uintptr_t rva,const unsigned char *expected,size_t size) {
    RuntimeDialogueChoices *q=&runtime_dialogue;unsigned char actual[1024],entry[16],trampoline[64];
    if(size>sizeof actual)return 0;
    if(rva==0xbb79b0&&q->installed) {
        if(base!=q->base||size<16||!q->original_update||
           !read_at(base+rva,entry,sizeof entry)||memcmp(entry,q->hook_entry,sizeof entry)||
           !read_at((uintptr_t)q->original_update,trampoline,sizeof trampoline)||memcmp(trampoline,q->trampoline_entry,sizeof trampoline))return 0;
        memcpy(actual,q->original_entry,16);
        if(!read_at(base+rva+16,actual+16,size-16))return 0;
    } else if(!read_at(base+rva,actual,size))return 0;
    return !memcmp(actual,expected,size);
}
static int runtime_dialogue_key_body(uintptr_t base,uintptr_t fn,int enabled) {
    unsigned char code[]={0x14,0x2d,1,0,0,0,0,0,0,0,0,0x27,4,0x0b,0x53};
    const GuestEquipmentOperand ops[]={{3,"BoolProperty","SFXGame.SFXGUIMovie.m_bHandleKeyPresses",0x2e8,1},{0,NULL,NULL,0,0}};
    const GuestEquipmentName names[]={{0,NULL}};GameArray script={0};uintptr_t field=0;uint64_t mask=0;
    if(!enabled)code[11]=0x28;
    return guest_equipment_body(base,fn,15,code,sizeof code,ops,names)&&
        read_at(fn+0x7c,&script,sizeof script)&&read_at(script.data+3,&field,8)&&
        read_at(field+0xd0,&mask,8)&&mask==0x80;
}
static const char *runtime_dialogue_script_names[]={"AS_SetConversationOptions","SelectEntry","QueueEntry","SelectConversationEntry",
    "SkipNode","SelectInterrupt","AS_HideWheel","SkipConversation","InterruptParagon","InterruptRenegade","HandleInputEvent","SelectConversationSegment"};
static int runtime_dialogue_enabled(void) {
    static int value=-1;if(value<0){wchar_t text[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_DIALOGUE_ADVICE",text,8);
        value=!n||(n==1&&text[0]==L'1');}return value;
}
static void runtime_dialogue_note(const char *event) {
    if(runtime_dialogue.events++>=80)return;char row[360];
    snprintf(row,sizeof row,"{\"runtime_dialogue_choices\":\"%s\",\"advisory_only\":true,\"story_selection_written\":false,\"full_scene_parity_verified\":false,\"monotonic_ms\":%lu}\r\n",event,GetTickCount());line(row);
}
static int runtime_dialogue_method(uintptr_t base,const char *owner,const char *name,uint32_t flags,
        unsigned size,unsigned ret,unsigned count,uintptr_t native,GuestPlayerContextMethod *out) {
    uintptr_t fn=function_named(base,owner,name);
    return guest_player_context_method_stamp(base,fn,out)&&
        guest_player_context_function_abi(base,fn,owner,name,flags,(uint16_t)size,(uint16_t)ret,0,native,(unsigned char)count);
}
static int runtime_dialogue_method_live(uintptr_t base,const GuestPlayerContextMethod *m,uintptr_t native) {
    return guest_player_context_method_matches(base,m,0)&&read_ptr(m->function.pointer+0xd8)==base+native;
}
static int runtime_dialogue_object_pe(uintptr_t base,uintptr_t object) {
    return is_live(base,object)&&read_ptr(read_ptr(object)+0x230)==base+0xf4f80&&guest_equipment_dispatch_pins(base);
}
static int runtime_dialogue_ui_value(uintptr_t panel,unsigned bit,int set,uint32_t *value) {
    RuntimeDialogueChoices *q=&runtime_dialogue;
    if(bit>=5||!runtime_dialogue_object_pe(q->base,panel))return 0;
    if(bit==4) {
        uint32_t flags=0;
        if(!runtime_dialogue_method_live(q->base,&q->keys[set&&*value?0:1],0xf52b0)||
           !runtime_dialogue_key_body(q->base,q->keys[set&&*value?0:1].function.pointer,set&&*value))return 0;
        if(set)((ProcessEvent)(q->base+0xf4f80))((void*)panel,(void*)q->keys[*value?0:1].function.pointer,NULL,NULL);
        else {if(!read_at(panel+0x2e8,&flags,4))return 0;*value=!!(flags&0x80);}return 1;
    }
    unsigned method=bit*2+(set?0:1);
    static const uintptr_t entry[]={0xb66c60,0xb66dd0,0xb671a0,0xb67280,0xb67350,0xb67420,0xb668d0,0xb66b40};
    if(!runtime_dialogue_method_live(q->base,&q->ui[method],entry[method]))return 0;
    uint32_t parameter=set?!!*value:0;
    ((ProcessEvent)(q->base+0xf4f80))((void*)panel,(void*)q->ui[method].function.pointer,&parameter,NULL);
    if(!set){if(parameter>1)return 0;*value=parameter;}return 1;
}
static int runtime_dialogue_query_full(uintptr_t base,uintptr_t info,uintptr_t *out) {
    RuntimeDialogueChoices *q=&runtime_dialogue;uintptr_t manager=read_ptr(info+0x948),result=0;
    GuestCameraStamp manager_stamp={0},world_stamp={0};
    if(!narrative_probe_object(base,manager,"BioConversationManager")||!runtime_dialogue_object_pe(base,manager)||
       !guest_camera_stamp_read(base,manager,&manager_stamp,0)||!guest_camera_stamp_read(base,info,&world_stamp,0)||
       !runtime_dialogue_method_live(base,&q->full,0xb51e40))return 0;
    ((ProcessEvent)(base+0xf4f80))((void*)manager,(void*)q->full.function.pointer,&result,NULL);
    if(!guest_camera_stamp_matches(base,&manager_stamp,0)||!guest_camera_stamp_matches(base,&world_stamp,0)||
       read_ptr(info+0x948)!=manager||(result&&(!narrative_probe_object(base,result,"BioConversationController")||
       read_ptr(result+0x28c)!=manager)))return 0;
    *out=result;return 1;
}
static int runtime_dialogue_needs_replies(uintptr_t base,uintptr_t controller) {
    RuntimeDialogueChoices *q=&runtime_dialogue;uint32_t result=0;
    if(!runtime_dialogue_object_pe(base,controller)||!narrative_probe_object(base,controller,"BioConversationController")||
       !runtime_dialogue_method_live(base,&q->need,0x4ca070))return 0;
    ((ProcessEvent)(base+0xf4f80))((void*)controller,(void*)q->need.function.pointer,&result,NULL);return result==1;
}
static int runtime_dialogue_panel_owned(uintptr_t base,uintptr_t panel,uintptr_t pc,uintptr_t info) {
    uintptr_t engine=read_ptr(base+0x18b31d0);GameArray players={0};int32_t index=-1;
    return narrative_probe_object(base,panel,"BioSFHandler_PCConversation")&&read_ptr(panel)==base+0x13ac170&&
        runtime_dialogue_object_pe(base,panel)&&read_ptr(panel+0x2bc)==info&&read_ptr(panel+0x318)==panel&&
        read_at(panel+0x1b8,&index,4)&&identity_array(engine+0x498,&players,4)&&index>=0&&index<players.count&&
        read_ptr(read_ptr(players.data+(SIZE_T)index*8)+0x68)==pc;
}
/* Discovery runs on bounded Tick preparation, never on every script call. */
static uintptr_t runtime_dialogue_find_panel(uintptr_t base,uintptr_t pc,uintptr_t info,RuntimeDialogueRecord *record) {
    uintptr_t registry=read_ptr(base+0x18b6be0),found=0;GameArray lists[2]={{0}};
    if(!registry||!runtime_power_wheel_registry_pins(base))return 0;
    const int offsets[]={0x4c,0xbc};
    for(unsigned l=0;l<2;++l){if(!identity_array(registry+offsets[l],&lists[l],128))return 0;
        for(int i=0;i<lists[l].count;++i){uintptr_t entry=read_ptr(lists[l].data+(SIZE_T)i*8),panel=read_ptr(entry+0x5c);
            if(!runtime_dialogue_panel_owned(base,panel,pc,info))continue;
            if(found&&found!=panel)return 0;found=panel;
            *record=(RuntimeDialogueRecord){registry,registry+offsets[l],entry,i};
        }}
    return found;
}
static int runtime_dialogue_panel_current(uintptr_t base,const RuntimeDialoguePanel *p) {
    GameArray list={0},again={0};uintptr_t entry=0,movie=0;
    return p->active&&guest_camera_stamp_matches(base,&p->panel,0)&&guest_camera_stamp_matches(base,&p->pc,0)&&
        guest_camera_stamp_matches(base,&p->world,0)&&read_ptr(base+0x18b6be0)==p->record.registry&&
        scene_readiness_array(p->record.list,&list,128)&&p->record.index>=0&&p->record.index<list.count&&
        read_at(list.data+(SIZE_T)p->record.index*8,&entry,8)&&entry==p->record.record&&
        read_at(entry+0x5c,&movie,8)&&movie==p->panel.pointer&&
        runtime_dialogue_panel_owned(base,p->panel.pointer,p->pc.pointer,p->world.pointer)&&
        read_at(p->record.list,&again,sizeof again)&&!memcmp(&list,&again,sizeof list)&&
        read_ptr(list.data+(SIZE_T)p->record.index*8)==entry&&read_ptr(entry+0x5c)==movie;
}
static int runtime_dialogue_guest_local_free(void) {
    RuntimeDialogueChoices *q=&runtime_dialogue;uintptr_t full=0;
    return runtime_dialogue_query_full(q->base,q->lease.world.pointer,&full)&&!full;
}
static int runtime_dialogue_guest_fresh(DWORD now) {
    RuntimeDialogueChoices *q=&runtime_dialogue;
    return q->active_guest&&runtime_dialogue_enabled()&&now-q->guest_received<=1000&&
        runtime_scene_camera_requested()&&runtime_scene_view.message.nonce==q->guest.nonce&&
        runtime_scene_scope_live(q->base,&q->guest_scope)&&runtime_scene_scope_live(q->base,&runtime_scene_view.scope)&&
        q->guest_scope.pc.pointer==runtime_scene_view.scope.pc.pointer&&
        q->guest_scope.channel.pointer==runtime_scene_view.scope.channel.pointer;
}
static int runtime_dialogue_options(uintptr_t panel,const DialogueChoiceCatalog *catalog,int marker) {
    RuntimeDialogueChoices *q=&runtime_dialogue;RuntimeDialogueOptions parameter={0};wchar_t text[6][DIALOGUE_CHOICE_TEXT+16];
    if(!dialogue_choice_catalog_valid(catalog)||!runtime_dialogue_object_pe(q->base,panel)||
       !runtime_dialogue_method_live(q->base,&q->script[0],0xf52b0)||!runtime_dialogue_body(q->base,q->script[0].function.pointer,0))return 0;
    for(unsigned i=0;i<6;++i){unsigned at=0;if((int)i==marker){const wchar_t prefix[]=L"[Ashley] ";memcpy(text[i],prefix,18);at=9;}
        memcpy(text[i]+at,catalog->rows[i].text,catalog->rows[i].length*2);at+=catalog->rows[i].length;text[i][at]=0;
        parameter.row[i].text=(GameArray){(uintptr_t)text[i],(int)at+1,(int)at+1};parameter.row[i].mode=(int)catalog->rows[i].mode;}
    ++q->busy;((ProcessEvent)(q->base+0xf4f80))((void*)panel,(void*)q->script[0].function.pointer,&parameter,NULL);--q->busy;return 1;
}
static void runtime_dialogue_lease_retire(void) {
    RuntimeDialogueChoices *q=&runtime_dialogue;RuntimeDialoguePanel *p=&q->lease;
    if(!p->active)return;
    int states[]={runtime_scene_camera_stamp_state(q->base,&p->panel),
        runtime_scene_camera_stamp_state(q->base,&p->pc),runtime_scene_camera_stamp_state(q->base,&p->world)};
    for(unsigned i=0;i<3;++i)if(states[i]<0){memset(p,0,sizeof *p);return;}
    for(unsigned i=0;i<3;++i)if(!states[i])return;
    uintptr_t full=0;
    if(!runtime_dialogue_query_full(q->base,p->world.pointer,&full))return;
    if(full){memset(p,0,sizeof *p);return;} /* Real native conversation wins. */
    if(!runtime_dialogue_panel_current(q->base,p)) {
        RuntimeDialogueRecord record={0};uintptr_t found=runtime_dialogue_find_panel(q->base,p->pc.pointer,p->world.pointer,&record);
        if(found!=p->panel.pointer||!guest_camera_stamp_matches(q->base,&p->panel,0))return;
        p->record=record;if(!runtime_dialogue_panel_current(q->base,p))return;
    }
    if(p->options&&runtime_dialogue_method_live(q->base,&q->script[6],0xf52b0)&&
       runtime_dialogue_body(q->base,q->script[6].function.pointer,6)) {
        ++q->busy;((ProcessEvent)(q->base+0xf4f80))((void*)p->panel.pointer,(void*)q->script[6].function.pointer,NULL,NULL);--q->busy;
    }
    for(int i=4;i>=0;--i)if(p->owned&(1u<<i)) {
        if(!runtime_dialogue_panel_current(q->base,p)||!runtime_dialogue_guest_local_free())return;
        uint32_t current=0;
        if(!runtime_dialogue_ui_value(p->panel.pointer,(unsigned)i,0,&current))return;
        if(!runtime_dialogue_panel_current(q->base,p)||!runtime_dialogue_guest_local_free())return;
        if(current==p->written[i]){uint32_t old=p->baseline[i];
            if(!runtime_dialogue_ui_value(p->panel.pointer,(unsigned)i,1,&old)||
               !runtime_dialogue_panel_current(q->base,p)||!runtime_dialogue_guest_local_free()||
               !runtime_dialogue_ui_value(p->panel.pointer,(unsigned)i,0,&current)||
               !runtime_dialogue_panel_current(q->base,p)||!runtime_dialogue_guest_local_free())return;
            if(current!=p->baseline[i])return;}
        p->owned&=~(1u<<i);
    }
    memset(p,0,sizeof *p);runtime_dialogue_note("guest-native-panel-released");
}
static int runtime_dialogue_lease_begin(uintptr_t pc,uintptr_t info,const DialogueChoiceCatalog *catalog) {
    RuntimeDialogueChoices *q=&runtime_dialogue;RuntimeDialoguePanel *p=&q->lease;
    if(p->active){if(!runtime_dialogue_panel_current(q->base,p)||!runtime_dialogue_guest_local_free())return 0;
        if(!runtime_dialogue_options(p->panel.pointer,catalog,-1)||!runtime_dialogue_panel_current(q->base,p)||
           !runtime_dialogue_guest_local_free())return 0;
        p->options=1;p->generation=catalog->generation;return 1;}
    RuntimeDialogueRecord record={0};uintptr_t panel=runtime_dialogue_find_panel(q->base,pc,info,&record),full=0;
    if(!panel||!runtime_dialogue_query_full(q->base,info,&full)||full)return 0;
    RuntimeDialoguePanel next={0};
    if(!guest_camera_stamp_read(q->base,panel,&next.panel,0)||!guest_camera_stamp_read(q->base,pc,&next.pc,0)||
       !guest_camera_stamp_read(q->base,info,&next.world,0))return 0;
    next.record=record;next.active=1;*p=next;
    for(unsigned i=0;i<5;++i)if(!runtime_dialogue_ui_value(panel,i,0,p->baseline+i)||
        !runtime_dialogue_panel_current(q->base,p)||!runtime_dialogue_guest_local_free()) {
        memset(p,0,sizeof *p);return 0;
    }
    for(unsigned i=0;i<5;++i){uint32_t desired=1,current=0;
        if(p->baseline[i])continue;
        p->owned|=1u<<i;p->written[i]=1;
        if(!runtime_dialogue_panel_current(q->base,p)||!runtime_dialogue_guest_local_free()||
           !runtime_dialogue_ui_value(panel,i,1,&desired)||!runtime_dialogue_panel_current(q->base,p)||
           !runtime_dialogue_guest_local_free()||!runtime_dialogue_ui_value(panel,i,0,&current)||current!=1){runtime_dialogue_lease_retire();return 0;}}
    if(!runtime_dialogue_panel_current(q->base,p)||!runtime_dialogue_guest_local_free()||
       !runtime_dialogue_options(panel,catalog,-1)||!runtime_dialogue_panel_current(q->base,p)||
       !runtime_dialogue_guest_local_free()){runtime_dialogue_lease_retire();return 0;}
    p->options=1;p->generation=catalog->generation;runtime_dialogue_note("guest-native-wheel-presented");return 1;
}
static int runtime_dialogue_host_current(uintptr_t base,uintptr_t *controller,uintptr_t *data,int32_t *entry) {
    RuntimeDialogueChoices *q=&runtime_dialogue;SessionContext c=session_context(session_engine);uintptr_t full=0;uint32_t flags=0;
    if(g_runtime_config.role!=ME3_ROLE_HOST||!runtime_dialogue_enabled()||!runtime_scene_scope_live(base,&q->host_scope)||
       q->host_scope.pc.pointer!=c.pc||q->host_scope.world.pointer!=c.world||
       !runtime_dialogue_query_full(base,c.world,&full)||!full||!read_at(full+0x358,&flags,4)||
       (flags&(2|0x80|0x1000|0x2000|0x8000))||!runtime_dialogue_needs_replies(base,full))return 0;
    uintptr_t player=read_ptr(full+0x29c),conversation=read_ptr(full+0x284);int32_t current=-1;
    if(!runtime_ambient_conversation_layout(base,full)||
       !runtime_power_property(base,full,"BioConversationController","m_pPlayer","ObjectProperty",0x29c,8)||
       !runtime_power_property(base,full,"BioConversationController","m_nCurrentEntry","IntProperty",0x2ec,4)||
       !guest_camera_actor_live(base,player)||!is_kind_of(base,player,"SFXPawn_Player")||read_ptr(player+0x198)!=c.world||
       !narrative_probe_object(base,conversation,"BioConversation")||!read_at(full+0x2ec,&current,4)||current<0)return 0;
    *controller=full;*data=conversation;*entry=current;return 1;
}
static int runtime_dialogue_catalog_native_current(uintptr_t base) {
    RuntimeDialogueChoices *q=&runtime_dialogue;uintptr_t controller=0,data=0;int32_t entry=-1;int32_t slots[6];
    return q->active_host&&dialogue_choice_catalog_valid(&q->host)&&
        guest_camera_stamp_matches(base,&q->controller,0)&&guest_camera_stamp_matches(base,&q->data,0)&&
        guest_camera_stamp_matches(base,&q->host_panel,0)&&runtime_dialogue_host_current(base,&controller,&data,&entry)&&
        controller==q->controller.pointer&&data==q->data.pointer&&entry==q->host.entry&&
        runtime_dialogue_panel_owned(base,q->host_panel.pointer,q->host_scope.pc.pointer,q->host_scope.world.pointer)&&
        read_at(q->host_panel.pointer+0x364,slots,sizeof slots)&&
        slots[0]==q->host.rows[0].reply&&slots[1]==q->host.rows[1].reply&&slots[2]==q->host.rows[2].reply&&
        slots[3]==q->host.rows[3].reply&&slots[4]==q->host.rows[4].reply&&slots[5]==q->host.rows[5].reply;
}
static int runtime_dialogue_frame(uintptr_t self,uintptr_t frame,unsigned kind,GameArray *script,uintptr_t *params) {
    RuntimeDialogueChoices *q=&runtime_dialogue;
    static const unsigned sizes[]={37,166,24,639,142,172,24,62,53,54,365,63};
    if(kind>=12||read_ptr(frame+0x18)!=q->script[kind].function.pointer||read_ptr(frame+0x20)!=self||
       !runtime_dialogue_method_live(q->base,&q->script[kind],0xf52b0)||
       !read_at(q->script[kind].function.pointer+0x7c,script,sizeof *script)||!script->data||
       script->count!=(int)sizes[kind]||script->max<script->count||script->max>4096||
       read_ptr(frame+0x28)!=script->data)return 0;
    if(params){*params=read_ptr(frame+0x30);if(!*params)return 0;}return 1;
}
static int runtime_dialogue_catalog_same(const DialogueChoiceCatalog *a,const DialogueChoiceCatalog *b) {
    if(a->entry!=b->entry||a->digest!=b->digest)return 0;
    for(unsigned i=0;i<6;++i)if(a->rows[i].reply!=b->rows[i].reply||a->rows[i].mode!=b->rows[i].mode||
       a->rows[i].length!=b->rows[i].length||memcmp(a->rows[i].text,b->rows[i].text,a->rows[i].length*2))return 0;
    return 1;
}
static void runtime_dialogue_host_capture(uintptr_t self,uintptr_t frame) {
    RuntimeDialogueChoices *q=&runtime_dialogue;SessionContext c=session_context(session_engine);RuntimeSceneScope scope={0};
    GameArray script={0};uintptr_t params=0,controller=0,data=0;int32_t entry=-1,slots[6],again[6];
    if(q->busy||g_runtime_config.role!=ME3_ROLE_HOST||!runtime_scene_host_peer(q->base,c,&scope)||
       !runtime_dialogue_panel_owned(q->base,self,c.pc,c.world)||!runtime_dialogue_frame(self,frame,0,&script,&params))return;
    if(!runtime_power_property_dim(q->base,self,"BioSFHandler_Conversation","m_nSlotsUsed","IntProperty",0x364,4,6))return;
    if(!runtime_scene_host_nonce||!runtime_scene_scope_live(q->base,&runtime_scene_host_scope)||
       scope.channel.pointer!=runtime_scene_host_scope.channel.pointer||scope.connection.pointer!=runtime_scene_host_scope.connection.pointer)return;
    q->host_scope=scope;
    if(!runtime_dialogue_host_current(q->base,&controller,&data,&entry)||!read_at(self+0x364,slots,sizeof slots))return;
    GuestCameraStamp initial_controller={0},initial_data={0};uintptr_t original_controller=controller,original_data=data;
    if(!guest_camera_stamp_read(q->base,controller,&initial_controller,0)||!guest_camera_stamp_read(q->base,data,&initial_data,0))return;
    DialogueChoiceCatalog catalog={0};catalog.nonce=runtime_scene_host_nonce;
    catalog.generation=q->generation+1;catalog.entry=entry;
    for(unsigned i=0;i<6;++i){GameArray text={0};int32_t mode=-1;
        if(!read_at(params+i*20,&text,sizeof text)||!read_at(params+i*20+16,&mode,4)||mode<0||mode>4||
           text.count<0||text.count>513||text.max<text.count||text.max>4096||(text.count&&!text.data))return;
        DialogueChoiceRow *r=catalog.rows+i;r->reply=slots[i];r->mode=(unsigned)mode;
        if(text.count){uint16_t copy[513];if(!read_at(text.data,copy,(SIZE_T)text.count*2)||copy[text.count-1])return;
            r->length=(unsigned)text.count-1;memcpy(r->text,copy,r->length*2);}
        GameArray check={0};if(!read_at(params+i*20,&check,sizeof check)||memcmp(&check,&text,sizeof text))return;
    }
    catalog.digest=dialogue_choice_digest(&catalog);
    if(!dialogue_choice_catalog_valid(&catalog)||!read_at(self+0x364,again,sizeof again)||memcmp(slots,again,sizeof slots)||
       !runtime_dialogue_host_current(q->base,&controller,&data,&entry)||entry!=catalog.entry||
       controller!=original_controller||data!=original_data||!guest_camera_stamp_matches(q->base,&initial_controller,0)||
       !guest_camera_stamp_matches(q->base,&initial_data,0)||!runtime_scene_scope_live(q->base,&q->host_scope)||
       catalog.nonce!=runtime_scene_host_nonce)return;
    GuestCameraStamp cc={0},dd={0},pp={0};
    if(!guest_camera_stamp_read(q->base,controller,&cc,0)||!guest_camera_stamp_read(q->base,data,&dd,0)||
       !guest_camera_stamp_read(q->base,self,&pp,0))return;
    int same=q->active_host&&q->controller.pointer==controller&&q->data.pointer==data&&q->host_panel.pointer==self&&
        guest_camera_stamp_matches(q->base,&q->controller,0)&&guest_camera_stamp_matches(q->base,&q->data,0)&&
        runtime_dialogue_catalog_same(&q->host,&catalog)&&q->host.nonce==catalog.nonce;
    if(same){if(q->advice_slot>=0)q->host_marked=0;return;}
    if(q->generation==UINT64_MAX)return;
    catalog.generation=++q->generation;q->host=catalog;q->controller=cc;q->data=dd;q->host_panel=pp;
    q->active_host=1;q->scene_suspended=0;q->send_catalog=1;q->send_close=0;q->row=q->offset=0;q->advice_slot=-1;q->host_marked=0;
    runtime_dialogue_note("host-native-options-captured");
}
static int runtime_dialogue_send(const RuntimeSceneScope *scope,const DialogueChoicePacket *packet,int direction) {
    RuntimeDialogueChoices *q=&runtime_dialogue;wchar_t wire[192];int count=dialogue_choice_format(packet,wire);
    return count&&runtime_scene_scope_live(q->base,scope)&&presentation_send_text(scope->peer.pointer,direction,wire,count);
}
/* Native owner-PC receive provenance survives scene unpossession. Only scalar
 * staging/queueing occurs here; rendering and all engine calls stay on Tick. */
static int runtime_dialogue_receive(uintptr_t self,uintptr_t frame,const wchar_t *text,size_t count) {
    RuntimeDialogueChoices *q=&runtime_dialogue;NativeRpcReceiveFrame *r=native_rpc_receive_current(session_game_thread);
    DialogueChoicePacket packet={0};SessionContext c=session_context(session_engine);NetworkActorScope world={0};
    NetworkActorIdentityEntry source={0};RuntimeSceneScope scope={0};int host=g_runtime_config.role==ME3_ROLE_HOST;
    if(!q->prepared||!runtime_dialogue_enabled()||!r||!r->channel||!dialogue_choice_parse(text,count,&packet)||
       !presentation_method_valid(host?0:1)||read_ptr(frame+0x18)!=presentation_methods[host?0:1].function.pointer||
       read_ptr(frame+0x20)!=self||!runtime_scene_context(q->base,c,&world)||
       !network_actor_channel_valid(q->base,self,r->channel,world.driver,&source))return 1;
    if(host) {
        if(packet.kind!=L'V'||!runtime_scene_host_peer(q->base,c,&scope)||scope.peer.pointer!=self||
           scope.channel.pointer!=source.channel||scope.connection.pointer!=source.connection||
           !q->active_host||!dialogue_choice_vote_matches(&q->host,&packet))return 1;
        if(q->advice_slot>=0&&(unsigned)q->advice_slot!=packet.slot)return 1;
        q->pending_host=packet;q->pending_host_valid=1;return 1;
    }
    if(self!=c.pc||read_ptr(world.driver+0x80)!=source.connection||read_ptr(source.connection+0x68)!=self||
       !runtime_scene_capture(q->base,c,&world,&source,&scope))return 1;
    int same=runtime_scene_scope_live(q->base,&q->guest_scope)&&q->guest_scope.channel.pointer==scope.channel.pointer;
    if(!same){memset(&q->stage,0,sizeof q->stage);q->active_guest=0;q->vote_pending=q->vote_acked=0;}
    q->guest_scope=scope;
    if(packet.kind==L'A') {
        if(q->vote_attempts&&!q->vote_acked&&dialogue_choice_vote_matches(&q->guest,&packet)&&packet.slot==q->vote.slot&&packet.reply==q->vote.reply){
            q->vote_pending=0;q->vote_acked=1;runtime_dialogue_note("host-advice-acknowledged");}
        return 1;
    }
    if(packet.kind!=L'F'&&packet.kind!=L'C'&&packet.kind!=L'Z')return 1;
    DialogueChoiceCatalog catalog={0};int state=dialogue_choice_stage(&q->stage,&packet,&catalog);
    if(state==3){q->active_guest=0;q->vote_pending=q->vote_acked=0;return 1;}
    if(state==2) {
        int changed=!q->active_guest||q->guest.nonce!=catalog.nonce||q->guest.generation!=catalog.generation||
            !runtime_dialogue_catalog_same(&q->guest,&catalog);
        if(catalog.generation==q->suppressed_generation&&q->guest.nonce==catalog.nonce)return 1;
        q->guest=catalog;q->active_guest=1;q->guest_received=GetTickCount();
        if(changed){q->vote_pending=q->vote_acked=0;q->vote_attempts=0;q->panel_scanned=0;
            runtime_dialogue_note("complete-host-catalog-received");}
    }
    return 1;
}
static int runtime_dialogue_field(uintptr_t base,uintptr_t fn,const char *name,const char *kind,int offset,int size,uint64_t flags) {
    uintptr_t field=read_ptr(fn+0x70);char text[100];
    for(unsigned i=0;field&&i<128;++i){if(!is_live(base,field)||read_ptr(field+0x40)!=fn)return 0;
        object_name(base,field,text,sizeof text);
        if(!strcmp(text,name)){GuestCameraStamp stamp={0};char path[300];uint64_t mask=0;
            if(!guest_camera_stamp_read(base,fn,&stamp,1))return 0;
            snprintf(path,sizeof path,"%.190s.%.90s",stamp.full_name,name);
            return runtime_delayed_fire_property(base,field,kind,path,offset,size,flags)&&
                (strcmp(kind,"BoolProperty")|| (read_at(field+0xd0,&mask,8)&&mask==1));}
        field=read_ptr(field+0x68);
    }return 0;
}
static void runtime_dialogue_native_update(void *self);
static int runtime_dialogue_prepare(uintptr_t base,DWORD now) {
    RuntimeDialogueChoices *q=&runtime_dialogue;
    if(q->prepared&&q->base==base&&runtime_dialogue_method_live(base,&q->script[0],0xf52b0)&&
       runtime_dialogue_method_live(base,&q->script[1],0xf52b0))return 1;
    if(q->prepared_at&&now-q->prepared_at<2000)return 0;q->prepared_at=now;q->base=base;
    q->prepare_step="native-entry-pins";
    if(!supported_image(base)||!runtime_dialogue_native_pins(base)||!runtime_dialogue_selected_native_pins(base))return 0;
    const unsigned size[]={120,4,4,8,0,4,0,0,0,0,12,0};
    const unsigned result[]={65535,65535,65535,4,65535,65535,65535,65535,65535,65535,8,65535};
    const unsigned count[]={12,1,1,2,0,1,0,0,0,0,3,0};
    for(unsigned i=0;i<12;++i){uint32_t flags=i<10?runtime_dialogue_flags[i]:(i==10?0x24802:0x20003);
        q->prepare_step=runtime_dialogue_script_names[i];guest_equipment_body_err[0]=0;
        if(!runtime_dialogue_method(base,i==10?"BioSFHandler_PCConversation":"BioSFHandler_Conversation",
            runtime_dialogue_script_names[i],flags,size[i],result[i],count[i],0xf52b0,&q->script[i])||
           !runtime_dialogue_body(base,q->script[i].function.pointer,i))return 0;
    }
    static const char *strings[]={"sMiddleRight","sBottomRight","sBottomLeft","sMiddleLeft","sTopLeft","sTopRight"};
    static const char *modes[]={"nMiddleRightMode","nBottomRightMode","nBottomLeftMode","nMiddleLeftMode","nTopLeftMode","nTopRightMode"};
    q->prepare_step="option-parameter-layout";
    for(unsigned i=0;i<6;++i)if(!runtime_dialogue_field(base,q->script[0].function.pointer,strings[i],"StrProperty",(int)i*20,16,0x400080)||
        !runtime_dialogue_field(base,q->script[0].function.pointer,modes[i],"IntProperty",(int)i*20+16,4,0x80))return 0;
    q->prepare_step="selection-input-parameter-layout";
    for(unsigned i=1;i<=3;++i)if(!runtime_dialogue_field(base,q->script[i].function.pointer,i==3?"nWheelLocation":"nEntry",
        i==3?"ByteProperty":"IntProperty",0,i==3?1:4,0x80))return 0;
    if(!runtime_dialogue_field(base,q->script[3].function.pointer,"ReturnValue","BoolProperty",4,4,0x580)||
       !runtime_dialogue_field(base,q->script[10].function.pointer,"Event","ByteProperty",0,1,0x80)||
       !runtime_dialogue_field(base,q->script[10].function.pointer,"fValue","FloatProperty",4,4,0x90)||
       !runtime_dialogue_field(base,q->script[10].function.pointer,"ReturnValue","BoolProperty",8,4,0x580))return 0;
    const char *names[]={"SetVisible","GetVisible","SetEnabled","GetEnabled","SetInputEnabled","GetInputEnabled","SetMovieFocus","HasFocus"};
    const uintptr_t entries[]={0xb66c60,0xb66dd0,0xb671a0,0xb67280,0xb67350,0xb67420,0xb668d0,0xb66b40};
    for(unsigned i=0;i<8;++i){int getter=i&1;const char *arg=getter?"ReturnValue":(i==0?"bVisible":i==6?"Focus":"bEnabled");
        q->prepare_step=names[i];
        uint32_t flags=i>=4||getter?0x20401:0x20400;
        if(!runtime_dialogue_method(base,"SFXGUIMovie",names[i],flags,4,getter?0:65535,1,entries[i],&q->ui[i])||
           !runtime_dialogue_field(base,q->ui[i].function.pointer,arg,"BoolProperty",0,4,getter?0x580:0x80))return 0;}
    q->prepare_step="native-conversation-getters";
    if(!runtime_dialogue_method(base,"BioConversationManager","GetFullConversation",0x20400,8,0,1,0xb51e40,&q->full)||
       !runtime_dialogue_field(base,q->full.function.pointer,"ReturnValue","ObjectProperty",0,8,0x580)||
       !runtime_dialogue_method(base,"BioConversationController","NeedToDisplayReplies",0x20400,4,0,1,0x4ca070,&q->need)||
       !runtime_dialogue_field(base,q->need.function.pointer,"ReturnValue","BoolProperty",0,4,0x580)||
       !runtime_script_executor_prepare(base))return 0;
    q->prepare_step="native-key-handling-setters";
    for(unsigned i=0;i<2;++i)if(!runtime_dialogue_method(base,"SFXGUIMovie",i?"StopHandlingKeyPresses":"StartHandlingKeyPresses",
        0x20003,0,65535,0,0xf52b0,&q->keys[i])||!runtime_dialogue_key_body(base,q->keys[i].function.pointer,!i))return 0;
    q->prepare_step="native-update-install";
    if(!q->installed){MH_STATUS status=MH_Initialize();if(status!=MH_OK&&status!=MH_ERROR_ALREADY_INITIALIZED)return 0;LPVOID original=NULL;
        if(!read_at(base+0xbb79b0,q->original_entry,16)||
           MH_CreateHook((void*)(base+0xbb79b0),(void*)runtime_dialogue_native_update,&original)!=MH_OK)return 0;
        q->original_update=(RuntimeDialogueNativeUpdate)original;
        if(!original||!read_at((uintptr_t)original,q->trampoline_entry,sizeof q->trampoline_entry)||
           MH_EnableHook((void*)(base+0xbb79b0))!=MH_OK||!read_at(base+0xbb79b0,q->hook_entry,16)){
            MH_DisableHook((void*)(base+0xbb79b0));MH_RemoveHook((void*)(base+0xbb79b0));q->original_update=NULL;return 0;}
        q->installed=1;}
    q->prepared=1;q->advice_slot=-1;q->prepare_step="ready";runtime_dialogue_note("native-wheel-methods-prepared");return 1;
}
static void runtime_dialogue_native_update(void *self) {
    RuntimeDialogueChoices *q=&runtime_dialogue;DWORD before=GetLastError();
    if(q->original_update){SetLastError(before);q->original_update(self);}DWORD after=GetLastError();
    if(!q->busy&&q->prepared&&g_runtime_config.role==ME3_ROLE_GUEST&&GetCurrentThreadId()==session_game_thread&&
       (uintptr_t)self==q->lease.panel.pointer&&runtime_dialogue_panel_current(q->base,&q->lease)&&
       runtime_dialogue_guest_fresh(GetTickCount())&&runtime_dialogue_guest_local_free()&&
       read_ptr(read_ptr((uintptr_t)self)+0x488)==q->base+0xba5b50) {
        uint64_t generation=q->lease.generation;
        int force=q->lease.arrow_generation!=generation;
        ++q->busy;((RuntimeDialogueArrowUpdate)(q->base+0xba5b50))(self,force);--q->busy;
        if(q->lease.generation==generation&&runtime_dialogue_panel_current(q->base,&q->lease))q->lease.arrow_generation=generation;
    }
    SetLastError(after);
}
static int runtime_dialogue_node_match(uintptr_t node) {
    if(!runtime_dialogue.prepared)return 0;
    for(unsigned i=0;i<12;++i)if(node==runtime_dialogue.script[i].function.pointer)return 1;return 0;
}
static int runtime_dialogue_redirect(uintptr_t frame,const GameArray *script,unsigned kind) {
    if(kind>=10||!runtime_dialogue_safe_return[kind])return 0;
    unsigned at=runtime_dialogue_safe_return[kind];unsigned char bytes[2];
    return at+2<=(unsigned)script->count&&read_at(script->data+at,bytes,2)&&bytes[0]==4&&bytes[1]==(kind==3?0x28:0x0b)&&
        runtime_power_action_context_pointer(frame+0x28,script->data,script->data+at);
}
static void runtime_dialogue_vote(unsigned slot) {
    RuntimeDialogueChoices *q=&runtime_dialogue;
    if(slot>=6||q->vote_pending||q->vote_acked||q->vote_attempts||!runtime_dialogue_guest_fresh(GetTickCount()))return;
    DialogueChoicePacket packet=dialogue_choice_packet(&q->guest,L'V',slot,0);
    if(!dialogue_choice_vote_matches(&q->guest,&packet))return;
    q->vote=packet;q->vote_pending=1;q->vote_sent=0;runtime_dialogue_note("guest-advisory-choice-queued");
}
static int runtime_dialogue_ui_ready(void) {
    RuntimeDialogueChoices *q=&runtime_dialogue;
    for(unsigned i=0;i<5;++i){uint32_t value=0;
        if(!runtime_dialogue_panel_current(q->base,&q->lease)||!runtime_dialogue_guest_local_free()||
           !runtime_dialogue_ui_value(q->lease.panel.pointer,i,0,&value)||!value)return 0;}
    return runtime_dialogue_panel_current(q->base,&q->lease)&&runtime_dialogue_guest_local_free();
}
static void runtime_dialogue_stock_or_refuse(void *self,void *frame,void *result,RuntimeAmmoPickupExec stock,
        int selected,int redirected,unsigned kind) {
    if(!selected||redirected){stock(self,frame,result);return;}
    DWORD error=GetLastError();
    /* A leased guest story callback must never fall through because its
     * return cursor could not be changed. Ordinary scripts still forward.
     * ProcessEvent owns local-frame teardown outside this executor. */
    if(kind==3&&result&&!((uintptr_t)result&3)&&background_focus_writable((uintptr_t)result,4))
        InterlockedExchange((LONG volatile*)result,0);
    SetLastError(error);
}
static void runtime_dialogue_forward(void *self,void *frame,void *result,RuntimeAmmoPickupExec stock) {
    RuntimeDialogueChoices *q=&runtime_dialogue;DWORD error=GetLastError();uintptr_t node=read_ptr((uintptr_t)frame+0x18),params=0;
    unsigned kind=12;GameArray script={0};int confirm=0,consume=0,selected=0;
    for(unsigned i=0;i<12;++i)if(node==q->script[i].function.pointer){kind=i;break;}
    int valid=kind<12&&runtime_dialogue_frame((uintptr_t)self,(uintptr_t)frame,kind,&script,kind==0||kind==1||kind==2||kind==3||kind==10?&params:NULL);
    if(valid&&kind==0&&!q->busy) {
        if(g_runtime_config.role==ME3_ROLE_HOST&&(uintptr_t)self==q->host_panel.pointer)q->host_marked=0;
        runtime_dialogue_host_capture((uintptr_t)self,(uintptr_t)frame);
    }
    if(valid&&kind==0&&!q->busy&&g_runtime_config.role==ME3_ROLE_GUEST&&q->lease.active&&
       (uintptr_t)self==q->lease.panel.pointer) {
        /* A native replacement of the actual options wins until a new
         * catalog. Do not hide/overwrite its strings during our retirement. */
        q->suppressed_generation=q->guest.generation;q->active_guest=0;q->lease.options=0;
    }
    if(valid&&kind==6&&!q->busy&&g_runtime_config.role==ME3_ROLE_HOST&&q->active_host&&
       (uintptr_t)self==q->host_panel.pointer){q->active_host=0;q->send_close=1;q->send_catalog=0;q->host_marked=0;}
    int owned=valid&&g_runtime_config.role==ME3_ROLE_GUEST&&(uintptr_t)self==q->lease.panel.pointer&&
        runtime_dialogue_panel_current(q->base,&q->lease)&&runtime_dialogue_guest_local_free();
    if(owned&&kind>=1&&kind<=9&&kind!=6) {
        selected=1;
        int32_t slot=-1;
        if(kind==1||kind==2){if(!read_at(params,&slot,4))slot=-1;}
        if(kind==3){unsigned char value=255;if(read_at(params,&value,1))slot=value;}
        consume=runtime_dialogue_body(q->base,node,kind)&&runtime_dialogue_redirect((uintptr_t)frame,&script,kind);
        if(consume&&slot>=0&&slot<6)runtime_dialogue_vote((unsigned)slot);
        if(!consume)runtime_dialogue_note("guest-selection-return-refused");
    }
    if(owned&&kind==10&&!q->busy&&runtime_dialogue_guest_fresh(GetTickCount())&&runtime_dialogue_ui_ready()) {
        unsigned char event=255;float amount=0;
        if(read_at(params,&event,1)&&read_at(params+4,&amount,4)&&isfinite(amount)&&amount>0.0f&&(event==15||event==31))confirm=1;
    }
    SetLastError(error);runtime_dialogue_stock_or_refuse(self,frame,result,stock,selected,consume,kind);error=GetLastError();
    if(confirm&&runtime_dialogue_ui_ready()&&
       runtime_dialogue_guest_fresh(GetTickCount())&&runtime_dialogue_method_live(q->base,&q->script[11],0xf52b0)&&
       runtime_dialogue_body(q->base,q->script[11].function.pointer,11)) {
        /* Only this real focused-panel UI event confirms the stock widget.
         * Nested SelectEntry remains intercepted even inside busy. */
        ++q->busy;((ProcessEvent)(q->base+0xf4f80))(self,(void*)q->script[11].function.pointer,NULL,NULL);--q->busy;
        uint32_t native=0;if(result&&!((uintptr_t)result&3)&&read_at((uintptr_t)result,&native,4)&&native<=1&&background_focus_writable((uintptr_t)result,4))
            InterlockedCompareExchange((LONG volatile*)result,1,(LONG)native);
    }
    (void)consume;SetLastError(error);
}
/* Catalogs are sent in a bounded stream; a failed native enqueue never moves
 * the fragment cursor. Once complete, reliable commit heartbeats refresh the
 * existing catalog without republishing strings or resetting a suggestion. */
static int runtime_dialogue_advance_fragment(unsigned *row,unsigned *offset,const DialogueChoicePacket *p) {
    if(!row||!offset||!p||p->kind!=L'F'||p->slot!=*row||p->offset!=*offset||
       !dialogue_choice_packet_valid(p))return 0;
    *offset+=p->count;
    if(*offset==p->length){++*row;*offset=0;}return 1;
}
static void runtime_dialogue_host_marker_retire(void);
static void runtime_dialogue_host_tick(DWORD now) {
    RuntimeDialogueChoices *q=&runtime_dialogue;
    if(!runtime_scene_scope_live(q->base,&q->host_scope)) {
        q->active_host=q->send_catalog=q->send_close=q->pending_host_valid=q->host_marked=q->scene_suspended=0;q->advice_slot=-1;return;
    }
    if(dialogue_choice_catalog_valid(&q->host)&&!runtime_scene_host_active&&!q->scene_suspended) {
        /* Scene inactivity is authenticated on the same owner-PC stream.
         * Old commit heartbeats must not reopen its wheel in a later scene. */
        runtime_dialogue_host_marker_retire();q->active_host=0;q->send_catalog=0;
        q->send_close=1;q->scene_suspended=1;q->pending_host_valid=0;q->advice_slot=-1;
    }
    if(runtime_scene_host_active&&q->scene_suspended) {
        q->active_host=1;
        if(!runtime_dialogue_catalog_native_current(q->base)||q->generation==UINT64_MAX){q->active_host=0;return;}
        /* A brief native scene-mode transition may leave the very same
         * real options visible. Revalidate them and publish a new generation,
         * rather than reviving the closed catalog. All native option writes
         * continue through the host capture boundary. */
        q->host.generation=++q->generation;q->send_catalog=1;q->send_close=0;
        q->row=q->offset=0;q->scene_suspended=0;q->host_marked=0;q->sent_at=0;
        runtime_dialogue_note("host-catalog-resumed-new-generation");
    }
    if(q->send_close) {
        DialogueChoicePacket close=dialogue_choice_packet(&q->host,L'Z',0,0);
        if(runtime_dialogue_send(&q->host_scope,&close,1)){q->send_close=0;q->sent_at=now;}return;
    }
    if(q->scene_suspended)return;
    if(!q->active_host)return;
    if(!runtime_dialogue_catalog_native_current(q->base))return;
    if(q->pending_host_valid) {
        DialogueChoicePacket packet=q->pending_host;
        q->pending_host_valid=0;
        if(dialogue_choice_vote_matches(&q->host,&packet)&&
           (q->advice_slot<0||q->advice_slot==(int)packet.slot)) {
            if(q->advice_slot<0){q->advice_slot=(int)packet.slot;q->host_marked=0;runtime_dialogue_note("guest-advice-accepted");}
            packet.kind=L'A';runtime_dialogue_send(&q->host_scope,&packet,1);
        }
    }
    if(q->advice_slot>=0&&!q->host_marked&&runtime_dialogue_catalog_native_current(q->base)) {
        uint64_t generation=q->host.generation;
        if(runtime_dialogue_options(q->host_panel.pointer,&q->host,q->advice_slot)&&
           generation==q->host.generation&&runtime_dialogue_catalog_native_current(q->base)) {
            q->host_marked=1;runtime_dialogue_note("host-advice-marker-presented");
        }
    }
    if(q->sent_at&&now-q->sent_at<16)return;
    if(q->send_catalog) {
        for(unsigned sent=0;sent<2&&q->send_catalog;++sent) {
            if(!runtime_dialogue_catalog_native_current(q->base))return;
            DialogueChoicePacket packet=dialogue_choice_packet(&q->host,q->row<6?L'F':L'C',q->row,q->offset);
            if(!runtime_dialogue_send(&q->host_scope,&packet,1))return;
            q->sent_at=now;
            if(packet.kind==L'C'){q->send_catalog=0;runtime_dialogue_note("host-catalog-committed");}
            else if(!runtime_dialogue_advance_fragment(&q->row,&q->offset,&packet)){q->send_catalog=0;return;}
        }
    } else if(!q->sent_at||now-q->sent_at>=250) {
        DialogueChoicePacket packet=dialogue_choice_packet(&q->host,L'C',0,0);
        if(runtime_dialogue_send(&q->host_scope,&packet,1))q->sent_at=now;
    }
}
static void runtime_dialogue_guest_tick(uintptr_t engine,DWORD now) {
    RuntimeDialogueChoices *q=&runtime_dialogue;
    if(q->active_guest&&runtime_scene_view.valid&&
       runtime_scene_view.message.nonce==q->guest.nonce&&!runtime_scene_view.message.active&&
       runtime_scene_scope_live(q->base,&runtime_scene_view.scope)&&runtime_scene_scope_live(q->base,&q->guest_scope)&&
       runtime_scene_view.scope.channel.pointer==q->guest_scope.channel.pointer) {
        q->active_guest=0;q->stage.failed=1;q->vote_pending=0;
    }
    if(!runtime_dialogue_guest_fresh(now)) {
        q->vote_pending=0;runtime_dialogue_lease_retire();return;
    }
    if(q->lease.active&&(!runtime_dialogue_panel_current(q->base,&q->lease)||!runtime_dialogue_guest_local_free())) {
        runtime_dialogue_lease_retire();if(q->lease.active)return;
    }
    if((!q->lease.active||q->lease.generation!=q->guest.generation)&&
       (!q->panel_scanned||now-q->panel_scanned>=250)) {
        q->panel_scanned=now;SessionContext c=session_context(engine);
        if(c.pc==q->guest_scope.pc.pointer&&c.world==q->guest_scope.world.pointer)
            runtime_dialogue_lease_begin(c.pc,c.world,&q->guest);
    }
    if(q->vote_pending&&q->vote_attempts<8&&(!q->vote_sent||now-q->vote_sent>=250)&&
       runtime_dialogue_guest_fresh(now)&&dialogue_choice_vote_matches(&q->guest,&q->vote)) {
        if(runtime_dialogue_send(&q->guest_scope,&q->vote,0)) {
            q->vote_sent=now;++q->vote_attempts;
            if(q->vote_attempts==8){q->vote_pending=0;runtime_dialogue_note("advice-awaiting-no-further-retry");}
        }
    }
}
/* Cleanup is local presentation ownership, so it may finish after networking
 * has been disabled. Native host choices/entry changes always win. */
static void runtime_dialogue_host_marker_retire(void) {
    RuntimeDialogueChoices *q=&runtime_dialogue;
    if(!q->host_marked)return;
    const GuestCameraStamp *stamps[]={&q->host_panel,&q->controller,&q->data,&q->host_scope.pc,&q->host_scope.world};
    for(unsigned i=0;i<5;++i){int state=runtime_scene_camera_stamp_state(q->base,stamps[i]);
        if(state<0){q->host_marked=0;return;}if(!state)return;}
    SessionContext c=session_context(session_engine);uintptr_t full=0;int32_t entry=-1,slots[6];
    if(c.pc!=q->host_scope.pc.pointer||c.world!=q->host_scope.world.pointer||
       !runtime_dialogue_panel_owned(q->base,q->host_panel.pointer,c.pc,c.world)||
       !runtime_dialogue_query_full(q->base,c.world,&full))return;
    if(full!=q->controller.pointer||read_ptr(full+0x284)!=q->data.pointer||
       !read_at(full+0x2ec,&entry,4)||entry!=q->host.entry||!read_at(q->host_panel.pointer+0x364,slots,sizeof slots)){
        q->host_marked=0;return;}
    for(unsigned i=0;i<6;++i)if(slots[i]!=q->host.rows[i].reply){q->host_marked=0;return;}
    if(runtime_dialogue_options(q->host_panel.pointer,&q->host,-1))q->host_marked=0;
}
static void runtime_dialogue_tick(uintptr_t base,uintptr_t engine,int after,DWORD now) {
    RuntimeDialogueChoices *q=&runtime_dialogue;presentation_dialogue_receive=runtime_dialogue_receive;
    if(GetCurrentThreadId()!=session_game_thread||q->busy)return;
    DWORD error=GetLastError();
    if(!runtime_dialogue_enabled()||!runtime_control_enabled||!runtime_scope_ready||
       !InterlockedCompareExchange(&g_enable_requested,0,0)) {
        if(g_runtime_config.role==ME3_ROLE_HOST)runtime_dialogue_host_marker_retire();
        q->active_guest=q->active_host=q->vote_pending=q->pending_host_valid=0;
        runtime_dialogue_lease_retire();SetLastError(error);return;
    }
    if(!after&&!runtime_dialogue_prepare(base,now)&&q->prepare_step) {
        char reason[180];snprintf(reason,sizeof reason,"%s:%s",q->prepare_step,guest_equipment_body_err);
        if(strcmp(reason,q->last_refused)) {
            char quoted[400],row[620];snprintf(q->last_refused,sizeof q->last_refused,"%s",reason);identity_quote(reason,quoted,sizeof quoted);
            snprintf(row,sizeof row,"{\"runtime_dialogue_choices\":\"native-preparation-deferred\",\"reason\":\"%s\",\"gameplay_verified\":false,\"monotonic_ms\":%lu}\r\n",quoted,now);line(row);
        }
    }
    if(q->prepared&&after) {
        ++q->busy;
        if(g_runtime_config.role==ME3_ROLE_HOST)runtime_dialogue_host_tick(now);
        else if(g_runtime_config.role==ME3_ROLE_GUEST)runtime_dialogue_guest_tick(engine,now);
        --q->busy;
    }
    SetLastError(error);
}
#ifdef ME3_RUNTIME_FIXTURE
static unsigned runtime_dialogue_fixture_calls;
static void runtime_dialogue_fixture_stock(void *self,void *frame,void *result) {
    if(self==(void*)1&&frame&&result==(void*)3&&GetLastError()==717)++runtime_dialogue_fixture_calls;
    SetLastError(818);
}
static int runtime_dialogue_selftest(void) {
    unsigned row=0,offset=0;DialogueChoiceCatalog c={0};c.nonce=1;c.generation=1;c.entry=0;
    for(unsigned i=0;i<6;++i)c.rows[i].reply=-1;
    c.rows[0].reply=0;c.rows[0].length=30;
    for(unsigned i=0;i<30;++i)c.rows[0].text[i]=L'a';c.digest=dialogue_choice_digest(&c);
    DialogueChoicePacket p=dialogue_choice_packet(&c,L'F',0,0);
    if(!runtime_dialogue_advance_fragment(&row,&offset,&p)||row||offset!=28)return 0;
    if(runtime_dialogue_advance_fragment(&row,&offset,&p))return 0;
    p=dialogue_choice_packet(&c,L'F',0,28);
    if(!runtime_dialogue_advance_fragment(&row,&offset,&p)||row!=1||offset)return 0;
    for(unsigned i=1;i<6;++i){p=dialogue_choice_packet(&c,L'F',i,0);
        if(!runtime_dialogue_advance_fragment(&row,&offset,&p)||row!=i+1||offset)return 0;}
    DialogueChoiceStage stage={0};DialogueChoiceCatalog out={0};p=dialogue_choice_packet(&c,L'Z',0,0);
    if(dialogue_choice_stage(&stage,&p,&out)!=3)return 0;
    p=dialogue_choice_packet(&c,L'C',0,0);if(dialogue_choice_stage(&stage,&p,&out))return 0;
    c.generation++;p=dialogue_choice_packet(&c,L'C',0,0);if(dialogue_choice_stage(&stage,&p,&out)!=1)return 0;
    unsigned char frame[64]={0},script[260]={0};GameArray code={(uintptr_t)script,260,260};
    uintptr_t start=(uintptr_t)script;memcpy(frame+0x28,&start,8);
    script[205]=4;script[206]=0x28;
    if(!runtime_dialogue_redirect((uintptr_t)frame,&code,3)||read_ptr((uintptr_t)frame+0x28)!=(uintptr_t)script+205)return 0;
    memcpy(frame+0x28,&start,8);script[206]=0x0b;
    if(runtime_dialogue_redirect((uintptr_t)frame,&code,3)||read_ptr((uintptr_t)frame+0x28)!=start)return 0;
    script[79]=4;script[80]=0x0b;
    if(!runtime_dialogue_redirect((uintptr_t)frame,&code,1))return 0;
    DWORD error=GetLastError();int prepared=runtime_dialogue.prepared;runtime_dialogue.prepared=0;
    runtime_dialogue_fixture_calls=0;SetLastError(717);
    runtime_dialogue_forward((void*)1,frame,(void*)3,runtime_dialogue_fixture_stock);
    int ok=runtime_dialogue_fixture_calls==1&&GetLastError()==818;
    uint32_t boolean=1;SetLastError(717);
    runtime_dialogue_stock_or_refuse((void*)1,frame,&boolean,runtime_dialogue_fixture_stock,1,0,3);
    ok=ok&&runtime_dialogue_fixture_calls==1&&boolean==0;
    SetLastError(717);runtime_dialogue_stock_or_refuse((void*)1,frame,(void*)3,runtime_dialogue_fixture_stock,1,1,1);
    ok=ok&&runtime_dialogue_fixture_calls==2&&GetLastError()==818;
    runtime_dialogue.prepared=prepared;SetLastError(error);return ok;
}
#endif
#endif
