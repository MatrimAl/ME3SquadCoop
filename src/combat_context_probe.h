#ifndef ME3_COMBAT_CONTEXT_PROBE_H
#define ME3_COMBAT_CONTEXT_PROBE_H

/* Read-only, <=1Hz; include after narrative_probe.h. No combat action is
 * invoked and no permission/squad/player pointers are written.
 * Exact private EXE b453871e...: native gate a5a9b0..a5aa0e; exec wrapper
 * b75250..b75296 calls it atb75284 and writes its bool result.
 * SDK: UWorld.PersistentLevel88; ULevelBase.Actors60;
 * BioWorldInfo.m_playerSquad8e8; BioBaseSquad.CachedPlayerPawn2a8,
 * flags2b0 combatEnabled4; BioPawn.flagsE18 combatPawn8.
 * BioPlayerInput.IsCombatEnabled: flags20401, returnbool0,size4.
 * See private LEMP-combat-gate-research.md. Pawn.Squad is a comparison,
 * never substituted for the actual native GWorld-derived squad.
 */
typedef struct CombatContextChain {
    uintptr_t world,level,info,squad,cached;
    GameArray actors;
    uint32_t squad_flags,pawn_flags;
    int world_read,world_valid,level_read,level_valid,actors_read,actors_valid;
    int info_read,info_valid,squad_read,squad_valid,cached_read,cached_valid;
    int squad_flags_read,pawn_flags_read;
} CombatContextChain;
typedef struct CombatContextGetter {
    uintptr_t base,input_class,function,owner;
    int32_t index,owner_index;
    uint64_t name,owner_name;
} CombatContextGetter;
static CombatContextGetter combat_context_getter_cache;
static DWORD combat_context_last_sample;
static int combat_context_sampled;
static int combat_context_actor(uintptr_t base,uintptr_t object,const char *kind) {
    return scene_readiness_actor(base,object)&&is_kind_of(base,object,kind);
}
static int combat_context_local_pc(uintptr_t base,uintptr_t engine,uintptr_t *pc) {
    GameArray players={0};uintptr_t player=0;*pc=0;
    if(!narrative_probe_object(base,engine,"Engine")||!read_at(engine+0x498,&players,sizeof players)||
       players.count<0||players.count>4||players.max<players.count||players.max>128||
       (players.count&&!players.data))return 0;
    if(!players.count)return 1;
    if(!read_at(players.data,&player,8))return 0;
    if(!player)return 1;
    return narrative_probe_object(base,player,"LocalPlayer")&&read_at(player+0x68,pc,8);
}
static int combat_context_layout(uintptr_t base) {
    static const unsigned char gate[]={0x48,0x83,0xec,0x28,0x48,0x8b,0x05,0x7d,0xbc,0xe5,0x00,0x48,0x8b,0x88,0x88,0x00,0x00,0x00,0x48,0x8b,0x41,0x60,0x48,0x8b,0x08,0xe8,0xf2,0x28,0x74,0xff,0x48,0x85,0xc0,0x74,0x34,0x48,0x8b,0x80,0xe8,0x08,0x00,0x00,0x48,0x85,0xc0,0x74,0x28,0x48,0x8b,0x88,0xa8,0x02,0x00,0x00,0x48,0x85,0xc9,0x74,0x1c,0xf6,0x80,0xb0,0x02,0x00,0x00,0x04,0x74,0x13,0xf6,0x81,0x18,0x0e,0x00,0x00,0x08,0x74,0x0a,0xb8,0x01,0x00,0x00,0x00,0x48,0x83,0xc4,0x28,0xc3,0x33,0xc0,0x48,0x83,0xc4,0x28,0xc3};
    static const unsigned char wrapper[]={0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20,0x48,0xff,0x42,0x28,0x49,0x8b,0xd8,0x48,0x8b,0x42,0x28,0x48,0x8b,0xf9,0x80,0x38,0x41,0x75,0x14,0x48,0x8b,0x4a,0x20,0x48,0xff,0xc0,0x45,0x33,0xc0,0x48,0x89,0x42,0x28,0xff,0x15,0x37,0xfb,0xc7,0x00,0x48,0x8b,0xcf,0xe8,0x27,0x57,0xee,0xff,0x89,0x03,0x48,0x8b,0x5c,0x24,0x30,0x48,0x83,0xc4,0x20,0x5f,0xc3};
    unsigned char a[sizeof gate],b[sizeof wrapper];
    return read_at(base+0xa5a9b0,a,sizeof a)&&!memcmp(a,gate,sizeof a)&&
        read_at(base+0xb75250,b,sizeof b)&&!memcmp(b,wrapper,sizeof b);
}
static void combat_context_read_chain(uintptr_t base,CombatContextChain *s) {
    memset(s,0,sizeof *s);
    s->world_read=read_at(base+0x18b6638,&s->world,8);
    s->world_valid=s->world_read&&narrative_probe_object(base,s->world,"World");
    s->level_read=s->world_valid&&read_at(s->world+0x88,&s->level,8);
    s->level_valid=s->level_read&&narrative_probe_object(base,s->level,"Level");
    s->actors_read=s->level_valid&&read_at(s->level+0x60,&s->actors,sizeof s->actors);
    s->actors_valid=s->actors_read&&s->actors.count>=0&&s->actors.count<=100000&&
        s->actors.max>=s->actors.count&&s->actors.max<=500000&&(!s->actors.count||s->actors.data);
    s->info_read=s->actors_valid&&s->actors.count>0&&read_at(s->actors.data,&s->info,8);
    s->info_valid=s->info_read&&combat_context_actor(base,s->info,"BioWorldInfo");
    s->squad_read=s->info_valid&&read_at(s->info+0x8e8,&s->squad,8);
    s->squad_valid=s->squad_read&&combat_context_actor(base,s->squad,"BioBaseSquad");
    s->cached_read=s->squad_valid&&read_at(s->squad+0x2a8,&s->cached,8);
    s->cached_valid=s->cached_read&&combat_context_actor(base,s->cached,"BioPawn");
    s->squad_flags_read=s->squad_valid&&read_at(s->squad+0x2b0,&s->squad_flags,4);
    s->pawn_flags_read=s->cached_valid&&read_at(s->cached+0xe18,&s->pawn_flags,4);
}
static int combat_context_safe_native(const CombatContextChain *s) {
    /* Native dereferences World/Level/Actors before its first null guard.
     * Guard even the branches which would return false, rather than using
     * an invalid loading/disposal chain as a callable input getter. */
    return s->info_valid&&s->squad_read&&(!s->squad||
        (s->squad_valid&&s->cached_read&&(!s->cached||
         (s->cached_valid&&s->squad_flags_read&&s->pawn_flags_read))));
}
static int combat_context_derived(const CombatContextChain *s,int pinned,int stable) {
    if(!pinned||!stable||!s->info_valid||!s->squad_read)return -1;
    if(!s->squad)return 0;
    if(!s->squad_valid||!s->cached_read)return -1;
    if(!s->cached)return 0;
    if(!s->squad_flags_read)return -1;
    if(!(s->squad_flags&4))return 0;
    if(!s->cached_valid||!s->pawn_flags_read)return -1;
    return (s->pawn_flags&8)?1:0;
}
static int combat_context_chain_equal(const CombatContextChain *a,const CombatContextChain *b) {
    /* Both snapshots are zero initialized; padding remains deterministic.
     * Includes array header, pointer identity, read/type validity and bits. */
    return !memcmp(a,b,sizeof *a);
}
static int combat_context_get(uintptr_t base,uintptr_t input,uint32_t *result) {
    if(!narrative_probe_object(base,input,"BioPlayerInput"))return 0;
    uint64_t object_flags=0;
    if(!read_at(input+0xc,&object_flags,8)||(object_flags&(UINT64_C(1)<<61)))return 0;
    uintptr_t klass=read_ptr(input+0x50),fn=0;CombatContextGetter *c=&combat_context_getter_cache;
    int32_t index=-1,owner_index=-1;uint64_t name=0,owner_name=0;
    if(c->base==base&&c->input_class==klass&&is_live(base,c->function)&&is_live(base,c->owner)&&
       read_at(c->function+8,&index,4)&&index==c->index&&read_at(c->function+0x48,&name,8)&&name==c->name&&
       read_at(c->owner+8,&owner_index,4)&&owner_index==c->owner_index&&read_at(c->owner+0x48,&owner_name,8)&&owner_name==c->owner_name&&
       read_ptr(c->function+0x40)==c->owner)fn=c->function;
    if(!fn) {
        uintptr_t seen_classes[16];int classes=0;
        while(is_live(base,klass)&&classes<16&&!fn) {
            for(int i=0;i<classes;++i)if(seen_classes[i]==klass)return 0;
            seen_classes[classes++]=klass;char text[96];object_name(base,klass,text,sizeof text);
            if(!strcmp(text,"BioPlayerInput")) {
                uintptr_t child=read_ptr(klass+0x70),seen[256];int count=0;
                while(child&&count<256) {
                    if(!is_live(base,child))return 0;
                    for(int i=0;i<count;++i)if(seen[i]==child)return 0;
                    seen[count++]=child;object_name(base,child,text,sizeof text);
                    if(!strcmp(text,"IsCombatEnabled")&&is_kind_of(base,child,"Function")&&read_ptr(child+0x40)==klass){fn=child;break;}
                    child=read_ptr(child+0x68);
                }
            }
            klass=read_ptr(klass+0x60);
        }
    }
    if(!narrative_probe_object(base,fn,"Function"))return 0;
    uintptr_t owner=read_ptr(fn+0x40),native=0;uint32_t flags=0;uint16_t size=0,offset=0;unsigned char count=0;
    char fn_name[96],class_name[96];object_name(base,fn,fn_name,sizeof fn_name);
    if(!narrative_probe_object(base,owner,"Class"))return 0;
    object_name(base,owner,class_name,sizeof class_name);
    if(strcmp(fn_name,"IsCombatEnabled")||strcmp(class_name,"BioPlayerInput")||
       !read_at(fn+0xe0,&flags,4)||flags!=0x20401||!read_at(fn+0xe6,&size,2)||size!=4||
       !read_at(fn+0xe8,&offset,2)||offset!=0||!read_at(fn+0xea,&count,1)||count!=1||
       !read_at(fn+0xd8,&native,8)||native!=base+0xb75250||
       !read_at(fn+8,&index,4)||!read_at(fn+0x48,&name,8)||
       !read_at(owner+8,&owner_index,4)||!read_at(owner+0x48,&owner_name,8))return 0;
    uintptr_t pe=read_ptr(read_ptr(input)+0x230);
    if(pe<base||pe>=base+0x1300000)return 0;
    c->base=base;c->input_class=read_ptr(input+0x50);c->function=fn;c->owner=owner;
    c->index=index;c->name=name;c->owner_index=owner_index;c->owner_name=owner_name;
    *result=UINT32_C(0x7fcdeeed); /* Distinguish a rejected/no-op PE from false. */
    ((ProcessEvent)pe)((void*)input,(void*)fn,result,NULL);return 1;
}
static void combat_context_sample(uintptr_t base,uintptr_t engine,DWORD now) {
    if(combat_context_sampled&&now-combat_context_last_sample<1000)return;
    combat_context_sampled=1;combat_context_last_sample=now;
    DWORD started=GetTickCount();int pinned=combat_context_layout(base);
    uintptr_t pc=0,pawn=0,ack=0,input=0,pc_world=0,own_squad=0,local_squad=0,game=0,game_squad=0;
    int pc_read=combat_context_local_pc(base,engine,&pc);
    int pc_valid=combat_context_actor(base,pc,"PlayerController");
    int pawn_read=pc_valid&&read_at(pc+0x3bc,&pawn,8),ack_read=pc_valid&&read_at(pc+0x694,&ack,8);
    int input_read=pc_valid&&read_at(pc+0x6dc,&input,8),world_read=pc_valid&&read_at(pc+0x198,&pc_world,8);
    int pawn_valid=pawn_read&&combat_context_actor(base,pawn,"BioPawn");
    int world_valid=world_read&&combat_context_actor(base,pc_world,"BioWorldInfo");
    int own_read=pawn_valid&&read_at(pawn+0xba8,&own_squad,8);
    int local_squad_read=world_valid&&read_at(pc_world+0x8e8,&local_squad,8);
    int game_read=world_valid&&read_at(pc_world+0x6b0,&game,8);
    int game_valid=game_read&&combat_context_actor(base,game,"SFXGame");
    int game_squad_read=game_valid&&read_at(game+0x7e4,&game_squad,8);
    unsigned char net=255,mode=255;uintptr_t modes=0;
    int net_read=world_valid&&read_at(pc_world+0x7c8,&net,1);
    int modes_read=pc_valid&&read_at(pc+0xc38,&modes,8)&&narrative_probe_object(base,modes,"SFXGameModeManager")&&read_at(modes+0xa8,&mode,1);
    CombatContextChain before,after;combat_context_read_chain(base,&before);
    int prefix_safe=pinned&&combat_context_safe_native(&before);
    /* Take a second complete native-chain snapshot immediately before PE. */
    combat_context_read_chain(base,&after);
    int pre_stable=combat_context_chain_equal(&before,&after);
    uint32_t value=UINT32_C(0x7fcdeeed);
    int dispatched=prefix_safe&&pre_stable&&pc_valid&&input_read&&combat_context_get(base,input,&value);
    int result_valid=dispatched&&value<=1;
    combat_context_read_chain(base,&after);
    int native_stable=combat_context_chain_equal(&before,&after);
    uintptr_t check_pc=0,check_pawn=0,check_input=0,check_world=0,check_ack=0;
    int local_stable=pc_valid&&combat_context_local_pc(base,engine,&check_pc)&&check_pc==pc&&read_at(pc+0x3bc,&check_pawn,8)&&check_pawn==pawn&&
        read_at(pc+0x6dc,&check_input,8)&&check_input==input&&read_at(pc+0x198,&check_world,8)&&check_world==pc_world&&
        read_at(pc+0x694,&check_ack,8)&&check_ack==ack;
    int derived=combat_context_derived(&before,pinned,native_stable);
    char a[1600],b[1600],c[1600],d[1600],e[1600],row[10000];
    narrative_probe_identity(base,pc,pc_read,a,sizeof a);narrative_probe_identity(base,pawn,pawn_read,b,sizeof b);
    narrative_probe_identity(base,ack,ack_read,c,sizeof c);narrative_probe_identity(base,input,input_read,d,sizeof d);
    narrative_probe_identity(base,pc_world,world_read,e,sizeof e);
    snprintf(row,sizeof row,"{\"combat_context_probe\":\"local-context\",\"pc\":%s,\"pawn\":%s,\"acknowledged_pawn\":%s,\"input\":%s,\"pc_world_info\":%s,\"pc_class_valid\":%s,\"pawn_class_valid\":%s,\"input_class_valid\":%s,\"reciprocal_possession\":%s,\"acknowledged_pawn_matches\":%s,\"net_mode\":%u,\"net_mode_read_valid\":%s,\"current_mode\":%u,\"mode_read_valid\":%s,\"local_snapshot_stable\":%s,\"monotonic_ms\":%lu}\r\n",
        a,b,c,d,e,narrative_probe_bool(1,pc_valid),narrative_probe_bool(1,pawn_valid),
        narrative_probe_bool(input_read,narrative_probe_object(base,input,"BioPlayerInput")),
        narrative_probe_bool(pc_valid&&pawn_valid,read_ptr(pawn+0x374)==pc),
        narrative_probe_bool(ack_read&&pawn_read,ack==pawn&&pawn),net,narrative_probe_bool(1,net_read),mode,narrative_probe_bool(1,modes_read),narrative_probe_bool(1,local_stable),now);line(row);
    narrative_probe_identity(base,before.world,before.world_read,a,sizeof a);
    narrative_probe_identity(base,before.level,before.level_read,b,sizeof b);
    narrative_probe_identity(base,before.info,before.info_read,c,sizeof c);
    snprintf(row,sizeof row,"{\"combat_context_probe\":\"native-world-chain\",\"layout_pinned\":%s,\"gworld\":%s,\"persistent_level\":%s,\"first_actor_world_info\":%s,\"world_class_valid\":%s,\"level_class_valid\":%s,\"actors_array_read_valid\":%s,\"actors_array_valid\":%s,\"actors_count\":%d,\"world_info_class_valid\":%s,\"same_as_pc_world_info\":%s,\"native_dereferences_safe\":%s,\"pre_getter_stable\":%s,\"post_getter_stable\":%s,\"monotonic_ms\":%lu}\r\n",
        narrative_probe_bool(1,pinned),a,b,c,narrative_probe_bool(1,before.world_valid),narrative_probe_bool(1,before.level_valid),
        narrative_probe_bool(1,before.actors_read),narrative_probe_bool(1,before.actors_valid),before.actors_read?before.actors.count:-1,
        narrative_probe_bool(1,before.info_valid),narrative_probe_bool(before.info_read&&world_read,before.info==pc_world),
        narrative_probe_bool(1,prefix_safe),narrative_probe_bool(1,pre_stable),narrative_probe_bool(1,native_stable),now);line(row);
    narrative_probe_identity(base,before.squad,before.squad_read,a,sizeof a);
    narrative_probe_identity(base,before.cached,before.cached_read,b,sizeof b);
    snprintf(row,sizeof row,"{\"combat_context_probe\":\"native-gate\",\"world_player_squad\":%s,\"cached_player_pawn\":%s,\"squad_class_valid\":%s,\"cached_pawn_class_valid\":%s,\"squad_flags_read_valid\":%s,\"squad_flags_raw\":%u,\"combat_enabled_bit\":%s,\"is_player_squad_bit\":%s,\"squad_enabled_bit\":%s,\"cached_pawn_flags_read_valid\":%s,\"cached_pawn_flags_raw\":%u,\"combat_pawn_bit\":%s,\"cached_is_local_pawn\":%s,\"derived_gate\":%s,\"getter_dispatched\":%s,\"getter_result_valid\":%s,\"getter_result\":%s,\"getter_agrees_with_derived\":%s,\"firing_tested\":false,\"monotonic_ms\":%lu}\r\n",
        a,b,narrative_probe_bool(1,before.squad_valid),narrative_probe_bool(1,before.cached_valid),
        narrative_probe_bool(1,before.squad_flags_read),before.squad_flags,narrative_probe_bool(before.squad_flags_read,before.squad_flags&4),
        narrative_probe_bool(before.squad_flags_read,before.squad_flags&2),narrative_probe_bool(before.squad_flags_read,before.squad_flags&8),
        narrative_probe_bool(1,before.pawn_flags_read),before.pawn_flags,narrative_probe_bool(before.pawn_flags_read,before.pawn_flags&8),
        narrative_probe_bool(before.cached_read&&pawn_read,before.cached==pawn&&pawn),narrative_probe_bool(derived>=0,derived>0),
        narrative_probe_bool(1,dispatched),narrative_probe_bool(1,result_valid),narrative_probe_bool(result_valid,value==1),
        narrative_probe_bool(result_valid&&derived>=0&&native_stable,(int)value==derived),now);line(row);
    narrative_probe_identity(base,own_squad,own_read,a,sizeof a);narrative_probe_identity(base,local_squad,local_squad_read,b,sizeof b);
    narrative_probe_identity(base,game,game_read,c,sizeof c);narrative_probe_identity(base,game_squad,game_squad_read,d,sizeof d);
    snprintf(row,sizeof row,"{\"combat_context_probe\":\"context-comparison\",\"controlled_pawn_squad\":%s,\"pc_world_player_squad\":%s,\"game_info\":%s,\"game_class_sfxgame_valid\":%s,\"game_player_squad\":%s,\"own_squad_matches_native\":%s,\"pc_world_squad_matches_native\":%s,\"game_squad_matches_native\":%s,\"sampling_duration_ms\":%lu,\"read_only\":true,\"firing_tested\":false,\"monotonic_ms\":%lu}\r\n",
        a,b,c,narrative_probe_bool(1,game_valid),d,narrative_probe_bool(own_read&&before.squad_read,own_squad==before.squad),
        narrative_probe_bool(local_squad_read&&before.squad_read,local_squad==before.squad),
        narrative_probe_bool(game_squad_read&&before.squad_read,game_squad==before.squad),GetTickCount()-started,now);line(row);
}
#endif
