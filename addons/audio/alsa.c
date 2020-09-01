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
#include "allegro5/internal/aintern_audio.h"

#include <alloca.h>
#include <alsa/asoundlib.h>

ALLEGRO_DEBUG_CHANNEL("alsa")

#define ALSA_CHECK(a) \
do {                                                                  \
   int err = (a);                                                     \
   if (err < 0) {                                                     \
      ALLEGRO_ERROR("%s: %s\n", snd_strerror(err), #a);               \
      goto Error;                                                     \
   }                                                                  \
} while(0)


static snd_output_t *snd_output = NULL;
static char *default_device = "default";
static char *alsa_device = NULL;
static _AL_LIST* device_list;

// TODO: Setting this to 256 causes (extreme, about than 10 seconds)
// lag if the alsa device is really pulseaudio.
//
// pw: But there are calls later which expect this variable to be set an on
// my machine (without PulseAudio) the driver doesn't work properly with
// anything lower than 32.
#define DEFAULT_PERIOD_SIZE   32
#define MIN_PERIOD_SIZE       1

// This value works well on my RPI3 and Linux machine.
#define DEFAULT_BUFFER_SIZE 2048

static unsigned int get_period_size(void)
{
   const char *val = al_get_config_value(al_get_system_config(),
      "alsa", "buffer_size");
   if (val && val[0] != '\0') {
      int n = atoi(val);
      if (n < MIN_PERIOD_SIZE)
         n = MIN_PERIOD_SIZE;
      return n;
   }

   return DEFAULT_PERIOD_SIZE;
}

static snd_pcm_uframes_t get_buffer_size(void)
{
   const char *val = al_get_config_value(al_get_system_config(),
      "alsa", "buffer_size2");
   if (val && val[0] != '\0') {
      int n = atoi(val);
      if (n < 1)
         n = 1;
      return n;
   }

   return DEFAULT_BUFFER_SIZE;
}

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

   snd_pcm_t *pcm_handle;
   bool mmapped;
} ALSA_VOICE;



/* initialized output */
static int alsa_open(void)
{
   alsa_device = default_device;

   const char *config_device;
   config_device = al_get_config_value(al_get_system_config(), "alsa", "device");
   if (config_device && config_device[0] != '\0')
      alsa_device = strdup(config_device);

   ALSA_CHECK(snd_output_stdio_attach(&snd_output, stdout, 0));

   /* We need to check if alsa is available in this function. */
   snd_pcm_t *test_pcm_handle;
   int alsa_err = snd_pcm_open(&test_pcm_handle, alsa_device,
                               SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
   if (alsa_err < 0) {
      ALLEGRO_WARN("ALSA is not available on the system.\n");
      return 1;
   }
   else {
      snd_pcm_close(test_pcm_handle);
   }

   return 0;

   /* ALSA check is a macro that 'goto' error*/
Error:
   ALLEGRO_ERROR("Error initializing alsa!\n");
   return 1;
}



/* The close method should close the device, freeing any resources, and allow
   other processes to use the device */
static void alsa_close(void)
{
   _al_list_destroy(device_list);

   if (alsa_device != default_device)
      al_free(alsa_device);
   
   alsa_device = NULL;
   
   snd_output_close(snd_output);
   snd_config_update_free_global();
}


/* Underrun and suspend recovery */
static int xrun_recovery(snd_pcm_t *handle, int err)
{
   if (err == -EPIPE) { /* under-run */
      err = snd_pcm_prepare(handle);
      if (err < 0) {
         ALLEGRO_ERROR("Can't recover from underrun, prepare failed: %s\n", snd_strerror(err));
      }
      else {
         ALLEGRO_DEBUG("Recovered from underrun\n");
      }
      return 0;
   }
   else if (err == -ESTRPIPE) { /* suspend */
      err = snd_pcm_resume(handle);
      if (err < 0) {
         ALLEGRO_ERROR("Can't recover from suspend, resume failed: %s\n", snd_strerror(err));
      }
      else {
         ALLEGRO_DEBUG("Resumed successfully\n");
      }
      return 0;
   }
   else {
      ALLEGRO_ERROR("Unknown error code: %d\n", err);
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

   *buf = (char *)voice->attached_stream->spl_data.buffer.ptr + bpos;

   if (!alsa_voice->reversed) {
      if (bpos + *bytes > blen) {
         *bytes = blen - bpos;
         if (voice->attached_stream->loop == ALLEGRO_PLAYMODE_ONCE) {
            alsa_voice->stop = true;
            voice->attached_stream->pos = 0;
         }
         if (voice->attached_stream->loop == ALLEGRO_PLAYMODE_LOOP) {
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


/* Returns true if the voice is ready for more data. */
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
            ALLEGRO_ERROR("Write error: %s\n", snd_strerror(err));
            return -POLLERR;
         }
      } else {
         ALLEGRO_ERROR("Wait for poll failed\n");
         return -POLLERR;
      }
   }

   if (revents & POLLOUT)
      return true;

   return false;
}


/* Custom routine which runs in another thread and fills the hardware PCM buffer
   from the voice buffer. */
static void *alsa_update_mmap(ALLEGRO_THREAD *self, void *arg)
{
   ALLEGRO_VOICE *voice = (ALLEGRO_VOICE*)arg;
   ALSA_VOICE *alsa_voice = (ALSA_VOICE*)voice->extra;
   snd_pcm_state_t last_state = -1;
   snd_pcm_state_t state;
   snd_pcm_uframes_t frames;
   const snd_pcm_channel_area_t *areas;
   snd_pcm_uframes_t offset;
   char *mmap;
   snd_pcm_sframes_t commitres;
   int ret;

   ALLEGRO_INFO("ALSA update_mmap thread started\n");

   while (!al_get_thread_should_stop(self)) {
      if (alsa_voice->stop && !alsa_voice->stopped) {
         snd_pcm_drop(alsa_voice->pcm_handle);
         al_lock_mutex(voice->mutex);
         alsa_voice->stopped = true;
         al_signal_cond(voice->cond);
         al_unlock_mutex(voice->mutex);
      }

      if (!alsa_voice->stop && alsa_voice->stopped) {
         alsa_voice->stopped = false;
      }

      if (alsa_voice->stopped) {
         /* Keep waiting while the voice is supposed to be stopped but present.
          */
         al_lock_mutex(voice->mutex);
         while (alsa_voice->stop && !al_get_thread_should_stop(self)) {
            al_wait_cond(voice->cond, voice->mutex);
         }
         al_unlock_mutex(voice->mutex);
         continue;
      }

      state = snd_pcm_state(alsa_voice->pcm_handle);
      if (state != last_state) {
         ALLEGRO_DEBUG("state changed to: %s\n", snd_pcm_state_name(state));
         last_state = state;
      }
      if (state == SND_PCM_STATE_SETUP) {
         int rc = snd_pcm_prepare(alsa_voice->pcm_handle);
         ALLEGRO_DEBUG("snd_pcm_prepare returned: %d\n", rc);
         continue;
      }
      if (state == SND_PCM_STATE_PREPARED) {
         int rc = snd_pcm_start(alsa_voice->pcm_handle);
         ALLEGRO_DEBUG("snd_pcm_start returned: %d\n", rc);
      }

      ret = alsa_voice_is_ready(alsa_voice);
      if (ret < 0)
         break;
      if (ret == 0) {
         al_rest(0.005); /* TODO: Why not use an event or condition variable? */
         continue;
      }

      snd_pcm_avail_update(alsa_voice->pcm_handle);
      frames = alsa_voice->frag_len;
      ret = snd_pcm_mmap_begin(alsa_voice->pcm_handle, &areas, &offset, &frames);
      if (ret < 0) {
         if ((ret = xrun_recovery(alsa_voice->pcm_handle, ret)) < 0) {
            ALLEGRO_ERROR("MMAP begin avail error: %s\n", snd_strerror(ret));
         }
         break;
      }

      if (frames == 0) {
         al_rest(0.005); /* TODO: Why not use an event or condition variable? */
         goto commit;
      }

      /* mmaped driver's memory. Interleaved channels format. */
      mmap = (char *) areas[0].addr
            + areas[0].first / 8
            + offset * areas[0].step / 8;

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
               memcpy(mmap + i * fs, (char *) buf - i * fs, fs);
         }
      }
      else if (voice->is_streaming && !alsa_voice->stopped) {
         /* This should fit. */
         unsigned int iframes = frames;
         const void *data = _al_voice_update(voice, voice->mutex, &iframes);
         frames = iframes;
         if (data == NULL)
            goto silence;
         memcpy(mmap, data, frames * alsa_voice->frame_size);
      }
      else {
silence:
         /* If stopped just fill with silence. */
         al_fill_silence(mmap, frames, voice->depth, voice->chan_conf);
      }

commit:
      commitres = snd_pcm_mmap_commit(alsa_voice->pcm_handle, offset, frames);
      if (commitres < 0 || (snd_pcm_uframes_t)commitres != frames) {
         if ((ret = xrun_recovery(alsa_voice->pcm_handle, commitres >= 0 ? -EPIPE : commitres)) < 0) {
            ALLEGRO_ERROR("MMAP commit error: %s\n", snd_strerror(ret));
            break;
         }
      }
   }

   ALLEGRO_INFO("ALSA update_mmap thread stopped\n");

   return NULL;
}


static void *alsa_update_rw(ALLEGRO_THREAD *self, void *arg)
{
   ALLEGRO_VOICE *voice = (ALLEGRO_VOICE*)arg;
   ALSA_VOICE *alsa_voice = (ALSA_VOICE*)voice->extra;
   snd_pcm_state_t last_state = -1;
   snd_pcm_state_t state;
   snd_pcm_uframes_t frames;
   snd_pcm_sframes_t err;

   ALLEGRO_INFO("ALSA update_rw thread started\n");

   while (!al_get_thread_should_stop(self)) {
      if (alsa_voice->stop && !alsa_voice->stopped) {
         snd_pcm_drop(alsa_voice->pcm_handle);
         al_lock_mutex(voice->mutex);
         alsa_voice->stopped = true;
         al_signal_cond(voice->cond);
         al_unlock_mutex(voice->mutex);
      }

      if (!alsa_voice->stop && alsa_voice->stopped) {
         alsa_voice->stopped = false;
      }

      if (alsa_voice->stopped) {
         /* Keep waiting while the voice is supposed to be stopped but present.
          */
         al_lock_mutex(voice->mutex);
         while (alsa_voice->stop && !al_get_thread_should_stop(self)) {
            al_wait_cond(voice->cond, voice->mutex);
         }
         al_unlock_mutex(voice->mutex);
         continue;
      }

      state = snd_pcm_state(alsa_voice->pcm_handle);
      if (state != last_state) {
         ALLEGRO_DEBUG("state changed to: %s\n", snd_pcm_state_name(state));
         last_state = state;
      }
      if (state == SND_PCM_STATE_SETUP) {
         int rc = snd_pcm_prepare(alsa_voice->pcm_handle);
         ALLEGRO_DEBUG("snd_pcm_prepare returned: %d\n", rc);
         continue;
      }
      if (state == SND_PCM_STATE_PREPARED) {
         int rc = snd_pcm_start(alsa_voice->pcm_handle);
         ALLEGRO_DEBUG("snd_pcm_start returned: %d\n", rc);
      }

      snd_pcm_wait(alsa_voice->pcm_handle, 10);
      err = snd_pcm_avail_update(alsa_voice->pcm_handle);
      if (err < 0) {
         if (err == -EPIPE) {
            snd_pcm_prepare(alsa_voice->pcm_handle);
         }
         else {
            ALLEGRO_WARN("Alsa r/w thread exited "
               "with error code %s.\n", snd_strerror(-err));
            break;
         }
      }
      if (err == 0) {
         continue;
      }
      frames = err;
      if (frames > alsa_voice->frag_len)
         frames = alsa_voice->frag_len;
      /* Write sample data into the buffer. */
      int bytes = frames * alsa_voice->frame_size;
      uint8_t data[bytes];
      void *buf;
      if (!voice->is_streaming && !alsa_voice->stopped) {
         ASSERT(!alsa_voice->reversed); // FIXME
         alsa_update_nonstream_voice(voice, &buf, &bytes);
         frames = bytes / alsa_voice->frame_size;
      }
      else if (voice->is_streaming && !alsa_voice->stopped) {
         /* This should fit. */
         unsigned int iframes = frames;
         buf = (void *)_al_voice_update(voice, voice->mutex, &iframes);
         if (buf == NULL)
            goto silence;
         frames = iframes;
      }
      else {
silence:
         /* If stopped just fill with silence. */
         al_fill_silence(data, frames, voice->depth, voice->chan_conf);
         buf = data;
      }
      err = snd_pcm_writei(alsa_voice->pcm_handle, buf, frames);
      if (err < 0) {
         if (err == -EPIPE) {
            snd_pcm_prepare(alsa_voice->pcm_handle);
         }
      }
   }

   ALLEGRO_INFO("ALSA update_rw thread stopped\n");

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
   ex_data->len = voice->attached_stream->spl_data.len;

   return 0;
   (void)data;
}



/* The unload_voice method unloads a sample previously loaded with load_voice.
   This method should not be called on a streaming voice. */
static void alsa_unload_voice(ALLEGRO_VOICE *voice)
{
   (void)voice;
}



/* The start_voice should, surprise, start the voice. For streaming voices, it
   should start polling the device and call _al_voice_update for audio data.
   For non-streaming voices, it should resume playing from the last set
   position */
static int alsa_start_voice(ALLEGRO_VOICE *voice)
{
   ALSA_VOICE *ex_data = voice->extra;

   /* We already hold voice->mutex. */
   ex_data->stop = false;
   al_signal_cond(voice->cond);

   return 0;
}



/* The stop_voice method should stop playback. For non-streaming voices, it
   should leave the data loaded, and reset the voice position to 0. */
static int alsa_stop_voice(ALLEGRO_VOICE *voice)
{
   ALSA_VOICE *ex_data = voice->extra;

   /* We already hold voice->mutex. */
   ex_data->stop = true;
   al_signal_cond(voice->cond);

   if (!voice->is_streaming) {
      voice->attached_stream->pos = 0;
   }

   while (!ex_data->stopped) {
      al_wait_cond(voice->cond, voice->mutex);
   }

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

   ALSA_VOICE *ex_data = al_calloc(1, sizeof(ALSA_VOICE));
   if (!ex_data)
      return 1;

   chan_count = al_get_channel_count(voice->chan_conf);
   ex_data->frame_size = chan_count * al_get_audio_depth_size(voice->depth);
   if (!ex_data->frame_size)
      goto Error;

   ex_data->stop = true;
   ex_data->stopped = true;
   ex_data->reversed = false;

   ex_data->frag_len = get_period_size();

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
   if (snd_pcm_hw_params_set_access(ex_data->pcm_handle, hwparams, SND_PCM_ACCESS_MMAP_INTERLEAVED) == 0) {
      ex_data->mmapped = true;
   }
   else {
      ALSA_CHECK(snd_pcm_hw_params_set_access(ex_data->pcm_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED));
      ex_data->mmapped = false;
   }
   ALSA_CHECK(snd_pcm_hw_params_set_format(ex_data->pcm_handle, hwparams, format));
   ALSA_CHECK(snd_pcm_hw_params_set_channels(ex_data->pcm_handle, hwparams, chan_count));
   ALSA_CHECK(snd_pcm_hw_params_set_rate_near(ex_data->pcm_handle, hwparams, &req_freq, NULL));
   ALSA_CHECK(snd_pcm_hw_params_set_period_size_near(ex_data->pcm_handle, hwparams, &ex_data->frag_len, NULL));
   snd_pcm_uframes_t buffer_size = get_buffer_size();
   ALSA_CHECK(snd_pcm_hw_params_set_buffer_size_near(ex_data->pcm_handle, hwparams, &buffer_size));
   ALSA_CHECK(snd_pcm_hw_params(ex_data->pcm_handle, hwparams));

   if (voice->frequency != req_freq) {
      ALLEGRO_ERROR("Unsupported rate! Requested %u, got %iu.\n", voice->frequency, req_freq);
      goto Error;
   }

   snd_pcm_sw_params_t *swparams;
   snd_pcm_sw_params_alloca(&swparams);

   ALSA_CHECK(snd_pcm_sw_params_current(ex_data->pcm_handle, swparams));
   ALSA_CHECK(snd_pcm_sw_params_set_start_threshold(ex_data->pcm_handle, swparams, ex_data->frag_len));
   ALSA_CHECK(snd_pcm_sw_params_set_avail_min(ex_data->pcm_handle, swparams, ex_data->frag_len));
   ALSA_CHECK(snd_pcm_sw_params(ex_data->pcm_handle, swparams));

   ex_data->ufds_count = snd_pcm_poll_descriptors_count(ex_data->pcm_handle);
   ex_data->ufds = al_malloc(sizeof(struct pollfd) * ex_data->ufds_count);
   ALSA_CHECK(snd_pcm_poll_descriptors(ex_data->pcm_handle, ex_data->ufds, ex_data->ufds_count));

   voice->extra = ex_data;

   if (ex_data->mmapped) {
      ex_data->poll_thread = al_create_thread(alsa_update_mmap, (void*)voice);
   }
   else {
      ALLEGRO_WARN("Falling back to non-mmapped transfer.\n");
      snd_pcm_nonblock(ex_data->pcm_handle, 0);
      ex_data->poll_thread = al_create_thread(alsa_update_rw, (void*)voice);
   }
   al_start_thread(ex_data->poll_thread);

   return 0;

Error:
   if (ex_data->pcm_handle)
      snd_pcm_close(ex_data->pcm_handle);
   al_free(ex_data);
   voice->extra = NULL;
   return 1;
}



/* The deallocate_voice method should free the resources for the given voice,
   but still retain a hold on the device. The voice should be stopped and
   unloaded by the time this is called */
static void alsa_deallocate_voice(ALLEGRO_VOICE *voice)
{
   ALSA_VOICE *alsa_voice = (ALSA_VOICE*)voice->extra;

   al_lock_mutex(voice->mutex);
   al_set_thread_should_stop(alsa_voice->poll_thread);
   al_broadcast_cond(voice->cond);
   al_unlock_mutex(voice->mutex);
   al_join_thread(alsa_voice->poll_thread, NULL);

   snd_pcm_drop(alsa_voice->pcm_handle);
   snd_pcm_close(alsa_voice->pcm_handle);

   al_destroy_thread(alsa_voice->poll_thread);
   al_free(alsa_voice->ufds);
   al_free(voice->extra);
   voice->extra = NULL;
}



/* The get_voice_position method should return the current sample position of
   the voice (sample_pos = byte_pos / (depth/8) / channels). This should never
   be called on a streaming voice. */
static unsigned int alsa_get_voice_position(const ALLEGRO_VOICE *voice)
{
   return voice->attached_stream->pos;
}



/* The set_voice_position method should set the voice's playback position,
   given the value in samples. This should never be called on a streaming
   voice. */
static int alsa_set_voice_position(ALLEGRO_VOICE *voice, unsigned int val)
{
   voice->attached_stream->pos = val;
   return 0;
}

typedef struct ALSA_RECORDER_DATA
{
   snd_pcm_t *capture_handle;
   snd_pcm_hw_params_t *hw_params;
} ALSA_RECORDER_DATA;

static void *alsa_update_recorder(ALLEGRO_THREAD *t, void *thread_data)
{
   ALLEGRO_AUDIO_RECORDER *r = thread_data;
   ALSA_RECORDER_DATA *alsa = r->extra;
   ALLEGRO_EVENT user_event;
   uint8_t *null_buffer;
   unsigned int fragment_i = 0;
   
   null_buffer = al_malloc(1024 * r->sample_size);
   if (!null_buffer) {
      ALLEGRO_ERROR("Unable to create buffer for draining ALSA.\n");
      return NULL;
   }
   
   while (!al_get_thread_should_stop(t))
   {
      al_lock_mutex(r->mutex);
      if (!r->is_recording) {
         /* Even if not recording, we still want to read from ALSA.
            Otherwise it will buffer everything and spit it all out whenever
            the recording resumes. (XXX: Just copied this from PulseAudio.
            This current implementation is just a bare minimum effort...)
          */
         al_unlock_mutex(r->mutex);
         snd_pcm_readi(alsa->capture_handle, null_buffer, 1024);
      }
      else {
         ALLEGRO_AUDIO_RECORDER_EVENT *e;
         snd_pcm_sframes_t count;
         al_unlock_mutex(r->mutex);
         if ((count = snd_pcm_readi(alsa->capture_handle, r->fragments[fragment_i], r->samples)) > 0) {
            user_event.user.type = ALLEGRO_EVENT_AUDIO_RECORDER_FRAGMENT;
            e = al_get_audio_recorder_event(&user_event);
            e->buffer = r->fragments[fragment_i];
            e->samples = count;
            al_emit_user_event(&r->source, &user_event, NULL);
         
            if (++fragment_i == r->fragment_count) {
               fragment_i = 0;
            }
         }
      }
   }

   al_free(null_buffer);
   return NULL;
}

static int alsa_allocate_recorder(ALLEGRO_AUDIO_RECORDER *r)
{
   ALSA_RECORDER_DATA *data;
   unsigned int frequency = r->frequency;
   snd_pcm_format_t format;
   const char *device = default_device;
   const char *config_device;
   config_device =
      al_get_config_value(al_get_system_config(), "alsa", "capture_device");
   if (config_device && config_device[0] != '\0')
      device = config_device;

   data = al_calloc(1, sizeof(*data));
   
   if (!data) {
      goto Error;
   }
   
   if (r->depth == ALLEGRO_AUDIO_DEPTH_INT8)
      format = SND_PCM_FORMAT_S8;
   else if (r->depth == ALLEGRO_AUDIO_DEPTH_UINT8)
      format = SND_PCM_FORMAT_U8;
   else if (r->depth == ALLEGRO_AUDIO_DEPTH_INT16)
      format = SND_PCM_FORMAT_S16;
   else if (r->depth == ALLEGRO_AUDIO_DEPTH_UINT16)
      format = SND_PCM_FORMAT_U16;
   else if (r->depth == ALLEGRO_AUDIO_DEPTH_INT24)
      format = SND_PCM_FORMAT_S24;
   else if (r->depth == ALLEGRO_AUDIO_DEPTH_UINT24)
      format = SND_PCM_FORMAT_U24;
   else if (r->depth == ALLEGRO_AUDIO_DEPTH_FLOAT32)
      format = SND_PCM_FORMAT_FLOAT;
   else
      goto Error;
   
   ALSA_CHECK(snd_pcm_open(&data->capture_handle, device, SND_PCM_STREAM_CAPTURE, 0));
   ALSA_CHECK(snd_pcm_hw_params_malloc(&data->hw_params));
   ALSA_CHECK(snd_pcm_hw_params_any(data->capture_handle, data->hw_params));
   ALSA_CHECK(snd_pcm_hw_params_set_access(data->capture_handle, data->hw_params, SND_PCM_ACCESS_RW_INTERLEAVED));
   ALSA_CHECK(snd_pcm_hw_params_set_format(data->capture_handle, data->hw_params, format));
   ALSA_CHECK(snd_pcm_hw_params_set_rate_near(data->capture_handle, data->hw_params, &frequency, 0));
   
   if (frequency != r->frequency) {
      ALLEGRO_ERROR("Unsupported rate! Requested %u, got %iu.\n", r->frequency, frequency);
      goto Error;
   }
   
   ALSA_CHECK(snd_pcm_hw_params_set_channels(data->capture_handle, data->hw_params, al_get_channel_count(r->chan_conf)));
   ALSA_CHECK(snd_pcm_hw_params(data->capture_handle, data->hw_params));
   
   ALSA_CHECK(snd_pcm_prepare(data->capture_handle));
   
   r->extra = data;
   r->thread = al_create_thread(alsa_update_recorder, r);
   
   return 0;
   
Error:
   
   if (data) {
      if (data->hw_params) {
         snd_pcm_hw_params_free(data->hw_params);
      }
      if (data->capture_handle) {
         snd_pcm_close(data->capture_handle);
      }
      al_free(data);
   }
   
   return 1;
}

static void alsa_deallocate_recorder(ALLEGRO_AUDIO_RECORDER *r)
{
   ALSA_RECORDER_DATA *data = r->extra;
   
   snd_pcm_hw_params_free(data->hw_params);
   snd_pcm_close(data->capture_handle);
}

void _device_list_dtor(void* value, void* userdata)
{
   ALLEGRO_AUDIO_DEVICE* device = (ALLEGRO_AUDIO_DEVICE*)value;
   al_free(device->name);
   al_free(device->identifier);
}

_AL_LIST* alsa_get_devices()
{
   if (!device_list) {
      device_list = _al_list_create();

      int rcard = -1;
      while(snd_card_next(&rcard) == 0) {
         if (rcard < 0) return device_list;

         snd_ctl_t *handle;
         char str[64];

         sprintf(str, "hw:%i", rcard);
         if (snd_ctl_open(&handle, str, 0) < 0) {
            return device_list;
         }

         int dev_num = -1;

         while(1) {
            if (snd_ctl_pcm_next_device(handle, &dev_num) < 0) {
               break;
            }

            if (dev_num < 0) break;

            snd_ctl_card_info_t *card_info;
            snd_ctl_card_info_alloca(&card_info);

            if (snd_ctl_card_info(handle, card_info) < 0) {
               break;
            }
            else {
               char *name = snd_ctl_card_info_get_name(card_info);
               char *identifier = snd_ctl_card_info_get_id(card_info);
               int len = strlen(name) + 1;
               int identifier_len = strlen(identifier) + 1;

               ALLEGRO_AUDIO_DEVICE* device = (ALLEGRO_AUDIO_DEVICE*)al_malloc(sizeof(ALLEGRO_AUDIO_DEVICE));
               device->identifier = (char*)al_malloc(identifier_len);
               device->name = (char*)al_malloc(len);

               memset(device->name, 0, len);
               strcpy(device->name, name);

               memset(device->identifier, 0, identifier_len);
               strcpy(device->identifier, identifier);

               // identifier or maybe even name should be subdevice gist.github.com/dontknowmyname/4536543
               _al_list_push_back_ex(device_list, device, _device_list_dtor);
            }
         }

         snd_ctl_close(handle);
      }
   }

   return device_list;
}

ALLEGRO_AUDIO_DRIVER _al_kcm_alsa_driver =
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
   alsa_set_voice_position,

   alsa_allocate_recorder,
   alsa_deallocate_recorder,

   alsa_get_devices
};

/* vim: set sts=3 sw=3 et: */
