
#include "allegro5/path.h"

char **_split_path(const char *path, uint32_t *num)
{
   char **arr = NULL, *ptr = NULL, *lptr = NULL;
   uint32_t count = 0, i = 0;
   char sep[2];

   al_fs_path_sep(2, sep);

   for(ptr=path; *ptr; ptr++) {
      if(*ptr == sep[0])
         count++;
   }

   arr = AL_MALLOC(sizeof(char *) * count);
   if(!arr)
      return NULL;

   lptr = path;
   for(ptr=path; *ptr; ptr++) {
      if(*ptr == sep[0]) {
         /* one minus current len, remove the trailing separator */
         uint32_t l = (ptr - lptr - 1);
         /* one plus total, to include trailing null */
         char *seg = AL_MALLOC(sizeof(char) *  (l + 1));
         if(!seg)
            goto _split_path_getout;

         memset(seg, 0, l+1);
         memcpy(seg, lptr, l);

         lptr = ptr+1;
      }
   }

   if(*lptr) {
         /* one minus current len, remove the trailing separator */
      uint32_t l = (ptr - lptr - 1);
      /* one plus total, to include trailing null */
      char *seg = AL_MALLOC(sizeof(char) *  (l + 1));
      if(!seg)
         goto _split_path_getout;

      memset(seg, 0, l+1);
      memcpy(seg, lptr, l);
   }

   return arr;
_split_path_getout:

   if(arr) {
      for(i = 0; arr[i]; i++) {
         AL_FREE(arr[i]);
      }

      al_free(arr);
   }
}

AL_PATH *al_path_init(void)
{
   AL_PATH *path = NULL;

   path = al_malloc(sizeof(AL_PATH));
   if(!path)
      return NULL;

   path->path = NULL;
   path->dirty = 0;
   path->segment_count = 0;
   path->segment = NULL;

   return path;
}

AL_PATH *al_path_init_from_string(const char *s, char delim)
{
   AL_PATH *path = NULL;

   path = al_path_init();
   if(!path)
      return NULL;


}

int al_path_num_components(AL_PATH *path)
{

}

const char *al_path_index(AL_PATH *path, int i)
{

}

void al_path_replace(AL_PATH *path, int i, const char *s)
{

}

void al_path_delete(AL_PATH *path, int i)
{

}

void al_path_insert(AL_PATH *path, int i, const char *s)
{

}

const char *al_path_tail(AL_PATH *path)
{

}

void al_path_drop_tail(AL_PATH *path)
{

}

void al_path_append(AL_PATH *path, const char *s)
{

}

void al_path_concat(AL_PATH *path, const AL_PATH *tail)
{

}

char *al_path_to_string(AL_PATH *path, char delim)
{

}

void al_path_free(AL_PATH *path)
{

}

