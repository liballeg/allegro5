/*         ______   ___    ___ 
 *        /\  _  \ /\_ \  /\_ \ 
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___ 
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Sample mixing code.
 *
 *      By Shawn Hargreaves.
 *
 *      Proper 16 bit sample support added by Salvador Eduardo Tropea.
 *
 *      Ben Davis provided the set_volume_per_voice() functionality,
 *      programmed the silent mixer so that silent voices don't freeze,
 *      and fixed a few minor bugs elsewhere.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"



typedef struct MIXER_VOICE
{
   int playing;               /* are we active? */
   int stereo;                /* mono or stereo input data? */
   unsigned char *data8;      /* data for 8 bit samples */
   unsigned short *data16;    /* data for 16 bit samples */
   long pos;                  /* fixed point position in sample */
   long diff;                 /* fixed point speed of play */
   long len;                  /* fixed point sample length */
   long loop_start;           /* fixed point loop start position */
   long loop_end;             /* fixed point loop end position */
   int lvol;                  /* left channel volume */
   int rvol;                  /* right channel volume */
} MIXER_VOICE;


#define MIX_VOLUME_LEVELS     32
#define MIX_FIX_SHIFT         8
#define MIX_FIX_SCALE         (1<<MIX_FIX_SHIFT)

#define UPDATE_FREQ           16
#define UPDATE_FREQ_SHIFT     4


/* the samples currently being played */
static MIXER_VOICE mixer_voice[MIXER_MAX_SFX];

/* temporary sample mixing buffer */
static unsigned short *mix_buffer = NULL; 

/* lookup table for converting sample volumes */
typedef signed short MIXER_VOL_TABLE[256];
static MIXER_VOL_TABLE *mix_vol_table = NULL;

/* lookup table for amplifying and clipping samples */
static unsigned short *mix_clip_table = NULL;

#define MIX_RES_16            14
#define MIX_RES_8             10

/* alternative table system for high-quality sample mixing */
#define BITS_PAN              7 
#define BITS_VOL              7 
#define BITS_MIXER_CORE       32
#define BITS_SAMPLES          16

typedef unsigned short VOLUME_T;

#define BITS_TOT (BITS_PAN+BITS_VOL) 
#define ENTRIES_VOL_TABLE (1<<BITS_TOT)
#define SIZE_VOLUME_TABLE (sizeof(VOLUME_T)*ENTRIES_VOL_TABLE)

static VOLUME_T *volume_table = NULL;

/* flags for the mixing code */
static int mix_voices;
static int mix_size;
static int mix_freq;
static int mix_stereo;
static int mix_16bit;

/* shift factor for volume per voice */
static int voice_volume_scale = -1;


static void mixer_lock_mem(void);



/* set_volume_per_voice:
 *  Enables the programmer (not the end-user) to alter the maximum volume of
 *  each voice:
 *  - pass -1 for Allegro to work as it did before this option was provided
 *    (volume dependent on number of voices),
 *  - pass 0 if you want a single centred sample to be as loud as possible
 *    without distorting,
 *  - pass 1 if you want to pan a full-volume sample to one side without
 *    distortion,
 *  - each time the scale parameter increases by 1, the volume halves.
 *
 *  This must be called _before_ install_sound().
 */
void set_volume_per_voice(int scale)
{
   voice_volume_scale = scale;
}


/* create_volume_table:
 *  Builds a volume table for the high quality 16 bit mixing mode.
 */
static int create_volume_table(int vol_scale)
{
   double step;
   double acum = 0;
   int i;

   if (!volume_table) {
      volume_table = (VOLUME_T *)malloc(SIZE_VOLUME_TABLE);
      if (!volume_table)
	 return 1;
      LOCK_DATA(volume_table, SIZE_VOLUME_TABLE);
   }

   step = (double)(32768 >> vol_scale) / ENTRIES_VOL_TABLE;

   for (i=0; i<ENTRIES_VOL_TABLE; i++, acum+=step)
      volume_table[i] = acum;

   return 0;
}



/* _mixer_init:
 *  Initialises the sample mixing code, returning 0 on success. You should
 *  pass it the number of samples you want it to mix each time the refill
 *  buffer routine is called, the sample rate to mix at, and two flags 
 *  indicating whether the mixing should be done in stereo or mono and with 
 *  eight or sixteen bits. The bufsize parameter is the number of samples,
 *  not bytes. It should take into account whether you are working in stereo 
 *  or not (eg. double it if in stereo), but it should not be affected by
 *  whether each sample is 8 or 16 bits.
 */
int _mixer_init(int bufsize, int freq, int stereo, int is16bit, int *voices)
{
   int i, j;
   int clip_size;
   int clip_scale;
   int clip_max;
   int mix_vol_scale;

   mix_voices = 1;
   mix_vol_scale = -1;

   while ((mix_voices < MIXER_MAX_SFX) && (mix_voices < *voices)) {
      mix_voices <<= 1;
      mix_vol_scale++;
   }

   if (voice_volume_scale >= 0)
      mix_vol_scale = voice_volume_scale;
   else {
      /* backward compatibility with 3.12 version */
      if (mix_vol_scale < 2)
         mix_vol_scale = 2;
   }

   *voices = mix_voices;

   mix_size = bufsize;
   mix_freq = freq;
   mix_stereo = stereo;
   mix_16bit = is16bit;

   for (i=0; i<MIXER_MAX_SFX; i++) {
      mixer_voice[i].playing = FALSE;
      mixer_voice[i].data8 = NULL;
      mixer_voice[i].data16 = NULL;
   }

   /* temporary buffer for sample mixing */
   mix_buffer = malloc(mix_size*sizeof(short));
   if (!mix_buffer)
      return -1;

   LOCK_DATA(mix_buffer, mix_size*sizeof(short));

   /* volume table for mixing samples into the temporary buffer */
   mix_vol_table = malloc(sizeof(MIXER_VOL_TABLE) * MIX_VOLUME_LEVELS);
   if (!mix_vol_table) {
      free(mix_buffer);
      mix_buffer = NULL;
      return -1;
   }

   LOCK_DATA(mix_vol_table, sizeof(MIXER_VOL_TABLE) * MIX_VOLUME_LEVELS);

   for (j=0; j<MIX_VOLUME_LEVELS; j++)
      for (i=0; i<256; i++)
         mix_vol_table[j][i] = ((i-128) * j * 128 / MIX_VOLUME_LEVELS) >> mix_vol_scale;

   if ((_sound_hq) && (mix_stereo) && (mix_16bit)) {
      /* make high quality table if requested and output is 16 bit stereo */
      if (create_volume_table(mix_vol_scale) != 0)
	 return -1;
   }
   else
      _sound_hq = 0;

   /* lookup table for amplifying and clipping sample buffers */
   if (mix_16bit) {
      clip_size = 1 << MIX_RES_16;
      clip_scale = 18 - MIX_RES_16;
      clip_max = 0xFFFF;
   }
   else {
      clip_size = 1 << MIX_RES_8;
      clip_scale = 10 - MIX_RES_8;
      clip_max = 0xFF;
   }

   /* We now always use a clip table, owing to the new set_volume_per_voice()
    * functionality. It is not a big loss in performance.
    */
   mix_clip_table = malloc(sizeof(short) * clip_size);
   if (!mix_clip_table) {
      free(mix_buffer);
      mix_buffer = NULL;
      free(mix_vol_table);
      mix_vol_table = NULL;
      free(volume_table);
      volume_table = NULL;
      return -1;
   }

   LOCK_DATA(mix_clip_table, sizeof(short) * clip_size);

   /* clip extremes of the sample range */
   for (i=0; i<clip_size*3/8; i++) {
      mix_clip_table[i] = 0;
      mix_clip_table[clip_size-1-i] = clip_max;
   }

   for (i=0; i<clip_size/4; i++)
      mix_clip_table[clip_size*3/8 + i] = i<<clip_scale;

   mixer_lock_mem();

   return 0;
}



/* _mixer_exit:
 *  Cleans up the sample mixer code when you are done with it.
 */
void _mixer_exit(void)
{
   if (mix_buffer) {
      free(mix_buffer);
      mix_buffer = NULL;
   }

   if (mix_vol_table) {
      free(mix_vol_table);
      mix_vol_table = NULL;
   }

   if (mix_clip_table) {
      free(mix_clip_table);
      mix_clip_table = NULL;
   }

   if (volume_table) {
      free(volume_table);
      volume_table = NULL;
   }
}



/* update_mixer_volume:
 *  Called whenever the voice volume or pan changes, to update the mixer 
 *  amplification table indexes.
 */
static void update_mixer_volume(MIXER_VOICE *mv, PHYS_VOICE *pv)
{
   int vol, pan, lvol, rvol;

   if (_sound_hq) {
      vol = pv->vol>>13;
      pan = pv->pan>>13;

      /* no need to check for mix_stereo if we're using hq */
      lvol = vol*(127-pan);
      rvol = vol*pan;

      /* adjust for 127*127<128*128-1 */
      lvol += lvol>>6;
      rvol += rvol>>6;

      mv->lvol = MID(0, lvol, ENTRIES_VOL_TABLE-1);
      mv->rvol = MID(0, rvol, ENTRIES_VOL_TABLE-1);
   }
   else {
      vol = pv->vol >> 12;
      pan = pv->pan >> 12;

      if (mix_stereo) {
	 lvol = vol * (256-pan) * MIX_VOLUME_LEVELS / 65536;
	 rvol = vol * pan * MIX_VOLUME_LEVELS / 65536;
      }
      else if (mv->stereo) {
	 lvol = vol * (256-pan) * MIX_VOLUME_LEVELS / 131072;
	 rvol = vol * pan * MIX_VOLUME_LEVELS / 131072;
      }
      else
	 lvol = rvol = vol * MIX_VOLUME_LEVELS / 512;

      mv->lvol = MID(0, lvol, MIX_VOLUME_LEVELS-1);
      mv->rvol = MID(0, rvol, MIX_VOLUME_LEVELS-1);
   }
}

END_OF_STATIC_FUNCTION(update_mixer_volume);



/* update_mixer_freq:
 *  Called whenever the voice frequency changes, to update the sample
 *  delta value.
 */
static INLINE void update_mixer_freq(MIXER_VOICE *mv, PHYS_VOICE *pv)
{
   mv->diff = (pv->freq >> (12 - MIX_FIX_SHIFT)) / mix_freq;

   if (pv->playmode & PLAYMODE_BACKWARD)
      mv->diff = -mv->diff;
}



/* update_mixer:
 *  Helper for updating the volume ramp and pitch/pan sweep status.
 */
static void update_mixer(MIXER_VOICE *spl, PHYS_VOICE *voice, int len)
{
   if ((len & (UPDATE_FREQ-1)) == 0) {
      if ((voice->dvol) || (voice->dpan)) {
	 /* update volume ramp */
	 if (voice->dvol) {
	    voice->vol += voice->dvol;
	    if (((voice->dvol > 0) && (voice->vol >= voice->target_vol)) ||
		((voice->dvol < 0) && (voice->vol <= voice->target_vol))) {
	       voice->vol = voice->target_vol; 
	       voice->dvol = 0; 
	    } 
	 } 

	 /* update pan sweep */ 
	 if (voice->dpan) { 
	    voice->pan += voice->dpan; 
	    if (((voice->dpan > 0) && (voice->pan >= voice->target_pan)) ||
		((voice->dpan < 0) && (voice->pan <= voice->target_pan))) { 
	       voice->pan = voice->target_pan; 
	       voice->dpan = 0; 
	    } 
	 } 

	 update_mixer_volume(spl, voice); 
      } 

      /* update frequency sweep */ 
      if (voice->dfreq) { 
	 voice->freq += voice->dfreq; 
	 if (((voice->dfreq > 0) && (voice->freq >= voice->target_freq)) || 
	     ((voice->dfreq < 0) && (voice->freq <= voice->target_freq))) { 
	    voice->freq = voice->target_freq; 
	    voice->dfreq = 0; 
	 } 

	 update_mixer_freq(spl, voice); 
      } 
   } 
}

END_OF_STATIC_FUNCTION(update_mixer);

/* update_silent_mixer:
 *  Another helper for updating the volume ramp and pitch/pan sweep status.
 *  This version is designed for the silent mixer, and it is called just once
 *  per buffer. The len parameter is used to work out how much the values
 *  must be adjusted.
 */
static void update_silent_mixer(MIXER_VOICE *spl, PHYS_VOICE *voice, int len)
{
   len >>= UPDATE_FREQ_SHIFT;

   if (voice->dpan) {
      /* update pan sweep */
      voice->pan += voice->dpan * len;
      if (((voice->dpan > 0) && (voice->pan >= voice->target_pan)) ||
	  ((voice->dpan < 0) && (voice->pan <= voice->target_pan))) {
	 voice->pan = voice->target_pan;
	 voice->dpan = 0;
      }
   }

   /* update frequency sweep */
   if (voice->dfreq) {
      voice->freq += voice->dfreq * len;
      if (((voice->dfreq > 0) && (voice->freq >= voice->target_freq)) ||
	  ((voice->dfreq < 0) && (voice->freq <= voice->target_freq))) {
         voice->freq = voice->target_freq;
	 voice->dfreq = 0;
      }

      update_mixer_freq(spl, voice);
   }
}

END_OF_STATIC_FUNCTION(update_silent_mixer);



/* helper for constructing the body of a sample mixing routine */
#define MIXER()                                                              \
{                                                                            \
   if ((voice->playmode & PLAYMODE_LOOP) &&                                  \
       (spl->loop_start < spl->loop_end)) {                                  \
									     \
      if (voice->playmode & PLAYMODE_BACKWARD) {                             \
	 /* mix a backward looping sample */                                 \
	 while (len-- > 0) {                                                 \
	    MIX();                                                           \
	    spl->pos += spl->diff;                                           \
	    if (spl->pos < spl->loop_start) {                                \
	       if (voice->playmode & PLAYMODE_BIDIR) {                       \
		  spl->diff = -spl->diff;                                    \
                  /* however far the sample has overshot, move it the same */\
                  /* distance from the loop point, within the loop section */\
                  spl->pos = (spl->loop_start << 1) - spl->pos;              \
		  voice->playmode ^= PLAYMODE_BACKWARD;                      \
	       }                                                             \
	       else                                                          \
		  spl->pos += (spl->loop_end - spl->loop_start);             \
	    }                                                                \
	    update_mixer(spl, voice, len);                                   \
	 }                                                                   \
      }                                                                      \
      else {                                                                 \
	 /* mix a forward looping sample */                                  \
	 while (len-- > 0) {                                                 \
	    MIX();                                                           \
	    spl->pos += spl->diff;                                           \
	    if (spl->pos >= spl->loop_end) {                                 \
	       if (voice->playmode & PLAYMODE_BIDIR) {                       \
		  spl->diff = -spl->diff;                                    \
                  /* however far the sample has overshot, move it the same */\
                  /* distance from the loop point, within the loop section */\
                  spl->pos = ((spl->loop_end - 1) << 1) - spl->pos;          \
		  voice->playmode ^= PLAYMODE_BACKWARD;                      \
	       }                                                             \
	       else                                                          \
		  spl->pos -= (spl->loop_end - spl->loop_start);             \
	    }                                                                \
	    update_mixer(spl, voice, len);                                   \
	 }                                                                   \
      }                                                                      \
   }                                                                         \
   else {                                                                    \
      /* mix a non-looping sample */                                         \
      while (len-- > 0) {                                                    \
	 MIX();                                                              \
	 spl->pos += spl->diff;                                              \
	 if ((unsigned long)spl->pos >= (unsigned long)spl->len) {           \
	    /* note: we don't need a different version for reverse play, */  \
	    /* as this will wrap and automatically do the Right Thing */     \
	    spl->playing = FALSE;                                            \
	    return;                                                          \
	 }                                                                   \
	 update_mixer(spl, voice, len);                                      \
      }                                                                      \
   }                                                                         \
}


/* mix_silent_samples:
 *  This is used when the voice is silent, instead of the other
 *  mix_*_samples() functions. It just extrapolates the sample position,
 *  and stops the sample if it reaches the end and isn't set to loop.
 *  Since no mixing is necessary, this function is much faster than
 *  its friends. In addition, no buffer parameter is required,
 *  and the same function can be used for all sample types.
 *
 *  There is a catch. All the mix_stereo_*_samples() and
 *  mix_hq?_*_samples() functions (those which write to a stereo mixing
 *  buffer) divide len by 2 before using it in the MIXER() macro.
 *  Therefore, all the mix_silent_samples() for stereo buffers must divide
 *  the len parameter by 2. This is done in _mix_some_samples().
 */
static void mix_silent_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, int len)
{
   if ((voice->playmode & PLAYMODE_LOOP) &&
       (spl->loop_start < spl->loop_end)) {

      if (voice->playmode & PLAYMODE_BACKWARD) {
	 /* mix a backward looping sample */
         spl->pos += spl->diff * len;
         if (spl->pos < spl->loop_start) {
            if (voice->playmode & PLAYMODE_BIDIR) {
               do {
                  spl->diff = -spl->diff;
                  spl->pos = (spl->loop_start << 1) - spl->pos;
                  voice->playmode ^= PLAYMODE_BACKWARD;
                  if (spl->pos < spl->loop_end) break;
                  spl->diff = -spl->diff;
                  spl->pos = ((spl->loop_end - 1) << 1) - spl->pos;
                  voice->playmode ^= PLAYMODE_BACKWARD;
               } while (spl->pos < spl->loop_start);
            }
            else {
               do {
                  spl->pos += (spl->loop_end - spl->loop_start);
               } while (spl->pos < spl->loop_start);
            }
         }
         update_silent_mixer(spl, voice, len);
      }
      else {
	 /* mix a forward looping sample */
         spl->pos += spl->diff * len;
         if (spl->pos >= spl->loop_end) {
            if (voice->playmode & PLAYMODE_BIDIR) {
               do {
                  spl->diff = -spl->diff;
                  spl->pos = ((spl->loop_end - 1) << 1) - spl->pos;
                  voice->playmode ^= PLAYMODE_BACKWARD;
                  if (spl->pos >= spl->loop_start) break;
                  spl->diff = -spl->diff;
                  spl->pos = (spl->loop_start << 1) - spl->pos;
                  voice->playmode ^= PLAYMODE_BACKWARD;
               } while (spl->pos >= spl->loop_end);
            }
            else {
               do {
                  spl->pos -= (spl->loop_end - spl->loop_start);
               } while (spl->pos >= spl->loop_end);
            }
	 }
         update_silent_mixer(spl, voice, len);
      }
   }
   else {
      /* mix a non-looping sample */
      spl->pos += spl->diff * len;
      if ((unsigned long)spl->pos >= (unsigned long)spl->len) {
	 /* note: we don't need a different version for reverse play, */
	 /* as this will wrap and automatically do the Right Thing */
	 spl->playing = FALSE;
	 return;
      }
      update_silent_mixer(spl, voice, len);
   }
}

END_OF_STATIC_FUNCTION(mix_silent_samples);



/* mix_mono_8x1_samples:
 *  Mixes from an eight bit sample into a mono buffer, until either len 
 *  samples have been mixed or until the end of the sample is reached.
 */
static void mix_mono_8x1_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   signed short *vol = (short *)(mix_vol_table + spl->lvol);

   #define MIX()                                                             \
      *(buf++) += vol[spl->data8[spl->pos>>MIX_FIX_SHIFT]];

   MIXER();

   #undef MIX
}

END_OF_STATIC_FUNCTION(mix_mono_8x1_samples);



/* mix_mono_8x2_samples:
 *  Mixes from an eight bit stereo sample into a mono buffer, until either 
 *  len samples have been mixed or until the end of the sample is reached.
 */
static void mix_mono_8x2_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   signed short *lvol = (short *)(mix_vol_table + spl->lvol);
   signed short *rvol = (short *)(mix_vol_table + spl->rvol);

   #define MIX()                                                             \
      *(buf)   += lvol[spl->data8[(spl->pos>>MIX_FIX_SHIFT)*2]];             \
      *(buf++) += rvol[spl->data8[(spl->pos>>MIX_FIX_SHIFT)*2+1]];

   MIXER();

   #undef MIX
}

END_OF_STATIC_FUNCTION(mix_mono_8x2_samples);



/* mix_mono_16x1_samples:
 *  Mixes from a 16 bit sample into a mono buffer, until either len samples 
 *  have been mixed or until the end of the sample is reached.
 */
static void mix_mono_16x1_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   signed short *vol = (short *)(mix_vol_table + spl->lvol);

   #define MIX()                                                             \
      *(buf++) += vol[(spl->data16[spl->pos>>MIX_FIX_SHIFT])>>8];

   MIXER();

   #undef MIX
}

END_OF_STATIC_FUNCTION(mix_mono_16x1_samples);



/* mix_mono_16x2_samples:
 *  Mixes from a 16 bit stereo sample into a mono buffer, until either len 
 *  samples have been mixed or until the end of the sample is reached.
 */
static void mix_mono_16x2_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   signed short *lvol = (short *)(mix_vol_table + spl->lvol);
   signed short *rvol = (short *)(mix_vol_table + spl->rvol);

   #define MIX()                                                             \
      *(buf)   += lvol[(spl->data16[(spl->pos>>MIX_FIX_SHIFT)*2])>>8];       \
      *(buf++) += rvol[(spl->data16[(spl->pos>>MIX_FIX_SHIFT)*2+1])>>8];

   MIXER();

   #undef MIX
}

END_OF_STATIC_FUNCTION(mix_mono_16x2_samples);



/* mix_stereo_8x1_samples:
 *  Mixes from an eight bit sample into a stereo buffer, until either len 
 *  samples have been mixed or until the end of the sample is reached.
 */
static void mix_stereo_8x1_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   signed short *lvol = (short *)(mix_vol_table + spl->lvol);
   signed short *rvol = (short *)(mix_vol_table + spl->rvol);

   len >>= 1;

   #define MIX()                                                             \
      *(buf++) += lvol[spl->data8[spl->pos>>MIX_FIX_SHIFT]];                 \
      *(buf++) += rvol[spl->data8[spl->pos>>MIX_FIX_SHIFT]];

   MIXER();

   #undef MIX
}

END_OF_STATIC_FUNCTION(mix_stereo_8x1_samples);



/* mix_stereo_8x2_samples:
 *  Mixes from an eight bit stereo sample into a stereo buffer, until either 
 *  len samples have been mixed or until the end of the sample is reached.
 */
static void mix_stereo_8x2_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   signed short *lvol = (short *)(mix_vol_table + spl->lvol);
   signed short *rvol = (short *)(mix_vol_table + spl->rvol);

   len >>= 1;

   #define MIX()                                                             \
      *(buf++) += lvol[spl->data8[(spl->pos>>MIX_FIX_SHIFT)*2]];             \
      *(buf++) += rvol[spl->data8[(spl->pos>>MIX_FIX_SHIFT)*2+1]];

   MIXER();

   #undef MIX
}

END_OF_STATIC_FUNCTION(mix_stereo_8x2_samples);



/* mix_stereo_16x1_samples:
 *  Mixes from a 16 bit sample into a stereo buffer, until either len samples 
 *  have been mixed or until the end of the sample is reached.
 */
static void mix_stereo_16x1_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   signed short *lvol = (short *)(mix_vol_table + spl->lvol);
   signed short *rvol = (short *)(mix_vol_table + spl->rvol);

   len >>= 1;

   #define MIX()                                                             \
      *(buf++) += lvol[(spl->data16[spl->pos>>MIX_FIX_SHIFT])>>8];           \
      *(buf++) += rvol[(spl->data16[spl->pos>>MIX_FIX_SHIFT])>>8];

   MIXER();

   #undef MIX
}

END_OF_STATIC_FUNCTION(mix_stereo_16x1_samples);



/* mix_stereo_16x2_samples:
 *  Mixes from a 16 bit stereo sample into a stereo buffer, until either len 
 *  samples have been mixed or until the end of the sample is reached.
 */
static void mix_stereo_16x2_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   signed short *lvol = (short *)(mix_vol_table + spl->lvol);
   signed short *rvol = (short *)(mix_vol_table + spl->rvol);

   len >>= 1;

   #define MIX()                                                             \
      *(buf++) += lvol[(spl->data16[(spl->pos>>MIX_FIX_SHIFT)*2])>>8];       \
      *(buf++) += rvol[(spl->data16[(spl->pos>>MIX_FIX_SHIFT)*2+1])>>8];

   MIXER();

   #undef MIX
}

END_OF_STATIC_FUNCTION(mix_stereo_16x2_samples);



/* mix_hq1_8x1_samples:
 *  Mixes from a mono 8 bit sample into a high quality stereo buffer, 
 *  until either len samples have been mixed or until the end of the 
 *  sample is reached.
 */
static void mix_hq1_8x1_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   int lvol = volume_table[spl->lvol];
   int rvol = volume_table[spl->rvol];

   len >>= 1;

   #define MIX()                                                             \
      *(buf++) += ((spl->data8[spl->pos>>MIX_FIX_SHIFT]-0x80)*lvol)>>8;      \
      *(buf++) += ((spl->data8[spl->pos>>MIX_FIX_SHIFT]-0x80)*rvol)>>8;

   MIXER();

   #undef MIX
}

END_OF_STATIC_FUNCTION(mix_hq1_8x1_samples);



/* mix_hq1_8x2_samples:
 *  Mixes from a stereo 8 bit sample into a high quality stereo buffer, 
 *  until either len samples have been mixed or until the end of the 
 *  sample is reached.
 */
static void mix_hq1_8x2_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   int lvol = volume_table[spl->lvol];
   int rvol = volume_table[spl->rvol];

   len >>= 1;

   #define MIX()                                                             \
      *(buf++) += ((spl->data8[(spl->pos>>MIX_FIX_SHIFT)*2]-0x80)*lvol)>>8;  \
      *(buf++) += ((spl->data8[(spl->pos>>MIX_FIX_SHIFT)*2+1]-0x80)*rvol)>>8;

   MIXER();

   #undef MIX
}

END_OF_STATIC_FUNCTION(mix_hq1_8x2_samples);



/* mix_hq1_16x1_samples:
 *  Mixes from a mono 16 bit sample into a high-quality stereo buffer, 
 *  until either len samples have been mixed or until the end of the sample 
 *  is reached.
 */
static void mix_hq1_16x1_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   int lvol = volume_table[spl->lvol];
   int rvol = volume_table[spl->rvol];

   len >>= 1;

   #define MIX()                                                             \
      *(buf++) += ((spl->data16[spl->pos>>MIX_FIX_SHIFT]-0x8000)*lvol)>>16;  \
      *(buf++) += ((spl->data16[spl->pos>>MIX_FIX_SHIFT]-0x8000)*rvol)>>16;

   MIXER();

   #undef MIX
}

END_OF_STATIC_FUNCTION(mix_hq1_16x1_samples);



/* mix_hq1_16x2_samples:
 *  Mixes from a stereo 16 bit sample into a high-quality stereo buffer, 
 *  until either len samples have been mixed or until the end of the sample 
 *  is reached.
 */
static void mix_hq1_16x2_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   int lvol = volume_table[spl->lvol];
   int rvol = volume_table[spl->rvol];

   len >>= 1;

   #define MIX()                                                                 \
      *(buf++) += ((spl->data16[(spl->pos>>MIX_FIX_SHIFT)*2]-0x8000)*lvol)>>16;  \
      *(buf++) += ((spl->data16[(spl->pos>>MIX_FIX_SHIFT)*2+1]-0x8000)*rvol)>>16;

   MIXER();

   #undef MIX
}

END_OF_STATIC_FUNCTION(mix_hq1_16x2_samples);



/* mix_hq2_8x1_samples:
 *  Mixes from a mono 8 bit sample into an interpolated stereo buffer, 
 *  until either len samples have been mixed or until the end of the 
 *  sample is reached.
 */
static void mix_hq2_8x1_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   int lvol = volume_table[spl->lvol];
   int rvol = volume_table[spl->rvol];
   int v, v1, v2;

   len >>= 1;

   #define MIX()                                                             \
      v = spl->pos>>MIX_FIX_SHIFT;                                           \
									     \
      v1 = spl->data8[v];                                                    \
                                                                             \
      if (spl->pos >= spl->len-MIX_FIX_SCALE) {                              \
         if ((voice->playmode & (PLAYMODE_LOOP |                             \
                                 PLAYMODE_BIDIR)) == PLAYMODE_LOOP &&        \
             spl->loop_start < spl->loop_end && spl->loop_end == spl->len)   \
            v2 = spl->data8[spl->loop_start>>MIX_FIX_SHIFT];                 \
         else                                                                \
            v2 = 0x80;                                                       \
      }                                                                      \
      else                                                                   \
	 v2 = spl->data8[v+1];                                               \
									     \
      v = spl->pos & (MIX_FIX_SCALE-1);                                      \
      v = (v1*(MIX_FIX_SCALE-v) + v2*v) / MIX_FIX_SCALE;                     \
									     \
      *(buf++) += ((v-0x80)*lvol)>>8;                                        \
      *(buf++) += ((v-0x80)*rvol)>>8;

   MIXER();

   #undef MIX
}

END_OF_STATIC_FUNCTION(mix_hq2_8x1_samples);



/* mix_hq2_8x2_samples:
 *  Mixes from a stereo 8 bit sample into an interpolated stereo buffer, 
 *  until either len samples have been mixed or until the end of the 
 *  sample is reached.
 */
static void mix_hq2_8x2_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   int lvol = volume_table[spl->lvol];
   int rvol = volume_table[spl->rvol];
   int v, va, v1a, v2a, vb, v1b, v2b;

   len >>= 1;

   #define MIX()                                                             \
      v = (spl->pos>>MIX_FIX_SHIFT) << 1; /* x2 for stereo */                \
                                                                             \
      v1a = spl->data8[v];                                                   \
      v1b = spl->data8[v+1];                                                 \
									     \
      if (spl->pos >= spl->len-MIX_FIX_SCALE) {                              \
         if ((voice->playmode & (PLAYMODE_LOOP |                             \
                                 PLAYMODE_BIDIR)) == PLAYMODE_LOOP &&        \
             spl->loop_start < spl->loop_end && spl->loop_end == spl->len) { \
            v2a = spl->data8[((spl->loop_start>>MIX_FIX_SHIFT)<<1)];         \
            v2b = spl->data8[((spl->loop_start>>MIX_FIX_SHIFT)<<1)+1];       \
         }                                                                   \
         else                                                                \
            v2a = v2b = 0x80;                                                \
      }                                                                      \
      else {                                                                 \
	 v2a = spl->data8[v+2];                                              \
	 v2b = spl->data8[v+3];                                              \
      }                                                                      \
									     \
      v = spl->pos & (MIX_FIX_SCALE-1);                                      \
      va = (v1a*(MIX_FIX_SCALE-v) + v2a*v) / MIX_FIX_SCALE;                  \
      vb = (v1b*(MIX_FIX_SCALE-v) + v2b*v) / MIX_FIX_SCALE;                  \
									     \
      *(buf++) += ((va-0x80)*lvol)>>8;                                       \
      *(buf++) += ((vb-0x80)*rvol)>>8;

   MIXER();

   #undef MIX
}

END_OF_STATIC_FUNCTION(mix_hq2_8x2_samples);



/* mix_hq2_16x1_samples:
 *  Mixes from a mono 16 bit sample into an interpolated stereo buffer, 
 *  until either len samples have been mixed or until the end of the sample 
 *  is reached.
 */
static void mix_hq2_16x1_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   int lvol = volume_table[spl->lvol];
   int rvol = volume_table[spl->rvol];
   int v, v1, v2;

   len >>= 1;

   #define MIX()                                                             \
      v = spl->pos>>MIX_FIX_SHIFT;                                           \
                                                                             \
      v1 = spl->data16[v];                                                   \
									     \
      if (spl->pos >= spl->len-MIX_FIX_SCALE) {                              \
         if ((voice->playmode & (PLAYMODE_LOOP |                             \
                                 PLAYMODE_BIDIR)) == PLAYMODE_LOOP &&        \
             spl->loop_start < spl->loop_end && spl->loop_end == spl->len)   \
            v2 = spl->data16[spl->loop_start>>MIX_FIX_SHIFT];                \
         else                                                                \
            v2 = 0x8000;                                                     \
      }                                                                      \
      else                                                                   \
	 v2 = spl->data16[v+1];                                              \
									     \
      v = spl->pos & (MIX_FIX_SCALE-1);                                      \
      v = (v1*(MIX_FIX_SCALE-v) + v2*v) / MIX_FIX_SCALE;                     \
									     \
      *(buf++) += ((v-0x8000)*lvol)>>16;                                     \
      *(buf++) += ((v-0x8000)*rvol)>>16;

   MIXER();

   #undef MIX
}

END_OF_STATIC_FUNCTION(mix_hq2_16x1_samples);



/* mix_hq2_16x2_samples:
 *  Mixes from a stereo 16 bit sample into an interpolated stereo buffer, 
 *  until either len samples have been mixed or until the end of the sample 
 *  is reached.
 */
static void mix_hq2_16x2_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   int lvol = volume_table[spl->lvol];
   int rvol = volume_table[spl->rvol];
   int v, va, v1a, v2a, vb, v1b, v2b;

   len >>= 1;

   #define MIX()                                                             \
      v = (spl->pos>>MIX_FIX_SHIFT) << 1; /* x2 for stereo */                \
                                                                             \
      v1a = spl->data16[v];                                                  \
      v1b = spl->data16[v+1];                                                \
									     \
      if (spl->pos >= spl->len-MIX_FIX_SCALE) {                              \
         if ((voice->playmode & (PLAYMODE_LOOP |                             \
                                 PLAYMODE_BIDIR)) == PLAYMODE_LOOP &&        \
             spl->loop_start < spl->loop_end && spl->loop_end == spl->len) { \
            v2a = spl->data16[((spl->loop_start>>MIX_FIX_SHIFT)<<1)];        \
            v2b = spl->data16[((spl->loop_start>>MIX_FIX_SHIFT)<<1)+1];      \
         }                                                                   \
         else                                                                \
            v2a = v2b = 0x8000;                                              \
      }                                                                      \
      else {                                                                 \
	 v2a = spl->data16[v+2];                                             \
	 v2b = spl->data16[v+3];                                             \
      }                                                                      \
									     \
      v = spl->pos & (MIX_FIX_SCALE-1);                                      \
      va = (v1a*(MIX_FIX_SCALE-v) + v2a*v) / MIX_FIX_SCALE;                  \
      vb = (v1b*(MIX_FIX_SCALE-v) + v2b*v) / MIX_FIX_SCALE;                  \
									     \
      *(buf++) += ((va-0x8000)*lvol)>>16;                                    \
      *(buf++) += ((vb-0x8000)*rvol)>>16;

   MIXER();

   #undef MIX
}

END_OF_STATIC_FUNCTION(mix_hq2_16x2_samples);



/* _mix_some_samples:
 *  Mixes samples into a buffer in conventional memory (the buf parameter
 *  should be a linear offset into the specified segment), using the buffer 
 *  size, sample frequency, etc, set when you called _mixer_init(). This 
 *  should be called by the hardware end-of-buffer interrupt routine to 
 *  get the next buffer full of samples to DMA to the card.
 */
void _mix_some_samples(unsigned long buf, unsigned short seg, int issigned)
{
   int i;
   unsigned short *p = mix_buffer;
   unsigned long *l = (unsigned long *)p;

   /* clear mixing buffer */
   for (i=0; i<mix_size/2; i++)
      *(l++) = 0x80008000;

   if (_sound_hq >= 2) {
      /* top quality interpolated 16 bit mixing */
      for (i=0; i<mix_voices; i++) {
	 if (mixer_voice[i].playing) {
            if ((_phys_voice[i].vol > 0) || (_phys_voice[i].dvol > 0)) {
	       if (mixer_voice[i].stereo) {
	          /* stereo input -> interpolated output */
	          if (mixer_voice[i].data8)
		     mix_hq2_8x2_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
	          else
		     mix_hq2_16x2_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
	       }
	       else {
	          /* mono input -> interpolated output */
	          if (mixer_voice[i].data8)
		     mix_hq2_8x1_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
	          else
		     mix_hq2_16x1_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
	       }
            }
            else
               mix_silent_samples(mixer_voice+i, _phys_voice+i, mix_size>>1);
	 }
      }
   }
   else if (_sound_hq) {
      /* high quality 16 bit mixing */
      for (i=0; i<mix_voices; i++) {
	 if (mixer_voice[i].playing) {
	    if ((_phys_voice[i].vol > 0) || (_phys_voice[i].dvol > 0)) {
	       if (mixer_voice[i].stereo) {
	          /* stereo input -> high quality output */
	          if (mixer_voice[i].data8)
		     mix_hq1_8x2_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
	          else
		     mix_hq1_16x2_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
	       }
	       else {
	          /* mono input -> high quality output */
	          if (mixer_voice[i].data8)
		     mix_hq1_8x1_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
	          else
		     mix_hq1_16x1_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
	       }
            }
            else
               mix_silent_samples(mixer_voice+i, _phys_voice+i, mix_size>>1);
	 }
      }
   }
   else if (mix_stereo) { 
      /* lower quality (faster) stereo mixing */
      for (i=0; i<mix_voices; i++) {
	 if (mixer_voice[i].playing) {
	    if ((_phys_voice[i].vol > 0) || (_phys_voice[i].dvol > 0)) {
	       if (mixer_voice[i].stereo) {
	          /* stereo input -> stereo output */
	          if (mixer_voice[i].data8)
		     mix_stereo_8x2_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
	          else
		     mix_stereo_16x2_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
	       }
	       else {
	          /* mono input -> stereo output */
	          if (mixer_voice[i].data8)
		     mix_stereo_8x1_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
	          else
		     mix_stereo_16x1_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
	       }
            }
            else
               mix_silent_samples(mixer_voice+i, _phys_voice+i, mix_size>>1);
	 }
      }
   }
   else {
      /* lower quality (fast) mono mixing */
      for (i=0; i<mix_voices; i++) {
	 if (mixer_voice[i].playing) {
	    if ((_phys_voice[i].vol > 0) || (_phys_voice[i].dvol > 0)) {
	       if (mixer_voice[i].stereo) {
	          /* stereo input -> mono output */
	          if (mixer_voice[i].data8)
		     mix_mono_8x2_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
	          else
		     mix_mono_16x2_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
	       }
	       else {
	          /* mono input -> mono output */
	          if (mixer_voice[i].data8)
		     mix_mono_8x1_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
	          else
		     mix_mono_16x1_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
	       }
            }
            else
               mix_silent_samples(mixer_voice+i, _phys_voice+i, mix_size);
	 }
      }
   }

   _farsetsel(seg);

   /* transfer to conventional memory buffer using a clip table */
   if (mix_16bit) {
      if (issigned) {
	 for (i=0; i<mix_size; i++) {
	    _farnspokew(buf, mix_clip_table[*p >> (16-MIX_RES_16)] ^ 0x8000);
	    buf += sizeof(short);
	    p++;
         }
      }
      else {
	 for (i=0; i<mix_size; i++) {
	    _farnspokew(buf, mix_clip_table[*p >> (16-MIX_RES_16)]);
            buf += sizeof(short);
            p++;
         }
      }
   }
   else {
      for (i=0; i<mix_size; i++) {
	 _farnspokeb(buf, mix_clip_table[*p >> (16-MIX_RES_8)]);
         buf++;
         p++;
      }
   }
}

END_OF_FUNCTION(_mix_some_samples);



/* _mixer_init_voice:
 *  Initialises the specificed voice ready for playing a sample.
 */
void _mixer_init_voice(int voice, AL_CONST SAMPLE *sample)
{
   mixer_voice[voice].playing = FALSE;
   mixer_voice[voice].stereo = sample->stereo;
   mixer_voice[voice].pos = 0;
   mixer_voice[voice].len = sample->len << MIX_FIX_SHIFT;
   mixer_voice[voice].loop_start = sample->loop_start << MIX_FIX_SHIFT;
   mixer_voice[voice].loop_end = sample->loop_end << MIX_FIX_SHIFT;

   if (sample->bits == 8) {
      mixer_voice[voice].data8 = sample->data;
      mixer_voice[voice].data16 = NULL;
   }
   else {
      mixer_voice[voice].data8 = NULL;
      mixer_voice[voice].data16 = sample->data;
   }

   update_mixer_volume(mixer_voice+voice, _phys_voice+voice);
   update_mixer_freq(mixer_voice+voice, _phys_voice+voice);
}

END_OF_FUNCTION(_mixer_init_voice);



/* _mixer_release_voice:
 *  Releases a voice when it is no longer required.
 */
void _mixer_release_voice(int voice)
{
   mixer_voice[voice].playing = FALSE;
   mixer_voice[voice].data8 = NULL;
   mixer_voice[voice].data16 = NULL;
}

END_OF_FUNCTION(_mixer_release_voice);



/* _mixer_start_voice:
 *  Activates a voice, with the currently selected parameters.
 */
void _mixer_start_voice(int voice)
{
   if (mixer_voice[voice].pos >= mixer_voice[voice].len)
      mixer_voice[voice].pos = 0;

   mixer_voice[voice].playing = TRUE;
}

END_OF_FUNCTION(_mixer_start_voice);



/* _mixer_stop_voice:
 *  Stops a voice from playing.
 */
void _mixer_stop_voice(int voice)
{
   mixer_voice[voice].playing = FALSE;
}

END_OF_FUNCTION(_mixer_stop_voice);



/* _mixer_loop_voice:
 *  Sets the loopmode for a voice.
 */
void _mixer_loop_voice(int voice, int loopmode)
{
   update_mixer_freq(mixer_voice+voice, _phys_voice+voice);
}

END_OF_FUNCTION(_mixer_loop_voice);



/* _mixer_get_position:
 *  Returns the current play position of a voice, or -1 if it has finished.
 */
int _mixer_get_position(int voice)
{
   if ((!mixer_voice[voice].playing) ||
       (mixer_voice[voice].pos >= mixer_voice[voice].len))
      return -1;

   return (mixer_voice[voice].pos >> MIX_FIX_SHIFT);
}

END_OF_FUNCTION(_mixer_get_position);



/* _mixer_set_position:
 *  Sets the current play position of a voice.
 */
void _mixer_set_position(int voice, int position)
{
   mixer_voice[voice].pos = (position << MIX_FIX_SHIFT);

   if (mixer_voice[voice].pos >= mixer_voice[voice].len)
      mixer_voice[voice].playing = FALSE;
}

END_OF_FUNCTION(_mixer_set_position);



/* _mixer_get_volume:
 *  Returns the current volume of a voice.
 */
int _mixer_get_volume(int voice)
{
   return (_phys_voice[voice].vol >> 12);
}

END_OF_FUNCTION(_mixer_get_volume);



/* _mixer_set_volume:
 *  Sets the volume of a voice.
 */
void _mixer_set_volume(int voice, int volume)
{
   update_mixer_volume(mixer_voice+voice, _phys_voice+voice);
}

END_OF_FUNCTION(_mixer_set_volume);



/* _mixer_ramp_volume:
 *  Starts a volume ramping operation.
 */
void _mixer_ramp_volume(int voice, int time, int endvol)
{
   int d = (endvol << 12) - _phys_voice[voice].vol;
   time = MAX(time * (mix_freq / UPDATE_FREQ) / 1000, 1);

   _phys_voice[voice].target_vol = endvol << 12;
   _phys_voice[voice].dvol = d / time;
}

END_OF_FUNCTION(_mixer_ramp_volume);



/* _mixer_stop_volume_ramp:
 *  Ends a volume ramp operation.
 */
void _mixer_stop_volume_ramp(int voice)
{
   _phys_voice[voice].dvol = 0;
}

END_OF_FUNCTION(_mixer_stop_volume_ramp);



/* _mixer_get_frequency:
 *  Returns the current frequency of a voice.
 */
int _mixer_get_frequency(int voice)
{
   return (_phys_voice[voice].freq >> 12);
}

END_OF_FUNCTION(_mixer_get_frequency);



/* _mixer_set_frequency:
 *  Sets the frequency of a voice.
 */
void _mixer_set_frequency(int voice, int frequency)
{
   update_mixer_freq(mixer_voice+voice, _phys_voice+voice);
}

END_OF_FUNCTION(_mixer_set_frequency);



/* _mixer_sweep_frequency:
 *  Starts a frequency sweep.
 */
void _mixer_sweep_frequency(int voice, int time, int endfreq)
{
   int d = (endfreq << 12) - _phys_voice[voice].freq;
   time = MAX(time * (mix_freq / UPDATE_FREQ) / 1000, 1);

   _phys_voice[voice].target_freq = endfreq << 12;
   _phys_voice[voice].dfreq = d / time;
}

END_OF_FUNCTION(_mixer_sweep_frequency);



/* _mixer_stop_frequency_sweep:
 *  Ends a frequency sweep.
 */
void _mixer_stop_frequency_sweep(int voice)
{
   _phys_voice[voice].dfreq = 0;
}

END_OF_FUNCTION(_mixer_stop_frequency_sweep);



/* _mixer_get_pan:
 *  Returns the current pan position of a voice.
 */
int _mixer_get_pan(int voice)
{
   return (_phys_voice[voice].pan >> 12);
}

END_OF_FUNCTION(_mixer_get_pan);



/* _mixer_set_pan:
 *  Sets the pan position of a voice.
 */
void _mixer_set_pan(int voice, int pan)
{
   update_mixer_volume(mixer_voice+voice, _phys_voice+voice);
}

END_OF_FUNCTION(_mixer_set_pan);



/* _mixer_sweep_pan:
 *  Starts a pan sweep.
 */
void _mixer_sweep_pan(int voice, int time, int endpan)
{
   int d = (endpan << 12) - _phys_voice[voice].pan;
   time = MAX(time * (mix_freq / UPDATE_FREQ) / 1000, 1);

   _phys_voice[voice].target_pan = endpan << 12;
   _phys_voice[voice].dpan = d / time;
}

END_OF_FUNCTION(_mixer_sweep_pan);



/* _mixer_stop_pan_sweep:
 *  Ends a pan sweep.
 */
void _mixer_stop_pan_sweep(int voice)
{
   _phys_voice[voice].dpan = 0;
}

END_OF_FUNCTION(_mixer_stop_pan_sweep);



/* _mixer_set_echo:
 *  Sets the echo parameters for a voice.
 */
void _mixer_set_echo(int voice, int strength, int delay)
{
   /* not implemented */
}

END_OF_FUNCTION(_mixer_set_echo);



/* _mixer_set_tremolo:
 *  Sets the tremolo parameters for a voice.
 */
void _mixer_set_tremolo(int voice, int rate, int depth)
{
   /* not implemented */
}

END_OF_FUNCTION(_mixer_set_tremolo);



/* _mixer_set_vibrato:
 *  Sets the amount of vibrato for a voice.
 */
void _mixer_set_vibrato(int voice, int rate, int depth)
{
   /* not implemented */
}

END_OF_FUNCTION(_mixer_set_vibrato);



/* mixer_lock_mem:
 *  Locks memory used by the functions in this file.
 */
static void mixer_lock_mem(void)
{
   LOCK_VARIABLE(mixer_voice);
   LOCK_VARIABLE(mix_buffer);
   LOCK_VARIABLE(mix_vol_table);
   LOCK_VARIABLE(mix_clip_table);
   LOCK_VARIABLE(mix_voices);
   LOCK_VARIABLE(mix_size);
   LOCK_VARIABLE(mix_freq);
   LOCK_VARIABLE(mix_stereo);
   LOCK_VARIABLE(mix_16bit);
   LOCK_FUNCTION(mix_silent_samples);
   LOCK_FUNCTION(mix_mono_8x1_samples);
   LOCK_FUNCTION(mix_mono_8x2_samples);
   LOCK_FUNCTION(mix_mono_16x1_samples);
   LOCK_FUNCTION(mix_mono_16x2_samples);
   LOCK_FUNCTION(mix_stereo_8x1_samples);
   LOCK_FUNCTION(mix_stereo_8x2_samples);
   LOCK_FUNCTION(mix_stereo_16x1_samples);
   LOCK_FUNCTION(mix_stereo_16x2_samples);
   LOCK_FUNCTION(mix_hq1_8x1_samples);
   LOCK_FUNCTION(mix_hq1_8x2_samples);
   LOCK_FUNCTION(mix_hq1_16x1_samples);
   LOCK_FUNCTION(mix_hq1_16x2_samples);
   LOCK_FUNCTION(mix_hq2_8x1_samples);
   LOCK_FUNCTION(mix_hq2_8x2_samples);
   LOCK_FUNCTION(mix_hq2_16x1_samples);
   LOCK_FUNCTION(mix_hq2_16x2_samples);
   LOCK_FUNCTION(update_mixer_volume);
   LOCK_FUNCTION(update_mixer);
   LOCK_FUNCTION(update_silent_mixer);
   LOCK_FUNCTION(_mix_some_samples);
   LOCK_FUNCTION(_mixer_init_voice);
   LOCK_FUNCTION(_mixer_release_voice);
   LOCK_FUNCTION(_mixer_start_voice);
   LOCK_FUNCTION(_mixer_stop_voice);
   LOCK_FUNCTION(_mixer_loop_voice);
   LOCK_FUNCTION(_mixer_get_position);
   LOCK_FUNCTION(_mixer_set_position);
   LOCK_FUNCTION(_mixer_get_volume);
   LOCK_FUNCTION(_mixer_set_volume);
   LOCK_FUNCTION(_mixer_ramp_volume);
   LOCK_FUNCTION(_mixer_stop_volume_ramp);
   LOCK_FUNCTION(_mixer_get_frequency);
   LOCK_FUNCTION(_mixer_set_frequency);
   LOCK_FUNCTION(_mixer_sweep_frequency);
   LOCK_FUNCTION(_mixer_stop_frequency_sweep);
   LOCK_FUNCTION(_mixer_get_pan);
   LOCK_FUNCTION(_mixer_set_pan);
   LOCK_FUNCTION(_mixer_sweep_pan);
   LOCK_FUNCTION(_mixer_stop_pan_sweep);
   LOCK_FUNCTION(_mixer_set_echo);
   LOCK_FUNCTION(_mixer_set_tremolo);
   LOCK_FUNCTION(_mixer_set_vibrato);
}



