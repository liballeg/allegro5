#ifndef ALLEGRO_SYSTEM_NEW_H
#define ALLEGRO_SYSTEM_NEW_H

#include "allegro5/path.h"

#ifdef __cplusplus
   extern "C" {
#endif

typedef struct ALLEGRO_SYSTEM ALLEGRO_SYSTEM;

/* Function: al_init
 */
#define al_init()    (al_install_system(atexit))

AL_FUNC(bool, al_install_system, (int (*atexit_ptr)(void (*)(void))));
AL_FUNC(void, al_uninstall_system, (void));
AL_FUNC(ALLEGRO_SYSTEM *, al_system_driver, (void));

enum {
   AL_PROGRAM_PATH = 0,
   AL_TEMP_PATH,
   AL_SYSTEM_DATA_PATH,
   AL_USER_DATA_PATH,
   AL_USER_HOME_PATH,
   AL_USER_SETTINGS_PATH,
   AL_SYSTEM_SETTINGS_PATH,
   AL_EXENAME_PATH,
   AL_LAST_PATH // must be last
};

AL_FUNC(ALLEGRO_PATH *, al_get_path, (int id));

AL_FUNC(void, al_set_orgname, (AL_CONST char *orgname));
AL_FUNC(void, al_set_appname, (AL_CONST char *appname));
AL_FUNC(AL_CONST char *, al_get_orgname, (void));
AL_FUNC(AL_CONST char *, al_get_appname, (void));

AL_FUNC(bool, al_inhibit_screensaver, (bool inhibit));

#ifdef __cplusplus
   }
#endif

#endif

/* vim: set sts=3 sw=3 et: */
