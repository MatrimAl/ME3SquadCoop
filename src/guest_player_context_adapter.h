#ifndef ME3_GUEST_PLAYER_CONTEXT_ADAPTER_H
#define ME3_GUEST_PLAYER_CONTEXT_ADAPTER_H
#include <stddef.h>

/* Opt-in experiment; game thread, pre/post Tick, including disabled contexts.
 * Include after guest_camera_adapter.h and combat_context_probe.h. enabled
 * must be the caller's PROGRESS_TEST plus private marker opt-in.
 *
 * Provenance: live-script-10292-player-bootstrap.json and private
 * player-bootstrap-design-research.md. Real BioPlayerSquad CDO has authored
 * flags2b0=0xe. Ordinary Spawn creates its TargetData; ordinary SetPlayerPawn
 * writes only current/cache pointers. BioPC.Possess script184..1ac normally
 * assigns WI.m_playerSquad. We reproduce that one transient registration on
 * the CLIENT's own actual WI, only from null, after normal actor creation.
 * No AddMember, Pawn.Squad, host, plot, permission, role or coordinate writes.
 * A successful getter is input-context evidence, never firing/damage proof.
 */

#pragma pack(push,4)
typedef struct GuestPlayerSpawnParms {
    uintptr_t klass,owner;
    uint32_t tag[2];
    float location[3];
    int32_t rotation[3];
    uintptr_t actor_template;
    uint32_t no_collision_fail,find_safe_location;
    uintptr_t result;
} GuestPlayerSpawnParms;
#pragma pack(pop)
_Static_assert(sizeof(GuestPlayerSpawnParms)==72,"Actor.Spawn ABI");
_Static_assert(offsetof(GuestPlayerSpawnParms,result)==64,"Actor.Spawn return ABI");

typedef struct GuestPlayerContextMethod {
    GuestCameraStamp function,owner;
} GuestPlayerContextMethod;
typedef struct GuestPlayerContextMethods {
    uintptr_t base;
    int ready;
    DWORD last_discovery;
    GuestCameraStamp klass,cdo;
    GuestPlayerContextMethod spawn,destroy,set_pawn;
} GuestPlayerContextMethods;
typedef struct GuestPlayerContextLease {
    int tracked,registered,pending_cleanup,suppressed,caches_set,cleanup_calls;
    unsigned long token;
    DWORD cleanup_at,monitor_at;
    GuestCameraStamp pc,pawn,info,world,level,player,squad,actor_level;
    GuestCameraStamp channel,connection,driver;
    uintptr_t pawn_squad,baseline_slot;
    int channel_index,getter_before,getter_after;
} GuestPlayerContextLease;
static GuestPlayerContextMethods guest_player_context_methods;
static GuestPlayerContextLease guest_player_context_lease;
static unsigned long guest_player_context_token;
static int guest_player_context_owned_last;
static uintptr_t guest_player_context_last_pc,guest_player_context_last_pawn,guest_player_context_last_info;
static DWORD guest_player_context_last_rejection;
static const char *guest_player_context_reason="not-evaluated";

static int guest_player_context_reject(const char *reason) {
    guest_player_context_reason=reason;return 0;
}
static void guest_player_context_log(const char *event,const char *reason,DWORD now,int verified) {
    GuestPlayerContextLease *l=&guest_player_context_lease;
    char row[2900],path[1200];identity_quote(l->squad.full_name,path,sizeof path);
    snprintf(row,sizeof row,"{\"guest_player_context_adapter\":\"%s\",\"reason\":\"%s\",\"lease_token\":%lu,\"verified\":%s,\"registered\":%s,\"pending_cleanup\":%s,\"squad_pointer\":\"0x%llx\",\"squad_path\":\"%s\",\"world_info\":\"0x%llx\",\"world_slot_baseline\":\"0x%llx\",\"controlled_pawn\":\"0x%llx\",\"pawn_squad_baseline\":\"0x%llx\",\"actor_channel_index\":%d,\"getter_before\":%s,\"getter_after\":%s,\"firing_tested\":false,\"monotonic_ms\":%lu}\r\n",
        event,reason,l->token,verified?"true":"false",l->registered?"true":"false",l->pending_cleanup?"true":"false",
        (unsigned long long)l->squad.pointer,path,(unsigned long long)l->info.pointer,(unsigned long long)l->baseline_slot,
        (unsigned long long)l->pawn.pointer,(unsigned long long)l->pawn_squad,l->channel_index,
        narrative_probe_bool(l->getter_before>=0,l->getter_before==1),narrative_probe_bool(l->getter_after>=0,l->getter_after==1),now);line(row);
}
static int guest_player_context_pins(uintptr_t base) {
    static const unsigned char spawn[]={0x40,0x55,0x53,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x57,0x48,0x8d,0x6c,0x24,0xd9};
    static const unsigned char destroy[]={0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20,0x48,0xff,0x42,0x28,0x49,0x8b};
    static const unsigned char actor_pe[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57};
    unsigned char bytes[16];
    return combat_context_layout(base)&&read_at(base+0x710d40,bytes,16)&&!memcmp(bytes,spawn,16)&&
        read_at(base+0x70ec00,bytes,16)&&!memcmp(bytes,destroy,16)&&
        read_at(base+0x40d450,bytes,16)&&!memcmp(bytes,actor_pe,16);
}
/* Class Children includes both properties and methods; permit1024 nodes.
 * Never call function_named or enumerate GObjects on this frame path. */
static uintptr_t guest_player_context_find_method(uintptr_t base,uintptr_t klass,const char *owner,const char *name) {
    uintptr_t classes[16];int depth=0;
    while(is_live(base,klass)&&depth<16) {
        for(int i=0;i<depth;++i)if(classes[i]==klass)return 0;
        classes[depth++]=klass;char text[96];object_name(base,klass,text,sizeof text);
        if(!strcmp(text,owner)) {
            uintptr_t child=read_ptr(klass+0x70),seen[1024];int count=0;
            while(child&&count<1024) {
                if(!is_live(base,child))return 0;
                for(int i=0;i<count;++i)if(seen[i]==child)return 0;
                seen[count++]=child;object_name(base,child,text,sizeof text);
                if(!strcmp(text,name)&&is_kind_of(base,child,"Function")&&read_ptr(child+0x40)==klass)return child;
                if(!read_at(child+0x68,&child,8))return 0;
            }
            return 0;
        }
        if(!read_at(klass+0x60,&klass,8))return 0;
    }
    return 0;
}
static int guest_player_context_function_abi(uintptr_t base,uintptr_t function,const char *owner,const char *name,
        uint32_t flags,uint16_t size,uint16_t result,uint16_t native_index,uintptr_t native,unsigned char num) {
    uint32_t got_flags=0;uint16_t got_size=0,got_result=0,got_index=0;unsigned char got_num=0;uintptr_t got_native=0;
    char a[96],b[96];uintptr_t declaring=read_ptr(function+0x40);
    if(!narrative_probe_object(base,function,"Function")||!narrative_probe_object(base,declaring,"Class"))return 0;
    object_name(base,function,a,sizeof a);object_name(base,declaring,b,sizeof b);
    return !strcmp(a,name)&&!strcmp(b,owner)&&read_at(function+0xe0,&got_flags,4)&&got_flags==flags&&
        read_at(function+0xe4,&got_index,2)&&got_index==native_index&&read_at(function+0xe6,&got_size,2)&&got_size==size&&
        read_at(function+0xe8,&got_result,2)&&got_result==result&&read_at(function+0xea,&got_num,1)&&got_num==num&&
        read_at(function+0xd8,&got_native,8)&&got_native==base+native;
}
static int guest_player_context_property(uintptr_t base,uintptr_t property,const char *name,int offset) {
    int32_t got=-1;char text[96];
    if(!narrative_probe_object(base,property,"ObjectProperty"))return 0;
    object_name(base,property,text,sizeof text);
    return !strcmp(text,name)&&read_at(property+0x94,&got,4)&&got==offset;
}
static int guest_player_context_set_body(uintptr_t base,uintptr_t fn) {
    GameArray script={0};unsigned char code[41];uintptr_t current=0,cached=0,input=0,other_input=0;
    if(!read_at(fn+0x7c,&script,sizeof script)||script.count!=41||script.max<41||script.max>4096||
       !read_at(script.data,code,sizeof code))return 0;
    memcpy(&current,code+2,8);memcpy(&input,code+11,8);memcpy(&cached,code+21,8);memcpy(&other_input,code+30,8);
    return code[0]==0x0f&&code[1]==0x62&&code[10]==0x5e&&code[19]==0x0f&&code[20]==0x62&&code[29]==0x5e&&
        code[38]==4&&code[39]==0x0b&&code[40]==0x53&&input==other_input&&read_ptr(input+0x40)==fn&&
        guest_player_context_property(base,input,"PlayerPawn",0)&&guest_player_context_property(base,current,"m_playerPawn",0x2cc)&&
        guest_player_context_property(base,cached,"CachedPlayerPawn",0x2a8);
}
static int guest_player_context_default(uintptr_t base,uintptr_t klass,uintptr_t cdo) {
    uint32_t cf=0,a=0,b=0,c=0,s=0;int32_t size=0;unsigned char roles[2];GameArray members={0};
    uintptr_t target=1,cached=1,current=1,initial=1,collision=1;char path[600];
    if(!narrative_probe_object(base,klass,"Class")||!is_live(base,cdo)||read_ptr(cdo+0x50)!=klass)return 0;
    identity_path(base,klass,path,sizeof path);
    return !strcmp(path,"SFXGame.BioPlayerSquad")&&read_at(klass+0x100,&cf,4)&&cf==0x08a00292&&!(cf&0x02000001)&&
        read_at(klass+0x78,&size,4)&&size==0x2e8&&read_ptr(klass+0x13c)==cdo&&
        read_at(cdo+0x25c,&a,4)&&a==0x20a22800&&read_at(cdo+0x260,&b,4)&&b==0x90000&&
        read_at(cdo+0x264,&c,4)&&c==0x800&&read_at(cdo+0x269,roles,2)&&roles[0]==1&&roles[1]==3&&
        read_at(cdo+0x270,&members,sizeof members)&&!members.data&&!members.count&&!members.max&&
        read_at(cdo+0x1e0,&collision,8)&&!collision&&read_at(cdo+0x2a0,&target,8)&&!target&&
        read_at(cdo+0x2a8,&cached,8)&&!cached&&read_at(cdo+0x2cc,&current,8)&&!current&&
        read_at(cdo+0x2d4,&initial,8)&&!initial&&read_at(cdo+0x2b0,&s,4)&&s==0xe;
}
static int guest_player_context_method_stamp(uintptr_t base,uintptr_t fn,GuestPlayerContextMethod *out) {
    return guest_camera_stamp_read(base,fn,&out->function,1)&&guest_camera_stamp_read(base,read_ptr(fn+0x40),&out->owner,1);
}
static int guest_player_context_method_matches(uintptr_t base,const GuestPlayerContextMethod *m,int full) {
    return guest_camera_stamp_matches(base,&m->function,full)&&guest_camera_stamp_matches(base,&m->owner,full)&&
        read_ptr(m->function.pointer+0x40)==m->owner.pointer;
}
static int guest_player_context_methods_valid(uintptr_t base,int full) {
    GuestPlayerContextMethods *m=&guest_player_context_methods;
    return m->ready&&m->base==base&&guest_camera_stamp_matches(base,&m->klass,full)&&guest_camera_stamp_matches(base,&m->cdo,full)&&
        guest_player_context_default(base,m->klass.pointer,m->cdo.pointer)&&
        guest_player_context_method_matches(base,&m->spawn,full)&&guest_player_context_method_matches(base,&m->destroy,full)&&
        guest_player_context_method_matches(base,&m->set_pawn,full)&&
        guest_player_context_function_abi(base,m->spawn.function.pointer,"Actor","Spawn",0x24401,72,64,0,0x710d40,9)&&
        guest_player_context_function_abi(base,m->destroy.function.pointer,"Actor","Destroy",0x20401,4,0,279,0x70ec00,1)&&
        guest_player_context_function_abi(base,m->set_pawn.function.pointer,"BioPlayerSquad","SetPlayerPawn",0x20802,8,65535,0,0xf52b0,1)&&
        guest_player_context_set_body(base,m->set_pawn.function.pointer);
}
static int guest_player_context_resolve(uintptr_t base,uintptr_t pc,DWORD now) {
    GuestPlayerContextMethods *m=&guest_player_context_methods;
    if(guest_player_context_methods_valid(base,1))return 1;
    if(m->last_discovery&&now-m->last_discovery<1000)return 0;
    m->last_discovery=now;m->ready=0;
    uintptr_t pc_class=read_ptr(pc+0x50),possess=guest_player_context_find_method(base,pc_class,"BioPlayerController","Possess");
    if(!guest_player_context_function_abi(base,possess,"BioPlayerController","Possess",0x20002,12,65535,0,0xf52b0,2))return 0;
    GameArray script={0};unsigned char code[520];uintptr_t klass=0;
    if(!read_at(possess+0x7c,&script,sizeof script)||script.count!=520||script.max<520||script.max>4096||
       !read_at(script.data,code,sizeof code)||memcmp(code+0xd8,"\x07\x05\x02\x72\x81\x2e",6)||code[0x1d7]!=0x1c||code[0x1e0]!=0x2e)return 0;
    memcpy(&klass,code+0x1e1,8);uintptr_t cdo=read_ptr(klass+0x13c);
    if(!guest_player_context_default(base,klass,cdo))return 0;
    uintptr_t spawn=guest_player_context_find_method(base,pc_class,"Actor","Spawn"),destroy=guest_player_context_find_method(base,klass,"Actor","Destroy");
    uintptr_t set_pawn=guest_player_context_find_method(base,klass,"BioPlayerSquad","SetPlayerPawn");
    if(!guest_camera_stamp_read(base,klass,&m->klass,1)||!guest_camera_stamp_read(base,cdo,&m->cdo,1)||
       !guest_player_context_method_stamp(base,spawn,&m->spawn)||!guest_player_context_method_stamp(base,destroy,&m->destroy)||
       !guest_player_context_method_stamp(base,set_pawn,&m->set_pawn))return 0;
    m->base=base;m->ready=1;
    if(!guest_player_context_methods_valid(base,1)){m->ready=0;return 0;}
    char row[1600];
    snprintf(row,sizeof row,"{\"guest_player_context_adapter\":\"methods-validated\",\"spawn_declaring_class\":\"Engine.Actor\",\"spawn_object_index\":%d,\"spawn_name_index\":%u,\"spawn_native_rva\":7408960,\"spawn_flags\":148481,\"spawn_parms_size\":72,\"spawn_return_offset\":64,\"destroy_declaring_class\":\"Engine.Actor\",\"destroy_object_index\":%d,\"destroy_name_index\":%u,\"destroy_native_rva\":7400448,\"destroy_flags\":132097,\"destroy_parms_size\":4,\"set_declaring_class\":\"SFXGame.BioPlayerSquad\",\"set_object_index\":%d,\"set_name_index\":%u,\"class_pointer\":\"0x%llx\",\"cdo_pointer\":\"0x%llx\",\"authored_squad_flags\":14,\"monotonic_ms\":%lu}\r\n",
        m->spawn.function.index,m->spawn.function.name[0],m->destroy.function.index,m->destroy.function.name[0],
        m->set_pawn.function.index,m->set_pawn.function.name[0],(unsigned long long)klass,(unsigned long long)cdo,now);line(row);
    return 1;
}
static int guest_player_context_dispatch(uintptr_t base,uintptr_t actor,const GuestPlayerContextMethod *method,void *parms) {
    uint64_t flags=0;
    if(!guest_camera_actor_live(base,actor)||!read_at(actor+0xc,&flags,8)||(flags&(UINT64_C(1)<<61))||
       !guest_player_context_methods_valid(base,1)||!guest_player_context_method_matches(base,method,1)||
       !guest_player_context_pins(base))return 0;
    uintptr_t pe=read_ptr(read_ptr(actor)+0x230);
    if(pe!=base+0x40d450)return 0;
    ((ProcessEvent)pe)((void*)actor,(void*)method->function.pointer,parms,NULL);return 1;
}
static int guest_player_context_channel(uintptr_t base,uintptr_t pawn,NetworkActorIdentityEntry *out) {
    memset(out,0,sizeof *out);
    if(!network_actor_layout_matches(base))return 0;
    for(unsigned int i=0;i<4;++i) {
        NetworkActorIdentityEntry *cached=&network_actor_identity_cache[i],current={0};
        if(cached->base!=base||cached->pawn!=pawn||cached->index<=0)continue;
        if(!network_actor_channel_valid(base,pawn,cached->channel,cached->driver,&current)||current.index!=cached->index||current.connection!=cached->connection)continue;
        if(out->channel&&out->channel!=current.channel)return 0;*out=current;
    }
    return out->channel&&out->index>0;
}
static int guest_player_context_basic_owned(uintptr_t base,uintptr_t pc,uintptr_t pawn,uintptr_t info) {
    unsigned char net=255;uintptr_t player=read_ptr(pc+0x67c);
    return guest_camera_actor_live(base,pc)&&is_kind_of(base,pc,"SFXPlayerController")&&guest_camera_actor_live(base,pawn)&&
        is_kind_of(base,pawn,"SFXPawn_Ashley")&&guest_camera_actor_live(base,info)&&is_kind_of(base,info,"BioWorldInfo")&&
        read_ptr(pc+0x198)==info&&read_ptr(pawn+0x198)==info&&read_at(info+0x7c8,&net,1)&&net==3&&
        read_ptr(pc+0x3bc)==pawn&&read_ptr(pawn+0x374)==pc&&read_ptr(pc+0x694)==pawn&&
        narrative_probe_object(base,player,"LocalPlayer")&&read_ptr(player+0x68)==pc;
}
static int guest_player_context_conditions(uintptr_t base,uintptr_t pc,uintptr_t pawn,uintptr_t info,
        int ordinary_mode,int enabled,int acquiring,NetworkActorIdentityEntry *channel,CombatContextChain *chain) {
    guest_player_context_reason="eligible";
    if(!enabled)return guest_player_context_reject("disabled");
    if(!ordinary_mode)return guest_player_context_reject("scene-gate");
    if(!guest_player_context_basic_owned(base,pc,pawn,info))return guest_player_context_reject("local-ownership-ack-world");
    unsigned char ignore=255,current_mode=255;uint32_t pc_flags=0,pawn_flags=0,busy=0,disable=0,permissions=0;int32_t action=-1,power=-1;
    if(!read_at(pc+0x77c,&pc_flags,4)||(pc_flags&(0x800|0x20000)))return guest_player_context_reject("controller-cinematic");
    if(acquiring&&(!read_at(pc+0x781,&ignore,1)||ignore))return guest_player_context_reject("controller-move-input");
    if(acquiring&&(!read_at(pawn+0xc20,&action,4)||action||!read_at(pawn+0xc28,&power,4)||power))return guest_player_context_reject("pawn-action-or-power");
    if(!read_at(pawn+0xe18,&pawn_flags,4)||!(pawn_flags&8)||(acquiring&&(pawn_flags&(0x200000|0x20000000)))||
       !read_at(pawn+0xe20,&busy,4)||(busy&0x40000))return guest_player_context_reject("pawn-combat-or-busy-state");
    uintptr_t manager=read_ptr(pc+0xc38);GameArray modes={0};
    if(!narrative_probe_object(base,manager,"SFXGameModeManager")||!read_at(manager+0xa8,&current_mode,1)||current_mode!=0||
       !read_at(manager+0xa4,&disable,4)||(acquiring&&(disable&1))||!read_at(manager+0x70,&modes,sizeof modes)||!modes.data||
       modes.count<1||modes.count>64||modes.max<modes.count||modes.max>128)return guest_player_context_reject("mode-manager-state");
    uintptr_t mode=read_ptr(modes.data);
    if(!narrative_probe_object(base,mode,"SFXGameModeDefault")||!read_at(mode+0xd0,&permissions,4)||!(permissions&2)||
       (acquiring&&((permissions&0x800)||!(permissions&0x400))))return guest_player_context_reject("mode-input-permission");
    if(!guest_player_context_channel(base,pawn,channel))return guest_player_context_reject("positive-channel-not-ready");
    combat_context_read_chain(base,chain);
    if(!chain->info_valid||!chain->squad_read||chain->info!=info||!combat_context_safe_native(chain))return guest_player_context_reject("native-world-chain");
    return 1;
}
/* GWorld owns both native Spawn and native Destroy. Never destroy an old-world
 * actor through a different current GWorld, even if its memory is still live. */
static int guest_player_context_current_world(uintptr_t base,int full) {
    GuestPlayerContextLease *l=&guest_player_context_lease;CombatContextChain chain;
    if(!guest_camera_stamp_matches(base,&l->world,full)||!guest_camera_stamp_matches(base,&l->level,full)||
       !guest_camera_stamp_matches(base,&l->info,full)||!guest_camera_actor_live(base,l->info.pointer))return 0;
    combat_context_read_chain(base,&chain);
    return chain.world_valid&&chain.level_valid&&chain.info_valid&&chain.squad_read&&
        chain.world==l->world.pointer&&chain.level==l->level.pointer&&chain.info==l->info.pointer;
}
static int guest_player_context_squad_valid(uintptr_t base,int full) {
    GuestPlayerContextLease *l=&guest_player_context_lease;
    return l->squad.pointer&&guest_camera_stamp_matches(base,&l->squad,full)&&guest_camera_actor_live(base,l->squad.pointer)&&
        read_ptr(l->squad.pointer+0x50)==guest_player_context_methods.klass.pointer&&
        read_ptr(l->squad.pointer+0x198)==l->info.pointer&&read_ptr(l->squad.pointer+0x180)==l->pc.pointer&&
        guest_camera_stamp_matches(base,&l->actor_level,full)&&read_ptr(l->squad.pointer+0x40)==l->actor_level.pointer;
}
static int guest_player_context_squad_shape(uintptr_t base,uintptr_t expected_pawn) {
    GuestPlayerContextLease *l=&guest_player_context_lease;uintptr_t p=l->squad.pointer,target=0,initial=1,current=1,cached=1;
    GameArray members={0};uint32_t flags=0,a=0,b=0,c=0;unsigned char roles[2];
    return guest_player_context_squad_valid(base,0)&&read_at(p+0x2b0,&flags,4)&&flags==0xe&&
        read_at(p+0x270,&members,sizeof members)&&!members.count&&!members.data&&!members.max&&
        read_at(p+0x2a8,&cached,8)&&cached==expected_pawn&&read_at(p+0x2cc,&current,8)&&current==expected_pawn&&
        read_at(p+0x2d4,&initial,8)&&!initial&&read_at(p+0x2a0,&target,8)&&narrative_probe_object(base,target,"BioSquadTargetData")&&
        read_at(p+0x25c,&a,4)&&!(a&(1|4|8))&&read_at(p+0x260,&b,4)&&!(b&(0x800000|0x60000000))&&
        read_at(p+0x264,&c,4)&&!(c&1)&&read_at(p+0x269,roles,2)&&roles[0]==1&&roles[1]==3;
}
/* After an ordinary script no-op or partial dispatch, either of our own two
 * caches can still be null. Only those exact values may be cleared; an engine
 * replacement pawn/cache is preserved. */
static int guest_player_context_cleanup_shape(uintptr_t base) {
    GuestPlayerContextLease *l=&guest_player_context_lease;uintptr_t current=0,cached=0;
    if(!l->caches_set)return guest_player_context_squad_shape(base,0);
    if(!read_at(l->squad.pointer+0x2cc,&current,8)||!read_at(l->squad.pointer+0x2a8,&cached,8)||
       (current&&current!=l->pawn.pointer)||(cached&&cached!=l->pawn.pointer))return 0;
    if(current==cached)return guest_player_context_squad_shape(base,current);
    /* All noncache fields are verified by a copy of the shape expression,
     * without writing either cache merely to run a validator. */
    uintptr_t p=l->squad.pointer,target=0,initial=1;GameArray members={0};uint32_t flags=0,a=0,b=0,c=0;unsigned char roles[2];
    return guest_player_context_squad_valid(base,0)&&read_at(p+0x2b0,&flags,4)&&flags==0xe&&
        read_at(p+0x270,&members,sizeof members)&&!members.count&&!members.data&&!members.max&&
        read_at(p+0x2d4,&initial,8)&&!initial&&read_at(p+0x2a0,&target,8)&&narrative_probe_object(base,target,"BioSquadTargetData")&&
        read_at(p+0x25c,&a,4)&&!(a&(1|4|8))&&read_at(p+0x260,&b,4)&&!(b&(0x800000|0x60000000))&&
        read_at(p+0x264,&c,4)&&!(c&1)&&read_at(p+0x269,roles,2)&&roles[0]==1&&roles[1]==3;
}
static int guest_player_context_no_actor_channel(uintptr_t base,uintptr_t actor,uintptr_t driver,uintptr_t connection) {
    GameArray open={0};uintptr_t channels[2048];
    if(!network_actor_connection_in_scope(base,driver,connection)||!read_at(connection+0x5f90,&open,sizeof open)||
       open.count<0||open.count>2048||open.max<open.count||open.max>4096||(open.count&&(!open.data||!read_at(open.data,channels,(SIZE_T)open.count*8))))return 0;
    for(int i=0;i<open.count;++i) {
        if(!narrative_probe_object(base,channels[i],"Channel"))return 0;
        if(is_kind_of(base,channels[i],"ActorChannel")) {
            uintptr_t target=0;if(!read_at(channels[i]+0xa0,&target,8)||target==actor)return 0;
        }
    }
    return 1;
}
static int guest_player_context_field_writable(uintptr_t at) {
    MEMORY_BASIC_INFORMATION region={0};
    if((at&7)||!VirtualQuery((void*)at,&region,sizeof region)||region.State!=MEM_COMMIT||(region.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return 0;
    DWORD p=region.Protect&0xff;
    return (p==PAGE_READWRITE||p==PAGE_WRITECOPY||p==PAGE_EXECUTE_READWRITE||p==PAGE_EXECUTE_WRITECOPY)&&
        at>=(uintptr_t)region.BaseAddress&&at+8>at&&at+8<=(uintptr_t)region.BaseAddress+region.RegionSize;
}
/* Restoring a WI slot does not write/dereference its pointee. A disposed or
 * unmapped old actor can be unlinked. If readable identity says the address
 * belongs to a different object now, preserve that external replacement. */
static int guest_player_context_pointee_not_reused(uintptr_t base) {
    GuestPlayerContextLease *l=&guest_player_context_lease;GuestCameraStamp current;
    if(guest_camera_stamp_read(base,l->squad.pointer,&current,1))
        return guest_camera_stamp_core_equal(&l->squad,&current)&&!strcmp(l->squad.full_name,current.full_name);
    int32_t index=0;uint32_t name[2];uintptr_t klass=0,outer=0;
    if(read_at(l->squad.pointer+8,&index,4)&&index!=l->squad.index)return 0;
    if(read_at(l->squad.pointer+0x48,name,8)&&memcmp(name,l->squad.name,8))return 0;
    if(read_at(l->squad.pointer+0x50,&klass,8)&&klass!=l->squad.klass)return 0;
    if(read_at(l->squad.pointer+0x40,&outer,8)&&l->actor_level.pointer&&outer!=l->actor_level.pointer)return 0;
    return 1;
}
/* All callers have just revalidated captured full identities on this same
 * game thread. CAS preserves an engine replacement; no protection changes. */
static int guest_player_context_compare_slot(uintptr_t base,uintptr_t expected,uintptr_t replacement,uintptr_t *actual,int restoring) {
    GuestPlayerContextLease *l=&guest_player_context_lease;uintptr_t at=l->info.pointer+0x8e8;
    if(!guest_player_context_current_world(base,1)||!guest_player_context_field_writable(at))return 0;
    if(restoring) {if(expected!=l->squad.pointer||replacement!=l->baseline_slot||!guest_player_context_pointee_not_reused(base))return 0;}
    else if(!guest_camera_stamp_matches(base,&l->pc,1)||!guest_camera_actor_live(base,l->pc.pointer)||
            !guest_camera_stamp_matches(base,&l->pawn,1)||!guest_player_context_squad_valid(base,1))return 0;
    *actual=(uintptr_t)InterlockedCompareExchangePointer((PVOID volatile*)at,(PVOID)replacement,(PVOID)expected);
    uintptr_t after=0;return *actual==expected&&read_at(at,&after,8)&&after==replacement;
}
static int guest_player_context_gate(uintptr_t base,const char *phase,DWORD now) {
    GuestPlayerContextLease *l=&guest_player_context_lease;CombatContextChain before,after;uint32_t value=UINT32_C(0x7fcdeeed);
    combat_context_read_chain(base,&before);int pinned=guest_player_context_pins(base);
    uintptr_t input=guest_camera_stamp_matches(base,&l->pc,0)&&guest_camera_actor_live(base,l->pc.pointer)?read_ptr(l->pc.pointer+0x6dc):0;
    int dispatched=pinned&&before.info==l->info.pointer&&combat_context_safe_native(&before)&&combat_context_get(base,input,&value);
    combat_context_read_chain(base,&after);int stable=combat_context_chain_equal(&before,&after);
    int valid=dispatched&&value<=1&&stable,derived=combat_context_derived(&before,pinned,stable);
    char row[1500];
    snprintf(row,sizeof row,"{\"guest_player_context_adapter\":\"gate-observation\",\"phase\":\"%s\",\"lease_token\":%lu,\"world_slot\":\"0x%llx\",\"cached_pawn\":\"0x%llx\",\"authored_squad_flags\":14,\"squad_flags_valid\":%s,\"squad_flags_raw\":%u,\"flags_unchanged\":%s,\"derived_gate\":%s,\"getter_dispatched\":%s,\"getter_result\":%s,\"getter_derived_agree\":%s,\"firing_tested\":false,\"monotonic_ms\":%lu}\r\n",
        phase,l->token,(unsigned long long)before.squad,(unsigned long long)before.cached,
        before.squad_flags_read?"true":"false",before.squad_flags,narrative_probe_bool(before.squad_flags_read,before.squad_flags==0xe),
        narrative_probe_bool(derived>=0,derived==1),dispatched?"true":"false",narrative_probe_bool(valid,value==1),
        narrative_probe_bool(valid&&derived>=0,(int)value==derived),now);line(row);
    return valid&&derived>=0&&(int)value==derived?(int)value:-1;
}
/* Ordinary Actor.Destroy's intrinsic index279 is intentionally not callable
 * through UObject.ProcessEvent: f5008..f5017 requires iNative==0 after the
 * object's normal virtual88 check. Do not alter that metadata or its guards.
 * Instead call the SAME public engine implementation that Actor.Destroy's
 * native wrapper70ec00 calls at70ec44: (GWorld,this,0,1), uint32 EAX result.
 * Native DestroyActor retains its static/noDelete/role/disposal checks and all
 * ordinary destruction callbacks. This helper is only for our captured empty
 * client-local context actor, after its WI slot and cache references clear.
 */
typedef uint32_t (*GuestPlayerContextDestroyActor)(void*,void*,uint32_t,uint32_t);
static int guest_player_context_destroy_native_pins(uintptr_t base) {
    static const unsigned char wrapper[]={0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20,0x48,0xff,0x42,0x28,0x49,0x8b,0xd8,0x48,0x8b,0x42,0x28,0x48,0x8b,0xf9,0x80,0x38,0x41,0x75,0x14,0x48,0x8b,0x4a,0x20,0x48,0xff,0xc0,0x45,0x33,0xc0,0x48,0x89,0x42,0x28,0xff,0x15,0x87,0x61,0x0e,0x01,0x48,0x8b,0x0d,0x00,0x7a,0x1a,0x01,0x41,0xb9,0x01,0x00,0x00,0x00,0x45,0x33,0xc0,0x48,0x8b,0xd7,0xe8,0xc7,0x11,0xe7,0xff,0x89,0x03,0x48,0x8b,0x5c,0x24,0x30,0x48,0x83,0xc4,0x20,0x5f,0xc3};
    static const unsigned char native[]={0x48,0x8b,0xc4,0x44,0x89,0x48,0x20,0x48,0x89,0x48,0x08,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x8d,0x68,0xa1,0x48,0x81,0xec,0xa0,0x00,0x00};
    static const unsigned char native_guards[]={0x8b,0x87,0x5c,0x02,0x00,0x00,0xa8,0x05,0x0f,0x85,0xb8,0x00,0x00,0x00,0xa8,0x08,0x0f,0x85,0xb7,0x04,0x00,0x00,0x80,0xbf,0x6a,0x02,0x00,0x00,0x03,0x74,0x0e,0x85,0xdb,0x75,0x0a,0x0f,0xba,0xe0,0x12,0x0f,0x83,0x99,0x00,0x00,0x00};
    static const unsigned char indexed_pe[]={0x85,0xc0,0x0f,0x85,0x63,0x02,0x00,0x00,0x66,0x39,0x86,0xe4,0x00,0x00,0x00,0x0f,0x85,0x56,0x02,0x00,0x00};
    unsigned char bytes[sizeof wrapper];
    return read_at(base+0x70ec00,bytes,sizeof wrapper)&&!memcmp(bytes,wrapper,sizeof wrapper)&&
        read_at(base+0x57fe10,bytes,sizeof native)&&!memcmp(bytes,native,sizeof native)&&
        read_at(base+0x57fe57,bytes,sizeof native_guards)&&!memcmp(bytes,native_guards,sizeof native_guards)&&
        read_at(base+0xf5008,bytes,sizeof indexed_pe)&&!memcmp(bytes,indexed_pe,sizeof indexed_pe);
}
static int guest_player_context_destroy_owned_native(uintptr_t base,uint32_t *result) {
    GuestPlayerContextLease *l=&guest_player_context_lease;uint64_t object_flags=0;uintptr_t slot=0,current_world=0;
    if(l->registered||l->caches_set||!guest_player_context_pins(base)||!guest_player_context_destroy_native_pins(base)||
       !guest_player_context_methods_valid(base,1)||!guest_player_context_current_world(base,1)||
       !guest_camera_stamp_matches(base,&l->pc,1)||!guest_camera_actor_live(base,l->pc.pointer)||
       !guest_player_context_squad_valid(base,1)||!guest_player_context_squad_shape(base,0)||
       !read_at(l->squad.pointer+0xc,&object_flags,8)||(object_flags&(UINT64_C(1)<<61))||
       !read_at(l->info.pointer+0x8e8,&slot,8)||slot!=l->baseline_slot||
       !read_at(base+0x18b6638,&current_world,8)||current_world!=l->world.pointer)return 0;
    /* The object's full class/world/owner identities, authored deletable
     * flags, Role3, empty Members and null two caches were just revalidated.
     * GWorld must still match the wrapper's source immediately before call. */
    *result=((GuestPlayerContextDestroyActor)(base+0x57fe10))((void*)current_world,(void*)l->squad.pointer,0,1);
    return 1;
}
static int guest_player_context_cleanup(uintptr_t base,const char *reason,DWORD now) {
    GuestPlayerContextLease *l=&guest_player_context_lease;
    if(!l->squad.pointer){l->registered=0;l->pending_cleanup=0;return 1;}
    l->pending_cleanup=1;l->suppressed=1;
    if(l->cleanup_at&&now-l->cleanup_at<1000)return 0;l->cleanup_at=now;
    int actor_identity=guest_camera_stamp_matches(base,&l->squad,0),actor_live=actor_identity&&guest_camera_actor_live(base,l->squad.pointer);
    if(!guest_player_context_current_world(base,1)) {
        if(!actor_live) {
            guest_player_context_log("cleanup-disposed","captured-world-unavailable-no-old-world-write",now,0);
            l->squad.pointer=0;l->registered=0;l->pending_cleanup=0;return 1;
        }
        guest_player_context_log("cleanup-pending","captured-context-unavailable",now,0);return 0;
    }
    uintptr_t slot=0;
    if(!read_at(l->info.pointer+0x8e8,&slot,8)){guest_player_context_log("cleanup-pending","world-slot-unreadable",now,0);return 0;}
    if(slot==l->squad.pointer) {
        uintptr_t previous=0;
        if(!guest_player_context_pointee_not_reused(base)) {
            guest_player_context_log("world-slot-preserved","pointee-address-reused-or-renamed",now,1);
            l->squad.pointer=0;l->registered=0;l->pending_cleanup=0;return 1;
        }
        if(!guest_player_context_compare_slot(base,l->squad.pointer,l->baseline_slot,&previous,1)) {
            guest_player_context_log("cleanup-pending","world-slot-compare-or-restore-failed",now,0);return 0;
        }
        l->registered=0;guest_player_context_log("world-slot-restored",reason,now,1);
    } else {
        l->registered=0;
        if(slot!=l->baseline_slot)guest_player_context_log("world-slot-preserved","engine-replaced-world-slot",now,1);
    }
    if(!actor_live) {
        guest_player_context_log("cleanup-disposed",reason,now,1);l->squad.pointer=0;l->pending_cleanup=0;
        guest_player_context_gate(base,"after-cleanup",now);return 1;
    }
    if(!guest_camera_stamp_matches(base,&l->pc,1)||!guest_camera_actor_live(base,l->pc.pointer)||
       !guest_player_context_squad_valid(base,1)||!guest_player_context_methods_valid(base,1)) {
        guest_player_context_log("cleanup-pending","engine-call-context-unavailable-after-unregister",now,0);return 0;
    }
    /* A genuine engine adoption/change wins. Do not erase its membership or
     * caches, and do not keep spawning replacements while cleanup is pending. */
    if(!guest_player_context_cleanup_shape(base)) {
        guest_player_context_log("cleanup-pending","owned-actor-externally-changed",now,0);return 0;
    }
    if(l->cleanup_calls>=8){guest_player_context_log("cleanup-pending","bounded-engine-call-retries-exhausted",now,0);return 0;}
    ++l->cleanup_calls;
    if(l->caches_set) {
        uintptr_t none=0;
        if(!guest_player_context_dispatch(base,l->squad.pointer,&guest_player_context_methods.set_pawn,&none)||
           !guest_player_context_squad_shape(base,0)) {
            guest_player_context_log("cleanup-pending","clear-cache-not-verified",now,0);return 0;
        }
        l->caches_set=0;
    }
    uint32_t result=UINT32_C(0x7fcdeeed);
    int dispatched=guest_player_context_destroy_owned_native(base,&result);
    int identity_valid=guest_camera_stamp_matches(base,&l->squad,0);uint32_t actor_flags=0,actor_flags2=0;
    int deletion_read=identity_valid&&read_at(l->squad.pointer+0x25c,&actor_flags,4)&&read_at(l->squad.pointer+0x260,&actor_flags2,4);
    int disposed=!identity_valid||(deletion_read&&((actor_flags&8)||(actor_flags2&0x800000)));
    char row[1100];
    snprintf(row,sizeof row,"{\"guest_player_context_adapter\":\"destroy-observation\",\"destroy_path\":\"ordinary-UWorld-DestroyActor\",\"destroy_native_rva\":5766672,\"net_force\":false,\"modify_level\":true,\"lease_token\":%lu,\"squad_pointer\":\"0x%llx\",\"destroy_dispatched\":%s,\"destroy_return\":%s,\"identity_still_valid\":%s,\"deletion_flags_read_valid\":%s,\"delete_me\":%s,\"pending_delete\":%s,\"disposed_observed\":%s,\"monotonic_ms\":%lu}\r\n",
        l->token,(unsigned long long)l->squad.pointer,dispatched?"true":"false",narrative_probe_bool(dispatched&&result<=1,result==1),
        identity_valid?"true":"false",deletion_read?"true":"false",narrative_probe_bool(deletion_read,actor_flags&8),
        narrative_probe_bool(deletion_read,actor_flags2&0x800000),disposed?"true":"false",now);line(row);
    if(dispatched&&result==1&&disposed) {
        guest_player_context_log("cleanup-success",reason,now,1);l->squad.pointer=0;l->pending_cleanup=0;
        guest_player_context_gate(base,"after-cleanup",now);return 1;
    }
    guest_player_context_log("cleanup-pending",dispatched?"ordinary-destroy-not-verified":"ordinary-destroy-rejected",now,0);return 0;
}
static int guest_player_context_capture(uintptr_t base,uintptr_t pc,uintptr_t pawn,uintptr_t info,
        const NetworkActorIdentityEntry *channel,const CombatContextChain *chain) {
    GuestPlayerContextLease *l=&guest_player_context_lease;GuestPlayerContextLease next={0};
    next.token=guest_player_context_token;next.getter_before=next.getter_after=-1;next.tracked=1;
    next.channel_index=channel->index;
    if(!guest_camera_stamp_read(base,pc,&next.pc,1)||!guest_camera_stamp_read(base,pawn,&next.pawn,1)||
       !guest_camera_stamp_read(base,info,&next.info,1)||!guest_camera_stamp_read(base,chain->world,&next.world,1)||
       !guest_camera_stamp_read(base,chain->level,&next.level,1)||!guest_camera_stamp_read(base,read_ptr(pc+0x67c),&next.player,1)||
       !guest_camera_stamp_read(base,channel->channel,&next.channel,1)||!guest_camera_stamp_read(base,channel->connection,&next.connection,1)||
       !guest_camera_stamp_read(base,channel->driver,&next.driver,1)||!read_at(pawn+0xba8,&next.pawn_squad,8)||
       !read_at(info+0x8e8,&next.baseline_slot,8)||next.baseline_slot)return 0;
    *l=next;return 1;
}
static int guest_player_context_same(uintptr_t base,uintptr_t pc,uintptr_t pawn,uintptr_t info,const NetworkActorIdentityEntry *channel) {
    GuestPlayerContextLease *l=&guest_player_context_lease;
    return l->token==guest_player_context_token&&l->pc.pointer==pc&&l->pawn.pointer==pawn&&l->info.pointer==info&&
        channel->channel==l->channel.pointer&&channel->connection==l->connection.pointer&&channel->driver==l->driver.pointer&&
        channel->index==l->channel_index&&guest_camera_stamp_matches(base,&l->pc,0)&&guest_camera_stamp_matches(base,&l->pawn,0)&&
        guest_camera_stamp_matches(base,&l->player,0)&&guest_camera_stamp_matches(base,&l->channel,0)&&
        guest_camera_stamp_matches(base,&l->connection,0)&&guest_camera_stamp_matches(base,&l->driver,0)&&
        guest_player_context_current_world(base,0);
}
static void guest_player_context_adapt(uintptr_t base,uintptr_t pc,uintptr_t pawn,uintptr_t info,
        int ordinary_mode,int enabled,DWORD now) {
    GuestPlayerContextLease *l=&guest_player_context_lease;
    int owned=guest_player_context_basic_owned(base,pc,pawn,info);
    int edge=owned&&(!guest_player_context_owned_last||pc!=guest_player_context_last_pc||pawn!=guest_player_context_last_pawn||info!=guest_player_context_last_info);
    if(edge)++guest_player_context_token;
    guest_player_context_owned_last=owned;guest_player_context_last_pc=pc;guest_player_context_last_pawn=pawn;guest_player_context_last_info=info;
    NetworkActorIdentityEntry channel={0};CombatContextChain chain;
    int eligible=guest_player_context_conditions(base,pc,pawn,info,ordinary_mode,enabled,!l->registered,&channel,&chain);
    if(!eligible&&owned&&(!guest_player_context_last_rejection||now-guest_player_context_last_rejection>=5000)) {
        guest_player_context_last_rejection=now;guest_player_context_log("eligibility",guest_player_context_reason,now,0);
    }
    if(l->pending_cleanup) {guest_player_context_cleanup(base,"pending-cleanup",now);return;}
    if(l->tracked) {
        int same=eligible&&guest_player_context_same(base,pc,pawn,info,&channel);
        if(!same) {
            guest_player_context_cleanup(base,!enabled?"disabled":!ordinary_mode?"scene":"ownership-or-context-change",now);
            if(!l->pending_cleanup&&l->token!=guest_player_context_token)memset(l,0,sizeof *l);
            return;
        }
        if(l->registered) {
            uintptr_t slot=0,own=0;
            if(!guest_player_context_squad_shape(base,pawn)||!read_at(info+0x8e8,&slot,8)||slot!=l->squad.pointer||
               !read_at(pawn+0xba8,&own,8)||own!=l->pawn_squad) {
                guest_player_context_cleanup(base,"monitored-context-divergence",now);return;
            }
            if(!l->monitor_at||now-l->monitor_at>=5000){l->monitor_at=now;guest_player_context_gate(base,"held",now);}
        }
        return;
    }
    if(!eligible)return;
    if(chain.squad){guest_player_context_reason="existing-world-context-preserved";return;}
    if(!guest_player_context_pins(base)||!guest_player_context_resolve(base,pc,now)) {
        if(!guest_player_context_last_rejection||now-guest_player_context_last_rejection>=5000) {
            guest_player_context_last_rejection=now;guest_player_context_log("eligibility","class-function-or-native-pin-not-ready",now,0);
        }
        return;
    }
    if(!guest_player_context_capture(base,pc,pawn,info,&channel,&chain))return;
    l->getter_before=guest_player_context_gate(base,"before-apply",now);
    /* One spawn attempt per observed ownership lease; all later failures
     * retain this tracked/suppressed record, never a perframe respawn loop. */
    l->suppressed=1;
    GuestPlayerSpawnParms parms={0};parms.klass=guest_player_context_methods.klass.pointer;parms.owner=pc;
    parms.result=UINTPTR_MAX;
    if(!read_at(pawn+0x108,parms.location,12)||!read_at(pawn+0x114,parms.rotation,12)||
       !isfinite(parms.location[0])||!isfinite(parms.location[1])||!isfinite(parms.location[2])||
       !guest_player_context_conditions(base,pc,pawn,info,ordinary_mode,enabled,1,&channel,&chain)||chain.squad||
       !guest_player_context_same(base,pc,pawn,info,&channel)||!guest_camera_stamp_matches(base,&l->pc,1)||
       !guest_camera_stamp_matches(base,&l->pawn,1)||!guest_player_context_dispatch(base,pc,&guest_player_context_methods.spawn,&parms)) {
        guest_player_context_log("spawn-rejected","pre-dispatch-guard",now,0);return;
    }
    if(!parms.result||parms.result==UINTPTR_MAX) {guest_player_context_log("spawn-rejected","ordinary-spawn-returned-none",now,0);return;}
    /* Capture the returned real actor before any mutation. A returned pointer
     * of an unexpected class is never destroyed through an assumed type. */
    if(!guest_camera_actor_live(base,parms.result)||read_ptr(parms.result+0x50)!=parms.klass||parms.result==guest_player_context_methods.cdo.pointer||
       !guest_camera_stamp_read(base,parms.result,&l->squad,1)) {
        guest_player_context_log("spawn-rejected","returned-actor-identity-unverified",now,0);return;
    }
    l->pending_cleanup=1;
    if(read_ptr(parms.result+0x198)!=info||read_ptr(parms.result+0x180)!=pc||
       !guest_camera_stamp_read(base,read_ptr(parms.result+0x40),&l->actor_level,1)||!is_kind_of(base,l->actor_level.pointer,"Level")) {
        guest_player_context_log("cleanup-pending","returned-actor-context-unverified",now,0);return;
    }
    if(!guest_player_context_same(base,pc,pawn,info,&channel)||!guest_player_context_squad_shape(base,0)||
       !guest_player_context_no_actor_channel(base,l->squad.pointer,channel.driver,channel.connection)) {
        guest_player_context_log("apply-rejected","spawn-lifecycle-or-channel-validation",now,0);guest_player_context_cleanup(base,"spawn-validation",now);return;
    }
    uintptr_t target=pawn;
    if(!guest_camera_stamp_matches(base,&l->pawn,1)||!guest_player_context_dispatch(base,l->squad.pointer,&guest_player_context_methods.set_pawn,&target)) {
        guest_player_context_log("apply-rejected","ordinary-set-player-pawn-rejected",now,0);guest_player_context_cleanup(base,"set-player-pawn-rejected",now);return;
    }
    /* The script has exactly two writes. Mark ownership of their target now,
     * even if verification fails, so partial/no-op outcomes remain explicit. */
    l->caches_set=1;
    NetworkActorIdentityEntry checked_channel={0};CombatContextChain checked_chain;uintptr_t own=0,previous=0;
    if(!guest_player_context_squad_shape(base,pawn)||!guest_player_context_conditions(base,pc,pawn,info,ordinary_mode,enabled,1,&checked_channel,&checked_chain)||
       checked_chain.squad||!guest_player_context_same(base,pc,pawn,info,&checked_channel)||
       !read_at(pawn+0xba8,&own,8)||own!=l->pawn_squad||!guest_player_context_compare_slot(base,0,l->squad.pointer,&previous,0)) {
        guest_player_context_log("apply-rejected","cache-or-registration-compare-validation",now,0);guest_player_context_cleanup(base,"registration-not-verified",now);return;
    }
    l->registered=1;l->pending_cleanup=0;l->getter_after=guest_player_context_gate(base,"after-apply",now);
    if(l->getter_before!=0||l->getter_after!=1) {
        guest_player_context_log("apply-rejected","ordinary-getter-transition-not-verified",now,0);guest_player_context_cleanup(base,"getter-not-positive",now);return;
    }
    l->suppressed=0;l->monitor_at=now;guest_player_context_log("applied","normal-spawn-cache-and-local-registration",now,1);
}

#endif
