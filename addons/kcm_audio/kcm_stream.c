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

#define PREFIX_E "kcm_stream ERROR: "
#define PREFIX_I "kcm_stream INFO: "


/* Function: al_create_stream
 *  Creates an audio stream, using the supplied values. The stream will be
 *  set to play by default.
 */
ALLEGRO_STREAM *al_create_stream(size_t buffer_count, unsigned long samples,
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

   bytes_per_buffer = samples * al_get_channel_count(chan_conf) *
      al_get_depth_size(depth);

   stream = calloc(1, sizeof(*stream));
   if (!stream) {
      _al_set_error(ALLEGRO_GENERIC_ERROR,
         "Out of memory allocating stream object");
      return NULL;
   }

   stream->spl.is_playing = true;
   stream->is_draining = false;

   stream->spl.loop      = _ALLEGRO_PLAYMODE_STREAM_ONCE;
   stream->spl.spl_data.depth     = depth;
   stream->spl.spl_data.chan_conf = chan_conf;
   stream->spl.spl_data.frequency = freq;
   stream->spl.speed     = 1.0f;
   stream->spl.gain      = 1.0f;

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

   stream->spl.es = al_create_user_event_source();

   _al_kcm_register_destructor(stream, (void (*)(void *)) al_destroy_stream);

   return stream;
}


/* Function: al_destroy_stream
 */
void al_destroy_stream(ALLEGRO_STREAM *stream)
{
   if (stream) {
      _al_kcm_unregister_destructor(stream);

      _al_kcm_detach_from_parent(&stream->spl);
      if (stream->feed_thread) {
         stream->unload_feeder(stream);
      }
      al_destroy_user_event_source(stream->spl.es);
      free(stream->main_buffer);
      free(stream->used_bufs);
      free(stream);
   }
}


/* Function: al_drain_stream
 * Called by the user if sample data is not going to be passed to the stream
 * any longer. This function waits for all pending buffers to finish playing.
 * Stream's playing state will change to false.
 */
void al_drain_stream(ALLEGRO_STREAM *stream)
{
   bool playing;
   stream->is_draining = true;
   do {
      al_rest(0.01);
      al_get_stream_bool(stream, ALLEGRO_AUDIOPROP_PLAYING, &playing);
   } while (playing);
   stream->is_draining = false;
}


/* Function: al_get_stream_long
 */
int al_get_stream_long(const ALLEGRO_STREAM *stream,
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


/* Function: al_get_stream_float
 */
int al_get_stream_float(const ALLEGRO_STREAM *stream,
   ALLEGRO_AUDIO_PROPERTY setting, float *val)
{
   ASSERT(stream);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_SPEED:
         *val = stream->spl.speed;
         return 0;

      case ALLEGRO_AUDIOPROP_GAIN:
         *val = stream->spl.gain;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to get invalid stream float setting");
         return 1;
   }
}


/* Function: al_get_stream_enum
 */
int al_get_stream_enum(const ALLEGRO_STREAM *stream,
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

      case ALLEGRO_AUDIOPROP_LOOPMODE:
         *val = stream->spl.loop;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to get invalid stream enum setting");
         return 1;
   }
}


/* Function: al_get_stream_bool
 */
int al_get_stream_bool(const ALLEGRO_STREAM *stream,
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


/* Function: al_get_stream_ptr
 */
int al_get_stream_ptr(const ALLEGRO_STREAM *stream,
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


/* Function: al_set_stream_long
 */
int al_set_stream_long(ALLEGRO_STREAM *stream,
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


/* Function: al_set_stream_float
 */
int al_set_stream_float(ALLEGRO_STREAM *stream,
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

      case ALLEGRO_AUDIOPROP_GAIN:
         if (stream->spl.parent.u.ptr && stream->spl.parent.is_voice) {
            _al_set_error(ALLEGRO_GENERIC_ERROR,
               "Could not set gain of stream attached to voice");
            return 1;
         }

         if (stream->spl.gain == val) {
            return 0;
         }
         stream->spl.gain = val;

         /* If attached to a mixer already, need to recompute the sample
          * matrix to take into account the gain.
          */
         if (stream->spl.parent.u.mixer) {
            ALLEGRO_MIXER *mixer = stream->spl.parent.u.mixer;

            al_lock_mutex(stream->spl.mutex);
            _al_kcm_mixer_rejig_sample_matrix(mixer, &stream->spl);
            al_unlock_mutex(stream->spl.mutex);
         }

         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to set invalid stream float setting");
         return 1;
   }
}


/* Function: al_set_stream_enum
 */
int al_set_stream_enum(ALLEGRO_STREAM *stream,
   ALLEGRO_AUDIO_PROPERTY setting, int val)
{
   ASSERT(stream);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_LOOPMODE:
         if (val == ALLEGRO_PLAYMODE_ONCE) {
            stream->spl.loop = _ALLEGRO_PLAYMODE_STREAM_ONCE;
            return 0;
         }
         else if (val == ALLEGRO_PLAYMODE_LOOP) {
            /* Only streams creating by al_stream_from_file() support
             * looping. */
            if (!stream->feeder)
               return 1;

            stream->spl.loop = _ALLEGRO_PLAYMODE_STREAM_ONEDIR;
            return 0;
         }

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to set invalid stream enum setting");
         return 1;
   }
}


/* Function: al_set_stream_bool
 */
int al_set_stream_bool(ALLEGRO_STREAM *stream,
   ALLEGRO_AUDIO_PROPERTY setting, bool val)
{
   ASSERT(stream);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_PLAYING:
         if (stream->spl.parent.u.ptr && stream->spl.parent.is_voice) {
            ALLEGRO_VOICE *voice = stream->spl.parent.u.voice;
            if (al_set_voice_bool(voice, ALLEGRO_AUDIOPROP_PLAYING, val) != 0) {
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


/* Function: al_set_stream_ptr
 */
int al_set_stream_ptr(ALLEGRO_STREAM *stream,
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
   ALLEGRO_EVENT_QUEUE *queue;
   (void)self;

   TRACE(PREFIX_I "Stream feeder thread started.\n");

   queue = al_create_event_queue();
   al_register_event_source(queue, stream->spl.es);

   stream->quit_feed_thread = false;

   while (!stream->quit_feed_thread) {
      void *fragment_void;
      char *fragment;
      ALLEGRO_EVENT event;

      al_wait_for_event(queue, &event);

      if (event.type == ALLEGRO_EVENT_STREAM_EMPTY_FRAGMENT
          && !stream->is_draining) {
         unsigned long bytes;
         unsigned long bytes_written;

         if (al_get_stream_ptr(stream, ALLEGRO_AUDIOPROP_BUFFER,
            &fragment_void) != 0) {
            TRACE(PREFIX_E "Error getting stream buffer.\n");
            continue;
         }
         fragment = fragment_void;

         bytes = (stream->spl.spl_data.len >> MIXER_FRAC_SHIFT) *
               al_get_channel_count(stream->spl.spl_data.chan_conf) *
               al_get_depth_size(stream->spl.spl_data.depth);

         al_lock_mutex(stream->spl.mutex);
         bytes_written = stream->feeder(stream, fragment, bytes);
         al_unlock_mutex(stream->spl.mutex);

        /* In case it reaches the end of the stream source, stream feeder will
         * fill the remaining space with silence. If we should loop, rewind the
         * stream and override the silence with the beginning. */
         if (bytes_written != bytes &&
            stream->spl.loop == _ALLEGRO_PLAYMODE_STREAM_ONEDIR) {
            size_t bw;
            al_rewind_stream(stream);
            al_lock_mutex(stream->spl.mutex);
            bw = stream->feeder(stream, fragment + bytes_written,
               bytes - bytes_written);
            al_unlock_mutex(stream->spl.mutex);
            ASSERT(bw == bytes - bytes_written);
         }

         if (al_set_stream_ptr(stream, ALLEGRO_AUDIOPROP_BUFFER,
            fragment) != 0) {
            TRACE(PREFIX_E "Error setting stream buffer.\n");
            continue;
         }

         /* The streaming source doesn't feed any more, drain buffers and quit. */
         if (bytes_written != bytes &&
            stream->spl.loop == _ALLEGRO_PLAYMODE_STREAM_ONCE) {
            al_drain_stream(stream);
            stream->quit_feed_thread = true;
         }
      }
      else if (event.type == _KCM_STREAM_FEEDER_QUIT_EVENT_TYPE) {
         stream->quit_feed_thread = true;
      }
   }

   al_destroy_event_queue(queue);

   TRACE(PREFIX_I "Stream feeder thread finished.\n");

   return NULL;
}


bool _al_kcm_emit_stream_event(ALLEGRO_STREAM *stream, unsigned long count)
{
   while (count--) {
      ALLEGRO_EVENT event;
      event.user.type = ALLEGRO_EVENT_STREAM_EMPTY_FRAGMENT;
      event.user.timestamp = al_current_time();
      al_emit_user_event(stream->spl.es, &event, NULL);
   }

   return true;
}


/* Function: al_rewind_stream
 * Set the streaming file playing position to the beginning. Returns true on
 * success. Currently this can only be called on streams created with acodec's
 * al_stream_from_file().
 */
bool al_rewind_stream(ALLEGRO_STREAM *stream)
{
   bool ret;
   if (stream->rewind_feeder) {
      al_lock_mutex(stream->spl.mutex);
      ret = stream->rewind_feeder(stream);
      al_unlock_mutex(stream->spl.mutex);
      return ret;
   }

   return false;
}

/* Function: al_seek_stream
 * Set the streaming file playing position to time. Returns true on success.
 * Currently this can only be called on streams created with acodec's
 * al_stream_from_file().
 */
bool al_seek_stream(ALLEGRO_STREAM *stream, double time)
{
   bool ret;
   if (stream->seek_feeder) {
      al_lock_mutex(stream->spl.mutex);
      ret = stream->seek_feeder(stream, time);
      al_unlock_mutex(stream->spl.mutex);
      return ret;
   }

   return false;
}

/* Function: al_get_stream_position
 * Return the position of the stream in seconds.
 * Currently this can only be called on streams created with acodec's
 * al_stream_from_file().
 */
double al_get_stream_position(ALLEGRO_STREAM *stream)
{
   double ret;
   if (stream->get_feeder_position) {
      al_lock_mutex(stream->spl.mutex);
      ret = stream->get_feeder_position(stream);
      al_unlock_mutex(stream->spl.mutex);
      return ret;
   }

   return 0.0;
}

/* Function: al_get_stream_length
 * Return the position of the stream in seconds.
 * Currently this can only be called on streams created with acodec's
 * al_stream_from_file().
 */
double al_get_stream_length(ALLEGRO_STREAM *stream)
{
   double ret;
   if (stream->get_feeder_length) {
      al_lock_mutex(stream->spl.mutex);
      ret = stream->get_feeder_length(stream);
      al_unlock_mutex(stream->spl.mutex);
      return ret;
   }
   return 0.0;
}

/* Function: al_set_stream_loop
 * Return the position of the stream in seconds.
 * Currently this can only be called on streams created with acodec's
 * al_stream_from_file().
 */
bool al_set_stream_loop(ALLEGRO_STREAM *stream, double start, double end)
{
   bool ret;

   if (start >= end)
      return false;

   if (stream->set_feeder_loop) {
      al_lock_mutex(stream->spl.mutex);
      ret = stream->set_feeder_loop(stream, start, end);
      al_unlock_mutex(stream->spl.mutex);
      return ret;
   }

   return false;
}

/* vim: set sts=3 sw=3 et: */

