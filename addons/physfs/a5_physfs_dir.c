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


typedef struct A5O_FS_ENTRY_PHYSFS A5O_FS_ENTRY_PHYSFS;

struct A5O_FS_ENTRY_PHYSFS
{
   A5O_FS_ENTRY fs_entry; /* must be first */
   A5O_PATH *path;
   const char *path_cstr;

   /* For directory listing. */
   char **file_list;
   char **file_list_pos;
   bool is_dir_open;
};

/* forward declaration */
static const A5O_FS_INTERFACE fs_phys_vtable;

/* current working directory */
/* We cannot use A5O_USTR because we have nowhere to free it. */
static char fs_phys_cwd[1024] = "/";

static bool path_is_absolute(const char *path)
{
   return (path && path[0] == '/');
}

static void ensure_trailing_slash(A5O_USTR *us)
{
   int pos = al_ustr_size(us);
   if (al_ustr_prev_get(us, &pos) != '/') {
      al_ustr_append_chr(us, '/');
   }
}

A5O_USTR *_al_physfs_process_path(const char *path)
{
   A5O_USTR *us;
   A5O_PATH *p = al_create_path(path);
   A5O_PATH *cwd = al_create_path(fs_phys_cwd);
   al_rebase_path(cwd, p);
   al_destroy_path(cwd);
   /* PHYSFS always uses / separator. */
   us = al_ustr_dup(al_path_ustr(p, '/'));
   al_destroy_path(p);
   return us;
}

static A5O_FS_ENTRY *fs_phys_create_entry(const char *path)
{
   A5O_FS_ENTRY_PHYSFS *e;
   A5O_USTR *us;

   e = al_calloc(1, sizeof *e);
   if (!e)
      return NULL;
   e->fs_entry.vtable = &fs_phys_vtable;

   us = _al_physfs_process_path(path);
   e->path = al_create_path(al_cstr(us));
   al_ustr_free(us);
   if (!e->path) {
      al_free(e);
      return NULL;
   }
   e->path_cstr = al_path_cstr(e->path, '/');
   return &e->fs_entry;
}

static char *fs_phys_get_current_directory(void)
{
   size_t size = strlen(fs_phys_cwd) + 1;
   char *s = al_malloc(size);
   if (s) {
      memcpy(s, fs_phys_cwd, size);
   }
   return s;
}

static bool fs_phys_change_directory(const char *path)
{
   A5O_USTR *us;
   bool ret;
   PHYSFS_Stat stat;

   /* '/' root is guaranteed to exist but PHYSFS won't find it. */
   if (path[0] == '/' && path[1] == '\0') {
      fs_phys_cwd[0] = '/';
      fs_phys_cwd[1] = '\0';
      return true;
   }

   /* Figure out which directory we are trying to change to. */
   if (path_is_absolute(path))
      us = al_ustr_new(path);
   else
      us = _al_physfs_process_path(path);

   ret = false;

   if (!PHYSFS_stat(al_cstr(us), &stat))
      return false;

   if (stat.filetype == PHYSFS_FILETYPE_DIRECTORY ) {
      ensure_trailing_slash(us);

      /* Copy to static buffer. */
      if ((size_t) al_ustr_size(us) < sizeof(fs_phys_cwd)) {
         al_ustr_to_buffer(us, fs_phys_cwd, sizeof(fs_phys_cwd));
         ret = true;
      }
   }

   al_ustr_free(us);

   return ret;
}

static bool fs_phys_filename_exists(const char *path)
{
   A5O_USTR *us;
   bool ret;

   us = _al_physfs_process_path(path);
   ret = PHYSFS_exists(al_cstr(us)) ? true : false;
   al_ustr_free(us);
   return ret;
}

static bool fs_phys_remove_filename(const char *path)
{
   A5O_USTR *us;
   bool ret;

   us = _al_physfs_process_path(path);
   ret = PHYSFS_delete(al_cstr(us)) ? true : false;
   al_ustr_free(us);
   return ret;
}

static bool fs_phys_make_directory(const char *path)
{
   A5O_USTR *us;
   bool ret;

   us = _al_physfs_process_path(path);
   ret = PHYSFS_mkdir(al_cstr(us)) ? true : false;
   al_ustr_free(us);
   return ret;
}

static const char *fs_phys_entry_name(A5O_FS_ENTRY *fse)
{
   A5O_FS_ENTRY_PHYSFS *e = (A5O_FS_ENTRY_PHYSFS *)fse;
   return al_path_cstr(e->path, '/');
}

static bool fs_phys_update_entry(A5O_FS_ENTRY *fse)
{
   (void)fse;
   return true;
}

static off_t fs_phys_entry_size(A5O_FS_ENTRY *fse)
{
   A5O_FS_ENTRY_PHYSFS *e = (A5O_FS_ENTRY_PHYSFS *)fse;
   PHYSFS_file *f = PHYSFS_openRead(e->path_cstr);
   if (f) {
      off_t s = PHYSFS_fileLength(f);
      PHYSFS_close(f);
      return s;
   }
   return 0;
}

static uint32_t fs_phys_entry_mode(A5O_FS_ENTRY *fse)
{
   A5O_FS_ENTRY_PHYSFS *e = (A5O_FS_ENTRY_PHYSFS *)fse;
   uint32_t mode = A5O_FILEMODE_READ;
   PHYSFS_Stat stat;
   if (!PHYSFS_stat(e->path_cstr, &stat))
      return mode;

   if (stat.filetype == PHYSFS_FILETYPE_DIRECTORY )
      mode |= A5O_FILEMODE_ISDIR | A5O_FILEMODE_EXECUTE;
   else
      mode |= A5O_FILEMODE_ISFILE;
   return mode;
}

static time_t fs_phys_entry_mtime(A5O_FS_ENTRY *fse)
{
   A5O_FS_ENTRY_PHYSFS *e = (A5O_FS_ENTRY_PHYSFS *)fse;
   PHYSFS_Stat stat;
   if (!PHYSFS_stat(e->path_cstr, &stat))
      return -1;
   return stat.modtime;
}

static bool fs_phys_entry_exists(A5O_FS_ENTRY *fse)
{
   A5O_FS_ENTRY_PHYSFS *e = (A5O_FS_ENTRY_PHYSFS *)fse;
   return PHYSFS_exists(e->path_cstr) != 0;
}

static bool fs_phys_remove_entry(A5O_FS_ENTRY *fse)
{
   A5O_FS_ENTRY_PHYSFS *e = (A5O_FS_ENTRY_PHYSFS *)fse;
   return PHYSFS_delete(e->path_cstr) != 0;
}

static bool fs_phys_open_directory(A5O_FS_ENTRY *fse)
{
   A5O_FS_ENTRY_PHYSFS *e = (A5O_FS_ENTRY_PHYSFS *)fse;

   e->file_list = PHYSFS_enumerateFiles(e->path_cstr);
   e->file_list_pos = e->file_list;

   e->is_dir_open = true;
   return true;
}

static A5O_FS_ENTRY *fs_phys_read_directory(A5O_FS_ENTRY *fse)
{
   A5O_FS_ENTRY_PHYSFS *e = (A5O_FS_ENTRY_PHYSFS *)fse;
   A5O_FS_ENTRY *next;
   A5O_USTR *tmp;

   if (!e->file_list_pos)
      return NULL;
   if (!*e->file_list_pos)
      return NULL;

   tmp = al_ustr_new(e->path_cstr);
   ensure_trailing_slash(tmp);
   al_ustr_append_cstr(tmp, *e->file_list_pos);
   next = fs_phys_create_entry(al_cstr(tmp));
   al_ustr_free(tmp);

   e->file_list_pos++;

   return next;
}

static bool fs_phys_close_directory(A5O_FS_ENTRY *fse)
{
   A5O_FS_ENTRY_PHYSFS *e = (A5O_FS_ENTRY_PHYSFS *)fse;
   PHYSFS_freeList(e->file_list);
   e->file_list = NULL;
   e->is_dir_open = false;
   return true;
}

static void fs_phys_destroy_entry(A5O_FS_ENTRY *fse)
{
   A5O_FS_ENTRY_PHYSFS *e = (A5O_FS_ENTRY_PHYSFS *)fse;
   if (e->is_dir_open)
      fs_phys_close_directory(fse);
   al_destroy_path(e->path);
   al_free(e);
}

static A5O_FILE *fs_phys_open_file(A5O_FS_ENTRY *fse, const char *mode)
{
   return al_fopen_interface(_al_get_phys_vtable(), fs_phys_entry_name(fse),
      mode);
}

static const A5O_FS_INTERFACE fs_phys_vtable =
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
