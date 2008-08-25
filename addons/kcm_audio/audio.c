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
#if defined(ALLEGRO_CFG_KCM_DSOUND)
   extern struct ALLEGRO_AUDIO_DRIVER _dsound_driver;
#endif

/* Channel configuration helpers */
bool al_is_channel_conf(ALLEGRO_CHANNEL_CONF conf)
{
   return ((conf >= ALLEGRO_CHANNEL_CONF_1) && (conf <= ALLEGRO_CHANNEL_CONF_7_1));
}

size_t al_channel_count(ALLEGRO_CHANNEL_CONF conf)
{
   return (conf>>4)+(conf&0xF);
}

/* Depth configuration helpers */
size_t al_depth_size(ALLEGRO_CHANNEL_CONF conf)
{
   ALLEGRO_AUDIO_DEPTH depth = conf & ~ALLEGRO_AUDIO_DEPTH_UNSIGNED;
   if (depth == ALLEGRO_AUDIO_DEPTH_INT8)
      return sizeof(int8_t);
   if (depth == ALLEGRO_AUDIO_DEPTH_INT16)
      return sizeof(int16_t);
   if (depth == ALLEGRO_AUDIO_DEPTH_INT24)
      return sizeof(int32_t);
   if (depth == ALLEGRO_AUDIO_DEPTH_FLOAT32)
      return sizeof(float);
   return 0;
}

/* Returns a silent sample frame. */
int _al_audio_get_silence(ALLEGRO_AUDIO_DEPTH depth) {
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
      default:
         silence = 0;
   }

   return silence;
}

/* TODO: possibly take extra parameters
 * (freq, channel, etc) and test if you 
 * can create a voice with them.. if not
 * try another driver.
 */
int al_audio_init(ALLEGRO_AUDIO_DRIVER_ENUM mode)
{
   int retVal = 0;

   /* check to see if a driver is already installed and running */
   if (_al_kcm_driver) {
      _al_set_error(ALLEGRO_GENERIC_ERROR, "A Driver already running"); 
      return 1;
   }

   switch (mode) {
      case ALLEGRO_AUDIO_DRIVER_AUTODETECT:
         /* check openal first then fallback on others */
         retVal = al_audio_init(ALLEGRO_AUDIO_DRIVER_OPENAL);
         if (retVal == 0)
            return 0;
         retVal = al_audio_init(ALLEGRO_AUDIO_DRIVER_ALSA);
         if (retVal == 0)
            return 0;
         retVal = al_audio_init(ALLEGRO_AUDIO_DRIVER_DSOUND);
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
            if (_alsa_driver.open() == 0) {
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
         #if defined(ALLEGRO_CFG_KCM_DSOUND)
            if (_dsound_driver.open() == 0) {
               fprintf(stderr, "Using DirectSound driver\n"); 
               _al_kcm_driver = &_dsound_driver;
               return 0;
            }
            return 1;
         #else
            _al_set_error(ALLEGRO_INVALID_PARAM, "DirectSound not available on this platform");
            return 1;
         #endif

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Invalid audio driver");
         return 1;
   }

}

void al_audio_deinit()
{
   if (_al_kcm_driver)
      _al_kcm_driver->close();
   _al_kcm_driver = NULL;
}
