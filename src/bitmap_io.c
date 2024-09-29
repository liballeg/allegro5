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
 *      Bitmap I/O framework.
 *
 *      See LICENSE.txt for copyright information.
 */

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_exitfunc.h"
#include "allegro5/internal/aintern_vector.h"

#include <string.h>

A5O_DEBUG_CHANNEL("bitmap")

#define MAX_EXTENSION   (32)


typedef struct Handler
{
   char extension[MAX_EXTENSION];
   A5O_IIO_LOADER_FUNCTION loader;
   A5O_IIO_SAVER_FUNCTION saver;
   A5O_IIO_FS_LOADER_FUNCTION fs_loader;
   A5O_IIO_FS_SAVER_FUNCTION fs_saver;
   A5O_IIO_IDENTIFIER_FUNCTION identifier;
} Handler;


/* globals */
static _AL_VECTOR iio_table = _AL_VECTOR_INITIALIZER(Handler);


static Handler *add_iio_table_f(const char *ext)
{
   Handler *ent;

   ent = _al_vector_alloc_back(&iio_table);
   strcpy(ent->extension, ext);
   ent->loader = NULL;
   ent->saver = NULL;
   ent->fs_loader = NULL;
   ent->fs_saver = NULL;
   ent->identifier = NULL;

   return ent;
}


static Handler *find_handler(const char *extension, bool create_if_not)
{
   unsigned i;

   ASSERT(extension);

   if (strlen(extension) + 1 >= MAX_EXTENSION) {
      return NULL;
   }

   for (i = 0; i < _al_vector_size(&iio_table); i++) {
      Handler *l = _al_vector_ref(&iio_table, i);
      if (0 == _al_stricmp(extension, l->extension)) {
         return l;
      }
   }

   if (create_if_not)
      return add_iio_table_f(extension);

   return NULL;
}


static Handler *find_handler_for_file(A5O_FILE *f)
{
   unsigned i;

   ASSERT(f);

   for (i = 0; i < _al_vector_size(&iio_table); i++) {
      Handler *l = _al_vector_ref(&iio_table, i);
      if (l->identifier) {
         int64_t pos = al_ftell(f);
         bool identified = l->identifier(f);
         al_fseek(f, pos, A5O_SEEK_SET);
         if (identified)
            return l;
      }
   }
   return NULL;
}


static void free_iio_table(void)
{
   _al_vector_free(&iio_table);
}


void _al_init_iio_table(void)
{
   _al_add_exit_func(free_iio_table, "free_iio_table");
}

#define REGISTER(function) \
   Handler *ent = find_handler(extension, function != NULL); \
   if (!function) { \
      if (!ent || !ent->function) { \
         return false; /* Nothing to remove. */ \
      } \
   } \
   ent->function = function; \
   return true;


/* Function: al_register_bitmap_loader
 */
bool al_register_bitmap_loader(const char *extension,
   A5O_BITMAP *(*loader)(const char *filename, int flags))
{
   REGISTER(loader)
}


/* Function: al_register_bitmap_saver
 */
bool al_register_bitmap_saver(const char *extension,
   bool (*saver)(const char *filename, A5O_BITMAP *bmp))
{
   REGISTER(saver)
}


/* Function: al_register_bitmap_loader_f
 */
bool al_register_bitmap_loader_f(const char *extension,
   A5O_BITMAP *(*fs_loader)(A5O_FILE *fp, int flags))
{
   REGISTER(fs_loader)
}


/* Function: al_register_bitmap_saver_f
 */
bool al_register_bitmap_saver_f(const char *extension,
   bool (*fs_saver)(A5O_FILE *fp, A5O_BITMAP *bmp))
{
   REGISTER(fs_saver)
}


/* Function: al_register_bitmap_identifier
 */
bool al_register_bitmap_identifier(const char *extension,
   bool (*identifier)(A5O_FILE *f))
{
   REGISTER(identifier)
}


/* Function: al_load_bitmap
 */
A5O_BITMAP *al_load_bitmap(const char *filename)
{
   int flags = 0;

   /* For backwards compatibility with the 5.0 branch. */
   if (al_get_new_bitmap_flags() & A5O_NO_PREMULTIPLIED_ALPHA) {
      flags |= A5O_NO_PREMULTIPLIED_ALPHA;
      A5O_WARN("A5O_NO_PREMULTIPLIED_ALPHA in new_bitmap_flags "
         "is deprecated\n");
   }

   return al_load_bitmap_flags(filename, flags);
}


/* Function: al_load_bitmap_flags
 */
A5O_BITMAP *al_load_bitmap_flags(const char *filename, int flags)
{
   const char *ext;
   Handler *h;
   A5O_BITMAP *ret;

   ext = al_identify_bitmap(filename);
   if (!ext) {
      ext = strrchr(filename, '.');
      if (!ext) {
         A5O_ERROR("Could not identify bitmap %s!\n", filename);
         return NULL;
      }
   }

   h = find_handler(ext, false);
   if (h && h->loader) {
      ret = h->loader(filename, flags);
      if (!ret)
         A5O_ERROR("Failed loading bitmap %s with %s handler.\n",
            filename, ext);
   }
   else {
      A5O_ERROR("No handler for bitmap %s!\n", filename);
      ret = NULL;
   }

   return ret;
}


/* Function: al_save_bitmap
 */
bool al_save_bitmap(const char *filename, A5O_BITMAP *bitmap)
{
   const char *ext;
   Handler *h;

   ext = strrchr(filename, '.');
   if (!ext) {
      A5O_ERROR("Unable to determine file format from %s\n", filename);
      return false;
   }

   h = find_handler(ext, false);
   if (h && h->saver)
      return h->saver(filename, bitmap);
   else {
      A5O_ERROR("No handler for image %s found\n", filename);
      return false;
   }
}


/* Function: al_load_bitmap_f
 */
A5O_BITMAP *al_load_bitmap_f(A5O_FILE *fp, const char *ident)
{
   int flags = 0;

   /* For backwards compatibility with the 5.0 branch. */
   if (al_get_new_bitmap_flags() & A5O_NO_PREMULTIPLIED_ALPHA) {
      flags |= A5O_NO_PREMULTIPLIED_ALPHA;
      A5O_WARN("A5O_NO_PREMULTIPLIED_ALPHA in new_bitmap_flags "
         "is deprecated\n");
   }

   return al_load_bitmap_flags_f(fp, ident, flags);
}


/* Function: al_load_bitmap_flags_f
 */
A5O_BITMAP *al_load_bitmap_flags_f(A5O_FILE *fp,
   const char *ident, int flags)
{
   Handler *h;
   if (ident)
      h = find_handler(ident, false);
   else
      h = find_handler_for_file(fp);
   if (h && h->fs_loader)
      return h->fs_loader(fp, flags);
   else
      return NULL;
}


/* Function: al_save_bitmap_f
 */
bool al_save_bitmap_f(A5O_FILE *fp, const char *ident,
   A5O_BITMAP *bitmap)
{
   Handler *h = find_handler(ident, false);
   if (h && h->fs_saver)
      return h->fs_saver(fp, bitmap);
   else {
      A5O_ERROR("No handler for image %s found\n", ident);
      return false;
   }
}


/* Function: al_identify_bitmap_f
 */
char const *al_identify_bitmap_f(A5O_FILE *fp)
{
   Handler *h = find_handler_for_file(fp);
   if (!h)
      return NULL;
   return h->extension;
}


/* Function: al_identify_bitmap
 */
char const *al_identify_bitmap(char const *filename)
{
   char const *ext;
   A5O_FILE *fp = al_fopen(filename, "rb");
   if (!fp)
      return NULL;
   ext = al_identify_bitmap_f(fp);
   al_fclose(fp);
   return ext;
}


/* vim: set sts=3 sw=3 et: */
