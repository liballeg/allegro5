/**
 * Originally digi.c from allegro wiki
 * Original authors: KC/Milan
 *
 * Converted to allegro5 by Ryan Dickie
 */

/* Title: Mixer functions
 */

#include <math.h>
#include <stdio.h>

#include "allegro5/kcm_audio.h"
#include "allegro5/internal/aintern_kcm_audio.h"
#include "allegro5/internal/aintern_kcm_cfg.h"

ALLEGRO_DEBUG_CHANNEL("sound")

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
   ALLEGRO_CHANNEL_CONF target, float gain, float pan)
{
   /* Max 7.1 (8 channels) for input and output */
   static float mat[ALLEGRO_MAX_CHANNELS][ALLEGRO_MAX_CHANNELS];

   size_t dst_chans = al_get_channel_count(target);
   size_t src_chans = al_get_channel_count(orig);
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

   /* Apply panning, which is supposed to maintain a constant power level.
    * I took that to mean we want:
    *    sqrt(rgain^2 + lgain^2) = 1.0
    */
   if (pan != ALLEGRO_AUDIO_PAN_NONE) {
      float rgain = gain * sqrt(( pan + 1.0f) / 2.0f);
      float lgain = gain * sqrt((-pan + 1.0f) / 2.0f);

      /* I dunno what to do about >2 channels, so don't even try for now. */
      for (j = 0; j < src_chans; j++) {
         mat[0][j] *= lgain;
         mat[1][j] *= rgain;
      }
   }

   /* Apply gain */
   if (gain != 1.0f) {
      for (i = 0; i < dst_chans; i++) {
         for (j = 0; j < src_chans; j++) {
            mat[i][j] *= gain;
         }
      }
   }

   {
      char debug[1024];
      ALLEGRO_DEBUG("sample matrix:\n");
      for (i = 0; i < dst_chans; i++) {
         strcpy(debug, "");
         for (j = 0; j < src_chans; j++) {
            sprintf(debug + strlen(debug), " %f", mat[i][j]);
         }
         ALLEGRO_DEBUG("%s\n", debug);
      }
   }

   return &mat[0][0];
}


/* _al_kcm_mixer_rejig_sample_matrix:
 *  Recompute the mixing matrix for a sample attached to a mixer.
 *  The caller must be holding the mixer mutex.
 */
void _al_kcm_mixer_rejig_sample_matrix(ALLEGRO_MIXER *mixer,
   ALLEGRO_SAMPLE_INSTANCE *spl)
{
   float *mat;
   size_t dst_chans;
   size_t src_chans;
   size_t i, j;

   if (spl->matrix) {
      free(spl->matrix);
   }

   mat = _al_rechannel_matrix(spl->spl_data.chan_conf,
      mixer->ss.spl_data.chan_conf, spl->gain, spl->pan);

   dst_chans = al_get_channel_count(mixer->ss.spl_data.chan_conf);
   src_chans = al_get_channel_count(spl->spl_data.chan_conf);

   spl->matrix = calloc(1, src_chans * dst_chans * sizeof(float));

   for (i = 0; i < dst_chans; i++) {
      for (j = 0; j < src_chans; j++) {
         spl->matrix[i*src_chans + j] = mat[i*ALLEGRO_MAX_CHANNELS + j];
      }
   }
}


/* fix_looped_position:
 *  When a stream loops, this will fix up the position and anything else to
 *  allow it to safely continue playing as expected. Returns false if it
 *  should stop being mixed.
 */
static bool fix_looped_position(ALLEGRO_SAMPLE_INSTANCE *spl)
{
   bool is_empty;
   unsigned long count = 0;
   ALLEGRO_STREAM *stream;

   /* Looping! Should be mostly self-explanitory */
   switch (spl->loop) {
      case ALLEGRO_PLAYMODE_LOOP:
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
         return true;

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
         return true;

      case ALLEGRO_PLAYMODE_ONCE:
         if (spl->pos < spl->spl_data.len) {
            return true;
         }
         spl->pos = 0;
         spl->is_playing = false;
         return false;

      case _ALLEGRO_PLAYMODE_STREAM_ONCE:
      case _ALLEGRO_PLAYMODE_STREAM_ONEDIR:
         if (spl->pos < spl->spl_data.len) {
            return true;
         }
         stream = (ALLEGRO_STREAM *)spl;
         is_empty = !_al_kcm_refill_stream(stream);
         if (is_empty && stream->is_draining) {
            stream->spl.is_playing = false;
         }

         al_get_stream_long(stream, ALLEGRO_AUDIOPROP_USED_FRAGMENTS, &count);
         if (count)
            _al_kcm_emit_stream_event(stream, count);

         return !(is_empty);
   }

   ASSERT(false);
   return false;
}


static INLINE const float *point_spl32(const ALLEGRO_SAMPLE_INSTANCE *spl,
   unsigned int maxc)
{
   any_buffer_t *buf = (any_buffer_t *) &spl->spl_data.buffer;
   float *s = _samp_buf;
   unsigned int i;

   for (i = 0; i < maxc; i++) {
      switch (spl->spl_data.depth) {
         case ALLEGRO_AUDIO_DEPTH_INT24:
            s[i] = (float) buf->s24[(spl->pos>>MIXER_FRAC_SHIFT)*maxc + i]
               / ((float)0x7FFFFF + 0.5f);
            break;
         case ALLEGRO_AUDIO_DEPTH_INT16:
            s[i] = (float) buf->s16[(spl->pos>>MIXER_FRAC_SHIFT)*maxc + i]
               / ((float)0x7FFF + 0.5f);
            break;
         case ALLEGRO_AUDIO_DEPTH_INT8:
            s[i] = (float) buf->s8[(spl->pos>>MIXER_FRAC_SHIFT)*maxc + i]
               / ((float)0x7F + 0.5f);
            break;
         case ALLEGRO_AUDIO_DEPTH_FLOAT32:
            s[i] = buf->f32[(spl->pos>>MIXER_FRAC_SHIFT)*maxc + i];
            break;

         case ALLEGRO_AUDIO_DEPTH_UINT8:
         case ALLEGRO_AUDIO_DEPTH_UINT16:
         case ALLEGRO_AUDIO_DEPTH_UINT24:
            ASSERT(false);
            break;
      }
   }
   return s;
}


static INLINE const float *point_spl32u(const ALLEGRO_SAMPLE_INSTANCE *spl,
   unsigned int maxc)
{
   any_buffer_t *buf = (any_buffer_t *) &spl->spl_data.buffer;
   float *s = _samp_buf;
   unsigned int i;

   for (i = 0; i < maxc; i++) {
      switch (spl->spl_data.depth) {
         case ALLEGRO_AUDIO_DEPTH_UINT24:
            s[i] = (float)buf->u24[(spl->pos>>MIXER_FRAC_SHIFT)*maxc + i]
               / ((float)0x7FFFFF+0.5f) - 1.0;
            break;
         case ALLEGRO_AUDIO_DEPTH_UINT16:
            s[i] = (float)buf->u16[(spl->pos>>MIXER_FRAC_SHIFT)*maxc + i]
               / ((float)0x7FFF+0.5f) - 1.0;
            break;
         case ALLEGRO_AUDIO_DEPTH_UINT8:
            s[i] = (float)buf->u8[(spl->pos>>MIXER_FRAC_SHIFT)*maxc + i]
               / ((float)0x7F+0.5f) - 1.0;
            break;
         case ALLEGRO_AUDIO_DEPTH_FLOAT32:
            s[i] = buf->f32[(spl->pos>>MIXER_FRAC_SHIFT)*maxc + i] - 1.0;
            break;

         case ALLEGRO_AUDIO_DEPTH_INT8:
         case ALLEGRO_AUDIO_DEPTH_INT16:
         case ALLEGRO_AUDIO_DEPTH_INT24:
            ASSERT(false);
            break;
      }
   }
   return s;
}


static INLINE const float *linear_spl32(const ALLEGRO_SAMPLE_INSTANCE *spl,
   unsigned int maxc)
{
   unsigned long p1, p2;
   unsigned int i;
   float frac, *s = _samp_buf;

   p1 = (spl->pos>>MIXER_FRAC_SHIFT)*maxc;
   p2 = p1 + maxc;

   switch (spl->loop) {
      case ALLEGRO_PLAYMODE_ONCE:
      case _ALLEGRO_PLAYMODE_STREAM_ONCE:
      case _ALLEGRO_PLAYMODE_STREAM_ONEDIR:
         if (spl->pos+MIXER_FRAC_ONE >= spl->spl_data.len)
            p2 = p1;
         break;
      case ALLEGRO_PLAYMODE_LOOP:
         if (spl->pos+MIXER_FRAC_ONE >= spl->loop_end)
            p2 = (spl->loop_start>>MIXER_FRAC_SHIFT)*maxc;
         break;
      case ALLEGRO_PLAYMODE_BIDIR:
         if (spl->pos+MIXER_FRAC_ONE >= spl->loop_end)
            p2 = ((spl->loop_end>>MIXER_FRAC_SHIFT)-1)*maxc;
         break;
   }

   frac = (float)(spl->pos & MIXER_FRAC_MASK) / (float)MIXER_FRAC_ONE;

   for (i = 0; i < maxc; i++) {
      float s1 = 0, s2 = 0;

      switch (spl->spl_data.depth) {
         case ALLEGRO_AUDIO_DEPTH_INT24:
            s1 = (float)spl->spl_data.buffer.s24[p1 + i] / ((float)0x7FFFFF + 0.5f);
            s2 = (float)spl->spl_data.buffer.s24[p2 + i] / ((float)0x7FFFFF + 0.5f);
            break;
         case ALLEGRO_AUDIO_DEPTH_INT16:
            s1 = (float)spl->spl_data.buffer.s16[p1 + i] / ((float)0x7FFF + 0.5f);
            s2 = (float)spl->spl_data.buffer.s16[p2 + i] / ((float)0x7FFF + 0.5f);
            break;
         case ALLEGRO_AUDIO_DEPTH_INT8:
            s1 = (float)spl->spl_data.buffer.s8[p1 + i] / ((float)0x7F + 0.5f);
            s2 = (float)spl->spl_data.buffer.s8[p2 + i] / ((float)0x7F + 0.5f);
            break;
         case ALLEGRO_AUDIO_DEPTH_FLOAT32:
            s1 = (float)spl->spl_data.buffer.f32[p1 + i];
            s2 = (float)spl->spl_data.buffer.f32[p2 + i];
            break;

         case ALLEGRO_AUDIO_DEPTH_UINT8:
         case ALLEGRO_AUDIO_DEPTH_UINT16:
         case ALLEGRO_AUDIO_DEPTH_UINT24:
            ASSERT(false);
            break;
      }

      s[i] = (s1*(1.0f-frac)) + (s2*frac);
   }
   return s;
}


static INLINE const float *linear_spl32u(const ALLEGRO_SAMPLE_INSTANCE *spl,
   unsigned int maxc)
{
   unsigned long p1, p2;
   unsigned int i;
   float frac, *s = _samp_buf;

   p1 = (spl->pos>>MIXER_FRAC_SHIFT)*maxc;
   p2 = p1 + maxc;

   switch (spl->loop) {
      case ALLEGRO_PLAYMODE_ONCE:
      case _ALLEGRO_PLAYMODE_STREAM_ONCE:
      case _ALLEGRO_PLAYMODE_STREAM_ONEDIR:
         if (spl->pos+MIXER_FRAC_ONE >= spl->spl_data.len)
            p2 = p1;
         break;
      case ALLEGRO_PLAYMODE_LOOP:
         if (spl->pos+MIXER_FRAC_ONE >= spl->loop_end)
            p2 = (spl->loop_start>>MIXER_FRAC_SHIFT)*maxc;
         break;
      case ALLEGRO_PLAYMODE_BIDIR:
         if (spl->pos+MIXER_FRAC_ONE >= spl->loop_end)
            p2 = ((spl->loop_end>>MIXER_FRAC_SHIFT)-1)*maxc;
         break;
   }

   frac = (float)(spl->pos & MIXER_FRAC_MASK) / (float)MIXER_FRAC_ONE;

   for (i = 0; i < maxc; i++) {
      float s1 = 0, s2 = 0;

      switch (spl->spl_data.depth) {
         case ALLEGRO_AUDIO_DEPTH_UINT24:
            s1 = (float)spl->spl_data.buffer.u24[p1 + i]/((float)0x7FFFFF + 0.5f) - 1.0f;
            s2 = (float)spl->spl_data.buffer.u24[p2 + i]/((float)0x7FFFFF + 0.5f) - 1.0f;
            break;
         case ALLEGRO_AUDIO_DEPTH_UINT16:
            s1 = (float)spl->spl_data.buffer.u16[p1 + i]/((float)0x7FFF + 0.5f) - 1.0f;
            s2 = (float)spl->spl_data.buffer.u16[p2 + i]/((float)0x7FFF + 0.5f) - 1.0f;
            break;
         case ALLEGRO_AUDIO_DEPTH_UINT8:
            s1 = (float)spl->spl_data.buffer.u8[p1 + i]/((float)0x7F + 0.5f) - 1.0f;
            s2 = (float)spl->spl_data.buffer.u8[p2 + i]/((float)0x7F + 0.5f) - 1.0f;
            break;
         case ALLEGRO_AUDIO_DEPTH_FLOAT32:
            s1 = (float)spl->spl_data.buffer.f32[p1 + i] - 1.0f;
            s2 = (float)spl->spl_data.buffer.f32[p2 + i] - 1.0f;
            break;

         case ALLEGRO_AUDIO_DEPTH_INT8:
         case ALLEGRO_AUDIO_DEPTH_INT16:
         case ALLEGRO_AUDIO_DEPTH_INT24:
            ASSERT(false);
            break;
      }

      s[i] = (s1*(1.0f-frac)) + (s2*frac);
   }
   return s;
}


/* Reads some samples into a mixer buffer. */
#define MAKE_MIXER(bits, interp)                                              \
static void read_to_mixer_##interp##bits(void *source, void **vbuf,           \
   unsigned long *samples, ALLEGRO_AUDIO_DEPTH buffer_depth,                  \
   size_t dest_maxc)                                                          \
{                                                                             \
   ALLEGRO_SAMPLE_INSTANCE *spl = (ALLEGRO_SAMPLE_INSTANCE *)source;                            \
   float *buf = *vbuf;                                                        \
   size_t maxc = al_get_channel_count(spl->spl_data.chan_conf);                   \
   size_t samples_l = *samples;                                               \
   size_t c;                                                                  \
                                                                              \
   if (!spl->is_playing)                                                      \
      return;                                                                 \
                                                                              \
   while (samples_l > 0) {                                                    \
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
      samples_l--;                                                            \
   }                                                                          \
   fix_looped_position(spl);                                                  \
   (void)buffer_depth;                                                        \
}

MAKE_MIXER(32,  point)
MAKE_MIXER(32u, point)
MAKE_MIXER(32,  linear)
MAKE_MIXER(32u, linear)

#undef MAKE_MIXER


static INLINE int32_t clamp(int32_t val, int32_t min, int32_t max)
{
   /* Clamp to min */
   val -= min;
   val &= (~val) >> 31;
   val += min;

   /* Clamp to max */
   val -= max;
   val &= val >> 31;
   val += max;

   return val;
}


/* _al_kcm_mixer_read:
 *  Mixes the streams attached to the mixer and writes additively to the
 *  specified buffer (or if *buf is NULL, indicating a voice, convert it and
 *  set it to the buffer pointer).
 */
void _al_kcm_mixer_read(void *source, void **buf, unsigned long *samples,
   ALLEGRO_AUDIO_DEPTH buffer_depth, size_t dest_maxc)
{
   const ALLEGRO_MIXER *mixer;
   ALLEGRO_MIXER *m = (ALLEGRO_MIXER *)source;
   int maxc = al_get_channel_count(m->ss.spl_data.chan_conf);
   unsigned long samples_l = *samples;
   int i;

   if (!m->ss.is_playing)
      return;

   /* Make sure the mixer buffer is big enough. */
   if (m->ss.spl_data.len*maxc < samples_l*maxc) {
      free(m->ss.spl_data.buffer.ptr);
      m->ss.spl_data.buffer.ptr = malloc(samples_l*maxc*sizeof(float));
      if (!m->ss.spl_data.buffer.ptr) {
         _al_set_error(ALLEGRO_GENERIC_ERROR,
            "Out of memory allocating mixer buffer");
         m->ss.spl_data.len = 0;
         return;
      }
      m->ss.spl_data.len = samples_l;
   }

   mixer = m;

   /* Clear the buffer to silence. */
   memset(mixer->ss.spl_data.buffer.ptr, 0, samples_l * maxc * sizeof(float));

   /* Mix the streams into the mixer buffer. */
   for (i = _al_vector_size(&mixer->streams) - 1; i >= 0; i--) {
      ALLEGRO_SAMPLE_INSTANCE **slot = _al_vector_ref(&mixer->streams, i);
      ALLEGRO_SAMPLE_INSTANCE *spl = *slot;
      spl->spl_read(spl, (void **) &mixer->ss.spl_data.buffer.ptr, samples,
         ALLEGRO_AUDIO_DEPTH_FLOAT32, maxc);
   }

   /* Call the post-processing callback. */
   if (mixer->postprocess_callback) {
      mixer->postprocess_callback(mixer->ss.spl_data.buffer.ptr,
         mixer->ss.spl_data.len, mixer->pp_callback_userdata);
   }

   samples_l *= maxc;

   if (*buf) {
      /* We don't need to clamp in the mixer yet. */
      float *lbuf = *buf;
      float *src = mixer->ss.spl_data.buffer.f32;

      while (samples_l > 0) {
         *(lbuf++) += *(src++);
         samples_l--;
      }
      return;
   }

   /* We're feeding to a voice, so we pass it back the mixed data (make sure
    * to clamp and convert it).
    */
   *buf = mixer->ss.spl_data.buffer.ptr;
   switch (buffer_depth & ~ALLEGRO_AUDIO_DEPTH_UNSIGNED) {
      case ALLEGRO_AUDIO_DEPTH_INT24: {
         int32_t off = ((buffer_depth & ALLEGRO_AUDIO_DEPTH_UNSIGNED)
                        ? 0x800000 : 0);
         int32_t *lbuf = mixer->ss.spl_data.buffer.s24;
         float *src = mixer->ss.spl_data.buffer.f32;

         while (samples_l > 0) {
            *lbuf = clamp(*(src++) * ((float)0x7FFFFF + 0.5f),
               ~0x7FFFFF, 0x7FFFFF);
            *lbuf += off;
            lbuf++;
            samples_l--;
         }
         break;
      }

      case ALLEGRO_AUDIO_DEPTH_INT16: {
         int16_t off = ((buffer_depth & ALLEGRO_AUDIO_DEPTH_UNSIGNED)
                        ? 0x8000 : 0);
         int16_t *lbuf = mixer->ss.spl_data.buffer.s16;
         float *src = mixer->ss.spl_data.buffer.f32;

         while (samples_l > 0) {
            *lbuf = clamp(*(src++) * ((float)0x7FFF + 0.5f), ~0x7FFF, 0x7FFF);
            *lbuf += off;
            lbuf++;
            samples_l--;
         }
         break;
      }

      case ALLEGRO_AUDIO_DEPTH_INT8: {
         int8_t off = ((buffer_depth & ALLEGRO_AUDIO_DEPTH_UNSIGNED)
                        ? 0x80 : 0);
         int8_t *lbuf = mixer->ss.spl_data.buffer.s8;
         float *src = mixer->ss.spl_data.buffer.f32;

         while (samples_l > 0) {
            *lbuf = clamp(*(src++) * ((float)0x7F + 0.5f), ~0x7F, 0x7F);
            *lbuf += off;
            lbuf++;
            samples_l--;
         }
         break;
      }

      case ALLEGRO_AUDIO_DEPTH_UINT8:
      case ALLEGRO_AUDIO_DEPTH_UINT16:
      case ALLEGRO_AUDIO_DEPTH_UINT24:
         ASSERT(false);
         break;
   }

   (void)dest_maxc;
}


/* Function: al_create_mixer
 */
ALLEGRO_MIXER *al_create_mixer(unsigned long freq,
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
   mixer->ss.spl_data.free_buf = true;

   mixer->ss.loop = ALLEGRO_PLAYMODE_ONCE;
   /* XXX should we have a specific loop mode? */
   mixer->ss.gain = 1.0f;
   mixer->ss.spl_data.depth     = depth;
   mixer->ss.spl_data.chan_conf = chan_conf;
   mixer->ss.spl_data.frequency = freq;

   mixer->ss.spl_read = _al_kcm_mixer_read;

   mixer->quality = ALLEGRO_MIXER_QUALITY_LINEAR;

   _al_vector_init(&mixer->streams, sizeof(ALLEGRO_SAMPLE_INSTANCE *));

   _al_kcm_register_destructor(mixer, (void (*)(void *)) al_destroy_mixer);

   return mixer;
}


/* Function: al_destroy_mixer
 */
void al_destroy_mixer(ALLEGRO_MIXER *mixer)
{
   if (mixer) {
      _al_kcm_unregister_destructor(mixer);
      _al_kcm_destroy_sample(&mixer->ss, false);
   }
}


/* This function is ALLEGRO_MIXER aware */
/* Function: al_attach_sample_to_mixer
 */
int al_attach_sample_to_mixer(ALLEGRO_MIXER *mixer, ALLEGRO_SAMPLE_INSTANCE *spl)
{
   ALLEGRO_SAMPLE_INSTANCE **slot;

   ASSERT(mixer);
   ASSERT(spl);

   /* Already referenced, do not attach. */
   if (spl->parent.u.ptr) {
      _al_set_error(ALLEGRO_INVALID_OBJECT,
         "Attempted to attach a sample that's already attached");
      return 1;
   }

   if (mixer->ss.mutex) {
      al_lock_mutex(mixer->ss.mutex);
   }

   slot = _al_vector_alloc_back(&mixer->streams);
   if (!slot) {
      if (mixer->ss.mutex) {
         al_unlock_mutex(mixer->ss.mutex);
      }
      _al_set_error(ALLEGRO_GENERIC_ERROR,
         "Out of memory allocating attachment pointers");
      return 1;
   }
   (*slot) = spl;

   spl->step = (spl->spl_data.frequency<<MIXER_FRAC_SHIFT) * spl->speed /
               mixer->ss.spl_data.frequency;
   /* Don't want to be trapped with a step value of 0. */
   if (spl->step == 0) {
      if (spl->speed > 0.0f)
         spl->step = 1;
      else
         spl->step = -1;
   }

   /* If this isn't a mixer, set the proper sample stream reader */
   if (spl->spl_read == NULL) {
      switch (mixer->quality) {
         case ALLEGRO_MIXER_QUALITY_LINEAR:
            if ((spl->spl_data.depth & ALLEGRO_AUDIO_DEPTH_UNSIGNED))
               spl->spl_read = read_to_mixer_linear32u;
            else
               spl->spl_read = read_to_mixer_linear32;
            break;
         case ALLEGRO_MIXER_QUALITY_POINT:
            if ((spl->spl_data.depth & ALLEGRO_AUDIO_DEPTH_UNSIGNED))
               spl->spl_read = read_to_mixer_point32u;
            else
               spl->spl_read = read_to_mixer_point32;
            break;
      }

      _al_kcm_mixer_rejig_sample_matrix(mixer, spl);
   }

   _al_kcm_stream_set_mutex(spl, mixer->ss.mutex);
   
   spl->parent.u.mixer = mixer;
   spl->parent.is_voice = false;

   if (mixer->ss.mutex) {
      al_unlock_mutex(mixer->ss.mutex);
   }

   return 0;
}


/* Function: al_attach_stream_to_mixer
 */
int al_attach_stream_to_mixer(ALLEGRO_MIXER *mixer, ALLEGRO_STREAM *stream)
{
   ASSERT(mixer);
   ASSERT(stream);

   return al_attach_sample_to_mixer(mixer, &stream->spl);
}


/* Function: al_attach_mixer_to_mixer
 */
int al_attach_mixer_to_mixer(ALLEGRO_MIXER *mixer, ALLEGRO_MIXER *stream)
{
   ASSERT(mixer);
   ASSERT(stream);

   if (mixer->ss.spl_data.frequency != stream->ss.spl_data.frequency) {
      _al_set_error(ALLEGRO_INVALID_OBJECT,
         "Attempted to attach a mixer with different frequencies");
      return 1;
   }

   return al_attach_sample_to_mixer(mixer, &stream->ss);
}


/* Function: al_mixer_set_postprocess_callback
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


/* Function: al_get_mixer_long
 */
int al_get_mixer_long(const ALLEGRO_MIXER *mixer,
   ALLEGRO_AUDIO_PROPERTY setting, unsigned long *val)
{
   ASSERT(mixer);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_FREQUENCY:
         *val = mixer->ss.spl_data.frequency;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to get invalid mixer long setting");
         return 1;
   }
}


/* Function: al_get_mixer_enum
 */
int al_get_mixer_enum(const ALLEGRO_MIXER *mixer,
   ALLEGRO_AUDIO_PROPERTY setting, int *val)
{
   ASSERT(mixer);

   switch (setting) {
      case ALLEGRO_AUDIOPROP_CHANNELS:
         *val = mixer->ss.spl_data.chan_conf;
         return 0;

      case ALLEGRO_AUDIOPROP_DEPTH:
         *val = mixer->ss.spl_data.depth;
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


/* Function: al_get_mixer_bool
 */
int al_get_mixer_bool(const ALLEGRO_MIXER *mixer,
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


/* Function: al_set_mixer_long
 */
int al_set_mixer_long(ALLEGRO_MIXER *mixer,
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
         mixer->ss.spl_data.frequency = val;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM,
            "Attempted to set invalid mixer long setting");
         return 1;
   }
}


/* Function: al_set_mixer_enum
 */
int al_set_mixer_enum(ALLEGRO_MIXER *mixer,
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
      ALLEGRO_SAMPLE_INSTANCE **slot = _al_vector_ref(&mixer->streams, i);
      ALLEGRO_SAMPLE_INSTANCE *spl = *slot;

      switch (new_quality) {
         case ALLEGRO_MIXER_QUALITY_LINEAR:
            if (spl->spl_read == read_to_mixer_point32) {
               spl->spl_read = read_to_mixer_linear32;
            }
            else if (spl->spl_read == read_to_mixer_point32u) {
               spl->spl_read = read_to_mixer_linear32u;
            }
            break;

         case ALLEGRO_MIXER_QUALITY_POINT:
            if (spl->spl_read == read_to_mixer_linear32) {
               spl->spl_read = read_to_mixer_point32;
            }
            else if (spl->spl_read == read_to_mixer_linear32u) {
               spl->spl_read = read_to_mixer_point32u;
            }
            break;
      }
   }

   mixer->quality = new_quality;

   al_unlock_mutex(mixer->ss.mutex);
}


/* Function: al_set_mixer_bool
 */
int al_set_mixer_bool(ALLEGRO_MIXER *mixer,
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
