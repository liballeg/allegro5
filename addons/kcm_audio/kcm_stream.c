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

   bytes_per_buffer = samples * al_channel_count(chan_conf) *
      al_depth_size(depth);

   stream = calloc(1, sizeof(*stream));
   if (!stream) {
      _al_set_error(ALLEGRO_GENERIC_ERROR,
         "Out of memory allocating stream object");
      return NULL;
   }

   stream->spl.is_playing = true;
   stream->drained = false;

   stream->spl.loop      = _ALLEGRO_PLAYMODE_STREAM;
   stream->spl.spl_data.depth     = depth;
   stream->spl.spl_data.chan_conf = chan_conf;
   stream->spl.spl_data.frequency = freq;
   stream->spl.speed     = 1.0f;

   stream->spl.step = 0;
   stream->spl.pos  = samples << MIXER_FRAC_SHIFT;
   stream->spl.spl_data.len  = stream->spl.pos;

   stream->buf_count = buffer_count;

   stream->used_bufs = calloc(1, buffer_count * sizeof(void *) * 2);
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

   _al_event_source_init(&stream->spl.es);

   return stream;
}


/* Function: al_stream_destroy
 */
void al_stream_destroy(ALLEGRO_STREAM *stream)
{
   if (stream) {
      _al_event_source_free(&stream->spl.es);
      _al_kcm_detach_from_parent(&stream->spl);
      free(stream->main_buffer);
      free(stream->used_bufs);
      free(stream);
   }
}


/* Function: al_stream_drain
 * Called by the user if sample data is not going to be passed to the stream
 * any longer. This function waits for all pending buffers to finish playing.
 * Stream's playing state will change to false.
 */
void al_stream_drain(ALLEGRO_STREAM *stream)
{
   bool playing;
   stream->drained = true;
   do {
      al_rest(0.01);
      al_stream_get_bool(stream, ALLEGRO_AUDIOPROP_PLAYING, &playing);
   } while (playing);
}


/* Function: al_stream_get_long
 */
int al_stream_get_long(const ALLEGRO_STREAM *stream,
   ALLEGRO_AUDIO_PROPERTY setting, unsigned long *val)
{
   ASSERT(stream);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_FREQUENCY:
         *val = stream->spl.spl_data.frequency;
         return 0;

      case ALLEGRO_AUDIOPROP_LENGTH:
         *val = stream->spl.spl_data.len >> MIXER_FRAC_SHIFT;
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
         *val = stream->spl.spl_data.depth;
         return 0;

      case ALLEGRO_AUDIOPROP_CHANNELS:
         *val = stream->spl.spl_data.chan_conf;
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

            i = (stream->spl.spl_data.frequency << MIXER_FRAC_SHIFT) *
               stream->spl.speed / mixer->ss.spl_data.frequency;

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
            stream->spl.pos = stream->spl.spl_data.len;
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


/* _al_kcm_refill_stream:
 *  Called by the mixer when the current buffer has been used up.  It should
 *  point to the next pending buffer and reset the sample position.
 *  Returns true if the next buffer is available and set up.
 *  Otherwise returns false.
 */
bool _al_kcm_refill_stream(ALLEGRO_STREAM *stream)
{
   void *old_buf = stream->spl.spl_data.buffer.ptr;
   size_t i;

   if (old_buf) {
      /* Slide the buffers down one position and put the
       * completed buffer into the used array to be refilled.
       */
      for (i = 0;
            stream->pending_bufs[i] && i < stream->buf_count-1;
            i++) {
         stream->pending_bufs[i] = stream->pending_bufs[i+1];
      }
      stream->pending_bufs[i] = NULL;

      for (i = 0; stream->used_bufs[i]; i++)
         ;
      stream->used_bufs[i] = old_buf;
   }

   stream->spl.spl_data.buffer.ptr = stream->pending_bufs[0];
   if (!stream->spl.spl_data.buffer.ptr)
      return false;

   stream->spl.pos = 0;

   return true;
}


/* _al_kcm_feed_stream:
 * A routine running in another thread that feeds the stream buffers as
 * neccesary, usually getting data from some acodec file reader backend.
 */
void *_al_kcm_feed_stream(ALLEGRO_THREAD *self, void *vstream)
{
   ALLEGRO_STREAM *stream = vstream;

   while (!stream->quit_feed_thread) {
      void *vbuf;
      unsigned long vbuf_waiting_count;
      unsigned long bytes;
      bool is_dry;

      if (al_stream_get_long(stream, ALLEGRO_AUDIOPROP_USED_FRAGMENTS,
                             &vbuf_waiting_count) != 0) {
         TRACE("Error getting the number of waiting buffers.\n");
         return NULL;
      }

      if (vbuf_waiting_count == 0) {
         al_rest(0.05); /* Precalculate some optimal value? */
         continue;
      }

      if (al_stream_get_ptr(stream, ALLEGRO_AUDIOPROP_BUFFER, &vbuf) != 0) {
         TRACE("Error getting the stream buffers.\n");
         return NULL;
      }

      bytes = (stream->spl.spl_data.len >> MIXER_FRAC_SHIFT) *
              al_channel_count(stream->spl.spl_data.chan_conf) *
              al_depth_size(stream->spl.spl_data.depth);

      is_dry = stream->feeder(stream, vbuf, bytes);

      if (al_stream_set_ptr(stream, ALLEGRO_AUDIOPROP_BUFFER, vbuf) != 0) {
         TRACE("Error setting stream buffer.\n");
         return NULL;
      }

      if (is_dry) {
         al_stream_drain(stream);
         return NULL;
      }
   }

   return NULL;
}


bool _al_kcm_emit_stream_event(ALLEGRO_STREAM *stream, bool is_dry, unsigned long count)
{
   _al_event_source_lock(&stream->spl.es);

   if (_al_event_source_needs_to_generate_event(&stream->spl.es)) {
      while (count--) {
         ALLEGRO_EVENT *event = _al_event_source_get_unused_event(&stream->spl.es);
         if (event) {
            event->stream.type = ALLEGRO_EVENT_STREAM_EMPTY_FRAGMENT;
            event->stream.timestamp = al_current_time();
            al_stream_get_ptr(stream, ALLEGRO_AUDIOPROP_BUFFER,
                              &event->stream.empty_fragment);
            ASSERT(event->stream.empty_fragment);
            event->stream.is_dry = is_dry;
            _al_event_source_emit_event(&stream->spl.es, event);
         }
      }
   }

   _al_event_source_unlock(&stream->spl.es);

   return true;
}

/* vim: set sts=3 sw=3 et: */
