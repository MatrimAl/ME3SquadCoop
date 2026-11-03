#ifndef ME3_NATIVE_RPC_RECEIVE_CONTEXT_H
#define ME3_NATIVE_RPC_RECEIVE_CONTEXT_H
/* Dynamic native receive provenance, not packet authentication. Only the
 * admitted game-thread wrapper pushes frames. Consumers must independently
 * validate the current channel, connection, RPC node and target ownership. */
typedef struct NativeRpcReceiveFrame {
    struct NativeRpcReceiveFrame *previous;
    uintptr_t channel;
    DWORD thread;
    int active;
} NativeRpcReceiveFrame;
static NativeRpcReceiveFrame *native_rpc_receive_frame;
static NativeRpcReceiveFrame *native_rpc_receive_current(DWORD owner) {
    if(!owner||GetCurrentThreadId()!=owner)return NULL;
    NativeRpcReceiveFrame *f=native_rpc_receive_frame;
    return f&&f->active&&f->thread==owner?f:NULL;
}
static void native_rpc_receive_begin(NativeRpcReceiveFrame *f,DWORD owner,uintptr_t channel) {
    memset(f,0,sizeof *f);
    if(!owner||GetCurrentThreadId()!=owner)return;
    f->previous=native_rpc_receive_frame;f->thread=owner;f->channel=channel;f->active=1;
    /* Push a null channel too, rather than inherit an outer actor's source. */
    native_rpc_receive_frame=f;
}
static void native_rpc_receive_end(NativeRpcReceiveFrame *f) {
    if(!f->active||GetCurrentThreadId()!=f->thread||native_rpc_receive_frame!=f)return;
    native_rpc_receive_frame=f->previous;f->active=0;
}
#endif
