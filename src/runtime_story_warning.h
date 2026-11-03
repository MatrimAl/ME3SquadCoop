#ifndef ME3_RUNTIME_STORY_WARNING_H
#define ME3_RUNTIME_STORY_WARNING_H
/* Include after runtime_guest_vitals_hud.h. Draw only inside its validated
 * Canvas transaction, before that caller restores the original pen state.
 * This is a short local hint; it never acquires or changes gameplay input. */
enum {RUNTIME_STORY_WARNING_NONE,RUNTIME_STORY_WARNING_SHEPARD,RUNTIME_STORY_WARNING_SHUTTLE};
typedef struct RuntimeStoryWarning {
    SessionHandle pc,pawn,world;
    DWORD started;
    int reason,active;
} RuntimeStoryWarning;
static RuntimeStoryWarning runtime_story_warning;
#define RUNTIME_STORY_WARNING_MS 3000u
static const wchar_t *runtime_story_warning_text(int reason) {
    if(reason==RUNTIME_STORY_WARNING_SHEPARD)
        return L"Only Shepard can start this event.\nWait for the host.";
    if(reason==RUNTIME_STORY_WARNING_SHUTTLE)
        return L"Waiting for all players\nto leave the shuttle.";
    return L"";
}
static int runtime_story_warning_fresh(const RuntimeStoryWarning *q,DWORD now) {
    return q->active&&*runtime_story_warning_text(q->reason)&&now-q->started<RUNTIME_STORY_WARNING_MS;
}
static void runtime_story_warning_clear(void) {memset(&runtime_story_warning,0,sizeof runtime_story_warning);}
static int runtime_story_warning_enabled(void) {
    return runtime_story_warning_fresh(&runtime_story_warning,GetTickCount());
}
static int runtime_story_warning_owned(uintptr_t base,SessionContext c) {
    RuntimeStoryWarning *q=&runtime_story_warning;
    return runtime_story_warning_enabled()&&runtime_control_enabled&&runtime_scope_ready&&
        InterlockedCompareExchange(&g_enable_requested,0,0)&&!session_engine_blocked(c)&&
        c.pc==q->pc.pointer&&c.pawn==q->pawn.pointer&&c.world==q->world.pointer&&
        session_handle_matches(q->pc)&&session_handle_matches(q->pawn)&&session_handle_matches(q->world)&&
        guest_camera_actor_live(base,c.pc)&&guest_camera_actor_live(base,c.pawn)&&
        read_ptr(c.pc+0x198)==c.world&&read_ptr(c.pawn+0x198)==c.world&&
        read_ptr(c.pc+0x3bc)==c.pawn&&read_ptr(c.pawn+0x374)==c.pc;
}
static void runtime_story_warning_show(uintptr_t base,SessionContext c,int reason,DWORD now) {
    if(GetCurrentThreadId()!=session_game_thread||!*runtime_story_warning_text(reason)||
       !guest_camera_actor_live(base,c.pc)||!guest_camera_actor_live(base,c.pawn)||
       !guest_camera_actor_live(base,c.world)||read_ptr(c.pc+0x198)!=c.world||read_ptr(c.pawn+0x198)!=c.world)return;
    RuntimeStoryWarning next={session_capture_handle(c.pc),session_capture_handle(c.pawn),
        session_capture_handle(c.world),now,reason,1};
    if(session_handle_matches(next.pc)&&session_handle_matches(next.pawn)&&session_handle_matches(next.world))
        runtime_story_warning=next;
}
static void runtime_story_warning_draw(uintptr_t canvas,int32_t *size,float scale) {
    RuntimeStoryWarning *q=&runtime_story_warning;uintptr_t base=guest_vitals.base;
    if(!runtime_story_warning_owned(base,session_context(session_engine))||size[0]<320||size[1]<200||
       !isfinite(scale)||scale<=0)return;
    const wchar_t *message=runtime_story_warning_text(q->reason);int length=(int)wcslen(message);
    /* The longest first line determines the centered hint width. Shrink the
     * font at narrow resolutions instead of overflowing the safe area. */
    int widest=0,current=0;
    for(int i=0;i<length;++i){if(message[i]==L'\n')current=0;else if(++current>widest)widest=current;}
    float font=fminf(0.8f*scale,(size[0]-48.0f)/(fmaxf(1,(float)widest)*12.0f));
    float width=widest*12.0f*font,x=fmaxf(24,(size[0]-width)*0.5f),y=fmaxf(32,size[1]*0.18f);
    float shadow_pos[2]={x+1,y+1},position[2]={x,y};
    unsigned char shadow[4]={10,10,10,240},amber[4]={105,215,255,255};
    GuestVitalsText text={{(uintptr_t)message,length+1,length+1},font,font};
    guest_vitals_call(canvas,&guest_vitals.pos,shadow_pos);guest_vitals_call(canvas,&guest_vitals.color,shadow);
    guest_vitals_call(canvas,&guest_vitals.text,&text);
    guest_vitals_call(canvas,&guest_vitals.pos,position);guest_vitals_call(canvas,&guest_vitals.color,amber);
    guest_vitals_call(canvas,&guest_vitals.text,&text);
}
static int runtime_story_warning_selftest(void) {
    RuntimeStoryWarning q={0};q.active=1;q.reason=RUNTIME_STORY_WARNING_SHEPARD;q.started=100;
    if(!runtime_story_warning_fresh(&q,100)||!runtime_story_warning_fresh(&q,3099)||
       runtime_story_warning_fresh(&q,3100)||runtime_story_warning_fresh(&q,99))return 0;
    q.started=UINT32_MAX-9;
    if(!runtime_story_warning_fresh(&q,0)||runtime_story_warning_fresh(&q,3000))return 0;
    q.reason=RUNTIME_STORY_WARNING_SHUTTLE;
    if(!runtime_story_warning_fresh(&q,0))return 0;
    q.reason=RUNTIME_STORY_WARNING_NONE;if(runtime_story_warning_fresh(&q,0))return 0;
    q.reason=99;if(runtime_story_warning_fresh(&q,0))return 0;
    q.reason=RUNTIME_STORY_WARNING_SHEPARD;q.active=0;
    return !runtime_story_warning_fresh(&q,0);
}
#endif
