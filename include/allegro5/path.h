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

typedef struct ALLEGRO_PATH {
   char *abspath;
   uint32_t dirty;

   uint32_t free;

   char *drive;
   char *filename;

   int32_t segment_count;
   char **segment;
} ALLEGRO_PATH;

AL_FUNC(ALLEGRO_PATH*, al_path_create, (AL_CONST char *str));
AL_FUNC(int32_t, al_path_init, (ALLEGRO_PATH *path, AL_CONST char *str));

// FIXME: rename to, al_path_num_dir_components
AL_FUNC(int, al_path_num_components, (ALLEGRO_PATH *path));
AL_FUNC(const char*, al_path_index, (ALLEGRO_PATH *path, int i));
AL_FUNC(void, al_path_replace, (ALLEGRO_PATH *path, int i, const char *s));
AL_FUNC(void, al_path_remove, (ALLEGRO_PATH *path, int i));
AL_FUNC(void, al_path_insert, (ALLEGRO_PATH *path, int i, const char *s));
AL_FUNC(const char*, al_path_tail, (ALLEGRO_PATH *path));
AL_FUNC(void, al_path_drop_tail, (ALLEGRO_PATH *path));
AL_FUNC(void, al_path_append, (ALLEGRO_PATH *path, const char *s));
AL_FUNC(void, al_path_concat, (ALLEGRO_PATH *path, const ALLEGRO_PATH *tail));
AL_FUNC(char*, al_path_to_string, (ALLEGRO_PATH *path, char *buffer, size_t len, char delim));
AL_FUNC(void, al_path_free, (ALLEGRO_PATH *path));

AL_FUNC(int32_t, al_path_set_drive, (ALLEGRO_PATH *path, AL_CONST char *drive));
AL_FUNC(const char*, al_path_get_drive, (ALLEGRO_PATH *path));

AL_FUNC(int32_t, al_path_set_filename, (ALLEGRO_PATH *path, AL_CONST char *filename));
AL_FUNC(const char*, al_path_get_filename, (ALLEGRO_PATH *path));

AL_FUNC(const char*, al_path_get_extension, (ALLEGRO_PATH *path, char *buf, size_t len));
AL_FUNC(const char*, al_path_get_basename, (ALLEGRO_PATH *path, char *buf, size_t len));

/* FIXME: implement kthx bye */
/*
AL_FUNC(char*, al_path_absolute, (ALLEGRO_PATH *path, char *buffer, size_t len, char delim));
AL_FUNC(char*, al_path_relative, (ALLEGRO_PATH *path, char *buffer, size_t len, char delim));
AL_FUNC(char*, al_path_cannonical, (ALLEGRO_PATH *path, char *buffer, size_t len, char delim));
*/

AL_FUNC(uint32_t, al_path_exists, (ALLEGRO_PATH *path));
AL_FUNC(uint32_t, al_path_emode, (ALLEGRO_PATH *path, uint32_t mode));

AL_END_EXTERN_C

#endif /* ALLEGRO_PATH_H */
