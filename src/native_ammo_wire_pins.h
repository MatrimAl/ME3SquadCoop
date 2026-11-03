#ifndef ME3_NATIVE_AMMO_WIRE_PINS_H
#define ME3_NATIVE_AMMO_WIRE_PINS_H
/* Shared read-only identities for the exact LE3 integer ammo fields and
 * serializer/call sites. Requires the existing memory, object and stamp
 * helpers; contains no session, observer, logging or opt-in policy. */
static int native_ammo_wire_property(uintptr_t base,uintptr_t prop,int field,GuestCameraStamp *stamp) {
    int32_t offset=-1,dim=0,size=0;uint64_t flags=0;uint16_t rep[2]={0};
    const char *path=field?"SFXGame.SFXWeapon_NativeBase.CurrentSpareAmmo":"SFXGame.SFXWeapon_NativeBase.AmmoUsedCount";
    return guest_camera_stamp_read(base,prop,stamp,1)&&is_kind_of(base,prop,"IntProperty")&&
        !strcmp(stamp->full_name,path)&&read_ptr(prop)==base+0x108dfb0&&
        read_at(prop+0x70,&dim,4)&&dim==1&&read_at(prop+0x74,&size,4)&&size==4&&
        read_at(prop+0x78,&flags,8)&&flags==0x20&&read_at(prop+0x80,rep,4)&&rep[0]==66&&rep[1]==(field?22:23)&&
        read_at(prop+0x94,&offset,4)&&offset==(field?0x3b4:0x3b8);
}
static int native_ammo_wire_properties(uintptr_t base,uintptr_t weapon,GuestCameraStamp out[2]) {
    uintptr_t klass=read_ptr(weapon+0x50);int found=0;
    for(int depth=0;klass&&depth<32;++depth){
        if(!is_live(base,klass))return 0;
        char name[96];object_name(base,klass,name,sizeof name);
        if(!strcmp(name,"SFXWeapon_NativeBase")){
            uintptr_t child=read_ptr(klass+0x70),seen[512];int n=0;
            while(child&&n<512){
                if(!is_live(base,child))return 0;
                for(int i=0;i<n;++i)if(seen[i]==child)return 0;
                seen[n++]=child;object_name(base,child,name,sizeof name);
                int field=!strcmp(name,"AmmoUsedCount")?0:!strcmp(name,"CurrentSpareAmmo")?1:-1;
                if(field>=0){if((found&(1<<field))||read_ptr(child+0x40)!=klass||!native_ammo_wire_property(base,child,field,&out[field]))return 0;found|=1<<field;}
                child=read_ptr(child+0x68);
            }
            return !child&&found==3;
        }
        klass=read_ptr(klass+0x60);
    }
    return 0;
}
static int native_ammo_wire_pins(uintptr_t base) {
    static const unsigned char fn[]={0x48,0x83,0xec,0x28,0x48,0x8b,0xca,0x41,0xb8,0x04,0x00,0x00,0x00,0x49,0x8b,0xd1,0xe8,0xdb,0xd9,0xf3,0xff,0xb8,0x01,0x00,0x00,0x00,0x48,0x83,0xc4,0x28,0xc3};
    static const unsigned char send[]={0x48,0x8b,0x03,0x4c,0x8b,0x90,0xf8,0x02,0x00,0x00,0x48,0x63,0xf6,0x4d,0x8b,0x8f,0xa0,0x00,0x00,0x00,0x4c,0x03,0xce,0x4d,0x8b,0x47,0x60,0x4d,0x8b,0x80,0xf0,0x00,0x00,0x00,0x48,0x8d,0x55,0x10,0x48,0x8b,0xcb,0x41,0xff,0xd2,0x83,0x7d,0x48,0x00};
    static const unsigned char receive[]={0x48,0x8b,0x07,0x4c,0x8b,0xad,0xf0,0x00,0x00,0x00,0x4d,0x8b,0x45,0x60,0x4c,0x8b,0xcb,0x4d,0x8b,0x80,0xf0,0x00,0x00,0x00,0x48,0x8b,0x95,0xf8,0x00,0x00,0x00,0x48,0x8b,0xcf,0xff,0x90,0xf8,0x02,0x00,0x00};
    unsigned char a[sizeof fn],b[sizeof receive],c[sizeof send];
    return read_at(base+0x146f40,a,sizeof a)&&!memcmp(a,fn,sizeof a)&&
        read_at(base+0x488eac,b,sizeof b)&&!memcmp(b,receive,sizeof b)&&
        read_at(base+0x48d407,c,sizeof c)&&!memcmp(c,send,sizeof c);
}
#endif
