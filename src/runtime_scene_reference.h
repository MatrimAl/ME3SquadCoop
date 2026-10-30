#ifndef ME3_RUNTIME_SCENE_REFERENCE_H
#define ME3_RUNTIME_SCENE_REFERENCE_H
/* Default-off, bounded output observation of MatineeActor.InterpAction.
 * Reuse the existing ReceivedBunch frame and scalar hook owner. Never decode
 * twice, resolve an index, assign InterpAction or activate a guest sequence. */
typedef int (*RuntimeSceneReferenceNative)(void*,void*,void*,void*);
typedef struct RuntimeSceneReferenceScope {
    struct RuntimeSceneReferenceScope *previous;
    NativeRpcReceiveFrame *receive;
    GuestCameraStamp actor,channel,connection,map,property;
    uintptr_t archive,requested;
    unsigned id,tag,index;
    int channel_index;
    int active,tag_known,index_known;
} RuntimeSceneReferenceScope;
typedef struct RuntimeSceneReference {
    uintptr_t base;
    RuntimeSceneReferenceNative original;
    RuntimeSceneReferenceScope *scope;
    unsigned calls;
    int installed,wanted;
} RuntimeSceneReference;
static RuntimeSceneReference runtime_scene_reference;
static int runtime_scene_reference_opted(void) {
    static int loaded,value;
    if(!loaded){wchar_t text[8]={0};loaded=1;
        value=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_SCENE_REFERENCE_TRACE",text,8)==1&&text[0]==L'1';}
    return value;
}
static int runtime_scene_reference_requested(void) {return runtime_scene_reference.wanted;}
static int runtime_scene_reference_scalar_policy(int active,int same_archive,int tag_read,
        unsigned tag,int count,uintptr_t caller,uintptr_t base,int index,uint32_t maximum) {
    if(!active||!same_archive)return 0;
    if(!index)return count==1&&caller==base+0x602c40;
    return tag_read&&((tag==1&&maximum==0x3ff&&caller==base+0x602c5d)||
        (tag==0&&maximum==UINT32_C(0x80000000)&&caller==base+0x602cc3));
}
static int runtime_scene_reference_current(RuntimeSceneReferenceScope *f,void *archive) {
    RuntimeSceneReference *s=&runtime_scene_reference;
    return s->installed&&GetCurrentThreadId()==runtime_rpc_receive_thread&&f&&f==s->scope&&f->active&&
        f->receive==native_rpc_receive_current(runtime_rpc_receive_thread)&&
        f->archive==(uintptr_t)archive&&f->archive==runtime_rpc_receive_archive&&read_ptr(f->archive)==s->base+0x119dda8&&
        guest_camera_stamp_matches(s->base,&f->channel,0)&&guest_camera_stamp_matches(s->base,&f->actor,0)&&
        read_ptr(f->channel.pointer+0xa0)==f->actor.pointer;
}
static void runtime_scene_reference_bits(void *archive,void *dest,int32_t count,uintptr_t caller) {
    RuntimeSceneReference *s=&runtime_scene_reference;RuntimeSceneReferenceScope *f=s->scope;
    if(!runtime_scene_reference_scalar_policy(runtime_scene_reference_current(f,archive),1,0,0,count,caller,s->base,0,0))return;
    unsigned char tag=255;
    if(!f->tag_known&&read_at((uintptr_t)dest,&tag,1)&&tag<=1){f->tag=tag;f->tag_known=1;}
}
static void runtime_scene_reference_index(void *archive,uint32_t *dest,uint32_t maximum,uintptr_t caller) {
    RuntimeSceneReference *s=&runtime_scene_reference;RuntimeSceneReferenceScope *f=s->scope;
    int current=runtime_scene_reference_current(f,archive);
    if(!runtime_scene_reference_scalar_policy(current,1,current?f->tag_known:0,current?f->tag:0,
        0,caller,s->base,1,maximum))return;
    uint32_t index=0;
    if(!f->index_known&&read_at((uintptr_t)dest,&index,4)){f->index=index;f->index_known=1;}
}
static int runtime_scene_reference_property(uintptr_t base,uintptr_t property) {
    int32_t dimension=0,size=0,offset=0;uint64_t flags=0;GuestCameraStamp field={0},owner={0},requested={0};
    return narrative_probe_object(base,property,"ObjectProperty")&&read_ptr(property)==base+0x1077d88&&
        read_at(property+0x70,&dimension,4)&&dimension==1&&read_at(property+0x74,&size,4)&&size==8&&
        read_at(property+0x94,&offset,4)&&offset==0x270&&read_at(property+0x78,&flags,8)&&flags==UINT64_C(0x22)&&
        guest_camera_stamp_read(base,property,&field,1)&&!strcmp(field.full_name,"Engine.MatineeActor.InterpAction")&&
        guest_camera_stamp_read(base,read_ptr(property+0x40),&owner,1)&&!strcmp(owner.full_name,"Engine.MatineeActor")&&
        narrative_probe_object(base,read_ptr(property+0xd0),"Class")&&
        guest_camera_stamp_read(base,read_ptr(property+0xd0),&requested,1)&&!strcmp(requested.full_name,"Engine.SeqAct_Interp");
}
static int runtime_scene_reference_capture(RuntimeSceneReferenceScope *f,uintptr_t property,
        uintptr_t archive,uintptr_t map,uintptr_t caller) {
    RuntimeSceneReference *s=&runtime_scene_reference;uintptr_t base=s->base;
    NativeRpcReceiveFrame *receive=native_rpc_receive_current(runtime_rpc_receive_thread);
    if(!s->installed||s->calls>=64||g_runtime_config.role!=ME3_ROLE_GUEST||!runtime_control_enabled||
       !runtime_scene_reference_opted()||caller!=base+0x488ed4||!receive||!receive->channel||archive!=runtime_rpc_receive_archive||
       read_ptr(archive)!=base+0x119dda8||read_ptr(map)!=base+0x11a0d20||!runtime_scene_reference_property(base,property))return 0;
    uintptr_t actor=read_ptr(receive->channel+0xa0);NetworkActorScope world={0};NetworkActorIdentityEntry channel={0};
    if(!guest_camera_actor_live(base,actor)||!is_kind_of(base,actor,"MatineeActor")||
       !network_actor_read_scope(base,actor,&world)||world.mode!=3||
       !network_actor_channel_valid(base,actor,receive->channel,world.driver,&channel)||channel.index<=0||
       read_ptr(world.driver+0x80)!=channel.connection||read_ptr(map+0x108)!=channel.connection||
       !guest_camera_stamp_read(base,actor,&f->actor,0)||!guest_camera_stamp_read(base,receive->channel,&f->channel,0)||
       !guest_camera_stamp_read(base,channel.connection,&f->connection,0)||!guest_camera_stamp_read(base,map,&f->map,0)||
       !guest_camera_stamp_read(base,property,&f->property,0))return 0;
    f->receive=receive;f->archive=archive;f->requested=read_ptr(property+0xd0);f->channel_index=channel.index;
    f->id=++s->calls;f->active=1;return 1;
}
static void runtime_scene_reference_emit(const RuntimeSceneReferenceScope *f,void *data,int result) {
    RuntimeSceneReference *s=&runtime_scene_reference;uintptr_t value=0,slot=0;uint32_t error=0;
    int scratch_read=read_at((uintptr_t)data,&value,8),slot_read=read_at(f->actor.pointer+0x270,&slot,8);
    int error_read=read_at(f->archive+0x38,&error,4);
    int same=runtime_scene_reference_current((RuntimeSceneReferenceScope*)f,(void*)f->archive)&&
        guest_camera_stamp_matches(s->base,&f->connection,0)&&guest_camera_stamp_matches(s->base,&f->map,0)&&
        guest_camera_stamp_matches(s->base,&f->property,0)&&read_ptr(f->map.pointer+0x108)==f->connection.pointer;
    char row[1100];snprintf(row,sizeof row,
        "{\"runtime_scene_reference\":\"native-property-output\",\"id\":%u,\"actor_index\":%d,\"channel_index\":%d,\"tag_known\":%s,\"tag\":%u,\"index_known\":%s,\"index\":%u,\"same_scope\":%s,\"scratch_read\":%s,\"scratch_object\":\"0x%llx\",\"current_slot_read\":%s,\"current_slot_object\":\"0x%llx\",\"slot_is_final_commit\":false,\"archive_error_read\":%s,\"archive_error\":%u,\"native_return\":%d,\"output_only\":true,\"archive_replayed\":false,\"object_written\":false,\"monotonic_ms\":%lu}\r\n",
        f->id,f->actor.index,f->channel_index,f->tag_known?"true":"false",f->tag,f->index_known?"true":"false",f->index,
        same?"true":"false",scratch_read?"true":"false",(unsigned long long)value,slot_read?"true":"false",(unsigned long long)slot,
        error_read?"true":"false",error,result,GetTickCount());line(row);
}
static int runtime_scene_reference_forward(void *property,void *archive,void *map,void *data,uintptr_t caller) {
    RuntimeSceneReference *s=&runtime_scene_reference;DWORD before=GetLastError();
    int thread=GetCurrentThreadId()==runtime_rpc_receive_thread;RuntimeSceneReferenceScope f={0};
    if(thread){f.previous=s->scope;runtime_scene_reference_capture(&f,(uintptr_t)property,(uintptr_t)archive,(uintptr_t)map,caller);
        /* Push inactive nested properties too; they cannot inherit our tag. */s->scope=&f;}
    SetLastError(before);int result=s->original(property,archive,map,data);DWORD after=GetLastError();
    if(thread){if(f.active)runtime_scene_reference_emit(&f,data,result);s->scope=f.previous;}
    SetLastError(after);return result;
}
static int runtime_scene_reference_serialize(void *property,void *archive,void *map,void *data) {
    return runtime_scene_reference_forward(property,archive,map,data,(uintptr_t)__builtin_return_address(0));
}
static void runtime_scene_reference_prepare(uintptr_t base) {
    RuntimeSceneReference *s=&runtime_scene_reference;
    if(s->installed||!runtime_rpc_receive_installed||!runtime_scene_reference_opted()||
       g_runtime_config.role!=ME3_ROLE_GUEST||GetCurrentThreadId()!=runtime_rpc_receive_thread)return;
    static const unsigned char pin[23]={0x49,0x8b,0,0x4d,0x8b,0xd0,0x4c,0x8b,0x81,0xd0,0,0,0,0x49,0x8b,0xca,0x48,0xff,0xa0,0xc8,2,0,0};
    unsigned char actual[23];uintptr_t slot=base+0x1077d88+0x2f8,expected=base+0x146f60;
    if(!supported_image(base)||read_ptr(slot)!=expected||!read_at(expected,actual,sizeof actual)||memcmp(actual,pin,sizeof pin))return;
    s->base=base;s->wanted=1;runtime_reference_extra_requested=runtime_scene_reference_requested;
    runtime_initial_reference_install(base);
    if(!runtime_initial_reference_installed)return;
    DWORD old=0,ignored;if(!VirtualProtect((void*)slot,8,PAGE_READWRITE,&old))return;
    s->original=(RuntimeSceneReferenceNative)expected;
    PVOID prior=InterlockedCompareExchangePointer((PVOID volatile*)slot,(PVOID)runtime_scene_reference_serialize,(PVOID)expected);
    VirtualProtect((void*)slot,8,old,&ignored);
    if((uintptr_t)prior!=expected){s->original=NULL;return;}
    runtime_reference_extra_bits=runtime_scene_reference_bits;runtime_reference_extra_index=runtime_scene_reference_index;
    s->installed=1;line("{\"runtime_scene_reference\":\"installed\",\"default_off\":true,\"limit\":64,\"output_only\":true,\"archive_replayed\":false}\r\n");
}
#endif
