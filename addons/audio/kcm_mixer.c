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

#include "allegro5/allegro_audio.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_audio.h"
#include "allegro5/internal/aintern_audio_cfg.h"

ALLEGRO_DEBUG_CHANNEL("audio")


typedef union {
   float f32[ALLEGRO_MAX_CHANNELS]; /* max: 7.1 */
   int16_t s16[ALLEGRO_MAX_CHANNELS];
   void *ptr;
} SAMP_BUF;



static void maybe_lock_mutex(ALLEGRO_MUTEX *mutex)
{
   if (mutex) {
      al_lock_mutex(mutex);
   }
}


static void maybe_unlock_mutex(ALLEGRO_MUTEX *mutex)
{
   if (mutex) {
      al_unlock_mutex(mutex);
   }
}


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
      float rgain = sqrt(( pan + 1.0f) / 2.0f);
      float lgain = sqrt((-pan + 1.0f) / 2.0f);

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

#ifdef DEBUGMODE
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
#endif

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

   mat = _al_rechannel_matrix(spl->spl_data.chan_conf,
      mixer->ss.spl_data.chan_conf, spl->gain, spl->pan);

   dst_chans = al_get_channel_count(mixer->ss.spl_data.chan_conf);
   src_chans = al_get_channel_count(spl->spl_data.chan_conf);

   if (!spl->matrix)
      spl->matrix = al_calloc(1, src_chans * dst_chans * sizeof(float));

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
   ALLEGRO_AUDIO_STREAM *stream;

   /* Looping! Should be mostly self-explanatory */
   switch (spl->loop) {
      case ALLEGRO_PLAYMODE_LOOP:
         if (spl->loop_end - spl->loop_start != 0) {
            if (spl->step > 0) {
               while (spl->pos >= spl->loop_end) {
                  spl->pos -= (spl->loop_end - spl->loop_start);
               }
            }
            else if (spl->step < 0) {
               while (spl->pos < spl->loop_start) {
                  spl->pos += (spl->loop_end - spl->loop_start);
               }
            }
         }
         return true;

      case ALLEGRO_PLAYMODE_BIDIR:
         /* When doing bi-directional looping, you need to do a follow-up
          * check for the opposite direction if a loop occurred, otherwise
          * you could end up misplaced on small, high-step loops.
          */
         if (spl->loop_end - spl->loop_start != 0) {
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
         }
         return true;

      case ALLEGRO_PLAYMODE_LOOP_ONCE:
         if (spl->pos < spl->loop_end && spl->pos >= 0) {
            return true;
         }
         if (spl->step >= 0)
            spl->pos = 0;
         else
            spl->pos = spl->loop_end - 1;
         spl->is_playing = false;
         return false;

      case ALLEGRO_PLAYMODE_ONCE:
         if (spl->pos < spl->spl_data.len && spl->pos >= 0) {
            return true;
         }
         if (spl->step >= 0)
            spl->pos = 0;
         else
            spl->pos = spl->spl_data.len - 1;
         spl->is_playing = false;
         return false;

      case _ALLEGRO_PLAYMODE_STREAM_ONCE:
      case _ALLEGRO_PLAYMODE_STREAM_LOOP_ONCE:
      case _ALLEGRO_PLAYMODE_STREAM_ONEDIR:
         stream = (ALLEGRO_AUDIO_STREAM *)spl;
         is_empty = false;
         while (spl->pos >= spl->spl_data.len && stream->spl.is_playing && !is_empty) {
            is_empty = !_al_kcm_refill_stream(stream);
            if (is_empty && stream->is_draining) {
               stream->spl.is_playing = false;
            }

            _al_kcm_emit_stream_events(stream);
         }
         return !(is_empty);
   }

   ASSERT(false);
   return false;
}


#include "kcm_mixer_helpers.inc"


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


/* Mix as many sample values as possible from the source sample into a mixer
 * buffer.  Implements stream_reader_t.
 *
 * TYPE is the type of the sample values in the mixer buffer, and
 * NEXT_SAMPLE_VALUE must return a buffer of the same type.
 *
 * Note: Uses Bresenham to keep the precise sample position.
 */
#define BRESENHAM                                                             \
   do {                                                                       \
      delta = spl->step > 0 ? spl->step : spl->step - spl->step_denom + 1;    \
      delta /= spl->step_denom;                                               \
      delta_error = spl->step - delta * spl->step_denom;                      \
   } while (0)

#define MAKE_MIXER(NAME, NEXT_SAMPLE_VALUE, TYPE)                             \
static void NAME(void *source, void **vbuf, unsigned int *samples,            \
   ALLEGRO_AUDIO_DEPTH buffer_depth, size_t dest_maxc)                        \
{                                                                             \
   ALLEGRO_SAMPLE_INSTANCE *spl = (ALLEGRO_SAMPLE_INSTANCE *)source;          \
   TYPE *buf = *vbuf;                                                         \
   size_t maxc = al_get_channel_count(spl->spl_data.chan_conf);               \
   size_t samples_l = *samples;                                               \
   size_t c;                                                                  \
   int delta, delta_error;                                                    \
   SAMP_BUF samp_buf;                                                         \
                                                                              \
   BRESENHAM;                                                                 \
                                                                              \
   if (!spl->is_playing)                                                      \
      return;                                                                 \
                                                                              \
   while (samples_l > 0) {                                                    \
      const TYPE *s;                                                          \
      int old_step = spl->step;                                               \
                                                                              \
      if (!fix_looped_position(spl))                                          \
         return;                                                              \
      if (old_step != spl->step) {                                            \
         BRESENHAM;                                                           \
      }                                                                       \
                                                                              \
      /* It might be worth preparing multiple sample values at once. */       \
      s = (TYPE *) NEXT_SAMPLE_VALUE(&samp_buf, spl, maxc);                   \
                                                                              \
      for (c = 0; c < dest_maxc; c++) {                                       \
         ALLEGRO_STATIC_ASSERT(kcm_mixer, ALLEGRO_MAX_CHANNELS == 8);         \
         switch (maxc) {                                                      \
            case 8: *buf += s[7] * spl->matrix[c*maxc + 7];                   \
            /* fall through */                                                \
            case 7: *buf += s[6] * spl->matrix[c*maxc + 6];                   \
            /* fall through */                                                \
            case 6: *buf += s[5] * spl->matrix[c*maxc + 5];                   \
            /* fall through */                                                \
            case 5: *buf += s[4] * spl->matrix[c*maxc + 4];                   \
            /* fall through */                                                \
            case 4: *buf += s[3] * spl->matrix[c*maxc + 3];                   \
            /* fall through */                                                \
            case 3: *buf += s[2] * spl->matrix[c*maxc + 2];                   \
            /* fall through */                                                \
            case 2: *buf += s[1] * spl->matrix[c*maxc + 1];                   \
            /* fall through */                                                \
            case 1: *buf += s[0] * spl->matrix[c*maxc + 0];                   \
            /* fall through */                                                \
            default: break;                                                   \
         }                                                                    \
         buf++;                                                               \
      }                                                                       \
                                                                              \
      spl->pos += delta;                                                      \
      spl->pos_bresenham_error += delta_error;                                \
      if (spl->pos_bresenham_error >= spl->step_denom) {                      \
         spl->pos++;                                                          \
         spl->pos_bresenham_error -= spl->step_denom;                         \
      }                                                                       \
      samples_l--;                                                            \
   }                                                                          \
   fix_looped_position(spl);                                                  \
   (void)buffer_depth;                                                        \
}

MAKE_MIXER(read_to_mixer_point_float_32, point_spl32, float)
MAKE_MIXER(read_to_mixer_linear_float_32, linear_spl32, float)
MAKE_MIXER(read_to_mixer_cubic_float_32, cubic_spl32, float)
MAKE_MIXER(read_to_mixer_point_int16_t_16, point_spl16, int16_t)
MAKE_MIXER(read_to_mixer_linear_int16_t_16, linear_spl16, int16_t)

#undef MAKE_MIXER


/* _al_kcm_mixer_read:
 *  Mixes the streams attached to the mixer and writes additively to the
 *  specified buffer (or if *buf is NULL, indicating a voice, convert it and
 *  set it to the buffer pointer).
 */
void _al_kcm_mixer_read(void *source, void **buf, unsigned int *samples,
   ALLEGRO_AUDIO_DEPTH buffer_depth, size_t dest_maxc)
{
   const ALLEGRO_MIXER *mixer;
   ALLEGRO_MIXER *m = (ALLEGRO_MIXER *)source;
   int maxc = al_get_channel_count(m->ss.spl_data.chan_conf);
   int samples_l = *samples;
   int i;

   if (!m->ss.is_playing)
      return;

   /* Make sure the mixer buffer is big enough. */
   if (m->ss.spl_data.len*maxc < samples_l*maxc) {
      al_free(m->ss.spl_data.buffer.ptr);
      m->ss.spl_data.buffer.ptr = al_malloc(samples_l*maxc*al_get_audio_depth_size(m->ss.spl_data.depth));
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
   memset(mixer->ss.spl_data.buffer.ptr, 0, samples_l * maxc * al_get_audio_depth_size(mixer->ss.spl_data.depth));

   /* Mix the streams into the mixer buffer. */
   for (i = _al_vector_size(&mixer->streams) - 1; i >= 0; i--) {
      ALLEGRO_SAMPLE_INSTANCE **slot = _al_vector_ref(&mixer->streams, i);
      ALLEGRO_SAMPLE_INSTANCE *spl = *slot;
      ASSERT(spl->spl_read);
      spl->spl_read(spl, (void **) &mixer->ss.spl_data.buffer.ptr, samples,
         m->ss.spl_data.depth, maxc);
   }

   /* Call the post-processing callback. */
   if (mixer->postprocess_callback) {
      mixer->postprocess_callback(mixer->ss.spl_data.buffer.ptr,
         *samples, mixer->pp_callback_userdata);
   }

   samples_l *= maxc;

   /* Apply the gain if necessary. */
   if (mixer->ss.gain != 1.0f) {
      float mixer_gain = mixer->ss.gain;
      unsigned long i = samples_l;

      switch (m->ss.spl_data.depth) {
         case ALLEGRO_AUDIO_DEPTH_FLOAT32: {
            float *p = mixer->ss.spl_data.buffer.f32;
            while (i-- > 0) {
               *p++ *= mixer_gain;
            }
            break;
         }

         case ALLEGRO_AUDIO_DEPTH_INT16: {
            int16_t *p = mixer->ss.spl_data.buffer.s16;
            while (i-- > 0) {
               *p++ *= mixer_gain;
            }
            break;
         }

         case ALLEGRO_AUDIO_DEPTH_INT8:
         case ALLEGRO_AUDIO_DEPTH_INT24:
         case ALLEGRO_AUDIO_DEPTH_UINT8:
         case ALLEGRO_AUDIO_DEPTH_UINT16:
         case ALLEGRO_AUDIO_DEPTH_UINT24:
            /* Unsupported mixer depths. */
            ASSERT(false);
            break;
      }
   }

   /* Feeding to a non-voice.
    * Currently we only support mixers of the same audio depth doing this.
    */
   if (*buf) {
      switch (m->ss.spl_data.depth) {
         case ALLEGRO_AUDIO_DEPTH_FLOAT32: {
            /* We don't need to clamp in the mixer yet. */
            float *lbuf = *buf;
            float *src = mixer->ss.spl_data.buffer.f32;
            while (samples_l-- > 0) {
               *lbuf += *src;
               lbuf++;
               src++;
            }
            break;

         case ALLEGRO_AUDIO_DEPTH_INT16: {
            int16_t *lbuf = *buf;
            int16_t *src = mixer->ss.spl_data.buffer.s16;
            while (samples_l-- > 0) {
               int32_t x = *lbuf + *src;
               if (x < -32768)
                  x = -32768;
               else if (x > 32767)
                  x = 32767;
               *lbuf = (int16_t)x;
               lbuf++;
               src++;
            }
            break;
         }

         case ALLEGRO_AUDIO_DEPTH_INT8:
         case ALLEGRO_AUDIO_DEPTH_INT24:
         case ALLEGRO_AUDIO_DEPTH_UINT8:
         case ALLEGRO_AUDIO_DEPTH_UINT16:
         case ALLEGRO_AUDIO_DEPTH_UINT24:
            /* Unsupported mixer depths. */
            ASSERT(false);
            break;
         }
      }
      return;
   }

   /* We're feeding to a voice.
    * Clamp and convert the mixed data for the voice.
    */
   *buf = mixer->ss.spl_data.buffer.ptr;
   switch (buffer_depth & ~ALLEGRO_AUDIO_DEPTH_UNSIGNED) {

      case ALLEGRO_AUDIO_DEPTH_FLOAT32:
         /* Do we need to clamp? */
         break;

      case ALLEGRO_AUDIO_DEPTH_INT24:
         switch (mixer->ss.spl_data.depth) {
            case ALLEGRO_AUDIO_DEPTH_FLOAT32: {
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

            case ALLEGRO_AUDIO_DEPTH_INT16:
               /* XXX not yet implemented */
               ASSERT(false);
               break;

            case ALLEGRO_AUDIO_DEPTH_INT8:
            case ALLEGRO_AUDIO_DEPTH_INT24:
            case ALLEGRO_AUDIO_DEPTH_UINT8:
            case ALLEGRO_AUDIO_DEPTH_UINT16:
            case ALLEGRO_AUDIO_DEPTH_UINT24:
               /* Unsupported mixer depths. */
               ASSERT(false);
               break;
         }
         break;

      case ALLEGRO_AUDIO_DEPTH_INT16:
         switch (mixer->ss.spl_data.depth) {
            case ALLEGRO_AUDIO_DEPTH_FLOAT32: {
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

            case ALLEGRO_AUDIO_DEPTH_INT16:
               /* Handle signedness differences. */
               if (buffer_depth != ALLEGRO_AUDIO_DEPTH_INT16) {
                  int16_t *lbuf = mixer->ss.spl_data.buffer.s16;
                  while (samples_l > 0) {
                     *lbuf++ ^= 0x8000;
                     samples_l--;
                  }
               }
               break;

            case ALLEGRO_AUDIO_DEPTH_INT8:
            case ALLEGRO_AUDIO_DEPTH_INT24:
            case ALLEGRO_AUDIO_DEPTH_UINT8:
            case ALLEGRO_AUDIO_DEPTH_UINT16:
            case ALLEGRO_AUDIO_DEPTH_UINT24:
               /* Unsupported mixer depths. */
               ASSERT(false);
               break;
         }
         break;

      /* Ugh, do we really want to support 8-bit output? */
      case ALLEGRO_AUDIO_DEPTH_INT8:
         switch (mixer->ss.spl_data.depth) {
            case ALLEGRO_AUDIO_DEPTH_FLOAT32: {
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

            case ALLEGRO_AUDIO_DEPTH_INT16:
               /* XXX not yet implemented */
               ASSERT(false);
               break;

            case ALLEGRO_AUDIO_DEPTH_INT8:
            case ALLEGRO_AUDIO_DEPTH_INT24:
            case ALLEGRO_AUDIO_DEPTH_UINT8:
            case ALLEGRO_AUDIO_DEPTH_UINT16:
            case ALLEGRO_AUDIO_DEPTH_UINT24:
               /* Unsupported mixer depths. */
               ASSERT(false);
               break;
         }
         break;

      case ALLEGRO_AUDIO_DEPTH_UINT8:
      case ALLEGRO_AUDIO_DEPTH_UINT16:
      case ALLEGRO_AUDIO_DEPTH_UINT24:
         /* Impossible. */
         ASSERT(false);
         break;
   }

   (void)dest_maxc;
}


/* Function: al_create_mixer
 */
ALLEGRO_MIXER *al_create_mixer(unsigned int freq,
   ALLEGRO_AUDIO_DEPTH depth, ALLEGRO_CHANNEL_CONF chan_conf)
{
   ALLEGRO_MIXER *mixer;
   int default_mixer_quality = ALLEGRO_MIXER_QUALITY_LINEAR;
   const char *p;

   /* XXX this is in the wrong place */
   p = al_get_config_value(al_get_system_config(), "audio",
      "default_mixer_quality");
   if (p && p[0] != '\0') {
      if (!_al_stricmp(p, "point")) {
         ALLEGRO_INFO("Point sampling\n");
         default_mixer_quality = ALLEGRO_MIXER_QUALITY_POINT;
      }
      else if (!_al_stricmp(p, "linear")) {
         ALLEGRO_INFO("Linear interpolation\n");
         default_mixer_quality = ALLEGRO_MIXER_QUALITY_LINEAR;
      }
      else if (!_al_stricmp(p, "cubic")) {
         ALLEGRO_INFO("Cubic interpolation\n");
         default_mixer_quality = ALLEGRO_MIXER_QUALITY_CUBIC;
      }
   }

   if (!freq) {
      _al_set_error(ALLEGRO_INVALID_PARAM,
         "Attempted to create mixer with no frequency");
      return NULL;
   }

   if (depth != ALLEGRO_AUDIO_DEPTH_FLOAT32 &&
         depth != ALLEGRO_AUDIO_DEPTH_INT16) {
      _al_set_error(ALLEGRO_INVALID_PARAM, "Unsupported mixer depth");
      return NULL;
   }

   mixer = al_calloc(1, sizeof(ALLEGRO_MIXER));
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

   mixer->ss.is_mixer = true;
   mixer->ss.spl_read = NULL;

   mixer->quality = default_mixer_quality;

   _al_vector_init(&mixer->streams, sizeof(ALLEGRO_SAMPLE_INSTANCE *));

   mixer->dtor_item = _al_kcm_register_destructor("mixer", mixer, (void (*)(void *)) al_destroy_mixer);

   return mixer;
}


/* Function: al_destroy_mixer
 */
void al_destroy_mixer(ALLEGRO_MIXER *mixer)
{
   if (mixer) {
      _al_kcm_unregister_destructor(mixer->dtor_item);
      _al_kcm_destroy_sample(&mixer->ss, false);
   }
}


/* This function is ALLEGRO_MIXER aware */
/* Function: al_attach_sample_instance_to_mixer
 */
bool al_attach_sample_instance_to_mixer(ALLEGRO_SAMPLE_INSTANCE *spl,
   ALLEGRO_MIXER *mixer)
{
   ALLEGRO_SAMPLE_INSTANCE **slot;

   ASSERT(mixer);
   ASSERT(spl);

   /* Already referenced, do not attach. */
   if (spl->parent.u.ptr) {
      _al_set_error(ALLEGRO_INVALID_OBJECT,
         "Attempted to attach a sample that's already attached");
      return false;
   }

   maybe_lock_mutex(mixer->ss.mutex);

   _al_kcm_stream_set_mutex(spl, mixer->ss.mutex);

   slot = _al_vector_alloc_back(&mixer->streams);
   if (!slot) {
      if (mixer->ss.mutex) {
         al_unlock_mutex(mixer->ss.mutex);
      }
      _al_set_error(ALLEGRO_GENERIC_ERROR,
         "Out of memory allocating attachment pointers");
      return false;
   }
   (*slot) = spl;

   spl->step = (spl->spl_data.frequency) * spl->speed;
   spl->step_denom = mixer->ss.spl_data.frequency;
   /* Don't want to be trapped with a step value of 0. */
   if (spl->step == 0) {
      if (spl->speed > 0.0f)
         spl->step = 1;
      else
         spl->step = -1;
   }

   /* Set the proper sample stream reader. */
   ASSERT(spl->spl_read == NULL);
   if (spl->is_mixer) {
      spl->spl_read = _al_kcm_mixer_read;
   }
   else {
      switch (mixer->ss.spl_data.depth) {
         case ALLEGRO_AUDIO_DEPTH_FLOAT32:
            switch (mixer->quality) {
               case ALLEGRO_MIXER_QUALITY_POINT:
                  spl->spl_read = read_to_mixer_point_float_32;
                  break;
               case ALLEGRO_MIXER_QUALITY_LINEAR:
                  spl->spl_read = read_to_mixer_linear_float_32;
                  break;
               case ALLEGRO_MIXER_QUALITY_CUBIC:
                  spl->spl_read = read_to_mixer_cubic_float_32;
                  break;
            }
            break;

         case ALLEGRO_AUDIO_DEPTH_INT16:
            switch (mixer->quality) {
               case ALLEGRO_MIXER_QUALITY_POINT:
                  spl->spl_read = read_to_mixer_point_int16_t_16;
                  break;
               case ALLEGRO_MIXER_QUALITY_CUBIC:
                  ALLEGRO_WARN("Falling back to linear interpolation\n");
                  /* fallthrough */
               case ALLEGRO_MIXER_QUALITY_LINEAR:
                  spl->spl_read = read_to_mixer_linear_int16_t_16;
                  break;
            }
            break;

         case ALLEGRO_AUDIO_DEPTH_INT8:
         case ALLEGRO_AUDIO_DEPTH_INT24:
         case ALLEGRO_AUDIO_DEPTH_UINT8:
         case ALLEGRO_AUDIO_DEPTH_UINT16:
         case ALLEGRO_AUDIO_DEPTH_UINT24:
            /* Unsupported mixer depths. */
            ASSERT(false);
            break;
      }

      _al_kcm_mixer_rejig_sample_matrix(mixer, spl);
   }

   spl->parent.u.mixer = mixer;
   spl->parent.is_voice = false;

   maybe_unlock_mutex(mixer->ss.mutex);

   return true;
}


/* Function: al_attach_audio_stream_to_mixer
 */
bool al_attach_audio_stream_to_mixer(ALLEGRO_AUDIO_STREAM *stream, ALLEGRO_MIXER *mixer)
{
   ASSERT(mixer);
   ASSERT(stream);

   return al_attach_sample_instance_to_mixer(&stream->spl, mixer);
}


/* Function: al_attach_mixer_to_mixer
 */
bool al_attach_mixer_to_mixer(ALLEGRO_MIXER *stream, ALLEGRO_MIXER *mixer)
{
   ASSERT(mixer);
   ASSERT(stream);
   ASSERT(mixer != stream);

   if (mixer->ss.spl_data.frequency != stream->ss.spl_data.frequency) {
      _al_set_error(ALLEGRO_INVALID_OBJECT,
         "Attempted to attach a mixer with different frequencies");
      return false;
   }

   if (mixer->ss.spl_data.depth != stream->ss.spl_data.depth) {
      _al_set_error(ALLEGRO_INVALID_OBJECT,
         "Mixers of different audio depths cannot be attached to one another");
      return false;
   }

   if (mixer->ss.spl_data.chan_conf != stream->ss.spl_data.chan_conf) {
      _al_set_error(ALLEGRO_INVALID_OBJECT,
         "Mixers of different channel configurations cannot be attached to one another");
      return false;
   }

   return al_attach_sample_instance_to_mixer(&stream->ss, mixer);
}


/* Function: al_set_mixer_postprocess_callback
 */
bool al_set_mixer_postprocess_callback(ALLEGRO_MIXER *mixer,
   void (*pp_callback)(void *buf, unsigned int samples, void *data),
   void *pp_callback_userdata)
{
   ASSERT(mixer);

   maybe_lock_mutex(mixer->ss.mutex);

   mixer->postprocess_callback = pp_callback;
   mixer->pp_callback_userdata = pp_callback_userdata;

   maybe_unlock_mutex(mixer->ss.mutex);

   return true;
}


/* Function: al_get_mixer_frequency
 */
unsigned int al_get_mixer_frequency(const ALLEGRO_MIXER *mixer)
{
   ASSERT(mixer);

   return mixer->ss.spl_data.frequency;
}


/* Function: al_get_mixer_channels
 */
ALLEGRO_CHANNEL_CONF al_get_mixer_channels(const ALLEGRO_MIXER *mixer)
{
   ASSERT(mixer);

   return mixer->ss.spl_data.chan_conf;
}


/* Function: al_get_mixer_depth
 */
ALLEGRO_AUDIO_DEPTH al_get_mixer_depth(const ALLEGRO_MIXER *mixer)
{
   ASSERT(mixer);

   return mixer->ss.spl_data.depth;
}


/* Function: al_get_mixer_quality
 */
ALLEGRO_MIXER_QUALITY al_get_mixer_quality(const ALLEGRO_MIXER *mixer)
{
   ASSERT(mixer);

   return mixer->quality;
}


/* Function: al_get_mixer_gain
 */
float al_get_mixer_gain(const ALLEGRO_MIXER *mixer)
{
   ASSERT(mixer);

   return mixer->ss.gain;
}


/* Function: al_get_mixer_playing
 */
bool al_get_mixer_playing(const ALLEGRO_MIXER *mixer)
{
   ASSERT(mixer);

   return mixer->ss.is_playing;
}


/* Function: al_get_mixer_attached
 */
bool al_get_mixer_attached(const ALLEGRO_MIXER *mixer)
{
   ASSERT(mixer);

   return mixer->ss.parent.u.ptr;
}

/* Function: al_mixer_has_attachments
 */
bool al_mixer_has_attachments(const ALLEGRO_MIXER* mixer)
{
   ASSERT(mixer);

   return _al_vector_is_nonempty(&mixer->streams);
}


/* Function: al_set_mixer_frequency
 */
bool al_set_mixer_frequency(ALLEGRO_MIXER *mixer, unsigned int val)
{
   ASSERT(mixer);

   /* You can change the frequency of a mixer as long as it's not attached
    * to anything.
    */
   if (mixer->ss.parent.u.ptr) {
      _al_set_error(ALLEGRO_INVALID_OBJECT,
            "Attempted to change the frequency of an attached mixer");
      return false;
   }

   mixer->ss.spl_data.frequency = val;
   return true;
}


/* Function: al_set_mixer_quality
 */
bool al_set_mixer_quality(ALLEGRO_MIXER *mixer, ALLEGRO_MIXER_QUALITY new_quality)
{
   bool ret;
   ASSERT(mixer);

   maybe_lock_mutex(mixer->ss.mutex);

   if (mixer->quality == new_quality) {
      ret = true;
   }
   else if (_al_vector_size(&mixer->streams) == 0) {
      mixer->quality = new_quality;
      ret = true;
   }
   else {
      _al_set_error(ALLEGRO_INVALID_OBJECT,
         "Attempted to change the quality of a mixer with attachments");
      ret = false;
   }

   maybe_unlock_mutex(mixer->ss.mutex);

   return ret;
}


/* Function: al_set_mixer_gain
 */
bool al_set_mixer_gain(ALLEGRO_MIXER *mixer, float new_gain)
{
   int i;
   ASSERT(mixer);

   maybe_lock_mutex(mixer->ss.mutex);

   if (mixer->ss.gain != new_gain) {
      mixer->ss.gain = new_gain;

      for (i = _al_vector_size(&mixer->streams) - 1; i >= 0; i--) {
         ALLEGRO_SAMPLE_INSTANCE **slot = _al_vector_ref(&mixer->streams, i);
         _al_kcm_mixer_rejig_sample_matrix(mixer, *slot);
      }
   }

   maybe_unlock_mutex(mixer->ss.mutex);

   return true;
}


/* Function: al_set_mixer_playing
 */
bool al_set_mixer_playing(ALLEGRO_MIXER *mixer, bool val)
{
   ASSERT(mixer);

   mixer->ss.is_playing = val;
   return true;
}


/* Function: al_detach_mixer
 */
bool al_detach_mixer(ALLEGRO_MIXER *mixer)
{
   ASSERT(mixer);

   _al_kcm_detach_from_parent(&mixer->ss);
   ASSERT(mixer->ss.spl_read == NULL);
   return true;
}


/* vim: set sts=3 sw=3 et: */
