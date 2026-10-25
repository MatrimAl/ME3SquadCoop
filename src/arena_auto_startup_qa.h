#ifndef ME3_ARENA_AUTO_STARTUP_QA_H
#define ME3_ARENA_AUTO_STARTUP_QA_H
/* Protected probe only. Drive the same fixed SPCer setup callbacks used by
 * normal menu confirmation and scene skipping; no plot/role/mode writes.
 * Requires arena_pair_qa + arena_pair_auto_qa and the private marker.
 * Include after guest_hit_damage_inputs.h and arena_pair_qa.h. */
typedef struct ArenaAutoStartupQA {
    DWORD next_at,started;
    int actions,selected,confirmation,weapon,record_committed,record_closed;
    GuestCameraStamp team,record;
} ArenaAutoStartupQA;
static ArenaAutoStartupQA arena_auto_startup;
static void arena_auto_startup_log(const char *step,int ok,DWORD now) {
    char row[320];snprintf(row,sizeof row,"{\"arena_auto_startup\":\"%s\",\"dispatch_validated\":%s,\"actions\":%d,\"monotonic_ms\":%lu}\r\n",step,ok?"true":"false",arena_auto_startup.actions,now);line(row);
}
static uintptr_t arena_auto_field(uintptr_t base,uintptr_t owner,const char *name) {
    uintptr_t p=read_ptr(owner+0x70),found=0;int count=0;
    for(int i=0;p&&i<1024;++i) {
        char text[96];if(!is_live(base,p)||read_ptr(p+0x40)!=owner)return 0;
        object_name(base,p,text,sizeof text);if(!strcmp(text,name)){found=p;++count;}
        uintptr_t next=read_ptr(p+0x68);if(next==p)return 0;p=next;
    }
    return !p&&count==1?found:0;
}
static int arena_auto_property(uintptr_t base,uintptr_t p,const char *kind,int offset,int size) {
    int32_t actual=-1,bytes=-1;
    return narrative_probe_object(base,p,kind)&&read_at(p+0x94,&actual,4)&&actual==offset&&read_at(p+0x74,&bytes,4)&&bytes==size;
}
static int arena_auto_call(uintptr_t base,uintptr_t object,const char *owner,const char *name,
        uint32_t flags,uint16_t bytes,unsigned char count,void *parms) {
    GuestCameraStamp before;uintptr_t fn=guest_player_context_find_method(base,read_ptr(object+0x50),owner,name);
    if(!is_live(base,object)||!is_kind_of(base,object,owner)||!guest_camera_stamp_read(base,object,&before,1)||
       !guest_player_context_function_abi(base,fn,owner,name,flags,bytes,65535,0,0xf52b0,count))return 0;
    static const unsigned char pe_pin[16]={0x40,0x55,0x41,0x56,0x41,0x57,0x48,0x81,0xec,0x90,0,0,0,0x48,0x8d,0x6c};
    unsigned char pe_code[16];uintptr_t pe=read_ptr(read_ptr(object)+0x230);uint64_t object_flags=0;
    if(!guest_camera_stamp_matches(base,&before,1)||!read_at(object+0xc,&object_flags,8)||object_flags&(UINT64_C(1)<<61)||
       pe!=base+0xf4f80||!read_at(pe,pe_code,16)||memcmp(pe_code,pe_pin,16))return 0;
    ((ProcessEvent)pe)((void*)object,(void*)fn,parms,NULL);
    ++arena_auto_startup.actions;return 1;
}
static int arena_auto_movie_active(uintptr_t base,uintptr_t object,uintptr_t world) {
    uint32_t flags=0;char name[96];
    if(!is_live(base,object)||!is_kind_of(base,object,"SFXGUIMovie"))return 0;
    object_name(base,object,name,sizeof name);
    if(!strncmp(name,"Default__",9)||read_ptr(object+0x2bc)!=world||!read_at(object+0x2e8,&flags,4)||flags&0xa)return 0;
    uintptr_t field=guest_hit_damage_field(base,read_ptr(object+0x50),"SFXGUIMovie","oWorldInfo");
    return arena_auto_property(base,field,"ObjectProperty",0x2bc,8);
}
static int arena_auto_party_ids(uintptr_t base,uintptr_t movie,int32_t ids[2]) {
    uintptr_t pool=read_ptr(movie+0x340),klass=read_ptr(pool+0x50);
    uintptr_t hp=guest_hit_damage_field(base,read_ptr(movie+0x50),"SFXGUI_TeamSelect","HenchmenPool");
    uintptr_t arr=guest_hit_damage_field(base,klass,"SFXGUIData_TeamSelect","HenchIdentities");
    if(!arena_auto_property(base,hp,"ObjectProperty",0x340,8)||!is_live(base,pool)||!is_kind_of(base,pool,"SFXGUIData_TeamSelect")||
       !arena_auto_property(base,arr,"ArrayProperty",0xa0,16))return 0;
    uintptr_t inner=read_ptr(arr+0xd0),structure=read_ptr(inner+0xd0);
    int32_t width=0;
    if(!narrative_probe_object(base,inner,"StructProperty")||!read_at(inner+0x74,&width,4)||width!=44||
       !narrative_probe_object(base,structure,"ScriptStruct"))return 0;
    char path[600];identity_path(base,structure,path,sizeof path);
    if(strcmp(path,"SFXGame.SFXGUIData_TeamSelect.SelectIdentity")||
       !arena_auto_property(base,arena_auto_field(base,structure,"MemberTag"),"NameProperty",0,8)||
       !arena_auto_property(base,arena_auto_field(base,structure,"MemberId"),"IntProperty",24,4))return 0;
    GameArray a={0},after={0};if(!read_at(pool+0xa0,&a,sizeof a)||a.count<2||a.count>32||a.max<a.count||a.max>128||!a.data)return 0;
    int matches[2]={0};ids[0]=ids[1]=-1;
    for(int i=0;i<a.count;++i) {
        uint32_t tag[2]={0};int32_t id=-1;char text[96];
        if(!read_at(a.data+(uintptr_t)i*44,tag,8)||tag[1]||!read_at(a.data+(uintptr_t)i*44+24,&id,4))return 0;
        name_of(base,tag[0],text,sizeof text);
        int slot=(!_stricmp(text,"hench_ashley")||!_stricmp(text,"Ashley"))?0:(!_stricmp(text,"hench_marine"))?1:-1;
        if(slot>=0){if(id<0||id>255)return 0;ids[slot]=id;++matches[slot];}
    }
    return matches[0]==1&&matches[1]==1&&ids[0]!=ids[1]&&read_at(pool+0xa0,&after,sizeof after)&&!memcmp(&a,&after,sizeof a);
}
static int arena_auto_startup_tick(uintptr_t base,uintptr_t engine,DWORD now) {
    ArenaAutoStartupQA *q=&arena_auto_startup;
    if(!arena_pair_qa||!arena_pair_auto_qa||!has_marker(root)||!arena_pair_world(base))return 0;
    if(!q->started)q->started=now;
    if(now-q->started>240000||q->actions>=96||now<q->next_at)return 0;
    q->next_at=now+1000;
    uintptr_t pc=local_controller(engine),world=read_ptr(pc+0x198),pawn=read_ptr(pc+0x3bc),manager=read_ptr(pc+0xc38);
    unsigned char net=255,mode=255;
    if(!is_live(base,pc)||!is_kind_of(base,pc,"SFXPlayerController")||!is_live(base,world)||!is_kind_of(base,world,"BioWorldInfo")||
       !is_live(base,pawn)||!is_kind_of(base,pawn,"SFXPawn_Player")||read_ptr(pawn+0x374)!=pc||
       !read_at(world+0x7c8,&net,1)||(net!=0&&net!=2)||!read_at(manager+0xa8,&mode,1))return 0;
    if(mode==7||mode==8) {
        const char *kind=mode==7?"SFXGameModeConversation":"SFXGameModeCinematic";
        uintptr_t target=mode_object(base,manager,kind);unsigned char parms[16]={0};
        if(!is_live(base,target)||read_ptr(target+0x40)!=pc)return 0;
        int ok=arena_auto_call(base,target,kind,mode==7?"SkipConversation":"SkipCinematic",0x20202,0,0,parms);
        arena_auto_startup_log(mode==7?"skip-conversation":"skip-cinematic",ok,now);return ok;
    }
    GameArray objects={0};uintptr_t *list=identity_object_list(base,&objects);if(!list)return 0;
    uintptr_t team=0,box=0,weapon=0,record=0;int counts[4]={0};
    for(int i=0;i<objects.count;++i) {
        uintptr_t o=list[i];if(!o)continue;char name[96];object_name(base,read_ptr(o+0x50),name,sizeof name);
        int slot=!strcmp(name,"SFXGUI_TeamSelect")?0:(!strcmp(name,"BioSFHandler_PCMessageBox")||!strcmp(name,"BioSFHandler_MessageBox"))?1:!strcmp(name,"SFXGUI_WeaponSelection")?2:!strcmp(name,"SFXGUI_SquadRecord")?3:-1;
        if(slot<0||!arena_auto_movie_active(base,o,world))continue;
        if(slot==1){uint32_t active=0;if(!read_at(o+0x3a0,&active,4)||!(active&0x100))continue;}
        if(slot==0)team=o;else if(slot==1)box=o;else if(slot==2)weapon=o;else record=o;++counts[slot];
    }
    HeapFree(GetProcessHeap(),0,list);
    unsigned char empty[16]={0};int ok=0;
    if(!q->selected&&counts[0]==1) {
        int32_t ids[2];if(!arena_auto_party_ids(base,team,ids)){arena_auto_startup_log("party-layout-unavailable",0,now);return 0;}
        if(!guest_camera_stamp_read(base,team,&q->team,1))return 0;
        ok=arena_auto_call(base,team,"SFXGUI_TeamSelect","ExSelectParty",0x20002,8,2,ids);
        q->selected=ok;arena_auto_startup_log("select-ashley-james",ok,now);return ok;
    }
    if(q->selected&&!q->confirmation&&counts[1]==1&&guest_camera_stamp_matches(base,&q->team,1)) {
        uint32_t flags=0;uintptr_t delegate=read_ptr(box+0x334);uint32_t callback[2]={0};char name[96];
        if(!read_at(box+0x3a0,&flags,4)||!(flags&0x100)||!read_at(box+0x33c,callback,8))return 0;
        name_of(base,callback[0],name,sizeof name);
        /* QueueNamedMessageBox installs the GUIInteraction queue callback on
         * the visible handler; its native queue keeps FinalizeParty. The
         * handler must belong to the same live GUI that owns our selected
         * TeamSelect object. Never invoke an unrelated active modal. */
        if(callback[1])return 0;
        int direct=delegate==q->team.pointer&&!strcmp(name,"FinalizeParty");
        int queued=0;
        if(!strcmp(name,"QueuedMessageBoxCallback")&&delegate==read_ptr(box+0x40)&&
           delegate==read_ptr(q->team.pointer+0x40)&&narrative_probe_object(base,delegate,"SFXGUIInteraction")) {
            uint32_t tag[2]={0},expected[2]={0};
            uintptr_t tag_prop=guest_hit_damage_field(base,read_ptr(box+0x50),"SFXGUIMovie","nmTag");
            uintptr_t queued_tag_prop=guest_hit_damage_field(base,read_ptr(delegate+0x50),"SFXGUIInteraction","MovieTag_QueuedMessageBox");
            uintptr_t fn=guest_player_context_find_method(base,read_ptr(delegate+0x50),"SFXGUIInteraction","QueuedMessageBoxCallback");
            uintptr_t native=read_ptr(fn+0xd8);
            queued=arena_auto_property(base,tag_prop,"NameProperty",0x2b4,8)&&
                arena_auto_property(base,queued_tag_prop,"NameProperty",0x338,8)&&
                read_at(box+0x2b4,tag,8)&&read_at(delegate+0x338,expected,8)&&tag[0]&&!memcmp(tag,expected,8)&&
                native>=base&&native<base+0x1300000&&
                guest_player_context_function_abi(base,fn,"SFXGUIInteraction","QueuedMessageBoxCallback",0x20401,8,65535,0,native-base,2);
        }
        if(!direct&&!queued)return 0;
        ok=arena_auto_call(base,box,"BioSFHandler_MessageBox","MessageBoxAPressed",0x20003,0,0,empty);
        q->confirmation=ok;arena_auto_startup_log("confirm-party-normal-messagebox",ok,now);return ok;
    }
    if(q->confirmation&&!q->weapon&&counts[2]==1) {
        uint32_t flags=0;if(!read_at(weapon+0x40c,&flags,4)||!(flags&8))return 0;
        ok=arena_auto_call(base,weapon,"SFXGUI_WeaponSelection","Exit",0x20003,0,0,empty);
        q->weapon=ok;arena_auto_startup_log("confirm-weapon-loadout",ok,now);return ok;
    }
    if(q->confirmation&&counts[3]==1&&!q->record_closed) {
        if(!q->record_committed) {
            if(!guest_camera_stamp_read(base,record,&q->record,1))return 0;
            ok=arena_auto_call(base,record,"SFXGUI_SquadRecord","ExCommit",0x20002,0,0,empty);q->record_committed=ok;
            arena_auto_startup_log("commit-character-record",ok,now);return ok;
        }
        if(!guest_camera_stamp_matches(base,&q->record,1)||record!=q->record.pointer)return 0;
        ok=arena_auto_call(base,record,"SFXGUI_SquadRecord","ExShutdownRecord",0x20002,0,0,empty);q->record_closed=ok;
        arena_auto_startup_log("confirm-character-record",ok,now);return ok;
    }
    return 0;
}
#endif
