#ifndef ALLEGRO_SYSTEM_NEW_H
#define ALLEGRO_SYSTEM_NEW_H

#ifdef __cplusplus
   extern "C" {
#endif

typedef struct ALLEGRO_SYSTEM ALLEGRO_SYSTEM;

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
   AL_LAST_PATH // must be last
};

AL_FUNC(AL_CONST char *, al_get_path, (uint32_t id, char *path, size_t size));
AL_FUNC(bool, al_get_screensaver_active, (void));
AL_FUNC(void, al_set_screensaver_active, (bool active));

#ifdef __cplusplus
   }
#endif

#endif

/* vim: set sts=3 sw=3 et: */
