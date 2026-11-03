#ifndef ME3_NATIVE_RPC_RECEIVE_SELFTEST_H
#define ME3_NATIVE_RPC_RECEIVE_SELFTEST_H
/* Fixture processes only: no native channel, engine call or game input. */
static DWORD WINAPI native_rpc_receive_fixture_foreign(void *argument) {
    DWORD owner=*(DWORD*)argument;NativeRpcReceiveFrame f;
    int ok=!native_rpc_receive_current(owner);
    native_rpc_receive_begin(&f,owner,33);
    ok=ok&&!f.active&&!native_rpc_receive_current(owner);
    native_rpc_receive_end(&f);return ok?1:0;
}
static int native_rpc_receive_context_selftest(void) {
    NativeRpcReceiveFrame *saved=native_rpc_receive_frame;DWORD error=GetLastError();
    native_rpc_receive_frame=NULL;DWORD owner=GetCurrentThreadId();
    NativeRpcReceiveFrame outer,inner;int ok=!native_rpc_receive_current(owner);
    native_rpc_receive_begin(&outer,owner,11);
    ok=ok&&native_rpc_receive_current(owner)==&outer&&outer.channel==11;
    HANDLE worker=CreateThread(NULL,0,native_rpc_receive_fixture_foreign,&owner,0,NULL);
    if(worker){DWORD code=0;WaitForSingleObject(worker,INFINITE);ok=ok&&GetExitCodeThread(worker,&code)&&code==1;CloseHandle(worker);}
    else ok=0;
    native_rpc_receive_begin(&inner,owner,0);
    ok=ok&&native_rpc_receive_current(owner)==&inner&&!inner.channel;
    native_rpc_receive_end(&outer); /* An out-of-order end cannot pop inner. */
    ok=ok&&native_rpc_receive_current(owner)==&inner&&outer.active;
    native_rpc_receive_end(&inner);
    ok=ok&&native_rpc_receive_current(owner)==&outer;
    native_rpc_receive_end(&outer);
    ok=ok&&!native_rpc_receive_current(owner)&&!outer.active;
    native_rpc_receive_frame=saved;SetLastError(error);return ok;
}
#endif
