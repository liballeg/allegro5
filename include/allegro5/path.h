#ifndef ALLEGRO_PATH_H
#define ALLEGRO_PATH_H

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

   char *drive;
   char *file;
   
   uint32_t segment_count;
   char **segment;
} AL_PATH;

AL_PATH *al_path_init(void);
AL_PATH *al_path_init_from_string(const char *s, char delim);
int al_path_num_components(AL_PATH *path);
const char *al_path_index(AL_PATH *path, int i);
void al_path_replace(AL_PATH *path, int i, const char *s);
void al_path_delete(AL_PATH *path, int i);
void al_path_insert(AL_PATH *path, int i, const char *s);
const char *al_path_tail(AL_PATH *path);
void al_path_drop_tail(AL_PATH *path);
void al_path_append(AL_PATH *path, const char *s);
void al_path_concat(AL_PATH *path, const AL_PATH *tail);
char *al_path_to_string(AL_PATH *path, char delim);
void al_path_free(AL_PATH *path);

const char *al_path_get_drive(AL_PATH *path);
const char *al_path_get_filename(AL_PATH *path);
const char *al_path_get_abspath(AL_PATH *path);

AL_END_EXTERN_C

#undef /* ALLEGRO_PATH_H */
