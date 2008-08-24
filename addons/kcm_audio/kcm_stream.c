/**
 * Originally digi.c from allegro wiki
 * Original authors: KC/Milan
 *
 * Converted to allegro5 by Ryan Dickie
 */

/* Title: Stream functions
 */

#include <stdio.h>

#include "allegro5/kcm_audio.h"
#include "allegro5/internal/aintern_kcm_audio.h"
#include "allegro5/internal/aintern_kcm_cfg.h"



/* Function: al_stream_create
 *  Creates an audio stream, using the supplied values. The stream will be
 *  set to play by default.
 */
ALLEGRO_STREAM *al_stream_create(size_t buffer_count, unsigned long samples,
   unsigned long freq, ALLEGRO_AUDIO_DEPTH depth,
   ALLEGRO_CHANNEL_CONF chan_conf)
{
   ALLEGRO_STREAM *stream;
   unsigned long bytes_per_buffer;
   size_t i;

   if (!buffer_count) {
      _al_set_error(ALLEGRO_INVALID_PARAM,
         "Attempted to create stream with no buffers");
      return NULL;
   }
   if (!samples) {
      _al_set_error(ALLEGRO_INVALID_PARAM,
          "Attempted to create stream with no buffer size");
      return NULL;
   }
   if (!freq) {
      _al_set_error(ALLEGRO_INVALID_PARAM,
         "Attempted to create stream with no frequency");
      return NULL;
   }

   bytes_per_buffer  = samples;
   bytes_per_buffer *= (chan_conf>>4) + (chan_conf&0xF);
   if ((depth & ~ALLEGRO_AUDIO_DEPTH_UNSIGNED) == ALLEGRO_AUDIO_DEPTH_INT8) {
      bytes_per_buffer *= sizeof(int8_t);
   }
   else if ((depth & ~ALLEGRO_AUDIO_DEPTH_UNSIGNED)
            == ALLEGRO_AUDIO_DEPTH_INT16) {
      bytes_per_buffer *= sizeof(int16_t);
   }
   else if ((depth & ~ALLEGRO_AUDIO_DEPTH_UNSIGNED)
            == ALLEGRO_AUDIO_DEPTH_INT24) {
      bytes_per_buffer *= sizeof(int32_t);
   }
   else if (depth == ALLEGRO_AUDIO_DEPTH_FLOAT32) {
      bytes_per_buffer *= sizeof(float);
   }
   else {
      return NULL;
   }

   stream = calloc(1, sizeof(*stream));
   if (!stream) {
      _al_set_error(ALLEGRO_GENERIC_ERROR,
         "Out of memory allocating stream object");
      return NULL;
   }

   stream->spl.is_playing = true;
   stream->spl.is_stream = true;

   stream->spl.loop      = ALLEGRO_PLAYMODE_ONCE;
   stream->spl.depth     = depth;
   stream->spl.chan_conf = chan_conf;
   stream->spl.frequency = freq;
   stream->spl.speed     = 1.0f;

   stream->spl.step = 0;
   stream->spl.pos  = samples<<MIXER_FRAC_SHIFT;
   stream->spl.len  = stream->spl.pos;

   stream->buf_count = buffer_count;

   stream->used_bufs = calloc(1, buffer_count*sizeof(void*)*2);
   if (!stream->used_bufs) {
      free(stream->used_bufs);
      free(stream);
      _al_set_error(ALLEGRO_GENERIC_ERROR,
         "Out of memory allocating stream buffer pointers");
      return NULL;
   }
   stream->pending_bufs = stream->used_bufs + buffer_count;

   stream->main_buffer = calloc(1, bytes_per_buffer * buffer_count);
   if (!stream->main_buffer) {
      free(stream->used_bufs);
      free(stream);
      _al_set_error(ALLEGRO_GENERIC_ERROR,
         "Out of memory allocating stream buffer");
      return NULL;
   }

   for (i = 0; i < buffer_count; i++) {
      stream->pending_bufs[i] =
         (char *) stream->main_buffer + i * bytes_per_buffer;
   }

   return stream;
}


/* Function: al_stream_destroy
 */
void al_stream_destroy(ALLEGRO_STREAM *stream)
{
   if (stream) {
      _al_kcm_detach_from_parent(&stream->spl);
      free(stream->main_buffer);
      free(stream->used_bufs);
      free(stream);
   }
}


/* Function: al_stream_get_long
 */
int al_stream_get_long(const ALLEGRO_STREAM *stream,
   ALLEGRO_AUDIO_PROPERTY setting, unsigned long *val)
{
   ASSERT(stream);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_FREQUENCY:
         *val = stream->spl.frequency;
         return 0;

      case ALLEGRO_AUDIOPROP_LENGTH:
         *val = stream->spl.len>>MIXER_FRAC_SHIFT;
         return 0;

      case ALLEGRO_AUDIOPROP_FRAGMENTS:
         *val = stream->buf_count;
         return 0;

      case ALLEGRO_AUDIOPROP_USED_FRAGMENTS: {
         size_t i;
         for (i = 0; stream->used_bufs[i] && i < stream->buf_count; i++)
            ;
         *val = i;
         return 0;
      }

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to get invalid stream long setting");
         return 1;
   }
}


/* Function: al_stream_get_float
 */
int al_stream_get_float(const ALLEGRO_STREAM *stream,
   ALLEGRO_AUDIO_PROPERTY setting, float *val)
{
   ASSERT(stream);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_SPEED:
         *val = stream->spl.speed;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to get invalid stream float setting");
         return 1;
   }
}


/* Function: al_stream_get_enum
 */
int al_stream_get_enum(const ALLEGRO_STREAM *stream,
   ALLEGRO_AUDIO_PROPERTY setting, int *val)
{
   ASSERT(stream);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_DEPTH:
         *val = stream->spl.depth;
         return 0;

      case ALLEGRO_AUDIOPROP_CHANNELS:
         *val = stream->spl.chan_conf;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to get invalid stream enum setting");
         return 1;
   }
}


/* Function: al_stream_get_bool
 */
int al_stream_get_bool(const ALLEGRO_STREAM *stream,
   ALLEGRO_AUDIO_PROPERTY setting, bool *val)
{
   ASSERT(stream);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_PLAYING:
         *val = stream->spl.is_playing;
         return 0;

      case ALLEGRO_AUDIOPROP_ATTACHED:
         *val = (stream->spl.parent.u.ptr != NULL);
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to get invalid stream bool setting");
         return 1;
   }
}


/* Function: al_stream_get_ptr
 */
int al_stream_get_ptr(const ALLEGRO_STREAM *stream,
   ALLEGRO_AUDIO_PROPERTY setting, void **val)
{
   ASSERT(stream);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_BUFFER: {
         size_t i;

         if (!stream->used_bufs[0]) {
            _al_set_error(ALLEGRO_INVALID_OBJECT,
               "Attempted to get the buffer of a non-waiting stream");
            return 1;
         }
         *val = stream->used_bufs[0];
         for (i = 0; stream->used_bufs[i] && i < stream->buf_count-1; i++) {
            stream->used_bufs[i] = stream->used_bufs[i+1];
         }
         stream->used_bufs[i] = NULL;
         return 0;
      }

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to get invalid stream pointer setting");
         return 1;
   }
}


/* Function: al_stream_set_long
 */
int al_stream_set_long(ALLEGRO_STREAM *stream,
   ALLEGRO_AUDIO_PROPERTY setting, unsigned long val)
{
   ASSERT(stream);

   (void)stream;
   (void)val;

   switch (setting) {
      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to set invalid stream long setting");
         return 1;
   }
}


/* Function: al_stream_set_float
 */
int al_stream_set_float(ALLEGRO_STREAM *stream,
   ALLEGRO_AUDIO_PROPERTY setting, float val)
{
   ASSERT(stream);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_SPEED:
         if (val <= 0.0f) {
            _al_set_error(ALLEGRO_INVALID_PARAM,
               "Attempted to set stream speed to a zero or negative value");
            return 1;
         }

         if (stream->spl.parent.u.ptr && stream->spl.parent.is_voice) {
            _al_set_error(ALLEGRO_GENERIC_ERROR,
               "Could not set voice playback speed");
            return 1;
         }

         stream->spl.speed = val;
         if (stream->spl.parent.u.mixer) {
            ALLEGRO_MIXER *mixer = stream->spl.parent.u.mixer;
            long i;

            al_lock_mutex(stream->spl.mutex);

            /* Make step 1 before applying the freq difference
             * (so we play forward).
             */
            stream->spl.step = 1;

            i = (stream->spl.frequency << MIXER_FRAC_SHIFT) *
               stream->spl.speed / mixer->ss.frequency;

            /* Don't wanna be trapped with a step value of 0. */
            if (i != 0) {
               stream->spl.step *= i;
            }

            al_unlock_mutex(stream->spl.mutex);
         }

         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to set invalid stream float setting");
         return 1;
   }
}


/* Function: al_stream_set_enum
 */
int al_stream_set_enum(ALLEGRO_STREAM *stream,
   ALLEGRO_AUDIO_PROPERTY setting, int val)
{
   ASSERT(stream);

   (void)stream;
   (void)val;

   switch (setting) {
      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to set invalid stream enum setting");
         return 1;
   }
}


/* Function: al_stream_set_bool
 */
int al_stream_set_bool(ALLEGRO_STREAM *stream,
   ALLEGRO_AUDIO_PROPERTY setting, bool val)
{
   ASSERT(stream);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_PLAYING:
         if (stream->spl.parent.u.ptr && stream->spl.parent.is_voice) {
            ALLEGRO_VOICE *voice = stream->spl.parent.u.voice;
            if (al_voice_set_bool(voice, ALLEGRO_AUDIOPROP_PLAYING, val) != 0) {
               return 1;
            }
         }
         stream->spl.is_playing = val;

         if (!val) {
            al_lock_mutex(stream->spl.mutex);
            stream->spl.pos = stream->spl.len;
            al_unlock_mutex(stream->spl.mutex);
         }
         return 0;

      case ALLEGRO_AUDIOPROP_ATTACHED:
         if (val) {
            _al_set_error(ALLEGRO_INVALID_PARAM,
               "Attempted to set stream attachment status true");
            return 1;
         }
         _al_kcm_detach_from_parent(&stream->spl);
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to set invalid stream bool setting");
         return 1;
   }
}


/* Function: al_stream_set_ptr
 */
int al_stream_set_ptr(ALLEGRO_STREAM *stream,
   ALLEGRO_AUDIO_PROPERTY setting, void *val)
{
   ASSERT(stream);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_BUFFER: {
         size_t i;
         int ret;

         al_lock_mutex(stream->spl.mutex);

         for (i = 0; stream->pending_bufs[i] && i < stream->buf_count; i++)
            ;
         if (i < stream->buf_count) {
            stream->pending_bufs[i] = val;
            ret = 0;
         }
         else {
            _al_set_error(ALLEGRO_INVALID_OBJECT,
               "Attempted to set a stream buffer with a full pending list");
            ret = 1;
         }

         al_unlock_mutex(stream->spl.mutex);
         return ret;
      }

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to set invalid stream pointer setting");
         return 1;
   }
}


/* vim: set sts=3 sw=3 et: */
