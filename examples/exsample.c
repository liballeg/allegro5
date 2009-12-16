/*
 *    Example program for the Allegro library, by Shawn Hargreaves.
 *
 *    This program demonstrates how to play samples. You have to
 *    use this example from the command line to specify as first
 *    parameter a WAV or VOC sound file to play. If the file is
 *    loaded successfully, the sound will be played in an infinite
 *    loop. While it is being played, you can use the left and right
 *    arrow keys to modify the panning of the sound. You can also
 *    use the up and down arrow keys to modify the pitch.
 */


#include <allegro.h>



int main(int argc, char *argv[])
{
   SAMPLE *the_sample;
   int pan = 128;
   int pitch = 1000;

   if (allegro_init() != 0)
      return 1;

   if (argc != 2) {
      allegro_message("Usage: 'exsample filename.[wav|voc]'\n");
      return 1;
   }

   install_keyboard(); 
   install_timer();

   /* install a digital sound driver */
   if (install_sound(DIGI_AUTODETECT, MIDI_NONE, NULL) != 0) {
      allegro_message("Error initialising sound system\n%s\n", allegro_error);
      return 1;
   }

   /* read in the WAV file */
   the_sample = load_sample(argv[1]);
   if (!the_sample) {
      allegro_message("Error reading WAV file '%s'\n", argv[1]);
      return 1;
   }

   if (set_gfx_mode(GFX_AUTODETECT_WINDOWED, 320, 200, 0, 0) != 0) {
      if (set_gfx_mode(GFX_SAFE, 320, 200, 0, 0) != 0) {
	 set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
	 allegro_message("Unable to set any graphic mode\n%s\n", allegro_error);
	 return 1;
      }
   }

   if (set_display_switch_mode(SWITCH_BACKGROUND) != 0) {
      set_display_switch_mode(SWITCH_BACKAMNESIA);
   }

   set_palette(desktop_palette);
   clear_to_color(screen, makecol(255,255,255));

   textprintf_centre_ex(screen, font, SCREEN_W/2, SCREEN_H/3, makecol(0, 0, 0), -1,
			"Driver: %s", digi_driver->name);
   textprintf_centre_ex(screen, font, SCREEN_W/2, SCREEN_H/2, makecol(0, 0, 0), -1,
			"Playing %s", argv[1]);
   textprintf_centre_ex(screen, font, SCREEN_W/2, SCREEN_H*2/3, makecol(0, 0, 0), -1,
			"Use the arrow keys to adjust it");

   /* start up the sample */
   play_sample(the_sample, 255, pan, pitch, TRUE);

   do {
      poll_keyboard();

      /* alter the pan position? */
      if ((key[KEY_LEFT]) && (pan > 0))
	 pan--;
      else if ((key[KEY_RIGHT]) && (pan < 255))
	 pan++;

      /* alter the pitch? */
      if ((key[KEY_UP]) && (pitch < 16384))
	 pitch = ((pitch * 513) / 512) + 1; 
      else if ((key[KEY_DOWN]) && (pitch > 64))
	 pitch = ((pitch * 511) / 512) - 1; 

      /* adjust the sample */
      adjust_sample(the_sample, 255, pan, pitch, TRUE);

      /* delay a bit */
      rest(2);

   } while ((!key[KEY_ESC]) && (!key[KEY_SPACE]));

   /* destroy the sample */
   destroy_sample(the_sample);

   return 0;
}

END_OF_MAIN()
