/*
 * Allegro audio codec loader.
 * author: Ryan Dickie (c) 2008
 */

#include "allegro5/acodec.h"
#include "allegro5/internal/aintern_acodec.h"


ALLEGRO_SAMPLE_DATA *al_load_sample(const char *filename)
{
   const char *ext;

   ASSERT(filename);

   ext = strrchr(filename, '.');
   if (ext == NULL)
      return NULL;

   ext++;   /* skip '.' */
   #if defined(ALLEGRO_CFG_ACODEC_VORBIS)
      if (stricmp("ogg", ext) == 0) {
         return al_load_sample_oggvorbis(filename);
      }
   #endif
   
   #if defined(ALLEGRO_CFG_ACODEC_FLAC)
      if (stricmp("flac", ext) == 0) {
         return al_load_sample_flac(filename);
      }
   #endif

   #if defined(ALLEGRO_CFG_ACODEC_SNDFILE)
      if (stricmp("wav", ext) == 0 ||
         stricmp("aiff", ext) == 0 ||
         stricmp("flac", ext) == 0)
      {
         return al_load_sample_sndfile(filename);
      }
   #endif
 
   return NULL;
}


/* al_stream_from_file:
 * Unlike a regular stream, a stream created with al_stream_from_file() doesn't
 * have to be fed by the user. Instead, it will be fed as needed by acodec
 * addon, progressively reading a file from the disk. When the stream finishes
 * the ALLEGRO_AUDIOPROP_PLAYING field will be set to false.
 */
ALLEGRO_STREAM *al_stream_from_file(size_t buffer_count, unsigned long samples,
                                    const char *filename)
{
   const char *ext;

   ASSERT(filename);

   ext = strrchr(filename, '.');
   if (ext == NULL)
      return NULL;

   ext++;   /* skip '.' */
   #if defined(ALLEGRO_CFG_ACODEC_VORBIS)
      if (stricmp("ogg", ext) == 0) {
         return  al_load_stream_oggvorbis(buffer_count, samples, filename);
      }
   #endif

   #if defined(ALLEGRO_CFG_ACODEC_FLAC)
      if (stricmp("flac", ext) == 0) {
         /* Disabled until flac streaming is implemented */
         //return al_load_stream_flac(buffer_count, samples, filename);
      }
   #endif

   #if defined(ALLEGRO_CFG_ACODEC_SNDFILE)
      if (stricmp("wav", ext) == 0 ||
         stricmp("aiff", ext) == 0 ||
         stricmp("flac", ext) == 0)
      {
         return al_load_stream_sndfile(buffer_count, samples, filename);
      }
   #endif

   TRACE("Error creating ALLEGRO_STREAM from '%s'.\n", filename);

   return NULL;
}


/* FIXME: use the allegro provided helpers */
ALLEGRO_CHANNEL_CONF _al_count_to_channel_conf(int num_channels)
{
   switch (num_channels) {
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


/* Note: assumes 8-bit is unsigned, and all others are signed. */
ALLEGRO_AUDIO_DEPTH _al_word_size_to_depth_conf(int word_size)
{
   switch (word_size) {
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

/* vim: set sts=3 sw=3 et: */

