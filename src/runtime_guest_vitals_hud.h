#ifndef ME3_RUNTIME_GUEST_VITALS_HUD_H
#define ME3_RUNTIME_GUEST_VITALS_HUD_H
/* Guest Canvas display after the existing BioHUD.PostRender. Values come
 * from the controlled pawn's damage module and owned shield inventory.
 * Never change health, shield state, target bars or the Scaleform movie. */
typedef struct GuestVitalsField {const char *name,*kind;int offset,size;uint64_t flags;} GuestVitalsField;
typedef struct GuestVitalsMethod {GuestCameraStamp object;uintptr_t entry;uint32_t flags;} GuestVitalsMethod;
typedef struct GuestVitalsValues {int hp_valid,shield_valid;float hp,hp_max,shield,shield_max;} GuestVitalsValues;
typedef struct GuestVitalsHud {
    GuestCameraStamp hud,pc,pawn,world;
    GuestCameraStamp sample_pc,sample_pawn,sample_world;
    unsigned long sample_token;
    GuestVitalsMethod post,line,text,pos,color;
    uintptr_t base,slot;ProcessEvent original;
    DWORD prepared,sampled;int installed,busy,hp_valid,shield_valid,draw_reported;
    float hp,hp_max,shield,shield_max;
} GuestVitalsHud;
static GuestVitalsHud guest_vitals;
static int (*guest_vitals_extra_enabled)(void);
static int (*guest_vitals_extra_owned)(uintptr_t,SessionContext);
static void (*guest_vitals_extra_draw)(uintptr_t,int32_t*,float);
static void (*guest_vitals_extra_sample)(uintptr_t,SessionContext,DWORD);
static int guest_vitals_enabled(void) {
    static int value=-1;
    if(value<0){wchar_t text[8]={0};DWORD n=GetEnvironmentVariableW(L"ME3_SQUAD_COOP_GUEST_VITALS_HUD",text,8);
        value=!n||(n==1&&text[0]==L'1');}
    return value;
}
static int guest_vitals_fraction(float value,float maximum,float *fraction) {
    if(!isfinite(value)||!isfinite(maximum)||value<0||maximum<=0||maximum>1e9f||value>maximum*4)return 0;
    *fraction=fminf(1,value/maximum);return 1;
}
static int guest_vitals_owned(uintptr_t base,SessionContext c) {
    return runtime_control_enabled&&runtime_scope_ready&&g_runtime_config.role==ME3_ROLE_GUEST&&
        InterlockedCompareExchange(&g_enable_requested,0,0)&&!session_engine_blocked(c)&&
        guest_player_context_registered_for(c.pc,c.pawn)&&guest_player_context_basic_owned(base,c.pc,c.pawn,c.world)&&
        is_kind_of(base,c.pawn,"SFXPawn_Ashley");
}
static int guest_vitals_hud_owned(uintptr_t base,SessionContext c) {
    return (guest_vitals_enabled()&&guest_vitals_owned(base,c))||
        (guest_vitals_extra_enabled&&guest_vitals_extra_enabled()&&guest_vitals_extra_owned&&guest_vitals_extra_owned(base,c));
}
static int guest_vitals_method(uintptr_t base,const char *owner,const char *name,uint32_t flags,
        unsigned size,const GuestVitalsField *params,unsigned count,GuestVitalsMethod *out) {
    uintptr_t fn=function_named(base,owner,name),field=read_ptr(fn+0x70),entry=read_ptr(fn+0xd8);
    uint32_t actual=0;uint16_t bytes=0,ret=0;unsigned char n=0;GuestCameraStamp stamp;char declaring[96];
    object_name(base,read_ptr(fn+0x40),declaring,sizeof declaring);
    if(strcmp(declaring,owner)||!guest_camera_stamp_read(base,fn,&stamp,1)||!narrative_probe_object(base,fn,"Function")||
       !read_at(fn+0xe0,&actual,4)||actual!=flags||!read_at(fn+0xe6,&bytes,2)||bytes!=size||
       !read_at(fn+0xe8,&ret,2)||ret!=65535||!read_at(fn+0xea,&n,1)||n!=count||
       entry<base+0x1000||entry>=base+0x1300000||((flags&0x400)?entry==base+0xf52b0:entry!=base+0xf52b0))return 0;
    unsigned found=0,seen=0;
    for(;field&&seen<128;++seen){uint64_t pf=0;char text[96];
        if(!is_live(base,field)||read_ptr(field+0x40)!=fn||!read_at(field+0x78,&pf,8))return 0;
        if(pf&0x80){int match=-1,offset=-1,dim=0,psize=0;object_name(base,field,text,sizeof text);
            for(unsigned j=0;j<count;++j)if(!strcmp(text,params[j].name))match=(int)j;
            if(match<0||(found&(1u<<match))||!narrative_probe_object(base,field,params[match].kind)||
               !read_at(field+0x94,&offset,4)||offset!=params[match].offset||!read_at(field+0x74,&psize,4)||psize!=params[match].size||
               !read_at(field+0x70,&dim,4)||dim!=1||pf!=params[match].flags)return 0;
            found|=1u<<match;
        }
        field=read_ptr(field+0x68);
    }
    if(field||found!=((1u<<count)-1)||!guest_camera_stamp_matches(base,&stamp,1))return 0;
    *out=(GuestVitalsMethod){stamp,entry,flags};return 1;
}
static int guest_vitals_method_live(uintptr_t base,const GuestVitalsMethod *m) {
    uint32_t flags=0;return guest_camera_stamp_matches(base,&m->object,0)&&
        read_ptr(m->object.pointer+0xd8)==m->entry&&read_at(m->object.pointer+0xe0,&flags,4)&&flags==m->flags;
}
static void guest_vitals_sample_actor(uintptr_t base,uintptr_t pawn,GuestVitalsValues *q) {
    GameArray modules={0},again={0};uintptr_t entries[128]={0},damage=0;
    memset(q,0,sizeof *q);
    if(!guest_camera_actor_live(base,pawn)||!is_kind_of(base,pawn,"SFXPawn_PlayerParty")||
       !scene_readiness_array(pawn+0x70,&modules,128)||
       (modules.count&&!read_at(modules.data,entries,(SIZE_T)modules.count*8)))return;
    for(int i=0;i<modules.count;++i)if(entries[i]&&is_live(base,entries[i])&&is_kind_of(base,entries[i],"SFXModule_DamageNativeBase")) {
        if(damage||read_ptr(entries[i]+0x60)!=pawn)return;damage=entries[i];
    }
    GuestCameraStamp stamp;float fraction=0;
    /* The scaled-float cached maximum is the same +0x90 source used by the
     * retained damage sampler; Pawn.Health is deliberately not sampled. */
    if(damage&&guest_camera_stamp_read(base,damage,&stamp,0)&&
       runtime_power_property(base,damage,"SFXModule_DamageNativeBase","CurrentHealth","FloatProperty",0x98,4)&&
       runtime_power_property(base,damage,"SFXModule_DamageNativeBase","MaxHealth","StructProperty",0x70,40)&&
       read_at(damage+0x98,&q->hp,4)&&read_at(damage+0x90,&q->hp_max,4)&&
       guest_vitals_fraction(q->hp,q->hp_max,&fraction)&&guest_camera_stamp_matches(base,&stamp,0)&&
       read_ptr(damage+0x60)==pawn)q->hp_valid=1;
    if(!read_at(pawn+0x70,&again,sizeof again)||memcmp(&modules,&again,sizeof again)){q->hp_valid=0;return;}
    for(int i=0;i<modules.count;++i)if(read_ptr(modules.data+(SIZE_T)i*8)!=entries[i]){q->hp_valid=0;return;}
    uintptr_t manager=read_ptr(pawn+0x414),items[128]={0},links[128]={0},shield=0;int count=0;
    if(!narrative_probe_object(base,manager,"SFXInventoryManager"))return;
    uintptr_t head=read_ptr(manager+0x280),item=head;
    while(item&&count<128){
        for(int i=0;i<count;++i)if(items[i]==item)return;
        if(!guest_camera_actor_live(base,item)||!is_kind_of(base,item,"Inventory"))return;
        items[count]=item;links[count++]=read_ptr(item+0x2a8);
        if(is_kind_of(base,item,"SFXShield_Base")){
            if(shield||read_ptr(item+0x180)!=pawn||read_ptr(item+0x190)!=pawn||read_ptr(item+0x2b0)!=manager)return;
            shield=item;
        }
        item=links[count-1];
    }
    if(item||!shield||!guest_camera_stamp_read(base,shield,&stamp,0)||
       !runtime_power_property(base,shield,"SFXShield_Base","CurrentShields","FloatProperty",0x3c4,4)||
       !runtime_power_property(base,shield,"SFXShield_Base","MaxShields","StructProperty",0x2e4,40)||
       !read_at(shield+0x3c4,&q->shield,4)||!read_at(shield+0x304,&q->shield_max,4)||
       !guest_vitals_fraction(q->shield,q->shield_max,&fraction)||!guest_camera_stamp_matches(base,&stamp,0)||
       read_ptr(pawn+0x414)!=manager||read_ptr(manager+0x280)!=head||
       read_ptr(shield+0x180)!=pawn||read_ptr(shield+0x190)!=pawn||read_ptr(shield+0x2b0)!=manager)return;
    for(int i=0;i<count;++i)if(!is_live(base,items[i])||read_ptr(items[i]+0x2a8)!=links[i])return;
    q->shield_valid=1;
}
static unsigned long guest_vitals_scope_token(void) {
    return g_runtime_config.role==ME3_ROLE_HOST?session_epoch:guest_player_context_registration_token();
}
static void guest_vitals_sample(uintptr_t base,SessionContext c,DWORD now) {
    GuestVitalsHud *q=&guest_vitals;GuestVitalsValues values={0};
    q->hp_valid=q->shield_valid=0;q->sampled=now;
    if(!guest_vitals_hud_owned(base,c)||!guest_camera_stamp_read(base,c.pc,&q->sample_pc,0)||
       !guest_camera_stamp_read(base,c.pawn,&q->sample_pawn,0)||!guest_camera_stamp_read(base,c.world,&q->sample_world,0))return;
    q->sample_token=guest_vitals_scope_token();
    if(guest_vitals_enabled()&&guest_vitals_owned(base,c))guest_vitals_sample_actor(base,c.pawn,&values);
    q->hp_valid=values.hp_valid;q->shield_valid=values.shield_valid;
    q->hp=values.hp;q->hp_max=values.hp_max;q->shield=values.shield;q->shield_max=values.shield_max;
    if(guest_vitals_extra_sample)guest_vitals_extra_sample(base,c,now);
}
static int guest_vitals_sample_matches(uintptr_t base,SessionContext c) {
    GuestVitalsHud *q=&guest_vitals;
    return c.pc==q->sample_pc.pointer&&c.pawn==q->sample_pawn.pointer&&c.world==q->sample_world.pointer&&
        q->sample_token==guest_vitals_scope_token()&&
        guest_camera_stamp_matches(base,&q->sample_pc,0)&&guest_camera_stamp_matches(base,&q->sample_pawn,0)&&
        guest_camera_stamp_matches(base,&q->sample_world,0);
}
typedef struct GuestVitalsLine {float x1,y1,x2,y2;unsigned char color[4];} GuestVitalsLine;
typedef struct GuestVitalsText {GameArray text;float sx,sy;} GuestVitalsText;
_Static_assert(sizeof(GuestVitalsLine)==20,"Canvas.Draw2DLine ABI");
_Static_assert(sizeof(GuestVitalsText)==24,"Canvas.DrawTextScaled ABI");
static void guest_vitals_call(uintptr_t canvas,const GuestVitalsMethod *m,void *p) {
    ((ProcessEvent)(guest_vitals.base+0xf4f80))((void*)canvas,(void*)m->object.pointer,p,NULL);
}
static void guest_vitals_bar(uintptr_t canvas,float x,float y,float width,float fraction,int valid,int blue,float scale) {
    int height=(int)fmaxf(3,5*scale);
    for(int i=0;i<height;++i){GuestVitalsLine p={x,y+i,x+width,y+i,{35,35,35,210}};
        guest_vitals_call(canvas,&guest_vitals.line,&p);
        if(valid&&fraction>0){p.x2=x+width*fraction;p.color[0]=blue?245:55;
            p.color[1]=blue?175:65;p.color[2]=blue?65:235;p.color[3]=240;
            guest_vitals_call(canvas,&guest_vitals.line,&p);}}
}
static void guest_vitals_draw(uintptr_t hud) {
    GuestVitalsHud *q=&guest_vitals;uintptr_t base=q->base;SessionContext c=session_context(session_engine);
    if(!guest_vitals_hud_owned(base,c)||hud!=q->hud.pointer||c.pc!=q->pc.pointer||
       c.pawn!=q->pawn.pointer||c.world!=q->world.pointer||GetTickCount()-q->sampled>1000||!guest_vitals_sample_matches(base,c)||
       !guest_camera_stamp_matches(base,&q->hud,0)||!guest_camera_stamp_matches(base,&q->pc,0)||
       !guest_camera_stamp_matches(base,&q->pawn,0)||!guest_camera_stamp_matches(base,&q->world,0)||
       read_ptr(c.pc+0x6b4)!=hud||read_ptr(hud+0x530)!=c.pc||read_ptr(hud+0x198)!=c.world)return;
    uintptr_t canvas=read_ptr(hud+0x550);GuestCameraStamp stamp;int32_t size[2]={0};uint32_t flags=0;
    if(!read_at(hud+0x5a4,&flags,4)||!(flags&2)||!narrative_probe_object(base,canvas,"Canvas")||
       read_ptr(read_ptr(canvas)+0x230)!=base+0xf4f80||!read_ptr(canvas+0x70)||
       !guest_camera_stamp_read(base,canvas,&stamp,0)||!read_at(canvas+0xb0,size,8)||
       size[0]<320||size[0]>16384||size[1]<200||size[1]>16384||
       !guest_vitals_method_live(base,&q->line)||!guest_vitals_method_live(base,&q->text)||
       !guest_vitals_method_live(base,&q->pos)||!guest_vitals_method_live(base,&q->color))return;
    float scale=fminf(2,fmaxf(0.7f,size[1]/720.0f)),width=fminf(size[0]*0.32f,300*scale);
    float x=(size[0]-width)*0.5f,y=size[1]-52*scale,h=0,s=0;
    int hv=q->hp_valid&&guest_vitals_fraction(q->hp,q->hp_max,&h),sv=q->shield_valid&&guest_vitals_fraction(q->shield,q->shield_max,&s);
    unsigned char saved[16];if(!read_at(canvas+0xa0,saved,sizeof saved)||!narrative_probe_object(base,read_ptr(canvas+0x80),"Font"))return;
    if(guest_vitals_enabled()&&guest_vitals_owned(base,c)&&runtime_native_hud_health_allowed(base,c)&&!runtime_native_hud_showing(base,c)) {
    guest_vitals_bar(canvas,x,y,width,h,hv,0,scale);guest_vitals_bar(canvas,x,y-9*scale,width,s,sv,1,scale);
    wchar_t title[160],hp[40],shield[40];
    if(hv)_snwprintf(hp,40,L"%.0f/%.0f",q->hp,q->hp_max);else wcscpy(hp,L"--");
    if(sv)_snwprintf(shield,40,L"%.0f/%.0f",q->shield,q->shield_max);else wcscpy(shield,L"--");
    int length=_snwprintf(title,160,L"ASHLEY   HP %ls   SHIELD %ls",hp,shield);
    float position[2]={x,y-29*scale};unsigned char white[4]={235,240,255,255};
    GuestVitalsText text={{(uintptr_t)title,length+1,length+1},0.7f*scale,0.7f*scale};
    if(length>0&&length<159){guest_vitals_call(canvas,&q->pos,position);guest_vitals_call(canvas,&q->color,white);guest_vitals_call(canvas,&q->text,&text);}
    }
    if(guest_vitals_extra_enabled&&guest_vitals_extra_enabled()&&guest_vitals_extra_draw)
        guest_vitals_extra_draw(canvas,size,scale);
    if(guest_camera_stamp_matches(base,&stamp,0)&&read_ptr(hud+0x550)==canvas){SIZE_T written;
        WriteProcessMemory(GetCurrentProcess(),(void*)(canvas+0xa0),saved,sizeof saved,&written);}
    if(!q->draw_reported){q->draw_reported=1;
        line("{\"guest_vitals_hud\":\"health-shield-draw-dispatched\",\"health_written\":false,\"shield_written\":false}\r\n");}
}
static void guest_vitals_process_event(void *self,void *fn,void *params,void *result) {
    GuestVitalsHud *q=&guest_vitals;DWORD error=GetLastError();
    int draw=q->installed&&!q->busy&&GetCurrentThreadId()==session_game_thread&&
        (uintptr_t)self==q->hud.pointer&&(uintptr_t)fn==q->post.object.pointer;
    SetLastError(error);q->original(self,fn,params,result);error=GetLastError();
    if(draw){q->busy=1;guest_vitals_draw((uintptr_t)self);q->busy=0;}SetLastError(error);
}
static void runtime_guest_vitals_tick(uintptr_t base,uintptr_t engine,DWORD now) {
    GuestVitalsHud *q=&guest_vitals;
    if((!guest_vitals_enabled()&&!(guest_vitals_extra_enabled&&guest_vitals_extra_enabled()))||GetCurrentThreadId()!=session_game_thread)return;
    SessionContext c=session_context(engine);
    if(!guest_vitals_hud_owned(base,c)){q->hp_valid=q->shield_valid=0;return;}
    if(!q->sampled||now-q->sampled>=200||!guest_vitals_sample_matches(base,c))guest_vitals_sample(base,c,now);
    if(q->prepared&&now-q->prepared<500)return;q->prepared=now;
    uintptr_t hud=read_ptr(c.pc+0x6b4);
    if(!guest_camera_actor_live(base,hud)||!is_kind_of(base,hud,"BioHUD")||read_ptr(hud+0x530)!=c.pc||read_ptr(hud+0x198)!=c.world)return;
    GuestVitalsHud next=*q;
    if(c.pc!=q->pc.pointer||c.pawn!=q->pawn.pointer||c.world!=q->world.pointer)next.draw_reported=0;
    static const GuestVitalsField line_fields[]={{"X1","FloatProperty",0,4,0x80},{"Y1","FloatProperty",4,4,0x80},
        {"X2","FloatProperty",8,4,0x80},{"Y2","FloatProperty",12,4,0x80},{"LineColor","StructProperty",16,4,0x80}};
    static const GuestVitalsField text[]={{"Text","StrProperty",0,16,0x400880},{"fXScale","FloatProperty",16,4,0x80},{"fYScale","FloatProperty",20,4,0x80}};
    static const GuestVitalsField pos[]={{"PosX","FloatProperty",0,4,0x80},{"PosY","FloatProperty",4,4,0x80}};
    static const GuestVitalsField color[]={{"R","ByteProperty",0,1,0x80},{"G","ByteProperty",1,1,0x80},
        {"B","ByteProperty",2,1,0x80},{"A","ByteProperty",3,1,0x90}};
    if(!guest_vitals_method(base,"BioHUD","PostRender",0x20002,0,NULL,0,&next.post)||
       !guest_vitals_method(base,"Canvas","Draw2DLine",0x20401,20,line_fields,5,&next.line)||
       !guest_vitals_method(base,"Canvas","DrawTextScaled",0x20401,24,text,3,&next.text)||
       !guest_vitals_method(base,"Canvas","SetPos",0x20401,8,pos,2,&next.pos)||
       !guest_vitals_method(base,"Canvas","SetDrawColor",0x24401,4,color,4,&next.color)||
       !guest_camera_stamp_read(base,hud,&next.hud,0)||!guest_camera_stamp_read(base,c.pc,&next.pc,0)||
       !guest_camera_stamp_read(base,c.pawn,&next.pawn,0)||!guest_camera_stamp_read(base,c.world,&next.world,0))return;
    uintptr_t slot=read_ptr(hud)+0x230,original=read_ptr(slot);
    if(q->installed){if(q->base!=base||q->slot!=slot||original!=(uintptr_t)guest_vitals_process_event)return;}
    else {if(original!=base+0x40d450&&original!=base+0xf4f80)return;
        DWORD old=0;if(!VirtualProtect((void*)slot,8,PAGE_READWRITE,&old))return;
        next.base=base;next.slot=slot;next.original=(ProcessEvent)original;*q=next;
        PVOID previous=InterlockedCompareExchangePointer((PVOID volatile*)slot,(PVOID)guest_vitals_process_event,(PVOID)original);
        DWORD ignored;VirtualProtect((void*)slot,8,old,&ignored);if((uintptr_t)previous!=original)return;
        next.installed=1;line("{\"guest_vitals_hud\":\"canvas-health-shield-installed\",\"gameplay_verified\":false}\r\n");}
    *q=next;
}
#endif
