#ifndef ME3_RUNTIME_JOIN_PROGRESS_H
#define ME3_RUNTIME_JOIN_PROGRESS_H
/* Bounded, read-only asynchronous observations after the ordinary join call.
 * SDK Engine.GameViewport +0x7a0 and GameViewportClient.ProgressMessage[2]
 * +0x78 are used only after their exact reflected property contracts pass.
 * The supported native engine progress path forwards to the viewport event;
 * these stored slots are observations, not proof of an event or a rejection.
 * No PendingLevel-private layout, hook, engine write, allocation or free.
 */
#define RUNTIME_JOIN_PROGRESS_WINDOW_MS 60000
#define RUNTIME_JOIN_PROGRESS_CADENCE_MS 250
typedef struct RuntimeJoinProgress {
    SessionHandle engine,engine_class,engine_property;
    SessionHandle viewport,viewport_class,text_property;
    DWORD started_at,last_poll;
    int active,polled,seen;
    const char *unavailable;
    RuntimeJoinText previous[2];
} RuntimeJoinProgress;
static RuntimeJoinProgress runtime_join_progress;
static uintptr_t runtime_join_progress_owner_class(uintptr_t base,uintptr_t object,const char *name) {
    uintptr_t klass=read_ptr(object+0x50),seen[48];unsigned count=0;
    while(klass&&count<48) {
        if(!is_live(base,klass)||!is_kind_of(base,klass,"Class"))return 0;
        for(unsigned i=0;i<count;++i)if(seen[i]==klass)return 0;
        seen[count++]=klass;char actual[96];object_name(base,klass,actual,sizeof actual);
        if(!strcmp(actual,name))return klass;
        if(!read_at(klass+0x60,&klass,8))return 0;
    }
    return 0;
}
static int runtime_join_progress_property_matches(uintptr_t base,SessionHandle owner,
    SessionHandle field,const char *name,const char *kind,int offset,int dim,int size,uint64_t flags) {
    int got_dim=0,got_size=0,got_offset=0;uint64_t got_flags=0;char actual[96];
    if(!session_handle_matches(owner)||!session_handle_matches(field)||
       !is_live(base,owner.pointer)||!is_kind_of(base,owner.pointer,"Class")||
       !is_live(base,field.pointer)||!is_kind_of(base,field.pointer,kind)||
       read_ptr(field.pointer+0x40)!=owner.pointer)return 0;
    object_name(base,field.pointer,actual,sizeof actual);
    return !strcmp(actual,name)&&
        read_at(field.pointer+0x70,&got_dim,4)&&got_dim==dim&&
        read_at(field.pointer+0x74,&got_size,4)&&got_size==size&&
        read_at(field.pointer+0x94,&got_offset,4)&&got_offset==offset&&
        read_at(field.pointer+0x78,&got_flags,8)&&got_flags==flags;
}
static SessionHandle runtime_join_progress_find_property(uintptr_t base,SessionHandle owner,
    const char *name,const char *kind,int offset,int dim,int size,uint64_t flags) {
    SessionHandle result={0};uintptr_t field=read_ptr(owner.pointer+0x70),seen[512];
    unsigned count=0,matches=0;
    while(field&&count<512) {
        if(!is_live(base,field))return (SessionHandle){0};
        for(unsigned i=0;i<count;++i)if(seen[i]==field)return (SessionHandle){0};
        seen[count++]=field;char actual[96];object_name(base,field,actual,sizeof actual);
        if(!strcmp(actual,name)) {
            result=session_capture_handle(field);
            if(++matches!=1||!runtime_join_progress_property_matches(base,owner,result,
                name,kind,offset,dim,size,flags))return (SessionHandle){0};
        }
        if(!read_at(field+0x68,&field,8))return (SessionHandle){0};
    }
    return !field&&matches==1?result:(SessionHandle){0};
}
static int runtime_join_progress_engine_layout(uintptr_t base,const RuntimeJoinProgress *state) {
    return session_handle_matches(state->engine)&&is_live(base,state->engine.pointer)&&
        is_kind_of(base,state->engine.pointer,"Engine")&&
        read_ptr(base+0x18b31d0)==state->engine.pointer&&
        runtime_join_progress_owner_class(base,state->engine.pointer,"Engine")==state->engine_class.pointer&&
        runtime_join_progress_property_matches(base,state->engine_class,state->engine_property,
            "GameViewport","ObjectProperty",0x7a0,1,8,UINT64_C(2));
}
static void runtime_join_progress_unavailable(RuntimeJoinProgress *state,const char *reason) {
    if(state->unavailable&&!strcmp(state->unavailable,reason))return;
    state->unavailable=reason;char row[280];
    snprintf(row,sizeof row,
        "{\"runtime_network\":\"join-progress-unavailable\",\"reason\":\"%s\",\"evidence_source\":\"reflected-viewport-progress-slots\",\"connection_verified\":false}\r\n",reason);
    line(row);
}
static void runtime_join_progress_begin(uintptr_t engine,DWORD now) {
    RuntimeJoinProgress *state=&runtime_join_progress;
    memset(state,0,sizeof *state);state->active=1;state->started_at=now;
    uintptr_t base=(uintptr_t)GetModuleHandleW(NULL);
    state->engine=session_capture_handle(engine);
    if(!session_handle_matches(state->engine)||!is_live(base,engine)||!is_kind_of(base,engine,"Engine")) {
        state->active=0;runtime_join_progress_unavailable(state,"engine-identity-unverified");return;
    }
    state->engine_class=session_capture_handle(runtime_join_progress_owner_class(base,engine,"Engine"));
    state->engine_property=runtime_join_progress_find_property(base,state->engine_class,
        "GameViewport","ObjectProperty",0x7a0,1,8,UINT64_C(2));
    if(!runtime_join_progress_engine_layout(base,state)) {
        state->active=0;runtime_join_progress_unavailable(state,"engine-layout-unverified");
    }
}
/* -1 expires the window, zero skips an observation, one permits one sample.
 * DWORD subtraction deliberately supports the native tick counter wrapping. */
static int runtime_join_progress_due(RuntimeJoinProgress *state,DWORD now) {
    if(!state->active)return 0;
    if(now-state->started_at>=RUNTIME_JOIN_PROGRESS_WINDOW_MS){state->active=0;return -1;}
    if(state->polled&&now-state->last_poll<RUNTIME_JOIN_PROGRESS_CADENCE_MS)return 0;
    state->polled=1;state->last_poll=now;return 1;
}
static int runtime_join_progress_changed(RuntimeJoinProgress *state,const RuntimeJoinText current[2]) {
    int changed=!state->seen||state->unavailable!=NULL;
    for(unsigned i=0;i<2;++i)if(state->previous[i].status!=current[i].status||
        state->previous[i].declared_units!=current[i].declared_units||
        state->previous[i].copied_units!=current[i].copied_units||
        strcmp(state->previous[i].escaped,current[i].escaped))changed=1;
    if(changed){memcpy(state->previous,current,sizeof state->previous);state->seen=1;}
    state->unavailable=NULL;return changed;
}
static void runtime_join_progress_poll(uintptr_t base,uintptr_t engine,DWORD now) {
    RuntimeJoinProgress *state=&runtime_join_progress;int due=runtime_join_progress_due(state,now);
    if(!due)return;
    if(due<0) {
        line("{\"runtime_network\":\"join-progress-window-closed\",\"evidence_source\":\"bounded-viewport-poll\",\"connection_verified\":false}\r\n");return;
    }
    if(engine!=state->engine.pointer||!runtime_join_progress_engine_layout(base,state)) {
        state->active=0;runtime_join_progress_unavailable(state,"engine-identity-or-layout-changed");return;
    }
    uintptr_t viewport=0;
    if(!read_at(engine+0x7a0,&viewport,8)||!is_live(base,viewport)||
       !is_kind_of(base,viewport,"GameViewportClient")) {
        runtime_join_progress_unavailable(state,"current-viewport-unavailable");return;
    }
    if(viewport!=state->viewport.pointer||!session_handle_matches(state->viewport)) {
        state->viewport=session_capture_handle(viewport);state->seen=0;
        state->viewport_class=session_capture_handle(runtime_join_progress_owner_class(base,viewport,"GameViewportClient"));
        state->text_property=runtime_join_progress_find_property(base,state->viewport_class,
            "ProgressMessage","StrProperty",0x78,2,16,UINT64_C(0x400000));
    }
    if(!runtime_join_progress_property_matches(base,state->viewport_class,state->text_property,
        "ProgressMessage","StrProperty",0x78,2,16,UINT64_C(0x400000))) {
        state->active=0;runtime_join_progress_unavailable(state,"viewport-layout-unverified");return;
    }
    GameString before[2],after[2];RuntimeJoinText current[2];
    if(!read_at(viewport+0x78,before,sizeof before)) {
        runtime_join_progress_unavailable(state,"viewport-headers-unreadable");return;
    }
    for(unsigned i=0;i<2;++i)current[i]=runtime_join_capture_console_result(
        (const GameString*)(viewport+0x78+i*sizeof(GameString)));
    if(!read_at(viewport+0x78,after,sizeof after)||memcmp(before,after,sizeof before)||
       !runtime_join_progress_engine_layout(base,state)||!session_handle_matches(state->viewport)||
       !is_live(base,viewport)||read_ptr(engine+0x7a0)!=viewport||
       runtime_join_progress_owner_class(base,viewport,"GameViewportClient")!=state->viewport_class.pointer||
       !runtime_join_progress_property_matches(base,state->viewport_class,state->text_property,
           "ProgressMessage","StrProperty",0x78,2,16,UINT64_C(0x400000))) {
        runtime_join_progress_unavailable(state,"viewport-identity-layout-or-headers-changed");return;
    }
    if(!runtime_join_progress_changed(state,current))return;
    char row[RUNTIME_JOIN_JSON_BYTES*2+600];
    snprintf(row,sizeof row,
        "{\"runtime_network\":\"join-progress-sample\",\"evidence_source\":\"GameViewportClient.ProgressMessage[2]\",\"sampling\":\"read-only-after-native-tick\",\"elapsed_ms\":%lu,\"slots\":[{\"index\":0,\"text_status\":\"%s\",\"declared_utf16_units\":%d,\"text\":\"%s\"},{\"index\":1,\"text_status\":\"%s\",\"declared_utf16_units\":%d,\"text\":\"%s\"}],\"native_rejection_verified\":false,\"connection_verified\":false}\r\n",
        (unsigned long)(now-state->started_at),runtime_join_text_status_name(current[0].status),
        current[0].declared_units,current[0].escaped,runtime_join_text_status_name(current[1].status),
        current[1].declared_units,current[1].escaped);
    line(row);
}
static int runtime_join_progress_selftest(void) {
    RuntimeJoinProgress state={0};state.active=1;
    if(runtime_join_progress_due(&state,0)!=1||runtime_join_progress_due(&state,249)!=0||
       runtime_join_progress_due(&state,250)!=1||runtime_join_progress_due(&state,59999)!=1||
       runtime_join_progress_due(&state,60000)!=-1||state.active||runtime_join_progress_due(&state,60250))return 0;
    memset(&state,0,sizeof state);state.active=1;state.started_at=UINT32_MAX-15;
    if(runtime_join_progress_due(&state,UINT32_MAX-15)!=1||runtime_join_progress_due(&state,234)!=1||
       runtime_join_progress_due(&state,59984)!=-1)return 0;
    RuntimeJoinText current[2]={{0},{0}};current[0].status=current[1].status=RUNTIME_JOIN_TEXT_EMPTY;
    if(!runtime_join_progress_changed(&state,current)||runtime_join_progress_changed(&state,current))return 0;
    current[1].status=RUNTIME_JOIN_TEXT_VALID;strcpy(current[1].escaped,"native failure");
    current[1].declared_units=15;current[1].copied_units=14;
    if(!runtime_join_progress_changed(&state,current)||runtime_join_progress_changed(&state,current))return 0;
    current[1].status=RUNTIME_JOIN_TEXT_UNREADABLE;current[1].escaped[0]=0;
    if(!runtime_join_progress_changed(&state,current)||runtime_join_progress_changed(&state,current))return 0;
    state.unavailable="viewport-headers-unreadable";
    if(!runtime_join_progress_changed(&state,current)||state.unavailable||runtime_join_progress_changed(&state,current))return 0;
    state.seen=0;return runtime_join_progress_changed(&state,current);
}
#endif
