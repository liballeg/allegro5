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
 *      Filesystem path routines.
 *
 *      By Thomas Fjellstrom.
 *
 *      See LICENSE.txt for copyright information.
 */

/* Title: Filesystem path routines
 */

#include <stdio.h>
#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_memory.h"
#include "allegro5/internal/aintern_vector.h"
#include "allegro5/internal/aintern_path.h"


/* get_segment:
 *  Return the i'th directory component of a path.
 */
static ALLEGRO_USTR *get_segment(const ALLEGRO_PATH *path, unsigned i)
{
   ALLEGRO_USTR **seg = _al_vector_ref(&path->segments, i);
   return *seg;
}


/* get_segment_cstr:
 *  Return the i'th directory component of a path as a C string.
 */
static const char *get_segment_cstr(const ALLEGRO_PATH *path, unsigned i)
{
   return al_cstr(get_segment(path, i));
}


/* replace_backslashes:
 *  Replace backslashes by slashes.
 */
static void replace_backslashes(ALLEGRO_USTR *path)
{
   al_ustr_find_replace_cstr(path, 0, "\\", "/");
}


/* parse_path_string:
 *
 * Parse a path string according to the following grammar.  The last
 * component, if it is not followed by a directory separator, is interpreted
 * as the filename component, unless it is "." or "..".
 *
 * GRAMMAR
 *
 * path     ::=   "//" c+ "/" nonlast        [Windows only]
 *            |   c ":" nonlast              [Windows only]
 *            |   nonlast
 *
 * nonlast  ::=   c* "/" nonlast
 *            |   last
 *
 * last     ::=   "."
 *            |   ".."
 *            |   filename
 *
 * filename ::=   c*                         [but not "." and ".."]
 *
 * c        ::=   any character but '/'
 */
static bool parse_path_string(const ALLEGRO_USTR *str, ALLEGRO_PATH *path)
{
   ALLEGRO_USTR_INFO    dot_info;
   ALLEGRO_USTR_INFO    dotdot_info;
   const ALLEGRO_USTR *  dot = al_ref_cstr(&dot_info, ".");
   const ALLEGRO_USTR *  dotdot = al_ref_cstr(&dotdot_info, "..");

   ALLEGRO_USTR *piece = al_ustr_new("");
   int pos = 0;
   bool on_windows;

   /* We compile the drive handling code on non-Windows platforms to prevent
    * it becoming broken.
    */
#ifdef ALLEGRO_WINDOWS
   on_windows = true;
#else
   on_windows = false;
#endif
   if (on_windows) {
      /* UNC \\server\share name */
      if (al_ustr_has_prefix_cstr(str, "//")) {
         int slash = al_ustr_find_chr(str, 2, '/');
         if (slash == -1 || slash == 2) {
            /* Missing slash or server component is empty. */
            goto Error;
         }
         al_ustr_assign_substr(path->drive, str, pos, slash);
         pos = slash + 1;
      }
      else {
         /* Drive letter. */
         int colon = al_ustr_offset(str, 1);
         if (colon > -1 && al_ustr_get(str, colon) == ':') {
            /* Include the colon in the drive string. */
            al_ustr_assign_substr(path->drive, str, 0, colon + 1);
            pos = colon + 1;
         }
      }
   }

   for (;;) {
      int slash = al_ustr_find_chr(str, pos, '/');

      if (slash == -1) {
         /* Last component. */
         al_ustr_assign_substr(piece, str, pos, al_ustr_size(str));
         if (al_ustr_equal(piece, dot) || al_ustr_equal(piece, dotdot)) {
            al_path_append(path, al_cstr(piece));
         }
         else {
            /* This might be an empty string, but that's okay. */
            al_ustr_assign(path->filename, piece);
         }
         break;
      }

      /* Non-last component. */
      al_ustr_assign_substr(piece, str, pos, slash);
      al_path_append(path, al_cstr(piece));
      pos = slash + 1;
   }

   al_ustr_free(piece);
   return true;

Error:

   al_ustr_free(piece);
   return false;
}


/* Function: al_path_create
 */
ALLEGRO_PATH *al_path_create(const char *str)
{
   ALLEGRO_PATH *path;

   path = _AL_MALLOC(sizeof(ALLEGRO_PATH));
   if (!path)
      return NULL;

   path->drive = al_ustr_new("");
   path->filename = al_ustr_new("");
   _al_vector_init(&path->segments, sizeof(ALLEGRO_USTR *));
   path->basename = al_ustr_new("");
   path->full_string = al_ustr_new("");

   if (str != NULL) {
      ALLEGRO_USTR *copy = al_ustr_new(str);
      replace_backslashes(copy);

      if (!parse_path_string(copy, path)) {
         al_path_free(path);
         path = NULL;
      }

      al_ustr_free(copy);
   }

   return path;
}

/* Function: al_path_create_dir
 */
ALLEGRO_PATH *al_path_create_dir(const char *str)
{
   ALLEGRO_PATH *path = al_path_create(str);
   if (al_ustr_length(path->filename)) {
      ALLEGRO_USTR *last = path->filename;
      path->filename = al_ustr_new("");
      al_path_append(path, al_cstr(last));
      al_ustr_free(last);
   }
   return path;
}


/* Function: al_path_clone
 */
ALLEGRO_PATH *al_path_clone(const ALLEGRO_PATH *path)
{
   ALLEGRO_PATH *clone;
   unsigned int i;

   ASSERT(path);

   clone = al_path_create(NULL);
   if (!clone) {
      return NULL;
   }

   al_ustr_assign(clone->drive, path->drive);
   al_ustr_assign(clone->filename, path->filename);

   for (i = 0; i < _al_vector_size(&path->segments); i++) {
      ALLEGRO_USTR **slot = _al_vector_alloc_back(&clone->segments);
      (*slot) = al_ustr_dup(get_segment(path, i));
   }

   return clone;
}


/* Function: al_path_num_components
 */
int al_path_num_components(const ALLEGRO_PATH *path)
{
   ASSERT(path);

   return _al_vector_size(&path->segments);
}


/* Function: al_path_index
 */
const char *al_path_index(const ALLEGRO_PATH *path, int i)
{
   ASSERT(path);
   ASSERT(i < (int)_al_vector_size(&path->segments));

   if (i < 0)
      i = _al_vector_size(&path->segments) + i;

   ASSERT(i >= 0);

   return get_segment_cstr(path, i);
}


/* Function: al_path_replace
 */
void al_path_replace(ALLEGRO_PATH *path, int i, const char *s)
{
   ASSERT(path);
   ASSERT(s);
   ASSERT(i < (int)_al_vector_size(&path->segments));

   if (i < 0)
      i = _al_vector_size(&path->segments) + i;

   ASSERT(i >= 0);

   al_ustr_assign_cstr(get_segment(path, i), s);
}


/* Function: al_path_remove
 */
void al_path_remove(ALLEGRO_PATH *path, int i)
{
   ASSERT(path);
   ASSERT(i < (int)_al_vector_size(&path->segments));

   if (i < 0)
      i = _al_vector_size(&path->segments) + i;

   ASSERT(i >= 0);

   al_ustr_free(get_segment(path, i));
   _al_vector_delete_at(&path->segments, i);
}


/* Function: al_path_insert
 */
void al_path_insert(ALLEGRO_PATH *path, int i, const char *s)
{
   ALLEGRO_USTR **slot;
   ASSERT(path);
   ASSERT(i <= (int)_al_vector_size(&path->segments));

   if (i < 0)
      i = _al_vector_size(&path->segments) + i;

   ASSERT(i >= 0);

   slot = _al_vector_alloc_mid(&path->segments, i);
   (*slot) = al_ustr_new(s);
}


/* Function: al_path_tail
 */
const char *al_path_tail(const ALLEGRO_PATH *path)
{
   ASSERT(path);

   if (al_path_num_components(path) == 0)
      return NULL;

   return al_path_index(path, -1);
}


/* Function: al_path_drop_tail
 */
void al_path_drop_tail(ALLEGRO_PATH *path)
{
   if (al_path_num_components(path) > 0) {
      al_path_remove(path, -1);
   }
}


/* Function: al_path_append
 */
void al_path_append(ALLEGRO_PATH *path, const char *s)
{
   ALLEGRO_USTR **slot = _al_vector_alloc_back(&path->segments);
   (*slot) = al_ustr_new(s);
}


static bool path_is_absolute(const ALLEGRO_PATH *path)
{
   /* If the first segment is empty, we have an absolute path. */
   return (_al_vector_size(&path->segments) > 0)
      && (al_ustr_size(get_segment(path, 0)) == 0);
}


/* Function: al_path_concat
 */
bool al_path_concat(ALLEGRO_PATH *path, const ALLEGRO_PATH *tail)
{
   unsigned i;
   ASSERT(path);
   ASSERT(tail);

   /* Don't bother concating if the tail is an absolute path. */
   if (path_is_absolute(tail)) {
      return false;
   }

   /* We ignore tail->drive.  The other option is to do nothing if tail
    * contains a drive letter.
    */

   al_ustr_assign(path->filename, tail->filename);

   for (i = 0; i < _al_vector_size(&tail->segments); i++) {
      al_path_append(path, get_segment_cstr(tail, i));
   }

   return true;
}


static void path_to_ustr(const ALLEGRO_PATH *path, int32_t delim,
   ALLEGRO_USTR *str)
{
   unsigned i;

   al_ustr_assign(str, path->drive);

   for (i = 0; i < _al_vector_size(&path->segments); i++) {
      al_ustr_append(str, get_segment(path, i));
      al_ustr_append_chr(str, delim);
   }

   al_ustr_append(str, path->filename);
}


/* Function: al_path_to_string
 */
const char *al_path_to_string(const ALLEGRO_PATH *path, char delim)
{
   path_to_ustr(path, delim, path->full_string);
   return al_cstr(path->full_string);
}


/* Function: al_path_free
 */
void al_path_free(ALLEGRO_PATH *path)
{
   unsigned i;

   if (!path) {
      return;
   }

   if (path->drive) {
      al_ustr_free(path->drive);
      path->drive = NULL;
   }

   if (path->filename) {
      al_ustr_free(path->filename);
      path->filename = NULL;
   }

   for (i = 0; i < _al_vector_size(&path->segments); i++) {
      al_ustr_free(get_segment(path, i));
   }
   _al_vector_free(&path->segments);

   if (path->basename) {
      al_ustr_free(path->basename);
      path->basename = NULL;
   }

   if (path->full_string) {
      al_ustr_free(path->full_string);
      path->full_string = NULL;
   }

   _AL_FREE(path);
}


/* Function: al_path_set_drive
 */
void al_path_set_drive(ALLEGRO_PATH *path, const char *drive)
{
   ASSERT(path);

   if (drive)
      al_ustr_assign_cstr(path->drive, drive);
   else
      al_ustr_truncate(path->drive, 0);
}


/* Function: al_path_get_drive
 */
const char *al_path_get_drive(const ALLEGRO_PATH *path)
{
   ASSERT(path);

   return al_cstr(path->drive);
}


/* Function: al_path_set_filename
 */
void al_path_set_filename(ALLEGRO_PATH *path, const char *filename)
{
   ASSERT(path);

   if (filename)
      al_ustr_assign_cstr(path->filename, filename);
   else
      al_ustr_truncate(path->filename, 0);
}


/* Function: al_path_get_filename
 */
const char *al_path_get_filename(const ALLEGRO_PATH *path)
{
   ASSERT(path);

   return al_cstr(path->filename);
}


/* Function: al_path_get_extension
 */
const char *al_path_get_extension(const ALLEGRO_PATH *path)
{
   int pos;
   ASSERT(path);

   pos = al_ustr_rfind_chr(path->filename, al_ustr_size(path->filename), '.');
   if (pos == -1)
      pos = al_ustr_size(path->filename);

   return al_cstr(path->filename) + pos;  /* include dot */
}


/* Function: al_path_set_extension
 */
bool al_path_set_extension(ALLEGRO_PATH *path, char const *extension)
{
   int dot;
   ASSERT(path);

   if (al_ustr_size(path->filename) == 0) {
      return false;
   }

   dot = al_ustr_rfind_chr(path->filename, al_ustr_size(path->filename), '.');
   if (dot >= 0) {
      al_ustr_truncate(path->filename, dot);
   }
   al_ustr_append_cstr(path->filename, extension);
   return true;
}


/* Function: al_path_get_basename
 */
const char *al_path_get_basename(const ALLEGRO_PATH *path)
{
   int dot;
   ASSERT(path);

   dot = al_ustr_rfind_chr(path->filename, al_ustr_size(path->filename), '.');
   if (dot >= 0) {
      al_ustr_assign_substr(path->basename, path->filename, 0, dot);
      return al_cstr(path->basename);
   }

   return al_cstr(path->filename);
}


/* Function: al_path_exists
 */
bool al_path_exists(const ALLEGRO_PATH *path)
{
   ALLEGRO_USTR *ustr;
   bool rc;
   ASSERT(path);

   ustr = al_ustr_new("");
   path_to_ustr(path, ALLEGRO_NATIVE_PATH_SEP, ustr);

   /* Windows' stat() doesn't like the slash at the end of the path when
    * the path is pointing to a directory. There are other places which
    * might require the same fix.
    */
#ifdef ALLEGRO_WINDOWS
   if (al_ustr_has_suffix_cstr(ustr, "\\")) {
      al_ustr_truncate(ustr, al_ustr_size(ustr) - 1);
   }
#endif

   rc = al_is_present_str(al_cstr(ustr));
   al_ustr_free(ustr);

   return rc;
}


/* Function: al_path_make_absolute
 */
bool al_path_make_absolute(ALLEGRO_PATH *path)
{
   ALLEGRO_PATH *cwd_path;
   int i;

   ASSERT(path);

   if (path_is_absolute(path)) {
      return false;
   }

   cwd_path = al_getcwd();
   if (!cwd_path) return false;

   al_path_set_drive(path, al_path_get_drive(cwd_path));

   for (i = al_path_num_components(cwd_path) - 1; i >= 0; i--) {
      al_path_insert(path, 0, al_path_index(cwd_path, i));
   }

   al_path_free(cwd_path);

   return true;
}


/* Function: al_path_make_canonical
 */
bool al_path_make_canonical(ALLEGRO_PATH *path)
{
   unsigned i;
   ASSERT(path);

   for (i = 0; i < _al_vector_size(&path->segments); ) {
      if (strcmp(get_segment_cstr(path, i), ".") == 0)
         al_path_remove(path, i);
      else
         i++;
   }

   /* Remove leading '..'s on absolute paths. */
   if (_al_vector_size(&path->segments) >= 1 &&
      al_ustr_size(get_segment(path, 0)) == 0)
   {
      while (_al_vector_size(&path->segments) >= 2 &&
         strcmp(get_segment_cstr(path, 1), "..") == 0)
      {
         al_path_remove(path, 1);
      }
   }

   return true;
}

/* vim: set sts=3 sw=3 et: */
