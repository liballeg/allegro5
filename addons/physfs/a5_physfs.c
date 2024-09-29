/*
 *       PhysicsFS addon for Allegro.
 *
 *       By Peter Wang.
 */

#include <physfs.h>
#include "allegro5/allegro.h"
#include "allegro5/allegro_physfs.h"

#include "allegro_physfs_intern.h"

A5O_DEBUG_CHANNEL("physfs")


typedef struct A5O_FILE_PHYSFS A5O_FILE_PHYSFS;

struct A5O_FILE_PHYSFS
{
   PHYSFS_file *phys;
   bool error_indicator;
   char error_msg[80];
};

/* forward declaration */
static const A5O_FILE_INTERFACE file_phys_vtable;

const A5O_FILE_INTERFACE *_al_get_phys_vtable(void)
{
   return &file_phys_vtable;
}


#define streq(a, b)  (0 == strcmp(a, b))


static A5O_FILE_PHYSFS *cast_stream(A5O_FILE *f)
{
   return (A5O_FILE_PHYSFS *)al_get_file_userdata(f);
}


static void phys_set_errno(A5O_FILE_PHYSFS *fp)
{
   /* It might be worth mapping some common error strings from
    * PHYSFS_getLastError() onto errno values.  There are no guarantees,
    * though.
    */
   al_set_errno(-1);

   PHYSFS_ErrorCode error = PHYSFS_getLastErrorCode();
   const char* msg = PHYSFS_getErrorByCode(error);
   if (error != PHYSFS_ERR_OK) {
      A5O_ERROR("PhysFS error code: %s\n", msg);
   }
   if (fp) {
      fp->error_indicator = true;
      if (error != PHYSFS_ERR_OK) {
         strncpy(fp->error_msg, msg, sizeof(fp->error_msg));
         fp->error_msg[sizeof(fp->error_msg) - 1] = '\0';
      }
      else {
         fp->error_msg[0] = '\0';
      }
   }
}


static void *file_phys_fopen(const char *filename, const char *mode)
{
   A5O_USTR *us;
   PHYSFS_file *phys;
   A5O_FILE_PHYSFS *fp;

   us = _al_physfs_process_path(filename);

   /* XXX handle '+' modes */
   /* It might be worth adding a function to parse mode strings, to be
    * shared amongst these kinds of addons.
    */
   if (streq(mode, "r") || streq(mode, "rb"))
      phys = PHYSFS_openRead(al_cstr(us));
   else if (streq(mode, "w") || streq(mode, "wb"))
      phys = PHYSFS_openWrite(al_cstr(us));
   else if (streq(mode, "a") || streq(mode, "ab"))
      phys = PHYSFS_openAppend(al_cstr(us));
   else
      phys = NULL;

   al_ustr_free(us);

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
   fp->error_msg[0] = '\0';

   return fp;
}


static bool file_phys_fclose(A5O_FILE *f)
{
   A5O_FILE_PHYSFS *fp = cast_stream(f);
   PHYSFS_file *phys_fp = fp->phys;

   al_free(fp);

   if (PHYSFS_close(phys_fp) == 0) {
      al_set_errno(-1);
      return false;
   }

   return true;
}


static size_t file_phys_fread(A5O_FILE *f, void *buf, size_t buf_size)
{
   A5O_FILE_PHYSFS *fp = cast_stream(f);
   PHYSFS_sint64 n;

   if (buf_size == 0)
      return 0;

   n = PHYSFS_readBytes(fp->phys, buf, buf_size);
   if (n < 0) {
      phys_set_errno(fp);
      return 0;
   }
   return n;
}


static size_t file_phys_fwrite(A5O_FILE *f, const void *buf,
   size_t buf_size)
{
   A5O_FILE_PHYSFS *fp = cast_stream(f);
   PHYSFS_sint64 n;

   n = PHYSFS_writeBytes(fp->phys, buf, buf_size);
   if (n < 0) {
      phys_set_errno(fp);
      return 0;
   }

   return n;
}


static bool file_phys_fflush(A5O_FILE *f)
{
   A5O_FILE_PHYSFS *fp = cast_stream(f);

   if (!PHYSFS_flush(fp->phys)) {
      phys_set_errno(fp);
      return false;
   }

   return true;
}


static int64_t file_phys_ftell(A5O_FILE *f)
{
   A5O_FILE_PHYSFS *fp = cast_stream(f);
   PHYSFS_sint64 n;

   n = PHYSFS_tell(fp->phys);
   if (n < 0) {
      phys_set_errno(fp);
      return -1;
   }

   return n;
}


static bool file_phys_seek(A5O_FILE *f, int64_t offset, int whence)
{
   A5O_FILE_PHYSFS *fp = cast_stream(f);
   PHYSFS_sint64 base;

   switch (whence) {
      case A5O_SEEK_SET:
         base = 0;
         break;

      case A5O_SEEK_CUR:
         base = PHYSFS_tell(fp->phys);
         if (base < 0) {
            phys_set_errno(fp);
            return false;
         }
         break;

      case A5O_SEEK_END:
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


static bool file_phys_feof(A5O_FILE *f)
{
   A5O_FILE_PHYSFS *fp = cast_stream(f);

   return PHYSFS_eof(fp->phys);
}


static int file_phys_ferror(A5O_FILE *f)
{
   A5O_FILE_PHYSFS *fp = cast_stream(f);

   return (fp->error_indicator) ? 1 : 0;
}


static const char *file_phys_ferrmsg(A5O_FILE *f)
{
   A5O_FILE_PHYSFS *fp = cast_stream(f);

   if (fp->error_indicator)
      return fp->error_msg;
   else
      return "";
}


static void file_phys_fclearerr(A5O_FILE *f)
{
   A5O_FILE_PHYSFS *fp = cast_stream(f);

   fp->error_indicator = false;

   /* PhysicsFS doesn't provide a way to clear the EOF indicator. */
}


static off_t file_phys_fsize(A5O_FILE *f)
{
   A5O_FILE_PHYSFS *fp = cast_stream(f);
   PHYSFS_sint64 n;

   n = PHYSFS_fileLength(fp->phys);
   if (n < 0) {
      phys_set_errno(fp);
      return -1;
   }

   return n;
}


static const A5O_FILE_INTERFACE file_phys_vtable =
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
   file_phys_ferrmsg,
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
   return A5O_VERSION_INT;
}


/* vim: set sts=3 sw=3 et: */
