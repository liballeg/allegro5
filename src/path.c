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
#include "allegro5/internal/aintern_vector.h"
#include "allegro5/internal/aintern_path.h"


/* get_segment:
 *  Return the i'th directory component of a path.
 */
static A5O_USTR *get_segment(const A5O_PATH *path, unsigned i)
{
   A5O_USTR **seg = _al_vector_ref(&path->segments, i);
   return *seg;
}


/* get_segment_cstr:
 *  Return the i'th directory component of a path as a C string.
 */
static const char *get_segment_cstr(const A5O_PATH *path, unsigned i)
{
   return al_cstr(get_segment(path, i));
}


/* replace_backslashes:
 *  Replace backslashes by slashes.
 */
static void replace_backslashes(A5O_USTR *path)
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
static bool parse_path_string(const A5O_USTR *str, A5O_PATH *path)
{
   A5O_USTR_INFO    dot_info;
   A5O_USTR_INFO    dotdot_info;
   const A5O_USTR *  dot = al_ref_cstr(&dot_info, ".");
   const A5O_USTR *  dotdot = al_ref_cstr(&dotdot_info, "..");

   A5O_USTR *piece = al_ustr_new("");
   int pos = 0;
   bool on_windows;

   /* We compile the drive handling code on non-Windows platforms to prevent
    * it becoming broken.
    */
#ifdef A5O_WINDOWS
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
         // Note: The slash will be parsed again, so we end up with
         // "//server/share" and not "//servershare"!
         pos = slash;
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
            al_append_path_component(path, al_cstr(piece));
         }
         else {
            /* This might be an empty string, but that's okay. */
            al_ustr_assign(path->filename, piece);
         }
         break;
      }

      /* Non-last component. */
      al_ustr_assign_substr(piece, str, pos, slash);
      al_append_path_component(path, al_cstr(piece));
      pos = slash + 1;
   }

   al_ustr_free(piece);
   return true;

Error:

   al_ustr_free(piece);
   return false;
}


/* Function: al_create_path
 */
A5O_PATH *al_create_path(const char *str)
{
   A5O_PATH *path;

   path = al_malloc(sizeof(A5O_PATH));
   if (!path)
      return NULL;

   path->drive = al_ustr_new("");
   path->filename = al_ustr_new("");
   _al_vector_init(&path->segments, sizeof(A5O_USTR *));
   path->basename = al_ustr_new("");
   path->full_string = al_ustr_new("");

   if (str != NULL) {
      A5O_USTR *copy = al_ustr_new(str);
      replace_backslashes(copy);

      if (!parse_path_string(copy, path)) {
         al_destroy_path(path);
         path = NULL;
      }

      al_ustr_free(copy);
   }

   return path;
}


/* Function: al_create_path_for_directory
 */
A5O_PATH *al_create_path_for_directory(const char *str)
{
   A5O_PATH *path = al_create_path(str);
   if (al_ustr_length(path->filename)) {
      A5O_USTR *last = path->filename;
      path->filename = al_ustr_new("");
      al_append_path_component(path, al_cstr(last));
      al_ustr_free(last);
   }
   return path;
}


/* Function: al_clone_path
 */
A5O_PATH *al_clone_path(const A5O_PATH *path)
{
   A5O_PATH *clone;
   unsigned int i;

   ASSERT(path);

   clone = al_create_path(NULL);
   if (!clone) {
      return NULL;
   }

   al_ustr_assign(clone->drive, path->drive);
   al_ustr_assign(clone->filename, path->filename);

   for (i = 0; i < _al_vector_size(&path->segments); i++) {
      A5O_USTR **slot = _al_vector_alloc_back(&clone->segments);
      (*slot) = al_ustr_dup(get_segment(path, i));
   }

   return clone;
}


/* Function: al_get_path_num_components
 */
int al_get_path_num_components(const A5O_PATH *path)
{
   ASSERT(path);

   return _al_vector_size(&path->segments);
}


/* Function: al_get_path_component
 */
const char *al_get_path_component(const A5O_PATH *path, int i)
{
   ASSERT(path);
   ASSERT(i < (int)_al_vector_size(&path->segments));

   if (i < 0)
      i = _al_vector_size(&path->segments) + i;

   ASSERT(i >= 0);

   return get_segment_cstr(path, i);
}


/* Function: al_replace_path_component
 */
void al_replace_path_component(A5O_PATH *path, int i, const char *s)
{
   ASSERT(path);
   ASSERT(s);
   ASSERT(i < (int)_al_vector_size(&path->segments));

   if (i < 0)
      i = _al_vector_size(&path->segments) + i;

   ASSERT(i >= 0);

   al_ustr_assign_cstr(get_segment(path, i), s);
}


/* Function: al_remove_path_component
 */
void al_remove_path_component(A5O_PATH *path, int i)
{
   ASSERT(path);
   ASSERT(i < (int)_al_vector_size(&path->segments));

   if (i < 0)
      i = _al_vector_size(&path->segments) + i;

   ASSERT(i >= 0);

   al_ustr_free(get_segment(path, i));
   _al_vector_delete_at(&path->segments, i);
}


/* Function: al_insert_path_component
 */
void al_insert_path_component(A5O_PATH *path, int i, const char *s)
{
   A5O_USTR **slot;
   ASSERT(path);
   ASSERT(i <= (int)_al_vector_size(&path->segments));

   if (i < 0)
      i = _al_vector_size(&path->segments) + i;

   ASSERT(i >= 0);

   slot = _al_vector_alloc_mid(&path->segments, i);
   (*slot) = al_ustr_new(s);
}


/* Function: al_get_path_tail
 */
const char *al_get_path_tail(const A5O_PATH *path)
{
   ASSERT(path);

   if (al_get_path_num_components(path) == 0)
      return NULL;

   return al_get_path_component(path, -1);
}


/* Function: al_drop_path_tail
 */
void al_drop_path_tail(A5O_PATH *path)
{
   if (al_get_path_num_components(path) > 0) {
      al_remove_path_component(path, -1);
   }
}


/* Function: al_append_path_component
 */
void al_append_path_component(A5O_PATH *path, const char *s)
{
   A5O_USTR **slot = _al_vector_alloc_back(&path->segments);
   (*slot) = al_ustr_new(s);
}


static bool path_is_absolute(const A5O_PATH *path)
{
   /* If the first segment is empty, we have an absolute path. */
   return (_al_vector_size(&path->segments) > 0)
      && (al_ustr_size(get_segment(path, 0)) == 0);
}


/* Function: al_join_paths
 */
bool al_join_paths(A5O_PATH *path, const A5O_PATH *tail)
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
      al_append_path_component(path, get_segment_cstr(tail, i));
   }

   return true;
}


/* Function: al_rebase_path
 */
bool al_rebase_path(const A5O_PATH *head, A5O_PATH *tail)
{
   unsigned i;
   ASSERT(head);
   ASSERT(tail);

   /* Don't bother concating if the tail is an absolute path. */
   if (path_is_absolute(tail)) {
      return false;
   }

   al_set_path_drive(tail, al_get_path_drive(head));

   for (i = 0; i < _al_vector_size(&head->segments); i++) {
      al_insert_path_component(tail, i, get_segment_cstr(head, i));
   }

   return true;
}


static void path_to_ustr(const A5O_PATH *path, int32_t delim,
   A5O_USTR *str)
{
   unsigned i;

   al_ustr_assign(str, path->drive);

   for (i = 0; i < _al_vector_size(&path->segments); i++) {
      al_ustr_append(str, get_segment(path, i));
      al_ustr_append_chr(str, delim);
   }

   al_ustr_append(str, path->filename);
}


/* Function: al_path_cstr
 */
const char *al_path_cstr(const A5O_PATH *path, char delim)
{
   return al_cstr(al_path_ustr(path, delim));
}

/* Function: al_path_ustr
 */
const A5O_USTR *al_path_ustr(const A5O_PATH *path, char delim)
{
   path_to_ustr(path, delim, path->full_string);
   return path->full_string;
}


/* Function: al_destroy_path
 */
void al_destroy_path(A5O_PATH *path)
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

   al_free(path);
}


/* Function: al_set_path_drive
 */
void al_set_path_drive(A5O_PATH *path, const char *drive)
{
   ASSERT(path);

   if (drive)
      al_ustr_assign_cstr(path->drive, drive);
   else
      al_ustr_truncate(path->drive, 0);
}


/* Function: al_get_path_drive
 */
const char *al_get_path_drive(const A5O_PATH *path)
{
   ASSERT(path);

   return al_cstr(path->drive);
}


/* Function: al_set_path_filename
 */
void al_set_path_filename(A5O_PATH *path, const char *filename)
{
   ASSERT(path);

   if (filename)
      al_ustr_assign_cstr(path->filename, filename);
   else
      al_ustr_truncate(path->filename, 0);
}


/* Function: al_get_path_filename
 */
const char *al_get_path_filename(const A5O_PATH *path)
{
   ASSERT(path);

   return al_cstr(path->filename);
}


/* Function: al_get_path_extension
 */
const char *al_get_path_extension(const A5O_PATH *path)
{
   int pos;
   ASSERT(path);

   pos = al_ustr_rfind_chr(path->filename, al_ustr_size(path->filename), '.');
   if (pos == -1)
      pos = al_ustr_size(path->filename);

   return al_cstr(path->filename) + pos;  /* include dot */
}


/* Function: al_set_path_extension
 */
bool al_set_path_extension(A5O_PATH *path, char const *extension)
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


/* Function: al_get_path_basename
 */
const char *al_get_path_basename(const A5O_PATH *path)
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


/* Function: al_make_path_canonical
 */
bool al_make_path_canonical(A5O_PATH *path)
{
   unsigned i;
   ASSERT(path);

   for (i = 0; i < _al_vector_size(&path->segments); ) {
      if (strcmp(get_segment_cstr(path, i), ".") == 0)
         al_remove_path_component(path, i);
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
         al_remove_path_component(path, 1);
      }
   }

   return true;
}

/* vim: set sts=3 sw=3 et: */
