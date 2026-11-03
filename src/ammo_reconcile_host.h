#ifndef ME3_AMMO_RECONCILE_HOST_H
#define ME3_AMMO_RECONCILE_HOST_H
#include "ammo_reconcile_protocol.h"
/* Transport-independent host snapshots. Caller must authenticate the owning
 * connection and invalidate this state whenever its weapon scope changes.
 * No client ammunition values are accepted or written. */
typedef struct AmmoReconcileHost {
    uint64_t request,last_replied;
    unsigned channel;
    uint32_t received_at,quiet_at;
    int pending,sampled,used,spare;
} AmmoReconcileHost;
/* 0 rejected, 1 newly accepted, 2 duplicate pending request. Duplicates must
 * not extend the lifetime or erase already observed quiet time. */
static int ammo_reconcile_host_request(AmmoReconcileHost *s,const AmmoReconcileMessage *m,uint32_t now) {
    if(m->kind!=L'Q'||!m->request||m->channel>2047||m->used||m->spare||
       m->request<=s->last_replied||(s->pending&&m->request<s->request))return 0;
    if(s->pending&&m->request==s->request)return m->channel==s->channel?2:0;
    s->request=m->request;s->channel=m->channel;s->received_at=now;
    s->pending=1;s->sampled=0;return 1;
}
static int ammo_reconcile_host_snapshot(AmmoReconcileHost *s,int idle,int used,int spare,
    unsigned channel,uint32_t now,AmmoReconcileMessage *reply) {
    if(!s->pending)return 0;
    if(channel!=s->channel||now-s->received_at>5000){s->pending=0;s->sampled=0;return 0;}
    if(idle!=1||used<0||used>1000000||spare<0||spare>1000000){s->sampled=0;return 0;}
    if(!s->sampled||s->used!=used||s->spare!=spare){
        s->used=used;s->spare=spare;s->quiet_at=now;s->sampled=1;return 0;
    }
    if(now-s->quiet_at<250)return 0;
    *reply=(AmmoReconcileMessage){L'A',s->request,s->channel,used,spare};return 1;
}
static int ammo_reconcile_host_sent(AmmoReconcileHost *s,const AmmoReconcileMessage *reply) {
    if(!s->pending||reply->kind!=L'A'||reply->request!=s->request||reply->channel!=s->channel||
       !s->sampled||reply->used!=s->used||reply->spare!=s->spare)return 0;
    s->last_replied=s->request;s->pending=0;s->sampled=0;return 1;
}
static int ammo_reconcile_host_selftest(void) {
    AmmoReconcileHost s={0};AmmoReconcileMessage q={L'Q',10,25,0,0},reply={0};
    if(ammo_reconcile_host_request(&s,&q,0)!=1||
       ammo_reconcile_host_snapshot(&s,1,4,210,25,0,&reply)||
       ammo_reconcile_host_snapshot(&s,1,4,210,25,249,&reply)||
       ammo_reconcile_host_request(&s,&q,249)!=2||s.received_at!=0||
       !ammo_reconcile_host_snapshot(&s,1,4,210,25,250,&reply)||
       reply.used!=4||reply.spare!=210||!ammo_reconcile_host_sent(&s,&reply)||
       ammo_reconcile_host_request(&s,&q,300))return 0;
    q.request=11;
    if(ammo_reconcile_host_request(&s,&q,400)!=1||
       ammo_reconcile_host_snapshot(&s,1,4,210,25,400,&reply)||
       ammo_reconcile_host_snapshot(&s,1,5,210,25,649,&reply)||
       ammo_reconcile_host_snapshot(&s,1,5,210,25,898,&reply)||
       !ammo_reconcile_host_snapshot(&s,1,5,210,25,899,&reply))return 0;
    /* Firing interrupts a previously quiet window; reusing its timer fails. */
    if(ammo_reconcile_host_snapshot(&s,0,5,210,25,900,&reply)||s.sampled||
       ammo_reconcile_host_snapshot(&s,1,5,210,25,1000,&reply)||
       ammo_reconcile_host_snapshot(&s,1,5,210,25,1249,&reply)||
       !ammo_reconcile_host_snapshot(&s,1,5,210,25,1250,&reply))return 0;
    AmmoReconcileMessage wrong=reply;wrong.channel=26;
    if(ammo_reconcile_host_sent(&s,&wrong)||!s.pending||
       ammo_reconcile_host_snapshot(&s,1,5,210,26,1300,&reply)||s.pending)return 0;
    q.request=12;q.used=99;if(ammo_reconcile_host_request(&s,&q,1400))return 0;q.used=0;
    if(ammo_reconcile_host_request(&s,&q,1400)!=1||
       ammo_reconcile_host_snapshot(&s,1,5,210,25,6401,&reply)||s.pending)return 0;
    s=(AmmoReconcileHost){0};q.request=13;
    if(ammo_reconcile_host_request(&s,&q,UINT32_MAX-99)!=1||
       ammo_reconcile_host_snapshot(&s,1,7,210,25,UINT32_MAX-99,&reply)||
       !ammo_reconcile_host_snapshot(&s,1,7,210,25,150,&reply))return 0;
    return 1;
}
#endif
