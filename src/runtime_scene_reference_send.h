#ifndef ME3_RUNTIME_SCENE_REFERENCE_SEND_H
#define ME3_RUNTIME_SCENE_REFERENCE_SEND_H
/* Output-only host metadata at the original property send. No extra codec,
 * package-map lookup, reference assignment or guest sequence is invoked. */
typedef struct RuntimeSceneReferenceSendMetadata {
    uintptr_t input,outer[16];GuestCameraStamp chain[16],package;
    GameArray package_records;
    GameArray package_netobjects,package_generations;
    uint32_t package_flags;
    int input_read,netindex_read,package_complete,records_read,package_flags_read,package_netobjects_read,package_generations_read,stable;
    int generation_last_read;int32_t generation_last,netindex;unsigned depth;
} RuntimeSceneReferenceSendMetadata;
typedef struct RuntimeSceneReferenceSendFrame {
    RuntimeSceneScope scope;GuestCameraStamp actor,action,channel,map,property;
    RuntimeSceneReferenceSendMetadata before;
    uintptr_t archive,data,writer_table,writer_bits,writer_index;
    unsigned id;int channel_index,active,writer_read;
} RuntimeSceneReferenceSendFrame;
typedef struct RuntimeSceneReferenceSend {
    uintptr_t base;unsigned calls,depth;int installed;
} RuntimeSceneReferenceSend;
static RuntimeSceneReferenceSend runtime_scene_reference_send;
static int runtime_scene_reference_send_opted(void) {
    static int loaded,value;
    if(!loaded){wchar_t text[8]={0};loaded=1;
        value=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_SCENE_REFERENCE_SEND_TRACE",text,8)==1&&text[0]==L'1';}
    return value;
}
static void runtime_scene_reference_send_metadata(uintptr_t base,uintptr_t map,uintptr_t data,
        RuntimeSceneReferenceSendMetadata *m) {
    memset(m,0,sizeof *m);
    m->records_read=read_at(map+0x68,&m->package_records,sizeof m->package_records);
    m->input_read=read_at(data,&m->input,8);
    if(!m->input_read)return;
    if(m->input){
        m->netindex_read=read_at(m->input+0x3c,&m->netindex,4);
        uintptr_t cursor=m->input;
        while(cursor&&m->depth<16){
            for(unsigned i=0;i<m->depth;++i)if(m->chain[i].pointer==cursor)return;
            unsigned i=m->depth;
            if(!guest_camera_stamp_read(base,cursor,&m->chain[i],0)||!read_at(cursor+0x40,&m->outer[i],8))return;
            ++m->depth;cursor=m->outer[i];
        }
        if(cursor||!m->depth||!narrative_probe_object(base,m->chain[m->depth-1].pointer,"Package")||
           !guest_camera_stamp_read(base,m->chain[m->depth-1].pointer,&m->package,1))return;
        m->package_complete=1;
        m->package_flags_read=read_at(m->package.pointer+0x104,&m->package_flags,4);
        m->package_netobjects_read=read_at(m->package.pointer+0x80,&m->package_netobjects,sizeof m->package_netobjects);
        m->package_generations_read=read_at(m->package.pointer+0x94,&m->package_generations,sizeof m->package_generations);
        /* Retained CEE70 copies int32 generation values and uses the last.
         * Read at most one element; unknown/malformed headers stay raw. */
        if(m->package_generations_read&&m->package_generations.count>0&&
           m->package_generations.count<=m->package_generations.max&&m->package_generations.max<=4096&&
           m->package_generations.data&&!(m->package_generations.data&3)){
            SIZE_T offset=(SIZE_T)(m->package_generations.count-1)*4;
            if(m->package_generations.data<=UINTPTR_MAX-offset)
                m->generation_last_read=read_at(m->package_generations.data+offset,&m->generation_last,4);
        }
        for(unsigned i=0;i<m->depth;++i){uintptr_t outer=0;
            if(!guest_camera_stamp_matches(base,&m->chain[i],0)||
               !read_at(m->chain[i].pointer+0x40,&outer,8)||outer!=m->outer[i])return;}
    }
    uintptr_t after=0;GameArray records={0};
    m->stable=(!m->input||(m->netindex_read&&m->package_complete&&m->package_flags_read&&
        m->package_netobjects_read&&m->package_generations_read))&&read_at(data,&after,8)&&after==m->input&&m->records_read&&
        read_at(map+0x68,&records,sizeof records)&&!memcmp(&records,&m->package_records,sizeof records);
}
static int runtime_scene_reference_send_metadata_equal(const RuntimeSceneReferenceSendMetadata *a,
        const RuntimeSceneReferenceSendMetadata *b) {
    if(!a->stable||!b->stable||a->input_read!=b->input_read||a->input!=b->input||
       a->netindex_read!=b->netindex_read||(a->netindex_read&&a->netindex!=b->netindex)||
       a->package_complete!=b->package_complete||a->depth!=b->depth||
       memcmp(&a->package_records,&b->package_records,sizeof a->package_records)||
       a->package_flags_read!=b->package_flags_read||(a->package_flags_read&&a->package_flags!=b->package_flags)||
       a->package_netobjects_read!=b->package_netobjects_read||
       (a->package_netobjects_read&&memcmp(&a->package_netobjects,&b->package_netobjects,sizeof a->package_netobjects))||
       a->package_generations_read!=b->package_generations_read||
       (a->package_generations_read&&memcmp(&a->package_generations,&b->package_generations,sizeof a->package_generations))||
       a->generation_last_read!=b->generation_last_read||(a->generation_last_read&&a->generation_last!=b->generation_last))return 0;
    for(unsigned i=0;i<a->depth;++i)if(!guest_camera_stamp_core_equal(&a->chain[i],&b->chain[i])||a->outer[i]!=b->outer[i])return 0;
    return !a->package_complete||guest_camera_stamp_core_equal(&a->package,&b->package);
}
static int runtime_scene_reference_send_lease(uintptr_t base,uintptr_t actor,uintptr_t world,GuestCameraStamp *action) {
    RuntimeSceneInterp *s=&runtime_scene_interp;int found=0;
    for(unsigned i=0;i<64;++i){RuntimeSceneInterpLease *l=&s->leases[i];
        if(!l->used||l->world.pointer!=world||read_ptr(l->action.pointer+0x15c)!=actor)continue;
        GuestCameraStamp current={0};
        if(!runtime_scene_interp_lease_live(l)||!runtime_scene_interp_action(base,l->action.pointer,world,&current,NULL,NULL)||
           !guest_camera_stamp_core_equal(&l->action,&current))return 0;
        if(++found>1)return 0;*action=current;
    }
    return found==1;
}
static int runtime_scene_reference_send_capture(RuntimeSceneReferenceSendFrame *f,uintptr_t property,
        uintptr_t archive,uintptr_t map,uintptr_t data,uintptr_t caller) {
    RuntimeSceneReferenceSend *s=&runtime_scene_reference_send;uintptr_t base=s->base;uint32_t loading=1;
    if(!s->installed||s->calls>=64||s->depth!=1||!runtime_scene_reference_send_opted()||
       g_runtime_config.role!=ME3_ROLE_HOST||!runtime_control_enabled||!runtime_scope_ready||
       GetCurrentThreadId()!=session_game_thread||caller!=base+0x48d433||data<0x270||
       !runtime_scene_reference_property(base,property)||read_ptr(map)!=base+0x11a0d20||
       !read_at(archive+0x14,&loading,4)||loading||
       !runtime_scene_interp_capability_live(GetTickCount())||
       !runtime_scene_host_peer(base,session_context(session_engine),&f->scope)||
       f->scope.connection.pointer!=runtime_scene_interp.capability.connection.pointer||
       read_ptr(map+0x108)!=f->scope.connection.pointer)return 0;
    uintptr_t actor=data-0x270;NetworkActorIdentityEntry channel={0};
    if(!guest_camera_actor_live(base,actor)||!is_kind_of(base,actor,"MatineeActor")||
       read_ptr(actor+0x198)!=f->scope.world.pointer||
       network_actor_scan_driver(base,actor,f->scope.driver.pointer,&channel)<0||
       channel.connection!=f->scope.connection.pointer||channel.index<=0||
       !runtime_scene_reference_send_lease(base,actor,f->scope.world.pointer,&f->action)||
       !guest_camera_stamp_read(base,actor,&f->actor,1)||!guest_camera_stamp_read(base,channel.channel,&f->channel,0)||
       !guest_camera_stamp_read(base,map,&f->map,0)||!guest_camera_stamp_read(base,property,&f->property,0))return 0;
    f->archive=archive;f->data=data;f->channel_index=channel.index;
    f->writer_read=read_at(archive,&f->writer_table,8)&&f->writer_table&&
        read_at(f->writer_table+0x10,&f->writer_bits,8)&&read_at(f->writer_table+0x18,&f->writer_index,8);
    runtime_scene_reference_send_metadata(base,map,data,&f->before);
    /* The lease's reciprocal ReplicatedActor still identifies a null input.
     * A different nonnull action is unrelated or stale, never ours to infer. */
    if(f->before.input_read&&f->before.input&&f->before.input!=f->action.pointer)return 0;
    f->id=++s->calls;f->active=1;return 1;
}
static int runtime_scene_reference_send_same(const RuntimeSceneReferenceSendFrame *f) {
    uintptr_t base=runtime_scene_reference_send.base;NetworkActorIdentityEntry channel={0};GuestCameraStamp action={0};
    uintptr_t writer=0,bits=0,index=0;
    return runtime_scene_scope_live(base,&f->scope)&&
        guest_camera_stamp_matches(base,&f->actor,1)&&guest_camera_stamp_matches(base,&f->channel,0)&&
        guest_camera_stamp_matches(base,&f->map,0)&&guest_camera_stamp_matches(base,&f->property,0)&&
        read_ptr(f->map.pointer+0x108)==f->scope.connection.pointer&&read_ptr(f->actor.pointer+0x198)==f->scope.world.pointer&&
        network_actor_channel_valid(base,f->actor.pointer,f->channel.pointer,f->scope.driver.pointer,&channel)&&
        channel.connection==f->scope.connection.pointer&&channel.index==f->channel_index&&
        runtime_scene_reference_send_lease(base,f->actor.pointer,f->scope.world.pointer,&action)&&
        guest_camera_stamp_core_equal(&f->action,&action)&&f->writer_read&&
        read_at(f->archive,&writer,8)&&writer==f->writer_table&&
        read_at(writer+0x10,&bits,8)&&bits==f->writer_bits&&read_at(writer+0x18,&index,8)&&index==f->writer_index;
}
static void runtime_scene_reference_send_emit(const RuntimeSceneReferenceSendFrame *f,int result) {
    RuntimeSceneReferenceSendMetadata after={0};uint32_t error=0;
    int same=runtime_scene_reference_send_same(f),error_read=read_at(f->archive+0x38,&error,4);
    if(same)runtime_scene_reference_send_metadata(runtime_scene_reference_send.base,f->map.pointer,f->data,&after);
    const RuntimeSceneReferenceSendMetadata *m=&f->before;
    char row[4600],action[1200],package[1200];identity_quote(f->action.full_name,action,sizeof action);
    identity_quote(m->package_complete?m->package.full_name:"",package,sizeof package);
    snprintf(row,sizeof row,
        "{\"runtime_scene_reference_send\":\"native-property-metadata\",\"id\":%u,\"actor_index\":%d,\"channel_index\":%d,\"action_path\":\"%s\",\"input_readable\":%s,\"input_null\":%s,\"input\":\"0x%llx\",\"netindex_readable\":%s,\"netindex\":%d,\"outer_chain_complete\":%s,\"outer_depth\":%u,\"package_path\":\"%s\",\"package_name\":[%u,%u],\"package_flags_readable\":%s,\"package_flags_raw\":%u,\"package_netobjects_readable\":%s,\"package_netobjects_count_raw\":%d,\"package_netobjects_capacity_raw\":%d,\"package_generations_readable\":%s,\"package_generations_count_raw\":%d,\"package_generations_capacity_raw\":%d,\"package_generation_last_readable\":%s,\"package_generation_last\":%d,\"map_records_readable\":%s,\"map_records_data\":\"0x%llx\",\"map_records_count_raw\":%d,\"map_records_capacity_raw\":%d,\"same_scope\":%s,\"metadata_stable\":%s,\"native_return\":%d,\"archive_error_readable\":%s,\"archive_error\":%u,\"writer_metadata_readable\":%s,\"writer_table\":\"0x%llx\",\"writer_bits\":\"0x%llx\",\"writer_index\":\"0x%llx\",\"package_lookup_performed\":false,\"wire_index_observed\":false,\"object_written\":false,\"monotonic_ms\":%lu}\r\n",
        f->id,f->actor.index,f->channel_index,action,m->input_read?"true":"false",m->input_read&&!m->input?"true":"false",
        (unsigned long long)m->input,m->netindex_read?"true":"false",m->netindex,m->package_complete?"true":"false",m->depth,
        package,m->package.name[0],m->package.name[1],m->package_flags_read?"true":"false",m->package_flags,
        m->package_netobjects_read?"true":"false",m->package_netobjects.count,m->package_netobjects.max,
        m->package_generations_read?"true":"false",m->package_generations.count,m->package_generations.max,
        m->generation_last_read?"true":"false",m->generation_last,
        m->records_read?"true":"false",(unsigned long long)m->package_records.data,
        m->package_records.count,m->package_records.max,same?"true":"false",
        same&&runtime_scene_reference_send_metadata_equal(m,&after)?"true":"false",result,error_read?"true":"false",error,
        f->writer_read?"true":"false",(unsigned long long)f->writer_table,(unsigned long long)f->writer_bits,(unsigned long long)f->writer_index,GetTickCount());line(row);
}
static int runtime_scene_reference_send_gate(uintptr_t property,uintptr_t caller) {
    RuntimeSceneReferenceSend *s=&runtime_scene_reference_send;int32_t offset=0;
    return s->installed&&s->calls<64&&!s->depth&&g_runtime_config.role==ME3_ROLE_HOST&&
        runtime_control_enabled&&runtime_scope_ready&&GetCurrentThreadId()==session_game_thread&&
        caller==s->base+0x48d433&&read_ptr(property)==s->base+0x1077d88&&
        read_at(property+0x94,&offset,4)&&offset==0x270;
}
static __attribute__((noinline)) int runtime_scene_reference_send_observed(void *property,void *archive,void *map,void *data,
        uintptr_t caller,RuntimeSceneReferenceNative original) {
    DWORD before=GetLastError();RuntimeSceneReferenceSend *s=&runtime_scene_reference_send;
    int thread=GetCurrentThreadId()==session_game_thread;RuntimeSceneReferenceSendFrame f={0};
    if(thread){++s->depth;runtime_scene_reference_send_capture(&f,(uintptr_t)property,(uintptr_t)archive,(uintptr_t)map,(uintptr_t)data,caller);}
    SetLastError(before);int result=original(property,archive,map,data);DWORD after=GetLastError();
    if(thread){if(f.active)runtime_scene_reference_send_emit(&f,result);--s->depth;}
    SetLastError(after);return result;
}
static int runtime_scene_reference_send_forward(void *property,void *archive,void *map,void *data,
        uintptr_t caller,RuntimeSceneReferenceNative original) {
    DWORD error=GetLastError();int observe=runtime_scene_reference_send_gate((uintptr_t)property,caller);
    SetLastError(error);
    return observe?runtime_scene_reference_send_observed(property,archive,map,data,caller,original):original(property,archive,map,data);
}
static void runtime_scene_reference_send_prepare(uintptr_t base) {
    RuntimeSceneReferenceSend *s=&runtime_scene_reference_send;
    if(s->installed||!runtime_scene_reference_send_opted()||g_runtime_config.role!=ME3_ROLE_HOST||
       GetCurrentThreadId()!=session_game_thread||!runtime_control_enabled||!runtime_scope_ready)return;
    if(!runtime_scene_reference_slot_install(base))return;
    s->base=base;s->installed=1;runtime_scene_reference_host_forward=runtime_scene_reference_send_forward;
    line("{\"runtime_scene_reference_send\":\"installed\",\"default_off\":true,\"limit\":64,\"package_lookup_performed\":false,\"object_written\":false}\r\n");
}
#endif
