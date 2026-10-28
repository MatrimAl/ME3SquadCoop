#ifndef ME3_RUNTIME_GUEST_TRAVERSAL_HANDOFF_PINS_H
#define ME3_RUNTIME_GUEST_TRAVERSAL_HANDOFF_PINS_H
#define RUNTIME_TRAVERSAL_UNPOSSESSED_FLAGS 0x20002
#define RUNTIME_TRAVERSAL_UNPOSSESSED_SIZE 25
static int runtime_traversal_handoff_bodies(uintptr_t base,uintptr_t unpossessed,uintptr_t end) {
    static const unsigned char unpossessed_code[25]={0x1c,0,0,0,0,0,0,0,0,0x16,0x14,0x2d,0x01,0,0,0,0,0,0,0,0,0x28,0x04,0x0b,0x53};
    static const GuestEquipmentOperand unpossessed_ops[]={
        {1,"Function","Engine.Pawn.UnPossessed",-1,0},
        {13,"BoolProperty","SFXGame.BioPawn.bIsAPlayer",0xe20,UINT64_C(0x2020)},{0,NULL,NULL,0,0}};
    static const unsigned char end_code[16]={0x1c,0,0,0,0,0,0,0,0,0x25,0x4a,0x4a,0x16,0x04,0x0b,0x53};
    static const GuestEquipmentOperand end_ops[]={
        {1,"Function","SFXGame.BioPawn.DoCustomAction",-1,0},{0,NULL,NULL,0,0}};
    static const GuestEquipmentName names[]={{0,NULL}};
    return guest_equipment_body(base,unpossessed,25,unpossessed_code,sizeof unpossessed_code,unpossessed_ops,names)&&
        guest_equipment_body(base,end,16,end_code,sizeof end_code,end_ops,names);
}
#endif
