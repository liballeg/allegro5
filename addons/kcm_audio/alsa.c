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
#include "allegro5/internal/aintern_thread.h"
#include "allegro5/internal/aintern_kcm_audio.h"

#include <alsa/asoundlib.h>

/* FIXME: alsa segfaults for unhandled formats (channel and depth etc) */


#define TRACE_E "a5-kcm-audio Error: "
#define TRACE_N "a5-kcm-audio Notice: "


#define ALSA_CHECK(a) \
do {                                                                  \
   int err = (a);                                                     \
   if (err < 0) {                                                     \
      TRACE(TRACE_E "%s: %s\n", snd_strerror(err), #a);               \
      goto Error;                                                     \
   }                                                                  \
} while(0)

#define ALLEGRO_ALSA_MIXER_DEVICE ALLEGRO_AUDIO_USER_START + 1

static unsigned int alsa_frag_count= 5;
static snd_pcm_uframes_t alsa_frag_size = 256;
static snd_output_t *snd_output = NULL;
const char alsa_device[128] = "default";
 
typedef struct SAMPLE_CACHE {
   char *buffer;
   size_t buf_len;
   size_t frame_size;

   bool in_use;
} SAMPLE_CACHE;

/* TODO: make dynamic */
#define MAX_SCACHE_SIZE 32
static SAMPLE_CACHE scache[MAX_SCACHE_SIZE];

int _al_cache_sample(ALLEGRO_SAMPLE *sample)
{
   size_t i = 0;

   do {
      if (!scache[i].in_use)
         break;

      if (++i == MAX_SCACHE_SIZE)
         return -1;
   } while (1);

   scache[i].frame_size = al_channel_count(sample->chan_conf) * al_depth_size(sample->depth);
   if (!scache[i].frame_size)
      return -1;

   scache[i].buf_len = (sample->len>>MIXER_FRAC_SHIFT) * scache[i].frame_size;
   scache[i].buffer = malloc(scache[i].buf_len * 2);
   if (!scache[i].buffer)
      return -1;

   /* Put a normal copy in the first half */
   memcpy(scache[i].buffer, sample->buffer.ptr, scache[i].buf_len); 

   /* Put a reversed copy in the second half */
   char *buf = scache[i].buffer + scache[i].buf_len;
   const unsigned int w = scache[i].frame_size;
   unsigned int x;

   for (x = 0; x < scache[i].buf_len / w; ++x)
      memcpy(&buf[x*w], &scache[i].buffer[scache[i].buf_len - (x+1)*w], w);

   scache[i].in_use = true;
   return i;
}


void _al_decache_sample(int i)
{
   free(scache[i].buffer);
   scache[i].buffer = NULL;
   scache[i].in_use = false;
}


void _al_read_sample_cache(int i, unsigned int pos, bool reverse, void **buf, unsigned int *len)
{
   if (pos >= scache[i].buf_len) {
      *len = 0;
      return;
   }

   if (pos + (*len) > scache[i].buf_len)
      *len = scache[i].buf_len - pos;

   *buf = scache[i].buffer + pos;
   if (reverse)
      (*buf) += scache[i].buf_len;
}



typedef struct ALSA_VOICE {
   int cache;

   unsigned int pos;
   unsigned int frame_size;

   unsigned int frag_len;

   unsigned int silence;

   bool reverse;

   _AL_MUTEX mutex;

   volatile bool stop;
   volatile bool stopped;
   ALLEGRO_PLAYMODE loop_mode;

   _AL_THREAD poll_thread;
   bool quit_poll_thread;

   snd_pcm_t *pcm_handle;
} ALSA_VOICE;


/* initialized output */
static int alsa_open()
{
   ALSA_CHECK(snd_output_stdio_attach(&snd_output, stdout, 0));
   return 0;
   
   /* ALSA check is a macro that 'goto' error*/
   Error:
      fprintf(stderr, "Error initializing alsa");
      return 1;
}



/* The close method should close the device, freeing any resources, and allow
   other processes to use the device */
static void alsa_close()
{
   snd_config_update_free_global();
}

/* Underrun and suspend recovery */
static int xrun_recovery(snd_pcm_t *handle, int err)
{
   if (err == -EPIPE) { /* under-run */
      err = snd_pcm_prepare(handle);
      if (err < 0) {
         fprintf(stderr, "Can't recover from underrun, prepare failed: %s\n", snd_strerror(err));
      }
      return 0;
   }
   else if (err == -ESTRPIPE) {
      while ((err = snd_pcm_resume(handle)) == -EAGAIN) {
         /* wait until the suspend flag is released */
         al_rest(0.010);
      }
      if (err < 0) {
         err = snd_pcm_prepare(handle);
         if (err < 0) {
            fprintf(stderr, "Can't recover from suspend, prepare failed: %s\n", snd_strerror(err));
         }
      }
      return 0;
   }
   return err;
}



/* Custom routine which runs in another thread and fills the harware PCM buffer with data
   from the stream. */
static int alsa_update_stream(ALLEGRO_VOICE *voice, void **buf)
{
   ALSA_VOICE *alsa_voice = (ALSA_VOICE*)voice->extra;
   int buf_len = alsa_voice->frag_len;

   *buf = (unsigned char*)_al_voice_update(voice, buf_len / alsa_voice->frame_size);
   if (!*buf)
      return 0;

   return buf_len;
}


/* Custom routine which runs in another thread and fills the hardware PCM buffer
   from the voice buffer. */
static int alsa_update_nonstream(ALLEGRO_VOICE *voice, void **buf)
{
   ALSA_VOICE *alsa_voice = (ALSA_VOICE*)voice->extra;
   unsigned int buf_len;

   do {
      buf_len = alsa_voice->frag_len;
      _al_read_sample_cache(alsa_voice->cache, alsa_voice->pos, alsa_voice->reverse, buf, &buf_len);
      alsa_voice->pos += buf_len;

      if (buf_len < alsa_voice->frag_len) {
         alsa_voice->pos -= alsa_voice->frag_len;
         if (alsa_voice->loop_mode == ALLEGRO_PLAYMODE_ONCE) {
            alsa_voice->stop = true;
            alsa_voice->pos = 0;
         }
         else if (alsa_voice->loop_mode == ALLEGRO_PLAYMODE_BIDIR)
            alsa_voice->reverse = !alsa_voice->reverse;
      }
   } while (!alsa_voice->stop && !buf_len);

   return buf_len;
}


static void alsa_update(_AL_THREAD* self, void *arg)
{
   ALLEGRO_VOICE *voice = (ALLEGRO_VOICE*)arg;
   ALSA_VOICE *alsa_voice = (ALSA_VOICE*)voice->extra;
   void *buf, *tmp_buf;
   int res;

   tmp_buf = malloc(alsa_frag_size);
   if (!tmp_buf)
      return;

   alsa_voice->stopped = true;

   while (!alsa_voice->quit_poll_thread) {
      _al_mutex_lock(&alsa_voice->mutex);

      if (alsa_voice->stop) {
         _al_mutex_unlock(&alsa_voice->mutex);

         if (!alsa_voice->stopped) {
            snd_pcm_drain(alsa_voice->pcm_handle);
            alsa_voice->stopped = true;
         }

         usleep(10000);
         continue;
      }

      if (alsa_voice->stopped) {
         alsa_voice->stopped = false;
         snd_pcm_start(alsa_voice->pcm_handle);
      }


      if (!voice->is_streaming)
         res = alsa_update_nonstream(voice, &buf);
      else
         res = alsa_update_stream(voice, &buf);

      /* We need to pass a copy of buffer fragment and paramaters to alsa
         because snd_pcm_writei() is blocking and we have to unlock the mutex
         before it in order to have it unlocked for a while. */
      if (!res) {
         res = alsa_voice->frag_len;
         if (alsa_voice->silence > 0xFFFFu) {
            int i = res / sizeof(uint32_t);
            while(i-- > 0)
               ((uint32_t*)tmp_buf)[i] = alsa_voice->silence; 
         }
         else if (alsa_voice->silence > 0xFFu) {
            int i = res / sizeof(uint16_t);
            while(i-- > 0)
               ((uint16_t*)tmp_buf)[i] = alsa_voice->silence;
         }
         else
            memset(tmp_buf, alsa_voice->silence, res);
      }
      else
         memcpy(tmp_buf, buf, res);

      _al_mutex_unlock(&alsa_voice->mutex);

      res = snd_pcm_writei(alsa_voice->pcm_handle, tmp_buf, res / alsa_voice->frame_size);
      if (res < 0) {
         if (xrun_recovery(alsa_voice->pcm_handle, res) < 0) {
            fprintf(stderr, "Write error: %s\n", snd_strerror(res));
            continue;
         }
      }
   }

   free(tmp_buf);
}



/* The load_voice method loads a sample into the driver's memory. The voice's
   'streaming' field will be set to false for these voices, and it's
   'buffer_size' field will be the total length in bytes of the sample data.
   The voice's attached stream's looping mode should be honored, and loading
   must fail if it cannot be. */
static int alsa_load_voice(ALLEGRO_VOICE *voice, const void *data)
{
   ALSA_VOICE *ex_data = voice->extra;

   _al_mutex_lock(&ex_data->mutex);

   ex_data->cache = _al_cache_sample(voice->stream);

   ex_data->pos = 0;
   ex_data->loop_mode = voice->stream->loop;

   ex_data->reverse = (voice->stream->speed < 0);

   _al_mutex_unlock(&ex_data->mutex);

   return 0;
   (void)data;
}



/* The unload_voice method unloads a sample previously loaded with load_voice.
   This method should not be called on a streaming voice. */
static void alsa_unload_voice(ALLEGRO_VOICE *voice)
{
   ALSA_VOICE *ex_data = voice->extra;

   _al_mutex_lock(&ex_data->mutex);

   _al_decache_sample(ex_data->cache);
   ex_data->cache = -1;

   _al_mutex_unlock(&ex_data->mutex);
}



/* The start_voice should, surprise, start the voice. For streaming voices, it
   should start polling the device and call _al_voice_update for audio data.
   For non-streaming voices, it should resume playing from the last set
   position */
static int alsa_start_voice(ALLEGRO_VOICE *voice)
{
   ALSA_VOICE *ex_data = voice->extra;

   _al_mutex_lock(&ex_data->mutex);

   if (voice->is_streaming && (voice->buffer_size || voice->num_buffers)) {
      snd_pcm_hw_params_t *hwparams;
      snd_pcm_hw_params_alloca(&hwparams);

      snd_pcm_hw_params_current(ex_data->pcm_handle, hwparams);

      if (voice->buffer_size)
         ALSA_CHECK(snd_pcm_hw_params_set_period_size(ex_data->pcm_handle, hwparams, voice->buffer_size, 1));
      if (voice->num_buffers)
         ALSA_CHECK(snd_pcm_hw_params_set_periods(ex_data->pcm_handle, hwparams, voice->num_buffers, 1));

      snd_pcm_hw_params(ex_data->pcm_handle, hwparams);
   }

   ex_data->stop = false;

   _al_mutex_unlock(&ex_data->mutex);
   return 0;

Error:
   _al_mutex_unlock(&ex_data->mutex);
   return 1;
}



/* The stop_voice method should stop playback. For non-streaming voices, it
   should leave the data loaded, and reset the voice position to 0. */
static int alsa_stop_voice(ALLEGRO_VOICE *voice)
{
   ALSA_VOICE *ex_data = voice->extra;

   _al_mutex_lock(&ex_data->mutex);

   ex_data->stop = true;
   if(!voice->is_streaming) {
      ex_data->pos = 0;
   }

   _al_mutex_unlock(&ex_data->mutex);

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
   int channels;

   ALSA_VOICE *ex_data = calloc(1, sizeof(ALSA_VOICE));
   if (!ex_data)
      return 1;

   ex_data->cache = -1;

   ex_data->frame_size = al_channel_count(voice->chan_conf) *
                         al_depth_size(voice->depth);
   if (!ex_data->frame_size)
      goto Error;

   ex_data->frag_len = alsa_frag_size;

   if (voice->depth == ALLEGRO_AUDIO_DEPTH_UINT8)
      ex_data->silence = 0x80;
   else if (voice->depth == ALLEGRO_AUDIO_DEPTH_UINT16)
      ex_data->silence = 0x8000;
   else if (voice->depth == ALLEGRO_AUDIO_DEPTH_UINT24)
      ex_data->silence = 0x800000;


   ex_data->stop = true;
   ex_data->stopped = true;
   ex_data->quit_poll_thread = false;


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

   if (voice->chan_conf != ALLEGRO_CHANNEL_CONF_1 &&
       voice->chan_conf != ALLEGRO_CHANNEL_CONF_2 &&
       voice->chan_conf != ALLEGRO_CHANNEL_CONF_4 &&
       voice->chan_conf != ALLEGRO_CHANNEL_CONF_5_1 &&
       voice->chan_conf != ALLEGRO_CHANNEL_CONF_7_1) {
         goto Error;
      voice->chan_conf = ALLEGRO_CHANNEL_CONF_2;
   }

   channels = al_channel_count(voice->chan_conf);

   unsigned int freq = voice->frequency;


   ALSA_CHECK(snd_pcm_open(&ex_data->pcm_handle, alsa_device, SND_PCM_STREAM_PLAYBACK, 0));



   snd_pcm_hw_params_t *hwparams;
   snd_pcm_hw_params_alloca(&hwparams);

   ALSA_CHECK(snd_pcm_hw_params_any(ex_data->pcm_handle, hwparams));
   ALSA_CHECK(snd_pcm_hw_params_set_access(ex_data->pcm_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED));
   ALSA_CHECK(snd_pcm_hw_params_set_format(ex_data->pcm_handle, hwparams, format));
   ALSA_CHECK(snd_pcm_hw_params_set_channels(ex_data->pcm_handle, hwparams, channels));

   ALSA_CHECK(snd_pcm_hw_params_set_rate_near(ex_data->pcm_handle, hwparams, &freq, NULL));
   ALSA_CHECK(snd_pcm_hw_params_set_period_size_near(ex_data->pcm_handle, hwparams, &alsa_frag_size, NULL));
   ALSA_CHECK(snd_pcm_hw_params_set_periods_near(ex_data->pcm_handle, hwparams, &alsa_frag_count, NULL));

   ALSA_CHECK(snd_pcm_hw_params(ex_data->pcm_handle, hwparams));

   ALSA_CHECK(snd_pcm_hw_params_get_rate(hwparams, &freq, NULL));
   ALSA_CHECK(snd_pcm_hw_params_get_period_size(hwparams, &alsa_frag_size, NULL));
   ALSA_CHECK(snd_pcm_hw_params_get_periods(hwparams, &alsa_frag_count, NULL));

   if (voice->frequency != freq)
      goto Error;


   snd_pcm_sw_params_t *swparams;
   snd_pcm_sw_params_alloca(&swparams);

   ALSA_CHECK(snd_pcm_sw_params_current(ex_data->pcm_handle, swparams));
   ALSA_CHECK(snd_pcm_sw_params_set_start_threshold(ex_data->pcm_handle, swparams, ex_data->frag_len));
   ALSA_CHECK(snd_pcm_sw_params_set_avail_min(ex_data->pcm_handle, swparams, ex_data->frag_len));
   ALSA_CHECK(snd_pcm_sw_params_set_xfer_align(ex_data->pcm_handle, swparams, 1));
   ALSA_CHECK(snd_pcm_sw_params(ex_data->pcm_handle, swparams));


   _al_mutex_init(&ex_data->mutex);

   voice->extra = ex_data;

   _al_thread_create(&ex_data->poll_thread, alsa_update, (void*) voice);

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
   _al_thread_join(&alsa_voice->poll_thread);

   snd_pcm_close(alsa_voice->pcm_handle);

   _al_mutex_destroy(&alsa_voice->mutex);

   free(voice->extra);
   voice->extra = NULL;
}



/* The get_voice_position method should return the current sample position of
   the voice (sample_pos = byte_pos / (depth/8) / channels). This should never
   be called on a streaming voice. */
static unsigned long alsa_get_voice_position(const ALLEGRO_VOICE *voice)
{
   ALSA_VOICE *ex_data = voice->extra;
   if (ex_data->reverse)
      return voice->stream->len - (ex_data->pos / ex_data->frame_size) - 1;
   return ex_data->pos / ex_data->frame_size;
}



/* The set_voice_position method should set the voice's playback position,
   given the value in samples. This should never be called on a streaming
   voice. */
static int alsa_set_voice_position(ALLEGRO_VOICE *voice, unsigned long val)
{
   ALSA_VOICE *ex_data = voice->extra;

   _al_mutex_lock(&ex_data->mutex);
   ex_data->pos = val * ex_data->frame_size;
   _al_mutex_unlock(&ex_data->mutex);

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
   alsa_set_voice_position,
};
