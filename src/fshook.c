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

#ifdef _MSC_VER
   #define _POSIX_
#endif
#include <limits.h>

#include "allegro5/allegro5.h"
#include ALLEGRO_INTERNAL_HEADER
#include "allegro5/debug.h"
#include "allegro5/fshook.h"
#include "allegro5/path.h"
#include "allegro5/internal/aintern_fshook.h"
#include "allegro5/internal/aintern_memory.h"

struct ALLEGRO_FS_HOOK_SYS_INTERFACE  *_al_sys_fshooks = &_al_stdio_sys_fshooks;
struct ALLEGRO_FS_HOOK_ENTRY_INTERFACE *_al_entry_fshooks = &_al_stdio_entry_fshooks;

/* Function: al_fs_create_handle
 * Creates a ALLEGRO_FS_ENTRY object pointing to path.
 * path can be a file or a directory, should not be NULL.
 */
ALLEGRO_FS_ENTRY *al_fs_create_handle(AL_CONST char *path)
{
   ALLEGRO_FS_ENTRY *handle = _al_fs_hook_create_handle(path);
   if (!handle)
      return NULL;

   return handle;
}

/* Function: al_fs_destroy_handle
 * Destroys a fs entry handle.
 * Closes file if it was open.
 */
void al_fs_destroy_handle(ALLEGRO_FS_ENTRY *handle)
{
   ASSERT(handle != NULL);

   _al_fs_hook_destroy_handle(handle);

}

/* Function: al_fs_open_handle
 * Opens handle with mode 'mode'.
 * mode is a stdio type mode, ie: "r", "w", etc
 */
int32_t al_fs_open_handle(ALLEGRO_FS_ENTRY *handle, AL_CONST char *mode)
{
   ASSERT(handle != NULL);
   ASSERT(mode != NULL);

   return _al_fs_hook_open_handle(handle, mode);
}

/* Function: al_fs_close_handle
 * Closes fs entry.
 */
void al_fs_close_handle(ALLEGRO_FS_ENTRY *handle)
{
   ASSERT(handle != NULL);
   _al_fs_hook_close_handle(handle);
}

/* Function: al_fs_entry_name
 * Fills in buf up to size bytes including trailing NULL char with the entry's name
 */
void al_fs_entry_name(ALLEGRO_FS_ENTRY *fp, size_t size, char *buf)
{
   ASSERT(fp != NULL);

   _al_fs_hook_entry_name(fp, size, buf);
}

/* Function: al_fs_entry_open
 * Creates and opens an ALLEGRO_FS_ENTRY object given path and mode.
 * 'path' is the path to open
 * 'mode' is the mode to open the entry in
 */
ALLEGRO_FS_ENTRY *al_fs_entry_open(const char *path, const char *mode)
{
   ASSERT(path != NULL);
   ASSERT(mode != NULL);

   return _al_fs_hook_entry_open(path, mode);
}

/* Function: al_fs_entry_close
 * Closes the given file entry object.
 * Will destroy the handle if it was opened with al_fs_entry_open.
 *
 * If you do not wish the entry object destroyed, use al_fs_close_handle instead.
 */
void al_fs_entry_close(ALLEGRO_FS_ENTRY *fp)
{
   ASSERT(fp != NULL);

   _al_fs_hook_entry_close(fp);
}

/* Function: al_fs_entry_read
 * Read 'size' bytes into 'ptr' from entry 'fp'
 *
 * Return number of bytes actually read.
 */
size_t al_fs_entry_read(void *ptr, size_t size, ALLEGRO_FS_ENTRY *fp)
{
   ASSERT(ptr != NULL);
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_read(ptr, size, fp);
}

/* Function: al_fs_entry_write
 * Write 'size' bytes from 'ptr' into file 'fp'
 *
 * Return number of bytes actually written or 0 on error.
 *
 * Does not distinguish between eof and other errors, use al_fs_entry_eof and al_fs_entry_error
 * to tell them apart.
 */
size_t al_fs_entry_write(const void *ptr, size_t size, ALLEGRO_FS_ENTRY *fp)
{
   ASSERT(ptr != NULL);
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_write(ptr, size, fp);
}

/* Function: al_fs_entry_flush
 * Flush any pending writes to 'fp' to disk.
 */
int32_t al_fs_entry_flush(ALLEGRO_FS_ENTRY *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_flush(fp);
}

/* Function: al_fs_entry_seek
 * Seek to 'offset' in file based on 'whence'.
 *
 * 'whence' can be:
 * ALLEGRO_SEEK_SET - Seek from beggining of file
 * ALLEGRO_SEEK_CUR - Seek from current position
 * ALLEGRO_SEEK_END - Seek from end of file
 */
int32_t al_fs_entry_seek(ALLEGRO_FS_ENTRY *fp, uint32_t offset, uint32_t whence)
{
   ASSERT(fp != NULL);
   ASSERT(offset > 0);
   ASSERT(whence == ALLEGRO_SEEK_SET || whence == ALLEGRO_SEEK_CUR || whence == ALLEGRO_SEEK_END);

   return _al_fs_hook_entry_seek(fp, offset, whence);
}

/* Function: al_fs_entry_tell
 * Returns the current position in file. Or -1 on error.
 */
int32_t al_fs_entry_tell(ALLEGRO_FS_ENTRY *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_tell(fp);
}

/* Function: al_fs_entry_error
 * Returns non zero if there was some sort of previous error.
 */
int32_t al_fs_entry_error(ALLEGRO_FS_ENTRY *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_error(fp);
}

/* Function: al_fs_entry_eof
 * Returns non zero if we have an end of file condition.
 */
int32_t al_fs_entry_eof(ALLEGRO_FS_ENTRY *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_eof(fp);
}

/* Function: al_fs_entry_stat
 * Updates stat info for entry 'fp'
 */
int32_t al_fs_entry_stat(ALLEGRO_FS_ENTRY *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_stat(fp);
}

/* Function: al_fs_opendir
 * Creates and opens a filestem entry object for a directory
 *
 * Returns NULL on error.
 */
ALLEGRO_FS_ENTRY *al_fs_opendir(const char *path)
{
   ALLEGRO_FS_ENTRY *dir = NULL;

   ASSERT(path != NULL);

   dir = _al_fs_hook_opendir(path);
   if (!dir)
      return NULL;

   return dir;
}

/* Function: al_fs_closedir
 * Closes a previously opened directory entry object.
 *
 * al_fs_close_handle is also a valid way to close any entry object.
 *
 * does not free the entry object if it was opened with al_fs_opendir.
 * This is probably a bug.
 */
int32_t al_fs_closedir(ALLEGRO_FS_ENTRY *dir)
{
   int32_t ret = 0;

   ASSERT(dir != NULL);

   ret = _al_fs_hook_closedir(dir);

   return ret;
}

/* Function: al_fs_readdir
 * Reads the next dir item name into 'name' buffer up to 'size' chars.
 *
 * Warning this may leave the filename truncated.
 *
 * Returns non zero on error.
 */
int32_t al_fs_readdir(ALLEGRO_FS_ENTRY *dir, size_t size, char *name)
{
   ASSERT(dir != NULL);
   ASSERT(size > 0);
   ASSERT(name != NULL);

   return _al_fs_hook_readdir(dir, size, name);
}

/* Function: al_fs_entry_mode
 * Returns the entry's mode flags.
 *
 * See the ALLEGRO_FS_MODE enum for valid flags
 */
uint32_t al_fs_entry_mode(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return _al_fs_hook_entry_mode(e);
}

/* Function: al_fs_entry_atime
 * Returns the time in seonds since the epoch since the entry was last accessed.
 *
 * Warning, some filesystem either don't support this flag, or people turn it off to increase performance.
 * It may not be valid in all circumstances.
 */
time_t al_fs_entry_atime(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return _al_fs_hook_entry_atime(e);
}

/* Function: al_fs_entry_mtime
 * Returns the time in seconds since the epoch since the entry was last modified.
 */
time_t al_fs_entry_mtime(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return _al_fs_hook_entry_mtime(e);
}

/* Function: al_fs_entry_ctime
 * Returns the time in seconds since the epoch this entry was created on the filsystem.
 */
time_t al_fs_entry_ctime(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return _al_fs_hook_entry_ctime(e);
}

/* Function: al_fs_entry_size
 * Returns the size of the entry.
 */
off_t al_fs_entry_size(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);

   return _al_fs_hook_entry_size(e);
}

/* Function: al_fs_entry_unlink
 * "unlink" or delete this file on disk.
 */
int32_t al_fs_entry_unlink(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);

   return _al_fs_hook_entry_unlink(e);
}

/* Function: al_fs_entry_exists
 * Check if the given entry exists on disk
 */
int32_t al_fs_entry_exists(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);

   return _al_fs_hook_entry_exists(e);
}

/* Function: al_fs_entry_isdir
 * Check if this entry is a directory
 */
int32_t al_fs_entry_isdir(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return al_fs_entry_mode(e) & AL_FM_ISDIR;
}

/* Function: al_fs_entry_isfile
 * Check if this entry is a file.
 */
int32_t al_fs_entry_isfile(ALLEGRO_FS_ENTRY *e)
{
   ASSERT(e != NULL);
   return al_fs_entry_mode(e) & AL_FM_ISFILE;
}

/* Function: al_fs_mktemp
 * Make a temporary randomly named file given a filename 'template' and 'ulink' flags
 *
 * 'template' is a string giving the format of the generated filename and should include one or more capital Xs.
 * Xs are replaced with random chars in the range 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789'
 *
 * 'ulink' is one of:
 * ALLEGRO_FS_MKTEMP_UNLINK_NOW - unlink now to create an anonymous temporary file
 * ALLEGRO_FS_MKTEMP_UNLINK_ON_CLOSE - unlink when entry is closed
 * ALLEGRO_FS_MKTEMP_UNLINK_NEVER - don't unlink
 */
ALLEGRO_FS_ENTRY *al_fs_mktemp(const char *template, uint32_t ulink)
{
   ASSERT(template != NULL);
   ASSERT(
      ulink == ALLEGRO_FS_MKTEMP_UNLINK_NOW ||
      ulink == ALLEGRO_FS_MKTEMP_UNLINK_ON_CLOSE ||
      ulink == ALLEGRO_FS_MKTEMP_UNLINK_NEVER
   );

   return _al_fs_hook_mktemp(template, ulink);
}

/* Function: al_fs_getcwd
 * Fill in 'buf' up to 'len' characters with the current working directory.
 *
 * If 'buf/len' is not large enough, the path may be truncated.
 */
int32_t al_fs_getcwd(char *buf, size_t len)
{
   ASSERT(buf != NULL);
   ASSERT(len != 0);

   return _al_fs_hook_getcwd(buf, len);
}

/* Function: al_fs_chdir
 * Changes the current working directory to 'path'.
 *
 * Returns -1 on error.
 */
int32_t al_fs_chdir(const char *path)
{
   ASSERT(path);

   return _al_fs_hook_chdir(path);
}

/* Function: al_fs_mkdir
 * Creates a new directory on disk given the path 'path'.
 *
 * Returns -1 on error.
 */
int32_t al_fs_mkdir(AL_CONST char *path)
{
   ASSERT(path);

   return _al_fs_hook_mkdir(path);
}

/* Function: al_fs_add_search_path
 * Adds a path to the list of directories to search for files when searching/opening files with a relative pathname.
 */
int32_t al_fs_add_search_path(const char *path)
{
   ASSERT(path);

   return _al_fs_hook_add_search_path(path);
}

/* Function: al_fs_search_path_count
 * Returns the number of items in the search path list.
 */
int32_t al_fs_search_path_count()
{
   return _al_fs_hook_search_path_count();
}

/* Function: al_fs_get_search_path
 * Fills in 'dest' up to 'len' bytes with the 'idx'th search path item.
 *
 * Warning, if 'dest/len' isn't large enough, path may be truncated.
 *
 * Parameters:
 *  idx - index of search path element requested
 *  dest - memory buffer to copy path to
 *  len - length of memory buffer
 *
 * Returns:
 *  -1 on error.
 */
int32_t al_fs_get_search_path(uint32_t idx, char *dest, size_t len)
{
   ASSERT(dest);
   ASSERT(len);

   return _al_fs_hook_get_search_path(idx, dest, len);
}

/* Function: al_fs_drive_sep
 * Fills in 'sep' up to 'len' characters with the drive separator string
 */
int32_t al_fs_drive_sep(size_t len, char *sep)
{
   ASSERT(len > 0);
   ASSERT(sep);

   return _al_fs_hook_drive_sep(len, sep);
}

/* Function: al_fs_path_sep
 * Fills in 'sep' up to 'len' characters with the path separator string.
 */
int32_t al_fs_path_sep(size_t len, char *sep)
{
   ASSERT(len > 0);
   ASSERT(sep);

   return _al_fs_hook_path_sep(len, sep);
}

/* not sure these two conversion hooks are needed, should the path conversion be in the driver? */
/* yup, driver may want to expose a "environment" that doesn't match the curren't platform's */
/* Function: al_fs_path_to_sys
 * Converts path 'orig' to system dependant format.
 */
int32_t al_fs_path_to_sys(const char *orig, size_t len, char *path)
{
   ASSERT(orig);
   ASSERT(len > 0);
   ASSERT(path);

   return _al_fs_hook_path_to_sys(orig, len, path);
}

/* Function: al_fs_path_to_uni
 * Converts path 'orig' to 'allegro' format.
 */
int32_t al_fs_path_to_uni(const char *orig, size_t len, char *path)
{
   ASSERT(orig);
   ASSERT(len > 0);
   ASSERT(path);

   return _al_fs_hook_path_to_uni(orig, len, path);
}

/* Function: al_fs_stat_mode
 * Returns stat 'mode' for fs entry 'path'.
 *
 * See Also:
 * <ALLEGRO_FS_MODE>
 */
uint32_t al_fs_stat_mode(const char *path)
{
   ASSERT(path != NULL);
   return _al_fs_hook_stat_mode(path);
}

/* Function: al_fs_stat_atime
 * Returns last access time for fs entry 'path'
 *
 * See Also:
 * <al_fs_entry_atime>
 */
time_t al_fs_stat_atime(const char *path)
{
   ASSERT(path != NULL);
   return _al_fs_hook_stat_atime(path);
}

/* Function: al_fs_stat_mtime
* Returns last modification time for fs entry 'path'
*
* See Also:
* <al_fs_entry_mtime>
*/
time_t al_fs_stat_mtime(const char *path)
{
   ASSERT(path != NULL);
   return _al_fs_hook_stat_mtime(path);
}

/* Function: al_fs_stat_ctime
* Returns creation time for fs entry 'path'
*
* See Also:
* <al_fs_entry_ctime>
*/
time_t al_fs_stat_ctime(const char *path)
{
   ASSERT(path != NULL);
   return _al_fs_hook_stat_ctime(path);
}

/* Function: al_fs_stat_size
* Returns file size for fs entry 'path'
*
* See Also:
* <al_fs_entry_size>
*/
off_t al_fs_stat_size(const char *path)
{
   ASSERT(path != NULL);
   return _al_fs_hook_stat_size(path);
}

/* Function: al_fs_unlink
 * Unlink 'path' entry from disk.
 *
 * See Also:
 * <al_fs_entry_unlink>
 */
int32_t al_fs_unlink(const char *path)
{
   ASSERT(path != NULL);
   return _al_fs_hook_unlink(path);
}

/* Function: al_fs_exists
 * Check if entry 'path' exists on disk.
 *
 * See Also:
 * <al_fs_entry_exists>
 */
int32_t al_fs_exists(const char *path)
{
   ASSERT(path != NULL);

   return _al_fs_hook_exists(path);
}

/* Function: al_fs_isdir
 * Return true if 'path' is a directory.
 *
 * See Also:
 * <al_fs_entry_isdir>
 */
int32_t al_fs_isdir(AL_CONST char *path)
{
   ASSERT(path != NULL);
   return _al_fs_hook_stat_mode(path) & AL_FM_ISDIR;
}

/* Function: al_fs_isfile
* Return true if 'path' is a file.
*
* See Also:
* <al_fs_entry_isfile>
*/
int32_t al_fs_isfile(AL_CONST char *path)
{
   ASSERT(path != NULL);
   return _al_fs_hook_stat_mode(path) & AL_FM_ISFILE;
}

/* Function: al_fs_entry_getc
 * read and return next byte in entry 'f'.
 * Returns EOF on end of file, and 0 on other errors (this is probably a bug)
 */
int al_fs_entry_getc(ALLEGRO_FS_ENTRY *f)
{
   uint8_t c = 0;
   ASSERT(f);

   if (al_fs_entry_read(&c, 1, f) != 1) {
      if (al_fs_entry_eof(f))
         return EOF;
   }

   return c;
}

/* Function: al_fs_entry_putc
 * write a single byte to entry.
 *
 * Parameters:
 *  c - byte value to write
 *  f - entry to write to
 *
 * Returns:
 *  EOF on error
 */
int al_fs_entry_putc(int c, ALLEGRO_FS_ENTRY *f)
{
   ASSERT(f);

   if (al_fs_entry_write(&c, 1, f) != 1) {
      if (al_fs_entry_error(f))
         return EOF;
   }

   return c;
}

/* Function: al_fs_entry_igetw
 * Reads a 16bit word in little-endian format (lsb first).
 *
 * Returns:
 * read 16bit word or EOF on error
 */
int16_t al_fs_entry_igetw(ALLEGRO_FS_ENTRY *f)
{
   int16_t b1 = 0, b2 = 0;
   ASSERT(f);

   if ((b1 = al_fs_entry_getc(f)) != EOF)
      if ((b2 = al_fs_entry_getc(f)) != EOF)
         return ((b2 << 8) | b1);

   return EOF;
}

/* Function: al_fs_entry_igetl
* Reads a 32bit word in little-endian format (lsb first).
*
* Returns:
* read 32bit word or EOF on error
*/
int32_t al_fs_entry_igetl(ALLEGRO_FS_ENTRY *f)
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

/* Function: al_fs_entry_iputw
* Writes a 16bit word in little-endian format (lsb first).
*
* Returns:
* written 16bit word or EOF on error
*/
int16_t al_fs_entry_iputw(int16_t w, ALLEGRO_FS_ENTRY *f)
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

/* Function: al_fs_entry_iputl
* Writes a 16bit word in little-endian format (lsb first).
*
* Returns:
* written 32bit word or EOF on error
*/
int32_t al_fs_entry_iputl(int32_t l, ALLEGRO_FS_ENTRY *f)
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

/* Function: al_fs_entry_mgetw
 * Reads a 16bit word in big-endian format (msb first)
 *
 * Returns:
 * read 16bit word or EOF on error
 */
int16_t al_fs_entry_mgetw(ALLEGRO_FS_ENTRY *f)
{
   int16_t b1 = 0, b2 = 0;
   ASSERT(f);

   if ((b1 = al_fs_entry_getc(f)) != EOF)
      if ((b2 = al_fs_entry_getc(f)) != EOF)
         return ((b1 << 8) | b2);

   return EOF;
}

/* Function: al_fs_entry_mgetl
* Writes a 32bit word in big-endian format (msb first)
*
* Returns:
* written 32bit word or EOF on error
*/
int32_t al_fs_entry_mgetl(ALLEGRO_FS_ENTRY *f)
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

/* Function: al_fs_entry_mputw
* Writes a 16bit word in big-endian format (msb first)
*
* Returns:
* written 16bit word or EOF on error
*/
int16_t al_fs_entry_mputw(int16_t w, ALLEGRO_FS_ENTRY *f)
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

/* Function: al_fs_entry_mputl
* Writes a 32bit word in big-endian format (msb first)
*
* Returns:
* written 32bit word or EOF on error
*/
int32_t al_fs_entry_mputl(int32_t l, ALLEGRO_FS_ENTRY *f)
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

/* Function: al_fs_entry_fgets
 * Reads a string of bytes terminated with a newline (\r,\n,\r\n).
 *
 * Parameters:
 * p - buffer to fill
 * max - maximum size of buffer
 * f - entry to read from
 *
 * Returns:
 * p
 */
char *al_fs_entry_fgets(char *p, size_t max, ALLEGRO_FS_ENTRY *f)
{
   char *pmax = NULL, *orig_p = p;
   int c = 0;
   ASSERT(f);

   al_set_errno(0);

   pmax = p+max - ucwidth(0);

   if ((c = al_fs_entry_getc(f)) == EOF) {
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
   } while ((c = al_fs_entry_getc(f)) != EOF);

   /* terminate the string */
   usetc(p, 0);

   if (c == '\0' || al_get_errno())
      return NULL;

   return orig_p; /* p has changed */
}

/* Function: al_fs_entry_fputs
 * Writes a string to file.
 *
 * Parameters:
 * p - string to write
 * f - file handle to write to
 *
 * Returns:
 * 0 on Success or -1 on error
 *
 * Note:
 * Function converts string to UTF8 before writing.
 */
int al_fs_entry_fputs(AL_CONST char *p, ALLEGRO_FS_ENTRY *f)
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

/* Function: al_fs_entry_ungetc
 * Ungets a single byte from a file. Does not write to file, it only places the char back into the entry's buffer
 */
int32_t al_fs_entry_ungetc(int32_t c, ALLEGRO_FS_ENTRY *fp)
{
   ASSERT(fp != NULL);

   return _al_fs_hook_entry_ungetc(c, fp);
}

/* maybe find a better place for this later */
static int32_t _al_find_resource_exists(const char *path, const char *base,
   const char *resource, uint32_t fm, char *buffer, size_t len)
{
   ALLEGRO_PATH *fp;
   int32_t ret = 0;

   memset(buffer, 0, len);

   fp = al_path_create(path);
   al_path_append(fp, base);

   if (resource) {
      ALLEGRO_PATH *resp = al_path_create(resource);
      al_path_concat(fp, resp);
      al_path_free(resp);
   }

   al_path_to_string(fp, buffer, len, ALLEGRO_NATIVE_PATH_SEP);
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

   al_path_free(fp);

   return ret;
}

/* Function: al_find_resource
 * Returns the full path of a 'resource' given mode 'fm'.
 *
 * Parameters:
 * base - sub directory name resource may be found in
 * resource - name of resource
 * fm - file mode/type of resource to match. See <ALLEGRO_FS_MODE>
 * buffer - buffer to write full path to
 * len - length of buffer
 *
 * Notes:
 * If asking for a read-only resource function scans the user data path, the program path,
 * the current working directory followed by the system data path, if it doesn't find anything,
 * it will return a bogus path in the users data path. This is probably a bug.
 *
 * If asking for a writable resource, it will scan the same places as the read only scan,
 * and will pick the first wirteable existing resource it finds.
 * If it doesn't find any pre existing resource, it finds the first writable directory path
 * made up of the path components of the path returned from <al_get_path>, the 'base', and 'resource'
 * arguments, if none are writable, it will return a path made up of the USER_DATA_PATH, and the
 * 'base' and 'resource' arguments.
 *
 * If someone can make this more usefull and faster, please do (it causes noticible startup lag with the demo).
 */
char *al_find_resource(const char *base, const char *resource, uint32_t fm,
   char *buffer, size_t len)
{
   ALLEGRO_PATH *path;
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
   path = al_path_create(tmp);
   al_path_append(path, base_new);

   if (resource) {
      ALLEGRO_PATH *resp = al_path_create(resource);
      al_path_concat(path, resp);
      al_path_free(resp);
   }

   al_path_to_string(path, buffer, len, ALLEGRO_NATIVE_PATH_SEP);
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
