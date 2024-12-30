/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Open Sound System sound driver.
 *
 *      By Milan Mimica.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_audio.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <poll.h>

ALLEGRO_DEBUG_CHANNEL("oss")

#if defined ALLEGRO_HAVE_SOUNDCARD_H
  #include <soundcard.h>
#elif defined ALLEGRO_HAVE_SYS_SOUNDCARD_H
  #include <sys/soundcard.h>
#elif defined ALLEGRO_HAVE_LINUX_SOUNDCARD_H
  #include <linux/soundcard.h>
#elif defined ALLEGRO_HAVE_MACHINE_SOUNDCARD_H
  #include <machine/soundcard.h>
#endif

#if OSS_VERSION >= 0x040000
   #define OSS_VER_4
#else
   #define OSS_VER_3
#endif

#ifndef AFMT_S16_NE
   #ifdef ALLEGRO_BIG_ENDIAN
      #define AFMT_S16_NE AFMT_S16_BE
   #else
      #define AFMT_S16_NE AFMT_S16_LE
   #endif
#endif
#ifndef AFMT_U16_NE
   #ifdef ALLEGRO_BIG_ENDIAN
      #define AFMT_U16_NE AFMT_U16_BE
   #else
      #define AFMT_U16_NE AFMT_U16_LE
   #endif
#endif


/* Audio device used by OSS3.
 * Make this configurable. */
static const char* oss_audio_device_ver3 = "/dev/dsp";

/* Audio device is dynamically retrived in OSS4. */
static char oss_audio_device[512];

/* timing policy (between 0 and 10), used by OSS4
 * Make this configurable? */
#ifdef OSS_VER_4
static const int oss_timing_policy = 5;
#endif

/* Fragment size, used by OSS3
 * Make this configurable? */
static int oss_fragsize = (8 << 16) | (10);

/* Auxiliary buffer used to store silence. */
#define SIL_BUF_SIZE 1024

static bool using_ver_4;


typedef struct OSS_VOICE {
   int fd;
   int volume;

   /* Copied from the parent ALLEGRO_VOICE. Used for convenince. */
   unsigned int len; /* in frames */
   unsigned int frame_size; /* in bytes */

   volatile bool stopped;
   volatile bool stop;

   ALLEGRO_THREAD *poll_thread;
} OSS_VOICE;


#ifdef OSS_VER_4
static int oss_open_ver4()
{
   int mixer_fd, i;
   oss_sysinfo sysinfo;

   if ((mixer_fd = open("/dev/mixer", O_RDWR, 0)) == -1) {
      switch (errno) {
         case ENXIO:
         case ENODEV:
            ALLEGRO_ERROR("Open Sound System is not running in your system.\n");
         break;

         case ENOENT:
            ALLEGRO_ERROR("No /dev/mixer device available in your system.\n");
            ALLEGRO_ERROR("Perhaps Open Sound System is not installed or "
                          "running.\n");
         break;

         default:
            ALLEGRO_ERROR("errno: %i -- %s\n", errno, strerror(errno));
      }

      return 1;
   }

   if (ioctl(mixer_fd, SNDCTL_SYSINFO, &sysinfo) == -1) {
      if (errno == ENXIO) {
         ALLEGRO_ERROR("OSS has not detected any supported sound hardware in "
                       "your system.\n");
      }
      else if (errno == EINVAL) {
         ALLEGRO_INFO("The version of OSS installed on the system is not "
                      "compatible with OSS4.\n");
      }
      else
         ALLEGRO_ERROR("errno: %i -- %s\n", errno, strerror(errno));

      close(mixer_fd);
      return 1;
   }

   /* Some OSS implementations (ALSA emulation) don't fail on SNDCTL_SYSINFO even
    * though they don't support OSS4. They *seem* to set numcards to 0. */
   if (sysinfo.numcards < 1) {
      ALLEGRO_WARN("The version of OSS installed on the system is not "
                   "compatible with OSS4.\n");
      return 1;
   }

   ALLEGRO_INFO("OSS Version: %s\n", sysinfo.version);
   ALLEGRO_INFO("Found %i sound cards.\n", sysinfo.numcards);

   for (i = 0; i < sysinfo.numcards; i++) {
      oss_audioinfo audioinfo;
      memset(&audioinfo, 0, sizeof(oss_audioinfo));
      audioinfo.dev = i;

      ALLEGRO_INFO("Trying sound card no. %i ...\n", audioinfo.dev);

      ioctl(mixer_fd, SNDCTL_AUDIOINFO, &audioinfo);

      if (audioinfo.enabled) {
         if (strlen(audioinfo.devnode)) {
            strncpy(oss_audio_device, audioinfo.devnode, 511);
         }
         else if (audioinfo.legacy_device != -1) {
            sprintf(oss_audio_device, "/dev/dsp%i", audioinfo.legacy_device);
         }
         else {
            ALLEGRO_ERROR("Cannot find device name.\n");
         }

         ALLEGRO_INFO("Using device: %s\n", oss_audio_device);

         break;
      }
      else {
         ALLEGRO_INFO("Device disabled.\n");
      }
   }

   if (i == sysinfo.numcards) {
      ALLEGRO_ERROR("Couldn't find a suitable device.\n");
      close(mixer_fd);
      return 1;
   }

   close(mixer_fd);

   using_ver_4 = true;

   return 0;
}
#endif

static int oss_open_ver3(void)
{
   const char *config_device;
   config_device = al_get_config_value(al_get_system_config(),
      "oss", "device");
   if (config_device && config_device[0] != '\0')
      oss_audio_device_ver3 = config_device;

   int fd = open(oss_audio_device_ver3, O_WRONLY);
   if (fd == -1) {
      switch (errno) {
         case ENXIO:
         case ENODEV:
            ALLEGRO_ERROR("Open Sound System is not running in your "
                          "system.\n");
         break;

         case ENOENT:
            ALLEGRO_ERROR("No '%s' device available in your system.\n",
               oss_audio_device_ver3);
            ALLEGRO_ERROR("Perhaps Open Sound System is not installed "
                          "or running.\n");
         break;

         default:
            ALLEGRO_ERROR("errno: %i -- %s\n", errno, strerror(errno));
      }

      return 1;
   }

   close(fd);
   strncpy(oss_audio_device, oss_audio_device_ver3, 511);
   ALLEGRO_INFO("Using device: %s\n", oss_audio_device);

   using_ver_4 = false;

   return 0;
}


static int oss_open(void)
{
   bool force_oss3 = false;
   const char *force_oss3_cfg;
   force_oss3_cfg = al_get_config_value(al_get_system_config(), "oss",
      "force_ver3");
   if (force_oss3_cfg && force_oss3_cfg[0] != '\0')
      force_oss3 = strcmp(force_oss3_cfg, "yes") ? false : true;

   if (force_oss3) {
      ALLEGRO_WARN("Skipping OSS4 probe.\n");
   }

#ifdef OSS_VER_4
   bool inited = false;
   if (!force_oss3) {
      if (oss_open_ver4())
         ALLEGRO_WARN("OSS ver. 4 init failed, trying ver. 3...\n");
      else
         inited = true;
   }
   if (!inited && oss_open_ver3()) {
      ALLEGRO_ERROR("Failed to init OSS.\n");
      return 1;
   }
#else
   ALLEGRO_INFO("OSS4 support not compiled in. Skipping OSS4 probe.\n");
   if (oss_open_ver3()) {
      ALLEGRO_ERROR("Failed to init OSS.\n");
      return 1;
   }
#endif

   return 0;
}


static void oss_close(void)
{
}


static void oss_deallocate_voice(ALLEGRO_VOICE *voice)
{
   OSS_VOICE *oss_voice = voice->extra;

   /* We do NOT hold the voice mutex here, so this does NOT result in a
    * deadlock when oss_update calls _al_voice_update (which tries to
    * acquire the voice->mutex).
    */
   al_join_thread(oss_voice->poll_thread, NULL);
   al_destroy_thread(oss_voice->poll_thread);

   close(oss_voice->fd);
   al_free(voice->extra);
   voice->extra = NULL;
}


static int oss_start_voice(ALLEGRO_VOICE *voice)
{
   OSS_VOICE *ex_data = voice->extra;
   ex_data->stop = false;
   return 0;
}


static int oss_stop_voice(ALLEGRO_VOICE *voice)
{
   OSS_VOICE *ex_data = voice->extra;

   ex_data->stop = true;
   if (!voice->is_streaming) {
      voice->attached_stream->pos = 0;
   }

   while (!ex_data->stopped)
      al_rest(0.001);

   return 0;
}


static int oss_load_voice(ALLEGRO_VOICE *voice, const void *data)
{
   OSS_VOICE *ex_data = voice->extra;

   /*
    * One way to support backward playing would be to do like alsa driver does:
    * mmap(2) the FD and write reversed samples into that. To much trouble for
    * an optional feature IMO. -- milan
    */
   if (voice->attached_stream->loop == ALLEGRO_PLAYMODE_BIDIR) {
      ALLEGRO_INFO("Backwards playing not supported by the driver.\n");
      return -1;
   }

   voice->attached_stream->pos = 0;
   ex_data->len = voice->attached_stream->spl_data.len;

   return 0;
   (void)data;
}


static void oss_unload_voice(ALLEGRO_VOICE *voice)
{
   (void)voice;
}


static bool oss_voice_is_playing(const ALLEGRO_VOICE *voice)
{
   OSS_VOICE *ex_data = voice->extra;
   return !ex_data->stopped;
}


static unsigned int oss_get_voice_position(const ALLEGRO_VOICE *voice)
{
   return voice->attached_stream->pos;
}


static int oss_set_voice_position(ALLEGRO_VOICE *voice, unsigned int val)
{
   voice->attached_stream->pos = val;
   return 0;
}



/*
 * Updates the supplied non-streaming voice.
 * buf   - Returns a pointer to the buffer containing sample data.
 * bytes - The requested size of the sample data buffer. Returns the actual
 *         size of returned the buffer.
 * Updates 'stop', 'pos' and 'reversed' fields of the supplied voice to the
 * future position.
 */
static int oss_update_nonstream_voice(ALLEGRO_VOICE *voice, void **buf, int *bytes)
{
   OSS_VOICE *oss_voice = voice->extra;
   int bpos = voice->attached_stream->pos * oss_voice->frame_size;
   int blen = oss_voice->len * oss_voice->frame_size;

   *buf = (char *)voice->attached_stream->spl_data.buffer.ptr + bpos;

   if (bpos + *bytes > blen) {
      *bytes = blen - bpos;
      if (voice->attached_stream->loop == ALLEGRO_PLAYMODE_ONCE) {
         oss_voice->stop = true;
         voice->attached_stream->pos = 0;
      }
      if (voice->attached_stream->loop == ALLEGRO_PLAYMODE_LOOP) {
         voice->attached_stream->pos = 0;
      }
      /*else if (voice->attached_stream->loop == ALLEGRO_PLAYMODE_BIDIR) {
         oss_voice->reversed = true;
         voice->attached_stream->pos = oss_voice->len;
      }*/
      return 1;
   }
   else
      voice->attached_stream->pos += *bytes / oss_voice->frame_size;

   return 0;
}


static void oss_update_silence(ALLEGRO_VOICE *voice, OSS_VOICE *oss_voice)
{
   char sil_buf[SIL_BUF_SIZE];
   unsigned int silent_samples;

   silent_samples = SIL_BUF_SIZE /
      (al_get_audio_depth_size(voice->depth) *
       al_get_channel_count(voice->chan_conf));
   al_fill_silence(sil_buf, silent_samples, voice->depth, voice->chan_conf);
   if (write(oss_voice->fd, sil_buf, SIL_BUF_SIZE) == -1) {
      ALLEGRO_ERROR("errno: %i -- %s\n", errno, strerror(errno));
   }
}


static void* oss_update(ALLEGRO_THREAD *self, void *arg)
{
   ALLEGRO_VOICE *voice = arg;
   OSS_VOICE *oss_voice = voice->extra;
   (void)self;

   while (!al_get_thread_should_stop(self)) {
      /*
      For possible eventual non-blocking mode:

      audio_buf_info bi;

      if (ioctl(oss_voice->fd, SNDCTL_DSP_GETOSPACE, &bi) == -1) {
         ALLEGRO_ERROR("Error SNDCTL_DSP_GETOSPACE, errno=%i (%s)\n",
            errno, strerror(errno));
         return NULL;
      }

      len = bi.bytes;
      */

      /* How many bytes are we supposed to try to write at once? */
      unsigned int frames = 1024;

      if (oss_voice->stop && !oss_voice->stopped) {
         oss_voice->stopped = true;
      }

      if (!oss_voice->stop && oss_voice->stopped) {
         oss_voice->stopped = false;
      }

      if (!voice->is_streaming && !oss_voice->stopped) {
         void *buf;
         int bytes = frames * oss_voice->frame_size;

         oss_update_nonstream_voice(voice, &buf, &bytes);
         frames = bytes / oss_voice->frame_size;
         if (write(oss_voice->fd, buf, bytes) == -1) {
            ALLEGRO_ERROR("errno: %i -- %s\n", errno, strerror(errno));
            if (errno != EINTR)
               return NULL;
         }
      }
      else if (voice->is_streaming && !oss_voice->stopped) {
         const void *data = _al_voice_update(voice, voice->mutex, &frames);
         if (data == NULL) {
            oss_update_silence(voice, oss_voice);
            continue;
         }
         if (write(oss_voice->fd, data, frames * oss_voice->frame_size) == -1) {
            ALLEGRO_ERROR("errno: %i -- %s\n", errno, strerror(errno));
            if (errno != EINTR)
               return NULL;
         }
      }
      else {
         /* If stopped just fill with silence. */
         oss_update_silence(voice, oss_voice);
      }
   }

   return NULL;
}


static int oss_allocate_voice(ALLEGRO_VOICE *voice)
{
   int format;
   int chan_count;

   OSS_VOICE *ex_data = al_calloc(1, sizeof(OSS_VOICE));
   if (!ex_data)
      return 1;

   ex_data->fd = open(oss_audio_device, O_WRONLY/*, O_NONBLOCK*/);
   if (ex_data->fd == -1) {
      ALLEGRO_ERROR("Failed to open audio device '%s'.\n",
            oss_audio_device);
      ALLEGRO_ERROR("errno: %i -- %s\n", errno, strerror(errno));
      al_free(ex_data);
      return 1;
   }

   chan_count = al_get_channel_count(voice->chan_conf);

   ex_data->frame_size = chan_count * al_get_audio_depth_size(voice->depth);
   if (!ex_data->frame_size)
      goto Error;

   ex_data->stop = true;
   ex_data->stopped = true;

   if (voice->depth == ALLEGRO_AUDIO_DEPTH_INT8)
      format = AFMT_S8;
   else if (voice->depth == ALLEGRO_AUDIO_DEPTH_UINT8)
      format = AFMT_U8;
   else if (voice->depth == ALLEGRO_AUDIO_DEPTH_INT16)
      format = AFMT_S16_NE;
   else if (voice->depth == ALLEGRO_AUDIO_DEPTH_UINT16)
      format = AFMT_U16_NE;
#ifdef OSS_VER_4
   else if (voice->depth == ALLEGRO_AUDIO_DEPTH_INT24)
      format = AFMT_S24_NE;
   else if (voice->depth == ALLEGRO_AUDIO_DEPTH_FLOAT32)
      format = AFMT_FLOAT;
#endif
   else {
      ALLEGRO_ERROR("Unsupported OSS sound format.\n");
      goto Error;
   }

   int tmp_format = format;
   int tmp_chan_count = chan_count;
   unsigned int tmp_freq = voice->frequency;
   int tmp_oss_fragsize = oss_fragsize;

   if (using_ver_4) {
#ifdef OSS_VER_4
      int tmp_oss_timing_policy = oss_timing_policy;
      if (ioctl(ex_data->fd, SNDCTL_DSP_POLICY, &tmp_oss_timing_policy) == -1) {
         ALLEGRO_ERROR("Failed to set_timig policity to '%i'.\n",
               tmp_oss_timing_policy);
         ALLEGRO_ERROR("errno: %i -- %s\n", errno, strerror(errno));
         goto Error;
      }
      ALLEGRO_INFO("Accepted timing policy value: %i\n", tmp_oss_timing_policy);
#endif
   }
   else {
      if (ioctl(ex_data->fd, SNDCTL_DSP_SETFRAGMENT, &tmp_oss_fragsize) == -1) {
          ALLEGRO_ERROR("Failed to set fragment size.\n");
          ALLEGRO_ERROR("errno: %i -- %s\n", errno, strerror(errno));
          goto Error;
      }
   }

   if (ioctl(ex_data->fd, SNDCTL_DSP_SETFMT, &tmp_format) == -1) {
      ALLEGRO_ERROR("Failed to set sample format.\n");
      ALLEGRO_ERROR("errno: %i -- %s\n", errno, strerror(errno));
      goto Error;
   }
   if (tmp_format != format) {
      ALLEGRO_ERROR("Sample format not supported by the driver.\n");
      goto Error;
   }

   if (ioctl(ex_data->fd, SNDCTL_DSP_CHANNELS, &tmp_chan_count)) {
      ALLEGRO_ERROR("Failed to set channel count.\n");
      ALLEGRO_ERROR("errno: %i -- %s\n", errno, strerror(errno));
      goto Error;
   }
   if (tmp_chan_count != chan_count) {
      ALLEGRO_ERROR("Requested sample channe count %i, got %i.\n",
            tmp_chan_count, chan_count);
   }

   if (ioctl(ex_data->fd, SNDCTL_DSP_SPEED, &tmp_freq) == -1) {
      ALLEGRO_ERROR("Failed to set sample rate.\n");
      ALLEGRO_ERROR("errno: %i -- %s\n", errno, strerror(errno));
      goto Error;
   }
   if (voice->frequency != tmp_freq) {
      ALLEGRO_ERROR("Requested sample rate %u, got %iu.\n", voice->frequency,
            tmp_freq);
   }

   voice->extra = ex_data;
   ex_data->poll_thread = al_create_thread(oss_update, (void*)voice);
   al_start_thread(ex_data->poll_thread);

   return 0;

Error:
   close(ex_data->fd);
   al_free(ex_data);
   return 1;
}


ALLEGRO_AUDIO_DRIVER _al_kcm_oss_driver =
{
   "OSS",

   oss_open,
   oss_close,

   oss_allocate_voice,
   oss_deallocate_voice,

   oss_load_voice,
   oss_unload_voice,

   oss_start_voice,
   oss_stop_voice,

   oss_voice_is_playing,

   oss_get_voice_position,
   oss_set_voice_position,

   NULL,
   NULL,

   NULL
};

/* vim: set sts=3 sw=3 et: */
