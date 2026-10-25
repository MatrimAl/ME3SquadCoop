/* Ordinary runtime's advisory readiness transport. No engine dependencies. */
#include <winsock2.h>
#include <windows.h>
#include <bcrypt.h>
#include <string.h>
#include "runtime_ready_channel.h"

typedef struct ReadyChannel {
    SOCKET socket;
    int wsa,initialized,failed,host,queried,pending,confirmed;
    unsigned char bind_ip[4],peer_ip[4],nonce[16];
    unsigned short port;
    uint64_t context,sent_at,last_query;
} ReadyChannel;
static ReadyChannel ready_channel={.socket=INVALID_SOCKET};

void me3_ready_close(void) {
    ReadyChannel *s=&ready_channel;int failed=s->failed;
    if(s->socket!=INVALID_SOCKET)closesocket(s->socket);
    if(s->wsa)WSACleanup();
    memset(s,0,sizeof *s);s->socket=INVALID_SOCKET;s->failed=failed;
}
static int ready_failure(void) {
    me3_ready_close();ready_channel.failed=1;return -1;
}
/* Missing peers, ICMP errors and nonblocking backpressure are not permanent
 * local failures. Oversized datagrams are discarded, not a channel kill. */
static int ready_transient(int error) {
    return error==WSAEWOULDBLOCK||error==WSAECONNRESET||error==WSAENETRESET||
        error==WSAEHOSTUNREACH||error==WSAENETUNREACH||error==WSAENETDOWN;
}
static struct sockaddr_in ready_address(const unsigned char ip[4],unsigned short port) {
    struct sockaddr_in address={0};address.sin_family=AF_INET;address.sin_port=htons(port);
    memcpy(&address.sin_addr,ip,4);return address;
}
static int ready_initialize(const unsigned char bind_ip[4],const unsigned char peer_ip[4],
    unsigned short port,int host) {
    ReadyChannel *s=&ready_channel;WSADATA data;
    if(!bind_ip||(!host&&!peer_ip)||!port||port==65535||
       !(bind_ip[0]|bind_ip[1]|bind_ip[2]|bind_ip[3]))return ready_failure();
    if(WSAStartup(MAKEWORD(2,2),&data))return ready_failure();
    s->wsa=1;
    if(data.wVersion!=MAKEWORD(2,2))return ready_failure();
    s->socket=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    if(s->socket==INVALID_SOCKET)return ready_failure();
    BOOL exclusive=TRUE;u_long nonblocking=1;
    if(setsockopt(s->socket,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,(const char*)&exclusive,sizeof exclusive)||
       ioctlsocket(s->socket,FIONBIO,&nonblocking))return ready_failure();
    struct sockaddr_in local=ready_address(bind_ip,host?(unsigned short)(port+1):0);
    if(bind(s->socket,(const struct sockaddr*)&local,sizeof local))return ready_failure();
    memcpy(s->bind_ip,bind_ip,4);if(peer_ip)memcpy(s->peer_ip,peer_ip,4);
    s->port=port;s->host=!!host;s->initialized=1;return 0;
}
static int ready_packet(const unsigned char *packet,int length,int kind) {
    return length==24&&!memcmp(packet,"M3RD",4)&&packet[4]==1&&packet[5]==kind&&
        packet[6]<=1&&!packet[7]&&(kind!=1||!packet[6]);
}
int me3_ready_poll(const unsigned char bind_ip[4],const unsigned char peer_ip[4],
    unsigned short game_port,int host,int host_ready,int query,uint64_t context,uint64_t now) {
    ReadyChannel *s=&ready_channel;
    if(s->failed)return -1;
    if(!s->initialized&&ready_initialize(bind_ip,peer_ip,game_port,host)<0)return -1;
    if(!bind_ip||(!host&&!peer_ip)||s->host!=!!host||s->port!=game_port||
       memcmp(s->bind_ip,bind_ip,4)||(!host&&memcmp(s->peer_ip,peer_ip,4)))return ready_failure();
    query=query&&context;
    if(!host&&(!query||s->context!=context)) {
        /* Keep the send cadence across unavailable/replaced local contexts. */
        s->pending=s->confirmed=0;s->context=query?context:0;
        memset(s->nonce,0,sizeof s->nonce);
    }
    /* A bounded receive loop avoids unbounded Tick work from queued traffic. */
    for(int i=0;i<4;++i) {
        unsigned char packet[64];struct sockaddr_in sender={0};int bytes=sizeof sender;
        int received=recvfrom(s->socket,(char*)packet,sizeof packet,0,(struct sockaddr*)&sender,&bytes);
        if(received==SOCKET_ERROR) {
            int error=WSAGetLastError();
            if(error==WSAEMSGSIZE)continue;
            if(ready_transient(error))break;
            return ready_failure();
        }
        if(bytes!=sizeof sender||sender.sin_family!=AF_INET)continue;
        if(host) {
            if(!ready_packet(packet,received,1))continue;
            packet[5]=2;packet[6]=!!host_ready;
            int sent=sendto(s->socket,(const char*)packet,24,0,(const struct sockaddr*)&sender,sizeof sender);
            if(sent==SOCKET_ERROR) {if(!ready_transient(WSAGetLastError()))return ready_failure();}
            else if(sent!=24)return ready_failure();
        } else if(query&&s->pending&&ready_packet(packet,received,2)&&
            sender.sin_port==htons((unsigned short)(s->port+1))&&!memcmp(&sender.sin_addr,s->peer_ip,4)&&
            !memcmp(packet+8,s->nonce,16)&&now>=s->sent_at&&now-s->sent_at<=1000) {
            s->confirmed=packet[6];
        }
    }
    if(host||!query)return 0;
    if(s->pending&&s->confirmed&&now>=s->sent_at&&now-s->sent_at<=1000)return 1;
    if(s->queried&&now>=s->last_query&&now-s->last_query<500)return 0;
    s->last_query=now;s->queried=1;s->pending=s->confirmed=0;
    unsigned char packet[24]={'M','3','R','D',1,1,0,0};
    if(BCryptGenRandom(NULL,s->nonce,sizeof s->nonce,BCRYPT_USE_SYSTEM_PREFERRED_RNG)<0)return ready_failure();
    memcpy(packet+8,s->nonce,16);
    struct sockaddr_in peer=ready_address(s->peer_ip,(unsigned short)(s->port+1));
    int sent=sendto(s->socket,(const char*)packet,sizeof packet,0,(const struct sockaddr*)&peer,sizeof peer);
    if(sent==SOCKET_ERROR) {if(ready_transient(WSAGetLastError()))return 0;return ready_failure();}
    if(sent!=sizeof packet)return ready_failure();
    s->pending=1;s->sent_at=now;return 0;
}
