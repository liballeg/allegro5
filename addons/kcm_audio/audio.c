/**
 * Originally digi.c from allegro wiki
 * Original authors: KC/Milan
 *
 * Converted to allegro5 by Ryan Dickie
 */


#include <math.h>
#include <stdio.h>

#include "allegro5/internal/aintern_kcm_audio.h"

typedef enum {
	ALLEGRO_NO_ERROR       = 0,
	ALLEGRO_INVALID_PARAM  = 1,
	ALLEGRO_INVALID_OBJECT = 2,
	ALLEGRO_GENERIC_ERROR  = 255
} AL_ERROR_ENUM;

void _al_set_error(int error, char* string)
{
   TRACE("%s (error code: %d)", string, error);
}

ALLEGRO_AUDIO_DRIVER* driver = NULL;
extern struct ALLEGRO_AUDIO_DRIVER _openal_driver;
#if defined(ALLEGRO_LINUX)
   extern struct ALLEGRO_AUDIO_DRIVER _alsa_driver;
#endif

/* This function provides a (temporary!) matrix that can be used to convert one
   channel configuration into another. */
float *_al_rechannel_matrix(ALLEGRO_AUDIO_ENUM orig, ALLEGRO_AUDIO_ENUM target)
{
   /* Max 7.1 (8 channels) for input and output */
   static float mat[ALLEGRO_MAX_CHANNELS][ALLEGRO_MAX_CHANNELS];

   size_t dst_chans = al_channel_count(target);
   size_t src_chans = al_channel_count(orig);
   size_t i;

   /* Start with a simple identity matrix */
   memset(mat, 0, sizeof(mat));
   for(i = 0;i < src_chans && i < dst_chans;++i)
      mat[i][i] = 1.0;

   /* Multi-channel -> mono conversion (cuts rear/side channels) */
   if(dst_chans == 1 && (orig>>4) > 1)
   {
      for(i = 0;i < 2;++i)
         mat[0][i] = 1.0 / sqrt(2.0);

      /* If the source has a center channel, make sure that's copied 1:1
         (perhaps it should scale the overall output?) */
      if(((orig>>4)&1))
         mat[0][(orig>>4)-1] = 1.0;
   }
   /* Center (or mono) -> front l/r conversion */
   else if(((orig>>4)&1) && !((target>>4)&1))
   {
      mat[0][(orig>>4)-1] = 1.0 / sqrt(2.0);
      mat[1][(orig>>4)-1] = 1.0 / sqrt(2.0);
   }

   /* Copy LFE */
   if((orig>>4) != (target>>4) &&
      (orig&0xF) && (target&0xF))
      mat[dst_chans-1][src_chans-1] = 1.0;

   fprintf(stderr, "sample matrix:\n");
   for(i = 0;i < dst_chans;++i)
   {
      size_t j;
      for(j = 0;j < src_chans;++j)
         fprintf(stderr, " %f", mat[i][j]);
      fprintf(stderr, "\n");
   }
   fprintf(stderr, "\n");

   return &mat[0][0];
}


/* Channel configuration helpers */
bool al_is_channel_conf(ALLEGRO_AUDIO_ENUM conf)
{
   return ((conf >= ALLEGRO_AUDIO_1_CH) && (conf <= ALLEGRO_AUDIO_7_1_CH));
}

size_t al_channel_count(ALLEGRO_AUDIO_ENUM conf)
{
   return (conf>>4)+(conf&0xF);
}

/* Depth configuration helpers */
size_t al_depth_size(ALLEGRO_AUDIO_ENUM conf)
{
   ALLEGRO_AUDIO_ENUM depth = conf&~ALLEGRO_AUDIO_UNSIGNED;
   if(depth == ALLEGRO_AUDIO_8_BIT_INT)
      return sizeof(int8_t);
   if(depth == ALLEGRO_AUDIO_16_BIT_INT)
      return sizeof(int16_t);
   if(depth == ALLEGRO_AUDIO_24_BIT_INT)
      return sizeof(int32_t);
   if(depth == ALLEGRO_AUDIO_32_BIT_FLOAT)
      return sizeof(float);
   return 0;
}



/* This function sets a sample's mutex pointer to the specified value. It is
   ALLEGRO_MIXER aware, and will recursively set any attached streams' mutex to
   the same value */
static void stream_set_mutex(ALLEGRO_SAMPLE *stream, _AL_MUTEX *mutex)
{
   if(stream->mutex == mutex)
      return;
   stream->mutex = mutex;

   /* If this is a mixer, we need to make sure all the attached streams also
      set the same mutex */
   if(stream->read == mixer_read)
   {
      ALLEGRO_MIXER *mixer = (ALLEGRO_MIXER*)stream;
      int i;

      for(i = 0;mixer->streams[i];++i)
         stream_set_mutex(mixer->streams[i], mutex);
   }
}

/* This function is ALLEGRO_MIXER aware and frees the memory associated with the
   sample or mixer, and detaches any attached streams or mixers */
static void stream_free(ALLEGRO_SAMPLE *spl)
{
   if(spl)
   {
      /* Make sure we free the mixer buffer and de-reference the attached
         streams if this is a mixer stream */
      if(spl->read == mixer_read)
      {
         ALLEGRO_MIXER *mixer = (ALLEGRO_MIXER*)spl;
         int i;

         stream_set_mutex(&mixer->ss, NULL);

         for(i = 0;mixer->streams[i];++i)
            mixer->streams[i]->parent.ptr = NULL;

         free(mixer->streams);
      }

      if(spl->orphan_buffer)
         free(spl->buffer.ptr);

      free(spl);
   }
}


static inline int32_t clamp(int32_t val, int32_t min, int32_t max)
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


/* When a stream loops, this will fix up the position and anything else to
   allow it to safely continue playing as expected. Returns 0 if it should stop
   being mixed */
static inline int fix_looped_position(ALLEGRO_SAMPLE *spl)
{
   /* Looping! Should be mostly self-explanitory */
   switch(spl->loop)
   {
      case ALLEGRO_AUDIO_ONE_DIR:
         if(spl->step > 0)
         {
            while(spl->pos < spl->loop_start ||
                  spl->pos >= spl->loop_end)
               spl->pos -= (spl->loop_end - spl->loop_start);
         }
         else if(spl->step < 0)
         {
            while(spl->pos < spl->loop_start ||
                  spl->pos >= spl->loop_end)
               spl->pos += (spl->loop_end - spl->loop_start);
         }
         break;

      case ALLEGRO_AUDIO_BI_DIR:
         /* When doing bi-directional looping, you need to do a follow-up
            check for the opposite direction if a loop occured, otherwise
            you could end up misplaced on small, high-step loops */
         if(spl->step >= 0)
         {
         check_forward:
            if(spl->pos >= spl->loop_end)
            {
               spl->step = -spl->step;
               spl->pos = spl->loop_end - (spl->pos - spl->loop_end) - 1;
               goto check_backward;
            }
         }
         else
         {
         check_backward:
            if(spl->pos < spl->loop_start ||
               spl->pos >= spl->loop_end)
            {
               spl->step = -spl->step;
               spl->pos = spl->loop_start + (spl->loop_start - spl->pos);
               goto check_forward;
            }
         }
         break;

      default:
         if(spl->pos >= spl->len)
         {
            if(spl->is_stream)
            {
               ALLEGRO_STREAM *stream = (ALLEGRO_STREAM*)spl;
               void *old_buf = stream->spl.buffer.ptr;
               size_t i;

               if(old_buf)
               {
                  /* Slide the buffers down one position and put the
                     completed buffer into the used array to be refilled */
                  for(i = 0;stream->pending_bufs[i] && i < stream->buf_count-1;++i)
                     stream->pending_bufs[i] = stream->pending_bufs[i+1];
                  stream->pending_bufs[i] = NULL;

                  for(i = 0;stream->used_bufs[i];++i)
                     ;
                  stream->used_bufs[i] = old_buf;
               }

               stream->spl.buffer.ptr = stream->pending_bufs[0];
               if(!stream->spl.buffer.ptr)
                  return 0;

               spl->pos -= spl->len;
               return 1;
            }

            spl->pos = spl->len;
            spl->playing = false;
            return 0;
         }
   }
   return 1;
}


static float _samp_buf[ALLEGRO_MAX_CHANNELS]; /* max: 7.1 */

static inline const float *point_spl32(const ALLEGRO_SAMPLE *spl, unsigned int maxc)
{
   float *s = _samp_buf;
   unsigned int i;
   for(i = 0;i < maxc;++i)
   {
      if(spl->depth == ALLEGRO_AUDIO_24_BIT_INT)
         s[i] = (float)spl->buffer.s24[(spl->pos>>MIXER_FRAC_SHIFT)*maxc + i] / ((float)0x7FFFFF + 0.5f);
      else if(spl->depth == ALLEGRO_AUDIO_16_BIT_INT)
         s[i] = (float)spl->buffer.s16[(spl->pos>>MIXER_FRAC_SHIFT)*maxc + i] / ((float)0x7FFF + 0.5f);
      else if(spl->depth == ALLEGRO_AUDIO_8_BIT_INT)
         s[i] = (float)spl->buffer.s8[(spl->pos>>MIXER_FRAC_SHIFT)*maxc + i] / ((float)0x7F + 0.5f);
      else
         s[i] = spl->buffer.f32[(spl->pos>>MIXER_FRAC_SHIFT)*maxc + i];
   }
   return s;
}

static inline const float *point_spl32u(const ALLEGRO_SAMPLE *spl, unsigned int maxc)
{
   float *s = _samp_buf;
   unsigned int i;
   for(i = 0;i < maxc;++i)
   {
      if(spl->depth == ALLEGRO_AUDIO_24_BIT_UINT)
         s[i] = (float)spl->buffer.u24[(spl->pos>>MIXER_FRAC_SHIFT)*maxc + i]/((float)0x7FFFFF+0.5f) - 1.0;
      else if(spl->depth == ALLEGRO_AUDIO_16_BIT_UINT)
         s[i] = (float)spl->buffer.u16[(spl->pos>>MIXER_FRAC_SHIFT)*maxc + i]/((float)0x7FFF+0.5f) - 1.0;
      else if(spl->depth == ALLEGRO_AUDIO_8_BIT_UINT)
         s[i] = (float)spl->buffer.u8[(spl->pos>>MIXER_FRAC_SHIFT)*maxc + i]/((float)0x7F+0.5f) - 1.0;
      else
         s[i] = spl->buffer.f32[(spl->pos>>MIXER_FRAC_SHIFT)*maxc + i] - 1.0;
   }
   return s;
}

static inline const float *linear_spl32(const ALLEGRO_SAMPLE *spl, unsigned int maxc)
{
   unsigned long p1, p2;
   unsigned int i;
   float frac, *s = _samp_buf;

   p1 = (spl->pos>>MIXER_FRAC_SHIFT)*maxc;
   p2 = p1 + maxc;

   if(spl->loop == ALLEGRO_AUDIO_PLAY_ONCE)
   {
      if(spl->pos+MIXER_FRAC_ONE >= spl->len)
         p2 = p1;
   }
   else if(spl->pos+MIXER_FRAC_ONE >= spl->loop_end)
   {
      if(spl->loop == ALLEGRO_AUDIO_ONE_DIR)
         p2 = (spl->loop_start>>MIXER_FRAC_SHIFT)*maxc;
      else
         p2 = ((spl->loop_end>>MIXER_FRAC_SHIFT)-1)*maxc;
   }

   frac = (float)(spl->pos&MIXER_FRAC_MASK) / (float)MIXER_FRAC_ONE;

   for(i = 0;i < maxc;++i)
   {
      float s1, s2;

      if(spl->depth == ALLEGRO_AUDIO_24_BIT_INT)
      {
         s1 = (float)spl->buffer.s24[p1 + i] / ((float)0x7FFFFF + 0.5f);
         s2 = (float)spl->buffer.s24[p2 + i] / ((float)0x7FFFFF + 0.5f);
      }
      else if(spl->depth == ALLEGRO_AUDIO_16_BIT_INT)
      {
         s1 = (float)spl->buffer.s16[p1 + i] / ((float)0x7FFF + 0.5f);
         s2 = (float)spl->buffer.s16[p2 + i] / ((float)0x7FFF + 0.5f);
      }
      else if(spl->depth == ALLEGRO_AUDIO_8_BIT_INT)
      {
         s1 = (float)spl->buffer.s8[p1 + i] / ((float)0x7F + 0.5f);
         s2 = (float)spl->buffer.s8[p2 + i] / ((float)0x7F + 0.5f);
      }
      else
      {
         s1 = (float)spl->buffer.f32[p1 + i];
         s2 = (float)spl->buffer.f32[p2 + i];
      }

      s[i] = (s1*(1.0f-frac)) + (s2*frac);
   }
   return s;
}

static inline const float *linear_spl32u(const ALLEGRO_SAMPLE *spl, unsigned int maxc)
{
   unsigned long p1, p2;
   unsigned int i;
   float frac, *s = _samp_buf;

   p1 = (spl->pos>>MIXER_FRAC_SHIFT)*maxc;
   p2 = p1 + maxc;

   if(spl->loop == ALLEGRO_AUDIO_PLAY_ONCE)
   {
      if(spl->pos+MIXER_FRAC_ONE >= spl->len)
         p2 = p1;
   }
   else if(spl->pos+MIXER_FRAC_ONE >= spl->loop_end)
   {
      if(spl->loop == ALLEGRO_AUDIO_ONE_DIR)
         p2 = (spl->loop_start>>MIXER_FRAC_SHIFT)*maxc;
      else
         p2 = ((spl->loop_end>>MIXER_FRAC_SHIFT)-1)*maxc;
   }

   frac = (float)(spl->pos&MIXER_FRAC_MASK) / (float)MIXER_FRAC_ONE;

   for(i = 0;i < maxc;++i)
   {
      float s1, s2;

      if(spl->depth == ALLEGRO_AUDIO_24_BIT_UINT)
      {
         s1 = (float)spl->buffer.u24[p1 + i]/((float)0x7FFFFF + 0.5f) - 1.0f;
         s2 = (float)spl->buffer.u24[p2 + i]/((float)0x7FFFFF + 0.5f) - 1.0f;
      }
      else if(spl->depth == ALLEGRO_AUDIO_16_BIT_UINT)
      {
         s1 = (float)spl->buffer.u16[p1 + i]/((float)0x7FFF + 0.5f) - 1.0f;
         s2 = (float)spl->buffer.u16[p2 + i]/((float)0x7FFF + 0.5f) - 1.0f;
      }
      else if(spl->depth == ALLEGRO_AUDIO_8_BIT_UINT)
      {
         s1 = (float)spl->buffer.u8[p1 + i]/((float)0x7F + 0.5f) - 1.0f;
         s2 = (float)spl->buffer.u8[p2 + i]/((float)0x7F + 0.5f) - 1.0f;
      }
      else
      {
         s1 = (float)spl->buffer.f32[p1 + i] - 1.0f;
         s2 = (float)spl->buffer.f32[p2 + i] - 1.0f;
      }

      s[i] = (s1*(1.0f-frac)) + (s2*frac);
   }
   return s;
}


/* Reads some samples into a mixer buffer. */
#define MAKE_MIXER(bits, interp)   \
static void read_to_mixer_##interp##bits(void *source, void **vbuf, unsigned long samples, ALLEGRO_AUDIO_ENUM buffer_depth, size_t dest_maxc)   \
{   \
   ALLEGRO_SAMPLE *spl = (ALLEGRO_SAMPLE*)source;   \
   float *buf = *vbuf;   \
   size_t maxc = al_channel_count(spl->chan_conf);   \
   size_t c;   \
   \
   if(!spl->playing)   \
      return;   \
   \
   while(samples > 0)   \
   {   \
      const float *s;   \
   \
      if(!fix_looped_position(spl))   \
         return;   \
   \
      s = interp##_spl##bits(spl, maxc);   \
      for(c = 0;c < dest_maxc;++c)   \
      {   \
         size_t i;   \
         for(i = 0;i < maxc;++i)   \
            *buf += s[i] * spl->matrix[c*maxc + i];   \
         ++buf;   \
      }   \
   \
      spl->pos += spl->step;   \
      --samples;   \
   }   \
   fix_looped_position(spl);   \
   (void)buffer_depth;   \
}

MAKE_MIXER(32,  point)
MAKE_MIXER(32u, point)
MAKE_MIXER(32,  linear)
MAKE_MIXER(32u, linear)

#undef MAKE_MIXER

/* This passes the next waiting stream buffer to the voice via vbuf, setting
   the last one as used */
static void stream_read(void *source, void **vbuf, unsigned long samples, ALLEGRO_AUDIO_ENUM buffer_depth, size_t dest_maxc)
{
   ALLEGRO_STREAM *stream = (ALLEGRO_STREAM*)source;
   void *old_buf;
   size_t i;

   if(!stream->spl.playing)
      return;

   old_buf = stream->pending_bufs[0];
   if(old_buf)
   {
      for(i = 0;stream->used_bufs[i] && i < stream->buf_count-1;++i)
         ;
      stream->used_bufs[i] = old_buf;
   }

   for(i = 0;stream->pending_bufs[i] && i < stream->buf_count-1;++i)
      ;
   stream->pending_bufs[i] = NULL;

   *vbuf = stream->pending_bufs[0];
   (void)dest_maxc;
   (void)buffer_depth;
   (void)samples;
}


/* Mixes the streams attached to the mixer and writes additively to the
   specified buffer (or if *buf is NULL, indicating a voice, convert it and
   set it to the buffer pointer) */
void mixer_read(void *source, void **buf, unsigned long samples, ALLEGRO_AUDIO_ENUM buffer_depth, size_t dest_maxc)
{
   const ALLEGRO_MIXER *mixer;
   ALLEGRO_MIXER *m = (ALLEGRO_MIXER*)source;
   int maxc = al_channel_count(m->ss.chan_conf);
   ALLEGRO_SAMPLE **spl;

   if(!m->ss.playing)
      return;

   /* make sure the mixer buffer is big enough */
   if(m->ss.len*maxc < samples*maxc)
   {
      free(m->ss.buffer.ptr);
      m->ss.buffer.ptr = malloc(samples*maxc*sizeof(float));
      if(!m->ss.buffer.ptr)
      {
         _al_set_error(ALLEGRO_GENERIC_ERROR, "Out of memory allocating mixer buffer");
         m->ss.len = 0;
         return;
      }
      m->ss.len = samples;
   }

   mixer = m;

   /* Clear the buffer to silence */
   memset(mixer->ss.buffer.ptr, 0, samples*maxc*sizeof(float));
   /* mix the streams into the mixer buffer */
   for(spl = mixer->streams;*spl;++spl)
      (*spl)->read(*spl, (void**)&mixer->ss.buffer.ptr, samples, ALLEGRO_AUDIO_32_BIT_FLOAT, maxc);
   /* Call the post-processing callback */
   if(mixer->post_process)
      mixer->post_process(mixer->ss.buffer.ptr, mixer->ss.len, mixer->cb_ptr);


   samples *= maxc;

   if(*buf)
   {
      /* We don't need to clamp in the mixer yet */
      float *lbuf = *buf;
      float *src = mixer->ss.buffer.f32;

      while(samples > 0)
      {
         *(lbuf++) += *(src++);
         --samples;
      }
      return;
   }

   /* We're feeding to a voice, so we pass it back the mixed data (make sure
      to clamp and convert it) */
   *buf = mixer->ss.buffer.ptr;
   switch(buffer_depth&~ALLEGRO_AUDIO_UNSIGNED)
   {
      case ALLEGRO_AUDIO_24_BIT_INT:
      {
         int32_t off = ((buffer_depth&ALLEGRO_AUDIO_UNSIGNED)?0x800000:0);
         int32_t *lbuf = mixer->ss.buffer.s24;
         float *src = mixer->ss.buffer.f32;

         while(samples > 0)
         {
            *lbuf  = clamp(*(src++) * ((float)0x7FFFFF+0.5f), ~0x7FFFFF, 0x7FFFFF);
            *lbuf += off;
            ++lbuf;
            --samples;
         }
         break;
      }
      case ALLEGRO_AUDIO_16_BIT_INT:
      {
         int16_t off = ((buffer_depth&ALLEGRO_AUDIO_UNSIGNED)?0x8000:0);
         int16_t *lbuf = mixer->ss.buffer.s16;
         float *src = mixer->ss.buffer.f32;

         while(samples > 0)
         {
            *lbuf  = clamp(*(src++) * ((float)0x7FFF+0.5f), ~0x7FFF, 0x7FFF);
            *lbuf += off;
            ++lbuf;
            --samples;
         }
         break;
      }
      case ALLEGRO_AUDIO_8_BIT_INT:
      default:
      {
         int8_t off = ((buffer_depth&ALLEGRO_AUDIO_UNSIGNED)?0x80:0);
         int8_t *lbuf = mixer->ss.buffer.s8;
         float *src = mixer->ss.buffer.f32;

         while(samples > 0)
         {
            *lbuf  = clamp(*(src++) * ((float)0x7F+0.5f), ~0x7F, 0x7F);
            *lbuf += off;
            ++lbuf;
            --samples;
         }
         break;
      }
   }
   (void)dest_maxc;
}


/* This detaches the sample, stream, or mixer from anything it may be attached
   to */
static void detach_from_parent(ALLEGRO_SAMPLE *spl)
{
   if(!spl || !spl->parent.ptr)
      return;

   if(!spl->parent_is_voice)
   {
      ALLEGRO_MIXER *mixer = spl->parent.mixer;
      int i;

      free(spl->matrix);
      spl->matrix = NULL;

      /* Search through the streams and check for this one */
      for(i = 0;mixer->streams[i];++i)
      {
         if(mixer->streams[i] == spl)
         {
            _al_mutex_lock(mixer->ss.mutex);

            do {
               mixer->streams[i] = mixer->streams[i+1];
            } while(mixer->streams[++i]);
            spl->parent.mixer = NULL;
            stream_set_mutex(spl, NULL);

            if(spl->read != mixer_read)
               spl->read = NULL;

            _al_mutex_unlock(mixer->ss.mutex);

            return;
         }
      }
   }
   else
      al_voice_detach(spl->parent.voice);
}


/* al_sample_create:
 *   Creates a sample stream, using the supplied buffer at the specified size,
 *   channel configuration, sample depth, and frequency. This must be attached
 *   to a voice or mixer before it can be played.
 */
ALLEGRO_SAMPLE *al_sample_create(void *buf, unsigned long samples, unsigned long freq, ALLEGRO_AUDIO_ENUM depth, ALLEGRO_AUDIO_ENUM chan_conf)
{
   ALLEGRO_SAMPLE *spl;

   if(!freq)
   {
      _al_set_error(ALLEGRO_INVALID_PARAM, "Invalid sample frequency");
      return NULL;
   }

   spl = calloc(1, sizeof(*spl));
   if(!spl)
   {
      _al_set_error(ALLEGRO_GENERIC_ERROR, "Out of memory allocating sample object");
      return NULL;
   }

   spl->loop       = ALLEGRO_AUDIO_PLAY_ONCE;
   spl->depth      = depth;
   spl->chan_conf  = chan_conf;
   spl->frequency  = freq;
   spl->speed      = 1.0f;
   spl->parent.ptr = NULL;
   spl->buffer.ptr = buf;

   spl->step = 0;
   spl->pos = 0;
   spl->len = samples<<MIXER_FRAC_SHIFT;

   spl->loop_start = 0;
   spl->loop_end = spl->len;

   return spl;
}

/* al_sample_destroy:
 *   Detaches the sample stream from anything it may be attached to and frees
 *   its data (note, the sample data is *not* freed!).
 */
/* This function is ALLEGRO_MIXER aware */
void al_sample_destroy(ALLEGRO_SAMPLE *spl)
{
   detach_from_parent(spl);
   stream_free(spl);
}


void al_sample_play(ALLEGRO_SAMPLE *spl)
{
   al_sample_set_bool(spl, ALLEGRO_AUDIO_PLAYING, 1);
}

void al_sample_stop(ALLEGRO_SAMPLE *spl)
{
   al_sample_set_bool(spl, ALLEGRO_AUDIO_PLAYING, 0);
}

int al_sample_get_long(const ALLEGRO_SAMPLE *spl, ALLEGRO_AUDIO_ENUM setting, unsigned long *val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_FREQUENCY:
         *val = spl->frequency;
         return 0;

      case ALLEGRO_AUDIO_LENGTH:
         *val = spl->len>>MIXER_FRAC_SHIFT;
         return 0;

      case ALLEGRO_AUDIO_POSITION:
         if(spl->parent.ptr && spl->parent_is_voice)
         {
            ALLEGRO_VOICE *voice = spl->parent.voice;
            return al_voice_get_long(voice, ALLEGRO_AUDIO_POSITION, val);
         }

         *val = spl->pos >> MIXER_FRAC_SHIFT;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to get invalid sample long setting");
         break;
   }
   return 1;
}

int al_sample_get_float(const ALLEGRO_SAMPLE *spl, ALLEGRO_AUDIO_ENUM setting, float *val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_SPEED:
         *val = spl->speed;
         return 0;
      case ALLEGRO_AUDIO_TIME:
         *val = (float)(spl->len>>MIXER_FRAC_SHIFT) / (float)spl->frequency;
         return 0;
      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to get invalid sample float setting");
         break;
   }
   return 1;
}

int al_sample_get_enum(const ALLEGRO_SAMPLE *spl, ALLEGRO_AUDIO_ENUM setting, ALLEGRO_AUDIO_ENUM *val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_DEPTH:
         *val = spl->depth;
         return 0;

      case ALLEGRO_AUDIO_CHANNELS:
         *val = spl->chan_conf;
         return 0;

      case ALLEGRO_AUDIO_LOOPMODE:
         *val = spl->loop;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to get invalid sample enum setting");
         break;
   }
   return 1;
}

int al_sample_get_bool(const ALLEGRO_SAMPLE *spl, ALLEGRO_AUDIO_ENUM setting, bool *val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_PLAYING:
         if(spl->parent.ptr && spl->parent_is_voice)
         {
            ALLEGRO_VOICE *voice = spl->parent.voice;
            return al_voice_get_bool(voice, ALLEGRO_AUDIO_PLAYING, val);
         }

         *val = spl->playing;
         return 0;

      case ALLEGRO_AUDIO_ATTACHED:
         *val = (spl->parent.ptr != NULL);
         return 0;

      case ALLEGRO_AUDIO_ORPHAN_BUFFER:
         *val = spl->orphan_buffer;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to get invalid sample bool setting");
         break;
   }
   return 1;
}

int al_sample_get_ptr(const ALLEGRO_SAMPLE *spl, ALLEGRO_AUDIO_ENUM setting, void **val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_BUFFER:
         if(spl->playing || spl->orphan_buffer)
         {
            _al_set_error(ALLEGRO_INVALID_OBJECT, (spl->orphan_buffer ?
                                    "Attempted to get an orphaned buffer" :
                                    "Attempted to get a playing buffer"));
            return 1;
         }
         *val = spl->buffer.ptr;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to get invalid sample pointer setting");
         break;
   }
   return 1;
}



int al_sample_set_long(ALLEGRO_SAMPLE *spl, ALLEGRO_AUDIO_ENUM setting, unsigned long val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_POSITION:
         if(spl->parent.ptr && spl->parent_is_voice)
         {
            ALLEGRO_VOICE *voice = spl->parent.voice;
            if(al_voice_set_long(voice, ALLEGRO_AUDIO_POSITION, val) != 0)
               return 1;
         }
         else
         {
            _al_mutex_lock(spl->mutex);
            spl->pos = val << MIXER_FRAC_SHIFT;
            _al_mutex_unlock(spl->mutex);
         }

         return 0;

      case ALLEGRO_AUDIO_LENGTH:
         if(spl->playing)
         {
            _al_set_error(ALLEGRO_INVALID_OBJECT, "Attempted to change the length of a playing sample");
            return 1;
         }
         spl->len = val << MIXER_FRAC_SHIFT;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to set invalid sample long setting");
         break;
   }
   return 1;
}

int al_sample_set_float(ALLEGRO_SAMPLE *spl, ALLEGRO_AUDIO_ENUM setting, float val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_SPEED:
         if(fabs(val) < (1.0f/64.0f))
         {
            _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to set zero speed");
            return 1;
         }

         if(spl->parent.ptr && spl->parent_is_voice)
         {
            _al_set_error(ALLEGRO_GENERIC_ERROR, "Could not set voice playback speed");
            return 1;
         }

         spl->speed = val;
         if(spl->parent.mixer)
         {
            ALLEGRO_MIXER *mixer = spl->parent.mixer;

            _al_mutex_lock(spl->mutex);

            spl->step = (spl->frequency<<MIXER_FRAC_SHIFT) *
                        spl->speed / mixer->ss.frequency;
            /* Don't wanna be trapped with a step value of 0 */
            if(spl->step == 0)
            {
               if(spl->speed > 0.0f)
                  spl->step = 1;
               else
                  spl->step = -1;
            }

            _al_mutex_unlock(spl->mutex);
         }

         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to set invalid sample float setting");
         break;
   }
   return 1;
}

int al_sample_set_enum(ALLEGRO_SAMPLE *spl, ALLEGRO_AUDIO_ENUM setting, ALLEGRO_AUDIO_ENUM val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_LOOPMODE:
         if(spl->parent.ptr && spl->parent_is_voice)
         {
            _al_set_error(ALLEGRO_GENERIC_ERROR, "Unable to set voice loop mode");
            return 1;
         }

         _al_mutex_lock(spl->mutex);
         spl->loop = val;
         if(spl->loop != ALLEGRO_AUDIO_PLAY_ONCE)
         {
            if(spl->pos < spl->loop_start)
               spl->pos = spl->loop_start;
            else if(spl->pos > spl->loop_end-MIXER_FRAC_ONE)
               spl->pos = spl->loop_end-MIXER_FRAC_ONE;
         }
         _al_mutex_unlock(spl->mutex);
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to set invalid sample enum setting");
         break;
   }
   return 1;
}

int al_sample_set_bool(ALLEGRO_SAMPLE *spl, ALLEGRO_AUDIO_ENUM setting, bool val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_PLAYING:
         if (!spl->parent.ptr)
         {
            fprintf(stderr, "Sample has no parent\n");
            return 1;
         }

         if(spl->parent_is_voice)
         {
            /* parent is voice */
            ALLEGRO_VOICE *voice = spl->parent.voice;

            /* FIXME: there is no else. what does this do? */
            if(al_voice_set_bool(voice, ALLEGRO_AUDIO_PLAYING, val) == 0)
            {
               unsigned long pos = spl->pos >> MIXER_FRAC_SHIFT;
               if(al_voice_get_long(voice, ALLEGRO_AUDIO_POSITION, &pos) == 0)
               {
                  spl->pos = pos << MIXER_FRAC_SHIFT;
                  spl->playing = val;
                  return 0;
               }
            }
            return 1;
         }
         else
         {
            /* parent is mixer */
            _al_mutex_lock(spl->mutex);
            spl->playing = val;
            if(!val)
               spl->pos = 0;
            _al_mutex_unlock(spl->mutex);
         }
         return 0;

      case ALLEGRO_AUDIO_ATTACHED:
         if(val)
         {
            _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to set sample attachment ststus true");
            return 1;
         }
         detach_from_parent(spl);
         return 0;

      case ALLEGRO_AUDIO_ORPHAN_BUFFER:
         if(spl->orphan_buffer && !val)
         {
            _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to adopt an orphaned buffer");
            return 1;
         }
         spl->orphan_buffer = val;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to set invalid sample long setting");
         break;
   }
   return 1;
}

int al_sample_set_ptr(ALLEGRO_SAMPLE *spl, ALLEGRO_AUDIO_ENUM setting, void *val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_BUFFER:
         if(spl->playing)
         {
            _al_set_error(ALLEGRO_INVALID_OBJECT, "Attempted to change the buffer of a playing sample");
            return 1;
         }
         if(spl->parent.ptr && spl->parent_is_voice)
         {
            ALLEGRO_VOICE *voice = spl->parent.voice;
            voice->driver->unload_voice(voice);
            if(voice->driver->load_voice(voice, val))
            {
               _al_set_error(ALLEGRO_GENERIC_ERROR, "Unable to load sample into voice");
               return 1;
            }
         }
         if(spl->orphan_buffer)
            free(spl->buffer.ptr);
         spl->orphan_buffer = false;
         spl->buffer.ptr = val;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to set invalid sample long setting");
         break;
   }
   return 1;
}



/* al_stream_create:
 *   Creates an audio stream, using the supplied values. The stream will be set
 *   to play by default.
 */
ALLEGRO_STREAM *al_stream_create(size_t buffer_count, unsigned long samples, unsigned long freq, ALLEGRO_AUDIO_ENUM depth, ALLEGRO_AUDIO_ENUM chan_conf)
{
   ALLEGRO_STREAM *stream;
   unsigned long bytes_per_buffer;
   size_t i;

   if(!buffer_count || !samples || !freq)
   {
      _al_set_error(ALLEGRO_INVALID_PARAM, ((!buffer_count)?"Attempted to create stream with no buffers" :
                                        ((!samples)?"Attempted to create stream with no buffer size" :
                                         "Attempted to create stream with no frequency")));
      return NULL;
   }

   bytes_per_buffer  = samples;
   bytes_per_buffer *= (chan_conf>>4) + (chan_conf&0xF);
   if((depth&~ALLEGRO_AUDIO_UNSIGNED) == ALLEGRO_AUDIO_8_BIT_INT)
      bytes_per_buffer *= sizeof(int8_t);
   else if((depth&~ALLEGRO_AUDIO_UNSIGNED) == ALLEGRO_AUDIO_16_BIT_INT)
      bytes_per_buffer *= sizeof(int16_t);
   else if((depth&~ALLEGRO_AUDIO_UNSIGNED) == ALLEGRO_AUDIO_24_BIT_INT)
      bytes_per_buffer *= sizeof(int32_t);
   else if(depth == ALLEGRO_AUDIO_32_BIT_FLOAT)
      bytes_per_buffer *= sizeof(float);
   else
      return NULL;

   stream = calloc(1, sizeof(*stream));
   if(!stream)
   {
      _al_set_error(ALLEGRO_GENERIC_ERROR, "Out of memory allocating stream object");
      return NULL;
   }

   stream->spl.playing   = true;
   stream->spl.is_stream = true;

   stream->spl.loop      = ALLEGRO_AUDIO_PLAY_ONCE;
   stream->spl.depth     = depth;
   stream->spl.chan_conf = chan_conf;
   stream->spl.frequency = freq;
   stream->spl.speed     = 1.0f;

   stream->spl.step = 0;
   stream->spl.pos  = samples<<MIXER_FRAC_SHIFT;
   stream->spl.len  = stream->spl.pos;

   stream->buf_count = buffer_count;

   stream->used_bufs = calloc(1, buffer_count*sizeof(void*)*2);
   if(!stream->used_bufs)
   {
      free(stream->used_bufs);
      free(stream);
      _al_set_error(ALLEGRO_GENERIC_ERROR, "Out of memory allocating stream buffer pointers");
      return NULL;
   }
   stream->pending_bufs = stream->used_bufs + buffer_count;

   stream->main_buffer = calloc(1, bytes_per_buffer * buffer_count);
   if(!stream->main_buffer)
   {
      free(stream->used_bufs);
      free(stream);
      _al_set_error(ALLEGRO_GENERIC_ERROR, "Out of memory allocating stream buffer");
      return NULL;
   }

   for(i = 0;i < buffer_count;++i)
      stream->pending_bufs[i] = (int8_t*)stream->main_buffer +
                                         i*bytes_per_buffer;

   return stream;
}

void al_stream_destroy(ALLEGRO_STREAM *stream)
{
   if(stream)
   {
      detach_from_parent(&stream->spl);
      free(stream->main_buffer);
      free(stream->used_bufs);
      free(stream);
   }
}

int al_stream_get_long(const ALLEGRO_STREAM *stream, ALLEGRO_AUDIO_ENUM setting, unsigned long *val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_FREQUENCY:
         *val = stream->spl.frequency;
         return 0;

      case ALLEGRO_AUDIO_LENGTH:
         *val = stream->spl.len>>MIXER_FRAC_SHIFT;
         return 0;

      case ALLEGRO_AUDIO_FRAGMENTS:
         *val = stream->buf_count;
         return 0;

      case ALLEGRO_AUDIO_USED_FRAGMENTS:
      {
         size_t i;
         for(i = 0;stream->used_bufs[i] && i < stream->buf_count;++i)
            ;
         *val = i;
         return 0;
      }

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to get invalid stream long setting");
         break;
   }
   return 1;
}

int al_stream_get_float(const ALLEGRO_STREAM *stream, ALLEGRO_AUDIO_ENUM setting, float *val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_SPEED:
         *val = stream->spl.speed;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to get invalid stream float setting");
         break;
   }
   return 1;
}

int al_stream_get_enum(const ALLEGRO_STREAM *stream, ALLEGRO_AUDIO_ENUM setting, ALLEGRO_AUDIO_ENUM *val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_DEPTH:
         *val = stream->spl.depth;
         return 0;

      case ALLEGRO_AUDIO_CHANNELS:
         *val = stream->spl.chan_conf;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to get invalid stream enum setting");
         break;
   }
   return 1;
}

int al_stream_get_bool(const ALLEGRO_STREAM *stream, ALLEGRO_AUDIO_ENUM setting, bool *val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_PLAYING:
         *val = stream->spl.playing;
         return 0;

      case ALLEGRO_AUDIO_ATTACHED:
         *val = (stream->spl.parent.ptr != NULL);
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to get invalid stream bool setting");
         break;
   }
   return 1;
}

int al_stream_get_ptr(const ALLEGRO_STREAM *stream, ALLEGRO_AUDIO_ENUM setting, void **val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_BUFFER:
         if(stream->used_bufs[0])
         {
            size_t i;
            *val = stream->used_bufs[0];
            for(i = 0;stream->used_bufs[i] && i < stream->buf_count-1;++i)
               stream->used_bufs[i] = stream->used_bufs[i+1];
            stream->used_bufs[i] = NULL;
            return 0;
         }
         _al_set_error(ALLEGRO_INVALID_OBJECT, "Attempted to get the buffer of a non-waiting stream");
         break;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to get invalid stream pointer setting");
         break;
   }
   return 1;
}



int al_stream_set_long(ALLEGRO_STREAM *stream, ALLEGRO_AUDIO_ENUM setting, unsigned long val)
{
   (void)stream;
   (void)val;

   switch(setting)
   {
      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to set invalid stream long setting");
         break;
   }
   return 1;
}

int al_stream_set_float(ALLEGRO_STREAM *stream, ALLEGRO_AUDIO_ENUM setting, float val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_SPEED:
         if(val <= 0.0f)
         {
            _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to set stream speed to a zero or negative value");
            return 1;
         }

         if(stream->spl.parent.ptr && stream->spl.parent_is_voice)
         {
            _al_set_error(ALLEGRO_GENERIC_ERROR, "Could not set voice playback speed");
            return 1;
         }

         stream->spl.speed = val;
         if(stream->spl.parent.mixer)
         {
            ALLEGRO_MIXER *mixer = stream->spl.parent.mixer;
            long i;

            _al_mutex_lock(stream->spl.mutex);

            /* Make step 1 before applying the freq difference (so we
               play forward) */
            stream->spl.step = 1;

            i = (stream->spl.frequency<<MIXER_FRAC_SHIFT) *
                stream->spl.speed / mixer->ss.frequency;
            /* Don't wanna be trapped with a step value of 0 */
            if(i) stream->spl.step *= i;

            _al_mutex_unlock(stream->spl.mutex);
         }

         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to set invalid stream float setting");
         break;
   }
   return 1;
}

int al_stream_set_enum(ALLEGRO_STREAM *stream, ALLEGRO_AUDIO_ENUM setting, ALLEGRO_AUDIO_ENUM val)
{
   (void)stream;
   (void)val;

   switch(setting)
   {
      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to set invalid stream enum setting");
         break;
   }
   return 1;
}

int al_stream_set_bool(ALLEGRO_STREAM *stream, ALLEGRO_AUDIO_ENUM setting, bool val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_PLAYING:
         if(stream->spl.parent.ptr && stream->spl.parent_is_voice)
         {
            ALLEGRO_VOICE *voice = stream->spl.parent.voice;
            if(al_voice_set_bool(voice, ALLEGRO_AUDIO_PLAYING, val) != 0)
               return 1;
         }
         stream->spl.playing = val;

         if(!val)
         {
            _al_mutex_lock(stream->spl.mutex);
            stream->spl.pos = stream->spl.len;
            _al_mutex_unlock(stream->spl.mutex);
         }
         return 0;

      case ALLEGRO_AUDIO_ATTACHED:
         if(val)
         {
            _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to set stream attachment status true");
            return 1;
         }
         detach_from_parent(&stream->spl);
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to set invalid stream bool setting");
         break;
   }
   return 1;
}

int al_stream_set_ptr(ALLEGRO_STREAM *stream, ALLEGRO_AUDIO_ENUM setting, void *val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_BUFFER:
      {
         size_t i;
         _al_mutex_lock(stream->spl.mutex);

         for(i = 0;stream->pending_bufs[i] && i < stream->buf_count;++i)
            ;
         if(i == stream->buf_count)
         {
            _al_mutex_unlock(stream->spl.mutex);
            _al_set_error(ALLEGRO_INVALID_OBJECT, "Attempted to set a stream buffer with a full pending list");
            return 1;
         }
         stream->pending_bufs[i] = val;

         _al_mutex_unlock(stream->spl.mutex);
         return 0;
      }

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to set invalid stream pointer setting");
         break;
   }
   return 1;
}



/* al_mixer_create:
 *   Creates a mixer stream, to attach sample streams or other mixers to. It
 *   will mix into a buffer at the requested frequency and channel count.
 *   Only 24-bit signed integer mixing is currently supported.
 */
ALLEGRO_MIXER *al_mixer_create(unsigned long freq, ALLEGRO_AUDIO_ENUM depth, ALLEGRO_AUDIO_ENUM chan_conf)
{
   ALLEGRO_MIXER *mixer;

   if(!freq || depth != ALLEGRO_AUDIO_32_BIT_FLOAT)
   {
      _al_set_error(ALLEGRO_INVALID_PARAM, ((!freq) ? "Attempted to create mixer with no frequency" :
                  "Attempted to create mixer with an invalid sample depth"));
      return NULL;
   }

   mixer = calloc(1, sizeof(ALLEGRO_MIXER));
   if(!mixer)
   {
      _al_set_error(ALLEGRO_GENERIC_ERROR, "Out of memory allocating mixer object");
      return NULL;
   }

   mixer->ss.playing = true;
   mixer->ss.orphan_buffer = true;

   mixer->ss.loop      = ALLEGRO_AUDIO_PLAY_ONCE;
   mixer->ss.depth     = depth;
   mixer->ss.chan_conf = chan_conf;
   mixer->ss.frequency = freq;

   mixer->ss.read = mixer_read;

   mixer->quality = ALLEGRO_AUDIO_LINEAR;

   mixer->streams = malloc(sizeof(ALLEGRO_SAMPLE*));
   if(!mixer->streams)
   {
      free(mixer);
      _al_set_error(ALLEGRO_GENERIC_ERROR, "Out of memory allocating attachment pointers");
      return NULL;
   }
   mixer->streams[0] = NULL;

   return mixer;
}

/* al_mixer_destroy:
 *   Destroys the mixer stream, identical to al_sample_destroy.
 */
void al_mixer_destroy(ALLEGRO_MIXER *mixer)
{
   al_sample_destroy(&mixer->ss);
}


/* al_mixer_attach_sample:
 */
/* This function is ALLEGRO_MIXER aware */
int al_mixer_attach_sample(ALLEGRO_MIXER *mixer, ALLEGRO_SAMPLE *spl)
{
   void *temp;
   unsigned long i;

   /* Already referenced, do not attach */
   if(spl->parent.ptr)
   {
      _al_set_error(ALLEGRO_INVALID_OBJECT, "Attempted to attach a sample that's already attached");
      return 1;
   }

   /* Get the number of streams attached */
   for(i = 0;mixer->streams[i];++i)
      ;

   _al_mutex_lock(mixer->ss.mutex);

   temp = realloc(mixer->streams, (i+2) * sizeof(ALLEGRO_SAMPLE*));
   if(!temp)
   {
      _al_mutex_unlock(mixer->ss.mutex);
      _al_set_error(ALLEGRO_GENERIC_ERROR, "Out of memory allocating attachment pointers");
      return 1;
   }
   mixer->streams = temp;

   mixer->streams[i++] = spl;
   mixer->streams[i  ] = NULL;

   spl->step = (spl->frequency<<MIXER_FRAC_SHIFT) * spl->speed /
               mixer->ss.frequency;
   /* Don't wanna be trapped with a step value of 0 */
   if(spl->step == 0)
   {
      if(spl->speed > 0.0f)
         spl->step = 1;
      else
         spl->step = -1;
   }

   /* If this isn't a mixer, set the proper sample stream reader */
   if(spl->read == NULL)
   {
      size_t dst_chans = (mixer->ss.chan_conf>>4) + (mixer->ss.chan_conf&0xF);
      size_t src_chans = (spl->chan_conf>>4) + (spl->chan_conf&0xF);
      float *mat;

      if(mixer->quality == ALLEGRO_AUDIO_LINEAR)
      {
         if((spl->depth&ALLEGRO_AUDIO_UNSIGNED))
            spl->read = read_to_mixer_linear32u;
         else
            spl->read = read_to_mixer_linear32;
      }
      else
      {
         if((spl->depth&ALLEGRO_AUDIO_UNSIGNED))
            spl->read = read_to_mixer_point32u;
         else
            spl->read = read_to_mixer_point32;
      }

      mat = _al_rechannel_matrix(spl->chan_conf, mixer->ss.chan_conf);

      spl->matrix = calloc(1, src_chans*dst_chans*sizeof(float));

      for(i = 0;i < dst_chans;++i)
      {
         size_t j;
         for(j = 0;j < src_chans;++j)
            spl->matrix[i*src_chans + j] = mat[i*ALLEGRO_MAX_CHANNELS + j];
      }
   }

   stream_set_mutex(spl, mixer->ss.mutex);

   spl->parent.mixer = mixer;
   spl->parent_is_voice = false;

   _al_mutex_unlock(mixer->ss.mutex);

   return 0;
}

/* al_mixer_attach_stream:
 */
int al_mixer_attach_stream(ALLEGRO_MIXER *mixer, ALLEGRO_STREAM *stream)
{
   return al_mixer_attach_sample(mixer, &stream->spl);
}

/* al_mixer_attach_mixer:
 *   Attaches a mixer onto another mixer. The same rules as with
 *   al_mixer_attach_sample apply, with the added caveat that both
 *   mixers must be the same frequency.
 */
int al_mixer_attach_mixer(ALLEGRO_MIXER *mixer, ALLEGRO_MIXER *stream)
{
   if(mixer->ss.frequency != stream->ss.frequency)
   {
      _al_set_error(ALLEGRO_INVALID_OBJECT, "Attempted to attach a mixer with different frequencies");
      return 1;
   }
   return al_mixer_attach_sample(mixer, &stream->ss);
}


/* al_mixer_set_postprocess_callback:
 *   Sets a post-processing filter function that's called after the attached
 *   streams have been mixed. The buffer's format will be whatever the mixer
 *   was created with. The sample count and user-data pointer is also passed.
 */
int al_mixer_set_postprocess_callback(ALLEGRO_MIXER *mixer, void (*cb)(void *buf, unsigned long samples, void *data), void *data)
{
   _al_mutex_lock(mixer->ss.mutex);
   mixer->post_process = cb;
   mixer->cb_ptr = data;
   _al_mutex_unlock(mixer->ss.mutex);

   return 0;
}


int al_mixer_get_long(const ALLEGRO_MIXER *mixer, ALLEGRO_AUDIO_ENUM setting, unsigned long *val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_FREQUENCY:
         *val = mixer->ss.frequency;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to get invalid mixer long setting");
         break;
   }
   return 1;
}

int al_mixer_get_enum(const ALLEGRO_MIXER *mixer, ALLEGRO_AUDIO_ENUM setting, ALLEGRO_AUDIO_ENUM *val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_CHANNELS:
         *val = mixer->ss.chan_conf;
         return 0;

      case ALLEGRO_AUDIO_DEPTH:
         *val = mixer->ss.depth;
         return 0;

      case ALLEGRO_AUDIO_QUALITY:
         *val = mixer->quality;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to get invalid mixer enum setting");
         break;
   }
   return 1;
}

int al_mixer_get_bool(const ALLEGRO_MIXER *mixer, ALLEGRO_AUDIO_ENUM setting, bool *val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_PLAYING:
         *val = mixer->ss.playing;
         return 0;

      case ALLEGRO_AUDIO_ATTACHED:
         *val = (mixer->streams[0] != NULL);
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to get invalid mixer bool setting");
         break;
   }
   return 1;
}


int al_mixer_set_long(ALLEGRO_MIXER *mixer, ALLEGRO_AUDIO_ENUM setting, unsigned long val)
{
   switch(setting)
   {
      /* You can change the frequency of a mixer as long as it's not attached
         to anything */
      case ALLEGRO_AUDIO_FREQUENCY:
         if(mixer->ss.parent.ptr)
         {
            _al_set_error(ALLEGRO_INVALID_OBJECT, "Attempted to change the frequency of an attached mixer");
            return 1;
         }
         mixer->ss.frequency = val;
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to set invalid mixer long setting");
         break;
   }
   return 1;
}

int al_mixer_set_enum(ALLEGRO_MIXER *mixer, ALLEGRO_AUDIO_ENUM setting, ALLEGRO_AUDIO_ENUM val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_QUALITY:
         if(val != ALLEGRO_AUDIO_POINT && val != ALLEGRO_AUDIO_LINEAR)
         {
            _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to set unknown mixer quality");
            return 1;
         }

         _al_mutex_lock(mixer->ss.mutex);
         {
            size_t i;
            for(i = 0;mixer->streams[i];++i)
            {
               if(val == ALLEGRO_AUDIO_LINEAR)
               {
                  if(mixer->streams[i]->read == read_to_mixer_point32)
                     mixer->streams[i]->read = read_to_mixer_linear32;
                  else if(mixer->streams[i]->read == read_to_mixer_point32u)
                     mixer->streams[i]->read = read_to_mixer_linear32u;
               }
               else if(val == ALLEGRO_AUDIO_POINT)
               {
                  if(mixer->streams[i]->read == read_to_mixer_linear32)
                     mixer->streams[i]->read = read_to_mixer_point32;
                  else if(mixer->streams[i]->read == read_to_mixer_linear32u)
                     mixer->streams[i]->read = read_to_mixer_point32u;
               }
            }
         }
         mixer->quality = val;
         _al_mutex_unlock(mixer->ss.mutex);
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to set invalid mixer enum setting");
         break;
   }
   return 1;
}

int al_mixer_set_bool(ALLEGRO_MIXER *mixer, ALLEGRO_AUDIO_ENUM setting, bool val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_PLAYING:
         mixer->ss.playing = val;
         return 0;

      case ALLEGRO_AUDIO_ATTACHED:
         if(val)
         {
            _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to set mixer attachment status true");
            return 1;
         }
         detach_from_parent(&mixer->ss);
         return 0;

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Attempted to set invalid mixer bool setting");
         break;
   }
   return 1;
}



/* _al_voice_update:
 *   Reads the attached stream and provides a buffer for the sound card. It is
 *   the driver's responsiblity to call this and to make sure any
 *   driver-specific resources associated with the voice are locked. This
 *   should only be called for streaming sources.
 * * The return value is a pointer to the next chunk of audio data in the
 *   format the voice was allocated with. It may return NULL, in which case it
 *   is the driver's responsilibty to play silence for the voice. The returned
 *   buffer must *not* be modified.
 */
const void *_al_voice_update(ALLEGRO_VOICE *voice, unsigned long samples)
{
   void *buf = NULL;

   if(voice->stream)
   {
      _al_mutex_lock(&voice->mutex);
      voice->stream->read(voice->stream, &buf, samples, voice->depth, 0);
      _al_mutex_unlock(&voice->mutex);
   }

   return buf;
}


/* al_voice_create:
 *   Creates a voice struct and allocates a voice from the digital sound
 *   driver. The sound driver's allocate_voice function should change the
 *   voice's frequency, depth, chan_conf, and settings fields to match what is
 *   actually allocated. If it cannot create a voice with exact settings it will fail
 *   Use a mixer in such a case.
 */
ALLEGRO_VOICE *al_voice_create(unsigned long freq, ALLEGRO_AUDIO_ENUM depth, ALLEGRO_AUDIO_ENUM chan_conf)
{
   ALLEGRO_VOICE *voice = NULL;

   if(!freq)
   {
      _al_set_error(ALLEGRO_INVALID_PARAM, "Invalid Voice Frequency");
      return NULL;
   }

   voice = calloc(1, sizeof(*voice));
   if(!voice)
      return NULL;

   voice->depth     = depth;
   voice->chan_conf = chan_conf;
   voice->frequency = freq;

   _al_mutex_init(&voice->mutex);
   voice->driver = driver;

   if(driver->allocate_voice(voice) != 0)
   {
      _al_mutex_destroy(&voice->mutex);
      free(voice);
      return NULL;
   }

   return voice;
}


/* al_voice_destroy:
 *   Destroys the voice and deallocates it from the digital driver.
 */
void al_voice_destroy(ALLEGRO_VOICE *voice)
{
   if(voice)
   {
      al_voice_detach(voice);
      voice->driver->deallocate_voice(voice);
      _al_mutex_destroy(&voice->mutex);

      free(voice);
   }
}


/* al_voice_attach_sample:
 *   Attaches a sample to a voice, and allows it to play. The sample's volume
 *   and loop mode will be ignored, and it must have the same frequency and
 *   depth (including signed-ness) as the voice. This function may fail if the
 *   selected driver doesn't support preloading sample data.
 */
int al_voice_attach_sample(ALLEGRO_VOICE *voice, ALLEGRO_SAMPLE *spl)
{
   if(voice->stream)
   {
      fprintf(stderr,"Attempted to attach to a voice that already has an attachment\n");
      _al_set_error(ALLEGRO_INVALID_OBJECT, "Attempted to attach to a voice that already has an attachment");
      return 1;
   }
   if(spl->parent.ptr)
   {
       fprintf(stderr,"Attempted to attach a sample that is already attached\n");
      _al_set_error(ALLEGRO_INVALID_OBJECT, "Attempted to attach a sample that is already attached");
      return 1;
   }

   if(voice->chan_conf != spl->chan_conf ||
      voice->frequency != spl->frequency ||
      voice->depth != spl->depth)
   {
       fprintf(stderr,"sample settings do not match voice settings\n");
      _al_set_error(ALLEGRO_INVALID_OBJECT, "Sample settings do not match voice settings");
      return 1;
   }

   _al_mutex_lock(&voice->mutex);

   voice->stream = spl;

   voice->streaming = false;
   voice->num_buffers = 1;
   voice->buffer_size = (spl->len>>MIXER_FRAC_SHIFT) * al_channel_count(voice->chan_conf) * al_depth_size(voice->depth);

   spl->read = NULL;
   stream_set_mutex(spl, &voice->mutex);

   spl->parent.voice = voice;
   spl->parent_is_voice = true;

   if(voice->driver->load_voice(voice, spl->buffer.ptr) != 0 ||
      (spl->playing && voice->driver->start_voice(voice) != 0))
   {      
      voice->stream = NULL;
      spl->read = NULL;
      stream_set_mutex(spl, NULL);
      spl->parent.voice = NULL;
      _al_mutex_unlock(&voice->mutex);
      fprintf(stderr,"Unable to load sample into voice\n");
      return 1;
   }

   _al_mutex_unlock(&voice->mutex);
   return 0;
}

/* al_voice_attach_stream:
 *   Attaches an audio stream to a voice. Thes same rules as
 *   al_voice_attach_sample apply. This may fail if the driver can't create a
 *   voice with the buffer count and buffer size the stream uses.
 */
int al_voice_attach_stream(ALLEGRO_VOICE *voice, ALLEGRO_STREAM *stream)
{
   if(voice->stream)
   {
      _al_set_error(ALLEGRO_INVALID_OBJECT, "Attempted to attach to a voice that already has an attachment");
      return 1;
   }
   if(stream->spl.parent.ptr)
   {
      _al_set_error(ALLEGRO_INVALID_OBJECT, "Attempted to attach a stream that is already attached");
      return 1;
   }

   if(voice->chan_conf != stream->spl.chan_conf ||
      voice->frequency != stream->spl.frequency ||
      voice->depth != stream->spl.depth)
   {
      _al_set_error(ALLEGRO_INVALID_OBJECT, "Stream settings do not match voice settings");
      return 1;
   }

   _al_mutex_lock(&voice->mutex);

   voice->stream = &stream->spl;

   stream_set_mutex(&stream->spl, &voice->mutex);

   stream->spl.parent.voice = voice;
   stream->spl.parent_is_voice = true;

   voice->streaming = true;
   voice->num_buffers = stream->buf_count;
   voice->buffer_size = (stream->spl.len>>MIXER_FRAC_SHIFT) *
                        al_channel_count(stream->spl.chan_conf) *
                        al_depth_size(stream->spl.depth);

   stream->spl.read = stream_read;

   if(voice->driver->start_voice(voice) != 0)
   {
      voice->stream = NULL;
      stream_set_mutex(&stream->spl, NULL);
      stream->spl.parent.voice = NULL;
      stream->spl.read = NULL;

      _al_mutex_unlock(&voice->mutex);
      _al_set_error(ALLEGRO_GENERIC_ERROR, "Unable to start stream");
      return 1;
   }

   _al_mutex_unlock(&voice->mutex);
   return 0;
}

/* al_voice_attach_mixer:
 *   Attaches a mixer to a voice. Thes same rules as
 *   al_voice_attach_sample apply, with the exception of the depth requirement.
 */
int al_voice_attach_mixer(ALLEGRO_VOICE *voice, ALLEGRO_MIXER *mixer)
{
   if(voice->stream)
      return 1;
   if(mixer->ss.parent.ptr)
      return 2;

   if(voice->chan_conf != mixer->ss.chan_conf ||
      voice->frequency != mixer->ss.frequency)
      return 3;

   _al_mutex_lock(&voice->mutex);

   voice->stream = &mixer->ss;

   stream_set_mutex(&mixer->ss, &voice->mutex);

   mixer->ss.parent.voice = voice;
   mixer->ss.parent_is_voice = true;

   voice->streaming = true;
   voice->num_buffers = 0;
   voice->buffer_size = 0;

   if(voice->driver->start_voice(voice) != 0)
   {
      voice->stream = NULL;
      stream_set_mutex(&mixer->ss, NULL);
      mixer->ss.parent.voice = NULL;

      _al_mutex_unlock(&voice->mutex);
      return 1;
   }

   _al_mutex_unlock(&voice->mutex);
   return 0;
}


/* al_voice_detech:
 *   Detaches the sample or mixer stream from the voice
 */
void al_voice_detach(ALLEGRO_VOICE *voice)
{
   if(!voice->stream)
      return;

   _al_mutex_lock(&voice->mutex);

   if(!voice->streaming)
   {
      ALLEGRO_SAMPLE *spl = voice->stream;
      bool playing = false;

      al_voice_get_long(voice, ALLEGRO_AUDIO_POSITION, &spl->pos);
      spl->pos <<= MIXER_FRAC_SHIFT;
      al_voice_get_bool(voice, ALLEGRO_AUDIO_PLAYING, &playing);
      spl->playing = playing;

      voice->driver->stop_voice(voice);
      voice->driver->unload_voice(voice);
   }
   else
      voice->driver->stop_voice(voice);

   stream_set_mutex(voice->stream, NULL);
   voice->stream->parent.voice = NULL;
   voice->stream = NULL;

   _al_mutex_unlock(&voice->mutex);
}


int al_voice_get_long(const ALLEGRO_VOICE *voice, ALLEGRO_AUDIO_ENUM setting, unsigned long *val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_FREQUENCY:
         *val = voice->frequency;
         return 0;

      case ALLEGRO_AUDIO_POSITION:
      {
         if(voice->stream && !voice->streaming)
            *val = voice->driver->get_voice_position(voice);
         else
            *val = 0;
         return 0;
      }

      default:
         break;
   }
   return 1;
}

int al_voice_get_enum(const ALLEGRO_VOICE *voice, ALLEGRO_AUDIO_ENUM setting, ALLEGRO_AUDIO_ENUM *val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_CHANNELS:
         *val = voice->chan_conf;
         return 0;

      case ALLEGRO_AUDIO_DEPTH:
         *val = voice->depth;
         return 0;

      default:
         break;
   }
   return 1;
}

int al_voice_get_bool(const ALLEGRO_VOICE *voice, ALLEGRO_AUDIO_ENUM setting, bool *val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_PLAYING:
         if(voice->stream && !voice->streaming)
            *val = voice->driver->voice_is_playing(voice);
         else if(voice->stream)
            *val = true;
         else
            *val = false;
         return 0;

      default:
         break;
   }
   return 1;
}


int al_voice_set_long(ALLEGRO_VOICE *voice, ALLEGRO_AUDIO_ENUM setting, unsigned long val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_POSITION:
         if(voice->stream && !voice->streaming)
            return voice->driver->set_voice_position(voice, val);
         return 1;

      default:
         break;
   }
   return 1;
}

int al_voice_set_enum(ALLEGRO_VOICE *voice, ALLEGRO_AUDIO_ENUM setting, ALLEGRO_AUDIO_ENUM val)
{
   (void)voice;
   (void)val;

   switch(setting)
   {
      default:
         break;
   }
   return 1;
}

int al_voice_set_bool(ALLEGRO_VOICE *voice, ALLEGRO_AUDIO_ENUM setting, bool val)
{
   switch(setting)
   {
      case ALLEGRO_AUDIO_PLAYING:
         if(voice->stream && !voice->streaming)
         {
            bool playing = false;
            if (al_voice_get_bool(voice, ALLEGRO_AUDIO_PLAYING, &playing))
            {
               fprintf(stderr, "Unable to get voice playing status\n");
               return 1;
            }

            if(playing == val)
            {
               if (playing)
                  fprintf(stderr, "Voice is already playing\n");
               else
                  fprintf(stderr, "Voice is already stopped\n");
               return 1;
            }
            
            if(val)
               return voice->driver->start_voice(voice);
            else
               return voice->driver->stop_voice(voice);
         } else
         {
            fprintf(stderr, "Voice has no sample or mixer attached\n");
            return 1;
         }
      default:
         break;
   }
   return 1;
}

/* TODO: possibly take extra parameters
 * (freq, channel, etc) and test if you 
 * can create a voice with them.. if not
 * try another driver.
 */
int al_audio_init(ALLEGRO_AUDIO_ENUM mode)
{
   int retVal = 0;

   /* check to see if a driver is already installed and running */
   if (driver)
   {
      _al_set_error(ALLEGRO_GENERIC_ERROR, "A Driver already running"); 
      return 1;
   }

   switch (mode)
   {
      case ALLEGRO_AUDIO_DRIVER_AUTODETECT:
         /* check openal first then fallback on others */
         retVal = al_audio_init(ALLEGRO_AUDIO_DRIVER_OPENAL);
         if (retVal == 0)
            return retVal;

         #if defined(ALLEGRO_LINUX)
            return al_audio_init(ALLEGRO_AUDIO_DRIVER_ALSA);
         #elif defined(ALLEGRO_WINDOWS)
            return al_audio_init(ALLEGRO_AUDIO_DRIVER_DSOUND);
         #elif defined(ALLEGRO_MACOSX)
            driver = NULL;
            return 1:
         #endif

      case ALLEGRO_AUDIO_DRIVER_OPENAL:
         if (_openal_driver.open() == 0 )
         {
            fprintf(stderr, "Using OpenAL driver\n"); 
            driver = &_openal_driver;
            return 0;
         }
         return 1;
      case ALLEGRO_AUDIO_DRIVER_ALSA:
         #if defined(ALLEGRO_LINUX)
            if(_alsa_driver.open() == 0)
            {
               fprintf(stderr, "Using ALSA driver\n"); 
               driver = &_alsa_driver;
               return 0;
            }
            return 1;
         #else
            _al_set_error(ALLEGRO_INVALID_PARAM, "Alsa not available on this platform");
            return 1;
         #endif

      case ALLEGRO_AUDIO_DRIVER_DSOUND:
         #if defined(ALLEGRO_WINDOWS)
            _al_set_error(ALLEGRO_INVALID_PARAM, "DirectSound driver not yet implemented");
            return 1;
         #else
            _al_set_error(ALLEGRO_INVALID_PARAM, "DirectSound not available on this platform");
            return 1;
         #endif

      default:
         _al_set_error(ALLEGRO_INVALID_PARAM, "Invalid audio driver");
         return 1;
   }

}

void al_audio_deinit()
{
   if(driver)
      driver->close();
   driver = NULL;
}
