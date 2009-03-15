/**
 * Originally digi.c from allegro wiki
 * Original authors: KC/Milan
 *
 * Converted to allegro5 by Ryan Dickie
 */


#include <math.h>
#include <stdio.h>

#include "allegro5/kcm_audio.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_kcm_audio.h"
#include "allegro5/internal/aintern_kcm_cfg.h"

void _al_set_error(int error, char* string)
{
   TRACE("%s (error code: %d)\n", string, error);
}

ALLEGRO_AUDIO_DRIVER *_al_kcm_driver = NULL;

#if defined(ALLEGRO_CFG_KCM_OPENAL)
   extern struct ALLEGRO_AUDIO_DRIVER _al_kcm_openal_driver;
#endif
#if defined(ALLEGRO_CFG_KCM_ALSA)
   extern struct ALLEGRO_AUDIO_DRIVER _al_kcm_alsa_driver;
#endif
#if defined(ALLEGRO_CFG_KCM_OSS)
   extern struct ALLEGRO_AUDIO_DRIVER _al_kcm_oss_driver;
#endif
#if defined(ALLEGRO_CFG_KCM_DSOUND)
   extern struct ALLEGRO_AUDIO_DRIVER _al_kcm_dsound_driver;
#endif

/* Channel configuration helpers */
bool al_is_channel_conf(ALLEGRO_CHANNEL_CONF conf)
{
   return ((conf >= ALLEGRO_CHANNEL_CONF_1) && (conf <= ALLEGRO_CHANNEL_CONF_7_1));
}

size_t al_get_channel_count(ALLEGRO_CHANNEL_CONF conf)
{
   return (conf>>4)+(conf&0xF);
}

/* Depth configuration helpers */
size_t al_get_depth_size(ALLEGRO_AUDIO_DEPTH depth)
{
   switch (depth) {
      case ALLEGRO_AUDIO_DEPTH_INT8:
      case ALLEGRO_AUDIO_DEPTH_UINT8:
         return sizeof(int8_t);
      case ALLEGRO_AUDIO_DEPTH_INT16:
      case ALLEGRO_AUDIO_DEPTH_UINT16:
         return sizeof(int16_t);
      case ALLEGRO_AUDIO_DEPTH_INT24:
      case ALLEGRO_AUDIO_DEPTH_UINT24:
         return sizeof(int32_t);
      case ALLEGRO_AUDIO_DEPTH_FLOAT32:
         return sizeof(float);
      default:
         ASSERT(false);
         return 0;
   }
}

/* Returns a silent sample frame. */
int _al_audio_get_silence(ALLEGRO_AUDIO_DEPTH depth)
{
   switch (depth) {
      case ALLEGRO_AUDIO_DEPTH_UINT8:
         return 0x80;
      case ALLEGRO_AUDIO_DEPTH_INT16:
         return 0x8000;
      case ALLEGRO_AUDIO_DEPTH_INT24:
         return 0x800000;
      default:
         return 0;
   }
}

static ALLEGRO_AUDIO_DRIVER_ENUM get_config_audio_driver(void)
{
   ALLEGRO_SYSTEM *sys = al_system_driver();
   const char *value;

   if (!sys || !sys->config)
      return ALLEGRO_AUDIO_DRIVER_AUTODETECT;

   value = al_get_config_value(sys->config, "sound", "driver");
   if (!value || value[0] == '\0')
      return ALLEGRO_AUDIO_DRIVER_AUTODETECT;

   if (0 == stricmp(value, "ALSA"))
      return ALLEGRO_AUDIO_DRIVER_ALSA;

   if (0 == stricmp(value, "OPENAL"))
      return ALLEGRO_AUDIO_DRIVER_OPENAL;

   if (0 == stricmp(value, "OSS"))
      return ALLEGRO_AUDIO_DRIVER_OSS;

   if (0 == stricmp(value, "DSOUND") || 0 == stricmp(value, "DIRECTSOUND"))
      return ALLEGRO_AUDIO_DRIVER_DSOUND;

   return ALLEGRO_AUDIO_DRIVER_AUTODETECT;
}

/* TODO: possibly take extra parameters
 * (freq, channel, etc) and test if you 
 * can create a voice with them.. if not
 * try another driver.
 */
static int do_install_audio(ALLEGRO_AUDIO_DRIVER_ENUM mode)
{
   int retVal = 0;

   /* check to see if a driver is already installed and running */
   if (_al_kcm_driver) {
      _al_set_error(ALLEGRO_GENERIC_ERROR, "A driver already running");
      return 1;
   }

   if (mode == ALLEGRO_AUDIO_DRIVER_AUTODETECT) {
      mode = get_config_audio_driver();
   }

   switch (mode) {
      case ALLEGRO_AUDIO_DRIVER_AUTODETECT:
         retVal = al_install_audio(ALLEGRO_AUDIO_DRIVER_ALSA);
         if (retVal == 0)
            return 0;
         retVal = al_install_audio(ALLEGRO_AUDIO_DRIVER_DSOUND);
         if (retVal == 0)
            return 0;
         retVal = al_install_audio(ALLEGRO_AUDIO_DRIVER_OSS);
         if (retVal == 0)
            return 0;
         retVal = al_install_audio(ALLEGRO_AUDIO_DRIVER_OPENAL);
         if (retVal == 0)
            return 0;
         _al_kcm_driver = NULL;
         return 1;

      case ALLEGRO_AUDIO_DRIVER_OPENAL:
         #if defined(ALLEGRO_CFG_KCM_OPENAL)
            if (_al_kcm_openal_driver.open() == 0) {
               fprintf(stderr, "Using OpenAL driver\n"); 
               _al_kcm_driver = &_al_kcm_openal_driver;
               return 0;
            }
            return 1;
         #else
            _al_set_error(ALLEGRO_INVALID_PARAM, "OpenAL not available on this platform");
            return 1;
         #endif

      case ALLEGRO_AUDIO_DRIVER_ALSA:
         #if defined(ALLEGRO_CFG_KCM_ALSA)
            if (_al_kcm_alsa_driver.open() == 0) {
               fprintf(stderr, "Using ALSA driver\n"); 
               _al_kcm_driver = &_al_kcm_alsa_driver;
               return 0;
            }
            return 1;
         #else
            _al_set_error(ALLEGRO_INVALID_PARAM, "ALSA not available on this platform");
            return 1;
         #endif

      case ALLEGRO_AUDIO_DRIVER_OSS:
         #if defined(ALLEGRO_CFG_KCM_OSS)
            if (_al_kcm_oss_driver.open() == 0) {
               fprintf(stderr, "Using OSS driver\n");
               _al_kcm_driver = &_al_kcm_oss_driver;
               return 0;
            }
            return 1;
         #else
            _al_set_error(ALLEGRO_INVALID_PARAM, "OSS not available on this platform");
            return 1;
         #endif

      case ALLEGRO_AUDIO_DRIVER_DSOUND:
         #if defined(ALLEGRO_CFG_KCM_DSOUND)
            if (_al_kcm_dsound_driver.open() == 0) {
               fprintf(stderr, "Using DirectSound driver\n"); 
               _al_kcm_driver = &_al_kcm_dsound_driver;
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

/* Function: al_install_audio
 */
int al_install_audio(ALLEGRO_AUDIO_DRIVER_ENUM mode)
{
   int ret = do_install_audio(mode);
   if (ret == 0) {
      _al_kcm_init_destructors();
   }
   return ret;
}

/* Function: al_uninstall_audio
 */
void al_uninstall_audio(void)
{
   if (_al_kcm_driver) {
      _al_kcm_shutdown_default_mixer();
      _al_kcm_shutdown_destructors();
      _al_kcm_driver->close();
      _al_kcm_driver = NULL;
   }
}

/* vim: set sts=3 sw=3 et: */
