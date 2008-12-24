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
} Handler;


static Handler **handlers = NULL;
static bool inited = false;
static unsigned int num_handlers = 0;



/* Function: al_iio_init
 * Initializes the IIO addon.
 */
bool al_iio_init(void)
{
   int success;

   if (inited)
      return true;

   success = 0;

   success |= al_iio_add_handler("pcx", iio_load_pcx, iio_save_pcx);
   success |= al_iio_add_handler("bmp", iio_load_bmp, iio_save_bmp);
   success |= al_iio_add_handler("tga", iio_load_tga, iio_save_tga);
#ifdef ALLEGRO_CFG_IIO_HAVE_PNG
   success |= al_iio_add_handler("png", iio_load_png, iio_save_png);
#endif
#ifdef ALLEGRO_CFG_IIO_HAVE_JPG
   success |= al_iio_add_handler("jpg", iio_load_jpg, iio_save_jpg);
   success |= al_iio_add_handler("jpeg", iio_load_jpg, iio_save_jpg);
#endif

   if (success)
      inited = true;

   return success;
}


bool al_iio_add_handler(AL_CONST char *extension,
                        IIO_LOADER_FUNCTION loader, IIO_SAVER_FUNCTION saver)
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


static char *iio_get_extension(AL_CONST char *filename)
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


static int iio_stricmp(AL_CONST char *s1, AL_CONST char *s2)
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


static Handler *find_handler(AL_CONST char *filename)
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
 * Loads an image file into an ALLEGRO_BITMAP. File type
 * is determined by the extension.
 */
ALLEGRO_BITMAP *al_iio_load(AL_CONST char *filename)
{
   Handler *h = find_handler(filename);
   if (h)
      return h->loader(filename);
   else
      return NULL;
}

/* Function: al_iio_save
 * Saves an ALLEGRO_BITMAP to an image file. File type
 * is determined by the extension.
 */
int al_iio_save(AL_CONST char *filename, ALLEGRO_BITMAP *bitmap)
{
   Handler *h = find_handler(filename);
   if (h)
      return h->saver(filename, bitmap);
   else {
      TRACE("No handler for image %s found\n", filename);
      return 1;
   }
}
