/*
 *       Filesystem driver for APK contents.
 *
 *       By Elias Pschernig.
 */


#include "allegro5/allegro.h"
#include "allegro5/allegro_android.h"
#include "allegro5/internal/aintern_android.h"

#include <jni.h>

A5O_DEBUG_CHANNEL("android")

typedef struct A5O_FS_ENTRY_APK A5O_FS_ENTRY_APK;

struct A5O_FS_ENTRY_APK
{
   A5O_FS_ENTRY fs_entry; /* must be first */
   A5O_PATH *path;
   const char *path_cstr;

   /* For directory listing. */
   char *file_list;
   char *file_list_pos;
   bool is_dir_open;
};

/* forward declaration */
static const A5O_FS_INTERFACE fs_apk_vtable;

/* current working directory */
/* TODO: free this somewhere */
static A5O_USTR *fs_apk_cwd_ustr;

static A5O_FILE *fs_apk_open_file(A5O_FS_ENTRY *fse, const char *mode);

static A5O_USTR *get_fake_cwd(void) {
   if (!fs_apk_cwd_ustr) {
      fs_apk_cwd_ustr = al_ustr_new("/");
   }
   return fs_apk_cwd_ustr;
}

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

static A5O_USTR *apply_cwd(const char *path)
{
   A5O_USTR *us;

   if (path_is_absolute(path)) {
      return al_ustr_new(path);
   }

   us = al_ustr_dup(get_fake_cwd());
   al_ustr_append_cstr(us, path);
   return us;
}

static A5O_FS_ENTRY *fs_apk_create_entry(const char *path)
{
   A5O_FS_ENTRY_APK *e;
   A5O_USTR *us;

   e = al_calloc(1, sizeof *e);
   if (!e)
      return NULL;
   e->fs_entry.vtable = &fs_apk_vtable;

   us = apply_cwd(path);
   e->path = al_create_path(al_cstr(us));
   al_ustr_free(us);
   if (!e->path) {
      al_free(e);
      return NULL;
   }
   e->path_cstr = al_path_cstr(e->path, '/');
   
   return &e->fs_entry;
}

static char *fs_apk_get_current_directory(void)
{
   return al_cstr_dup(get_fake_cwd());
}

static bool fs_apk_change_directory(const char *path)
{
   A5O_USTR *us;
   A5O_USTR *cwd = get_fake_cwd();
   
   /* Figure out which directory we are trying to change to. */
   if (path_is_absolute(path))
      us = al_ustr_new(path);
   else
      us = apply_cwd(path);

   ensure_trailing_slash(us);

   al_ustr_assign(cwd, us);

   al_ustr_free(us);

   return true;
}

static bool fs_apk_remove_filename(const char *path)
{
   (void)path;
   return false;
}

static bool fs_apk_make_directory(const char *path)
{
   (void)path;
   return false;
}

static const char *fs_apk_entry_name(A5O_FS_ENTRY *fse)
{
   A5O_FS_ENTRY_APK *e = (A5O_FS_ENTRY_APK *)fse;
   return al_path_cstr(e->path, '/');
}

static bool fs_apk_update_entry(A5O_FS_ENTRY *fse)
{
   (void)fse;
   return true;
}

static off_t fs_apk_entry_size(A5O_FS_ENTRY *fse)
{
   // Only way to determine the size would be to read the file...
   // we won't do that.
   (void)fse;
   return 0;
}

static uint32_t fs_apk_entry_mode(A5O_FS_ENTRY *fse)
{
   A5O_FS_ENTRY_APK *e = (A5O_FS_ENTRY_APK *)fse;
   uint32_t mode = A5O_FILEMODE_READ;
   int n = strlen(e->path_cstr);
   if (e->path_cstr[n - 1] == '/')
      mode |= A5O_FILEMODE_ISDIR | A5O_FILEMODE_EXECUTE;
   else
      mode |= A5O_FILEMODE_ISFILE;
   return mode;
}

static time_t fs_apk_entry_mtime(A5O_FS_ENTRY *fse)
{
   (void)fse;
   return 0;
}

static bool fs_apk_entry_exists(A5O_FS_ENTRY *fse)
{
   A5O_FILE *f = fs_apk_open_file(fse, "r");
   if (f) {
      al_fclose(f);
      return true;
   }
   return false;
}

static bool fs_apk_remove_entry(A5O_FS_ENTRY *fse)
{
   (void)fse;
   return false;
}

static bool fs_apk_open_directory(A5O_FS_ENTRY *fse)
{
   A5O_FS_ENTRY_APK *e = (A5O_FS_ENTRY_APK *)fse;

   JNIEnv *jnienv;
   jnienv = _al_android_get_jnienv();
   jstring jpath = (*jnienv)->NewStringUTF(jnienv, e->path_cstr);
   jstring js = _jni_callStaticObjectMethodV(jnienv,
      _al_android_apk_fs_class(), "list",
      "(L" A5O_ANDROID_PACKAGE_NAME_SLASH "/AllegroActivity;Ljava/lang/String;)Ljava/lang/String;",
      _al_android_activity_object(), jpath);

   const char *cs = _jni_call(jnienv, const char *, GetStringUTFChars, js, NULL);
   e->file_list = al_malloc(strlen(cs) + 1);
   strcpy(e->file_list, cs);
   e->file_list_pos = e->file_list;

   _jni_callv(jnienv, ReleaseStringUTFChars, js, cs);
   _jni_callv(jnienv, DeleteLocalRef, js);
   _jni_callv(jnienv, DeleteLocalRef, jpath);

   e->is_dir_open = true;
   return true;
}

static A5O_FS_ENTRY *fs_apk_read_directory(A5O_FS_ENTRY *fse)
{
   A5O_FS_ENTRY_APK *e = (A5O_FS_ENTRY_APK *)fse;
   A5O_FS_ENTRY *next;
   A5O_USTR *tmp;

   if (!e->file_list_pos)
      return NULL;
   if (!*e->file_list_pos)
      return NULL;

   tmp = al_ustr_new(e->path_cstr);
   ensure_trailing_slash(tmp);
   char *name = e->file_list_pos;
   char *semi = strchr(name, ';');
   if (semi) {
      *semi = 0;
      e->file_list_pos = semi + 1;
   }
   else {
      e->file_list_pos = name + strlen(name);
   }
   al_ustr_append_cstr(tmp, name);
   next = fs_apk_create_entry(al_cstr(tmp));
   al_ustr_free(tmp);

   return next;
}

static bool fs_apk_close_directory(A5O_FS_ENTRY *fse)
{
   A5O_FS_ENTRY_APK *e = (A5O_FS_ENTRY_APK *)fse;
   al_free(e->file_list);
   e->file_list = NULL;
   e->is_dir_open = false;
   return true;
}

static void fs_apk_destroy_entry(A5O_FS_ENTRY *fse)
{
   A5O_FS_ENTRY_APK *e = (A5O_FS_ENTRY_APK *)fse;
   if (e->is_dir_open)
      fs_apk_close_directory(fse);
   al_destroy_path(e->path);
   al_free(e);
}

static A5O_FILE *fs_apk_open_file(A5O_FS_ENTRY *fse, const char *mode)
{
   return al_fopen_interface(_al_get_apk_file_vtable(), fs_apk_entry_name(fse),
      mode);
}

static bool fs_apk_filename_exists(const char *path)
{
   bool ret;

   A5O_FS_ENTRY *e = fs_apk_create_entry(path);
   ret = fs_apk_entry_exists(e);
   fs_apk_destroy_entry(e);
   return ret;
}

static const A5O_FS_INTERFACE fs_apk_vtable =
{
   fs_apk_create_entry,
   fs_apk_destroy_entry,
   fs_apk_entry_name,
   fs_apk_update_entry,
   fs_apk_entry_mode,
   fs_apk_entry_mtime,
   fs_apk_entry_mtime,
   fs_apk_entry_mtime,
   fs_apk_entry_size,
   fs_apk_entry_exists,
   fs_apk_remove_entry,

   fs_apk_open_directory,
   fs_apk_read_directory,
   fs_apk_close_directory,

   fs_apk_filename_exists,
   fs_apk_remove_filename,
   fs_apk_get_current_directory,
   fs_apk_change_directory,
   fs_apk_make_directory,

   fs_apk_open_file
};

/* Function: al_android_set_apk_fs_interface
 */
void al_android_set_apk_fs_interface(void)
{
   al_set_fs_interface(&fs_apk_vtable);
}

/* vim: set sts=3 sw=3 et: */

