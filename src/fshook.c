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

struct ALLEGRO_FS_HOOK_SYS_INTERFACE  *_al_sys_fshooks = &_al_stdio_sys_fshooks;
struct ALLEGRO_FS_HOOK_ENTRY_INTERFACE *_al_entry_fshooks = &_al_stdio_entry_fshooks;

/* Function: al_create_entry
 */
ALLEGRO_FS_ENTRY *al_create_entry(AL_CONST char *path)
{
   ALLEGRO_FS_ENTRY *handle = _al_fs_hook_create(path);
   if (!handle)
      return NULL;

   return handle;
}

/* Function: al_destroy_entry
 */
void al_destroy_entry(ALLEGRO_FS_ENTRY *handle)
{
   ASSERT(handle != NULL);

   _al_fs_hook_destroy(handle);

}

/* Function: al_open_entry
 */
bool al_open_entry(ALLEGRO_FS_ENTRY *handle, AL_CONST char *mode)
{
   ASSERT(handle != NULL);
   ASSERT(mode != NULL);

   return _al_fs_hook_open(handle, mode);
}

/* Function: al_close_entry
 */
void al_close_entry(ALLEGRO_FS_ENTRY *handle)
{
   ASSERT(handle != NULL);
   _al_fs_hook_close(handle);
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
size_t al_fread(ALLEGRO_FS_ENTRY *fp, size_t size, void *ptr)
{
   ASSERT(ptr != NULL);
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_read(fp, size, ptr);
}

/* Function: al_fwrite
 */
size_t al_fwrite(ALLEGRO_FS_ENTRY *fp, size_t size, const void *ptr)
{
   ASSERT(ptr != NULL);
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_write(fp, size, ptr);
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
   ASSERT(whence == ALLEGRO_SEEK_SET || whence == ALLEGRO_SEEK_CUR || whence == ALLEGRO_SEEK_END);

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

/* Function: al_unlink_entry
 */
bool al_unlink_entry(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);

   return _al_fs_hook_entry_unlink(e);
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
   return al_get_entry_mode(e) & AL_FM_ISDIR;
}

/* Function: al_is_file
 */
bool al_is_file(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return al_get_entry_mode(e) & AL_FM_ISFILE;
}

/* Function: al_mktemp
 */
ALLEGRO_FS_ENTRY *al_mktemp(const char *template, uint32_t ulink)
{
   ASSERT(template != NULL);
   ASSERT(
      ulink == ALLEGRO_FS_MKTEMP_UNLINK_NOW ||
      ulink == ALLEGRO_FS_MKTEMP_UNLINK_ON_CLOSE ||
      ulink == ALLEGRO_FS_MKTEMP_UNLINK_NEVER
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
bool al_mkdir(AL_CONST char *path)
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
bool al_get_search_path(uint32_t idx, size_t len, char *dest)
{
   ASSERT(dest);
   ASSERT(len);

   return _al_fs_hook_get_search_path(idx, len, dest);
}

/* Function: al_drive_sep
 */
int32_t al_drive_sep(size_t len, char *sep)
{
   ASSERT(len > 0);
   ASSERT(sep);

   return _al_fs_hook_drive_sep(len, sep);
}

/* Function: al_path_sep
 */
int32_t al_path_sep(size_t len, char *sep)
{
   ASSERT(len > 0);
   ASSERT(sep);

   return _al_fs_hook_path_sep(len, sep);
}

/* not sure these two conversion hooks are needed, should the path conversion be in the driver? */
/* yup, driver may want to expose a "environment" that doesn't match the curren't platform's */
/* Function: al_path_to_sys
 */
int32_t al_path_to_sys(const char *orig, size_t len, char *path)
{
   ASSERT(orig);
   ASSERT(len > 0);
   ASSERT(path);

   return _al_fs_hook_path_to_sys(orig, len, path);
}

/* Function: al_path_to_uni
 */
int32_t al_path_to_uni(const char *orig, size_t len, char *path)
{
   ASSERT(orig);
   ASSERT(len > 0);
   ASSERT(path);

   return _al_fs_hook_path_to_uni(orig, len, path);
}

/* Function: al_get_entry_mode_str
 */
uint32_t al_get_entry_mode_str(const char *path)
{
   ASSERT(path != NULL);
   return _al_fs_hook_stat_mode(path);
}

/* Function: al_get_entry_atime_str
 */
time_t al_get_entry_atime_str(const char *path)
{
   ASSERT(path != NULL);
   return _al_fs_hook_stat_atime(path);
}

/* Function: al_get_entry_mtime_str
 */
time_t al_get_entry_mtime_str(const char *path)
{
   ASSERT(path != NULL);
   return _al_fs_hook_stat_mtime(path);
}

/* Function: al_get_entry_ctime_str
 */
time_t al_get_entry_ctime_str(const char *path)
{
   ASSERT(path != NULL);
   return _al_fs_hook_stat_ctime(path);
}

/* Function: al_get_entry_size_str
 */
off_t al_get_entry_size_str(const char *path)
{
   ASSERT(path != NULL);
   return _al_fs_hook_stat_size(path);
}

/* Function: al_unlink_str
 */
bool al_unlink_str(const char *path)
{
   ASSERT(path != NULL);
   return _al_fs_hook_unlink(path);
}

/* Function: al_is_present_str
 */
bool al_is_present_str(const char *path)
{
   ASSERT(path != NULL);

   return _al_fs_hook_exists(path);
}

/* Function: al_is_directory_str
 */
bool al_is_directory_str(AL_CONST char *path)
{
   ASSERT(path != NULL);
   return _al_fs_hook_stat_mode(path) & AL_FM_ISDIR;
}

/* Function: al_is_file_str
 */
bool al_is_file_str(AL_CONST char *path)
{
   ASSERT(path != NULL);
   return _al_fs_hook_stat_mode(path) & AL_FM_ISFILE;
}

/* Function: al_fgetc
 */
int al_fgetc(ALLEGRO_FS_ENTRY *f)
{
   uint8_t c = 0;
   ASSERT(f);

   if (al_fread(f, 1, (void *)&c) != 1) {
      if (al_feof(f))
         return EOF;
   }

   return c;
}

/* Function: al_fputc
 */
int al_fputc(ALLEGRO_FS_ENTRY *f, int c)
{
   ASSERT(f);

   if (al_fwrite(f, 1, (void *)&c) != 1) {
      if (al_ferror(f))
         return EOF;
   }

   return c;
}

/* Function: al_fread16le
 */
int16_t al_fread16le(ALLEGRO_FS_ENTRY *f)
{
   int16_t b1 = 0, b2 = 0;
   ASSERT(f);

   if ((b1 = al_fgetc(f)) != EOF)
      if ((b2 = al_fgetc(f)) != EOF)
         return ((b2 << 8) | b1);

   return EOF;
}

/* Function: al_fread32le
 */
int32_t al_fread32le(ALLEGRO_FS_ENTRY *f)
{
   int32_t b1 = 0, b2 = 0, b3 = 0, b4 = 0;
   ASSERT(f);

   if ((b1 = al_fgetc(f)) != EOF)
      if ((b2 = al_fgetc(f)) != EOF)
         if ((b3 = al_fgetc(f)) != EOF)
            if ((b4 = al_fgetc(f)) != EOF)
               return (((int32_t)b4 << 24) | ((int32_t)b3 << 16) |
                       ((int32_t)b2 << 8) | (int32_t)b1);

   return EOF;
}

/* Function: al_fwrite16le
 */
int16_t al_fwrite16le(ALLEGRO_FS_ENTRY *f, int16_t w)
{
   int16_t b1 = 0, b2 = 0;
   ASSERT(f);

   b1 = (w & 0xFF00) >> 8;
   b2 = w & 0x00FF;

   if (al_fputc(f, b2)==b2)
      if (al_fputc(f, b1)==b1)
         return w;

   return EOF;
}

/* Function: al_fwrite32le
 */
int32_t al_fwrite32le(ALLEGRO_FS_ENTRY *f, int32_t l)
{
   int32_t b1 = 0, b2 = 0, b3 = 0, b4 = 0;
   ASSERT(f);

   b1 = (int32_t)((l & 0xFF000000L) >> 24);
   b2 = (int32_t)((l & 0x00FF0000L) >> 16);
   b3 = (int32_t)((l & 0x0000FF00L) >> 8);
   b4 = (int32_t)l & 0x00FF;

   if (al_fputc(f, b4)==b4)
      if (al_fputc(f, b3)==b3)
         if (al_fputc(f, b2)==b2)
            if (al_fputc(f, b1)==b1)
               return l;

   return EOF;
}

/* Function: al_fread16be
 */
int16_t al_fread16be(ALLEGRO_FS_ENTRY *f)
{
   int16_t b1 = 0, b2 = 0;
   ASSERT(f);

   if ((b1 = al_fgetc(f)) != EOF)
      if ((b2 = al_fgetc(f)) != EOF)
         return ((b1 << 8) | b2);

   return EOF;
}

/* Function: al_fread32be
 */
int32_t al_fread32be(ALLEGRO_FS_ENTRY *f)
{
   int32_t b1 = 0, b2 = 0, b3 = 0, b4 = 0;
   ASSERT(f);

   if ((b1 = al_fgetc(f)) != EOF)
      if ((b2 = al_fgetc(f)) != EOF)
         if ((b3 = al_fgetc(f)) != EOF)
            if ((b4 = al_fgetc(f)) != EOF)
               return (((int32_t)b1 << 24) | ((int32_t)b2 << 16) |
                       ((int32_t)b3 << 8) | (int32_t)b4);

   return EOF;
}

/* Function: al_fwrite16be
 */
int16_t al_fwrite16be(ALLEGRO_FS_ENTRY *f, int16_t w)
{
   int16_t b1 = 0, b2 = 0;
   ASSERT(f);

   b1 = (w & 0xFF00) >> 8;
   b2 = w & 0x00FF;

   if (al_fputc(f, b1)==b1)
      if (al_fputc(f, b2)==b2)
         return w;

   return EOF;
}

/* Function: al_fwrite32be
 */
int32_t al_fwrite32be(ALLEGRO_FS_ENTRY *f, int32_t l)
{
   int32_t b1 = 0, b2 = 0, b3 = 0, b4 = 0;
   ASSERT(f);

   b1 = (int32_t)((l & 0xFF000000L) >> 24);
   b2 = (int32_t)((l & 0x00FF0000L) >> 16);
   b3 = (int32_t)((l & 0x0000FF00L) >> 8);
   b4 = (int32_t)l & 0x00FF;

   if (al_fputc(f, b1)==b1)
      if (al_fputc(f, b2)==b2)
         if (al_fputc(f, b3)==b3)
            if (al_fputc(f, b4)==b4)
               return l;

   return EOF;
}

/* Function: al_fgets
 */
char *al_fgets(ALLEGRO_FS_ENTRY *f, size_t max, char *p)
{
   char *pmax = NULL, *orig_p = p;
   int c = 0;
   ASSERT(f);

   al_set_errno(0);

   pmax = p+max - ucwidth(0);

   if ((c = al_fgetc(f)) == EOF) {
      if ((unsigned) ucwidth(0) <= max)
         usetc(p,0);
      return NULL;
   }

   do {
      if (c == '\r' || c == '\n') {
         /* Technically we should check there's space in the buffer, and if so,
          * add a \n.  But pack_fgets has never done this. */
         if (c == '\r') {
            /* eat the following \n, if any */
            c = al_fgetc(f);
            if ((c != '\n') && (c != EOF))
               al_fungetc(f, c);
         }
         break;
      }

      /* is there room in the buffer? */
      if (ucwidth(c) > pmax - p) {
         al_fungetc(f, c);
         c = '\0';
         break;
      }

      /* write the character */
      p += usetc(p, c);
   } while ((c = al_fgetc(f)) != EOF);

   /* terminate the string */
   usetc(p, 0);

   if (c == '\0' || al_get_errno())
      return NULL;

   return orig_p; /* p has changed */
}

/* Function: al_fputs
 */
int al_fputs(ALLEGRO_FS_ENTRY *f, AL_CONST char *p)
{
   char *buf = NULL, *s = NULL;
   int bufsize = 0;
   ASSERT(f);
   ASSERT(p);

   al_set_errno(0);

   bufsize = uconvert_size(p, U_UTF8, U_UTF8);
   buf = _AL_MALLOC_ATOMIC(bufsize);
   if (!buf)
      return -1;

   s = uconvert(p, U_UTF8, buf, U_UTF8, bufsize);

   while (*s) {
      #if (defined ALLEGRO_DOS) || (defined ALLEGRO_WINDOWS)
         if (*s == '\n')
            al_fputc(f, '\r');
      #endif

      al_fputc(f, *s);
      s++;
   }

   _AL_FREE(buf);

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
   else if (fm & AL_FM_WRITE) {
      /* XXX update this */
      /* XXX is this supposed to be chr or rchr? */
      char *rchr = ustrchr(buffer, ALLEGRO_NATIVE_PATH_SEP);
      if (rchr) {
         usetc(rchr, '\0');

         if (al_is_present_str(buffer) && al_get_entry_mode_str(buffer) & AL_FM_WRITE) {
            ret = true;
         }

         usetc(rchr, ALLEGRO_NATIVE_PATH_SEP);
      }
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

   fm |= AL_FM_READ;

   memset(buffer, 0, len);

#ifdef ALLEGRO_WINDOWS
   memset(base_new, 0, 256);
#else
   ustrcpy(base_new, ".");
#endif

   ustrcat(base_new, base);

   path = al_get_path(AL_USER_DATA_PATH);
   //printf("find_resource: AL_USER_DATA_PATH\n");
   if (_al_find_resource_exists(path_to_buffer(path, tmp, PATH_MAX),
      base_new, resource, fm, buffer, len)) {
      return buffer;
   }

   path = al_get_path(AL_PROGRAM_PATH);
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

   path = al_get_path(AL_SYSTEM_DATA_PATH);
   //printf("find_resource: AL_SYSTEM_DATA_PATH\n");
   if (_al_find_resource_exists(path_to_buffer(path, tmp, PATH_MAX),
      base_new, resource, fm, buffer, len)) {
      return buffer;
   }

   /* file didn't exist anywhere, lets return whatever we can */

   path = al_get_path(AL_USER_DATA_PATH);
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
