/* Standalone authored-state handshake; no engine headers, hooks or game. */
#include "../src/ammo_reconcile_state.h"
int main(void) {
    if(!ammo_reconcile_protocol_selftest()||!ammo_reconcile_host_selftest()||
       !ammo_reconcile_guest_selftest()||!ammo_reconcile_state_selftest())return 1;
    AmmoReconcileGuest guest={0};AmmoReconcileHost host={0};
    AmmoReconcileMessage request={0},reply={0};unsigned sequence=0;
    if(ammo_reconcile_guest_poll(&guest,7,29,1,3,210,0,&sequence,&request)||
       ammo_reconcile_guest_poll(&guest,7,29,1,4,210,10,&sequence,&request)||
       ammo_reconcile_guest_poll(&guest,7,29,1,4,210,260,&sequence,&request)!=1||
       ammo_reconcile_host_request(&host,&request,261)!=1||
       ammo_reconcile_host_snapshot(&host,1,3,210,29,261,&reply)||
       !ammo_reconcile_host_snapshot(&host,1,3,210,29,511,&reply)||
       !ammo_reconcile_host_sent(&host,&reply)||
       !ammo_reconcile_response_allowed(&guest.pending,&reply,7,29,520,1,4,210))return 2;
    /* An adapter would verify the actual game write/readback before reporting
     * applied. This fixture models that completion; no game storage exists. */
    ammo_reconcile_guest_complete(&guest,&reply,1);
    if(guest.used!=3||guest.spare!=210||guest.pending.active||guest.dirty||host.pending)return 3;
    return 0;
}
