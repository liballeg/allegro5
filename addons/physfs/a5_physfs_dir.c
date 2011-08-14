/*
 *       Filesystem driver for the PhysFS addon.
 *
 *       By Elias Pschernig.
 */

#include <physfs.h>
#include "allegro5/allegro.h"
#include "allegro5/allegro_physfs.h"

#include "allegro_physfs_intern.h"

#if defined(ENOTSUP)
   #define NOTSUP ENOTSUP
#elif defined(ENOSYS)
   #define NOTSUP ENOSYS
#else
   #define NOTSUP EINVAL
#endif


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

static ALLEGRO_FS_ENTRY *fs_phys_create_entry(const char *path)
{
   ALLEGRO_FS_ENTRY_PHYSFS *e;
   e = al_calloc(1, sizeof *e);
   e->fs_entry.vtable = &fs_phys_vtable;
   e->path = al_create_path(path);
   e->path_cstr = al_path_cstr(e->path, '/');
   return &e->fs_entry;
}

static char *fs_phys_get_current_directory(void)
{
   char *s = al_malloc(2);
   if (s) {
      s[0] = '/';
      s[1] = '\0';
   }
   return s;
}

static bool fs_phys_change_directory(const char *path)
{
   (void)path;
   al_set_errno(NOTSUP);
   return false;
}

static bool fs_phys_filename_exists(const char *path)
{
   return PHYSFS_exists(path) != 0;
}

static bool fs_phys_remove_filename(const char *path)
{
   return PHYSFS_delete(path) != 0;
}

static bool fs_phys_make_directory(const char *path)
{
   return PHYSFS_mkdir(path) != 0;
}

static const char *fs_phys_entry_name(ALLEGRO_FS_ENTRY *fse)
{
   ALLEGRO_FS_ENTRY_PHYSFS *e = (ALLEGRO_FS_ENTRY_PHYSFS *)fse;
   return al_path_cstr(e->path, '/');
}

static bool fs_phys_update_entry(ALLEGRO_FS_ENTRY *fse)
{
   (void)fse;
   return true;
}

static off_t fs_phys_entry_size(ALLEGRO_FS_ENTRY *fse)
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

static uint32_t fs_phys_entry_mode(ALLEGRO_FS_ENTRY *fse)
{
   ALLEGRO_FS_ENTRY_PHYSFS *e = (ALLEGRO_FS_ENTRY_PHYSFS *)fse;
   uint32_t mode = ALLEGRO_FILEMODE_READ;
   if (PHYSFS_isDirectory(e->path_cstr))
      mode |= ALLEGRO_FILEMODE_ISDIR | ALLEGRO_FILEMODE_EXECUTE;
   else
      mode |= ALLEGRO_FILEMODE_ISFILE;
   return mode;
}

static time_t fs_phys_entry_mtime(ALLEGRO_FS_ENTRY *fse)
{
   ALLEGRO_FS_ENTRY_PHYSFS *e = (ALLEGRO_FS_ENTRY_PHYSFS *)fse;
   return PHYSFS_getLastModTime(e->path_cstr);
}

static bool fs_phys_entry_exists(ALLEGRO_FS_ENTRY *fse)
{
   ALLEGRO_FS_ENTRY_PHYSFS *e = (ALLEGRO_FS_ENTRY_PHYSFS *)fse;
   return PHYSFS_exists(e->path_cstr) != 0;
}

static bool fs_phys_remove_entry(ALLEGRO_FS_ENTRY *fse)
{
   ALLEGRO_FS_ENTRY_PHYSFS *e = (ALLEGRO_FS_ENTRY_PHYSFS *)fse;
   return PHYSFS_delete(e->path_cstr) != 0;
}

static bool fs_phys_open_directory(ALLEGRO_FS_ENTRY *fse)
{
   ALLEGRO_FS_ENTRY_PHYSFS *e = (ALLEGRO_FS_ENTRY_PHYSFS *)fse;

   e->file_list = PHYSFS_enumerateFiles(e->path_cstr);
   e->file_list_pos = e->file_list;

   e->is_dir_open = true;
   return true;
}

static ALLEGRO_FS_ENTRY *fs_phys_read_directory(ALLEGRO_FS_ENTRY *fse)
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
   next = fs_phys_create_entry(al_cstr(tmp));
   al_ustr_free(tmp);

   e->file_list_pos++;

   return next;
}

static bool fs_phys_close_directory(ALLEGRO_FS_ENTRY *fse)
{
   ALLEGRO_FS_ENTRY_PHYSFS *e = (ALLEGRO_FS_ENTRY_PHYSFS *)fse;
   PHYSFS_freeList(e->file_list);
   e->file_list = NULL;
   e->is_dir_open = false;
   return true;
}

static void fs_phys_destroy_entry(ALLEGRO_FS_ENTRY *fse)
{
   ALLEGRO_FS_ENTRY_PHYSFS *e = (ALLEGRO_FS_ENTRY_PHYSFS *)fse;
   if (e->is_dir_open)
      fs_phys_close_directory(fse);
   al_destroy_path(e->path);
   al_free(e);
}

static ALLEGRO_FILE *fs_phys_open_file(ALLEGRO_FS_ENTRY *fse, const char *mode)
{
   return al_fopen_interface(_al_get_phys_vtable(), fs_phys_entry_name(fse),
      mode);
}

static const ALLEGRO_FS_INTERFACE fs_phys_vtable =
{
   fs_phys_create_entry,
   fs_phys_destroy_entry,
   fs_phys_entry_name,
   fs_phys_update_entry,
   fs_phys_entry_mode,
   fs_phys_entry_mtime,
   fs_phys_entry_mtime,
   fs_phys_entry_mtime,
   fs_phys_entry_size,
   fs_phys_entry_exists,
   fs_phys_remove_entry,

   fs_phys_open_directory,
   fs_phys_read_directory,
   fs_phys_close_directory,

   fs_phys_filename_exists,
   fs_phys_remove_filename,
   fs_phys_get_current_directory,
   fs_phys_change_directory,
   fs_phys_make_directory,

   fs_phys_open_file
};

void _al_set_physfs_fs_interface(void)
{
   al_set_fs_interface(&fs_phys_vtable);
}

/* vim: set sts=3 sw=3 et: */
