#ifndef ME3_RUNTIME_SCENE_ACTOR_OBSERVER_H
#define ME3_RUNTIME_SCENE_ACTOR_OBSERVER_H
/* Read-only evidence at real stock BioPawn animation-control boundaries.
 * This observer never calls an event, changes a pawn or repairs animation. */
#include "runtime_scene_actor_observer_pins.h"

typedef struct RuntimeSceneActorCell {
    uintptr_t raw;int readable,live;GuestCameraStamp stamp;
} RuntimeSceneActorCell;
typedef struct RuntimeSceneActorArray {
    GameArray header;uint64_t hash;unsigned sampled;int readable,valid,complete,stable,truncated;
} RuntimeSceneActorArray;
typedef struct RuntimeSceneActorSnapshot {
    RuntimeSceneScope scope;GuestCameraStamp actor,klass,world;
    RuntimeSceneActorCell group,weapon,inventory,owner,instigator,weapon_inventory,mesh,gestures,gesture_manager;
    RuntimeSceneActorArray modules,anim_sets,backup_sets;
    int valid,reason,channel_state,channel_index,action_read,power_read,matinee_read,action,power;
    uint32_t matinee;uintptr_t gesture_owner;int gesture_owner_read,gesture_state_read,gesture_state;
} RuntimeSceneActorSnapshot;
typedef struct RuntimeSceneActorCapture {
    RuntimeSceneActorSnapshot before;uintptr_t actor,frame,node,locals;unsigned kind,serial,depth;int frame_valid,relevant;
} RuntimeSceneActorCapture;
typedef struct RuntimeSceneActorObserver {
    uintptr_t base,engine;GuestPlayerContextMethod method[2];DWORD checked;unsigned pairs,depth,preparation_rows;int prepared;
} RuntimeSceneActorObserver;
static RuntimeSceneActorObserver runtime_scene_actor_observer;
static const char *runtime_scene_actor_observer_names[]={"BeginAnimControl","FinishAnimControl"};

static int runtime_scene_actor_observer_node_match(uintptr_t node) {
    RuntimeSceneActorObserver *s=&runtime_scene_actor_observer;
    return s->prepared&&(node==s->method[0].function.pointer||node==s->method[1].function.pointer);
}
static RuntimeSceneActorCell runtime_scene_actor_observer_cell(uintptr_t base,uintptr_t at,const char *kind) {
    RuntimeSceneActorCell c={0};c.readable=read_at(at,&c.raw,8);
    if(c.readable&&c.raw&&is_live(base,c.raw)&&(!kind||is_kind_of(base,c.raw,kind)))
        c.live=guest_camera_stamp_read(base,c.raw,&c.stamp,1);
    return c;
}
/* Pointer hashes describe the bounded sample, never animation playback. An
 * oversized or changing header is explicit; a read failure is not an empty list. */
static RuntimeSceneActorArray runtime_scene_actor_observer_array(uintptr_t at,unsigned limit) {
    RuntimeSceneActorArray a={0};GameArray after={0};uintptr_t pointers[128];
    a.readable=read_at(at,&a.header,sizeof a.header);
    if(!a.readable||a.header.count<0||a.header.count>4096||a.header.max<a.header.count||a.header.max>4096||
       (a.header.count&&(!a.header.data||(a.header.data&7))))return a;
    a.valid=1;a.sampled=(unsigned)a.header.count<limit?(unsigned)a.header.count:limit;
    a.truncated=(unsigned)a.header.count>a.sampled;
    if(a.sampled>128||(a.sampled&&!read_at(a.header.data,pointers,a.sampled*8)))return a;
    a.hash=UINT64_C(14695981039346656037);
    for(unsigned i=0;i<a.sampled;++i)for(unsigned j=0;j<8;++j){a.hash^=(pointers[i]>>(j*8))&255;a.hash*=UINT64_C(1099511628211);}
    a.complete=1;a.stable=read_at(at,&after,sizeof after)&&!memcmp(&a.header,&after,sizeof after);return a;
}
/* Current native owner-PC context remains valid when a scene temporarily
 * releases Ashley. No pawn possession or actor-channel requirement is added. */
static int runtime_scene_actor_observer_scope(uintptr_t base,uintptr_t engine,RuntimeSceneScope *out) {
    SessionContext c=session_context(engine);NetworkActorScope world={0};NetworkActorIdentityEntry peer={0};
    if(!runtime_scene_context(base,c,&world))return 0;
    if(g_runtime_config.role==ME3_ROLE_HOST)return runtime_scene_host_peer(base,c,out);
    if(g_runtime_config.role!=ME3_ROLE_GUEST)return 0;
    uintptr_t connection=read_ptr(world.driver+0x80);
    if(!network_actor_connection_in_scope(base,world.driver,connection)||read_ptr(connection+0x68)!=c.pc||
       network_actor_scan_driver(base,c.pc,world.driver,&peer)<0||peer.connection!=connection)return 0;
    return runtime_scene_capture(base,c,&world,&peer,out);
}
static int runtime_scene_actor_observer_method(uintptr_t base,unsigned kind,const GuestPlayerContextMethod *m,int body) {
    uintptr_t fn=m->function.pointer,field=read_ptr(fn+0x70);GameArray script={0};GuestCameraStamp parameter_class={0};
    char path[160];snprintf(path,sizeof path,"SFXGame.BioPawn.%s.InInterpGroup",runtime_scene_actor_observer_names[kind]);
    if(!guest_player_context_method_matches(base,m,1)||
       !guest_player_context_function_abi(base,fn,"BioPawn",runtime_scene_actor_observer_names[kind],0x20902,8,65535,0,0xf52b0,1)||
       !read_at(fn+0x7c,&script,sizeof script)||script.count!=(kind?313:539)||script.max<script.count||script.max>4096)return 0;
    unsigned count=0,seen=0;uintptr_t prior[32];
    while(field&&seen<32){uint64_t flags=0;
        for(unsigned i=0;i<seen;++i)if(prior[i]==field)return 0;prior[seen++]=field;
        if(!is_live(base,field)||read_ptr(field+0x40)!=fn||!read_at(field+0x78,&flags,8))return 0;
        if(flags&0x80){if(++count!=1||!runtime_delayed_fire_property(base,field,"ObjectProperty",path,0,8,0x80)||
            !guest_camera_stamp_read(base,read_ptr(field+0xd0),&parameter_class,1)||strcmp(parameter_class.full_name,"Engine.InterpGroup"))return 0;}
        if(!read_at(field+0x68,&field,8))return 0;
    }
    /* The sole parameter has an exact class, layout and flags. Local variables
     * are allowed and independently covered by the relocated body pin. */
    return !field&&count==1&&(!body||runtime_scene_actor_observer_body(base,fn,kind));
}

static void runtime_scene_actor_observer_snapshot(uintptr_t actor,uintptr_t group,int group_read,RuntimeSceneActorSnapshot *q) {
    RuntimeSceneActorObserver *s=&runtime_scene_actor_observer;uintptr_t base=s->base;GameArray modules_after={0};uintptr_t modules[128];
    memset(q,0,sizeof *q);q->channel_state=-1;q->channel_index=-1;q->reason=1;
    if(!runtime_scene_actor_observer_scope(base,s->engine,&q->scope))return;
    q->reason=2;
    if(!guest_camera_actor_live(base,actor)||!is_kind_of(base,actor,"BioPawn")||!is_kind_of(base,actor,"SFXPawn_PlayerParty")||
       read_ptr(actor+0x198)!=q->scope.world.pointer||!guest_camera_stamp_read(base,actor,&q->actor,1)||
       !guest_camera_stamp_read(base,q->actor.klass,&q->klass,1)||!guest_camera_stamp_read(base,q->scope.world.pointer,&q->world,1))return;
    q->valid=1;q->reason=0;q->group.raw=group;q->group.readable=group_read;
    if(group_read&&group&&is_live(base,group)&&is_kind_of(base,group,"InterpGroup"))q->group.live=guest_camera_stamp_read(base,group,&q->group.stamp,1);
    NetworkActorIdentityEntry channel={0};q->channel_state=network_actor_scan_driver(base,actor,q->scope.driver.pointer,&channel);
    if(q->channel_state>=0)q->channel_index=channel.index;
    q->weapon=runtime_scene_actor_observer_cell(base,actor+0x41c,"Inventory");
    q->inventory=runtime_scene_actor_observer_cell(base,actor+0x414,"InventoryManager");
    q->mesh=runtime_scene_actor_observer_cell(base,actor+0x3f4,"SkeletalMeshComponent");
    q->action_read=read_at(actor+0xc20,&q->action,4);q->power_read=read_at(actor+0xc28,&q->power,4);
    if(q->weapon.live){q->owner=runtime_scene_actor_observer_cell(base,q->weapon.raw+0x180,"Actor");
        q->instigator=runtime_scene_actor_observer_cell(base,q->weapon.raw+0x190,"Pawn");
        q->weapon_inventory=runtime_scene_actor_observer_cell(base,q->weapon.raw+0x2b0,"InventoryManager");}
    q->modules=runtime_scene_actor_observer_array(actor+0x70,128);
    if(q->modules.valid&&q->modules.complete&&q->modules.stable&&!q->modules.truncated&&
       (!q->modules.sampled||read_at(q->modules.header.data,modules,q->modules.sampled*8))){
        unsigned found=0;
        for(unsigned i=0;i<q->modules.sampled;++i)if(is_live(base,modules[i])&&is_kind_of(base,modules[i],"SFXModule_Gestures")){
            ++found;q->gestures.raw=modules[i];q->gestures.readable=1;
            q->gestures.live=guest_camera_stamp_read(base,modules[i],&q->gestures.stamp,1);
        }
        if(found!=1||!read_at(actor+0x70,&modules_after,sizeof modules_after)||memcmp(&modules_after,&q->modules.header,sizeof modules_after))q->gestures.live=0;
    }
    if(q->gestures.live){q->gesture_owner_read=read_at(q->gestures.raw+0x60,&q->gesture_owner,8);
        if(q->gesture_owner_read&&q->gesture_owner==actor){
            q->matinee_read=read_at(q->gestures.raw+0x174,&q->matinee,4);
            q->gesture_manager=runtime_scene_actor_observer_cell(base,q->gestures.raw+0x15c,"BioGestureAnimSetMgr");
            if(q->gesture_manager.live)q->gesture_state_read=read_at(q->gesture_manager.raw+0xbc,&q->gesture_state,4);
            q->backup_sets=runtime_scene_actor_observer_array(q->gestures.raw+0x100,64);
        }}
    if(q->mesh.live)q->anim_sets=runtime_scene_actor_observer_array(q->mesh.raw+0x320,16);
}
static const char *runtime_scene_actor_observer_bool(int value){return value?"true":"false";}
static void runtime_scene_actor_observer_emit_cell(unsigned serial,unsigned depth,const char *phase,const char *field,const RuntimeSceneActorCell *c) {
    char row[2000],path[1200];identity_quote(c->live?c->stamp.full_name:"",path,sizeof path);snprintf(row,sizeof row,
        "{\"runtime_scene_actor_observer\":\"pointer\",\"serial\":%u,\"depth\":%u,\"phase\":\"%s\",\"field\":\"%s\",\"readable\":%s,\"raw\":\"%p\",\"null\":%s,\"live\":%s,\"index\":%d,\"class_index\":%d,\"name\":[%u,%u],\"class_name\":[%u,%u],\"path\":\"%s\"}\r\n",
        serial,depth,phase,field,runtime_scene_actor_observer_bool(c->readable),(void*)c->raw,runtime_scene_actor_observer_bool(c->readable&&!c->raw),runtime_scene_actor_observer_bool(c->live),
        c->stamp.index,c->stamp.class_index,c->stamp.name[0],c->stamp.name[1],c->stamp.class_name[0],c->stamp.class_name[1],path);line(row);
}
static void runtime_scene_actor_observer_emit_array(unsigned serial,unsigned depth,const char *phase,const char *field,const RuntimeSceneActorArray *a) {
    char row[660];snprintf(row,sizeof row,
        "{\"runtime_scene_actor_observer\":\"array\",\"serial\":%u,\"depth\":%u,\"phase\":\"%s\",\"field\":\"%s\",\"readable\":%s,\"data\":\"%p\",\"count\":%d,\"max\":%d,\"valid\":%s,\"sampled\":%u,\"sample_complete\":%s,\"header_stable\":%s,\"truncated\":%s,\"pointer_hash\":\"%016llx\"}\r\n",
        serial,depth,phase,field,runtime_scene_actor_observer_bool(a->readable),(void*)a->header.data,a->header.count,a->header.max,runtime_scene_actor_observer_bool(a->valid),a->sampled,
        runtime_scene_actor_observer_bool(a->complete),runtime_scene_actor_observer_bool(a->stable),runtime_scene_actor_observer_bool(a->truncated),(unsigned long long)a->hash);line(row);
}
static void runtime_scene_actor_observer_emit(const RuntimeSceneActorCapture *c,const RuntimeSceneActorSnapshot *q,const char *phase,int same_actor,int same_scope,int same_group) {
    char row[5000],actor_path[1200],class_path[1200];identity_quote(q->valid?q->actor.full_name:"",actor_path,sizeof actor_path);identity_quote(q->valid?q->klass.full_name:"",class_path,sizeof class_path);snprintf(row,sizeof row,
        "{\"runtime_scene_actor_observer\":\"%s\",\"boundary\":\"%s\",\"role\":%d,\"serial\":%u,\"depth\":%u,\"snapshot_valid\":%s,\"refusal\":%d,\"actor\":\"%p\",\"actor_index\":%d,\"actor_name\":[%u,%u],\"actor_path\":\"%s\",\"class\":\"%p\",\"class_index\":%d,\"class_name\":[%u,%u],\"class_path\":\"%s\",\"world\":\"%p\",\"world_index\":%d,\"pc_index\":%d,\"owner_pc_channel_index\":%d,\"actor_channel_scan_index_or_unknown\":%d,\"actor_channel_found\":%s,\"actor_channel_index\":%d,\"same_actor\":%s,\"same_scope\":%s,\"same_group\":%s,\"group_raw_equal\":%s,\"frame_valid\":%s,\"action_readable\":%s,\"action_id\":%d,\"power_readable\":%s,\"power_action_id\":%d,\"matinee_readable\":%s,\"matinee_bit\":%s,\"gesture_owner_readable\":%s,\"gesture_owner\":\"%p\",\"gesture_state_readable\":%s,\"gesture_state\":%d,\"read_only\":true,\"gameplay_verified\":false,\"monotonic_ms\":%lu}\r\n",
        phase,runtime_scene_actor_observer_names[c->kind],(int)g_runtime_config.role,c->serial,c->depth,runtime_scene_actor_observer_bool(q->valid),q->reason,(void*)c->actor,q->actor.index,q->actor.name[0],q->actor.name[1],actor_path,
        (void*)q->klass.pointer,q->klass.index,q->klass.name[0],q->klass.name[1],class_path,(void*)q->world.pointer,q->world.index,q->scope.pc.index,q->scope.channel.index,q->channel_state,runtime_scene_actor_observer_bool(q->channel_state>=0),q->channel_index,
        runtime_scene_actor_observer_bool(same_actor),runtime_scene_actor_observer_bool(same_scope),runtime_scene_actor_observer_bool(same_group),runtime_scene_actor_observer_bool(q->group.readable&&c->before.group.readable&&q->group.raw==c->before.group.raw),runtime_scene_actor_observer_bool(c->frame_valid),runtime_scene_actor_observer_bool(q->action_read),q->action,
        runtime_scene_actor_observer_bool(q->power_read),q->power,runtime_scene_actor_observer_bool(q->matinee_read),runtime_scene_actor_observer_bool(q->matinee_read&&(q->matinee&1)),runtime_scene_actor_observer_bool(q->gesture_owner_read),(void*)q->gesture_owner,runtime_scene_actor_observer_bool(q->gesture_state_read),q->gesture_state,GetTickCount());line(row);
    if(!q->valid)return;
    const RuntimeSceneActorCell *cells[]={&q->group,&q->weapon,&q->inventory,&q->owner,&q->instigator,&q->weapon_inventory,&q->mesh,&q->gestures,&q->gesture_manager};
    const char *names[]={"InInterpGroup","Pawn.Weapon","Pawn.InvManager","Weapon.Owner","Weapon.Instigator","Weapon.InvManager","Pawn.Mesh","Gestures","Gestures.AnimSetMgr"};
    for(unsigned i=0;i<9;++i)runtime_scene_actor_observer_emit_cell(c->serial,c->depth,phase,names[i],cells[i]);
    runtime_scene_actor_observer_emit_array(c->serial,c->depth,phase,"Actor.Modules",&q->modules);
    runtime_scene_actor_observer_emit_array(c->serial,c->depth,phase,"Mesh.AnimSets",&q->anim_sets);
    runtime_scene_actor_observer_emit_array(c->serial,c->depth,phase,"Gestures.BackupAnimSets",&q->backup_sets);
}
static void runtime_scene_actor_observer_before(void *self,void *frame,RuntimeSceneActorCapture *c) {
    RuntimeSceneActorObserver *s=&runtime_scene_actor_observer;uintptr_t node=0,frame_actor=0,group=0,locals=0;GameArray script={0};
    c->actor=(uintptr_t)self;c->frame=(uintptr_t)frame;
    runtime_scene_actor_observer_snapshot(c->actor,0,0,&c->before);c->relevant=c->before.valid;
    if(!c->relevant)return;
    c->before.reason=3;
    if(!read_at(c->frame+0x18,&node,8)||!runtime_scene_actor_observer_node_match(node))goto emit;
    c->node=node;c->kind=node==s->method[1].function.pointer;
    if(!runtime_scene_actor_observer_method(s->base,c->kind,&s->method[c->kind],1)||
       !read_at(c->frame+0x20,&frame_actor,8)||frame_actor!=c->actor||!read_at(c->frame+0x30,&locals,8)||!locals||
       !read_at(node+0x7c,&script,sizeof script)||read_ptr(c->frame+0x28)!=script.data)goto emit;
    c->locals=locals;c->frame_valid=1;c->before.reason=0;
    c->before.group.readable=read_at(locals,&group,8);c->before.group.raw=group;
    if(c->before.group.readable&&group&&is_live(s->base,group)&&is_kind_of(s->base,group,"InterpGroup"))
        c->before.group.live=guest_camera_stamp_read(s->base,group,&c->before.group.stamp,1);
 emit:
    runtime_scene_actor_observer_emit(c,&c->before,"before",c->before.valid,c->before.valid,c->before.group.readable&&(!group||c->before.group.live));
}
static void runtime_scene_actor_observer_after(void *self,void *frame,const RuntimeSceneActorCapture *c) {
    (void)self;(void)frame;RuntimeSceneActorObserver *s=&runtime_scene_actor_observer;RuntimeSceneActorSnapshot q={0};uintptr_t group=0,locals=0;
    int group_read=c->frame_valid&&read_at(c->frame+0x30,&locals,8)&&locals==c->locals&&read_at(locals,&group,8);
    runtime_scene_actor_observer_snapshot(c->actor,group,group_read,&q);if(!c->frame_valid)q.reason=3;
    int same_actor=q.valid&&c->before.valid&&guest_camera_stamp_matches(s->base,&c->before.actor,1)&&q.actor.pointer==c->before.actor.pointer;
    int same_scope=q.valid&&c->before.valid&&runtime_scene_scope_live(s->base,&c->before.scope)&&
        q.scope.pc.pointer==c->before.scope.pc.pointer&&q.scope.world.pointer==c->before.scope.world.pointer&&q.scope.root.pointer==c->before.scope.root.pointer&&
        q.scope.driver.pointer==c->before.scope.driver.pointer&&q.scope.connection.pointer==c->before.scope.connection.pointer&&q.scope.peer.pointer==c->before.scope.peer.pointer&&
        q.scope.channel.pointer==c->before.scope.channel.pointer&&q.scope.camera.pointer==c->before.scope.camera.pointer&&q.scope.manager.pointer==c->before.scope.manager.pointer;
    int same_group=q.group.readable&&c->before.group.readable&&q.group.raw==c->before.group.raw&&
        ((!q.group.raw)||(q.group.live&&c->before.group.live&&guest_camera_stamp_matches(s->base,&c->before.group.stamp,1)));
    runtime_scene_actor_observer_emit(c,&q,"after",same_actor,same_scope,same_group);
}
/* Kept pure with injected captures for the fixture. Stock receives the exact
 * three incoming arguments once, including refused, exhausted and nested calls. */
static void runtime_scene_actor_observer_observe(void *self,void *frame,void *result,RuntimeAmmoPickupExec stock,
        RuntimeSceneActorObserver *s,void (*before)(void*,void*,RuntimeSceneActorCapture*),
        void (*after)(void*,void*,const RuntimeSceneActorCapture*)) {
    DWORD entry=GetLastError();unsigned depth=s->depth;int admitted=depth<4&&s->pairs<64;
    RuntimeSceneActorCapture capture={0};
    if(admitted){s->depth=depth+1;capture.depth=depth+1;capture.serial=s->pairs+1;before(self,frame,&capture);
        if(capture.relevant)++s->pairs;else {s->depth=depth;admitted=0;}}
    SetLastError(entry);stock(self,frame,result);DWORD exit_error=GetLastError();
    if(admitted){after(self,frame,&capture);s->depth=depth;}
    SetLastError(exit_error);
}
static void runtime_scene_actor_observer_forward(void *self,void *frame,void *result,RuntimeAmmoPickupExec stock) {
    RuntimeSceneActorObserver *s=&runtime_scene_actor_observer;
    if(!s->prepared||GetCurrentThreadId()!=session_game_thread){stock(self,frame,result);return;}
    runtime_scene_actor_observer_observe(self,frame,result,stock,s,runtime_scene_actor_observer_before,runtime_scene_actor_observer_after);
}
static void runtime_scene_actor_observer_prepare(uintptr_t base,uintptr_t engine,DWORD now) {
    DWORD saved=GetLastError();RuntimeSceneActorObserver *s=&runtime_scene_actor_observer;
    if(!runtime_control_enabled||!runtime_scope_ready||GetCurrentThreadId()!=session_game_thread)goto done;
    if(s->prepared&&s->base==base&&s->engine==engine&&guest_player_context_method_matches(base,&s->method[0],0)&&guest_player_context_method_matches(base,&s->method[1],0))goto done;
    if(s->checked&&now-s->checked<1000)goto done;s->checked=now;s->prepared=0;
    if(!supported_image(base))goto done;
    for(unsigned i=0;i<2;++i){uintptr_t fn=function_named(base,"BioPawn",runtime_scene_actor_observer_names[i]);char path[120];
        snprintf(path,sizeof path,"SFXGame.BioPawn.%s",runtime_scene_actor_observer_names[i]);
        if(!guest_player_context_method_stamp(base,fn,&s->method[i])||strcmp(s->method[i].function.full_name,path)||
           !runtime_scene_actor_observer_method(base,i,&s->method[i],1))goto done;
    }
    if(!runtime_script_executor_prepare(base))goto done;
    s->base=base;s->engine=engine;s->prepared=1;
    if(s->preparation_rows++<8){char row[400];snprintf(row,sizeof row,
        "{\"runtime_scene_actor_observer\":\"native-boundaries-prepared\",\"role\":%d,\"begin_vm_bytes\":539,\"finish_vm_bytes\":313,\"exact_relocated_bodies\":true,\"parameter_bytes\":8,\"read_only\":true,\"events_invoked\":false,\"gameplay_verified\":false,\"monotonic_ms\":%lu}\r\n",(int)g_runtime_config.role,now);line(row);}
 done:SetLastError(saved);
}
#ifdef ME3_RUNTIME_FIXTURE
static RuntimeSceneActorObserver runtime_scene_actor_fixture_state;
static unsigned runtime_scene_actor_fixture_calls,runtime_scene_actor_fixture_before_count,runtime_scene_actor_fixture_after_count;
static int runtime_scene_actor_fixture_bad,runtime_scene_actor_fixture_nested,runtime_scene_actor_fixture_refuse,runtime_scene_actor_fixture_nonparty;
static void *runtime_scene_actor_fixture_args[3];
static void runtime_scene_actor_fixture_before(void *self,void *frame,RuntimeSceneActorCapture *c) {
    if(self!=runtime_scene_actor_fixture_args[0]||frame!=runtime_scene_actor_fixture_args[1]||!c->serial||!c->depth||c->depth>4)runtime_scene_actor_fixture_bad=1;
    ++runtime_scene_actor_fixture_before_count;c->relevant=!runtime_scene_actor_fixture_nonparty;c->frame_valid=!runtime_scene_actor_fixture_refuse;SetLastError(11);
}
static void runtime_scene_actor_fixture_after(void *self,void *frame,const RuntimeSceneActorCapture *c) {
    if(self!=runtime_scene_actor_fixture_args[0]||frame!=runtime_scene_actor_fixture_args[1]||c->frame_valid==runtime_scene_actor_fixture_refuse)runtime_scene_actor_fixture_bad=1;
    ++runtime_scene_actor_fixture_after_count;SetLastError(79);
}
static void runtime_scene_actor_fixture_stock(void *self,void *frame,void *result) {
    if(self!=runtime_scene_actor_fixture_args[0]||frame!=runtime_scene_actor_fixture_args[1]||result!=runtime_scene_actor_fixture_args[2]||GetLastError()!=37)runtime_scene_actor_fixture_bad=1;
    ++runtime_scene_actor_fixture_calls;
    if(runtime_scene_actor_fixture_nested&&runtime_scene_actor_fixture_calls<6){SetLastError(37);
        runtime_scene_actor_observer_observe(self,frame,result,runtime_scene_actor_fixture_stock,&runtime_scene_actor_fixture_state,runtime_scene_actor_fixture_before,runtime_scene_actor_fixture_after);}
    SetLastError(53);
}
static int runtime_scene_actor_observer_selftest(void) {
    DWORD saved=GetLastError();int a=1,b=2,c=3,ok=1;
    runtime_scene_actor_fixture_args[0]=&a;runtime_scene_actor_fixture_args[1]=&b;runtime_scene_actor_fixture_args[2]=&c;
    for(unsigned mode=0;mode<6;++mode){memset(&runtime_scene_actor_fixture_state,0,sizeof runtime_scene_actor_fixture_state);
        runtime_scene_actor_fixture_calls=runtime_scene_actor_fixture_before_count=runtime_scene_actor_fixture_after_count=0;runtime_scene_actor_fixture_bad=0;
        runtime_scene_actor_fixture_nested=mode==1;runtime_scene_actor_fixture_refuse=mode==2;runtime_scene_actor_fixture_nonparty=mode==5;
        if(mode==3)runtime_scene_actor_fixture_state.pairs=64;if(mode==4)runtime_scene_actor_fixture_state.depth=4;
        SetLastError(37);runtime_scene_actor_observer_observe(&a,&b,&c,runtime_scene_actor_fixture_stock,&runtime_scene_actor_fixture_state,runtime_scene_actor_fixture_before,runtime_scene_actor_fixture_after);
        unsigned wanted=mode==1?4:(mode==3||mode==4)?0:1;
        ok=ok&&!runtime_scene_actor_fixture_bad&&runtime_scene_actor_fixture_calls==(mode==1?6u:1u)&&runtime_scene_actor_fixture_before_count==wanted&&runtime_scene_actor_fixture_after_count==(mode==5?0:wanted)&&
            (mode!=5||runtime_scene_actor_fixture_state.pairs==0)&&
            runtime_scene_actor_fixture_state.depth==(mode==4?4u:0u)&&GetLastError()==53&&a==1&&b==2&&c==3;
    }
    GameArray empty_header={0};RuntimeSceneActorArray empty=runtime_scene_actor_observer_array((uintptr_t)&empty_header,16);
    ok=ok&&empty.readable&&empty.valid&&empty.complete&&empty.stable&&!empty.truncated&&empty.sampled==0;
    RuntimeSceneActorArray failed=runtime_scene_actor_observer_array(0,16);ok=ok&&!failed.readable&&!failed.valid&&!failed.complete;
    uintptr_t entries[17]={0};GameArray oversized={(uintptr_t)entries,17,17};
    RuntimeSceneActorArray truncated=runtime_scene_actor_observer_array((uintptr_t)&oversized,16);
    ok=ok&&truncated.readable&&truncated.valid&&truncated.complete&&truncated.stable&&truncated.truncated&&truncated.sampled==16;
    oversized.count=-1;RuntimeSceneActorArray malformed=runtime_scene_actor_observer_array((uintptr_t)&oversized,16);
    ok=ok&&malformed.readable&&!malformed.valid&&!malformed.complete;
    runtime_scene_actor_fixture_nested=runtime_scene_actor_fixture_refuse=runtime_scene_actor_fixture_nonparty=0;SetLastError(saved);return ok;
}
#endif
#endif
