#ifndef ME3_SCENE_INTERP_CAPABILITY_PROTOCOL_H
#define ME3_SCENE_INTERP_CAPABILITY_PROTOCOL_H
#include <stddef.h>
#include <wchar.h>
/* Version 5 also covers the actual localized Package outers and the Mars
 * airlock scene. Older guests do not guard these new action identities.
 * Requires resident story guards, the natural Player resolver,
 * an independently authenticated current host-Shepard actor identity, and
 * the validated local cinematic effect methods and isolated speech adapter.
 * The receiver separately authenticates the current owner-PC connection and
 * expires its local receipt. This fixed payload carries no action or clock. */
#define SCENE_INTERP_CAPABILITY_VERSION 5
#define SCENE_INTERP_CAPABILITY_CAPACITY 32
static const wchar_t scene_interp_capability_ready[]=L"ME3CI1 R 5";
static const wchar_t scene_interp_bootstrap_ready[]=L"ME3CI1 H 5";
static int scene_interp_bootstrap_parse(const wchar_t *text,size_t count) {
    const size_t required=sizeof scene_interp_bootstrap_ready/sizeof *scene_interp_bootstrap_ready;
    return text&&count==required&&!wmemcmp(text,scene_interp_bootstrap_ready,required);
}
static int scene_interp_bootstrap_format(wchar_t text[SCENE_INTERP_CAPABILITY_CAPACITY]) {
    if(!text)return 0;const size_t n=sizeof scene_interp_bootstrap_ready/sizeof *scene_interp_bootstrap_ready;
    wmemcpy(text,scene_interp_bootstrap_ready,n);return (int)n;
}
static int scene_interp_capability_parse(const wchar_t *text,size_t count) {
    const size_t required=sizeof scene_interp_capability_ready/sizeof *scene_interp_capability_ready;
    return text&&count<=SCENE_INTERP_CAPABILITY_CAPACITY&&count==required&&
        !wmemcmp(text,scene_interp_capability_ready,required);
}
static int scene_interp_capability_format(wchar_t text[SCENE_INTERP_CAPABILITY_CAPACITY]) {
    const size_t count=sizeof scene_interp_capability_ready/sizeof *scene_interp_capability_ready;
    if(!text)return 0;
    wmemcpy(text,scene_interp_capability_ready,count);return (int)count;
}
static int scene_interp_capability_protocol_selftest(void) {
    wchar_t text[SCENE_INTERP_CAPABILITY_CAPACITY]={0};
    int count=scene_interp_capability_format(text);
    wchar_t bootstrap[SCENE_INTERP_CAPABILITY_CAPACITY];int boot=scene_interp_bootstrap_format(bootstrap);
    if(boot!=11||!scene_interp_bootstrap_parse(bootstrap,(size_t)boot)||scene_interp_capability_parse(bootstrap,(size_t)boot)||
       scene_interp_bootstrap_parse(text,(size_t)count)||scene_interp_bootstrap_parse(NULL,11)||scene_interp_bootstrap_format(NULL)||
       scene_interp_bootstrap_parse(L"ME3CI1 H 3",11)||scene_interp_bootstrap_parse(L"ME3CI1 H 4",11)||
       scene_interp_bootstrap_parse(L"ME3CI1 H 5 extra",17)||
       scene_interp_bootstrap_parse(bootstrap,10)||scene_interp_bootstrap_parse(bootstrap,12))return 0;
    if(count!=11||!scene_interp_capability_parse(text,(size_t)count)||
       scene_interp_capability_parse(NULL,(size_t)count)||scene_interp_capability_format(NULL)||
       scene_interp_capability_parse(text,0)||scene_interp_capability_parse(text,10)||
       scene_interp_capability_parse(text,12)||scene_interp_capability_parse(text,33))return 0;
    const wchar_t *bad[]={
        L"ME3SC1 R 1",L"ME3DC1 R 1",L"ME3AM1 R 1",L"ME3RV1 R 1",
        L"ME3CI1 Q 1",L"ME3CI1 R 0",L"ME3CI1 R 1",L"ME3CI1 R 2",L"ME3CI1 R 3",L"ME3CI1 R 4",L"ME3CI1 R 05",
        L"ME3CI1 R 1 ",L"ME3CI1 R 1 extra",L"ME3CI1  R 1",L"ME3CI1 R\t1"};
    for(unsigned i=0;i<sizeof bad/sizeof *bad;++i)
        if(scene_interp_capability_parse(bad[i],wcslen(bad[i])+1))return 0;
    text[8]=0;if(scene_interp_capability_parse(text,(size_t)count))return 0;
    scene_interp_capability_format(text);text[8]=128;
    if(scene_interp_capability_parse(text,(size_t)count))return 0;
    scene_interp_capability_format(text);text[count-1]=L'x';
    return !scene_interp_capability_parse(text,(size_t)count);
}
#endif
