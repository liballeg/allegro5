#include <ctype.h>


#include "allegro5/allegro5.h"
#include "allegro5/allegro_image.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_vector.h"

#include "iio.h"


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
static bool iio_inited = false;
static _AL_VECTOR iio_table = _AL_VECTOR_INITIALIZER(Handler);


/* Function: al_init_image_addon
 */
bool al_init_image_addon(void)
{
   int success;

   if (iio_inited)
      return true;

   success = 0;

   success |= al_register_bitmap_loader(".pcx", al_load_pcx);
   success |= al_register_bitmap_saver(".pcx", al_save_pcx);
   success |= al_register_bitmap_loader_entry(".pcx", al_load_pcx_entry);
   success |= al_register_bitmap_saver_entry(".pcx", al_save_pcx_entry);

   success |= al_register_bitmap_loader(".bmp", al_load_bmp);
   success |= al_register_bitmap_saver(".bmp", al_save_bmp);
   success |= al_register_bitmap_loader_entry(".bmp", al_load_bmp_entry);
   success |= al_register_bitmap_saver_entry(".bmp", al_save_bmp_entry);

   success |= al_register_bitmap_loader(".tga", al_load_tga);
   success |= al_register_bitmap_saver(".tga", al_save_tga);
   success |= al_register_bitmap_loader_entry(".tga", al_load_tga_entry);
   success |= al_register_bitmap_saver_entry(".tga", al_save_tga_entry);

#ifdef ALLEGRO_CFG_IIO_HAVE_PNG
   success |= al_register_bitmap_loader(".png", al_load_png);
   success |= al_register_bitmap_saver(".png", al_save_png);
   success |= al_register_bitmap_loader_entry(".png", al_load_png_entry);
   success |= al_register_bitmap_saver_entry(".png", al_save_png_entry);
#endif

#ifdef ALLEGRO_CFG_IIO_HAVE_JPG
   success |= al_register_bitmap_loader(".jpg", al_load_jpg);
   success |= al_register_bitmap_saver(".jpg", al_save_jpg);
   success |= al_register_bitmap_loader_entry(".jpg", al_load_jpg_entry);
   success |= al_register_bitmap_saver_entry(".jpg", al_save_jpg_entry);

   success |= al_register_bitmap_loader(".jpeg", al_load_jpg);
   success |= al_register_bitmap_saver(".jpeg", al_save_jpg);
   success |= al_register_bitmap_loader_entry(".jpeg", al_load_jpg_entry);
   success |= al_register_bitmap_saver_entry(".jpeg", al_save_jpg_entry);
#endif

   if (success)
      iio_inited = true;

   _al_add_exit_func(al_shutdown_image_addon, "al_shutdown_image_addon");

   return success;
}


/* Function: al_shutdown_image_addon
 */
void al_shutdown_image_addon(void)
{
   if (iio_inited) {
      _al_vector_free(&iio_table);
      iio_inited = false;
   }
}


static Handler *find_handler(const char *extension)
{
   unsigned i;

   for (i = 0; i < _al_vector_size(&iio_table); i++) {
      Handler *l = _al_vector_ref(&iio_table, i);
      if (0 == stricmp(extension, l->extension)) {
         return l;
      }
   }

   return NULL;
}


static Handler *add_iio_table_entry(const char *ext)
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


/* Function: al_register_bitmap_loader
 */
bool al_register_bitmap_loader(const char *extension,
   ALLEGRO_BITMAP *(*loader)(const char *filename))
{
   Handler *ent;

   ASSERT(extension);
   ASSERT(loader);

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
       ent = add_iio_table_entry(extension);
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
       ent = add_iio_table_entry(extension);
   }

   ent->saver = saver;

   return true;
}


/* Function: al_register_bitmap_loader_entry
 */
bool al_register_bitmap_loader_entry(const char *extension,
   ALLEGRO_BITMAP *(*loader_entry)(ALLEGRO_FILE *pf))
{
   Handler *ent;

   ASSERT(extension);
   ASSERT(loader_entry);

   if (strlen(extension) + 1 >= MAX_EXTENSION) {
      return false;
   }

   ent = find_handler(extension);
   if (!loader_entry) {
       if (!ent || !ent->fs_loader) {
         return false; /* Nothing to remove. */
       }
   }
   else if (!ent) {
       ent = add_iio_table_entry(extension);
   }

   ent->fs_loader = loader_entry;

   return true;
}


/* Function: al_register_bitmap_saver_entry
 */
bool al_register_bitmap_saver_entry(const char *extension,
   bool (*saver_entry)(ALLEGRO_FILE *pf, ALLEGRO_BITMAP *bmp))
{
   Handler *ent;

   ASSERT(extension);
   ASSERT(saver_entry);

   if (strlen(extension) + 1 >= MAX_EXTENSION) {
      return false;
   }

   ent = find_handler(extension);
   if (!saver_entry) {
       if (!ent || !ent->fs_saver) {
         return false; /* Nothing to remove. */
       }
   }
   else if (!ent) {
       ent = add_iio_table_entry(extension);
   }

   ent->fs_saver = saver_entry;

   return true;
}


/* Function: al_load_bitmap
 */
ALLEGRO_BITMAP *al_load_bitmap(const char *filename)
{
   const char *ext;
   Handler *h;

   ext = strrchr(filename, '.');
   if (!ext)
      return NULL;

   h = find_handler(ext);
   if (h)
      return h->loader(filename);
   else
      return NULL;
}


/* Function: al_save_bitmap
 */
bool al_save_bitmap(const char *filename, ALLEGRO_BITMAP *bitmap)
{
   const char *ext;
   Handler *h;

   ext = strrchr(filename, '.');
   if (!ext)
      return NULL;

   h = find_handler(ext);
   if (h)
      return h->saver(filename, bitmap);
   else {
      TRACE("No handler for image %s found\n", filename);
      return false;
   }
}


/* Function: al_load_bitmap_entry
 */
ALLEGRO_BITMAP *al_load_bitmap_entry(ALLEGRO_FILE *pf, const char *ident)
{
   Handler *h = find_handler(ident);
   if (h)
      return h->fs_loader(pf);
   else
      return NULL;
}


/* Function: al_save_bitmap_entry
 */
bool al_save_bitmap_entry(ALLEGRO_FILE *pf, const char *ident,
   ALLEGRO_BITMAP *bitmap)
{
   Handler *h = find_handler(ident);
   if (h)
      return h->fs_saver(pf, bitmap);
   else {
      TRACE("No handler for image %s found\n", ident);
      return false;
   }
}


/* Function: al_get_allegro_image_version */
uint32_t al_get_allegro_image_version(void)
{
    return ALLEGRO_VERSION_INT;
}


/* vim: set sts=3 sw=3 et: */
