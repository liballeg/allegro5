/**
 * Originally digi.c from allegro wiki
 * Original authors: KC/Milan
 *
 * Converted to allegro5 by Ryan Dickie
 */

/* Title: Mixer functions
 */

#include <stdio.h>
#include <math.h>

#include "allegro5/kcm_audio.h"
#include "allegro5/internal/aintern_kcm_audio.h"
#include "allegro5/internal/aintern_kcm_cfg.h"


/* forward declarations */
static void mixer_change_quality(ALLEGRO_MIXER *mixer,
   ALLEGRO_MIXER_QUALITY new_quality);


/* globals */
static float _samp_buf[ALLEGRO_MAX_CHANNELS]; /* max: 7.1 */



/* _al_rechannel_matrix:
 *  This function provides a (temporary!) matrix that can be used to convert
 *  one channel configuration into another.
 *
 *  Returns a pointer to a statically allocated array.
 */
static float *_al_rechannel_matrix(ALLEGRO_CHANNEL_CONF orig,
   ALLEGRO_CHANNEL_CONF target)
{
   /* Max 7.1 (8 channels) for input and output */
   static float mat[ALLEGRO_MAX_CHANNELS][ALLEGRO_MAX_CHANNELS];

   size_t dst_chans = al_channel_count(target);
   size_t src_chans = al_channel_count(orig);
   size_t i, j;

   /* Start with a simple identity matrix */
   memset(mat, 0, sizeof(mat));
   for (i = 0; i < src_chans && i < dst_chans; i++) {
      mat[i][i] = 1.0;
   }

   /* Multi-channel -> mono conversion (cuts rear/side channels) */
   if (dst_chans == 1 && (orig>>4) > 1) {
      for (i = 0; i < 2; i++) {
         mat[0][i] = 1.0 / sqrt(2.0);
      }

      /* If the source has a center channel, make sure that's copied 1:1
       * (perhaps it should scale the overall output?)
       */
      if ((orig >> 4) & 1) {
         mat[0][(orig >> 4) - 1] = 1.0;
      }
   }
   /* Center (or mono) -> front l/r conversion */
   else if (((orig >> 4) & 1) && !((target >> 4) & 1)) {
      mat[0][(orig >> 4) - 1] = 1.0 / sqrt(2.0);
      mat[1][(orig >> 4) - 1] = 1.0 / sqrt(2.0);
   }

   /* Copy LFE */
   if ((orig >> 4) != (target >> 4) &&
      (orig & 0xF) && (target & 0xF))
   {
      mat[dst_chans-1][src_chans-1] = 1.0;
   }

   TRACE("sample matrix:\n");
   for (i = 0; i < dst_chans; i++) {
      for (j = 0; j < src_chans; j++) {
         TRACE(" %f", mat[i][j]);
      }
      TRACE("\n");
   }
   TRACE("\n");

   return &mat[0][0];
}


/* fix_looped_position:
 *  When a stream loops, this will fix up the position and anything else to
 *  allow it to safely continue is_playing as expected. Returns 0 if it should
 *  stop being mixed.
 *
 * XXX reconsider inlining this; it looks rather big
 */
static INLINE int fix_looped_position(ALLEGRO_SAMPLE *spl)
{
   /* Looping! Should be mostly self-explanitory */
   switch (spl->loop) {
      case ALLEGRO_PLAYMODE_ONEDIR:
         if (spl->step > 0) {
            while (spl->pos < spl->loop_start || spl->pos >= spl->loop_end) {
               spl->pos -= (spl->loop_end - spl->loop_start);
            }
         }
         else if (spl->step < 0) {
            while (spl->pos < spl->loop_start || spl->pos >= spl->loop_end) {
               spl->pos += (spl->loop_end - spl->loop_start);
            }
         }
         break;

      case ALLEGRO_PLAYMODE_BIDIR:
         /* When doing bi-directional looping, you need to do a follow-up
          * check for the opposite direction if a loop occurred, otherwise
          * you could end up misplaced on small, high-step loops.
          */
         if (spl->step >= 0) {
         check_forward:
            if (spl->pos >= spl->loop_end) {
               spl->step = -spl->step;
               spl->pos = spl->loop_end - (spl->pos - spl->loop_end) - 1;
               goto check_backward;
            }
         }
         else {
         check_backward:
            if (spl->pos < spl->loop_start || spl->pos >= spl->loop_end) {
               spl->step = -spl->step;
               spl->pos = spl->loop_start + (spl->loop_start - spl->pos);
               goto check_forward;
            }
         }
         break;

      default:
         if (spl->pos >= spl->len) {
            if (spl->is_stream) {
               ALLEGRO_STREAM *stream = (ALLEGRO_STREAM *)spl;
               void *old_buf = stream->spl.buffer.ptr;
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

               stream->spl.buffer.ptr = stream->pending_bufs[0];
               if (!stream->spl.buffer.ptr)
                  return 0;

               spl->pos -= spl->len;
               return 1;
            }

            spl->pos = spl->len;
            spl->is_playing = false;
            return 0;
         }
   }

   return 1;
}


static INLINE const float *point_spl32(const ALLEGRO_SAMPLE *spl,
   unsigned int maxc)
{
   float *s = _samp_buf;
   unsigned int i;

   for (i = 0; i < maxc; i++) {
      if (spl->depth == ALLEGRO_AUDIO_DEPTH_INT24) {
         s[i] =
            (float) spl->buffer.s24[(spl->pos>>MIXER_FRAC_SHIFT)*maxc + i] /
            ((float)0x7FFFFF + 0.5f);
      }
      else if (spl->depth == ALLEGRO_AUDIO_DEPTH_INT16) {
         s[i] =
            (float) spl->buffer.s16[(spl->pos>>MIXER_FRAC_SHIFT)*maxc + i] /
            ((float)0x7FFF + 0.5f);
      }
      else if (spl->depth == ALLEGRO_AUDIO_DEPTH_INT8) {
         s[i] = (float) spl->buffer.s8[(spl->pos>>MIXER_FRAC_SHIFT)*maxc + i] /
            ((float)0x7F + 0.5f);
      }
      else {
         s[i] = spl->buffer.f32[(spl->pos>>MIXER_FRAC_SHIFT)*maxc + i];
      }
   }
   return s;
}


static INLINE const float *point_spl32u(const ALLEGRO_SAMPLE *spl,
   unsigned int maxc)
{
   float *s = _samp_buf;
   unsigned int i;

   for (i = 0; i < maxc; i++) {
      if (spl->depth == ALLEGRO_AUDIO_DEPTH_UINT24) {
         s[i] = (float)spl->buffer.u24[(spl->pos>>MIXER_FRAC_SHIFT)*maxc + i]
            / ((float)0x7FFFFF+0.5f) - 1.0;
      }
      else if (spl->depth == ALLEGRO_AUDIO_DEPTH_UINT16) {
         s[i] = (float)spl->buffer.u16[(spl->pos>>MIXER_FRAC_SHIFT)*maxc + i]
            / ((float)0x7FFF+0.5f) - 1.0;
      }
      else if (spl->depth == ALLEGRO_AUDIO_DEPTH_UINT8) {
         s[i] = (float)spl->buffer.u8[(spl->pos>>MIXER_FRAC_SHIFT)*maxc + i]
            / ((float)0x7F+0.5f) - 1.0;
      }
      else {
         s[i] = spl->buffer.f32[(spl->pos>>MIXER_FRAC_SHIFT)*maxc + i] - 1.0;
      }
   }
   return s;
}


static INLINE const float *linear_spl32(const ALLEGRO_SAMPLE *spl,
   unsigned int maxc)
{
   unsigned long p1, p2;
   unsigned int i;
   float frac, *s = _samp_buf;

   p1 = (spl->pos>>MIXER_FRAC_SHIFT)*maxc;
   p2 = p1 + maxc;

   if (spl->loop == ALLEGRO_PLAYMODE_ONCE) {
      if (spl->pos+MIXER_FRAC_ONE >= spl->len)
         p2 = p1;
   }
   else if (spl->pos+MIXER_FRAC_ONE >= spl->loop_end) {
      if (spl->loop == ALLEGRO_PLAYMODE_ONEDIR)
         p2 = (spl->loop_start>>MIXER_FRAC_SHIFT)*maxc;
      else
         p2 = ((spl->loop_end>>MIXER_FRAC_SHIFT)-1)*maxc;
   }

   frac = (float)(spl->pos&MIXER_FRAC_MASK) / (float)MIXER_FRAC_ONE;

   for (i = 0; i < maxc; i++) {
      float s1, s2;

      if (spl->depth == ALLEGRO_AUDIO_DEPTH_INT24) {
         s1 = (float)spl->buffer.s24[p1 + i] / ((float)0x7FFFFF + 0.5f);
         s2 = (float)spl->buffer.s24[p2 + i] / ((float)0x7FFFFF + 0.5f);
      }
      else if (spl->depth == ALLEGRO_AUDIO_DEPTH_INT16) {
         s1 = (float)spl->buffer.s16[p1 + i] / ((float)0x7FFF + 0.5f);
         s2 = (float)spl->buffer.s16[p2 + i] / ((float)0x7FFF + 0.5f);
      }
      else if (spl->depth == ALLEGRO_AUDIO_DEPTH_INT8) {
         s1 = (float)spl->buffer.s8[p1 + i] / ((float)0x7F + 0.5f);
         s2 = (float)spl->buffer.s8[p2 + i] / ((float)0x7F + 0.5f);
      }
      else {
         s1 = (float)spl->buffer.f32[p1 + i];
         s2 = (float)spl->buffer.f32[p2 + i];
      }

      s[i] = (s1*(1.0f-frac)) + (s2*frac);
   }
   return s;
}


static INLINE const float *linear_spl32u(const ALLEGRO_SAMPLE *spl,
   unsigned int maxc)
{
   unsigned long p1, p2;
   unsigned int i;
   float frac, *s = _samp_buf;

   p1 = (spl->pos>>MIXER_FRAC_SHIFT)*maxc;
   p2 = p1 + maxc;

   if (spl->loop == ALLEGRO_PLAYMODE_ONCE) {
      if (spl->pos+MIXER_FRAC_ONE >= spl->len)
         p2 = p1;
   }
   else if (spl->pos+MIXER_FRAC_ONE >= spl->loop_end) {
      if (spl->loop == ALLEGRO_PLAYMODE_ONEDIR)
         p2 = (spl->loop_start>>MIXER_FRAC_SHIFT)*maxc;
      else
         p2 = ((spl->loop_end>>MIXER_FRAC_SHIFT)-1)*maxc;
   }

   frac = (float)(spl->pos&MIXER_FRAC_MASK) / (float)MIXER_FRAC_ONE;

   for (i = 0; i < maxc; i++) {
      float s1, s2;

      if (spl->depth == ALLEGRO_AUDIO_DEPTH_UINT24) {
         s1 = (float)spl->buffer.u24[p1 + i]/((float)0x7FFFFF + 0.5f) - 1.0f;
         s2 = (float)spl->buffer.u24[p2 + i]/((float)0x7FFFFF + 0.5f) - 1.0f;
      }
      else if (spl->depth == ALLEGRO_AUDIO_DEPTH_UINT16) {
         s1 = (float)spl->buffer.u16[p1 + i]/((float)0x7FFF + 0.5f) - 1.0f;
         s2 = (float)spl->buffer.u16[p2 + i]/((float)0x7FFF + 0.5f) - 1.0f;
      }
      else if (spl->depth == ALLEGRO_AUDIO_DEPTH_UINT8) {
         s1 = (float)spl->buffer.u8[p1 + i]/((float)0x7F + 0.5f) - 1.0f;
         s2 = (float)spl->buffer.u8[p2 + i]/((float)0x7F + 0.5f) - 1.0f;
      }
      else {
         s1 = (float)spl->buffer.f32[p1 + i] - 1.0f;
         s2 = (float)spl->buffer.f32[p2 + i] - 1.0f;
      }

      s[i] = (s1*(1.0f-frac)) + (s2*frac);
   }
   return s;
}


/* Reads some samples into a mixer buffer. */
#define MAKE_MIXER(bits, interp)                                              \
static void read_to_mixer_##interp##bits(void *source, void **vbuf,           \
   unsigned long samples, ALLEGRO_AUDIO_DEPTH buffer_depth,                   \
   size_t dest_maxc)                                                          \
{                                                                             \
   ALLEGRO_SAMPLE *spl = (ALLEGRO_SAMPLE *)source;                            \
   float *buf = *vbuf;                                                        \
   size_t maxc = al_channel_count(spl->chan_conf);                            \
   size_t c;                                                                  \
                                                                              \
   if (!spl->is_playing)                                                      \
      return;                                                                 \
                                                                              \
   while (samples > 0) {                                                      \
      const float *s;                                                         \
                                                                              \
      if (!fix_looped_position(spl))                                          \
         return;                                                              \
                                                                              \
      s = interp##_spl##bits(spl, maxc);                                      \
      for (c = 0; c < dest_maxc; c++) {                                       \
         size_t i;                                                            \
         for (i = 0; i < maxc; i++) {                                         \
            *buf += s[i] * spl->matrix[c*maxc + i];                           \
         }                                                                    \
         buf++;                                                               \
      }                                                                       \
                                                                              \
      spl->pos += spl->step;                                                  \
      samples--;                                                              \
   }                                                                          \
   fix_looped_position(spl);                                                  \
   (void)buffer_depth;                                                        \
}

MAKE_MIXER(32,  point)
MAKE_MIXER(32u, point)
MAKE_MIXER(32,  linear)
MAKE_MIXER(32u, linear)

#undef MAKE_MIXER


/* Function: al_mixer_create
 *  Creates a mixer stream, to attach sample streams or other mixers to. It
 *  will mix into a buffer at the requested frequency and channel count.
 *  Only 24-bit signed integer mixing is currently supported.
 */
ALLEGRO_MIXER *al_mixer_create(unsigned long freq,
   ALLEGRO_AUDIO_DEPTH depth, ALLEGRO_CHANNEL_CONF chan_conf)
{
   ALLEGRO_MIXER *mixer;

   if (!freq) {
      _al_set_error(ALLEGRO_INVALID_PARAM,
         "Attempted to create mixer with no frequency");
      return NULL;
   }
   if (depth != ALLEGRO_AUDIO_DEPTH_FLOAT32) {
      _al_set_error(ALLEGRO_INVALID_PARAM,
         "Attempted to create mixer with an invalid sample depth");
      return NULL;
   }

   mixer = calloc(1, sizeof(ALLEGRO_MIXER));
   if (!mixer) {
      _al_set_error(ALLEGRO_GENERIC_ERROR,
         "Out of memory allocating mixer object");
      return NULL;
   }

   mixer->ss.is_playing = true;
   mixer->ss.orphan_buffer = true;

   mixer->ss.loop      = ALLEGRO_PLAYMODE_ONCE;
   mixer->ss.depth     = depth;
   mixer->ss.chan_conf = chan_conf;
   mixer->ss.frequency = freq;

   mixer->ss.spl_read = _al_kcm_mixer_read;

   mixer->quality = ALLEGRO_MIXER_QUALITY_LINEAR;

   _al_vector_init(&mixer->streams, sizeof(ALLEGRO_SAMPLE *));

   return mixer;
}


/* Function: al_mixer_destroy
 *  Destroys the mixer stream, identical to al_sample_destroy.
 */
void al_mixer_destroy(ALLEGRO_MIXER *mixer)
{
   al_sample_destroy(&mixer->ss);
}


/* Function: al_mixer_attach_sample
 */
/* This function is ALLEGRO_MIXER aware */
int al_mixer_attach_sample(ALLEGRO_MIXER *mixer, ALLEGRO_SAMPLE *spl)
{
   ALLEGRO_SAMPLE **slot;
   unsigned long i;

   ASSERT(mixer);
   ASSERT(spl);

   /* Already referenced, do not attach. */
   if (spl->parent.u.ptr) {
      _al_set_error(ALLEGRO_INVALID_OBJECT,
         "Attempted to attach a sample that's already attached");
      return 1;
   }

   ASSERT(mixer->ss.mutex);
   al_lock_mutex(mixer->ss.mutex);

   slot = _al_vector_alloc_back(&mixer->streams);
   if (!slot) {
      al_unlock_mutex(mixer->ss.mutex);
      _al_set_error(ALLEGRO_GENERIC_ERROR,
         "Out of memory allocating attachment pointers");
      return 1;
   }
   (*slot) = spl;

   spl->step = (spl->frequency<<MIXER_FRAC_SHIFT) * spl->speed /
               mixer->ss.frequency;
   /* Don't want to be trapped with a step value of 0. */
   if (spl->step == 0) {
      if (spl->speed > 0.0f)
         spl->step = 1;
      else
         spl->step = -1;
   }

   /* If this isn't a mixer, set the proper sample stream reader */
   if (spl->spl_read == NULL) {
      size_t dst_chans;
      size_t src_chans;
      float *mat;

      dst_chans = (mixer->ss.chan_conf >> 4) + (mixer->ss.chan_conf & 0xF);
      src_chans = (spl->chan_conf >> 4) + (spl->chan_conf & 0xF);

      if (mixer->quality == ALLEGRO_MIXER_QUALITY_LINEAR) {
         if ((spl->depth & ALLEGRO_AUDIO_DEPTH_UNSIGNED))
            spl->spl_read = read_to_mixer_linear32u;
         else
            spl->spl_read = read_to_mixer_linear32;
      }
      else {
         if ((spl->depth & ALLEGRO_AUDIO_DEPTH_UNSIGNED))
            spl->spl_read = read_to_mixer_point32u;
         else
            spl->spl_read = read_to_mixer_point32;
      }

      mat = _al_rechannel_matrix(spl->chan_conf, mixer->ss.chan_conf);

      spl->matrix = calloc(1, src_chans * dst_chans * sizeof(float));

      for (i = 0; i < dst_chans; i++) {
         size_t j;
         for (j = 0; j < src_chans; j++) {
            spl->matrix[i*src_chans + j] = mat[i*ALLEGRO_MAX_CHANNELS + j];
         }
      }
   }

   _al_kcm_stream_set_mutex(spl, mixer->ss.mutex);

   spl->parent.u.mixer = mixer;
   spl->parent.is_voice = false;

   al_unlock_mutex(mixer->ss.mutex);

   return 0;
}


/* Function: al_mixer_attach_stream
 */
int al_mixer_attach_stream(ALLEGRO_MIXER *mixer, ALLEGRO_STREAM *stream)
{
   ASSERT(mixer);
   ASSERT(stream);

   return al_mixer_attach_sample(mixer, &stream->spl);
}


/* Function: al_mixer_attach_mixer
 *  Attaches a mixer onto another mixer. The same rules as with
 *  <al_mixer_attach_sample> apply, with the added caveat that both
 *  mixers must be the same frequency.
 */
int al_mixer_attach_mixer(ALLEGRO_MIXER *mixer, ALLEGRO_MIXER *stream)
{
   ASSERT(mixer);
   ASSERT(stream);

   if (mixer->ss.frequency != stream->ss.frequency) {
      _al_set_error(ALLEGRO_INVALID_OBJECT,
         "Attempted to attach a mixer with different frequencies");
      return 1;
   }

   return al_mixer_attach_sample(mixer, &stream->ss);
}


/* Function: al_mixer_set_postprocess_callback
 *  Sets a post-processing filter function that's called after the attached
 *  streams have been mixed. The buffer's format will be whatever the mixer
 *  was created with. The sample count and user-data pointer is also passed.
 */
int al_mixer_set_postprocess_callback(ALLEGRO_MIXER *mixer,
   postprocess_callback_t postprocess_callback, void *pp_callback_userdata)
{
   ASSERT(mixer);

   al_lock_mutex(mixer->ss.mutex);
   mixer->postprocess_callback = postprocess_callback;
   mixer->pp_callback_userdata = pp_callback_userdata;
   al_unlock_mutex(mixer->ss.mutex);

   return 0;
}


/* Function: al_mixer_get_long
 */
int al_mixer_get_long(const ALLEGRO_MIXER *mixer,
   ALLEGRO_AUDIO_PROPERTY setting, unsigned long *val)
{
   ASSERT(mixer);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_FREQUENCY:
         *val = mixer->ss.frequency;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to get invalid mixer long setting");
         return 1;
   }
}


/* Function: al_mixer_get_enum
 */
int al_mixer_get_enum(const ALLEGRO_MIXER *mixer,
   ALLEGRO_AUDIO_PROPERTY setting, int *val)
{
   ASSERT(mixer);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_CHANNELS:
         *val = mixer->ss.chan_conf;
         return 0;

      case ALLEGRO_AUDIOPROP_DEPTH:
         *val = mixer->ss.depth;
         return 0;

      case ALLEGRO_AUDIOPROP_QUALITY:
         *val = mixer->quality;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to get invalid mixer enum setting");
         return 1;
   }
}


/* Function: al_mixer_get_bool
 */
int al_mixer_get_bool(const ALLEGRO_MIXER *mixer,
   ALLEGRO_AUDIO_PROPERTY setting, bool *val)
{
   ASSERT(mixer);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_PLAYING:
         *val = mixer->ss.is_playing;
         return 0;

      case ALLEGRO_AUDIOPROP_ATTACHED:
         *val = _al_vector_is_nonempty(&mixer->streams);
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to get invalid mixer bool setting");
         return 1;
   }
}


/* Function: al_mixer_set_long
 */
int al_mixer_set_long(ALLEGRO_MIXER *mixer,
   ALLEGRO_AUDIO_PROPERTY setting, unsigned long val)
{
   ASSERT(mixer);

   switch (setting) {
      /* You can change the frequency of a mixer as long as it's not attached
       * to anything.
       */
      case ALLEGRO_AUDIOPROP_FREQUENCY:
         if (mixer->ss.parent.u.ptr) {
            _al_set_error(ALLEGRO_INVALID_OBJECT,
               "Attempted to change the frequency of an attached mixer");
            return 1;
         }
         mixer->ss.frequency = val;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to set invalid mixer long setting");
         return 1;
   }
}


/* Function: al_mixer_set_enum
 */
int al_mixer_set_enum(ALLEGRO_MIXER *mixer,
   ALLEGRO_AUDIO_PROPERTY setting, int val)
{
   ASSERT(mixer);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_QUALITY:
         if (val != ALLEGRO_MIXER_QUALITY_POINT &&
               val != ALLEGRO_MIXER_QUALITY_LINEAR)
         {
            _al_set_error(ALLEGRO_INVALID_PARAM,
               "Attempted to set unknown mixer quality");
            return 1;
         }
         mixer_change_quality(mixer, val);
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to set invalid mixer enum setting");
         return 1;
   }
}


/* mixer_change_quality:
 *  Change the mixer quality, updating the streams already attached to it.
 *  XXX this seems an unnecessary and perhaps dangerous function
 */
static void mixer_change_quality(ALLEGRO_MIXER *mixer,
   ALLEGRO_MIXER_QUALITY new_quality)
{
   int i;

   al_lock_mutex(mixer->ss.mutex);

   for (i = _al_vector_size(&mixer->streams) - 1; i >= 0; i--) {
      ALLEGRO_SAMPLE **slot = _al_vector_ref(&mixer->streams, i);
      ALLEGRO_SAMPLE *spl = *slot;

      if (new_quality == ALLEGRO_MIXER_QUALITY_LINEAR) {
         if (spl->spl_read == read_to_mixer_point32) {
            spl->spl_read = read_to_mixer_linear32;
         }
         else if (spl->spl_read == read_to_mixer_point32u) {
            spl->spl_read = read_to_mixer_linear32u;
         }
      }
      else if (new_quality == ALLEGRO_MIXER_QUALITY_POINT) {
         if (spl->spl_read == read_to_mixer_linear32) {
            spl->spl_read = read_to_mixer_point32;
         }
         else if (spl->spl_read == read_to_mixer_linear32u) {
            spl->spl_read = read_to_mixer_point32u;
         }
      }
   }

   mixer->quality = new_quality;

   al_unlock_mutex(mixer->ss.mutex);
}


/* Function: al_mixer_set_bool
 */
int al_mixer_set_bool(ALLEGRO_MIXER *mixer,
   ALLEGRO_AUDIO_PROPERTY setting, bool val)
{
   ASSERT(mixer);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_PLAYING:
         mixer->ss.is_playing = val;
         return 0;

      case ALLEGRO_AUDIOPROP_ATTACHED:
         if (val) {
            _al_set_error(ALLEGRO_INVALID_PARAM,
               "Attempted to set mixer attachment status true");
            return 1;
         }
         _al_kcm_detach_from_parent(&mixer->ss);
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to set invalid mixer bool setting");
         return 1;
   }
}


/* vim: set sts=3 sw=3 et: */
