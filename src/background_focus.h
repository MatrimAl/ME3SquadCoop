#ifndef ME3_BACKGROUND_FOCUS_H
#define ME3_BACKGROUND_FOCUS_H
/* Private candidate. Include after squad_session_core.h and its existing
 * camera/context helpers. prepare runs before original Engine.Tick, even when
 * disabled. Only the measured BioWorldInfo.Tick HasFocus expressions may have
 * their returned false adapted; no world, pause-owner or browser flags change.
 * The tiny forwarding hook remains resident after disable, like existing hooks.
 */
typedef void (*BackgroundFocusExec)(void *,void *,void *);
typedef struct BackgroundFocusState {
    uintptr_t base,engine;
    DWORD thread,discovery_at;
    int enabled,installed,context_logged,refusal_logged,override_logged;
    GuestCameraStamp function,tick,owner;
    BackgroundFocusExec original;
} BackgroundFocusState;
static BackgroundFocusState background_focus;
typedef struct BackgroundFocusScope {SessionHandle handles[7];uintptr_t klass[7];} BackgroundFocusScope;
static const char *background_focus_reason="disabled";
static void background_focus_exec(void *self,void *frame,void *result);

static const unsigned char background_focus_exec_pin[]={
    0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20,0x48,0xff,0x42,0x28,0x49,0x8b,
    0xf8,0x48,0x8b,0x42,0x28,0x48,0x8b,0xd9,0x80,0x38,0x41,0x75,0x14,0x48,0x8b,0x4a,
    0x20,0x48,0xff,0xc0,0x45,0x33,0xc0,0x48,0x89,0x42,0x28,0xff,0x15,0xf7,0x52,0xca,
    0x00,0x48,0x8b,0x03,0x48,0x8b,0xcb,0xff,0x90,0x90,0x08,0x00,0x00,0x48,0x8b,0x5c,
    0x24,0x30,0x89,0x07,0x48,0x83,0xc4,0x20,0x5f,0xc3};
static const unsigned char background_focus_getter_pin[]={
    0x48,0x83,0xec,0x28,0x48,0x8b,0x05,0xe5,0xf0,0xdd,0x00,0x48,0x85,0xc0,0x74,0x2c,
    0x48,0x8b,0x88,0xa0,0x07,0x00,0x00,0x48,0x85,0xc9,0x74,0x20,0x48,0x8b,0x89,0x48,
    0x01,0x00,0x00,0x48,0x85,0xc9,0x74,0x14,0x48,0x8b,0x01,0xff,0x50,0x50,0x85,0xc0,
    0x74,0x0a,0xb8,0x01,0x00,0x00,0x00,0x48,0x83,0xc4,0x28,0xc3,0x33,0xc0,0x48,0x83,
    0xc4,0x28,0xc3};

static int background_focus_fail(const char *reason) {background_focus_reason=reason;return 0;}
static int background_focus_bytes(uintptr_t at,const unsigned char *want,size_t count) {
    unsigned char got[396];return count<=sizeof got&&read_at(at,got,count)&&!memcmp(got,want,count);
}
static int background_focus_pins(uintptr_t base) {
    return background_focus_bytes(base+0xb4fa90,background_focus_exec_pin,sizeof background_focus_exec_pin)&&
        background_focus_bytes(base+0xad40e0,background_focus_getter_pin,sizeof background_focus_getter_pin);
}
static int background_focus_writable(uintptr_t at,size_t size) {
    MEMORY_BASIC_INFORMATION m;DWORD p;
    if(!at||!size||at+size<at||VirtualQuery((void*)at,&m,sizeof m)!=sizeof m||m.State!=MEM_COMMIT||
       (m.Protect&(PAGE_GUARD|PAGE_NOACCESS))||at+size>(uintptr_t)m.BaseAddress+m.RegionSize)return 0;
    p=m.Protect&0xff;
    return p==PAGE_READWRITE||p==PAGE_WRITECOPY||p==PAGE_EXECUTE_READWRITE||p==PAGE_EXECUTE_WRITECOPY;
}
static void background_focus_log(const char *event,const char *reason,int before,int after,int original,int changed) {
    char row[650];snprintf(row,sizeof row,
        "{\"background_focus\":\"%s\",\"reason\":\"%s\",\"entry_cursor\":%d,\"post_cursor\":%d,\"original_result\":%d,\"result_adapted\":%s,\"world_flags_written\":false,\"monotonic_ms\":%lu}\r\n",
        event,reason,before,after,original,changed?"true":"false",GetTickCount());line(row);
}
static int background_focus_method_abi(int installed) {
    uintptr_t b=background_focus.base,fn=background_focus.function.pointer;
    uintptr_t expected=installed?(uintptr_t)background_focus_exec:b+0xb4fa90;
    uintptr_t field=read_ptr(fn+0x70),owner=read_ptr(fn+0x40);
    int32_t offset=-1;uint64_t flags=0,mask=0;char name[96];
    if(!guest_camera_stamp_matches(b,&background_focus.function,1)||
       !guest_camera_stamp_matches(b,&background_focus.tick,1)||
       !guest_camera_stamp_matches(b,&background_focus.owner,1)||owner!=background_focus.owner.pointer||
       !guest_player_context_function_abi(b,fn,"BioWorldInfo","HasFocus",0x20400,4,0,0,expected-b,1)||
       !narrative_probe_object(b,field,"BoolProperty")||read_ptr(field+0x40)!=fn||read_ptr(field+0x68)||
       !read_at(field+0x94,&offset,4)||offset||!read_at(field+0x78,&flags,8)||flags!=0x580||
       !read_at(field+0xd0,&mask,8)||mask!=1)return 0;
    object_name(b,field,name,sizeof name);return !strcmp(name,"ReturnValue");
}
static int background_focus_script(GameArray *script) {
    unsigned char bytes[396];uint32_t method_name[2];
    uintptr_t fn=background_focus.tick.pointer;
    if(!read_at(fn+0x7c,script,sizeof *script)||script->count!=396||script->max<396||script->max>262144||
       !read_at(script->data,bytes,sizeof bytes)||!read_at(background_focus.function.pointer+0x48,method_name,8))return 0;
    /* Small caller-shape guards, not an embedded game-function dump. */
    if(bytes[22]!=0x07||bytes[25]!=0x82||bytes[26]!=0x82||bytes[27]!=0x82||
       bytes[28]!=0x82||bytes[29]!=0x82||bytes[30]!=0x81||bytes[265]!=0x07||bytes[268]!=0x82)return 0;
    for(unsigned k=0;k<2;++k) {
        unsigned start=k?269:60;uint32_t names[2];uint16_t index=0;
        memcpy(names,bytes+start+1,8);memcpy(&index,bytes+start+9,2);
        if(bytes[start]!=0x65||memcmp(names,method_name,8)||index!=184||bytes[start+11]!=0x16)return 0;
    }
    return 1;
}
static int background_focus_array(uintptr_t at,GameArray *out,int limit) {
    return read_at(at,out,sizeof *out)&&out->count>=0&&out->count<=limit&&out->max>=out->count&&
        out->max<=limit*4+256&&(!out->count||(out->data&&!(out->data&7)));
}
static int background_focus_context(uintptr_t self,uintptr_t frame,int expected_cursor,GameArray *script,BackgroundFocusScope *scope) {
    BackgroundFocusState *s=&background_focus;uintptr_t b=s->base;
    if(!s->enabled||GetCurrentThreadId()!=s->thread)return background_focus_fail("disabled-or-wrong-thread");
    if(!background_focus_pins(b)||!background_focus_method_abi(1)||!background_focus_script(script))return background_focus_fail("method-or-script-changed");
    uintptr_t cursor=read_ptr(frame+0x28);
    if(read_ptr(frame+0x18)!=s->tick.pointer||read_ptr(frame+0x20)!=self||
       cursor!=script->data+(uintptr_t)expected_cursor)return background_focus_fail("caller-or-cursor");
    uintptr_t world=read_ptr(b+0x18b6638),level=read_ptr(world+0x88);GameArray actors={0},players={0};
    if(!narrative_probe_object(b,world,"World")||!narrative_probe_object(b,level,"Level")||
       !background_focus_array(level+0x60,&actors,100000)||!actors.count||read_ptr(actors.data)!=self||
       !guest_camera_actor_live(b,self)||!is_kind_of(b,self,"BioWorldInfo")||
       read_ptr(read_ptr(self)+0x890)!=b+0xad40e0)return background_focus_fail("current-world-or-getter");
    if(read_ptr(b+0x18b31d0)!=s->engine||!narrative_probe_object(b,s->engine,"Engine")||
       !background_focus_array(s->engine+0x498,&players,4)||players.count!=1)return background_focus_fail("local-player");
    uintptr_t player=read_ptr(players.data),pc=read_ptr(player+0x68);
    if(!narrative_probe_object(b,player,"LocalPlayer")||!guest_camera_actor_live(b,pc)||
       !is_kind_of(b,pc,"PlayerController")||read_ptr(pc+0x67c)!=player||read_ptr(pc+0x198)!=self)return background_focus_fail("local-controller");
    SessionContext c=session_context(s->engine);uint32_t flags=0,wflags=0,focus=0;
    if(c.pc!=pc||c.world!=self||!narrative_probe_object(b,c.manager,"SFXGameModeManager")||c.mode!=0||
       !read_at(pc+0x77c,&flags,4)||(flags&(0x20000|0x800))||
       !read_at(self+0x7c0,&wflags,4)||!read_at(self+0x9c8,&focus,4))return background_focus_fail("not-ordinary-gameplay");
    if((wflags&0x800)||((wflags&0x400)&&!(focus&0x8000)))return background_focus_fail("intentional-or-pending-pause");
    uintptr_t conversation=read_ptr(self+0x948);
    if(conversation) {
        int32_t count=-1;uint32_t activating=1;
        if(!narrative_probe_object(b,conversation,"BioConversationManager")||
           !read_at(conversation+0xd0,&count,4)||count!=0||!read_at(conversation+0xe8,&activating,4)||(activating&1))return background_focus_fail("conversation");
    }
    uintptr_t driver=read_ptr(world+0x120);unsigned char net=0;
    if(!narrative_probe_object(b,driver,"NetDriver")||!read_at(self+0x7c8,&net,1))return background_focus_fail("native-driver");
    uintptr_t connection=0,remote=0;GameArray peers={0};
    if(net==2) {
        if(!background_focus_array(driver+0x70,&peers,4)||peers.count!=1)return background_focus_fail("host-peer-count");
        connection=read_ptr(peers.data);remote=read_ptr(connection+0x68);
        if(!narrative_probe_object(b,connection,"NetConnection")||!guest_camera_actor_live(b,remote)||
           !is_kind_of(b,remote,"PlayerController")||read_ptr(remote+0x67c)!=connection||read_ptr(remote+0x198)!=self||
           read_ptr(connection+0x8c)!=driver||!read_at(remote+0x77c,&flags,4)||(flags&0x800))return background_focus_fail("host-peer-membership");
    } else if(net==3) {
        connection=read_ptr(driver+0x80);
        if(!narrative_probe_object(b,connection,"NetConnection")||read_ptr(connection+0x68)!=pc||
           read_ptr(connection+0x8c)!=driver)return background_focus_fail("guest-server-membership");
    } else return background_focus_fail("not-network-world");
    if(read_ptr(b+0x18b6638)!=world||read_ptr(world+0x88)!=level||read_ptr(actors.data)!=self||
       local_controller(s->engine)!=pc||read_ptr(pc+0x198)!=self||read_ptr(world+0x120)!=driver||
       read_ptr(connection+0x8c)!=driver||(net==2?(read_ptr(peers.data)!=connection||read_ptr(connection+0x68)!=remote||
       read_ptr(remote+0x67c)!=connection):read_ptr(driver+0x80)!=connection))return background_focus_fail("context-changed");
    uintptr_t objects[7]={world,level,self,pc,driver,connection,remote};
    for(unsigned i=0;i<7;++i)if(objects[i]) {
        scope->handles[i]=session_capture_handle(objects[i]);scope->klass[i]=read_ptr(objects[i]+0x50);
        if(!scope->handles[i].pointer||!scope->klass[i]||!session_handle_matches(scope->handles[i]))return background_focus_fail("context-identity");
    }
    background_focus_reason="verified-tick-context";
    return 1;
}
static void background_focus_exec(void *self,void *frame,void *result) {
    DWORD incoming=GetLastError();BackgroundFocusState *s=&background_focus;
    BackgroundFocusExec original=s->original;GameArray script={0};int entry=-1,post=-1,allowed=0,changed=0,value=-1;
    BackgroundFocusScope before={0},after={0};
    uintptr_t cursor=read_ptr((uintptr_t)frame+0x28);
    if(read_at(s->tick.pointer+0x7c,&script,sizeof script)&&script.count==396) {
        if(cursor==script.data+71)entry=71;else if(cursor==script.data+280)entry=280;
    }
    if(entry>=0)allowed=background_focus_context((uintptr_t)self,(uintptr_t)frame,entry,&script,&before);
    else background_focus_reason="caller-or-cursor";
    SetLastError(incoming);
    /* Installation never publishes this wrapper without an exact original. */
    if(original)original(self,frame,result);
    DWORD forwarded=GetLastError();
    cursor=read_ptr((uintptr_t)frame+0x28);
    if(entry>=0&&cursor==script.data+(uintptr_t)entry+1)post=entry+1;
    int post_allowed=allowed&&post>=0&&background_focus_context((uintptr_t)self,(uintptr_t)frame,post,&script,&after);
    if(post_allowed)for(unsigned i=0;i<7;++i)
        if(before.handles[i].pointer!=after.handles[i].pointer||before.handles[i].index!=after.handles[i].index||
           before.handles[i].name!=after.handles[i].name||before.klass[i]!=after.klass[i]){
            post_allowed=background_focus_fail("context-replaced-during-getter");break;
        }
    if(post_allowed&&read_at((uintptr_t)result,&value,4)&&(value==0||value==1)) {
        if(value==0&& !((uintptr_t)result&3)&&background_focus_writable((uintptr_t)result,4))
            changed=InterlockedCompareExchange((LONG volatile*)result,1,0)==0;
    } else if(allowed&&post<0)background_focus_reason="post-cursor";
    else if(post_allowed&&value!=0&&value!=1)background_focus_reason="unknown-or-unreadable-result";
    if(GetCurrentThreadId()==s->thread&&s->enabled) {
        if(!s->context_logged){s->context_logged=1;background_focus_log("context",background_focus_reason,entry,post,value,changed);}
        if(changed&&!s->override_logged){s->override_logged=1;background_focus_log("adapted","verified-tick-focus",entry,post,value,1);}
        else if((!post_allowed||value<0||value>1)&&!s->refusal_logged){s->refusal_logged=1;background_focus_log("refused",background_focus_reason,entry,post,value,0);}
    }
    SetLastError(forwarded);
}
static void background_focus_prepare(uintptr_t base,uintptr_t engine,int enabled) {
    DWORD error=GetLastError();BackgroundFocusState *s=&background_focus;
    if(!s->thread)s->thread=GetCurrentThreadId();
    if(GetCurrentThreadId()!=s->thread){SetLastError(error);return;}
    s->enabled=enabled==1;s->engine=engine;
    if(!s->enabled||s->installed){SetLastError(error);return;}
    DWORD now=GetTickCount();if(s->discovery_at&&now-s->discovery_at<1000){SetLastError(error);return;}s->discovery_at=now;
    s->base=base;uintptr_t pc=local_controller(engine),info=read_ptr(pc+0x198),owner=read_ptr(info+0x50);
    uintptr_t fn=guest_player_context_find_method(base,owner,"BioWorldInfo","HasFocus");
    uintptr_t tick=guest_player_context_find_method(base,owner,"BioWorldInfo","Tick");
    owner=read_ptr(fn+0x40);GameArray script={0};
    if(!fn||!tick||read_ptr(tick+0x40)!=owner||!background_focus_pins(base)||
       !guest_camera_stamp_read(base,fn,&s->function,1)||!guest_camera_stamp_read(base,tick,&s->tick,1)||
       !guest_camera_stamp_read(base,owner,&s->owner,1)||strcmp(s->function.full_name,"SFXGame.BioWorldInfo.HasFocus")||
       strcmp(s->tick.full_name,"SFXGame.BioWorldInfo.Tick")||!background_focus_method_abi(0)||!background_focus_script(&script)||
       !guest_player_context_function_abi(base,tick,"BioWorldInfo","Tick",0x20102,4,65535,0,0xf52b0,1)||
       ((fn+0xd8)&7)||!background_focus_writable(fn+0xd8,8)){SetLastError(error);return;}
    s->original=(BackgroundFocusExec)(base+0xb4fa90);
    if((uintptr_t)InterlockedCompareExchangePointer((PVOID volatile*)(fn+0xd8),(PVOID)background_focus_exec,(PVOID)s->original)==(uintptr_t)s->original) {
        s->installed=1;background_focus_log("installed","exact-native-function",-1,-1,-1,0);
    }
    SetLastError(error);
}
#endif
