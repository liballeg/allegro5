#include <stdio.h>
#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern_file.h"
#include "allegro5/internal/aintern_memory.h"


typedef struct ALLEGRO_FILE_STDIO ALLEGRO_FILE_STDIO;
struct ALLEGRO_FILE_STDIO {
   ALLEGRO_FILE file;   /* must be first */
   FILE *fp;
};


/* forward declaration */
const struct ALLEGRO_FILE_INTERFACE _al_file_interface_stdio;


static FILE *get_fp(ALLEGRO_FILE *f)
{
   if (f)
      return ((ALLEGRO_FILE_STDIO *)f)->fp;
   else
      return NULL;
}


static ALLEGRO_FILE *file_stdio_fopen(const char *path, const char *mode)
{
   FILE *fp;
   ALLEGRO_FILE_STDIO *f;

   fp = fopen(path, mode);
   if (!fp)
      return NULL;

   f = _AL_MALLOC(sizeof(*f));
   if (!f) {
      fclose(fp);
      return NULL;
   }

   f->file.vtable = &_al_file_interface_stdio;
   f->fp = fp;
   return (ALLEGRO_FILE *)f;
}


static void file_stdio_fclose(ALLEGRO_FILE *f)
{
   if (f) {
      fclose(get_fp(f));
      _AL_FREE(f);
   }
}


static size_t file_stdio_fread(ALLEGRO_FILE *f, void *ptr, size_t size)
{
   size_t ret;

   ret = fread(ptr, 1, size, get_fp(f));
   if (ret < size) {
      al_set_errno(errno);
   }

   return ret;
}


static size_t file_stdio_fwrite(ALLEGRO_FILE *f, const void *ptr, size_t size)
{
   size_t ret;

   ret = fwrite(ptr, 1, size, get_fp(f));
   if (ret < size) {
      al_set_errno(errno);
   }

   return ret;
}


static bool file_stdio_fflush(ALLEGRO_FILE *f)
{
   FILE *fp = get_fp(f);

   if (fflush(fp) == EOF) {
      al_set_errno(errno);
      return false;
   }

   return true;
}


static int64_t file_stdio_ftell(ALLEGRO_FILE *f)
{
   FILE *fp = get_fp(f);
   int64_t ret;

#ifdef ALLEGRO_HAVE_FTELLO
   ret = ftello(fp);
#else
   ret = ftell(fp);
#endif
   if (ret == -1) {
      al_set_errno(errno);
   }

   return ret;
}


static bool file_stdio_fseek(ALLEGRO_FILE *f, int64_t offset,
   int whence)
{
   FILE *fp = get_fp(f);
   int rc;

   switch (whence) {
      case ALLEGRO_SEEK_SET: whence = SEEK_SET; break;
      case ALLEGRO_SEEK_CUR: whence = SEEK_CUR; break;
      case ALLEGRO_SEEK_END: whence = SEEK_END; break;
   }

#ifdef ALLEGRO_HAVE_FSEEKO
   rc = fseeko(fp, offset, whence);
#else
   rc = fseek(fp, offset, whence);
#endif

   if (rc == -1) {
      al_set_errno(errno);
      return false;
   }

   return true;
}


static bool file_stdio_feof(ALLEGRO_FILE *f)
{
   return feof(get_fp(f));
}


static bool file_stdio_ferror(ALLEGRO_FILE *f)
{
   return ferror(get_fp(f));
}


static int file_stdio_fungetc(ALLEGRO_FILE *f, int c)
{
   return ungetc(c, get_fp(f));
}


static off_t file_stdio_fsize(ALLEGRO_FILE *f)
{
   int64_t old_pos;
   int64_t new_pos;

   old_pos = file_stdio_ftell(f);
   if (old_pos == -1)
      return -1;

   if (!file_stdio_fseek(f, ALLEGRO_SEEK_END, 0))
      return -1;

   new_pos = file_stdio_ftell(f);
   if (new_pos == -1)
      return -1;

   if (!file_stdio_fseek(f, ALLEGRO_SEEK_SET, old_pos))
      return -1;

   return new_pos;
}


const struct ALLEGRO_FILE_INTERFACE _al_file_interface_stdio =
{
   file_stdio_fopen,
   file_stdio_fclose,
   file_stdio_fread,
   file_stdio_fwrite,
   file_stdio_fflush,
   file_stdio_ftell,
   file_stdio_fseek,
   file_stdio_feof,
   file_stdio_ferror,
   file_stdio_fungetc,
   file_stdio_fsize
};


/* vim: set sts=3 sw=3 et: */
