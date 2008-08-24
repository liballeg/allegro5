/**
 * Originally digi.c from allegro wiki
 * Original authors: KC/Milan
 *
 * Converted to allegro5 by Ryan Dickie
 */

/* Title: Voice functions
 */

#include <stdio.h>

#include "allegro5/kcm_audio.h"
#include "allegro5/internal/aintern_kcm_audio.h"
#include "allegro5/internal/aintern_kcm_cfg.h"


/* forward declarations */
static void stream_read(void *source, void **vbuf, unsigned long samples,
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
 *  must *not* be modified.
 */
const void *_al_voice_update(ALLEGRO_VOICE *voice, unsigned long samples)
{
   void *buf = NULL;

   ASSERT(voice);

   if (voice->stream) {
      _al_mutex_lock(&voice->mutex);
      voice->stream->spl_read(voice->stream, &buf, samples, voice->depth, 0);
      _al_mutex_unlock(&voice->mutex);
   }

   return buf;
}


/* Function: al_voice_create
 *  Creates a voice struct and allocates a voice from the digital sound driver.
 *  The sound driver's allocate_voice function should change the voice's
 *  frequency, depth, chan_conf, and settings fields to match what is actually
 *  allocated. If it cannot create a voice with exact settings it will fail.
 *  Use a mixer in such a case.
 */
ALLEGRO_VOICE *al_voice_create(unsigned long freq,
   ALLEGRO_AUDIO_DEPTH depth, ALLEGRO_CHANNEL_CONF chan_conf)
{
   ALLEGRO_VOICE *voice = NULL;

   if (!freq) {
      _al_set_error(ALLEGRO_INVALID_PARAM, "Invalid Voice Frequency");
      return NULL;
   }

   voice = calloc(1, sizeof(*voice));
   if (!voice) {
      return NULL;
   }

   voice->depth     = depth;
   voice->chan_conf = chan_conf;
   voice->frequency = freq;

   _al_mutex_init(&voice->mutex);
   /* XXX why is this needed? there should only be one active driver */
   voice->driver = _al_kcm_driver;

   ASSERT(_al_kcm_driver);
   if (_al_kcm_driver->allocate_voice(voice) != 0) {
      _al_mutex_destroy(&voice->mutex);
      free(voice);
      return NULL;
   }

   return voice;
}


/* Function: al_voice_destroy
 *  Destroys the voice and deallocates it from the digital driver.
 *  Does nothing if the voice is NULL.
 */
void al_voice_destroy(ALLEGRO_VOICE *voice)
{
   if (voice) {
      al_voice_detach(voice);
      voice->driver->deallocate_voice(voice);
      _al_mutex_destroy(&voice->mutex);

      free(voice);
   }
}


/* Function: al_voice_attach_sample
 *  Attaches a sample to a voice, and allows it to play. The sample's volume
 *  and loop mode will be ignored, and it must have the same frequency and
 *  depth (including signed-ness) as the voice. This function may fail if the
 *  selected driver doesn't support preloading sample data.
 */
int al_voice_attach_sample(ALLEGRO_VOICE *voice, ALLEGRO_SAMPLE *spl)
{
   int ret;

   ASSERT(voice);
   ASSERT(spl);

   if (voice->stream) {
      TRACE(
         "Attempted to attach to a voice that already has an attachment\n");
      _al_set_error(ALLEGRO_INVALID_OBJECT,
         "Attempted to attach to a voice that already has an attachment");
      return 1;
   }

   if (spl->parent.ptr) {
      TRACE("Attempted to attach a sample that is already attached\n");
      _al_set_error(ALLEGRO_INVALID_OBJECT,
         "Attempted to attach a sample that is already attached");
      return 1;
   }

   if (voice->chan_conf != spl->chan_conf ||
      voice->frequency != spl->frequency ||
      voice->depth != spl->depth)
   {
      TRACE("Sample settings do not match voice settings\n");
      _al_set_error(ALLEGRO_INVALID_OBJECT,
         "Sample settings do not match voice settings");
      return 1;
   }

   _al_mutex_lock(&voice->mutex);

   voice->stream = spl;

   voice->streaming = false;
   voice->num_buffers = 1;
   voice->buffer_size = (spl->len >> MIXER_FRAC_SHIFT) *
                        al_channel_count(voice->chan_conf) *
                        al_depth_size(voice->depth);

   spl->spl_read = NULL;
   _al_kcm_stream_set_mutex(spl, &voice->mutex);

   spl->parent.voice = voice;
   spl->parent_is_voice = true;

   if (voice->driver->load_voice(voice, spl->buffer.ptr) != 0 ||
      (spl->playing && voice->driver->start_voice(voice) != 0))
   {      
      voice->stream = NULL;
      spl->spl_read = NULL;
      _al_kcm_stream_set_mutex(spl, NULL);
      spl->parent.voice = NULL;

      TRACE("Unable to load sample into voice\n");
      ret = 1;
   }
   else {
      ret = 0;
   }

   _al_mutex_unlock(&voice->mutex);

   return ret;
}


/* stream_read:
 *  This passes the next waiting stream buffer to the voice via vbuf, setting
 *  the last one as used
 */
static void stream_read(void *source, void **vbuf, unsigned long samples,
   ALLEGRO_AUDIO_DEPTH buffer_depth, size_t dest_maxc)
{
   ALLEGRO_STREAM *stream = (ALLEGRO_STREAM*)source;
   void *old_buf;
   size_t i;

   if (!stream->spl.playing) {
      return;
   }

   old_buf = stream->pending_bufs[0];
   if (old_buf) {
      for (i = 0; stream->used_bufs[i] && i < stream->buf_count-1; i++)
         ;
      stream->used_bufs[i] = old_buf;
   }

   for (i = 0; stream->pending_bufs[i] && i < stream->buf_count-1; i++)
      ;
   stream->pending_bufs[i] = NULL;

   *vbuf = stream->pending_bufs[0];

   (void)dest_maxc;
   (void)buffer_depth;
   (void)samples;
}


/* Function: al_voice_attach_stream
 *  Attaches an audio stream to a voice. The same rules as
 *  <al_voice_attach_sample> apply. This may fail if the driver can't create
 *  a voice with the buffer count and buffer size the stream uses.
 */
int al_voice_attach_stream(ALLEGRO_VOICE *voice, ALLEGRO_STREAM *stream)
{
   int ret;

   ASSERT(voice);
   ASSERT(stream);

   if (voice->stream) {
      _al_set_error(ALLEGRO_INVALID_OBJECT,
         "Attempted to attach to a voice that already has an attachment");
      return 1;
   }

   if (stream->spl.parent.ptr) {
      _al_set_error(ALLEGRO_INVALID_OBJECT,
         "Attempted to attach a stream that is already attached");
      return 1;
   }

   if (voice->chan_conf != stream->spl.chan_conf ||
      voice->frequency != stream->spl.frequency ||
      voice->depth != stream->spl.depth)
   {
      _al_set_error(ALLEGRO_INVALID_OBJECT,
         "Stream settings do not match voice settings");
      return 1;
   }

   _al_mutex_lock(&voice->mutex);

   voice->stream = &stream->spl;

   _al_kcm_stream_set_mutex(&stream->spl, &voice->mutex);

   stream->spl.parent.voice = voice;
   stream->spl.parent_is_voice = true;

   voice->streaming = true;
   voice->num_buffers = stream->buf_count;
   voice->buffer_size = (stream->spl.len>>MIXER_FRAC_SHIFT) *
                        al_channel_count(stream->spl.chan_conf) *
                        al_depth_size(stream->spl.depth);

   stream->spl.spl_read = stream_read;

   if (voice->driver->start_voice(voice) != 0) {
      voice->stream = NULL;
      _al_kcm_stream_set_mutex(&stream->spl, NULL);
      stream->spl.parent.voice = NULL;
      stream->spl.spl_read = NULL;

      _al_set_error(ALLEGRO_GENERIC_ERROR, "Unable to start stream");
      ret = 1;
   }
   else {
      ret = 0;
   }

   _al_mutex_unlock(&voice->mutex);

   return ret;
}


/* Function: al_voice_attach_mixer
 *  Attaches a mixer to a voice. The same rules as <al_voice_attach_sample>
 *  apply, with the exception of the depth requirement.
 */
int al_voice_attach_mixer(ALLEGRO_VOICE *voice, ALLEGRO_MIXER *mixer)
{
   int ret;

   ASSERT(voice);
   ASSERT(mixer);

   if (voice->stream)
      return 1;
   if (mixer->ss.parent.ptr)
      return 2;

   if (voice->chan_conf != mixer->ss.chan_conf ||
         voice->frequency != mixer->ss.frequency) {
      return 3;
   }

   _al_mutex_lock(&voice->mutex);

   voice->stream = &mixer->ss;

   _al_kcm_stream_set_mutex(&mixer->ss, &voice->mutex);

   mixer->ss.parent.voice = voice;
   mixer->ss.parent_is_voice = true;

   voice->streaming = true;
   voice->num_buffers = 0;
   voice->buffer_size = 0;

   if (voice->driver->start_voice(voice) != 0) {
      voice->stream = NULL;
      _al_kcm_stream_set_mutex(&mixer->ss, NULL);
      mixer->ss.parent.voice = NULL;
      ret = 1;
   }
   else {
      ret = 0;
   }

   _al_mutex_unlock(&voice->mutex);

   return ret;
}


/* Function: al_voice_detach
 *  Detaches the sample or mixer stream from the voice.
 */
void al_voice_detach(ALLEGRO_VOICE *voice)
{
   ASSERT(voice);

   if (!voice->stream) {
      return;
   }

   _al_mutex_lock(&voice->mutex);

   if (!voice->streaming) {
      ALLEGRO_SAMPLE *spl = voice->stream;
      bool playing = false;

      al_voice_get_long(voice, ALLEGRO_AUDIOPROP_POSITION, &spl->pos);
      spl->pos <<= MIXER_FRAC_SHIFT;
      al_voice_get_bool(voice, ALLEGRO_AUDIOPROP_PLAYING, &playing);
      spl->playing = playing;

      voice->driver->stop_voice(voice);
      voice->driver->unload_voice(voice);
   }
   else {
      voice->driver->stop_voice(voice);
   }

   _al_kcm_stream_set_mutex(voice->stream, NULL);
   voice->stream->parent.voice = NULL;
   voice->stream = NULL;

   _al_mutex_unlock(&voice->mutex);
}


/* Function: al_voice_get_long
 */
int al_voice_get_long(const ALLEGRO_VOICE *voice,
   ALLEGRO_AUDIO_PROPERTY setting, unsigned long *val)
{
   ASSERT(voice);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_FREQUENCY:
         *val = voice->frequency;
         return 0;

      case ALLEGRO_AUDIOPROP_POSITION:
         if (voice->stream && !voice->streaming)
            *val = voice->driver->get_voice_position(voice);
         else
            *val = 0;
         return 0;

      default:
         return 1;
   }
}


/* Function: al_voice_get_enum
 */
int al_voice_get_enum(const ALLEGRO_VOICE *voice,
   ALLEGRO_AUDIO_PROPERTY setting, int *val)
{
   ASSERT(voice);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_CHANNELS:
         *val = voice->chan_conf;
         return 0;

      case ALLEGRO_AUDIOPROP_DEPTH:
         *val = voice->depth;
         return 0;

      default:
         return 1;
   }
}


/* Function: al_voice_get_bool
 */
int al_voice_get_bool(const ALLEGRO_VOICE *voice,
   ALLEGRO_AUDIO_PROPERTY setting, bool *val)
{
   ASSERT(voice);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_PLAYING:
         if (voice->stream && !voice->streaming) {
            *val = voice->driver->voice_is_playing(voice);
         }
         else if (voice->stream) {
            *val = true;
         }
         else {
            *val = false;
         }
         return 0;

      default:
         return 1;
   }
}


/* Function: al_voice_set_long
 */
int al_voice_set_long(ALLEGRO_VOICE *voice,
   ALLEGRO_AUDIO_PROPERTY setting, unsigned long val)
{
   ASSERT(voice);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_POSITION:
         if (voice->stream && !voice->streaming) {
            return voice->driver->set_voice_position(voice, val);
         }
         return 1;

      default:
         return 1;
   }
}


/* Function: al_voice_set_enum
 */
int al_voice_set_enum(ALLEGRO_VOICE *voice,
   ALLEGRO_AUDIO_PROPERTY setting, int val)
{
   ASSERT(voice);

   (void)voice;
   (void)val;

   switch (setting) {
      default:
         return 1;
   }
}


/* Function: al_voice_set_bool
 */
int al_voice_set_bool(ALLEGRO_VOICE *voice,
   ALLEGRO_AUDIO_PROPERTY setting, bool val)
{
   ASSERT(voice);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_PLAYING:
         if (voice->stream && !voice->streaming) {
            bool playing = false;
            if (al_voice_get_bool(voice, ALLEGRO_AUDIOPROP_PLAYING, &playing)) {
               TRACE("Unable to get voice playing status\n");
               return 1;
            }

            if (playing == val) {
               if (playing)
                  TRACE("Voice is already playing\n");
               else
                  TRACE("Voice is already stopped\n");
               return 1;
            }
            
            if (val)
               return voice->driver->start_voice(voice);
            else
               return voice->driver->stop_voice(voice);
         }
         else {
            TRACE("Voice has no sample or mixer attached\n");
            return 1;
         }

      default:
         return 1;
   }
}


/* vim: set sts=3 sw=3 et: */
