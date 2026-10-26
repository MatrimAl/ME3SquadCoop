#ifndef ME3_AMMO_RECONCILE_STATE_H
#define ME3_AMMO_RECONCILE_STATE_H
#include "ammo_reconcile_host.h"
#include "ammo_reconcile_guest.h"
/* Bind before every admitted receive, local activity or scheduler poll. The
 * adapter supplies a validated local scope generation, or zero on scope loss.
 * Request sequence ownership stays outside this disposable state. */
typedef struct AmmoReconcileState {
    uint32_t generation;
    AmmoReconcileGuest guest;
    AmmoReconcileHost host;
} AmmoReconcileState;
static int ammo_reconcile_state_bind(AmmoReconcileState *s,uint32_t generation) {
    if(!generation||s->generation!=generation){
        *s=(AmmoReconcileState){0};s->generation=generation;
    }
    return generation!=0;
}
static int ammo_reconcile_state_selftest(void) {
    AmmoReconcileState s={0};AmmoReconcileMessage q={L'Q',10,25,0,0},reply={0};
    /* A receive before the first scheduler poll must survive that poll's bind. */
    if(!ammo_reconcile_state_bind(&s,3)||
       ammo_reconcile_host_request(&s.host,&q,100)!=1||
       !ammo_reconcile_state_bind(&s,3)||!s.host.pending||
       ammo_reconcile_host_snapshot(&s.host,1,4,210,25,100,&reply)||
       !ammo_reconcile_state_bind(&s,3)||
       !ammo_reconcile_host_snapshot(&s.host,1,4,210,25,350,&reply)||
       !ammo_reconcile_host_sent(&s.host,&reply))return 0;
    unsigned sequence=8;
    ammo_reconcile_guest_activity(&s.guest);
    if(!ammo_reconcile_state_bind(&s,3)||!s.guest.dirty||
       ammo_reconcile_guest_poll(&s.guest,3,25,1,5,210,400,&sequence,&q)||
       ammo_reconcile_guest_poll(&s.guest,3,25,1,5,210,650,&sequence,&q)!=1||sequence!=9)return 0;
    reply=(AmmoReconcileMessage){L'A',q.request,25,4,210};
    /* Even identical counts/channel in a new scope cannot admit an old reply. */
    if(!ammo_reconcile_state_bind(&s,4)||s.host.last_replied||s.host.pending||
       s.guest.have||s.guest.dirty||s.guest.pending.active||
       ammo_reconcile_response_allowed(&s.guest.pending,&reply,4,25,700,1,5,210)||sequence!=9)return 0;
    ammo_reconcile_guest_activity(&s.guest);
    if(!ammo_reconcile_state_bind(&s,4)||!s.guest.dirty||
       ammo_reconcile_guest_poll(&s.guest,4,25,1,5,210,700,&sequence,&q)||
       ammo_reconcile_guest_poll(&s.guest,4,25,1,5,210,950,&sequence,&q)!=1||
       sequence!=10||q.request==reply.request)return 0;
    /* Scope loss invalidates both peers even before another generation exists. */
    q=(AmmoReconcileMessage){L'Q',11,25,0,0};
    if(ammo_reconcile_host_request(&s.host,&q,960)!=1||
       ammo_reconcile_state_bind(&s,0)||s.generation||s.host.pending||
       s.guest.pending.active||s.guest.have||sequence!=10)return 0;
    return 1;
}
#endif
