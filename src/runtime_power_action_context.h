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
static int runtime_power_action_context_scope(uintptr_t base,SessionContext c,RuntimePowerCatalog *catalog) {
    return guest_player_context_registered_for(c.pc,c.pawn)&&
        guest_player_context_basic_owned(base,c.pc,c.pawn,c.world)&&
        runtime_power_read_membership(base,c,catalog);
}
static void runtime_power_action_context_log(const char *event,unsigned count) {
    if(runtime_power_action_context_events++>=64)return;char row[250];
    snprintf(row,sizeof row,"{\"runtime_power_action_context\":\"%s\",\"instances\":%u,\"action_started\":false,\"cast_verified\":false,\"monotonic_ms\":%lu}\r\n",event,count,GetTickCount());line(row);
}
/* Return1 only when no owned pointer changes remain. Context release must not
 * fabricate idle state: a running power keeps pending cleanup until it ends. */
static int runtime_power_action_context_retire(uintptr_t base,SessionContext c,int ordinary,int enabled,int menu_held) {
    RuntimePowerActionContext *lease=&runtime_power_action_context;
    if(!lease->active)return 1;
    RuntimePowerCatalog current;int same=runtime_power_action_context_scope(base,c,&current)&&
        runtime_power_catalog_same(&lease->catalog,&current);
    if(!lease->retiring&&enabled&&runtime_power_action_context_enabled()&&g_runtime_config.role==ME3_ROLE_GUEST&&
       same&&(ordinary||menu_held))return 1;
    lease->retiring=1;
    /* World/pawn disposal retires bookkeeping only, never recycled objects. */
    if(!session_handle_matches(lease->catalog.info)||!guest_camera_actor_live(base,lease->catalog.info.pointer)||
       !session_handle_matches(lease->catalog.pawn)||!guest_camera_actor_live(base,lease->catalog.pawn.pointer)){
        memset(lease,0,sizeof *lease);return 1;
    }
    uintptr_t pawn=lease->catalog.pawn.pointer,pc=lease->catalog.pc.pointer,world=lease->catalog.info.pointer;
    if(c.world!=world||!session_handle_matches(lease->catalog.pc)||read_ptr(pawn+0x198)!=world||
       menu_held||session_engine_blocked(c)||c.mode!=0||!runtime_power_action_context_idle(pawn,pc))return 0;
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
            if(!runtime_power_action_context_idle(pawn,pc)||
               !runtime_power_action_context_pointer(action+0xa0,pc,0)){++pending;continue;}
            row->changed&=~1u;
        } else if(current_pc==0)row->changed&=~1u;
        if(row->changed&2){
            if(row->old_ai&&(!guest_camera_stamp_matches(base,&row->ai,1)||
               !runtime_power_action_context_ai(base,row->old_ai,pawn,world))){row->changed&=~2u;continue;}
            if(!runtime_power_action_context_idle(pawn,pc)||read_ptr(action+0xa0)||
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
       g_runtime_config.role!=ME3_ROLE_GUEST||!runtime_power_action_context_enabled()||
       !runtime_power_action_context_idle(c.pawn,c.pc)||!runtime_power_action_context_scope(base,c,&catalog))return;
    RuntimePowerActionContext candidate={0};candidate.catalog=catalog;
    for(int n=0;n<catalog.count;++n){uintptr_t action=catalog.powers[n].pointer,ai=read_ptr(action+0xa8);
        if(!runtime_power_action_context_layout(base,action)||read_ptr(action+0x98)!=c.pawn||read_ptr(action+0xa0)||
           !runtime_power_action_context_action_idle(action)||!runtime_power_action_context_ai(base,ai,c.pawn,c.world))continue;
        RuntimePowerActionContextRow *row=&candidate.rows[candidate.count];
        if(!guest_camera_stamp_read(base,action,&row->action,1)||(ai&&!guest_camera_stamp_read(base,ai,&row->ai,1)))continue;
        row->old_ai=ai;++candidate.count;
    }
    if(!candidate.count)return;
    RuntimePowerCatalog checked;
    if(!runtime_power_action_context_idle(c.pawn,c.pc)||!runtime_power_action_context_scope(base,c,&checked)||
       !runtime_power_catalog_same(&catalog,&checked))return;
    candidate.active=1;*lease=candidate;
    for(unsigned n=0;n<lease->count;++n){RuntimePowerActionContextRow *row=&lease->rows[n];uintptr_t action=row->action.pointer;
        if(!guest_camera_stamp_matches(base,&row->action,1)||read_ptr(action+0x98)!=c.pawn||
           !runtime_power_action_context_idle(c.pawn,c.pc)||!runtime_power_action_context_action_idle(action)||
           read_ptr(action+0xa0)||read_ptr(action+0xa8)!=row->old_ai||
           (row->old_ai&&(!guest_camera_stamp_matches(base,&row->ai,1)||
             !runtime_power_action_context_ai(base,row->old_ai,c.pawn,c.world)))||
           !runtime_power_action_context_scope(base,c,&checked)||!runtime_power_catalog_same(&catalog,&checked)) {lease->retiring=1;break;}
        if(row->old_ai){
            row->changed|=2;
            if(!runtime_power_action_context_pointer(action+0xa8,row->old_ai,0)){row->changed&=~2u;lease->retiring=1;break;}
        }
        if(!runtime_power_action_context_idle(c.pawn,c.pc)||read_ptr(action+0x98)!=c.pawn||read_ptr(action+0xa8)||
           !guest_camera_stamp_matches(base,&row->action,1)||!runtime_power_action_context_action_idle(action)||
           !runtime_power_action_context_scope(base,c,&checked)||!runtime_power_catalog_same(&catalog,&checked)) {lease->retiring=1;break;}
        row->changed|=1;
        if(!runtime_power_action_context_pointer(action+0xa0,0,c.pc)){row->changed&=~1u;lease->retiring=1;break;}
        if(read_ptr(action+0xa0)!=c.pc||read_ptr(action+0xa8)||read_ptr(action+0x98)!=c.pawn||
           !runtime_power_action_context_idle(c.pawn,c.pc)||!runtime_power_action_context_scope(base,c,&checked)||
           !runtime_power_catalog_same(&catalog,&checked)){lease->retiring=1;break;}
    }
    runtime_power_action_context_log(lease->retiring?"partial-pending-retirement":"idle-controller-context-bound",lease->count);
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
    return 1;
}
#endif
