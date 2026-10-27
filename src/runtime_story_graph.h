#ifndef ME3_RUNTIME_STORY_GRAPH_H
#define ME3_RUNTIME_STORY_GRAPH_H
/* Read-only, bounded classification of the current level's authored graph.
 * Include after runtime_native_hud.h and runtime_story_gate_policy.h. Reflected
 * Array.Inner/ScriptStruct layouts must prove every derived native stride.
 * No GObjects search, graph mutation, event activation or native calls. */
static uintptr_t runtime_story_graph_class(uintptr_t base,uintptr_t object,const char *name) {
    uintptr_t klass=read_ptr(object+0x50),seen[48]={0};char text[96];
    for(unsigned n=0;klass&&n<48;++n) {
        if(!narrative_probe_object(base,klass,"Class"))return 0;
        for(unsigned j=0;j<n;++j)if(seen[j]==klass)return 0;
        seen[n]=klass;object_name(base,klass,text,sizeof text);
        if(!strcmp(text,name))return klass;
        if(!read_at(klass+0x60,&klass,8))return 0;
    }
    return 0;
}
static int runtime_story_graph_level_member(uintptr_t base,uintptr_t world,uintptr_t info,uintptr_t level) {
    if(read_ptr(world+0x88)==level)return 1;
    GameArray levels={0},streaming={0},after={0};
    if(!scene_readiness_array(world+0x78,&levels,4096))return 0;
    for(int i=0;i<levels.count;++i)if(read_ptr(levels.data+(SIZE_T)i*8)==level)
        return read_at(world+0x78,&after,sizeof after)&&!memcmp(&levels,&after,sizeof levels);
    if(!scene_readiness_array(info+0x3e4,&streaming,4096))return 0;
    for(int i=0;i<streaming.count;++i) {
        uintptr_t entry=read_ptr(streaming.data+(SIZE_T)i*8);
        if(narrative_probe_object(base,entry,"LevelStreaming")&&read_ptr(entry+0xa0)==level)
            return read_at(info+0x3e4,&after,sizeof after)&&!memcmp(&streaming,&after,sizeof streaming)&&
                read_ptr(streaming.data+(SIZE_T)i*8)==entry&&read_ptr(entry+0xa0)==level;
    }
    return 0;
}
static uintptr_t runtime_story_graph_level(uintptr_t base,uintptr_t object,uintptr_t *world) {
    uintptr_t seen[32]={0},cursor=object;
    for(unsigned n=0;cursor&&n<32;++n) {
        if(!is_live(base,cursor))return 0;
        for(unsigned j=0;j<n;++j)if(seen[j]==cursor)return 0;
        seen[n]=cursor;
        if(is_kind_of(base,cursor,"Level")) {
            /* A streamed level retains its package's own TheWorld as Outer.
             * Its membership in the current play world is the authority. */
            uintptr_t current=read_ptr(base+0x18b6638),persistent=read_ptr(current+0x88),info=0;
            GameArray actors={0};
            if(!narrative_probe_object(base,current,"World")||!narrative_probe_object(base,persistent,"Level")||
               !scene_readiness_array(persistent+0x60,&actors,100000)||!actors.count||
               !read_at(actors.data,&info,8)||!narrative_probe_object(base,info,"WorldInfo")||
               read_ptr(info+0x40)!=persistent||read_ptr(info+0x198)!=info||
               !runtime_story_graph_level_member(base,current,info,cursor)||read_ptr(base+0x18b6638)!=current)return 0;
            *world=current;return cursor;
        }
        if(!read_at(cursor+0x40,&cursor,8))return 0;
    }
    return 0;
}
static int runtime_story_graph_same_level(uintptr_t base,uintptr_t object,uintptr_t level,uintptr_t world) {
    uintptr_t actual=0;
    return runtime_story_graph_level(base,object,&actual)==level&&actual==world;
}
static int runtime_story_graph_struct(uintptr_t base,uintptr_t array,const char *name,int stride,uintptr_t *type) {
    uintptr_t inner=read_ptr(array+0xd0),structure=read_ptr(inner+0xd0);int size=0,element=0,offset=-1,dim=0;char text[96];
    if(!narrative_probe_object(base,array,"ArrayProperty")||!narrative_probe_object(base,inner,"StructProperty")||
       !narrative_probe_object(base,structure,"ScriptStruct")||
       !read_at(inner+0x74,&element,4)||element!=stride||!read_at(inner+0x94,&offset,4)||offset!=0||
       !read_at(inner+0x70,&dim,4)||dim!=1||!read_at(structure+0x78,&size,4)||size!=stride)return 0;
    object_name(base,structure,text,sizeof text);
    if(strcmp(text,name))return 0;
    *type=structure;return 1;
}
static int runtime_story_graph_object_field(uintptr_t base,uintptr_t owner,const char *name,int offset,const char *target) {
    uintptr_t field=runtime_native_hud_field(base,owner,name,"ObjectProperty",offset,8),klass=read_ptr(field+0xd0);char text[96];
    if(!field||!narrative_probe_object(base,klass,"Class"))return 0;
    object_name(base,klass,text,sizeof text);return !strcmp(text,target);
}
static int runtime_story_graph_output_layout(uintptr_t base,uintptr_t object) {
    uintptr_t owner=runtime_story_graph_class(base,object,"SequenceOp"),output=0,edge=0;
    uintptr_t array=runtime_native_hud_field(base,owner,"OutputLinks","ArrayProperty",0x98,16);
    if(!owner||!array||!runtime_story_graph_struct(base,array,"SeqOpOutputLink",0x34,&output)||
       read_ptr(output+0x40)!=owner)return 0;
    uintptr_t links=runtime_native_hud_field(base,output,"Links","ArrayProperty",0,16);
    uint64_t mask=0;uintptr_t disabled=runtime_native_hud_field(base,output,"bDisabled","BoolProperty",0x30,4);
    return links&&runtime_story_graph_struct(base,links,"SeqOpOutputInputLink",0xc,&edge)&&read_ptr(edge+0x40)==owner&&
        runtime_story_graph_object_field(base,edge,"LinkedOp",0,"SequenceOp")&&
        runtime_native_hud_field(base,edge,"InputLinkIdx","IntProperty",8,4)&&
        runtime_native_hud_field(base,output,"LinkDesc","StrProperty",0x10,16)&&
        runtime_story_graph_object_field(base,output,"LinkedOp",0x28,"SequenceOp")&&
        disabled&&read_at(disabled+0xd0,&mask,8)&&mask==2;
}
static int runtime_story_graph_input_member(uintptr_t base,uintptr_t object,int index) {
    uintptr_t owner=runtime_story_graph_class(base,object,"SequenceOp"),type=0;GameArray inputs={0};
    uintptr_t array=runtime_native_hud_field(base,owner,"InputLinks","ArrayProperty",0x88,16);
    uint64_t mask=0;uint32_t flags=0;
    if(!owner||!array||!runtime_story_graph_struct(base,array,"SeqOpInputLink",0x28,&type)||
       read_ptr(type+0x40)!=owner||!runtime_native_hud_field(base,type,"LinkDesc","StrProperty",0,16)||
       !runtime_story_graph_object_field(base,type,"LinkedOp",0x18,"SequenceOp")||
       !runtime_native_hud_field(base,type,"QueuedActivations","IntProperty",0x20,4)||
       !scene_readiness_array(object+0x88,&inputs,128)||index<0||index>=inputs.count)return 0;
    uintptr_t disabled=runtime_native_hud_field(base,type,"bDisabled","BoolProperty",0x24,4);
    if(!disabled||!read_at(disabled+0xd0,&mask,8)||mask!=2||
       !read_at(inputs.data+(SIZE_T)index*0x28+0x24,&flags,4))return 0;
    if(flags&2)return 2; /* Valid disabled inputs cannot reach a story action. */
    int flow=is_kind_of(base,object,"SeqAct_Gate")?RUNTIME_STORY_FLOW_GATE:
        is_kind_of(base,object,"SeqAct_Interp")?RUNTIME_STORY_FLOW_INTERP:0;
    if(flow) {
        /* Opening/closing a Gate does not emit its Out. Likewise Stop/Pause
         * must not turn restoration of a Matinee into a new story entry.
         * Inspect the actual reflected label rather than assuming indices. */
        uintptr_t at=inputs.data+(SIZE_T)index*0x28;GameString text={0},again={0};GameArray same={0};
        wchar_t label[32]={0};
        if(!read_at(at,&text,sizeof text)||!text.data||text.count<2||text.count>32||
           text.max<text.count||text.max>128||!read_at((uintptr_t)text.data,label,(SIZE_T)text.count*2)||
           label[text.count-1]||wcslen(label)!=(size_t)text.count-1||
           !read_at(at,&again,sizeof again)||memcmp(&text,&again,sizeof text)||
           !read_at(object+0x88,&same,sizeof same)||memcmp(&inputs,&same,sizeof inputs))return 0;
        if(!runtime_story_flow_input(flow,label))return 2;
    }
    return 1;
}
static int runtime_story_graph_variable_layout(uintptr_t base,uintptr_t object) {
    uintptr_t owner=runtime_story_graph_class(base,object,"SequenceOp"),type=0;
    uintptr_t array=runtime_native_hud_field(base,owner,"VariableLinks","ArrayProperty",0xa8,16);
    if(!owner||!array||!runtime_story_graph_struct(base,array,"SeqVarLink",0x4c,&type)||read_ptr(type+0x40)!=owner)return 0;
    uintptr_t linked=runtime_native_hud_field(base,type,"LinkedVariables","ArrayProperty",0,16),inner=read_ptr(linked+0xd0);
    int bytes=0,offset=-1,dim=0;uintptr_t klass=read_ptr(inner+0xd0);char name[96];
    if(!linked||!narrative_probe_object(base,inner,"ObjectProperty")||!narrative_probe_object(base,klass,"Class")||
       !read_at(inner+0x74,&bytes,4)||bytes!=8||!read_at(inner+0x94,&offset,4)||offset||
       !read_at(inner+0x70,&dim,4)||dim!=1||
       !runtime_native_hud_field(base,type,"LinkDesc","StrProperty",0x10,16)||
       !runtime_native_hud_field(base,type,"ExpectedType","ClassProperty",0x20,8))return 0;
    object_name(base,klass,name,sizeof name);return !strcmp(name,"SequenceVariable");
}
static int runtime_story_graph_data_has_director(uintptr_t base,uintptr_t data,uintptr_t level,uintptr_t world) {
    GameArray groups={0},again={0};uintptr_t values[64]={0};int found=0;
    if(!narrative_probe_object(base,data,"InterpData")||!runtime_story_graph_same_level(base,data,level,world)||
       !runtime_power_property(base,data,"InterpData","InterpGroups","ArrayProperty",0x90,16)||
       !scene_readiness_array(data+0x90,&groups,64)||
       (groups.count&&!read_at(groups.data,values,(SIZE_T)groups.count*8)))return -1;
    for(int i=0;i<groups.count;++i) {
        if(!narrative_probe_object(base,values[i],"InterpGroup")||read_ptr(values[i]+0x40)!=data)return -1;
        if(is_kind_of(base,values[i],"InterpGroupDirector"))found=1;
    }
    if(!read_at(data+0x90,&again,sizeof again)||memcmp(&groups,&again,sizeof groups))return -1;
    for(int i=0;i<groups.count;++i)if(read_ptr(groups.data+(SIZE_T)i*8)!=values[i])return -1;
    return found;
}
static int runtime_story_graph_data_link(uintptr_t base,uintptr_t link) {
    uintptr_t expected=read_ptr(link+0x20);char name[96];
    if(!narrative_probe_object(base,expected,"Class"))return 0;
    object_name(base,expected,name,sizeof name);return !strcmp(name,"InterpData");
}
static int runtime_story_graph_interp(uintptr_t base,uintptr_t object,uintptr_t level,uintptr_t world) {
    uintptr_t cached=0,data=0,values[32]={0};GameArray links={0},again={0};unsigned variables=0;
    if(!runtime_power_property(base,object,"SeqAct_Interp","InterpData","ObjectProperty",0x154,8)||
       !read_at(object+0x154,&cached,8))return -1;
    if(cached) {
        int result=runtime_story_graph_data_has_director(base,cached,level,world);
        if(result<0)return -1;
        data=cached;
    }
    /* InterpData is often not cached until activation. Its native Data link
     * directly contains the InterpData variable; do not invoke population. */
    if(!runtime_story_graph_variable_layout(base,object)||!scene_readiness_array(object+0xa8,&links,32))return -1;
    for(int i=0;i<links.count;++i) {
        uintptr_t link=links.data+(SIZE_T)i*0x4c;
        if(!runtime_story_graph_data_link(base,link))continue;
        GameArray refs={0},same={0};
        if(!scene_readiness_array(link,&refs,32)||(variables+=(unsigned)refs.count)>64||
           (refs.count&&!read_at(refs.data,values,(SIZE_T)refs.count*8)))return -1;
        for(int j=0;j<refs.count;++j) {
            if(!narrative_probe_object(base,values[j],"InterpData")||
               !runtime_story_graph_same_level(base,values[j],level,world)||(data&&data!=values[j]))return -1;
            data=values[j];
        }
        if(!read_at(link,&same,sizeof same)||memcmp(&same,&refs,sizeof same))return -1;
        for(int j=0;j<refs.count;++j)if(read_ptr(refs.data+(SIZE_T)j*8)!=values[j])return -1;
    }
    if(!read_at(object+0xa8,&again,sizeof again)||memcmp(&links,&again,sizeof again)||read_ptr(object+0x154)!=cached)return -1;
    return data?runtime_story_graph_data_has_director(base,data,level,world):0;
}
static int runtime_story_graph_shuttle_exit(uintptr_t base,uintptr_t event,uintptr_t originator) {
    static const char *event_path="BioD_ProMar_110Shuttle.TheWorld.PersistentLevel.Main_Sequence.Initialize_And_Enter_ProMar.SFXSeqEvt_Touch_0";
    static const char *volume_path="BioD_ProMar_110Shuttle.TheWorld.PersistentLevel.BioTriggerVolume_3";
    GuestCameraStamp event_stamp,volume_stamp;
    return narrative_probe_object(base,event,"SFXSeqEvt_Touch")&&narrative_probe_object(base,originator,"BioTriggerVolume")&&
        guest_camera_stamp_read(base,event,&event_stamp,1)&&guest_camera_stamp_read(base,originator,&volume_stamp,1)&&
        !strcmp(event_stamp.full_name,event_path)&&!strcmp(volume_stamp.full_name,volume_path);
}
static int runtime_story_graph_classify(uintptr_t base,uintptr_t event) {
    uintptr_t world=0,level=0,originator=read_ptr(event+0xfc);
    if(!narrative_probe_object(base,event,"SequenceEvent")||
       !runtime_power_property(base,event,"SequenceEvent","Originator","ObjectProperty",0xfc,8)||
       !(level=runtime_story_graph_level(base,event,&world))||!guest_camera_actor_live(base,originator)||
       !runtime_story_graph_same_level(base,originator,level,world))return RUNTIME_STORY_GATE_NONE;
    if(runtime_story_graph_shuttle_exit(base,event,originator))return RUNTIME_STORY_GATE_GROUP_EXIT;
    uintptr_t nodes[64]={event};unsigned char depths[64]={0};unsigned count=1,links_seen=0;
    for(unsigned cursor=0;cursor<count;++cursor) {
        uintptr_t object=nodes[cursor];
        if(!narrative_probe_object(base,object,"SequenceOp")||!runtime_story_graph_same_level(base,object,level,world))return RUNTIME_STORY_GATE_NONE;
        /* A cinematic-mode toggle can merely restore input/HUD. Its incoming
         * input semantics are not established here, so it is not a terminal.
         * A proven story endpoint ends this search: a large unrelated graph
         * beyond that endpoint must not invalidate the path already found. */
        if(is_kind_of(base,object,"SFXSeqAct_StartConversation"))return RUNTIME_STORY_GATE_SHEPARD;
        if(is_kind_of(base,object,"SeqAct_Interp")) {
            int director=runtime_story_graph_interp(base,object,level,world);
            if(director<0)return RUNTIME_STORY_GATE_NONE;
            if(director)return RUNTIME_STORY_GATE_SHEPARD;
        }
        if(!runtime_story_graph_output_layout(base,object))return RUNTIME_STORY_GATE_NONE;
        GameArray outputs={0},after={0};
        if(!scene_readiness_array(object+0x98,&outputs,32))return RUNTIME_STORY_GATE_NONE;
        for(int i=0;i<outputs.count;++i) {
            uintptr_t output=outputs.data+(SIZE_T)i*0x34;GameArray edges={0},same={0};
            uint32_t flags=0;
            if(!read_at(output+0x30,&flags,4))return RUNTIME_STORY_GATE_NONE;
            if(flags&2)continue; /* The reflected native bDisabled bit. */
            if(!scene_readiness_array(output,&edges,128)||
               (links_seen+=(unsigned)edges.count)>128)return RUNTIME_STORY_GATE_NONE;
            for(int j=0;j<edges.count;++j) {
                uintptr_t edge=edges.data+(SIZE_T)j*0xc,next=0;int input=-1;
                if(!read_at(edge,&next,8)||!read_at(edge+8,&input,4)||!next)return RUNTIME_STORY_GATE_NONE;
                int member=runtime_story_graph_input_member(base,next,input);
                if(!member)return RUNTIME_STORY_GATE_NONE;
                if(member==2)continue;
                unsigned known=0;for(unsigned k=0;k<count;++k)if(nodes[k]==next){known=1;break;}
                if(known)continue;
                if(count==64||depths[cursor]>=12)return RUNTIME_STORY_GATE_NONE;
                nodes[count]=next;depths[count]=(unsigned char)(depths[cursor]+1);++count;
            }
            if(!read_at(output,&same,sizeof same)||memcmp(&same,&edges,sizeof same))return RUNTIME_STORY_GATE_NONE;
        }
        if(!read_at(object+0x98,&after,sizeof after)||memcmp(&outputs,&after,sizeof after))return RUNTIME_STORY_GATE_NONE;
    }
    return RUNTIME_STORY_GATE_NONE;
}
#endif
