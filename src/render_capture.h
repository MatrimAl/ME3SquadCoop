/* Request the engine screenshot inside its own Draw path. A direct ReadPixels
 * call from Tick was experimentally rejected: viewport RHI surface is null there.
 * Dispatch is not image proof; inspect the generated bitmap separately. */
static int render_capture_count = 0;
static int render_capture(uintptr_t base, uintptr_t engine, const wchar_t *label) {
    uintptr_t pc=local_controller(engine);
    if (!is_live(base,pc)) return 0;
    wchar_t command[100];
    _snwprintf(command,100,L"SHOT %ls_%lu_%d",label,GetCurrentProcessId(),render_capture_count++);
    command[99]=0;
    InterlockedExchange(&capture_pending,1);
    ConsoleParms params={0};params.command.data=command;params.command.count=params.command.max=(int32_t)wcslen(command)+1;
    int dispatched=invoke(base,pc,"PlayerController","ConsoleCommand",&params);
    wchar_t output[400];char escaped[1700],row[1900];game_string((uintptr_t)&params.result,output,400);json_text(output,escaped,sizeof escaped);
    snprintf(row,sizeof row,"{\"capture_command_output\":\"%s\"}\r\n",escaped);line(row);
    line(dispatched?"{\"capture_request\":true,\"source\":\"ordinary-engine-shot\",\"image_verified\":false}\r\n":"{\"capture_request\":false}\r\n");
    return dispatched;
}
