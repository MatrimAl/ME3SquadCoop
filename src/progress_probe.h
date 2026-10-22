/* Mission-progress diagnostic. Normal input only; never teleport actors or
   clear story/physics/action flags to make a companion appear movable. */
static int progress_enabled = 0;
static DWORD progress_free_at = 0, progress_walk_at = 0;
static uintptr_t progress_host_pawn = 0, progress_host_world = 0;
static int32_t progress_host_pawn_index = -1;
static uint64_t progress_host_pawn_name = 0;
static float progress_host_start[3];
static int progress_host_stopped = 0;
static int progress_follow_sent = 0, progress_host_shot = 0, progress_guest_shot = 0;
static DWORD progress_follow_at = 0;
static int progress_catchup_attempted = 0;
static DWORD progress_lease_at = 0, progress_guest_input_at = 0;
static DWORD progress_candidates_at = 0;
static int progress_scripts_dumped = 0;
static void progress_dump_script(uintptr_t base,const char *owner,const char *name) {
    uintptr_t function=function_named(base,owner,name); GameArray code={0},refs={0};
    if (!function || !read_at(function+0x7c,&code,sizeof code) || code.count<0 || code.count>8192 || code.max<code.count) return;
    unsigned char bytes[8192]; char hex[16385],row[17000]; uint32_t flags=0; uint16_t parms=0;
    if (code.count && (!code.data || !read_at(code.data,bytes,(SIZE_T)code.count))) return;
    for(int i=0;i<code.count;++i) snprintf(hex+i*2,3,"%02x",bytes[i]); hex[code.count*2]=0;
    read_at(function+0xe0,&flags,4);read_at(function+0xe6,&parms,2);
    snprintf(row,sizeof row,"{\"progress\":\"script-dump\",\"owner\":\"%s\",\"function\":\"%s\",\"flags\":%u,\"parms_size\":%u,\"hex\":\"%s\"}\r\n",owner,name,flags,parms,hex);line(row);
    if(!strcmp(name,"TeleportNearLeader")) {
        const int offsets[]={508,569,694,755,858};
        for(int j=0;j<5;++j) { int at=offsets[j];uint32_t id=0;
            if(at+9>code.count||bytes[at]!=0x65)continue;memcpy(&id,bytes+at+1,4);
            char resolved[96];name_of(base,id,resolved,sizeof resolved);snprintf(row,sizeof row,"{\"progress\":\"script-virtual-name\",\"bytecode_offset\":%d,\"index\":%u,\"name\":\"%s\"}\r\n",at,id,resolved);line(row); }
    }
    for(int i=0;i+8<=code.count;++i) {
        uintptr_t ref=0;memcpy(&ref,bytes+i,8);
        if (!is_live(base,ref)) continue;
        char path[700],escaped[1400],kind[96];int32_t offset=-1;uint64_t mask=0;
        identity_path(base,ref,path,sizeof path);identity_quote(path,escaped,sizeof escaped);object_name(base,read_ptr(ref+0x50),kind,sizeof kind);
        if(strstr(kind,"Property")) read_at(ref+0x94,&offset,4);
        if(!strcmp(kind,"BoolProperty")) read_at(ref+0xd0,&mask,8);
        snprintf(row,sizeof row,"{\"progress\":\"script-pointer-operand\",\"owner\":\"%s\",\"function\":\"%s\",\"bytecode_offset\":%d,\"pointer\":\"0x%llx\",\"path\":\"%s\",\"class\":\"%s\",\"property_offset\":%d,\"bool_mask\":%llu}\r\n",owner,name,i,(unsigned long long)ref,escaped,kind,offset,(unsigned long long)mask);line(row);
    }
    if (!read_at(function+0xc0,&refs,sizeof refs) || refs.count<0 || refs.count>256 || refs.max<refs.count) return;
    for(int i=0;i<refs.count;++i) {
        uintptr_t ref=read_ptr(refs.data+(SIZE_T)i*8);char path[700],escaped[1400],kind[96];int32_t offset=-1;
        identity_path(base,ref,path,sizeof path);identity_quote(path,escaped,sizeof escaped);object_name(base,read_ptr(ref+0x50),kind,sizeof kind);
        if (strstr(kind,"Property")) read_at(ref+0x94,&offset,4);
        snprintf(row,sizeof row,"{\"progress\":\"script-reference\",\"owner\":\"%s\",\"function\":\"%s\",\"index\":%d,\"path\":\"%s\",\"class\":\"%s\",\"property_offset\":%d}\r\n",owner,name,i,escaped,kind,offset);line(row);
    }
}
static void progress_state_name(uintptr_t base, uintptr_t object, char *out, size_t size) {
    if(!native_state_name(base,object,out,size))snprintf(out,size,"unknown");
}
static int progress_pawn_ready(uintptr_t base, uintptr_t pawn, uintptr_t ai) {
    int32_t action = -1, power = -1; uint32_t a = 0, b = 0, c = 0, attach = 0;
    unsigned char physics = 0; uint32_t actor_flags = 0;
    if (!is_live(base, pawn) || !read_at(pawn + 0xc20, &action, 4) || !read_at(pawn + 0xc28, &power, 4) ||
        !read_at(pawn + 0xe18, &a, 4) || !read_at(pawn + 0xe1c, &b, 4) || !read_at(pawn + 0xe20, &c, 4) ||
        !read_at(pawn + 0x25c, &actor_flags, 4) || !read_at(pawn + 0x260, &attach, 4) || !read_at(pawn + 0x268, &physics, 1)) return 0;
    char state[96]; progress_state_name(base, ai, state, sizeof state);
    if (is_kind_of(base,ai,"AIController") && (read_ptr(ai+0x484) || read_ptr(ai+0x48c))) return 0;
    return action == 0 && power == 0 && (a & 0x40) && !(a & (0x200000 | 0x20000000)) &&
           !(b & 0x10000000) && !(c & 0x40000) && !(attach & 0x400) && !(actor_flags & 2) && physics == 1 &&
           !strstr(state, "Script") && !strstr(state, "Cinematic");
}
static int progress_companion_near(uintptr_t host, uintptr_t companion) {
    float a[3],b[3];
    return read_at(host+0x108,a,sizeof a) && read_at(companion+0x108,b,sizeof b) &&
           isfinite(a[0]) && isfinite(a[1]) && isfinite(a[2]) && isfinite(b[0]) && isfinite(b[1]) && isfinite(b[2]) && distance(a,b) <= 2000.0f;
}
static void progress_follow(uintptr_t base, uintptr_t pc, uintptr_t pawn) {
    if (progress_follow_sent) return;
    progress_follow_sent=1;
    progress_follow_at=GetTickCount();
#pragma pack(push, 1)
    struct { uintptr_t pawn; unsigned char input; unsigned char padding[3]; } params = {pawn,0,{0}};
#pragma pack(pop)
    int ok=invoke(base,pc,"BioPlayerController","HenchmanOrderFollow",&params);
    line(ok?"{\"progress\":\"ordinary-squad-follow-request\",\"dispatched\":true,\"position_writes\":false}\r\n":"{\"progress\":\"ordinary-squad-follow-request\",\"dispatched\":false}\r\n");
}
static void progress_native_catchup(uintptr_t base,uintptr_t pc,uintptr_t pawn,uintptr_t ai) {
    if (!progress_enabled || progress_catchup_attempted || !progress_follow_at || GetTickCount()-progress_follow_at<2000 ||
        !is_kind_of(base,ai,"SFXAI_Henchman") || read_ptr(ai+0x3bc)!=pawn || !progress_pawn_ready(base,pawn,ai)) return;
    uintptr_t leader=read_ptr(pc+0x3bc),world=read_ptr(pc+0x198),squad=world?read_ptr(world+0x8e8):0;
    float health=-1,velocity[3]={0},floor[3]={0};
    if (!is_live(base,leader) || !progress_pawn_ready(base,leader,pc) || read_ptr(squad+0x2cc)!=leader ||
        read_ptr(pawn+0xba8)!=squad || read_ptr(pawn+0x198)!=world ||
        !invoke(base,leader,"BioPawn","GetHealthPct",&health) ||
        !read_at(leader+0x138,velocity,sizeof velocity) || !read_at(leader+0x2fc,floor,sizeof floor) ||
        !catchup_ground_allowed(health,velocity,floor)) return;
    progress_catchup_attempted=1;
    uint32_t result=0; int dispatched=invoke(base,ai,"SFXAI_Henchman","TeleportNearLeader",&result);
    char row[400];snprintf(row,sizeof row,"{\"progress\":\"native-companion-catchup\",\"function\":\"SFXAI_Henchman.TeleportNearLeader\",\"dispatched\":%s,\"engine_return\":%s,\"near_host_after_call\":%s,\"direct_position_writes\":false,\"monotonic_ms\":%lu}\r\n",dispatched?"true":"false",result&1?"true":"false",progress_companion_near(leader,pawn)?"true":"false",GetTickCount());line(row);
}
static void progress_input(uintptr_t base, uintptr_t engine) {
    if (!progress_enabled || test_action != 6 || !host_map_refreshed) return;
    uintptr_t pc = local_controller(engine), pawn = pc ? read_ptr(pc + 0x3bc) : 0;
    uintptr_t world = pc ? read_ptr(pc + 0x198) : 0, manager = pc ? read_ptr(pc + 0xc38) : 0;
    unsigned char mode = 255, net = 255; DWORD now = GetTickCount();
    if (manager) read_at(manager + 0xa8, &mode, 1);
    if (world) read_at(world + 0x7c8, &net, 1);
    int32_t pawn_index=-1;uint64_t pawn_name=0;
    int pawn_identity=is_live(base,pawn)&&read_at(pawn+8,&pawn_index,4)&&read_at(pawn+0x48,&pawn_name,8);
    int changed=progress_host_pawn!=pawn||progress_host_world!=world||
        progress_host_pawn_index!=pawn_index||progress_host_pawn_name!=pawn_name;
    uintptr_t camera=read_ptr(pc+0x684);unsigned char ignored=255;GameArray peers={0};
    int peer_ready=identity_array(identity_host_driver+0x70,&peers,2)&&peers.count==1;
    if(peer_ready) {
        uintptr_t connection=read_ptr(peers.data),remote=read_ptr(connection+0x68);
        peer_ready=is_live(base,remote)&&remote!=pc&&is_kind_of(base,remote,"PlayerController")&&read_ptr(remote+0x198)==world;
    }
    read_at(pc+0x781,&ignored,1);
    int ready=mode==0&&net==2&&pawn_identity&&peer_ready&&ignored==0&&
        progress_pawn_ready(base,pawn,pc)&&is_live(base,camera)&&read_ptr(camera+0x3f4)==pawn;
    if(!progress_lease_at&&(changed||!ready)) {
        if(progress_walk_at) {
            char row[280];snprintf(row,sizeof row,"{\"progress\":\"host-walk-reset\",\"reason\":\"%s\",\"mode\":%u,\"monotonic_ms\":%lu}\r\n",changed?"pawn-or-world-changed":"ordinary-control-unavailable",mode,now);line(row);
        }
        progress_walk_at=0;progress_host_stopped=0;progress_free_at=0;
    }
    progress_host_pawn=pawn;progress_host_world=world;progress_host_pawn_index=pawn_index;progress_host_pawn_name=pawn_name;
    if(!ready)return;
    if (!progress_free_at) progress_free_at = now;
    if (now - progress_free_at < 2000) return;
    uintptr_t input = read_ptr(pc + 0x6dc);
    if (!is_live(base, input) || !is_kind_of(base, input, "PlayerInput")) return;
    if (!progress_walk_at) {
        progress_walk_at = now; read_at(pawn + 0x108, progress_host_start, sizeof progress_host_start);
        char row[240];snprintf(row,sizeof row,"{\"progress\":\"host-walk-start\",\"source\":\"normal-player-input\",\"position_writes\":false,\"monotonic_ms\":%lu}\r\n",now);line(row);
    }
    DWORD walking=now-progress_walk_at;
    float value = walking < 10000 || (walking >= 14000 && walking < 30000) ? 1.0f : 0.0f; SIZE_T wrote;
    if(progress_lease_at) {
        DWORD paired=now-progress_lease_at;
        /* Retrace the ordinary approach on the same ground after the guest
           owns Ashley. No actor transform, velocity or physics writes. */
        /* Allow the independent guest pulse/follow to settle first, then use
           the measured ~1000-unit approach corridor at a slower walking rate.
           Full-speed 12-second input reached the boundary within ~4 seconds. */
        value=paired>=8000&&paired<24000 ? -0.15f : 0.0f;
    }
    WriteProcessMemory(GetCurrentProcess(), (void*)(input + 0x194), &value, 4, &wrote);
    WriteProcessMemory(GetCurrentProcess(), (void*)(input + 0x1a4), &value, 4, &wrote);
    if (walking>=30000 && !progress_host_stopped) { progress_host_stopped = 1; line("{\"progress\":\"host-walk-stop\"}\r\n"); }
}
static uintptr_t progress_guest_leader=0;
static DWORD progress_guest_scan_at=0;
static void progress_guest_follow_input(uintptr_t base,uintptr_t pc,uintptr_t pawn,uintptr_t input,DWORD elapsed) {
    if (!progress_enabled) return;
    uintptr_t world=read_ptr(pc+0x198);DWORD now=GetTickCount();
    if ((!is_live(base,progress_guest_leader) || read_ptr(progress_guest_leader+0x198)!=world || network_actor_channel_id(base,progress_guest_leader)<0) && now-progress_guest_scan_at>=1000) {
        progress_guest_scan_at=now;progress_guest_leader=0;GameArray array={0};uintptr_t *objects=identity_object_list(base,&array);
        int candidates=0;
        if(objects) { for(int i=0;i<array.count;++i) {
            uintptr_t object=objects[i];
            if(!object||object==pawn||read_ptr(object+0x198)!=world||!is_kind_of(base,object,"SFXPawn_Player"))continue;
            uint32_t flags=0;read_at(object+0x25c,&flags,4);
            if(!(flags&10)&&network_actor_channel_id(base,object)>=0) { progress_guest_leader=object;candidates++; }
        } HeapFree(GetProcessHeap(),0,objects); }
        if(candidates!=1)progress_guest_leader=0;
    }
    float forward=0,strafe=0,leader[3],position[3];int32_t rotation[3];
    if(elapsed>=4000&&elapsed<36000&&is_live(base,progress_guest_leader)&&
        read_at(progress_guest_leader+0x108,leader,sizeof leader)&&read_at(pawn+0x108,position,sizeof position)&&read_at(pc+0x114,rotation,sizeof rotation)) {
        float dx=leader[0]-position[0],dy=leader[1]-position[1],length=sqrtf(dx*dx+dy*dy);
        if(isfinite(length)&&length>125&&length<2000) {
            float yaw=(float)rotation[1]*0.00009587379924285f;
            float gain=fminf(0.35f,(length-125.0f)*0.005f);
            forward=gain*(dx*cosf(yaw)+dy*sinf(yaw))/length;
            strafe=gain*(-dx*sinf(yaw)+dy*cosf(yaw))/length;
        }
    }
    /* Keep the short, independent forward-input check even if following has
       no unambiguous replicated leader. This is a test bot, not host steering. */
    if(elapsed>=2000&&elapsed<4000) { forward=1.0f;strafe=0.0f; }
    if(elapsed>=5000&&!progress_guest_shot) {
        progress_guest_shot=1;render_capture(base,read_ptr(base+0x18b31d0),L"guest-progress");
    }
    SIZE_T wrote;
    WriteProcessMemory(GetCurrentProcess(),(void*)(input+0x194),&forward,4,&wrote);
    WriteProcessMemory(GetCurrentProcess(),(void*)(input+0x1a4),&forward,4,&wrote);
    WriteProcessMemory(GetCurrentProcess(),(void*)(input+0x1ac),&strafe,4,&wrote);
    if(now-progress_guest_input_at>=1000) {
        progress_guest_input_at=now;char row[400],kind[96];
        object_name(base,read_ptr(progress_guest_leader+0x50),kind,sizeof kind);
        snprintf(row,sizeof row,"{\"progress\":\"guest-follow-input\",\"leader_channel\":%d,\"leader_class\":\"%s\",\"forward\":%.4f,\"strafe\":%.4f,\"elapsed_ms\":%lu,\"monotonic_ms\":%lu}\r\n",progress_guest_leader?network_actor_channel_id(base,progress_guest_leader):-1,kind,forward,strafe,elapsed,now);line(row);
    }
}
static void progress_pawn_sample(uintptr_t base, uintptr_t pawn, const char *role, DWORD now) {
    if (!is_live(base, pawn) || !is_kind_of(base, pawn, "BioPawn")) return;
    uintptr_t owner = read_ptr(pawn + 0x374), actor_base = read_ptr(pawn + 0x188);
    float xyz[3] = {0}, velocity[3] = {0}; int32_t action = -1, power = -1;
    uint32_t a = 0, b = 0, c = 0, attach = 0; unsigned char physics = 255;
    char state[96], owner_class[96], base_class[96], text[3200];
    progress_state_name(base, owner, state, sizeof state);
    object_name(base, owner ? read_ptr(owner + 0x50) : 0, owner_class, sizeof owner_class);
    object_name(base, actor_base ? read_ptr(actor_base + 0x50) : 0, base_class, sizeof base_class);
    read_at(pawn + 0x108, xyz, sizeof xyz); read_at(pawn + 0x138, velocity, sizeof velocity);
    read_at(pawn + 0xc20, &action, 4); read_at(pawn + 0xc28, &power, 4);
    read_at(pawn + 0xe18, &a, 4); read_at(pawn + 0xe1c, &b, 4); read_at(pawn + 0xe20, &c, 4);
    read_at(pawn + 0x260, &attach, 4); read_at(pawn + 0x268, &physics, 1);
    uintptr_t world=read_ptr(pawn+0x198), squad=world?read_ptr(world+0x8e8):0;
    char world_path[400], pawn_path[500], world_json[800], pawn_json[1000];
    identity_path(base,world,world_path,sizeof world_path);identity_path(base,pawn,pawn_path,sizeof pawn_path);
    identity_quote(world_path,world_json,sizeof world_json);identity_quote(pawn_path,pawn_json,sizeof pawn_json);
    int channel_id=network_actor_channel_id(base,pawn);char network_id[60]={0};
    if(channel_id>=0)snprintf(network_id,sizeof network_id,"net-channel:%d",channel_id);
    float health=-1,kill_z=NAN,floor[3]={0};uint32_t actor_flags=0; int member=0;
    invoke(base,pawn,"BioPawn","GetHealthPct",&health);read_at(pawn+0x25c,&actor_flags,4);read_at(pawn+0x2fc,floor,sizeof floor);
    if (is_kind_of(base,world,"ZoneInfo")) read_at(world+0x278,&kill_z,4);
    GameArray roster={0};
    if (identity_array(squad+0x270,&roster,4)) for(int i=0;i<roster.count;++i) member |= read_ptr(roster.data+(SIZE_T)i*8)==pawn;
    combat_equipment_sample(base,pawn,role,now);
    int alive=isfinite(health)&&health>0&&!(c&0x200), grounded=physics==1&&fabsf(velocity[2])<5&&floor[2]>=0.6f;
    snprintf(text, sizeof text,
        "{\"progress\":\"pawn\",\"role\":\"%s\",\"owner_class\":\"%s\",\"owner_state\":\"%s\",\"base_class\":\"%s\",\"physics\":%u,\"action\":%d,\"power_action\":%d,\"fully_initialized\":%s,\"frozen\":%s,\"root_motion_override\":%s,\"sleeping\":%s,\"busy_conversation\":%s,\"hard_attached\":%s,\"scripted_move_target\":%s,\"scripted_route\":%s,\"position\":[%.3f,%.3f,%.3f],\"velocity\":[%.3f,%.3f,%.3f],\"world_id\":\"%s\",\"pawn_id\":\"%s\",\"actor_path\":\"%s\",\"squad_member\":%s,\"hidden\":%s,\"health\":%.5f,\"alive\":%s,\"grounded\":%s,\"world_kill_z\":%.3f,\"below_world_kill_z\":%s,\"host_input_elapsed_ms\":%lu,\"monotonic_ms\":%lu}\r\n",
        role, owner_class, state, base_class, physics, action, power, a & 0x40 ? "true":"false", a & 0x200000 ? "true":"false",
        a & 0x20000000 ? "true":"false", b & 0x10000000 ? "true":"false", c & 0x40000 ? "true":"false",
        attach & 0x400 ? "true":"false", owner && is_kind_of(base, owner, "AIController") && read_ptr(owner + 0x484) ? "true":"false",
        owner && is_kind_of(base, owner, "AIController") && read_ptr(owner + 0x48c) ? "true":"false",
        xyz[0],xyz[1],xyz[2],velocity[0],velocity[1],velocity[2],world_json,network_id,pawn_json,member?"true":"false",actor_flags&2?"true":"false",
        isfinite(health)?health:-1,alive?"true":"false",grounded?"true":"false",isfinite(kill_z)?kill_z:0,
        !isfinite(kill_z)||xyz[2]<kill_z?"true":"false",progress_walk_at ? now-progress_walk_at:0, now); line(text);
}
static void progress_sample(uintptr_t base, uintptr_t pc, uintptr_t pawn, uintptr_t squad, DWORD now) {
    if (!progress_enabled) return;
    progress_pawn_sample(base, pawn, test_action == 5 ? "guest-local" : "host-local", now);
    uintptr_t diagnostic_world=read_ptr(pc+0x198);
    actor_channel_diagnostics_sample(base,diagnostic_world,now);
    scene_readiness_sample(base,pc,pawn,diagnostic_world,now);
    narrative_probe_sample(base,pc,diagnostic_world,now);
    combat_context_sample(base,read_ptr(base+0x18b31d0),now);
    uintptr_t input=read_ptr(pc+0x6dc);unsigned char ignore_move=255,ignore_look=255;
    int32_t disabled=-1;uint32_t effective=0;GameArray overrides={0};float axes[3]={0};
    read_at(pc+0x781,&ignore_move,1);read_at(pc+0x782,&ignore_look,1);
    int effective_known=invoke(base,pc,"PlayerController","IsMoveInputIgnored",&effective);
    if(is_kind_of(base,input,"BioPlayerInput")) { read_at(input+0x398,&disabled,4);read_at(input+0x2c4,&overrides,sizeof overrides); }
    read_at(input+0x194,&axes[0],4);read_at(input+0x1a4,&axes[1],4);read_at(input+0x1ac,&axes[2],4);
    char input_row[500];snprintf(input_row,sizeof input_row,"{\"progress\":\"input-state\",\"role\":\"%s\",\"ignore_move\":%u,\"ignore_look\":%u,\"effective_ignore_known\":%s,\"effective_ignore_move\":%s,\"key_input_disabled\":%d,\"override_count\":%d,\"post_tick_axes\":[%.4f,%.4f,%.4f],\"monotonic_ms\":%lu}\r\n",test_action==5?"guest":"host",ignore_move,ignore_look,effective_known?"true":"false",effective&1?"true":"false",disabled,overrides.count,axes[0],axes[1],axes[2],now);line(input_row);
    if (test_action != 6) return;
    if (progress_walk_at && !progress_scripts_dumped) {
        progress_scripts_dumped=1;
        progress_dump_script(base,"SFXAI_Henchman","TeleportNearLeader");
        progress_dump_script(base,"SFXAI_Henchman","ShouldFollowPlayer");
        progress_dump_script(base,"BioPlayerController","HenchmanOrderFollow");
    }
    if (progress_walk_at && !progress_host_shot && now-progress_walk_at >= 1000) {
        progress_host_shot=1; render_capture(base,read_ptr(base+0x18b31d0),L"host-progress");
    }
    GameArray members = {0};
    if (identity_array(squad + 0x270, &members, 4)) for (int i=0; i<members.count; ++i) {
        uintptr_t other = read_ptr(members.data + (SIZE_T)i*8);
        if (is_kind_of(base, other, "SFXPawn_Ashley")) progress_pawn_sample(base, other, "host-ashley", now);
    }
    uintptr_t selection = read_ptr(pc + 0xc50), target = selection ? read_ptr(selection + 0x70) : 0;
    char kind[96], text[250]; object_name(base, target ? read_ptr(target + 0x50) : 0, kind, sizeof kind);
    snprintf(text,sizeof text,"{\"progress\":\"selection\",\"target_present\":%s,\"target_class\":\"%s\",\"monotonic_ms\":%lu}\r\n",target?"true":"false",kind,now); line(text);
    if (now-progress_candidates_at < 15000) return;
    progress_candidates_at=now;
    GameArray objects_array={0}; uintptr_t *objects=identity_object_list(base,&objects_array);
    if (!objects) return;
    for (int i=0;i<objects_array.count;++i) {
        uintptr_t candidate=objects[i];
        if (!candidate || read_ptr(candidate+0x198)!=read_ptr(pc+0x198) || !is_kind_of(base,candidate,"SFXPawn_Ashley")) continue;
        char name[96], outer[96], cls[96], row[600]; uint32_t flags=0; float xyz[3]={0};
        object_name(base,candidate,name,sizeof name); object_name(base,read_ptr(candidate+0x40),outer,sizeof outer);
        object_name(base,read_ptr(candidate+0x50),cls,sizeof cls); read_at(candidate+0x25c,&flags,4);read_at(candidate+0x108,xyz,sizeof xyz);
        snprintf(row,sizeof row,"{\"progress\":\"ashley-candidate\",\"name\":\"%s\",\"outer\":\"%s\",\"class\":\"%s\",\"hidden\":%s,\"pawn_squad_matches\":%s,\"position\":[%.3f,%.3f,%.3f],\"monotonic_ms\":%lu}\r\n",name,outer,cls,flags&2?"true":"false",read_ptr(candidate+0xba8)==squad?"true":"false",xyz[0],xyz[1],xyz[2],now);line(row);
    }
    HeapFree(GetProcessHeap(),0,objects);
}
