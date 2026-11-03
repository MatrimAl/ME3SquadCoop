#ifndef ME3_RUNTIME_REVIVE_PEER_H
#define ME3_RUNTIME_REVIVE_PEER_H
#include "revive_peer_protocol.h"
/* Admission for the existing native Used/StartRevive path. This protocol
 * carries readiness only; it never carries use targets, health or actions. */
typedef struct {
    RevivePeerMessage message;
    GuestCameraStamp pc,world,connection,driver,channel;
    unsigned long registration;
    DWORD received_at;
    int active;
} RuntimeRevivePeerInbox;
typedef struct {
    RuntimeRevivePair pair;
    uint64_t guest_nonce,host_nonce;
    DWORD peer_at,sent_at,phase_at;
    int phase,activated,confirmed;
} RuntimeRevivePeerState;
static RuntimeRevivePeerInbox runtime_revive_peer_inbox;
static RuntimeRevivePeerState runtime_revive_peer_state;
static uint64_t runtime_revive_peer_serial;
static unsigned runtime_revive_peer_events;
static int runtime_revive_peer_enabled_now(void) {
    return runtime_revive_player_opted()&&presentation_installed&&runtime_rpc_receive_installed;
}
static void runtime_revive_peer_note(const char *event) {
    if(runtime_revive_peer_events++>=128)return;
    RuntimeRevivePeerState *s=&runtime_revive_peer_state;char row[340];
    snprintf(row,sizeof row,"{\"revive_peer\":\"%s\",\"role\":%d,\"phase\":%d,"
        "\"pair_generation\":%llu,\"shepard_channel\":%d,\"ashley_channel\":%d,\"monotonic_ms\":%lu}\r\n",
        event,(int)g_runtime_config.role,s->phase,(unsigned long long)s->pair.generation,
        s->pair.scope[0].channel_index,s->pair.scope[1].channel_index,GetTickCount());line(row);
}
static int runtime_revive_peer_pair_message(const RuntimeRevivePair *p,const RevivePeerMessage *m) {
    return p->generation&&m->shepard_channel==(unsigned)p->scope[0].channel_index&&
        m->ashley_channel==(unsigned)p->scope[1].channel_index&&!memcmp(m->guid,p->package_guid,16);
}
/* Called after presentation_source has checked the real ReceivedBunch frame.
 * No ProcessEvent, preparation, activation or retirement in this callback. */
static int runtime_revive_peer_receive(uintptr_t self,uintptr_t frame,int k,const wchar_t *text,
    size_t count,const NetworkActorIdentityEntry *source) {
    (void)frame;(void)k;
    RuntimeRevivePeerInbox next={0};
    if(!runtime_revive_peer_enabled_now()||!revive_peer_parse(text,count,&next.message))return 1;
    next.registration=g_runtime_config.role==ME3_ROLE_HOST?session_epoch:guest_player_context_registration_token();
    uintptr_t values[]={self,read_ptr(self+0x198),source->connection,source->driver,source->channel};
    GuestCameraStamp *stamps[]={&next.pc,&next.world,&next.connection,&next.driver,&next.channel};
    if(!next.registration)return 1;
    for(unsigned i=0;i<5;++i)if(!guest_camera_stamp_read(presentation_base,values[i],stamps[i],1))return 1;
    next.received_at=GetTickCount();next.active=1;runtime_revive_peer_inbox=next;return 1;
}
static void runtime_revive_peer_configure(void) {
    presentation_peer_enabled=runtime_revive_player_opted;
    presentation_peer_receive=runtime_revive_peer_receive;
    runtime_revive_peer_enabled=runtime_revive_peer_enabled_now;
}
static int runtime_revive_peer_inbox_valid(uintptr_t base,const RuntimeRevivePeerInbox *r,
    const RuntimeRevivePair *p,DWORD now) {
    if(!r->active||now-r->received_at>3000||r->registration!=p->scope[0].registration||
       !guest_camera_stamp_core_equal(&r->world,&p->scope[0].world)||
       !guest_camera_stamp_core_equal(&r->connection,&p->scope[0].connection)||
       !guest_camera_stamp_core_equal(&r->driver,&p->scope[0].driver))return 0;
    const GuestCameraStamp *stamps[]={&r->pc,&r->world,&r->connection,&r->driver,&r->channel};
    for(unsigned i=0;i<5;++i)if(!guest_camera_stamp_matches(base,stamps[i],1))return 0;
    uintptr_t pc=g_runtime_config.role==ME3_ROLE_HOST?session_remote:p->scope[0].pc.pointer;
    NetworkActorIdentityEntry current={0};
    return r->pc.pointer==pc&&read_ptr(r->connection.pointer+0x68)==pc&&
        network_actor_channel_valid(base,pc,r->channel.pointer,r->driver.pointer,&current)&&
        current.connection==r->connection.pointer;
}
static int runtime_revive_peer_still_owns(const RuntimeRevivePair *pair) {
    if(!pair->generation)return 0;
    for(unsigned i=0;i<2;++i) {
        RuntimeRevivePreparation *q=&runtime_revive_preparations[runtime_revive_canonical_slot(i)];
        if(q->token!=pair->token[i]||!runtime_revive_scope_same(&q->scope,&pair->scope[i])||
           !guest_camera_stamp_core_equal(&q->service,&pair->service[i]))return 0;
    }
    return 1;
}
static void runtime_revive_peer_reset(uintptr_t base,uintptr_t engine,DWORD now,int retire) {
    RuntimeRevivePeerState *s=&runtime_revive_peer_state;
    if(s->phase)runtime_revive_peer_note(retire?"scope-retired":"agreement-reset");
    /* A later registration may already have replaced a fully retired slot.
     * An old protocol state must never close that newly prepared lifetime. */
    if(s->activated&&runtime_revive_peer_still_owns(&s->pair)) {
        if(retire)runtime_revive_pair_revoke(base,engine,now);
        else runtime_revive_pair_suspend(base,engine,now);
    } else memset(&runtime_revive_pair_armed,0,sizeof runtime_revive_pair_armed);
    memset(s,0,sizeof *s);memset(&runtime_revive_peer_inbox,0,sizeof runtime_revive_peer_inbox);
}
static int runtime_revive_peer_send(uintptr_t base,uintptr_t engine,DWORD now,wchar_t kind) {
    RuntimeRevivePeerState *s=&runtime_revive_peer_state;RevivePeerMessage m={0};
    SessionContext c=session_context(engine);RuntimeReviveAdmission admission;
    if(!s->phase||!runtime_revive_peer_enabled_now()||!runtime_control_enabled||!runtime_scope_ready||
       session_engine_blocked(c)||now-s->pair.sampled_at>3000)return 0;
    for(unsigned i=0;i<2;++i)if(!runtime_revive_read_admission(base,c,s->pair.scope[i].pawn.pointer,
        s->pair.scope[i].registration,&admission)||!runtime_revive_scope_same(&admission,&s->pair.scope[i]))return 0;
    uintptr_t pc=g_runtime_config.role==ME3_ROLE_HOST?session_remote:c.pc;
    if(read_ptr(s->pair.scope[0].connection.pointer+0x68)!=pc)return 0;
    m.kind=kind;m.guest_nonce=s->guest_nonce;m.host_nonce=kind==L'Q'?0:s->host_nonce;
    m.shepard_channel=(unsigned)s->pair.scope[0].channel_index;
    m.ashley_channel=(unsigned)s->pair.scope[1].channel_index;memcpy(m.guid,s->pair.package_guid,16);
    wchar_t text[REVIVE_PEER_MESSAGE_CAPACITY];int count=revive_peer_format(&m,text);
    if(!count||!presentation_send_text(pc,g_runtime_config.role==ME3_ROLE_HOST?1:0,text,count))return 0;
    s->sent_at=now;return 1;
}
/* The phase predicate is shared with the fixture. Correct nonce and channels
 * alone cannot turn a readiness query/offer into an activation instruction. */
static int runtime_revive_peer_accepts(int host,int phase,uint64_t guest,uint64_t host_nonce,
    const RevivePeerMessage *m) {
    if(!guest||m->guest_nonce!=guest)return 0;
    if(host&&m->kind==L'Q')return phase==1&&!m->host_nonce;
    if(!host_nonce||m->host_nonce!=host_nonce)return 0;
    if(host)return (phase==1&&m->kind==L'A')||(phase==2&&(m->kind==L'A'||m->kind==L'D'))||
        (phase==3&&(m->kind==L'D'||m->kind==L'H'));
    return (phase==1&&m->kind==L'O')||(phase==2&&(m->kind==L'O'||m->kind==L'C'))||
        (phase==3&&(m->kind==L'C'||m->kind==L'H'));
}
static int runtime_revive_peer_commit_expired(int host,int phase,int confirmed,DWORD phase_at,DWORD now) {
    return ((host&&phase==2)||(!host&&phase==3&&!confirmed))&&now-phase_at>3000;
}
static void runtime_revive_peer_tick(uintptr_t base,uintptr_t engine,DWORD now) {
    static DWORD sample_at;
    RuntimeRevivePeerState *s=&runtime_revive_peer_state;SessionContext c=session_context(engine);
    int host=g_runtime_config.role==ME3_ROLE_HOST;
    if(!runtime_revive_peer_enabled_now()||!runtime_control_enabled||!runtime_scope_ready||session_engine_blocked(c)) {
        runtime_revive_clear_guest_offer();runtime_revive_peer_reset(base,engine,now,1);return;
    }
    if(!host) {
        PresentationGuestScope *g=&presentation_guest;
        if(!presentation_guest_valid(base)||!runtime_revive_offer_guest(base,c,g->objects[2].pointer,
            g->objects[6].pointer,g->objects[4].pointer,g->objects[5].pointer,g->index,now)) {
            runtime_revive_clear_guest_offer();runtime_revive_peer_reset(base,engine,now,1);return;
        }
    }
    if(sample_at&&now-sample_at<250)return;
    sample_at=now;
    RuntimeRevivePair pair;
    if(!runtime_revive_pair_snapshot(base,engine,now,&pair)) {
        runtime_revive_peer_reset(base,engine,now,1);return;
    }
    now=GetTickCount();
    if(s->phase&&(s->pair.generation!=pair.generation||!runtime_revive_pair_identity_same(&s->pair,&pair)||
       (s->activated&&(pair.status[0]!=2||pair.status[1]!=2)))) {
        runtime_revive_peer_reset(base,engine,now,1);return;
    }
    s->pair=pair;
    if(s->phase&&(now-s->peer_at>3000||runtime_revive_peer_commit_expired(host,s->phase,s->confirmed,s->phase_at,now))) {
        runtime_revive_peer_reset(base,engine,now,0);return;
    }
    RuntimeRevivePeerInbox r=runtime_revive_peer_inbox;runtime_revive_peer_inbox.active=0;
    int received=runtime_revive_peer_inbox_valid(base,&r,&pair,now)&&runtime_revive_peer_pair_message(&pair,&r.message);
    if(!s->phase) {
        if(runtime_revive_peer_serial==UINT64_MAX)return;
        if(host) {
            if(!received||r.message.kind!=L'Q')return;
            s->guest_nonce=r.message.guest_nonce;s->host_nonce=++runtime_revive_peer_serial;
        } else s->guest_nonce=++runtime_revive_peer_serial;
        s->phase=1;s->phase_at=now;s->peer_at=now;s->sent_at=0;runtime_revive_peer_note("local-pair-prepared");
    }
    if(received) {
        RevivePeerMessage *m=&r.message;
        if(!host&&s->phase==1&&m->kind==L'O'&&m->guest_nonce==s->guest_nonce)s->host_nonce=m->host_nonce;
        if(runtime_revive_peer_accepts(host,s->phase,s->guest_nonce,s->host_nonce,m)) {
            s->peer_at=now;
            if(!host&&s->phase==1) {
                if(runtime_revive_pair_can_activate(base,engine,&pair,now)&&
                   runtime_revive_pair_arm(base,engine,&pair,GetTickCount())){s->phase=2;s->phase_at=GetTickCount();s->sent_at=0;}
            } else if((host&&s->phase==1&&m->kind==L'A')||(!host&&s->phase==2&&m->kind==L'C')) {
                if(runtime_revive_pair_can_activate(base,engine,&pair,now)&&
                   runtime_revive_pair_arm(base,engine,&pair,GetTickCount())&&
                   runtime_revive_pair_activate(base,engine,&pair,1,GetTickCount())) {
                    s->activated=1;s->phase=host?2:3;s->phase_at=GetTickCount();s->sent_at=0;
                    runtime_revive_peer_note("local-admission-open");
                }
            } else if(host&&s->phase==2&&m->kind==L'D') {
                s->phase=3;s->phase_at=now;s->confirmed=1;s->sent_at=0;runtime_revive_peer_note("both-peers-active");
            } else if(!host&&s->phase==3&&m->kind==L'H') {
                if(!s->confirmed)runtime_revive_peer_note("both-peers-active");s->confirmed=1;
            } else if(!host&&s->phase==3&&m->kind==L'C')s->sent_at=0;
        }
    }
    now=GetTickCount();
    DWORD cadence=s->phase==3&&s->confirmed?1000:500;
    if(!s->sent_at||now-s->sent_at>=cadence) {
        wchar_t kind=host?(s->phase==1?L'O':s->phase==2?L'C':L'H'):
            (s->phase==1?L'Q':s->phase==2?L'A':s->confirmed?L'H':L'D');
        runtime_revive_peer_send(base,engine,now,kind);
    }
}
static int runtime_revive_peer_selftest(void) {
    if(!revive_peer_protocol_selftest())return 0;
    if(runtime_revive_peer_commit_expired(1,2,0,100,3100)||!runtime_revive_peer_commit_expired(1,2,0,100,3101)||
       !runtime_revive_peer_commit_expired(0,3,0,100,3101)||runtime_revive_peer_commit_expired(0,3,1,100,3101)||
       runtime_revive_peer_commit_expired(1,3,1,100,3101))return 0;
    RevivePeerMessage m={L'A',7,9,2,3,{1,0,0,0}};RuntimeRevivePair p={0};
    p.generation=1;p.scope[0].channel_index=2;p.scope[1].channel_index=3;p.package_guid[0]=1;
    if(!runtime_revive_peer_pair_message(&p,&m)||!runtime_revive_peer_accepts(1,1,7,9,&m)||
       runtime_revive_peer_accepts(1,2,8,9,&m)||runtime_revive_peer_accepts(1,1,7,10,&m))return 0;
    m.kind=L'C';
    if(runtime_revive_peer_accepts(0,1,7,9,&m)||!runtime_revive_peer_accepts(0,2,7,9,&m)||
       runtime_revive_peer_accepts(1,1,7,9,&m))return 0;
    m.kind=L'D';if(runtime_revive_peer_accepts(1,1,7,9,&m)||!runtime_revive_peer_accepts(1,2,7,9,&m))return 0;
    m.guid[3]=1;if(runtime_revive_peer_pair_message(&p,&m))return 0;m.guid[3]=0;
    m.ashley_channel=4;if(runtime_revive_peer_pair_message(&p,&m))return 0;
    return 1;
}
#endif
