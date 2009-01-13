#ifndef ALLEGRO_INTERNAL_MEMFILE_H
#define ALLEGRO_INTERNAL_MEMFILE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ALLEGRO_FS_ENTRY_MEMFILE ALLEGRO_FS_ENTRY_MEMFILE;
struct ALLEGRO_FS_ENTRY_MEMFILE {
   ALLEGRO_FS_ENTRY fs_entry;   /* must be first */

   bool eof;
   int64_t size;
   int64_t pos;
   char *mem;

   unsigned char ungetc;

   time_t atime;
   time_t mtime;
};

#ifdef __cplusplus
}
#endif

#endif /* ALLEGRO_INTERNAL_MEMFILE_H */
