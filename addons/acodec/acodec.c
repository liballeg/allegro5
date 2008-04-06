/**
 * allegro audio codec loader
 * author: Ryan Dickie (c) 2008
 * todo: unicode file paths ;)
 */
#include "allegro5/internal/aintern_acodec.h"

struct ALLEGRO_SAMPLE* allegro_load_sample(const char* filename)
{
   if (filename == NULL)
      return NULL;

   //now to decide which file extension
   char * ext = strrchr ( filename, '.' );
   if (ext == NULL)
      return NULL;

   //pardon me if this is ugly/unsafe
   //i've only ever done this in higher level
   //languages
   ext++; //get past the '.' character
   #if defined(WANT_OGG)
      if (strcmp("ogg",ext) == 0)
         return al_load_sample_oggvorbis(filename);
   #endif
   
   #if defined(WANT_FLAC)
      if (strcmp("flac",ext) == 0)
         return al_load_sample_flac(filename);
   #endif

   #if defined(WANT_SNDFILE)
      if (strcmp("wav",ext) == 0 || strcmp("aiff",ext) == 0)
         return al_load_sample_sndfile(filename);
   #endif
 
   //no codec found!
   return NULL;
}
