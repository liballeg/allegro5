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

#define TRACE_PREFIX "a5-alsa: "


/* To be put into ALSA_CHECK macro. */
/* TODO: replace this with something cleaner */
#define ALSA_CHECK(a) \
do {                                                                  \
   int err = (a);                                                     \
   if (err < 0) {                                                     \
      TRACE(TRACE_PREFIX "%s: %s\n", snd_strerror(err), #a);          \
      goto Error;                                                     \
   }                                                                  \
} while(0)

static snd_output_t *alsa_snd_output = NULL;
static const char alsa_device[128] = "default";

static _AL_THREAD alsa_poll_thread;
static volatile bool alsa_quit_poll_thread;
static _AL_VECTOR alsa_voices_list;
static _AL_MUTEX alsa_voices_list_mutex;
static void alsa_update(_AL_THREAD* self, void *alsa_voices_list);


typedef struct ALSA_VOICE {
   char *buf;
   unsigned int buf_len;
   unsigned int pos;

   unsigned int frame_size;

   /* in frames! */
   snd_pcm_uframes_t frag_len;

   struct pollfd *ufds;
   int ufds_count;

   volatile bool stop;
   volatile bool stopped;
   bool loop_mode;

   snd_pcm_t *pcm_handle;
} ALSA_VOICE;


/* initialized output */
static int alsa_open(void)
{
   ALSA_CHECK(snd_output_stdio_attach(&alsa_snd_output, stdout, 0));

   _al_vector_init(&alsa_voices_list, sizeof(ALSA_VOICE*));
   _al_mutex_init(&alsa_voices_list_mutex);

   alsa_quit_poll_thread = false;
   _al_thread_create(&alsa_poll_thread, alsa_update, &alsa_voices_list);

   return 0;

   /* ALSA check is a macro that 'goto' error*/
   Error:
      TRACE(TRACE_PREFIX "Error initializing alsa\n");
      return 1;
}



/* The close method should close the device, freeing any resources, and allow
   other processes to use the device */
static void alsa_close(void)
{
   alsa_quit_poll_thread = true;
   _al_thread_join(&alsa_poll_thread);

   _al_mutex_destroy(&alsa_voices_list_mutex);
   _al_vector_free(&alsa_voices_list);

   snd_config_update_free_global();
}

/* Underrun and suspend recovery */
static int xrun_recovery(snd_pcm_t *handle, int err)
{
   if (err == -EPIPE) { /* under-run */
      err = snd_pcm_prepare(handle);
      if (err < 0) {
         TRACE(TRACE_PREFIX "Can't recover from underrun, prepare failed: %s\n", snd_strerror(err));
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
            TRACE(TRACE_PREFIX "Can't recover from suspend, prepare failed: %s\n", snd_strerror(err));
         }
      }
      return 0;
   }
   return err;
}


static int alsa_update_stream_voice(ALLEGRO_VOICE *voice, void *buf, int bytes)
{
   ALSA_VOICE *alsa_voice = (ALSA_VOICE*)voice->extra;

   voice->stream->dried_up = !voice->stream->stream_update(voice->stream, buf, bytes);

   return 0;
}


static int alsa_update_nonstream_voice(ALLEGRO_VOICE *voice, void **buf, int *bytes)
{
   ALSA_VOICE *alsa_voice = (ALSA_VOICE*)voice->extra;

   *buf = alsa_voice->buf + alsa_voice->pos;

   if (alsa_voice->pos + *bytes > alsa_voice->buf_len) {
      *bytes = alsa_voice->buf_len - alsa_voice->pos;
      if (!alsa_voice->loop_mode)
         alsa_voice->stop = true;

      alsa_voice->pos = 0;
      return 1;
   }
   else
      alsa_voice->pos += *bytes;

   return 0;
}


static int alsa_update_voice_status(ALSA_VOICE *alsa_voice)
{
   if (alsa_voice->stop && !alsa_voice->stopped) {
      alsa_voice->stopped = true;
   }
   else if (!alsa_voice->stop && alsa_voice->stopped) {
      alsa_voice->stopped = false;
   }

   snd_pcm_state_t state = snd_pcm_state(alsa_voice->pcm_handle);
   if (state == SND_PCM_STATE_XRUN) {
      int err = xrun_recovery(alsa_voice->pcm_handle, -EPIPE);
      if (err < 0) {
         TRACE(TRACE_PREFIX "XRUN recovery failed: %s\n", snd_strerror(err));
         return err;
      }
   } else if (state == SND_PCM_STATE_SUSPENDED) {
      int err = xrun_recovery(alsa_voice->pcm_handle, -ESTRPIPE);
      if (err < 0) {
         TRACE(TRACE_PREFIX "SUSPEND recovery failed: %s\n", snd_strerror(err));
         return err;
      }
   }

   if (snd_pcm_state(alsa_voice->pcm_handle) == SND_PCM_STATE_PREPARED) {
      snd_pcm_start(alsa_voice->pcm_handle);
   }

   return 0;
}


static int alsa_voice_can_IO(ALSA_VOICE *alsa_voice)
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
            TRACE(TRACE_PREFIX "Write error: %s\n", snd_strerror(err));
            return -POLLERR;
         }
      } else {
         TRACE(TRACE_PREFIX "Wait for poll failed\n");
         return -POLLERR;
      }
   }

   if (revents & POLLOUT)
      return 1;

   return 0;
}


static int alsa_poll_voice(ALLEGRO_VOICE *voice) {
   int err;
   snd_pcm_uframes_t frames;
   const snd_pcm_channel_area_t *areas;
   snd_pcm_uframes_t offset;
   void *mmap;
   ALSA_VOICE *alsa_voice = (ALSA_VOICE*)voice->extra;

   alsa_update_voice_status(alsa_voice);

   if ((err = alsa_voice_can_IO(alsa_voice)) <= 0)
      return err;

   snd_pcm_avail_update(alsa_voice->pcm_handle);

   frames = alsa_voice->frag_len;
   err = snd_pcm_mmap_begin(alsa_voice->pcm_handle, &areas, &offset, &frames);
   if (err < 0) {
      if ((err = xrun_recovery(alsa_voice->pcm_handle, err)) < 0) {
         TRACE(TRACE_PREFIX "MMAP begin avail error: %s\n", snd_strerror(err));
         return err;
      }
      return 0;
   }

   if (!frames)
      return 0; /* Shouldn't happen. */

   /* Driver's memory. Interleaved channels format. */
   mmap = areas[0].addr + areas[0].first / 8 + offset * areas[0].step / 8;

   /* Read sample data into the buffer. */
   if (!voice->streaming && !alsa_voice->stopped) {
      void *buf;
      int bytes = frames * alsa_voice->frame_size;
      alsa_update_nonstream_voice(voice, &buf, &bytes);
      memcpy(mmap, buf, bytes);
      frames = bytes / alsa_voice->frame_size;
   }
   else if (voice->streaming && !alsa_voice->stopped) {
      int bytes = frames * alsa_voice->frame_size;
      alsa_update_stream_voice(voice, mmap, bytes);
   }
   else {
      /* If stopped just fill with silence. */
      ALLEGRO_AUDIO_ENUM depth;
      if (voice->streaming)
         depth = voice->stream->depth;
      else
         depth = voice->sample->depth;
      int silence = _al_audio_get_silence(depth);
      memset(mmap, silence, frames * alsa_voice->frame_size);
   }

   snd_pcm_sframes_t commitres = snd_pcm_mmap_commit(alsa_voice->pcm_handle, offset, frames);
   if (commitres < 0 || (snd_pcm_uframes_t)commitres != frames) {
      if ((err = xrun_recovery(alsa_voice->pcm_handle, commitres >= 0 ? -EPIPE : commitres)) < 0) {
         TRACE(TRACE_PREFIX "MMAP commit error: %s\n", snd_strerror(err));
         return err;
      }
   }

   return 0;
}


/* Custom routine which runs in another thread and fills the hardware PCM buffer.
   Common to stream and non-stream sources. */
static void alsa_update(_AL_THREAD* self, void* alsa_voices_list_p)
{
   unsigned int i;
   _AL_VECTOR *alsa_voices_list = alsa_voices_list_p;

   while (!alsa_quit_poll_thread) {
      _al_mutex_lock(&alsa_voices_list_mutex);
      for (i = 0; i < _al_vector_size(alsa_voices_list); ++i) {
         ALLEGRO_VOICE *voice = *((ALLEGRO_VOICE**)_al_vector_ref(alsa_voices_list, i));
         int res = alsa_poll_voice(voice);
         if (res < 0) {
            TRACE(TRACE_PREFIX "Error polling voice: %i\n", res);
            ALSA_VOICE* alsa_voice = voice->extra;
            alsa_voice->stop = true;
         }
      }
      _al_mutex_unlock(&alsa_voices_list_mutex);

      al_rest(0.005);
   }
}


/* The load_voice method loads a sample into the driver's memory. The voice's
   'streaming' field will be set to false for these voices, and it's
   'buffer_size' field will be the total length in bytes of the sample data.
   The voice's attached stream's looping mode should be honored, and loading
   must fail if it cannot be. */
static int alsa_load_voice(ALLEGRO_VOICE *voice)
{
   int len;
   ALSA_VOICE *ex_data = voice->extra;

   ex_data->pos = 0;
   ex_data->loop_mode = FALSE;

   len = voice->sample->length * al_audio_channel_count(voice->sample->chan_conf)
                               * al_audio_depth_size(voice->sample->depth);
   ex_data->buf_len = len;
   ex_data->buf = malloc(len);
   memcpy(ex_data->buf, voice->sample->buffer, len);

   return 0;
}



/* The unload_voice method unloads a sample previously loaded with load_voice.
   This method should not be called on a streaming voice. */
static void alsa_unload_voice(ALLEGRO_VOICE *voice)
{
   ALSA_VOICE *ex_data = voice->extra;

   free(ex_data->buf);
   ex_data->buf = NULL;
}



/* The start_voice should, surprise, start the voice. For streaming voices, it
   should start polling the device and call _al_voice_update for audio data.
   For non-streaming voices, it should resume playing from the last set
   position */
static int alsa_start_voice(ALLEGRO_VOICE *voice)
{
   if (!voice->streaming) {
      alsa_load_voice(voice);
   }

   ALSA_VOICE *ex_data = voice->extra;
   ex_data->stop = false;

   return 0;
}



/*
   FIXME: either the comments or the code is wrong!
   The stop_voice method should stop playback. For non-streaming voices, it
   should leave the data loaded, and reset the voice position to 0. */
static int alsa_stop_voice(ALLEGRO_VOICE *voice)
{
   ALSA_VOICE *ex_data = voice->extra;

   snd_pcm_drop(ex_data->pcm_handle);

   ex_data->stop = true;

   /* Before we exit or free the sample data make sure the voice is not about
    * to be played. */
   _al_mutex_lock(&alsa_voices_list_mutex);
   if (!voice->streaming) {
      ex_data->pos = 0;
      alsa_unload_voice(voice);
   }
   _al_mutex_unlock(&alsa_voices_list_mutex);

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
   unsigned int freq, req_freq;

   ALSA_VOICE *ex_data = calloc(1, sizeof(ALSA_VOICE));
   if (!ex_data)
      return 1;

   memset(ex_data, 0, sizeof(*ex_data));

   if (voice->streaming) {
      depth = voice->stream->depth;
      chan_conf = voice->stream->chan_conf;
      freq = req_freq = voice->stream->frequency;
   }
   else {
      depth = voice->sample->depth;
      chan_conf = voice->sample->chan_conf;
      freq = req_freq = voice->sample->frequency;
   }

   chan_count = al_audio_channel_count(chan_conf);

   ex_data->frame_size = chan_count * al_audio_depth_size(depth);
   if (!ex_data->frame_size)
      goto Error;

   ex_data->stop = true;
   ex_data->stopped = true;
   /* ALSA usually overrides this, but set it just in case. */
   ex_data->frag_len = 256;

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
         TRACE(TRACE_PREFIX "Unsupported ALSA sound format");
         goto Error;
   }

   if (chan_conf == ALLEGRO_AUDIO_3_CH)
      goto Error;

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

   if (freq != req_freq) {
      TRACE(TRACE_PREFIX "Unsupported rate! Requested %i, got %i.\n", req_freq, freq);
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

   _al_mutex_lock(&alsa_voices_list_mutex);
   ALLEGRO_VOICE** add = _al_vector_alloc_back(&alsa_voices_list);
   *add = voice;
   _al_mutex_unlock(&alsa_voices_list_mutex);

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

   _al_mutex_lock(&alsa_voices_list_mutex);
   _al_vector_find_and_delete(&alsa_voices_list, &voice);
   _al_mutex_unlock(&alsa_voices_list_mutex);

   snd_pcm_drop(alsa_voice->pcm_handle);
   snd_pcm_close(alsa_voice->pcm_handle);

   free(alsa_voice->ufds);
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

ALLEGRO_AUDIO_DRIVER _al_alsa_driver =
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
