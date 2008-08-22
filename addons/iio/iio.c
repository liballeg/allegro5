#include <ctype.h>


#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"


#include "iio.h"


#define MAX_EXTENSION 100


typedef struct Loader {
   char *extension;
   IIO_LOADER_FUNCTION function;
} Loader;


static Loader **loaders = NULL;
static bool inited = false;
static unsigned int num_loaders = 0;


bool iio_init(void)
{
   int success;

   if (inited)
      return true;

   success = 0;

   success |= iio_add_loader("pcx", iio_load_pcx);
   success |= iio_add_loader("bmp", iio_load_bmp);
   success |= iio_add_loader("tga", iio_load_tga);

   if (success)
      inited = true;

   return success;
}


bool iio_add_loader(AL_CONST char *extension, IIO_LOADER_FUNCTION function)
{
   Loader *l;

   ASSERT(extension);
   ASSERT(function);

   l = (Loader *)_AL_MALLOC(sizeof(*l));
   if (!l)
      return false;

   l->extension = strdup(extension);
   l->function = function;

   num_loaders++;

   if (loaders == NULL) {
      loaders = _AL_MALLOC(sizeof(Loader*));
   }
   else {
      loaders = _AL_REALLOC(loaders, num_loaders*sizeof(Loader*));
   }

   if (!loaders) {
      _AL_FREE(l);
      return false;
   }

   loaders[num_loaders-1] = l;

   return true;
}


static char *iio_get_extension(AL_CONST char *filename)
{
   int count = 0;
   int pos = strlen(filename)-1;
   char *result;

   while (pos >= 0 && filename[pos] != '.') {
      count++;
      pos--;
   }

   if (filename[pos] == '.') {
      result = strdup(filename+pos+1);
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


ALLEGRO_BITMAP *iio_load(AL_CONST char *filename)
{
   char *p = iio_get_extension(filename);
   char extension[MAX_EXTENSION];
   unsigned int i;
   
   strncpy(extension, p, MAX_EXTENSION);

   _AL_FREE(p);

   for (i = 0; i < num_loaders; i++) {
      Loader *l = loaders[i];
      if (!iio_stricmp(extension, l->extension)) {
         return l->function(filename);
      }
   }

   return NULL;
}

