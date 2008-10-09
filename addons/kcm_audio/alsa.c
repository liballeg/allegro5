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
 *      ALSA 1.0 sound driver.
 *
 *      By Thomas Fjellstrom.
 *
 *      Extensively modified by Elias Pschernig.
 *
 *      Rewritten for 4.3 sound API by Milan Mimica, with additional
 *      improvements by Chris Robinson. Updated for 4.9 by Ryan Dickie
 *
 *      See readme.txt for copyright information.
 */

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_kcm_audio.h"

#include <alsa/asoundlib.h>


#define PREFIX_E "a5-kcm-alsa Error: "
#define PREFIX_N "a5-kcm-alsa Notice: "


#define ALSA_CHECK(a) \
do {                                                                  \
   int err = (a);                                                     \
   if (err < 0) {                                                     \
      TRACE(PREFIX_E "%s: %s\n", snd_strerror(err), #a);              \
      goto Error;                                                     \
   }                                                                  \
} while(0)


static snd_output_t *snd_output = NULL;
static char* default_device = "default";
static char* alsa_device = NULL;


typedef struct ALSA_VOICE {
   unsigned int frame_size; /* in bytes */
   unsigned int len; /* in frames */
   snd_pcm_uframes_t frag_len; /* in frames */
   bool reversed; /* true if playing reversed ATM. */

   volatile bool stop;
   volatile bool stopped;

   struct pollfd *ufds;
   int ufds_count;

   ALLEGRO_THREAD *poll_thread;
   volatile bool quit_poll_thread;

   snd_pcm_t *pcm_handle;
} ALSA_VOICE;



/* initialized output */
static int alsa_open(void)
{
   alsa_device = default_device;
   
   ALLEGRO_SYSTEM *sys = al_system_driver();
   if(sys->config) {
      const char* config_device;
      config_device = al_config_get_value(sys->config, "sound", "alsa_device");
      if(config_device)
         alsa_device = strdup(config_device);
   }
   
   ALSA_CHECK(snd_output_stdio_attach(&snd_output, stdout, 0));

   /* We need to check if alsa is available in this function. */
   snd_pcm_t *test_pcm_handle;
   int alsa_err = snd_pcm_open(&test_pcm_handle, alsa_device,
                               SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
   if (alsa_err < 0) {
      TRACE(PREFIX_N "ALSA is not available on the system.\n");
	  return 1;
   }
   else
      snd_pcm_close(test_pcm_handle);

   return 0;

   /* ALSA check is a macro that 'goto' error*/
   Error:
      TRACE(PREFIX_E "Error initializing alsa!\n");
      return 1;
}



/* The close method should close the device, freeing any resources, and allow
   other processes to use the device */
static void alsa_close(void)
{
   if(alsa_device != default_device)
      free(alsa_device);
   
   alsa_device = NULL;
   
   snd_config_update_free_global();
}


/* Underrun and suspend recovery */
static int xrun_recovery(snd_pcm_t *handle, int err)
{
   if (err == -EPIPE) { /* under-run */
      err = snd_pcm_prepare(handle);
      if (err < 0) {
         TRACE(PREFIX_E "Can't recover from underrun, prepare failed: %s\n", snd_strerror(err));
      }
      return 0;
   }
   else {
      ASSERT(0);
   }

   return err;
}


/*
 * Updates the supplied non-streaming voice.
 * buf   - Returns a pointer to the buffer containing sample data.
 * bytes - The requested size of the sample data buffer. Returns the actual
 *         size of returned the buffer.
 * Updates 'stop', 'pos' and 'reversed' fields of the supplied voice to the
 * future position.
 * If the voice is played backwards, 'buf' will point to the end of the buffer
 * and 'bytes' is the size that can be read towards the beginning.
 */
static int alsa_update_nonstream_voice(ALLEGRO_VOICE *voice, void **buf, int *bytes)
{
   ALSA_VOICE *alsa_voice = (ALSA_VOICE*)voice->extra;
   int bpos = voice->attached_stream->pos * alsa_voice->frame_size;
   int blen = alsa_voice->len * alsa_voice->frame_size;

   *buf = voice->attached_stream->spl_data.buffer.ptr + bpos;

   if (!alsa_voice->reversed) {
      if (bpos + *bytes > blen) {
         *bytes = blen - bpos;
         if (voice->attached_stream->loop == ALLEGRO_PLAYMODE_ONCE) {
            alsa_voice->stop = true;
            voice->attached_stream->pos = 0;
         }
         if (voice->attached_stream->loop == ALLEGRO_PLAYMODE_ONEDIR) {
            voice->attached_stream->pos = 0;
         }
         else if (voice->attached_stream->loop == ALLEGRO_PLAYMODE_BIDIR) {
            alsa_voice->reversed = true;
            voice->attached_stream->pos = alsa_voice->len;
         }
         return 1;
      }
      else
         voice->attached_stream->pos += *bytes / alsa_voice->frame_size;
   }
   else {
      if (bpos - *bytes < 0) {
         *bytes = bpos;
         /* loop will be ALLEGRO_PLAYMODE_BIDIR, other playing modes that play
            backwards are not currently supported by the API */
         /*if (voice->attached_stream->loop != ALLEGRO_PLAYMODE_BIDIR)
            alsa_voice->stop = true;*/

         voice->attached_stream->pos = 0;
         alsa_voice->reversed = false;
         return 1;
      }
      else
         voice->attached_stream->pos -= *bytes / alsa_voice->frame_size;
   }

   return 0;
}


/* Returns TRUE if the voice is ready for more data. */
static int alsa_voice_is_ready(ALSA_VOICE *alsa_voice)
{
   unsigned short revents;
   int err;

   poll(alsa_voice->ufds, alsa_voice->ufds_count, 0);
   snd_pcm_poll_descriptors_revents(alsa_voice->pcm_handle, alsa_voice->ufds,
                                    alsa_voice->ufds_count, &revents);

   if (revents & POLLERR) {
      if (snd_pcm_state(alsa_voice->pcm_handle) == SND_PCM_STATE_XRUN ||
          snd_pcm_state(alsa_voice->pcm_handle) == SND_PCM_STATE_SUSPENDED) {

         if (snd_pcm_state(alsa_voice->pcm_handle) == SND_PCM_STATE_XRUN)
            err = -EPIPE;
         else
            err = -ESTRPIPE;

         if (xrun_recovery(alsa_voice->pcm_handle, err) < 0) {
            TRACE(PREFIX_E "Write error: %s\n", snd_strerror(err));
            return -POLLERR;
         }
      } else {
         TRACE(PREFIX_E "Wait for poll failed\n");
         return -POLLERR;
      }
   }

   if (revents & POLLOUT)
      return TRUE;

   return FALSE;
}


/* Custom routine which runs in another thread and fills the hardware PCM buffer
   from the voice buffer. */
static void* alsa_update(ALLEGRO_THREAD *self, void *arg)
{
   ALLEGRO_VOICE *voice = (ALLEGRO_VOICE*)arg;
   ALSA_VOICE *alsa_voice = (ALSA_VOICE*)voice->extra;
   snd_pcm_uframes_t frames;
   const snd_pcm_channel_area_t *areas;
   snd_pcm_uframes_t offset;
   void *mmap;
   int ret;

   while (!alsa_voice->quit_poll_thread) {
      if (alsa_voice->stop && !alsa_voice->stopped) {
         snd_pcm_drop(alsa_voice->pcm_handle);
         alsa_voice->stopped = true;
      }

      if (!alsa_voice->stop && alsa_voice->stopped) {
         alsa_voice->stopped = false;
      }

      if (snd_pcm_state(alsa_voice->pcm_handle) == SND_PCM_STATE_PREPARED) {
         snd_pcm_start(alsa_voice->pcm_handle);
      }

      ret = alsa_voice_is_ready(alsa_voice);
      if (ret < 0)
         return NULL;
      if (ret == 0) {
         al_rest(0.005); /* some sensitive value */
         continue;
      }

      snd_pcm_avail_update(alsa_voice->pcm_handle);
      frames = alsa_voice->frag_len;
      ret = snd_pcm_mmap_begin(alsa_voice->pcm_handle, &areas, &offset, &frames);
      if (ret < 0) {
         if ((ret = xrun_recovery(alsa_voice->pcm_handle, ret)) < 0) {
            TRACE(PREFIX_E "MMAP begin avail error: %s\n", snd_strerror(ret));
         }
         return NULL;
      }

      ASSERT(frames);

      /* mmaped driver's memory. Interleaved channels format. */
      mmap = areas[0].addr + areas[0].first / 8 + offset * areas[0].step / 8;

      /* Read sample data into the buffer. */
      if (!voice->is_streaming && !alsa_voice->stopped) {
         void *buf;
         bool reverse = alsa_voice->reversed;
         int bytes = frames * alsa_voice->frame_size;

         alsa_update_nonstream_voice(voice, &buf, &bytes);
         frames = bytes / alsa_voice->frame_size;
         if (!reverse) {
            memcpy(mmap, buf, bytes);
         }
         else {
            /* Put a reversed copy in the driver's buffer. */
            unsigned int i;
            int fs = alsa_voice->frame_size;
            for (i = 1; i <= frames; i++)
               memcpy(mmap + i * fs, buf - i * fs, fs);
         }
      }
      else if (voice->is_streaming && !alsa_voice->stopped) {
         const void *data = _al_voice_update(voice, &frames);
         if (data == NULL)
            goto silence;
         memcpy(mmap, data, frames * alsa_voice->frame_size);
      }
      else {
         int silence;
silence:
         /* If stopped just fill with silence. */
         silence = _al_audio_get_silence(voice->depth);
         memset(mmap, silence, frames * alsa_voice->frame_size);
      }

      snd_pcm_sframes_t commitres = snd_pcm_mmap_commit(alsa_voice->pcm_handle, offset, frames);
      if (commitres < 0 || (snd_pcm_uframes_t)commitres != frames) {
         if ((ret = xrun_recovery(alsa_voice->pcm_handle, commitres >= 0 ? -EPIPE : commitres)) < 0) {
            TRACE(PREFIX_E "MMAP commit error: %s\n", snd_strerror(ret));
            return NULL;
         }
      }
   }

   return NULL;
}


/* The load_voice method loads a sample into the driver's memory. The voice's
   'streaming' field will be set to false for these voices, and it's
   'buffer_size' field will be the total length in bytes of the sample data.
   The voice's attached stream's looping mode should be honored, and loading
   must fail if it cannot be. */
static int alsa_load_voice(ALLEGRO_VOICE *voice, const void *data)
{
   ALSA_VOICE *ex_data = voice->extra;

   voice->attached_stream->pos = 0;
   ex_data->len = voice->attached_stream->spl_data.len >> MIXER_FRAC_SHIFT;

   return 0;
   (void)data;
}



/* The unload_voice method unloads a sample previously loaded with load_voice.
   This method should not be called on a streaming voice. */
static void alsa_unload_voice(ALLEGRO_VOICE *voice)
{
}



/* The start_voice should, surprise, start the voice. For streaming voices, it
   should start polling the device and call _al_voice_update for audio data.
   For non-streaming voices, it should resume playing from the last set
   position */
static int alsa_start_voice(ALLEGRO_VOICE *voice)
{
   ALSA_VOICE *ex_data = voice->extra;
   ex_data->stop = false;
   return 0;
}



/* The stop_voice method should stop playback. For non-streaming voices, it
   should leave the data loaded, and reset the voice position to 0. */
static int alsa_stop_voice(ALLEGRO_VOICE *voice)
{
   ALSA_VOICE *ex_data = voice->extra;

   ex_data->stop = true;
   if(!voice->is_streaming) {
      voice->attached_stream->pos = 0;
   }

   while (!ex_data->stopped)
      al_rest(0.001);

   return 0;
}



/* The voice_is_playing method should only be called on non-streaming sources,
   and should return true if the voice is playing */
static bool alsa_voice_is_playing(const ALLEGRO_VOICE *voice)
{
   ALSA_VOICE *ex_data = voice->extra;
   return !ex_data->stopped;
}


/* The allocate_voice method should grab a voice from the system, and allocate
   any data common to streaming and non-streaming sources. */
static int alsa_allocate_voice(ALLEGRO_VOICE *voice)
{
   snd_pcm_format_t format;
   int chan_count;
   unsigned int req_freq;

   ALSA_VOICE *ex_data = calloc(1, sizeof(ALSA_VOICE));
   if (!ex_data)
      return 1;

   chan_count = al_channel_count(voice->chan_conf);
   ex_data->frame_size = chan_count * al_depth_size(voice->depth);
   if (!ex_data->frame_size)
      goto Error;

   ex_data->stop = true;
   ex_data->stopped = true;
   ex_data->reversed = false;
   /* ALSA usually overrides this, but set it just in case. */
   ex_data->frag_len = 256;

   if (voice->depth == ALLEGRO_AUDIO_DEPTH_INT8)
      format = SND_PCM_FORMAT_S8;
   else if (voice->depth == ALLEGRO_AUDIO_DEPTH_UINT8)
      format = SND_PCM_FORMAT_U8;
   else if (voice->depth == ALLEGRO_AUDIO_DEPTH_INT16)
      format = SND_PCM_FORMAT_S16;
   else if (voice->depth == ALLEGRO_AUDIO_DEPTH_UINT16)
      format = SND_PCM_FORMAT_U16;
   else if (voice->depth == ALLEGRO_AUDIO_DEPTH_INT24)
      format = SND_PCM_FORMAT_S24;
   else if (voice->depth == ALLEGRO_AUDIO_DEPTH_UINT24)
      format = SND_PCM_FORMAT_U24;
   else if (voice->depth == ALLEGRO_AUDIO_DEPTH_FLOAT32)
      format = SND_PCM_FORMAT_FLOAT;
   else
      goto Error;

   /* Why is this? And what is this? */
   if (voice->chan_conf == ALLEGRO_CHANNEL_CONF_3)
      goto Error;

   req_freq = voice->frequency;

   ALSA_CHECK(snd_pcm_open(&ex_data->pcm_handle, alsa_device, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK));

   snd_pcm_hw_params_t *hwparams;
   snd_pcm_hw_params_alloca(&hwparams);

   ALSA_CHECK(snd_pcm_hw_params_any(ex_data->pcm_handle, hwparams));
   ALSA_CHECK(snd_pcm_hw_params_set_access(ex_data->pcm_handle, hwparams, SND_PCM_ACCESS_MMAP_INTERLEAVED));
   ALSA_CHECK(snd_pcm_hw_params_set_format(ex_data->pcm_handle, hwparams, format));
   ALSA_CHECK(snd_pcm_hw_params_set_channels(ex_data->pcm_handle, hwparams, chan_count));
   ALSA_CHECK(snd_pcm_hw_params_set_rate_near(ex_data->pcm_handle, hwparams, &req_freq, NULL));
   ALSA_CHECK(snd_pcm_hw_params_set_period_size_near(ex_data->pcm_handle, hwparams, &ex_data->frag_len, NULL));
   ALSA_CHECK(snd_pcm_hw_params(ex_data->pcm_handle, hwparams));

   if (voice->frequency != req_freq) {
      TRACE(PREFIX_E "Unsupported rate! Requested %lu, got %iu.\n", voice->frequency, req_freq);
      goto Error;
   }

   snd_pcm_sw_params_t *swparams;
   snd_pcm_sw_params_alloca(&swparams);

   ALSA_CHECK(snd_pcm_sw_params_current(ex_data->pcm_handle, swparams));
   ALSA_CHECK(snd_pcm_sw_params_set_start_threshold(ex_data->pcm_handle, swparams, ex_data->frag_len));
   ALSA_CHECK(snd_pcm_sw_params_set_avail_min(ex_data->pcm_handle, swparams, ex_data->frag_len));
   ALSA_CHECK(snd_pcm_sw_params_set_xfer_align(ex_data->pcm_handle, swparams, 1));
   ALSA_CHECK(snd_pcm_sw_params(ex_data->pcm_handle, swparams));

   ex_data->ufds_count = snd_pcm_poll_descriptors_count(ex_data->pcm_handle);
   ex_data->ufds = malloc(sizeof(struct pollfd) * ex_data->ufds_count);
   ALSA_CHECK(snd_pcm_poll_descriptors(ex_data->pcm_handle, ex_data->ufds, ex_data->ufds_count));

   voice->extra = ex_data;
   ex_data->quit_poll_thread = false;
   ex_data->poll_thread = al_create_thread(alsa_update, (void*)voice);
   al_start_thread(ex_data->poll_thread);

   return 0;

Error:
   if (ex_data->pcm_handle)
      snd_pcm_close(ex_data->pcm_handle);
   free(ex_data);
   voice->extra = NULL;
   return 1;
}



/* The deallocate_voice method should free the resources for the given voice,
   but still retain a hold on the device. The voice should be stopped and
   unloaded by the time this is called */
static void alsa_deallocate_voice(ALLEGRO_VOICE *voice)
{
   ALSA_VOICE *alsa_voice = (ALSA_VOICE*)voice->extra;

   alsa_voice->quit_poll_thread = true;
   al_join_thread(alsa_voice->poll_thread, NULL);

   snd_pcm_drop(alsa_voice->pcm_handle);
   snd_pcm_close(alsa_voice->pcm_handle);

   al_destroy_thread(alsa_voice->poll_thread);
   free(alsa_voice->ufds);
   free(voice->extra);
   voice->extra = NULL;
}



/* The get_voice_position method should return the current sample position of
   the voice (sample_pos = byte_pos / (depth/8) / channels). This should never
   be called on a streaming voice. */
static unsigned long alsa_get_voice_position(const ALLEGRO_VOICE *voice)
{
   return voice->attached_stream->pos;
}



/* The set_voice_position method should set the voice's playback position,
   given the value in samples. This should never be called on a streaming
   voice. */
static int alsa_set_voice_position(ALLEGRO_VOICE *voice, unsigned long val)
{
   voice->attached_stream->pos = val;
   return 0;
}


ALLEGRO_AUDIO_DRIVER _alsa_driver =
{
   "ALSA",

   alsa_open,
   alsa_close,

   alsa_allocate_voice,
   alsa_deallocate_voice,

   alsa_load_voice,
   alsa_unload_voice,

   alsa_start_voice,
   alsa_stop_voice,

   alsa_voice_is_playing,

   alsa_get_voice_position,
   alsa_set_voice_position
};

/* vim: set sts=3 sw=3 et: */
