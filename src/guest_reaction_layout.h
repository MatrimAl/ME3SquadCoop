#ifndef ME3_GUEST_REACTION_LAYOUT_H
#define ME3_GUEST_REACTION_LAYOUT_H
enum {GUEST_REACTION_COUNT=13,GUEST_REACTION_STANDARD_COUNT=6,GUEST_REACTION_STAGGER_FIRST=7,GUEST_REACTION_STAGGER_COUNT=5};
static const unsigned guest_reaction_indices[GUEST_REACTION_COUNT]={81,82,83,84,85,86,100,87,88,89,90,91,96};
static int guest_reaction_row(int index) {
    for(unsigned i=0;i<GUEST_REACTION_COUNT;++i)if(guest_reaction_indices[i]==(unsigned)index)return (int)i;return -1;
}
static const char *guest_reaction_player_class(int index) {
    static const char *names[]={"SFXGameContent.SFXCustomAction_PlayerStandardImpact",NULL,
        "SFXGameContent.SFXCustomAction_PlayerStandardImpactForward","SFXGameContent.SFXCustomAction_PlayerStandardImpactLeft",
        "SFXGameContent.SFXCustomAction_PlayerStandardImpactRight",NULL,"SFXGame.SFXCustomAction_PlayerShieldStandardImpact",
        "SFXGameContent.SFXCustomAction_PlayerMeleed",NULL,"SFXGameContent.SFXCustomAction_KnockbackImpactForward",
        "SFXGameContent.SFXCustomAction_MeleedLeft","SFXGameContent.SFXCustomAction_MeleedRight","SFXGameContent.SFXCustomAction_PlayerMeleed"};
    int row=guest_reaction_row(index);return row>=0?names[row]:NULL;
}
static int guest_reaction_action(int index) {
    return guest_reaction_player_class(index)!=NULL;
}
static int guest_reaction_hit_camera(int index) {
    return index==87||index==89||index==90||index==91||index==96;
}
#endif
