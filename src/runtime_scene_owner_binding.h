#ifndef ME3_RUNTIME_SCENE_OWNER_BINDING_H
#define ME3_RUNTIME_SCENE_OWNER_BINDING_H
/* The selected later Liara conversation authors Owner as FindByTag
 * hench_liara. The caller independently proves the actual native Matinee,
 * owner connection and natural mode0 Owner query, with a NULL stock result.
 * This supplies only that existing local actor; no world or engine call. */
typedef struct RuntimeSceneOwnerLevel {
    GuestCameraStamp level;
    GameArray actors;
} RuntimeSceneOwnerLevel;
typedef struct RuntimeSceneOwnerCache {
    uintptr_t base,root_key,info_key;
    GuestCameraStamp root,info,actor;
    GameArray levels;
    RuntimeSceneOwnerLevel lists[65];
    unsigned count,actor_list,actor_slot,learn_list,learn_slot;
    uint32_t name_index,tag[2],unique[2];
    ULONGLONG checked,completed;
    int have_name,valid,have_attempt;
} RuntimeSceneOwnerCache;
static RuntimeSceneOwnerCache runtime_scene_owner_cache;
static int runtime_scene_owner_path(const char *path) {
    static const char *prefixes[]={
        "BioD_ProMar_205BLiaraConv.promar_intro_liara_m_d.Node_Data_Sequence.SeqAct_Interp_",
        "BioD_ProMar_205BLiaraConv_LOC_INT.promar_intro_liara_m_d.Node_Data_Sequence.SeqAct_Interp_",
        "promar_intro_liara_m_d.Node_Data_Sequence.SeqAct_Interp_"};
    if(!path)return 0;
    for(unsigned i=0;i<sizeof prefixes/sizeof *prefixes;++i){size_t n=strlen(prefixes[i]);
        if(_strnicmp(path,prefixes[i],n))continue;
        const char *p=path+n;if(*p<'0'||*p>'9')return 0;
        for(;*p;++p)if(*p<'0'||*p>'9')return 0;
        return 1;
    }
    return 0;
}
static int runtime_scene_owner_expired(LARGE_INTEGER start,LARGE_INTEGER frequency) {
    LARGE_INTEGER now;
    return !QueryPerformanceCounter(&now)||now.QuadPart<start.QuadPart||
        now.QuadPart-start.QuadPart>=frequency.QuadPart*3/1000;
}
static int runtime_scene_owner_tag(uintptr_t base,const uint32_t tag[2],RuntimeSceneOwnerCache *c) {
    if(!tag[0]||tag[1])return 0;
    if(c->have_name)return tag[0]==c->name_index;
    char name[96];name_of(base,tag[0],name,sizeof name);
    if(_stricmp(name,"hench_liara"))return 0;
    c->name_index=tag[0];c->have_name=1;return 1;
}
static int runtime_scene_owner_headers(uintptr_t base,uintptr_t info,const NetworkActorScope *world,
        RuntimeSceneOwnerCache *c,int collect) {
    GameArray levels={0},again={0};uintptr_t entries[64]={0};unsigned total=0;
    if(!world||world->world!=read_ptr(base+0x18b6638)||world->info!=info||world->level!=read_ptr(world->world+0x88)||
       world->driver!=read_ptr(world->world+0x120)||!narrative_probe_object(base,info,"WorldInfo")||
       world->actors.count<1||read_ptr(world->actors.data)!=info||
       !scene_readiness_array(world->world+0x78,&levels,64)||
       (levels.count&&!read_at(levels.data,entries,(SIZE_T)levels.count*8)))return 0;
    if(!collect&&(!guest_camera_stamp_matches(base,&c->root,0)||!guest_camera_stamp_matches(base,&c->info,0)||
       c->root.pointer!=world->world||c->info.pointer!=info||memcmp(&c->levels,&levels,sizeof levels)))return 0;
    uintptr_t all[65]={world->level};unsigned count=1;
    for(int i=0;i<levels.count;++i){
        if(!entries[i])return 0;
        int seen=0;for(unsigned j=0;j<count;++j)if(all[j]==entries[i])seen=1;
        if(!seen)all[count++]=entries[i];
    }
    if(!collect&&count!=c->count)return 0;
    for(unsigned i=0;i<count;++i){GameArray actors={0};
        if(!narrative_probe_object(base,all[i],"Level")||!scene_readiness_array(all[i]+0x60,&actors,100000)||
           (unsigned)actors.count>100000-total)return 0;
        if(!i&&memcmp(&actors,&world->actors,sizeof actors))return 0;
        total+=(unsigned)actors.count;
        if(collect){if(!guest_camera_stamp_read(base,all[i],&c->lists[i].level,0))return 0;c->lists[i].actors=actors;}
        else if(c->lists[i].level.pointer!=all[i]||!guest_camera_stamp_matches(base,&c->lists[i].level,0)||
                memcmp(&c->lists[i].actors,&actors,sizeof actors))return 0;
    }
    if(!read_at(world->world+0x78,&again,sizeof again)||memcmp(&again,&levels,sizeof again))return 0;
    for(int i=0;i<levels.count;++i)if(read_ptr(levels.data+(SIZE_T)i*8)!=entries[i])return 0;
    if(collect){c->count=count;c->levels=levels;
        if(!guest_camera_stamp_read(base,world->world,&c->root,0)||!guest_camera_stamp_read(base,info,&c->info,0))return 0;}
    return 1;
}
static uintptr_t runtime_scene_owner_cached(uintptr_t base,uintptr_t info,const NetworkActorScope *world,
        RuntimeSceneOwnerCache *c,ULONGLONG now) {
    uint32_t words[6];uintptr_t slot=0;
    if(!c->valid||now<c->completed||now-c->completed>250||c->actor_list>=c->count||
       c->actor_slot>=(unsigned)c->lists[c->actor_list].actors.count||
       !runtime_scene_owner_headers(base,info,world,c,0)||!guest_camera_stamp_matches(base,&c->actor,0)||
       !read_at(c->lists[c->actor_list].actors.data+(SIZE_T)c->actor_slot*8,&slot,8)||slot!=c->actor.pointer||
       !guest_camera_actor_live(base,slot)||!is_kind_of(base,slot,"SFXPawn_Liara")||
       !read_at(slot+0x198,words,sizeof words)||memcmp(words,&info,8)||
       memcmp(words+2,c->tag,8)||memcmp(words+4,c->unique,8)||
       (!runtime_scene_owner_tag(base,words+2,c)&&!runtime_scene_owner_tag(base,words+4,c)))return 0;
    return slot;
}
static uintptr_t runtime_scene_owner_resolve(uintptr_t base,uintptr_t info,const NetworkActorScope *world) {
    RuntimeSceneOwnerCache *c=&runtime_scene_owner_cache;ULONGLONG now=GetTickCount64();
    if(GetCurrentThreadId()!=session_game_thread||g_runtime_config.role!=ME3_ROLE_GUEST||!world||world->info!=info)return 0;
    if(c->base!=base||c->root_key!=world->world||c->info_key!=info){
        memset(c,0,sizeof *c);c->base=base;c->root_key=world->world;c->info_key=info;
    }
    LARGE_INTEGER start,frequency;
    if(!QueryPerformanceFrequency(&frequency)||frequency.QuadPart<1000||!QueryPerformanceCounter(&start))return 0;
    uintptr_t cached=runtime_scene_owner_cached(base,info,world,c,now);
    if(cached)return runtime_scene_owner_expired(start,frequency)?0:cached;
    c->valid=0;if(c->have_attempt&&now>=c->checked&&now-c->checked<250)return 0;
    c->have_attempt=1;c->checked=now;
    if(!runtime_scene_owner_headers(base,info,world,c,1)||runtime_scene_owner_expired(start,frequency))return 0;
    unsigned found=0,match_list=0,match_slot=0;uintptr_t match=0;uint32_t matched_words[6]={0};
    unsigned begin_list=c->have_name?0:c->learn_list,begin_slot=c->have_name?0:c->learn_slot;
    if(begin_list>=c->count){begin_list=0;begin_slot=0;}
    for(unsigned level=begin_list;level<c->count;++level){const GameArray *actors=&c->lists[level].actors;
        unsigned first=level==begin_list?begin_slot:0;
        if(first>(unsigned)actors->count)first=0;
        for(unsigned at=first;at<(unsigned)actors->count;){uintptr_t pointers[128]={0};
            unsigned count=(unsigned)actors->count-at;if(count>128)count=128;
            if(!read_at(actors->data+(SIZE_T)at*8,pointers,(SIZE_T)count*8))return 0;
            for(unsigned i=0;i<count;++i){uintptr_t actor=pointers[i];uint32_t words[6];
                if(runtime_scene_owner_expired(start,frequency)){
                    if(!c->have_name){c->learn_list=level;c->learn_slot=at+i;}return 0;}
                if(!actor)continue;
                if(!read_at(actor+0x198,words,sizeof words))return 0;
                if(memcmp(words,&info,8))continue;
                int had_name=c->have_name;
                int matches=runtime_scene_owner_tag(base,words+2,c)||runtime_scene_owner_tag(base,words+4,c);
                /* Learning the immutable FName index may resume across
                 * bounded attempts. Actor selection always starts a fresh
                 * complete same-thread pass once the exact name is known. */
                if(!had_name&&c->have_name){c->learn_list=c->learn_slot=0;return 0;}
                if(!matches)continue;
                if(!guest_camera_actor_live(base,actor)||!is_kind_of(base,actor,"SFXPawn_Liara")||
                   !runtime_power_property(base,actor,"Actor","WorldInfo","ObjectProperty",0x198,8)||
                   !runtime_power_property(base,actor,"Actor","Tag","NameProperty",0x1a0,8)||
                   !runtime_power_property(base,actor,"Actor","UniqueTag","NameProperty",0x1a8,8))return 0;
                if(++found!=1)return 0;
                match=actor;match_list=level;match_slot=at+i;memcpy(matched_words,words,sizeof words);
            }
            at+=count;
        }
    }
    c->learn_list=c->learn_slot=0;
    if(found!=1||!runtime_scene_owner_headers(base,info,world,c,0)||runtime_scene_owner_expired(start,frequency)||
       !guest_camera_stamp_read(base,match,&c->actor,0)||!guest_camera_actor_live(base,match)||
       !is_kind_of(base,match,"SFXPawn_Liara"))return 0;
    uint32_t checked[6];if(!read_at(match+0x198,checked,sizeof checked)||memcmp(checked,matched_words,sizeof checked)||
       read_ptr(c->lists[match_list].actors.data+(SIZE_T)match_slot*8)!=match)return 0;
    c->actor_list=match_list;c->actor_slot=match_slot;memcpy(c->tag,matched_words+2,8);memcpy(c->unique,matched_words+4,8);
    c->completed=GetTickCount64();c->valid=1;
    return runtime_scene_owner_expired(start,frequency)?0:match;
}
/* Optional ordinary Tick prewarming avoids spending the first natural Owner
 * query only learning its immutable native name index. It does not admit any
 * action; the natural resolver must still pass the exact family gate below. */
static void runtime_scene_owner_prepare(uintptr_t base,uintptr_t info,const NetworkActorScope *world) {
    (void)runtime_scene_owner_resolve(base,info,world);
}
static uintptr_t runtime_scene_owner_actor(uintptr_t base,uintptr_t action,uintptr_t info,const NetworkActorScope *world) {
    GuestCameraStamp stamp={0};
    if(!narrative_probe_object(base,action,"SeqAct_Interp")||!guest_camera_stamp_read(base,action,&stamp,1)||
       !runtime_scene_owner_path(stamp.full_name))return 0;
    return runtime_scene_owner_resolve(base,info,world);
}
#ifdef ME3_RUNTIME_FIXTURE
static int runtime_scene_owner_binding_selftest(void) {
    return runtime_scene_owner_path("BioD_ProMar_205BLiaraConv.promar_intro_liara_m_d.Node_Data_Sequence.SeqAct_Interp_40")&&
        runtime_scene_owner_path("BioD_ProMar_205BLiaraConv_LOC_INT.promar_intro_liara_m_d.Node_Data_Sequence.SeqAct_Interp_0")&&
        runtime_scene_owner_path("promar_intro_liara_m_D.Node_Data_Sequence.SeqAct_Interp_40")&&
        !runtime_scene_owner_path("promar_airlock_m_D.Node_Data_Sequence.SeqAct_Interp_40")&&
        !runtime_scene_owner_path("promar_intro_liara_m_DForeign.Node_Data_Sequence.SeqAct_Interp_40")&&
        !runtime_scene_owner_path("promar_intro_liara_m_D.Node_Data_Sequence.SeqAct_Interp_40.Foreign")&&
        !runtime_scene_owner_path("BioD_ProMar_205ALiaraIntro_LOC_INT.promar_intro_liara_m_d.Node_Data_Sequence.SeqAct_Interp_0")&&
        !runtime_scene_owner_path("BioD_ProMar_205BLiaraConv_LOC_INT.promar_level02_b_d.Node_Data_Sequence.SeqAct_Interp_40")&&
        !runtime_scene_owner_path("BioD_ProMar_205BLiaraConv_LOC_INT.promar_intro_liara_m_d.Node_Data_Sequence.SeqAct_Interp_")&&
        !runtime_scene_owner_path("BioD_ProMar_205BLiaraConv_LOC_INT.promar_intro_liara_m_d.Node_Data_Sequence.SeqAct_Interp_40.Foreign");
}
#endif
#endif
