/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      File System Hooks.
 *
 *      By Thomas Fjellstrom.
 *
 *      See readme.txt for copyright information.
 */

/* Title: Filesystem routines
*/

#include "allegro5/allegro5.h"
#include ALLEGRO_INTERNAL_HEADER
#include "allegro5/debug.h"
#include "allegro5/fshook.h"
#include "allegro5/path.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_fshook.h"
#include "allegro5/internal/aintern_memory.h"


struct ALLEGRO_FS_HOOK_SYS_INTERFACE *_al_sys_fshooks = &_al_stdio_sys_fshooks;
struct ALLEGRO_FS_HOOK_ENTRY_INTERFACE *_al_entry_fshooks = &_al_stdio_entry_fshooks;


/* Function: al_create_entry
 */
ALLEGRO_FS_ENTRY *al_create_entry(const char *path)
{
   return _al_fs_hook_create(path);
}


/* Function: al_destroy_entry
 */
void al_destroy_entry(ALLEGRO_FS_ENTRY *handle)
{
   if (handle) {
      _al_fs_hook_destroy(handle);
   }
}


/* Function: al_open_entry
 */
bool al_open_entry(ALLEGRO_FS_ENTRY *handle, const char *mode)
{
   ASSERT(handle != NULL);
   ASSERT(mode != NULL);

   return _al_fs_hook_open(handle, mode);
}


/* Function: al_close_entry
 */
void al_close_entry(ALLEGRO_FS_ENTRY *handle)
{
   if (handle) {
      _al_fs_hook_close(handle);
   }
}


/* Function: al_get_entry_name
 */
ALLEGRO_PATH *al_get_entry_name(ALLEGRO_FS_ENTRY *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_name(fp);
}


/* Function: al_fopen
 */
ALLEGRO_FS_ENTRY *al_fopen(const char *path, const char *mode)
{
   ASSERT(path != NULL);
   ASSERT(mode != NULL);

   return _al_fs_hook_entry_open(path, mode);
}


/* Function: al_fclose
 */
void al_fclose(ALLEGRO_FS_ENTRY *fp)
{
   ASSERT(fp != NULL);

   _al_fs_hook_entry_close(fp);
}


/* Function: al_fread
 */
size_t al_fread(ALLEGRO_FS_ENTRY *fp, void *ptr, size_t size)
{
   ASSERT(ptr != NULL);
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_read(fp, ptr, size);
}


/* Function: al_fwrite
 */
size_t al_fwrite(ALLEGRO_FS_ENTRY *fp, const void *ptr, size_t size)
{
   ASSERT(ptr != NULL);
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_write(fp, ptr, size);
}


/* Function: al_fflush
 */
bool al_fflush(ALLEGRO_FS_ENTRY *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_flush(fp);
}


/* Function: al_fseek
 */
bool al_fseek(ALLEGRO_FS_ENTRY *fp, int64_t offset, uint32_t whence)
{
   ASSERT(fp != NULL);
   ASSERT(offset >= 0);
   ASSERT(
      whence == ALLEGRO_SEEK_SET ||
      whence == ALLEGRO_SEEK_CUR ||
      whence == ALLEGRO_SEEK_END
   );

   return _al_fs_hook_entry_seek(fp, offset, whence);
}


/* Function: al_ftell
 */
int64_t al_ftell(ALLEGRO_FS_ENTRY *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_tell(fp);
}


/* Function: al_ferror
 */
bool al_ferror(ALLEGRO_FS_ENTRY *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_error(fp);
}


/* Function: al_feof
 */
bool al_feof(ALLEGRO_FS_ENTRY *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_eof(fp);
}


/* Function: al_fstat
 */
bool al_fstat(ALLEGRO_FS_ENTRY *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_stat(fp);
}


/* Function: al_opendir
 */
ALLEGRO_FS_ENTRY *al_opendir(const char *path)
{
   ALLEGRO_FS_ENTRY *dir = NULL;

   ASSERT(path != NULL);

   dir = _al_fs_hook_opendir(path);
   if (!dir)
      return NULL;

   return dir;
}


/* Function: al_closedir
 */
bool al_closedir(ALLEGRO_FS_ENTRY *dir)
{
   ASSERT(dir != NULL);

   return _al_fs_hook_closedir(dir);
}


/* Function: al_readdir
 */
ALLEGRO_FS_ENTRY *al_readdir(ALLEGRO_FS_ENTRY *dir)
{
   ASSERT(dir != NULL);

   return _al_fs_hook_readdir(dir);
}


/* Function: al_get_entry_mode
 */
uint32_t al_get_entry_mode(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return _al_fs_hook_entry_mode(e);
}


/* Function: al_get_entry_atime
 */
time_t al_get_entry_atime(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return _al_fs_hook_entry_atime(e);
}


/* Function: al_get_entry_mtime
 */
time_t al_get_entry_mtime(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return _al_fs_hook_entry_mtime(e);
}


/* Function: al_get_entry_ctime
 */
time_t al_get_entry_ctime(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return _al_fs_hook_entry_ctime(e);
}


/* Function: al_get_entry_size
 */
off_t al_get_entry_size(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);

   return _al_fs_hook_entry_size(e);
}


/* Function: al_remove_entry
 */
bool al_remove_entry(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);

   return _al_fs_hook_entry_remove(e);
}


/* Function: al_is_present
 */
bool al_is_present(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);

   return _al_fs_hook_entry_exists(e);
}


/* Function: al_is_directory
 */
bool al_is_directory(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return al_get_entry_mode(e) & ALLEGRO_FILEMODE_ISDIR;
}


/* Function: al_is_file
 */
bool al_is_file(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return al_get_entry_mode(e) & ALLEGRO_FILEMODE_ISFILE;
}


/* Function: al_mktemp
 */
ALLEGRO_FS_ENTRY *al_mktemp(const char *template, uint32_t ulink)
{
   ASSERT(template != NULL);
   ASSERT(
      ulink == ALLEGRO_FS_MKTEMP_REMOVE_NOW ||
      ulink == ALLEGRO_FS_MKTEMP_REMOVE_ON_CLOSE ||
      ulink == ALLEGRO_FS_MKTEMP_REMOVE_NEVER
   );

   return _al_fs_hook_mktemp(template, ulink);
}


/* Function: al_getcwd
 */
ALLEGRO_PATH *al_getcwd(void)
{
   return _al_fs_hook_getcwd();
}


/* Function: al_chdir
 */
bool al_chdir(const char *path)
{
   ASSERT(path);

   return _al_fs_hook_chdir(path);
}


/* Function: al_mkdir
 */
bool al_mkdir(const char *path)
{
   ASSERT(path);

   return _al_fs_hook_mkdir(path);
}


/* Function: al_add_search_path
 */
bool al_add_search_path(const char *path)
{
   ASSERT(path);

   return _al_fs_hook_add_search_path(path);
}


/* Function: al_search_path_count
 */
int32_t al_search_path_count()
{
   return _al_fs_hook_search_path_count();
}


/* Function: al_get_search_path
 */
bool al_get_search_path(uint32_t idx, char *dest, size_t len)
{
   ASSERT(dest);
   ASSERT(len);

   return _al_fs_hook_get_search_path(idx, dest, len);
}


/* Function: al_drive_sep
 */
int32_t al_drive_sep(char *sep, size_t len)
{
   ASSERT(len > 0);
   ASSERT(sep);

   return _al_fs_hook_drive_sep(sep, len);
}


/* Function: al_path_sep
 */
int32_t al_path_sep(char *sep, size_t len)
{
   ASSERT(len > 0);
   ASSERT(sep);

   return _al_fs_hook_path_sep(sep, len);
}


/* Function: al_get_entry_mode_str
 */
uint32_t al_get_entry_mode_str(const char *path)
{
   ASSERT(path != NULL);
   return _al_fs_hook_stat_mode(path);
}


/* Function: al_remove_str
 */
bool al_remove_str(const char *path)
{
   ASSERT(path != NULL);
   return _al_fs_hook_remove(path);
}


/* Function: al_is_present_str
 */
bool al_is_present_str(const char *path)
{
   ASSERT(path != NULL);

   return _al_fs_hook_exists(path);
}


/* Function: al_fgetc
 */
int al_fgetc(ALLEGRO_FS_ENTRY *f)
{
   uint8_t c;
   ASSERT(f);

   if (al_fread(f, &c, 1) != 1) {
      return EOF;
   }

   return c;
}


/* Function: al_fputc
 */
int al_fputc(ALLEGRO_FS_ENTRY *f, int c)
{
   ASSERT(f);

   if (al_fwrite(f, &c, 1) != 1) {
      return EOF;
   }

   return c;
}


/* Function: al_fread16le
 */
int16_t al_fread16le(ALLEGRO_FS_ENTRY *f)
{
   int b1, b2;
   ASSERT(f);

   if ((b1 = al_fgetc(f)) != EOF)
      if ((b2 = al_fgetc(f)) != EOF)
         return ((b2 << 8) | b1);

   return EOF;
}


/* Function: al_fread32le
 */
int32_t al_fread32le(ALLEGRO_FS_ENTRY *f, bool *ret_success)
{
   int b1, b2, b3, b4;
   ASSERT(f);

   if ((b1 = al_fgetc(f)) != EOF) {
      if ((b2 = al_fgetc(f)) != EOF) {
         if ((b3 = al_fgetc(f)) != EOF) {
            if ((b4 = al_fgetc(f)) != EOF) {
               if (ret_success)
                  *ret_success = true;
               return (((int32_t)b4 << 24) | ((int32_t)b3 << 16) |
                       ((int32_t)b2 << 8) | (int32_t)b1);
            }
         }
      }
   }

   if (ret_success)
      *ret_success = false;
   return EOF;
}


/* Function: al_fwrite16le
 */
size_t al_fwrite16le(ALLEGRO_FS_ENTRY *f, int16_t w)
{
   int8_t b1, b2;
   ASSERT(f);

   b1 = (w & 0xFF00) >> 8;
   b2 = w & 0x00FF;

   if (al_fputc(f, b2) == b2) {
      if (al_fputc(f, b1) == b1) {
         return 2;
      }
      return 1;
   }
   return 0;
}


/* Function: al_fwrite32le
 */
size_t al_fwrite32le(ALLEGRO_FS_ENTRY *f, int32_t l)
{
   int8_t b1, b2, b3, b4;
   ASSERT(f);

   b1 = ((l & 0xFF000000L) >> 24);
   b2 = ((l & 0x00FF0000L) >> 16);
   b3 = ((l & 0x0000FF00L) >> 8);
   b4 = l & 0x00FF;

   if (al_fputc(f, b4) == b4) {
      if (al_fputc(f, b3) == b3) {
         if (al_fputc(f, b2) == b2) {
            if (al_fputc(f, b1) == b1) {
               return 4;
            }
            return 3;
         }
         return 2;
      }
      return 1;
   }
   return 0;
}


/* Function: al_fread16be
 */
int16_t al_fread16be(ALLEGRO_FS_ENTRY *f)
{
   int b1, b2;
   ASSERT(f);

   if ((b1 = al_fgetc(f)) != EOF)
      if ((b2 = al_fgetc(f)) != EOF)
         return ((b1 << 8) | b2);

   return EOF;
}


/* Function: al_fread32be
 */
int32_t al_fread32be(ALLEGRO_FS_ENTRY *f, bool *ret_success)
{
   int b1, b2, b3, b4;
   ASSERT(f);

   if ((b1 = al_fgetc(f)) != EOF) {
      if ((b2 = al_fgetc(f)) != EOF) {
         if ((b3 = al_fgetc(f)) != EOF) {
            if ((b4 = al_fgetc(f)) != EOF) {
               if (ret_success)
                  *ret_success = true;
               return (((int32_t)b1 << 24) | ((int32_t)b2 << 16) |
                       ((int32_t)b3 << 8) | (int32_t)b4);
            }
         }
      }
   }

   if (ret_success)
      *ret_success = false;
   return EOF;
}


/* Function: al_fwrite16be
 */
size_t al_fwrite16be(ALLEGRO_FS_ENTRY *f, int16_t w)
{
   int b1, b2;
   ASSERT(f);

   b1 = (w & 0xFF00) >> 8;
   b2 = w & 0x00FF;

   if (al_fputc(f, b1) == b1) {
      if (al_fputc(f, b2) == b2) {
         return 2;
      }
      return 1;
   }
   return 0;
}


/* Function: al_fwrite32be
 */
size_t al_fwrite32be(ALLEGRO_FS_ENTRY *f, int32_t l)
{
   int b1, b2, b3, b4;
   ASSERT(f);

   b1 = ((l & 0xFF000000L) >> 24);
   b2 = ((l & 0x00FF0000L) >> 16);
   b3 = ((l & 0x0000FF00L) >> 8);
   b4 = l & 0x00FF;

   if (al_fputc(f, b1)==b1) {
      if (al_fputc(f, b2)==b2) {
         if (al_fputc(f, b3)==b3) {
            if (al_fputc(f, b4)==b4)
               return 4;
            return 3;
         }
         return 2;
      }
      return 1;
   }
   return 0;
}


/* Function: al_fgets
 */
char *al_fgets(ALLEGRO_FS_ENTRY *f, char *p, size_t max)
{
   int c = 0;
   ALLEGRO_USTR *u;
   ASSERT(f);

   al_set_errno(0);

   if ((c = al_fgetc(f)) == EOF) {
      return NULL;
   }

   u = al_ustr_new("");

   do {
      char c2[] = " ";
      if (c == '\r' || c == '\n') {
         if (c == '\r') {
            /* eat the following \n, if any */
            c = al_fgetc(f);
            if ((c != '\n') && (c != EOF))
               al_fungetc(f, c);
         }
         break;
      }

      /* write the character */
      c2[0] = c;
      al_ustr_append_cstr(u, c2);
   } while ((c = al_fgetc(f)) != EOF);

   _al_sane_strncpy(p, al_cstr(u), max);
   al_ustr_free(u);

   if (c == '\0' || al_get_errno())
      return NULL;

   return p;
}


/* Function: al_fputs
 */
int al_fputs(ALLEGRO_FS_ENTRY *f, char const *p)
{
   char const *s = p;
   ASSERT(f);
   ASSERT(p);

   al_set_errno(0);

   while (*s) {
      #if (defined ALLEGRO_DOS) || (defined ALLEGRO_WINDOWS)
         if (*s == '\n')
            al_fputc(f, '\r');
      #endif

      al_fputc(f, *s);
      s++;
   }

   if (al_get_errno())
      return -1;
   else
      return 0;
}


/* Function: al_fungetc
 */
int al_fungetc(ALLEGRO_FS_ENTRY *fp, int c)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_ungetc(fp, c);
}


/* maybe find a better place for this later */
static bool _al_find_resource_exists(const char *path, const char *base,
   const char *resource, uint32_t fm, char *buffer, size_t len)
{
   ALLEGRO_PATH *fp;
   bool ret = false;
   const char *s;

   memset(buffer, 0, len);

   fp = al_path_create(path);
   /* XXX this isn't strictly correct, is it? */
   al_path_append(fp, base);

   if (resource) {
      ALLEGRO_PATH *resp = al_path_create(resource);
      al_path_concat(fp, resp);
      al_path_free(resp);
   }

   s = al_path_to_string(fp, ALLEGRO_NATIVE_PATH_SEP);
   _al_sane_strncpy(buffer, s, len);

   if (al_is_present_str(buffer) && (al_get_entry_mode_str(buffer) & fm) == fm) {
      ret = true;
   }
   else if (fm & ALLEGRO_FILEMODE_WRITE) {
      /* XXX update this */
      /* XXX is this supposed to be chr or rchr? */
      /* FIXME: Or rather, what was this even supposed to accomplish??? */
      /*char *rchr = strchr(buffer, ALLEGRO_NATIVE_PATH_SEP);
      if (rchr) {
         usetc(rchr, '\0');

         if (al_is_present_str(buffer) && al_get_entry_mode_str(buffer) & ALLEGRO_FILEMODE_WRITE) {
            ret = true;
         }

         *rchr = ALLEGRO_NATIVE_PATH_SEP;
      }
      * */
   }

   al_path_free(fp);

   return ret;
}


// FIXME: Not needed after some code cleanup.
static char const *path_to_buffer(ALLEGRO_PATH *path, char *buffer, size_t size)
{
   char const *str = al_path_to_string(path, '/');
   _al_sane_strncpy(buffer, str, size);
   al_path_free(path);
   return buffer;
}


/* Function: al_find_resource
 */
char *al_find_resource(const char *base, const char *resource, uint32_t fm,
   char *buffer, size_t len)
{
   ALLEGRO_PATH *path;
   char tmp[PATH_MAX];
   char base_new[256];
   const char *s;
   bool r;

   ASSERT(base != NULL);
   ASSERT(resource != NULL);
   ASSERT(buffer != NULL);

   fm |= ALLEGRO_FILEMODE_READ;

   memset(buffer, 0, len);

#ifdef ALLEGRO_WINDOWS
   memset(base_new, 0, 256);
#else
   strcpy(base_new, ".");
#endif

   strcat(base_new, base);

   path = al_get_standard_path(ALLEGRO_USER_DATA_PATH);
   //printf("find_resource: AL_USER_DATA_PATH\n");
   if (_al_find_resource_exists(path_to_buffer(path, tmp, PATH_MAX),
      base_new, resource, fm, buffer, len)) {
      return buffer;
   }

   path = al_get_standard_path(ALLEGRO_PROGRAM_PATH);
   //printf("find_resource: AL_PROGRAM_PATH\n");
   if (_al_find_resource_exists(path_to_buffer(path, tmp, PATH_MAX),
      "data", resource, fm, buffer, len)) {
      return buffer;
   }

   path = al_getcwd();
   r = _al_find_resource_exists(al_path_to_string(path, '/'), "data",
      resource, fm, buffer, len);
   al_path_free(path);
   //printf("find_resource: getcwd\n");
   if (r) {
      return buffer;
   }

   path = al_get_standard_path(ALLEGRO_SYSTEM_DATA_PATH);
   //printf("find_resource: AL_SYSTEM_DATA_PATH\n");
   if (_al_find_resource_exists(path_to_buffer(path, tmp, PATH_MAX),
      base_new, resource, fm, buffer, len)) {
      return buffer;
   }

   /* file didn't exist anywhere, lets return whatever we can */

   path = al_get_standard_path(ALLEGRO_USER_DATA_PATH);
   //printf("find_resource: def AL_USER_DATA_PATH\n");
   al_path_append(path, base_new);

   if (resource) {
      ALLEGRO_PATH *resp = al_path_create(resource);
      al_path_concat(path, resp);
      al_path_free(resp);
   }

   s = al_path_to_string(path, ALLEGRO_NATIVE_PATH_SEP);
   _al_sane_strncpy(buffer, s, len);

   al_path_free(path);

   return buffer;
}


/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
/* vim: set sts=3 sw=3 et: */
