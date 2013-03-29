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

ALLEGRO_DEBUG_CHANNEL("bitmap")

#define MAX_EXTENSION   (32)


typedef struct Handler
{
   char extension[MAX_EXTENSION];
   ALLEGRO_IIO_LOADER_FUNCTION loader;
   ALLEGRO_IIO_SAVER_FUNCTION saver;
   ALLEGRO_IIO_FS_LOADER_FUNCTION fs_loader;
   ALLEGRO_IIO_FS_SAVER_FUNCTION fs_saver;
} Handler;


/* globals */
static _AL_VECTOR iio_table = _AL_VECTOR_INITIALIZER(Handler);


static Handler *find_handler(const char *extension)
{
   unsigned i;

   for (i = 0; i < _al_vector_size(&iio_table); i++) {
      Handler *l = _al_vector_ref(&iio_table, i);
      if (0 == _al_stricmp(extension, l->extension)) {
         return l;
      }
   }

   return NULL;
}


static Handler *add_iio_table_f(const char *ext)
{
   Handler *ent;

   ent = _al_vector_alloc_back(&iio_table);
   strcpy(ent->extension, ext);
   ent->loader = NULL;
   ent->saver = NULL;
   ent->fs_loader = NULL;
   ent->fs_saver = NULL;

   return ent;
}


static void free_iio_table(void)
{
   _al_vector_free(&iio_table);
}


void _al_init_iio_table(void)
{
   _al_add_exit_func(free_iio_table, "free_iio_table");
}


/* Function: al_register_bitmap_loader
 */
bool al_register_bitmap_loader(const char *extension,
   ALLEGRO_BITMAP *(*loader)(const char *filename))
{
   Handler *ent;

   ASSERT(extension);

   if (strlen(extension) + 1 >= MAX_EXTENSION) {
      return false;
   }

   ent = find_handler(extension);
   if (!loader) {
       if (!ent || !ent->loader) {
         return false; /* Nothing to remove. */
       }
   }
   else if (!ent) {
       ent = add_iio_table_f(extension);
   }

   ent->loader = loader;

   return true;
}


/* Function: al_register_bitmap_saver
 */
bool al_register_bitmap_saver(const char *extension,
   bool (*saver)(const char *filename, ALLEGRO_BITMAP *bmp))
{
   Handler *ent;

   ASSERT(extension);
   ASSERT(saver);

   if (strlen(extension) + 1 >= MAX_EXTENSION) {
      return false;
   }

   ent = find_handler(extension);
   if (!saver) {
       if (!ent || !ent->saver) {
         return false; /* Nothing to remove. */
       }
   }
   else if (!ent) {
       ent = add_iio_table_f(extension);
   }

   ent->saver = saver;

   return true;
}


/* Function: al_register_bitmap_loader_f
 */
bool al_register_bitmap_loader_f(const char *extension,
   ALLEGRO_BITMAP *(*loader_f)(ALLEGRO_FILE *fp))
{
   Handler *ent;

   ASSERT(extension);

   if (strlen(extension) + 1 >= MAX_EXTENSION) {
      return false;
   }

   ent = find_handler(extension);
   if (!loader_f) {
       if (!ent || !ent->fs_loader) {
         return false; /* Nothing to remove. */
       }
   }
   else if (!ent) {
       ent = add_iio_table_f(extension);
   }

   ent->fs_loader = loader_f;

   return true;
}


/* Function: al_register_bitmap_saver_f
 */
bool al_register_bitmap_saver_f(const char *extension,
   bool (*saver_f)(ALLEGRO_FILE *fp, ALLEGRO_BITMAP *bmp))
{
   Handler *ent;

   ASSERT(extension);
   ASSERT(saver_f);

   if (strlen(extension) + 1 >= MAX_EXTENSION) {
      return false;
   }

   ent = find_handler(extension);
   if (!saver_f) {
       if (!ent || !ent->fs_saver) {
         return false; /* Nothing to remove. */
       }
   }
   else if (!ent) {
       ent = add_iio_table_f(extension);
   }

   ent->fs_saver = saver_f;

   return true;
}


/* Function: al_load_bitmap
 */
ALLEGRO_BITMAP *al_load_bitmap(const char *filename)
{
   const char *ext;
   Handler *h;
   ALLEGRO_BITMAP *ret;

   ext = strrchr(filename, '.');
   if (!ext) {
      ALLEGRO_WARN("Bitmap %s has no extension - "
         "not even trying to load it.\n", filename);
      return NULL;
   }

   h = find_handler(ext);
   if (h) {
      ret = h->loader(filename);
      if (!ret)
         ALLEGRO_WARN("Failed loading %s with %s handler.\n", filename,
            ext);
   }
   else {
      ALLEGRO_WARN("No handler for bitmap extensions %s - "
         "therefore not trying to load %s.\n", ext, filename);
      ret = NULL;
   }

   return ret;
}


/* Function: al_save_bitmap
 */
bool al_save_bitmap(const char *filename, ALLEGRO_BITMAP *bitmap)
{
   const char *ext;
   Handler *h;

   ext = strrchr(filename, '.');
   if (!ext)
      return false;

   h = find_handler(ext);
   if (h)
      return h->saver(filename, bitmap);
   else {
      ALLEGRO_WARN("No handler for image %s found\n", filename);
      return false;
   }
}


/* Function: al_load_bitmap_f
 */
ALLEGRO_BITMAP *al_load_bitmap_f(ALLEGRO_FILE *fp, const char *ident)
{
   Handler *h = find_handler(ident);
   if (h)
      return h->fs_loader(fp);
   else
      return NULL;
}


/* Function: al_save_bitmap_f
 */
bool al_save_bitmap_f(ALLEGRO_FILE *fp, const char *ident,
   ALLEGRO_BITMAP *bitmap)
{
   Handler *h = find_handler(ident);
   if (h)
      return h->fs_saver(fp, bitmap);
   else {
      ALLEGRO_WARN("No handler for image %s found\n", ident);
      return false;
   }
}


/* vim: set sts=3 sw=3 et: */
