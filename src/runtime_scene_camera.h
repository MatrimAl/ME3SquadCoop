#ifndef ME3_RUNTIME_SCENE_CAMERA_H
#define ME3_RUNTIME_SCENE_CAMERA_H
/* Called only around the real Camera.UpdateCamera script executor. Restore
 * the preceding owned transient POV before native update, then supplement its
 * completed CameraCache.POV. No actor transform, view target, mode or timestamp
 * is changed. A stale/ended scene returns to the engine's own cached result. */
typedef struct RuntimeSceneCameraLease {
    GuestCameraStamp camera,pc,world;
    uint32_t native[7],written[7];
    uint32_t timestamp;
    unsigned owned;
} RuntimeSceneCameraLease;
typedef struct RuntimeSceneCamera {
    uintptr_t base;
    GuestPlayerContextMethod update,fov;
    GuestCameraStamp pc,camera,world;
    RuntimeSceneCameraLease lease;
    unsigned events;
    unsigned publications;
    DWORD published_at;
    DWORD checked_at;
    uint64_t published_nonce;
    int prepared,busy;
} RuntimeSceneCamera;
static RuntimeSceneCamera runtime_scene_camera;
static void runtime_scene_camera_log(const char *event) {
    if(runtime_scene_camera.events++>=64)return;
    char row[320];snprintf(row,sizeof row,"{\"runtime_scene_camera\":\"%s\",\"cache_pov_only\":true,\"timestamp_written\":false,\"actors_moved\":false,\"gameplay_verified\":false,\"monotonic_ms\":%lu}\r\n",event,GetTickCount());line(row);
}
static int runtime_scene_camera_leaf_pin(uintptr_t base) {
    static const unsigned char leaf[]={0xf2,0x0f,0x10,0x81,0x48,0x04,0x00,0x00,0xf2,0x0f,0x11,0x02,0x8b,0x81,0x50,0x04,0x00,0x00,0x89,0x42,0x08,0xf2,0x0f,0x10,0x81,0x54,0x04,0x00,0x00,0xf2,0x41,0x0f,0x11,0x00,0x8b,0x81,0x5c,0x04,0x00,0x00,0x41,0x89,0x40,0x08,0xc3};
    unsigned char actual[sizeof leaf];return read_at(base+0x46fc60,actual,sizeof actual)&&!memcmp(actual,leaf,sizeof leaf);
}
static int runtime_scene_camera_struct(uintptr_t base,uintptr_t property,const char *path,int size,uintptr_t *out) {
    uintptr_t type=read_ptr(property+0xd0);int bytes=0,element=0,dimension=0;GuestCameraStamp stamp={0};
    if(!narrative_probe_object(base,property,"StructProperty")||!narrative_probe_object(base,type,"ScriptStruct")||
       !guest_camera_stamp_read(base,type,&stamp,1)||strcmp(stamp.full_name,path)||
       !read_at(property+0x74,&element,4)||element!=size||!read_at(property+0x70,&dimension,4)||dimension!=1||
       !read_at(type+0x78,&bytes,4)||bytes!=size)return 0;
    *out=type;return 1;
}
static int runtime_scene_camera_layout(uintptr_t base,uintptr_t camera) {
    uintptr_t owner=runtime_story_graph_class(base,camera,"Camera"),cache_type=0,pov_type=0;
    uintptr_t cache=runtime_native_hud_field(base,owner,"CameraCache","StructProperty",0x448,32);
    if(!owner||!cache||!runtime_scene_camera_struct(base,cache,"Engine.Camera.TCameraCache",32,&cache_type)||
       read_ptr(cache_type+0x40)!=owner||
       !runtime_native_hud_field(base,cache_type,"TimeStamp","FloatProperty",28,4))return 0;
    uintptr_t pov=runtime_native_hud_field(base,cache_type,"POV","StructProperty",0,28);
    return pov&&runtime_scene_camera_struct(base,pov,"Core.Object.TPOV",28,&pov_type)&&
        runtime_native_hud_field(base,owner,"PCOwner","ObjectProperty",0x4a8,8)&&runtime_scene_camera_leaf_pin(base);
}
static int runtime_scene_camera_fov_body(uintptr_t base,uintptr_t fn) {
    static const unsigned char code[81]={0x64,0,0,0,0,0,0,0,0,0x01,0x16,0,0x04,0x5f,0,0,0,0,0,0,0,0,0x04,0x35,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x35,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x01,0,0,0,0,0,0,0,0,0x04,0x3a,0,0,0,0,0,0,0,0,0x53};
    static const GuestEquipmentOperand ops[]={
        {1,"BoolProperty","Engine.Camera.bLockedFOV",0x504,UINT64_C(0)},
        {14,"FloatProperty","Engine.Camera.LockedFOV",0x4d4,UINT64_C(0)},
        {24,"FloatProperty","Core.Object.TPOV.FOV",24,UINT64_C(0)},
        {32,"ScriptStruct","Core.Object.TPOV",-1,UINT64_C(0)},
        {43,"StructProperty","Engine.Camera.TCameraCache.POV",0,UINT64_C(0)},
        {51,"ScriptStruct","Engine.Camera.TCameraCache",-1,UINT64_C(0)},
        {62,"StructProperty","Engine.Camera.CameraCache",0x448,UINT64_C(0)},
        {72,"FloatProperty","Engine.Camera.GetFOVAngle.ReturnValue",0,UINT64_C(0x580)},
        {0,NULL,NULL,0,0}};
    static const GuestEquipmentName names[]={{0,NULL}};
    return guest_equipment_body(base,fn,81,code,sizeof code,ops,names);
}
static int runtime_scene_camera_cells(uintptr_t address,const uint32_t expected[7],const uint32_t desired[7],unsigned *owned) {
    if((address&3)||!background_focus_writable(address,28))return 0;
    unsigned changed=0;
    for(unsigned i=0;i<7;++i){
        LONG previous=InterlockedCompareExchange((LONG volatile*)(address+i*4),(LONG)desired[i],(LONG)expected[i]);
        if((uint32_t)previous!=expected[i]){
            for(unsigned j=0;j<i;++j)if(changed&(1u<<j))
                InterlockedCompareExchange((LONG volatile*)(address+j*4),(LONG)expected[j],(LONG)desired[j]);
            *owned=0;return 0;}
        if(expected[i]!=desired[i])changed|=1u<<i;
    }
    *owned=changed;return 1;
}
/* Restore only a complete unchanged owned POV at its original timestamp.
 * Foreign/native tuples are preserved together, including unchanged cells. */
static int runtime_scene_camera_restore_cells(uintptr_t address,RuntimeSceneCameraLease *l) {
    if(!l->owned)return 1;
    if((address&3)||!background_focus_writable(address,28))return 0;
    uint32_t current[8];if(!read_at(address,current,sizeof current))return 0;
    if(memcmp(current,l->written,28)||current[7]!=l->timestamp){l->owned=0;return 1;}
    unsigned restored=0;
    if(!runtime_scene_camera_cells(address,l->written,l->native,&restored)){
        if(!read_at(address,current,sizeof current))return 0;
        if(!memcmp(current,l->written,28)&&current[7]==l->timestamp)return 0;
        /* A readable foreign/native tuple is preserved as one unit. An
         * unreadable or still-owned tuple retains the ledger for retry. */
    }
    l->owned=0;
    return 1;
}
static int runtime_scene_camera_lease_live(uintptr_t base,const RuntimeSceneCameraLease *l) {
    return guest_camera_stamp_matches(base,&l->camera,0)&&guest_camera_stamp_matches(base,&l->pc,0)&&
        guest_camera_stamp_matches(base,&l->world,0)&&read_ptr(l->camera.pointer+0x4a8)==l->pc.pointer&&
        read_ptr(l->camera.pointer+0x198)==l->world.pointer&&read_ptr(l->pc.pointer+0x198)==l->world.pointer;
}
/* 1 same captured identity, -1 proven expired/replaced, 0 unreadable/unknown.
 * Never convert a failed read or temporary relationship loss into disposal. */
static int runtime_scene_camera_stamp_state(uintptr_t base,const GuestCameraStamp *stamp) {
    GameArray objects={0},again={0};uintptr_t slot=0,current=0,klass=0;int32_t index=0,class_index=0;uint32_t name[2],class_name[2];
    if(!base||!stamp->pointer||stamp->index<0||!read_at(base+0x1887e40,&objects,sizeof objects)||
       !objects.data||objects.count<=0||objects.count>2000000||objects.max<objects.count)return 0;
    if(stamp->index>=objects.count)return read_at(base+0x1887e40,&again,sizeof again)&&!memcmp(&again,&objects,sizeof again)?-1:0;
    if(!read_at(objects.data+(uintptr_t)stamp->index*8,&slot,8))return 0;
    if(slot!=stamp->pointer){
        if(!read_at(base+0x1887e40,&again,sizeof again)||memcmp(&again,&objects,sizeof again)||
           !read_at(objects.data+(uintptr_t)stamp->index*8,&current,8)||current!=slot)return 0;
        return -1;}
    if(!read_at(stamp->pointer+8,&index,4)||!read_at(stamp->pointer+0x48,name,8)||
       !read_at(stamp->pointer+0x50,&klass,8))return 0;
    int expired=index!=stamp->index||memcmp(name,stamp->name,8)||klass!=stamp->klass;
    int32_t again_index=0;uint32_t again_name[2];uintptr_t again_class=0;
    if(!read_at(stamp->pointer+8,&again_index,4)||!read_at(stamp->pointer+0x48,again_name,8)||
       !read_at(stamp->pointer+0x50,&again_class,8)||again_index!=index||memcmp(again_name,name,8)||again_class!=klass)return 0;
    if(expired){
        if(!read_at(base+0x1887e40,&again,sizeof again)||memcmp(&again,&objects,sizeof again)||
           !read_at(objects.data+(uintptr_t)stamp->index*8,&current,8)||current!=slot)return 0;
        return -1;}
    if(!read_at(klass+8,&class_index,4)||!read_at(klass+0x48,class_name,8))return 0;
    expired=class_index!=stamp->class_index||memcmp(class_name,stamp->class_name,8);
    if(!read_at(klass+8,&again_index,4)||!read_at(klass+0x48,again_name,8)||again_index!=class_index||memcmp(again_name,class_name,8))return 0;
    if(!read_at(base+0x1887e40,&again,sizeof again)||memcmp(&again,&objects,sizeof again)||
       !read_at(objects.data+(uintptr_t)stamp->index*8,&current,8))return 0;
    return current==stamp->pointer?(expired?-1:1):0;
}
static int runtime_scene_camera_restore(uintptr_t base) {
    RuntimeSceneCameraLease *l=&runtime_scene_camera.lease;
    if(!l->owned)return 1;
    int camera=runtime_scene_camera_stamp_state(base,&l->camera),pc=runtime_scene_camera_stamp_state(base,&l->pc),world=runtime_scene_camera_stamp_state(base,&l->world);
    if(camera<0||pc<0||world<0){memset(l,0,sizeof *l);return 1;}
    if(!camera||!pc||!world||!runtime_scene_camera_lease_live(base,l))return 0;
    if(!runtime_scene_camera_restore_cells(l->camera.pointer+0x448,l))return 0;
    memset(l,0,sizeof *l);return 1;
}
static void runtime_scene_camera_retire(uintptr_t base) {
    RuntimeSceneCamera *s=&runtime_scene_camera;SceneSpectatorMessage message={0};
    if(!s->lease.owned)return;
    if(base!=s->base||!runtime_scene_camera_value(s->lease.camera.pointer,&message)){
        s->published_at=0;
        if(runtime_scene_camera_restore(s->base))runtime_scene_camera_log("cache-lease-retired");
    }
}
static int runtime_scene_camera_current_node(uintptr_t camera,uintptr_t frame,uintptr_t node) {
    RuntimeSceneCamera *s=&runtime_scene_camera;
    return s->prepared&&g_runtime_config.role==ME3_ROLE_GUEST&&GetCurrentThreadId()==session_game_thread&&
        camera==s->camera.pointer&&read_ptr(frame+0x18)==node&&read_ptr(frame+0x20)==camera&&
        guest_player_context_method_matches(s->base,&s->update,0)&&
        read_ptr(s->update.function.pointer+0xd8)==s->base+0xf52b0&&
        guest_camera_stamp_matches(s->base,&s->camera,0)&&guest_camera_stamp_matches(s->base,&s->pc,0)&&
        guest_camera_stamp_matches(s->base,&s->world,0)&&local_controller(session_engine)==s->pc.pointer&&
        read_ptr(s->pc.pointer+0x684)==camera&&read_ptr(s->pc.pointer+0xe28)==camera&&
        read_ptr(camera+0x4a8)==s->pc.pointer&&read_ptr(camera+0x198)==s->world.pointer&&
        read_ptr(s->pc.pointer+0x198)==s->world.pointer;
}
static int runtime_scene_camera_current(uintptr_t camera,uintptr_t frame) {
    return runtime_scene_camera_current_node(camera,frame,runtime_scene_camera_node);
}
static int runtime_scene_camera_update_selected(int busy,int current) {return !busy&&current;}
static void runtime_scene_camera_published(uintptr_t camera,uintptr_t frame,const SceneSpectatorMessage *message,const uint32_t expected[8]) {
    RuntimeSceneCamera *s=&runtime_scene_camera;DWORD now=GetTickCount();
    if(s->publications>=32||(s->published_at&&s->published_nonce==message->nonce&&now-s->published_at<1000))return;
    uint32_t readback[8]={0};int read=read_at(camera+0x448,readback,sizeof readback);
    SceneSpectatorMessage actual=*message;
    memcpy(actual.position,readback,12);memcpy(actual.rotation,readback+3,12);memcpy(&actual.fov,readback+6,4);
    int finite=read&&scene_spectator_message_valid(&actual);
    int verified=finite&&!memcmp(readback,expected,28)&&readback[7]==expected[7]&&runtime_scene_camera_current(camera,frame);
    char row[850];snprintf(row,sizeof row,"{\"runtime_scene_camera\":\"host-scene-cache-published\",\"nonce\":%llu,\"sequence\":%llu,\"readback_verified\":%s,\"actual_pov_valid\":%s,\"actual_position\":[%.6g,%.6g,%.6g],\"actual_rotation\":[%d,%d,%d],\"actual_fov\":%.6g,\"timestamp_written\":false,\"actors_moved\":false,\"gameplay_verified\":false,\"monotonic_ms\":%lu}\r\n",
        (unsigned long long)message->nonce,(unsigned long long)message->sequence,verified?"true":"false",finite?"true":"false",
        finite?(double)actual.position[0]:0,finite?(double)actual.position[1]:0,finite?(double)actual.position[2]:0,
        finite?actual.rotation[0]:0,finite?actual.rotation[1]:0,finite?actual.rotation[2]:0,finite?(double)actual.fov:0,now);line(row);
    ++s->publications;s->published_at=now;s->published_nonce=message->nonce;
}
static void runtime_scene_camera_after(uintptr_t camera,uintptr_t frame) {
    RuntimeSceneCamera *s=&runtime_scene_camera;SceneSpectatorMessage message={0},again={0};
    RuntimeSceneCameraLease lease={0};uint32_t before[8],checked[8];
    if(s->lease.owned||!runtime_scene_camera_current(camera,frame)||!runtime_scene_camera_value(camera,&message)||
       !scene_spectator_message_valid(&message)||!message.active||!read_at(camera+0x448,before,sizeof before))return;
    float native_fov=0,timestamp=0;memcpy(&native_fov,before+6,4);memcpy(&timestamp,before+7,4);
    if(!isfinite(native_fov)||native_fov<1.0f||native_fov>179.0f||!isfinite(timestamp))return;
    lease.camera=s->camera;lease.pc=s->pc;lease.world=s->world;lease.timestamp=before[7];memcpy(lease.native,before,28);
    scene_spectator_words(&message,lease.written);
    if(!runtime_scene_camera_value(camera,&again)||!scene_spectator_message_same(&message,&again)||
       !runtime_scene_camera_current(camera,frame)||!read_at(camera+0x448,checked,sizeof checked)||memcmp(before,checked,sizeof before))return;
    if(runtime_scene_camera_cells(camera+0x448,lease.native,lease.written,&lease.owned)&&lease.owned){
        s->lease=lease;uint32_t published[8];memcpy(published,lease.written,28);published[7]=lease.timestamp;
        runtime_scene_camera_published(camera,frame,&message,published);}
}
static void runtime_scene_camera_forward(void *self,void *frame,void *result,RuntimeAmmoPickupExec stock) {
    DWORD error=GetLastError();RuntimeSceneCamera *s=&runtime_scene_camera;
    int needed=s->lease.owned||runtime_scene_camera_requested();
    int selected=runtime_scene_camera_update_selected(s->busy,needed&&!s->busy&&runtime_scene_camera_current((uintptr_t)self,(uintptr_t)frame));
    if(selected&&s->lease.owned&&s->lease.camera.pointer==(uintptr_t)self)runtime_scene_camera_restore(s->base);
    if(selected)++s->busy;
    SetLastError(error);stock(self,frame,result);error=GetLastError();
    if(selected)--s->busy;
    if(selected)runtime_scene_camera_after((uintptr_t)self,(uintptr_t)frame);
    SetLastError(error);
}
static int runtime_scene_camera_fov_select(int eligible,uintptr_t result,uint32_t expected,float fov) {
    uint32_t desired=0;memcpy(&desired,&fov,4);
    return eligible&&result&&!(result&3)&&isfinite(fov)&&fov>=1.0f&&fov<=179.0f&&
        background_focus_writable(result,4)&&
        (uint32_t)InterlockedCompareExchange((LONG volatile*)result,(LONG)desired,(LONG)expected)==expected;
}
static void runtime_scene_camera_fov_forward(void *self,void *frame,void *result,RuntimeAmmoPickupExec stock) {
    DWORD error=GetLastError();RuntimeSceneCamera *s=&runtime_scene_camera;
    int selected=runtime_scene_camera_requested()&&!s->busy&&runtime_scene_camera_current_node((uintptr_t)self,(uintptr_t)frame,runtime_scene_camera_fov_node)&&
        guest_player_context_method_matches(s->base,&s->fov,0)&&read_ptr(s->fov.function.pointer+0xd8)==s->base+0xf52b0;
    SetLastError(error);stock(self,frame,result);error=GetLastError();
    if(selected&&!s->busy){SceneSpectatorMessage value={0},again={0};uint32_t native=0;
        if(runtime_scene_camera_current_node((uintptr_t)self,(uintptr_t)frame,runtime_scene_camera_fov_node)&&
           runtime_scene_camera_value((uintptr_t)self,&value)&&scene_spectator_message_valid(&value)&&value.active&&
           result&&read_at((uintptr_t)result,&native,4)&&runtime_scene_camera_value((uintptr_t)self,&again)&&
           scene_spectator_message_same(&value,&again))runtime_scene_camera_fov_select(1,(uintptr_t)result,native,value.fov);
    }
    SetLastError(error);
}
static void runtime_scene_camera_prepare(uintptr_t base,uintptr_t pc) {
    RuntimeSceneCamera *s=&runtime_scene_camera;
    runtime_scene_camera_retire(base);
    if(g_runtime_config.role!=ME3_ROLE_GUEST||GetCurrentThreadId()!=session_game_thread||
       !guest_camera_actor_live(base,pc)||!is_kind_of(base,pc,"SFXPlayerController"))return;
    uintptr_t camera=read_ptr(pc+0x684),world=read_ptr(pc+0x198);GameArray script={0};
    if(!guest_camera_actor_live(base,camera)||!is_kind_of(base,camera,"SFXPlayerCamera")||
       read_ptr(pc+0xe28)!=camera||read_ptr(camera+0x4a8)!=pc||read_ptr(camera+0x198)!=world)return;
    if(s->prepared&&s->base==base&&s->pc.pointer==pc&&s->camera.pointer==camera&&
       guest_camera_stamp_matches(base,&s->pc,0)&&guest_camera_stamp_matches(base,&s->camera,0)&&
       guest_camera_stamp_matches(base,&s->world,0)&&guest_player_context_method_matches(base,&s->update,0)&&
       guest_player_context_method_matches(base,&s->fov,0))return;
    DWORD now=GetTickCount();if(s->checked_at&&now-s->checked_at<1000)return;s->checked_at=now;
    if(s->lease.owned&&!runtime_scene_camera_restore(s->base))return;
    s->prepared=0;runtime_scene_camera_node=0;runtime_scene_camera_fov_node=0;
    uintptr_t fn=guest_player_context_find_method(base,read_ptr(camera+0x50),"Camera","UpdateCamera");
    uintptr_t fov=guest_player_context_find_method(base,read_ptr(camera+0x50),"Camera","GetFOVAngle");
    GuestPlayerContextMethod method={0},fov_method={0};
    if(!supported_image(base)||!guest_player_context_method_stamp(base,fn,&method)||
       strcmp(method.function.full_name,"Engine.Camera.UpdateCamera")||
       !guest_player_context_function_abi(base,fn,"Camera","UpdateCamera",0x820902,4,65535,0,0xf52b0,1)||
       !guest_player_context_method_stamp(base,fov,&fov_method)||strcmp(fov_method.function.full_name,"Engine.Camera.GetFOVAngle")||
       !guest_player_context_function_abi(base,fov,"Camera","GetFOVAngle",0x20002,4,0,0,0xf52b0,1)||
       !runtime_delayed_fire_property(base,read_ptr(fn+0x70),"FloatProperty","Engine.Camera.UpdateCamera.DeltaTime",0,4,0x80)||
       !runtime_delayed_fire_property(base,read_ptr(fov+0x70),"FloatProperty","Engine.Camera.GetFOVAngle.ReturnValue",0,4,0x580)||
       !runtime_scene_camera_fov_body(base,fov)||
       !read_at(fn+0x7c,&script,sizeof script)||script.count!=1342||script.max<script.count||script.max>4096||
       !runtime_scene_camera_layout(base,camera)||
       !guest_camera_stamp_read(base,pc,&s->pc,0)||!guest_camera_stamp_read(base,camera,&s->camera,0)||
       !guest_camera_stamp_read(base,world,&s->world,0)||!runtime_script_executor_prepare(base))return;
    s->base=base;s->update=method;s->fov=fov_method;s->prepared=1;runtime_scene_camera_node=fn;runtime_scene_camera_fov_node=fov;
    runtime_scene_camera_log("native-cache-boundary-prepared");
}
#ifdef ME3_RUNTIME_FIXTURE
static void *runtime_scene_camera_fixture_args[3];static unsigned runtime_scene_camera_fixture_calls;static int runtime_scene_camera_fixture_bad;
static void runtime_scene_camera_fixture_stock(void *self,void *frame,void *result) {
    if(self!=runtime_scene_camera_fixture_args[0]||frame!=runtime_scene_camera_fixture_args[1]||
       result!=runtime_scene_camera_fixture_args[2]||GetLastError()!=37)runtime_scene_camera_fixture_bad=1;
    ++runtime_scene_camera_fixture_calls;SetLastError(53);
}
static int runtime_scene_camera_selftest(void) {
    DWORD error=GetLastError();uint32_t storage[8]={1,2,3,4,5,6,7,UINT32_C(0xdeadbeef)};
    uint32_t wanted[7]={11,12,13,14,15,16,17};RuntimeSceneCameraLease lease={0};
    memcpy(lease.native,storage,28);memcpy(lease.written,wanted,28);lease.timestamp=storage[7];
    int ok=runtime_scene_camera_cells((uintptr_t)storage,lease.native,wanted,&lease.owned)&&lease.owned==127&&
        !memcmp(storage,wanted,28)&&storage[7]==UINT32_C(0xdeadbeef);
    storage[3]=99;
    ok=ok&&runtime_scene_camera_restore_cells((uintptr_t)storage,&lease)&&!lease.owned&&storage[3]==99&&
        storage[0]==11&&storage[6]==17&&storage[7]==UINT32_C(0xdeadbeef);
    memcpy(storage,lease.native,28);ok=ok&&runtime_scene_camera_cells((uintptr_t)storage,lease.native,wanted,&lease.owned);
    ok=ok&&runtime_scene_camera_restore_cells((uintptr_t)storage,&lease)&&!memcmp(storage,lease.native,28);
    memcpy(storage,wanted,28);lease.owned=127;storage[7]++;
    ok=ok&&runtime_scene_camera_restore_cells((uintptr_t)storage,&lease)&&!memcmp(storage,wanted,28);storage[7]--;
    memcpy(storage,lease.native,28);storage[3]=99;unsigned owned=0;
    ok=ok&&!runtime_scene_camera_cells((uintptr_t)storage,lease.native,wanted,&owned)&&!owned&&storage[0]==1&&storage[2]==3&&storage[3]==99;
    RuntimeSceneCamera saved=runtime_scene_camera;runtime_scene_camera.prepared=0;
    int a=1,b=2,c=3;runtime_scene_camera_fixture_args[0]=&a;runtime_scene_camera_fixture_args[1]=&b;runtime_scene_camera_fixture_args[2]=&c;
    runtime_scene_camera_fixture_calls=0;runtime_scene_camera_fixture_bad=0;SetLastError(37);
    runtime_scene_camera_forward(&a,&b,&c,runtime_scene_camera_fixture_stock);
    ok=ok&&runtime_scene_camera_fixture_calls==1&&!runtime_scene_camera_fixture_bad&&GetLastError()==53;
    ok=ok&&runtime_scene_camera_update_selected(0,1)&&!runtime_scene_camera_update_selected(1,1)&&
        !runtime_scene_camera_update_selected(0,0)&&!runtime_scene_camera_update_selected(2,1);
    runtime_scene_camera.busy=1;runtime_scene_camera_fixture_calls=0;SetLastError(37);
    runtime_scene_camera_forward(&a,&b,&c,runtime_scene_camera_fixture_stock);
    ok=ok&&runtime_scene_camera.busy==1&&runtime_scene_camera_fixture_calls==1&&GetLastError()==53;
    runtime_scene_camera.busy=0;
    runtime_scene_camera_fixture_calls=0;SetLastError(37);
    runtime_scene_camera_fov_forward(&a,&b,&c,runtime_scene_camera_fixture_stock);
    ok=ok&&runtime_scene_camera_fixture_calls==1&&!runtime_scene_camera_fixture_bad&&GetLastError()==53;
    uint32_t result[3]={UINT32_C(0x12345678),UINT32_C(0x3f800000),UINT32_C(0x87654321)};
    ok=ok&&runtime_scene_camera_fov_select(1,(uintptr_t)(result+1),result[1],90.0f)&&
        result[0]==UINT32_C(0x12345678)&&result[1]==UINT32_C(0x42b40000)&&result[2]==UINT32_C(0x87654321)&&
        !runtime_scene_camera_fov_select(0,(uintptr_t)(result+1),result[1],100.0f)&&
        !runtime_scene_camera_fov_select(1,(uintptr_t)(result+1),UINT32_C(0x3f800000),100.0f)&&
        !runtime_scene_camera_fov_select(1,(uintptr_t)(result+1),result[1],NAN);
    runtime_scene_camera=saved;SetLastError(error);return ok;
}
#endif
#endif
