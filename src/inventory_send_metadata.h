#ifndef ME3_INVENTORY_SEND_METADATA_H
#define ME3_INVENTORY_SEND_METADATA_H
/* Metadata only: no writer hook, payload read, native call or data mutation. */
static unsigned inventory_send_metadata_rows;
static int inventory_send_metadata_capture(uintptr_t property,uintptr_t archive,uintptr_t map,
        uintptr_t data,uintptr_t caller) {
    DWORD saved=GetLastError();uintptr_t base=initial_actor_trace_base;int captured=0;
    if(!inventory_decode_installed||inventory_send_metadata_rows>=32||!progress_enabled||
       test_action!=6||strcmp(test_role,"host")||!has_marker(root)||
       GetCurrentThreadId()!=initial_actor_trace_thread||caller!=base+0x48d433||
       !inventory_decode_property(property)||data<0x414)goto done;
    uintptr_t pawn=data-0x414,world=read_ptr(base+0x18b6638),level=0,info=0;GameArray actors={0};
    if(!is_live(base,pawn)||!is_kind_of(base,pawn,"SFXPawn_Ashley")||
       !is_kind_of(base,pawn,"Pawn")||!is_live(base,world)||
       !(level=read_ptr(world+0x88))||!is_live(base,level)||
       !read_at(level+0x60,&actors,sizeof actors)||actors.count<1||actors.count>100000||
       actors.max<actors.count||actors.max>1000000||
       !(info=read_ptr(actors.data))||!is_live(base,info)||read_ptr(pawn+0x198)!=info)goto done;
    uintptr_t value=0,klass=0,table=read_ptr(archive),mapvt=read_ptr(map),connection=0;
    uint32_t loading=0,error=0;int loading_valid=read_at(archive+0x14,&loading,4);
    int error_valid=read_at(archive+0x38,&error,4);
    if(!read_at(data,&value,8))goto done;
    if(value){if(!is_live(base,value))goto done;klass=read_ptr(value+0x50);}
    if(mapvt!=base+0x11a0d20||!read_at(map+0x108,&connection,8)||!connection)goto done;
    /* Read bounded table metadata only. Report ambiguity instead of selecting first match. */
    unsigned matches=0,index=0;uintptr_t channel=0;uint32_t type=0,flags=0;int scan_valid=1;
    for(unsigned i=1;i<0x3ff;++i){uintptr_t c=0,actor=0;
        if(!read_at(connection+0xf84+(uintptr_t)i*8,&c,8)){scan_valid=0;break;}
        if(!c)continue;
        uint32_t channel_type=0;
        if(!read_at(c+0x78,&channel_type,4)){scan_valid=0;break;}
        if(channel_type!=2)continue;
        if(!read_at(c+0xa0,&actor,8)){scan_valid=0;break;}
        if(actor==value&&value){++matches;index=i;channel=c;}
    }
    int channel_valid=matches==1&&scan_valid&&read_at(channel+0x78,&type,4)&&read_at(channel+0x68,&flags,4);
    ++inventory_send_metadata_rows;char row[1400];snprintf(row,sizeof row,
      "{\"inventory_send_metadata\":\"field-entry\",\"pawn\":\"0x%llx\",\"value\":\"0x%llx\","
      "\"value_class\":\"0x%llx\",\"archive_vtable_rva\":\"0x%llx\","
      "\"archive_bits_rva\":\"0x%llx\",\"archive_index_rva\":\"0x%llx\","
      "\"map_vtable_rva\":\"0x%llx\",\"connection\":\"0x%llx\","
      "\"loading_valid\":%s,\"loading\":%u,\"error_valid\":%s,\"archive_error\":%u,"
      "\"channel_scan_valid\":%s,\"channel_matches\":%u,\"channel_index\":%u,"
      "\"channel\":\"0x%llx\",\"channel_details_valid\":%s,\"channel_type\":%u,\"channel_flags\":%u,"
      "\"writer_hook_installed\":false,\"payload_modified\":false,\"monotonic_ms\":%lu}\r\n",
      (unsigned long long)pawn,(unsigned long long)value,(unsigned long long)klass,
      (unsigned long long)(table>=base?table-base:0),
      (unsigned long long)(table&&read_ptr(table+0x10)>=base?read_ptr(table+0x10)-base:0),
      (unsigned long long)(table&&read_ptr(table+0x18)>=base?read_ptr(table+0x18)-base:0),
      (unsigned long long)(mapvt-base),(unsigned long long)connection,
      loading_valid?"true":"false",loading,error_valid?"true":"false",error,
      scan_valid?"true":"false",matches,index,(unsigned long long)channel,
      channel_valid?"true":"false",type,flags,GetTickCount());line(row);captured=1;
 done:SetLastError(saved);return captured;
}
#endif
