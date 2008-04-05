/**
 * allegro audio codec loader
 * author: Ryan Dickie (c) 2008
 * todo: unicode file paths ;)
 */
#include "allegro5/acodec.h"

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
   if (strcmp("ogg",ext) == 0)
      return al_load_sample_oggvorbis(filename);
   else if (strcmp("flac",ext) == 0)
      return al_load_sample_flac(filename);
   else //out last hope.. 
      return al_load_sample_sndfile(filename); 
}
