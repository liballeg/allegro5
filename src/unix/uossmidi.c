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
 *      Open Sound System sequencer support.
 *
 *      By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"

#ifdef MIDI_OSS

#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if defined(HAVE_SOUNDCARD_H)
   #include <soundcard.h>
#elif defined(HAVE_SYS_SOUNDCARD_H)
   #include <sys/soundcard.h>
#elif defined(HAVE_MACHINE_SOUNDCARD_H)
   #include <machine/soundcard.h>
#elif defined(HAVE_LINUX_SOUNDCARD_H)
   #include <linux/soundcard.h>
#endif
#include <sys/ioctl.h>



/* our patch data */
#include "../misc/fm_instr.h"
#include "../misc/fm_emu.h"



static int oss_midi_detect(int input);
static int oss_midi_init(int input, int voices);
static void oss_midi_exit(int input);
static int oss_midi_mixer_volume(int volume);
static void oss_midi_key_on(int inst, int note, int bend, int vol, int pan);
static void oss_midi_key_off(int voice);
static void oss_midi_set_volume(int voice, int vol);
static void oss_midi_set_pitch(int voice, int note, int bend);



static int seq_fd = -1;
static int seq_device;
static int seq_synth_type; 
static int seq_patch[18];
static int seq_drum_start;
static char seq_desc[256] = EMPTY_STRING;

static char seq_driver[256] = EMPTY_STRING;
static char mixer_driver[256] = EMPTY_STRING;

SEQ_DEFINEBUF(2048);



MIDI_DRIVER midi_oss =
{
   MIDI_OSS,
   empty_string,
   empty_string,
   "Open Sound System", 
   0, 0, 0xFFFF, 0, -1, -1,
   oss_midi_detect,
   oss_midi_init,
   oss_midi_exit,
   oss_midi_mixer_volume, 
   NULL,
   _dummy_load_patches, 
   _dummy_adjust_patches, 
   oss_midi_key_on,
   oss_midi_key_off,
   oss_midi_set_volume,
   oss_midi_set_pitch,
   _dummy_noop2,                /* TODO */
   _dummy_noop2                 /* TODO */
};



/* as required by the OSS API */
void seqbuf_dump()
{
   if (_seqbufptr) {
      write(seq_fd, _seqbuf, _seqbufptr);
      _seqbufptr = 0;
   }
}



/* attempt to open sequencer device */
static int seq_attempt_open()
{
   char tmp1[128], tmp2[128], tmp3[128];
   int fd;

   ustrzcpy(seq_driver, sizeof(seq_driver), get_config_string(uconvert_ascii("sound", tmp1),
					                      uconvert_ascii("oss_midi_driver", tmp2),
					                      uconvert_ascii("/dev/sequencer", tmp3)));

   fd = open(uconvert_toascii(seq_driver, tmp1), O_WRONLY);
   if (fd < 0) 
      uszprintf(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("%s: %s"), seq_driver, ustrerror(errno));

   return fd;
}



/* find the best (supported) synth type for device */
static int seq_find_synth(int fd)
{
   struct synth_info info;
   int num_synths, i, ret = 0;
   char *s;
   char tmp1[64], tmp2[256];

   if (ioctl(fd, SNDCTL_SEQ_NRSYNTHS, &num_synths) == -1)
      return 0;

   for (i = 0; i < num_synths; i++) {
      info.device = i;
      if (ioctl(fd, SNDCTL_SYNTH_INFO, &info) == -1)
	 return 0;

      /* only FM synthesis supported, for now */
      if (info.synth_type == SYNTH_TYPE_FM) {
	 seq_device = i;
	 seq_synth_type = SYNTH_TYPE_FM;
	 midi_oss.voices = info.nr_voices;
	 ret = 1;
	 break;
      }
   }

   switch (info.synth_subtype) {
      case FM_TYPE_ADLIB:
	 s = uconvert_ascii("Adlib", tmp1);
	 break;
      case FM_TYPE_OPL3:
	 s = uconvert_ascii("OPL3", tmp1);
	 break;
      default:
	 s = uconvert_ascii("Error!", tmp1);
	 break;
   }

   uszprintf(seq_desc, sizeof(seq_desc), uconvert_ascii("Open Sound System (%s)", tmp2), s);
   midi_driver->desc = seq_desc;

   return ret;
}



/* FM synth: load our instrument patches */
static void seq_set_fm_patches(int fd)
{
   struct sbi_instrument ins;
   int i;

   ins.device = seq_device;
   ins.key = FM_PATCH;
   memset(ins.operators, 0, sizeof(ins.operators));

   /* instruments */
   for (i=0; i<128; i++) {
      ins.channel = i;
      memcpy(&ins.operators, &fm_instrument[i], sizeof(FM_INSTRUMENT));
      write(fd, &ins, sizeof(ins));
   }

   /* (emulated) drums */
   for (i=0; i<47; i++) {
      ins.channel = 128+i;
      memset(ins.operators, 0, sizeof(ins.operators));
      memcpy(&ins.operators, &fm_emulated_drum[i], sizeof(FM_INSTRUMENT));
      write(fd, &ins, sizeof(ins));
   }
}



/* oss_midi_detect:
 *  Sequencer detection routine.
 */
static int oss_midi_detect(int input)
{
   if (input) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Input is not supported"));
      return FALSE;
   }

   seq_fd = seq_attempt_open();
   if (seq_fd < 0)
      return FALSE;

   close(seq_fd);
   return TRUE;
}



/* oss_midi_init:
 *  Init the driver.
 */
static int oss_midi_init(int input, int voices)
{
   char tmp1[128], tmp2[128], tmp3[128];
   int i;

   if (input) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Input is not supported"));
      return -1;
   }

   seq_fd = seq_attempt_open();
   if (seq_fd < 0) 
      return -1;

   if (!seq_find_synth(seq_fd)) {
      close(seq_fd);
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("No support synth type found"));
      return -1;
   }

   ioctl(seq_fd, SNDCTL_SEQ_RESET);

   if (seq_synth_type == SYNTH_TYPE_FM) {
      seq_set_fm_patches(seq_fd);
      seq_drum_start = midi_oss.voices - 5;
   }

   for (i = 0; i < (sizeof(seq_patch) / sizeof(int)); i++) 
      seq_patch[i] = -1;

   /* for the mixer routine */
   ustrzcpy(mixer_driver, sizeof(mixer_driver), get_config_string(uconvert_ascii("sound", tmp1),
					                          uconvert_ascii("oss_mixer_driver", tmp2),
					                          uconvert_ascii("/dev/mixer", tmp3)));

   return 0;
}



/* oss_midi_mixer_volume:
 *  Sets the mixer volume for output.
 */
static int oss_midi_mixer_volume(int volume)
{
   int fd, vol, ret;
   char tmp[128];

   fd = open(uconvert_toascii(mixer_driver, tmp), O_WRONLY);
   if (fd < 0)
      return -1;

   vol = (volume * 100) / 255;
   vol = (vol << 8) | (vol);
   ret = ioctl(fd, MIXER_WRITE(SOUND_MIXER_SYNTH), &vol);
   close(fd);

   return ((ret == -1) ? -1 : 0);
}



/* oss_midi_key_on:
 *  Triggers the specified voice. 
 */
static void oss_midi_key_on(int inst, int note, int bend, int vol, int pan)
{
   int voice;

   /* percussion? */
   if (inst > 127) {
      voice = _midi_allocate_voice(seq_drum_start, midi_driver->voices-1);
      inst -= 35;
   }
   else
      voice = _midi_allocate_voice(0, seq_drum_start-1);

   if (voice < 0)
      return;

   /* make sure the voice is set up with the right sound */
   if (inst != seq_patch[voice]) {
      SEQ_SET_PATCH(seq_device, voice, inst);
      seq_patch[voice] = inst;
   }

   SEQ_CONTROL(seq_device, voice, CTL_PAN, pan);
   SEQ_BENDER(seq_device, voice, 8192 + bend);
   SEQ_START_NOTE(seq_device, voice, note, vol);
   SEQ_DUMPBUF();
}



/* oss_midi_key_off:
 *  Insert witty remark about this line being unhelpful.
 */
static void oss_midi_key_off(int voice)
{
   SEQ_STOP_NOTE(seq_device, voice, 0, 64);
   SEQ_DUMPBUF();
}



/* oss_midi_set_volume:
 *  Sets the volume of the specified voice.
 */
static void oss_midi_set_volume(int voice, int vol)
{
   SEQ_CONTROL(seq_device, voice, CTL_MAIN_VOLUME, vol);
}



/* oss_midi_set_pitch:
 *  Sets the pitch of the specified voice.
 */
static void oss_midi_set_pitch(int voice, int note, int bend)
{
   SEQ_CONTROL(seq_device, voice, CTRL_PITCH_BENDER, 8192 + bend);
}



/* oss_midi_exit:
 *  Cleanup when we are finished.
 */
static void oss_midi_exit(int input)
{
   if (input)
      return;

   if (seq_fd > 0) {
      close(seq_fd);
      seq_fd = -1;
   }
}

#endif
