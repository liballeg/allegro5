/**
 * Original authors: KC/Milan
 * Rewritten with new API by Ryan Dickie
 */


#include <math.h>
#include <stdio.h>

#include "allegro5/internal/aintern_audio.h"

ALLEGRO_AUDIO_DRIVER* driver = NULL;
extern struct ALLEGRO_AUDIO_DRIVER _openal_driver;
#if defined(ALLEGRO_LINUX)
   extern struct ALLEGRO_AUDIO_DRIVER _alsa_driver;
#endif

int al_audio_channel_count(ALLEGRO_AUDIO_ENUM conf)
{
   return (conf>>4)+(conf&0xF);
}

/* Depth configuration helpers */
int al_audio_depth_size(ALLEGRO_AUDIO_ENUM depth)
{
   if(depth == ALLEGRO_AUDIO_8_BIT_UINT)
      return 1;
   if(depth == ALLEGRO_AUDIO_16_BIT_INT)
      return 2;
   if(depth == ALLEGRO_AUDIO_24_BIT_INT)
      return 3;
   if(depth == ALLEGRO_AUDIO_32_BIT_FLOAT)
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

void fill_with_silence(void *buf, int bytes, ALLEGRO_AUDIO_ENUM depth) {
   int silence;

   switch(depth)
   {
      case ALLEGRO_AUDIO_8_BIT_UINT:
         silence = 0x80;
         break;
      case ALLEGRO_AUDIO_16_BIT_INT:
         silence = 0x8000;
         break;
      case ALLEGRO_AUDIO_24_BIT_INT:
         silence = 0x800000;
         break;
      case ALLEGRO_AUDIO_32_BIT_FLOAT:
         silence = 0;
         break;
      default:
         TRACE("Unsupported sound format\n");
         return;
   }

   memset(buf, silence, bytes);
}

int al_audio_init(ALLEGRO_AUDIO_ENUM mode)
{
   int retVal = 0;

   /* check to see if a driver is already installed and running */
   if (driver)
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
            driver = NULL;
            return 1:
         #endif

      case ALLEGRO_AUDIO_DRIVER_OPENAL:
         if (_openal_driver.open() == 0 )
         {
            fprintf(stderr, "Using OpenAL driver\n"); 
            driver = &_openal_driver;
            return 0;
         }
         return 1;
      case ALLEGRO_AUDIO_DRIVER_ALSA:
         #if defined(ALLEGRO_LINUX)
            if (_alsa_driver.open() == 0)
            {
               fprintf(stderr, "Using ALSA driver\n");
               driver = &_alsa_driver;
               return 0;
            }
            return 1;
         #else
            TRACE("Alsa not available on this platform\n");
            return 1;
         #endif

      case ALLEGRO_AUDIO_DRIVER_DSOUND:
         #if defined(ALLEGRO_WINDOWS)
            TRACE("DirectSound driver not yet implemented\n");
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

void al_audio_deinit()
{
   if(driver)
      driver->close();
   driver = NULL;
}
