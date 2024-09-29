#ifndef __al_included_allegro5_config_h
#define __al_included_allegro5_config_h

#include "allegro5/file.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Type: A5O_CONFIG
 */
typedef struct A5O_CONFIG A5O_CONFIG;

/* Type: A5O_CONFIG_SECTION
 */
typedef struct A5O_CONFIG_SECTION A5O_CONFIG_SECTION;

/* Type: A5O_CONFIG_ENTRY
 */
typedef struct A5O_CONFIG_ENTRY A5O_CONFIG_ENTRY;

AL_FUNC(A5O_CONFIG *, al_create_config, (void));
AL_FUNC(void, al_add_config_section, (A5O_CONFIG *config, const char *name));
AL_FUNC(void, al_set_config_value, (A5O_CONFIG *config, const char *section, const char *key, const char *value));
AL_FUNC(void, al_add_config_comment, (A5O_CONFIG *config, const char *section, const char *comment));
AL_FUNC(const char*, al_get_config_value, (const A5O_CONFIG *config, const char *section, const char *key));
AL_FUNC(A5O_CONFIG*, al_load_config_file, (const char *filename));
AL_FUNC(A5O_CONFIG*, al_load_config_file_f, (A5O_FILE *filename));
AL_FUNC(bool, al_save_config_file, (const char *filename, const A5O_CONFIG *config));
AL_FUNC(bool, al_save_config_file_f, (A5O_FILE *file, const A5O_CONFIG *config));
AL_FUNC(void, al_merge_config_into, (A5O_CONFIG *master, const A5O_CONFIG *add));
AL_FUNC(A5O_CONFIG *, al_merge_config, (const A5O_CONFIG *cfg1, const A5O_CONFIG *cfg2));
AL_FUNC(void, al_destroy_config, (A5O_CONFIG *config));
AL_FUNC(bool, al_remove_config_section, (A5O_CONFIG *config,
		char const *section));
AL_FUNC(bool, al_remove_config_key, (A5O_CONFIG *config,
		char const *section, char const *key));

AL_FUNC(char const *, al_get_first_config_section, (A5O_CONFIG const *config, A5O_CONFIG_SECTION **iterator));
AL_FUNC(char const *, al_get_next_config_section, (A5O_CONFIG_SECTION **iterator));
AL_FUNC(char const *, al_get_first_config_entry, (A5O_CONFIG const *config, char const *section,
	A5O_CONFIG_ENTRY **iterator));
AL_FUNC(char const *, al_get_next_config_entry, (A5O_CONFIG_ENTRY **iterator));

#ifdef __cplusplus
}
#endif

#endif
