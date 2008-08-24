/*
 * Allegro audio codec loader.
 * author: Ryan Dickie (c) 2008
 * todo: unicode file paths ;)
 */

#include "allegro5/acodec.h"
#include "allegro5/internal/aintern_acodec.h"


ALLEGRO_SAMPLE* al_load_sample(const char* filename)
{
   char *ext;

   if (filename == NULL)
      return NULL;

   //now to decide which file extension
   ext = strrchr ( filename, '.' );
   if (ext == NULL)
      return NULL;

   //pardon me if this is ugly/unsafe
   //i've only ever done this in higher level
   //languages
   ext++; //get past the '.' character
   #if defined(ALLEGRO_CFG_ACODEC_VORBIS)
      if (stricmp("ogg",ext) == 0) {
         return al_load_sample_oggvorbis(filename);
      }
   #endif
   
   #if defined(ALLEGRO_CFG_ACODEC_FLAC)
      if (stricmp("flac",ext) == 0) {
         return al_load_sample_flac(filename);
      }
   #endif

   #if defined(ALLEGRO_CFG_ACODEC_SNDFILE)
      if (stricmp("wav",ext) == 0 || stricmp("aiff",ext) == 0 || stricmp("flac",ext) == 0) {
         return al_load_sample_sndfile(filename);
      }
   #endif
 
   //no codec found!
   return NULL;
}

ALLEGRO_STREAM* al_load_stream(const char* filename)
{
   char *ext;

   if (filename == NULL)
      return NULL;

   //now to decide which file extension
   ext = strrchr ( filename, '.' );
   if (ext == NULL)
      return NULL;

   //pardon me if this is ugly/unsafe
   //i've only ever done this in higher level
   //languages
   ext++; //get past the '.' character
   #if defined(ALLEGRO_CFG_ACODEC_VORBIS)
      if (stricmp("ogg",ext) == 0) {
         return al_load_stream_oggvorbis(filename);
      }
   #endif
   
   #if defined(ALLEGRO_CFG_ACODEC_FLAC)
      if (stricmp("flac",ext) == 0) {
      /* Disabled until flac streaming is implemented */
      /*   return al_load_stream_flac(filename); */
      }
   #endif

   #if defined(ALLEGRO_CFG_ACODEC_SNDFILE)
      if (stricmp("wav",ext) == 0 || stricmp("aiff",ext) == 0 || stricmp("flac",ext) == 0) {
         return al_load_stream_sndfile(filename);
      }
   #endif
 
   //no codec found!
   return NULL;
}


/* FIXME: use the allegro provided helpers */
ALLEGRO_CHANNEL_CONF _al_count_to_channel_conf(int num_channels)
{
   switch (num_channels)
   {
      case 1:
         return ALLEGRO_CHANNEL_CONF_1;
      case 2:
         return ALLEGRO_CHANNEL_CONF_2;
      case 3:
         return ALLEGRO_CHANNEL_CONF_3;
      case 4:
         return ALLEGRO_CHANNEL_CONF_4;
      case 6:
         return ALLEGRO_CHANNEL_CONF_5_1;
      case 7:
         return ALLEGRO_CHANNEL_CONF_6_1;
      case 8:
         return ALLEGRO_CHANNEL_CONF_7_1;
      default:
         return 0;
   }
}

/* note: assumes 8-bit is unsigned, and 32-bit float all others are signed 
 * make your own version if the codec is different */
ALLEGRO_AUDIO_DEPTH _al_word_size_to_depth_conf(int word_size)
{
   switch (word_size)
   {
      case 1:
         return ALLEGRO_AUDIO_DEPTH_UINT8;
      case 2:
         return ALLEGRO_AUDIO_DEPTH_INT16;
      case 3:
         return ALLEGRO_AUDIO_DEPTH_INT24;
      case 4:
         return ALLEGRO_AUDIO_DEPTH_FLOAT32;
      default:
         return 0;
   }
}
