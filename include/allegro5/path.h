#ifndef ALLEGRO_PATH_H
#define ALLEGRO_PATH_H

AL_BEGIN_EXTERN_C

typedef struct AL_PATH {
   char *path;
   uint32_t dirty;

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

AL_END_EXTERN_C

#undef /* ALLEGRO_PATH_H */
