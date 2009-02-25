#ifndef ALLEGRO_PATH_H
#define ALLEGRO_PATH_H

#include "allegro5/base.h"
AL_BEGIN_EXTERN_C

#ifdef ALLEGRO_WINDOWS
#  define ALLEGRO_NATIVE_PATH_SEP '\\'
#  define ALLEGRO_NATIVE_DRIVE_SEP ':'
#else
#  define ALLEGRO_NATIVE_PATH_SEP '/'
#  define ALLEGRO_NATIVE_DRIVE_SEP '\0'
#endif

typedef struct ALLEGRO_PATH ALLEGRO_PATH;

AL_FUNC(ALLEGRO_PATH*, al_path_create, (const char *str));
AL_FUNC(ALLEGRO_PATH*, al_path_clone, (const ALLEGRO_PATH *path));

// FIXME: rename to, al_path_num_dir_components
AL_FUNC(int, al_path_num_components, (const ALLEGRO_PATH *path));
AL_FUNC(const char*, al_path_index, (const ALLEGRO_PATH *path, int i));
AL_FUNC(void, al_path_replace, (ALLEGRO_PATH *path, int i, const char *s));
AL_FUNC(void, al_path_remove, (ALLEGRO_PATH *path, int i));
AL_FUNC(void, al_path_insert, (ALLEGRO_PATH *path, int i, const char *s));
AL_FUNC(const char*, al_path_tail, (const ALLEGRO_PATH *path));
AL_FUNC(void, al_path_drop_tail, (ALLEGRO_PATH *path));
AL_FUNC(void, al_path_append, (ALLEGRO_PATH *path, const char *s));
AL_FUNC(bool, al_path_concat, (ALLEGRO_PATH *path, const ALLEGRO_PATH *tail));
AL_FUNC(const char*, al_path_to_string, (const ALLEGRO_PATH *path, char delim));
AL_FUNC(void, al_path_free, (ALLEGRO_PATH *path));

AL_FUNC(void, al_path_set_drive, (ALLEGRO_PATH *path, const char *drive));
AL_FUNC(const char*, al_path_get_drive, (const ALLEGRO_PATH *path));

AL_FUNC(void, al_path_set_filename, (ALLEGRO_PATH *path, const char *filename));
AL_FUNC(const char*, al_path_get_filename, (const ALLEGRO_PATH *path));

AL_FUNC(const char*, al_path_get_extension, (const ALLEGRO_PATH *path));
AL_FUNC(bool, al_path_set_extension, (ALLEGRO_PATH *path, char const *extension));
AL_FUNC(const char*, al_path_get_basename, (const ALLEGRO_PATH *path));

AL_FUNC(bool, al_path_make_absolute, (ALLEGRO_PATH *path));
AL_FUNC(bool, al_path_make_canonical, (ALLEGRO_PATH *path));

AL_FUNC(bool, al_path_exists, (const ALLEGRO_PATH *path));
AL_FUNC(bool, al_path_emode, (const ALLEGRO_PATH *path, uint32_t mode));

AL_END_EXTERN_C

#endif /* ALLEGRO_PATH_H */
