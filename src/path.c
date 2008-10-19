#include <stdio.h>
#ifdef _MSC_VER
   #define _POSIX_
#endif
#include <limits.h>
#include "allegro5/debug.h"
#include "allegro5/path.h"
#include "allegro5/unicode.h"
#include "allegro5/internal/aintern_memory.h"
#include "allegro5/fshook.h"

static char *_ustrduprange(const char *start, const char *end)
{
   const char *ptr = start;
   char *dup = NULL;
   int32_t blen = 0, eslen = ustrsizez(empty_string);

   while (ptr < end) {
      int32_t c = ugetxc(&ptr);
      blen += ucwidth(c);
   }

   dup = _AL_MALLOC(blen+eslen+1);
   if (!dup) {
      return NULL;
   }

   memset(dup, 0, blen+eslen+1);
   memcpy(dup, start, blen);

   return dup;
}

static void _ustrcpyrange(char *dest, const char *start, const char *end)
{
   const char *ptr = start;
   int32_t blen = 0, eslen = ustrsizez(empty_string);

   while (ptr < end) {
      int32_t c = ugetxc(&ptr);
      blen += ucwidth(c);
   }

   memset(dest, 0, blen+eslen+1);
   memcpy(dest, start, blen);

}

static void _fix_slashes(char *path)
{
   char *ptr = path;
   uint32_t cc = ugetc(ptr);

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
   char *path_info = NULL, *path_info_end = NULL;
   int32_t path_info_len = 0;
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

   //if (ptr)
    //  ptr+=uoffset(ptr, 1);
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
      char *file_info = NULL;
      char *tmp_file_info = path_info_end+uoffset(path_info_end, 1);

      /* if the last bit is a . or .., its actually a dir, not a file */
      if (ustrcmp(tmp_file_info, ".") == 0 || ustrcmp(tmp_file_info, "..") == 0) {
            path_info_end = (char *)(ptr + ustrsize(ptr));
      }
      else {
         file_info = ustrdup(tmp_file_info);
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
      path_info = _ustrduprange(ptr, path_info_end);
      if (!path_info)
         goto _parse_path_error;

      *path = path_info;
   }

   return 0;

_parse_path_error:
   if (*drive) {
      free(*drive);
      *drive = 0;
   }

   if (*path) {
      free(*path);
      *path = 0;
   }

   if (*file) {
      free(*file);
      *file = 0;
   }

   return -1;
}

static char **_split_path(char *path, int32_t *num)
{
   char **arr = NULL, *ptr = NULL, *lptr = NULL;
   int32_t count = 0, i = 0;

   //printf("_split_path: '%s'\n", path);

   for (ptr=path; ugetc(ptr); ptr+=ucwidth(*ptr)) {
      if (ugetc(ptr) == '/')
         count++;
      lptr = ptr;
   }

   if (ugetc(lptr) != '/') {
      count++;
   }

   arr = malloc(sizeof(char *) * count);
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
         free((void*)arr[i]);
      }

      free(arr);
   }

   return NULL;
}

static int32_t _al_path_init(AL_PATH *path, AL_CONST char *str)
{
   memset(path, 0, sizeof(AL_PATH));

   if (str != NULL) {
      char *part_path = NULL;
      char *orig = ustrdup(str);
      if (!orig)
         goto _path_init_fatal;

      _fix_slashes(orig);

      //if (ugetc(orig) == '/' || ugetat(orig, 1) == ':') {
      if (_parse_path(orig, &(path->drive), &part_path, &(path->filename))!=0)
         goto _path_init_fatal;

      //			printf("drive:'%s', path:'%s', file:'%s'\n", path->drive, part_path, path->filename);

      free(orig);
      orig = NULL;
      //} else {
      //	part_path = orig;
      //	orig = NULL;
      //}

      /*{
        char buffer[1024], *c = NULL;
        usprintf(buffer, "_al_path_init: drive:'%s', filename:'%s', part_path:'%s'", path->drive?path->drive:"(null)", path->filename?path->filename:"(null)", part_path?part_path:"(null)");
        c = uconvert(buffer, U_CURRENT, NULL, U_ASCII, 0);
        printf("%s\n", c);
        }*/

      if (part_path) {
         path->segment = _split_path(part_path, &(path->segment_count));
         if (!path->segment)
            goto _path_init_fatal;
      }

      return 1;

_path_init_fatal:
      if (path->segment) {
         int32_t i = 0;
         for (i = 0; i < path->segment_count; i++)
            free(path->segment[i]);

         free(path->segment);
      }

      if (path->drive)
         free(path->drive);

      if (path->filename)
         free(path->filename);

      if (part_path)
         free(part_path);

      if (orig)
         free(orig);

      return 0;
   }

   return 1;
}

AL_PATH *al_path_create(AL_CONST char *str)
{
   AL_PATH *path = NULL;

   path = _AL_MALLOC(sizeof(AL_PATH));
   if (!path)
      return NULL;

   if (!_al_path_init(path, str)) {
      free(path);
      return NULL;
   }

   path->free = 1;

   return path;
}

int32_t al_path_init(AL_PATH *path, AL_CONST char *str)
{
   ASSERT(path);
   return _al_path_init(path, str);
}


int al_path_num_components(AL_PATH *path)
{
   ASSERT(path);
   return path->segment_count;
}

const char *al_path_index(AL_PATH *path, int i)
{
   ASSERT(path);
   ASSERT(i < path->segment_count);

   if (i < 0)
      i = path->segment_count + i;

   ASSERT(i >= 0);

   return path->segment[i];
}

void al_path_replace(AL_PATH *path, int i, const char *s)
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

void al_path_remove(AL_PATH *path, int i)
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
}

void al_path_insert(AL_PATH *path, int i, const char *s)
{
   char **tmp = NULL;
   char *tmps = NULL;

   ASSERT(path);
   ASSERT(i < path->segment_count);

   if (i < 0)
      i = path->segment_count - (-i) + 1;

   ASSERT(i >= 0);

   tmps = ustrdup(s);
   if (!tmps)
      return;

   tmp = _AL_REALLOC(path->segment, sizeof(char *) * (path->segment_count+1));
   if (!tmp)
      return;

   path->segment = tmp;

   //printf("path_insert[%i]: '%s'\n", i, tmps);

   memmove(path->segment+i, path->segment+i+1, path->segment_count - i);
   path->segment[i] = tmps;
   path->segment_count++;

}

const char *al_path_tail(AL_PATH *path)
{
   ASSERT(path);

   return al_path_index(path, -1);
}

void al_path_drop_tail(AL_PATH *path)
{
   al_path_remove(path, -1);
}

void al_path_append(AL_PATH *path, const char *s)
{
   al_path_insert(path, -1, s);
}

void al_path_concat(AL_PATH *path, const AL_PATH *tail)
{
   char **tmp = NULL, *new_filename = NULL;
   int32_t i = 0;

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
char *al_path_to_string(AL_PATH *path, char *buffer, size_t len, char delim)
{
   int32_t i = 0;
   char d[2] = { delim, '\0' };

   ASSERT(path);
   ASSERT(buffer);

   memset(buffer, 0, len);

   if (path->drive) {
      size_t drive_size = ustrlen(path->drive);
      ustrzncat(buffer, len, path->drive, drive_size);
      ustrzncat(buffer, len, d, 1);
   }

   //printf("path->segment_count: %i\n", path->segment_count);
   for (i=0; i < path->segment_count; i++) {
      //printf("segment: '%p'\n", path->segment[i]);
      ustrzncat(buffer, len, path->segment[i], ustrlen(path->segment[i]));
      ustrzncat(buffer, len, d, 1);
   }

   if (path->filename) {
      ustrzncat(buffer, len, path->filename, ustrlen(path->filename));
   }

   return buffer;
}

void al_path_free(AL_PATH *path)
{
   int32_t i = 0;

   ASSERT(path);

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

   if (path->free)
      _AL_FREE(path);
}

int32_t al_path_set_drive(AL_PATH *path, AL_CONST char *drive)
{
   ASSERT(path);

   if (path->drive) {
      free(path->drive);
      path->drive = NULL;
   }

   if (drive) {
      path->drive = ustrdup(drive);
      if (!path->drive)
         return -1;
   }

   return 0;
}

const char *al_path_get_drive(AL_PATH *path)
{
   ASSERT(path);
   return path->drive;
}

int32_t al_path_set_filename(AL_PATH *path, AL_CONST char *filename)
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

const char *al_path_get_filename(AL_PATH *path)
{
   ASSERT(path);
   return path->filename;
}

const char *al_path_get_extension(AL_PATH *path, char *buf, size_t len)
{
   ASSERT(path);
   ASSERT(buf);

   memset(buf, 0, len);

   if (path->filename) {
      char *ext = NULL;

      ext = ustrrchr(path->filename, '.');
      if (ext) {
         ustrncpy(buf, ext, len);
      }
   }

   return buf;
}

const char *al_path_get_basename(AL_PATH *path, char *buf, size_t len)
{
   ASSERT(path);
   ASSERT(buf);

   memset(buf, 0, len);

   if (path->filename) {
      char *ext = NULL;

      ext = ustrrchr(path->filename, '.');
      if (ext) {
         _ustrcpyrange(buf, path->filename, ext);
      }
   }

   return buf;
}

uint32_t al_path_exists(AL_PATH *path)
{
   char buffer[PATH_MAX];
   ASSERT(path);

   al_path_to_string(path, buffer, PATH_MAX, ALLEGRO_NATIVE_PATH_SEP);

   /* Windows' stat() doesn't like the slash at the end of the path when
    * the path is pointing to a directory. There are other places which
    * might require the same fix.*/
#ifdef ALLEGRO_WINDOWS
   if (ugetat(buffer, ustrlen(buffer) - 1) == '\\' &&
       al_path_num_components(path)) {
      usetc(buffer + ustrlen(buffer) - 1, '\0');
   }
#endif

   return al_fs_exists(buffer);
}

uint32_t al_path_emode(AL_PATH *path, uint32_t mode)
{
   char buffer[PATH_MAX];
   ASSERT(path);

   al_path_to_string(path, buffer, PATH_MAX, ALLEGRO_NATIVE_PATH_SEP);

   return (al_fs_stat_mode(buffer) & mode) == mode;
}

/* vim: set sts=3 sw=3 et: */
