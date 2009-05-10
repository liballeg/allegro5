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

AL_FUNC(ALLEGRO_PATH*, al_create_path, (const char *str));
AL_FUNC(ALLEGRO_PATH*, al_create_path_for_dir, (const char *str));
AL_FUNC(ALLEGRO_PATH*, al_clone_path, (const ALLEGRO_PATH *path));

// FIXME: rename to, al_path_num_dir_components
AL_FUNC(int, al_get_path_num_components, (const ALLEGRO_PATH *path));
AL_FUNC(const char*, al_get_path_component, (const ALLEGRO_PATH *path, int i));
AL_FUNC(void, al_replace_path_component, (ALLEGRO_PATH *path, int i, const char *s));
AL_FUNC(void, al_remove_path_component, (ALLEGRO_PATH *path, int i));
AL_FUNC(void, al_insert_path_component, (ALLEGRO_PATH *path, int i, const char *s));
AL_FUNC(const char*, al_get_path_tail, (const ALLEGRO_PATH *path));
AL_FUNC(void, al_drop_path_tail, (ALLEGRO_PATH *path));
AL_FUNC(void, al_append_path_component, (ALLEGRO_PATH *path, const char *s));
AL_FUNC(bool, al_join_paths, (ALLEGRO_PATH *path, const ALLEGRO_PATH *tail));
AL_FUNC(const char*, al_path_cstr, (const ALLEGRO_PATH *path, char delim));
AL_FUNC(void, al_free_path, (ALLEGRO_PATH *path));

AL_FUNC(void, al_set_path_drive, (ALLEGRO_PATH *path, const char *drive));
AL_FUNC(const char*, al_get_path_drive, (const ALLEGRO_PATH *path));

AL_FUNC(void, al_set_path_filename, (ALLEGRO_PATH *path, const char *filename));
AL_FUNC(const char*, al_get_path_filename, (const ALLEGRO_PATH *path));

AL_FUNC(const char*, al_get_path_extension, (const ALLEGRO_PATH *path));
AL_FUNC(bool, al_set_path_extension, (ALLEGRO_PATH *path, char const *extension));
AL_FUNC(const char*, al_get_path_basename, (const ALLEGRO_PATH *path));

AL_FUNC(bool, al_make_path_absolute, (ALLEGRO_PATH *path));
AL_FUNC(bool, al_make_path_canonical, (ALLEGRO_PATH *path));

AL_FUNC(bool, al_is_path_present, (const ALLEGRO_PATH *path));

AL_END_EXTERN_C

#endif /* ALLEGRO_PATH_H */
