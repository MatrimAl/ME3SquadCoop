#ifndef ME3_LOOKUP_PROFILE_H
#define ME3_LOOKUP_PROFILE_H
/* Explicit research opt-in. Cumulative wall-clock CPU-path timings, not GPU
 * timings or an engine frame profiler. No game memory is changed. */
typedef struct LookupProfile {
    char owner[96],name[96];
    unsigned long long calls,total_us,max_us,hits,hit_total_us,hit_max_us,examined,emitted;
} LookupProfile;
static LookupProfile lookup_profiles[48];
static int lookup_profile_enabled=-1;
static long long lookup_profile_frequency;
static DWORD lookup_profile_emitted_at;
static unsigned lookup_profile_windows;
static unsigned long long lookup_clock(void) {
    if(lookup_profile_enabled<0){
        wchar_t value[8]={0};LARGE_INTEGER frequency;
        lookup_profile_enabled=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_LOOKUP_PROFILE",value,8)==1&&value[0]==L'1'&&
            QueryPerformanceFrequency(&frequency)&&frequency.QuadPart>0;
        if(lookup_profile_enabled)lookup_profile_frequency=frequency.QuadPart;
    }
    LARGE_INTEGER now;
    return lookup_profile_enabled&&QueryPerformanceCounter(&now)?(unsigned long long)now.QuadPart:0;
}
static void lookup_record(const char *owner,const char *name,unsigned long long started,int hit,unsigned examined) {
    if(!started)return;
    unsigned long long ended=lookup_clock();if(ended<started||!lookup_profile_frequency)return;
    unsigned long long us=(ended-started)*1000000/(unsigned long long)lookup_profile_frequency;
    for(unsigned i=0;i<48;++i){
        LookupProfile *p=&lookup_profiles[i];
        if(!p->calls){snprintf(p->owner,sizeof p->owner,"%s",owner);snprintf(p->name,sizeof p->name,"%s",name);}
        if(strcmp(p->owner,owner)||strcmp(p->name,name))continue;
        ++p->calls;p->total_us+=us;if(us>p->max_us)p->max_us=us;p->hits+=hit!=0;
        if(hit){p->hit_total_us+=us;if(us>p->hit_max_us)p->hit_max_us=us;}p->examined+=examined;return;
    }
}
static void lookup_profile_emit(void) {
    if(!lookup_clock()||lookup_profile_windows>=128)return;
    DWORD now=GetTickCount();if(now-lookup_profile_emitted_at<5000)return;
    lookup_profile_emitted_at=now;++lookup_profile_windows;
    for(unsigned i=0;i<48;++i){
        LookupProfile *p=&lookup_profiles[i];if(!p->calls||p->emitted==p->calls)continue;
        char owner[200],name[200],row[1000];identity_quote(p->owner,owner,sizeof owner);identity_quote(p->name,name,sizeof name);
        snprintf(row,sizeof row,"{\"lookup_profile\":\"cumulative\",\"owner\":\"%s\",\"name\":\"%s\",\"calls\":%llu,\"total_us\":%llu,\"max_us\":%llu,\"cache_hits\":%llu,\"cache_hit_total_us\":%llu,\"cache_hit_max_us\":%llu,\"objects_examined\":%llu,\"monotonic_ms\":%lu,\"gpu_timing_measured\":false}\r\n",
            owner,name,p->calls,p->total_us,p->max_us,p->hits,p->hit_total_us,p->hit_max_us,p->examined,now);line(row);p->emitted=p->calls;
    }
}
#endif
