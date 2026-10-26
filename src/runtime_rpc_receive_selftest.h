#ifndef ME3_RUNTIME_RPC_RECEIVE_SELFTEST_H
#define ME3_RUNTIME_RPC_RECEIVE_SELFTEST_H
#include "native_rpc_receive_selftest.h"
static unsigned runtime_rpc_fixture_calls;
static int runtime_rpc_fixture_ok;
static void runtime_rpc_fixture_forward(void *self,void *bunch) {
    ++runtime_rpc_fixture_calls;
    NativeRpcReceiveFrame *f=native_rpc_receive_current(runtime_rpc_receive_thread);
    uintptr_t actor=(uintptr_t)self;
    if(actor==11){
        runtime_rpc_fixture_ok=runtime_rpc_fixture_ok&&f&&f->channel==11&&
            (uintptr_t)bunch==101&&GetLastError()==111;
        SetLastError(222);runtime_rpc_receive((void*)22,(void*)202);
        f=native_rpc_receive_current(runtime_rpc_receive_thread);
        runtime_rpc_fixture_ok=runtime_rpc_fixture_ok&&f&&f->channel==11&&GetLastError()==333;
        SetLastError(444);
    } else if(actor==22){
        runtime_rpc_fixture_ok=runtime_rpc_fixture_ok&&f&&f->channel==22&&
            (uintptr_t)bunch==202&&GetLastError()==222;
        SetLastError(333);
    } else {
        runtime_rpc_fixture_ok=runtime_rpc_fixture_ok&&actor==33&&f&&!f->channel&&
            (uintptr_t)bunch==303&&GetLastError()==555;
        SetLastError(666);
    }
}
static int runtime_rpc_receive_selftest(void) {
    int ok=native_rpc_receive_context_selftest();
    RuntimeRpcReceive original=runtime_rpc_receive_original;
    DWORD thread=runtime_rpc_receive_thread,error=GetLastError();
    NativeRpcReceiveFrame *saved=native_rpc_receive_frame;native_rpc_receive_frame=NULL;
    int enabled=runtime_control_enabled,ready=runtime_scope_ready;
    runtime_rpc_receive_original=runtime_rpc_fixture_forward;runtime_rpc_receive_thread=GetCurrentThreadId();
    runtime_control_enabled=1;runtime_scope_ready=1;runtime_rpc_fixture_calls=0;runtime_rpc_fixture_ok=1;
    SetLastError(111);runtime_rpc_receive((void*)11,(void*)101);
    ok=ok&&runtime_rpc_fixture_ok&&runtime_rpc_fixture_calls==2&&GetLastError()==444&&!native_rpc_receive_frame;
    NativeRpcReceiveFrame outer;native_rpc_receive_begin(&outer,runtime_rpc_receive_thread,44);
    runtime_control_enabled=0;SetLastError(555);runtime_rpc_receive((void*)33,(void*)303);
    ok=ok&&runtime_rpc_fixture_ok&&runtime_rpc_fixture_calls==3&&GetLastError()==666&&
        native_rpc_receive_current(runtime_rpc_receive_thread)==&outer;
    native_rpc_receive_end(&outer);
    runtime_rpc_receive_original=original;runtime_rpc_receive_thread=thread;
    runtime_control_enabled=enabled;runtime_scope_ready=ready;native_rpc_receive_frame=saved;
    SetLastError(error);return ok;
}
#endif
