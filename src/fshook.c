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

#include "allegro5/allegro5.h"
#include ALLEGRO_INTERNAL_HEADER
#include "allegro5/debug.h"
#include "allegro5/fshook.h"
#include "allegro5/internal/fshook.h"
#include "allegro5/internal/aintern_memory.h"
#include "allegro5/path.h"

struct AL_FS_HOOK_SYS_INTERFACE  *_al_sys_fshooks = &_al_stdio_sys_fshooks;
struct AL_FS_HOOK_ENTRY_INTERFACE *_al_entry_fshooks = &_al_stdio_entry_fshooks;

AL_FS_ENTRY *al_fs_create_handle(AL_CONST char *path)
{
   AL_FS_ENTRY *handle = _al_fs_hook_create_handle(path);
   if (!handle)
      return NULL;

   return handle;
}

void al_fs_destroy_handle(AL_FS_ENTRY *handle)
{
   ASSERT(handle != NULL);

   _al_fs_hook_destroy_handle(handle);

}

int32_t al_fs_open_handle(AL_FS_ENTRY *handle, AL_CONST char *mode)
{
   ASSERT(handle != NULL);
   ASSERT(mode != NULL);

   return _al_fs_hook_open_handle(handle, mode);
}

void al_fs_close_handle(AL_FS_ENTRY *handle)
{
   ASSERT(handle != NULL);
   _al_fs_hook_close_handle(handle);
}

void al_fs_entry_name(AL_FS_ENTRY *fp, size_t size, char *buf)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_name(fp, size, buf);
}

AL_FS_ENTRY *al_fs_entry_open(const char *path, const char *mode)
{
   ASSERT(path != NULL);
   ASSERT(mode != NULL);

   return _al_fs_hook_entry_open(path, mode);
}

void al_fs_entry_close(AL_FS_ENTRY *fp)
{
   ASSERT(fp != NULL);

   _al_fs_hook_entry_close(fp);
}

ssize_t   al_fs_entry_read(void *ptr, size_t size, AL_FS_ENTRY *fp)
{
   ASSERT(ptr != NULL);
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_read(ptr, size, fp);
}

ssize_t   al_fs_entry_write(const void *ptr, size_t size, AL_FS_ENTRY *fp)
{
   ASSERT(ptr != NULL);
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_write(ptr, size, fp);
}

int32_t al_fs_entry_flush(AL_FS_ENTRY *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_flush(fp);
}

int32_t al_fs_entry_seek(AL_FS_ENTRY *fp, uint32_t offset, uint32_t whence)
{
   ASSERT(fp != NULL);
   ASSERT(offset > 0);
   ASSERT(whence == AL_SEEK_SET || whence == AL_SEEK_CUR || whence == AL_SEEK_END);

   return _al_fs_hook_entry_seek(fp, offset, whence);
}

int32_t al_fs_entry_tell(AL_FS_ENTRY *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_tell(fp);
}

int32_t al_fs_entry_error(AL_FS_ENTRY *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_error(fp);
}

int32_t al_fs_entry_eof(AL_FS_ENTRY *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_eof(fp);
}


int32_t al_fs_entry_stat(AL_FS_ENTRY *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_stat(fp);
}


AL_FS_ENTRY *al_fs_opendir(const char *path)
{
   AL_FS_ENTRY *dir = NULL;

   ASSERT(path != NULL);

   dir = _al_fs_hook_opendir(path);
   if (!dir)
      return NULL;

   return dir;
}

int32_t al_fs_closedir(AL_FS_ENTRY *dir)
{
   struct _al_entry_fshooks *vtable = NULL;
   int32_t ret = 0;

   ASSERT(dir != NULL);

   ret = _al_fs_hook_closedir(dir);

   return ret;
}

int32_t al_fs_readdir(AL_FS_ENTRY *dir, size_t size, char *name)
{
   ASSERT(dir != NULL);
   ASSERT(size > 0);
   ASSERT(name != NULL);

   return _al_fs_hook_readdir(dir, size, name);
}

uint32_t al_fs_entry_mode(AL_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return _al_fs_hook_entry_mode(e);
}

time_t al_fs_entry_atime(AL_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return _al_fs_hook_entry_atime(e);
}

time_t al_fs_entry_mtime(AL_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return _al_fs_hook_entry_mtime(e);
}

time_t al_fs_entry_ctime(AL_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return _al_fs_hook_entry_ctime(e);
}

off_t al_fs_entry_size(AL_FS_ENTRY *e)
{
   ASSERT(e != NULL);

   return _al_fs_hook_entry_size(e);
}

int32_t al_fs_entry_unlink( AL_FS_ENTRY *e )
{
   ASSERT(e != NULL);

   return _al_fs_hook_entry_unlink( e );
}

int32_t al_fs_entry_exists( AL_FS_ENTRY *e )
{
   ASSERT(e != NULL);

   return _al_fs_hook_entry_exists( e );
}

int32_t al_fs_entry_isdir(AL_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return al_fs_entry_mode(e) & AL_FM_ISDIR;
}

int32_t al_fs_entry_isfile(AL_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return al_fs_entry_mode(e) & AL_FM_ISFILE;
}

AL_FS_ENTRY *al_fs_mktemp(const char *template, uint32_t ulink)
{
   ASSERT(template != NULL);
   ASSERT(ulink != AL_FS_MKTEMP_UNLINK_NOW && ulink != AL_FS_MKTEMP_UNLINK_ON_CLOSE && ulink != AL_FS_MKTEMP_UNLINK_NEVER);

   return _al_fs_hook_mktemp(template, ulink);
}

int32_t al_fs_getcwd(char *buf, size_t len)
{
   ASSERT(buf != NULL);
   ASSERT(len != 0);

   return _al_fs_hook_getcwd(buf, len);
}

int32_t al_fs_chdir(const char *path)
{
   ASSERT(path);

   return _al_fs_hook_chdir(path);
}

int32_t al_fs_mkdir(AL_CONST char *path)
{
   ASSERT(path);

   return _al_fs_hook_mkdir(path);
}

int32_t al_fs_add_search_path(const char *path)
{
   ASSERT(path);

   return _al_fs_hook_add_search_path(path);
}

int32_t al_fs_search_path_count()
{
   return _al_fs_hook_search_path_count();
}

int32_t al_fs_get_search_path(uint32_t idx, char *dest, size_t len)
{
   ASSERT(dest);
   ASSERT(len);

   return _al_fs_hook_get_search_path(idx, dest, len);
}

int32_t al_fs_drive_sep(size_t len, char *sep)
{
   ASSERT(len > 0);
   ASSERT(sep);

   return _al_fs_hook_drive_sep(len, sep);
}

int32_t al_fs_path_sep(size_t len, char *sep)
{
   ASSERT(len > 0);
   ASSERT(sep);

   return _al_fs_hook_path_sep(len, sep);
}

/* not sure these two conversion hooks are needed, should the path conversion be in the driver? */
/* yup, driver may want to expose a "environment" that doesn't match the curren't platform's */
int32_t al_fs_path_to_sys(const char *orig, size_t len, char *path)
{
   ASSERT(orig);
   ASSERT(len > 0);
   ASSERT(path);

   return _al_fs_hook_path_to_sys(orig, len, path);
}

int32_t al_fs_path_to_uni(const char *orig, size_t len, char *path)
{
   ASSERT(orig);
   ASSERT(len > 0);
   ASSERT(path);

   return _al_fs_hook_path_to_uni(orig, len, path);
}

uint32_t al_fs_stat_mode( const char *path )
{
   ASSERT(path != NULL);
   return _al_fs_hook_stat_mode(path);
}

time_t al_fs_stat_atime( const char *path )
{
   ASSERT(path != NULL);
   return _al_fs_hook_stat_atime(path);
}

time_t al_fs_stat_mtime( const char *path )
{
   ASSERT(path != NULL);
   return _al_fs_hook_stat_mtime(path);
}

time_t al_fs_stat_ctime( const char *path )
{
   ASSERT(path != NULL);
   return _al_fs_hook_stat_ctime(path);
}

off_t al_fs_stat_size( const char *path )
{
   ASSERT(path != NULL);
   return _al_fs_hook_stat_size(path);
}

int32_t al_fs_unlink( const char *path )
{
   ASSERT(path != NULL);
   return _al_fs_hook_unlink(path);
}

int32_t al_fs_exists( const char *path )
{
   ASSERT(path != NULL);

   return _al_fs_hook_exists( path );
}

int32_t al_fs_isdir(AL_CONST char *path)
{
   ASSERT(path != NULL);
   return _al_fs_hook_stat_mode(path) & AL_FM_ISDIR;
}

int32_t al_fs_isfile(AL_CONST char *path)
{
   ASSERT(path != NULL);
   return _al_fs_hook_stat_mode(path) & AL_FM_ISFILE;
}

int al_fs_entry_getc (AL_FS_ENTRY *f)
{
   uint32_t c = 0;
   ASSERT(f);

   if (al_fs_entry_read(&c, 1, f) != 1) {
      if (al_fs_entry_eof(f))
         return EOF;
   }

   return c;
}

int al_fs_entry_putc (int c, AL_FS_ENTRY *f)
{
   ASSERT(f);

   if (al_fs_entry_write(&c, 1, f) != 1) {
      if (al_fs_entry_error(f))
         return EOF;
   }

   return c;
}


int16_t al_fs_entry_igetw (AL_FS_ENTRY *f)
{
   int16_t b1 = 0, b2 = 0;
   ASSERT(f);

   if ((b1 = al_fs_entry_getc(f)) != EOF)
      if ((b2 = al_fs_entry_getc(f)) != EOF)
         return ((b2 << 8) | b1);

   return EOF;
}

int32_t al_fs_entry_igetl (AL_FS_ENTRY *f)
{
   int32_t b1 = 0, b2 = 0, b3 = 0, b4 = 0;
   ASSERT(f);

   if ((b1 = al_fs_entry_getc(f)) != EOF)
      if ((b2 = al_fs_entry_getc(f)) != EOF)
         if ((b3 = al_fs_entry_getc(f)) != EOF)
            if ((b4 = al_fs_entry_getc(f)) != EOF)
               return (((int32_t)b4 << 24) | ((int32_t)b3 << 16) |
                       ((int32_t)b2 << 8) | (int32_t)b1);

   return EOF;
}

int16_t al_fs_entry_iputw (int16_t w, AL_FS_ENTRY *f)
{
   int16_t b1 = 0, b2 = 0;
   ASSERT(f);

   b1 = (w & 0xFF00) >> 8;
   b2 = w & 0x00FF;

   if (al_fs_entry_putc(b2,f)==b2)
      if (al_fs_entry_putc(b1,f)==b1)
         return w;

   return EOF;
}

int32_t al_fs_entry_iputl (int32_t l, AL_FS_ENTRY *f)
{
   int32_t b1 = 0, b2 = 0, b3 = 0, b4 = 0;
   ASSERT(f);

   b1 = (int32_t)((l & 0xFF000000L) >> 24);
   b2 = (int32_t)((l & 0x00FF0000L) >> 16);
   b3 = (int32_t)((l & 0x0000FF00L) >> 8);
   b4 = (int32_t)l & 0x00FF;

   if (al_fs_entry_putc(b4,f)==b4)
      if (al_fs_entry_putc(b3,f)==b3)
         if (al_fs_entry_putc(b2,f)==b2)
            if (al_fs_entry_putc(b1,f)==b1)
               return l;

   return EOF;
}

int16_t al_fs_entry_mgetw (AL_FS_ENTRY *f)
{
   int16_t b1 = 0, b2 = 0;
   ASSERT(f);

   if ((b1 = al_fs_entry_getc(f)) != EOF)
      if ((b2 = al_fs_entry_getc(f)) != EOF)
         return ((b1 << 8) | b2);

   return EOF;
}

int32_t al_fs_entry_mgetl (AL_FS_ENTRY *f)
{
   int32_t b1 = 0, b2 = 0, b3 = 0, b4 = 0;
   ASSERT(f);

   if ((b1 = al_fs_entry_getc(f)) != EOF)
      if ((b2 = al_fs_entry_getc(f)) != EOF)
         if ((b3 = al_fs_entry_getc(f)) != EOF)
            if ((b4 = al_fs_entry_getc(f)) != EOF)
               return (((int32_t)b1 << 24) | ((int32_t)b2 << 16) |
                       ((int32_t)b3 << 8) | (int32_t)b4);

   return EOF;
}

int16_t al_fs_entry_mputw (int16_t w, AL_FS_ENTRY *f)
{
   int16_t b1 = 0, b2 = 0;
   ASSERT(f);

   b1 = (w & 0xFF00) >> 8;
   b2 = w & 0x00FF;

   if (al_fs_entry_putc(b1,f)==b1)
      if (al_fs_entry_putc(b2,f)==b2)
         return w;

   return EOF;
}

int32_t al_fs_entry_mputl (int32_t l, AL_FS_ENTRY *f)
{
   int32_t b1 = 0, b2 = 0, b3 = 0, b4 = 0;
   ASSERT(f);

   b1 = (int32_t)((l & 0xFF000000L) >> 24);
   b2 = (int32_t)((l & 0x00FF0000L) >> 16);
   b3 = (int32_t)((l & 0x0000FF00L) >> 8);
   b4 = (int32_t)l & 0x00FF;

   if (al_fs_entry_putc(b1,f)==b1)
      if (al_fs_entry_putc(b2,f)==b2)
         if (al_fs_entry_putc(b3,f)==b3)
            if (al_fs_entry_putc(b4,f)==b4)
               return l;

   return EOF;
}

char *al_fs_entry_fgets (char *p, ssize_t max, AL_FS_ENTRY *f)
{
   char *pmax = NULL, *orig_p = p;
   int c = 0;
   ASSERT(f);

   al_set_errno(0);

   pmax = p+max - ucwidth(0);

   if ((c = al_fs_entry_getc(f)) == EOF) {
      if (ucwidth(0) <= max)
         usetc(p,0);
      return NULL;
   }

   do {

      if (c == '\r' || c == '\n') {
         /* Technically we should check there's space in the buffer, and if so,
          * add a \n.  But pack_fgets has never done this. */
         if (c == '\r') {
            /* eat the following \n, if any */
            c = al_fs_entry_getc(f);
            if ((c != '\n') && (c != EOF))
               al_fs_entry_ungetc(c, f);
         }
         break;
      }

      /* is there room in the buffer? */
      if (ucwidth(c) > pmax - p) {
         al_fs_entry_ungetc(c, f);
         c = '\0';
         break;
      }

      /* write the character */
      p += usetc(p, c);
   }
   while ((c = al_fs_entry_getc(f)) != EOF);

   /* terminate the string */
   usetc(p, 0);

   if (c == '\0' || al_get_errno())
      return NULL;

   return orig_p; /* p has changed */
}

int   al_fs_entry_fputs (AL_CONST char *p, AL_FS_ENTRY *f)
{
   char *buf = NULL, *s = NULL;
   int bufsize = 0;
   ASSERT(f);
   ASSERT(p);

   al_set_errno(0);

   bufsize = uconvert_size(p, U_CURRENT, U_UTF8);
   buf = _AL_MALLOC_ATOMIC(bufsize);
   if (!buf)
      return -1;

   s = uconvert(p, U_CURRENT, buf, U_UTF8, bufsize);

   while (*s) {
      #if (defined ALLEGRO_DOS) || (defined ALLEGRO_WINDOWS)
         if (*s == '\n')
            al_fs_entry_putc('\r', f);
      #endif

      al_fs_entry_putc(*s, f);
      s++;
   }

   _AL_FREE(buf);

   if (al_get_errno())
      return -1;
   else
      return 0;
}

int32_t al_fs_entry_ungetc(int32_t c, AL_FS_ENTRY *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_ungetc(c, fp);
}

/* maybe find a better place for this later */
int32_t _al_find_resource_exists(char *path, char *base, char *resource, uint32_t fm, char *buffer, size_t len)
{
   AL_PATH fp, resp;
   int32_t ret = 0;

   memset(buffer, 0, len);

   al_path_init(&fp, path);
   al_path_append(&fp, base);

   if (resource) {
      al_path_init(&resp, resource);
      al_path_concat(&fp, &resp);
      al_path_free(&resp);
   }

   al_path_to_string(&fp, buffer, len, ALLEGRO_NATIVE_PATH_SEP);
   //printf("_find_resource: '%s' exists:%i sfm:%i fm:%i eq:%i\n", buffer, al_fs_exists(buffer), al_fs_stat_mode(buffer), fm, (al_fs_stat_mode(buffer) & fm) == fm);
   if (al_fs_exists(buffer) && (al_fs_stat_mode(buffer) & fm) == fm) {
      ret = 1;
   }
   else if (fm & AL_FM_WRITE) {
      char *rchr = ustrchr(buffer, ALLEGRO_NATIVE_PATH_SEP);
      if (rchr) {
         usetc(rchr, '\0');

         //printf("testing '%s' for WRITE perms.\n", buffer);
         if (al_fs_exists(buffer) && al_fs_stat_mode(buffer) & AL_FM_WRITE) {
            ret = 1;
         }

         usetc(rchr, ALLEGRO_NATIVE_PATH_SEP);
      }
   }

   al_path_free(&fp);

   return ret;
}

char *al_find_resource(char *base, char *resource, uint32_t fm, char *buffer, size_t len)
{
   AL_PATH path, resp;
   char tmp[PATH_MAX];
   char base_new[256];

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

   al_get_path(AL_USER_DATA_PATH, tmp, PATH_MAX);
   //printf("find_resource: AL_USER_DATA_PATH\n");
   if (_al_find_resource_exists(tmp, base_new, resource, fm, buffer, len)) {
      return buffer;
   }

   al_get_path(AL_PROGRAM_PATH, tmp, PATH_MAX);
   //printf("find_resource: AL_PROGRAM_PATH\n");
   if (_al_find_resource_exists(tmp, "data", resource, fm, buffer, len)) {
      return buffer;
   }

   al_fs_getcwd(tmp, PATH_MAX);
   //printf("find_resource: getcwd\n");
   if (_al_find_resource_exists(tmp, "data", resource, fm, buffer, len)) {
      return buffer;
   }

   al_get_path(AL_SYSTEM_DATA_PATH, tmp, PATH_MAX);
   //printf("find_resource: AL_SYSTEM_DATA_PATH\n");
   if (_al_find_resource_exists(tmp, base_new, resource, fm, buffer, len)) {
      return buffer;
   }

   /* file didn't exist anywhere, lets return whatever we can */

   al_get_path(AL_USER_DATA_PATH, tmp, PATH_MAX);
   //printf("find_resource: def AL_USER_DATA_PATH\n");
   al_path_init(&path, tmp);
   al_path_append(&path, base_new);

   if (resource) {
      al_path_init(&resp, resource);
      al_path_concat(&path, &resp);
      al_path_free(&resp);
   }

   al_path_to_string(&path, buffer, len, ALLEGRO_NATIVE_PATH_SEP);

   return buffer;
}

/*
 * Local Variables:
 * c-basic-offset: 3
 * indent-tabs-mode: nil
 * End:
 */
/* vim: set sts=3 sw=3 et: */
