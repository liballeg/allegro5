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
 *      General setup utility for the Allegro library.
 *
 *      By Shawn Hargreaves.
 *
 *      Unicode support added by Eric Botcazou.
 *
 *      See readme.txt for copyright information.
 */

#define ALLEGRO_USE_CONSOLE

#include <stdio.h>
#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"

#ifdef ALLEGRO_LINUX
   #include <sys/time.h>
#endif

#ifdef ALLEGRO_INTERNAL_HEADER
   #include ALLEGRO_INTERNAL_HEADER
#endif

#include "setup.h"



/* these can be customised to suit your program... */
#ifndef SETUP_TITLE
   #define SETUP_TITLE           "Allegro Setup " ALLEGRO_VERSION_STR ", " ALLEGRO_PLATFORM_STR
#endif

#ifndef SETUP_CFG_FILE
   #define SETUP_CFG_FILE        "allegro.cfg"
#endif

#ifndef SETUP_KEYBOARD_FILE
   #define SETUP_KEYBOARD_FILE   "keyboard.dat"
#endif

#ifndef SETUP_LANGUAGE_FILE
   #define SETUP_LANGUAGE_FILE   "language.dat"
#endif

#ifndef SETUP_DATA_FILE
   #define SETUP_DATA_FILE       "setup.dat"
#endif

#ifndef SETUP_SCREEN_W
   #define SETUP_SCREEN_W        320
   #define SETUP_SCREEN_H        200
#endif

/* define if you want to use compiled datafiles */
/* #undef SETUP_USE_COMPILED_DATAFILES */



/* to save executable size, since we only need VGA 13h, 256 color mode */
#if (defined ALLEGRO_DOS) && (!defined SETUP_EMBEDDED)

   #if (SETUP_SCREEN_W == 320) && (SETUP_SCREEN_H == 200)
      BEGIN_GFX_DRIVER_LIST
	 GFX_DRIVER_VGA
      END_GFX_DRIVER_LIST
   #endif

   #ifdef ALLEGRO_COLOR8
      BEGIN_COLOR_DEPTH_LIST
	 COLOR_DEPTH_8
      END_COLOR_DEPTH_LIST
   #endif

#endif



/* helper for converting a static string */
static char *uconvert_static_string(char *str)
{
   int size;
   char *new_str;

   if (need_uconvert(str, U_ASCII, U_CURRENT)) {
      size = uconvert_size(str, U_ASCII, U_CURRENT);
      new_str = malloc(size);
      do_uconvert(str, U_ASCII, new_str, U_CURRENT, size);
      return new_str;
   }
   else
      return str;
}



/* helper for converting a static const string */
static AL_CONST char *uconvert_static_const_string(AL_CONST char *str)
{
   int size;
   char *new_str;

   if (need_uconvert(str, U_ASCII, U_CURRENT)) {
      size = uconvert_size(str, U_ASCII, U_CURRENT);
      new_str = malloc(size);
      do_uconvert(str, U_ASCII, new_str, U_CURRENT, size);
      return new_str;
   }
   else
      return str;
}



/* helper for converting a static string array */
static char **uconvert_static_string_array(char *str_array[])
{
   int i = 0;

   while (str_array[i]) {
      str_array[i] = uconvert_static_string(str_array[i]);
      i++;
   }

   return str_array;
}



/* info about a hardware driver */
typedef struct SOUNDCARD
{
   int id;
   AL_CONST char *name;
   char **param;
   AL_CONST char *desc;
   int present;
} SOUNDCARD;


static SOUNDCARD *soundcard;
static SOUNDCARD digi_cards[64], midi_cards[64];



/* different types of parameter */
typedef enum PARAM_TYPE
{
   param_none,
   param_int,
   param_hex,
   param_id,
   param_bool,
   param_file,
   param_list,
   param_num,
   param_str
} PARAM_TYPE;



/* info about a soundcard parameter */
typedef struct PARAMETER
{
   char *name;
   PARAM_TYPE type;
   char value[512];
   char *def;
   int *detect;
   char *label;
   char *e1, *e2;
   char *desc;
} PARAMETER;



/* list of soundcard parameters */
static PARAMETER parameters[] =
{
   /* name                 type           value    default     detect            label       extra1      extra2   desc */
   { "digi_card",          param_id,      "",      "-1",       NULL,             "",         NULL,       NULL,    "" },
   { "midi_card",          param_id,      "",      "-1",       NULL,             "",         NULL,       NULL,    "" },
   { "quality",            param_int,     "",      "0",        NULL,             "Qual:",    NULL,       NULL,    "Sample mixing quality (0 = fast mixing of 8 bit data into 16 bit buffers, 1 = true 16 bit mixing, 2 = interpolated 16 bit mixing)" },
   { "digi_volume",        param_int,     "",      "-1",       NULL,             "Vol:",     NULL,       NULL,    "Digital sound volume (0 to 255)" },
   { "midi_volume",        param_int,     "",      "-1",       NULL,             "Vol:",     NULL,       NULL,    "MIDI music volume (0 to 255)" },
   { "digi_voices",        param_int,     "",      "-1",       NULL,             "Chan:",    NULL,       NULL,    "Number of channels reserved for playing digital sounds (higher values increase polyphony but degrade speed and quality)" },
   { "midi_voices",        param_int,     "",      "-1",       NULL,             "Chan:",    NULL,       NULL,    "Number of channels reserved for playing MIDI music (higher values increase polyphony but degrade speed and quality)" },
   { "flip_pan",           param_bool,    "",      "0",        NULL,             "Pan:",     NULL,       NULL,    "Reverses the left/right stereo placement" },

 #ifndef ALLEGRO_WINDOWS
   { "sound_port",         param_hex,     "",      "-1",       &_sound_port,     "Port:",    NULL,       NULL,    "Port address (usually 220)" },
   { "sound_dma",          param_int,     "",      "-1",       &_sound_dma,      "DMA:",     NULL,       NULL,    "DMA channel (usually 1)" },
   { "sound_irq",          param_int,     "",      "-1",       &_sound_irq,      "IRQ:",     NULL,       NULL,    "IRQ number (usually 7)" },
   { "sound_freq",         param_list,    "",      "-1",       &_sound_freq,     "Freq:",    NULL,       NULL,    "Sample mixing frequency (higher values sound better but require more CPU processing time)" },
   { "sound_bits",         param_int,     "",      "-1",       &_sound_bits,     "Bits:",    NULL,       NULL,    "Bits per sample (8 or 16: higher values sound better but require more memory)" },
   { "sound_stereo",       param_bool,    "",      "0",        &_sound_stereo,   "Stereo:",  NULL,       NULL,    "Use device in stereo mode" },
 #endif

 #ifdef ALLEGRO_DOS
   { "fm_port",            param_hex,     "",      "-1",       &_fm_port,        "Port:",    NULL,       NULL,    "Port address (usually 388)" },
   { "mpu_port",           param_hex,     "",      "-1",       &_mpu_port,       "Port:",    NULL,       NULL,    "Port address (usually 330)" },
   { "mpu_irq",            param_int,     "",      "-1",       &_mpu_irq,        "IRQ:",     NULL,       NULL,    "IRQ number (usually 7)" },
   { "ibk_file",           param_file,    "",      "",         NULL,             "IBK:",     "IBK",      NULL,    "Custom .IBK instrument patch set" },
   { "ibk_drum_file",      param_file,    "",      "",         NULL,             "drumIBK:", "IBK",      NULL,    "Custom .IBK percussion patch set" },
 #endif

 #ifdef DIGI_OSS
   { "oss_driver",         param_str,     "",      "/dev/dsp", NULL,             "Drv:",     "",         NULL,    "Name of the driver (e.g. /dev/dsp)" },
   { "oss_numfrags",       param_int,     "",      "-1",       NULL,             "NumFr:",   NULL,       NULL,    "Number of fragments (use 2, other values make little difference)" },
   { "oss_fragsize",       param_num,     "",      "-1",       NULL,             "Siz:",     NULL,       NULL,    "Size of fragment (small values -- choppy sound, large values -- delayed sound)" },
 #endif

 #ifdef MIDI_OSS
   { "oss_midi_driver",    param_str,     "",      "/dev/sequencer",    NULL,    "Drv:",     "",         NULL,    "Name of the driver (e.g. /dev/sequencer)" },
 #endif

 #ifdef DIGI_ESD
   { "esd_server",         param_str,     "",      "",         NULL,             "Srv:",     "",         NULL,    "Name of the server" },
 #endif

 #ifdef DIGI_ALSA
   { "alsa_card",          param_int,     "",      "",         NULL,             "Card:",    NULL,       NULL,    "ALSA card number" },
   { "alsa_pcmdevice",     param_int,     "",      "",         NULL,             "Dev:",     NULL,       NULL,    "ALSA PCM device number" },
   { "alsa_numfrags",      param_int,     "",      "",         NULL,             "NumFr:",   NULL,       NULL,    "Number of fragments (use 2, other values make little difference)" },
 #endif

 #ifdef MIDI_ALSA
   { "alsa_rawmidi_card",  param_int,     "",      "",         NULL,             "Card:",    NULL,       NULL,    "ALSA RawMIDI card number" },
   { "alsa_rawmidi_device",param_int,     "",      "",         NULL,             "Dev:",     NULL,       NULL,    "ALSA RawMIDI device number" },
 #endif
 
 #ifdef MIDI_BEOS
   { "be_midi_quality",    param_int,     "",      "1",        NULL,             "Qual:",    NULL,       NULL,    "BeOS MIDI synthetizer instruments quality (0 = low, 1 = high)" },
   { "be_midi_freq",       param_list,    "",      "-1",       NULL,             "Freq:",    NULL,       NULL,    "BeOS MIDI sample mixing frequency in Hz" },
   { "be_midi_interpolation",param_int,   "",      "0",        NULL,             "Intrp:",   NULL,       NULL,    "BeOS MIDI sample interpolation method (0 = none, 1 = fast linear, 2 = linear)" },
   { "be_midi_reverb",     param_int,     "",      "0",        NULL,             "Revrb:",   NULL,       NULL,    "BeOS MIDI reverberation intensity (0 to 5)" },
 #endif
 
 #ifdef MIDI_CORE_AUDIO
   { "ca_midi_quality",    param_int,     "",      "127",      NULL,             "Qual:",    NULL,       NULL,    "CoreAudio MIDI synthetizer render quality (0 = low, 127 = high)" },
   { "ca_midi_reverb",     param_int,     "",      "0",        NULL,             "Revrb:",   NULL,       NULL,    "CoreAudio MIDI reverberation intensity (0 to 5)" },
 #endif

 #ifdef MIDI_DIGMID
   { "patches",            param_file,    "",      "",         NULL,             "Patches:", "CFG;DAT",  NULL,    "MIDI patch set (GUS format default.cfg or Allegro format patches.dat)" },
 #endif

   { NULL,                 param_none,    "",      "",         NULL,             NULL,       NULL,       NULL,    NULL }
};



/* helper for converting a parameter array */
static void uconvert_static_parameter(PARAMETER *p)
{
   while (p->name) {
      p->name = uconvert_static_string(p->name);
      usetc(p->value, 0);
      p->def = uconvert_static_string(p->def);
      p->label = uconvert_static_string(p->label);
      if (p->e1)
         p->e1 = uconvert_static_string(p->e1);
      if (p->e2)
         p->e2 = uconvert_static_string(p->e2);
      p->desc = uconvert_static_string(p->desc);

      p++;
   }
}



/* This is the important bit, where we decide what drivers and parameters to
 * list. We ask the system driver for a list of the available drivers, and
 * then fill in extra information for any that we know about (eg. which
 * port numbers and suchlike things they want to use). If we don't know
 * about a driver, we can still select it, but obviously won't be able to
 * give the user any of the more specialised options for it.
 */
static void find_sound_drivers(void)
{
   static char *digi_param[] = {"11", "digi_volume", NULL};
   static char *midi_param[] = {"11", "midi_volume", NULL};
   _DRIVER_INFO *info;
   int c;

   /* have to do this at runtime to work with DLL linkage */
   parameters[0].detect = &digi_card;
   parameters[1].detect = &midi_card;
   parameters[2].detect = &_sound_hq;

   /* fill in list of digital drivers */
   digi_cards[0].id = DIGI_AUTODETECT;
   digi_cards[0].name = uconvert_static_string("Autodetect");
   digi_cards[0].param = uconvert_static_string_array(digi_param);
   digi_cards[0].desc = uconvert_static_string("Attempt to autodetect the digital sound hardware");
   digi_cards[0].present = FALSE;

   if (system_driver->digi_drivers)
      info = system_driver->digi_drivers();
   else
      info = _digi_driver_list;

   c = 1;

   while ((info->driver) && (info->id != DIGI_NONE)) {
      digi_cards[c].id = info->id;
      digi_cards[c].name = digi_cards[c].desc = uconvert_static_const_string(((DIGI_DRIVER *)info->driver)->ascii_name);
      digi_cards[c].param = NULL;
      digi_cards[c].present = FALSE;

      switch (info->id) {

       #ifdef DIGI_SB10
         case DIGI_SB10:
	 {
	    static char *param[] = {"sound_port", "sound_dma", "sound_irq", "digi_volume", "digi_voices", "sound_freq", NULL};
	    digi_cards[c].param = uconvert_static_string_array(param);
	    digi_cards[c].desc = uconvert_static_string("SB v1.0, 8 bit mono using single-shot dma");
	    break;
	 }
       #endif

       #ifdef DIGI_SB15
	 case DIGI_SB15:
	 {
	    static char *param[] = {"sound_port", "sound_dma", "sound_irq", "digi_volume", "digi_voices", "sound_freq", NULL};
	    digi_cards[c].param = uconvert_static_string_array(param);
	    digi_cards[c].desc = uconvert_static_string("SB v1.5, 8 bit mono using single shot dma");
	    break;
	 }
       #endif

       #ifdef DIGI_SB20
	 case DIGI_SB20:
	 {
	    static char *param[] = {"sound_port", "sound_dma", "sound_irq", "digi_volume", "digi_voices", "sound_freq", NULL};
	    digi_cards[c].param = uconvert_static_string_array(param);
	    digi_cards[c].desc = uconvert_static_string("SB v2.0, 8 bit mono using auto-initialised dma");
	    break;
	 }
       #endif

       #ifdef DIGI_SBPRO
	 case DIGI_SBPRO:
	 {
	    static char *param[] = {"sound_port", "sound_dma", "sound_irq", "flip_pan", "quality", "sound_freq", "digi_volume", "digi_voices", NULL};
	    digi_cards[c].param = uconvert_static_string_array(param);
	    digi_cards[c].desc = uconvert_static_string("SB Pro, 8 bit stereo DAC");
	    break;
	 }
       #endif

       #ifdef DIGI_SB16
	 case DIGI_SB16:
	 {
	    static char *param[] = {"sound_port", "sound_dma", "sound_irq", "flip_pan", "quality", "sound_freq", "digi_volume", "digi_voices", NULL};
	    digi_cards[c].param = uconvert_static_string_array(param);
	    digi_cards[c].desc = uconvert_static_string("SB16 or AWE32, 16 bit stereo DAC");
	    break;
	 }
       #endif

       #ifdef DIGI_AUDIODRIVE
	 case DIGI_AUDIODRIVE:
	 {
	    static char *param[] = {"sound_port", "sound_dma", "sound_irq", "flip_pan", "quality", "sound_freq", "digi_volume", "digi_voices", NULL};
	    digi_cards[c].param = uconvert_static_string_array(param);
	    digi_cards[c].desc = uconvert_static_string("ESS AudioDrive (16 bit stereo DAC)");
	    break;
	 }
       #endif

       #ifdef DIGI_SOUNDSCAPE
	 case DIGI_SOUNDSCAPE:
	 {
	    static char *param[] = {"02", "flip_pan", "quality", "sound_freq", "digi_volume", "digi_voices", NULL};
	    digi_cards[c].param = uconvert_static_string_array(param);
	    digi_cards[c].desc = uconvert_static_string("Ensoniq Soundscape (16 bit stereo DAC)");
	    break;
	 }
       #endif

       #ifdef DIGI_WINSOUNDSYS
	 case DIGI_WINSOUNDSYS:
	 {
	    static char *param[] = {"sound_port", "sound_dma", "sound_irq", "flip_pan", "quality", "sound_freq", "digi_volume", "digi_voices", NULL};
	    digi_cards[c].param = uconvert_static_string_array(param);
	    digi_cards[c].desc = uconvert_static_string("Windows Sound System or compatible (AD1848, CS4248, CS4231/A or CS4232 CODEC)");
	    break;
	 }
       #endif

       #ifdef DIGI_OSS
	 case DIGI_OSS:
	 {
	    static char *param[] = {"flip_pan", "sound_bits", "sound_stereo", "oss_numfrags", "oss_fragsize", "sound_freq", "oss_driver", "digi_volume", NULL};
	    digi_cards[c].param = uconvert_static_string_array(param);
	    digi_cards[c].desc = uconvert_static_string("Open Sound System");
	    break;
	 }
       #endif

       #ifdef DIGI_ESD
	 case DIGI_ESD:
	 {
	    static char *param[] = {"flip_pan", "sound_bits", "sound_stereo", "sound_freq", "esd_server", "digi_volume", NULL};
	    digi_cards[c].param = uconvert_static_string_array(param);
	    digi_cards[c].desc = uconvert_static_string("Enlightened Sound Daemon");
	    break;
	 }
       #endif

       #ifdef DIGI_ARTS
	 case DIGI_ARTS:
	 {
	    static char *param[] = {"flip_pan", "sound_bits", "sound_stereo", "sound_freq", "digi_volume", NULL};
	    digi_cards[c].param = uconvert_static_string_array(param);
	    digi_cards[c].desc = uconvert_static_string("aRts");
	    break;
	 }
       #endif

       #ifdef DIGI_ALSA
	 case DIGI_ALSA:
	 {
	    static char *param[] = {"flip_pan", "sound_bits", "sound_stereo", "alsa_numfrags", "alsa_card", "alsa_pcmdevice", "sound_freq", "digi_volume", NULL};
	    digi_cards[c].param = uconvert_static_string_array(param);
	    digi_cards[c].desc = uconvert_static_string("ALSA Sound System");
	    break;
	 }
       #endif
       
       #ifdef DIGI_BEOS
	 case DIGI_BEOS:
	 {
	    static char *param[] = {"flip_pan", "sound_bits", "sound_stereo", "sound_freq", "digi_volume", NULL};
	    digi_cards[c].param = uconvert_static_string_array(param);
	    digi_cards[c].desc = uconvert_static_string("BeOS Sound");
	 }
       #endif

      }

      info++;
      c++;
   }

   digi_cards[c].id = DIGI_NONE;
   digi_cards[c].name = uconvert_static_string("No Sound");
   digi_cards[c].param = NULL;
   digi_cards[c].desc = uconvert_static_string("The Sound of Silence...");
   digi_cards[c].present = FALSE;

   /* fill in list of MIDI drivers */
   midi_cards[0].id = MIDI_AUTODETECT;
   midi_cards[0].name = uconvert_static_string("Autodetect");
   midi_cards[0].param = uconvert_static_string_array(midi_param);
   midi_cards[0].desc = uconvert_static_string("Attempt to autodetect the MIDI hardware");
   midi_cards[0].present = FALSE;

   if (system_driver->midi_drivers)
      info = system_driver->midi_drivers();
   else
      info = _midi_driver_list;

   c = 1;

   while ((info->driver) && (info->id != MIDI_NONE)) {
      midi_cards[c].id = info->id;
      midi_cards[c].name = midi_cards[c].desc = uconvert_static_const_string(((MIDI_DRIVER *)info->driver)->ascii_name);
      midi_cards[c].param = NULL;
      midi_cards[c].present = FALSE;

      switch (info->id) {

       #ifdef MIDI_OPL2
	 case MIDI_OPL2:
	 {
	    static char *param[] = {"22", "fm_port", "midi_volume", "", "ibk_file", "ibk_drum_file", NULL};
	    midi_cards[c].param = uconvert_static_string_array(param);
	    midi_cards[c].desc = uconvert_static_string("(mono) OPL2 FM synth (used in Adlib and standard SB cards)");
	    break;
	 }
       #endif

       #ifdef MIDI_2XOPL2
	 case MIDI_2XOPL2:
	 {
	    static char *param[] = {"22", "fm_port", "midi_volume", "", "ibk_file", "ibk_drum_file", NULL};
	    midi_cards[c].param = uconvert_static_string_array(param);
	    midi_cards[c].desc = uconvert_static_string("(stereo) Two OPL2 FM synths (early SB Pro cards)");
	    break;
	 }
       #endif

       #ifdef MIDI_OPL3
	 case MIDI_OPL3:
	 {
	    static char *param[] = {"22", "fm_port", "midi_volume", "", "ibk_file", "ibk_drum_file", NULL};
	    midi_cards[c].param = uconvert_static_string_array(param);
	    midi_cards[c].desc = uconvert_static_string("(stereo) OPL3 FM synth (Adlib Gold, later SB Pro boards, SB16)");
	    break;
	 }
       #endif

       #ifdef MIDI_AWE32
	 case MIDI_AWE32:
	 {
	    static char *param[] = {"21", "midi_voices", "midi_volume", NULL};
	    midi_cards[c].param = uconvert_static_string_array(param);
	    midi_cards[c].desc = uconvert_static_string("SoundBlaster AWE32 (EMU8000 synth chip)");
	    break;
	 }
       #endif

       #ifdef MIDI_SB_OUT
	 case MIDI_SB_OUT:
	 {
	    static char *param[] = {"21", "sound_port", "midi_volume", NULL};
	    midi_cards[c].param = uconvert_static_string_array(param);
	    midi_cards[c].desc = uconvert_static_string("Raw SB MIDI output to an external synth module");
	    break;
	 }
       #endif

       #ifdef MIDI_MPU
	 case MIDI_MPU:
	 {
	    static char *param[] = {"22", "mpu_port", "mpu_irq", "", "midi_volume", NULL};
	    midi_cards[c].param = uconvert_static_string_array(param);
	    midi_cards[c].desc = uconvert_static_string("MIDI output to an external synth module or wavetable card");
	    break;
	 }
       #endif

       #ifdef MIDI_OSS
	 case MIDI_OSS:
	 {
	    static char *param[] = {"21", "oss_midi_driver", "midi_volume", NULL};
	    midi_cards[c].param = uconvert_static_string_array(param);
	    midi_cards[c].desc = uconvert_static_string("Open Sound System");
	    break;
	 }
       #endif

       #ifdef MIDI_ALSA
	 case MIDI_ALSA:
	 {
	    static char *param[] = {"22", "alsa_rawmidi_card", "alsa_rawmidi_device", "", "midi_volume", NULL};
	    midi_cards[c].param = uconvert_static_string_array(param);
	    midi_cards[c].desc = uconvert_static_string("ALSA RawMIDI");
	    break;
	 }
       #endif
       
       #ifdef MIDI_BEOS
	 case MIDI_BEOS:
	 {
	    static char *param[] = {"be_midi_quality", "be_midi_interpolation", "be_midi_reverb", "be_midi_freq", "midi_volume", NULL};
	    midi_cards[c].param = uconvert_static_string_array(param);
	    midi_cards[c].desc = uconvert_static_string("BeOS MIDI");
	    break;
	 }
       #endif

       #ifdef MIDI_CORE_AUDIO
	 case MIDI_CORE_AUDIO:
	 {
	    static char *param[] = {"22", "ca_midi_quality", "ca_midi_reverb", "", "midi_volume", NULL};
	    midi_cards[c].param = uconvert_static_string_array(param);
	    midi_cards[c].desc = uconvert_static_string("CoreAudio MIDI synthesizer");
	    break;
	 }
       #endif

       #ifdef MIDI_DIGMID
	 case MIDI_DIGMID:
	 {
	    static char *param[] = {"22", "midi_voices", "midi_volume", "", "patches", NULL};
	    midi_cards[c].param = uconvert_static_string_array(param);
	    midi_cards[c].desc = uconvert_static_string("Software wavetable synthesis using the digital sound hardware");
	    break;
	 }
       #endif
      }

      info++;
      c++;
   }

   midi_cards[c].id = MIDI_NONE;
   midi_cards[c].name = uconvert_static_string("No Sound");
   midi_cards[c].param = NULL;
   midi_cards[c].desc = uconvert_static_string("The Sound of Silence...");
   midi_cards[c].present = FALSE;
}



/* in some places we need to double-buffer the display... */
static BITMAP *buffer;



/* background graphic, font, and test sounds */
#ifdef SETUP_USE_COMPILED_DATAFILES
extern DATAFILE setup_data[];
#else
static DATAFILE *setup_data;
#endif



/* dialogs do fancy stuff as they slide on and off the screen */
typedef enum DIALOG_STATE
{
   state_start,
   state_slideon,
   state_active,
   state_slideoff,
   state_exit,
   state_chain,
   state_redraw
} DIALOG_STATE;



/* info about an active dialog */
typedef struct ACTIVE_DIALOG
{
   DIALOG_STATE state;
   int time;
   DIALOG *dialog;
   DIALOG_PLAYER *player;
   BITMAP *buffer;
   DIALOG_STATE (*handler)(int c);
} ACTIVE_DIALOG;



/* list of active dialogs */
static ACTIVE_DIALOG dialogs[4];

static int dialog_count = 0;



/* scrolly text message at the base of the screen */
#define SCROLLER_LENGTH  (SCREEN_W/8+2)

static volatile int scroller_time = 0;
static char scroller_msg[256];
static int scroller_pos = 0;
static int scroller_alpha = 256;
static char *scroller_string = EMPTY_STRING;
static char *wanted_scroller = EMPTY_STRING;
static int scroller_string_pos = 0;



/* helper function for clearing the scroller message */
static void clear_scroller(void)
{
   int i;
   char *p;

   p = scroller_msg;

   for (i=0; i<SCROLLER_LENGTH; i++)
      p += usetc(p, ' ');
}



/* helper function to shift the scroller message */
static void shift_scroller(int c)
{
   int i;
   char *buffer, *p_src, *p_dest;

   buffer = malloc(SCROLLER_LENGTH * uwidth_max(U_CURRENT));
   p_src = scroller_msg + uwidth(scroller_msg);
   p_dest = buffer;

   for (i=0; i<SCROLLER_LENGTH-1; i++)
      p_dest += usetc(p_dest, ugetx(&p_src));

   usetc(p_dest, c);
   ustrncpy(scroller_msg, buffer, SCROLLER_LENGTH);
   free(buffer);
}



/* timer interrupt handler */
static void inc_scroller_time(void)
{
   scroller_time++;
}

END_OF_STATIC_FUNCTION(inc_scroller_time);



/* dialog procedure for animating the scroller text */
static int scroller_proc(int msg, DIALOG *d, int c)
{
   int redraw = FALSE;
   int ch, a, i, x;
   char *p;
   char tmp[32];

   if (msg == MSG_IDLE) {
      while (scroller_time > 0) {
	 scroller_pos--;
	 if (scroller_pos <= -8) {
	    scroller_pos = 0;
	    if ((ch = ugetat(scroller_string, scroller_string_pos)) != 0) {
	       shift_scroller(ch);
               scroller_string_pos++;
            }
	    else
               shift_scroller(' ');
	    if (wanted_scroller != scroller_string) {
	       scroller_alpha -= MIN(32, scroller_alpha);
	       if (scroller_alpha <= 0) {
                  clear_scroller();
		  scroller_string = wanted_scroller;
		  scroller_string_pos = 0;
		  for (i=0; i<4; i++) {
		     if ((ch = ugetat(scroller_string, scroller_string_pos)) != 0) {
                        shift_scroller(ch);
			scroller_string_pos++;
		     }
		  }
		  scroller_alpha = 256;
	       }
	    }
	    else
	       scroller_alpha += MIN(16, 256-scroller_alpha);
	 }
	 redraw = TRUE;
	 scroller_time--;
      }
   }
   else if (msg == MSG_RADIO) {
      clear_scroller();
      scroller_string = wanted_scroller;
      scroller_string_pos = ustrlen(scroller_string);
      scroller_alpha = 256;
      redraw = TRUE;
   }

   if (redraw) {
      freeze_mouse_flag = TRUE;
      acquire_screen();

      p = scroller_msg;
      for (i=0; i<SCROLLER_LENGTH; i++) {
	 x = i*8+scroller_pos;
	 a = 16 + CLAMP(0, 15-ABS(SCREEN_W/2-x)/(SCREEN_W/32), 15) * scroller_alpha/256;
	 textprintf_ex(screen, font, x, SCREEN_H-16, a, 0, uconvert_ascii("%c", tmp), ugetx(&p));
      }

      release_screen();
      freeze_mouse_flag = FALSE;
   }

   return D_O_K;
}



/* helper for drawing a dialog onto a memory bitmap */
static void draw_dialog(ACTIVE_DIALOG *d)
{
   BITMAP *oldscreen = screen;
   int nowhere;

   if (d->player->focus_obj >= 0) {
      object_message(d->dialog+d->player->focus_obj, MSG_LOSTFOCUS, 0);
      d->dialog[d->player->focus_obj].flags &= ~D_GOTFOCUS;
      d->player->focus_obj = -1;
   }

   if (d->player->mouse_obj >= 0) {
      object_message(d->dialog+d->player->mouse_obj, MSG_LOSTMOUSE, 0);
      d->dialog[d->player->mouse_obj].flags &= ~D_GOTMOUSE;
      d->player->mouse_obj = -1;
   }

   d->player->res &= ~D_WANTFOCUS;

   clear_bitmap(d->buffer);
   screen = d->buffer;
   dialog_message(d->dialog, MSG_DRAW, 0, &nowhere);
   screen = oldscreen;
}



/* start up another dialog */
static void activate_dialog(DIALOG *dlg, DIALOG_STATE (*handler)(int c), int chain)
{
   ACTIVE_DIALOG *d = &dialogs[dialog_count];
   int i;

   for (i=0; dlg[i].proc; i++) {
      if (dlg[i].proc == scroller_proc) {
	 dlg[i].proc = NULL;
	 centre_dialog(dlg);
	 dlg[i].proc = scroller_proc;
	 break;
      }
   }

   d->state = state_start;
   d->time = retrace_count;
   d->dialog = dlg;
   d->player = init_dialog(dlg, -1);
   d->buffer = create_bitmap(SCREEN_W, SCREEN_H);
   d->handler = handler;

   draw_dialog(d);

   if (dialog_count > 0) {
      draw_dialog(&dialogs[dialog_count-1]);
      dialogs[dialog_count-1].state = (chain ? state_chain : state_slideoff);
      dialogs[dialog_count-1].time = retrace_count;
   }
}



/* main dialog update routine */
static int update(void)
{
   BITMAP *oldscreen = screen;
   ACTIVE_DIALOG *d;
   DIALOG_STATE state;
   PALETTE pal;
   BITMAP *b;
   int pos, ppos, pppos;
   int ret;
   char tmp[256];

   if (dialog_count <= 0)
      return FALSE;

   d = &dialogs[dialog_count-1];

   if (d->state == state_active) {
      /* process the dialog */
      if (_mouse_screen != screen) {
         set_mouse_sprite(NULL);
         show_mouse(screen);
      }

      ret = update_dialog(d->player);

      if (!ret) {
	 if (d->handler)
	    state = d->handler(d->player->obj);
	 else
	    state = state_exit;

	 if (state == state_exit) {
	    /* exit this dialog */
	    draw_dialog(d);
	    d->state = state_exit;
	    d->time = retrace_count;
	 }
	 else if (state == state_redraw) {
	    /* redraw the dialog */
	    d->player->res |= D_REDRAW;
	 }
      }
      else {
	 pos = find_dialog_focus(d->dialog);
	 if ((pos >= 0) && (d->dialog[pos].dp3))
	    wanted_scroller = d->dialog[pos].dp3;
      }
   }
   else {
      /* sliding on or off */
      show_mouse(NULL);

      b = setup_data[BACKGROUND].dat;
      stretch_blit(b, buffer, 0, 0, b->w, b->h, 0, 0, SCREEN_W, SCREEN_H);

      textout_centre_ex(buffer, font, uconvert_ascii(SETUP_TITLE, tmp), SCREEN_W/2, 0, -1, 0);

      wanted_scroller = empty_string;
      screen = buffer;
      scroller_proc(MSG_IDLE, NULL, 0);
      screen = oldscreen;

      pos = retrace_count - d->time;

      if ((dialog_count == 1) && (d->state == state_start))
	 pos *= 64;
      else if ((dialog_count == 1) && (d->state == state_exit))
	 pos *= 48;
      else if (d->state == state_start)
	 pos *= 96;
      else
	 pos *= 128;

      pos = CLAMP(0, 4096-pos, 4096);
      pppos = (4096 - pos * pos / 4096);
      pos = pppos / 16;

      /* draw the slide effect */
      switch (d->state) {

	 case state_start:
	    ppos = pos;
	    stretch_sprite(buffer, d->buffer, 0, 0, SCREEN_W+(1024-pppos/4)*SCREEN_W/320, SCREEN_H+(1024-pppos/4)*SCREEN_H/200);
	    break;

	 case state_slideon:
	    ppos = pos;
	    stretch_sprite(buffer, d->buffer, 0, 0, SCREEN_W*ppos/256, SCREEN_H*ppos/256);
	    break;

	 case state_slideoff:
	 case state_chain:
	    ppos = 256 - pos;
	    stretch_sprite(buffer, d->buffer, SCREEN_W/2-SCREEN_W*ppos/512, SCREEN_H/2-SCREEN_H*ppos/512, SCREEN_W*ppos/256, SCREEN_H*ppos/256);
	    break;

	 case state_exit:
	    ppos = 256 - pos;
	    stretch_sprite(buffer, d->buffer, SCREEN_W-SCREEN_W*ppos/256, SCREEN_H-SCREEN_H*ppos/256, SCREEN_W*ppos/256, SCREEN_H*ppos/256);
	    break;

	 default:
	    ppos = 0;
	    break;
      }

      if ((dialog_count == 1) && (d->state != state_slideon) && (d->state != state_slideoff) && (d->state != state_chain)) {
	 fade_interpolate(black_palette, setup_data[SETUP_PAL].dat, pal, ppos/4, 0, 255);
	 set_palette(pal);
      }
      else
	 vsync();

      blit(buffer, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);

      if (pos >= 256) {
	 /* finished the slide */
	 switch (d->state) {

	    case state_start:
	    case state_slideon:
	       /* become active */
	       d->state = state_active;
	       d->player->res |= D_REDRAW;
	       break;

	    case state_slideoff:
	       /* activate next dialog, and then get ready to slide back on */
	       dialogs[dialog_count].time = retrace_count;
	       dialog_count++;
	       d->state = state_slideon;
	       break;

	    case state_exit:
	       /* time to die! */
	       shutdown_dialog(d->player);
	       destroy_bitmap(d->buffer);
	       dialog_count--;
	       if (dialog_count > 0)
		  dialogs[dialog_count-1].time = retrace_count;
	       break;

	    case state_chain:
	       /* kill myself, then activate the next dialog */
	       shutdown_dialog(d->player);
	       destroy_bitmap(d->buffer);
	       dialogs[dialog_count].time = retrace_count;
	       dialogs[dialog_count-1] = dialogs[dialog_count];
	       break;

	    default:
	       break;
	 }
      }
   }

   return TRUE;
}



/* helper for checking which drivers are valid */
static void detect_sound(void)
{
   int i;

   for (i=0; digi_cards[i].id != DIGI_NONE; i++) {
      if (detect_digi_driver(digi_cards[i].id) == 0)
	 digi_cards[i].present = FALSE;
      else
	 digi_cards[i].present = TRUE;
   }
   digi_cards[i].present = TRUE;

   for (i=0; midi_cards[i].id != MIDI_NONE; i++) {
      if (detect_midi_driver(midi_cards[i].id) == 0)
	 midi_cards[i].present = FALSE;
      else
	 midi_cards[i].present = TRUE;
   }
   midi_cards[i].present = TRUE;
}



/* helper for initialising the sound code */
static int init_sound(char *msg)
{
   char b1[256], b2[256], tmp[32];
   int i, begin, end;

   show_mouse(NULL);

   detect_sound();

   if (install_sound(DIGI_AUTODETECT, MIDI_AUTODETECT, NULL) == 0)
      return 0;

   if (ustrlen(allegro_error) <= 32) {
      ustrzcpy(b1, sizeof(b1), allegro_error);
      usetc(b2, 0);
   }
   else {
      begin = ustrlen(allegro_error)*9/16;
      end = 10;

      for (i=begin; i>end; i--)
	 if (ugetat(allegro_error, i) == ' ')
	    break;

      ustrzncpy(b1, sizeof(b1), allegro_error, i);
      ustrzcpy(b2, sizeof(b2), allegro_error+uoffset(allegro_error, (i == end) ? end : i+1));
   }

   alert(msg, b1, b2, uconvert_ascii("Ok", tmp), NULL, 0, 0);

   return -1;
}



static BITMAP *popup_bitmap = NULL;
static BITMAP *popup_bitmap2 = NULL;



/* helper for displaying a popup message */
static void popup(AL_CONST char *s1, AL_CONST char *s2)
{
   int w, w2, c, ct;

   if (!popup_bitmap) {
      c = MAX(512*SCREEN_W/320, 512*SCREEN_H/200);
      ct = 16*SCREEN_W/320;

      acquire_screen();

      for (w=c; w>=0; w-=2) {
	 line(screen, w, 16, 0, 16+w, 0);
	 if (!(w%ct)) {
	    release_screen();
	    vsync();
	    acquire_screen();
	 }
      }

      for (w=0; w<c; w+=2) {
	 line(screen, w+1, 16, 0, 17+w, 0);
	 if (!(w%ct)) {
	    release_screen();
	    vsync();
	    acquire_screen();
	 }
      }

      release_screen();

      popup_bitmap = create_bitmap(SCREEN_W, SCREEN_H);
      popup_bitmap2 = create_bitmap(SCREEN_W, SCREEN_H);
      blit(screen, popup_bitmap, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
   }

   blit(popup_bitmap, popup_bitmap2, 0, 0, 0, 0, SCREEN_W, SCREEN_H);

   if ((s1) || (s2)) {
      if (s1)
	 w = text_length(font, s1);
      else
	 w = 0;

      if (s2)
	 w2 = text_length(font, s2);
      else
	 w2 = 0;

      w = MAX(w, w2);

      rectfill(popup_bitmap2, (SCREEN_W-w)/2-15, SCREEN_H/2-31, (SCREEN_W+w)/2+15, SCREEN_H/2+31, 0);
      rect(popup_bitmap2, (SCREEN_W-w)/2-16, SCREEN_H/2-32, (SCREEN_W+w)/2+16, SCREEN_H/2+32, 255);

      if (s1)
	 textout_centre_ex(popup_bitmap2, font, s1, SCREEN_W/2, SCREEN_H/2-20, -1, -1);

      if (s2)
	 textout_centre_ex(popup_bitmap2, font, s2, SCREEN_W/2, SCREEN_H/2+4, -1, -1);
   }

   blit(popup_bitmap2, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
}



/* ends the display of a popup message */
static void end_popup(void)
{
   if (popup_bitmap) {
      blit(popup_bitmap, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
      destroy_bitmap(popup_bitmap);
      popup_bitmap = NULL;
      destroy_bitmap(popup_bitmap2);
      popup_bitmap2 = NULL;
   }
}



/* joystick test display */
static void plot_joystick_state(BITMAP *bmp, int i)
{
   char buf[512], tmp[256];
   int j, x, y;
   int c = 0;

   if (joystick_driver) {
      if (num_joysticks > 1)
	 textprintf_ex(bmp, font, SCREEN_W/2-96, SCREEN_H/2-60+c*20, -1, -1, uconvert_ascii("%s (%d/%d)", tmp),
                    joystick_driver->name, i+1, num_joysticks);
      else
	 textout_ex(bmp, font, joystick_driver->name, SCREEN_W/2-96, SCREEN_H/2-60+c*20, -1, -1);
      c++;
   }

   for (j=0; j<joy[i].num_sticks; j++) {
      if (joy[i].stick[j].num_axis == 2) {
	 if (joy[i].stick[j].flags & JOYFLAG_ANALOGUE) {
	    x = (joy[i].stick[j].axis[0].pos+128)*SCREEN_W/256;
	    y = (joy[i].stick[j].axis[1].pos+128)*SCREEN_H/256;
	    hline(bmp, x-12, y, x+12, 15);
	    vline(bmp, x, y-12, y+12, 15);
	    circle(bmp, x, y, 12, 1);
	    circlefill(bmp, x, y, 4, 31);
	 }
	 else {
	    uszprintf(buf, sizeof(buf), uconvert_ascii("%s:", tmp), joy[i].stick[j].name);
	    if (joy[i].stick[j].axis[1].d1)
	       ustrzcat(buf, sizeof(buf), uconvert_ascii(" up", tmp));
	    if (joy[i].stick[j].axis[1].d2)
	       ustrzcat(buf, sizeof(buf), uconvert_ascii(" down", tmp));
	    if (joy[i].stick[j].axis[0].d1)
	       ustrzcat(buf, sizeof(buf), uconvert_ascii(" left", tmp));
	    if (joy[i].stick[j].axis[0].d2)
	       ustrzcat(buf, sizeof(buf), uconvert_ascii(" right", tmp));
	    textout_ex(bmp, font, buf, SCREEN_W/2-96, SCREEN_H/2-60+c*20, -1, -1);
	    c++;
	 }
      }
      else {
	 uszprintf(buf, sizeof(buf), uconvert_ascii("%s: %s %4d %s", tmp),
                   joy[i].stick[j].name,
		   uconvert_ascii((joy[i].stick[j].axis[0].d1) ? "<-" : "  ", tmp),
		   joy[i].stick[j].axis[0].pos,
		   uconvert_ascii((joy[i].stick[j].axis[0].d2) ? "->" : "  ", tmp));

	 textout_ex(bmp, font, buf, SCREEN_W/2-96, SCREEN_H/2-60+c*20, -1, -1);
	 c++;
      }
   }

   for (j=0; j<joy[i].num_buttons; j++) {
      uszprintf(buf, sizeof(buf), uconvert_ascii("%s: %c", tmp), joy[i].button[j].name, joy[i].button[j].b ? '*' : 0);

      if (j&1) {
	 textout_ex(bmp, font, buf, SCREEN_W/2+32, SCREEN_H/2-60+c*20, -1, -1);
	 c++;
      }
      else
	 textout_ex(bmp, font, buf, SCREEN_W/2-96, SCREEN_H/2-60+c*20, -1, -1);
   }

   textout_centre_ex(bmp, font, uconvert_ascii("- press a key to accept -", tmp), SCREEN_W/2, SCREEN_H-16, 255, -1);
}



/* helper for calibrating the joystick */
static void joystick_proc(int type)
{
   int i, c;
   char tmp1[256], tmp2[32];

   scroller_proc(MSG_RADIO, NULL, 0);
   scare_mouse();

   remove_joystick();

   if (install_joystick(type) != 0) {
      alert(uconvert_ascii("Error:", tmp1), allegro_error, NULL, uconvert_ascii("Ok", tmp2), NULL, 0, 0);
      unscare_mouse();
      return;
   }

   for (i=0; i<num_joysticks; i++) {
      while (joy[i].flags & JOYFLAG_CALIBRATE) {
	 popup(calibrate_joystick_name(i), uconvert_ascii("and press a button", tmp1));

	 rest(10);
	 do {
	    poll_joystick();
	 } while ((!joy[i].button[0].b) && (!joy[i].button[1].b) && (!keypressed()));

	 if (calibrate_joystick(i) != 0) {
	    remove_joystick();
	    alert(uconvert_ascii("Error calibrating joystick", tmp1), NULL, NULL, uconvert_ascii("Ok", tmp2), NULL, 0, 0);
	    end_popup();
	    unscare_mouse();
	    return;
	 }

	 rest(10);
	 do {
	    poll_joystick();
	 } while ((joy[i].button[0].b) || (joy[i].button[1].b));

	 clear_keybuf();
      }
   }

   if (!popup_bitmap)
      popup(NULL, NULL);

   do {
      poll_mouse();
   } while (mouse_b);

   clear_keybuf();

   i = 0;
   c = 0;

   while ((!c) && (!mouse_b)) {
      poll_joystick();
      poll_mouse();

      blit(popup_bitmap, popup_bitmap2, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
      plot_joystick_state(popup_bitmap2, i);
      blit(popup_bitmap2, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);

      if (keypressed()) {
	 c = readkey();

	 switch (c>>8) {

	    case KEY_UP:
	    case KEY_RIGHT:
	       i++;
	       c = 0;
	       break;

	    case KEY_DOWN:
	    case KEY_LEFT:
	       i--;
	       c = 0;
	       break;

	    default:
	       c &= 0xFF;
	       break;
	 }

	 if (i >= num_joysticks)
	    i = num_joysticks-1;

	 if (i < 0)
	    i = 0;
      }
   }

   end_popup();

   do {
      poll_mouse();
   } while (mouse_b);

   clear_keybuf();

   unscare_mouse();
}



/* helper for choosing a mouse type */
static void mouse_proc(int type)
{
   remove_mouse();
   _mouse_type = type;
   install_mouse();
}



#ifdef ALLEGRO_LINUX
static void get_mouse_drivers(_DRIVER_INFO **list, int *list_size);

/* Number of devices tested. */
#define NUM_FD 6

/* Some devices can't be read one byte at a time. */
#define PACKET_SIZE 16



static void reset_for_select(int *fd, fd_set *readfds, struct timeval *timeout)
{
   int n;

   FD_ZERO(readfds);

   for (n=0; n<NUM_FD; n++) {
      if (fd[n] != -1)
         FD_SET(fd[n], readfds);
   }

   if (timeout) {
     timeout->tv_sec = 0;
     timeout->tv_usec = 0;
   }
}



static int detect_mouse(void)
{
   int fd[NUM_FD];
   char *devices[NUM_FD] = {
      "/dev/mouse",
      "/dev/input/mice", /* Collects all the evdev mice, PS/2-style */
      "/dev/input/event0",
      "/dev/input/event1",
      "/dev/input/event2",
      "/dev/input/event3"
   };
   fd_set readfds;
   struct timeval timeout;
   int n, i;
   int w,l,r,t,b;
   int retval = -1;
   char buffer_array[NUM_FD][16*PACKET_SIZE];
   int buffer_size = sizeof(buffer_array[0]);
   char *buffer;
   char tmp1[256], tmp2[256], tmp3[64], tmp4[64];
   AL_CONST char *drv_name;
   int count_array[NUM_FD] = {0, 0, 0, 0};
   int count;
   _DRIVER_INFO *list;
   int list_size;

   remove_mouse();

   /* We try to open several devices because we don't know yet which one
    * is the mouse.  If at least one was opened, we continue detection.
    */
   retval = -1;
   for (n=0; n<NUM_FD; n++) {
      fd[n] = open(devices[n], O_RDONLY | O_NONBLOCK);
      if (fd[n] != -1)
         retval = 0;
   }

   if (retval == -1) {
      alert(uconvert_ascii("Couldn't open any device file:", tmp1), ustrerror(errno), NULL, uconvert_ascii("Ok", tmp2), NULL, 0, 0);
      goto End;
   }

   popup(uconvert_ascii("Move your mouse around", tmp1), uconvert_ascii("Press any key to cancel", tmp2));

   /* Flush the files. */
   while (TRUE) {
      reset_for_select(fd, &readfds, &timeout);
      retval = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);  /* zero timeout */

      if ((retval == -1) && (errno != EINTR)) {
	 alert(uconvert_ascii("Couldn't flush device files:", tmp1), ustrerror(errno), NULL, uconvert_ascii("Ok", tmp2), NULL, 0, 0);
	 goto End;
      }

      if (retval == 0)
	 break;

      if (retval > 0) {
	 for (n=0; n<NUM_FD; n++) {
	    if ((fd[n] != -1) && FD_ISSET(fd[n], &readfds)) {
	       while (read(fd[n], buffer_array[n], PACKET_SIZE) > 0)
		  ;
	    }
	 }
      }
   }

   w = buffer_size;
   l = (SCREEN_W - w)/2;
   r = l + w;
   t = SCREEN_H/2 + 64;
   b = t + 16;

   rect(popup_bitmap2, l-1, t-1, r, b+1, gui_fg_color);
   blit(popup_bitmap2, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);

   /* Read the data produced by the user moving the mouse.
    * In most case data will be read from only one file.
    */
   for (count = 0; count < buffer_size; ) {
      reset_for_select(fd, &readfds, NULL);

      if (select(FD_SETSIZE, &readfds, NULL, NULL, &timeout) > 0) {
	 for (n=0; n<NUM_FD; n++) {
	    if ((fd[n] != -1) && FD_ISSET(fd[n], &readfds)) {
	       unsigned int size = MIN(PACKET_SIZE, buffer_size - count_array[n]);
	       count_array[n] += read(fd[n], buffer_array[n]+count_array[n], size);

	       if (count_array[n] > count) {
		  for ( ; count != count_array[n]; count++) {
		     vline(popup_bitmap2, l + count*w/buffer_size, t, b, gui_mg_color);
		     blit(popup_bitmap2, screen, 0, 0, 0, 0, SCREEN_W, SCREEN_H);
		  }
	       }
	    }
	 }
      }

      if (keypressed() && readkey())
	 break;
   }

   /* Catch problems with data reading. */
   if (count < buffer_size) {
      if (count == 0)
	 popup(uconvert_ascii("No data received", tmp1), NULL);
      else
	 popup(uconvert_ascii("Insufficient data received", tmp1), NULL);   

      retval = -1;
      goto End;
   }

   /* The buffer with the most data is very likely the mouse device. */
   buffer = NULL;
   for (n=0; n<NUM_FD; n++) {
      if (count_array[n] == count)
         buffer = buffer_array[n];
   }

   ASSERT(buffer);

   popup(uconvert_ascii("Analysing data...", tmp1), NULL);

   /* Ask the drivers if they understand the mouse. */
   get_mouse_drivers(&list, &list_size);
   for (i = 0; i < list_size; i++) {
      MOUSE_DRIVER *drv = list[i].driver;
      if (drv->analyse_data && drv->analyse_data (buffer, count)) {
	 drv_name = get_config_text(drv->ascii_name);
	 if (alert(uconvert_ascii("This driver understands your mouse:", tmp1), drv_name, uconvert_ascii("Select it?", tmp2),
		   uconvert_ascii("Yes", tmp3), uconvert_ascii("No", tmp4), 'y', 'n') == 1)
	    break;
      }
   }

   if (i < list_size) {
      retval = i;
   }
   else {
      alert(uconvert_ascii("No driver understands your mouse", tmp1), uconvert_ascii("Try using the GPM repeater", tmp2),
	    uconvert_ascii("or set the device filename by hand", tmp3), uconvert_ascii("Ok", tmp4), NULL, 0, 0);
      retval = -1;
   }

 End:
   for (n=0; n<NUM_FD; n++) {
      if (fd[n] != -1)
         close(fd[n]);
   }

   end_popup();
   install_mouse();
   return retval;
}
#endif



/* helper function for building a frequency list */
#define FREQ_ENTRY_SIZE  64

static void build_freq_list(char *freq[], int nfreq, int freq_value[])
{
   int i;
   char tmp[64];

   for (i=0; i<nfreq; i++) {
      freq[i] = malloc(FREQ_ENTRY_SIZE);
      uszprintf(freq[i], FREQ_ENTRY_SIZE, uconvert_ascii("%d hz", tmp), freq_value[i]);
   }
}



static void destroy_freq_list(char *freq[], int nfreq)
{
   int i;

   for (i=0; i<nfreq; i++) {
      if (freq[i]) {
         free(freq[i]);
         freq[i] = NULL;
      }
   }
}



/* dialog callback for retrieving the SB frequency list */
static char *freq_getter(int index, int *list_size)
{
   static char *freq[4];

   if (index < 0) {
      if (list_size) {
	 switch (soundcard->id) {

          #if (defined DIGI_SB10) || (defined DIGI_SB15)
            case DIGI_SB10:
	    case DIGI_SB15:
	       *list_size = 2;
               break;
          #endif

          #ifdef DIGI_SBPRO
	    case DIGI_SBPRO:
               *list_size = 3;
               break;
          #endif

	    default:
	       *list_size = 4;
	       break;
	 }
      }
      else {
         destroy_freq_list(freq, 4);
      }

      return NULL;
   }

   /* first time ? */
   if (!freq[0]) {
    #ifdef DIGI_AUDIODRIVE
      if (soundcard->id == DIGI_AUDIODRIVE) {
	 int ess_freq[] =
	 {
	    11363,
	    17046,
	    22729,
	    44194
	 };

         build_freq_list(freq, 4, ess_freq);
      }
      else
    #endif

    #ifdef DIGI_SOUNDSCAPE
      if (soundcard->id == DIGI_SOUNDSCAPE) {
	 int esc_freq[] =
	 {
	    11025,
	    16000,
	    22050,
	    48000
	 };

         build_freq_list(freq, 4, esc_freq);
      }
      else
    #endif

    #ifdef DIGI_WINSOUNDSYS
      if (soundcard->id == DIGI_WINSOUNDSYS) {
	 int wss_freq[] =
	 {
	    11025,
	    22050,
	    44100,
	    48000
	 };

         build_freq_list(freq, 4, wss_freq);
      }
      else
    #endif

    #ifdef ALLEGRO_DOS
      {
	 int sb_freq[] =
	 {
	    11906,
	    16129,
	    22727,
	    45454
	 };

         build_freq_list(freq, 4, sb_freq);
      }
    #else
      {
	 int default_freq[] =
	 {
	    11025,
	    22050,
	    44100,
	    48000
	 };

         build_freq_list(freq, 4, default_freq);
      }
    #endif
   }

   return freq[index];
}



/* dialog callback for retrieving information about the soundcard list */
static char *card_getter(int index, int *list_size)
{
   static char buf[256];
   char tmp[64];
   int i;

   if (index < 0) {
      i = 0;
      while (soundcard[i].id)
	 i++;
      if (list_size)
	 *list_size = i+1;
      return NULL;
   }

   if (soundcard[index].present)
      uszprintf(buf, sizeof(buf), uconvert_ascii("- %s", tmp), soundcard[index].name);
   else
      uszprintf(buf, sizeof(buf), uconvert_ascii("  %s", tmp), soundcard[index].name);

   return buf;
}



static char keyboard_type[256] = EMPTY_STRING;

static int num_keyboard_layouts = 0;
static char *keyboard_layouts[256];
static char *keyboard_names[256];



static char language_type[256] = EMPTY_STRING;

static int num_language_layouts = 0;
static char *language_layouts[256];
static char *language_names[256];



/* helper for sorting the keyboard list */
static void sort_keyboards(void)
{
   int done, i;

   do {
      done = TRUE;

      for (i=0; i<num_keyboard_layouts-1; i++) {
	 if (ustricmp(keyboard_names[i], keyboard_names[i+1]) > 0) {
	    char *tl = keyboard_layouts[i];
	    char *tn = keyboard_names[i];

	    keyboard_layouts[i] = keyboard_layouts[i+1];
	    keyboard_names[i] = keyboard_names[i+1];

	    keyboard_layouts[i+1] = tl;
	    keyboard_names[i+1] = tn;

	    done = FALSE;
	 }
      }

   } while (!done);
}



/* helper for sorting the language list */
static void sort_languages(void)
{
   int done, i;

   do {
      done = TRUE;

      for (i=0; i<num_language_layouts-1; i++) {
	 if (ustricmp(language_names[i], language_names[i+1]) > 0) {
	    char *tl = language_layouts[i];
	    char *tn = language_names[i];

	    language_layouts[i] = language_layouts[i+1];
	    language_names[i] = language_names[i+1];

	    language_layouts[i+1] = tl;
	    language_names[i+1] = tn;

	    done = FALSE;
	 }
      }

   } while (!done);
}



/* dialog callback for retrieving information about the keyboard list */
static char *keyboard_getter(int index, int *list_size)
{
   if (index < 0) {
      if (list_size)
	 *list_size = num_keyboard_layouts;
      return NULL;
   }

   return keyboard_names[index];
}



/* dialog callback for retrieving information about the language list */
static char *language_getter(int index, int *list_size)
{
   if (index < 0) {
      if (list_size)
	 *list_size = num_language_layouts;
      return NULL;
   }

   return language_names[index];
}



/* counts how many joystick drivers we have, and returns a list of them */
static void get_joystick_drivers(_DRIVER_INFO **list, int *list_size)
{
   _DRIVER_INFO *l;
   int c;

   if (system_driver->joystick_drivers)
      l = system_driver->joystick_drivers();
   else
      l = _joystick_driver_list;

   c = 0;
   while (l[c].driver)
      c++;

   *list = l;
   *list_size = c;
}



/* dialog callback for retrieving information about the joystick list */
static AL_CONST char *joystick_getter(int index, int *list_size)
{
   _DRIVER_INFO *list;
   int size;

   get_joystick_drivers(&list, &size);

   if (index < 0) {
      *list_size = size;
      return NULL;
   }

   return uconvert_static_const_string(((JOYSTICK_DRIVER *)list[index].driver)->ascii_name);
}



/* counts how many mouse drivers we have, and returns a list of them */
static void get_mouse_drivers(_DRIVER_INFO **list, int *list_size)
{
   _DRIVER_INFO *l;
   int c;

   if (system_driver->mouse_drivers)
      l = system_driver->mouse_drivers();
   else
      l = _mouse_driver_list;

   c = 0;
   while (l[c].driver)
      c++;

   *list = l;
   *list_size = c;
}



/* dialog callback for retrieving information about the mouse list */
static AL_CONST char *mouse_getter(int index, int *list_size)
{
   _DRIVER_INFO *list;
   int size;

   get_mouse_drivers(&list, &size);

   if (index < 0) {
      *list_size = size;
      return NULL;
   }

   return uconvert_static_const_string(((MOUSE_DRIVER *)list[index].driver)->ascii_name);
}



/* dialog procedure for the soundcard selection listbox */
static int card_proc(int msg, DIALOG *d, int c)
{
   int ret = d_list_proc(msg, d, c);
   d->dp3 = (void*)soundcard[d->d1].desc;
   return ret;
}



/* dialog procedure for the filename selection objects */
#define FILENAME_SIZE  512  /* > 80 chars * max UTF8 char width */

static int filename_proc(int msg, DIALOG *d, int c)
{
   PARAMETER *p = d->dp2;
   BITMAP *b;
   char buf[FILENAME_SIZE], buf2[256], tmp[256];
   char *q;
   int ret;

   if (msg == MSG_START) {
      if (!p->e2)
         p->e2 = malloc(FILENAME_SIZE);
         ustrzcpy(p->e2, FILENAME_SIZE, p->value);
   }
   else if (msg == MSG_END) {
      if (p->e2) {
	 free(p->e2);
	 p->e2 = NULL;
      }
   }

   ret = d_check_proc(msg, d, c);

   if (ret & D_CLOSE) {
      if (ugetc(p->value)) {
	 ustrzcpy(p->e2, FILENAME_SIZE, p->value);
	 usetc(p->value, 0);
      }
      else {
	 scroller_proc(MSG_RADIO, NULL, 0);

	 ustrzcpy(buf2, sizeof(buf2), p->desc);

         q = ustrchr(buf2 + uwidth(buf2), '(');
         if (q) {
            usetc(q, 0);
            usetat(buf2, -1, 0);
         }

	 ustrzcpy(buf, sizeof(buf), p->e2);

	 if (file_select_ex(buf2, buf, p->e1, sizeof(buf), 0, 0)) {
	    ustrzcpy(p->value, sizeof(p->value), buf);
	    ustrzcpy(p->e2, FILENAME_SIZE, buf);
	 }

	 scare_mouse();

	 b = setup_data[BACKGROUND].dat;
	 stretch_blit(b, screen, 0, 0, b->w, b->h, 0, 0, SCREEN_W, SCREEN_H);

	 textout_centre_ex(screen, font, uconvert_ascii(SETUP_TITLE, tmp), SCREEN_W/2, 0, -1, 0);
	 unscare_mouse();
      }

      if (p->value[0])
	 d->flags |= D_SELECTED;
      else
	 d->flags &= ~D_SELECTED;

      ret &= ~D_CLOSE;
      ret |= D_REDRAW;
   }

   return ret;
}



/* wrapper for d_list_proc() to free up the dp2 parameter */
static int d_xlist_proc(int msg, DIALOG *d, int c)
{
   void *old_dp2;
   int ret;

   old_dp2 = d->dp2;
   d->dp2 = NULL;

   ret = d_list_proc(msg, d, c);

   d->dp2 = old_dp2;

   return ret;
}



static char backup_str[256] = "Go back to the previous menu";
static char digi_desc[256] = EMPTY_STRING;
static char midi_desc[256] = EMPTY_STRING;



static DIALOG main_dlg[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key)    (flags)     (d1)           (d2)     (dp)                          (p)      (help message) */
   { d_button_proc,     30,   32,   125,  23,   -1,   16,   0,       D_EXIT,     0,             0,       "Autodetect",                 NULL,    "Attempt to autodetect your soundcard (ie. guess :-)" },
   { d_button_proc,     166,  32,   125,  23,   -1,   16,   0,       D_EXIT,     0,             0,       "Test",                       NULL,    "Test the current settings" },
   { d_button_proc,     30,   60,   125,  23,   -1,   16,   0,       D_EXIT,     0,             0,       "Digital Driver",             NULL,    "Manually select a driver for playing digital samples" },
   { d_button_proc,     166,  60,   125,  23,   -1,   16,   0,       D_EXIT,     0,             0,       "Midi Driver",                NULL,    "Manually select a driver for playing MIDI music" },
   { d_button_proc,     30,   88,   125,  23,   -1,   16,   0,       D_EXIT,     0,             0,       "Locale",                     NULL,    "Select a keyboard layout and system language" },
   { d_button_proc,     166,  88,   125,  23,   -1,   16,   0,       D_EXIT,     0,             0,       "Joystick",                   NULL,    "Calibrate your joystick" },
   { d_button_proc,     30,   116,  125,  23,   -1,   16,   0,       D_EXIT,     0,             0,       "Mouse",                      NULL,    "Configure your mouse" },
   { d_button_proc,     30,   144,  125,  23,   -1,   16,   0,       D_EXIT,     0,             0,       "Save and Exit",              NULL,    "Exit from the program, saving the current settings into the file '" SETUP_CFG_FILE "'" },
   { d_button_proc,     166,  144,  125,  23,   -1,   16,   0,       D_EXIT,     0,             0,       "Just Exit",                  NULL,    "Exit from the program, without saving the current settings" },
   { scroller_proc,     0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,                         NULL,    NULL },
   { d_yield_proc,      0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,                         NULL,    NULL },
   { NULL,              0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,                         NULL,    NULL }
};



static DIALOG test_dlg[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key)    (flags)     (d1)           (d2)     (dp)                          (p)      (help message) */
   { d_button_proc,     100,  50,   121,  25,   -1,   16,   0,       D_EXIT,     0,             0,       "MIDI",                       NULL,    midi_desc },
   { d_button_proc,     30,   87,   81,   25,   -1,   16,   0,       D_EXIT,     0,             0,       "Left",                       NULL,    digi_desc },
   { d_button_proc,     120,  87,   81,   25,   -1,   16,   0,       D_EXIT,     0,             0,       "Centre",                     NULL,    digi_desc },
   { d_button_proc,     210,  87,   81,   25,   -1,   16,   0,       D_EXIT,     0,             0,       "Right",                      NULL,    digi_desc },
   { d_button_proc,     100,  124,  121,  25,   -1,   16,   0,       D_EXIT,     0,             0,       "Exit",                       NULL,    backup_str },
   { scroller_proc,     0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,                         NULL,    NULL },
   { d_yield_proc,      0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,                         NULL,    NULL },
   { NULL,              0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,                         NULL,    NULL }
};



static DIALOG card_dlg[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key)    (flags)     (d1)           (d2)     (dp)                          (p)      (help message) */
   { d_button_proc,     30,   132,  125,  25,   -1,   16,   0,       D_EXIT,     0,             0,       "OK",                         NULL,    "Use this driver" },
   { d_button_proc,     166,  132,  125,  25,   -1,   16,   0,       D_EXIT,     0,             0,       "Cancel",                     NULL,    backup_str },
   { card_proc,         20,   36,   281,  84,   255,  16,   0,       D_EXIT,     0,             0,       card_getter,                  NULL,    NULL },
   { scroller_proc,     0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,                         NULL,    NULL },
   { d_yield_proc,      0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,                         NULL,    NULL },
   { NULL,              0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,                         NULL,    NULL }
};



static DIALOG locale_dlg[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key)    (flags)     (d1)           (d2)     (dp)                          (p)      (help message) */
   { d_button_proc,     30,   132,  125,  25,   -1,   16,   0,       D_EXIT,     0,             0,       "OK",                         NULL,    "Use this keyboard layout and language" },
   { d_button_proc,     166,  132,  125,  25,   -1,   16,   0,       D_EXIT,     0,             0,       "Cancel",                     NULL,    backup_str },
   { d_list_proc,       8,    50,   147,  68,   255,  16,   0,       D_EXIT,     0,             0,       keyboard_getter,              NULL,    "Select a keyboard layout" },
   { d_list_proc,       166,  50,   147,  68,   255,  16,   0,       D_EXIT,     0,             0,       language_getter,              NULL,    "Select language for system messages" },
   { d_ctext_proc,      81,   30,   0,    0,    16,   -1,   0,       0,          0,             0,       "Keyboard",                   NULL,    NULL },
   { d_ctext_proc,      239,  30,   0,    0,    16,   -1,   0,       0,          0,             0,       "Language",                   NULL,    NULL },
   { scroller_proc,     0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,                         NULL,    NULL },
   { d_yield_proc,      0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,                         NULL,    NULL },
   { NULL,              0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,                         NULL,    NULL }
};



static DIALOG joystick_dlg[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key)    (flags)     (d1)           (d2)     (dp)                          (p)      (help message) */
   { d_button_proc,     30,   132,  125,  25,   -1,   16,   0,       D_EXIT,     0,             0,       "OK",                         NULL,    "Use this joystick type" },
   { d_button_proc,     166,  132,  125,  25,   -1,   16,   0,       D_EXIT,     0,             0,       "Cancel",                     NULL,    backup_str },
   { d_list_proc,       60,   36,   201,  84,   255,  16,   0,       D_EXIT,     0,             0,       (void*)joystick_getter,       NULL,    "Select a type of joystick" },
   { scroller_proc,     0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,                         NULL,    NULL },
   { d_yield_proc,      0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,                         NULL,    NULL },
   { NULL,              0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,                         NULL,    NULL }
};



static DIALOG mouse_dlg[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key)    (flags)     (d1)           (d2)     (dp)                          (p)      (help message) */
   { d_button_proc,     30,   132,  125,  25,   -1,   16,   0,       D_EXIT,     0,             0,       "OK",                         NULL,    "Use this mouse type" },
   { d_button_proc,     166,  132,  125,  25,   -1,   16,   0,       D_EXIT,     0,             0,       "Cancel",                     NULL,    backup_str },
   { d_list_proc,       70,   36,   181,  84,   255,  16,   0,       D_EXIT,     0,             0,       (void*)mouse_getter,          NULL,    "Select a type of mouse" },
   { scroller_proc,     0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,                         NULL,    NULL },
   { d_yield_proc,      0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,                         NULL,    NULL },
   { NULL,              0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,                         NULL,    NULL }
};



#ifdef ALLEGRO_LINUX
static DIALOG linux_mouse_dlg[] =
{
   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key)    (flags)     (d1)           (d2)     (dp)                          (p)      (help message) */
   { d_button_proc,     30,   132,  125,  25,   -1,   16,   0,       D_EXIT,     0,             0,       "OK",                         NULL,    "Use this mouse type" },
   { d_button_proc,     166,  132,  125,  25,   -1,   16,   0,       D_EXIT,     0,             0,       "Cancel",                     NULL,    backup_str },
   { d_list_proc,       30,   36,   151,  84,   255,  16,   0,       D_EXIT,     0,             0,       mouse_getter,                 NULL,    "Select a type of mouse" },
   { d_button_proc,     192,  36,   125,  25,   -1,   16,   0,       D_EXIT,     0,             0,       "Detect",                     NULL,    "Attempt to detect the type of your mouse" },
   { scroller_proc,     0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,                         NULL,    NULL },
   { d_yield_proc,      0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,                         NULL,    NULL },
   { NULL,              0,    0,    0,    0,    0,    0,    0,       0,          0,             0,       NULL,                         NULL,    NULL }
};
#endif



/* this one is generated depending on which parameters we need */
static DIALOG param_dlg[32];

static int param_ok;



/* handle input from the parameter dialog */
static DIALOG_STATE param_handler(int c)
{
   PARAMETER *p;
   DIALOG *d = param_dlg;
   int i;
   char tmp[64];

   if (c == param_ok) {
      /* save the changes */
      while (d->proc) {
	 p = d->dp2;

	 if (p) {
	    switch (p->type) {

	       case param_int:
	       case param_num:
		  if (p->value[0])
		     i = ustrtol(p->value, NULL, 0);
		  else
		     i = -1;
		  set_config_int(uconvert_ascii("sound", tmp), p->name, i);
		  ustrzcpy(p->value, sizeof(p->value), get_config_string(uconvert_ascii("sound", tmp), p->name, empty_string));
		  break;

	       case param_hex:
		  if (p->value[0])
		     i = ustrtol(p->value, NULL, 16);
		  else
		     i = -1;
		  set_config_hex(uconvert_ascii("sound", tmp), p->name, i);
		  ustrzcpy(p->value, sizeof(p->value), get_config_string(uconvert_ascii("sound", tmp), p->name, empty_string));
		  break;

	       case param_id:
		  if (p->value[0])
		     i = ustrtol(p->value, NULL, 0);
		  else
		     i = -1;
		  set_config_id(uconvert_ascii("sound", tmp), p->name, i);
		  ustrzcpy(p->value, sizeof(p->value), get_config_string(uconvert_ascii("sound", tmp), p->name, empty_string));
		  break;

	       case param_bool:
		  set_config_int(uconvert_ascii("sound", tmp), p->name, (d->flags & D_SELECTED) ? 1 : 0);
		  ustrzcpy(p->value, sizeof(p->value), get_config_string(uconvert_ascii("sound", tmp), p->name, empty_string));
		  break;

	       case param_file:
		  set_config_string(uconvert_ascii("sound", tmp), p->name, p->value);
		  ustrzcpy(p->value, sizeof(p->value), get_config_string(uconvert_ascii("sound", tmp), p->name, empty_string));
		  break;

	       case param_list:
		  i = ustrtol(freq_getter(d->d1, NULL), NULL, 0);
		  set_config_int(uconvert_ascii("sound", tmp), p->name, i);
		  ustrzcpy(p->value, sizeof(p->value), get_config_string(uconvert_ascii("sound", tmp), p->name, empty_string));
		  break;

	       case param_str:
		  set_config_string(uconvert_ascii("sound", tmp), p->name, p->value);
		  ustrzcpy(p->value, sizeof(p->value), get_config_string(uconvert_ascii("sound", tmp), p->name, empty_string));
		  break;

	       default:
		  break;
	    }

	 }

	 d++;
      }
   }
   else {
      /* discard the changes */
      while (d->proc) {
	 p = d->dp2;

	 if (p)
	    ustrzcpy(p->value, sizeof(p->value), get_config_string(uconvert_ascii("sound", tmp), p->name, empty_string));

	 d++;
      }
   }

   return state_exit;
}



/* sets up the soundcard parameter dialog box */
static void setup_param_dialog(void)
{
   PARAMETER *p;
   DIALOG *d = param_dlg;
   char **c = soundcard->param;
   char *s;
   char tmp[64];
   int pos = 0;
   int xo = 0;
   int yo = 0;
   int i, x, y, f, g;

   #define DLG(_p, _x, _y, _w, _h, _f, _b, _k, _l, _d1, _d2, _dp, _pa, _m)   \
   {                                                                         \
      d->proc = _p;                                                          \
      d->x = _x;                                                             \
      d->y = _y;                                                             \
      d->w = _w;                                                             \
      d->h = _h;                                                             \
      d->fg = _f;                                                            \
      d->bg = _b;                                                            \
      d->key = _k;                                                           \
      d->flags = _l;                                                         \
      d->d1 = _d1;                                                           \
      d->d2 = _d2;                                                           \
      d->dp = _dp;                                                           \
      d->dp2 = _pa;                                                          \
      d->dp3 = _m;                                                           \
      d++;                                                                   \
   }

   while (*c) {
      if ((uisdigit(ugetat(*c, 0))) && (uisdigit(ugetat(*c, 1)))) {
	 xo = ugetat(*c, 0) - '0';
	 if (xo)
	    xo = 100 / xo;

	 yo = ugetat(*c, 1) - '0';
	 if (yo)
	    yo = 38 / yo;
      }
      else {
	 x = 16 + (pos%3) * 100 + xo;
	 y = 30 + (pos/3) * 38 + yo;
	 pos++;

	 p = NULL;

	 for (i=0; parameters[i].name; i++) {
	    if (ustricmp(parameters[i].name, *c) == 0) {
	       p = &parameters[i];
	       break;
	    }
	 }

	 if (p) {
	    switch (p->type) {

	       case param_int:
		  /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp)           (p)         (help) */
		  DLG(d_box_proc,      x,    y,    89,   22,   255,  16,   0,    0,       0,    0,    NULL,          NULL,       NULL);
		  DLG(d_text_proc,     x+4,  y+3,  0,    0,    255,  16,   0,    0,       0,    0,    p->label,      NULL,       NULL);
		  DLG(d_edit_proc,     x+54, y+3,  32,   16,   255,  16,   0,    0,       3,    0,    p->value,      p,          p->desc);
		  break;

	       case param_hex:
		  if (ustricmp(p->value, uconvert_ascii("FFFFFFFF", tmp)) == 0)
		     ustrzcpy(p->value, sizeof(p->value), uconvert_ascii("-1", tmp));

		  /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp)           (p)         (help) */
		  DLG(d_box_proc,      x,    y,    89,   22,   255,  16,   0,    0,       0,    0,    NULL,          NULL,       NULL);
		  DLG(d_text_proc,     x+4,  y+3,  0,    0,    255,  16,   0,    0,       0,    0,    p->label,      NULL,       NULL);
		  DLG(d_edit_proc,     x+54, y+3,  32,   16,   255,  16,   0,    0,       3,    0,    p->value,      p,          p->desc);
		  break;

	       case param_bool:
		  if (ustrtol(p->value, NULL, 0) != 0)
		     f = D_SELECTED;
		  else
		     f = 0;

		  /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp)           (p)         (help) */
		  DLG(d_box_proc,      x,    y,    89,   22,   255,  16,   0,    0,       0,    0,    NULL,          NULL,       NULL);
		  DLG(d_text_proc,     x+4,  y+3,  0,    0,    255,  16,   0,    0,       0,    0,    p->label,      NULL,       NULL);
		  DLG(d_check_proc,    x+54, y+3,  32,   16,   255,  16,   0,    f,       0,    0,    uconvert_static_string(" "),p,p->desc);
		  break;

	       case param_file:
		  if (ugetc(p->value))
		     f = D_SELECTED | D_EXIT;
		  else
		     f = D_EXIT;

		  /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp)           (p)         (help) */
		  DLG(d_box_proc,      x,    y,    89,   22,   255,  16,   0,    0,       0,    0,    NULL,          NULL,       NULL);
		  DLG(d_text_proc,     x+4,  y+3,  0,    0,    255,  16,   0,    0,       0,    0,    p->label,      NULL,       NULL);
		  DLG(filename_proc,   x+62, y+3,  32,   16,   255,  16,   0,    f,       0,    0,    empty_string,  p,          p->desc);
		  break;

	       case param_list:
		  i = ustrtol(p->value, NULL, 0);
		  freq_getter(-1, &f);
		  if (i > 0) {
		     for (g=0; g<f; g++) {
			s = freq_getter(g, NULL);
			if (i <= ustrtol(s, NULL, 0))
			   break;
		     }
		  }
		  else {
		     g = 2;
		  }

		  /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp)           (p)         (help) */
		  DLG(d_xlist_proc,    x,    y-8,  89,   68,   255,  16,   0,    0,       g,    0,    freq_getter,   p,          p->desc);
		  break;

	       case param_num:
		  /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp)           (p)         (help) */
		  DLG(d_box_proc,      x,    y,    89,   22,   255,  16,   0,    0,       0,    0,    NULL,          NULL,       NULL);
		  DLG(d_text_proc,     x+4,  y+3,  0,    0,    255,  16,   0,    0,       0,    0,    p->label,      NULL,       NULL);
		  DLG(d_edit_proc,     x+40, y+3,  46,   16,   255,  16,   0,    0,       6,    0,    p->value,      p,          p->desc);
		  break;

	       case param_str:
		  /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp)           (p)         (help) */
		  DLG(d_box_proc,      x,    y,    89,   22,   255,  16,   0,    0,       0,    0,    NULL,          NULL,       NULL);
		  DLG(d_text_proc,     x+4,  y+3,  0,    0,    255,  16,   0,    0,       0,    0,    p->label,      NULL,       NULL);
		  DLG(d_edit_proc,     x+40, y+3,  46,   16,   255,  16,   0,    0,       20,   0,    p->value,      p,          p->desc);
		  break;

	       default:
		  break;
	    }
	 }
      }

      c++;
   }

   param_ok = (int)(d - param_dlg);

   /* (dialog proc)     (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp)        (p)                        (help) */
   DLG(d_button_proc,   30,   142,  125,  25,   -1,   16,   13,   D_EXIT,  0,    0,    uconvert_static_string("OK"), NULL,    uconvert_static_string("Use these parameters"));
   DLG(d_button_proc,   166,  142,  125,  25,   -1,   16,   0,    D_EXIT,  0,    0,    uconvert_static_string("Cancel"), NULL,backup_str);
   DLG(scroller_proc,   0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL,       NULL,                      NULL);
   DLG(d_yield_proc,    0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL,       NULL,                      NULL);
   DLG(NULL,            0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL,       NULL,                      NULL);

   activate_dialog(param_dlg, param_handler, TRUE);
}



/* helper for converting a static dialog array */
static void uconvert_static_dialog(DIALOG *d)
{
   while (d->proc) {
      if ((d->proc == d_button_proc) || (d->proc == d_ctext_proc)) {
         if (d->dp)
            d->dp = uconvert_static_string((char *)d->dp);

         if (d->dp3 && (d->dp3 != backup_str) && (d->dp3 != digi_desc) && (d->dp3 != midi_desc))
            d->dp3 = uconvert_static_string((char *)d->dp3);
      }
      else if (d->proc == d_list_proc) {
         if (d->dp3)
            d->dp3 = uconvert_static_string((char *)d->dp3);
      }

      d++;
   }
}



/* handle input from the test dialog */
static DIALOG_STATE test_handler(int c)
{
   switch (c) {

      case 0:
	 /* MIDI test */
	 play_midi(setup_data[TEST_MIDI].dat, FALSE);
	 return state_redraw;

      case 1:
	 /* left pan */
	 play_sample(setup_data[TEST_SAMPLE].dat, 255, 0, 1000, FALSE);
	 return state_redraw;

      case 2:
	 /* centre pan */
	 play_sample(setup_data[TEST_SAMPLE].dat, 255, 128, 1000, FALSE);
	 return state_redraw;

      case 3:
	 /* right pan */
	 play_sample(setup_data[TEST_SAMPLE].dat, 255, 255, 1000, FALSE);
	 return state_redraw;

      default:
	 /* quit */
	 remove_sound();
	 return state_exit;
   }

   return state_active;
}



/* handle input from the card selection dialog */
static DIALOG_STATE card_handler(int c)
{
   int i;
   char tmp[64];

   switch (c) {

      case 0:
      case 2:
	 /* select driver */
	 i = (soundcard == digi_cards) ? 0 : 1;
	 soundcard += card_dlg[2].d1;
	 set_config_id(uconvert_ascii("sound", tmp), parameters[i].name, soundcard->id);
	 ustrzcpy(parameters[i].value, sizeof(parameters[i].value), get_config_string(uconvert_ascii("sound", tmp), parameters[i].name, empty_string));
	 if (soundcard->param)
	    setup_param_dialog();
	 else
	    return state_exit;
	 break;

      default:
	 /* quit */
	 return state_exit;
   }

   return state_active;
}



/* handle input from the locale selection dialog */
static DIALOG_STATE locale_handler(int c)
{
   char buf[256], tmp[256];

   switch (c) {

      case 0:
      case 2:
      case 3:
	 /* select driver */
	 if (locale_dlg[2].d1 < num_keyboard_layouts)
	    ustrzcpy(keyboard_type, sizeof(keyboard_type), keyboard_layouts[locale_dlg[2].d1]);

	 if (locale_dlg[3].d1 < num_language_layouts) {
	    ustrzcpy(language_type, sizeof(language_type), language_layouts[locale_dlg[3].d1]);

	    push_config_state();
	    uszprintf(buf, sizeof(buf), uconvert_ascii("language = %s\n", tmp), language_type);
	    set_config_data(buf, ustrsize(buf));
	    reload_config_texts(NULL);
	    pop_config_state();
	 }

	 return state_exit;

      default:
	 /* quit */
	 return state_exit;
   }

   return state_active;
}



/* handle input from the joystick selection dialog */
static DIALOG_STATE joystick_handler(int c)
{
   _DRIVER_INFO *list;
   int list_size;

   switch (c) {

      case 0:
      case 2:
	 /* select joystick */
	 get_joystick_drivers(&list, &list_size);
	 joystick_proc(list[joystick_dlg[2].d1].id);
	 return state_exit;

      default:
	 /* quit */
	 return state_exit;
   }

   return state_active;
}



/* handle input from the mouse selection dialog */
static DIALOG_STATE mouse_handler(int c)
{
   _DRIVER_INFO *list;
   int list_size;

 #ifdef ALLEGRO_LINUX
   int x;
 #endif

   switch (c) {

      case 0:
      case 2:
	 /* select mouse */
	 get_mouse_drivers(&list, &list_size);

       #ifdef ALLEGRO_LINUX
         if (system_driver->id == SYSTEM_LINUX)
	    mouse_proc(list[linux_mouse_dlg[2].d1].id);
	 else
       #endif

	 mouse_proc(list[mouse_dlg[2].d1].id);
	 return state_exit;

    #ifdef ALLEGRO_LINUX
      case 3:
	 if (system_driver->id == SYSTEM_LINUX) {
	    x = detect_mouse();
	    if (x >= 0) {
	       get_mouse_drivers(&list, &list_size);
	       mouse_proc(list[x].id);
	    }
	 }
	 return state_exit;
    #endif

      default:
	 /* quit */
	 return state_exit;
   }

   return state_active;
}



/* handle input from the main dialog */
static DIALOG_STATE main_handler(int c)
{
   char b1[256], b2[256], tmp1[256], tmp2[256];
   int i;
   AL_CONST char *s;
   char* s2;
   DATAFILE *data;
   _DRIVER_INFO *list;
   DIALOG *dlg;
   int list_size;

   switch (c) {

      case 0:
	 /* autodetect */
	 scroller_proc(MSG_RADIO, NULL, 0);

	 for (i=0; parameters[i].name; i++) {
	    set_config_string(uconvert_ascii("sound", tmp1), parameters[i].name, empty_string);
	    parameters[i].value[0] = 0;
	 }

	 reserve_voices(-1, -1);

	 if (cpu_family <= 4)
	    _sound_hq = 0;
	 else if (cpu_family <= 5)
	    _sound_hq = 1;
	 else
	    _sound_hq = 2;

	 if (init_sound(uconvert_ascii("Unable to autodetect!", tmp1)) != 0)
	    return state_redraw;

	 uszprintf(b1, sizeof(b1), uconvert_ascii("Digital: %s", tmp1), uconvert_ascii(digi_driver->ascii_name, tmp2));
	 uszprintf(b2, sizeof(b2), uconvert_ascii("MIDI: %s", tmp1), uconvert_ascii(midi_driver->ascii_name, tmp2));
	 alert(uconvert_ascii("- detected hardware -", tmp1), b1, b2, uconvert_ascii("Ok", tmp2), NULL, 0, 0);

	 for (i=0; parameters[i].name; i++) {
	    if (parameters[i].detect) {
	       switch (parameters[i].type) {

		  case param_int:
		  case param_num:
		  case param_bool:
		  case param_list:
		     set_config_int(uconvert_ascii("sound", tmp1), parameters[i].name, *parameters[i].detect);
		     break;

		  case param_id:
		     set_config_id(uconvert_ascii("sound", tmp1), parameters[i].name, *parameters[i].detect);
		     break;

		  case param_hex:
		     set_config_hex(uconvert_ascii("sound", tmp1), parameters[i].name, *parameters[i].detect);
		     break;

		  default:
		     break;
	       }
	    }
	    else
	       set_config_string(uconvert_ascii("sound", tmp1), parameters[i].name, parameters[i].def);

	    ustrzcpy(parameters[i].value, sizeof(parameters[i].value), get_config_string(uconvert_ascii("sound", tmp1), parameters[i].name, empty_string));
	 }

	 remove_sound();
	 return state_redraw;

      case 1:
	 /* test */
	 scroller_proc(MSG_RADIO, NULL, 0);
	 if (init_sound(uconvert_ascii("Sound initialization failed!", tmp1)) != 0)
	    return state_redraw;
	 uszprintf(digi_desc, sizeof(digi_desc), uconvert_ascii("Driver: %s        Description: %s", tmp1),
		   uconvert_ascii(digi_driver->ascii_name, tmp2), digi_driver->desc);
	 uszprintf(midi_desc, sizeof(midi_desc), uconvert_ascii("Driver: %s        Description: %s", tmp1),
		   uconvert_ascii(midi_driver->ascii_name, tmp2), midi_driver->desc);
	 activate_dialog(test_dlg, test_handler, FALSE);
	 break;

      case 2:
	 /* choose digital driver */
	 soundcard = digi_cards;
	 for (i=0; soundcard[i].id; i++)
	    if (soundcard[i].id == get_config_id(uconvert_ascii("sound", tmp1), uconvert_ascii("digi_card", tmp2), DIGI_AUTODETECT))
	       break;
	 card_dlg[2].d1 = i;
	 activate_dialog(card_dlg, card_handler, FALSE);
	 break;

      case 3:
	 /* choose MIDI driver */
	 soundcard = midi_cards;
	 for (i=0; soundcard[i].id; i++)
	    if (soundcard[i].id == get_config_id(uconvert_ascii("sound", tmp1), uconvert_ascii("midi_card", tmp2), MIDI_AUTODETECT))
	       break;
	 card_dlg[2].d1 = i;
	 activate_dialog(card_dlg, card_handler, FALSE);
	 break;

      case 4:
	 /* read list of keyboard mappings */
	 if (num_keyboard_layouts <= 0) {
	    if (find_allegro_resource(b1, uconvert_ascii(SETUP_KEYBOARD_FILE, tmp1), NULL, NULL, NULL, NULL, NULL, sizeof(b1)) == 0)
	       data = load_datafile(b1);
	    else
	       data = NULL;

	    if (!data) {
	       scroller_proc(MSG_RADIO, NULL, 0);
	       alert(uconvert_ascii("Error reading " SETUP_KEYBOARD_FILE, tmp1), NULL, NULL, uconvert_ascii("Ok", tmp2), NULL, 0, 0);
	    }
	    else {
               ustrzcat(b1, sizeof(b1), uconvert_ascii("#", tmp1));

	       for (i=0; data[i].type != DAT_END; i++) {
		  s = get_datafile_property(data+i, DAT_ID('N','A','M','E'));

		  if (s) {
		     s2 = ustrstr(s, uconvert_ascii("_CFG", tmp1));

		     if ((s2) && (ugetat(s2, 4) == 0)) {
			s2 = ustrdup(s);
			usetat(s2, -4, 0);
			keyboard_layouts[num_keyboard_layouts] = s2;

                        ustrzcpy(b2, sizeof(b2), b1);
                        ustrzcat(b2, sizeof(b2), s);

			push_config_state();
			set_config_file(b2);
			s = get_config_string(NULL, uconvert_ascii("keyboard_name", tmp1), s2);
			s2 = ustrdup(s);
			keyboard_names[num_keyboard_layouts] = s2;
			pop_config_state();

			num_keyboard_layouts++;
		     }
		  }
	       }
	       unload_datafile(data);
	    }
	 }

	 /* find the currently selected keyboard mapping */
	 if (num_keyboard_layouts > 0) {
	    sort_keyboards();

	    for (i=0; i<num_keyboard_layouts; i++)
	       if (ustricmp(keyboard_type, keyboard_layouts[i]) == 0)
		  break;

	    if (i==num_keyboard_layouts) {
	       uszprintf(b1, sizeof(b1), uconvert_ascii("(%s)", tmp1), keyboard_type);
	       scroller_proc(MSG_RADIO, NULL, 0);
	       alert(uconvert_ascii("Warning: current keyboard", tmp1), b1, uconvert_ascii("not found in " SETUP_KEYBOARD_FILE, tmp2),
                     uconvert_ascii("Ok", NULL), NULL, 0, 0);
	       keyboard_layouts[num_keyboard_layouts] = ustrdup(keyboard_type);
	       keyboard_names[num_keyboard_layouts] = ustrdup(keyboard_type);
	       num_keyboard_layouts++;
	    }

	    locale_dlg[2].d1 = i;
	 }

	 /* read list of languages */
	 if (num_language_layouts <= 0) {
	    if (find_allegro_resource(b1, uconvert_ascii(SETUP_LANGUAGE_FILE, tmp1), NULL, NULL, NULL, NULL, NULL, sizeof(b1)) == 0)
	       data = load_datafile(b1);
	    else
	       data = NULL;

	    if (!data) {
	       scroller_proc(MSG_RADIO, NULL, 0);
	       alert(uconvert_ascii("Error reading " SETUP_LANGUAGE_FILE, tmp1), NULL, NULL, uconvert_ascii("Ok", tmp2), NULL, 0, 0);
	    }
	    else {
               ustrzcat(b1, sizeof(b1), uconvert_ascii("#", tmp1));

	       for (i=0; data[i].type != DAT_END; i++) {
		  s = get_datafile_property(data+i, DAT_ID('N','A','M','E'));

		  if (s) {
		     s2 = ustrstr(s, uconvert_ascii("TEXT_CFG", tmp1));

		     if ((s2) && (ugetat(s2, 8) == 0)) {
			s2 = ustrdup(s);
			usetat(s2, -8, 0);
			language_layouts[num_language_layouts] = s2;

                        ustrzcpy(b2, sizeof(b2), b1);
                        ustrzcat(b2, sizeof(b2), s);

			push_config_state();
			set_config_file(b2);
			s = get_config_string(NULL, uconvert_ascii("language_name", tmp1), s2);
			s2 = ustrdup(s);
			language_names[num_language_layouts] = s2;
			pop_config_state();

			num_language_layouts++;
		     }
		  }
	       }
	       unload_datafile(data);
	    }
	 }

	 /* find the currently selected language mapping */
	 if (num_language_layouts > 0) {
	    sort_languages();

	    for (i=0; i<num_language_layouts; i++)
	       if (stricmp(language_type, language_layouts[i]) == 0)
		  break;

	    if (i==num_language_layouts) {
	       uszprintf(b1, sizeof(b1), uconvert_ascii("(%s)", tmp1), language_type);
	       scroller_proc(MSG_RADIO, NULL, 0);
	       alert(uconvert_ascii("Warning: current language", tmp1), b1, uconvert_ascii("not found in " SETUP_LANGUAGE_FILE, tmp2),
                     uconvert_ascii("Ok", NULL), NULL, 0, 0);
	       language_layouts[num_language_layouts] = ustrdup(language_type);
	       language_names[num_language_layouts] = ustrdup(language_type);
	       num_language_layouts++;
	    }

	    locale_dlg[3].d1 = i;
	 }

	 if ((num_keyboard_layouts > 0) || (num_language_layouts > 0))
	    activate_dialog(locale_dlg, locale_handler, FALSE);
	 break;

      case 5:
	 /* calibrate joystick */
	 joystick_dlg[2].d1 = 0;
	 get_joystick_drivers(&list, &list_size);

	 install_joystick(JOY_TYPE_AUTODETECT);

	 for (i=0; list[i].driver; i++) {
	    if (list[i].id == _joy_type) {
	       joystick_dlg[2].d1 = i;
	       break;
	    }
	 }

	 remove_joystick();

	 activate_dialog(joystick_dlg, joystick_handler, FALSE);
	 break;

      case 6:
	 /* configure mouse */
	 dlg = mouse_dlg;

       #ifdef ALLEGRO_LINUX
         if (system_driver->id == SYSTEM_LINUX)
	    dlg = linux_mouse_dlg;
       #endif

	 dlg[2].d1 = 0;
	 get_mouse_drivers(&list, &list_size);

	 for (i=0; list[i].driver; i++) {
	    if (list[i].id == _mouse_type) {
	       dlg[2].d1 = i;
	       break;
	    }
	 }

	 activate_dialog(dlg, mouse_handler, FALSE);
	 break;

      case 7:
	 /* save settings and quit */
	 set_config_file(uconvert_ascii(SETUP_CFG_FILE, tmp1));
	 set_config_string(uconvert_ascii("system", tmp1), uconvert_ascii("keyboard", tmp2), keyboard_type);
	 set_config_string(uconvert_ascii("system", tmp1), uconvert_ascii("language", tmp2), language_type);
	 set_config_id(uconvert_ascii("mouse", tmp1), uconvert_ascii("mouse", tmp2), _mouse_type);
	 for (i=0; parameters[i].name; i++) {
	    if (parameters[i].value[0])
	       set_config_string(uconvert_ascii("sound", tmp1), parameters[i].name, parameters[i].value);
	    else
	       set_config_string(uconvert_ascii("sound", tmp1), parameters[i].name, uconvert_ascii(" ", tmp2));
	 }
	 save_joystick_data(NULL);
	 return state_exit;

      default:
	 /* quit */
	 return state_exit;
   }

   return state_active;
}



#ifdef SETUP_EMBEDDED
int setup_main(void)
#else
int main(void)
#endif
{
   char buf[512], tmp1[256], tmp2[256];
   int i;

 #ifndef SETUP_EMBEDDED
   if (allegro_init() != 0)
      return 1;
   install_mouse();
   install_keyboard();
   install_timer();

   fade_out(4);
   if (set_gfx_mode(GFX_AUTODETECT, SETUP_SCREEN_W, SETUP_SCREEN_H, 0, 0) != 0) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message(uconvert_ascii("Unable to set graphic mode\n%s\n", tmp1), allegro_error);
      return 1;
   }
 #endif

   set_palette(black_palette);

 #ifdef SETUP_USE_COMPILED_DATAFILES
 #ifndef ALLEGRO_USE_CONSTRUCTOR
   fixup_datafile(setup_data);
 #endif
 #else
   setup_data = load_datafile(uconvert_ascii("#", tmp1));

   if (!setup_data) {
      if (find_allegro_resource(buf, uconvert_ascii(SETUP_DATA_FILE, tmp1), NULL, NULL, NULL, NULL, NULL, sizeof(buf)) == 0)
	 setup_data = load_datafile(buf);
   }

   if (!setup_data) {
    #ifdef SETUP_EMBEDDED
      set_palette(default_palette);
      alert(uconvert_ascii("Error loading " SETUP_DATA_FILE, tmp1), NULL, NULL, uconvert_ascii("OK", tmp2), NULL, 13, 0);
    #else
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("%s", uconvert_ascii("Error loading " SETUP_DATA_FILE "\n", tmp1));
    #endif
      return 1;
   }
 #endif

   set_mouse_range(0, 20, SCREEN_W-1, SCREEN_H-32);

   font = setup_data[SETUP_FONT].dat;

   clear_scroller();

   buffer = create_bitmap(SCREEN_W, SCREEN_H);

   LOCK_VARIABLE(scroller_time);
   LOCK_FUNCTION(inc_scroller_time);
   install_int_ex(inc_scroller_time, BPS_TO_TIMER(160));

   set_config_file(uconvert_ascii(SETUP_CFG_FILE, tmp1));
   uconvert_static_parameter(parameters);
   for (i=0; parameters[i].name; i++)
      ustrzcpy(parameters[i].value, sizeof(parameters[i].value), get_config_string(uconvert_ascii("sound", tmp1), parameters[i].name, parameters[i].def));

   ustrzcpy(keyboard_type, sizeof(keyboard_type), get_config_string(uconvert_ascii("system", tmp1), uconvert_ascii("keyboard", tmp2), empty_string));
   for (i=0; keyboard_type[i]; i++)
      if (!uisspace(keyboard_type[i]))
	 break;

   if (!keyboard_type[i])
      ustrzcpy(keyboard_type, sizeof(keyboard_type), uconvert_ascii("us", tmp1));

   ustrzcpy(language_type, sizeof(language_type), get_config_string(uconvert_ascii("system", tmp1), uconvert_ascii("language", tmp2), empty_string));
   for (i=0; language_type[i]; i++)
      if (!uisspace(language_type[i]))
	 break;

   if (!language_type[i])
      ustrzcpy(language_type, sizeof(language_type), uconvert_ascii("en", tmp2));

   find_sound_drivers();

   set_config_data(empty_string, 0);
   for (i=0; parameters[i].name; i++)
      set_config_string(uconvert_ascii("sound", tmp1), parameters[i].name, parameters[i].value);

   detect_sound();

   if (need_uconvert(backup_str, U_ASCII, U_CURRENT)) {
      do_uconvert(backup_str, U_ASCII, tmp1, U_CURRENT, sizeof(tmp1));
      ustrzcpy(backup_str, sizeof(backup_str), tmp1);
   }
   uconvert_static_dialog(main_dlg);
   uconvert_static_dialog(test_dlg);
   uconvert_static_dialog(card_dlg);
   uconvert_static_dialog(locale_dlg);
   uconvert_static_dialog(joystick_dlg);
   uconvert_static_dialog(mouse_dlg);
 #ifdef ALLEGRO_LINUX
   uconvert_static_dialog(linux_mouse_dlg);
 #endif

   activate_dialog(main_dlg, main_handler, FALSE);
   dialog_count++;

   do {
   } while (update());

   /* to avoid leaking memory */
   freq_getter(-1, NULL);

   destroy_bitmap(buffer);

   for (i=0; i<num_keyboard_layouts; i++) {
      free(keyboard_layouts[i]);
      free(keyboard_names[i]);
   }

   for (i=0; i<num_language_layouts; i++) {
      free(language_layouts[i]);
      free(language_names[i]);
   }

   set_mouse_range(0, 0, SCREEN_W-1, SCREEN_H-1);

 #ifndef SETUP_USE_COMPILED_DATAFILES
   unload_datafile(setup_data);
 #endif

   remove_int(inc_scroller_time);

   return 0;
}

#ifndef SETUP_EMBEDDED
END_OF_MAIN()
#endif
