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
 *      ALSA sound driver.
 *
 *      By Peter Wang (based heavily on uoss.c)
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"

#ifdef DIGI_ALSA

#include "allegro/aintern.h"
#include "allegro/aintunix.h"

#include <sys/asoundlib.h>


#ifndef SND_PCM_SFMT_S16_NE
   #ifdef ALLEGRO_BIG_ENDIAN
      #define SND_PCM_SFMT_S16_NE SND_PCM_SFMT_S16_BE
   #else
      #define SND_PCM_SFMT_S16_NE SND_PCM_SFMT_S16_LE
   #endif
#endif
#ifndef SND_PCM_SFMT_U16_NE
   #ifdef ALLEGRO_BIG_ENDIAN
      #define SND_PCM_SFMT_U16_NE SND_PCM_SFMT_U16_BE
   #else
      #define SND_PCM_SFMT_U16_NE SND_PCM_SFMT_U16_LE
   #endif
#endif


#define ALSA_DEFAULT_NUMFRAGS   4


static snd_pcm_t *pcm_handle;
static int alsa_bufsize;
static unsigned char *alsa_bufdata;
static int alsa_bits, alsa_signed, alsa_rate, alsa_stereo;
static int alsa_fragments;

static char alsa_desc[320] = EMPTY_STRING;



static int alsa_detect(int input);
static int alsa_init(int input, int voices);
static void alsa_exit(int input);
static int alsa_mixer_volume(int volume);
static int alsa_buffer_size(void);



DIGI_DRIVER digi_alsa =
{
   DIGI_ALSA,
   empty_string,
   empty_string,
   "ALSA",
   0,
   0,
   MIXER_MAX_SFX,
   MIXER_DEF_SFX,

   alsa_detect,
   alsa_init,
   alsa_exit,
   alsa_mixer_volume,

   NULL,
   NULL,
   alsa_buffer_size,
   _mixer_init_voice,
   _mixer_release_voice,
   _mixer_start_voice,
   _mixer_stop_voice,
   _mixer_loop_voice,

   _mixer_get_position,
   _mixer_set_position,

   _mixer_get_volume,
   _mixer_set_volume,
   _mixer_ramp_volume,
   _mixer_stop_volume_ramp,

   _mixer_get_frequency,
   _mixer_set_frequency,
   _mixer_sweep_frequency,
   _mixer_stop_frequency_sweep,

   _mixer_get_pan,
   _mixer_set_pan,
   _mixer_sweep_pan,
   _mixer_stop_pan_sweep,

   _mixer_set_echo,
   _mixer_set_tremolo,
   _mixer_set_vibrato,
   0, 0,
   0,
   0,
   0,
   0,
   0,
   0
};



/* alsa_buffer_size:
 *  Returns the current DMA buffer size, for use by the audiostream code.
 */
static int alsa_buffer_size()
{
   return alsa_bufsize * alsa_fragments / (alsa_bits / 8) / (alsa_stereo ? 2 : 1);
}



/* alsa_update:
 *  Update data.
 */
static void alsa_update(unsigned long interval)
{
   int i;

   DISABLE();

   for (i = 0;  i < alsa_fragments; i++) {
      if (snd_pcm_write(pcm_handle, alsa_bufdata, alsa_bufsize) <= 0)
	 break;
      _mix_some_samples((unsigned long) alsa_bufdata, 0, alsa_signed);
   }

   ENABLE();
}



/* alsa_detect:
 *  Detect driver presence.
 */
static int alsa_detect(int input)
{
   snd_pcm_t *handle;
   int card, device;
   char tmp1[80], tmp2[80];

   card = get_config_int(uconvert_ascii("sound", tmp1),
			 uconvert_ascii("alsa_card", tmp2),
			 snd_defaults_card());

   device = get_config_int(uconvert_ascii("sound", tmp1),
			   uconvert_ascii("alsa_pcmdevice", tmp2),
			   snd_defaults_pcm_device());

   if (snd_pcm_open(&handle, card, device, SND_PCM_OPEN_PLAYBACK) == 0) {
      snd_pcm_close(handle);
      return TRUE;
   }

   return FALSE;
}



/* alsa_init:
 *  ALSA init routine.
 */
static int alsa_init(int input, int voices)
{
   int card, device;
   int format, fragsize, numfrags;
   snd_pcm_format_t fmt;
   snd_pcm_playback_info_t info;
   snd_pcm_playback_params_t params;
   snd_pcm_playback_status_t status;
   char tmp1[80], tmp2[80];

   if (input) {
      usprintf(allegro_error, get_config_text("Input is not supported"));
      return -1;
   }

   card = get_config_int(uconvert_ascii("sound", tmp1),
			 uconvert_ascii("alsadigi_card", tmp2),
			 snd_defaults_card());

   device = get_config_int(uconvert_ascii("sound", tmp1),
			   uconvert_ascii("alsadigi_pcmdevice", tmp2),
			   snd_defaults_pcm_device());

   if (snd_pcm_open(&pcm_handle, card, device, SND_PCM_OPEN_PLAYBACK) != 0) {
      usprintf(allegro_error, get_config_text("Cannot open card/pcm device"));
      return -1;
   }

   numfrags = get_config_int(uconvert_ascii("sound", tmp1),
			     uconvert_ascii("alsa_numfrags", tmp2),
			     ALSA_DEFAULT_NUMFRAGS);

   alsa_bits = (_sound_bits == 8) ? 8 : 16;
   alsa_stereo = (_sound_stereo) ? 1 : 0;
   alsa_rate = (_sound_freq > 0) ? _sound_freq : 45454;

   format = ((alsa_bits == 16) ? SND_PCM_SFMT_S16_NE : SND_PCM_SFMT_U8);

   memset(&fmt, 0, sizeof(fmt));
   fmt.format = format;
   fmt.rate = alsa_rate;
   fmt.channels = alsa_stereo + 1;
   if (snd_pcm_playback_format(pcm_handle, &fmt) != 0) {
      usprintf(allegro_error, get_config_text("Cannot set playback format"));
      snd_pcm_close(pcm_handle);
      return -1;
   }

   alsa_signed = 0;
   switch (format) {
      case SND_PCM_SFMT_S8:
	 alsa_signed = 1;
      case SND_PCM_SFMT_U8:
	 alsa_bits = 8;
	 break;
      case SND_PCM_SFMT_S16_NE:
	 alsa_signed = 1;
      case SND_PCM_SFMT_U16_NE:
	 alsa_bits = 16;
	 if (sizeof(short) != 2) {
	    usprintf(allegro_error, get_config_text("Unsupported sample format"));
	    snd_pcm_close(pcm_handle);
	    return -1;
	 }
	 break;
      default:
	 usprintf(allegro_error, get_config_text("Unsupported sample format"));
	 snd_pcm_close(pcm_handle);
	 return -1;
   }

   snd_pcm_playback_info(pcm_handle, &info);
   fragsize = MID(info.min_fragment_size, info.buffer_size / numfrags, info.max_fragment_size);
   numfrags = info.buffer_size / fragsize;

   memset(&params, 0, sizeof(params));
   params.fragment_size = fragsize;
   params.fragments_max = numfrags;
   params.fragments_room = 1;
   snd_pcm_playback_params(pcm_handle, &params);

   snd_pcm_playback_status(pcm_handle, &status);
   alsa_fragments = status.fragments;
   alsa_bufsize = status.fragment_size;

   alsa_bufdata = malloc(alsa_bufsize);
   if (!alsa_bufdata) {
      usprintf(allegro_error, get_config_text("Can not allocate audio buffer"));
      snd_pcm_close(pcm_handle);
      return -1;
   }

   snd_pcm_block_mode(pcm_handle, 0);

   digi_alsa.voices = voices;

   if (_mixer_init(alsa_bufsize / (alsa_bits / 8), alsa_rate,
		   alsa_stereo, ((alsa_bits == 16) ? 1 : 0),
		   &digi_alsa.voices) != 0) {
      usprintf(allegro_error, get_config_text("Can not init software mixer"));
      snd_pcm_close(pcm_handle);
      return -1;
   }

   _mix_some_samples((unsigned long) alsa_bufdata, 0, alsa_signed);

   /* Add audio interrupt. */
   DISABLE();
   _sigalrm_digi_interrupt_handler = alsa_update;
   ENABLE();

   usprintf(alsa_desc, get_config_text("Card #%d, device #%d: %d bits, %s, %d bps, %s"),
				       card, device, alsa_bits,
				       uconvert_ascii((alsa_signed ? "signed" : "unsigned"), tmp1), alsa_rate,
				       uconvert_ascii((alsa_stereo ? "stereo" : "mono"), tmp2));

   digi_driver->desc = alsa_desc;

   return 0;
}



/* alsa_exit:
 *  Shutdown ALSA driver.
 */
static void alsa_exit(int input)
{
   if (input) {
      return;
   }

   DISABLE();
   _sigalrm_digi_interrupt_handler = NULL;
   ENABLE();

   free(alsa_bufdata);
   alsa_bufdata = NULL;

   _mixer_exit();

   snd_pcm_close(pcm_handle);
}



/* alsa_mixer_volume:
 *  Set mixer volume (0-255)
 */
static int alsa_mixer_volume(int volume)
{
   /* TODO */ 
#if 0
   snd_mixer_t *handle;
   int card, device;

   if (snd_mixer_open(&handle, card, device) == 0) {
      /* do something special */
      snd_mixer_close(handle);
      return 0;
   }

   return -1;
#else
   return 0;
#endif
}

#endif

