#ifndef ME3_GUEST_REACTION_LAYOUT_H
#define ME3_GUEST_REACTION_LAYOUT_H
enum {GUEST_REACTION_COUNT=7,GUEST_REACTION_STANDARD_COUNT=6};
static const unsigned guest_reaction_indices[GUEST_REACTION_COUNT]={81,82,83,84,85,86,100};
static int guest_reaction_row(int index) {
    for(unsigned i=0;i<GUEST_REACTION_COUNT;++i)if(guest_reaction_indices[i]==(unsigned)index)return (int)i;return -1;
}
static const char *guest_reaction_player_class(int index) {
    static const char *names[]={"SFXGameContent.SFXCustomAction_PlayerStandardImpact",NULL,
        "SFXGameContent.SFXCustomAction_PlayerStandardImpactForward","SFXGameContent.SFXCustomAction_PlayerStandardImpactLeft",
        "SFXGameContent.SFXCustomAction_PlayerStandardImpactRight",NULL,"SFXGame.SFXCustomAction_PlayerShieldStandardImpact"};
    int row=guest_reaction_row(index);return row>=0?names[row]:NULL;
}
static int guest_reaction_action(int index) {
    return guest_reaction_player_class(index)!=NULL;
}
#endif
