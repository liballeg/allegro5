/*
 * Allegro audio codec loader.
 * author: Ryan Dickie (c) 2008
 * todo: unicode file paths ;)
 */


#include "allegro5/acodec.h"
#include "allegro5/internal/aintern_acodec.h"
#include "allegro5/audio.h"


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
ALLEGRO_AUDIO_ENUM _al_count_to_channel_conf(int num_channels)
{
   switch (num_channels)
   {
      case 1:
         return ALLEGRO_AUDIO_1_CH;
      case 2:
         return ALLEGRO_AUDIO_2_CH;
      case 3:
         return ALLEGRO_AUDIO_3_CH;
      case 4:
         return ALLEGRO_AUDIO_4_CH;
      case 6:
         return ALLEGRO_AUDIO_5_1_CH;
      case 7:
         return ALLEGRO_AUDIO_6_1_CH;
      case 8:
         return ALLEGRO_AUDIO_7_1_CH;
      default:
         return 0;
   }
}

/* note: assumes 8-bit is unsigned, and 32-bit float all others are signed 
 * make your own version if the codec is different */
ALLEGRO_AUDIO_ENUM _al_word_size_to_depth_conf(int word_size)
{
   switch (word_size)
   {
      case 1:
         return ALLEGRO_AUDIO_8_BIT_UINT;
      case 2:
         return ALLEGRO_AUDIO_16_BIT_INT;
      case 3:
         return ALLEGRO_AUDIO_24_BIT_INT;
      case 4:
         return ALLEGRO_AUDIO_32_BIT_FLOAT;
      default:
         return 0;
   }
}
