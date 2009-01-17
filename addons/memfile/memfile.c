#define ALLEGRO_LIB_BUILD 1
#include <allegro5/allegro5.h>
#include "allegro5/memfile.h"
#include "allegro5/internal/aintern_memory.h"
#include "allegro5/internal/aintern_fshook.h"
#include "allegro5/internal/aintern_memfile.h"
#include <time.h>
#include <stdio.h>

void al_memfile_destroy_handle(ALLEGRO_FS_ENTRY *handle)
{
   _AL_FREE(handle);
}

bool al_memfile_fname(ALLEGRO_FS_ENTRY *fh, size_t s, char *name)
{
   (void)fh;
   ustrzcpy(name, s, "<memfile>");
   return true;
}

void al_memfile_fclose(ALLEGRO_FS_ENTRY *fp)
{
   _AL_FREE(fp);
}

size_t al_memfile_fread(ALLEGRO_FS_ENTRY *fp, size_t size, void *ptr)
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

size_t al_memfile_fwrite(ALLEGRO_FS_ENTRY *fp, size_t size, AL_CONST void *ptr)
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

bool al_memfile_fflush(ALLEGRO_FS_ENTRY *fp)
{
   (void)fp;
   return true;
}

bool al_memfile_fseek(ALLEGRO_FS_ENTRY *fp, int64_t offset, uint32_t whence)
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

int64_t al_memfile_ftell(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_MEMFILE *mf = (ALLEGRO_FS_ENTRY_MEMFILE*)fp;

   return mf->pos;
}

bool al_memfile_ferror(ALLEGRO_FS_ENTRY *fp)
{
   (void)fp;
   return false;
}

/* doesn't quite match up to stdio here,
   an feof after a seek will return false,
   even if it seeks past the end of the file */
bool al_memfile_feof(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_MEMFILE *mf = (ALLEGRO_FS_ENTRY_MEMFILE*)fp;
   return mf->eof;
}

bool al_memfile_fstat(ALLEGRO_FS_ENTRY *fp)
{
   (void)fp;
   return true;
}

int al_memfile_ungetc(int c, ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_MEMFILE *mf = (ALLEGRO_FS_ENTRY_MEMFILE*)fp;

   mf->ungetc = (unsigned char)c;

   return c;
}

off_t al_memfile_entry_size(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_MEMFILE *mf = (ALLEGRO_FS_ENTRY_MEMFILE*)fp;

   return mf->size;
}

uint32_t al_memfile_entry_mode(ALLEGRO_FS_ENTRY *fp)
{
   (void)fp;
   return AL_FM_READ | AL_FM_WRITE | AL_FM_ISFILE;
}

time_t al_memfile_entry_atime(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_MEMFILE *mf = (ALLEGRO_FS_ENTRY_MEMFILE*)fp;
   return mf->atime;
}

time_t al_memfile_entry_mtime(ALLEGRO_FS_ENTRY *fp)
{
   ALLEGRO_FS_ENTRY_MEMFILE *mf = (ALLEGRO_FS_ENTRY_MEMFILE*)fp;
   return mf->mtime;
}

time_t al_memfile_entry_ctime(ALLEGRO_FS_ENTRY *fp)
{
   (void)fp;
   return 0;
}

bool al_memfile_exists(ALLEGRO_FS_ENTRY *fp)
{
   (void)fp;
   return true;
}

bool al_memfile_unlink(ALLEGRO_FS_ENTRY *fp)
{
   (void)fp;
   return true;
}

static struct ALLEGRO_FS_HOOK_ENTRY_INTERFACE _al_memfile_entry_hooks = {
   al_memfile_destroy_handle,
   NULL,
   NULL,

   al_memfile_fname,

   al_memfile_fclose,
   al_memfile_fread,
   al_memfile_fwrite,
   al_memfile_fflush,
   al_memfile_fseek,
   al_memfile_ftell,
   al_memfile_ferror,
   al_memfile_feof,
   al_memfile_fstat,
   al_memfile_ungetc,

   al_memfile_entry_size,
   al_memfile_entry_mode,
   al_memfile_entry_atime,
   al_memfile_entry_mtime,
   NULL,

   NULL,
   NULL,

   NULL,
   NULL
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

   memfile->fs_entry.vtable = &_al_memfile_entry_hooks;

   memfile->size = size;
   memfile->pos = 0;
   memfile->mem = mem;

   return (ALLEGRO_FS_ENTRY *)memfile;
}
