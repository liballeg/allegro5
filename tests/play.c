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
 *      Sound code test program for the Allegro library.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <stdio.h>
#include <string.h>

#include "allegro.h"



void format_id(char *buf, int n, int id, AL_CONST char *name)
{
   char tmp[8];

   if (id < 256) {
      sprintf(tmp, "%d", id);
   }
   else {
      tmp[0] = (id >> 24) & 0xFF;
      tmp[1] = (id >> 16) & 0xFF;
      tmp[2] = (id >> 8) & 0xFF;
      tmp[3] = id & 0xFF;
      tmp[4] = 0;
   }

   sprintf(buf+strlen(buf), "\t%s\t- %s\n", tmp, name);
}



void usage(void)
{
   _DRIVER_INFO *digi, *midi;
   char *msg = malloc(4096);
   int i;

   strcpy(msg, "\nSound code test program for Allegro " ALLEGRO_VERSION_STR ", " ALLEGRO_PLATFORM_STR "\n");
   strcat(msg, "By Shawn Hargreaves, " ALLEGRO_DATE_STR "\n\n");
   strcat(msg, "Usage: play [digital driver [midi driver]] files.(mid|voc|wav)\n\n");
   strcat(msg, "Digital drivers:\n\n");

   if (system_driver->digi_drivers)
      digi = system_driver->digi_drivers();
   else
      digi = _digi_driver_list;

   for (i=0; digi[i].driver; i++)
      format_id(msg, i, digi[i].id, ((DIGI_DRIVER *)digi[i].driver)->ascii_name);

   strcat(msg, "\nMIDI drivers:\n\n");

   if (system_driver->midi_drivers)
      midi = system_driver->midi_drivers();
   else
      midi = _midi_driver_list;

   for (i=0; midi[i].driver; i++)
      format_id(msg, i, midi[i].id, ((MIDI_DRIVER *)midi[i].driver)->ascii_name);

   strcat(msg, "\nIf you don't specify the card, Allegro will auto-detect (ie. guess :-)\n");

   allegro_message(msg);
   free(msg);
}



int is_driver_id(char *s)
{
   int i;

   for (i=0; s[i]; i++) {
      if (((s[i] < '0') || (s[i] > '9')) &&
	  ((s[i] < 'A') || (s[i] > 'Z')) &&
	  ((s[i] < 'a') || (s[i] > 'z')))
	 return FALSE;

      if (i >= 4)
	 return FALSE;
   }

   return TRUE;
}



int get_driver_id(char *s)
{
   char tmp[4];
   char *endp;
   int val, i;

   val = strtol(s, &endp, 0);
   if (!*endp)
      return val;

   tmp[0] = tmp[1] = tmp[2] = tmp[3] = ' ';

   for (i=0; i<4; i++) {
      if (s[i])
	 tmp[i] = utoupper(s[i]);
      else
	 break;
   }

   return AL_ID(tmp[0], tmp[1], tmp[2], tmp[3]);
}



int main(int argc, char *argv[])
{
   int digicard = -1;
   int midicard = -1;
   int k = 0;
   int vol = 255;
   int pan = 128;
   int freq = 1000;
   int paused = FALSE;
   char buf[80];
   void *item[9];
   int is_midi[9];
   int item_count = 0;
   long old_midi_pos = -1;
   int doodle_note = -1;
   unsigned char midi_msg[3];
   int doodle_pitch[12] = { 60, 62, 64, 65, 67, 69, 71, 72, 74, 76, 77, 79 };
   int i;

   allegro_init();

   if (argc < 2) {
      usage();
      return 1;
   }

   if ((argc > 2) && (is_driver_id(argv[1]))) {
      digicard = get_driver_id(argv[1]);
      if ((argc > 3) && (is_driver_id(argv[2]))) {
	 midicard = get_driver_id(argv[2]);
	 i = 3;
      }
      else
	 i = 2;
   }
   else
      i = 1;

   install_timer();
   install_keyboard();
   if (set_gfx_mode(GFX_SAFE, 640, 480, 0, 0) != 0) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Unable to set any graphic mode\n%s\n", allegro_error);
      return 1;
   }
   set_palette(default_palette);
   clear_to_color(screen, makecol(255, 255, 255));
   text_mode(makecol(255, 255, 255));

   if (install_sound(digicard, midicard, NULL) != 0) {
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
      allegro_message("Error initialising sound system\n%s\n", allegro_error);
      return 1;
   }

   textprintf_centre(screen, font, SCREEN_W/2, 12, palette_color[4], "Sound code test program for Allegro " ALLEGRO_VERSION_STR ", " ALLEGRO_PLATFORM_STR);
   textprintf_centre(screen, font, SCREEN_W/2, 24, palette_color[4], "By Shawn Hargreaves, " ALLEGRO_DATE_STR);

   textprintf(screen, font, 0, 84,  palette_color[2], "Digital sound driver: %s", digi_driver->name);
   textprintf(screen, font, 0, 96,  palette_color[2], "Description: %s", digi_driver->desc);
   textprintf(screen, font, 0, 108, palette_color[2], "Voices: %d", digi_driver->voices);

   textprintf(screen, font, 0, 132, palette_color[2], "Midi music driver: %s", midi_driver->name);
   textprintf(screen, font, 0, 144, palette_color[2], "Description: %s", midi_driver->desc);
   textprintf(screen, font, 0, 156, palette_color[2], "Voices: %d", midi_driver->voices);

   while ((i < argc) && (item_count < 9)) {
      if ((stricmp(get_extension(argv[i]), "voc") == 0) || (stricmp(get_extension(argv[i]), "wav") == 0)) {
	 item[item_count] = load_sample(argv[i]);
	 is_midi[item_count] = 0;
      }
      else if (stricmp(get_extension(argv[i]), "mid") == 0) {
	 item[item_count] = load_midi(argv[i]);
	 is_midi[item_count] = 1;
      }
      else {
	 set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	 allegro_message("Unknown file type '%s'\n", argv[i]);
	 goto get_out;
      }

      if (!item[item_count]) {
	 set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	 allegro_message("Error reading %s\n", argv[i]);
	 goto get_out;
      }

      textprintf(screen, font, 32, 192+item_count*12, palette_color[255], "%d: %s", item_count+1, argv[i]);

      item_count++;
      i++;
   }

   textprintf_centre(screen, font, SCREEN_W/2, SCREEN_H-48, palette_color[4], "Press a number 1-9 to trigger a sample or midi file");
   textprintf_centre(screen, font, SCREEN_W/2, SCREEN_H-36, palette_color[4], "v/V changes sfx volume, p/P changes sfx pan, and f/F changes sfx frequency");
   textprintf_centre(screen, font, SCREEN_W/2, SCREEN_H-24, palette_color[4], "space pauses/resumes MIDI playback, and the arrow keys seek through the tune");
   textprintf_centre(screen, font, SCREEN_W/2, SCREEN_H-12, palette_color[4], "Use the function keys to doodle a tune");

   k = '1';      /* start sound automatically */

   do {
      switch (k & 0xFF) {

	 case 'v':
	    vol -= 8;
	    if (vol < 0)
	       vol = 0;
	    set_volume(-1, vol);
	    break;

	 case 'V':
	    vol += 8;
	    if (vol > 255)
	       vol = 255;
	    set_volume(-1, vol);
	    break;

	 case 'p':
	    pan -= 8;
	    if (pan < 0)
	       pan = 0;
	    break;

	 case 'P':
	    pan += 8;
	    if (pan > 255)
	       pan = 255;
	    break;

	 case 'f':
	    freq -= 8;
	    if (freq < 1)
	       freq = 1;
	    break;

	 case 'F':
	    freq += 8;
	    break;

	 case '0':
	    play_midi(NULL, FALSE);
	    paused = FALSE;
	    break;

	 case ' ':
	    if (paused) {
	       midi_resume();
	       paused = FALSE;
	    }
	    else {
	       midi_pause();
	       paused = TRUE;
	    }
	    break;

	 default:
	    if ((k >> 8) == KEY_LEFT) {
	       for (i=midi_pos-1; (midi_pos == old_midi_pos) && (midi_pos > 0); i--)
		  midi_seek(i);
	       paused = FALSE;
	    }
	    else if ((k >> 8) == KEY_RIGHT) {
	       for (i=midi_pos+1; (midi_pos == old_midi_pos) && (midi_pos > 0); i++)
		  midi_seek(i);
	       paused = FALSE;
	    }
	    if ((k >> 8) == KEY_DOWN) {
	       for (i=midi_pos-16; (midi_pos == old_midi_pos) && (midi_pos > 0); i--)
		  midi_seek(i);
	       paused = FALSE;
	    }
	    else if ((k >> 8) == KEY_UP) {
	       for (i=midi_pos+16; (midi_pos == old_midi_pos) && (midi_pos > 0); i++)
		  midi_seek(i);
	       paused = FALSE;
	    }
	    else if (((k & 0xFF) >= '1') && ((k & 0xFF) < '1'+item_count)) {
	       k = (k & 0xFF) - '1';
	       if (is_midi[k]) {
		  play_midi((MIDI *)item[k], FALSE);
		  paused = FALSE;
	       }
	       else
		  play_sample((SAMPLE *)item[k], vol, pan, freq, FALSE);
	    }
	    else {
	       k >>= 8;
	       if (((k >= KEY_F1) && (k <= KEY_F10)) ||
		   ((k >= KEY_F11) && (k <= KEY_F12))) {
		  if (doodle_note >= 0) {
		     midi_msg[0] = 0x80;
		     midi_msg[1] = doodle_pitch[doodle_note];
		     midi_msg[2] = 0;
		     midi_out(midi_msg, 3);
		  }

		  if ((k >= KEY_F1) && (k <= KEY_F10))
		     doodle_note = k - KEY_F1;
		  else
		     doodle_note = 10 + k - KEY_F11;

		  midi_msg[0] = 0x90;
		  midi_msg[1] = doodle_pitch[doodle_note];
		  midi_msg[2] = 127;
		  midi_out(midi_msg, 3);
	       }
	    }
	    break;
      }

      old_midi_pos = midi_pos;

      sprintf(buf, "        midi pos: %ld    vol: %d    pan: %d    freq: %d        ", midi_pos, vol, pan, freq);
      textout_centre(screen, font, buf, SCREEN_W/2, SCREEN_H-120, palette_color[255]);

      do {
      } while ((!keypressed()) && (midi_pos == old_midi_pos));

      if (keypressed())
	 k = readkey();
      else
	 k = 0;

   } while ((k & 0xFF) != 27);

   get_out:

   for (i=0; i<item_count; i++)
      if (is_midi[i])
	 destroy_midi((MIDI *)item[i]);
      else
	 destroy_sample((SAMPLE *)item[i]);

   return 0;
}

END_OF_MAIN();
