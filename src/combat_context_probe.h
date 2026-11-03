#ifndef ME3_COMBAT_CONTEXT_PROBE_H
#define ME3_COMBAT_CONTEXT_PROBE_H
#include "combat_context_support.h"
static DWORD combat_context_last_sample;
static int combat_context_sampled;
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
