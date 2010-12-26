#include <allegro5/allegro.h>
#include "allegro5/memfile.h"


typedef struct ALLEGRO_FILE_MEMFILE ALLEGRO_FILE_MEMFILE;

struct ALLEGRO_FILE_MEMFILE {
   bool eof;
   int64_t size;
   int64_t pos;
   char *mem;

   unsigned char ungetc;
};

static void memfile_fclose(ALLEGRO_FILE *fp)
{
   al_free(al_get_file_userdata(fp));
}

static size_t memfile_fread(ALLEGRO_FILE *fp, void *ptr, size_t size)
{
   ALLEGRO_FILE_MEMFILE *mf = al_get_file_userdata(fp);
   size_t n;

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

static size_t memfile_fwrite(ALLEGRO_FILE *fp, const void *ptr, size_t size)
{
   ALLEGRO_FILE_MEMFILE *mf = al_get_file_userdata(fp);
   size_t n;

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

static bool memfile_fflush(ALLEGRO_FILE *fp)
{
   (void)fp;
   return true;
}

static int64_t memfile_ftell(ALLEGRO_FILE *fp)
{
   ALLEGRO_FILE_MEMFILE *mf = al_get_file_userdata(fp);

   return mf->pos;
}

static bool memfile_fseek(ALLEGRO_FILE *fp, int64_t offset,
   int whence)
{
   ALLEGRO_FILE_MEMFILE *mf = al_get_file_userdata(fp);
   int64_t pos = mf->pos;

   switch (whence) {
      case ALLEGRO_SEEK_SET:
         pos = offset;
         break;

      case ALLEGRO_SEEK_CUR:
         pos = mf->pos + offset;
         break;

      case ALLEGRO_SEEK_END:
         pos = mf->size + offset;
         break;
   }

   if (pos >= mf->size)
      pos = mf->size;
   else if(pos < 0)
      pos = 0;

   mf->pos = pos;

   mf->eof = false;
   
   return true;
}

/* doesn't quite match up to stdio here,
   an feof after a seek will return false,
   even if it seeks past the end of the file */
static bool memfile_feof(ALLEGRO_FILE *fp)
{
   ALLEGRO_FILE_MEMFILE *mf = al_get_file_userdata(fp);
   return mf->eof;
}

static bool memfile_ferror(ALLEGRO_FILE *fp)
{
   (void)fp;
   return false;
}

static void memfile_fclearerr(ALLEGRO_FILE *fp)
{
   ALLEGRO_FILE_MEMFILE *mf = al_get_file_userdata(fp);
   mf->eof = false;
}

static int memfile_fungetc(ALLEGRO_FILE *fp, int c)
{
   ALLEGRO_FILE_MEMFILE *mf = al_get_file_userdata(fp);

   /* XXX this isn't used at all */
   mf->ungetc = (unsigned char)c;

   return c;
}

static off_t memfile_fsize(ALLEGRO_FILE *fp)
{
   ALLEGRO_FILE_MEMFILE *mf = al_get_file_userdata(fp);

   return mf->size;
}

static struct ALLEGRO_FILE_INTERFACE memfile_vtable = {
   NULL,    /* open */
   memfile_fclose,
   memfile_fread,
   memfile_fwrite,
   memfile_fflush,
   memfile_ftell,
   memfile_fseek,
   memfile_feof,
   memfile_ferror,
   memfile_fclearerr,
   memfile_fungetc,
   memfile_fsize
};

/* Function: al_open_memfile
 */
ALLEGRO_FILE *al_open_memfile(int64_t size, void *mem)
{
   ALLEGRO_FILE *memfile;
   ALLEGRO_FILE_MEMFILE *userdata = NULL;

   ASSERT(mem);
   ASSERT(size > 0);
   
   userdata = al_malloc(sizeof(ALLEGRO_FILE_MEMFILE));
   if(!userdata) {
      al_set_errno(ENOMEM);
      return NULL;
   }
   
   memset(userdata, 0, sizeof(*userdata));
   userdata->size = size;
   userdata->pos = 0;
   userdata->mem = mem;
      
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
   return ALLEGRO_VERSION_INT;
}

/* vim: set sts=3 sw=3 et: */
