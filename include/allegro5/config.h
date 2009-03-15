#ifndef CONFIG_H
#define CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Type: ALLEGRO_CONFIG_ENTRY
 */
typedef struct ALLEGRO_CONFIG_ENTRY ALLEGRO_CONFIG_ENTRY;

/* Type: ALLEGRO_CONFIG_SECTION
 */
typedef struct ALLEGRO_CONFIG_SECTION ALLEGRO_CONFIG_SECTION;

/* Type: ALLEGRO_CONFIG
 */
typedef struct ALLEGRO_CONFIG ALLEGRO_CONFIG;

AL_FUNC(ALLEGRO_CONFIG *, al_create_config, (void));
AL_FUNC(void, al_add_config_section, (ALLEGRO_CONFIG *config, const char *name));
AL_FUNC(void, al_set_config_value, (ALLEGRO_CONFIG *config, const char *section, const char *key, const char *value));
AL_FUNC(const char*, al_get_config_value, (const ALLEGRO_CONFIG *config, const char *section, const char *key));
AL_FUNC(ALLEGRO_CONFIG*, al_load_config_file, (const char *filename));
AL_FUNC(int, al_save_config_file, (const ALLEGRO_CONFIG *config, const char *filename));
AL_FUNC(void, al_merge_config_into, (ALLEGRO_CONFIG *master, const ALLEGRO_CONFIG *add));
AL_FUNC(ALLEGRO_CONFIG *, al_merge_config, (const ALLEGRO_CONFIG *cfg1, const ALLEGRO_CONFIG *cfg2));
AL_FUNC(void, al_destroy_config, (ALLEGRO_CONFIG *config));

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */
