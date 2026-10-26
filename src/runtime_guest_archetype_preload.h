#ifndef ME3_RUNTIME_GUEST_ARCHETYPE_PRELOAD_H
#define ME3_RUNTIME_GUEST_ARCHETYPE_PRELOAD_H
/* Ordinary guest preparation for the observed missing BioChar_Player net object.
 * Use the same stock API as host morph loading, before native guest travel.
 * No package-map/cache/actor/role writes or synthetic input. */
static RuntimePackagePreload runtime_guest_preload;
static int runtime_guest_archetype_preload(uintptr_t base,SessionContext context) {
    if(!runtime_package_preload_option(L"ME3_SQUAD_COOP_GUEST_ARCHETYPE_PRELOAD"))return 1;
    GuestCameraStamp world={0};
    if(!guest_camera_stamp_read(base,context.world,&world,1)||!guest_camera_stamp_matches(base,&world,1))return 0;
    int decision=runtime_package_preload_decision(&runtime_guest_preload,&world);
    if(decision!=PACKAGE_PRELOAD_BEGIN)return decision==PACKAGE_PRELOAD_READY;
    uintptr_t game=read_ptr(context.world+0x6b0),fn=function_named(base,"SFXGame","LoadPackage");
    unsigned char mode=255;
    if(g_runtime_config.role!=ME3_ROLE_GUEST||!session_actor_live(base,game)||
       !is_kind_of(base,game,"SFXGame")||read_ptr(game+0x198)!=context.world||
       !session_actor_live(base,context.pawn)||!is_kind_of(base,context.pawn,"SFXPawn_Player")||
       read_ptr(context.pawn+0x374)!=context.pc||read_ptr(context.pawn+0x198)!=context.world||
       !read_at(context.world+0x7c8,&mode,1)||mode!=0||
       !runtime_package_load_abi(base,fn)||read_ptr(fn+0xd8)!=base+0xb6cfa0)return 0;
    wchar_t name[]=L"BioChar_Player";
    GameString params={name,(int32_t)wcslen(name)+1,(int32_t)wcslen(name)+1};
    runtime_guest_preload.world=world;runtime_guest_preload.attempted=runtime_guest_preload.inflight=1;
    runtime_guest_preload.dispatched=invoke(base,game,"SFXGame","LoadPackage",&params);
    runtime_guest_preload.inflight=0;
    char row[340];snprintf(row,sizeof row,
        "{\"runtime_network\":\"guest-archetype-preload\",\"package\":\"BioChar_Player\",\"native_api_dispatched\":%s,\"package_map_written\":false,\"connection_verified\":false,\"monotonic_ms\":%lu}\r\n",
        runtime_guest_preload.dispatched?"true":"false",GetTickCount());line(row);
    return 0; /* Discard this sample; refresh world/player readiness next Tick. */
}
#endif
