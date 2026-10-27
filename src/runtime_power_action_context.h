#ifndef ME3_RUNTIME_POWER_ACTION_CONTEXT_H
#define ME3_RUNTIME_POWER_ACTION_CONTEXT_H
/* Existing idle power instances only. Stock VerifyCAHasBeenInstanced binds
 * m_oPawn/m_oAI/m_oPC only when constructing a missing instance. This lease
 * changes only the two controller caches of an already Ashley-owned power;
 * no action array, pawn binding, rank, cooldown or lifecycle is replayed. */
typedef struct RuntimePowerActionContextRow {
    GuestCameraStamp action,ai;
    uintptr_t old_ai;
    unsigned changed;
} RuntimePowerActionContextRow;
typedef struct RuntimePowerActionContext {
    RuntimePowerCatalog catalog;
    RuntimePowerActionContextRow rows[RUNTIME_POWER_LIMIT];
    unsigned count;
    int active,retiring;
} RuntimePowerActionContext;
static RuntimePowerActionContext runtime_power_action_context;
static unsigned runtime_power_action_context_events;
static int runtime_power_action_context_enabled(void) {
    static int configured,opted;
    if(!configured){wchar_t text[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_POWER_ACTION_CONTEXT",text,8);
        opted=!n||(n==1&&text[0]==L'1');configured=1;}
    return opted;
}
static int runtime_power_action_context_pointer(uintptr_t at,uintptr_t expected,uintptr_t desired) {
    if((at&7)||!background_focus_writable(at,8))return 0;
    return (uintptr_t)InterlockedCompareExchangePointer((PVOID volatile*)at,(PVOID)desired,(PVOID)expected)==expected;
}
static int runtime_power_action_context_layout(uintptr_t base,uintptr_t action) {
    static const char *names[]={"m_oPawn","m_oPC","m_oAI"};
    static const char *classes[]={"BioPawn","SFXPlayerController","SFXAI_Core"};
    static const int offsets[]={0x98,0xa0,0xa8};
    uintptr_t klass=read_ptr(action+0x50),seen[48]={0},declaring=0;
    for(unsigned n=0;klass&&n<48;++n){char name[96];
        if(!narrative_probe_object(base,klass,"Class"))return 0;
        for(unsigned j=0;j<n;++j)if(seen[j]==klass)return 0;seen[n]=klass;
        object_name(base,klass,name,sizeof name);
        if(!strcmp(name,"BioCustomAction")){declaring=klass;break;}
        klass=read_ptr(klass+0x60);
    }
    GuestCameraStamp owner;
    if(!declaring||!guest_camera_stamp_read(base,declaring,&owner,1)||strcmp(owner.full_name,"SFXGame.BioCustomAction"))return 0;
    for(unsigned index=0;index<3;++index){uintptr_t field=read_ptr(declaring+0x70);int found=0;
        for(unsigned n=0;field&&n<256;++n){char name[96];
            if(!is_live(base,field)||read_ptr(field+0x40)!=declaring)return 0;
            object_name(base,field,name,sizeof name);
            if(!strcmp(name,names[index])){int32_t at=-1,size=0,dim=0;uint64_t flags=1;GuestCameraStamp target;char path[120];
                snprintf(path,sizeof path,"SFXGame.%s",classes[index]);
                if(found++||!narrative_probe_object(base,field,"ObjectProperty")||
                   !read_at(field+0x94,&at,4)||at!=offsets[index]||!read_at(field+0x74,&size,4)||size!=8||
                   !read_at(field+0x70,&dim,4)||dim!=1||!read_at(field+0x78,&flags,8)||flags||
                   !guest_camera_stamp_read(base,read_ptr(field+0xd0),&target,1)||strcmp(target.full_name,path)||
                   !narrative_probe_object(base,target.pointer,"Class"))return 0;
            }
            field=read_ptr(field+0x68);
        }
        if(field||found!=1)return 0;
    }
    return guest_camera_stamp_matches(base,&owner,1);
}
static int runtime_power_action_context_idle(uintptr_t pawn,uintptr_t pc) {
    int32_t action=-1,power=-1;unsigned char ignored=255;uint32_t life=0;
    return read_at(pawn+0xc20,&action,4)&&!action&&read_at(pawn+0xc28,&power,4)&&!power&&
        read_at(pc+0x782,&ignored,1)&&!ignored&&read_at(pawn+0xe20,&life,4)&&!(life&0x300);
}
static int runtime_power_action_context_action_idle(uintptr_t action) {
    uint32_t flags=0;
    return read_at(action+0x100,&flags,4)&&!(flags&0x10800000)&&
        !read_ptr(action+0xb0)&&!read_ptr(action+0xb8);
}
static int runtime_power_action_context_ai(uintptr_t base,uintptr_t ai,uintptr_t pawn,uintptr_t world) {
    if(!ai)return 1;
    uintptr_t controlled=read_ptr(ai+0x3bc);
    return guest_camera_actor_live(base,ai)&&is_kind_of(base,ai,"SFXAI_Core")&&
        read_ptr(ai+0x198)==world&&(!controlled||controlled==pawn);
}
static int runtime_power_action_context_host_ai(uintptr_t ai,uintptr_t captured,int live,uintptr_t controlled) {
    return !ai||(ai==captured&&live&&!controlled);
}
/* The authoritative remote controller has no local PlayerInput. Its power
 * context is admitted through the existing captured host possession lease,
 * not through guest UI/catalog registration or any revive package setting. */
static int runtime_power_action_context_host_scope(uintptr_t base,SessionContext c,RuntimePowerCatalog *out) {
    SessionContext host=session_context(session_engine);RuntimeReviveAdmission admitted;
    if(GetCurrentThreadId()!=session_game_thread||g_runtime_config.role!=ME3_ROLE_HOST||
       !runtime_control_enabled||!runtime_scope_ready||!InterlockedCompareExchange(&g_enable_requested,0,0)||
       c.pc!=session_remote||c.pawn!=session_pawn||c.world!=host.world||
       !guest_camera_actor_live(base,c.pc)||!is_kind_of(base,c.pc,"SFXPlayerController")||
       !runtime_revive_read_admission(base,host,session_pawn,session_epoch,&admitted)||
       !session_handle_matches(session_pc_handle)||!session_handle_matches(session_ai_handle))return 0;
    uint32_t life=0;uintptr_t manager=read_ptr(c.pawn+0xc18);GameArray array={0},again={0};
    if(!read_at(c.pawn+0xe20,&life,4)||(life&0x300)||
       !narrative_probe_object(base,manager,"SFXPowerManager")||read_ptr(manager+0xd0)!=c.pawn||
       !runtime_power_property(base,c.pawn,"BioPawn","PowerManager","ObjectProperty",0xc18,8)||
       !runtime_power_property(base,manager,"SFXPowerManager","Powers","ArrayProperty",0x60,16)||
       !runtime_power_property(base,manager,"SFXPowerManager","MyPawn","ObjectProperty",0xd0,8)||
       !identity_array(manager+0x60,&array,RUNTIME_POWER_LIMIT)||!array.count)return 0;
    memset(out,0,sizeof *out);out->pc=session_pc_handle;out->pawn=session_pawn_handle;
    out->info=session_world_handle;out->manager=session_capture_handle(manager);out->array=array;
    out->channel=session_capture_handle(admitted.channel.pointer);out->connection=session_connection_handle;
    out->driver=session_capture_handle(admitted.driver.pointer);out->registration_token=session_epoch;
    if(!session_handle_matches(out->manager)||!session_handle_matches(out->channel)||!session_handle_matches(out->driver))return 0;
    uintptr_t sampled[RUNTIME_POWER_LIMIT]={0},declaring=0;
    for(int i=0;i<array.count;++i) {
        uintptr_t action=0;if(!read_at(array.data+(SIZE_T)i*8,&action,8))return 0;
        sampled[i]=action;if(!action)continue;
        if(!narrative_probe_object(base,action,"SFXPowerCustomActionBase"))return 0;
        uintptr_t owner=runtime_power_declaring_base(base,action);uint32_t flags=0;float rank=0;
        if(!owner)return 0;
        if(!declaring) {
            declaring=owner;
            if(!runtime_power_property(base,action,"SFXPowerCustomActionBase","Rank","FloatProperty",0x488,4)||
               !runtime_power_property(base,action,"SFXPowerCustomActionBase","PowerName","NameProperty",0x46c,8)||
               !runtime_power_property(base,action,"SFXPowerCustomActionBase","bEnabled","BoolProperty",0x4dc,4)||
               !runtime_power_property(base,action,"SFXPowerCustomActionBase","DisplayInHUD","BoolProperty",0x4dc,4))return 0;
        }
        if(owner!=declaring||!read_at(action+0x488,&rank,4)||!isfinite(rank)||!read_at(action+0x4dc,&flags,4))return 0;
        if(rank<=0||(flags&0x101)!=0x101)continue;
        int index=out->count;
        out->powers[index]=session_capture_handle(action);out->source_indices[index]=i;
        if(!session_handle_matches(out->powers[index])||!read_at(action+0x46c,out->names[index],8))return 0;
        ++out->count;
    }
    for(int i=0;i<array.count;++i){uintptr_t action=0;
        if(!read_at(array.data+(SIZE_T)i*8,&action,8)||action!=sampled[i])return 0;}
    RuntimeReviveAdmission checked;
    return out->count&&read_at(manager+0x60,&again,sizeof again)&&!memcmp(&array,&again,sizeof array)&&
        read_ptr(c.pawn+0xc18)==manager&&read_ptr(manager+0xd0)==c.pawn&&
        runtime_revive_read_admission(base,session_context(session_engine),c.pawn,session_epoch,&checked)&&
        runtime_revive_scope_same(&admitted,&checked);
}
static int runtime_power_action_context_catalog_same(const RuntimePowerCatalog *a,const RuntimePowerCatalog *b) {
    if(g_runtime_config.role!=ME3_ROLE_HOST)return runtime_power_catalog_same(a,b);
    /* Host remote scope deliberately has no local input object. */
    if(a->count<1||a->count>RUNTIME_POWER_LIMIT||a->input.pointer||b->input.pointer||!runtime_power_same_handle(a->pc,b->pc)||
       !runtime_power_same_handle(a->pawn,b->pawn)||!runtime_power_same_handle(a->info,b->info)||
       !runtime_power_same_handle(a->manager,b->manager)||!runtime_power_same_handle(a->channel,b->channel)||
       !runtime_power_same_handle(a->connection,b->connection)||!runtime_power_same_handle(a->driver,b->driver)||
       !a->registration_token||a->registration_token!=b->registration_token||a->count!=b->count||
       memcmp(&a->array,&b->array,sizeof a->array))return 0;
    for(int i=0;i<a->count;++i)if(a->source_indices[i]!=b->source_indices[i]||
       !runtime_power_same_handle(a->powers[i],b->powers[i])||memcmp(a->names[i],b->names[i],8))return 0;
    return 1;
}
static int runtime_power_action_context_scope(uintptr_t base,SessionContext c,RuntimePowerCatalog *catalog) {
    if(g_runtime_config.role==ME3_ROLE_HOST)return runtime_power_action_context_host_scope(base,c,catalog);
    return guest_player_context_registered_for(c.pc,c.pawn)&&
        guest_player_context_basic_owned(base,c.pc,c.pawn,c.world)&&
        runtime_power_read_membership(base,c,catalog);
}
static void runtime_power_action_context_log(const char *event,unsigned count) {
    if(runtime_power_action_context_events++>=64)return;char row[250];
    snprintf(row,sizeof row,"{\"runtime_power_action_context\":\"%s\",\"role\":%d,\"instances\":%u,\"action_started\":false,\"cast_verified\":false,\"monotonic_ms\":%lu}\r\n",event,(int)g_runtime_config.role,count,GetTickCount());line(row);
}
static int runtime_power_action_context_cleanup_idle(uintptr_t base,const RuntimePowerCatalog *catalog) {
    uintptr_t pawn=catalog->pawn.pointer,pc=catalog->pc.pointer;
    if(g_runtime_config.role!=ME3_ROLE_HOST)return runtime_power_action_context_idle(pawn,pc);
    int32_t action=-1,power=-1;uint32_t life=0;unsigned char ignored=255;
    if(!read_at(pawn+0xc20,&action,4)||action||!read_at(pawn+0xc28,&power,4)||power||
       !read_at(pawn+0xe20,&life,4)||(life&0x300))return 0;
    /* A disconnected PC need not remain live to clear our exact cached
     * pointer on an idle, stamped action. Never dereference a disposed PC. */
    if(read_ptr(pawn+0x374)!=pc||!session_handle_matches(catalog->pc)||!guest_camera_actor_live(base,pc))return 1;
    return read_at(pc+0x782,&ignored,1)&&!ignored;
}
static int runtime_power_action_context_restore_ai_allowed(uintptr_t controller,uintptr_t pc,uintptr_t old_ai) {
    return !controller||controller==pc||controller==old_ai;
}
/* Return1 only when no owned pointer changes remain. Context release must not
 * fabricate idle state: a running power keeps pending cleanup until it ends. */
static int runtime_power_action_context_retire(uintptr_t base,SessionContext c,int ordinary,int enabled,int menu_held) {
    RuntimePowerActionContext *lease=&runtime_power_action_context;
    if(!lease->active)return 1;
    RuntimePowerCatalog current;int same=runtime_power_action_context_scope(base,c,&current)&&
        runtime_power_action_context_catalog_same(&lease->catalog,&current);
    if(!lease->retiring&&enabled&&runtime_power_action_context_enabled()&&
       same&&(ordinary||menu_held))return 1;
    lease->retiring=1;
    /* World/pawn disposal retires bookkeeping only, never recycled objects. */
    if(!session_handle_matches(lease->catalog.info)||!guest_camera_actor_live(base,lease->catalog.info.pointer)||
       !session_handle_matches(lease->catalog.pawn)||!guest_camera_actor_live(base,lease->catalog.pawn.pointer)){
        memset(lease,0,sizeof *lease);return 1;
    }
    uintptr_t pawn=lease->catalog.pawn.pointer,pc=lease->catalog.pc.pointer,world=lease->catalog.info.pointer;
    SessionContext environment=g_runtime_config.role==ME3_ROLE_HOST?session_context(session_engine):c;
    if(environment.world!=world||(g_runtime_config.role!=ME3_ROLE_HOST&&!session_handle_matches(lease->catalog.pc))||
       read_ptr(pawn+0x198)!=world||menu_held||session_engine_blocked(environment)||environment.mode!=0||
       !runtime_power_action_context_cleanup_idle(base,&lease->catalog))return 0;
    unsigned pending=0;
    for(unsigned n=0;n<lease->count;++n){RuntimePowerActionContextRow *row=&lease->rows[n];
        if(!row->changed)continue;
        uintptr_t action=row->action.pointer;
        if(!guest_camera_stamp_matches(base,&row->action,1)||!is_live(base,action)) {row->changed=0;continue;}
        if(!runtime_power_action_context_layout(base,action)||read_ptr(action+0x98)!=pawn||
           !runtime_power_action_context_action_idle(action)) {++pending;continue;}
        uintptr_t current_pc=read_ptr(action+0xa0),current_ai=read_ptr(action+0xa8);
        /* A native/external context replacement owns its own pair. Preserve
         * it rather than append an old AI beside an unrelated controller. */
        if((current_pc&&current_pc!=pc)||current_ai){row->changed=0;continue;}
        if((row->changed&1)&&current_pc==pc){
            if(!runtime_power_action_context_cleanup_idle(base,&lease->catalog)||
               !runtime_power_action_context_pointer(action+0xa0,pc,0)){++pending;continue;}
            row->changed&=~1u;
        } else if(current_pc==0)row->changed&=~1u;
        if(row->changed&2){
            uintptr_t controller=read_ptr(pawn+0x374);
            if(g_runtime_config.role==ME3_ROLE_HOST&&!runtime_power_action_context_restore_ai_allowed(controller,pc,row->old_ai)) {
                row->changed&=~2u;continue; /* never append the old AI to a different owner's context */
            }
            if(row->old_ai&&(!guest_camera_stamp_matches(base,&row->ai,1)||
               !runtime_power_action_context_ai(base,row->old_ai,pawn,world))){row->changed&=~2u;continue;}
            if(!runtime_power_action_context_cleanup_idle(base,&lease->catalog)||read_ptr(action+0xa0)||
               !runtime_power_action_context_pointer(action+0xa8,0,row->old_ai)){++pending;continue;}
            row->changed&=~2u;
        }
        if(row->changed)++pending;
    }
    if(pending)return 0;
    runtime_power_action_context_log("retired",lease->count);memset(lease,0,sizeof *lease);return 1;
}
static void runtime_power_action_context_prepare(uintptr_t base,SessionContext c,int ordinary,int enabled) {
    RuntimePowerActionContext *lease=&runtime_power_action_context;RuntimePowerCatalog catalog;
    if(lease->active||GetCurrentThreadId()!=session_game_thread||!enabled||!ordinary||
       (g_runtime_config.role!=ME3_ROLE_GUEST&&g_runtime_config.role!=ME3_ROLE_HOST)||!runtime_power_action_context_enabled()||
       !runtime_power_action_context_idle(c.pawn,c.pc)||!runtime_power_action_context_scope(base,c,&catalog))return;
    RuntimePowerActionContext candidate={0};candidate.catalog=catalog;
    for(int n=0;n<catalog.count;++n){uintptr_t action=catalog.powers[n].pointer,ai=read_ptr(action+0xa8);
        if(!runtime_power_action_context_layout(base,action)||read_ptr(action+0x98)!=c.pawn||read_ptr(action+0xa0)||
           !runtime_power_action_context_action_idle(action)||!runtime_power_action_context_ai(base,ai,c.pawn,c.world))continue;
        if(g_runtime_config.role==ME3_ROLE_HOST&&!runtime_power_action_context_host_ai(ai,session_ai,
            session_handle_matches(session_ai_handle),ai?read_ptr(ai+0x3bc):0))continue;
        RuntimePowerActionContextRow *row=&candidate.rows[candidate.count];
        if(!guest_camera_stamp_read(base,action,&row->action,1)||(ai&&!guest_camera_stamp_read(base,ai,&row->ai,1)))continue;
        row->old_ai=ai;++candidate.count;
    }
    if(!candidate.count)return;
    RuntimePowerCatalog checked;
    if(!runtime_power_action_context_idle(c.pawn,c.pc)||!runtime_power_action_context_scope(base,c,&checked)||
       !runtime_power_action_context_catalog_same(&catalog,&checked))return;
    candidate.active=1;*lease=candidate;
    for(unsigned n=0;n<lease->count;++n){RuntimePowerActionContextRow *row=&lease->rows[n];uintptr_t action=row->action.pointer;
        if(!guest_camera_stamp_matches(base,&row->action,1)||read_ptr(action+0x98)!=c.pawn||
           !runtime_power_action_context_idle(c.pawn,c.pc)||!runtime_power_action_context_action_idle(action)||
           read_ptr(action+0xa0)||read_ptr(action+0xa8)!=row->old_ai||
           (g_runtime_config.role==ME3_ROLE_HOST&&!runtime_power_action_context_host_ai(row->old_ai,session_ai,
                session_handle_matches(session_ai_handle),row->old_ai?read_ptr(row->old_ai+0x3bc):0))||
           (row->old_ai&&(!guest_camera_stamp_matches(base,&row->ai,1)||
             !runtime_power_action_context_ai(base,row->old_ai,c.pawn,c.world)))||
           !runtime_power_action_context_scope(base,c,&checked)||!runtime_power_action_context_catalog_same(&catalog,&checked)) {lease->retiring=1;break;}
        if(row->old_ai){
            row->changed|=2;
            if(!runtime_power_action_context_pointer(action+0xa8,row->old_ai,0)){row->changed&=~2u;lease->retiring=1;break;}
        }
        if(!runtime_power_action_context_idle(c.pawn,c.pc)||read_ptr(action+0x98)!=c.pawn||read_ptr(action+0xa8)||
           !guest_camera_stamp_matches(base,&row->action,1)||!runtime_power_action_context_action_idle(action)||
           !runtime_power_action_context_scope(base,c,&checked)||!runtime_power_action_context_catalog_same(&catalog,&checked)) {lease->retiring=1;break;}
        row->changed|=1;
        if(!runtime_power_action_context_pointer(action+0xa0,0,c.pc)){row->changed&=~1u;lease->retiring=1;break;}
        if(read_ptr(action+0xa0)!=c.pc||read_ptr(action+0xa8)||read_ptr(action+0x98)!=c.pawn||
           !runtime_power_action_context_idle(c.pawn,c.pc)||!runtime_power_action_context_scope(base,c,&checked)||
           !runtime_power_action_context_catalog_same(&catalog,&checked)){lease->retiring=1;break;}
    }
    runtime_power_action_context_log(lease->retiring?"partial-pending-retirement":"idle-controller-context-bound",lease->count);
}
static int runtime_power_action_context_host_selftest(void) {
    RuntimePowerCatalog a={0},b;Me3RuntimeRole saved=g_runtime_config.role;
    g_runtime_config.role=ME3_ROLE_HOST;
    SessionHandle *handles[]={&a.pc,&a.pawn,&a.info,&a.manager,&a.channel,&a.connection,&a.driver,&a.powers[0]};
    for(unsigned i=0;i<8;++i)*handles[i]=(SessionHandle){0x1000+i*8,(int)i+1,(uint64_t)i+10};
    a.registration_token=7;a.count=1;a.array=(GameArray){0x2000,2,2};a.names[0][0]=21;
    b=a;int ok=runtime_power_action_context_catalog_same(&a,&b);
    for(unsigned i=0;i<8;++i){handles[i]->index++;ok=ok&&!runtime_power_action_context_catalog_same(&a,&b);handles[i]->index--;}
    a.registration_token++;ok=ok&&!runtime_power_action_context_catalog_same(&a,&b);a=b;
    a.array.data++;ok=ok&&!runtime_power_action_context_catalog_same(&a,&b);a=b;
    a.names[0][1]++;ok=ok&&!runtime_power_action_context_catalog_same(&a,&b);a=b;
    a.source_indices[0]++;ok=ok&&!runtime_power_action_context_catalog_same(&a,&b);a=b;
    a.input=a.pc;ok=ok&&!runtime_power_action_context_catalog_same(&a,&b);a=b;
    unsigned char pawn[0xe24]={0};uintptr_t disposed_pc=1;int32_t action=5;uint32_t life=0x100;
    a.pawn.pointer=(uintptr_t)pawn;a.pc=(SessionHandle){disposed_pc,1,1};
    memcpy(pawn+0x374,&disposed_pc,8);
    ok=ok&&runtime_power_action_context_cleanup_idle(0,&a);
    memcpy(pawn+0xc20,&action,4);ok=ok&&!runtime_power_action_context_cleanup_idle(0,&a);memset(pawn+0xc20,0,4);
    memcpy(pawn+0xc28,&action,4);ok=ok&&!runtime_power_action_context_cleanup_idle(0,&a);memset(pawn+0xc28,0,4);
    memcpy(pawn+0xe20,&life,4);ok=ok&&!runtime_power_action_context_cleanup_idle(0,&a);
    ok=ok&&runtime_power_action_context_restore_ai_allowed(0,1,2)&&
        runtime_power_action_context_restore_ai_allowed(1,1,2)&&runtime_power_action_context_restore_ai_allowed(2,1,2)&&
        !runtime_power_action_context_restore_ai_allowed(3,1,2);
    ok=ok&&runtime_power_action_context_host_ai(0,2,0,0)&&runtime_power_action_context_host_ai(2,2,1,0)&&
        !runtime_power_action_context_host_ai(3,2,1,0)&&!runtime_power_action_context_host_ai(2,2,0,0)&&
        !runtime_power_action_context_host_ai(2,2,1,1);
    g_runtime_config.role=saved;return ok;
}
static int runtime_power_action_context_selftest(void) {
    uintptr_t fields[2]={0,0x1230};uintptr_t pc=(uintptr_t)0x4560;
    /* Install's second step lost a race: rollback only our zeroed AI, keeping
     * a foreign PC untouched. An externally replaced AI must also survive. */
    if(!runtime_power_action_context_pointer((uintptr_t)&fields[1],0x1230,0))return 0;
    fields[0]=0x7890;
    if(runtime_power_action_context_pointer((uintptr_t)&fields[0],0,pc)||fields[0]!=0x7890)return 0;
    fields[1]=0xabc0;
    if(runtime_power_action_context_pointer((uintptr_t)&fields[1],0,0x1230)||fields[1]!=0xabc0)return 0;
    fields[0]=0;fields[1]=0;
    if(!runtime_power_action_context_pointer((uintptr_t)&fields[1],0,0x1230)||fields[0]||fields[1]!=0x1230)return 0;
    return runtime_power_action_context_host_selftest();
}
#endif
