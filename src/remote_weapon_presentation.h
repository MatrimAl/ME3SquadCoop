#ifndef ME3_REMOTE_WEAPON_PRESENTATION_H
#define ME3_REMOTE_WEAPON_PRESENTATION_H
/* Catch up an existing selected weapon after delayed remote-player appearance.
 * Never create inventory, switch weapons, rebuild animation or alter visibility.
 * The stock simulated AttachWeaponTo checks the socket and supplies its normal
 * lighting/muzzle-effect setup. A non-null foreign attachment is not ours.
 * Exact LE3 SDK: Pawn.Weapon41c/InvManager414, Weapon.Mesh380,
 * SMC.AttachedToSkelComponent260, BioPawn.RightHandSocketNameaf0,
 * SFXPawn.bSupportsVisibleWeaponsff4 mask20. Selected script export35099.
 * Include after remote_appearance_qa.h and the shared equipment helpers. */
typedef struct RemoteWeaponPresentationAttempt {
    int valid;
    GuestCameraStamp pawn,world,mesh,weapon,inventory,component;
} RemoteWeaponPresentationAttempt;
static RemoteWeaponPresentationAttempt remote_weapon_presentation_attempt;
static DWORD remote_weapon_presentation_scan_at;
static int remote_weapon_presentation_active;
static int remote_weapon_presentation_case(int ordinary,int guest,int role,int distinct,
        int channels,int chain,int meshes,int supported,uintptr_t parent) {
    return ordinary==1&&guest==1&&role==1&&distinct==1&&channels==1&&chain==1&&
        meshes==1&&supported==1&&!parent;
}
static int remote_weapon_presentation_same(const RemoteWeaponPresentationAttempt *a,
        const RemoteWeaponPresentationAttempt *b) {
    return a->valid&&b->valid&&guest_camera_stamp_core_equal(&a->pawn,&b->pawn)&&
        guest_camera_stamp_core_equal(&a->world,&b->world)&&guest_camera_stamp_core_equal(&a->mesh,&b->mesh)&&
        guest_camera_stamp_core_equal(&a->weapon,&b->weapon)&&guest_camera_stamp_core_equal(&a->inventory,&b->inventory)&&
        guest_camera_stamp_core_equal(&a->component,&b->component);
}
static void remote_weapon_presentation_tick(uintptr_t base,uintptr_t pc,uintptr_t local,
        uintptr_t info,int ordinary,int enabled,DWORD now) {
    if(!enabled||!ordinary||remote_weapon_presentation_active||
       now-remote_weapon_presentation_scan_at<1000)return;
    remote_weapon_presentation_scan_at=now;
    unsigned char net=255,role=255;
    /* Limit this repair to the exact player/world whose delayed appearance
     * request we already own. Merely finding a similarly named pawn is not
     * enough, nor is an old request after channel/world replacement. */
    RemoteAppearanceAttempt *appearance=&remote_appearance_attempt;
    if(!appearance->valid||appearance->apply_pending||appearance->world.pointer!=info||
       !guest_camera_stamp_matches(base,&appearance->pawn,1)||
       !guest_camera_stamp_matches(base,&appearance->world,1)||
       !guest_camera_actor_live(base,local)||!is_kind_of(base,local,"SFXPawn_Ashley")||
       read_ptr(local+0x374)!=pc||read_ptr(pc+0x3bc)!=local||read_ptr(local+0x198)!=info||
       !read_at(info+0x7c8,&net,1)||net!=3)return;
    uintptr_t pawn=appearance->pawn.pointer,mesh=read_ptr(pawn+0x3f4),
        weapon=read_ptr(pawn+0x41c),inventory=read_ptr(pawn+0x414),component=read_ptr(weapon+0x380);
    uint32_t supported=0,socket[2]={0};uintptr_t parent=0;
    GuestEquipmentSnapshot before,after;
    int chain=guest_camera_actor_live(base,pawn)&&is_kind_of(base,pawn,"SFXPawn_Player")&&
        guest_camera_actor_live(base,weapon)&&is_kind_of(base,weapon,"SFXWeapon")&&
        guest_camera_actor_live(base,inventory)&&is_kind_of(base,inventory,"SFXInventoryManager")&&
        read_ptr(pawn+0x198)==info&&read_ptr(weapon+0x198)==info&&read_ptr(inventory+0x198)==info&&
        guest_equipment_snapshot(pawn,weapon,inventory,&before)&&before.owner==pawn&&before.instigator==pawn&&
        before.weapon_inventory==inventory&&before.pawn_weapon==weapon&&before.pawn_inventory==inventory&&
        before.inventory_owner==pawn&&before.inventory_instigator==pawn;
    int meshes=narrative_probe_object(base,mesh,"SkeletalMeshComponent")&&
        narrative_probe_object(base,component,"SkeletalMeshComponent")&&
        read_ptr(mesh+0x40)==pawn&&read_ptr(component+0x40)==weapon&&
        narrative_probe_object(base,read_ptr(mesh+0x258),"SkeletalMesh")&&
        narrative_probe_object(base,read_ptr(component+0x258),"SkeletalMesh")&&
        read_at(component+0x260,&parent,8);
    if(!chain||!meshes||parent)return;
    NetworkActorScope scope;NetworkActorIdentityEntry pawn_channel={0},weapon_channel={0},inventory_channel={0};
    int channels=network_actor_read_scope(base,pawn,&scope)&&
        network_actor_scan_driver(base,pawn,scope.driver,&pawn_channel)>=0&&
        network_actor_scan_driver(base,weapon,scope.driver,&weapon_channel)>=0&&
        network_actor_scan_driver(base,inventory,scope.driver,&inventory_channel)>=0&&
        pawn_channel.connection==weapon_channel.connection&&pawn_channel.connection==inventory_channel.connection;
    if(!read_at(pawn+0x26a,&role,1)||!read_at(pawn+0xff4,&supported,4)||
       !remote_weapon_presentation_case(ordinary,net==3,role,pawn!=local,channels,chain,meshes,
           (supported&0x20)!=0,parent)||!read_at(pawn+0xaf0,socket,8))return;
    char socket_text[96];name_of(base,socket[0],socket_text,sizeof socket_text);
    if(!socket_text[0]||!strcmp(socket_text,"None"))return;
    RemoteWeaponPresentationAttempt current={0};current.valid=1;
    if(!guest_camera_stamp_read(base,pawn,&current.pawn,1)||
       !guest_camera_stamp_read(base,info,&current.world,1)||
       !guest_camera_stamp_read(base,mesh,&current.mesh,1)||
       !guest_camera_stamp_read(base,weapon,&current.weapon,1)||
       !guest_camera_stamp_read(base,inventory,&current.inventory,1)||
       !guest_camera_stamp_read(base,component,&current.component,1)||
       remote_weapon_presentation_same(&remote_weapon_presentation_attempt,&current))return;
    uintptr_t fn=guest_equipment_find_first(base,read_ptr(weapon+0x50),"AttachWeaponTo");
    GuestCameraStamp method;
    if(!guest_camera_stamp_read(base,fn,&method,1)||strcmp(method.full_name,"SFXGame.SFXWeapon.AttachWeaponTo")||
       !guest_player_context_function_abi(base,fn,"SFXWeapon","AttachWeaponTo",0x24102,16,65535,0,0xf52b0,2))return;
    uintptr_t pe=read_ptr(read_ptr(weapon)+0x230);
    if(!guest_equipment_weapon_pe_valid(base,weapon,pe)||read_ptr(read_ptr(weapon)+0x248)!=base+0x70a250||
       !guest_camera_stamp_matches(base,&current.pawn,1)||!guest_camera_stamp_matches(base,&current.world,1)||
       !guest_camera_stamp_matches(base,&current.mesh,1)||!guest_camera_stamp_matches(base,&current.weapon,1)||
       !guest_camera_stamp_matches(base,&current.inventory,1)||!guest_camera_stamp_matches(base,&current.component,1)||
       !guest_camera_stamp_matches(base,&method,1)||read_ptr(pc+0x3bc)!=local||read_ptr(local+0x374)!=pc||
       read_ptr(pawn+0x3f4)!=mesh||read_ptr(pawn+0x41c)!=weapon||read_ptr(pawn+0x414)!=inventory||
       read_ptr(weapon+0x380)!=component||read_ptr(component+0x260)||
       !guest_equipment_snapshot(pawn,weapon,inventory,&after)||memcmp(&before,&after,sizeof before))return;
    struct {uintptr_t mesh;uint32_t socket[2];} parms={mesh,{socket[0],socket[1]}};
    _Static_assert(sizeof parms==16,"AttachWeaponTo parameter ABI");
    remote_weapon_presentation_attempt=current;
    remote_weapon_presentation_active=1;
    ((ProcessEvent)pe)((void*)weapon,(void*)fn,&parms,NULL);
    int stable=guest_camera_stamp_matches(base,&current.pawn,1)&&guest_camera_stamp_matches(base,&current.world,1)&&
        guest_camera_stamp_matches(base,&current.mesh,1)&&guest_camera_stamp_matches(base,&current.weapon,1)&&
        guest_camera_stamp_matches(base,&current.inventory,1)&&guest_camera_stamp_matches(base,&current.component,1);
    int attached=stable&&read_ptr(pawn+0x3f4)==mesh&&read_ptr(weapon+0x380)==component&&read_ptr(component+0x260)==mesh;
    int counters=stable&&guest_equipment_snapshot(pawn,weapon,inventory,&after)&&
        before.used==after.used&&before.spare==after.spare;
    char row[700];snprintf(row,sizeof row,
        "{\"remote_weapon_presentation\":\"stock-attachment-returned\",\"same_identity\":%s,\"attached_to_current_mesh\":%s,\"ammo_unchanged\":%s,\"rendered_visibility_verified\":false,\"monotonic_ms\":%lu}\r\n",
        stable?"true":"false",attached?"true":"false",counters?"true":"false",now);line(row);
    remote_weapon_presentation_active=0;
}
static int remote_weapon_presentation_selftest(void) {
    if(!remote_weapon_presentation_case(1,1,1,1,1,1,1,1,0)||
       remote_weapon_presentation_case(1,1,1,1,1,1,1,1,0x1234))return 0;
    for(int i=0;i<8;++i){int gate[8]={1,1,1,1,1,1,1,1};gate[i]=0;
        if(remote_weapon_presentation_case(gate[0],gate[1],gate[2],gate[3],gate[4],gate[5],gate[6],gate[7],0))return 0;}
    RemoteWeaponPresentationAttempt a={0},b={0};a.valid=b.valid=1;
    a.pawn.pointer=b.pawn.pointer=1;a.world.pointer=b.world.pointer=2;a.mesh.pointer=b.mesh.pointer=3;
    a.weapon.pointer=b.weapon.pointer=4;a.inventory.pointer=b.inventory.pointer=5;a.component.pointer=b.component.pointer=6;
    if(!remote_weapon_presentation_same(&a,&b))return 0;
    b.component.index++;if(remote_weapon_presentation_same(&a,&b))return 0;b=a;
    b.mesh.pointer++;if(remote_weapon_presentation_same(&a,&b))return 0;b=a;
    b.world.index++;return !remote_weapon_presentation_same(&a,&b);
}
/* A replicated weapon can arrive without its unreplicated Owner/selected
 * reference while the manager's replicated index targets an incomplete chain.
 * The caller supplies the host-selected actor, confirmed across two native
 * owner-PC replies in the same captured actor-channel lifetime. Use only the
 * stock existing-item ownership and immediate simulated equip paths. Do not
 * manufacture a pickup, chain entry, replicated index, weapon or ammunition. */
static RemoteWeaponPresentationAttempt remote_weapon_selection_attempt;
static DWORD remote_weapon_selection_scan_at;
static void runtime_remote_weapon_apply(uintptr_t base,uintptr_t pc,uintptr_t local,uintptr_t world,
        uintptr_t pawn,uintptr_t weapon,DWORD now) {
    if(now-remote_weapon_selection_scan_at<1000)return;
    remote_weapon_selection_scan_at=now;
    unsigned char pawn_role=255,weapon_role=255,inventory_role=255;
    uintptr_t inventory=read_ptr(pawn+0x414),mesh=read_ptr(pawn+0x3f4),component=read_ptr(weapon+0x380);
    RemoteAppearanceAttempt *appearance=&remote_appearance_attempt;uint32_t support=0;
    if(!appearance->valid||appearance->apply_pending||appearance->pawn.pointer!=pawn||appearance->world.pointer!=world||
       !guest_camera_stamp_matches(base,&appearance->pawn,1)||!guest_camera_stamp_matches(base,&appearance->world,1)||
       !guest_player_context_basic_owned(base,pc,local,world)||pawn==local||!is_kind_of(base,pawn,"SFXPawn_Player")||
       !guest_camera_actor_live(base,pawn)||!guest_camera_actor_live(base,weapon)||!is_kind_of(base,weapon,"SFXWeapon")||
       !guest_camera_actor_live(base,inventory)||!is_kind_of(base,inventory,"SFXInventoryManager")||
       read_ptr(pawn+0x198)!=world||read_ptr(weapon+0x198)!=world||read_ptr(inventory+0x198)!=world||
       !read_at(pawn+0x26a,&pawn_role,1)||pawn_role!=1||!read_at(weapon+0x26a,&weapon_role,1)||weapon_role!=1||
       !read_at(inventory+0x26a,&inventory_role,1)||inventory_role!=1||read_ptr(pawn+0x374)||
       read_ptr(pawn+0x41c)||read_ptr(inventory+0x288)||!read_at(pawn+0xff4,&support,4)||!(support&0x20))return;
    GuestEquipmentSnapshot before,after;
    if(!guest_equipment_snapshot(pawn,weapon,inventory,&before)||(before.owner&&before.owner!=pawn)||
       before.instigator!=pawn||before.weapon_inventory!=inventory||before.inventory_instigator!=pawn||
       (before.inventory_owner&&before.inventory_owner!=pawn)||before.pawn_inventory!=inventory||before.pawn_weapon||
       !narrative_probe_object(base,mesh,"SkeletalMeshComponent")||read_ptr(mesh+0x40)!=pawn||
       !narrative_probe_object(base,read_ptr(mesh+0x258),"SkeletalMesh")||
       !narrative_probe_object(base,component,"SkeletalMeshComponent")||read_ptr(component+0x40)!=weapon||
       !narrative_probe_object(base,read_ptr(component+0x258),"SkeletalMesh")||read_ptr(component+0x260))return;
    NetworkActorScope scope;NetworkActorIdentityEntry pch={0},wch={0},ich={0};
    if(!network_actor_read_scope(base,pawn,&scope)||network_actor_scan_driver(base,pawn,scope.driver,&pch)<0||
       network_actor_scan_driver(base,weapon,scope.driver,&wch)<0||network_actor_scan_driver(base,inventory,scope.driver,&ich)<0||
       pch.connection!=wch.connection||pch.connection!=ich.connection)return;
    RemoteWeaponPresentationAttempt stamp={0};stamp.valid=1;
    if(!guest_camera_stamp_read(base,pawn,&stamp.pawn,1)||!guest_camera_stamp_read(base,world,&stamp.world,1)||
       !guest_camera_stamp_read(base,mesh,&stamp.mesh,1)||!guest_camera_stamp_read(base,weapon,&stamp.weapon,1)||
       !guest_camera_stamp_read(base,inventory,&stamp.inventory,1)||!guest_camera_stamp_read(base,component,&stamp.component,1)||
       remote_weapon_presentation_same(&remote_weapon_selection_attempt,&stamp))return;
    uintptr_t given=guest_player_context_find_method(base,read_ptr(weapon+0x50),"Inventory","ClientGivenTo");
    uintptr_t equip=guest_equipment_find_first(base,read_ptr(inventory+0x50),"SetWeaponImmediately");
    GuestCameraStamp given_stamp,equip_stamp;
    if(!guest_camera_stamp_read(base,given,&given_stamp,1)||strcmp(given_stamp.full_name,"Engine.Inventory.ClientGivenTo")||
       !guest_player_context_function_abi(base,given,"Inventory","ClientGivenTo",0x10201c2,12,65535,0,0xf52b0,2)||
       !runtime_remote_helmet_param(base,given,"NewOwner","ObjectProperty",0,0x80)||
       !runtime_remote_helmet_param(base,given,"bDoNotActivate","BoolProperty",8,0x80)||
       !guest_camera_stamp_read(base,equip,&equip_stamp,1)||strcmp(equip_stamp.full_name,"SFXGame.SFXInventoryManager.SetWeaponImmediately")||
       !guest_player_context_function_abi(base,equip,"SFXInventoryManager","SetWeaponImmediately",0x20902,12,8,0,0xf52b0,2)||
       !runtime_remote_helmet_param(base,equip,"WpnForSwitch","ObjectProperty",0,0x80)||
       !runtime_remote_helmet_param(base,equip,"ReturnValue","BoolProperty",8,0x580)||
       !guest_equipment_dispatch_pins(base)||!guest_equipment_snapshot(pawn,weapon,inventory,&after)||memcmp(&before,&after,sizeof before)) {
        static unsigned reported;
        if(reported++<4)line("{\"remote_weapon_presentation\":\"existing-weapon-method-admission-refused\"}\r\n");
        return;
    }
    remote_weapon_selection_attempt=stamp;
    if(!before.owner) {
        unsigned char params[12]={0};uint32_t inactive=1;memcpy(params,&pawn,8);memcpy(params+8,&inactive,4);
        if(!runtime_remote_helmet_dispatch(base,weapon,given,params))return;
    }
    if(!guest_camera_stamp_matches(base,&stamp.pawn,1)||!guest_camera_stamp_matches(base,&stamp.world,1)||
       !guest_camera_stamp_matches(base,&stamp.weapon,1)||!guest_camera_stamp_matches(base,&stamp.inventory,1)||
       !guest_camera_stamp_matches(base,&given_stamp,1)||!guest_camera_stamp_matches(base,&equip_stamp,1)||
       read_ptr(pawn+0x374)||read_ptr(pawn+0x41c)||read_ptr(inventory+0x288)||
       read_ptr(weapon+0x180)!=pawn||read_ptr(weapon+0x190)!=pawn||read_ptr(weapon+0x2b0)!=inventory)return;
    unsigned char params[12]={0};memcpy(params,&weapon,8);
    if(!runtime_remote_helmet_dispatch(base,inventory,equip,params))return;
    uint32_t returned=0;memcpy(&returned,params+8,4);
    int stable=guest_camera_stamp_matches(base,&stamp.pawn,1)&&guest_camera_stamp_matches(base,&stamp.world,1)&&
        guest_camera_stamp_matches(base,&stamp.weapon,1)&&guest_camera_stamp_matches(base,&stamp.inventory,1)&&
        guest_camera_stamp_matches(base,&stamp.mesh,1)&&guest_camera_stamp_matches(base,&stamp.component,1);
    int selected=stable&&read_ptr(pawn+0x41c)==weapon,attached=stable&&read_ptr(component+0x260)==mesh;
    int counters=stable&&guest_equipment_snapshot(pawn,weapon,inventory,&after)&&before.used==after.used&&before.spare==after.spare;
    char row[520];snprintf(row,sizeof row,
        "{\"remote_weapon_presentation\":\"stock-existing-weapon-equipped\",\"native_result\":%u,\"same_identity\":%s,\"selected\":%s,\"attached_to_current_mesh\":%s,\"ammo_unchanged\":%s,\"rendered_visibility_verified\":false,\"monotonic_ms\":%lu}\r\n",
        returned,stable?"true":"false",selected?"true":"false",attached?"true":"false",counters?"true":"false",now);line(row);
}
#endif
