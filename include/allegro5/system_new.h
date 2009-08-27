#ifndef ALLEGRO_SYSTEM_NEW_H
#define ALLEGRO_SYSTEM_NEW_H

#include "allegro5/config.h"
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
AL_FUNC(ALLEGRO_CONFIG *, al_get_system_config, (void));

enum {
   ALLEGRO_PROGRAM_PATH = 0,
   ALLEGRO_TEMP_PATH,
   ALLEGRO_SYSTEM_DATA_PATH,
   ALLEGRO_USER_DATA_PATH,
   ALLEGRO_USER_HOME_PATH,
   ALLEGRO_USER_SETTINGS_PATH,
   ALLEGRO_SYSTEM_SETTINGS_PATH,
   ALLEGRO_EXENAME_PATH,
   ALLEGRO_LAST_PATH // must be last
};

AL_FUNC(ALLEGRO_PATH *, al_get_standard_path, (int id));

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
