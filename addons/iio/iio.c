#include <ctype.h>


#include "allegro5/allegro5.h"
#include "allegro5/a5_iio.h"


#include "iio.h"


#define MAX_EXTENSION 100


typedef struct Handler
{
   char *extension;
   IIO_LOADER_FUNCTION loader;
   IIO_SAVER_FUNCTION saver;
   IIO_FS_LOADER_FUNCTION fs_loader;
   IIO_FS_SAVER_FUNCTION fs_saver;
} Handler;


static Handler **handlers = NULL;
static bool inited = false;
static unsigned int num_handlers = 0;



/* Function: al_iio_init
 */
bool al_iio_init(void)
{
   int success;

   if (inited)
      return true;

   success = 0;

   success |= al_iio_add_handler("pcx", iio_load_pcx, iio_save_pcx, iio_load_pcx_entry, iio_save_pcx_entry);
   success |= al_iio_add_handler("bmp", iio_load_bmp, iio_save_bmp, iio_load_bmp_entry, iio_save_bmp_entry);
   success |= al_iio_add_handler("tga", iio_load_tga, iio_save_tga, iio_load_tga_entry, iio_save_tga_entry);
#ifdef ALLEGRO_CFG_IIO_HAVE_PNG
   success |= al_iio_add_handler("png", iio_load_png, iio_save_png, iio_load_png_entry, iio_save_png_entry);
#endif
#ifdef ALLEGRO_CFG_IIO_HAVE_JPG
   success |= al_iio_add_handler("jpg", iio_load_jpg, iio_save_jpg, iio_load_jpg_entry, iio_save_jpg_entry);
   success |= al_iio_add_handler("jpeg", iio_load_jpg, iio_save_jpg, iio_load_jpg_entry, iio_save_jpg_entry);
#endif

   if (success)
      inited = true;

   return success;
}


/* Function: al_iio_add_handler
 */
bool al_iio_add_handler(const char *extension,
   IIO_LOADER_FUNCTION loader, IIO_SAVER_FUNCTION saver,
   IIO_FS_LOADER_FUNCTION fs_loader, IIO_FS_SAVER_FUNCTION fs_saver)
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


/* Function: al_iio_load
 */
ALLEGRO_BITMAP *al_iio_load(const char *filename)
{
   Handler *h = find_handler(filename);
   if (h)
      return h->loader(filename);
   else
      return NULL;
}

/* Function: al_iio_save
 */
int al_iio_save(const char *filename, ALLEGRO_BITMAP *bitmap)
{
   Handler *h = find_handler(filename);
   if (h)
      return h->saver(filename, bitmap);
   else {
      TRACE("No handler for image %s found\n", filename);
      return 1;
   }
}

/* Function: al_iio_load_entry
 */
ALLEGRO_BITMAP *al_iio_load_entry(ALLEGRO_FS_ENTRY *pf, const char *ident)
{
   Handler *h = find_handler(ident);
   if (h)
      return h->fs_loader(pf);
   else
      return NULL;
}

/* Function: al_iio_save_entry
 */
int al_iio_save_entry(ALLEGRO_FS_ENTRY *pf, const char *ident,
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
