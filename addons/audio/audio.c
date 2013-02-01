/**
 * Originally digi.c from allegro wiki
 * Original authors: KC/Milan
 *
 * Converted to allegro5 by Ryan Dickie
 */


#include <math.h>
#include <stdio.h>

#include "allegro5/allegro_audio.h"
#include "allegro5/internal/aintern_audio.h"
#include "allegro5/internal/aintern_audio_cfg.h"
#include "allegro5/internal/aintern.h"

ALLEGRO_DEBUG_CHANNEL("audio")

static ALLEGRO_EVENT_SOURCE audio_event_source;


void _al_set_error(int error, char* string)
{
   ALLEGRO_ERROR("%s (error code: %d)\n", string, error);
}

ALLEGRO_AUDIO_DRIVER *_al_kcm_driver = NULL;

#if defined(ALLEGRO_CFG_KCM_OPENAL)
   extern struct ALLEGRO_AUDIO_DRIVER _al_kcm_openal_driver;
#endif
#if defined(ALLEGRO_CFG_KCM_OPENSL)
   extern struct ALLEGRO_AUDIO_DRIVER _al_kcm_opensl_driver;
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
#if defined(ALLEGRO_CFG_KCM_AQUEUE)
   extern struct ALLEGRO_AUDIO_DRIVER _al_kcm_aqueue_driver;
#endif
#if defined(ALLEGRO_CFG_KCM_PULSEAUDIO)
   extern struct ALLEGRO_AUDIO_DRIVER _al_kcm_pulseaudio_driver;
#endif

/* Channel configuration helpers */

/* Function: al_get_channel_count
 */
size_t al_get_channel_count(ALLEGRO_CHANNEL_CONF conf)
{
   return (conf>>4)+(conf&0xF);
}

/* Depth configuration helpers */
/* Function: al_get_audio_depth_size
 */
size_t al_get_audio_depth_size(ALLEGRO_AUDIO_DEPTH depth)
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

/* FIXME: assumes 8-bit is unsigned, and all others are signed. */
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

/* Returns a silent sample frame. */
int _al_kcm_get_silence(ALLEGRO_AUDIO_DEPTH depth)
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
   ALLEGRO_CONFIG *config = al_get_system_config();
   const char *value;

   if (!config)
      return ALLEGRO_AUDIO_DRIVER_AUTODETECT;

   value = al_get_config_value(config, "audio", "driver");
   if (!value || value[0] == '\0')
      return ALLEGRO_AUDIO_DRIVER_AUTODETECT;

   if (0 == _al_stricmp(value, "ALSA"))
      return ALLEGRO_AUDIO_DRIVER_ALSA;

   if (0 == _al_stricmp(value, "OPENAL"))
      return ALLEGRO_AUDIO_DRIVER_OPENAL;
   
   if (0 == _al_stricmp(value, "OPENSL"))
      return ALLEGRO_AUDIO_DRIVER_OPENSL;

   if (0 == _al_stricmp(value, "OSS"))
      return ALLEGRO_AUDIO_DRIVER_OSS;

   if (0 == _al_stricmp(value, "PULSEAUDIO"))
      return ALLEGRO_AUDIO_DRIVER_PULSEAUDIO;

   if (0 == _al_stricmp(value, "DSOUND") || 0 == _al_stricmp(value, "DIRECTSOUND"))
      return ALLEGRO_AUDIO_DRIVER_DSOUND;

   return ALLEGRO_AUDIO_DRIVER_AUTODETECT;
}

static bool do_install_audio(ALLEGRO_AUDIO_DRIVER_ENUM mode)
{
   bool retVal;

   /* check to see if a driver is already installed and running */
   if (_al_kcm_driver) {
      _al_set_error(ALLEGRO_GENERIC_ERROR, "A driver already running");
      return false;
   }

   if (mode == ALLEGRO_AUDIO_DRIVER_AUTODETECT) {
      mode = get_config_audio_driver();
   }

   switch (mode) {
      case ALLEGRO_AUDIO_DRIVER_AUTODETECT:
#if defined(ALLEGRO_CFG_KCM_AQUEUE)
         retVal = do_install_audio(ALLEGRO_AUDIO_DRIVER_AQUEUE);
         if (retVal)
            return retVal;
#endif
/* If a PA server is running, we should use it by default as it will
 * hijack ALSA and OSS and using those then just means extra lag.
 * 
 * FIXME: Detect if no PA server is running and in that case prefer
 * ALSA and OSS first.
 */
#if defined(ALLEGRO_CFG_KCM_PULSEAUDIO)
         retVal = do_install_audio(ALLEGRO_AUDIO_DRIVER_PULSEAUDIO);
         if (retVal)
            return retVal;
#endif
#if defined(ALLEGRO_CFG_KCM_ALSA)
         retVal = do_install_audio(ALLEGRO_AUDIO_DRIVER_ALSA);
         if (retVal)
            return retVal;
#endif
#if defined(ALLEGRO_CFG_KCM_DSOUND)
         retVal = do_install_audio(ALLEGRO_AUDIO_DRIVER_DSOUND);
         if (retVal)
            return retVal;
#endif
#if defined(ALLEGRO_CFG_KCM_OSS)
         retVal = do_install_audio(ALLEGRO_AUDIO_DRIVER_OSS);
         if (retVal)
            return retVal;
#endif
#if defined(ALLEGRO_CFG_KCM_OPENAL)
         retVal = do_install_audio(ALLEGRO_AUDIO_DRIVER_OPENAL);
         if (retVal)
            return retVal;
#endif
#if defined(ALLEGRO_CFG_KCM_OPENSL)
         retVal = do_install_audio(ALLEGRO_AUDIO_DRIVER_OPENSL);
         if (retVal)
            return retVal;
#endif

         _al_set_error(ALLEGRO_INVALID_PARAM, "No audio driver can be used.");
         _al_kcm_driver = NULL;
         return false;

      case ALLEGRO_AUDIO_DRIVER_AQUEUE:
         #if defined(ALLEGRO_CFG_KCM_AQUEUE)
            if (_al_kcm_aqueue_driver.open() == 0) {
               ALLEGRO_INFO("Using Apple Audio Queue driver\n"); 
               _al_kcm_driver = &_al_kcm_aqueue_driver;
               return true;
            }
            return false;
         #else
            _al_set_error(ALLEGRO_INVALID_PARAM, "Audio Queue driver not available on this platform");
            return false;
         #endif

      case ALLEGRO_AUDIO_DRIVER_OPENAL:
         #if defined(ALLEGRO_CFG_KCM_OPENAL)
            if (_al_kcm_openal_driver.open() == 0) {
               ALLEGRO_INFO("Using OpenAL driver\n"); 
               _al_kcm_driver = &_al_kcm_openal_driver;
               return true;
            }
            return false;
         #else
            _al_set_error(ALLEGRO_INVALID_PARAM, "OpenAL not available on this platform");
            return false;
         #endif

      case ALLEGRO_AUDIO_DRIVER_OPENSL:
         #if defined(ALLEGRO_CFG_KCM_OPENSL)
            if (_al_kcm_opensl_driver.open() == 0) {
               ALLEGRO_INFO("Using OpenSL driver\n"); 
               _al_kcm_driver = &_al_kcm_opensl_driver;
               return true;
            }
            return false;
         #else
            _al_set_error(ALLEGRO_INVALID_PARAM, "OpenSL not available on this platform");
            return false;
         #endif

      case ALLEGRO_AUDIO_DRIVER_ALSA:
         #if defined(ALLEGRO_CFG_KCM_ALSA)
            if (_al_kcm_alsa_driver.open() == 0) {
               ALLEGRO_INFO("Using ALSA driver\n"); 
               _al_kcm_driver = &_al_kcm_alsa_driver;
               return true;
            }
            return false;
         #else
            _al_set_error(ALLEGRO_INVALID_PARAM, "ALSA not available on this platform");
            return false;
         #endif

      case ALLEGRO_AUDIO_DRIVER_OSS:
         #if defined(ALLEGRO_CFG_KCM_OSS)
            if (_al_kcm_oss_driver.open() == 0) {
               ALLEGRO_INFO("Using OSS driver\n");
               _al_kcm_driver = &_al_kcm_oss_driver;
               return true;
            }
            return false;
         #else
            _al_set_error(ALLEGRO_INVALID_PARAM, "OSS not available on this platform");
            return false;
         #endif

      case ALLEGRO_AUDIO_DRIVER_PULSEAUDIO:
         #if defined(ALLEGRO_CFG_KCM_PULSEAUDIO)
            if (_al_kcm_pulseaudio_driver.open() == 0) {
               ALLEGRO_INFO("Using PulseAudio driver\n");
               _al_kcm_driver = &_al_kcm_pulseaudio_driver;
               return true;
            }
            return false;
         #else
            _al_set_error(ALLEGRO_INVALID_PARAM, "PulseAudio not available on this platform");
            return false;
         #endif

      case ALLEGRO_AUDIO_DRIVER_DSOUND:
         #if defined(ALLEGRO_CFG_KCM_DSOUND)
            if (_al_kcm_dsound_driver.open() == 0) {
               ALLEGRO_INFO("Using DirectSound driver\n"); 
               _al_kcm_driver = &_al_kcm_dsound_driver;
               return true;
            }
            return false;
         #else
            _al_set_error(ALLEGRO_INVALID_PARAM, "DirectSound not available on this platform");
            return false;
         #endif

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Invalid audio driver");
         return false;
   }
}

/* Function: al_install_audio
 */
bool al_install_audio(void)
{
   bool ret;

   if (_al_kcm_driver)
      return true;

   /* The destructors are initialised even if the audio driver fails to install
    * because the user may still create samples.
    */
   _al_kcm_init_destructors();
   _al_add_exit_func(al_uninstall_audio, "al_uninstall_audio");

   ret = do_install_audio(ALLEGRO_AUDIO_DRIVER_AUTODETECT);
   if (ret) {
      al_init_user_event_source(&audio_event_source);
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
      al_destroy_user_event_source(&audio_event_source);
   }
   else {
      _al_kcm_shutdown_destructors();
   }
}

/* Function: al_is_audio_installed
 */
bool al_is_audio_installed(void)
{
   return _al_kcm_driver ? true : false;
}

/* Function: al_get_allegro_audio_version
 */
uint32_t al_get_allegro_audio_version(void)
{
   return ALLEGRO_VERSION_INT;
}

/* This doesn't need to do anything yet but may if we add some data
 * to some events.
 */
static void destroy_audio_event(ALLEGRO_USER_EVENT *e)
{
   (void)e;
}

void _al_emit_audio_event(int event_type)
{
   ALLEGRO_EVENT event;
   ASSERT(_al_kcm_driver);

   event.type = event_type;
   al_emit_user_event(&audio_event_source, &event, destroy_audio_event);
}

/* Function: al_get_audio_event_source
 */
ALLEGRO_EVENT_SOURCE *al_get_audio_event_source(void)
{
   return &audio_event_source;
}

/* vim: set sts=3 sw=3 et: */
