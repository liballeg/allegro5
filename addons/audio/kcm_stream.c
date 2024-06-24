/**
 * Originally digi.c from allegro wiki
 * Original authors: KC/Milan
 *
 * Converted to allegro5 by Ryan Dickie
 */

/* Title: Stream functions
 */

#include <stdio.h>

#include "allegro5/allegro_audio.h"
#include "allegro5/internal/aintern_audio.h"
#include "allegro5/internal/aintern_audio_cfg.h"

ALLEGRO_DEBUG_CHANNEL("audio")

/*
 * The highest quality interpolator is a cubic interpolator requiring
 * four sample points.  In the streaming case we lag the true sample
 * position by three.
 */
#define MAX_LAG   (3)


/*
 * To avoid deadlocks, unlock the mutex returned by this function, rather than
 * whatever you passed as the argument.
 */
static ALLEGRO_MUTEX *maybe_lock_mutex(ALLEGRO_MUTEX *mutex)
{
   if (mutex) {
      al_lock_mutex(mutex);
   }
   return mutex;
}


static void maybe_unlock_mutex(ALLEGRO_MUTEX *mutex)
{
   if (mutex) {
      al_unlock_mutex(mutex);
   }
}

/* Function: al_create_audio_stream
 */
ALLEGRO_AUDIO_STREAM *al_create_audio_stream(size_t fragment_count,
   unsigned int frag_samples, unsigned int freq, ALLEGRO_AUDIO_DEPTH depth,
   ALLEGRO_CHANNEL_CONF chan_conf)
{
   ALLEGRO_AUDIO_STREAM *stream;
   unsigned long bytes_per_sample;
   unsigned long bytes_per_frag_buf;
   size_t i;

   if (!fragment_count) {
      _al_set_error(ALLEGRO_INVALID_PARAM,
         "Attempted to create stream with no buffers");
      return NULL;
   }
   if (!frag_samples) {
      _al_set_error(ALLEGRO_INVALID_PARAM,
          "Attempted to create stream with no buffer size");
      return NULL;
   }
   if (!freq) {
      _al_set_error(ALLEGRO_INVALID_PARAM,
         "Attempted to create stream with no frequency");
      return NULL;
   }

   bytes_per_sample = al_get_channel_count(chan_conf) *
         al_get_audio_depth_size(depth);
   bytes_per_frag_buf = frag_samples * bytes_per_sample;

   stream = al_calloc(1, sizeof(*stream));
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
   stream->spl.pos  = frag_samples;
   stream->spl.spl_data.len  = stream->spl.pos;

   stream->buf_count = fragment_count;

   stream->used_bufs = al_calloc(1, fragment_count * sizeof(void *) * 2);
   if (!stream->used_bufs) {
      al_free(stream->used_bufs);
      al_free(stream);
      _al_set_error(ALLEGRO_GENERIC_ERROR,
         "Out of memory allocating stream buffer pointers");
      return NULL;
   }
   stream->pending_bufs = stream->used_bufs + fragment_count;

   /* The main_buffer holds all the buffer fragments in contiguous memory.
    * To support interpolation across buffer fragments, we allocate extra
    * MAX_LAG samples at the start of each buffer fragment, to hold the
    * last few sample values which came before that fragment.
    */
   stream->main_buffer = al_calloc(1,
      (MAX_LAG * bytes_per_sample + bytes_per_frag_buf) * fragment_count);
   if (!stream->main_buffer) {
      al_free(stream->used_bufs);
      al_free(stream);
      _al_set_error(ALLEGRO_GENERIC_ERROR,
         "Out of memory allocating stream buffer");
      return NULL;
   }

   for (i = 0; i < fragment_count; i++) {
      char *buffer = (char *)stream->main_buffer
         + i * (MAX_LAG * bytes_per_sample + bytes_per_frag_buf);
      al_fill_silence(buffer, MAX_LAG, depth, chan_conf);
      stream->used_bufs[i] = buffer + MAX_LAG * bytes_per_sample;
   }

   al_init_user_event_source(&stream->spl.es);

   /* This can lead to deadlocks on shutdown, hence we don't do it. */
   /* stream->dtor_item = _al_kcm_register_destructor(stream, (void (*)(void *)) al_destroy_audio_stream); */

   return stream;
}


/* Function: al_destroy_audio_stream
 */
void al_destroy_audio_stream(ALLEGRO_AUDIO_STREAM *stream)
{
   if (stream) {
      if (stream->feed_thread) {
         stream->unload_feeder(stream);
      }
      /* See commented out call to _al_kcm_register_destructor. */
      /* _al_kcm_unregister_destructor(stream->dtor_item); */
      _al_kcm_detach_from_parent(&stream->spl);

      al_destroy_user_event_source(&stream->spl.es);
      al_free(stream->main_buffer);
      al_free(stream->used_bufs);
      al_free(stream);
   }
}


/* Function: al_drain_audio_stream
 */
void al_drain_audio_stream(ALLEGRO_AUDIO_STREAM *stream)
{
   bool playing;

   if (!al_get_audio_stream_attached(stream)) {
      al_set_audio_stream_playing(stream, false);
      return;
   }

   stream->is_draining = true;
   do {
      al_rest(0.01);
      playing = al_get_audio_stream_playing(stream);
   } while (playing);
}


/* Function: al_get_audio_stream_frequency
 */
unsigned int al_get_audio_stream_frequency(const ALLEGRO_AUDIO_STREAM *stream)
{
   ASSERT(stream);

   return stream->spl.spl_data.frequency;
}


/* Function: al_get_audio_stream_length
 */
unsigned int al_get_audio_stream_length(const ALLEGRO_AUDIO_STREAM *stream)
{
   ASSERT(stream);

   return stream->spl.spl_data.len;
}


/* Function: al_get_audio_stream_fragments
 */
unsigned int al_get_audio_stream_fragments(const ALLEGRO_AUDIO_STREAM *stream)
{
   ASSERT(stream);

   return stream->buf_count;
}


/* Function: al_get_available_audio_stream_fragments
 */
unsigned int al_get_available_audio_stream_fragments(
   const ALLEGRO_AUDIO_STREAM *stream)
{
   unsigned int i;
   ASSERT(stream);

   for (i = 0; i < stream->buf_count && stream->used_bufs[i]; i++)
      ;
   return i;
}


/* Function: al_get_audio_stream_speed
 */
float al_get_audio_stream_speed(const ALLEGRO_AUDIO_STREAM *stream)
{
   ASSERT(stream);

   return stream->spl.speed;
}


/* Function: al_get_audio_stream_gain
 */
float al_get_audio_stream_gain(const ALLEGRO_AUDIO_STREAM *stream)
{
   ASSERT(stream);

   return stream->spl.gain;
}


/* Function: al_get_audio_stream_pan
 */
float al_get_audio_stream_pan(const ALLEGRO_AUDIO_STREAM *stream)
{
   ASSERT(stream);

   return stream->spl.pan;
}


/* Function: al_get_audio_stream_channels
 */
ALLEGRO_CHANNEL_CONF al_get_audio_stream_channels(
   const ALLEGRO_AUDIO_STREAM *stream)
{
   ASSERT(stream);

   return stream->spl.spl_data.chan_conf;
}


/* Function: al_get_audio_stream_depth
 */
ALLEGRO_AUDIO_DEPTH al_get_audio_stream_depth(
   const ALLEGRO_AUDIO_STREAM *stream)
{
   ASSERT(stream);

   return stream->spl.spl_data.depth;
}


/* Function: al_get_audio_stream_playmode
 */
ALLEGRO_PLAYMODE al_get_audio_stream_playmode(
   const ALLEGRO_AUDIO_STREAM *stream)
{
   ASSERT(stream);

   return stream->spl.loop;
}


/* Function: al_get_audio_stream_playing
*/
bool al_get_audio_stream_playing(const ALLEGRO_AUDIO_STREAM *stream)
{
   ASSERT(stream);

   return stream->spl.is_playing;
}


/* Function: al_get_audio_stream_attached
*/
bool al_get_audio_stream_attached(const ALLEGRO_AUDIO_STREAM *stream)
{
   ASSERT(stream);

   return (stream->spl.parent.u.ptr != NULL);
}

/* Function: al_get_audio_stream_played_samples
*/
uint64_t al_get_audio_stream_played_samples(const ALLEGRO_AUDIO_STREAM *stream)
{
   uint64_t result;
   ALLEGRO_MUTEX *stream_mutex;
   ASSERT(stream);

   stream_mutex = maybe_lock_mutex(stream->spl.mutex);
   if (stream->spl.spl_data.buffer.ptr) {
      result = stream->consumed_fragments * stream->spl.spl_data.len +
         stream->spl.pos;
   }
   else {
      result = 0;
   }
   maybe_unlock_mutex(stream_mutex);

   return result;
}

/* Function: al_get_audio_stream_fragment
*/
void *al_get_audio_stream_fragment(const ALLEGRO_AUDIO_STREAM *stream)
{
   size_t i;
   void *fragment;
   ALLEGRO_MUTEX *stream_mutex;
   ASSERT(stream);

   stream_mutex = maybe_lock_mutex(stream->spl.mutex);

   if (!stream->used_bufs[0]) {
      /* No free fragments are available. */
      fragment = NULL;
   }
   else {
      fragment = stream->used_bufs[0];
      for (i = 0; i < stream->buf_count-1 && stream->used_bufs[i]; i++) {
         stream->used_bufs[i] = stream->used_bufs[i+1];
      }
      stream->used_bufs[i] = NULL;
   }

   maybe_unlock_mutex(stream_mutex);

   return fragment;
}


/* Function: al_set_audio_stream_speed
 */
bool al_set_audio_stream_speed(ALLEGRO_AUDIO_STREAM *stream, float val)
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
      ALLEGRO_MUTEX *stream_mutex = maybe_lock_mutex(stream->spl.mutex);

      stream->spl.step = (stream->spl.spl_data.frequency) * stream->spl.speed;
      stream->spl.step_denom = mixer->ss.spl_data.frequency;
      /* Don't wanna be trapped with a step value of 0 */
      if (stream->spl.step == 0) {
         stream->spl.step = 1;
      }

      maybe_unlock_mutex(stream_mutex);
   }

   return true;
}


/* Function: al_set_audio_stream_gain
 */
bool al_set_audio_stream_gain(ALLEGRO_AUDIO_STREAM *stream, float val)
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
         ALLEGRO_MUTEX *stream_mutex = maybe_lock_mutex(stream->spl.mutex);
         _al_kcm_mixer_rejig_sample_matrix(mixer, &stream->spl);
         maybe_unlock_mutex(stream_mutex);
      }
   }

   return true;
}


/* Function: al_set_audio_stream_pan
 */
bool al_set_audio_stream_pan(ALLEGRO_AUDIO_STREAM *stream, float val)
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
         ALLEGRO_MUTEX *stream_mutex = maybe_lock_mutex(stream->spl.mutex);
         _al_kcm_mixer_rejig_sample_matrix(mixer, &stream->spl);
         maybe_unlock_mutex(stream_mutex);
      }
   }

   return true;
}


/* Function: al_set_audio_stream_playmode
 */
bool al_set_audio_stream_playmode(ALLEGRO_AUDIO_STREAM *stream,
   ALLEGRO_PLAYMODE val)
{
   ASSERT(stream);
   bool ret = false;

   if (val == ALLEGRO_PLAYMODE_ONCE) {
      stream->spl.loop = _ALLEGRO_PLAYMODE_STREAM_ONCE;
      ret = true;
   }
   if (val == ALLEGRO_PLAYMODE_LOOP_ONCE) {
      stream->spl.loop = _ALLEGRO_PLAYMODE_STREAM_LOOP_ONCE;
      ret = true;
   }
   else if (val == ALLEGRO_PLAYMODE_LOOP) {
      /* Only streams creating by al_load_audio_stream() support
       * looping. */
      if (!stream->feeder) {
         ret = false;
      } else {
         stream->spl.loop = _ALLEGRO_PLAYMODE_STREAM_ONEDIR;
         ret = true;
      }
   }
   if (ret) {
      stream->is_draining = false;
   }

   // XXX _al_set_error
   return ret;
}


static void reset_stopped_stream(ALLEGRO_AUDIO_STREAM *stream)
{
   const int bytes_per_sample =
      al_get_channel_count(stream->spl.spl_data.chan_conf) *
      al_get_audio_depth_size(stream->spl.spl_data.depth);
   const int fragment_buffer_size =
      bytes_per_sample * (stream->spl.spl_data.len + MAX_LAG);
   size_t i, n;

   /* Write silence to the "invisible" part in between fragment buffers to
    * avoid interpolation artifacts.  It's tempting to zero the complete
    * memory block in one go but some of the buffers might be getting
    * refilled.  So they are currently "owned" by the library user and
    * should not be overwritten.  But zeroing the parts not visible to the
    * user should be OK.
    */
   for (i = 0; i < stream->buf_count; ++i) {
      al_fill_silence((char *)stream->main_buffer + i * fragment_buffer_size,
         MAX_LAG, stream->spl.spl_data.depth, stream->spl.spl_data.chan_conf);
   }

   /* Get the current number of entries in the used_buf list. */
   for (n = 0; n < stream->buf_count && stream->used_bufs[n]; n++)
      ;

   /* Move everything from pending_bufs to used_bufs. */
   i = 0;
   while (i < stream->buf_count &&
         n < stream->buf_count &&
         stream->pending_bufs[i])
   {
      stream->used_bufs[n] = stream->pending_bufs[i];
      stream->pending_bufs[i] = NULL;
      n++;
      i++;
   }

   /* No fragment buffer is currently playing. */
   stream->spl.spl_data.buffer.ptr = NULL;
   stream->spl.pos = stream->spl.spl_data.len;
   stream->spl.pos_bresenham_error = 0;
   stream->consumed_fragments = 0;
}


/* Function: al_set_audio_stream_playing
 */
bool al_set_audio_stream_playing(ALLEGRO_AUDIO_STREAM *stream, bool val)
{
   bool rc = true;
   ALLEGRO_MUTEX *stream_mutex;
   ASSERT(stream);

   if (stream->spl.parent.u.ptr && stream->spl.parent.is_voice) {
      ALLEGRO_VOICE *voice = stream->spl.parent.u.voice;
      if (val != stream->spl.is_playing) {
         rc = _al_kcm_set_voice_playing(voice, voice->mutex, val);
      }
   }

   stream_mutex = maybe_lock_mutex(stream->spl.mutex);

   stream->spl.is_playing = rc && val;

   if (stream->spl.is_playing) {
      /* We usually emit fragment events when a fragment is used up, but if the
       * stream has zero pending fragments to begin with then no events would
       * ever be emitted.
       */
      _al_kcm_emit_stream_events(stream);
   }
   else if (!val) {
      reset_stopped_stream(stream);
   }

   maybe_unlock_mutex(stream_mutex);

   return rc;
}


/* Function: al_detach_audio_stream
 */
bool al_detach_audio_stream(ALLEGRO_AUDIO_STREAM *stream)
{
   ASSERT(stream);

   _al_kcm_detach_from_parent(&stream->spl);
   ASSERT(stream->spl.spl_read == NULL);
   return !al_get_audio_stream_attached(stream);
}


/* Function: al_set_audio_stream_fragment
 */
bool al_set_audio_stream_fragment(ALLEGRO_AUDIO_STREAM *stream, void *val)
{
   size_t i;
   bool ret;
   ALLEGRO_MUTEX *stream_mutex;
   ASSERT(stream);

   stream_mutex = maybe_lock_mutex(stream->spl.mutex);

   for (i = 0; i < stream->buf_count && stream->pending_bufs[i] ; i++)
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

   maybe_unlock_mutex(stream_mutex);

   return ret;
}


/* _al_kcm_refill_stream:
 *  Called by the mixer when the current buffer has been used up.  It should
 *  point to the next pending buffer and adjust the sample position to reflect
 *  the buffer being updated. It may be necessary to call this function multiple
 *  times if the sample position is so far ahead that multiple buffers need to
 *  be consumed.
 *  Returns true if the next buffer is available and set up.
 *  Otherwise returns false.
 */
bool _al_kcm_refill_stream(ALLEGRO_AUDIO_STREAM *stream)
{
   ALLEGRO_SAMPLE_INSTANCE *spl = &stream->spl;
   void *old_buf = spl->spl_data.buffer.ptr;
   void *new_buf;
   size_t i;
   int new_pos = spl->pos - spl->spl_data.len;

   if (old_buf) {
      /* Slide the buffers down one position and put the
       * completed buffer into the used array to be refilled.
       */
      for (i = 0;
            i < stream->buf_count-1 && stream->pending_bufs[i];
            i++) {
         stream->pending_bufs[i] = stream->pending_bufs[i+1];
      }
      stream->pending_bufs[i] = NULL;

      for (i = 0; stream->used_bufs[i]; i++)
         ;
      stream->used_bufs[i] = old_buf;
   }

   new_buf = stream->pending_bufs[0];
   stream->spl.spl_data.buffer.ptr = new_buf;
   if (!new_buf) {
      ALLEGRO_WARN("Out of buffers\n");
      return false;
   }

   /* Copy the last MAX_LAG sample values to the front of the new buffer
    * for interpolation.
    */
   if (old_buf) {
      const int bytes_per_sample =
         al_get_channel_count(spl->spl_data.chan_conf) *
         al_get_audio_depth_size(spl->spl_data.depth);

      memcpy(
         (char *) new_buf - bytes_per_sample * MAX_LAG,
         (char *) old_buf + bytes_per_sample * (spl->pos-MAX_LAG-new_pos),
         bytes_per_sample * MAX_LAG);

      stream->consumed_fragments++;
   }

   stream->spl.pos = new_pos;

   return true;
}


/* _al_kcm_feed_stream:
 * A routine running in another thread that feeds the stream buffers as
 * necessary, usually getting data from some file reader backend.
 */
void *_al_kcm_feed_stream(ALLEGRO_THREAD *self, void *vstream)
{
   ALLEGRO_AUDIO_STREAM *stream = vstream;
   ALLEGRO_EVENT_QUEUE *queue;
   bool finished_event_sent = false;
   bool prefill = true;
   (void)self;

   ALLEGRO_DEBUG("Stream feeder thread started.\n");

   queue = al_create_event_queue();
   al_register_event_source(queue, &stream->spl.es);

   stream->quit_feed_thread = false;

   while (!stream->quit_feed_thread) {
      char *fragment;
      ALLEGRO_EVENT event;

      if (!prefill)
         al_wait_for_event(queue, &event);

      if ((prefill || event.type == ALLEGRO_EVENT_AUDIO_STREAM_FRAGMENT)
          && !stream->is_draining) {
         unsigned long bytes;
         unsigned long bytes_written;
         ALLEGRO_MUTEX *stream_mutex;

         fragment = al_get_audio_stream_fragment(stream);
         if (!fragment) {
            /* This is not an error. */
            continue;
         }

         bytes = (stream->spl.spl_data.len) *
               al_get_channel_count(stream->spl.spl_data.chan_conf) *
               al_get_audio_depth_size(stream->spl.spl_data.depth);

         stream_mutex = maybe_lock_mutex(stream->spl.mutex);
         bytes_written = stream->feeder(stream, fragment, bytes);
         maybe_unlock_mutex(stream_mutex);

         if (stream->spl.loop == _ALLEGRO_PLAYMODE_STREAM_ONEDIR) {
            /* Keep rewinding until the fragment is filled. */
            while (bytes_written < bytes &&
                     stream->spl.loop == _ALLEGRO_PLAYMODE_STREAM_ONEDIR) {
               size_t bw;
               al_rewind_audio_stream(stream);
               stream_mutex = maybe_lock_mutex(stream->spl.mutex);
               bw = stream->feeder(stream, fragment + bytes_written,
                  bytes - bytes_written);
               bytes_written += bw;
               maybe_unlock_mutex(stream_mutex);
            }
         }
         else if (bytes_written < bytes) {
            /* Fill the rest of the fragment with silence. */
            int silence_samples = (bytes - bytes_written) /
               (al_get_channel_count(stream->spl.spl_data.chan_conf) *
                al_get_audio_depth_size(stream->spl.spl_data.depth));
            al_fill_silence(fragment + bytes_written, silence_samples,
                            stream->spl.spl_data.depth, stream->spl.spl_data.chan_conf);
         }

         if (!al_set_audio_stream_fragment(stream, fragment)) {
            ALLEGRO_ERROR("Error setting stream buffer.\n");
            continue;
         }

         /* The streaming source doesn't feed any more, so drain the stream.
          * Don't quit in case the user decides to seek and then restart the
          * stream. */
         if (bytes_written != bytes &&
             (stream->spl.loop == _ALLEGRO_PLAYMODE_STREAM_ONCE ||
              stream->spl.loop == _ALLEGRO_PLAYMODE_STREAM_LOOP_ONCE)) {
            /* Why not al_drain_audio_stream? We don't want to block on draining
             * because the user might adjust the stream loop points and restart
             * the stream. */
            stream->is_draining = true;

            if (!finished_event_sent) {
               ALLEGRO_EVENT fin_event;
               fin_event.user.type = ALLEGRO_EVENT_AUDIO_STREAM_FINISHED;
               fin_event.user.timestamp = al_get_time();
               al_emit_user_event(&stream->spl.es, &fin_event, NULL);
               finished_event_sent = true;
            }
         } else {
            finished_event_sent = false;
         }
      }
      else if (event.type == _KCM_STREAM_FEEDER_QUIT_EVENT_TYPE) {
         ALLEGRO_EVENT fin_event;
         stream->quit_feed_thread = true;

         fin_event.user.type = ALLEGRO_EVENT_AUDIO_STREAM_FINISHED;
         fin_event.user.timestamp = al_get_time();
         al_emit_user_event(&stream->spl.es, &fin_event, NULL);
      }
      if (prefill) {
         al_lock_mutex(stream->feed_thread_started_mutex);
         stream->feed_thread_started = true;
         al_broadcast_cond(stream->feed_thread_started_cond);
         al_unlock_mutex(stream->feed_thread_started_mutex);
      }
      prefill = false;
   }

   al_destroy_event_queue(queue);

   ALLEGRO_DEBUG("Stream feeder thread finished.\n");

   return NULL;
}


void _al_kcm_emit_stream_events(ALLEGRO_AUDIO_STREAM *stream)
{
   /* Emit one event for each stream fragment available right now.
    *
    * There may already be an event corresponding to an available fragment in
    * some event queue, but there's nothing we can do about that.  Streams may
    * be added and removed from queues, events may be lost by the user, etc.
    * so it would be dangerous to assume that each fragment event would be
    * responded to, once and exactly once.
    *
    * Having said that, event queues are empty in the steady state so it is
    * relatively rare that this situation occurs.
    */
   int count = al_get_available_audio_stream_fragments(stream);

   while (count--) {
      ALLEGRO_EVENT event;
      event.user.type = ALLEGRO_EVENT_AUDIO_STREAM_FRAGMENT;
      event.user.timestamp = al_get_time();
      al_emit_user_event(&stream->spl.es, &event, NULL);
   }
}


/* Function: al_rewind_audio_stream
 */
bool al_rewind_audio_stream(ALLEGRO_AUDIO_STREAM *stream)
{
   bool ret;

   if (stream->rewind_feeder) {
      ALLEGRO_MUTEX *stream_mutex = maybe_lock_mutex(stream->spl.mutex);
      ret = stream->rewind_feeder(stream);
      stream->is_draining = false;
      maybe_unlock_mutex(stream_mutex);
      return ret;
   }

   return false;
}


/* Function: al_seek_audio_stream_secs
 */
bool al_seek_audio_stream_secs(ALLEGRO_AUDIO_STREAM *stream, double time)
{
   bool ret;

   if (stream->seek_feeder) {
      ALLEGRO_MUTEX *stream_mutex = maybe_lock_mutex(stream->spl.mutex);
      ret = stream->seek_feeder(stream, time);
      stream->is_draining = false;
      maybe_unlock_mutex(stream_mutex);
      return ret;
   }

   return false;
}


/* Function: al_get_audio_stream_position_secs
 */
double al_get_audio_stream_position_secs(ALLEGRO_AUDIO_STREAM *stream)
{
   double ret;

   if (stream->get_feeder_position) {
      ALLEGRO_MUTEX *stream_mutex = maybe_lock_mutex(stream->spl.mutex);
      ret = stream->get_feeder_position(stream);
      maybe_unlock_mutex(stream_mutex);
      return ret;
   }

   return 0.0;
}


/* Function: al_get_audio_stream_length_secs
 */
double al_get_audio_stream_length_secs(ALLEGRO_AUDIO_STREAM *stream)
{
   double ret;

   if (stream->get_feeder_length) {
      ALLEGRO_MUTEX *stream_mutex = maybe_lock_mutex(stream->spl.mutex);
      ret = stream->get_feeder_length(stream);
      maybe_unlock_mutex(stream_mutex);
      return ret;
   }

   return 0.0;
}


/* Function: al_set_audio_stream_loop_secs
 */
bool al_set_audio_stream_loop_secs(ALLEGRO_AUDIO_STREAM *stream,
   double start, double end)
{
   bool ret;

   if (start >= end)
      return false;

   if (stream->set_feeder_loop) {
      ALLEGRO_MUTEX *stream_mutex = maybe_lock_mutex(stream->spl.mutex);
      ret = stream->set_feeder_loop(stream, start, end);
      stream->is_draining = false;
      maybe_unlock_mutex(stream_mutex);
      return ret;
   }

   return false;
}


/* Function: al_get_audio_stream_event_source
 */
ALLEGRO_EVENT_SOURCE *al_get_audio_stream_event_source(
   ALLEGRO_AUDIO_STREAM *stream)
{
   return &stream->spl.es;
}

bool al_set_audio_stream_channel_matrix(ALLEGRO_AUDIO_STREAM *stream, const float *matrix)
{
   ASSERT(stream);

   if (stream->spl.parent.u.ptr && stream->spl.parent.is_voice) {
      _al_set_error(ALLEGRO_GENERIC_ERROR,
         "Could not set channel matrix of stream attached to voice");
      return false;
   }

   return al_set_sample_instance_channel_matrix(&stream->spl, matrix);
}

/* vim: set sts=3 sw=3 et: */
