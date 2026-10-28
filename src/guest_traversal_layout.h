#ifndef ME3_GUEST_TRAVERSAL_LAYOUT_H
#define ME3_GUEST_TRAVERSAL_LAYOUT_H
static int guest_traversal_row(int index) {
    static const int indices[]={31,33,36,37,38,39,46,47};
    for(unsigned i=0;i<8;++i)if(indices[i]==index)return (int)i;return -1;
}
static const char *guest_traversal_player_class(int index) {
    static const char *names[]={"SFXGameContent.SFXCustomAction_PlayerMantleOverCover","SFXGameContent.SFXCustomAction_PlayerMantleUp",
        "SFXGameContent.SFXCustomAction_PlayerCoverSlipLeft","SFXGameContent.SFXCustomAction_PlayerCoverSlipLeftStanding",
        "SFXGameContent.SFXCustomAction_PlayerCoverSlipRight","SFXGameContent.SFXCustomAction_PlayerCoverSlipRightStanding",
        "SFXGameContent.SFXCustomAction_PlayerLadderClimbUp","SFXGameContent.SFXCustomAction_PlayerLadderClimbDown"};
    int row=guest_traversal_row(index);return row>=0?names[row]:NULL;
}
static int guest_traversal_camera_name(const char *name) {
    return !strcmp(name,"SFXGame.SFXCameraMode_LadderUp")||!strcmp(name,"SFXGame.SFXCameraMode_LadderDown");
}
#endif
