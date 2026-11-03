#ifndef ME3_GUEST_COVER_MELEE_LAYOUT_H
#define ME3_GUEST_COVER_MELEE_LAYOUT_H
/* Supported retail slots observed on the actual loaded Soldier player.
 * Names marked Hvy still derive PlayerMeleeBase, not HeavyMeleeBase. */
static int guest_cover_melee_index(int index) {return index>=62&&index<=72;}
static int guest_cover_melee_synced(int index) {return index==62||index==72;}
static const char *guest_cover_melee_class(int index) {
    static const char *paths[]={"SFXGameContent.SFXCustomAction_SyncCoverGrab",
        "SFXGameContent.SFXCustomAction_SyncCoverMelee","SFXGameContent.SFXCustomAction_StdCoverMeleeLeft",
        "SFXGameContent.SFXCustomAction_StdCoverMeleeRight","SFXGameContent.SFXCustomAction_CoverMeleeRight",
        "SFXGameContent.SFXCustomAction_CoverMeleeLeft","SFXGameContent.SFXCustomAction_HvyStdCoverMeleeLeft",
        "SFXGameContent.SFXCustomAction_HvyStdCoverMeleeRight","SFXGameContent.SFXCustomAction_HvyCoverMeleeRight",
        "SFXGameContent.SFXCustomAction_HvyCoverMeleeLeft","SFXGame.SFXCustomAction_HeavyStdCoverMeleeRight"};
    return guest_cover_melee_index(index)?paths[index-62]:NULL;
}
#endif
