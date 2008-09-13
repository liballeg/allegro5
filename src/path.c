
#include "allegro5/path.h"

static char *_ustrduprange(const char *start, const char *end)
{
   const char *ptr = start;
   char *dup = NULL;
   int32_t blen = 0, eslen = ustrsizez(empty_string);

   while(ptr < end) {
      int32_t c = ugetxc(&ptr);
      blen += ucwidth(c);
   }

   dup = malloc(blen+eslen);
   if(!dup) {
      return NULL;
   }

   memcpy(dup, start, blen);
   memset(dup+blen, 0, eslen);

   return dup;
}

// last component if its not followed by a `dirsep` is interpreted as a file component, always.
static int32_t _parse_path(const char *p, char **drive, char **path, char **file)
{
   const char *ptr = p;
   char *path_info = NULL, *path_info_end = NULL;
   int32_t path_info_len = 0;
   const char dirsep = ALLEGRO_NATIVE_PATH_SEP;
   
#ifdef ALLEGRO_WINDOWS
   // UNC \\server\share name
   if(ugetc(ptr) == dirsep && ugetat(ptr, 1) == dirsep) {
      char *uncdrive = NULL;
      int32_t udlen = 0;

      ptr += uwidth(ptr);
      ptr += uwidth(ptr);
      
      ptr = ustrchr(ptr, dirsep);
      if(!ptr)
         goto _parse_path_error;
      
      if(!ugetc(ptr)) {
         goto _parse_path_error;
      }

      ptr += uwidth(ptr);
      if(!ugetc(ptr))
         goto _parse_path_error;
      
      ptr = ustrchr(ptr, dirsep);
      if(!ptr) {
         ptr = p + ustrsize(p);
      }

      uncdrive = _ustrduprange(p, ptr);
      if(!uncdrive) {
         goto _parse_path_error;
      }

      *drive = uncdrive;
   }
   // drive name
   else if(ugetc(ptr) != dirsep && ugetat(ptr, 1) == ALLEGRO_NATIVE_DRIVE_SEP) {
      char *tmp = NULL;

      tmp = _ustrduprange(ptr, ptr+uoffset(ptr, 2));
      if(!tmp) {
         // ::)
         goto _parse_path_error;
      }

      *drive = tmp;
      ptr = ustrchr(ptr, dirsep);
   }

   if(!ptr || !ugetc(ptr)) {
      // no path, or file info
      return 0;
   }
#endif

   // grab path, and/or file info

   path_info_end = ustrrchr(ptr, dirsep);
   if(!path_info_end) { // no path, just file
      char *file_info = NULL;

      file_info = ustrdup(ptr);
      if(!file_info)
         goto _parse_path_error;

      *file = file_info;
      return 0;
   }
   else if(ugetat(path_info_end, 1)) {
      // dirsep is not at end
      char *file_info = NULL;

      file_info = ustrdup(path_info_end+uoffset(path_info_end,1));
      if(!file_info)
         goto _parse_path_error;

      *file = file_info;
   }

   /* detect if the path_info should be a single '/' */
   if(ugetc(ptr) == dirsep && (!ugetat(ptr, 2) || ptr == path_info_end)) {
      path_info_end += ucwidth(dirsep);
   }

   path_info_len = path_info_end - ptr;
   if(path_info_len) {
      path_info = _ustrduprange(ptr, path_info_end);
      if(!path_info)
         goto _parse_path_error;

      *path = path_info;
   }

   return 0;

_parse_path_error:
   if(*drive) {
      free(*drive);
      *drive = 0;
   }

   if(*path) {
      free(*path);
      *path = 0;
   }

   if(*file) {
      free(*file);
      *file = 0;
   }

   return -1;
}

static const char **_split_path(const char *path, uint32_t *num)
{
   const char **arr = NULL, *ptr = NULL, *lptr = NULL;
   int32_t count = 0, i = 0;
   const char dirsep = ALLEGRO_NATIVE_PATH_SEP;

   for(ptr=path; ugetc(ptr); ptr+=uwidth(ptr)) {
      if(ugetc(ptr) == dirsep)
         count++;
      lptr = ptr;
   }

   if(ugetc(lptr) != dirsep) {
      count++;
   }

   arr = malloc(sizeof(char *) * count);
   if(!arr)
      return NULL;

   lptr = path;
   for(ptr=path; ugetc(ptr); ptr++) {
      if(ugetc(ptr) == dirsep) {
         char *seg = _ustrduprange(lptr, ptr);
         if(!seg)
            goto _split_path_getout;

         arr[i++] = seg;
         lptr = ptr+uoffset(ptr, 1);
      }
   }

   if(ugetc(lptr)) {
      char *seg = _ustrduprange(lptr, ptr);
      if(!seg)
         goto _split_path_getout;

      arr[i++] = seg;
   }

   *num = count;
   return arr;
_split_path_getout:

   if(arr) {
      for(i = 0; arr[i]; i++) {
         free((void*)arr[i]);
      }

      free(arr);
   }

   return NULL;
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

