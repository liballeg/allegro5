#ifndef __al_included_allegro5_path_h
#define __al_included_allegro5_path_h

#include "allegro5/base.h"
#include "allegro5/utf8.h"

#ifdef __cplusplus
   extern "C" {
#endif


#ifdef A5O_WINDOWS
#  define A5O_NATIVE_PATH_SEP '\\'
#  define A5O_NATIVE_DRIVE_SEP ':'
#else
#  define A5O_NATIVE_PATH_SEP '/'
#  define A5O_NATIVE_DRIVE_SEP '\0'
#endif

typedef struct A5O_PATH A5O_PATH;

AL_FUNC(A5O_PATH*, al_create_path, (const char *str));
AL_FUNC(A5O_PATH*, al_create_path_for_directory, (const char *str));
AL_FUNC(A5O_PATH*, al_clone_path, (const A5O_PATH *path));

AL_FUNC(int, al_get_path_num_components, (const A5O_PATH *path));
AL_FUNC(const char*, al_get_path_component, (const A5O_PATH *path, int i));
AL_FUNC(void, al_replace_path_component, (A5O_PATH *path, int i, const char *s));
AL_FUNC(void, al_remove_path_component, (A5O_PATH *path, int i));
AL_FUNC(void, al_insert_path_component, (A5O_PATH *path, int i, const char *s));
AL_FUNC(const char*, al_get_path_tail, (const A5O_PATH *path));
AL_FUNC(void, al_drop_path_tail, (A5O_PATH *path));
AL_FUNC(void, al_append_path_component, (A5O_PATH *path, const char *s));
AL_FUNC(bool, al_join_paths, (A5O_PATH *path, const A5O_PATH *tail));
AL_FUNC(bool, al_rebase_path, (const A5O_PATH *head, A5O_PATH *tail));
AL_FUNC(const char*, al_path_cstr, (const A5O_PATH *path, char delim));
AL_FUNC(const A5O_USTR*, al_path_ustr, (const A5O_PATH *path, char delim));
AL_FUNC(void, al_destroy_path, (A5O_PATH *path));

AL_FUNC(void, al_set_path_drive, (A5O_PATH *path, const char *drive));
AL_FUNC(const char*, al_get_path_drive, (const A5O_PATH *path));

AL_FUNC(void, al_set_path_filename, (A5O_PATH *path, const char *filename));
AL_FUNC(const char*, al_get_path_filename, (const A5O_PATH *path));

AL_FUNC(const char*, al_get_path_extension, (const A5O_PATH *path));
AL_FUNC(bool, al_set_path_extension, (A5O_PATH *path, char const *extension));
AL_FUNC(const char*, al_get_path_basename, (const A5O_PATH *path));

AL_FUNC(bool, al_make_path_canonical, (A5O_PATH *path));


#ifdef __cplusplus
   }
#endif

#endif
