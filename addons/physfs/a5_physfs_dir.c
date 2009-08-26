/*
 *       Filesystem driver for the PhysFS addon.
 *
 *       By Elias Pschernig.
 */

#include <physfs.h>
#include "allegro5/allegro5.h"
#include "allegro5/allegro_physfs.h"
#include "allegro5/internal/aintern_memory.h"

#include "allegro_physfs_intern.h"


typedef struct ALLEGRO_FS_ENTRY_PHYSFS ALLEGRO_FS_ENTRY_PHYSFS;

struct ALLEGRO_FS_ENTRY_PHYSFS
{
   ALLEGRO_FS_ENTRY fs_entry; /* must be first */
   ALLEGRO_PATH *path;
   const char *path_cstr;

   /* For directory listing. */
   char **file_list;
   char **file_list_pos;
   bool is_dir_open;
};

/* forward declaration */
static const ALLEGRO_FS_INTERFACE fs_phys_vtable;

static ALLEGRO_FS_ENTRY *fs_phys_create(const char *path)
{
   ALLEGRO_FS_ENTRY_PHYSFS *e;
   e = _AL_MALLOC(sizeof *e);
   memset(e, 0, sizeof *e);
   e->fs_entry.vtable = &fs_phys_vtable;
   e->path = al_create_path(path);
   e->path_cstr = al_path_cstr(e->path, '/');
   return &e->fs_entry;
}

static ALLEGRO_PATH *fs_phys_getcwd(void)
{
   return NULL;
}

static bool fs_phys_chdir(const char *path)
{
   (void)path;
   return false;
}

static bool fs_phys_exists_str(const char *path)
{
   return PHYSFS_exists(path) != 0;
}

static bool fs_phys_remove_str(const char *path)
{
   return PHYSFS_delete(path) != 0;
}

static bool fs_phys_mkdir(const char *path)
{
   return PHYSFS_mkdir(path) != 0;
}

static const ALLEGRO_PATH *fs_phys_fname(ALLEGRO_FS_ENTRY *fse)
{
   ALLEGRO_FS_ENTRY_PHYSFS *e = (ALLEGRO_FS_ENTRY_PHYSFS *)fse;
   return e->path;
}

static bool fs_phys_fstat(ALLEGRO_FS_ENTRY *fse)
{
   (void)fse;
   return true;
}

static off_t fs_phys_size(ALLEGRO_FS_ENTRY *fse)
{
   ALLEGRO_FS_ENTRY_PHYSFS *e = (ALLEGRO_FS_ENTRY_PHYSFS *)fse;
   PHYSFS_file *f = PHYSFS_openRead(e->path_cstr);
   if (f) {
      off_t s = PHYSFS_fileLength(f);
      PHYSFS_close(f);
      return s;
   }
   return 0;
}

static uint32_t fs_phys_mode(ALLEGRO_FS_ENTRY *fse)
{
   ALLEGRO_FS_ENTRY_PHYSFS *e = (ALLEGRO_FS_ENTRY_PHYSFS *)fse;
   uint32_t mode = ALLEGRO_FILEMODE_READ;
   if (PHYSFS_isDirectory(e->path_cstr))
      mode |= ALLEGRO_FILEMODE_ISDIR | ALLEGRO_FILEMODE_EXECUTE;
   else
      mode |= ALLEGRO_FILEMODE_ISFILE;
   return mode;
}

static time_t fs_phys_mtime(ALLEGRO_FS_ENTRY *fse)
{
   ALLEGRO_FS_ENTRY_PHYSFS *e = (ALLEGRO_FS_ENTRY_PHYSFS *)fse;
   return PHYSFS_getLastModTime(e->path_cstr);
}

static bool fs_phys_exists(ALLEGRO_FS_ENTRY *fse)
{
   ALLEGRO_FS_ENTRY_PHYSFS *e = (ALLEGRO_FS_ENTRY_PHYSFS *)fse;
   return PHYSFS_exists(e->path_cstr) != 0;
}

static bool fs_phys_remove(ALLEGRO_FS_ENTRY *fse)
{
   ALLEGRO_FS_ENTRY_PHYSFS *e = (ALLEGRO_FS_ENTRY_PHYSFS *)fse;
   return PHYSFS_delete(e->path_cstr) != 0;
}

static bool fs_phys_opendir(ALLEGRO_FS_ENTRY *fse)
{
   ALLEGRO_FS_ENTRY_PHYSFS *e = (ALLEGRO_FS_ENTRY_PHYSFS *)fse;

   e->file_list = PHYSFS_enumerateFiles(e->path_cstr);
   e->file_list_pos = e->file_list;

   e->is_dir_open = true;
   return true;
}

static ALLEGRO_FS_ENTRY *fs_phys_readdir(ALLEGRO_FS_ENTRY *fse)
{
   ALLEGRO_FS_ENTRY_PHYSFS *e = (ALLEGRO_FS_ENTRY_PHYSFS *)fse;
   ALLEGRO_FS_ENTRY *next;
   ALLEGRO_USTR *tmp;

   if (!e->file_list_pos)
      return NULL;
   if (!*e->file_list_pos)
      return NULL;

   tmp = al_ustr_new(e->path_cstr);
   if (al_ustr_length(tmp) > 0)
      al_ustr_append_chr(tmp, '/');
   al_ustr_append_cstr(tmp, *e->file_list_pos);
   next = fs_phys_create(al_cstr(tmp));
   al_ustr_free(tmp);

   e->file_list_pos++;

   return next;
}

static bool fs_phys_closedir(ALLEGRO_FS_ENTRY *fse)
{
   ALLEGRO_FS_ENTRY_PHYSFS *e = (ALLEGRO_FS_ENTRY_PHYSFS *)fse;
   PHYSFS_freeList(e->file_list);
   e->file_list = NULL;
   e->is_dir_open = false;
   return true;
}

static void fs_phys_destroy(ALLEGRO_FS_ENTRY *fse)
{
   ALLEGRO_FS_ENTRY_PHYSFS *e = (ALLEGRO_FS_ENTRY_PHYSFS *)fse;
   if (e->is_dir_open)
      fs_phys_closedir(fse);
   al_free_path(e->path);
   _AL_FREE(e);
}

static const ALLEGRO_FS_INTERFACE fs_phys_vtable =
{
   fs_phys_create,
   fs_phys_getcwd,
   fs_phys_chdir,
   fs_phys_exists_str,
   fs_phys_remove_str,
   fs_phys_mkdir,

   fs_phys_destroy,
   fs_phys_fname,
   fs_phys_fstat,

   fs_phys_size,
   fs_phys_mode,
   fs_phys_mtime,
   fs_phys_mtime,
   fs_phys_mtime,

   fs_phys_exists,
   fs_phys_remove,

   fs_phys_opendir,
   fs_phys_readdir,
   fs_phys_closedir
};

void _al_set_physfs_fs_interface(void)
{
   al_set_fs_interface(&fs_phys_vtable);
}

/* vim: set sts=3 sw=3 et: */
