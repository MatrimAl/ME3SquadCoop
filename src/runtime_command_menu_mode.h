#ifndef ME3_RUNTIME_COMMAND_MENU_MODE_H
#define ME3_RUNTIME_COMMAND_MENU_MODE_H
/* Exact supported native menu IDs. Shift enters Command (5), while the
 * separate PowerWheel uses 3. Never retain scene/GUI modes by proximity. */
static const char *runtime_command_menu_class(int mode) {
    return mode==3?"SFXGameModePowerWheel":mode==5?"SFXGameModeCommand":NULL;
}
static int runtime_command_menu_mode_selftest(void) {
    for(int mode=-1;mode<=255;++mode) {
        const char *klass=runtime_command_menu_class(mode);
        if(mode==3) {if(!klass||strcmp(klass,"SFXGameModePowerWheel"))return 0;}
        else if(mode==5) {if(!klass||strcmp(klass,"SFXGameModeCommand"))return 0;}
        else if(klass)return 0;
    }
    return runtime_command_menu_class(INT_MAX)==NULL&&runtime_command_menu_class(INT_MIN)==NULL;
}
#endif
