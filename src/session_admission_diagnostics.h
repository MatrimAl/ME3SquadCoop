#ifndef ME3_SESSION_ADMISSION_DIAGNOSTICS_H
#define ME3_SESSION_ADMISSION_DIAGNOSTICS_H
#ifdef ME3_PROTECTED_SESSION_ADMISSION_DIAGNOSTICS
/* Protected observer only. Never assigns scope or feeds a possession predicate.
 * No discovery scan: counters come from the existing pair_find_remote scan.
 * At most 120 rows/process, >=2 seconds apart, including readiness failures. */
typedef struct SessionAdmissionBudget {DWORD last;unsigned rows;} SessionAdmissionBudget;
static SessionAdmissionBudget session_admission_budget;
static int session_admission_take(SessionAdmissionBudget *b,DWORD now) {
    if(b->rows>=120||(b->rows&&now-b->last<2000))return 0;
    b->last=now;++b->rows;return 1;
}
typedef struct SessionAdmissionFacts {
    int remote_present,remote_actor,remote_kind,remote_world,flags_read,pending;
    int connection_live,connection_kind,reciprocal,member,remote_pawn;
} SessionAdmissionFacts;
/* 0 eligible; 1 absent; 2 disposal; 3 kind; 4 world; 5 unreadable flags;
 * 6 pending; 7 connection disposal; 8 connection kind; 9 reciprocity;
 * 10 unknown membership; 11 absent membership; 12 already possessed. */
static int session_admission_remote_code(const SessionAdmissionFacts *f) {
    if(!f->remote_present)return 1;if(!f->remote_actor)return 2;
    if(!f->remote_kind)return 3;if(!f->remote_world)return 4;
    if(!f->flags_read)return 5;if(f->pending)return 6;
    if(!f->connection_live)return 7;if(!f->connection_kind)return 8;
    if(!f->reciprocal)return 9;if(f->member<0)return 10;
    if(f->member!=1)return 11;if(f->remote_pawn)return 12;return 0;
}
static int session_admission_format(char *row,size_t capacity,const int *v,DWORD now) {
    int n=snprintf(row,capacity,"{\"session\":\"admission-gates\",\"host_ready\":%d,\"fault\":%d,\"enabled\":%d,\"engine_blocked\":%d,\"synthetic_blocked\":%d,\"mode\":%d,\"pc_present\":%d,\"host_pawn_actor\":%d,\"host_pawn_kind\":%d,\"squad_live\":%d,\"layout_pinned\":%d,\"current_chain_valid\":%d,\"current_chain_stable\":%d,\"world_matches_context\":%d,\"cached_driver_live\":%d,\"current_driver_live\":%d,\"drivers_equal\":%d,\"cached_clients_read\":%d,\"cached_client_count\":%d,\"current_clients_read\":%d,\"current_client_count\":%d,\"discovery_current\":%d,\"discovery_snapshot_valid\":%d,\"discovery_class_count\":%d,\"discovery_kind_count\":%d,\"discovery_world_count\":%d,\"discovery_reciprocal_count\":%d,\"remote_present\":%d,\"remote_gate_code\":%d,\"remote_has_pawn\":%d,\"connection_membership\":%d,\"free_delay_ms\":%d,\"read_only\":true,\"monotonic_ms\":%lu}\r\n",
        v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7],v[8],v[9],v[10],v[11],v[12],v[13],v[14],v[15],v[16],v[17],v[18],v[19],v[20],v[21],v[22],v[23],v[24],v[25],v[26],v[27],v[28],v[29],v[30],v[31],now);
    return n>0&&(size_t)n<capacity;
}
static void session_admission_diagnostic(uintptr_t base,SessionContext c,DWORD now) {
    if(!session_admission_take(&session_admission_budget,now))return;
    CombatContextChain first,last;combat_context_read_chain(base,&first);
    uintptr_t current=0;int driver_read=first.world_valid&&read_at(first.world+0x120,&current,8);
    int cached_live=is_live(base,identity_host_driver)&&is_kind_of(base,identity_host_driver,"NetDriver");
    int current_live=driver_read&&is_live(base,current)&&is_kind_of(base,current,"NetDriver");
    GameArray cached_clients={0},current_clients={0};
    int cached_read=cached_live&&identity_array(identity_host_driver+0x70,&cached_clients,32);
    int current_read=current_live&&identity_array(current+0x70,&current_clients,32);
    uintptr_t remote=session_remote,connection=read_ptr(remote+0x67c);uint32_t flags=0;
    SessionAdmissionFacts f={0};f.remote_present=remote!=0;f.remote_actor=session_actor_live(base,remote);
    f.remote_kind=f.remote_actor&&is_kind_of(base,remote,"BioPlayerController");
    f.remote_world=f.remote_actor&&read_ptr(remote+0x198)==c.world;
    f.flags_read=f.remote_actor&&read_at(remote+0x77c,&flags,4);f.pending=!!(flags&0x800);
    f.connection_live=is_live(base,connection);f.connection_kind=f.connection_live&&is_kind_of(base,connection,"NetConnection");
    f.reciprocal=f.connection_live&&read_ptr(connection+0x68)==remote;
    f.member=session_connection_member(base,connection);f.remote_pawn=read_ptr(remote+0x3bc)!=0;
    combat_context_read_chain(base,&last);uintptr_t current_after=0;
    int stable=combat_context_chain_equal(&first,&last)&&driver_read&&read_at(last.world+0x120,&current_after,8)&&current_after==current;
    int discovery=pair_remote_discovery.host==c.pc&&pair_remote_discovery.world==c.world&&
        now-pair_remote_discovery.at<=2500;
    int v[32]={session_policy_host_ready(),session_fault,session_policy_enabled(),session_engine_blocked(c),
        session_policy_synthetic(session_first_acquired,now),c.mode,c.pc!=0,session_actor_live(base,c.pawn),
        is_kind_of(base,c.pawn,"SFXPawn_Player"),is_live(base,c.squad),combat_context_layout(base),first.info_valid,
        stable,first.info_valid&&first.info==c.world,cached_live,current_live,current&&current==identity_host_driver,
        cached_read,cached_read?cached_clients.count:-1,current_read,current_read?current_clients.count:-1,
        discovery,discovery?pair_remote_discovery.snapshot_valid:0,discovery?pair_remote_discovery.class_prefilter:-1,
        discovery?pair_remote_discovery.controller_kind:-1,discovery?pair_remote_discovery.world_matches:-1,
        discovery?pair_remote_discovery.reciprocal_matches:-1,f.remote_present,session_admission_remote_code(&f),f.remote_pawn,f.member,
        session_free_at?(int)(now-session_free_at):-1};
    char row[2048];if(session_admission_format(row,sizeof row,v,now))line(row);
}
static int session_admission_diagnostics_selftest(void) {
    SessionAdmissionBudget b={0};if(!session_admission_take(&b,0)||session_admission_take(&b,1999)||!session_admission_take(&b,2000))return 0;
    b.last=UINT32_MAX-999;b.rows=2;if(session_admission_take(&b,999)||!session_admission_take(&b,1000))return 0;
    b.rows=119;b.last=0;if(!session_admission_take(&b,2000)||b.rows!=120||session_admission_take(&b,4000))return 0;
    SessionAdmissionFacts f={1,1,1,1,1,0,1,1,1,1,0};if(session_admission_remote_code(&f))return 0;
    for(int code=1;code<=12;++code) {SessionAdmissionFacts x=f;
        switch(code){case 1:x.remote_present=0;break;case 2:x.remote_actor=0;break;case 3:x.remote_kind=0;break;
        case 4:x.remote_world=0;break;case 5:x.flags_read=0;break;case 6:x.pending=1;break;
        case 7:x.connection_live=0;break;case 8:x.connection_kind=0;break;case 9:x.reciprocal=0;break;
        case 10:x.member=-1;break;case 11:x.member=0;break;case 12:x.remote_pawn=1;break;}
        if(session_admission_remote_code(&x)!=code)return 0;}
    f.member=-1;f.remote_pawn=1;if(session_admission_remote_code(&f)!=10)return 0;
    f.reciprocal=0;if(session_admission_remote_code(&f)!=9)return 0;
    int values[32];for(unsigned i=0;i<32;++i)values[i]=INT32_MIN;
    char row[2048],small[32];return session_admission_format(row,sizeof row,values,UINT32_MAX)&&
        !session_admission_format(small,sizeof small,values,UINT32_MAX)&&strstr(row,"\"read_only\":true")&&strlen(row)<sizeof row;
}
#endif
#endif
