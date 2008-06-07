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
#include "allegro5/internal/aintern_audio.h"

#include <alsa/asoundlib.h>

/* To be put into ALSA_CHECK macro. */
/* TODO: replace this with something cleaner */
#define ALSA_CHECK(a) \
do {                                                                  \
   int err = (a);                                                     \
   if (err < 0) {                                                     \
      TRACE("%s: %s\n", snd_strerror(err), #a);                       \
      goto Error;                                                     \
   }                                                                  \
} while(0)

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

   scache[i].frame_size = al_audio_channel_count(sample->chan_conf) * al_audio_depth_size(sample->depth);
   if (!scache[i].frame_size)
      return -1;

   scache[i].buf_len = (sample->length) * scache[i].frame_size;
   scache[i].buffer = malloc(scache[i].buf_len * 2);
   if (!scache[i].buffer)
      return -1;

   /* Put a normal copy in the first half */
   memcpy(scache[i].buffer, sample->buffer, scache[i].buf_len); 

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


void _al_read_sample_cache(int i, unsigned int pos, void **buf, unsigned int *len)
{
   if (pos >= scache[i].buf_len) {
      *len = 0;
      return;
   }

   if (pos + (*len) > scache[i].buf_len)
      *len = scache[i].buf_len - pos;

   *buf = scache[i].buffer + pos;
}



typedef struct ALSA_VOICE {
   int cache;

   unsigned int pos;
   unsigned int frame_size;

    unsigned int frag_len;
//   unsigned int buffer_size;
//   unsigned int num_buffers;

   volatile bool stop;
   volatile bool stopped;
   bool loop_mode;

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
      TRACE("Error initializing alsa\n");
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
         TRACE("Can't recover from underrun, prepare failed: %s\n", snd_strerror(err));
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
            TRACE("Can't recover from suspend, prepare failed: %s\n", snd_strerror(err));
         }
      }
      return 0;
   }
   return err;
}


static int alsa_update_stream(ALLEGRO_VOICE *voice, void **buf)
{
   ALSA_VOICE *alsa_voice = (ALSA_VOICE*)voice->extra;
   int buf_len = alsa_voice->frag_len;

   voice->stream->dried_up = !voice->stream->stream_update(voice->stream, buf, buf_len);

   return buf_len;
}


static int alsa_update_nonstream(ALLEGRO_VOICE *voice, void **buf)
{
   ALSA_VOICE *alsa_voice = (ALSA_VOICE*)voice->extra;
   unsigned int buf_len;

   do {
      buf_len = alsa_voice->frag_len;
      _al_read_sample_cache(alsa_voice->cache, alsa_voice->pos, buf, &buf_len);
      alsa_voice->pos += buf_len;

      if (buf_len < alsa_voice->frag_len)
      {
         alsa_voice->pos = 0;
         if (!alsa_voice->loop_mode)
         {
            alsa_voice->stop = true;
         }
      }
   } while (!alsa_voice->stop && !buf_len);

   return buf_len;
}


/* Custom routine which runs in another thread and fills the hardware PCM buffer.
   Common to stream and non-stream sources. */
static void alsa_update(_AL_THREAD* self, void *arg)
{
   ALLEGRO_VOICE *voice = (ALLEGRO_VOICE*)arg;
   ALSA_VOICE *alsa_voice = (ALSA_VOICE*)voice->extra;
   void *buf, *tmp_buf;
   int res;

   tmp_buf = malloc(alsa_frag_size);
   if (!tmp_buf)
      return;

   while (!alsa_voice->quit_poll_thread) {

      if (alsa_voice->stop) {
         if (!alsa_voice->stopped) {
            snd_pcm_drain(alsa_voice->pcm_handle);
            alsa_voice->stopped = true;
         }

         al_rest(0.01);
         continue;
      }

      /* The voice has just be de-stopped, since we got here. */
      if (alsa_voice->stopped) {
         alsa_voice->stopped = false;
         snd_pcm_start(alsa_voice->pcm_handle);
      }

      /* Read some sample data into the buffer. */
      if (!voice->streaming) {
         res = alsa_update_nonstream(voice, &buf);

         if (!res)
            fill_with_silence(tmp_buf, alsa_frag_size, voice->sample->depth);
         else
            memcpy(tmp_buf, buf, res);
      }
      else {
         res = alsa_update_stream(voice, tmp_buf);
      }

      res = snd_pcm_writei(alsa_voice->pcm_handle, tmp_buf, res / alsa_voice->frame_size);
      if (res < 0) {
         if (xrun_recovery(alsa_voice->pcm_handle, res) < 0) {
            TRACE("Write error: %s\n", snd_strerror(res));
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
static int alsa_load_voice(ALLEGRO_VOICE *voice)
{
   ALSA_VOICE *ex_data = voice->extra;

   ex_data->cache = _al_cache_sample(voice->sample);

   ex_data->pos = 0;
   ex_data->loop_mode = FALSE;


   return 0;
}



/* The unload_voice method unloads a sample previously loaded with load_voice.
   This method should not be called on a streaming voice. */
static void alsa_unload_voice(ALLEGRO_VOICE *voice)
{
   ALSA_VOICE *ex_data = voice->extra;

   _al_decache_sample(ex_data->cache);
   ex_data->cache = -1;

}



/* The start_voice should, surprise, start the voice. For streaming voices, it
   should start polling the device and call _al_voice_update for audio data.
   For non-streaming voices, it should resume playing from the last set
   position */
static int alsa_start_voice(ALLEGRO_VOICE *voice)
{
   if (!voice->stream) {
      alsa_load_voice(voice);
   }

   ALSA_VOICE *ex_data = voice->extra;
   ex_data->stop = false;

   return 0;
}



/* The stop_voice method should stop playback. For non-streaming voices, it
   should leave the data loaded, and reset the voice position to 0. */
static int alsa_stop_voice(ALLEGRO_VOICE *voice)
{

   alsa_unload_voice(voice);
   ALSA_VOICE *ex_data = voice->extra;

   ex_data->stop = true;
   if(!voice->streaming) {
      ex_data->pos = 0;
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
   ALLEGRO_AUDIO_ENUM chan_conf;
   ALLEGRO_AUDIO_ENUM depth;
   int chan_count;
   unsigned int freq;

   ALSA_VOICE *ex_data = calloc(1, sizeof(ALSA_VOICE));
   if (!ex_data)
      return 1;

   memset(ex_data, 0, sizeof(*ex_data));

   if (voice->streaming) {
      depth = voice->stream->depth;
      chan_conf = voice->stream->chan_conf;
      freq = voice->stream->frequency;
   }
   else {
      depth = voice->sample->depth;
      chan_conf = voice->sample->chan_conf;
      freq = voice->sample->frequency;
   }

   chan_count = al_audio_channel_count(chan_conf);

   ex_data->frame_size = chan_count * al_audio_depth_size(depth);
   if (!ex_data->frame_size)
      goto Error;

   ex_data->stop = true;
   ex_data->stopped = true;
   ex_data->quit_poll_thread = false;
   ex_data->cache = -1;

   /* FIXME: move to a better place */
//   ex_data->buffer_size = al_audio_buffer_size(voice->sample);
   /* FIXME: correct this for streaming */
//   ex_data->num_buffers = 1;

/*   if (voice->streaming && (ex_data->buffer_size || ex_data->num_buffers)) {
      snd_pcm_hw_params_t *hwparams;
      snd_pcm_hw_params_alloca(&hwparams);

      snd_pcm_hw_params_current(ex_data->pcm_handle, hwparams);

      if (ex_data->buffer_size)
         ALSA_CHECK(snd_pcm_hw_params_set_period_size(ex_data->pcm_handle, hwparams, ex_data->buffer_size, 1));
      if (ex_data->num_buffers)
         ALSA_CHECK(snd_pcm_hw_params_set_periods(ex_data->pcm_handle, hwparams, ex_data->num_buffers, 1));

      snd_pcm_hw_params(ex_data->pcm_handle, hwparams);
   }*/

   switch(depth)
   {
      case ALLEGRO_AUDIO_8_BIT_UINT:
         format = SND_PCM_FORMAT_U8;
         break;
      case ALLEGRO_AUDIO_16_BIT_INT:
         format = SND_PCM_FORMAT_S16;
         break;
      case ALLEGRO_AUDIO_24_BIT_INT:
         format = SND_PCM_FORMAT_S24;
         break;
      case ALLEGRO_AUDIO_32_BIT_FLOAT:
         format = SND_PCM_FORMAT_FLOAT;
         break;
      default:
         TRACE("Unsupported ALSA sound format");
         goto Error;
   }

   if (chan_conf == ALLEGRO_AUDIO_3_CH)
      goto Error;

   ALSA_CHECK(snd_pcm_open(&ex_data->pcm_handle, alsa_device, SND_PCM_STREAM_PLAYBACK, 0));

   snd_pcm_hw_params_t *hwparams;
   snd_pcm_hw_params_alloca(&hwparams);

   ALSA_CHECK(snd_pcm_hw_params_any(ex_data->pcm_handle, hwparams));
   ALSA_CHECK(snd_pcm_hw_params_set_access(ex_data->pcm_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED));
   ALSA_CHECK(snd_pcm_hw_params_set_format(ex_data->pcm_handle, hwparams, format));
   ALSA_CHECK(snd_pcm_hw_params_set_channels(ex_data->pcm_handle, hwparams, chan_count));

   ALSA_CHECK(snd_pcm_hw_params_set_rate_near(ex_data->pcm_handle, hwparams, &freq, NULL));
   ALSA_CHECK(snd_pcm_hw_params_set_period_size_near(ex_data->pcm_handle, hwparams, &alsa_frag_size, NULL));
   ALSA_CHECK(snd_pcm_hw_params_set_periods_near(ex_data->pcm_handle, hwparams, &alsa_frag_count, NULL));

   ALSA_CHECK(snd_pcm_hw_params(ex_data->pcm_handle, hwparams));

   ALSA_CHECK(snd_pcm_hw_params_get_rate(hwparams, &freq, NULL));
   ALSA_CHECK(snd_pcm_hw_params_get_period_size(hwparams, &alsa_frag_size, NULL));
   ALSA_CHECK(snd_pcm_hw_params_get_periods(hwparams, &alsa_frag_count, NULL));

   if (voice->stream) {
      if (voice->stream->frequency != freq)
         goto Error;
   }
   else {
      if (voice->sample->frequency != freq)
         goto Error;
   }

   ex_data->frag_len = alsa_frag_size;

   snd_pcm_sw_params_t *swparams;
   snd_pcm_sw_params_alloca(&swparams);

   ALSA_CHECK(snd_pcm_sw_params_current(ex_data->pcm_handle, swparams));
   ALSA_CHECK(snd_pcm_sw_params_set_start_threshold(ex_data->pcm_handle, swparams, ex_data->frag_len));
   ALSA_CHECK(snd_pcm_sw_params_set_avail_min(ex_data->pcm_handle, swparams, ex_data->frag_len));
   ALSA_CHECK(snd_pcm_sw_params_set_xfer_align(ex_data->pcm_handle, swparams, 1));
   ALSA_CHECK(snd_pcm_sw_params(ex_data->pcm_handle, swparams));

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

   free(voice->extra);
   voice->extra = NULL;
}



/* The get_voice_position method should return the current sample position of
   the voice (sample_pos = byte_pos / (depth/8) / channels). This should never
   be called on a streaming voice. */
static unsigned long alsa_get_voice_position(const ALLEGRO_VOICE *voice)
{
   ALSA_VOICE *ex_data = voice->extra;
   return ex_data->pos / ex_data->frame_size;
}



/* The set_voice_position method should set the voice's playback position,
   given the value in samples. This should never be called on a streaming
   voice. */
static int alsa_set_voice_position(ALLEGRO_VOICE *voice, unsigned long val)
{
   ALSA_VOICE *ex_data = voice->extra;

   ex_data->pos = val * ex_data->frame_size;

   return 0;
}

static int alsa_set_loop(ALLEGRO_VOICE* voice, bool loop)
{
   assert(voice);
   ALSA_VOICE *ex_data = voice->extra;
   ex_data->loop_mode = loop;
   return 0;
}

static bool alsa_get_loop(ALLEGRO_VOICE* voice)
{
   assert(voice);
   ALSA_VOICE *ex_data = voice->extra;
   return ex_data->loop_mode;
}

ALLEGRO_AUDIO_DRIVER _alsa_driver =
{
   alsa_open,
   alsa_close,

   alsa_allocate_voice,
   alsa_deallocate_voice,

   alsa_start_voice,
   alsa_stop_voice,

   alsa_voice_is_playing,

   alsa_get_voice_position,
   alsa_set_voice_position,

   alsa_set_loop,
   alsa_get_loop
};
