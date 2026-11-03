#ifndef ME3_RUNTIME_READY_CHANNEL_H
#define ME3_RUNTIME_READY_CHANNEL_H
#include <stdint.h>
/* Single game-thread caller. No game object or wire pointer parameters.
 * -1: sticky local failure, 0: waiting, 1: fresh correlated ready reply. */
int me3_ready_poll(const unsigned char bind_ip[4],const unsigned char peer_ip[4],
    unsigned short game_port,int host,int host_ready,int query,uint64_t context,uint64_t now);
void me3_ready_close(void);
#endif
