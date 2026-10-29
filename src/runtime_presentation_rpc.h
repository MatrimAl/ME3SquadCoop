#ifndef ME3_RUNTIME_PRESENTATION_RPC_H
#define ME3_RUNTIME_PRESENTATION_RPC_H
#include "remote_presentation_protocol.h"
/* Ordinary-entry presentation snapshots over the existing native owner-PC
 * RPCs. The native receive frame, current connection and actor channel are
 * checked separately from the scalar parser. No ammo/story/role writes. */
typedef void (*PresentationRpcExec)(void*,void*,void*);
typedef struct PresentationGuestScope {
    GuestCameraStamp objects[7]; /* pc, local pawn, replica, info, connection, driver, channel */
    int valid,index;
} PresentationGuestScope;
static PresentationGuestScope presentation_guest;
static GuestPlayerContextMethod presentation_methods[2];
static PresentationRpcExec presentation_original;
static uintptr_t presentation_base;
static int presentation_installed,presentation_attempted,presentation_busy;
static unsigned presentation_events;
/* Additional authored gameplay admission uses these same two owner-PC RPCs.
 * Its receiver may only queue scalars; engine calls stay on the normal Tick. */
static int (*presentation_peer_enabled)(void);
static int (*presentation_scope_enabled)(void);
static int (*presentation_peer_receive)(uintptr_t,uintptr_t,int,const wchar_t*,size_t,const NetworkActorIdentityEntry*);
/* Scene viewing has its own native owner-PC provenance check: a full scene
 * may legitimately release the companion and ordinary presentation scope. */
static int (*presentation_scene_receive)(uintptr_t,uintptr_t,const wchar_t*,size_t);
/* A mirrored conversation may have released companion possession. */
static int (*presentation_dialogue_receive)(uintptr_t,uintptr_t,const wchar_t*,size_t);
static uint64_t presentation_sequence,presentation_pending;
static DWORD presentation_sent_at,presentation_scan_at,presentation_received_at;
static unsigned char presentation_helmet[4];
static int presentation_received;
/* Confirm a weapon actor/channel lifetime across a second correlated request.
 * A reply never resolves a recycled channel into a newly admitted weapon. */
static GuestCameraStamp presentation_weapon,presentation_weapon_channel;
static unsigned presentation_weapon_index=2048;
static int presentation_weapon_candidate,presentation_weapon_pending,presentation_weapon_confirmed;
typedef struct PresentationHostRequest {
    RemotePresentationMessage message;
    GuestCameraStamp pc,pawn,world,connection;
    uint32_t epoch;
    DWORD received_at;
    int active;
} PresentationHostRequest;
static PresentationHostRequest presentation_host_request;
static void presentation_exec(void*,void*,void*);
static int presentation_opted(void) {
    static int value=-1;
    if(value<0){wchar_t text[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_REMOTE_PRESENTATION",text,8);
        value=!n||(n==1&&text[0]==L'1');}
    return value;
}
static void presentation_log(const char *event,unsigned channel,const unsigned char states[4]) {
    if(presentation_events++>=128)return;
    char row[320];snprintf(row,sizeof row,
        "{\"remote_presentation\":\"%s\",\"channel_index\":%u,\"helmet\":[%u,%u,%u,%u],\"monotonic_ms\":%lu}\r\n",
        event,channel,states?states[0]:0,states?states[1]:0,states?states[2]:0,states?states[3]:0,GetTickCount());line(row);
}
static int presentation_method_valid(int k) {
    uintptr_t fn=presentation_methods[k].function.pointer;uint32_t flags=0;
    uint16_t size=0,result=0,index=1;unsigned char params=0;
    return presentation_installed&&guest_player_context_method_matches(presentation_base,&presentation_methods[k],1)&&
        read_ptr(fn+0xd8)==(uintptr_t)presentation_exec&&read_at(fn+0xe0,&flags,4)&&flags==(k?0x10201c2:0x220042)&&
        read_at(fn+0xe6,&size,2)&&size==16&&read_at(fn+0xe8,&result,2)&&result==65535&&
        read_at(fn+0xe4,&index,2)&&!index&&read_at(fn+0xea,&params,1)&&params==1;
}
static int presentation_send_text(uintptr_t pc,int k,const wchar_t *text,int count) {
    if(!text||count<8||count>192||text[count-1]||!presentation_method_valid(k)||!session_actor_live(presentation_base,pc)||
       read_ptr(read_ptr(pc)+0x230)!=presentation_base+0x40d450||!guest_equipment_dispatch_pins(presentation_base))return 0;
    GameArray parameter={(uintptr_t)text,count,count};
    ((ProcessEvent)(presentation_base+0x40d450))((void*)pc,(void*)presentation_methods[k].function.pointer,&parameter,NULL);
    return 1;
}
static int presentation_send(uintptr_t pc,int k,const RemotePresentationMessage *message) {
    wchar_t text[96];int count=remote_presentation_format(message,text);
    return count&&presentation_send_text(pc,k,text,count);
}
static int presentation_guest_valid(uintptr_t base) {
    PresentationGuestScope *s=&presentation_guest;
    if(!s->valid||!runtime_control_enabled||!runtime_scope_ready||g_runtime_config.role!=ME3_ROLE_GUEST)return 0;
    for(unsigned i=0;i<7;++i)if(!guest_camera_stamp_matches(base,&s->objects[i],1))return 0;
    SessionContext c=session_context(session_engine);NetworkActorIdentityEntry actor={0};unsigned char role=255;
    if(c.pc!=s->objects[0].pointer||c.pawn!=s->objects[1].pointer||c.world!=s->objects[3].pointer||
       session_engine_blocked(c)||read_ptr(c.pawn+0x374)!=c.pc||
       read_ptr(s->objects[2].pointer+0x198)!=c.world||!read_at(s->objects[2].pointer+0x26a,&role,1)||role!=1||
       read_ptr(s->objects[5].pointer+0x80)!=s->objects[4].pointer)return 0;
    return network_actor_channel_valid(base,s->objects[2].pointer,s->objects[6].pointer,s->objects[5].pointer,&actor)&&
        actor.index==s->index&&actor.connection==s->objects[4].pointer;
}
static int presentation_weapon_valid(uintptr_t base) {
    NetworkActorIdentityEntry entry={0};PresentationGuestScope *s=&presentation_guest;
    return presentation_weapon_candidate&&presentation_guest_valid(base)&&
        guest_camera_stamp_matches(base,&presentation_weapon,1)&&guest_camera_stamp_matches(base,&presentation_weapon_channel,1)&&
        network_actor_channel_valid(base,presentation_weapon.pointer,presentation_weapon_channel.pointer,s->objects[5].pointer,&entry)&&
        entry.connection==s->objects[4].pointer&&entry.index==(int)presentation_weapon_index&&
        is_kind_of(base,presentation_weapon.pointer,"SFXWeapon")&&
        read_ptr(presentation_weapon.pointer+0x198)==s->objects[3].pointer&&
        read_ptr(presentation_weapon.pointer+0x190)==s->objects[2].pointer;
}
static void presentation_weapon_capture(uintptr_t base,unsigned index) {
    presentation_weapon_candidate=presentation_weapon_confirmed=0;presentation_weapon_index=index;
    if(index>2047||!presentation_guest_valid(base))return;
    PresentationGuestScope *s=&presentation_guest;NetworkActorIdentityEntry entry={0};
    uintptr_t channel=read_ptr(s->objects[4].pointer+0xf84+(uintptr_t)index*8),weapon=read_ptr(channel+0xa0);
    if(!network_actor_channel_valid(base,weapon,channel,s->objects[5].pointer,&entry)||entry.index!=(int)index||
       entry.connection!=s->objects[4].pointer||!is_kind_of(base,weapon,"SFXWeapon")||
       read_ptr(weapon+0x198)!=s->objects[3].pointer||read_ptr(weapon+0x190)!=s->objects[2].pointer||
       !guest_camera_stamp_read(base,weapon,&presentation_weapon,1)||
       !guest_camera_stamp_read(base,channel,&presentation_weapon_channel,1))return;
    presentation_weapon_candidate=1;
}
static int presentation_source(uintptr_t self,uintptr_t frame,int k,NetworkActorIdentityEntry *source) {
    NativeRpcReceiveFrame *r=native_rpc_receive_current(session_game_thread);NetworkActorScope world;
    if(!runtime_control_enabled||!runtime_scope_ready||!r||!r->channel||
       !presentation_method_valid(k)||read_ptr(frame+0x18)!=presentation_methods[k].function.pointer||
       read_ptr(frame+0x20)!=self||!network_actor_read_scope(presentation_base,self,&world)||
       !network_actor_channel_valid(presentation_base,self,r->channel,world.driver,source)||
       read_ptr(source->connection+0x68)!=self)return 0;
    if(g_runtime_config.role==ME3_ROLE_HOST)return k==0&&session_active&&!session_releasing&&
        self==session_remote&&source->connection==session_connection&&session_peer_connected(presentation_base,self,session_world);
    return k==1&&presentation_guest_valid(presentation_base)&&self==presentation_guest.objects[0].pointer&&
        source->connection==presentation_guest.objects[4].pointer;
}
static int presentation_receive(uintptr_t self,uintptr_t frame) {
    if(!presentation_installed||presentation_busy)return 0;
    int k=g_runtime_config.role==ME3_ROLE_HOST?0:1;GameArray string={0};wchar_t text[192]={0};
    if(!presentation_method_valid(k)||read_ptr(frame+0x18)!=presentation_methods[k].function.pointer)return 0;
    uintptr_t locals=read_ptr(frame+0x30);
    if(!read_at(locals,&string,sizeof string)||!string.data||string.count<8||string.count>192||
       string.max<string.count||string.max>256||!read_at(string.data,text,(SIZE_T)string.count*sizeof(wchar_t))||
       text[string.count-1])return 0;
    if(!wcsncmp(text,L"ME3SC1 ",7))return presentation_scene_receive?
        presentation_scene_receive(self,frame,text,(size_t)string.count):1;
    if(!wcsncmp(text,L"ME3DC1 ",7))return presentation_dialogue_receive?
        presentation_dialogue_receive(self,frame,text,(size_t)string.count):1;
    int peer=!wcsncmp(text,L"ME3RV1 ",7);
    if(!peer&&(wcsncmp(text,L"ME3PR2 ",7)||!presentation_opted()))return 0;
    NetworkActorIdentityEntry source={0};
    if(!presentation_source(self,frame,k,&source))return 0;
    if(peer)return presentation_peer_receive?
        presentation_peer_receive(self,frame,k,text,(size_t)string.count,&source):1;
    RemotePresentationMessage m={0};
    if(!remote_presentation_parse(text,string.count,&m))return 1;
    DWORD now=GetTickCount();
    if(k==0) {
        NetworkActorIdentityEntry target={0};
        if(m.kind!=L'Q'||!session_story_valid(presentation_base)||
           network_actor_scan_driver(presentation_base,session_shepard,source.driver,&target)<0||
           target.index!=(int)m.channel||target.connection!=source.connection)return 1;
        PresentationHostRequest request={0};request.message=m;request.epoch=session_epoch;request.received_at=now;
        if(!guest_camera_stamp_read(presentation_base,self,&request.pc,1)||
           !guest_camera_stamp_read(presentation_base,session_shepard,&request.pawn,1)||
           !guest_camera_stamp_read(presentation_base,session_world,&request.world,1)||
           !guest_camera_stamp_read(presentation_base,source.connection,&request.connection,1))return 1;
        request.active=1;presentation_host_request=request;return 1;
    }
    if(!remote_presentation_reply_matches(&m,presentation_pending,(unsigned)presentation_guest.index,presentation_sent_at,now))return 1;
    memcpy(presentation_helmet,m.helmet,4);presentation_received=1;presentation_received_at=now;
    if(presentation_weapon_pending&&m.weapon_channel==presentation_weapon_index&&presentation_weapon_valid(presentation_base))
        presentation_weapon_confirmed=1;
    else presentation_weapon_capture(presentation_base,m.weapon_channel);
    presentation_weapon_pending=0;
    presentation_pending=0;presentation_log("received-host-helmet",m.channel,m.helmet);return 1;
}
static void presentation_exec(void *self,void *frame,void *result) {
    DWORD error=GetLastError();int consumed=presentation_receive((uintptr_t)self,(uintptr_t)frame);SetLastError(error);
    if(!consumed&&presentation_original)presentation_original(self,frame,result);
}
static void presentation_install(uintptr_t base,uintptr_t pc) {
    if(presentation_attempted||!runtime_rpc_receive_installed)return;
    presentation_attempted=1;presentation_base=base;
    const char *names[]={"ServerSay","ClientAdminMessage"};
    for(int k=0;k<2;++k) {
        uintptr_t fn=guest_player_context_find_method(base,read_ptr(pc+0x50),"PlayerController",names[k]);
        if(!guest_player_context_method_stamp(base,fn,&presentation_methods[k])||
           guest_equipment_find_first(base,read_ptr(pc+0x50),names[k])!=fn||
           !guest_player_context_function_abi(base,fn,"PlayerController",names[k],k?0x10201c2:0x220042,16,65535,0,0xf52b0,1))goto rejected;
        uintptr_t child=read_ptr(fn+0x70);int found=0;
        for(int i=0;child&&i<32;++i) {
            GuestCameraStamp stamp;int32_t offset=-1;uint64_t flags=0;char name[96],path[180];
            if(!is_live(base,child))goto rejected;
            object_name(base,child,name,sizeof name);
            if(!strcmp(name,"Msg")) {
                snprintf(path,sizeof path,"Engine.PlayerController.%s.Msg",names[k]);
                if(found++||!narrative_probe_object(base,child,"StrProperty")||read_ptr(child+0x40)!=fn||
                   !guest_camera_stamp_read(base,child,&stamp,1)||strcmp(stamp.full_name,path)||
                   !read_at(child+0x94,&offset,4)||offset||!read_at(child+0x78,&flags,8)||flags!=0x400080)goto rejected;
            }
            child=read_ptr(child+0x68);
        }
        if(child||found!=1)goto rejected;
    }
    if(!guest_equipment_dispatch_pins(base))goto rejected;
    presentation_original=(PresentationRpcExec)(base+0xf52b0);
    for(int k=0;k<2;++k)if(InterlockedCompareExchangePointer((PVOID volatile*)(presentation_methods[k].function.pointer+0xd8),
            (PVOID)presentation_exec,(PVOID)presentation_original)!=(PVOID)presentation_original) {
        if(k)InterlockedCompareExchangePointer((PVOID volatile*)(presentation_methods[0].function.pointer+0xd8),
            (PVOID)presentation_original,(PVOID)presentation_exec);
        goto rejected;
    }
    presentation_installed=1;presentation_log("native-owner-pc-installed",0,NULL);return;
rejected:presentation_log("native-owner-pc-refused",0,NULL);
}
static int presentation_capture_guest(uintptr_t base,SessionContext c,PresentationGuestScope *out) {
    NetworkActorScope world;NetworkActorIdentityEntry target={0};uintptr_t replica=0;int found=0;
    if(!session_actor_live(base,c.pawn)||!is_kind_of(base,c.pawn,"SFXPawn_Ashley")||read_ptr(c.pawn+0x374)!=c.pc||
       !network_actor_read_scope(base,c.pawn,&world)||world.info!=c.world)return 0;
    for(int i=0;i<world.actors.count;++i) {
        uintptr_t actor=read_ptr(world.actors.data+(uintptr_t)i*8);unsigned char role=255;NetworkActorIdentityEntry candidate={0};
        if(actor&&actor!=c.pawn&&session_actor_live(base,actor)&&read_ptr(actor+0x198)==c.world&&
           is_kind_of(base,actor,"SFXPawn_Player")&&read_at(actor+0x26a,&role,1)&&role==1&&
           network_actor_scan_driver(base,actor,world.driver,&candidate)>=0) {replica=actor;target=candidate;if(++found>1)return 0;}
    }
    if(found!=1)return 0;
    uintptr_t objects[]={c.pc,c.pawn,replica,c.world,target.connection,world.driver,target.channel};
    memset(out,0,sizeof *out);
    for(unsigned i=0;i<7;++i)if(!guest_camera_stamp_read(base,objects[i],&out->objects[i],1))return 0;
    out->index=target.index;out->valid=1;return 1;
}
static int presentation_default_host(uintptr_t base,uintptr_t pawn) {
    int ids[14];uint32_t sex=0,flags=0;
    if(!is_kind_of(base,pawn,"SFXPawn_Player")||!read_at(pawn+0x14dc,ids,sizeof ids)||
       !read_at(pawn+0x578,&sex,4)||(sex&0x20)||read_ptr(pawn+0xb40)||
       !read_at(pawn+0x1560,&flags,4)||(flags&0x18c1))return 0;
    for(unsigned i=0;i<14;++i)if(ids[i]!=(i==1?-1:0))return 0;
    return 1;
}
static void runtime_presentation_tick(uintptr_t base,uintptr_t engine,DWORD now) {
    SessionContext c=session_context(engine);
    int rpc_needed=presentation_opted()||(presentation_peer_enabled&&presentation_peer_enabled());
    if((!rpc_needed&&!(presentation_scope_enabled&&presentation_scope_enabled()))||
       !runtime_control_enabled||!runtime_scope_ready||session_engine_blocked(c)) {
        memset(&presentation_guest,0,sizeof presentation_guest);presentation_pending=0;presentation_received=0;
        presentation_weapon_candidate=presentation_weapon_pending=presentation_weapon_confirmed=0;
        presentation_host_request.active=0;return;
    }
    if(rpc_needed){presentation_install(base,c.pc);if(!presentation_installed)return;}
    if(g_runtime_config.role==ME3_ROLE_HOST) {
        if(!presentation_opted()){presentation_host_request.active=0;return;}
        PresentationHostRequest r=presentation_host_request;presentation_host_request.active=0;
        if(!r.active||now-r.received_at>3000||r.epoch!=session_epoch||!session_active||session_releasing||
           !session_story_valid(base)||c.pawn!=r.pawn.pointer||session_remote!=r.pc.pointer||
           session_connection!=r.connection.pointer||c.world!=r.world.pointer||
           !guest_camera_stamp_matches(base,&r.pc,1)||!guest_camera_stamp_matches(base,&r.pawn,1)||
           !guest_camera_stamp_matches(base,&r.world,1)||!guest_camera_stamp_matches(base,&r.connection,1)||
           !presentation_default_host(base,c.pawn))return;
        NetworkActorIdentityEntry target={0};unsigned char check[4];
        if(network_actor_scan_driver(base,c.pawn,identity_host_driver,&target)<0||target.connection!=session_connection||
           target.index!=(int)r.message.channel||!read_at(c.pawn+0x105c,r.message.helmet,4))return;
        for(unsigned i=0;i<4;++i)if(r.message.helmet[i]>3)return;
        if(!read_at(c.pawn+0x105c,check,4)||memcmp(check,r.message.helmet,4))return;
        r.message.weapon_channel=2048;
        uintptr_t weapon=read_ptr(c.pawn+0x41c),inventory=read_ptr(c.pawn+0x414);NetworkActorIdentityEntry weapon_entry={0};
        if(session_actor_live(base,weapon)&&is_kind_of(base,weapon,"SFXWeapon")&&
           read_ptr(weapon+0x198)==c.world&&read_ptr(weapon+0x190)==c.pawn&&read_ptr(weapon+0x180)==c.pawn&&
           is_live(base,inventory)&&read_ptr(weapon+0x2b0)==inventory&&
           network_actor_scan_driver(base,weapon,identity_host_driver,&weapon_entry)>=0&&weapon_entry.connection==session_connection&&
           read_ptr(c.pawn+0x41c)==weapon)r.message.weapon_channel=(unsigned)weapon_entry.index;
        r.message.kind=L'A';if(presentation_send(r.pc.pointer,1,&r.message))presentation_log("sent-host-helmet",r.message.channel,r.message.helmet);
        return;
    }
    if(!presentation_guest_valid(base)) {
        memset(&presentation_guest,0,sizeof presentation_guest);presentation_pending=0;presentation_received=0;
        presentation_weapon_candidate=presentation_weapon_pending=presentation_weapon_confirmed=0;
    }
    if(!presentation_scan_at||now-presentation_scan_at>=1000) {
        presentation_scan_at=now;
        if(!presentation_guest.valid) {
            PresentationGuestScope candidate;
            if(presentation_capture_guest(base,c,&candidate))presentation_guest=candidate;
        }
        if(presentation_opted()&&presentation_guest_valid(base)&&(!presentation_pending||now-presentation_sent_at>3000)&&presentation_sequence!=UINT64_MAX) {
            RemotePresentationMessage request={L'Q',++presentation_sequence,(unsigned)presentation_guest.index,{0}};
            presentation_pending=request.request;presentation_sent_at=now;
            presentation_weapon_pending=presentation_weapon_valid(base);
            if(!presentation_send(c.pc,0,&request))presentation_pending=0;
        }
    }
    if(presentation_opted()&&presentation_received&&now-presentation_received_at<=3000&&presentation_guest_valid(base)) {
        presentation_busy=1;
        runtime_remote_helmet_apply(base,engine,c.pc,c.pawn,c.world,presentation_guest.objects[2].pointer,presentation_helmet,now);
        if(presentation_weapon_confirmed&&presentation_weapon_valid(base)) {
            c=session_context(engine);
            runtime_remote_weapon_apply(base,c.pc,c.pawn,c.world,presentation_guest.objects[2].pointer,presentation_weapon.pointer,now);
        }
        presentation_busy=0;
    }
}
static int runtime_presentation_selftest(void) {
    return remote_presentation_protocol_selftest()&&runtime_remote_helmet_selftest();
}
#endif
