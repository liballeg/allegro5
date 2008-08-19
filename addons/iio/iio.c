#include <ctype.h>


#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_vector.h"


#include "iio.h"


const int MAX_EXTENSION = 100;


typedef struct Loader {
   char *extension;
   IIO_LOADER_FUNCTION function;
} Loader;


static _AL_VECTOR loaders = _AL_VECTOR_INITIALIZER(Loader *);


bool iio_init(void)
{
   int success = 0;

   success |= iio_add_loader("pcx", iio_load_pcx);
   success |= iio_add_loader("bmp", iio_load_bmp);
   success |= iio_add_loader("tga", iio_load_tga);

   return success;
}


bool iio_add_loader(AL_CONST char *extension, IIO_LOADER_FUNCTION function)
{
   Loader **add;
   Loader *l;

   ASSERT(extension);
   ASSERT(function);

   l = (Loader *)_AL_MALLOC(sizeof(*l));
   if (!l)
      return false;

   l->extension = strdup(extension);
   l->function = function;

   add = _al_vector_alloc_back(&loaders);

   if (add) {
      *add = l;
      return true;
   }
   else {
      _AL_FREE(l);
      return false;
   }
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


int iio_stricmp(AL_CONST char *s1, AL_CONST char *s2)
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

   for (i = 0; i < loaders._size; i++) {
      Loader **lptr = _al_vector_ref(&loaders, i);
      Loader *l = *lptr;
      if (!iio_stricmp(extension, l->extension)) {
         return l->function(filename);
      }
   }

   return NULL;
}

