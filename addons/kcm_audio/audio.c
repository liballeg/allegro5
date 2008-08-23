/**
 * Originally digi.c from allegro wiki
 * Original authors: KC/Milan
 *
 * Converted to allegro5 by Ryan Dickie
 */


#include <math.h>
#include <stdio.h>

#include "allegro5/kcm_audio.h"
#include "allegro5/internal/aintern_kcm_audio.h"
#include "allegro5/internal/aintern_kcm_cfg.h"

void _al_set_error(int error, char* string)
{
   TRACE("%s (error code: %d)", string, error);
}

ALLEGRO_AUDIO_DRIVER *_al_kcm_driver = NULL;

#if defined(ALLEGRO_CFG_KCM_OPENAL)
   extern struct ALLEGRO_AUDIO_DRIVER _openal_driver;
#endif
#if defined(ALLEGRO_CFG_KCM_ALSA)
   extern struct ALLEGRO_AUDIO_DRIVER _alsa_driver;
#endif

/* Channel configuration helpers */
bool al_is_channel_conf(ALLEGRO_AUDIO_ENUM conf)
{
   return ((conf >= ALLEGRO_AUDIO_1_CH) && (conf <= ALLEGRO_AUDIO_7_1_CH));
}

size_t al_channel_count(ALLEGRO_AUDIO_ENUM conf)
{
   return (conf>>4)+(conf&0xF);
}

/* Depth configuration helpers */
size_t al_depth_size(ALLEGRO_AUDIO_ENUM conf)
{
   ALLEGRO_AUDIO_ENUM depth = conf&~ALLEGRO_AUDIO_UNSIGNED;
   if(depth == ALLEGRO_AUDIO_8_BIT_INT)
      return sizeof(int8_t);
   if(depth == ALLEGRO_AUDIO_16_BIT_INT)
      return sizeof(int16_t);
   if(depth == ALLEGRO_AUDIO_24_BIT_INT)
      return sizeof(int32_t);
   if(depth == ALLEGRO_AUDIO_32_BIT_FLOAT)
      return sizeof(float);
   return 0;
}

/* TODO: possibly take extra parameters
 * (freq, channel, etc) and test if you 
 * can create a voice with them.. if not
 * try another driver.
 */
int al_audio_init(ALLEGRO_AUDIO_ENUM mode)
{
   int retVal = 0;

   /* check to see if a driver is already installed and running */
   if (_al_kcm_driver)
   {
      _al_set_error(ALLEGRO_GENERIC_ERROR, "A Driver already running"); 
      return 1;
   }

   switch (mode)
   {
      case ALLEGRO_AUDIO_DRIVER_AUTODETECT:
         /* check openal first then fallback on others */
         retVal = al_audio_init(ALLEGRO_AUDIO_DRIVER_OPENAL);
         if (retVal == 0)
            return 0;
         retVal = al_audio_init(ALLEGRO_AUDIO_DRIVER_ALSA);
         if (retVal == 0)
            return 0;
         _al_kcm_driver = NULL;
         return 1;

      case ALLEGRO_AUDIO_DRIVER_OPENAL:
         #if defined(ALLEGRO_CFG_KCM_OPENAL)
            if (_openal_driver.open() == 0) {
               fprintf(stderr, "Using OpenAL driver\n"); 
               _al_kcm_driver = &_openal_driver;
               return 0;
            }
            return 1;
         #else
            _al_set_error(ALLEGRO_INVALID_PARAM, "OpenAL not available on this platform");
            return 1;
         #endif

      case ALLEGRO_AUDIO_DRIVER_ALSA:
         #if defined(ALLEGRO_CFG_KCM_ALSA)
            if(_alsa_driver.open() == 0) {
               fprintf(stderr, "Using ALSA driver\n"); 
               _al_kcm_driver = &_alsa_driver;
               return 0;
            }
            return 1;
         #else
            _al_set_error(ALLEGRO_INVALID_PARAM, "ALSA not available on this platform");
            return 1;
         #endif

      case ALLEGRO_AUDIO_DRIVER_DSOUND:
            _al_set_error(ALLEGRO_INVALID_PARAM, "DirectSound driver not yet implemented");
            return 1;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Invalid audio driver");
         return 1;
   }

}

void al_audio_deinit()
{
   if(_al_kcm_driver)
      _al_kcm_driver->close();
   _al_kcm_driver = NULL;
}
