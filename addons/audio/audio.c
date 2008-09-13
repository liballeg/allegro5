/**
 * Original authors: KC/Milan
 * Rewritten with new API by Ryan Dickie
 */


#include <math.h>
#include <stdio.h>

#include "allegro5/internal/aintern_audio.h"

ALLEGRO_AUDIO_DRIVER* _al_audio_driver = NULL;

int al_audio_channel_count(ALLEGRO_AUDIO_ENUM conf)
{
   return (conf>>4)+(conf&0xF);
}

/* Depth configuration helpers */
int al_audio_depth_size(ALLEGRO_AUDIO_ENUM depth)
{
   if(depth == ALLEGRO_AUDIO_DEPTH_UINT8)
      return 1;
   if(depth == ALLEGRO_AUDIO_DEPTH_INT16)
      return 2;
   if(depth == ALLEGRO_AUDIO_DEPTH_INT24)
      return 3;
   if(depth == ALLEGRO_AUDIO_DEPTH_FLOAT32)
      return 4;

   TRACE("oops. unsupported depth size\n");
   ASSERT(FALSE);
   return 0;

}

int al_audio_buffer_size(const ALLEGRO_SAMPLE* sample)
{
   int samples = sample->length;
   ALLEGRO_AUDIO_ENUM ch = sample->chan_conf;
   ALLEGRO_AUDIO_ENUM dep = sample->depth;
   return al_audio_size_bytes(samples, ch, dep);
}

int al_audio_size_bytes(int samples, ALLEGRO_AUDIO_ENUM channels, ALLEGRO_AUDIO_ENUM depth)
{
   return samples * al_audio_channel_count(channels) * al_audio_depth_size(depth);
}

int _al_audio_get_silence(ALLEGRO_AUDIO_ENUM depth) {
   int silence;

   switch(depth)
   {
      case ALLEGRO_AUDIO_DEPTH_UINT8:
         silence = 0x80;
         break;
      case ALLEGRO_AUDIO_DEPTH_INT16:
         silence = 0x8000;
         break;
      case ALLEGRO_AUDIO_DEPTH_INT24:
         silence = 0x800000;
         break;
      case ALLEGRO_AUDIO_DEPTH_FLOAT32:
         silence = 0;
         break;
      default:
         TRACE("Unsupported sound format\n");
         return 0;
   }

   return silence;
}

int al_audio_init(ALLEGRO_AUDIO_ENUM mode)
{
   int retVal = 0;

   /* check to see if a driver is already installed and running */
   if (_al_audio_driver)
   {
      TRACE("An audio driver is already running\n"); 
      return 1;
   }

   switch (mode)
   {
      case ALLEGRO_AUDIO_DRIVER_AUTODETECT:
         /* check openal first then fallback on others */
         retVal = al_audio_init(ALLEGRO_AUDIO_DRIVER_OPENAL);
         if (retVal == 0)
            return retVal;

         #if defined(ALLEGRO_LINUX)
            return al_audio_init(ALLEGRO_AUDIO_DRIVER_ALSA);
         #elif defined(ALLEGRO_WINDOWS)
            return al_audio_init(ALLEGRO_AUDIO_DRIVER_DSOUND);
         #elif defined(ALLEGRO_MACOSX)
            _al_audio_driver = NULL;
            return 1:
         #endif

      case ALLEGRO_AUDIO_DRIVER_OPENAL:
         if (_al_openal_driver.open() == 0 )
         {
            fprintf(stderr, "Using OpenAL driver\n"); 
            _al_audio_driver = &_al_openal_driver;
            return 0;
         }
         return 1;
      case ALLEGRO_AUDIO_DRIVER_ALSA:
         #if defined(ALLEGRO_LINUX)
            if (_al_alsa_driver.open() == 0)
            {
               fprintf(stderr, "Using ALSA driver\n");
               _al_audio_driver = &_al_alsa_driver;
               return 0;
            }
            return 1;
         #else
            TRACE("Alsa not available on this platform\n");
            return 1;
         #endif

      case ALLEGRO_AUDIO_DRIVER_DSOUND:
         #if defined(ALLEGRO_WINDOWS)
            if (_al_dsound_driver.open() == 0)
            {
               fprintf(stderr, "Using DirectSound driver\n");
               _al_audio_driver = &_al_dsound_driver;
               return 0;
            }
            return 1;
         #else
            TRACE("DirectSound not available on this platform\n");
            return 1;
         #endif

      default:
         TRACE("Invalid audio driver\n");
         return 1;
   }

}

void al_audio_deinit(void)
{
   if (_al_audio_driver)
      _al_audio_driver->close();
   _al_audio_driver = NULL;
}
