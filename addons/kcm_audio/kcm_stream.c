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

ALLEGRO_DEBUG_CHANNEL("sound")

/* Function: al_create_stream
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
   stream->spl.pan       = 0.0f;

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
 */
void al_drain_stream(ALLEGRO_STREAM *stream)
{
   bool playing;
   stream->is_draining = true;
   do {
      al_rest(0.01);
      playing = al_get_stream_playing(stream);
   } while (playing);
   stream->is_draining = false;
}


/* Function: al_get_stream_frequency
 */
unsigned int al_get_stream_frequency(const ALLEGRO_STREAM *stream)
{
   ASSERT(stream);

   // XXX long not needed
   return stream->spl.spl_data.frequency;
}


/* Function: al_get_stream_length
 */
unsigned long al_get_stream_length(const ALLEGRO_STREAM *stream)
{
   ASSERT(stream);

   // XXX long not needed?
   return stream->spl.spl_data.len >> MIXER_FRAC_SHIFT;
}


/* Function: al_get_stream_fragments
 */
unsigned int al_get_stream_fragments(const ALLEGRO_STREAM *stream)
{
   ASSERT(stream);

   return stream->buf_count;
}


/* Function: al_get_stream_used_fragments
 */
unsigned int al_get_stream_used_fragments(const ALLEGRO_STREAM *stream)
{
   unsigned int i;
   ASSERT(stream);

   for (i = 0; stream->used_bufs[i] && i < stream->buf_count; i++)
      ;
   return i;
}


/* Function: al_get_stream_speed
 */
float al_get_stream_speed(const ALLEGRO_STREAM *stream)
{
   ASSERT(stream);

   return stream->spl.speed;
}


/* Function: al_get_stream_gain
 */
float al_get_stream_gain(const ALLEGRO_STREAM *stream)
{
   ASSERT(stream);

   return stream->spl.gain;
}


/* Function: al_get_stream_pan
 */
float al_get_stream_pan(const ALLEGRO_STREAM *stream)
{
   ASSERT(stream);

   return stream->spl.pan;
}


/* Function: al_get_stream_channels
 */
ALLEGRO_CHANNEL_CONF al_get_stream_channels(const ALLEGRO_STREAM *stream)
{
   ASSERT(stream);

   return stream->spl.spl_data.chan_conf;
}


/* Function: al_get_stream_depth
 */
ALLEGRO_AUDIO_DEPTH al_get_stream_depth(const ALLEGRO_STREAM *stream)
{
   ASSERT(stream);

   return stream->spl.spl_data.depth;
}


/* Function: al_get_stream_playmode
 */
ALLEGRO_PLAYMODE al_get_stream_playmode(const ALLEGRO_STREAM *stream)
{
   ASSERT(stream);

   return stream->spl.loop;
}


/* Function: al_get_stream_playing
*/
bool al_get_stream_playing(const ALLEGRO_STREAM *stream)
{
   ASSERT(stream);

   return stream->spl.is_playing;
}


/* Function: al_get_stream_attached
*/
bool al_get_stream_attached(const ALLEGRO_STREAM *stream)
{
   ASSERT(stream);

   return (stream->spl.parent.u.ptr != NULL);
}


/* Function: al_get_stream_buffer
*/
bool al_get_stream_buffer(const ALLEGRO_STREAM *stream, void **val)
{
   size_t i;
   ASSERT(stream);

   if (!stream->used_bufs[0]) {
      _al_set_error(ALLEGRO_INVALID_OBJECT,
         "Attempted to get the buffer of a non-waiting stream");
      return false;
   }

   *val = stream->used_bufs[0];
   for (i = 0; stream->used_bufs[i] && i < stream->buf_count-1; i++) {
      stream->used_bufs[i] = stream->used_bufs[i+1];
   }
   stream->used_bufs[i] = NULL;
   return true;
}


/* Function: al_set_stream_speed
 */
bool al_set_stream_speed(ALLEGRO_STREAM *stream, float val)
{
   ASSERT(stream);

   if (val <= 0.0f) {
      _al_set_error(ALLEGRO_INVALID_PARAM,
         "Attempted to set stream speed to a zero or negative value");
      return false;
   }

   if (stream->spl.parent.u.ptr && stream->spl.parent.is_voice) {
      _al_set_error(ALLEGRO_GENERIC_ERROR,
         "Could not set voice playback speed");
      return false;
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

   return true;
}


/* Function: al_set_stream_gain
 */
bool al_set_stream_gain(ALLEGRO_STREAM *stream, float val)
{
   ASSERT(stream);

   if (stream->spl.parent.u.ptr && stream->spl.parent.is_voice) {
      _al_set_error(ALLEGRO_GENERIC_ERROR,
         "Could not set gain of stream attached to voice");
      return false;
   }

   if (stream->spl.gain != val) {
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
   }

   return true;
}


/* Function: al_set_stream_pan
 */
bool al_set_stream_pan(ALLEGRO_STREAM *stream, float val)
{
   ASSERT(stream);

   if (stream->spl.parent.u.ptr && stream->spl.parent.is_voice) {
      _al_set_error(ALLEGRO_GENERIC_ERROR,
         "Could not set gain of stream attached to voice");
      return false;
   }
   if (val != ALLEGRO_AUDIO_PAN_NONE && (val < -1.0 || val > 1.0)) {
      _al_set_error(ALLEGRO_GENERIC_ERROR, "Invalid pan value");
      return false;
   }

   if (stream->spl.pan != val) {
      stream->spl.pan = val;

      /* If attached to a mixer already, need to recompute the sample
       * matrix to take into account the panning.
       */
      if (stream->spl.parent.u.mixer) {
         ALLEGRO_MIXER *mixer = stream->spl.parent.u.mixer;

         al_lock_mutex(stream->spl.mutex);
         _al_kcm_mixer_rejig_sample_matrix(mixer, &stream->spl);
         al_unlock_mutex(stream->spl.mutex);
      }
   }

   return true;
}


/* Function: al_set_stream_playmode
 */
bool al_set_stream_playmode(ALLEGRO_STREAM *stream, ALLEGRO_PLAYMODE val)
{
   ASSERT(stream);

   if (val == ALLEGRO_PLAYMODE_ONCE) {
      stream->spl.loop = _ALLEGRO_PLAYMODE_STREAM_ONCE;
      return true;
   }
   else if (val == ALLEGRO_PLAYMODE_LOOP) {
      /* Only streams creating by al_stream_from_file() support
       * looping. */
      if (!stream->feeder)
         return false;

      stream->spl.loop = _ALLEGRO_PLAYMODE_STREAM_ONEDIR;
      return true;
   }

   // XXX _al_set_error
   return false;
}


/* Function: al_set_stream_playing
 */
bool al_set_stream_playing(ALLEGRO_STREAM *stream, bool val)
{
   ASSERT(stream);

   if (stream->spl.parent.u.ptr && stream->spl.parent.is_voice) {
      ALLEGRO_VOICE *voice = stream->spl.parent.u.voice;
      if (!al_set_voice_playing(voice, val)) {
         return false;
      }
   }
   stream->spl.is_playing = val;

   if (!val) {
      al_lock_mutex(stream->spl.mutex);
      stream->spl.pos = stream->spl.spl_data.len;
      al_unlock_mutex(stream->spl.mutex);
   }
   return true;
}


/* Function: al_detach_stream
 */
bool al_detach_stream(ALLEGRO_STREAM *stream)
{
   ASSERT(stream);

   _al_kcm_detach_from_parent(&stream->spl);
   return true;
}


/* Function: al_set_stream_buffer
 */
bool al_set_stream_buffer(ALLEGRO_STREAM *stream, void *val)
{
   size_t i;
   bool ret;
   ASSERT(stream);

   al_lock_mutex(stream->spl.mutex);

   for (i = 0; stream->pending_bufs[i] && i < stream->buf_count; i++)
      ;
   if (i < stream->buf_count) {
      stream->pending_bufs[i] = val;
      ret = true;
   }
   else {
      _al_set_error(ALLEGRO_INVALID_OBJECT,
         "Attempted to set a stream buffer with a full pending list");
      ret = false;
   }

   al_unlock_mutex(stream->spl.mutex);
   return ret;
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

   ALLEGRO_DEBUG("Stream feeder thread started.\n");

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

         if (!al_get_stream_buffer(stream, &fragment_void)) {
            ALLEGRO_ERROR("Error getting stream buffer.\n");
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

         if (!al_set_stream_buffer(stream, fragment)) {
            ALLEGRO_ERROR("Error setting stream buffer.\n");
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

   ALLEGRO_DEBUG("Stream feeder thread finished.\n");

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


/* Function: al_seek_stream_secs
 */
bool al_seek_stream_secs(ALLEGRO_STREAM *stream, double time)
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


/* Function: al_get_stream_position_secs
 */
double al_get_stream_position_secs(ALLEGRO_STREAM *stream)
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


/* Function: al_get_stream_length_secs
 */
double al_get_stream_length_secs(ALLEGRO_STREAM *stream)
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


/* Function: al_set_stream_loop_secs
 */
bool al_set_stream_loop_secs(ALLEGRO_STREAM *stream, double start, double end)
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
