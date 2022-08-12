/**
 * Originally digi.c from allegro wiki
 * Original authors: KC/Milan
 *
 * Converted to allegro5 by Ryan Dickie
 */

/* Title: Voice functions
 */

#include <stdio.h>

#include "allegro5/allegro_audio.h"
#include "allegro5/internal/aintern_audio.h"
#include "allegro5/internal/aintern_audio_cfg.h"

ALLEGRO_DEBUG_CHANNEL("audio")


/* forward declarations */
static void stream_read(void *source, void **vbuf, unsigned int *samples,
   ALLEGRO_AUDIO_DEPTH buffer_depth, size_t dest_maxc);



/* _al_voice_update:
 *  Reads the attached stream and provides a buffer for the sound card. It is
 *  the driver's responsiblity to call this and to make sure any
 *  driver-specific resources associated with the voice are locked. This should
 *  only be called for streaming sources.
 *
 *  The return value is a pointer to the next chunk of audio data in the format
 *  the voice was allocated with. It may return NULL, in which case it is the
 *  driver's responsilibty to play silence for the voice. The returned buffer
 *  must *not* be modified. The 'samples' argument is set to the samples count
 *  in the returned audio data and it may be less or equal to the requested
 *  samples count.
 */
const void *_al_voice_update(ALLEGRO_VOICE *voice, ALLEGRO_MUTEX *mutex,
   unsigned int *samples)
{
   void *buf = NULL;

   /* The mutex parameter is intended to make it obvious at the call site
    * that the voice mutex will be acquired here.
    */
   ASSERT(voice);
   ASSERT(voice->mutex == mutex);
   (void)mutex;

   al_lock_mutex(voice->mutex);
   if (voice->attached_stream) {
      ASSERT(voice->attached_stream->spl_read);
      voice->attached_stream->spl_read(voice->attached_stream, &buf, samples,
         voice->depth, 0);
   }
   al_unlock_mutex(voice->mutex);

   return buf;
}


/* Function: al_create_voice
 */
ALLEGRO_VOICE *al_create_voice(unsigned int freq,
   ALLEGRO_AUDIO_DEPTH depth, ALLEGRO_CHANNEL_CONF chan_conf)
{
   ALLEGRO_VOICE *voice = NULL;

   if (!freq) {
      _al_set_error(ALLEGRO_INVALID_PARAM, "Invalid Voice Frequency");
      return NULL;
   }

   voice = al_calloc(1, sizeof(*voice));
   if (!voice) {
      return NULL;
   }

   voice->depth     = depth;
   voice->chan_conf = chan_conf;
   voice->frequency = freq;

   voice->mutex = al_create_mutex();
   voice->cond = al_create_cond();
   /* XXX why is this needed? there should only be one active driver */
   voice->driver = _al_kcm_driver;

   ASSERT(_al_kcm_driver);
   if (_al_kcm_driver->allocate_voice(voice) != 0) {
      al_destroy_mutex(voice->mutex);
      al_destroy_cond(voice->cond);
      al_free(voice);
      return NULL;
   }

   voice->dtor_item = _al_kcm_register_destructor("voice", voice,
      (void (*)(void *)) al_destroy_voice);

   return voice;
}


/* Function: al_destroy_voice
 */
void al_destroy_voice(ALLEGRO_VOICE *voice)
{
   if (voice) {
      _al_kcm_unregister_destructor(voice->dtor_item);

      al_detach_voice(voice);
      ASSERT(al_get_voice_playing(voice) == false);

      /* We do NOT lock the voice mutex when calling this method. */
      voice->driver->deallocate_voice(voice);
      al_destroy_mutex(voice->mutex);
      al_destroy_cond(voice->cond);

      al_free(voice);
   }
}


/* Function: al_attach_sample_instance_to_voice
 */
bool al_attach_sample_instance_to_voice(ALLEGRO_SAMPLE_INSTANCE *spl,
   ALLEGRO_VOICE *voice)
{
   bool ret;

   ASSERT(voice);
   ASSERT(spl);

   if (voice->attached_stream) {
      ALLEGRO_WARN(
         "Attempted to attach to a voice that already has an attachment\n");
      _al_set_error(ALLEGRO_INVALID_OBJECT,
         "Attempted to attach to a voice that already has an attachment");
      return false;
   }

   if (spl->parent.u.ptr) {
      ALLEGRO_WARN("Attempted to attach a sample that is already attached\n");
      _al_set_error(ALLEGRO_INVALID_OBJECT,
         "Attempted to attach a sample that is already attached");
      return false;
   }

   if (voice->chan_conf != spl->spl_data.chan_conf ||
      voice->frequency != spl->spl_data.frequency ||
      voice->depth != spl->spl_data.depth)
   {
      ALLEGRO_WARN("Sample settings do not match voice settings\n");
      _al_set_error(ALLEGRO_INVALID_OBJECT,
         "Sample settings do not match voice settings");
      return false;
   }

   al_lock_mutex(voice->mutex);

   voice->attached_stream = spl;

   voice->is_streaming = false;
   voice->num_buffers = 1;
   voice->buffer_size = (spl->spl_data.len) *
                        al_get_channel_count(voice->chan_conf) *
                        al_get_audio_depth_size(voice->depth);

   spl->spl_read = NULL;
   _al_kcm_stream_set_mutex(spl, voice->mutex);

   spl->parent.u.voice = voice;
   spl->parent.is_voice = true;

   if (voice->driver->load_voice(voice, spl->spl_data.buffer.ptr) != 0 ||
      (spl->is_playing && voice->driver->start_voice(voice) != 0))
   {      
      voice->attached_stream = NULL;
      spl->spl_read = NULL;
      _al_kcm_stream_set_mutex(spl, NULL);
      spl->parent.u.voice = NULL;

      ALLEGRO_ERROR("Unable to load sample into voice\n");
      ret = false;
   }
   else {
      ret = true;
   }

   al_unlock_mutex(voice->mutex);

   return ret;
}


/* stream_read:
 *  This passes the next waiting stream buffer to the voice via vbuf.
 */
static void stream_read(void *source, void **vbuf, unsigned int *samples,
   ALLEGRO_AUDIO_DEPTH buffer_depth, size_t dest_maxc)
{
   ALLEGRO_AUDIO_STREAM *stream = (ALLEGRO_AUDIO_STREAM*)source;
   unsigned int len = stream->spl.spl_data.len;
   unsigned int pos = stream->spl.pos;

   if (!stream->spl.is_playing) {
      *vbuf = NULL;
      *samples = 0;
      return;
   }

   if (*samples > len)
      *samples = len;

   if (pos >= len) {
      /* XXX: Handle the case where we need to call _al_kcm_refill_stream
       * multiple times due to ludicrous playback speed. */
      _al_kcm_refill_stream(stream);
      if (!stream->pending_bufs[0]) {
         if (stream->is_draining) {
            stream->spl.is_playing = false;
         }
         *vbuf = NULL;
         *samples = 0;
         return;
      }
      *vbuf = stream->pending_bufs[0];
      pos = *samples;

      _al_kcm_emit_stream_events(stream);
   }
   else {
      int bytes = pos * al_get_channel_count(stream->spl.spl_data.chan_conf)
                      * al_get_audio_depth_size(stream->spl.spl_data.depth);
      *vbuf = ((char *)stream->pending_bufs[0]) + bytes;

      if (pos + *samples > len)
         *samples = len - pos;
      pos += *samples;
   }

   stream->spl.pos = pos;

   (void)dest_maxc;
   (void)buffer_depth;
}


/* Function: al_attach_audio_stream_to_voice
 */
bool al_attach_audio_stream_to_voice(ALLEGRO_AUDIO_STREAM *stream,
   ALLEGRO_VOICE *voice)
{
   bool ret;

   ASSERT(voice);
   ASSERT(stream);

   if (voice->attached_stream) {
      _al_set_error(ALLEGRO_INVALID_OBJECT,
         "Attempted to attach to a voice that already has an attachment");
      return false;
   }

   if (stream->spl.parent.u.ptr) {
      _al_set_error(ALLEGRO_INVALID_OBJECT,
         "Attempted to attach a stream that is already attached");
      return false;
   }

   if (voice->chan_conf != stream->spl.spl_data.chan_conf ||
      voice->frequency != stream->spl.spl_data.frequency ||
      voice->depth != stream->spl.spl_data.depth)
   {
      _al_set_error(ALLEGRO_INVALID_OBJECT,
         "Stream settings do not match voice settings");
      return false;
   }

   al_lock_mutex(voice->mutex);

   voice->attached_stream = &stream->spl;

   _al_kcm_stream_set_mutex(&stream->spl, voice->mutex);

   stream->spl.parent.u.voice = voice;
   stream->spl.parent.is_voice = true;

   voice->is_streaming = true;
   voice->num_buffers = stream->buf_count;
   voice->buffer_size = (stream->spl.spl_data.len) *
                        al_get_channel_count(stream->spl.spl_data.chan_conf) *
                        al_get_audio_depth_size(stream->spl.spl_data.depth);

   ASSERT(stream->spl.spl_read == NULL);
   stream->spl.spl_read = stream_read;

   if (voice->driver->start_voice(voice) != 0) {
      voice->attached_stream = NULL;
      _al_kcm_stream_set_mutex(&stream->spl, NULL);
      stream->spl.parent.u.voice = NULL;
      stream->spl.spl_read = NULL;

      _al_set_error(ALLEGRO_GENERIC_ERROR, "Unable to start stream");
      ret = false;
   }
   else {
      ret = true;
   }

   al_unlock_mutex(voice->mutex);

   return ret;
}


/* Function: al_attach_mixer_to_voice
 */
bool al_attach_mixer_to_voice(ALLEGRO_MIXER *mixer, ALLEGRO_VOICE *voice)
{
   bool ret;

   ASSERT(voice);
   ASSERT(mixer);
   ASSERT(mixer->ss.is_mixer);

   if (voice->attached_stream)
      return false;
   if (mixer->ss.parent.u.ptr)
      return false;

   if (voice->chan_conf != mixer->ss.spl_data.chan_conf ||
         voice->frequency != mixer->ss.spl_data.frequency) {
      return false;
   }

   al_lock_mutex(voice->mutex);

   voice->attached_stream = &mixer->ss;
   ASSERT(mixer->ss.spl_read == NULL);
   mixer->ss.spl_read = _al_kcm_mixer_read;

   _al_kcm_stream_set_mutex(&mixer->ss, voice->mutex);

   mixer->ss.parent.u.voice = voice;
   mixer->ss.parent.is_voice = true;

   voice->is_streaming = true;
   voice->num_buffers = 0;
   voice->buffer_size = 0;

   if (voice->driver->start_voice(voice) != 0) {
      voice->attached_stream = NULL;
      _al_kcm_stream_set_mutex(&mixer->ss, NULL);
      mixer->ss.parent.u.voice = NULL;
      ret = false;
   }
   else {
      ret = true;
   }

   al_unlock_mutex(voice->mutex);

   return ret;
}


/* Function: al_detach_voice
 */
void al_detach_voice(ALLEGRO_VOICE *voice)
{
   ASSERT(voice);

   if (!voice->attached_stream) {
      return;
   }

   al_lock_mutex(voice->mutex);

   if (!voice->is_streaming) {
      ALLEGRO_SAMPLE_INSTANCE *spl = voice->attached_stream;

      spl->pos = voice->driver->get_voice_position(voice);
      spl->is_playing = voice->driver->voice_is_playing(voice);

      voice->driver->stop_voice(voice);
      voice->driver->unload_voice(voice);
   }
   else {
      voice->driver->stop_voice(voice);
   }

   _al_kcm_stream_set_mutex(voice->attached_stream, NULL);
   voice->attached_stream->parent.u.voice = NULL;
   voice->attached_stream->spl_read = NULL;
   voice->attached_stream = NULL;

   al_unlock_mutex(voice->mutex);
}


/* Function: al_get_voice_frequency
 */
unsigned int al_get_voice_frequency(const ALLEGRO_VOICE *voice)
{
   ASSERT(voice);

   return voice->frequency;
}


/* Function: al_get_voice_position
 */
unsigned int al_get_voice_position(const ALLEGRO_VOICE *voice)
{
   ASSERT(voice);

   if (voice->attached_stream && !voice->is_streaming) {
      unsigned int ret;
      al_lock_mutex(voice->mutex);
      ret = voice->driver->get_voice_position(voice);
      al_unlock_mutex(voice->mutex);
      return ret;
   }
   else
      return 0;
}


/* Function: al_get_voice_channels
 */
ALLEGRO_CHANNEL_CONF al_get_voice_channels(const ALLEGRO_VOICE *voice)
{
   ASSERT(voice);

   return voice->chan_conf;
}


/* Function: al_get_voice_depth
 */
ALLEGRO_AUDIO_DEPTH al_get_voice_depth(const ALLEGRO_VOICE *voice)
{
   ASSERT(voice);

   return voice->depth;
}


/* Function: al_get_voice_playing
 */
bool al_get_voice_playing(const ALLEGRO_VOICE *voice)
{
   ASSERT(voice);

   if (voice->attached_stream && !voice->is_streaming) {
      bool ret;
      al_lock_mutex(voice->mutex);
      ret = voice->driver->voice_is_playing(voice);
      al_unlock_mutex(voice->mutex);
      return ret;
   }

   return voice->attached_stream ? true : false;
}

/* Function: al_voice_has_attachments
 */
bool al_voice_has_attachments(const ALLEGRO_VOICE* voice)
{
   ASSERT(voice);

   return voice->attached_stream;
}

/* Function: al_set_voice_position
 */
bool al_set_voice_position(ALLEGRO_VOICE *voice, unsigned int val)
{
   ASSERT(voice);

   if (voice->attached_stream && !voice->is_streaming) {
      bool ret;
      al_lock_mutex(voice->mutex);
      // XXX change method
      ret = voice->driver->set_voice_position(voice, val) == 0;
      al_unlock_mutex(voice->mutex);
      return ret;
   }

   return false;
}


/* Function: al_set_voice_playing
 */
bool al_set_voice_playing(ALLEGRO_VOICE *voice, bool val)
{
   ASSERT(voice);

   if (!voice->attached_stream) {
      ALLEGRO_DEBUG("Voice has no attachment\n");
      return false;
   }

   if (voice->is_streaming) {
      ALLEGRO_WARN("Attempted to change the playing state of a voice "
         "with a streaming attachment (mixer or audiostreams)\n");
      return false;
   }
   else {
      bool playing = al_get_voice_playing(voice);
      if (playing == val) {
         if (playing) {
            ALLEGRO_DEBUG("Voice is already playing\n");
         }
         else {
            ALLEGRO_DEBUG("Voice is already stopped\n");
         }
         return true;
      }

      return _al_kcm_set_voice_playing(voice, voice->mutex, val);
   }
}


bool _al_kcm_set_voice_playing(ALLEGRO_VOICE *voice, ALLEGRO_MUTEX *mutex,
   bool val)
{
   bool ret;

   /* The mutex parameter is intended to make it obvious at the call site
    * that the voice mutex will be acquired here.
    */
   ASSERT(voice);
   ASSERT(voice->mutex == mutex);
   (void)mutex;

   al_lock_mutex(voice->mutex);
   // XXX change methods
   if (val)
      ret = voice->driver->start_voice(voice) == 0;
   else
      ret = voice->driver->stop_voice(voice) == 0;
   al_unlock_mutex(voice->mutex);

   return ret;
}


/* vim: set sts=3 sw=3 et: */
