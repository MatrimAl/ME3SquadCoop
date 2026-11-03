#ifndef ME3_PROBE_GUARDS_FIXTURE_H
#define ME3_PROBE_GUARDS_FIXTURE_H
static unsigned dlc_resolve_fixture_calls;
static void *dlc_resolve_fixture_self;
static const wchar_t *dlc_resolve_fixture_name;
static GameString *dlc_resolve_fixture_output;
static int dlc_resolve_fixture_original(void *self,const wchar_t *name,GameString *output) {
    ++dlc_resolve_fixture_calls;
    int valid=self==dlc_resolve_fixture_self&&name==dlc_resolve_fixture_name&&output==dlc_resolve_fixture_output;
    if(output)*output=(GameString){(wchar_t*)L"CookedPCConsole\\Default_DLC_MOD_ME3SquadCoop.bin",46,46};
    SetLastError(ERROR_PATH_NOT_FOUND);return valid?7:0;
}
__declspec(dllexport) int SquadProbeDlcResolveSelfTest(void) {
    DlcResolveOriginal saved=dlc_resolve_original;LONG rows=dlc_resolve_rows;HANDLE saved_log=log_file;
    log_file=INVALID_HANDLE_VALUE;dlc_resolve_original=dlc_resolve_fixture_original;dlc_resolve_rows=0;
    wchar_t selected[64]=L"Default_DLC_MOD_ME3SquadCoop.bin",other[64]=L"Default_Other.bin";
    GameString output={0};dlc_resolve_fixture_calls=0;dlc_resolve_fixture_self=&output;dlc_resolve_fixture_output=&output;
    int ok=dlc_resolve_name(selected)&&!dlc_resolve_name(other)&&!dlc_resolve_name(NULL);
    dlc_resolve_fixture_name=selected;
    ok=ok&&dlc_resolve_forward(&output,selected,&output)==7&&GetLastError()==ERROR_PATH_NOT_FOUND&&
        dlc_resolve_fixture_calls==1&&dlc_resolve_rows==1&&output.data;
    dlc_resolve_fixture_name=other;
    ok=ok&&dlc_resolve_forward(&output,other,&output)==7&&GetLastError()==ERROR_PATH_NOT_FOUND&&
        dlc_resolve_fixture_calls==2&&dlc_resolve_rows==1;
    dlc_resolve_fixture_name=selected;dlc_resolve_rows=16;
    ok=ok&&dlc_resolve_forward(&output,selected,&output)==7&&dlc_resolve_fixture_calls==3&&dlc_resolve_rows==17;
    int saved_trace=probe_dlc_io_trace,saved_only=probe_guards_only,saved_installed=dlc_resolve_installed;
    unsigned saved_attempts=dlc_resolve_install_attempts;
    const char *saved_reason=dlc_resolve_install_reason;
    unsigned char *image=VirtualAlloc(NULL,0x1900000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!image)ok=0;
    else {
        uintptr_t base=(uintptr_t)image,manager=base+0x1000,vt=base+0x12ff310,target=base+0xa48b80;
        probe_dlc_io_trace=probe_guards_only=1;
        ok=ok&&!dlc_resolve_install(base)&&!strcmp(dlc_resolve_install_reason,"manager-not-ready");
        memcpy(image+0x17cd5d0,&manager,8);memcpy((void*)manager,&vt,8);memcpy((void*)(vt+0xe8),&target,8);
        ok=ok&&!dlc_resolve_install(base)&&!strcmp(dlc_resolve_install_reason,"wrapper-pins");
        const unsigned char pins[]={0x48,0x83,0xec,0x28,0x45,0x33,0xc9,0xe8,0xc4,0xd7,0xff,0xff,0x33,0xc9,0x48,0x85,0xc0,0x0f,0x95,0xc1,0x8b,0xc1,0x48,0x83,0xc4,0x28,0xc3};
        memcpy((void*)target,pins,sizeof pins);
        dlc_resolve_install_attempts=140;dlc_resolve_installed=0;dlc_resolve_install_observe(base);
        ok=ok&&read_ptr(vt+0xe8)==target&&dlc_resolve_install_attempts==140;
        dlc_resolve_install_attempts=0;dlc_resolve_install_observe(base);
        ok=ok&&dlc_resolve_installed&&dlc_resolve_install_attempts==1&&read_ptr(vt+0xe8)==(uintptr_t)dlc_resolve_forward;
        dlc_resolve_install_observe(base);ok=ok&&dlc_resolve_install_attempts==1;
        VirtualFree(image,0,MEM_RELEASE);
    }
    probe_dlc_io_trace=saved_trace;probe_guards_only=saved_only;dlc_resolve_installed=saved_installed;
    dlc_resolve_install_attempts=saved_attempts;dlc_resolve_install_reason=saved_reason;
    dlc_resolve_original=saved;dlc_resolve_rows=rows;log_file=saved_log;return ok;
}
static LPCWSTR probe_io_expected_path;
static unsigned probe_io_fixture_calls,probe_io_fixture_args;
static HANDLE probe_io_fixture_result;
static HANDLE WINAPI probe_io_fixture_open(LPCWSTR path,DWORD access,DWORD share,LPSECURITY_ATTRIBUTES sa,
    DWORD creation,DWORD attributes,HANDLE template_file) {
    ++probe_io_fixture_calls;
    probe_io_fixture_args=path==probe_io_expected_path&&access==GENERIC_READ&&share==FILE_SHARE_READ&&
        sa==NULL&&creation==OPEN_EXISTING&&attributes==FILE_ATTRIBUTE_NORMAL&&template_file==NULL;
    SetLastError(ERROR_FILE_NOT_FOUND);return probe_io_fixture_result;
}
__declspec(dllexport) int SquadProbeDlcIoSelfTest(const wchar_t *folder) {
    if(!folder||wcslen(folder)>MAX_PATH-90)return 0;
    wchar_t saved_root[MAX_PATH],path[MAX_PATH];wcscpy(saved_root,root);wcscpy(root,folder);
    FileW saved_file=original_file;int saved_trace=probe_dlc_io_trace,saved_only=probe_guards_only;
    LONG saved_rows=probe_dlc_io_rows;HANDLE saved_log=log_file;log_file=INVALID_HANDLE_VALUE;
    probe_dlc_io_trace=1;probe_guards_only=1;probe_dlc_io_rows=0;
    _snwprintf(path,MAX_PATH,L"%ls\\BioGame\\DLC\\dlc_mod_me3squadcoop\\CookedPCConsole\\Mount.dlc",folder);
    int ok=probe_dlc_io_selected(path)&&!probe_dlc_io_selected(L"Z:\\outside\\DLC_MOD_ME3SquadCoop\\Mount.dlc");
    probe_guards_only=0;ok=ok&&!probe_dlc_io_selected(path);probe_guards_only=1;
    probe_dlc_io_trace=0;ok=ok&&!probe_dlc_io_selected(path);probe_dlc_io_trace=1;
    original_file=probe_io_fixture_open;probe_io_expected_path=path;probe_io_fixture_calls=0;
    for(int i=0;i<2;++i) {
        probe_io_fixture_result=i?(HANDLE)(uintptr_t)0x1234:INVALID_HANDLE_VALUE;
        HANDLE result=private_file(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
        ok=ok&&result==probe_io_fixture_result&&GetLastError()==ERROR_FILE_NOT_FOUND&&
            probe_io_fixture_args&&probe_io_fixture_calls==(unsigned)i+1;
    }
    probe_dlc_io_rows=64;probe_dlc_io_record(path,INVALID_HANDLE_VALUE,2,GENERIC_READ,OPEN_EXISTING);
    ok=ok&&probe_dlc_io_rows==65;
    wcscpy(root,saved_root);original_file=saved_file;probe_dlc_io_trace=saved_trace;
    probe_guards_only=saved_only;probe_dlc_io_rows=saved_rows;log_file=saved_log;return ok;
}
/* Actual import-guard code operates only on this allocated synthetic image. */
static HRESULT WINAPI probe_guards_fixture_folder(HWND h,int f,HANDLE t,DWORD flags,LPWSTR out) {
    (void)h;(void)f;(void)t;(void)flags;(void)out;return E_INVALIDARG;
}
__declspec(dllexport) int SquadProbeGuardsOnlySelfTest(const wchar_t *folder) {
    if(!folder||wcslen(folder)>MAX_PATH-45)return 0;
    unsigned char *image=VirtualAlloc(NULL,0x1900000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);
    if(!image)return 0;
    wchar_t saved_root[MAX_PATH],saved_documents[MAX_PATH];char saved_role[8];
    wcscpy(saved_root,root);wcscpy(saved_documents,documents);strcpy(saved_role,test_role);
    FolderW old_folder=original_folder;FileW old_file=original_file;DirectoryW old_directory=original_directory;
    DeleteW old_delete=original_delete;MutexW old_mutex=original_mutex;
    int old_action=test_action,old_ready=probe_private_guards_ready,old_only=probe_guards_only;
    wcscpy(root,folder);_snwprintf(documents,MAX_PATH,L"%ls\\ME3SquadCoopRuntime\\Documents-client",folder);
    strcpy(test_role,"client");uintptr_t base=(uintptr_t)image;
    IMAGE_DOS_HEADER *dos=(IMAGE_DOS_HEADER*)image;dos->e_magic=0x5a4d;dos->e_lfanew=0x80;
    IMAGE_NT_HEADERS64 *nt=(IMAGE_NT_HEADERS64*)(image+0x80);nt->FileHeader.TimeDateStamp=1622077636;
    nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress=0x1000;
    const unsigned char landmark[]={0x48,0x8d,0x15,0xe3,0xf9,0xbb,0x00};memcpy(image+0x65fde6,landmark,sizeof landmark);
    IMAGE_IMPORT_DESCRIPTOR *descriptor=(IMAGE_IMPORT_DESCRIPTOR*)(image+0x1000);
    descriptor->Name=0x1800;descriptor->OriginalFirstThunk=0x2000;descriptor->FirstThunk=0x2100;
    strcpy((char*)image+0x1800,"fixture.dll");
    const char *names[]={"CreateFileW","CreateDirectoryW","DeleteFileW","CreateMutexW","SHGetFolderPathW"};
    uintptr_t functions[]={(uintptr_t)CreateFileW,(uintptr_t)CreateDirectoryW,(uintptr_t)DeleteFileW,
        (uintptr_t)CreateMutexW,(uintptr_t)probe_guards_fixture_folder};
    IMAGE_THUNK_DATA64 *imports=(IMAGE_THUNK_DATA64*)(image+0x2000),*iat=(IMAGE_THUNK_DATA64*)(image+0x2100);
    for(int i=0;i<5;++i) {imports[i].u1.AddressOfData=0x3000+i*64;
        strcpy((char*)image+imports[i].u1.AddressOfData+2,names[i]);iat[i].u1.Function=functions[i];}
    /* No private-guards acknowledgement means no orchestration. */
    probe_private_guards_ready=0;int ok=!probe_configure_orchestration(1);
    ok=ok&&probe_apply_private_guards(base)&&probe_private_guards_ready&&
        iat[0].u1.Function==(uintptr_t)private_file&&iat[1].u1.Function==(uintptr_t)private_directory&&
        iat[2].u1.Function==(uintptr_t)private_delete&&iat[3].u1.Function==(uintptr_t)private_engine_mutex&&
        iat[4].u1.Function==(uintptr_t)private_folder;
    if(ok) {
        test_action=6;session_enabled=pair_control_enabled=progress_enabled=1;
        ok=probe_configure_orchestration(1)&&probe_guards_only&&!test_action&&!session_enabled&&
            !pair_control_enabled&&!progress_enabled&&!client_prewarm&&!host_reload_mars;
        wchar_t actual[MAX_PATH]={0};ok=ok&&private_folder(NULL,5,NULL,0,actual)==S_OK&&!wcscmp(actual,documents);
        DWORD mode=OPEN_EXISTING;HANDLE denied=private_file(L"Z:\\outside-original-fixture.txt",GENERIC_WRITE,FILE_SHARE_READ,NULL,mode,0,NULL);
        ok=ok&&denied==INVALID_HANDLE_VALUE&&GetLastError()==ERROR_ACCESS_DENIED;
        if(denied!=INVALID_HANDLE_VALUE)CloseHandle(denied);
        uintptr_t engine=base+0x5000,vt=base+0x13983a0,target=base+0xb9dda0;
        memcpy(image+0x18b31d0,&engine,8);memcpy((void*)engine,&vt,8);memcpy((void*)(vt+0x2b0),&target,8);
        const unsigned char head[]={0x40,0x53,0x55,0x41,0x56,0x48,0x83,0xec,0x50,0x33,0xed};memcpy((void*)target,head,sizeof head);
        ok=ok&&!install_tick_bridge(base)&&read_ptr(vt+0x2b0)==target;
        /* Default mode still takes the normal client fallback; the supplied
         * fixture action-plan would have selected host-pair beforehand. */
        probe_guards_only=0;test_action=0;ok=ok&&probe_configure_orchestration(0)&&test_action==5;
        image[0x65fde6]^=1;ok=ok&&!probe_apply_private_guards(base)&&!probe_private_guards_ready;
        image[0x65fde6]^=1;ok=ok&&!probe_configure_orchestration(1);
    }
    wcscpy(root,saved_root);wcscpy(documents,saved_documents);strcpy(test_role,saved_role);
    original_folder=old_folder;original_file=old_file;original_directory=old_directory;original_delete=old_delete;original_mutex=old_mutex;
    test_action=old_action;probe_private_guards_ready=old_ready;probe_guards_only=old_only;
    session_enabled=pair_control_enabled=progress_enabled=0;VirtualFree(image,0,MEM_RELEASE);return ok;
}
__declspec(dllexport) int SquadProbeGuardsEnvironmentFixture(void) {return probe_guards_only_environment();}
#endif
