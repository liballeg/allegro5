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

typedef struct AL_PATH {
   char *abspath;
   uint32_t dirty;

   uint32_t free;

   char *drive;
   char *filename;

   int32_t segment_count;
   char **segment;
} AL_PATH;

AL_FUNC(AL_PATH*, al_path_create, (AL_CONST char *str));
AL_FUNC(int32_t, al_path_init, (AL_PATH *path, AL_CONST char *str));

// FIXME: rename to, al_path_num_dir_components
AL_FUNC(int, al_path_num_components, (AL_PATH *path));
AL_FUNC(const char*, al_path_index, (AL_PATH *path, int i));
AL_FUNC(void, al_path_replace, (AL_PATH *path, int i, const char *s));
AL_FUNC(void, al_path_remove, (AL_PATH *path, int i));
AL_FUNC(void, al_path_insert, (AL_PATH *path, int i, const char *s));
AL_FUNC(const char*, al_path_tail, (AL_PATH *path));
AL_FUNC(void, al_path_drop_tail, (AL_PATH *path));
AL_FUNC(void, al_path_append, (AL_PATH *path, const char *s));
AL_FUNC(void, al_path_concat, (AL_PATH *path, const AL_PATH *tail));
AL_FUNC(char*, al_path_to_string, (AL_PATH *path, char *buffer, size_t len, char delim));
AL_FUNC(void, al_path_free, (AL_PATH *path));

AL_FUNC(int32_t, al_path_set_drive, (AL_PATH *path, AL_CONST char *drive));
AL_FUNC(const char*, al_path_get_drive, (AL_PATH *path));

AL_FUNC(int32_t, al_path_set_filename, (AL_PATH *path, AL_CONST char *filename));
AL_FUNC(const char*, al_path_get_filename, (AL_PATH *path));

AL_FUNC(const char*, al_path_get_extension, (AL_PATH *path, char *buf, size_t len));
AL_FUNC(const char*, al_path_get_basename, (AL_PATH *path, char *buf, size_t len));

/* FIXME: implement kthx bye */
/*
AL_FUNC(char*, al_path_absolute, (AL_PATH *path, char *buffer, size_t len, char delim));
AL_FUNC(char*, al_path_relative, (AL_PATH *path, char *buffer, size_t len, char delim));
AL_FUNC(char*, al_path_cannonical, (AL_PATH *path, char *buffer, size_t len, char delim));
*/

AL_FUNC(uint32_t, al_path_exists, (AL_PATH *path));
AL_FUNC(uint32_t, al_path_emode, (AL_PATH *path, uint32_t mode));

AL_END_EXTERN_C

#endif /* ALLEGRO_PATH_H */
