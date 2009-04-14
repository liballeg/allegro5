#define ALLEGRO_LIB_BUILD 1
#include <allegro5/allegro5.h>
#include "allegro5/memfile.h"
#include "allegro5/internal/aintern_memory.h"
#include "allegro5/internal/aintern_fshook.h"
#include "allegro5/internal/aintern_memfile.h"
#include <time.h>
#include <stdio.h>

static void memfile_destroy_handle(ALLEGRO_FS_ENTRY *handle)
{
   _AL_FREE(handle);
}

static ALLEGRO_PATH *memfile_fname(ALLEGRO_FS_ENTRY *fh)
{
   (void)fh;
   return al_path_create("");
}

static void memfile_fclose(ALLEGRO_FS_ENTRY *fp)
{
   _AL_FREE(fp);
}

static size_t memfile_fread(ALLEGRO_FS_ENTRY *fp, void *ptr, size_t size)
{
   ALLEGRO_FS_ENTRY_MEMFILE *mf = (ALLEGRO_FS_ENTRY_MEMFILE*)fp;
   size_t ret = 0;

   mf->atime = time(NULL);
   
   /* EOF? return 0 */
   if(mf->pos >= mf->size) {
      mf->eof = true;
      return 0;
   }

   ret = mf->pos + size;
   if(ret > mf->size) {
      /* partial read, return remainder */
      ret = mf->size - ret;
      memcpy(ptr, mf->mem + mf->pos, ret);
      ret = mf->size;
      mf->eof = true;
   } else {
      /* full read */
      memcpy(ptr, mf->mem + mf->pos, size);
   }

   /* move file pos */
   mf->pos = ret;

   return ret;
}

static size_t memfile_fwrite(ALLEGRO_FS_ENTRY *fp, const void *ptr, size_t size)
{
   ALLEGRO_FS_ENTRY_MEMFILE *mf = (ALLEGRO_FS_ENTRY_MEMFILE*)fp;
   size_t ret = 0;

   mf->mtime = time(NULL);
   
   /* EOF? return 0 */
   if(mf->pos >= mf->size) {
      mf->eof = true;
      return 0;
   }

   ret = mf->pos + size;
   if(ret > mf->size) {
      /* partial write, return remainder */
      ret = mf->size - ret;
      memcpy(mf->mem + mf->pos, ptr, ret);
      ret = mf->size;
      mf->eof = true;
   } else {
      /* full write */
      memcpy(mf->mem + mf->pos, ptr, size);
   }

   /* move file pos */
   mf->pos = ret;

   return ret;
}

static bool memfile_fflush(ALLEGRO_FS_ENTRY *fp)
{
   (void)fp;
   return true;
}

static bool memfile_fseek(ALLEGRO_FS_ENTRY *fp, int64_t offset,
   uint32_t whence)
{
   ALLEGRO_FS_ENTRY_MEMFILE *mf = (ALLEGRO_FS_ENTRY_MEMFILE*)fp;
   int64_t pos = mf->pos;

   switch(whence) {
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

   if(pos >= mf->size)
      pos = mf->size;
   else if(pos < 0)
      pos = 0;

   mf->pos = pos;

   mf->eof = false;
   
   return true;
}

static int64_t memfile_ftell(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_MEMFILE *mf = (ALLEGRO_FS_ENTRY_MEMFILE*)fp;

   return mf->pos;
}

static bool memfile_ferror(ALLEGRO_FS_ENTRY *fp)
{
   (void)fp;
   return false;
}

/* doesn't quite match up to stdio here,
   an feof after a seek will return false,
   even if it seeks past the end of the file */
static bool memfile_feof(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_MEMFILE *mf = (ALLEGRO_FS_ENTRY_MEMFILE*)fp;
   return mf->eof;
}

static bool memfile_fstat(ALLEGRO_FS_ENTRY *fp)
{
   (void)fp;
   return true;
}

static int memfile_ungetc(ALLEGRO_FS_ENTRY *fp, int c)
{
   ALLEGRO_FS_ENTRY_MEMFILE *mf = (ALLEGRO_FS_ENTRY_MEMFILE*)fp;

   mf->ungetc = (unsigned char)c;

   return c;
}

static off_t memfile_entry_size(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_MEMFILE *mf = (ALLEGRO_FS_ENTRY_MEMFILE*)fp;

   return mf->size;
}

static uint32_t memfile_entry_mode(ALLEGRO_FS_ENTRY *fp)
{
   (void)fp;
   return ALLEGRO_FM_READ | ALLEGRO_FM_WRITE | ALLEGRO_FM_ISFILE;
}

static time_t memfile_entry_atime(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_MEMFILE *mf = (ALLEGRO_FS_ENTRY_MEMFILE*)fp;
   return mf->atime;
}

static time_t memfile_entry_mtime(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_MEMFILE *mf = (ALLEGRO_FS_ENTRY_MEMFILE*)fp;
   return mf->mtime;
}

static time_t memfile_entry_ctime(ALLEGRO_FS_ENTRY *fp)
{
   (void)fp;
   return 0;      /* XXX can't we implement this? */
}

static bool memfile_exists(ALLEGRO_FS_ENTRY *fp)
{
   (void)fp;
   return true;
}

static bool memfile_unlink(ALLEGRO_FS_ENTRY *fp)
{
   (void)fp;
   return true;   /* XXX should this fail? */
}

static struct ALLEGRO_FS_HOOK_ENTRY_INTERFACE memfile_entry_hooks = {
   memfile_destroy_handle,
   NULL,    /* open */
   NULL,    /* close */

   memfile_fname,

   memfile_fclose,
   memfile_fread,
   memfile_fwrite,
   memfile_fflush,
   memfile_fseek,
   memfile_ftell,
   memfile_ferror,
   memfile_feof,
   memfile_fstat,
   memfile_ungetc,

   memfile_entry_size,
   memfile_entry_mode,
   memfile_entry_atime,
   memfile_entry_mtime,
   memfile_entry_ctime,

   memfile_exists,
   memfile_unlink,

   NULL,    /* readdir */
   NULL     /* closedir */
};

ALLEGRO_FS_ENTRY *al_open_memfile(int64_t size, void *mem)
{
   ALLEGRO_FS_ENTRY_MEMFILE *memfile = NULL;

   ASSERT(mem);
   ASSERT(size > 0);

   memfile = _AL_MALLOC(sizeof(ALLEGRO_FS_ENTRY_MEMFILE));
   if(!memfile) {
      al_set_errno(ENOMEM);
      return NULL;
   }

   memset(memfile, 0, sizeof(*memfile));

   memfile->fs_entry.vtable = &memfile_entry_hooks;

   memfile->size = size;
   memfile->pos = 0;
   memfile->mem = mem;

   return (ALLEGRO_FS_ENTRY *)memfile;
}

/* vim: set sts=3 sw=3 et: */
