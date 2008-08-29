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
 *      Open Sound System ver. 4 sound driver.
 *
 *      By Milan Mimica.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_kcm_audio.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <poll.h>


/*

  TODO: Write support for OSS ver. < 4, which is far more common.

 */

#if defined ALLEGRO_HAVE_SOUNDCARD_H
  #include <soundcard.h>
#elif defined ALLEGRO_HAVE_SYS_SOUNDCARD_H
  #include <sys/soundcard.h>
#elif defined ALLEGRO_HAVE_LINUX_SOUNDCARD_H
  #include <linux/soundcard.h>
#elif defined ALLEGRO_HAVE_MACHINE_SOUNDCARD_H
  #include <machine/soundcard.h>
#endif

#define PREFIX_E "a5-oss Error: "
#define PREFIX_N "a5-oss Notice: "


static char oss_audio_device[512];

/* the timing policy (between 0 and 10) */
static const int oss_timing_policy = 5;


typedef struct OSS_VOICE {
   int fd;
   int volume;

   unsigned int len; /* in frames */
   unsigned int pos; /* in frames */
   unsigned int frame_size; /* in bytes */

   bool stopped;
   volatile bool stop;

   ALLEGRO_THREAD *poll_thread;
   volatile bool quit_poll_thread;
} OSS_VOICE;


/* fills oss_audio_device */
static int oss_open()
{
   int mixer_fd, i;
   oss_sysinfo sysinfo;

   if ((mixer_fd = open("/dev/mixer", O_RDWR, 0)) == -1) {
      switch (errno) {
         case ENXIO:
         case ENODEV:
            TRACE(PREFIX_E "Open Sound System is not running in your");
            TRACE("system.\n");
         break;

         case ENOENT:
            TRACE(PREFIX_E "No /dev/mixer device available in your");
            TRACE("system.\n");
            TRACE(PREFIX_E "Perhaps Open Sound System is not installed");
            TRACE("or running.\n");
         break;

         default:
            TRACE(PREFIX_E "errno: %i -- %s\n", errno, strerror(errno));
      }

      return 1;
   }

   if (ioctl(mixer_fd, SNDCTL_SYSINFO, &sysinfo) == -1) {
      if (errno == ENXIO) {
         TRACE(PREFIX_E "OSS has not detected any supported sound");
         TRACE("hardware in your system.\n");
      }
      else if (errno == EINVAL) {
         TRACE(PREFIX_E "OSS version 4.0 or later is required\n");
      }
      else
         TRACE(PREFIX_E "errno: %i -- %s\n", errno, strerror(errno));

      close(mixer_fd);
      return 1;
   }

   /* Some OSS implementations (ALSA emulation) don't fail on SNDCTL_SYSINFO even
    * though they don't support OSS4. They *seem* to set numcards to 0. */
   if (sysinfo.numcards < 1) {
      TRACE(PREFIX_E "OSS has not detected any supported sound hardware.\n");
      TRACE(PREFIX_E "OSS version 4.0 or later is required.\n");
      return 1;
   }

   TRACE(PREFIX_N "OSS Version: %s\n", sysinfo.version);
   TRACE(PREFIX_N "Found %i sound cards.\n", sysinfo.numcards);

   for (i = 0; i < sysinfo.numcards; i++) {
      oss_audioinfo audioinfo;
      memset(&audioinfo, 0, sizeof(oss_audioinfo));
      audioinfo.dev = i;

      TRACE(PREFIX_N "Trying sound card no. %i ...\n", audioinfo.dev);

      ioctl(mixer_fd, SNDCTL_AUDIOINFO, &audioinfo);

      if (audioinfo.enabled) {
         if (strlen(audioinfo.devnode)) {
            strncpy(oss_audio_device, audioinfo.devnode, 512);
         }
         else if (audioinfo.legacy_device != -1) {
            sprintf(oss_audio_device, "/dev/dsp%i", audioinfo.legacy_device);
         }
         else {
            TRACE(PREFIX_E "Cannot find device name.\n");
         }

         TRACE(PREFIX_N "Using device: %s\n", oss_audio_device);

         break;
      }
      else {
         TRACE(PREFIX_N "Device disabled.\n");
      }
   }

   if (i == sysinfo.numcards) {
      TRACE(PREFIX_E "Couldn't find a suitable device.\n");
      close(mixer_fd);
      return 1;
   }

   close(mixer_fd);

   return 0;
}


static void oss_close(void)
{
}


static void oss_deallocate_voice(ALLEGRO_VOICE *voice)
{
   OSS_VOICE *oss_voice = voice->extra;

   oss_voice->quit_poll_thread = true;
   al_join_thread(oss_voice->poll_thread, NULL);

   close(oss_voice->fd);
   free(voice->extra);
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
      ex_data->pos = 0;
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
      TRACE(PREFIX_N "Backwards playing not supported by the driver.\n");
      return -1;
   }

   ex_data->pos = 0;
   ex_data->len = voice->attached_stream->len >> MIXER_FRAC_SHIFT;

   return 0;
   (void)data;
}


static void oss_unload_voice(ALLEGRO_VOICE *voice)
{
   OSS_VOICE *ex_data = voice->extra;
}


static bool oss_voice_is_playing(const ALLEGRO_VOICE *voice)
{
   OSS_VOICE *ex_data = voice->extra;
   return !ex_data->stopped;
}


static unsigned long oss_get_voice_position(const ALLEGRO_VOICE *voice)
{
   OSS_VOICE *ex_data = voice->extra;
   return ex_data->pos;
}


static int oss_set_voice_position(ALLEGRO_VOICE *voice, unsigned long val)
{
   OSS_VOICE *ex_data = voice->extra;
   ex_data->pos = val;
   return 0;
}



/*
 * Updates the supplied non-streaming voice.
 * buf   - Returns a pointer to the buffer containing sample data.
 * bytes - The requested size of the sample data buffer. Returns the actual
 *         size of returned the buffer.
 * Updates 'stop', 'pos' and 'reversed' fields of the supplied voice.
 */
static int oss_update_nonstream_voice(ALLEGRO_VOICE *voice, void **buf, int *bytes)
{
   OSS_VOICE *oss_voice = voice->extra;
   int bpos = oss_voice->pos * oss_voice->frame_size;
   int blen = oss_voice->len * oss_voice->frame_size;

   *buf = voice->attached_stream->buffer.ptr + bpos;

   if (bpos + *bytes > blen) {
      *bytes = blen - bpos;
      if (voice->attached_stream->loop == ALLEGRO_PLAYMODE_ONCE) {
         oss_voice->stop = true;
         oss_voice->pos = 0;
      }
      if (voice->attached_stream->loop == ALLEGRO_PLAYMODE_ONEDIR) {
         oss_voice->pos = 0;
      }
      /*else if (voice->attached_stream->loop == ALLEGRO_PLAYMODE_BIDIR) {
         oss_voice->reversed = true;
         oss_voice->pos = oss_voice->len;
      }*/
      return 1;
   }
   else
      oss_voice->pos += *bytes / oss_voice->frame_size;

   return 0;
}


static void* oss_update(ALLEGRO_THREAD *self, void *arg)
{
   ALLEGRO_VOICE *voice = arg;
   OSS_VOICE *oss_voice = voice->extra;
   audio_buf_info bi;
   void *buf;

   while (!oss_voice->quit_poll_thread) {
      /*
      For possible eventual non-blocking mode:

      if (ioctl(oss_voice->fd, SNDCTL_DSP_GETOSPACE, &bi) == -1) {
         TRACE(PREFIX_E "Error SNDCTL_DSP_GETOSPACE.\n");
         TRACE(PREFIX_E "errno: %i -- %s\n", errno, strerror(errno));
         return NULL;
      }

      len = bi.bytes;
      */

      int frames = 1024; /* How many bytes are we supposed to try to
                          * write at once? */


      if (oss_voice->stop && !oss_voice->stopped) {
         oss_voice->stopped = true;
         ioctl(oss_voice->fd, SNDCTL_DSP_SKIP, NULL);
      }

      if (!oss_voice->stop && oss_voice->stopped) {
         oss_voice->stopped = false;
         ioctl(oss_voice->fd, SNDCTL_DSP_SKIP, NULL);
      }

      if (!voice->is_streaming && !oss_voice->stopped) {
         void *buf;
         int bytes = frames * oss_voice->frame_size;

         oss_update_nonstream_voice(voice, &buf, &bytes);
         frames = bytes / oss_voice->frame_size;
         if (write(oss_voice->fd, buf, bytes) == -1) {
            TRACE(PREFIX_E "errno: %i -- %s\n", errno, strerror(errno));
            return NULL;
         }
      }
      else if (voice->is_streaming && !oss_voice->stopped) {
         const void *data = _al_voice_update(voice, frames);
         if (data == NULL)
            goto silence;

         if (write(oss_voice->fd, data, frames * oss_voice->frame_size) == -1) {
            TRACE(PREFIX_E "errno: %i -- %s\n", errno, strerror(errno));
            return NULL;
         }
      }
      else {
         int silence;
silence:
         /* If stopped just fill with silence. */
         ioctl(oss_voice->fd, SNDCTL_DSP_SILENCE, NULL);
      }
   }

   return NULL;
}


static int oss_allocate_voice(ALLEGRO_VOICE *voice)
{
   int format;
   int chan_count;
   unsigned int req_freq;

   OSS_VOICE *ex_data = calloc(1, sizeof(OSS_VOICE));
   if (!ex_data)
      return 1;

   ex_data->fd = open(oss_audio_device, O_WRONLY/*, O_NONBLOCK*/);
   if (ex_data->fd == -1) {
      TRACE(PREFIX_E "Failed to open audio device '%s'.\n",
            oss_audio_device);
      TRACE(PREFIX_E "errno: %i -- %s\n", errno, strerror(errno));
      free(ex_data);
      return 1;
   }

   chan_count = al_channel_count(voice->chan_conf);

   ex_data->frame_size = chan_count * al_depth_size(voice->depth);
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
   else if (voice->depth == ALLEGRO_AUDIO_DEPTH_INT24)
      format = AFMT_S24_NE;
   else if (voice->depth == ALLEGRO_AUDIO_DEPTH_FLOAT32)
      format = AFMT_FLOAT;
   else {
         TRACE(PREFIX_E "Unsupported OSS sound format.\n");
         goto Error;
   }

   int tmp_oss_timing_policy = oss_timing_policy;
   int tmp_format = format;
   int tmp_chan_count = chan_count;
   unsigned int tmp_freq = voice->frequency;

   if (ioctl(ex_data->fd, SNDCTL_DSP_POLICY, &tmp_oss_timing_policy) == -1) {
      TRACE(PREFIX_E "Failed to set_timig policity to '%i'.\n",
            tmp_oss_timing_policy);
      TRACE(PREFIX_E "errno: %i -- %s\n", errno, strerror(errno));
      goto Error;
   }
   TRACE(PREFIX_N "Accepted timing policy value: %i\n",
         tmp_oss_timing_policy);

   if (ioctl(ex_data->fd, SNDCTL_DSP_SETFMT, &tmp_format) == -1) {
      TRACE(PREFIX_E "Failed to set sample format.\n");
      TRACE(PREFIX_E "errno: %i -- %s\n", errno, strerror(errno));
      goto Error;
   }
   if (tmp_format != format) {
      TRACE(PREFIX_E "Sample format not supported by the driver.\n");
      goto Error;
   }

   if (ioctl(ex_data->fd, SNDCTL_DSP_CHANNELS, &tmp_chan_count)) {
      TRACE(PREFIX_E "Failed to set channel count.\n");
      TRACE(PREFIX_E "errno: %i -- %s\n", errno, strerror(errno));
      goto Error;
   }
   if (tmp_chan_count != chan_count) {
      TRACE(PREFIX_E "Requested sample channe count %i, got %i.\n",
            tmp_chan_count, chan_count);
   }

   if (ioctl(ex_data->fd, SNDCTL_DSP_SPEED, &tmp_freq) == -1) {
      TRACE(PREFIX_E "Failed to set sample rate.\n");
      TRACE(PREFIX_E "errno: %i -- %s\n", errno, strerror(errno));
      goto Error;
   }
   if (voice->frequency != tmp_freq) {
      TRACE(PREFIX_E "Requested sample rate %lu, got %iu.\n", voice->frequency,
            tmp_freq);
   }

   voice->extra = ex_data;
   ex_data->quit_poll_thread = false;
   ex_data->poll_thread = al_create_thread(oss_update, (void*)voice);
   al_start_thread(ex_data->poll_thread);

   return 0;

Error:
   close(ex_data->fd);
   free(ex_data);
   return 1;
}


ALLEGRO_AUDIO_DRIVER _oss_driver =
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
   oss_set_voice_position
};

/* vim: set sts=3 sw=3 et: */
