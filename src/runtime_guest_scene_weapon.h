#ifndef ME3_RUNTIME_GUEST_SCENE_WEAPON_H
#define ME3_RUNTIME_GUEST_SCENE_WEAPON_H
/* A conditional native selection continuation after an authenticated host
 * scene. Preserve the last selected existing weapon; the received inventory
 * index must independently select that same actor. No new inventory, guessed
 * index, ownership repair, weapon RPC, input, timer or ammunition write. */
typedef struct RuntimeSceneWeaponIdentity {
    SessionHandle pc,pawn,info,world,driver,connection,pawn_channel;
    SessionHandle inventory,weapon,inventory_channel,weapon_channel;
    int valid;
} RuntimeSceneWeaponIdentity;
typedef struct RuntimeGuestSceneWeapon {
    RuntimeSceneWeaponIdentity prior;
    GuestVitalsMethod updated;
    uint64_t nonce,started_sequence;
    DWORD ended,checked,method_checked;
    unsigned events;
    unsigned return_refusals;
    int phase,busy; /* 0 observe, 1 real host scene, 2 bounded native return */
} RuntimeGuestSceneWeapon;
static RuntimeGuestSceneWeapon runtime_guest_scene_weapon;
static int runtime_scene_weapon_enabled(void) {
    static int enabled=-1;
    if(enabled<0){wchar_t text[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_SCENE_WEAPON",text,8);
        enabled=!n||(n==1&&text[0]==L'1');}return enabled;
}
static int runtime_scene_weapon_policy(int phase,int ordinary,int selected_read,uintptr_t selected,
        int pending_read,uintptr_t pending,int index,int chain_same,DWORD ended,DWORD now) {
    return phase==2&&ordinary&&selected_read&&!selected&&pending_read&&!pending&&
        index>=0&&index<64&&chain_same&&now-ended<=10000;
}
static void runtime_scene_weapon_note(const char *event,DWORD now,int selected,int ammo) {
    RuntimeGuestSceneWeapon *q=&runtime_guest_scene_weapon;if(q->events++>=48)return;
    char row[430];snprintf(row,sizeof row,
        "{\"runtime_guest_scene_weapon\":\"%s\",\"selected_readback\":%s,\"ammo_unchanged\":%s,\"ownership_written\":false,\"input_synthesized\":false,\"gameplay_verified\":false,\"monotonic_ms\":%lu}\r\n",
        event,selected?"true":"false",ammo?"true":"false",now);line(row);
}
static int runtime_scene_weapon_handles(const RuntimeSceneWeaponIdentity *s) {
    const SessionHandle *h[]={&s->pc,&s->pawn,&s->info,&s->world,&s->driver,&s->connection,&s->pawn_channel,
        &s->inventory,&s->weapon,&s->inventory_channel,&s->weapon_channel};
    if(!s->valid)return 0;
    for(unsigned i=0;i<11;++i)if(!session_handle_matches(*h[i]))return 0;
    return 1;
}
static int runtime_scene_weapon_same(const RuntimeSceneWeaponIdentity *a,const RuntimeSceneWeaponIdentity *b) {
    const SessionHandle *aa[]={&a->pc,&a->pawn,&a->info,&a->world,&a->driver,&a->connection,&a->pawn_channel,
        &a->inventory,&a->weapon,&a->inventory_channel,&a->weapon_channel};
    const SessionHandle *bb[]={&b->pc,&b->pawn,&b->info,&b->world,&b->driver,&b->connection,&b->pawn_channel,
        &b->inventory,&b->weapon,&b->inventory_channel,&b->weapon_channel};
    if(!a->valid||!b->valid)return 0;
    for(unsigned i=0;i<11;++i)if(!runtime_movement_same_handle(*aa[i],*bb[i]))return 0;
    return 1;
}
static int runtime_scene_weapon_channel(uintptr_t base,uintptr_t actor,const RuntimeSceneWeaponIdentity *s,
        SessionHandle *captured) {
    NetworkActorIdentityEntry channel={0};
    if(captured->pointer) {
        if(!session_handle_matches(*captured)||!network_actor_channel_valid(base,actor,captured->pointer,s->driver.pointer,&channel))return 0;
    } else if(!network_actor_layout_matches(base)||
        network_actor_scan_driver(base,actor,s->driver.pointer,&channel)<0)return 0;
    if(channel.index<=0||channel.connection!=s->connection.pointer||channel.driver!=s->driver.pointer)return 0;
    *captured=session_capture_handle(channel.channel);return session_handle_matches(*captured);
}
static int runtime_scene_weapon_ordinary(uintptr_t base,uintptr_t engine,SessionContext *out,
        RuntimeSceneWeaponIdentity *s) {
    SessionContext c=session_context(engine);NetworkActorScope world={0};NetworkActorIdentityEntry channel={0};
    CombatContextChain chain;memset(s,0,sizeof *s);
    if(g_runtime_config.role!=ME3_ROLE_GUEST||!runtime_control_enabled||!runtime_scope_ready||
       !InterlockedCompareExchange(&g_enable_requested,0,0)||c.mode||session_engine_blocked(c)||runtime_scene_camera_requested()||
       !guest_player_context_registered_for(c.pc,c.pawn)||!guest_player_context_basic_owned(base,c.pc,c.pawn,c.world)||
       !guest_player_context_conditions(base,c.pc,c.pawn,c.world,1,1,1,&channel,&chain)||
       !network_actor_read_scope(base,c.pawn,&world)||world.info!=c.world||channel.driver!=world.driver||channel.index<=0)return 0;
    s->pc=session_capture_handle(c.pc);s->pawn=session_capture_handle(c.pawn);s->info=session_capture_handle(c.world);
    s->world=session_capture_handle(world.world);s->driver=session_capture_handle(world.driver);
    s->connection=session_capture_handle(channel.connection);s->pawn_channel=session_capture_handle(channel.channel);
    *out=c;return 1;
}
static int runtime_scene_weapon_item_idle(uintptr_t base,uintptr_t pc,uintptr_t pawn,uintptr_t inventory,uintptr_t weapon) {
    unsigned char fire=255,pending=255,current=255;uint32_t busy=0;GameArray requests={0};char state[96];
    if(!runtime_movement_idle(pawn)||!read_at(pc+0x480,&fire,1)||fire||
       !read_at(weapon+0x10c2,&pending,1)||pending||!read_at(weapon+0x3b0,&current,1)||current||
       !read_at(weapon+0x10b4,&busy,4)||(busy&0x1c000)||
       !runtime_guest_team_field(base,inventory,"InventoryManager","PendingFire","ArrayProperty",0x270,16,0x400000)||
       !scene_readiness_array(inventory+0x270,&requests,32)||!native_state_name(base,weapon,state,sizeof state)||
       (strcmp(state,"Active")&&strcmp(state,"Inactive")))return 0;
    for(int i=0;i<requests.count;++i){int32_t value=-1;if(!read_at(requests.data+(SIZE_T)i*4,&value,4)||value)return 0;}
    return 1;
}
/* Inactive.BeginState does not clear CurrentFireMode, PendingFireMode or the
 * drawing/holstering animation bits. Those are not an equipped idle contract.
 * A truly null selection with the same received inventory index is precisely
 * the case handled by stock ReplicatedWeaponUpdated -> ChangedWeapon ->
 * Activate. Never clear those fields or demand that this inactive actor has
 * already completed the activation that we are asking stock code to resume.
 * Keep the stricter equipped predicate unchanged for ammo and baseline use. */
static int runtime_scene_weapon_return_ready(uintptr_t base,uintptr_t pc,uintptr_t pawn,
        uintptr_t inventory,uintptr_t weapon) {
    unsigned char fire=255;GameArray requests={0};char state[96];
    if(!runtime_movement_idle(pawn)||!read_at(pc+0x480,&fire,1)||fire||
       !native_state_name(base,weapon,state,sizeof state)||strcmp(state,"Inactive")||
       !runtime_guest_team_field(base,inventory,"InventoryManager","PendingFire","ArrayProperty",0x270,16,0x400000)||
       !scene_readiness_array(inventory+0x270,&requests,32))return 0;
    for(int i=0;i<requests.count;++i){int32_t value=-1;if(!read_at(requests.data+(SIZE_T)i*4,&value,4)||value)return 0;}
    return 1;
}
static void runtime_scene_weapon_return_refused(unsigned gate,const char *reason,DWORD now) {
    RuntimeGuestSceneWeapon *q=&runtime_guest_scene_weapon;
    if(q->phase!=2||gate>=16||(q->return_refusals&(1u<<gate)))return;
    q->return_refusals|=1u<<gate;runtime_scene_weapon_note(reason,now,0,0);
}
/* Complete zero-based chain, including nonweapon items. A repeated node,
 * foreign owner/manager/world, unreadable link, or chain over64 is refused. */
static int runtime_scene_weapon_chain(uintptr_t base,const RuntimeSceneWeaponIdentity *s,int index) {
    uintptr_t inventory=s->inventory.pointer,pawn=s->pawn.pointer,head=0,next=0,items[64]={0},links[64]={0};
    SessionHandle handles[64];unsigned count=0;int32_t actual=-1;
    if(index<0||index>=64||!runtime_scene_weapon_handles(s)||
       !runtime_guest_weapon_chain(base,s->pc.pointer,pawn,s->weapon.pointer,inventory,s->info.pointer,0)||
       !runtime_guest_team_field(base,inventory,"SFXInventoryManager","ReplicatedWeaponIndex","IntProperty",0x3a0,4,UINT64_C(0x100000020))||
       !runtime_guest_team_field(base,inventory,"InventoryManager","InventoryChain","ObjectProperty",0x280,8,0x20)||
       !runtime_guest_team_field(base,inventory,"InventoryManager","PendingWeapon","ObjectProperty",0x288,8,0)||
       !read_at(inventory+0x3a0,&actual,4)||actual!=index||!read_at(inventory+0x280,&head,8)||!head)return 0;
    next=head;
    while(next&&count<64) {
        for(unsigned j=0;j<count;++j)if(items[j]==next)return 0;
        if(!guest_camera_actor_live(base,next)||!is_kind_of(base,next,"Inventory")||
           read_ptr(next+0x198)!=s->info.pointer||read_ptr(next+0x180)!=pawn||read_ptr(next+0x190)!=pawn||
           read_ptr(next+0x2b0)!=inventory||
           !runtime_guest_team_field(base,next,"Inventory","Inventory","ObjectProperty",0x2a8,8,0x20)||
           !runtime_guest_team_field(base,next,"Inventory","InvManager","ObjectProperty",0x2b0,8,0x20))return 0;
        items[count]=next;handles[count]=session_capture_handle(next);
        if(!session_handle_matches(handles[count])||!read_at(next+0x2a8,&links[count],8))return 0;
        next=links[count++];
    }
    if(next||index>=(int)count||items[index]!=s->weapon.pointer)return 0;
    for(unsigned i=0;i<count;++i)if(!session_handle_matches(handles[i])||
        read_ptr(items[i]+0x198)!=s->info.pointer||read_ptr(items[i]+0x180)!=pawn||read_ptr(items[i]+0x190)!=pawn||
        read_ptr(items[i]+0x2b0)!=inventory||!read_at(items[i]+0x2a8,&next,8)||next!=links[i])return 0;
    return read_at(inventory+0x280,&next,8)&&next==head&&read_at(inventory+0x3a0,&actual,4)&&actual==index;
}

/* Full exact stock VM after native pointer expansion. Source package
 * SHA256 1d8f7acb6f28e78030982c9f2d935cddec16877272aba314ce1c21dff5ed2d18;
 * storage417 -> runtime513, all24 object and6 FName relocations checked. */
static int runtime_scene_weapon_body(uintptr_t base,uintptr_t fn) {
    static const unsigned char code[513]={
        0x0f,0x5e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x65,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xe0,0x00,0x60,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x16,0x07,0x81,0x00,0x84,0x84,0x72,0x80,0x62,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x2a,
        0x16,0x18,0x0e,0x00,0x72,0x80,0x62,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x2a,0x16,0x16,0x18,0x0e,0x00,0x72,0x80,0x5e,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x2a,0x16,0x16,0x07,0x7f,0x00,0x81,0x1c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x21,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x4a,0x16,0x16,0x71,0x18,0x1e,0xcd,0xcc,0xcc,0x3d,0x27,0x21,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x4a,0x16,0x04,0x0b,0x1c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x21,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x4a,0x16,0x0f,0x5e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x2e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x5e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x07,0xe9,0x01,0x84,0x84,0x72,0x80,0x5e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x00,0x2a,0x16,0x18,0x2b,0x00,0x72,0x80,0x5e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x19,0x62,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x09,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x62,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x16,
        0x16,0x18,0x16,0x00,0x72,0x80,0x5e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x62,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x16,0x16,0xe7,0xa8,0xa8,0xa8,0x72,0x58,0x72,0x58,0xa8,0xa8,0xa8,0xa8,0xa8,0x38,0x55,0x19,0x62,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x09,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x5f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1f,
        0x53,0x65,0x6c,0x66,0x3a,0x00,0x16,0x38,0x56,0x17,0x16,0x1f,0x49,0x6e,0x73,0x74,0x69,0x67,0x61,0x74,0x6f,0x72,0x3a,0x00,
        0x16,0x38,0x56,0x62,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x16,0x38,0x57,0x71,0x1c,0x16,0x16,0x1f,0x3a,0x3a,0x00,0x16,
        0x38,0x57,0x1c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x16,0x16,0x1f,0x2d,0x2d,0x3c,0x20,0x49,0x67,0x6e,0x6f,0x72,0x69,
        0x6e,0x67,0x20,0x77,0x65,0x61,0x70,0x6f,0x6e,0x20,0x73,0x77,0x69,0x74,0x63,0x68,0x20,0x72,0x65,0x70,0x6c,0x69,0x63,0x61,
        0x74,0x69,0x6f,0x6e,0x20,0x69,0x6e,0x66,0x6f,0x72,0x6d,0x61,0x74,0x69,0x6f,0x6e,0x20,0x77,0x69,0x74,0x68,0x00,0x16,0x45,
        0x72,0x81,0x5e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x2a,0x16,0x0a,0x00,0x1f,0x6d,0x61,0x74,0x63,0x68,0x69,0x6e,0x67,
        0x00,0x06,0x00,0x1f,0x6e,0x6f,0x6e,0x65,0x00,0x16,0x1f,0x54,0x6f,0x57,0x65,0x61,0x70,0x6f,0x6e,0x00,0x16,0x21,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x16,0x04,0x0b,0x65,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xbc,0x00,0x5e,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x16,0x04,0x0b,0x53,
    };
    static const GuestEquipmentOperand ops[]={
        {2,"ObjectProperty","SFXGame.SFXInventoryManager.ReplicatedWeaponUpdated.oInventory",0,UINT64_C(0x0)},
        {22,"IntProperty","SFXGame.SFXInventoryManager.ReplicatedWeaponIndex",928,UINT64_C(0x100000020)},
        {39,"ObjectProperty","Engine.Actor.Instigator",400,UINT64_C(0x20)},
        {55,"ObjectProperty","Engine.InventoryManager.InventoryChain",640,UINT64_C(0x20)},
        {72,"ObjectProperty","SFXGame.SFXInventoryManager.ReplicatedWeaponUpdated.oInventory",0,UINT64_C(0x0)},
        {88,"Function","Engine.Actor.IsTimerActive",-1,UINT64_C(0x0)},
        {130,"Function","Engine.Actor.ClearTimer",-1,UINT64_C(0x0)},
        {151,"ObjectProperty","SFXGame.SFXInventoryManager.ReplicatedWeaponUpdated.oNewWeapon",8,UINT64_C(0x0)},
        {160,"Class","SFXGame.SFXWeapon",-1,UINT64_C(0x0)},
        {169,"ObjectProperty","SFXGame.SFXInventoryManager.ReplicatedWeaponUpdated.oInventory",0,UINT64_C(0x0)},
        {185,"ObjectProperty","SFXGame.SFXInventoryManager.ReplicatedWeaponUpdated.oNewWeapon",8,UINT64_C(0x0)},
        {201,"ObjectProperty","SFXGame.SFXInventoryManager.ReplicatedWeaponUpdated.oNewWeapon",8,UINT64_C(0x0)},
        {211,"ObjectProperty","Engine.Actor.Instigator",400,UINT64_C(0x20)},
        {221,"ObjectProperty","Engine.Pawn.Weapon",1052,UINT64_C(0x1)},
        {231,"ObjectProperty","Engine.Pawn.Weapon",1052,UINT64_C(0x1)},
        {247,"ObjectProperty","SFXGame.SFXInventoryManager.ReplicatedWeaponUpdated.oNewWeapon",8,UINT64_C(0x0)},
        {256,"ObjectProperty","Engine.InventoryManager.PendingWeapon",648,UINT64_C(0x0)},
        {283,"ObjectProperty","Engine.Actor.WorldInfo",408,UINT64_C(0x2002)},
        {293,"FloatProperty","Engine.WorldInfo.TimeSeconds",1868,UINT64_C(0x0)},
        {303,"FloatProperty","Engine.WorldInfo.TimeSeconds",1868,UINT64_C(0x0)},
        {340,"ObjectProperty","Engine.Actor.Instigator",400,UINT64_C(0x20)},
        {363,"Function","Core.Object.GetFuncName",-1,UINT64_C(0x0)},
        {435,"ObjectProperty","SFXGame.SFXInventoryManager.ReplicatedWeaponUpdated.oNewWeapon",8,UINT64_C(0x0)},
        {501,"ObjectProperty","SFXGame.SFXInventoryManager.ReplicatedWeaponUpdated.oNewWeapon",8,UINT64_C(0x0)},
        {0,NULL,NULL,0,0}};
    static const GuestEquipmentName names[]={
        {11,"GetInventoryByIndex"},
        {97,"ReplicatedWeaponUpdated"},
        {117,"ReplicatedWeaponUpdated"},
        {139,"ReplicatedWeaponUpdated"},
        {478,"Inventory"},
        {490,"InternalSetCurrentWeapon"},
        {0,NULL}};
    return guest_equipment_body(base,fn,513,code,sizeof code,ops,names);
}

/* The selector must enumerate this same native chain/index ordering. */
static int runtime_scene_weapon_index_body(uintptr_t base,uintptr_t fn) {
    static const unsigned char code[142]={
        0x0f,0x5c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x25,0x0f,0x5e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x62,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x63,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x81,0x00,0x07,0x4b,0x00,0x9a,0x5c,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x5c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x16,0x04,0x5e,0x00,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0xa5,0x5c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x16,0x0f,0x5e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x19,0x5e,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x09,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x62,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x1e,0x00,0x04,0x2a,0x04,0x3a,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x53,
    };
    static const GuestEquipmentOperand ops[]={
        {2,"IntProperty","SFXGame.SFXInventoryManager.GetInventoryByIndex.i",12,UINT64_C(0x0)},
        {13,"ObjectProperty","SFXGame.SFXInventoryManager.GetInventoryByIndex.InvIter",16,UINT64_C(0x0)},
        {22,"ObjectProperty","Engine.InventoryManager.InventoryChain",640,UINT64_C(0x20)},
        {31,"ObjectProperty","SFXGame.SFXInventoryManager.GetInventoryByIndex.InvIter",16,UINT64_C(0x0)},
        {47,"IntProperty","SFXGame.SFXInventoryManager.GetInventoryByIndex.Index",0,UINT64_C(0x80)},
        {56,"IntProperty","SFXGame.SFXInventoryManager.GetInventoryByIndex.i",12,UINT64_C(0x0)},
        {67,"ObjectProperty","SFXGame.SFXInventoryManager.GetInventoryByIndex.InvIter",16,UINT64_C(0x0)},
        {77,"IntProperty","SFXGame.SFXInventoryManager.GetInventoryByIndex.i",12,UINT64_C(0x0)},
        {88,"ObjectProperty","SFXGame.SFXInventoryManager.GetInventoryByIndex.InvIter",16,UINT64_C(0x0)},
        {98,"ObjectProperty","SFXGame.SFXInventoryManager.GetInventoryByIndex.InvIter",16,UINT64_C(0x0)},
        {108,"ObjectProperty","Engine.Inventory.Inventory",680,UINT64_C(0x20)},
        {118,"ObjectProperty","Engine.Inventory.Inventory",680,UINT64_C(0x20)},
        {133,"ObjectProperty","SFXGame.SFXInventoryManager.GetInventoryByIndex.ReturnValue",4,UINT64_C(0x580)},
        {0,NULL,NULL,0,0}};
    static const GuestEquipmentName names[]={{0,NULL}};
    return guest_equipment_body(base,fn,142,code,sizeof code,ops,names);
}

static int runtime_scene_weapon_method(uintptr_t base,uintptr_t inventory,GuestVitalsMethod *out) {
    uintptr_t klass=read_ptr(inventory+0x50),get=guest_equipment_find_first(base,klass,"GetInventoryByIndex");
    uintptr_t internal=guest_equipment_find_first(base,klass,"InternalSetCurrentWeapon");GuestCameraStamp a={0},b={0};
    return guest_equipment_dispatch_pins(base)&&
        guest_vitals_method(base,"SFXInventoryManager","ReplicatedWeaponUpdated",0x20102,0,NULL,0,out)&&
        guest_player_context_function_abi(base,out->object.pointer,"SFXInventoryManager","ReplicatedWeaponUpdated",0x20102,0,65535,0,0xf52b0,0)&&
        runtime_scene_weapon_body(base,out->object.pointer)&&
        guest_equipment_find_first(base,klass,"ReplicatedWeaponUpdated")==out->object.pointer&&
        guest_camera_stamp_read(base,get,&a,1)&&!strcmp(a.full_name,"SFXGame.SFXInventoryManager.GetInventoryByIndex")&&
        guest_camera_stamp_read(base,internal,&b,1)&&!strcmp(b.full_name,"SFXGame.SFXInventoryManager.InternalSetCurrentWeapon")&&
        guest_player_context_function_abi(base,get,"SFXInventoryManager","GetInventoryByIndex",0x20102,12,4,0,0xf52b0,2)&&
        runtime_scene_weapon_index_body(base,get)&&
        native_script_slot_matches(base,klass,224,get)&&native_script_slot_matches(base,klass,188,internal)&&
        read_ptr(read_ptr(inventory)+0x230)==base+0x40d450;
}
static int runtime_scene_weapon_scene_scope(uintptr_t base,const RuntimeSceneWeaponIdentity *s) {
    RuntimeSceneScope *scene=&runtime_scene_view.scope;
    return runtime_scene_weapon_handles(s)&&runtime_scene_scope_live(base,scene)&&
        scene->pc.pointer==s->pc.pointer&&scene->world.pointer==s->info.pointer&&scene->root.pointer==s->world.pointer&&
        scene->driver.pointer==s->driver.pointer&&scene->connection.pointer==s->connection.pointer;
}
static void runtime_guest_scene_weapon_tick(uintptr_t base,uintptr_t engine,DWORD now) {
    RuntimeGuestSceneWeapon *q=&runtime_guest_scene_weapon;SessionContext c;RuntimeSceneWeaponIdentity current={0};
    if(GetCurrentThreadId()!=session_game_thread||q->busy)return;
    if(!runtime_scene_weapon_enabled()||g_runtime_config.role!=ME3_ROLE_GUEST||!runtime_control_enabled||!runtime_scope_ready||
       !InterlockedCompareExchange(&g_enable_requested,0,0)) {q->phase=0;q->prior.valid=0;return;}
    if(q->prior.valid&&!runtime_scene_weapon_handles(&q->prior)){q->phase=0;q->prior.valid=0;}
    if(runtime_scene_camera_requested()) {
        if(q->prior.valid&&runtime_scene_weapon_scene_scope(base,&q->prior)) {
            if(q->phase!=1){q->phase=1;q->nonce=runtime_scene_view.message.nonce;
                q->started_sequence=runtime_scene_view.message.sequence;runtime_scene_weapon_note("host-scene-observed",now,0,0);}
        } else {q->phase=0;q->prior.valid=0;}
        return;
    }
    if(q->phase==1) {
        if(!runtime_scene_weapon_scene_scope(base,&q->prior)||!runtime_scene_view.valid||
           runtime_scene_view.message.nonce!=q->nonce){q->phase=0;q->prior.valid=0;return;}
        /* A freshness timeout is not a scene end. Require its authenticated
         * later inactive message; no repair while a scene merely stops sending. */
        if(runtime_scene_view.message.active||runtime_scene_view.message.sequence<=q->started_sequence)return;
        q->phase=2;q->ended=now;q->checked=0;q->return_refusals=0;
    }
    if(q->phase==2&&now-q->ended>10000){q->phase=0;q->prior.valid=0;runtime_scene_weapon_note("return-window-expired",now,0,0);}
    if(q->checked&&now-q->checked<250)return;q->checked=now;
    if(!runtime_scene_weapon_ordinary(base,engine,&c,&current)){
        runtime_scene_weapon_return_refused(0,"return-waiting-for-ordinary-owned-context",now);return;}
    uintptr_t selected=1,inventory=0,pending=1;int32_t index=-1;
    if(!read_at(c.pawn+0x41c,&selected,8)||!read_at(c.pawn+0x414,&inventory,8)||
       !guest_camera_actor_live(base,inventory)||!is_kind_of(base,inventory,"SFXInventoryManager")||
       !read_at(inventory+0x288,&pending,8)||pending||!read_at(inventory+0x3a0,&index,4)){
        runtime_scene_weapon_return_refused(1,"return-inventory-unready-or-native-switch-pending",now);return;}
    current.inventory=session_capture_handle(inventory);
    current.weapon=q->phase==2?q->prior.weapon:session_capture_handle(selected);
    if(!session_handle_matches(current.inventory)||!session_handle_matches(current.weapon))return;
    if(q->prior.valid&&current.inventory.pointer==q->prior.inventory.pointer&&current.weapon.pointer==q->prior.weapon.pointer) {
        current.inventory_channel=q->prior.inventory_channel;current.weapon_channel=q->prior.weapon_channel;
    }
    if(!runtime_scene_weapon_channel(base,inventory,&current,&current.inventory_channel)||
       !runtime_scene_weapon_channel(base,current.weapon.pointer,&current,&current.weapon_channel)){
        runtime_scene_weapon_return_refused(2,"return-actor-channel-scope-unavailable",now);return;}
    current.valid=1;
    if(q->phase==2&&selected) {
        /* Native return can initially succeed and then lose the selection.
         * Observe the same prior weapon for this bounded return window. A
         * different native choice ends our continuation immediately. */
        if(selected!=q->prior.weapon.pointer) {
            q->phase=0;q->prior.valid=0;runtime_scene_weapon_note("different-native-selection-preserved",now,0,0);
        }
        return;
    }
    int item_ready=q->phase==2?
        runtime_scene_weapon_return_ready(base,c.pc,c.pawn,inventory,current.weapon.pointer):
        runtime_scene_weapon_item_idle(base,c.pc,c.pawn,inventory,current.weapon.pointer);
    if(!item_ready||!runtime_scene_weapon_chain(base,&current,index)){
        runtime_scene_weapon_return_refused(3,item_ready?"return-native-index-chain-mismatch":"return-native-state-or-fire-request-busy",now);return;}
    if(q->phase==0) {
        if(selected!=current.weapon.pointer)return;
        int same=runtime_scene_weapon_same(&q->prior,&current);q->prior=current;
        if(!same){memset(&q->updated,0,sizeof q->updated);q->method_checked=0;}
        if(guest_vitals_method_live(base,&q->updated))return;
        if(q->method_checked&&now-q->method_checked<5000)return;q->method_checked=now;
        GuestVitalsMethod prepared={0};
        if(runtime_scene_weapon_method(base,inventory,&prepared)) {
            q->updated=prepared;
            runtime_scene_weapon_note("baseline-stock-method-ready",now,1,0);
        }
        return;
    }
    if(!runtime_scene_weapon_policy(q->phase,1,1,selected,1,pending,index,
       runtime_scene_weapon_same(&q->prior,&current),q->ended,now)){
        runtime_scene_weapon_return_refused(4,"return-captured-scope-or-selection-changed",now);return;}
    GuestVitalsMethod method={0};GuestEquipmentSnapshot before,after;RuntimeSceneWeaponIdentity final={0};SessionContext fresh;
    if(!runtime_scene_weapon_method(base,inventory,&method)||
       !guest_equipment_snapshot(c.pawn,current.weapon.pointer,inventory,&before)||before.pawn_weapon||
       !runtime_scene_weapon_ordinary(base,engine,&fresh,&final)){
        runtime_scene_weapon_return_refused(5,"return-stock-method-or-final-context-unavailable",now);return;}
    final.inventory=current.inventory;final.weapon=current.weapon;final.inventory_channel=current.inventory_channel;
    final.weapon_channel=current.weapon_channel;final.valid=1;
    uintptr_t raw=1,waiting=1;
    if(!runtime_scene_weapon_same(&current,&final)||!runtime_scene_weapon_handles(&final)||
       !runtime_scene_weapon_channel(base,inventory,&final,&final.inventory_channel)||
       !runtime_scene_weapon_channel(base,final.weapon.pointer,&final,&final.weapon_channel)||
       !read_at(fresh.pawn+0x41c,&raw,8)||raw||!read_at(inventory+0x288,&waiting,8)||waiting||
       !runtime_scene_weapon_return_ready(base,fresh.pc,fresh.pawn,inventory,final.weapon.pointer)||
       !runtime_scene_weapon_chain(base,&final,index)||!guest_vitals_method_live(base,&method)||
       !runtime_scene_weapon_body(base,method.object.pointer)||runtime_scene_camera_requested()||
       read_ptr(read_ptr(inventory)+0x230)!=base+0x40d450)return;
    /* Consume the only attempt before entering ordinary engine code. */
    q->phase=0;q->prior.valid=0;q->busy=1;
    ((ProcessEvent)(base+0x40d450))((void*)inventory,(void*)method.object.pointer,NULL,NULL);q->busy=0;
    int stable=runtime_scene_weapon_handles(&final)&&
        guest_player_context_basic_owned(base,fresh.pc,fresh.pawn,fresh.world)&&
        read_ptr(base+0x18b6638)==final.world.pointer&&read_ptr(final.world.pointer+0x120)==final.driver.pointer&&
        runtime_guest_weapon_chain(base,fresh.pc,fresh.pawn,final.weapon.pointer,inventory,fresh.world,0)&&
        runtime_scene_weapon_channel(base,inventory,&final,&final.inventory_channel)&&
        runtime_scene_weapon_channel(base,final.weapon.pointer,&final,&final.weapon_channel);
    int selected_after=stable&&read_at(fresh.pawn+0x41c,&raw,8)&&raw==final.weapon.pointer;
    int ammo=stable&&guest_equipment_snapshot(fresh.pawn,final.weapon.pointer,inventory,&after)&&before.used==after.used&&before.spare==after.spare;
    runtime_scene_weapon_note("stock-replicated-selection-returned",now,selected_after,ammo);
}
#ifdef ME3_RUNTIME_FIXTURE
static int runtime_guest_scene_weapon_selftest(void) {
    int ok=runtime_scene_weapon_policy(2,1,1,0,1,0,0,1,100,101)&&
        runtime_scene_weapon_policy(2,1,1,0,1,0,63,1,100,10100)&&
        !runtime_scene_weapon_policy(2,1,1,0,1,0,64,1,100,101)&&
        !runtime_scene_weapon_policy(2,1,1,0,1,0,-1,1,100,101)&&
        !runtime_scene_weapon_policy(2,1,1,0,1,0,0,1,100,10101);
    for(int bad=0;bad<7;++bad)ok=ok&&!runtime_scene_weapon_policy(bad==0?1:2,bad!=1,bad!=2,bad==3?1:0,
        bad!=4,bad==5?1:0,0,bad!=6,100,101);
    RuntimeSceneWeaponIdentity a={0},b;SessionHandle *h[]={&a.pc,&a.pawn,&a.info,&a.world,&a.driver,&a.connection,
        &a.pawn_channel,&a.inventory,&a.weapon,&a.inventory_channel,&a.weapon_channel};
    for(unsigned i=0;i<11;++i)*h[i]=(SessionHandle){0x100+i*8,(int32_t)i+1,i+2};a.valid=1;b=a;
    ok=ok&&runtime_scene_weapon_same(&a,&b);
    for(unsigned i=0;i<11;++i){++h[i]->index;ok=ok&&!runtime_scene_weapon_same(&a,&b);--h[i]->index;}
    return ok;
}
#endif
#endif
