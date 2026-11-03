#ifndef ME3_RUNTIME_HEAVY_PLOT_GUARD_H
#define ME3_RUNTIME_HEAVY_PLOT_GUARD_H
/* The stock Soldier heavy action enables a shared campaign omni-weapon plot.
 * Keep its animation/damage path, but consume that one write for an Ashley
 * instance authorized by our human melee lease. Other SetBool calls forward.
 * The stock exec still decodes every argument and advances its own VM frame. */
typedef void (*RuntimeHeavyPlotExec)(void*,void*,void*);
typedef void (*RuntimeHeavyPlotNative)(void*,int32_t,uint32_t,uint32_t,uint32_t);
typedef struct RuntimeHeavyPlotOwner {
    SessionHandle pawn,info,world;
    GuestCameraStamp klass,cdo,table_class,table;
} RuntimeHeavyPlotOwner;
typedef struct RuntimeHeavyPlotFrame {
    RuntimeHeavyPlotOwner owner;
    GuestCameraStamp action,table;
    uintptr_t frame;
    int valid,consumed;
} RuntimeHeavyPlotFrame;
typedef struct RuntimeHeavyPlotGuard {
    uintptr_t base;
    RuntimeHeavyPlotExec original_exec;
    RuntimeHeavyPlotNative original_native;
    GuestPlayerContextMethod set_bool,heavy_start,get_globals;
    RuntimeHeavyPlotOwner owners[16];
    unsigned char exec_entry[16],native_entry[16];
    int installed;
    unsigned events;
} RuntimeHeavyPlotGuard;
static RuntimeHeavyPlotGuard runtime_heavy_plot_guard;
static RuntimeHeavyPlotFrame *runtime_heavy_plot_frame;
static int runtime_heavy_plot_ready(uintptr_t base) {
    RuntimeHeavyPlotGuard *g=&runtime_heavy_plot_guard;unsigned char entry[16];
    return g->installed&&g->base==base&&
        read_at(base+0xb70da0,entry,16)&&!memcmp(entry,g->exec_entry,16)&&
        read_at(base+0xab52b0,entry,16)&&!memcmp(entry,g->native_entry,16)&&
        read_ptr(base+0x1345f00+0x2b8)==base+0xab52b0;
}
static int runtime_heavy_plot_owner_live(uintptr_t base,const RuntimeHeavyPlotOwner *o) {
    int32_t plot=-1;
    return session_handle_matches(o->pawn)&&session_handle_matches(o->info)&&session_handle_matches(o->world)&&
        guest_camera_actor_live(base,o->pawn.pointer)&&is_kind_of(base,o->pawn.pointer,"SFXPawn_Ashley")&&
        read_ptr(o->pawn.pointer+0x198)==o->info.pointer&&guest_camera_stamp_matches(base,&o->klass,1)&&
        guest_camera_stamp_matches(base,&o->cdo,1)&&read_ptr(o->cdo.pointer+0x50)==o->klass.pointer&&
        read_ptr(o->klass.pointer+0x13c)==o->cdo.pointer&&
        !strcmp(o->klass.full_name,"SFXGameContent.SFXCustomAction_SoldierPunch")&&
        is_kind_of(base,o->cdo.pointer,"SFXCustomAction_PlayerHeavyMeleeBase")&&
        read_at(o->cdo.pointer+0x1a4,&plot,4)&&plot==20881&&
        guest_camera_stamp_matches(base,&o->table_class,1)&&guest_camera_stamp_matches(base,&o->table,1)&&
        !strcmp(o->table_class.full_name,"SFXGame.BioGlobalVariableTable")&&
        read_ptr(base+0x1cbbc70)==o->table_class.pointer&&read_ptr(o->table_class.pointer+0x13c)==o->table.pointer&&
        read_ptr(o->table.pointer+0x50)==o->table_class.pointer&&read_ptr(o->table.pointer)==base+0x1345f00;
}
/* Weak authorization survives ordinary retirement: an already queued native
 * Start can finish after possession/scene handoff. It still cannot enable the
 * guest's campaign cosmetic flag. No UObject is kept alive by this table. */
static int runtime_heavy_plot_authorize(uintptr_t base,const RuntimeMovementScope *scope,
        GuestCameraStamp klass,GuestCameraStamp cdo) {
    if(GetCurrentThreadId()!=session_game_thread||!runtime_heavy_plot_ready(base))return 0;
    RuntimeHeavyPlotOwner candidate={0};candidate.pawn=scope->pawn;candidate.info=scope->info;candidate.world=scope->world;
    candidate.klass=klass;candidate.cdo=cdo;
    uintptr_t table_class=read_ptr(base+0x1cbbc70),table=read_ptr(table_class+0x13c);
    /* Exact existing-object path of stock GetGlobalVariables. Never invoke
     * its lazy class/CDO construction or publish against an unknown table. */
    if(!narrative_probe_object(base,table_class,"Class")||!guest_camera_stamp_read(base,table_class,&candidate.table_class,1)||
       !narrative_probe_object(base,table,"BioGlobalVariableTable")||!guest_camera_stamp_read(base,table,&candidate.table,1))return 0;
    if(!runtime_heavy_plot_owner_live(base,&candidate)||
       !runtime_power_property(base,cdo.pointer,"SFXCustomAction_PlayerHeavyMeleeBase","OmniWeaponPlotID","IntProperty",0x1a4,4))return 0;
    RuntimeHeavyPlotGuard *g=&runtime_heavy_plot_guard;int free_slot=-1;
    for(unsigned i=0;i<16;++i){RuntimeHeavyPlotOwner *o=g->owners+i;
        if(o->pawn.pointer==candidate.pawn.pointer&&runtime_movement_same_handle(o->pawn,candidate.pawn)&&
           o->klass.pointer==klass.pointer&&runtime_heavy_plot_owner_live(base,o))return 1;
        if(free_slot<0&&(!o->pawn.pointer||!runtime_heavy_plot_owner_live(base,o)))free_slot=(int)i;
    }
    if(free_slot<0)return 0;g->owners[free_slot]=candidate;return 1;
}
static int runtime_heavy_plot_method_live(uintptr_t base) {
    RuntimeHeavyPlotGuard *g=&runtime_heavy_plot_guard;GameArray script={0},native_script={0};
    return guest_player_context_method_matches(base,&g->set_bool,0)&&guest_player_context_method_matches(base,&g->heavy_start,0)&&
        guest_player_context_method_matches(base,&g->get_globals,0)&&read_ptr(g->get_globals.function.pointer+0xd8)==base+0xb4fae0&&
        read_ptr(g->set_bool.function.pointer+0xd8)==base+0xb70da0&&
        read_at(g->set_bool.function.pointer+0x7c,&native_script,sizeof native_script)&&native_script.data&&native_script.count==48&&native_script.max>=48&&native_script.max<=4096&&
        read_ptr(g->heavy_start.function.pointer+0xd8)==base+0xf52b0&&
        read_at(g->heavy_start.function.pointer+0x7c,&script,sizeof script)&&script.data&&
        script.count==1227&&script.max>=script.count&&script.max<=4096;
}
static int runtime_heavy_plot_source_live(uintptr_t base,const RuntimeHeavyPlotFrame *f,int decoded) {
    RuntimeHeavyPlotGuard *g=&runtime_heavy_plot_guard;GameArray script={0};int32_t plot=-1;
    uintptr_t code=read_ptr(f->frame+0x28);
    return f->valid&&runtime_heavy_plot_owner_live(base,&f->owner)&&
        guest_camera_stamp_matches(base,&f->action,1)&&guest_camera_stamp_matches(base,&f->table,1)&&
        narrative_probe_object(base,f->table.pointer,"BioGlobalVariableTable")&&
        read_ptr(f->table.pointer)==base+0x1345f00&&f->table.pointer==f->owner.table.pointer&&read_ptr(f->action.pointer+0x98)==f->owner.pawn.pointer&&
        read_ptr(f->action.pointer+0x50)==f->owner.klass.pointer&&
        read_at(f->action.pointer+0x1a4,&plot,4)&&plot==20881&&runtime_heavy_plot_method_live(base)&&
        read_ptr(f->frame+0x18)==g->heavy_start.function.pointer&&read_ptr(f->frame+0x20)==f->action.pointer&&
        read_at(g->heavy_start.function.pointer+0x7c,&script,sizeof script)&&
        code==script.data+(uintptr_t)(decoded?1224:1211);
}
static RuntimeHeavyPlotFrame runtime_heavy_plot_capture(void *self,void *frame) {
    RuntimeHeavyPlotFrame f={0};RuntimeHeavyPlotGuard *g=&runtime_heavy_plot_guard;uintptr_t base=g->base;
    if(!g->installed||read_ptr((uintptr_t)frame+0x18)!=g->heavy_start.function.pointer||
       !runtime_heavy_plot_ready(base)||!runtime_heavy_plot_method_live(base)||
       !narrative_probe_object(base,(uintptr_t)self,"BioGlobalVariableTable")||read_ptr((uintptr_t)self)!=base+0x1345f00)return f;
    uintptr_t action=read_ptr((uintptr_t)frame+0x20),pawn=read_ptr(action+0x98),klass=read_ptr(action+0x50);
    if(!narrative_probe_object(base,action,"SFXCustomAction_PlayerHeavyMeleeBase"))return f;
    for(unsigned i=0;i<16;++i)if(g->owners[i].pawn.pointer==pawn&&g->owners[i].klass.pointer==klass&&g->owners[i].table.pointer==(uintptr_t)self&&
            runtime_heavy_plot_owner_live(base,g->owners+i)){
        f.owner=g->owners[i];f.frame=(uintptr_t)frame;
        if(!guest_camera_stamp_read(base,action,&f.action,1)||!guest_camera_stamp_read(base,(uintptr_t)self,&f.table,1))return (RuntimeHeavyPlotFrame){0};
        f.valid=1;if(!runtime_heavy_plot_source_live(base,&f,0))f.valid=0;return f;
    }
    return f;
}
static void runtime_heavy_plot_forward_exec(void *self,void *frame,void *result,RuntimeHeavyPlotExec original,
        RuntimeHeavyPlotFrame (*capture)(void*,void*)) {
    DWORD error=GetLastError();RuntimeHeavyPlotFrame local=capture(self,frame),*previous=runtime_heavy_plot_frame;
    runtime_heavy_plot_frame=&local;SetLastError(error);original(self,frame,result);error=GetLastError();
    runtime_heavy_plot_frame=previous;SetLastError(error);
}
static void runtime_heavy_plot_exec(void *self,void *frame,void *result) {
    RuntimeHeavyPlotGuard *g=&runtime_heavy_plot_guard;DWORD error=GetLastError();
    if(GetCurrentThreadId()!=session_game_thread||(!runtime_heavy_plot_frame&&
       read_ptr((uintptr_t)frame+0x18)!=g->heavy_start.function.pointer)){
        SetLastError(error);g->original_exec(self,frame,result);return;}
    SetLastError(error);
    runtime_heavy_plot_forward_exec(self,frame,result,g->original_exec,runtime_heavy_plot_capture);
}
static int runtime_heavy_plot_decision(int owned,int consumed,int same_source,int wrapper_caller,
        int32_t index,uint32_t value,uint32_t evaluate,uint32_t skip) {
    return owned&&!consumed&&same_source&&wrapper_caller&&index==20881&&value==1&&evaluate<=1&&skip<=1;
}
static void runtime_heavy_plot_forward_native(void *self,int32_t index,uint32_t value,uint32_t evaluate,uint32_t skip,
        int consume,RuntimeHeavyPlotNative original) {
    if(!consume)original(self,index,value,evaluate,skip);
}
static void runtime_heavy_plot_native(void *self,int32_t index,uint32_t value,uint32_t evaluate,uint32_t skip) {
    RuntimeHeavyPlotGuard *g=&runtime_heavy_plot_guard;RuntimeHeavyPlotFrame *f=NULL;
    uintptr_t caller=(uintptr_t)__builtin_return_address(0);DWORD error=GetLastError();
    if(GetCurrentThreadId()==session_game_thread)f=runtime_heavy_plot_frame;
    int consume=f&&
        runtime_heavy_plot_decision(f->valid,f->consumed,
            f->table.pointer==(uintptr_t)self&&runtime_heavy_plot_source_live(g->base,f,1),caller==g->base+0xb70ec8,
            index,value,evaluate,skip);
    if(consume){f->consumed=1;if(g->events++<32)
        line("{\"runtime_guest_heavy_melee\":\"guest-cosmetic-plot-write-consumed\",\"plot_written\":false,\"native_exec_forwarded\":true,\"damage_verified\":false}\r\n");}
    SetLastError(error);runtime_heavy_plot_forward_native(self,index,value,evaluate,skip,consume,g->original_native);
}
static int runtime_heavy_plot_pins(uintptr_t base) {
    static const unsigned char body[]={0x48,0x8b,0xc4,0x44,0x89,0x40,0x18,0x55,0x41,0x54,0x41,0x55,0x41,0x56};
    static const unsigned char exec[]={0x48,0x89,0x5c,0x24,0x18,0x55,0x56,0x57,0x48,0x83,0xec,0x40,0x48,0x8b,0x42,0x28};
    static const unsigned char call[]={0x48,0x8b,0x06,0x48,0x8b,0xce,0x44,0x8b,0x4c,0x24,0x68,0x44,0x8b,0x44,0x24,0x78,
        0x8b,0x54,0x24,0x30,0x89,0x7c,0x24,0x20,0xff,0x90,0xb8,0x02,0x00,0x00};
    static const unsigned char getter[]={0x48,0x83,0xec,0x28,0x48,0x8b,0x0d,0x75,0x98,0x1e,0x01,0x48,0x85,0xc9,0x75,0x1f,
        0x48,0x8d,0x0d,0x31,0xc2,0x5d,0x00,0xe8,0xf4,0xdf,0xfd,0xff,0x48,0x89,0x05,0x5d,0x98,0x1e,0x01,
        0xe8,0x28,0xf6,0xfd,0xff,0x48,0x8b,0x0d,0x51,0x98,0x1e,0x01,0x33,0xd2,0x48,0x83,0xc4,0x28,0xe9,0xb6,0xaa,0x5f,0xff};
    static const unsigned char cdo_branch[]={0x40,0x57,0x48,0x83,0xec,0x40,0x48,0x83,0xb9,0x3c,0x01,0x00,0x00,0x00,
        0x48,0x8b,0xf9,0x0f,0x85,0x1c,0x01,0x00,0x00};
    static const unsigned char cdo_return[]={0x48,0x8b,0x81,0x3c,0x01,0x00,0x00,0x48,0x83,0xc4,0x40,0x5f,0xc3};
    unsigned char bytes[64];
    return read_at(base+0xab52b0,bytes,sizeof body)&&!memcmp(bytes,body,sizeof body)&&
        read_at(base+0xb70da0,bytes,sizeof exec)&&!memcmp(bytes,exec,sizeof exec)&&
        read_at(base+0xb70eaa,bytes,sizeof call)&&!memcmp(bytes,call,sizeof call)&&
        read_at(base+0xad23f0,bytes,sizeof getter)&&!memcmp(bytes,getter,sizeof getter)&&
        read_at(base+0xccee0,bytes,sizeof cdo_branch)&&!memcmp(bytes,cdo_branch,sizeof cdo_branch)&&
        read_at(base+0xcd013,bytes,sizeof cdo_return)&&!memcmp(bytes,cdo_return,sizeof cdo_return)&&
        read_ptr(base+0x1345f00+0x2b8)==base+0xab52b0;
}
static int runtime_heavy_plot_prepare(uintptr_t base,uintptr_t info,uintptr_t heavy_class) {
    RuntimeHeavyPlotGuard *g=&runtime_heavy_plot_guard;
    if(g->installed)return runtime_heavy_plot_ready(base);
    if(GetCurrentThreadId()!=session_game_thread||!supported_image(base)||!runtime_heavy_plot_pins(base))return 0;
    uintptr_t table_class=read_ptr(base+0x1cbbc70);
    if(!narrative_probe_object(base,table_class,"Class")||!guest_camera_actor_live(base,info)||
       !is_kind_of(base,info,"BioWorldInfo")||!narrative_probe_object(base,heavy_class,"Class"))return 0;
    uintptr_t set=guest_player_context_find_method(base,table_class,"BioGlobalVariableTable","SetBool");
    uintptr_t heavy=guest_player_context_find_method(base,heavy_class,"SFXCustomAction_PlayerHeavyMeleeBase","StartCustomAction");
    uintptr_t globals=guest_player_context_find_method(base,read_ptr(info+0x50),"BioWorldInfo","GetGlobalVariables");
    if(!guest_player_context_method_stamp(base,set,&g->set_bool)||!guest_player_context_method_stamp(base,heavy,&g->heavy_start)||
       !guest_player_context_method_stamp(base,globals,&g->get_globals)||
       strcmp(g->set_bool.function.full_name,"SFXGame.BioGlobalVariableTable.SetBool")||
       strcmp(g->heavy_start.function.full_name,"SFXGame.SFXCustomAction_PlayerHeavyMeleeBase.StartCustomAction")||
       strcmp(g->get_globals.function.full_name,"SFXGame.BioWorldInfo.GetGlobalVariables")||
       !guest_player_context_function_abi(base,globals,"BioWorldInfo","GetGlobalVariables",0x20400,8,0,0,0xb4fae0,1)||
       !guest_player_context_function_abi(base,set,"BioGlobalVariableTable","SetBool",0x24400,16,65535,0,0xb70da0,4)||
       !guest_player_context_function_abi(base,heavy,"SFXCustomAction_PlayerHeavyMeleeBase","StartCustomAction",0x20002,0,65535,0,0xf52b0,0))return 0;
    uintptr_t field=read_ptr(set+0x70);
    static const char *names[]={"nIndex","bValue","bEvaluateAchievement","bSkipAllAdditionalProcessing"};
    for(unsigned i=0;i<4;++i){char path[160];snprintf(path,sizeof path,"SFXGame.BioGlobalVariableTable.SetBool.%s",names[i]);
        if(read_ptr(field+0x40)!=set||!runtime_delayed_fire_property(base,field,i?"BoolProperty":"IntProperty",path,(int)i*4,4,i<2?0x80:0x90))return 0;
        field=read_ptr(field+0x68);
    }
    if(field)return 0;
    g->base=base;if(!runtime_heavy_plot_method_live(base))return 0;
    MH_STATUS status=MH_Initialize();if(status!=MH_OK&&status!=MH_ERROR_ALREADY_INITIALIZED)return 0;
    LPVOID exec=NULL,body=NULL;int exec_created=0,body_created=0;
    if(MH_CreateHook((void*)(base+0xb70da0),(void*)runtime_heavy_plot_exec,&exec)!=MH_OK)goto failed;exec_created=1;
    if(MH_CreateHook((void*)(base+0xab52b0),(void*)runtime_heavy_plot_native,&body)!=MH_OK)goto failed;body_created=1;
    g->original_exec=(RuntimeHeavyPlotExec)exec;g->original_native=(RuntimeHeavyPlotNative)body;
    if(!exec||!body||MH_EnableHook((void*)(base+0xb70da0))!=MH_OK||MH_EnableHook((void*)(base+0xab52b0))!=MH_OK||
       !read_at(base+0xb70da0,g->exec_entry,16)||!read_at(base+0xab52b0,g->native_entry,16))goto failed;
    g->installed=1;
    line("{\"runtime_guest_heavy_melee\":\"native-cosmetic-plot-guard-installed\",\"plot_written\":false,\"gameplay_verified\":false}\r\n");
    return runtime_heavy_plot_ready(base);
failed:
    if(body_created){MH_DisableHook((void*)(base+0xab52b0));MH_RemoveHook((void*)(base+0xab52b0));}
    if(exec_created){MH_DisableHook((void*)(base+0xb70da0));MH_RemoveHook((void*)(base+0xb70da0));}
    return 0;
}
#ifdef ME3_RUNTIME_FIXTURE
static void *runtime_heavy_fixture_self,*runtime_heavy_fixture_outer,*runtime_heavy_fixture_result;
static unsigned runtime_heavy_fixture_calls;
static int runtime_heavy_fixture_bad;
static RuntimeHeavyPlotFrame runtime_heavy_fixture_capture(void *self,void *frame) {
    RuntimeHeavyPlotFrame f={0};f.valid=frame==runtime_heavy_fixture_outer;
    if(self!=runtime_heavy_fixture_self)runtime_heavy_fixture_bad=1;
    SetLastError(13);return f;
}
static void runtime_heavy_fixture_nested(void *self,void *frame,void *result) {
    if(self!=runtime_heavy_fixture_self||frame==runtime_heavy_fixture_outer||result!=runtime_heavy_fixture_result||
       !runtime_heavy_plot_frame||runtime_heavy_plot_frame->valid||GetLastError()!=41)runtime_heavy_fixture_bad=1;
    ++runtime_heavy_fixture_calls;SetLastError(59);
}
static void runtime_heavy_fixture_outer_exec(void *self,void *frame,void *result) {
    if(self!=runtime_heavy_fixture_self||frame!=runtime_heavy_fixture_outer||result!=runtime_heavy_fixture_result||
       !runtime_heavy_plot_frame||!runtime_heavy_plot_frame->valid||GetLastError()!=41)runtime_heavy_fixture_bad=1;
    ++runtime_heavy_fixture_calls;RuntimeHeavyPlotFrame *outer=runtime_heavy_plot_frame;int inner;
    runtime_heavy_plot_forward_exec(self,&inner,result,runtime_heavy_fixture_nested,runtime_heavy_fixture_capture);
    if(runtime_heavy_plot_frame!=outer||!outer->valid||GetLastError()!=59)runtime_heavy_fixture_bad=1;
    SetLastError(67);
}
static void runtime_heavy_fixture_native(void *self,int32_t index,uint32_t value,uint32_t evaluate,uint32_t skip) {
    if(self!=runtime_heavy_fixture_self||index!=20881||value!=1||evaluate!=1||skip||GetLastError()!=41)runtime_heavy_fixture_bad=1;
    ++runtime_heavy_fixture_calls;SetLastError(67);
}
static int runtime_heavy_plot_guard_selftest(void) {
    DWORD error=GetLastError();RuntimeHeavyPlotFrame *saved=runtime_heavy_plot_frame,previous={0};int self,frame,result;
    runtime_heavy_fixture_self=&self;runtime_heavy_fixture_outer=&frame;runtime_heavy_fixture_result=&result;
    runtime_heavy_fixture_bad=0;runtime_heavy_fixture_calls=0;runtime_heavy_plot_frame=&previous;
    SetLastError(41);runtime_heavy_plot_forward_exec(&self,&frame,&result,runtime_heavy_fixture_outer_exec,runtime_heavy_fixture_capture);
    int ok=runtime_heavy_fixture_calls==2&&!runtime_heavy_fixture_bad&&runtime_heavy_plot_frame==&previous&&GetLastError()==67;
    runtime_heavy_plot_frame=saved;runtime_heavy_fixture_calls=0;
    SetLastError(41);runtime_heavy_plot_forward_native(&self,20881,1,1,0,1,runtime_heavy_fixture_native);
    ok=ok&&!runtime_heavy_fixture_calls&&GetLastError()==41;
    runtime_heavy_plot_forward_native(&self,20881,1,1,0,0,runtime_heavy_fixture_native);
    ok=ok&&runtime_heavy_fixture_calls==1&&GetLastError()==67&&!runtime_heavy_fixture_bad;
    ok=ok&&runtime_heavy_plot_decision(1,0,1,1,20881,1,1,0)&&
        !runtime_heavy_plot_decision(0,0,1,1,20881,1,1,0)&&!runtime_heavy_plot_decision(1,1,1,1,20881,1,1,0)&&
        !runtime_heavy_plot_decision(1,0,0,1,20881,1,1,0)&&!runtime_heavy_plot_decision(1,0,1,0,20881,1,1,0)&&
        !runtime_heavy_plot_decision(1,0,1,1,20880,1,1,0)&&!runtime_heavy_plot_decision(1,0,1,1,20881,0,1,0)&&
        !runtime_heavy_plot_decision(1,0,1,1,20881,1,2,0)&&!runtime_heavy_plot_decision(1,0,1,1,20881,1,1,2);
    SetLastError(error);return ok;
}
#endif
#endif
