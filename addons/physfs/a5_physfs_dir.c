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

/* current working directory */
static char fs_phys_cwd[__FS_PHYS_CWD_MAX] = {'/'};

static bool fs_phys_path_is_absolute(const char * path)
{
   if (path && path[0] == '/')
      return true;
   return false;
}

/* returns a new ALLEGRO_PATH structure containing the specified path, taking
 * into account the current working directory.
 */
const char * _al_fs_phys_get_path_with_cwd(const char * path, char * buffer)
{
   ALLEGRO_USTR * final_path;

   /* returned as-is if path is absolute or cwd not set */
   if (fs_phys_path_is_absolute(path))
      return path;

   /* start with fs_phys_cwd */
   final_path = al_ustr_new(fs_phys_cwd);
   if (!final_path) {
	   return NULL;
   }
   /* append our relative path to the CWD */
   al_ustr_append_cstr(final_path, path);

   /* copy the data to the internal buffer */
   al_ustr_to_buffer(final_path, buffer, __FS_PHYS_CWD_MAX);

   al_ustr_free(final_path);

   return buffer;
}

static ALLEGRO_FS_ENTRY *fs_phys_create_entry(const char *path)
{
   char combined_path[__FS_PHYS_CWD_MAX];
   ALLEGRO_FS_ENTRY_PHYSFS *e;
   e = al_calloc(1, sizeof *e);
   if (!e)
      return NULL;
   e->fs_entry.vtable = &fs_phys_vtable;

   e->path = al_create_path(_al_fs_phys_get_path_with_cwd(path, combined_path));
   if (!e->path) {
      al_free(e);
      return NULL;
   }
   e->path_cstr = al_path_cstr(e->path, '/');
   return &e->fs_entry;
}

static char *fs_phys_get_current_directory(void)
{
   const ALLEGRO_USTR * ustr;
   ALLEGRO_USTR_INFO info;
   char * current_dir = NULL;
   int size;

   ustr = al_ref_cstr(&info, fs_phys_cwd);
   size = al_ustr_size(ustr);
   current_dir = al_malloc(size);
   if (!current_dir)
      return NULL;
   memcpy(current_dir, fs_phys_cwd, size);
   return current_dir;
}

static bool fs_phys_change_directory(const char *path)
{
   char combined_path[__FS_PHYS_CWD_MAX];
   const ALLEGRO_USTR * ustr;
   ALLEGRO_USTR * ustr2;
   ALLEGRO_USTR_INFO info;
   const char * apath;
   bool is_absolute;

   /* figure out which directory we are trying to change to */
   is_absolute = fs_phys_path_is_absolute(path);
   if (is_absolute)
      apath = path;
   else
      apath = _al_fs_phys_get_path_with_cwd(path, combined_path);

   ustr = al_ref_cstr(&info, apath);

   /* '/' root is gauranteed to exist and PHYSFS won't find it */
   if (al_ustr_length(ustr) < 2 && al_ustr_get(ustr, 0) == '/') {
      fs_phys_cwd[0] = '/';
      fs_phys_cwd[1] = '\0';
      return true;
   }
   /* if the path exists and is a directory, replace current working dir */
   else if(PHYSFS_exists(apath) && PHYSFS_isDirectory(apath)) {
      ustr2 = al_ustr_new(apath);
      if(!ustr2)
         return false;

      /* make sure current working directory always ends in '/' */
      if (al_ustr_get(ustr2, al_ustr_length(ustr2) - 1) != '/')
		  al_ustr_append_chr(ustr2, '/');

      /* copy ustr to static buffer */
      al_ustr_to_buffer(ustr2, fs_phys_cwd, __FS_PHYS_CWD_MAX);
      al_ustr_free(ustr2);
      return true;
   }
   return false;
}

static bool fs_phys_filename_exists(const char *path)
{
   char combined_path[__FS_PHYS_CWD_MAX];
   if (PHYSFS_exists(_al_fs_phys_get_path_with_cwd(path, combined_path)))
      return true;
   return false;
}

static bool fs_phys_remove_filename(const char *path)
{
   char combined_path[__FS_PHYS_CWD_MAX];
   return  PHYSFS_delete(_al_fs_phys_get_path_with_cwd(path, combined_path)) != 0;
}

static bool fs_phys_make_directory(const char *path)
{
   char combined_path[__FS_PHYS_CWD_MAX];
   return PHYSFS_mkdir(_al_fs_phys_get_path_with_cwd(path, combined_path)) != 0;
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
   int len;

   if (!e->file_list_pos)
      return NULL;
   if (!*e->file_list_pos)
      return NULL;

   tmp = al_ustr_new(e->path_cstr);
   len = al_ustr_length(tmp);
   /* append '/' to path string if it doesn't already end with it */
   if (len > 0 && al_ustr_get(tmp, len - 1) != '/')
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
