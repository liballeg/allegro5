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
 *      ALSA RawMIDI Sound driver.
 *
 *      By Thomas Fjellstrom.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"

#if (defined MIDI_ALSA) && ((!defined ALLEGRO_WITH_MODULES) || (defined ALLEGRO_MODULE))

#include "allegro/internal/aintern.h"

#ifdef ALLEGRO_QNX
#include "allegro/platform/aintqnx.h"
#else
#include "allegro/platform/aintunix.h"
#endif

#ifndef SCAN_DEPEND
   #include <stdlib.h>
   #include <stdio.h>
   #include <string.h>
   #include <errno.h>
   #include <sys/asoundlib.h>
#endif


static int alsa_rawmidi_detect(int input);
static int alsa_rawmidi_init(int input, int voices);
static void alsa_rawmidi_exit(int input);
static void alsa_rawmidi_output(int data);

static char alsa_rawmidi_desc[256];

static snd_rawmidi_t *rawmidi_handle = NULL;


MIDI_DRIVER midi_alsa =
{
   MIDI_ALSA,                /* id */
   empty_string,             /* name */
   empty_string,             /* desc */
   "ALSA RawMIDI",           /* ASCII name */
   0,                        /* voices */
   0,                        /* basevoice */
   0xFFFF,                   /* max_voices */
   0,                        /* def_voices */
   -1,                       /* xmin */
   -1,                       /* xmax */
   alsa_rawmidi_detect,      /* detect */
   alsa_rawmidi_init,        /* init */
   alsa_rawmidi_exit,        /* exit */
   NULL,                     /* mixer_volume */
   alsa_rawmidi_output,      /* raw_midi */
   _dummy_load_patches,      /* load_patches */
   _dummy_adjust_patches,    /* adjust_patches */
   _dummy_key_on,            /* key_on */
   _dummy_noop1,             /* key_off */
   _dummy_noop2,             /* set_volume */
   _dummy_noop3,             /* set_pitch */
   _dummy_noop2,             /* set_pan */
   _dummy_noop2              /* set_vibrato */
};



/* alsa_rawmidi_detect:
 *  ALSA RawMIDI detection.
 */
static int alsa_rawmidi_detect(int input)
{
   int card = -1;
   int device = -1;
   int ret = FALSE, err;
   char tmp1[128], tmp2[128], temp[256];
   snd_rawmidi_t *handle = NULL;

   if (input) {
      ret = FALSE;
   }
   else {
      card = get_config_int(uconvert_ascii("sound", tmp1),
			    uconvert_ascii("alsa_rawmidi_card", tmp2),
			    snd_defaults_rawmidi_card());

      device = get_config_int(uconvert_ascii("sound", tmp1),
			      uconvert_ascii("alsa_rawmidi_device", tmp2),
			      snd_defaults_rawmidi_device());

      if ((err = snd_rawmidi_open(&handle, card, device, SND_RAWMIDI_OPEN_OUTPUT_APPEND)) < 0) {
	 snprintf(temp, sizeof(temp), "Could not open card/rawmidi device: %s", snd_strerror(err));
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text(temp));
	 ret = FALSE;
      }
      
      snd_rawmidi_close(handle);
      ret = TRUE;
   }

   return ret;
}



/* alsa_rawmidi_init:
 *  Inits the ALSA RawMIDI interface.
 */
static int alsa_rawmidi_init(int input, int voices)
{
   int card = -1;
   int device = -1;
   int ret = -1, err;
   char tmp1[128], tmp2[128], temp[256];
   snd_rawmidi_info_t info;

   if (input) {
      ret = -1;
   }
   else {
      card = get_config_int(uconvert_ascii("sound", tmp1),
			    uconvert_ascii("alsa_rawmidi_card", tmp2),
			    snd_defaults_rawmidi_card());

      device = get_config_int(uconvert_ascii("sound", tmp1),
			     uconvert_ascii("alsa_rawmidi_device", tmp2),
			     snd_defaults_rawmidi_device());

      if ((err = snd_rawmidi_open(&rawmidi_handle, card, device, SND_RAWMIDI_OPEN_OUTPUT_APPEND)) < 0) {
	 snprintf(temp, sizeof(temp), "Could not open card/rawmidi device: %s", snd_strerror(err));
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text(temp));
	 ret = -1;
      }

      ret = 0;
   }

   if (rawmidi_handle) {
      snd_rawmidi_block_mode(rawmidi_handle, 1);
      snd_rawmidi_info(rawmidi_handle, &info);
      strcpy(alsa_rawmidi_desc, info.name);
      midi_alsa.desc = alsa_rawmidi_desc;
   }

   return ret;
}



/* alsa_rawmidi_exit:
 *   Cleans up.
 */
static void alsa_rawmidi_exit(int input)
{
   if (rawmidi_handle) {
      snd_rawmidi_output_drain(rawmidi_handle);
      snd_rawmidi_close(rawmidi_handle);
   }

   rawmidi_handle = NULL;
}



/* alsa_rawmidi_output:
 *   Outputs MIDI data.
 */
static void alsa_rawmidi_output(int data)
{
   snd_rawmidi_write(rawmidi_handle, &data, sizeof(char));
}



#ifdef ALLEGRO_MODULE

/* _module_init:
 *   Called when loaded as a dynamically linked module.
 */
void _module_init(int system_driver)
{
   _unix_register_midi_driver(MIDI_ALSA, &midi_alsa, TRUE, TRUE);
}

#endif /* ALLEGRO_MODULE */

#endif /* MIDI_ALSA */

