/*
 *       PhysicsFS addon for Allegro.
 *
 *       By Peter Wang.
 */

#include <physfs.h>
#include "allegro5/allegro.h"
#include "allegro5/allegro_physfs.h"

#include "allegro_physfs_intern.h"


typedef struct ALLEGRO_FILE_PHYSFS ALLEGRO_FILE_PHYSFS;

struct ALLEGRO_FILE_PHYSFS
{
   PHYSFS_file *phys;
   bool error_indicator;
};

/* forward declaration */
static const ALLEGRO_FILE_INTERFACE file_phys_vtable;

const ALLEGRO_FILE_INTERFACE *_al_get_phys_vtable(void)
{
   return &file_phys_vtable;
}


#define streq(a, b)  (0 == strcmp(a, b))


static ALLEGRO_FILE_PHYSFS *cast_stream(ALLEGRO_FILE *f)
{
   return (ALLEGRO_FILE_PHYSFS *)al_get_file_userdata(f);
}


static void phys_set_errno(ALLEGRO_FILE_PHYSFS *fp)
{
   /* It might be worth mapping some common error strings from
    * PHYSFS_getLastError() onto errno values.  There are no guarantees,
    * though.
    */
   al_set_errno(-1);

   if (fp) {
      fp->error_indicator = true;
   }
}


static void *file_phys_fopen(const char *filename, const char *mode)
{
   PHYSFS_file *phys;
   ALLEGRO_FILE_PHYSFS *fp;

   /* XXX handle '+' modes */
   /* It might be worth adding a function to parse mode strings, to be
    * shared amongst these kinds of addons.
    */
   if (streq(mode, "r") || streq(mode, "rb"))
      phys = PHYSFS_openRead(filename);
   else if (streq(mode, "w") || streq(mode, "wb"))
      phys = PHYSFS_openWrite(filename);
   else if (streq(mode, "a") || streq(mode, "ab"))
      phys = PHYSFS_openAppend(filename);
   else
      phys = NULL;

   if (!phys) {
      phys_set_errno(NULL);
      return NULL;
   }

   fp = al_malloc(sizeof(*fp));
   if (!fp) {
      al_set_errno(ENOMEM);
      PHYSFS_close(phys);
      return NULL;
   }

   fp->phys = phys;
   fp->error_indicator = false;

   return fp;
}


static void file_phys_fclose(ALLEGRO_FILE *f)
{
   ALLEGRO_FILE_PHYSFS *fp = cast_stream(f);

   PHYSFS_close(fp->phys);

   al_free(fp);
}


static size_t file_phys_fread(ALLEGRO_FILE *f, void *buf, size_t buf_size)
{
   ALLEGRO_FILE_PHYSFS *fp = cast_stream(f);
   PHYSFS_sint64 n;

   if (buf_size == 0)
      return 0;

   n = PHYSFS_read(fp->phys, buf, 1, buf_size);
   if (n < 0) {
      phys_set_errno(fp);
      return 0;
   }
   return n;
}


static size_t file_phys_fwrite(ALLEGRO_FILE *f, const void *buf,
   size_t buf_size)
{
   ALLEGRO_FILE_PHYSFS *fp = cast_stream(f);
   PHYSFS_sint64 n;

   n = PHYSFS_write(fp->phys, buf, 1, buf_size);
   if (n < 0) {
      phys_set_errno(fp);
      return 0;
   }

   return n;
}


static bool file_phys_fflush(ALLEGRO_FILE *f)
{
   ALLEGRO_FILE_PHYSFS *fp = cast_stream(f);

   if (!PHYSFS_flush(fp->phys)) {
      phys_set_errno(fp);
      return false;
   }

   return true;
}


static int64_t file_phys_ftell(ALLEGRO_FILE *f)
{
   ALLEGRO_FILE_PHYSFS *fp = cast_stream(f);
   PHYSFS_sint64 n;

   n = PHYSFS_tell(fp->phys);
   if (n < 0) {
      phys_set_errno(fp);
      return -1;
   }

   return n;
}


static bool file_phys_seek(ALLEGRO_FILE *f, int64_t offset, int whence)
{
   ALLEGRO_FILE_PHYSFS *fp = cast_stream(f);
   PHYSFS_sint64 base;

   switch (whence) {
      case ALLEGRO_SEEK_SET:
         base = 0;
         break;

      case ALLEGRO_SEEK_CUR:
         base = PHYSFS_tell(fp->phys);
         if (base < 0) {
            phys_set_errno(fp);
            return false;
         }
         break;

      case ALLEGRO_SEEK_END:
         base = PHYSFS_fileLength(fp->phys);
         if (base < 0) {
            phys_set_errno(fp);
            return false;
         }
         break;

      default:
         al_set_errno(EINVAL);
         return false;
   }

   if (!PHYSFS_seek(fp->phys, base + offset)) {
      phys_set_errno(fp);
      return false;
   }
   
   return true;
}


static bool file_phys_feof(ALLEGRO_FILE *f)
{
   ALLEGRO_FILE_PHYSFS *fp = cast_stream(f);

   return PHYSFS_eof(fp->phys);
}


static bool file_phys_ferror(ALLEGRO_FILE *f)
{
   ALLEGRO_FILE_PHYSFS *fp = cast_stream(f);

   return fp->error_indicator;
}


static void file_phys_fclearerr(ALLEGRO_FILE *f)
{
   ALLEGRO_FILE_PHYSFS *fp = cast_stream(f);

   fp->error_indicator = false;

   /* PhysicsFS doesn't provide a way to clear the EOF indicator. */
}


static off_t file_phys_fsize(ALLEGRO_FILE *f)
{
   ALLEGRO_FILE_PHYSFS *fp = cast_stream(f);
   PHYSFS_sint64 n;

   n = PHYSFS_fileLength(fp->phys);
   if (n < 0) {
      phys_set_errno(fp);
      return -1;
   }

   return n;
}


static const ALLEGRO_FILE_INTERFACE file_phys_vtable =
{
   file_phys_fopen,
   file_phys_fclose,
   file_phys_fread,
   file_phys_fwrite,
   file_phys_fflush,
   file_phys_ftell,
   file_phys_seek,
   file_phys_feof,
   file_phys_ferror,
   file_phys_fclearerr,
   NULL,  /* ungetc */
   file_phys_fsize
};


/* Function: al_set_physfs_file_interface
 */
void al_set_physfs_file_interface(void)
{
   al_set_new_file_interface(&file_phys_vtable);
   _al_set_physfs_fs_interface();
}


/* Function: al_get_allegro_physfs_version
 */
uint32_t al_get_allegro_physfs_version(void)
{
   return ALLEGRO_VERSION_INT;
}


/* vim: set sts=3 sw=3 et: */
