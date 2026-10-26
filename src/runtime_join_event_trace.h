#ifndef ME3_RUNTIME_JOIN_EVENT_TRACE_H
#define ME3_RUNTIME_JOIN_EVENT_TRACE_H
/* Default-off resident observer of the exact supported Engine progress virtual.
 * Copies event arguments synchronously before unchanged native forwarding.
 * A progress kind/message is evidence, not an inferred connection verdict. */
static EngineProgress runtime_join_event_original;
static SessionHandle runtime_join_event_engine;
static volatile LONG runtime_join_event_rows;
static int runtime_join_event_opted(void) {
    wchar_t value[2]={0};
    return g_runtime_config.role==ME3_ROLE_GUEST&&
        GetEnvironmentVariableW(L"ME3_SQUAD_COOP_JOIN_EVENT_TRACE",value,2)==1&&value[0]==L'1';
}
static int runtime_join_event_due(int active,DWORD started,DWORD now,int identity,int thread) {
    return active&&identity&&thread&&now-started<RUNTIME_JOIN_PROGRESS_WINDOW_MS;
}
static void runtime_join_event_forward(EngineProgress original,void *self,int32_t kind,
    void *title,void *message,int admitted,DWORD now) {
    DWORD error=GetLastError();
    if(admitted&&InterlockedIncrement(&runtime_join_event_rows)<=16) {
        RuntimeJoinText a=runtime_join_capture_console_result((const GameString*)title);
        RuntimeJoinText b=runtime_join_capture_console_result((const GameString*)message);
        char row[RUNTIME_JOIN_JSON_BYTES*2+480];
        snprintf(row,sizeof row,"{\"runtime_network\":\"join-progress-event\",\"evidence_source\":\"Engine.UpdateProgress-virtual\",\"kind\":%d,\"title_status\":\"%s\",\"title\":\"%s\",\"message_status\":\"%s\",\"message\":\"%s\",\"monotonic_ms\":%lu,\"forwarded_unchanged\":true,\"connection_verified\":false}\r\n",kind,runtime_join_text_status_name(a.status),a.escaped,runtime_join_text_status_name(b.status),b.escaped,now);
        line(row);
    }
    SetLastError(error);original(self,kind,title,message);
}
static void runtime_join_event_trace(void *self,int32_t kind,void *title,void *message) {
    DWORD now=GetTickCount(),error=GetLastError();uintptr_t base=(uintptr_t)GetModuleHandleW(NULL);
    int identity=runtime_join_event_engine.pointer==(uintptr_t)self&&
        read_ptr(base+0x18b31d0)==(uintptr_t)self&&session_handle_matches(runtime_join_event_engine);
    int admitted=runtime_join_event_due(runtime_join_progress.active,runtime_join_progress.started_at,now,
        identity,GetCurrentThreadId()==session_game_thread);
    SetLastError(error);
    runtime_join_event_forward(runtime_join_event_original,self,kind,title,message,admitted,now);
}
static int runtime_join_event_install(uintptr_t base,uintptr_t engine) {
    if(!runtime_join_event_opted())return 0;
    uintptr_t vt=read_ptr(engine),slot=vt+0x2e0,target=base+0xb9c5c0;
    const unsigned char expected[]={0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57};
    unsigned char actual[sizeof expected];SessionHandle identity=session_capture_handle(engine);
    if(vt!=base+0x13983a0||read_ptr(base+0x18b31d0)!=engine||!session_handle_matches(identity)||
       read_ptr(slot)!=target||!read_at(target,actual,sizeof actual)||memcmp(actual,expected,sizeof expected))return 0;
    DWORD old=0;if(!VirtualProtect((void*)slot,8,PAGE_READWRITE,&old))return 0;
    runtime_join_event_original=(EngineProgress)target;runtime_join_event_engine=identity;
    PVOID previous=InterlockedCompareExchangePointer((PVOID volatile*)slot,(PVOID)runtime_join_event_trace,(PVOID)target);
    VirtualProtect((void*)slot,8,old,&old);
    if(previous!=(PVOID)target){runtime_join_event_original=NULL;memset(&runtime_join_event_engine,0,sizeof runtime_join_event_engine);return 0;}
    line("{\"runtime_network\":\"join-event-observer-installed\",\"max_events\":16,\"window_ms\":60000,\"native_arguments_modified\":false}\r\n");return 1;
}
#ifdef ME3_RUNTIME_FIXTURE
static unsigned runtime_join_event_fixture_calls;
static void *runtime_join_event_fixture_self,*runtime_join_event_fixture_title,*runtime_join_event_fixture_message;
static int runtime_join_event_fixture_args;
static void runtime_join_event_fixture_original(void *self,int32_t kind,void *title,void *message) {
    ++runtime_join_event_fixture_calls;
    runtime_join_event_fixture_args=self==runtime_join_event_fixture_self&&kind==4&&
        title==runtime_join_event_fixture_title&&message==runtime_join_event_fixture_message&&GetLastError()==ERROR_PATH_NOT_FOUND;
    SetLastError(ERROR_ACCESS_DENIED);
}
static int runtime_join_event_selftest(void) {
    LONG saved=runtime_join_event_rows;HANDLE saved_log=log_file;log_file=INVALID_HANDLE_VALUE;
    wchar_t text[]=L"Synthetic connection failure";GameString title={0},message={text,29,29};
    /* Include the actual terminating NUL; do not depend on a guessed length. */
    message.count=message.max=(int32_t)wcslen(text)+1;
    runtime_join_event_fixture_self=&text;runtime_join_event_fixture_title=&title;runtime_join_event_fixture_message=&message;
    runtime_join_event_fixture_calls=0;runtime_join_event_rows=0;
    SetLastError(ERROR_PATH_NOT_FOUND);
    runtime_join_event_forward(runtime_join_event_fixture_original,&text,4,&title,&message,1,1000);
    int ok=runtime_join_event_fixture_calls==1&&runtime_join_event_fixture_args&&GetLastError()==ERROR_ACCESS_DENIED&&runtime_join_event_rows==1;
    runtime_join_event_rows=16;SetLastError(ERROR_PATH_NOT_FOUND);
    runtime_join_event_forward(runtime_join_event_fixture_original,&text,4,&title,&message,1,1100);
    ok=ok&&runtime_join_event_fixture_calls==2&&runtime_join_event_fixture_args&&runtime_join_event_rows==17;
    SetLastError(ERROR_PATH_NOT_FOUND);
    runtime_join_event_forward(runtime_join_event_fixture_original,&text,4,&title,&message,0,1200);
    ok=ok&&runtime_join_event_fixture_calls==3&&runtime_join_event_fixture_args&&runtime_join_event_rows==17;
    ok=ok&&runtime_join_event_due(1,UINT32_MAX-15,234,1,1)&&!runtime_join_event_due(1,1000,61000,1,1)&&
        !runtime_join_event_due(0,1000,1001,1,1)&&!runtime_join_event_due(1,1000,1001,0,1)&&!runtime_join_event_due(1,1000,1001,1,0);
    runtime_join_event_rows=saved;log_file=saved_log;return ok;
}
#endif
#endif
