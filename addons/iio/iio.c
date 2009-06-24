#include <ctype.h>


#include "allegro5/allegro5.h"
#include "allegro5/a5_iio.h"
#include "allegro5/internal/aintern.h"


#include "iio.h"


#define MAX_EXTENSION 100


typedef struct Handler
{
   char *extension;
   ALLEGRO_IIO_LOADER_FUNCTION loader;
   ALLEGRO_IIO_SAVER_FUNCTION saver;
   ALLEGRO_IIO_FS_LOADER_FUNCTION fs_loader;
   ALLEGRO_IIO_FS_SAVER_FUNCTION fs_saver;
} Handler;


static Handler **handlers = NULL;
static bool inited = false;
static unsigned int num_handlers = 0;


static void iio_shutdown(void)
{
   if (inited) {
      unsigned int i;
      for (i = 0; i < num_handlers; i++) {
         free(handlers[i]->extension);
         free(handlers[i]);
      }
      free(handlers);
      inited = false;
   }
}


/* Function: al_init_iio_addon
 */
bool al_init_iio_addon(void)
{
   int success;

   if (inited)
      return true;

   success = 0;

   success |= al_add_image_handler("pcx", al_load_pcx, al_save_pcx, al_load_pcx_entry, al_save_pcx_entry);
   success |= al_add_image_handler("bmp", al_load_bmp, al_save_bmp, al_load_bmp_entry, al_save_bmp_entry);
   success |= al_add_image_handler("tga", al_load_tga, al_save_tga, al_load_tga_entry, al_save_tga_entry);
#ifdef ALLEGRO_CFG_IIO_HAVE_PNG
   success |= al_add_image_handler("png", al_load_png, al_save_png, al_load_png_entry, al_save_png_entry);
#endif
#ifdef ALLEGRO_CFG_IIO_HAVE_JPG
   success |= al_add_image_handler("jpg", al_load_jpg, al_save_jpg, al_load_jpg_entry, al_save_jpg_entry);
   success |= al_add_image_handler("jpeg", al_load_jpg, al_save_jpg, al_load_jpg_entry, al_save_jpg_entry);
#endif

   if (success)
      inited = true;


   _al_add_exit_func(iio_shutdown, "iio_shutdown");

   return success;
}


/* Function: al_shutdown_iio_addon
 */
void al_shutdown_iio_addon(void)
{
   iio_shutdown();
}


/* Function: al_add_image_handler
 */
bool al_add_image_handler(const char *extension,
   ALLEGRO_IIO_LOADER_FUNCTION loader, ALLEGRO_IIO_SAVER_FUNCTION saver,
   ALLEGRO_IIO_FS_LOADER_FUNCTION fs_loader, ALLEGRO_IIO_FS_SAVER_FUNCTION fs_saver)
{
   Handler *l;

   ASSERT(extension);
   ASSERT(loader);

   l = (Handler *) malloc(sizeof(Handler));
   if (!l)
      return false;

   l->extension = strdup(extension);
   l->loader = loader;
   l->saver = saver;
   l->fs_loader = fs_loader;
   l->fs_saver = fs_saver;

   num_handlers++;

   if (num_handlers == 1) {
      handlers = malloc(sizeof(Handler *));
   }
   else {
      handlers = realloc(handlers, num_handlers * sizeof(Handler *));
   }

   if (!handlers) {
      free(l);
      handlers = NULL;
      return false;
   }

   handlers[num_handlers - 1] = l;

   return true;
}


static char *iio_get_extension(const char *filename)
{
   int count = 0;
   int pos = strlen(filename) - 1;
   char *result;

   while (pos >= 0 && filename[pos] != '.') {
      count++;
      pos--;
   }

   if (filename[pos] == '.') {
      result = strdup(filename + pos + 1);
   }
   else {
      result = strdup("");
   }

   return result;
}


static int iio_stricmp(const char *s1, const char *s2)
{
   int i;

   for (i = 0; s1[i] && s2[i] && (tolower(s1[i]) == tolower(s2[i])); i++) {
      /* hm, this is empty */
   }

   if (s1[i] == 0 && s2[i] == 0)
      return 0;

   if (s1[i] == 0)
      return -1;
   if (s2[i] == 0)
      return 1;

   return s1[i] - s2[i];
}


static Handler *find_handler(const char *filename)
{
   char *p = iio_get_extension(filename);
   char extension[MAX_EXTENSION];
   unsigned int i;

   strncpy(extension, p, MAX_EXTENSION);

   free(p);

   for (i = 0; i < num_handlers; i++) {
      Handler *l = handlers[i];
      if (!iio_stricmp(extension, l->extension)) {
         return l;
      }
   }

   return NULL;
}


/* Function: al_load_bitmap
 */
ALLEGRO_BITMAP *al_load_bitmap(const char *filename)
{
   Handler *h = find_handler(filename);
   if (h)
      return h->loader(filename);
   else
      return NULL;
}

/* Function: al_save_bitmap
 */
int al_save_bitmap(const char *filename, ALLEGRO_BITMAP *bitmap)
{
   Handler *h = find_handler(filename);
   if (h)
      return h->saver(filename, bitmap);
   else {
      TRACE("No handler for image %s found\n", filename);
      return 1;
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
int al_save_bitmap_entry(ALLEGRO_FILE *pf, const char *ident,
   ALLEGRO_BITMAP *bitmap)
{
   Handler *h = find_handler(ident);
   if (h)
      return h->fs_saver(pf, bitmap);
   else {
      TRACE("No handler for image %s found\n", ident);
      return 1;
   }
}
