#include <allegro5/allegro.h>
#include "allegro5/allegro_memfile.h"

typedef struct A5O_FILE_MEMFILE A5O_FILE_MEMFILE;

struct A5O_FILE_MEMFILE {
   bool readable;
   bool writable;
   
   bool eof;
   int64_t size;
   int64_t pos;
   char *mem;
};

static bool memfile_fclose(A5O_FILE *fp)
{
   al_free(al_get_file_userdata(fp));
   return true;
}

static size_t memfile_fread(A5O_FILE *fp, void *ptr, size_t size)
{
   A5O_FILE_MEMFILE *mf = al_get_file_userdata(fp);
   size_t n = 0;
   
   if (!mf->readable) {
      al_set_errno(EPERM);
      return 0;
   }
   
   if (mf->size - mf->pos < (int64_t)size) { 
      /* partial read */
      n = mf->size - mf->pos;
      mf->eof = true;
   }
   else {
      n = size;
   }

   memcpy(ptr, mf->mem + mf->pos, n);
   mf->pos += n;   
   
   return n;
}

static size_t memfile_fwrite(A5O_FILE *fp, const void *ptr, size_t size)
{
   A5O_FILE_MEMFILE *mf = al_get_file_userdata(fp);
   size_t n;
   
   if (!mf->writable) {
      al_set_errno(EPERM);
      return 0;
   }   
   
   if (mf->size - mf->pos < (int64_t)size) {
      /* partial write */
      n = mf->size - mf->pos;
      mf->eof = true;
   }
   else {
      n = size;
   }

   memcpy(mf->mem + mf->pos, ptr, n);
   mf->pos += n;
   
   return n;
}

static bool memfile_fflush(A5O_FILE *fp)
{
   (void)fp;
   return true;
}

static int64_t memfile_ftell(A5O_FILE *fp)
{
   A5O_FILE_MEMFILE *mf = al_get_file_userdata(fp);

   return mf->pos;
}

static bool memfile_fseek(A5O_FILE *fp, int64_t offset,
   int whence)
{
   A5O_FILE_MEMFILE *mf = al_get_file_userdata(fp);
   int64_t pos = mf->pos;

   switch (whence) {
      case A5O_SEEK_SET:
         pos = offset;
         break;

      case A5O_SEEK_CUR:
         pos = mf->pos + offset;
         break;

      case A5O_SEEK_END:
         pos = mf->size + offset;
         break;
   }

   if (pos >= mf->size)
      pos = mf->size;
   else if (pos < 0)
      pos = 0;

   mf->pos = pos;

   mf->eof = false;
   
   return true;
}

/* doesn't quite match up to stdio here,
   an feof after a seek will return false,
   even if it seeks past the end of the file */
static bool memfile_feof(A5O_FILE *fp)
{
   A5O_FILE_MEMFILE *mf = al_get_file_userdata(fp);
   return mf->eof;
}

static int memfile_ferror(A5O_FILE *fp)
{
   (void)fp;
   return 0;
}

static const char *memfile_ferrmsg(A5O_FILE *fp)
{
   (void)fp;
   return "";
}

static void memfile_fclearerr(A5O_FILE *fp)
{
   A5O_FILE_MEMFILE *mf = al_get_file_userdata(fp);
   mf->eof = false;
}

static off_t memfile_fsize(A5O_FILE *fp)
{
   A5O_FILE_MEMFILE *mf = al_get_file_userdata(fp);

   return mf->size;
}

static struct A5O_FILE_INTERFACE memfile_vtable = {
   NULL,    /* open */
   memfile_fclose,
   memfile_fread,
   memfile_fwrite,
   memfile_fflush,
   memfile_ftell,
   memfile_fseek,
   memfile_feof,
   memfile_ferror,
   memfile_ferrmsg,
   memfile_fclearerr,
   NULL,   /* ungetc */
   memfile_fsize
};

/* Function: al_open_memfile
 */
A5O_FILE *al_open_memfile(void *mem, int64_t size, const char *mode)
{
   A5O_FILE *memfile;
   A5O_FILE_MEMFILE *userdata = NULL;

   ASSERT(mem);
   ASSERT(size > 0);
   
   userdata = al_malloc(sizeof(A5O_FILE_MEMFILE));
   if (!userdata) {
      al_set_errno(ENOMEM);
      return NULL;
   }
   
   memset(userdata, 0, sizeof(*userdata));
   userdata->size = size;
   userdata->pos = 0;
   userdata->mem = mem;
   
   userdata->readable = strchr(mode, 'r') || strchr(mode, 'R');
   userdata->writable = strchr(mode, 'w') || strchr(mode, 'W');
      
   memfile = al_create_file_handle(&memfile_vtable, userdata);
   if (!memfile) {
      al_free(userdata);
   }

   return memfile;
}

/* Function: al_get_allegro_memfile_version
 */
uint32_t al_get_allegro_memfile_version(void)
{
   return A5O_VERSION_INT;
}

/* vim: set sts=3 sw=3 et: */
