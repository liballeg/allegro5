#ifndef CONFIG_H
#define CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ALLEGRO_CONFIG_ENTRY ALLEGRO_CONFIG_ENTRY;
typedef struct ALLEGRO_CONFIG_SECTION ALLEGRO_CONFIG_SECTION;
typedef struct ALLEGRO_CONFIG ALLEGRO_CONFIG;

AL_FUNC(void, al_config_add_section, (ALLEGRO_CONFIG *config, AL_CONST char *name));
AL_FUNC(void, al_config_set_global, (ALLEGRO_CONFIG *config, AL_CONST char *key, AL_CONST char *value));
AL_FUNC(void, al_config_set_value, (ALLEGRO_CONFIG *config, AL_CONST char *section, AL_CONST char *key, AL_CONST char *value));
AL_FUNC(AL_CONST char*, al_config_get_value, (ALLEGRO_CONFIG *config, AL_CONST char *section, AL_CONST char *key));
AL_FUNC(ALLEGRO_CONFIG*, al_config_read, (AL_CONST char *filename));
AL_FUNC(int, al_config_write, (ALLEGRO_CONFIG *config, AL_CONST char *filename));
AL_FUNC(ALLEGRO_CONFIG *, al_config_merge, (ALLEGRO_CONFIG *cfg1, ALLEGRO_CONFIG *cfg2));
AL_FUNC(void, al_config_destroy, (ALLEGRO_CONFIG *config));

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */
