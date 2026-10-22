#ifndef ME3_COMBAT_CONTEXT_SUPPORT_H
#define ME3_COMBAT_CONTEXT_SUPPORT_H

/* Read-only, <=1Hz; include after narrative_probe.h. No combat action is
 * invoked and no permission/squad/player pointers are written.
 * Exact private EXE b453871e...: native gate a5a9b0..a5aa0e; exec wrapper
 * b75250..b75296 calls it atb75284 and writes its bool result.
 * SDK: UWorld.PersistentLevel88; ULevelBase.Actors60;
 * BioWorldInfo.m_playerSquad8e8; BioBaseSquad.CachedPlayerPawn2a8,
 * flags2b0 combatEnabled4; BioPawn.flagsE18 combatPawn8.
 * BioPlayerInput.IsCombatEnabled: flags20401, returnbool0,size4.
 * See private LEMP-combat-gate-research.md. Pawn.Squad is a comparison,
 * never substituted for the actual native GWorld-derived squad.
 */
typedef struct CombatContextChain {
    uintptr_t world,level,info,squad,cached;
    GameArray actors;
    uint32_t squad_flags,pawn_flags;
    int world_read,world_valid,level_read,level_valid,actors_read,actors_valid;
    int info_read,info_valid,squad_read,squad_valid,cached_read,cached_valid;
    int squad_flags_read,pawn_flags_read;
} CombatContextChain;
typedef struct CombatContextGetter {
    uintptr_t base,input_class,function,owner;
    int32_t index,owner_index;
    uint64_t name,owner_name;
} CombatContextGetter;
static CombatContextGetter combat_context_getter_cache;
static int combat_context_actor(uintptr_t base,uintptr_t object,const char *kind) {
    return scene_readiness_actor(base,object)&&is_kind_of(base,object,kind);
}
static int combat_context_local_pc(uintptr_t base,uintptr_t engine,uintptr_t *pc) {
    GameArray players={0};uintptr_t player=0;*pc=0;
    if(!narrative_probe_object(base,engine,"Engine")||!read_at(engine+0x498,&players,sizeof players)||
       players.count<0||players.count>4||players.max<players.count||players.max>128||
       (players.count&&!players.data))return 0;
    if(!players.count)return 1;
    if(!read_at(players.data,&player,8))return 0;
    if(!player)return 1;
    return narrative_probe_object(base,player,"LocalPlayer")&&read_at(player+0x68,pc,8);
}
static int combat_context_layout(uintptr_t base) {
    static const unsigned char gate[]={0x48,0x83,0xec,0x28,0x48,0x8b,0x05,0x7d,0xbc,0xe5,0x00,0x48,0x8b,0x88,0x88,0x00,0x00,0x00,0x48,0x8b,0x41,0x60,0x48,0x8b,0x08,0xe8,0xf2,0x28,0x74,0xff,0x48,0x85,0xc0,0x74,0x34,0x48,0x8b,0x80,0xe8,0x08,0x00,0x00,0x48,0x85,0xc0,0x74,0x28,0x48,0x8b,0x88,0xa8,0x02,0x00,0x00,0x48,0x85,0xc9,0x74,0x1c,0xf6,0x80,0xb0,0x02,0x00,0x00,0x04,0x74,0x13,0xf6,0x81,0x18,0x0e,0x00,0x00,0x08,0x74,0x0a,0xb8,0x01,0x00,0x00,0x00,0x48,0x83,0xc4,0x28,0xc3,0x33,0xc0,0x48,0x83,0xc4,0x28,0xc3};
    static const unsigned char wrapper[]={0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20,0x48,0xff,0x42,0x28,0x49,0x8b,0xd8,0x48,0x8b,0x42,0x28,0x48,0x8b,0xf9,0x80,0x38,0x41,0x75,0x14,0x48,0x8b,0x4a,0x20,0x48,0xff,0xc0,0x45,0x33,0xc0,0x48,0x89,0x42,0x28,0xff,0x15,0x37,0xfb,0xc7,0x00,0x48,0x8b,0xcf,0xe8,0x27,0x57,0xee,0xff,0x89,0x03,0x48,0x8b,0x5c,0x24,0x30,0x48,0x83,0xc4,0x20,0x5f,0xc3};
    unsigned char a[sizeof gate],b[sizeof wrapper];
    return read_at(base+0xa5a9b0,a,sizeof a)&&!memcmp(a,gate,sizeof a)&&
        read_at(base+0xb75250,b,sizeof b)&&!memcmp(b,wrapper,sizeof b);
}
static void combat_context_read_chain(uintptr_t base,CombatContextChain *s) {
    memset(s,0,sizeof *s);
    s->world_read=read_at(base+0x18b6638,&s->world,8);
    s->world_valid=s->world_read&&narrative_probe_object(base,s->world,"World");
    s->level_read=s->world_valid&&read_at(s->world+0x88,&s->level,8);
    s->level_valid=s->level_read&&narrative_probe_object(base,s->level,"Level");
    s->actors_read=s->level_valid&&read_at(s->level+0x60,&s->actors,sizeof s->actors);
    s->actors_valid=s->actors_read&&s->actors.count>=0&&s->actors.count<=100000&&
        s->actors.max>=s->actors.count&&s->actors.max<=500000&&(!s->actors.count||s->actors.data);
    s->info_read=s->actors_valid&&s->actors.count>0&&read_at(s->actors.data,&s->info,8);
    s->info_valid=s->info_read&&combat_context_actor(base,s->info,"BioWorldInfo");
    s->squad_read=s->info_valid&&read_at(s->info+0x8e8,&s->squad,8);
    s->squad_valid=s->squad_read&&combat_context_actor(base,s->squad,"BioBaseSquad");
    s->cached_read=s->squad_valid&&read_at(s->squad+0x2a8,&s->cached,8);
    s->cached_valid=s->cached_read&&combat_context_actor(base,s->cached,"BioPawn");
    s->squad_flags_read=s->squad_valid&&read_at(s->squad+0x2b0,&s->squad_flags,4);
    s->pawn_flags_read=s->cached_valid&&read_at(s->cached+0xe18,&s->pawn_flags,4);
}
static int combat_context_safe_native(const CombatContextChain *s) {
    /* Native dereferences World/Level/Actors before its first null guard.
     * Guard even the branches which would return false, rather than using
     * an invalid loading/disposal chain as a callable input getter. */
    return s->info_valid&&s->squad_read&&(!s->squad||
        (s->squad_valid&&s->cached_read&&(!s->cached||
         (s->cached_valid&&s->squad_flags_read&&s->pawn_flags_read))));
}
static int combat_context_derived(const CombatContextChain *s,int pinned,int stable) {
    if(!pinned||!stable||!s->info_valid||!s->squad_read)return -1;
    if(!s->squad)return 0;
    if(!s->squad_valid||!s->cached_read)return -1;
    if(!s->cached)return 0;
    if(!s->squad_flags_read)return -1;
    if(!(s->squad_flags&4))return 0;
    if(!s->cached_valid||!s->pawn_flags_read)return -1;
    return (s->pawn_flags&8)?1:0;
}
static int combat_context_chain_equal(const CombatContextChain *a,const CombatContextChain *b) {
    /* Both snapshots are zero initialized; padding remains deterministic.
     * Includes array header, pointer identity, read/type validity and bits. */
    return !memcmp(a,b,sizeof *a);
}
static int combat_context_get(uintptr_t base,uintptr_t input,uint32_t *result) {
    if(!narrative_probe_object(base,input,"BioPlayerInput"))return 0;
    uint64_t object_flags=0;
    if(!read_at(input+0xc,&object_flags,8)||(object_flags&(UINT64_C(1)<<61)))return 0;
    uintptr_t klass=read_ptr(input+0x50),fn=0;CombatContextGetter *c=&combat_context_getter_cache;
    int32_t index=-1,owner_index=-1;uint64_t name=0,owner_name=0;
    if(c->base==base&&c->input_class==klass&&is_live(base,c->function)&&is_live(base,c->owner)&&
       read_at(c->function+8,&index,4)&&index==c->index&&read_at(c->function+0x48,&name,8)&&name==c->name&&
       read_at(c->owner+8,&owner_index,4)&&owner_index==c->owner_index&&read_at(c->owner+0x48,&owner_name,8)&&owner_name==c->owner_name&&
       read_ptr(c->function+0x40)==c->owner)fn=c->function;
    if(!fn) {
        uintptr_t seen_classes[16];int classes=0;
        while(is_live(base,klass)&&classes<16&&!fn) {
            for(int i=0;i<classes;++i)if(seen_classes[i]==klass)return 0;
            seen_classes[classes++]=klass;char text[96];object_name(base,klass,text,sizeof text);
            if(!strcmp(text,"BioPlayerInput")) {
                uintptr_t child=read_ptr(klass+0x70),seen[256];int count=0;
                while(child&&count<256) {
                    if(!is_live(base,child))return 0;
                    for(int i=0;i<count;++i)if(seen[i]==child)return 0;
                    seen[count++]=child;object_name(base,child,text,sizeof text);
                    if(!strcmp(text,"IsCombatEnabled")&&is_kind_of(base,child,"Function")&&read_ptr(child+0x40)==klass){fn=child;break;}
                    child=read_ptr(child+0x68);
                }
            }
            klass=read_ptr(klass+0x60);
        }
    }
    if(!narrative_probe_object(base,fn,"Function"))return 0;
    uintptr_t owner=read_ptr(fn+0x40),native=0;uint32_t flags=0;uint16_t size=0,offset=0;unsigned char count=0;
    char fn_name[96],class_name[96];object_name(base,fn,fn_name,sizeof fn_name);
    if(!narrative_probe_object(base,owner,"Class"))return 0;
    object_name(base,owner,class_name,sizeof class_name);
    if(strcmp(fn_name,"IsCombatEnabled")||strcmp(class_name,"BioPlayerInput")||
       !read_at(fn+0xe0,&flags,4)||flags!=0x20401||!read_at(fn+0xe6,&size,2)||size!=4||
       !read_at(fn+0xe8,&offset,2)||offset!=0||!read_at(fn+0xea,&count,1)||count!=1||
       !read_at(fn+0xd8,&native,8)||native!=base+0xb75250||
       !read_at(fn+8,&index,4)||!read_at(fn+0x48,&name,8)||
       !read_at(owner+8,&owner_index,4)||!read_at(owner+0x48,&owner_name,8))return 0;
    uintptr_t pe=read_ptr(read_ptr(input)+0x230);
    if(pe<base||pe>=base+0x1300000)return 0;
    c->base=base;c->input_class=read_ptr(input+0x50);c->function=fn;c->owner=owner;
    c->index=index;c->name=name;c->owner_index=owner_index;c->owner_name=owner_name;
    *result=UINT32_C(0x7fcdeeed); /* Distinguish a rejected/no-op PE from false. */
    ((ProcessEvent)pe)((void*)input,(void*)fn,result,NULL);return 1;
}
#endif
