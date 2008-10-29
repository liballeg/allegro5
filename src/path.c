#include <stdio.h>
#ifdef _MSC_VER
   #define _POSIX_
#endif
#include <limits.h>
#include "allegro5/debug.h"
#include "allegro5/fshook.h"
#include "allegro5/path.h"
#include "allegro5/unicode.h"
#include "allegro5/internal/aintern_memory.h"
#include "allegro5/internal/aintern_path.h"


static char *_ustrduprange(const char *start, const char *end)
{
   const char *ptr = start;
   int blen = 0;
   char *copy;
   int eslen = ustrsizez(empty_string);

   ptr = start;
   blen = 0;
   while (ptr < end) {
      int c = ugetxc(&ptr);
      blen += ucwidth(c);
   }

   copy = _AL_MALLOC(blen + eslen + 1);
   ASSERT(copy);
   if (!copy) {
      return NULL;
   }

   memset(copy, 0, blen + eslen + 1);
   memcpy(copy, start, blen);

   return copy;
}

static void _ustrcpyrange(char *dest, const char *start, const char *end)
{
   const char *ptr = start;
   int blen = 0;

   while (ptr < end) {
      int c = ugetxc(&ptr);
      blen += ucwidth(c);
   }

   memset(dest, 0, blen + ustrsizez(empty_string) + 1);
   memcpy(dest, start, blen);
}

static void _fix_slashes(char *path)
{
   char *ptr = path;
   int cc = ugetc(ptr);

   while (cc) {
      if (cc == '\\')
         usetc(ptr, '/');

      ptr++;
      cc = ugetc(ptr);
   }
}

// last component if its not followed by a `dirsep` is interpreted as a file component, if it is not a . or ..
static int32_t _parse_path(const char *p, char **drive, char **path, char **file)
{
   const char *ptr = p;
   char *path_info_end;
   int32_t path_info_len;
   const char dirsep = '/';
#ifdef ALLEGRO_WINDOWS
   const char drivesep = ':';
#endif

#ifdef ALLEGRO_WINDOWS
   // UNC \\server\share name
   if (ugetc(ptr) == dirsep && ugetat(ptr, 1) == dirsep) {
      char *uncdrive = NULL;

      ptr += uwidth(ptr);
      ptr += uwidth(ptr);

      ptr = ustrchr(ptr, dirsep);
      if (!ptr)
         goto _parse_path_error;

      if (!ugetc(ptr)) {
         goto _parse_path_error;
      }

      ptr += uwidth(ptr);
      if (!ugetc(ptr))
         goto _parse_path_error;

      ptr = ustrchr(ptr, dirsep);
      if (!ptr) {
         ptr = p + ustrsize(p);
      }

      uncdrive = _ustrduprange(p, ptr);
      if (!uncdrive) {
         goto _parse_path_error;
      }

      *drive = uncdrive;
   }
   // drive name
   else if (ugetc(ptr) != dirsep && ugetat(ptr, 1) == drivesep) {
      char *tmp = NULL;

      tmp = _ustrduprange(ptr, ptr+uoffset(ptr, 2));
      if (!tmp) {
         // ::)
         goto _parse_path_error;
      }

      *drive = tmp;
      ptr = ustrchr(ptr+uoffset(ptr,1), dirsep);
   }

   if (!ptr || !ugetc(ptr)) {
      // no path, or file info
      return 0;
   }
#endif

   // grab path, and/or file info

   path_info_end = ustrrchr(ptr, dirsep);
   if (!path_info_end) { // no path, just file
      char *file_info = NULL;

      file_info = ustrdup(ptr);
      if (!file_info)
         goto _parse_path_error;

      /* if the last bit is a . or .., its actually a dir, not a file */
      if (ustrcmp(file_info, ".") == 0 || ustrcmp(file_info, "..") == 0) {
         *path = file_info;
         return 0;
      }

      *file = file_info;
      return 0;
   }
   else if (ugetat(path_info_end, 1)) {
      // dirsep is not at end
      const char *tmp_file_info = path_info_end + uoffset(path_info_end, 1);

      /* if the last bit is a . or .., its actually a dir, not a file */
      if (ustrcmp(tmp_file_info, ".") == 0 ||
            ustrcmp(tmp_file_info, "..") == 0)
      {
         path_info_end = (char *)(ptr + ustrsize(ptr));
      }
      else {
         char *file_info = ustrdup(tmp_file_info);
         if (!file_info)
            goto _parse_path_error;

	 *file = file_info;
      }
   }

   /* detect if the path_info should be a single '/' */
   if (ugetc(ptr) == dirsep && (!ugetat(ptr, 2) || ptr == path_info_end)) {
      path_info_end += ucwidth(dirsep);
   }

   path_info_len = path_info_end - ptr;
   if (path_info_len) {
      char *path_info = _ustrduprange(ptr, path_info_end);
      if (!path_info)
         goto _parse_path_error;

      *path = path_info;
   }

   return 0;

_parse_path_error:
   if (*drive) {
      _AL_FREE(*drive);
      *drive = NULL;
   }

   if (*path) {
      _AL_FREE(*path);
      *path = NULL;
   }

   if (*file) {
      _AL_FREE(*file);
      *file = NULL;
   }

   return -1;
}

static char **_split_path(char *path, int32_t *num)
{
   char **arr, *ptr, *lptr = NULL;
   int count = 0, i = 0;

   for (ptr=path; ugetc(ptr); ptr+=ucwidth(*ptr)) {
      if (ugetc(ptr) == '/')
         count++;
      lptr = ptr;
   }

   if (ugetc(lptr) != '/') {
      count++;
   }

   arr = _AL_MALLOC(sizeof(char *) * count);
   if (!arr)
      return NULL;

   lptr = path;
   for (ptr=path; ugetc(ptr); ptr++) {
      if (ugetc(ptr) == '/') {
         char *seg = _ustrduprange(lptr, ptr);
         if (!seg)
            goto _split_path_getout;

         arr[i++] = seg;
         lptr = ptr+uoffset(ptr, 1);
      }
   }

   if (ugetc(lptr)) {
      char *seg = _ustrduprange(lptr, ptr);
      if (!seg)
         goto _split_path_getout;

      arr[i++] = seg;
   }

   *num = count;
   return arr;

_split_path_getout:

   if (arr) {
      for (i = 0; arr[i]; i++) {
         _AL_FREE(arr[i]);
      }

      _AL_FREE(arr);
   }

   return NULL;
}

static int32_t _al_path_init(ALLEGRO_PATH *path, const char *str)
{
   char *part_path = NULL;
   char *orig;

   memset(path, 0, sizeof(ALLEGRO_PATH));

   if (str == NULL) {
      /* Empty path. */
      return 1;
   }

   orig = ustrdup(str);
   if (!orig)
      goto _path_init_fatal;

   _fix_slashes(orig);

   if (_parse_path(orig, &(path->drive), &part_path, &(path->filename)) != 0)
      goto _path_init_fatal;

   _AL_FREE(orig);
   orig = NULL;

   if (part_path) {
      path->segment = _split_path(part_path, &(path->segment_count));
      if (!path->segment)
         goto _path_init_fatal;
   }

   return 1;

_path_init_fatal:

   if (path->segment) {
      int i = 0;
      for (i = 0; i < path->segment_count; i++)
         _AL_FREE(path->segment[i]);

      _AL_FREE(path->segment);
   }

   _AL_FREE(path->drive);
   _AL_FREE(path->filename);
   _AL_FREE(part_path);
   _AL_FREE(orig);

   return 0;
}

ALLEGRO_PATH *al_path_create(const char *str)
{
   ALLEGRO_PATH *path;

   path = _AL_MALLOC(sizeof(ALLEGRO_PATH));
   if (!path)
      return NULL;

   if (!_al_path_init(path, str)) {
      _AL_FREE(path);
      return NULL;
   }

   return path;
}

int al_path_num_components(ALLEGRO_PATH *path)
{
   ASSERT(path);
   return path->segment_count;
}

const char *al_path_index(ALLEGRO_PATH *path, int i)
{
   ASSERT(path);
   ASSERT(i < path->segment_count);

   if (i < 0)
      i = path->segment_count + i;

   ASSERT(i >= 0);

   return path->segment[i];
}

void al_path_replace(ALLEGRO_PATH *path, int i, const char *s)
{
   char *tmp = NULL;

   ASSERT(path);
   ASSERT(s);
   ASSERT(i < path->segment_count);

   if (i < 0)
      i = path->segment_count + i;

   ASSERT(i >= 0);

   tmp = ustrdup(s);
   if (!tmp)
      return;

   _AL_FREE(path->segment[i]);
   path->segment[i] = tmp;
}

void al_path_remove(ALLEGRO_PATH *path, int i)
{
   ASSERT(path);
   ASSERT(i < path->segment_count);

   if (i < 0)
      i = path->segment_count + i;

   ASSERT(i >= 0);

   _AL_FREE(path->segment[i]);
   memmove(path->segment+i, path->segment+i+1, sizeof(char *)*(path->segment_count-i-1));
   path->segment = (char **)_AL_REALLOC(path->segment, (path->segment_count-1) * sizeof(char *));
   /* I don't think realloc can fail if its just shrinking the existing block,
      so I'm not bothering to check for it, or work around it */

   path->segment_count--;
}

void al_path_insert(ALLEGRO_PATH *path, int i, const char *s)
{
   char *copy_s;
   char **tmp;

   ASSERT(path);
   ASSERT(i < path->segment_count);

   if (i < 0)
      i = path->segment_count - (-i) + 1;

   ASSERT(i >= 0);

   copy_s = ustrdup(s);
   ASSERT(copy_s);
   if (!copy_s) {
      /* XXX what to do? */
      return;
   }

   tmp = _AL_REALLOC(path->segment, sizeof(char *) * (path->segment_count+1));
   ASSERT(tmp);
   if (!tmp) {
      /* XXX what to do? */
      return;
   }

   path->segment = tmp;

   memmove(path->segment+i, path->segment+i+1, path->segment_count - i);
   path->segment[i] = copy_s;
   path->segment_count++;
}

const char *al_path_tail(ALLEGRO_PATH *path)
{
   ASSERT(path);

   /* XXX handle empty path */
   return al_path_index(path, -1);
}

void al_path_drop_tail(ALLEGRO_PATH *path)
{
   /* XXX handle empty path */
   al_path_remove(path, -1);
}

void al_path_append(ALLEGRO_PATH *path, const char *s)
{
   al_path_insert(path, -1, s);
}

void al_path_concat(ALLEGRO_PATH *path, const ALLEGRO_PATH *tail)
{
   char *new_filename;
   char **tmp;
   int i;

   ASSERT(path);
   ASSERT(tail);

   if (tail->drive)
      /* don't bother concating if the tail is an absolute path */
      return;

   if (tail->segment_count) {
      if (tail->segment[0] && ustrcmp(tail->segment[0], "")==0) {
         /* if the first segment is empty, we have an absolute path */
         return;
      }
   }

   if (tail->filename) {
      new_filename = ustrdup(tail->filename);
      if (!new_filename)
         return;

      if (path->filename) _AL_FREE(path->filename);
      path->filename = new_filename;
   }

   tmp = _AL_REALLOC(path->segment, (path->segment_count + tail->segment_count) * sizeof(char *));
   if (!tmp)
      return;

   path->segment = tmp;

   memset(tmp+path->segment_count, 0, sizeof(char*)*tail->segment_count);

   for (i = 0; i < tail->segment_count; i++) {
      char *seg = ustrdup(tail->segment[i]);
      if (!seg) {
         i--;
         break;
      }

      tmp[path->segment_count+i] = seg;
   }

   path->segment_count += i;
}

/* FIXME: this is a rather dumb implementation, not using ustr*cat should speed it up */
char *al_path_to_string(ALLEGRO_PATH *path, char *buffer, size_t len, char delim)
{
   const char delims[2] = { delim, '\0' };
   int32_t i;

   ASSERT(path);
   ASSERT(buffer);

   memset(buffer, 0, len);

   if (path->drive) {
      size_t drive_size = ustrlen(path->drive);
      ustrzncat(buffer, len, path->drive, drive_size);
      ustrzncat(buffer, len, delims, 1);
   }

   for (i=0; i < path->segment_count; i++) {
      ustrzncat(buffer, len, path->segment[i], ustrlen(path->segment[i]));
      ustrzncat(buffer, len, delims, 1);
   }

   if (path->filename) {
      ustrzncat(buffer, len, path->filename, ustrlen(path->filename));
   }

   return buffer;
}

void al_path_free(ALLEGRO_PATH *path)
{
   int32_t i;

   if (!path) {
      return;
   }

   if (path->drive) {
      _AL_FREE(path->drive);
      path->drive = NULL;
   }

   if (path->filename) {
      _AL_FREE(path->filename);
      path->filename = NULL;
   }

   for (i = 0; i < path->segment_count; i++) {
      _AL_FREE(path->segment[i]);
      path->segment[i] = NULL;
   }

   _AL_FREE(path->segment);
   path->segment = NULL;

   path->segment_count = 0;

   _AL_FREE(path);
}

int32_t al_path_set_drive(ALLEGRO_PATH *path, const char *drive)
{
   ASSERT(path);

   if (path->drive) {
      _AL_FREE(path->drive);
      path->drive = NULL;
   }

   if (drive) {
      path->drive = ustrdup(drive);
      if (!path->drive)
         return -1;
   }

   return 0;
}

const char *al_path_get_drive(ALLEGRO_PATH *path)
{
   ASSERT(path);
   return path->drive;
}

int32_t al_path_set_filename(ALLEGRO_PATH *path, const char *filename)
{
   ASSERT(path);

   if (path->filename) {
      _AL_FREE(path->filename);
      path->filename = NULL;
   }

   if (filename) {
      path->filename = ustrdup(filename);
      if (!path->filename)
         return -1;
   }

   return 0;
}

const char *al_path_get_filename(ALLEGRO_PATH *path)
{
   ASSERT(path);
   return path->filename;
}

const char *al_path_get_extension(ALLEGRO_PATH *path, char *buf, size_t len)
{
   ASSERT(path);
   ASSERT(buf);

   memset(buf, 0, len);

   if (path->filename) {
      char *ext = ustrrchr(path->filename, '.');
      if (ext) {
         ustrncpy(buf, ext, len);
      }
   }

   return buf;
}

const char *al_path_get_basename(ALLEGRO_PATH *path, char *buf, size_t len)
{
   ASSERT(path);
   ASSERT(buf);

   memset(buf, 0, len);

   if (path->filename) {
      char *ext = ustrrchr(path->filename, '.');
      if (ext) {
         _ustrcpyrange(buf, path->filename, ext);
      }
   }

   return buf;
}

uint32_t al_path_exists(ALLEGRO_PATH *path)
{
   char buffer[PATH_MAX];
   ASSERT(path);

   al_path_to_string(path, buffer, PATH_MAX, ALLEGRO_NATIVE_PATH_SEP);

   /* Windows' stat() doesn't like the slash at the end of the path when
    * the path is pointing to a directory. There are other places which
    * might require the same fix.
    */
#ifdef ALLEGRO_WINDOWS
   if (ugetat(buffer, ustrlen(buffer) - 1) == '\\' &&
      al_path_num_components(path))
   {
      usetc(buffer + ustrlen(buffer) - 1, '\0');
   }
#endif

   return al_fs_exists(buffer);
}

bool al_path_emode(ALLEGRO_PATH *path, uint32_t mode)
{
   char buffer[PATH_MAX];
   ASSERT(path);

   al_path_to_string(path, buffer, PATH_MAX, ALLEGRO_NATIVE_PATH_SEP);

   return (al_fs_stat_mode(buffer) & mode) == mode;
}

/* vim: set sts=3 sw=3 et: */
