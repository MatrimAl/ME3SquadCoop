#ifndef ME3_GUEST_POWER_ORDER_CONTRACT_H
#define ME3_GUEST_POWER_ORDER_CONTRACT_H
/* Supported LE3 ABI for the existing own-pawn tactical power order.
 * Owned package function14688 corroborates the nine field names/types/order,
 * flags and native index. Retained SDK corroborates the four-byte packing and
 * offsets; native registration identifies exec RVA b64a20. No call is made
 * here: this contract alone proves neither a cast nor a target effect. */
#pragma pack(push,4)
typedef struct GuestPowerOrderParms {
    uint32_t power_name[2];
    uintptr_t pawn;
    int32_t queue;
    uint32_t show_indicator;
    uintptr_t selection_target;
    unsigned char input_method;
    unsigned char padding[3];
    uintptr_t desired_target;
    float desired_location[3];
    uint32_t result;
} GuestPowerOrderParms;
#pragma pack(pop)
_Static_assert(sizeof(GuestPowerOrderParms)==0x3c,"power order parameter storage");
_Static_assert(offsetof(GuestPowerOrderParms,pawn)==8,"power order pawn offset");
_Static_assert(offsetof(GuestPowerOrderParms,desired_target)==0x24,"power order unaligned target");
_Static_assert(offsetof(GuestPowerOrderParms,result)==0x38,"power order return offset");

typedef struct GuestPowerOrderField {
    const char *name,*kind;
    int offset,size;
    uint64_t flags;
} GuestPowerOrderField;
static int guest_power_order_contract(uintptr_t base,uintptr_t function) {
    static const GuestPowerOrderField wanted[]={
        {"nmPower","NameProperty",0,8,0x80},
        {"pPawn","ObjectProperty",8,8,0x80},
        {"nQueue","IntProperty",0x10,4,0x90},
        {"bShowIndicator","BoolProperty",0x14,4,0x90},
        {"pDesiredSelectionTarget","ObjectProperty",0x18,8,0x90},
        {"eInputMethod","ByteProperty",0x20,1,0x90},
        {"pDesiredTarget","ObjectProperty",0x24,8,0x90},
        {"vDesiredTargetLocation","StructProperty",0x2c,12,0x90},
        {"ReturnValue","BoolProperty",0x38,4,0x580}};
    if(!guest_player_context_function_abi(base,function,"BioPlayerController","SquadOrderUsePower",
        0x24401,sizeof(GuestPowerOrderParms),0x38,0,0xb64a20,9))return 0;
    GuestCameraStamp method={0};
    if(!guest_camera_stamp_read(base,function,&method,1)||
       strcmp(method.full_name,"SFXGame.BioPlayerController.SquadOrderUsePower"))return 0;
    uintptr_t field=read_ptr(function+0x70),seen[9]={0};
    for(unsigned i=0;i<9;++i) {
        const GuestPowerOrderField *want=&wanted[i];
        int32_t offset=-1,size=0,dim=0;uint64_t flags=0;char name[96];
        if(!narrative_probe_object(base,field,want->kind)||read_ptr(field+0x40)!=function)return 0;
        for(unsigned j=0;j<i;++j)if(seen[j]==field)return 0;
        seen[i]=field;object_name(base,field,name,sizeof name);
        if(strcmp(name,want->name)||!read_at(field+0x70,&dim,4)||dim!=1||
           !read_at(field+0x74,&size,4)||size!=want->size||
           !read_at(field+0x94,&offset,4)||offset!=want->offset||
           !read_at(field+0x78,&flags,8)||flags!=want->flags)return 0;
        if(!strcmp(want->kind,"BoolProperty")) {
            uint64_t mask=0;if(!read_at(field+0xd0,&mask,8)||mask!=1)return 0;
        }
        if(!read_at(field+0x68,&field,8))return 0;
    }
    return !field&&guest_camera_stamp_matches(base,&method,1)&&
        guest_player_context_function_abi(base,function,"BioPlayerController","SquadOrderUsePower",
            0x24401,sizeof(GuestPowerOrderParms),0x38,0,0xb64a20,9);
}
#endif
