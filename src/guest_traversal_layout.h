#ifndef ME3_GUEST_TRAVERSAL_LAYOUT_H
#define ME3_GUEST_TRAVERSAL_LAYOUT_H
enum { GUEST_TRAVERSAL_ROW_COUNT=14 };
static const unsigned guest_traversal_indices[GUEST_TRAVERSAL_ROW_COUNT]={31,33,36,37,38,39,46,47,28,29,40,41,42,43};
static int guest_traversal_row(int index) {
    for(unsigned i=0;i<GUEST_TRAVERSAL_ROW_COUNT;++i)if(guest_traversal_indices[i]==(unsigned)index)return (int)i;return -1;
}
static const char *guest_traversal_player_class(int index) {
    static const char *names[]={"SFXGameContent.SFXCustomAction_PlayerMantleOverCover","SFXGameContent.SFXCustomAction_PlayerMantleUp",
        "SFXGameContent.SFXCustomAction_PlayerCoverSlipLeft","SFXGameContent.SFXCustomAction_PlayerCoverSlipLeftStanding",
        "SFXGameContent.SFXCustomAction_PlayerCoverSlipRight","SFXGameContent.SFXCustomAction_PlayerCoverSlipRightStanding",
        "SFXGameContent.SFXCustomAction_PlayerLadderClimbUp","SFXGameContent.SFXCustomAction_PlayerLadderClimbDown",
        "SFXGameContent.SFXCustomAction_GapJump","SFXGameContent.SFXCustomAction_StandingGapJump",
        "SFXGame.SFXCustomAction_Cover90TurnRight","SFXGame.SFXCustomAction_Cover90TurnRightStanding",
        "SFXGame.SFXCustomAction_Cover90TurnLeft","SFXGame.SFXCustomAction_Cover90TurnLeftStanding"};
    int row=guest_traversal_row(index);return row>=0?names[row]:NULL;
}
/* These slots are absent in the retained authored Ashley table. Do not replace
 * a class or cached instance installed by a different subsystem. */
static int guest_traversal_empty_authored(int index) {
    return index==29||(index>=40&&index<=43);
}
static int guest_traversal_camera_name(const char *name) {
    return !strcmp(name,"SFXGame.SFXCameraMode_LadderUp")||!strcmp(name,"SFXGame.SFXCameraMode_LadderDown")||
        !strcmp(name,"SFXGame.SFXCameraMode_JumpStart");
}
#endif
