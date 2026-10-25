#ifndef ME3_SESSION_TRAVEL_RETIREMENT_H
#define ME3_SESSION_TRAVEL_RETIREMENT_H
/* Positive disposal evidence, not !is_live or a timeout. An unreadable table
 * or object is unknown and cannot retire a native ownership lease. */
static int session_registry_retired(uintptr_t base,SessionHandle saved,const GameArray *objects) {
    (void)base;
    if(!saved.pointer||saved.index<0||saved.index>=objects->count)return 0;
    uintptr_t current=0;
    if(!read_at(objects->data+(size_t)saved.index*8,&current,8))return 0;
    /* A rename at the same registered address is not disposal evidence.
     * Require the old object's actual registry slot to stop holding it. */
    return current!=saved.pointer;
}
static int session_travel_old_graph_gone(uintptr_t base,SessionContext c,const GameArray *objects) {
    if(!session_registry_retired(base,session_world_handle,objects)||
       !session_registry_retired(base,session_pawn_handle,objects))return 0;
    if(session_registry_retired(base,session_pc_handle,objects))return 1;
    /* Native seamless travel may retain the remote controller. It must now
     * belong reciprocally to the new world's actual connection and no longer
     * reference the disposed companion; never UnPossess this new context. */
    uintptr_t pawn=0;
    return session_handle_matches(session_pc_handle)&&session_actor_live(base,session_remote)&&
        read_ptr(session_remote+0x198)==c.world&&read_at(session_remote+0x3bc,&pawn,8)&&
        pawn!=session_pawn&&session_peer_connected(base,session_remote,c.world);
}
static int session_retire_disposed_travel(uintptr_t base,SessionContext c,DWORD now) {
    if(!session_active||session_releasing||session_fault||session_cleanup_pending||session_pending_reason||
       !session_policy_is_host()||!session_policy_enabled()||!session_engine||!c.world||c.world==session_world||
       !session_actor_live(base,c.pc)||!session_actor_live(base,c.pawn)||!session_actor_live(base,c.world)||
       !is_kind_of(base,c.pawn,"SFXPawn_Player")||read_ptr(c.pc+0x3bc)!=c.pawn||read_ptr(c.pawn+0x374)!=c.pc||
       !session_actor_live(base,c.squad)||read_ptr(c.squad+0x2cc)!=c.pawn)return 0;
    CombatContextChain before,after;combat_context_read_chain(base,&before);
    if(!before.world_valid||!before.level_valid||!before.info_valid||before.info!=c.world||
       !before.squad_read||before.squad!=c.squad||local_controller(session_engine)!=c.pc)return 0;
    SessionHandle pc=session_capture_handle(c.pc),pawn=session_capture_handle(c.pawn),
        info=session_capture_handle(c.world),squad=session_capture_handle(c.squad);
    GameArray objects={0},again={0};
    if(!read_at(base+OBJECTS_RVA,&objects,sizeof objects)||!objects.data||objects.count<=0||
       objects.count>2000000||objects.max<objects.count||objects.max>4000000||
       !session_travel_old_graph_gone(base,c,&objects))return 0;
    combat_context_read_chain(base,&after);
    if(!combat_context_chain_equal(&before,&after)||!session_handle_matches(pc)||!session_handle_matches(pawn)||
       !session_handle_matches(info)||!session_handle_matches(squad)||local_controller(session_engine)!=c.pc||
       read_ptr(c.pc+0x3bc)!=c.pawn||read_ptr(c.pawn+0x374)!=c.pc||read_ptr(c.squad+0x2cc)!=c.pawn||
       !read_at(base+OBJECTS_RVA,&again,sizeof again)||memcmp(&objects,&again,sizeof objects)||
       !session_travel_old_graph_gone(base,c,&again))return 0;
#ifdef ME3_PROTECTED_GUEST_TEAM_QA
    /* Preserve the probe's own scoped team-lease cleanup; its implementation
     * refuses old-world native writes and retires changed identities. */
    guest_team_qa_release(base,now);
#endif
    char row[420];snprintf(row,sizeof row,
        "{\"session\":\"travel-lease-retired\",\"epoch\":%u,\"old_world_and_pawn_disposed\":true,\"native_possession_calls\":0,\"old_world_written\":false,\"ai_restoration_claimed\":false,\"campaign_travel_verified\":false,\"monotonic_ms\":%lu}\r\n",
        session_epoch,now);line(row);
    /* Retire mod bookkeeping only. Genuine fault/cleanup paths never enter
     * here, and no fault is cleared. New acquisition still needs full ordinary
     * admission, a real peer, live squad AI and its fresh stability window. */
    session_active=0;session_release_failures=0;session_remote=session_connection=0;
    session_pawn=session_ai=session_host=session_shepard=session_world=session_squad=0;
    session_story_player=session_story_initial=0;session_admission_wait_reset();
    return 1;
}
#endif
