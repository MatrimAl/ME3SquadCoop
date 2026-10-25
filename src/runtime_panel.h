#ifndef ME3_RUNTIME_PANEL_H
#define ME3_RUNTIME_PANEL_H
/* Optional local control surface. Pointer-free shared memory, no remote
 * transport or external process injection. Commands run only on game Tick. */
#pragma pack(push,4)
typedef struct RuntimePanelFrame {
    uint32_t magic,version,bytes,pid;
    uint64_t created;
    LONG sequence;
    uint32_t role,enabled,startup,netmode,companion_owned,power_status,power_scope,power_count;
    uint32_t ack_id,ack_result,tick_ms;
    char power_names[64][80];
} RuntimePanelFrame;
typedef struct RuntimePanelMemory {
    RuntimePanelFrame frame;
    LONG command;
    uint32_t command_id,scope,index,sent_at;
} RuntimePanelMemory;
#pragma pack(pop)
_Static_assert(sizeof(RuntimePanelFrame)==5192,"panel frame v2");
_Static_assert(offsetof(RuntimePanelMemory,command)==5192,"panel mailbox alignment");
_Static_assert(sizeof(RuntimePanelMemory)==5212,"panel protocol v2");
static RuntimePanelMemory *runtime_panel_memory;
static HANDLE runtime_panel_mapping;
static uint64_t runtime_panel_created;
static DWORD runtime_panel_published;
static uint32_t runtime_panel_ack,runtime_panel_result;
static void runtime_panel_prepare(void) {
    wchar_t opted[2]={0},name[96];int present=0;
    if(!direct_ip_native_read_environment(L"ME3_SQUAD_COOP_PANEL",opted,2,&present)||
       !present||wcscmp(opted,L"1"))return;
    FILETIME created,exit_time,kernel,user;
    if(!GetProcessTimes(GetCurrentProcess(),&created,&exit_time,&kernel,&user))return;
    runtime_panel_created=((uint64_t)created.dwHighDateTime<<32)|created.dwLowDateTime;
    _snwprintf(name,96,L"Local\\ME3SquadCoopPanel_%lu",GetCurrentProcessId());
    HANDLE mapping=CreateFileMappingW(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,sizeof(RuntimePanelMemory),name);
    DWORD error=GetLastError();
    if(!mapping)return;
    if(error==ERROR_ALREADY_EXISTS){CloseHandle(mapping);line("{\"runtime_panel\":\"unavailable-existing-name\"}\r\n");return;}
    RuntimePanelMemory *memory=MapViewOfFile(mapping,FILE_MAP_READ|FILE_MAP_WRITE,0,0,sizeof *memory);
    if(!memory){CloseHandle(mapping);return;}
    memset(memory,0,sizeof *memory);runtime_panel_mapping=mapping;runtime_panel_memory=memory;
    line("{\"runtime_panel\":\"prepared\",\"commands_on_tick\":true}\r\n");
}
/* Mailbox values are untrusted local input. They cannot supply an address,
 * engine function, console string, caster, target or new endpoint. */
static void runtime_panel_commands(void) {
    RuntimePanelMemory *memory=runtime_panel_memory;if(!memory)return;
    LONG command=InterlockedCompareExchange(&memory->command,0,0);
    if(command<=0)return; /* -1 is a client reserving the mailbox. */
    MemoryBarrier();uint32_t id=memory->command_id,scope=memory->scope,index=memory->index,sent_at=memory->sent_at;
    MemoryBarrier();
    if(InterlockedCompareExchange(&memory->command,0,command)!=command)return;
    if(!id||id==runtime_panel_ack)return;
    /* This limits mailbox delivery delay only. A native join request which
     * was accepted in time may still wait for ordinary player/story readiness. */
    if((DWORD)(GetTickCount()-sent_at)>2000) {
        runtime_panel_ack=id;runtime_panel_result=3;return;
    }
    int accepted=0;
    if(g_dlc_active&&g_config_status==ME3_CONFIG_READY) {
        switch(command) {
            case 1:accepted=runtime_request_listen();break;
            case 2:accepted=runtime_request_join();break;
            case 3:accepted=runtime_request_rejoin();break;
            case 4:accepted=runtime_request_power(scope,index);break;
            case 5:InterlockedExchange(&g_enable_requested,0);accepted=1;break;
            case 6:InterlockedExchange(&g_enable_requested,1);accepted=1;break;
            default:break;
        }
    }
    runtime_panel_ack=id;runtime_panel_result=accepted?1:2;
}
static void runtime_panel_publish(uintptr_t base,uintptr_t engine,DWORD now) {
    RuntimePanelMemory *memory=runtime_panel_memory;
    if(!memory||(runtime_panel_published&&now-runtime_panel_published<250))return;
    runtime_panel_published=now;
    RuntimePanelFrame frame={0};frame.magic=UINT32_C(0x4d334350);frame.version=2;
    frame.bytes=sizeof(RuntimePanelMemory);frame.pid=GetCurrentProcessId();frame.created=runtime_panel_created;
    frame.role=g_runtime_config.role;frame.enabled=runtime_control_enabled;
    frame.startup=InterlockedCompareExchange(&runtime_network_start_status,0,0);frame.netmode=255;
    SessionContext c=session_context(engine);unsigned char net=255;
    if(session_actor_live(base,c.world)&&read_at(c.world+0x7c8,&net,1))frame.netmode=net;
    frame.companion_owned=g_runtime_config.role==ME3_ROLE_HOST?session_active:
        guest_player_context_basic_owned(base,c.pc,c.pawn,c.world);
    frame.power_status=InterlockedCompareExchange64(&runtime_power_request,0,0)?RUNTIME_POWER_QUEUED:
        InterlockedCompareExchange(&runtime_power_status,0,0);
    frame.power_scope=InterlockedCompareExchange(&runtime_power_scope,0,0);
    int count=InterlockedCompareExchange(&runtime_power_count,0,0);
    if(frame.power_scope&&count>=0&&count<=64) {
        frame.power_count=count;
        for(int i=0;i<count;++i)name_of(base,runtime_power_catalog.names[i][0],frame.power_names[i],80);
    }
    frame.ack_id=runtime_panel_ack;frame.ack_result=runtime_panel_result;frame.tick_ms=now;
    /* Client snapshots retry if either sequence read is odd or differs. The
     * command mailbox lies outside this frame and is never overwritten here. */
    InterlockedIncrement(&memory->frame.sequence);MemoryBarrier();
    memcpy(&memory->frame,&frame,offsetof(RuntimePanelFrame,sequence));
    memcpy((unsigned char*)&memory->frame+offsetof(RuntimePanelFrame,role),
        (unsigned char*)&frame+offsetof(RuntimePanelFrame,role),sizeof frame- offsetof(RuntimePanelFrame,role));
    MemoryBarrier();InterlockedIncrement(&memory->frame.sequence);
}
#endif
