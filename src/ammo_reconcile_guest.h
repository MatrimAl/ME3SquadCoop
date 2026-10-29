#ifndef ME3_AMMO_RECONCILE_GUEST_H
#define ME3_AMMO_RECONCILE_GUEST_H
#include "ammo_reconcile_protocol.h"
/* Request scheduling only. The transport authenticates replies and revalidates
 * live ownership before any counter write. Scope changes discard this state;
 * the process-wide sequence must survive those changes to prevent ID reuse. */
typedef struct AmmoReconcileGuest {
    int have,used,spare,dirty,quiet_started;
    uint32_t quiet_at,retry_at;
    unsigned attempts;
    AmmoReconcilePending pending;
} AmmoReconcileGuest;
static void ammo_reconcile_guest_activity(AmmoReconcileGuest *s) {
    ammo_reconcile_cancel_pending(&s->pending);s->dirty=1;s->quiet_started=0;
}
/* 0 no send, 1 new request, 2 retry. No host or game state is written here. */
static int ammo_reconcile_guest_poll_limit(AmmoReconcileGuest *s,uint32_t generation,unsigned channel,
    int idle,int used,int spare,uint32_t now,unsigned *sequence,unsigned limit,AmmoReconcileMessage *request) {
    if(!generation||channel>2047||used<0||used>1000000||spare<0||spare>1000000){
        ammo_reconcile_guest_activity(s);return 0;
    }
    if(!s->have){s->have=1;s->used=used;s->spare=spare;}
    if(s->used!=used||s->spare!=spare){s->used=used;s->spare=spare;ammo_reconcile_guest_activity(s);}
    if(idle!=1){s->quiet_started=0;ammo_reconcile_cancel_pending(&s->pending);return 0;}
    if(s->pending.active){
        if(s->pending.generation!=generation||s->pending.channel!=channel){ammo_reconcile_guest_activity(s);return 0;}
        if(now-s->pending.sent_at>5000){ammo_reconcile_guest_activity(s);return 0;}
        if(now-s->retry_at>=1000&&s->attempts<3){
            *request=(AmmoReconcileMessage){L'Q',s->pending.request,s->pending.channel,0,0};
            s->retry_at=now;++s->attempts;return 2;
        }
        return 0;
    }
    if(!s->dirty)return 0;
    if(!s->quiet_started){s->quiet_started=1;s->quiet_at=now;return 0;}
    if(now-s->quiet_at<250||*sequence>=limit)return 0;
    uint64_t id=((uint64_t)generation<<32)|++*sequence;
    s->pending=(AmmoReconcilePending){id,generation,channel,used,spare,1,now};
    s->retry_at=now;s->attempts=1;
    *request=(AmmoReconcileMessage){L'Q',id,channel,0,0};return 1;
}
/* Preserve the protected experiment's collection limit. The ordinary adapter
 * supplies its own non-reusing operational sequence bound. */
static int ammo_reconcile_guest_poll(AmmoReconcileGuest *s,uint32_t generation,unsigned channel,
    int idle,int used,int spare,uint32_t now,unsigned *sequence,AmmoReconcileMessage *request) {
    return ammo_reconcile_guest_poll_limit(s,generation,channel,idle,used,spare,now,sequence,64,request);
}
static void ammo_reconcile_guest_complete(AmmoReconcileGuest *s,const AmmoReconcileMessage *reply,int applied) {
    /* The caller must first accept this reply via response_allowed and verify
     * actual counter readback. Failure retains a need for a fresh request. */
    ammo_reconcile_cancel_pending(&s->pending);s->quiet_started=0;s->dirty=!applied;
    if(applied){s->have=1;s->used=reply->used;s->spare=reply->spare;}
}
static int ammo_reconcile_guest_selftest(void) {
    AmmoReconcileGuest s={0};AmmoReconcileMessage q={0};unsigned serial=0;
    if(ammo_reconcile_guest_poll(&s,3,25,1,0,210,0,&serial,&q)||
       ammo_reconcile_guest_poll(&s,3,25,1,1,210,10,&serial,&q)||
       ammo_reconcile_guest_poll(&s,3,25,1,1,210,259,&serial,&q)||
       ammo_reconcile_guest_poll(&s,3,25,1,1,210,260,&serial,&q)!=1||
       serial!=1||q.request!=UINT64_C(12884901889)||q.kind!=L'Q'||q.used||q.spare)return 0;
    uint64_t original=q.request;
    if(ammo_reconcile_guest_poll(&s,3,25,1,1,210,1259,&serial,&q)||
       ammo_reconcile_guest_poll(&s,3,25,1,1,210,1260,&serial,&q)!=2||q.request!=original||serial!=1||
       ammo_reconcile_guest_poll(&s,3,25,1,1,210,2260,&serial,&q)!=2||
       ammo_reconcile_guest_poll(&s,3,25,1,1,210,3260,&serial,&q)||s.attempts!=3)return 0;
    /* Even if counts return to1, the observed intermediate2 cancels request1. */
    if(ammo_reconcile_guest_poll(&s,3,25,1,2,210,3300,&serial,&q)||s.pending.active||
       ammo_reconcile_guest_poll(&s,3,25,1,1,210,3400,&serial,&q)||
       ammo_reconcile_guest_poll(&s,3,25,1,1,210,3650,&serial,&q)!=1||serial!=2||q.request==original)return 0;
    if(ammo_reconcile_guest_poll(&s,3,25,0,1,210,3660,&serial,&q)||s.pending.active||s.quiet_started)return 0;
    if(ammo_reconcile_guest_poll(&s,3,25,1,1,210,3700,&serial,&q)||
       ammo_reconcile_guest_poll(&s,3,25,1,1,210,3950,&serial,&q)!=1)return 0;
    AmmoReconcileMessage response={L'A',q.request,25,0,210};
    if(!ammo_reconcile_response_allowed(&s.pending,&response,3,25,4000,1,1,210))return 0;
    ammo_reconcile_guest_complete(&s,&response,1);
    if(s.pending.active||s.dirty||s.used||s.spare!=210)return 0;
    ammo_reconcile_guest_activity(&s);serial=64;
    if(ammo_reconcile_guest_poll(&s,3,25,1,0,210,5000,&serial,&q)||
       ammo_reconcile_guest_poll(&s,3,25,1,0,210,5250,&serial,&q)||serial!=64)return 0;
    s=(AmmoReconcileGuest){0};s.dirty=1;serial=0;
    if(ammo_reconcile_guest_poll(&s,4,25,1,2,210,UINT32_MAX-99,&serial,&q)||
       ammo_reconcile_guest_poll(&s,4,25,1,2,210,150,&serial,&q)!=1||
       ammo_reconcile_guest_poll(&s,4,25,1,2,210,5151,&serial,&q)||s.pending.active)return 0;
    if(ammo_reconcile_guest_poll(&s,4,25,1,2,210,5200,&serial,&q)||
       ammo_reconcile_guest_poll(&s,4,25,1,2,210,5450,&serial,&q)!=1||
       ammo_reconcile_guest_poll(&s,5,25,1,2,210,5460,&serial,&q)||s.pending.active)return 0;
    s=(AmmoReconcileGuest){0};s.dirty=1;serial=64;
    if(ammo_reconcile_guest_poll_limit(&s,5,25,1,2,210,6000,&serial,UINT32_MAX,&q)||
       ammo_reconcile_guest_poll_limit(&s,5,25,1,2,210,6250,&serial,UINT32_MAX,&q)!=1||
       serial!=65||q.request!=((UINT64_C(5)<<32)|65))return 0;
    s=(AmmoReconcileGuest){0};s.dirty=1;serial=UINT32_MAX;
    if(ammo_reconcile_guest_poll_limit(&s,5,25,1,2,210,6300,&serial,UINT32_MAX,&q)||
       ammo_reconcile_guest_poll_limit(&s,5,25,1,2,210,6550,&serial,UINT32_MAX,&q)||serial!=UINT32_MAX)return 0;
    return 1;
}
#endif
